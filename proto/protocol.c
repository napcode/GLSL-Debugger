#include "protocol.h"
#include "build-config.h"
#include "utils/notify.h"
#include "utils/os/os.h"
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

int proto_get_announce(void *buffer, char *client_name)
{
    MsgAnnounce msg = MSG_ANNOUNCE__INIT;
    MsgVersion version = MSG_VERSION__INIT;
    msg.id = PROTO_ID; msg.has_id = 1;
    version.major = PROTO_MAJOR; version.has_major = 1;
    version.minor = PROTO_MINOR; version.has_minor = 1;
    version.revision = PROTO_REVISION; version.has_revision = 1;
    msg.version = &version;
    int len = msg_announce__get_packed_size(&msg);
    assert(MAX_MESSAGE_SIZE > len);
    msg_announce__pack(&msg, buffer);
    return len;
}
int proto_get_announce_response(void *buffer, int accepted, char *msg)
{
    MsgAnnounceResponse response = MSG_ANNOUNCE_RESPONSE__INIT;
    response.accepted = accepted; response.has_accepted = 1;
    response.message = msg;

    int len = msg_announce_response__get_packed_size(&response);
    assert(MAX_MESSAGE_SIZE > len);
    msg_announce_response__pack(&response, buffer);
    return len;
}