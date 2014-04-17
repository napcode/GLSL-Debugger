#ifndef CONNECTION_H
#define CONNECTION_H 1

#include "utils/socket.h"
#include "utils/queue.h"
#include "proto/protocol.h"

enum CN_CLOSE_REASON 
{
	NORMAL, 
	DISCONNECT
};

typedef struct _connection connection_t;
typedef void (*cb_connection_closed_t)(connection_t *cn, enum CN_CLOSE_REASON r);

struct _connection {
    socket_t *sock;
    int end_connection;
    pthread_t receiver;
    pthread_t sender;
    Proto__ClientRequest *request;
    queue_t *send_queue;
    pthread_mutex_t mtx_queue;
  	pthread_cond_t cond_queue; 
  	cb_connection_closed_t close_callback;
};

connection_t* cn_create(socket_t *s, cb_connection_closed_t close_callback);
void cn_destroy(connection_t *cn);
void cn_send_message(connection_t *cn, Proto__ServerMessage *response);

//int cn_create_tcp(connection_t* c, const char* host, int port);
//int cn_create_unix(connection_t* c, const char* path);
//int cn_verify(socket_t*);
//int cn_destroy(connection_t* con);

//int cn_send(connection_t* con, const pkg_header_t* pkg);
//int cn_receive(connection_t* con, pkg_header_t* pkg);

#endif
