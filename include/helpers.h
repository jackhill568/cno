#include <stdbool.h>
#include "envlope.h"

#ifndef NOTE_H
#define NOTE_H

#define MAX_NOTES 16
typedef struct {
	float phase;
	float frequency;
	float amplitude;
	float active;
	float (*func)(float);
	Env envlope;
} Note;

typedef struct {
	Note notes[MAX_NOTES];
	float amplitude;
	float sample_rate;
} Synth;

typedef enum {
	SINE, SAW, SQUARE, TRIANGLE, NUM_WAVES
} Wave;

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


#endif
