#include <stdbool.h>
#include <stdlib.h>
#include "envlope.h"
#include <stdint.h>

#ifndef NOTE_H
#define NOTE_H

#define MAX_NOTES 20
#define MAX_KARPLUS_DELAY 2400
typedef enum {
	SINE, SAW, SQUARE, TRIANGLE, KARPLUS, NUM_WAVES
} Wave;


typedef struct Note{
	float phase;
	float frequency;
	float amplitude;
	float active; // is the note on or off
	float (*func)(struct Note *); // the primitive sound function e.g. sine or square
	bool started;
	Env envlope;
	float delay[MAX_KARPLUS_DELAY];
	int delayIndex;
	int delayLength;
} Note;


// NoteArrays is the structure that store notes that start at the same time in a block
// so most likely a note array is storing a chord
typedef struct { 
	float time;
	Note *notes;
	size_t num_notes;
	uint64_t start_sample;
} NoteArray;

// 
typedef struct {
	NoteArray *master_queue;
	size_t size;
	float amplitude;
	float sample_rate;
	uint64_t current_sample;
	float start_time;
} Synth;

// effects can be applied to blocks or individually to notes
typedef struct {
	Wave wave;
	Env adsr;
} Effect;


typedef struct
{
    size_t num_lines;
    size_t num_notes;
} blockSizeData;

// a music block represents the notes of one 'instrument' 
//
typedef struct { 
	NoteArray *note_lines;
	Effect effects;
	blockSizeData music_size;
} MusicBlock;

typedef struct {
	MusicBlock **blocks;
	float speed;
	size_t num_blocks;
} Song;
extern float (*wave_functions[NUM_WAVES])(Note *);

float saw_wave(Note *note);

float sine_wave(Note *note);

float square_wave(Note *note);

float triangle_wave(Note *note);

int note_name_to_midi(const char *note);

float note_to_freq(int note);

int compare_by_time(const void *a, const void *b);

typedef struct SongTree {
	NoteArray *na;
	int size;
	struct SongTree *next;
} SongTree;

bool addNodeToSongBranch(SongTree *array_current, NoteArray *na);

void cleanSongTree(SongTree *head);

SongTree *createSongTree(NoteArray *masterqueue, int nasize);

#endif
