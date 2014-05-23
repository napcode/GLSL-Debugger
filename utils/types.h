#ifndef DBG_TYPE_H
#define DBG_TYPE_H

#include <stdlib.h>
#include "proto/protocol.h"

#if defined(__cplusplus)
typedef enum proto::DebugType Proto__DebugType;
extern "C" {
#endif


size_t sizeof_dbg_type(Proto__DebugType d);
void print_dbg_type(Proto__DebugType d, const void* buf);

#if defined(__cplusplus)
}
#endif

#endif