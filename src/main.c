#include "cnof.h"
#include "helpers.h"
#include "portaudio.h"
#include <stdbool.h>
#include <stdio.h>

static unsigned int seed = 1;
float white_noise ()
{
    seed = seed * 1664525 + 1013904223;
    return ((seed >> 16) / 32768.0f) - 1.0f;
}

static int patestCallback (const void* inputBuffer, void* outputBuffer,
                           unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags, void* userData)
{
    Synth* data = (Synth*)userData;
    float* out = (float*)outputBuffer;
    unsigned int i;
    (void)inputBuffer;
    float sample;
		const double epsilon = 1e-5; // ~0.04 samples at 44.1kHz

    for (i = 0; i < framesPerBuffer; i++)
    {
        sample = 0.0f;
				double now = timeInfo->outputBufferDacTime + i * 1.0/data->sample_rate;
        for (int index = 0; index < (int)data->size; index++)
        {
            NoteArray* na = &data->master_queue[index];
            for (int j = 0; j < (int)na->num_notes; j++)
            {
                Note* note = &na->notes[j];
							                if (na->time+ data->start_time <= epsilon+now && !note->started)
                {

                    note->envlope.state = ATTACK;
                    note->started = 1;
                }
                if (note->envlope.state != OFF)
                {
                    if (!note->func)
                    {
                        //fprintf (stderr, "ERROR: NULL note->func at sample %llu (note index %d)\n",
                         //        data->current_sample, j);
												note->func = wave_functions[SINE];
                        continue; // or return paAbort;
                    }

                    sample += note->func (note->phase) * note->amplitude
                              * env_process (&note->envlope);
                    if (note->envlope.state == OFF)
                        note->active = 0.0f;

                    note->phase += note->frequency / data->sample_rate;
                    if (note->phase >= 1)
                        note->phase -= 1.0f;
                    if (note->active > 0.0f)
                        note->active -= 1.0f / data->sample_rate;
                    else if (note->envlope.state == SUSTAIN)
                        note->envlope.state = RELEASE;
                }
            }
        }
        sample *= data->amplitude;
        *out++ = sample; /* left */
        *out++ = sample; /* right */
    }
    return paContinue;
}

int main ()
{

    PaError err;

    PaStream* stream;

    Synth data;
    data.amplitude = 0.1f;
    data.sample_rate = 44100.0f;
    data.current_sample = 0;

    Song song;
    parseSong (&song, "../test.CNOF");
    printf ("Compliling complete\n");

    int total_notearrys = 0;
    for (int blockptr = 0; blockptr < song.num_blocks; blockptr++)
    {
        total_notearrys += song.blocks[blockptr]->music_size.num_lines;
    }

    NoteArray master_queue[total_notearrys];

    int mqptr = 0;

    data.master_queue = &master_queue[0];
    data.size = total_notearrys;

    for (int blockptr = 0; blockptr < (int)song.num_blocks; blockptr++)
    {
        for (int i = 0; i < (int)song.blocks[blockptr]->music_size.num_lines; i++)
        {
            master_queue[mqptr] = song.blocks[blockptr]->note_lines[i];
            master_queue[mqptr].start_sample
                = (uint64_t)(master_queue[mqptr].time * data.sample_rate);
            mqptr++;
        }
    }
    qsort (master_queue, (size_t)total_notearrys, sizeof (NoteArray), compare_by_time);

    err = Pa_Initialize ();
    if (err != paNoError)
        goto error;
    /* Open an audio I/O stream. */
    err = Pa_OpenDefaultStream (&stream, 0,      /* no input channels */
                                2,               /* stereo output */
                                paFloat32,       /* 32 bit floating point output */
                                SAMPLE_RATE, 256, /* frames per buffer, i.e. the number
                                                         of sample frames that PortAudio will
                                                         request from the callback. Many apps
                                                         may want to use
                                                         paFramesPerBufferUnspecified, which
                                                         tells PortAudio to pick the best,
                                                         possibly changing, buffer size.*/
                                patestCallback,  /* this is your callback function */
                                &data);          /*This is a pointer that will be passed to
                                                           your callback*/
    if (err != paNoError)
        goto error;

    err = Pa_StartStream (stream);
    if (err != paNoError)
        goto error;
		
		data.start_time = Pa_GetStreamTime(stream)+0.05;

    //print_song (song);

    Pa_Sleep (30000);

    err = Pa_StopStream (stream);
    if (err != paNoError)
        goto error;

error:
    Pa_CloseStream (stream);
    clean_song (&song);
    err = Pa_Terminate ();
    if (err != paNoError)
        printf ("PortAudio error: %s\n", Pa_GetErrorText (err));
    return 1;
}
