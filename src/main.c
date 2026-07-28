#include "cnof.h"
#include "helpers.h"
#include "activeNotes.h"
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
    for (int blockptr = 0; blockptr < (int)song.num_blocks; blockptr++)
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
	
	int lastTime = master_queue[total_notearrys-1].start_sample + master_queue[total_notearrys-1].time;
	int total_samples = lastTime * SAMPLE_RATE;
	
	printf("Attempting to convert to SongTree\n");
	SongTree *songtree = createSongTree(master_queue, data.size);
	if (songtree == NULL) {
		printf("Failed to Create Song Tree\n");
	} else {
		printf("Conversion complete\n");
	}
	
	// linsndfile stuff
	memset(&sfinfo, 0, sizeof(sfinfo));
	sfinfo.samplerate = data.sample_rate;
	sfinfo.channels = 2;
	sfinfo.format		= (SF_FORMAT_WAV | SF_FORMAT_PCM_24) ;
	
	printf("Creating sndfile write buffer\n");
	float *buffer = malloc(sizeof(float) * 2 * total_samples);
    if (!(file = sf_open ("sine.wav", SFM_WRITE, &sfinfo)))
    {
        printf ("Error : Not able to open output file.\n");
		free(buffer);
        return 1;
    }
	printf("Creation Complete\n");

    int i;
    float sample;
	bool playedCurrent=false;
	SongTree *current_branch = songtree;
	printf("Starting main sample loop\n");
    for (i = 0; i < total_samples; i++) 
    {
        if (current_branch->next != NULL && current_branch->next->na->start_sample <= (long unsigned int)i)
        {
			playedCurrent = false;
			current_branch = current_branch->next;
        }
        if (!playedCurrent) {
			printf("Stating playing block at %ld\n", current_branch->na->start_sample);
        	for (int index = 0; index < current_branch->size;  index++) 
        	{
        	    NoteArray* na = &current_branch->na[index];
        	    for (int j = 0; j < (int)na->num_notes; j++) 
        	    {
        	        Note* note = &na->notes[j];
        	        note->envlope.state = ATTACK;
        	        note->started = 1;
        	        if (!note->func)
        	        {
        	            note->func = wave_functions[SINE];
        	            continue; 
        	        }
        	    }
        	}
			playedCurrent = true;
		}
		Node *head = get_head();	
		Node *currentNode = head->next;
        sample = 0.0f;
		while (currentNode != NULL)  {
			Note *note =currentNode->note;
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

			currentNode = currentNode->next;
		}
        sample *= data.amplitude;
		buffer[2* i] = sample;
		buffer[2* i + 1] = sample;
    }
	printf("Sample loop Complete\n");
	if (sf_write_float (file, buffer, sfinfo.channels * total_samples) !=
											sfinfo.channels * total_samples)
		puts (sf_strerror (file)) ;

	sf_close(file);
	free(buffer);
	cleanSongTree(songtree);
	return 0;
}
