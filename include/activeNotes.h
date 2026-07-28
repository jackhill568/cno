



#ifndef ACTIVE_NOTES_H
#define ACTIVE_NOTES_H

typedef struct Node {
	struct Node *prev;
	Note *note;
	struct Node *next;
} Node;

void add_note(Note *note);

void remove_note(Note *note);

Node *get_head();


#endif
