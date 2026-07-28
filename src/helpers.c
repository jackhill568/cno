#include "helpers.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

float saw_wave (Note *note) { return note->phase * 2 - 1.0f; }
float sine_wave (Note *note) { return sin (2 * 3.14 * note->phase); }
float square_wave (Note *note) { return 1.0f - 2.0f * (note->phase >= 0.5f); }
float triangle_wave (Note *note) { return 1.0f - 4.0f * fabsf (0.5f - note->phase); }

float karplus_strong(Note *note) {
	int next = (note->delayIndex + 1) % note->delayLength;
	float output = note->delay[note->delayIndex];
	float average = 0.5 * (output + note->delay[next]);
	note->delay[note->delayIndex] = average * 0.996;
	note->delayIndex = next;
	return output;
}

extern float (*wave_functions[NUM_WAVES]) (Note *)
    = { sine_wave, saw_wave, square_wave, triangle_wave, karplus_strong };

int note_name_to_midi (const char* note)
{
    int noteIndex;
    switch (note[0])
    {
    case 'C':
        noteIndex = 0;
        break;
    case 'D':
        noteIndex = 2;
        break;
    case 'E':
        noteIndex = 4;
        break;
    case 'F':
        noteIndex = 5;
        break;
    case 'G':
        noteIndex = 7;
        break;
    case 'A':
        noteIndex = 9;
        break;
    case 'B':
        noteIndex = 11;
        break;
    default:
        return -1;
    }
    int i = 1;
    if (note[i] == 's')
    {
        noteIndex++;
        i++;
    }
    else if (note[i] == 'b')
    {
        noteIndex--;
        i++;
    }
    int octave = note[i] - '0';
    return (octave + 1) * 12 + noteIndex;
}

float white_noise ()
{
	static unsigned int seed = 1;
    seed = seed * 1664525 + 1013904223;
    return ((seed >> 16) / 32768.0f) - 1.0f;
}

inline float note_to_freq (int note) { return 440.0f * powf (2.0f, (note - 69) / 12.0f); }

int compare_by_time(const void *a, const void *b) {
    const NoteArray *e1 = (const NoteArray *)a;
    const NoteArray *e2 = (const NoteArray *)b;

    if (e1->time < e2->time) return -1;
    if (e1->time > e2->time) return 1;
    return 0;
}

bool addNodeToSongBranch(SongTree *array_current, NoteArray *na) {
	NoteArray *temp = realloc(array_current->na, ++array_current->size * sizeof(NoteArray));
	if (!temp) {
		printf("ERROR resizing NoteArray for SongTree\n");
		array_current->size--;
		return 1;
	}
	array_current->na= temp;
	array_current->na[array_current->size -1] = *na;
	return 0;
}

void cleanSongTree(SongTree *head) {
	SongTree *current = head;	
	while (current!=NULL) {
		if (current->na != NULL) {
			free(current->na);
		}
		SongTree *temp = current->next;	
		free(current);
		current = temp;
	}
}

SongTree *createSongTree(NoteArray *masterqueue, int nasize) {
	SongTree *array_current = calloc(1, sizeof(SongTree));
	if (!array_current) {
		printf("ERROR allocating initial memory for SongTree\n");
		return NULL;
	}
	SongTree* head = array_current;
	for (int i = 0; i < nasize; i++) {
		NoteArray *na = &masterqueue[i];
		if (array_current->na == NULL || na->start_sample == array_current->na->start_sample) {
			if (1== addNodeToSongBranch(array_current, na)){
				cleanSongTree(head);
				return NULL;
			}
		} else {
			SongTree *new = calloc(1, sizeof(SongTree));
			if (!new)  {
				printf("ERROR allocating for new SongTree branch\n");
				cleanSongTree(head);
				return NULL;
			}
			array_current->next = new;
			array_current = new;
			if (addNodeToSongBranch(array_current, na)==1){
				cleanSongTree(head);
				return NULL;
			}
		}
	}
	return head;
}

