#include "cnof.h"
#include "helpers.h"
#include <sndfile.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_RATE 44100.0f


static unsigned int seed = 1;
float white_noise ()
{
    seed = seed * 1664525 + 1013904223;
    return ((seed >> 16) / 32768.0f) - 1.0f;
}


int main ()
{
    SNDFILE* file;
    SF_INFO sfinfo;

    Synth data;
    data.amplitude = 0.1f;
    data.sample_rate = 44100.0f;
    data.current_sample = 0;

    Song song;
    parseSong (&song, "test.CNOF");
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
    print_song (song);
	
	memset(&sfinfo, 0, sizeof(sfinfo));
	sfinfo.samplerate = data.sample_rate;
	sfinfo.channels = 2;
	sfinfo.format		= (SF_FORMAT_WAV | SF_FORMAT_PCM_24) ;

	
	int total_samples = 30 * SAMPLE_RATE;

	float *buffer = malloc(sizeof(float) * 2 * total_samples);
    if (!(file = sf_open ("sine.wav", SFM_WRITE, &sfinfo)))
    {
        printf ("Error : Not able to open output file.\n");
		free(buffer);
        return 1;
    }

    unsigned int i;
    float sample;
    for (i = 0; i < total_samples; i++) // for each sample
    {
        sample = 0.0f;
		double now = data.current_sample / SAMPLE_RATE;
        for (int index = 0; index < (int)data.size; index++) // loops through each array of notes
        {
            NoteArray* na = &data.master_queue[index];
            for (int j = 0; j < (int)na->num_notes; j++) // for each note in the array
            {
                Note* note = &na->notes[j];
                if (na->time + data.start_time <= now && !note->started)
                {
                    note->envlope.state = ATTACK;
                    note->started = 1;
                }
                if (note->envlope.state != OFF)
                {
                    if (!note->func)
                    {
                        // fprintf (stderr, "ERROR: NULL note->func at sample %llu (note index
                        // %d)\n",
                        //         data->current_sample, j);
                        note->func = wave_functions[SINE];
                        continue; // or return paAbort;
                    }
                    sample += note->func (note->phase) * note->amplitude
                              * env_process (&note->envlope);
                    if (note->envlope.state == OFF)
                        note->active = 0.0f;

                    note->phase += note->frequency / data.sample_rate;
                    if (note->phase >= 1)
                        note->phase -= 1.0f;
                    if (note->active > 0.0f)
                        note->active -= 1.0f / SAMPLE_RATE;
                    else if (note->envlope.state == SUSTAIN)
                        note->envlope.state = RELEASE;
                }
            }
        }
		// multiplies by global amplitude then writes left and right to buffer
        sample *= data.amplitude;
		data.current_sample++;
		buffer[2* i] = sample;
		buffer[2* i + 1] = sample;
    }

	if (sf_write_float (file, buffer, sfinfo.channels * total_samples) !=
											sfinfo.channels * total_samples)
		puts (sf_strerror (file)) ;

	sf_close(file);
	free(buffer);
	return 0;
}
