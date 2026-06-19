#include <stdbool.h>
#include <stdlib.h>
#include "envlope.h"
#include <stdint.h>

#ifndef NOTE_H
#define NOTE_H

#define MAX_NOTES 20
typedef enum {
	SINE, SAW, SQUARE, TRIANGLE, NUM_WAVES
} Wave;


typedef struct {
	float phase;
	float frequency;
	float amplitude;
	float active; // is the note on or off
	float (*func)(float); // the primitive sound function e.g. sine or square
	bool started;
	Env envlope;
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
extern float (*wave_functions[NUM_WAVES])(float);

float saw_wave(float phase);

float sine_wave(float phase);

float square_wave(float phase);

float triangle_wave(float phase);

int get_free_note(Synth *synth);

void noteOn(Synth *synth, Note *note);

void noteOff(Synth *synth, int index);

int note_name_to_midi(const char *note);

float note_to_freq(int note);

int compare_by_time(const void *a, const void *b);

#endif
