#ifndef PROTOCOL_H
#define PROTOCOL_H

#define PROTO_MAJOR 0
#define PROTO_MINOR 0
#define PROTO_REVISION 1

/***************** HEADER *****************/
#define PROTO_ID 0xBEAF
#define PROTO_LITTLE_ENDIAN 0x00
#define PROTO_BIG_ENDIAN 0x01

#if defined(__cplusplus)
#	include "proto/debugger.pb.h"
#else
#	include "proto/debugger.pb-c.h"
#endif

#if defined(__cplusplus)
extern "C" {
#endif

int proto_get_announce(void* buffer, char* client_name);
int proto_get_announce_response(void* buffer, int accepted, char* msg);

#if defined(__cplusplus)
}
#endif

#endif
