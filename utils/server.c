
#include "socket.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include "server.h"
#include "proto/protocol.h"

static struct {
    pthread_t* acceptor_thread;
    pthread_mutex_t mutex;
    int shutdown;
    cb_connection_t new_connection_callback;
} g_server = { NULL, PTHREAD_MUTEX_INITIALIZER, 0, NULL };

void* server_end(void* args)
{
    socket_t *sock = (socket_t*)args;
    sck_close(sock);
    sck_destroy(sock);
    return NULL;
}

void* acceptor_run(void* args)
{
    pthread_cleanup_push(server_end, args);
    socket_t *sock = (socket_t*)args;
    socket_t *peer;
    while(1) {
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        peer = NULL;
        if(sck_accept(sock, &peer) != 0) {
            sv_acceptor_stop();
            return NULL;
        }
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
        if(g_server.shutdown)
            break;
        if(!peer)
            continue;
        /* check if we accept the connection */
        if(g_server.new_connection_callback && g_server.new_connection_callback(peer) == 0) 
            continue;
        /* would be nice to tell the peer why we kick him but we don't know
         * him anyway... */
        sck_close(peer);
        sck_destroy(peer);
    }
    pthread_cleanup_pop(1);
    return NULL;
}
int acceptor_create(socket_t *s, cb_connection_t callback)
{
    if(sck_listen(s) != 0)
        return -1;
    g_server.new_connection_callback = callback;
    g_server.acceptor_thread = malloc(sizeof(pthread_t));
    pthread_create(g_server.acceptor_thread, NULL, acceptor_run, (void*)s);
    return 0;
}
int sv_acceptor_start_tcp(int port, cb_connection_t callback)
{
    if(sv_acceptor_is_running())
        return 0;
    sck_begin();
    socket_t *s = sck_create(NULL, port, SCK_INET, SCK_STREAM);
    if(!s)
        return -1;
    return acceptor_create(s, callback);
}
int sv_acceptor_start_unix(const char* path, cb_connection_t callback)
{
    if(sv_acceptor_is_running())
        return 0;
    socket_t *s = sck_create(path, 0, SCK_UNIX, SCK_STREAM);
    if(!s)
        return -1;
    return acceptor_create(s, callback);
}
int sv_acceptor_is_running()
{
    return g_server.acceptor_thread == 0 ? 0 : 1;
}
void sv_acceptor_stop()
{
    if(g_server.acceptor_thread) {
        g_server.shutdown = 1;
        pthread_cancel(*g_server.acceptor_thread);
        pthread_join(*g_server.acceptor_thread, NULL);
        free(g_server.acceptor_thread);
        g_server.acceptor_thread = NULL;
        g_server.shutdown = 0;
        sck_end();
    }
}

