#include "notify.h"
#include "build-config.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define SIZE_PATH 512
#define SIZE_FILENAME 128
#define SIZE_LOGGERNAME 32

static struct {
	severity_t level;
	int log_to_file;
	FILE *file;
	char path[SIZE_PATH];
	char filename[SIZE_FILENAME];
	char logger[SIZE_LOGGERNAME];

} utils_notify_settings = { 
	LV_DEBUG, 
	0, 
	0, 
	{ "." },
	{ 0 },
	{ 0 }
	};

char *sev_strings[] = {
	"[FATAL]", 
	"[ERROR]", 
	"[WARN ]", 
	"[INFO ]", 
	"[DEBUG]", 
	"[TRACE]" 
};

const char* get_time_string(void) 
{
	static char t[30];
	time_t timeval = time(0);
	struct tm *tp = localtime(&timeval);
	snprintf(t, 30, "%4d-%02d-%02d %02d:%02d:%02d", 
			1900 + tp->tm_year,
			tp->tm_mon + 1, 
			tp->tm_mday, 
			tp->tm_hour, 
			tp->tm_min, 
			tp->tm_sec);
	return t;
}

int check_notify_file(void)
{
	if (utils_notify_settings.file == 0) {
		snprintf(utils_notify_settings.filename, SIZE_FILENAME, "%s_%jd.log", utils_notify_settings.logger, (intmax_t)time(NULL));
		size_t len = strlen(utils_notify_settings.path) + strlen(utils_notify_settings.filename) + 2;
		char *tmp = malloc(len);
		assert(tmp);
		int ret = snprintf(tmp, len, "%s/%s", utils_notify_settings.path, utils_notify_settings.filename);
		if(ret == -1)
			return 0;
		utils_notify_settings.file = fopen(tmp, "w+");
		free(tmp);
		if (!utils_notify_settings.file) {
			utils_notify_settings.log_to_file = 0;
			fprintf(stderr,
					"Error: unable to open log file %s. File logging disabled.\n",
					utils_notify_settings.filename);
			return 0;
		}
	}
	return 1;
}
int utils_notify_to_file(const int* value)
{
	if (value)
		utils_notify_settings.log_to_file = *value;
	return utils_notify_settings.log_to_file;
}
const char* utils_notify_path(const char* path)
{
	if (path) {
		strncpy(utils_notify_settings.path, path, SIZE_PATH);
	}
	return utils_notify_settings.path;
}
const char* utils_notify_logger(const char* name)
{
	if (name) {
		strncpy(utils_notify_settings.logger, name, SIZE_LOGGERNAME);
	}
	return utils_notify_settings.logger;
}
severity_t utils_notify_level(const severity_t* value)
{
	if (value) 
		utils_notify_settings.level = *value;
	return utils_notify_settings.level;
}
void utils_notify_startup(void)
{
	if(utils_notify_settings.logger[0] == '\0')
		strncpy(utils_notify_settings.logger, "DEF", SIZE_LOGGERNAME);
	if (utils_notify_settings.log_to_file)
		check_notify_file();
}
void utils_notify_shutdown(void)
{
	if (utils_notify_settings.file) {
		fclose(utils_notify_settings.file);
		utils_notify_settings.file = 0;
	}
}
void utils_notify_va(const severity_t sev, const char* path, const char* func,
		unsigned int line, const char *fmt, ...)
{
	if (sev > utils_notify_settings.level || sev > LV_TRACE)
		return;

	va_list list;
	static char prefix[512];
	static char msg[MAX_NOTIFY_SIZE];
	const char* sev_str = sev_strings[sev];
	const char* time_str = get_time_string();

	char *filename = strrchr(path, '/');
	if(utils_notify_settings.log_to_file) {
		snprintf(prefix, 512, "%s::%s::%s:%s()::%d: ", time_str, sev_str, filename + 1, func, line);
	} 
	else {
		snprintf(prefix, 512, "%s::%s::%s::%s:%s()::%d: ", utils_notify_settings.logger, time_str, sev_str, filename + 1, func, line);
	}

	va_start(list, fmt);
	vsnprintf(msg, MAX_NOTIFY_SIZE, fmt, list);
	va_end(list);

#if defined GLSLDB_WINDOWS
	OutputDebugStringA(prefix);
	OutputDebugStringA(msg);
#else
	if (utils_notify_settings.log_to_file && utils_notify_settings.file)
		fprintf(utils_notify_settings.file, "%s%s\n", prefix, msg);
	else
		fprintf(stdout, "%s%s\n", prefix, msg);
#endif
}
