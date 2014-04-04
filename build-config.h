#ifndef BUILD_CONFIG_H
#define BUILD_CONFIG_H

#define MAX_NOTIFY_SIZE 512
#define MAX_BACKTRACE_DEPTH 100
#define MEM_ALIGNMENT 8
#define SERVER_PORT 60123

#ifdef GLSLDB_WIN
#define DEBUGLIB "\\debuglib.dll"
#define LIBDLSYM ""
#define DBG_FUNCTIONS_PATH "plugins"
#else
#define DEBUGLIB "/../lib/libglsldebug.so"
#define LIBDLSYM "/../lib/libdlsym.so"
#define DBG_FUNCTIONS_PATH "/../lib/plugins"
#endif

// define for suppressing "defined but not used" warnings
#ifdef __GNUC__
#define UNUSED __attribute__ ((unused))
#else
#define UNUSED
#endif

#endif