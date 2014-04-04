#ifndef SERVER_H
#define SERVER_H 1

#define MAX_CONNECTIONS 4

#include "socket.h"

/* function should return 0 when this connection has been accepted. Otherwise != 0 
 * If the connection is accepted ownership is transfered to the acceptor
 */
typedef int (*cb_connection_t)(socket_t*);

int cn_acceptor_start_tcp(int port, cb_connection_t);
int cn_acceptor_start_unix(const char* path, cb_connection_t);
int cn_acceptor_is_running();
void cn_acceptor_stop();

#endif
