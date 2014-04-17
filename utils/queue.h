#ifndef UTILS_COMMANDQUEUE_H
#define UTILS_COMMANDQUEUE_H

#include <stdint.h>

typedef struct node
{
	struct node *next;
	void *data;

} node_t;

typedef struct queue {
	node_t* head;
	node_t* tail;
	uint32_t size;
} queue_t; 

queue_t* queue_create(void);
void queue_destroy(queue_t* queue);

void queue_enqueue(queue_t* queue, void* c);
void* queue_dequeue(queue_t* queue);

int queue_empty (queue_t* queue);

#endif