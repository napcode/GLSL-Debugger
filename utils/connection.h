#ifndef CONNECTION_H
#define CONNECTION_H 1

#include "server.h"

typedef int (*cb_request_t)(socket_t*);
typedef int (*cb_response_t)(socket_t*);

typedef struct {
    socket_t* sock;
    cb_request_t callback_request;
    cb_response_t callback_response;
} connection_t;

int cn_create_tcp(connection_t* c, const char* host, int port);
int cn_create_unix(connection_t* c, const char* path);
int cn_handshake(socket_t*);
int cn_destroy(connection_t* con);

//int cn_send(connection_t* con, const pkg_header_t* pkg);
//int cn_receive(connection_t* con, pkg_header_t* pkg);

#endif
