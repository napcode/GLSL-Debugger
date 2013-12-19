#ifndef UT_NOTIFY_H
#define UT_NOTIFY_H

#include <stdio.h>

typedef enum {
	LV_FATAL = 0,
	LV_ERROR = 1,
	LV_WARN = 2,
	LV_INFO = 3,
	LV_DEBUG = 4,
	LV_TRACE = 5
} severity_t;

/* these are just for convenience */
#define UTILS_NOTIFY_LOGGER(a) utils_notify_logger(a)
#define UTILS_NOTIFY_LEVEL(a) utils_notify_level(a)
#define UTILS_NOTIFY_TO_FILE(a) utils_notify_to_file(a)
#define UTILS_NOTIFY_PATH(a) utils_notify_path(a)
#define UTILS_NOTIFY_STARTUP() utils_notify_startup()
#define UTILS_NOTIFY_SHUTDOWN() utils_notify_shutdown()

#if defined(__cplusplus)

#include <string>
#include <sstream>
#include <iostream>

#define UT_NOTIFY(sev, msg)									\
    do {													\
        std::stringstream buf;								\
        buf << msg;											\
        utils_notify_va(sev, __FILE__, __func__, __LINE__, buf.str().c_str());	\
    }														\
    while(0)

#else   // C
#define UT_NOTIFY(sev, ...)                                 \
            utils_notify_va(sev, __FILE__, __func__, __LINE__, __VA_ARGS__)
#endif  // __cplusplus
#define UT_NOTIFY_VA(sev, ...)                                 \
            utils_notify_va(sev, __FILE__, __func__, __LINE__, __VA_ARGS__)

#if defined(__cplusplus)
extern "C" {
#endif
extern int utils_notify_to_file(const int* value);
extern const char* utils_notify_logger(const char* name);
extern const char* utils_notify_path(const char* path);
extern severity_t utils_notify_level(const severity_t* value);
extern void utils_notify_startup(void);
extern void utils_notify_shutdown(void);
extern void utils_notify_va(const severity_t sev, const char* filename,
		const char* func, unsigned int line, const char *fmt, ...);
#if defined(__cplusplus)
}
#endif
#endif  // UT_NOTIFY_H
