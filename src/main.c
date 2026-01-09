#include "cnof.h"
#include "helpers.h"
#include "portaudio.h"
#include <math.h>
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

    for (i = 0; i < framesPerBuffer; i++)
    {
        sample = 0.0f;
        for (int index = 0; index < MAX_NOTES; index++)
        {
            Note* note = &data->notes[index];
            if (note->envlope.state != OFF)
            {
                sample += note->func (note->phase) * note->amplitude * env_process (&note->envlope);
                if (note->envlope.state == OFF)
                    note->active = 0.0f;
                if (note->active <= 0.0f)
                    note->envlope.state = RELEASE;

                note->phase += note->frequency / data->sample_rate;
                if (note->phase >= 1)
                    note->phase -= 1.0f;
                note->active -= 1.0f / data->sample_rate;
            }
        }
        sample *= data->amplitude;
        *out++ = sample; /* left */
        *out++ = sample; /* right */
    }
    return paContinue;
}

NoteArray* get_next_toplay (NoteArray* mq, int size, float *note_time)
{
    NoteArray* lowest_time = NULL;
    for (int i = 0; i < size; i++)
    {
        if (mq[i].time >= 0.0f)
        {
            if (lowest_time == NULL || mq[i].time < lowest_time->time)
            {
                lowest_time = &mq[i];
            }
        }
    }
    if (lowest_time)
    {
        *note_time = lowest_time->time;  // store time before marking
        lowest_time->time = -1.0;
    }
    return lowest_time;
}

int main ()
{

    PaError err;

    PaStream* stream;

    Synth data = { 0.0f };
    data.amplitude = 0.1f;
    data.sample_rate = 44100.0f;

    for (int i = 0; i < MAX_NOTES; i++)
    {
        data.notes[i] = (Note){ 0.0f };
        data.notes[i].func = wave_functions[SINE];
    }

    Song song;
    parseSong (&song, "../test.CNOF");
    printf ("Compliling complete\n");

    int total_notearrys = 0;
    for (int blockptr = 0; blockptr < song.num_blocks; blockptr++)
    {
        total_notearrys += song.blocks[blockptr]->num_lines;
    }

    NoteArray master_queue[total_notearrys];
    int mqptr = 0;

    err = Pa_Initialize ();
    if (err != paNoError)
        goto error;
    /* Open an audio I/O stream. */
    err = Pa_OpenDefaultStream (&stream, 0,       /* no input channels */
                                2,                /* stereo output */
                                paFloat32,        /* 32 bit floating point output */
                                SAMPLE_RATE, 256, /* frames per buffer, i.e. the number
                                                         of sample frames that PortAudio will
                                                         request from the callback. Many apps
                                                         may want to use
                                                         paFramesPerBufferUnspecified, which
                                                         tells PortAudio to pick the best,
                                                         possibly changing, buffer size.*/
                                patestCallback,   /* this is your callback function */
                                &data);           /*This is a pointer that will be passed to
                                                            your callback*/
    if (err != paNoError)
        goto error;

    err = Pa_StartStream (stream);
    if (err != paNoError)
        goto error;

    for (int blockptr = 0; blockptr < song.num_blocks; blockptr++)
    {
        for (int i = 0; i < song.blocks[blockptr]->num_lines; i++)
        {
            master_queue[mqptr++] = song.blocks[blockptr]->note_lines[i];
        }
    }

    print_song (song);

    float time = 0.0f;
		float note_time = 0.0f;
    NoteArray* toplay;
    do
    {
        toplay = get_next_toplay (master_queue, total_notearrys, &note_time);
        if (toplay == NULL)
            break;
        for (int i = 0; i < toplay->num_notes; i++)
        {
            noteOn (&data, &toplay->notes[i]);
        }
        if ( note_time!= time)
        {
            Pa_Sleep (fabsf (toplay->time - time) * 100);
        }

				time = note_time;
    } while (toplay);

    // for (NoteArray* ptr = song.blocks[1]->note_lines; ptr->notes != NULL; ptr++)
    // {
    //     for (Note* note = (*ptr).notes; note->active != 0.0f; note++)
    //     {
    //         noteOn (&data, note);
    //     }
    //     Pa_Sleep (fabsf ((*ptr).time - time) * 2000);
    //     time = (*ptr).time;
    // }

    err = Pa_StopStream (stream);
    if (err != paNoError)
        goto error;

    Song* songptr = &song;

error:
    Pa_CloseStream (stream);
    clean_song (&song);
    err = Pa_Terminate ();
    if (err != paNoError)
        printf ("PortAudio error: %s\n", Pa_GetErrorText (err));
    return 1;
}
