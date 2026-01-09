#include "helpers.h"
#include <stdlib.h>

#define SAMPLE_RATE 44100.0f
#define EFFECT_LENGTH 6
#define NOTE_LENGTH 6

typedef struct {
	Wave wave;
	Env adsr;
} Effect;

// a group of notes that happen at the same time
typedef struct { 
	float time;
	Note *notes;
	size_t num_notes;
} NoteArray;

// a group of notes that form the same phrase / have similar properties or uses
typedef struct { 
	NoteArray *note_lines;
	Effect effects;
	size_t num_lines;
} MusicBlock;

typedef struct {
	MusicBlock **blocks;
	float speed;
	size_t num_blocks;
} Song;


void parseSong(Song *song, const char *filename);

void clean_song(Song *song);

void print_song(Song song);
