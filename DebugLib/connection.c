#include "connection.h"
#include "utils/notify.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include "build-config.h"
#include "DebugLib/debuglib.h"

static void *connection_handler_receiver(void *args);
static void *connection_handler_sender(void *args);
static Proto__ServerMessage* handle_request_announce(connection_t *cn);
static void send_message(connection_t *cn, Proto__ServerMessage *response);

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
        pthread_create(&cn->receiver, NULL, connection_handler_receiver, cn);
    }
    {   /* sender */
        pthread_create(&cn->sender, NULL, connection_handler_sender, cn);
    }
    return cn;
}
void cn_destroy(connection_t *cn)
{
    cn->end_connection = 1;
    sck_destroy(cn->sock);
    pthread_join(cn->receiver, NULL);
    pthread_cancel(cn->sender);
    pthread_join(cn->sender, NULL);
    pthread_mutex_destroy(&cn->mtx_queue);
    pthread_cond_destroy(&cn->cond_queue);
    queue_destroy(cn->send_queue);
    free(cn->request);
    cn->request = NULL;
    cn->send_queue = NULL;
}
static void *connection_handler_receiver(void *args)
{
    connection_t *cn = (connection_t*)args;
    uint8_t *buf = NULL;
    size_t num_bytes;
    int ret;
    msg_size_t length;
    msg_size_t allocated_length = 0;
    while (!cn->end_connection) {
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
            UT_NOTIFY(LV_DEBUG, "Receiving message of size %d", length);
            int read = sck_recv(cn->sock, buf, length);
            if (read <= 0) {
                UT_NOTIFY(LV_ERROR, "Receiving request failed");
                goto fail;
            }
            num_bytes += read;
        } while (num_bytes < length);
        cn->request = proto__client_request__unpack(NULL, num_bytes, buf);
        if (!cn->request) {
            UT_NOTIFY(LV_ERROR, "Unpacking request failed");
            goto fail;
        }
        Proto__ServerMessage *response = NULL;
        switch (cn->request->type) {
        case PROTO__CLIENT_REQUEST__TYPE__ANNOUNCE:
            UT_NOTIFY(LV_INFO, "announce received");
            response = handle_request_announce(cn);
            break;
        case PROTO__CLIENT_REQUEST__TYPE__PROCESS_INFO:
            UT_NOTIFY(LV_INFO, "process info request received");
            break;
        case PROTO__CLIENT_REQUEST__TYPE__FUNCTION_CALL:
            UT_NOTIFY(LV_WARN, "current function call requested");
            break;
        case PROTO__CLIENT_REQUEST__TYPE__EXECUTION:
            UT_NOTIFY(LV_INFO, "exec details received");
            response = handle_request_execution(cn);
            break;
        case PROTO__CLIENT_REQUEST__TYPE__DEBUG_COMMAND:
            UT_NOTIFY(LV_INFO, "debug cmd received");
            response = handle_request_debug(cn);
            break;
        default:
            UT_NOTIFY(LV_WARN, "Unknown request received");
        }
        proto__client_request__free_unpacked(cn->request, NULL);

        if(response) {
            cn_send_message(cn, response);
        }
    }
fail:
    UT_NOTIFY(LV_DEBUG, "connection handler quits");
    free(buf);
    cn->close_callback(cn, NORMAL);
    return NULL;
}
static void *connection_handler_sender(void *args)
{
    connection_t *cn = (connection_t*)args;
    UT_NOTIFY(LV_DEBUG, "connection handler quits");
    while(!cn->end_connection) {
        pthread_mutex_lock(&cn->mtx_queue);
        while (queue_empty(cn->send_queue))
            pthread_cond_wait(&cn->cond_queue, &cn->mtx_queue);

        /* unqueue cmd & handle it */
        Proto__ServerMessage *response = (Proto__ServerMessage*)queue_dequeue(cn->send_queue);
        pthread_mutex_unlock(&cn->mtx_queue);
        send_message(cn, response);
        free(response);
    }
    return NULL;
}

static Proto__ServerMessage* handle_request_announce(connection_t *cn)
{
    Proto__ServerMessage *response = malloc(sizeof(Proto__ServerMessage));
    proto__server_message__init(response);
    response->id = cn->request->id;
    Proto__AnnounceRequestDetails *msg = cn->request->announce;
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
static void send_message(connection_t *cn, Proto__ServerMessage *response)
{
    UT_NOTIFY(LV_TRACE, "Sending message");
    msg_size_t len = proto__server_message__get_packed_size(response);
    size_t num_bytes = sck_send(cn->sock, &len, sizeof(msg_size_t));
    assert(num_bytes == sizeof(msg_size_t));
    uint8_t *buffer = malloc(len);
    assert(buffer);
    proto__server_message__pack(response, buffer);
    num_bytes = sck_send(cn->sock, buffer, len);
    assert(num_bytes == len);
    free(buffer);
}
void cn_send_message(connection_t *cn, Proto__ServerMessage *msg)
{
    pthread_mutex_lock(&cn->mtx_queue);
    queue_enqueue(cn->send_queue, msg);
    pthread_mutex_unlock(&cn->mtx_queue);
    pthread_cond_signal(&cn->cond_queue);
}