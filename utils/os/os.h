#ifndef GLSLDB_OS_H
#define GLSLDB_OS_H

/* includes */
#ifdef GLSLDB_WIN32
#	include <windows.h>
#endif
#ifdef GLSLDB_LINUX
#	include <sys/wait.h>
#	include <sys/syscall.h>
#	include <dlfcn.h>
#	include <unistd.h>
#endif
#ifdef GLSLDB_OSX

#endif

/* types */
/* don't define them with the namespace */
#ifdef GLSLDB_WIN32
	typedef DWORD os_pid_t;
	typedef DWORD os_tid_t;
	typedef HINSTANCE os_LibraryHandle_t;
#else
	typedef void* os_LibraryHandle_t;
	typedef pid_t os_pid_t;
	typedef pid_t os_tid_t;
#endif

#ifdef __cplusplus
namespace os
{
#endif // __cplusplus

#ifdef GLSLDB_WIN
	/* defines */
	#define OS_PATH_SEPARATOR "\\"
	
	/* functions */
	/* HAZARD BUG OMGWTF This is plain wrong. Use GetCurrentThreadId() */
	#define os_getpid GetCurrentProcessId
	#define os_gettid GetCurrentThreadId
	#define os_dlclose FreeLibrary
	#define os_dlsym GetProcAddress
	#define os_write _write
	#define os_read _read
#else /* GLSLDB_WIN */
	/* defines */
	#define OS_PATH_SEPARATOR "/"

	/* functions */
	#define os_getpid getpid
	#define os_gettid() pthread_self()
	#define os_dlclose dlclose
	#define os_dlsym dlsym
	#define os_write write
	#define os_read read
#endif /* GLSLDB_WIN */


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // GLSLDB_OS_H