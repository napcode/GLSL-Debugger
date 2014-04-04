#ifndef UTILS_COMMANDQUEUE_H
#define UTILS_COMMANDQUEUE_H

#include <stdint.h>

typedef struct command 
{
	uint32_t dummy;
} command_t;

typedef struct commandnode
{
	struct commandnode *next;
	command_t *command;

} commandnode_t;

typedef struct queue {
	commandnode_t* head;
	commandnode_t* tail;
	uint32_t size;
} queue_t; 

queue_t* queue_create(void);
void queue_destroy(queue_t* queue);

void queue_enqueue(queue_t* queue, command_t* c);
command_t* queue_dequeue(queue_t* queue);

int queue_empty (queue_t* queue);

#endif