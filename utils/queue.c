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

void queue_enqueue(queue_t* queue, void* cmd)
{
	node_t *cn = malloc(sizeof(node_t));
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
	cn->data = cmd;
}
void* queue_dequeue(queue_t* queue)
{
	node_t* cn = queue->head;
	if(!cn)
		return NULL;
	queue->head = cn->next;
	--queue->size;
	void *data = cn->data;
	free(cn);
	return data;
}
int queue_empty(queue_t* queue)
{
    return queue->size == 0;
}
