#ifndef SOCKET_H
#define SOCKET_H 1

#include <stdint.h>

#define SCK_BACKLOG 10
#define SCK_HOSTLEN 256

enum SCK_DOMAIN {
#ifndef GLSLDB_WIN
    SCK_UNIX, 
#endif
    SCK_INET, 
    SCK_INET6
};

enum SCK_TYPE {
    SCK_STREAM, 
    SCK_DGRAM
};

typedef struct 
{
    int fd;
    int port;                   // TCP socket
    char host[SCK_HOSTLEN];     // path if UNIX socket
    enum SCK_DOMAIN domain;
    enum SCK_TYPE type;
} socket_t;

int sck_begin();
void sck_end();

socket_t* sck_create(const char* host, int port, enum SCK_DOMAIN, enum SCK_TYPE);
int sck_listen(socket_t* s);
int sck_accept(socket_t* listen, socket_t** out);
void sck_destroy(socket_t* s);
int sck_connect(socket_t* s);
int sck_send(socket_t* s, const void* msg, uint32_t len);
int sck_recv(socket_t* s, void* msg, uint32_t len);
int sck_close(socket_t* s);

#endif
