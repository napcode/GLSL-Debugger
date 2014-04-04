#include "connection.h"
#include "notify.h"
#include <stdio.h>
#include <stdlib.h>
#include "proto/protocol.h"
#include "build-config.h"

int do_handshake(connection_t *c, socket_t *s);

int cn_create_tcp(connection_t* c, const char* host, int port)
{
    if(!c)
        return -1;

    socket_t *s = sck_create(host, port, SCK_INET, SCK_STREAM);
    if(sck_connect(s) != 0) {
        return -1;
    }
    return do_handshake(c, s);
}
int cn_create_unix(connection_t *c, const char* path)
{
    if(!c)
        return -1;

    socket_t *s = sck_create(path, 0, SCK_UNIX, SCK_STREAM);
    if(sck_connect(s) != 0) {
        return -1;
    }
    return do_handshake(c, s);
}
int do_handshake(connection_t *c, socket_t *s)
{
    size_t num_bytes;
    void *buf;
    MsgAnnounce msg = MSG_ANNOUNCE__INIT; 
    MsgVersion version = MSG_VERSION__INIT; 
    msg.id = PROTO_ID; 
    msg.client_name = "DEMO_CLIENT"; 
    version.major = PROTO_MAJOR;
    version.minor = PROTO_MINOR;
    version.revision = PROTO_REVISION;
    msg.version = &version;

    int len = msg_announce__get_packed_size(&msg);
    buf = malloc(len);
    msg_announce__pack(&msg, buf);
    free(buf);
    
    if((num_bytes = sck_send(s, buf, len) ) != len) {
        UT_NOTIFY(LV_ERROR, "unable to send proto_header");
        return -1;
    }
    if(cn_handshake(s) == -1)
        return -1;
    c->sock = s;
    return 0;
}
int cn_handshake(socket_t *s)
{
    MsgAnnounce *msg;
    MsgVersion *version;
    uint8_t buf[MAX_MESSAGE_SIZE];
    size_t num_bytes;
    if ((num_bytes = sck_recv(s, buf, MAX_MESSAGE_SIZE) ) > 0) {
        msg = msg_announce__unpack(NULL, num_bytes, buf);
        if (msg == NULL) {
            UT_NOTIFY(LV_ERROR, "could not read header package");
            return -1;
        }
        if(msg->id != PROTO_ID) {
            UT_NOTIFY(LV_ERROR, "header mismatch");
            goto fail;
        }
        version = msg->version;
        if(version->major != PROTO_MAJOR) {
            UT_NOTIFY(LV_ERROR, "protocol version mismatch");
            goto fail;
        }
        return 0;
    }
fail:
    msg_announce__free_unpacked(msg, NULL);
    return -1;
}
int cn_destroy(connection_t* con)
{
    sck_close(con->sock);
    sck_destroy(con->sock);
    return 0;
}
/*
void *cn_listen(void *args)
{
    socket_t *peer = (socket_t *)args;
    header_t header;
    header.msg[0] = '\0';
    int read;
    while ((read = sck_recv(peer, (char *)&header, sizeof(header)) ) > 0)
    {
        if (read != sizeof(header))
        {
            UT_NOTIFY(LV_ERROR, "read invalid header");
            break;
        }
        g_server.connection_callback(peer, &header);
    }
    printf("end peer handler");
    sck_close(peer);
    sck_destroy(peer);
    return NULL;
}
*/
