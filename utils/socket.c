#include "socket.h"
#include "notify.h"
#include "build-config.h"
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef GLSLDB_WIN
#	include <unistd.h>
#	include <sys/types.h>
#	include <arpa/inet.h>
#   include <sys/un.h>
#	include <sys/socket.h>
#   include <netdb.h>
#endif

#ifdef GLSLDB_WIN
#   define _WIN32_WINNT 0x0501
//#	include <windows.h>
#   include <winsock2.h>
#   include <ws2tcpip.h>
#endif

static int g_sck_initialized = 0;

#ifdef GLSLDB_WIN
int inet_pton(int af, const char *src, void *dst)
{
    struct sockaddr_storage ss;
    int size = sizeof(ss);
    char src_copy[INET6_ADDRSTRLEN+1];

    ZeroMemory(&ss, sizeof(ss));
    /* stupid non-const API */
    strncpy (src_copy, src, INET6_ADDRSTRLEN+1);
    src_copy[INET6_ADDRSTRLEN] = 0;

    if (WSAStringToAddress(src_copy, af, NULL, (struct sockaddr *)&ss, &size) == 0) {
        switch(af) {
            case AF_INET:
                *(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
                return 1;
            case AF_INET6:
                *(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
                return 1;
        }
    }
    return 0;
}

const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
    struct sockaddr_storage ss;
    unsigned long s = size;

    ZeroMemory(&ss, sizeof(ss));
    ss.ss_family = af;

    switch(af) {
        case AF_INET:
            ((struct sockaddr_in *)&ss)->sin_addr = *(struct in_addr *)src;
            break;
        case AF_INET6:
            ((struct sockaddr_in6 *)&ss)->sin6_addr = *(struct in6_addr *)src;
            break;
        default:
            return NULL;
    }
    /* cannot direclty use &size because of strict aliasing rules */
    return (WSAAddressToString((struct sockaddr *)&ss, sizeof(ss), NULL, dst, &s) == 0)?
        dst : NULL;
}
#endif

int platform_domain(enum SCK_DOMAIN d)
{
    switch(d) {
        case SCK_INET:
            return AF_INET;
        case SCK_INET6:
            return AF_INET6;
#ifndef GLSLDB_WIN
        case SCK_UNIX:
            return AF_UNIX;
#endif
    }
    return AF_INET;
}
int platform_type(enum SCK_TYPE t)
{
    switch(t) {
        case SCK_STREAM:
            return SOCK_STREAM;
        case SCK_DGRAM:
            return SOCK_DGRAM;
    }
    return SOCK_STREAM;
}

int sck_begin()
{
    if(g_sck_initialized)
        return 0;
#ifdef GLSLDB_WIN
    WSADATA wsa;
    int res = WSAStartup(MAKEWORD(2, 2), &wsa);
    if(res != 0) {
        UT_NOTIFY(LV_ERROR, "Unable to initialise socket API");
        return -1;
    }
#endif
    g_sck_initialized = 1;
    return 0;
}
void sck_end()
{
#ifdef GLSLDB_WIN
    WSACleanup();
#endif
    g_sck_initialized = 0;
}

socket_t* sck_create(const char* host, int port, enum SCK_DOMAIN d, enum SCK_TYPE t)
{
    socket_t *s = (socket_t*)malloc(sizeof(socket_t));
    memset(s, 0, sizeof(socket_t));
    s->fd = socket(platform_domain(d), platform_type(t), 0);
    if(s->fd == -1) {
        UT_NOTIFY(LV_ERROR, "Unable to create socket: %s", strerror(errno));
        free(s);
        return NULL;
    }

    
    if(host) {
        strncpy(s->host, host, SCK_HOSTLEN - 1);
    }
    else
        s->host[0] = '\0';
    s->domain = d;
    s->type = t;
    s->port = port;
    return s;
}
void sck_destroy(socket_t* s)
{
    /* TODO disconnect/close socket */
    free(s);
}
struct addrinfo sck_get_hints(socket_t *s)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = platform_domain(s->domain);
    hints.ai_socktype = platform_type(s->type);
    hints.ai_protocol = 0;
    return hints;
}
int sck_listen(socket_t* s)
{
    int res;
    char value[6];
    if(s->domain == SCK_UNIX) {
        struct sockaddr_un addr;
        if(remove(s->host) == -1 && errno != ENOENT) {
            UT_NOTIFY(LV_ERROR, "failed to remove %s: %s", s->host, strerror(errno));
            return -1;
        }
        memset(&addr, 0, sizeof(struct sockaddr_un));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, s->host, sizeof(addr.sun_path) - 1);
        if(bind(s->fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1) {
            UT_NOTIFY(LV_ERROR, "Binding socket failed: %s", strerror(errno));
            return -1;
        }
    } 
    else {
        struct addrinfo hints, *infos;
        hints = sck_get_hints(s);
        hints.ai_flags = AI_PASSIVE;
        snprintf(value, 6, "%d", s->port);
        if((res = getaddrinfo(NULL, value, &hints, &infos)) != 0) {
            UT_NOTIFY(LV_ERROR, "Unable to retrieve suitable listening addresses: %s", gai_strerror(res));
            return -1;
        }

        snprintf(value, 6, "%d", 1);
        if(setsockopt(s->fd, SOL_SOCKET, SO_REUSEADDR, value, sizeof(value)) == -1) {
            UT_NOTIFY(LV_ERROR, "Setting socket options failed: %s", strerror(errno));
            freeaddrinfo(infos);
            return -1;
        }
   

        if(bind(s->fd, infos->ai_addr, infos->ai_addrlen) == -1) {
            UT_NOTIFY(LV_ERROR, "Binding socket failed: %s", strerror(errno));
            freeaddrinfo(infos);
            return -1;
        }
        freeaddrinfo(infos);
    }

    if(listen(s->fd, SCK_BACKLOG) == -1) {
        UT_NOTIFY(LV_ERROR, "Listening on socket failed: %s", strerror(errno));
        return -1;
    }
    return 0;
}
int sck_accept(socket_t* listen, socket_t** out)
{
    struct sockaddr_storage storage;
    socklen_t length = sizeof(storage);
    int res;
    if((res = accept(listen->fd, (struct sockaddr*)&storage, &length)) == -1) {
        UT_NOTIFY(LV_ERROR, "Accepting on socket failed: %s", strerror(errno));
        return -1;
    }
    *out = (socket_t*)malloc(sizeof(socket_t));
    memset(*out, 0, sizeof(sizeof(socket_t)));
    **out = *listen;
    (*out)->fd = res;

    struct timeval tv;
    tv.tv_sec = NETWORK_TIMEOUT;  /* 5 Secs Timeout */
    tv.tv_usec = 0;      
    if(setsockopt((*out)->fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval)) == -1) {
        UT_NOTIFY(LV_ERROR, "Setting socket timeout failed: %s", strerror(errno));
    }
    return 0;
}
int sck_connect(socket_t* s)
{
    if(s->domain == SCK_UNIX) {
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(struct sockaddr_un));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, s->host, sizeof(addr.sun_path) - 1);
        if(connect(s->fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            UT_NOTIFY(LV_ERROR, "Unable to connect: %s", strerror(errno));
            return -1;
        }
    }
    else {
        struct sockaddr_in addr;
        memset(&addr, '\0', sizeof(addr));
        addr.sin_family = platform_domain(s->domain);
        addr.sin_port = htons(s->port);
        int res = inet_pton(platform_domain(s->domain), s->host, &addr.sin_addr);
        if(res == 0) {
            UT_NOTIFY(LV_ERROR, "Host does not contain a valid address");
            return -1;
        }
        else if (res == -1) {
            UT_NOTIFY(LV_ERROR, "Erroneous address family specified");
            return -1;
        }
        if(connect(s->fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            UT_NOTIFY(LV_ERROR, "Unable to connect: %s", strerror(errno));
            return -1;
        }
    }
    return 0;
}
int sck_close(socket_t* s)
{
#ifdef GLSLDB_WIN
    shutdown(s->fd, SD_BOTH);
#else
    shutdown(s->fd, SHUT_RDWR);
#endif // GLSLDB_WIN
    return close(s->fd);
}
int sck_send(socket_t* s, const void* msg, uint32_t len)
{
    int ret;
    ret = send(s->fd, msg, len, 0);
    if(ret == -1) {
        UT_NOTIFY(LV_ERROR, "Send failed: %s", strerror(errno));
    }
    return ret;
}
int sck_recv(socket_t* s, void* msg, uint32_t len)
{
    int ret;
    /* receive as long as the error is timeout */
    do {
        ret = recv(s->fd, msg, len, 0);
    } while(ret == -1 && errno == EAGAIN);

    if(ret == -1) {
        UT_NOTIFY(LV_ERROR, "Receive failed: %s", strerror(errno));
    }
    return ret;
}


