#include "queue.h"
#include <assert.h>
#include <stdlib.h>

queue_t* queue_create(void)
{
	queue_t *q = malloc(sizeof(queue_t));
	q->head = NULL;
	q->tail = NULL;
	q->size = 0;
	return q;
}
void queue_destroy(queue_t* queue)
{
	while(!queue_empty(queue))
		queue_dequeue(queue);
	free(queue);
}

void queue_enqueue(queue_t* queue, command_t* cmd)
{
	commandnode_t *cn = malloc(sizeof(commandnode_t));
	assert(cn);
	if(queue_empty(queue)) {
		queue->head = cn;
		queue->tail = cn;
	} 
	else {
		queue->tail->next = cn;
		queue->tail = cn;
	}
	++queue->size;
	cn->command = cmd;
}
command_t* queue_dequeue(queue_t* queue)
{
	commandnode_t* cn = queue->head;
	if(!cn)
		return NULL;
	queue->head = cn->next;
	--queue->size;
	command_t *c = cn->command;
	free(cn);
	return c;
}
int queue_empty (queue_t* queue)
{
    return queue->size == 0;
}
