#include "helpers.h"

#define SAMPLE_RATE 44100.0f
#define EFFECT_LENGTH 6
#define NOTE_LENGTH 6

void parseSong(Song *song, const char *filename);

void clean_song(Song *song);

void print_song(Song song);

void print_note (Note note);
void print_note_array (NoteArray na);

