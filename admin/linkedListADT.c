#include <stdio.h>
#include <string.h>
#include <linkedListADT.h>

static struct Node *createNode(const void *data, size_t size);
static void freeNode(struct Node *node);

struct Node {
	void *data;
	size_t size;
	struct Node *next;
};

void enqueue(struct Node **head, const void *newData, size_t size) {
	if (head == NULL) {
		printf("error\n"); // TODO: check later
		return;
	}

	struct Node *newNode = createNode(newData, size);

	if (*head == NULL) {
		*head = newNode;
		return;
	}

	struct Node *aux = *head;

	while (aux->next != NULL) {
		aux = aux->next;
	}

	aux->next = newNode;
}

uint8_t isEmpty(struct Node **head) {
	if (*head == NULL) {
		return 1;
	}

	return 0;
}

void removeFirst(struct Node **head) {
	if (head != NULL) {
		struct Node *node = *head;
		if (node != NULL) {
			*head = (*head)->next;
			freeNode(node);
		}
	}
}

void *getFirstWithSize(struct Node **head, size_t *size) {
	if (head == NULL || *head == NULL) {
		return NULL;
	}

	void *data = malloc(sizeof((*head)->size));
	*size	  = (*head)->size;

	if (data == NULL) {
		printf("error\n"); // TODO: check later
	}

	memcpy(data, (*head)->data, *size);
	removeFirst(head);
	return data;
}

void *getFirst(struct Node **head) {
	if (head == NULL || *head == NULL) {
		return NULL;
	}

	void *data  = malloc(sizeof((*head)->size));
	size_t size = (*head)->size;

	if (data == NULL) {
		printf("error\n"); // TODO: check later
	}

	memcpy(data, (*head)->data, size);
	removeFirst(head);
	return data;
}

void freeList(struct Node **head) {
	if (head != NULL) {
		struct Node *aux = *head;

		while (aux != NULL) {
			*head = (*head)->next;
			free(aux);
			aux = *head;
		}
	}
}

static struct Node *createNode(const void *data, size_t size) {
	struct Node *newNode = (struct Node *) malloc(sizeof(struct Node));

	if (newNode == NULL) {
		goto fail;
	}

	newNode->data = malloc(size);

	if (newNode->data == NULL) {
		goto fail;
	}

	newNode->size = size;
	newNode->next = NULL;
	memcpy(newNode->data, data, size);

	return newNode;

fail:

	printf("error\n"); // TODO: check later
	exit(0);
}

static void freeNode(struct Node *node) {
	free(node->data);
	free(node);
}
