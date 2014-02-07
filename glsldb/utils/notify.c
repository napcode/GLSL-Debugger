#include "notify.h"
#include "build-config.h"

#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

static struct
{
    severity_t level;
    int log_to_file;
    FILE *file;
    char filename[256];
} utils_notify_settings = { LV_DEBUG, 0, 0, { 0 } };

void output(const char* msg);

int check_notify_file()
{
    if (utils_notify_settings.file == 0)
    {
        utils_notify_settings.file = fopen(utils_notify_settings.filename,
                                           "a+");
        if (!utils_notify_settings.file)
        {
            utils_notify_settings.log_to_file = 0;
            fprintf(stderr,
                    "Error: unable to open log file %s. File logging disabled.\n",
                    utils_notify_settings.filename);
            return 0;
        }
    }
    return 1;
}
int utils_notify_to_file(const int *value)
{
    if (value)
        utils_notify_settings.log_to_file = *value;
    return utils_notify_settings.log_to_file;
}
const char *utils_notify_filename(const char *filename)
{
    if (filename)
    {
        strncpy(utils_notify_settings.filename, filename, 256);
    }
    return utils_notify_settings.filename;
}
severity_t utils_notify_level(const severity_t *value)
{
    if (value)
        utils_notify_settings.level = *value;
    return utils_notify_settings.level;
}
void utils_notify_startup()
{
    if (utils_notify_settings.log_to_file)
        check_notify_file();
}
void utils_notify_shutdown()
{
    if (utils_notify_settings.file)
    {
        fclose(utils_notify_settings.file);
        utils_notify_settings.file = 0;
    }
}
void utils_notify_va(const severity_t sev, const char *path, const char *func,
                     unsigned int line, unsigned int newline, const char *fmt, ...)
{
    if (sev > utils_notify_settings.level)
        return;

    va_list list;
	static char str_sev[12];
    static char str_time[22];
    static char str_prfx[128];
    static char str_msg[MAX_NOTIFY_SIZE];
    static char out[MAX_NOTIFY_SIZE];

    time_t timeval = time(0);
    struct tm *tp = localtime(&timeval);
    //snprintf(t, 22, "%4d-%02d-%02d %02d:%02d:%02d", 1900 + tp->tm_year,
    //      tp->tm_mon + 1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
    /* FIXME won't work on Windows */
    snprintf(str_time, 22, "%02d:%02d:%02d %d", tp->tm_hour, tp->tm_min, tp->tm_sec, getpid());
    switch (sev)
    {
    case LV_TRACE:
        strcpy(str_sev, " [TRACE]::");
        break;
    case LV_DEBUG:
        strcpy(str_sev, " [DEBUG]::");
        break;
    case LV_INFO:
        strcpy(str_sev, " [INFO ]::");
        break;
    case LV_WARN:
        strcpy(str_sev, " [WARN ]::");
        break;
    case LV_ERROR:
        strcpy(str_sev, " [ERROR]::");
        break;
    case LV_FATAL:
        strcpy(str_sev, " [FATAL]::");
        break;
    default:
        strcpy(str_sev, " [DEF  ]::");
        break;
    }

#ifdef GLSLDB_DEBUG
    const char *filename = strrchr(path, '/');
    if(!filename)
        filename = path;
    else
        filename += 1;
    snprintf(str_prfx, 128, "%s%s%s:%s()::%d: ", str_time, str_sev, filename, func, line);
#else
    snprintf(str_prfx, 128, "%s%s", str_time, str_sev);
#endif
    strncpy(str_msg, str_prfx, MAX_NOTIFY_SIZE);
    va_start(list, fmt);
    vsnprintf(str_msg, MAX_NOTIFY_SIZE, fmt, list);
    va_end(list);
    const char* out_fmt = "%s%s\n";
    if(!newline)
        out_fmt = "%s%s";

    snprintf(out, MAX_NOTIFY_SIZE, out_fmt, str_prfx, str_msg);
    output(out);
}

void utils_notify_out(const char* fmt, ...)
{
    va_list list;
    static char msg[MAX_NOTIFY_SIZE];
    va_start(list, fmt);
    vsnprintf(msg, MAX_NOTIFY_SIZE, fmt, list);
    va_end(list);
    output(msg);
}
void output(const char* msg)
{
#ifdef GLSLDB_WINDOWS
    OutputDebugStringA(msg);
#else
    if (utils_notify_settings.log_to_file && check_notify_file())
        fprintf(utils_notify_settings.file, msg);
    else
        fprintf(stdout, msg);
#endif

}
