#include "connection.h"
#include "utils/notify.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "build-config.h"
#include "DebugLib/debuglib.h"

static void *connection_receiver_func(void *args);
static void *connection_sender_func(void *args);
static Proto__ServerMessage* handle_message_announce(connection_t *cn);

typedef Proto__ServerMessage* (*cb_message_handler_t)(connection_t *cn);
/*
    enum Type {
        ANNOUNCE = 0;
        PROCESS_INFO = 1;
        GL_FUNCTIONS = 2;
        FUNCTION_CALL = 3;
        EXECUTION = 4;
        DEBUG_COMMAND = 5;
    }
    */
static cb_message_handler_t handler_funcs[] =
{
    handle_message_announce, 
    handle_message_proc_info, 
    NULL, 
    handle_message_func_call, 
    handle_message_execution, 
    handle_message_debug
};

connection_t* cn_create(socket_t *s, cb_connection_closed_t close_callback)
{
    connection_t *cn = malloc(sizeof(connection_t));
    assert(cn);
    cn->sock = s;
    cn->end_connection = 0;
    cn->receiver = 0;
    cn->sender = 0;
    cn->request = NULL;
    cn->send_queue = queue_create();
    cn->close_callback = close_callback;
    pthread_mutex_init(&cn->mtx_queue, NULL);
    pthread_cond_init(&cn->cond_queue, NULL);
    {   /* receiver */
        pthread_create(&cn->receiver, NULL, connection_receiver_func, cn);
    }
    {   /* sender */
        pthread_create(&cn->sender, NULL, connection_sender_func, cn);
    }
    return cn;
}
void cn_destroy(connection_t *cn)
{
    cn->end_connection = 1;
    pthread_join(cn->receiver, NULL);
    pthread_cancel(cn->sender);
    pthread_join(cn->sender, NULL);
    pthread_mutex_destroy(&cn->mtx_queue);
    pthread_cond_destroy(&cn->cond_queue);
    queue_destroy(cn->send_queue);
    sck_destroy(cn->sock);
    free(cn->request);
    cn->request = NULL;
    cn->send_queue = NULL;
}
static void *connection_receiver_func(void *args)
{
    connection_t *cn = (connection_t*)args;
    uint8_t *buf = NULL;
    size_t num_bytes;
    int ret;
    msg_size_t length;
    msg_size_t allocated_length = 0;
    while (!cn->end_connection) {
        UT_NOTIFY(LV_DEBUG, "awaiting message...");
        if ((ret = sck_recv(cn->sock, &length, sizeof(msg_size_t))) <= 0) {
            UT_NOTIFY(LV_ERROR, "Unable to receive message size (Client disconnected)", ret);
            break;
        }
        if (length > MAX_MESSAGE_SIZE) {
            UT_NOTIFY(LV_ERROR, "Message size exeeds maximum size");
            break;
        }
        if (allocated_length < length) {
            buf = realloc(buf, length);
            assert(buf);
            allocated_length = length;
        }
        num_bytes = 0;
        do {
            UT_NOTIFY(LV_INFO, "Receiving message of size %d", length);
            int read = sck_recv(cn->sock, buf, length);
            if (read <= 0) {
                UT_NOTIFY(LV_ERROR, "Receiving request failed");
                goto fail;
            }
            num_bytes += read;
        } while (num_bytes < length);
        cn->request = proto__client_message__unpack(NULL, num_bytes, buf);
        if (!cn->request) {
            UT_NOTIFY(LV_ERROR, "Unpacking request failed");
            goto fail;
        }
        Proto__ServerMessage *response = NULL;
        /* call handler function */
        response = handler_funcs[cn->request->type](cn);

        proto__client_message__free_unpacked(cn->request, NULL);
        cn->request = NULL;
        if(response) {
            cn_send_message_sync(cn, response);
        }
    }
fail:
    free(buf);
    cn->close_callback(cn, NORMAL);
    return NULL;
}
static void *connection_sender_func(void *args)
{
    connection_t *cn = (connection_t*)args;
    while(!cn->end_connection) {
        pthread_mutex_lock(&cn->mtx_queue);
        while (queue_empty(cn->send_queue))
            pthread_cond_wait(&cn->cond_queue, &cn->mtx_queue);

        /* unqueue cmd & handle it */
        Proto__ServerMessage *response = (Proto__ServerMessage*)queue_dequeue(cn->send_queue);
        pthread_mutex_unlock(&cn->mtx_queue);
        UT_NOTIFY(LV_DEBUG, "server sends message(%s) to client", response->message);
        cn_send_message_sync(cn, response);
        free(response);
    }
    return NULL;
}

static Proto__ServerMessage* handle_message_announce(connection_t *cn)
{
    UT_NOTIFY(LV_INFO, "announce received");
    Proto__ServerMessage *response = malloc(sizeof(Proto__ServerMessage));
    proto__server_message__init(response);
    response->id = cn->request->id;
    Proto__AnnounceDetails *msg = cn->request->announce;
    response->error_code = PROTO__ERROR_CODE__NONE;
    response->message = "Welcome dude!";
    if (msg->id != PROTO_ID) {
        UT_NOTIFY(LV_ERROR, "header mismatch");
        response->error_code = PROTO__ERROR_CODE__HEADER_MISMATCH;
        response->message = "header mismatch";
        cn->end_connection = 1;
    }
    Proto__Version *version = msg->version;
    if (version->major != PROTO_MAJOR) {
        UT_NOTIFY(LV_ERROR, "protocol version mismatch");
        response->error_code = PROTO__ERROR_CODE__VERSION_MISMATCH;
        response->message = "protocol version mismatch";
        cn->end_connection = 1;
    }
    return response;
}
void cn_send_message_sync(connection_t *cn, Proto__ServerMessage *response)
{
    pthread_mutex_lock(&cn->mtx_queue);
    msg_size_t len = proto__server_message__get_packed_size(response);
    UT_NOTIFY(LV_TRACE, "Sending message (%d Bytes)", len);
    size_t num_bytes = sck_send(cn->sock, &len, sizeof(msg_size_t));
    assert(num_bytes == sizeof(msg_size_t));
    uint8_t *buffer = malloc(len);
    assert(buffer);
    proto__server_message__pack(response, buffer);
    num_bytes = sck_send(cn->sock, buffer, len);
    assert(num_bytes == len);
    free(buffer);
    UT_NOTIFY(LV_TRACE, "message sent");
    pthread_mutex_unlock(&cn->mtx_queue);
}
void cn_send_message_async(connection_t *cn, Proto__ServerMessage *msg)
{
    pthread_mutex_lock(&cn->mtx_queue);
    queue_enqueue(cn->send_queue, msg);
    pthread_mutex_unlock(&cn->mtx_queue);
    pthread_cond_signal(&cn->cond_queue);
}