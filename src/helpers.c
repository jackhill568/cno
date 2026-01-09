#include "helpers.h"
#include <math.h>
#include <stdbool.h>

float saw_wave (float phase) { return phase * 2 - 1.0f; }
float sine_wave (float phase) { return sin (2 * 3.14 * phase); }
float square_wave (float phase) { return 1.0f - 2.0f * (phase >= 0.5f); }
float triangle_wave (float phase) { return 1.0f - 4.0f * fabsf (0.5f - phase); }

extern float (*wave_functions[NUM_WAVES]) (float)
    = { sine_wave, saw_wave, square_wave, triangle_wave };

int get_free_note (Synth* synth)
{
    for (int i = 0; i < MAX_NOTES; i++)
    {
        if (synth->notes[i].active <= 0.0f)
            return i;
    }
    return -1;
}

void noteOn (Synth* synth, Note* note)
{
    int index = get_free_note (synth);
    if (index < 0)
        return;
    synth->notes[index] = *note;
}

void noteOff (Synth* synth, int index)
{
    if (index < 0 || index >= MAX_NOTES)
        return;
    synth->notes[index].active = 0.0f;
}

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

float note_to_freq (int note) { return 440.0f * powf (2.0f, (note - 69) / 12.0f); }
