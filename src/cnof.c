#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cnof.h"

typedef struct
{
    size_t num_lines;
    size_t num_notes;
} blockSizeData;

typedef enum
{
    HEADER,
    INTER_EFFECT,
    EFFECT_BLOCK,
    INTER_NOTE,
    NOTE_BLOCK
} Cnof_state;

typedef enum
{
    READ_NOTE,
    READ_TIME,
    READ_NOTE_TIME
} Read_Notes;

typedef enum
{
    READ_ADSR,
    READ_WAVE,
    READ_EFFECT_NAME
} Read_effect;

typedef enum
{
    WAITING,
    INBLOCK
} Song_state;

static inline void resetchar (char* ptr, int* i, size_t size)
{
    memset (ptr, 0, size);
    *i = 0;
}

static inline bool isnotspace (const char c)
{
    return !(c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r');
}

blockSizeData get_block_sizes (char* data)
{
    bool inNoteBlock = false;
    bool inAddition = false;
    blockSizeData sizeData = { 0 };

    for (char* ptr = data; *ptr != ']'; ptr++)
    {
        if (*ptr == '[')
            inNoteBlock = true;

        if (!inNoteBlock)
            continue;

        switch (*ptr)
        {
        case '{':
            sizeData.num_lines++;
            break;
        case ',':
            if (!inAddition)
                sizeData.num_notes++;
            break;
        case '+':
            inAddition = true;
            break;
        case ';':
            inAddition = false;
            break;
        default:
            break;
        }
    }
    printf ("size of block: lines: %d, notes: %d\n", (int)sizeData.num_lines,
            (int)sizeData.num_notes);
    return sizeData;
}

char* effects (Effect* sound, char* data)
{
    Read_effect state = READ_EFFECT_NAME;
    char effect[EFFECT_LENGTH];
    int effectptr = 0;
    float adsr[4];
    int adsrptr = 0;
    char* ptr = data;

    while (*ptr != ')')
    {
        char c = *ptr;
        switch (state)
        {
        case READ_EFFECT_NAME:
            if (!isnotspace (c))
                break; // is space
            if (c == ',')
                return ptr; // case when called by notes
            else if (c != ':')
            {
                if (effectptr < EFFECT_LENGTH - 1)
                    effect[effectptr++] = c;
                else
                    printf ("EFFECT OVERFLOW ... current char: %c\n", c);
            }
            else
            {
                effect[effectptr++] = '\0';
                if (strcmp (effect, "WAVE") == 0)
                    state = READ_WAVE;
                else if (strcmp (effect, "ADSR") == 0)
                    state = READ_ADSR;
                resetchar (&effect[0], &effectptr, sizeof (effect));
            }
            break;

        case READ_WAVE:
            if (c != ';')
                if (effectptr < EFFECT_LENGTH - 1)
                    effect[effectptr++] = c;
                else
                    printf ("EFFECT OVERFLOW ... current char: %c\n", c);
            else
            {
                effect[effectptr++] = '\0';
                if (strcmp (effect, "sin") == 0)
                    sound->wave = SINE;
                else if (strcmp (effect, "saw") == 0)
                    sound->wave = SAW;
                else if (strcmp (effect, "squ") == 0)
                    sound->wave = SQUARE;
                else
                    sound->wave = TRIANGLE;
                state = READ_EFFECT_NAME;
                resetchar (&effect[0], &effectptr, sizeof (effect));
            }
            break;

        case READ_ADSR:
            if (c == ';')
            {
                effect[effectptr++] = '\0';
                adsr[adsrptr++] = atof (effect);
                env_init (&sound->adsr, SAMPLE_RATE, adsr[0], adsr[1], adsr[2], adsr[3]);
                state = READ_EFFECT_NAME;
                resetchar (&effect[0], &effectptr, sizeof (effect));
            }
            else if (c == ',')
            {
                adsr[adsrptr++] = atof (effect);
                resetchar (&effect[0], &effectptr, sizeof (effect));
            }
            else if (effectptr < EFFECT_LENGTH - 1)
                effect[effectptr++] = c;
            else
                printf ("EFFECT OVERFLOW ... current char: %c\n", c);
            break;

        default:
            break;
        }
        ptr++;
    }
    return ptr;
}

char* notes (MusicBlock* block, char* data, Note* freeptr)
{
    Read_Notes state = READ_TIME;
    char note[NOTE_LENGTH];
    int noteptr = 0;
    char* ptr = data;
    int notearrptr = 0;
    Note* cursor = NULL;

    while (*ptr != ']')
    {
        switch (state)
        {
        case READ_TIME:
            if (!isnotspace (*ptr))
                break;
            if (*ptr == '{')
            {
                if (!isnotspace (*ptr))
                    break;
                block->note_lines[notearrptr].time = atof (note);
                block->note_lines[notearrptr].num_notes = 0;
                state = READ_NOTE;
                resetchar (&note[0], &noteptr, sizeof (note));
            }
            else
                note[noteptr++] = *ptr;
            break;

        case READ_NOTE_TIME:
            if (*ptr == '+' || *ptr == ',')
            { // if the next line isnt part of the number
                (*freeptr).active = atof (note);
                ptr--;
                state = READ_NOTE;
                resetchar (&note[0], &noteptr, sizeof (note));
            }
            else
                note[noteptr++] = *ptr;
            break;

        case READ_NOTE:
            if (*ptr == ',')
            { // end of note info
                (*freeptr).amplitude = 1.0f;
                (*freeptr).phase = 0.0f;
                if (!(*freeptr).func)
                    (*freeptr).func = wave_functions[block->effects.wave];
                if (!(*freeptr).envlope.state)
                    (*freeptr).envlope = block->effects.adsr;
                if (!cursor)
                    cursor = freeptr;
                // initialise the next note to have block conf
                if (*(ptr + 1) != '}')
                {
                    *(++freeptr) = (Note){};
                    freeptr->func = wave_functions[block->effects.wave];
                    freeptr->envlope = block->effects.adsr;
                    block->note_lines[notearrptr].num_notes++;
                }
                resetchar (&note[0], &noteptr, sizeof (note));
            }
            else if (*ptr == '/')
            { // end of pitch info
                state = READ_NOTE_TIME;
                resetchar (&note[0], &noteptr, sizeof (note));
            }
            else if (*ptr == '+')
            { // end of rythm info (start of extra effects)
                Effect temp;
                ptr = effects (&temp, ptr + 1) - 1;
                (*freeptr).func = wave_functions[temp.wave];
                (*freeptr).envlope = temp.adsr;
            }
            else if (*ptr == '}')
            { // enf of time
                block->note_lines[notearrptr++].notes = cursor;
                freeptr++;
                cursor = NULL;
                state = READ_TIME;
                resetchar (&note[0], &noteptr, sizeof (note));
            }
            else
            {
                note[noteptr++] = *ptr;
                if (*(ptr + 1) == '/' || *(ptr + 1) == '+' || *(ptr + 1) == ',')
                { // note buffer is full and not loading time
                    // therefore must be a freq
                    (*freeptr).frequency = note_to_freq (note_name_to_midi (&note[0]));
                    resetchar (&note[0], &noteptr, sizeof (note));
                    if (*(ptr + 1) != '/')
                        (*freeptr).active = 1.0f; // if no explicit length
                }
            }
            break;

        default:
            break;
        }
        ptr++;
    }
    return ptr;
}

void* parseBlock (Song *song, int block_index, char* data)
{
    blockSizeData music_size = get_block_sizes (data);

    size_t block_mem = sizeof (MusicBlock) + sizeof (NoteArray) * (music_size.num_lines)
                       + sizeof (Note) * (music_size.num_notes+1);

    song->blocks[block_index]= calloc (1, block_mem);
		MusicBlock * block = song->blocks[block_index];
    if (!block)
        return NULL;
    block->num_lines = music_size.num_lines;

    char* freeptr = (char*)(block + 1);
    block->note_lines = (NoteArray*)freeptr;
    freeptr += sizeof (NoteArray) * (music_size.num_lines + 1);

    Cnof_state state = HEADER;
    for (char* ptr = data; *ptr != ']'; ptr++)
    {
        switch (state)
        {
        case HEADER:
            if (*ptr == '(')
                state = INTER_EFFECT;
            break;
        case INTER_EFFECT:
            if (isnotspace (*ptr))
            {
                state = EFFECT_BLOCK;
                ptr--;
            }
            break;
        case EFFECT_BLOCK:
            ptr = effects (&block->effects, ptr);
            state = INTER_NOTE;
            break;
        case INTER_NOTE:
            if (*ptr != ')' && isnotspace (*ptr) && *ptr != '[')
            {
                state = NOTE_BLOCK;
                ptr--;
            }
            break;
        case NOTE_BLOCK:
            ptr = notes (block, ptr, (Note*)freeptr);
            return block;
            break;
        }
    }
    return block;
}

char* read_file (const char* filename)
{
    FILE* f = fopen (filename, "r");
    if (!f)
    {
        printf ("file open failed");
        return NULL;
    }
    fseek (f, 0, SEEK_END);
    long size = ftell (f);
    rewind (f);
    char* buffer = malloc (size + 1);
    if (!buffer)
    {
        fclose (f);
        return NULL;
    }
    fread (buffer, 1, size, f);
    buffer[size] = '\0';
    fclose (f);
    return buffer;
}

size_t count_blocks (char* data)
{
    size_t count=0;
    for (char* ptr = data; *ptr != '\0'; ptr++)
    {
        if (*ptr == ']')
            count++;
    }
    return count;
}

void parseSong (Song* song, const char* filename)
{
    char* data = read_file (filename);
    if (!data)
    {
        printf ("file read failed");
        return;
    }
    char* blockstart;
    char* ptr = data;
    song->num_blocks = count_blocks (data);
    int block_index = 0;
    song->blocks = (MusicBlock**)malloc (sizeof (MusicBlock*) * (song->num_blocks));
    if (!song->blocks)
        return;
    Song_state state = WAITING;

    printf ("starting song parsing\n");
    while (*ptr != '\0')
    {
        switch (state)
        {
        case WAITING:
            if (isnotspace (*ptr))
            {
                blockstart = ptr;
                state = INBLOCK;
            }
            break;
        case INBLOCK:
            if (*ptr == ']')
            {
                parseBlock (song, block_index++, blockstart);
                state = WAITING;
                ptr++;
            }
            break;
        }
        ptr++;
    }
    free (data);
}

void clean_song (Song* song)
{
    for (int blockptr = 0; blockptr<song->num_blocks; blockptr++)
    {
        free (song->blocks[blockptr]);
    }
    free (song->blocks);
}

void print_note (Note note) { printf (" %f", note.frequency); }
void print_note_array (NoteArray na)
{
    printf ("time: %f\n", na.time);
    for (int i =0; i< na.num_notes; i++)
    {
        print_note (na.notes[i]);
    }
    printf ("\n");
}
void print_music_block (MusicBlock block)
{
    printf ("block: \n");
    for (int i = 0; i < block.num_lines; i++)
    {
        print_note_array (block.note_lines[i]);
    }
}
void print_song (Song song)
{
    for (int blockptr = 0; blockptr < song.num_blocks; blockptr++)
    {
        print_music_block (*song.blocks[blockptr]);
    }
}
