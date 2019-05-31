#ifndef LINKED_LIST_ADT_H
#define LINKED_LIST_ADT_H

#include <stdlib.h>
#include <stdint.h>

typedef struct Node *listADT_t;
typedef struct Node *queueADT_t;

void enqueue(struct Node **head, const void *newData, size_t size);
void removeFirst(struct Node **head);
void *getFirstWithSize(struct Node **head, size_t *size);
void *getFirst(struct Node **head);
void freeList(struct Node **head);
uint8_t isEmpty(struct Node **head);

#endif
