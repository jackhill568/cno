
#include "helpers.h"
#include "activeNotes.h"

#include <stdio.h>

int currentNotes = 0;

Node head = {NULL, NULL, NULL};
Node *iter_node = &head;

void add_note(Note *note) {
	Node *current = &head;
	while (current->next != NULL) {
		current = current->next;
	}
	Node *newNode = calloc(1, sizeof(Node));
	if (!newNode) {
		printf("ERROR allocating for new Node\n");
	}
	current->next = newNode;
	newNode->prev = current;
	newNode->note = note;
	currentNotes++;
	return;
}

void remove_note(Note *note) {
	Node *current = &head;
	while (current->next != NULL) {
		if (current->note == note) {
			current->prev->next = current->next;
			current->next->prev = current->prev;
			currentNotes--;
			free(current);
			return;
		}
	}
	printf("Failed to find Note to remove\n");
	return;
}

Node *get_head() {
	return &head;
}


