/******************************************************************************

Copyright (C) 2006-2009 Institute for Visualization and Interactive Systems
(VIS), Universität Stuttgart.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
	list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice, this
    list of conditions and the following disclaimer in the documentation and/or
    other materials provided with the distribution.

  * Neither the name of the name of VIS, Universität Stuttgart nor the names
    of its contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include <stdlib.h>
#ifndef _WIN32
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#endif /* _WIN32 */
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>

#ifdef _WIN32
//#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <crtdbg.h>
#include <io.h>
#include <direct.h>
#define GL_GLEXT_PROTOTYPES 1
#define WGL_WGLEXT_PROTOTYPES 1
#else /* _WIN32 */
#include <dirent.h>
#endif /* _WIN32 */

#include "GL/gl.h"
//#include "../GL/glext.h"
#ifndef _WIN32
//#include "../GL/glx.h"
#else /* _WIN32 */

#include "../GL/WinGDI.h"
#include "../GL/wglext.h"
#include "generated/trampolines.h"
#endif /* !_WIN32 */

#include "utils/dlutils.h"
#include "utils/hash.h"
#include "utils/notify.h"
#include "utils/types.h"
#include "build-config.h"
#include "utils/glenumerants.h"
#include "debuglib.h"
#include "debuglibInternal.h"
#include "glstate.h"
#include "readback.h"
#include "streamRecorder.h"
#include "streamRecording.h"
#include "memory.h"
#include "shader.h"
#include "initLib.h"
#include "queries.h"
#include "utils/socket.h"
#include "utils/connection.h"
#include "utils/server.h"
#include "utils/queue.h"
#include "proto/protocol.h"

#ifdef _WIN32
#  define LIBGL "opengl32.dll"
#  define SO_EXTENSION ".dll"
#	define SHMEM_NAME_LEN 64
#else
#  define LIBGL "libGL.so"
#  define SO_EXTENSION ".so"
#endif


#define USE_DLSYM_HARDCODED_LIB

typedef struct {
    os_LibraryHandle_t handle;
    const char *fname;
    void (*function)(void);
} debug_function_t;


/* TODO: threads! Should be local to each thread, isn't it? */
#ifndef _WIN32
static struct {
    int initialized;
#ifdef USE_DLSYM_HARDCODED_LIB
    os_LibraryHandle_t libgl;
#endif
    void *(*origdlsym)(void *, const char *);

    thread_state_t *thread;
    debug_function_t *dbgFunctions;
    int numDbgFunctions;
    Hash origFunctions;
    socket_t *socket;
} g = {
    0, /* initialized */
#ifdef USE_DLSYM_HARDCODED_LIB
    NULL, /* libgl */
#endif
    NULL, /* origdlsym */
    NULL, /* fcalls */
    NULL, /* dbgFunctions */
    0, /* numDbgFunctions */
    {
        0,
        NULL,
        NULL,
        NULL
    },  /* origFunctions */
    NULL
};
#else /* _WIN32 */
static struct {
    HANDLE hEvtDebugee; /* wait for debugger */
    HANDLE hEvtDebugger; /* signal debugger */
    HANDLE hShMem; /* shared memory handle */
    thread_state_t *thread;
    debug_function_t *dbgFunctions;
    int numDbgFunctions;
} g = {NULL, NULL, NULL, NULL, NULL, 0};
#endif /* _WIN32 */


#ifdef _WIN32
#	define GET_MODULE(h, s) (char *)GetProcAddress(h, s)
#else /* _WIN32 */
#	define GET_MODULE(h, s) g.origdlsym(h, s)
#endif /* _WIN32 */



/* global data */
DBGLIBLOCAL Globals G;

typedef struct {
    socket_t *sock;
    int end_connection;
    Proto__ClientRequest *request;
    Proto__ServerResponse *response;
} handlerstate_t;
#define HANDLERSTATE_INIT {NULL, 0, NULL, NULL}

extern Proto__GLFunction glFunctions[];

/* internal functions */
static int new_connection_callback(socket_t *s);
static void handle_request_announce(handlerstate_t *state);
static void handle_request_execution(handlerstate_t *state);
static void send_response(handlerstate_t *state);
static Proto__FunctionCall *fcall_create(const char *funcname, int num_args);
static void fcall_destroy(Proto__FunctionCall *pc);

#ifndef _WIN32
static int getShmid()
{
    char *s = getenv("GLSL_DEBUGGER_SHMID");

    if (s) {
        return atoi(s);
    } else {
        UT_NOTIFY(LV_ERROR, "No Shmid! Set GLSL_DEBUGGER_SHMID!\n");
        exit(1);
    }
}
#endif /* !_WIN32 */

static void setLogging(void)
{
    int level;

#ifndef _WIN32
    char *s;

    s = getenv("GLSL_DEBUGGER_LOGDIR");
    if (s) {
        int i = 1;
        utils_notify_to_file(&i);
        utils_notify_filename(s);
    } else {
#else /* !_WIN32 */
    char s[MAX_PATH];

    if (GetEnvironmentVariableA("GLSL_DEBUGGER_LOGDIR", s,
                                MAX_PATH)) {
        s[MAX_PATH - 1] = '\0'; /* just to be sure ... */
        utils_notify_filename(s);
    } else {
#endif /* !_WIN32 */
        utils_notify_filename(NULL);
    }

    UTILS_NOTIFY_STARTUP();
#ifndef _WIN32
    s = getenv("GLSL_DEBUGGER_LOGLEVEL");
    if (s) {
#else /* !_WIN32 */
    if (GetEnvironmentVariableA("GLSL_DEBUGGER_LOGLEVEL", s,
                                MAX_PATH)) {
        s[MAX_PATH - 1] = '\0'; /* just to be sure ... */
#endif /* !_WIN32 */
        level = atoi(s);
        /* FIXME that's not enirely correct */
        UTILS_NOTIFY_LEVEL((severity_t *)&level);
        UT_NOTIFY(LV_INFO, "Log level set to %s", utils_notify_strlevel((severity_t)level));
    } else {
        severity_t t = LV_DEBUG;
        UTILS_NOTIFY_LEVEL(&t);
        UT_NOTIFY(LV_WARN, "Log level not set!");
    }
}

static void addDbgFunction(const char *soFile)
{
	os_LibraryHandle_t handle = NULL;
	void (*dbgFunc)(void) = NULL;
	const char *provides = NULL;

	if (!(handle = openLibrary(soFile))) {
		UT_NOTIFY(LV_WARN, "Opening dbgPlugin \"%s\" failed\n", soFile);
		return;
	}
	if (!(provides = GET_MODULE(handle, "provides"))) {
		UT_NOTIFY(LV_WARN, "Could not determine what \"%s\" provides!\n"
		"Export the " "\"provides\"-string!\n", soFile);
		os_dlclose(handle);
		return;
	}

	if (!(dbgFunc = (void (*)(void)) GET_MODULE(handle, provides))) {
		os_dlclose(handle);
		return;
	}
	g.numDbgFunctions++;
	g.dbgFunctions = realloc(g.dbgFunctions,
			g.numDbgFunctions * sizeof(debug_function_t));
	if (!g.dbgFunctions) {
		UT_NOTIFY(LV_ERROR,
				"Allocating g.dbgFunctions failed: %s (%d)\n", strerror(errno), g.numDbgFunctions*sizeof(debug_function_t));
		os_dlclose(handle);
		exit(1);
	}
	g.dbgFunctions[g.numDbgFunctions - 1].handle = handle;
	g.dbgFunctions[g.numDbgFunctions - 1].fname = provides;
	g.dbgFunctions[g.numDbgFunctions - 1].function = dbgFunc;
}

static void freeDbgFunctions()
{
    int i;

    for (i = 0; i < g.numDbgFunctions; i++) {
        if (g.dbgFunctions[i].handle != NULL) {
            os_dlclose(g.dbgFunctions[i].handle);
            g.dbgFunctions[i].handle = NULL;
        }
    }
}

static int endsWith(const char *s, const char *t)
{
    return strlen(t) < strlen(s) && !strcmp(s + strlen(s) - strlen(t), t);
}

static void loadDbgFunctions(void)
{
    char *file;
#if !defined WIN32
    struct dirent *entry;
    struct stat statbuf;
    DIR *dp;
    char *dbgFctsPath = NULL;
#else
    struct _finddata_t fd;
    char *files, *cwd;
    intptr_t handle;
    char dbgFctsPath[MAX_PATH];
#endif

#ifndef _WIN32
    dbgFctsPath = getenv("GLSL_DEBUGGER_DBGFCTNS_PATH");
#else /* !_WIN32 */
    /*
     * getenv is less cool than GetEnvironmentVariableA because it ignores our
     * great CreateRemoteThread efforts ...
     */
    if (GetEnvironmentVariableA("GLSL_DEBUGGER_DBGFCTNS_PATH", dbgFctsPath,
                                MAX_PATH)) {
        dbgFctsPath[MAX_PATH - 1] = 0; /* just to be sure ... */
    } else {
        *dbgFctsPath = 0;
    }
#endif /* !_WIN32 */

    if (!dbgFctsPath || dbgFctsPath[0] == '\0') {
        UT_NOTIFY(LV_ERROR,
                  "No dbgFctsPath! Set GLSL_DEBUGGER_DBGFCTNS_PATH!\n");
        exit(1);
    }

#if ! defined WIN32
    if ((dp = opendir(dbgFctsPath)) == NULL) {
        UT_NOTIFY(LV_ERROR,
                  "cannot open so directory \"%s\"\n", dbgFctsPath);
        exit(1);
    }

    while ((entry = readdir(dp))) {
        if (endsWith(entry->d_name, SO_EXTENSION)) {
            if (!(file = (char *) malloc(
                             strlen(dbgFctsPath) + strlen(entry->d_name) + 2))) {
                UT_NOTIFY(LV_ERROR, "not enough memory for file template\n");
                exit(1);
            }
            strcpy(file, dbgFctsPath);
            if (dbgFctsPath[strlen(dbgFctsPath) - 1] != '/') {
                strcat(file, "/");
            }
            strcat(file, entry->d_name);
            stat(file, &statbuf);
            if (S_ISREG(statbuf.st_mode)) {
                addDbgFunction(file);
            }
            free(file);
        }
    }
    closedir(dp);
#else
    if (! (files = (char *)malloc(strlen(dbgFctsPath) + 3 + strlen(SO_EXTENSION)))) {
        UT_NOTIFY(LV_ERROR, "not enough memory for file template\n");
        exit(1);
    }
    if (!(cwd = _getcwd(NULL, 512))) {
        UT_NOTIFY(LV_ERROR, "Failed to get current working directory\n");
        exit(1);
    }
    if (_chdir(dbgFctsPath) != 0) {
        UT_NOTIFY(LV_ERROR, "directory '%s' not found\n", dbgFctsPath);
        exit(1);
    }
    strcpy(files, ".\\*.*");
    if ((handle = _findfirst(files, &fd)) == -1) {
        UT_NOTIFY(LV_WARNING, "no dbg functions found in %s\n", files);
        return;
    }
    /* restore working directory */
    if (_chdir(cwd) != 0) {
        UT_NOTIFY(LV_ERROR, "Failed to restore working directory\n");
        exit(1);
    }
    free(cwd);
    free(files);

    do {
        if (endsWith(fd.name, SO_EXTENSION)) {
            if (! (file = (char *)malloc(strlen(dbgFctsPath) + strlen(fd.name) + 2))) {
                UT_NOTIFY(LV_ERROR, "not enough memory for file template\n");
                exit(1);
            }
            strcpy(file, dbgFctsPath);
            if (dbgFctsPath[strlen(dbgFctsPath) - 1] != '\\') {
                strcat(file, "\\");
            }
            strcat(file, fd.name);
            if ((fd.attrib & _A_HIDDEN) != _A_HIDDEN) {
                addDbgFunction(file);
            }
            free(file);
        }
    } while (! _findnext(handle, &fd));
#endif
}

#ifdef _WIN32
/* mueller: I will need this function for a detach hack. */
__declspec(dllexport) BOOL __cdecl uninitialiseDll(void)
{
    BOOL retval = TRUE;

    EnterCriticalSection(&G.lock);

    freeDbgFunctions();

    clearRecordedCalls(&G.recordedStream);

    cleanupQueryStateTracker();

    /* We must detach first, as trampolines use events. */
    if (detachTrampolines()) {
        UT_NOTIFY(LV_INFO, "Trampolines detached.\n");
    } else {
        UT_NOTIFY(LV_WARN, "Detaching trampolines failed.\n");
        retval = FALSE;
    }

	if (!closeEvents(&g.hEvtDebugee, &g.hEvtDebugger)) {
		retval = FALSE;
	}

	if (!closeSharedMemory(&g.hShMem, &g.fcalls)) {
		retval = FALSE;
	}

    UTILS_NOTIFY_SHUTDOWN();

    LeaveCriticalSection(&G.lock);
    return retval;
}

BOOL APIENTRY DllMain(HANDLE hModule,
                      DWORD reason_for_call,
                      LPVOID lpReserved)
{
<<<<<<< HEAD
	BOOL retval = TRUE;
	switch (reason_for_call) {
		case DLL_PROCESS_ATTACH:
		setLogging();
=======
    BOOL retval = TRUE;
    //GlInitContext initCtx;
    //   thread_state_t *rec = NULL;

    switch (reason_for_call) {
    case DLL_PROCESS_ATTACH:

        setLogging();
>>>>>>> basic communication should work

#ifdef DEBUG
		//AllocConsole();     /* Force availability of console in debug mode. */
		UT_NOTIFY(LV_DEBUG, "I am in Debug mode.\n");

//		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_WNDW);
//		if (_CrtDbgReport(_CRT_ERROR, __FILE__, __LINE__, "", "This is the "
//			  "breakpoint crowbar in DllMain. You should attach to the "
//			  "debugged process before continuing.")) {
//			_CrtDbgBreak();
//		}
#endif /* DEBUG */
		/* Open synchronisation events. */

#ifdef GLSLDEBUGLIB_HOST
		closeEvents(&g.hEvtDebugee, &g.hEvtDebugger);
		if (!createEvents(&g.hEvtDebugee, &g.hEvtDebugger))
			return FALSE;
#else
		if (!openEvents(&g.hEvtDebugee, &g.hEvtDebugger))
			return FALSE;
#endif

<<<<<<< HEAD
		dbgPrint(DBGLVL_DEBUG, "Events opened.\n");
=======
        // TODO: This is part of the extension detours initialisation
        // (replacing) current lazy initialisation. However, I think this
        // is even unsafer than the current solution.
        //* HAZARD BUG OMGWTF This is plain wrong. Use GetCurrentThreadId() */
        //rec = getThreadState(GetCurrentProcessId());
        //rec->isRecursing = 1;
        //if (!releaseGlInitContext(&initCtx)) {
        //  return FALSE;
        //}
        //rec->isRecursing = 0;

        G.errorCheckAllowed = 1;
        initStreamRecorder(&G.recordedStream);

        initQueryStateTracker();

        /* __asm int 3 FTW! */
        //__asm int 3
        /*
         * HAZARD: This is dangerous to public safety, we must remove it.
         * MSDN says "It [DllMain] must not call the LoadLibrary or
         * LoadLibraryEx function (or a function that calls  these functions),
         * ..."
         */
        loadDbgFunctions();
>>>>>>> basic communication should work


		/* Create global crit section. */
		InitializeCriticalSection(&G.lock);

		if (!attachTrampolines())
			return FALSE;
	    UT_NOTIFY(LV_INFO, "Trampolines attached");

		/* Attach to shared mem segment */
#ifdef GLSLDEBUGLIB_HOST
		closeSharedMemory(&g.hShMem, &g.fcalls);
		if (!initSharedMemory(&g.hShMem, &g.fcalls, SHM_SIZE))
			return FALSE;
#else
		if (!openSharedMemory(&g.hShMem, &g.fcalls, SHM_SIZE))
			return FALSE;
#endif


		// TODO: This is part of the extension detours initialisation
		// (replacing) current lazy initialisation. However, I think this
		// is even unsafer than the current solution.
		//* HAZARD BUG OMGWTF This is plain wrong. Use GetCurrentThreadId() */
		//rec = getThreadRecord(GetCurrentProcessId());
		//rec->isRecursing = 1;
		//if (!releaseGlInitContext(&initCtx)) {
		//	return FALSE;
		//}
		//rec->isRecursing = 0;

		G.errorCheckAllowed = 1;
		initStreamRecorder(&G.recordedStream);

		initQueryStateTracker();

		/* __asm int 3 FTW! */
		//__asm int 3
		/*
		 * HAZARD: This is dangerous to public safety, we must remove it.
		 * MSDN says "It [DllMain] must not call the LoadLibrary or
		 * LoadLibraryEx function (or a function that calls  these functions),
		 * ..."
		 */
		//loadDbgFunctions();

		//g.initialized = 1;

		/*
		 * We do not want to do anything if a thread attaches, so tell
		 * Windows not annoy us with these notifications in the future.
		 */
		DisableThreadLibraryCalls(hModule);
		break;

		case DLL_THREAD_ATTACH:
		break;

		case DLL_THREAD_DETACH:
		break;

		case DLL_PROCESS_DETACH:
		UT_NOTIFY(LV_INFO, "DLL_PROCESS_DETACH");
		EnterCriticalSection(&G.lock);
		retval = uninitialiseDll();
		DeleteCriticalSection(&G.lock);
		break;
	}

	return retval;
}
#else

void __attribute__ ((constructor)) debuglib_init(void)
{
#ifndef RTLD_DEEPBIND
    g.origdlsym = dlsym;
#endif

    setLogging();

#ifdef USE_DLSYM_HARDCODED_LIB
    if (!(g.libgl = openLibrary(LIBGL))) {
        UT_NOTIFY(LV_ERROR, "Error opening OpenGL library");
        exit(1);
    }
#endif

    UT_NOTIFY(LV_INFO, "Executable: %s", program_invocation_name);

    // create debugger listener
    {
        char *comm = getenv("GLSL_DEBUGGER_COMM");
        char *strport = getenv("GLSL_DEBUGGER_PORT");
        if (comm && strcmp(comm, "tcp") == 0) {
            int port = (strport == NULL) ? DEFAULT_SERVER_PORT_TCP : atoi(strport);
            cn_acceptor_start_tcp(port, new_connection_callback);
            UT_NOTIFY(LV_INFO, "Server uses TCP and port %d", port);
        } else if (comm && strcmp(comm, "unix") == 0) {
            char *port = (strport == NULL) ? DEFAULT_SERVER_PORT_UNIX : strport;
            cn_acceptor_start_unix(port, new_connection_callback);
            UT_NOTIFY(LV_INFO, "Server uses Unix domain sockets. Socket is %s", port);
        } else {
            cn_acceptor_start_tcp(DEFAULT_SERVER_PORT_TCP, new_connection_callback);
            UT_NOTIFY(LV_INFO, "TCP Server started on port %d", DEFAULT_SERVER_PORT_TCP);
        }
    }

    /* attach to shared mem segment */
    //g.fcalls = shmat(getShmid(), NULL, 0);
    //if ((long)g.fcalls == -1) {
    //  UT_NOTIFY(LV_ERROR, "Could not attach to shared memory segment: %s\n", strerror(errno));
    //  exit(1);
    //}
    // for now:
    g.thread = malloc(MAX_THREADS * sizeof(thread_state_t));
    memset(g.thread, 0, MAX_THREADS * sizeof(thread_state_t));
    thread_state_t *this = getThreadState(os_getpid());
    if (getenv("GLSL_DEBUGGER_INTERACTIVE")) {
        this->mode = EX_MODE_INTERACTIVE;
        this->halt_on = EX_HALT_ALL;
        UT_NOTIFY(LV_INFO, "Library starts in interactive mode");
    } else {
        this->mode = EX_MODE_UNATTENDED;
        UT_NOTIFY(LV_INFO, "Library starts in unattended mode");
    }

    pthread_mutex_init(&G.lock, NULL);
    pthread_cond_init(&G.cond, NULL);

    G.cmdqueue = queue_create();

    hash_create(&g.origFunctions, hashString, compString, 512, 0);

    initQueryStateTracker();

#ifdef USE_DLSYM_HARDCODED_LIB
    /* paranoia mode: ensure that g.origdlsym is initialized */
    dlsym(g.libgl, "glFinish");

    G.origGlXGetProcAddress = (void (*(*)(const GLubyte *))(void)) g.origdlsym(
                                  g.libgl, "glXGetProcAddress");
    if (!G.origGlXGetProcAddress) {
        G.origGlXGetProcAddress =
            (void (*(*)(const GLubyte *))(void)) g.origdlsym(g.libgl,
                    "glXGetProcAddressARB");
        if (!G.origGlXGetProcAddress) {
            UT_NOTIFY(LV_ERROR, "Hmm, cannot resolve glXGetProcAddress");
            exit(1);
        }
    }
#else
    /* paranoia mode: ensure that g.origdlsym is initialized */
    dlsym(RTLD_NEXT, "glFinish");

    G.origGlXGetProcAddress = g.origdlsym(RTLD_NEXT, "glXGetProcAddress");
    if (!G.origGlXGetProcAddress) {
        G.origGlXGetProcAddress = g.origdlsym(RTLD_NEXT, "glXGetProcAddressARB");
        if (!G.origGlXGetProcAddress) {
            UT_NOTIFY(LV_ERROR, "Hmm, cannot resolve glXGetProcAddress");
            exit(1);
        }
    }
#endif

    G.errorCheckAllowed = 1;

    initStreamRecorder(&G.recordedStream);

    loadDbgFunctions();

    g.initialized = 1;
}

void __attribute__ ((destructor)) debuglib_fini(void)
{
    /* detach shared mem segment */
    //shmdt(g.fcalls);
    cn_acceptor_stop();

#ifdef USE_DLSYM_HARDCODED_LIB
    if (g.libgl) {
        os_dlclose(g.libgl);
    }
#endif

    freeDbgFunctions();

    hash_free(&g.origFunctions);

    cleanupQueryStateTracker();

    clearRecordedCalls(&G.recordedStream);

    UTILS_NOTIFY_SHUTDOWN();

    pthread_mutex_destroy(&G.lock);

    free(g.thread);
}
#endif

thread_state_t *getThreadState(os_pid_t pid)
{
    int i;
    for (i = 0; i < MAX_THREADS; ++i) {
        if (g.thread[i].threadId == 0 || g.thread[i].threadId == pid) {
            break;
        }
    }
    if (i == MAX_THREADS) {
        /* TODO */
        UT_NOTIFY(LV_ERROR, "Error: max. number of debuggable threads exceeded!");
        exit(1);
    }
    if (g.thread[i].threadId == 0)
        g.thread[i].threadId = pid;
    return &g.thread[i];
}

static void printArgument(void *addr, int type)
{
    char *s;

    switch (type) {
    case DBG_TYPE_CHAR:
        UT_NOTIFY_NO_PRFX("%i", *(char *)addr);
        break;
    case DBG_TYPE_UNSIGNED_CHAR:
        UT_NOTIFY_NO_PRFX("%i", *(unsigned char *)addr);
        break;
    case DBG_TYPE_SHORT_INT:
        UT_NOTIFY_NO_PRFX("%i", *(short *)addr);
        break;
    case DBG_TYPE_UNSIGNED_SHORT_INT:
        UT_NOTIFY_NO_PRFX("%i", *(unsigned short *)addr);
        break;
    case DBG_TYPE_INT:
        UT_NOTIFY_NO_PRFX("%i", *(int *)addr);
        break;
    case DBG_TYPE_UNSIGNED_INT:
        UT_NOTIFY_NO_PRFX("%u", *(unsigned int *)addr);
        break;
    case DBG_TYPE_LONG_INT:
        UT_NOTIFY_NO_PRFX("%li", *(long *)addr);
        break;
    case DBG_TYPE_UNSIGNED_LONG_INT:
        UT_NOTIFY_NO_PRFX("%lu", *(unsigned long *)addr);
        break;
    case DBG_TYPE_LONG_LONG_INT:
        UT_NOTIFY_NO_PRFX("%lli", *(long long *)addr);
        break;
    case DBG_TYPE_UNSIGNED_LONG_LONG_INT:
        UT_NOTIFY_NO_PRFX("%llu", *(unsigned long long *)addr);
        break;
    case DBG_TYPE_FLOAT:
        UT_NOTIFY_NO_PRFX("%f", *(float *)addr);
        break;
    case DBG_TYPE_DOUBLE:
        UT_NOTIFY_NO_PRFX("%f", *(double *)addr);
        break;
    case DBG_TYPE_POINTER:
        UT_NOTIFY_NO_PRFX("%p", *(void **)addr);
        break;
    case DBG_TYPE_BOOLEAN:
        UT_NOTIFY_NO_PRFX("%s", *(GLboolean *)addr ? "TRUE" : "FALSE");
        break;
    case DBG_TYPE_BITFIELD:
        s = dissectBitfield(*(GLbitfield *) addr);
        UT_NOTIFY_NO_PRFX("%s", s);
        free(s);
        break;
    case DBG_TYPE_ENUM:
        UT_NOTIFY_NO_PRFX("%s", lookupEnum(*(GLenum *)addr));
        break;
    case DBG_TYPE_STRUCT:
        UT_NOTIFY_NO_PRFX("STRUCT");
        break;
    default:
        UT_NOTIFY_NO_PRFX("UNKNOWN TYPE [%i]", type);
    }
}

void storeFunctionCall(const char *fname, int numArgs, ...)
{
    int i;
    va_list argp;
    os_pid_t pid = os_getpid();
    thread_state_t *rec = getThreadState(pid);
    fcall_destroy(rec->current_call);
    rec->current_call = fcall_create(fname, numArgs);
    rec->current_call->thread_id = pid; rec->current_call->has_thread_id = 1;
    UT_NOTIFY_NL(LV_INFO, "STORE CALL %s(", fname);
    va_start(argp, numArgs);
    for (i = 0; i < numArgs;) {
        rec->current_call->arguments[i]->data = va_arg(argp, void *);
        rec->current_call->arguments[i]->type = va_arg(argp, int);

        //rec->items[2 * i] = (ALIGNED_DATA) va_arg(argp, void *);
        //rec->items[2 * i + 1] = (ALIGNED_DATA) va_arg(argp, int);
        printArgument((void *) rec->current_call->arguments[i]->data,
                      rec->current_call->arguments[i]->type);
        ++i;
        if (i != numArgs)
            UT_NOTIFY_NO_PRFX(", ");
    }
    va_end(argp);
    UT_NOTIFY_NO_PRFX(")\n");
}

void storeResult(void *result, int type)
{
    os_pid_t pid = os_getpid();
    thread_state_t *rec = getThreadState(pid);

    UT_NOTIFY_NL(LV_INFO, "STORE RESULT: ");
    printArgument(result, type);
    UT_NOTIFY_NO_PRFX("\n");
    rec->current_call->return_type = type;
    rec->current_call->has_return_type = 1;
    rec->current_call->return_data = result;
}


/* FIXME noone actually calls this anymore
void storeResultOrError(unsigned int error, void *result, int type)
{
    os_pid_t pid = os_getpid();
    thread_state_t *rec = getThreadState(pid);

    if (error) {
        setErrorCode(error);
        UT_NOTIFY(LV_WARN, "NO RESULT STORED: %u", error);
    } else {
        UT_NOTIFY_NL(LV_INFO, "STORE RESULT: ");
        printArgument(result, type);
        UT_NOTIFY_NO_PRFX("\n");
        rec->result = DBG_RETURN_VALUE;
        rec->items[0] = (ALIGNED_DATA) result;
        rec->items[1] = (ALIGNED_DATA) type;
    }
}
*/

void stop(void)
{
    pthread_mutex_lock(&G.lock);
    while (queue_empty(G.cmdqueue))
        pthread_cond_wait(&G.cond, &G.lock);
    pthread_mutex_unlock(&G.lock);
}

static void startRecording(void)
{
    DMARK
    clearRecordedCalls(&G.recordedStream);
    setErrorCode(DBG_NO_ERROR);
}

static void replayRecording(int target)
{
    int error;
    DMARK
    error = setSavedGLState(target);
    if (error) {
        setErrorCode(error);
    }
    replayFunctionCalls(&G.recordedStream, 0);
    setErrorCode(glError());
}

static void endReplay(void)
{
    DMARK
    replayFunctionCalls(&G.recordedStream, 1);
    clearRecordedCalls(&G.recordedStream);
    setErrorCode(glError());
}

/**
 *	Does all operations necessary to get the result of a given debug shader
 *	back to the caller, i.e. setup the shader and its environment, replay the
 *	draw call and readback the result.
 *	Parameters:
 *		items[0] : pointer to vertex shader src
 *		items[1] : pointer to geometry shader src
 *		items[2] : pointer to fragment shader src
 *		items[3] : debug target, see DBG_TARGETS below
 *		if target == DBG_TARGET_FRAGMENT_SHADER:
 *			items[4] : number of components to read (1:R, 3:RGB, 4:RGBA)
 *			items[5] : format of readback (GL_FLOAT, GL_INT, GL_UINT)
 *		if target == DBG_TARGET_VERTEX_SHADER or DBG_TARGET_GEOMETRY_SHADER:
 *			items[4] : primitive mode
 *			items[5] : force primitive mode even for geometry shader target
 *			items[6] : expected size of debugResult (# floats) per vertex
 *	Returns:
 *		if target == DBG_TARGET_FRAGMENT_SHADER:
 *			result   : DBG_READBACK_RESULT_FRAGMENT_DATA or DBG_ERROR_CODE
 *					   on error
 *			items[0] : buffer address
 *			items[1] : image width
 *			items[2] : image height
 *		if target == DBG_TARGET_VERTEX_SHADER or DBG_TARGET_GEOMETRY_SHADER:
 *			result   : DBG_READBACK_RESULT_VERTEX_DATA or DBG_ERROR_CODE on
 *					   error
 *			items[0] : buffer address
 *			items[1] : number of vertices
 *			items[2] : number of primitives
 */
static void shaderStep(void)
{
    // int error;

    // os_pid_t pid = os_getpid();
    // thread_state_t *rec = getThreadState(pid);
    // const char *vshader = (const char *) rec->items[0];
    // const char *gshader = (const char *) rec->items[1];
    // const char *fshader = (const char *) rec->items[2];
    // int target = (int) rec->items[3];

    // UT_NOTIFY(LV_INFO,
    //           "SHADER STEP: v=%p g=%p f=%p target=%i", vshader, gshader, fshader, target);

    // UT_NOTIFY(LV_INFO, "############# V-Shader ##############\n%s\n"
    //           "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$", vshader);
    // UT_NOTIFY(LV_INFO, "############# G-Shader ##############\n%s\n"
    //           "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$", gshader);
    // UT_NOTIFY(LV_INFO, "############# F-Shader ##############\n%s\n"
    //           "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$", fshader);

    // if (target == DBG_TARGET_GEOMETRY_SHADER
    //         || target == DBG_TARGET_VERTEX_SHADER) {
    //     int primitiveMode = (int) rec->items[4];
    //     int forcePointPrimitiveMode = (int) rec->items[5];
    //     int numFloatsPerVertex = (int) rec->items[6];
    //     int numVertices;
    //     int numPrimitives;
    //     float *buffer;

    //     /* set debug shader code */
    //     error = loadDbgShader(vshader, gshader, fshader, target,
    //                           forcePointPrimitiveMode);
    //     if (error) {
    //         setErrorCode(error);
    //         return;
    //     }

    //     /* replay recorded drawcall */
    //     error = setSavedGLState(target);
    //     if (error) {
    //         setErrorCode(error);
    //         return;
    //     }

    //     /* output primitive mode from (geometry) shader program over writtes
    //      * primitive mode of draw call!
    //      */
    //     if (target == DBG_TARGET_GEOMETRY_SHADER) {
    //         if (forcePointPrimitiveMode) {
    //             primitiveMode = GL_POINTS;
    //         } else {
    //             primitiveMode = getShaderPrimitiveMode();
    //         }
    //     }

    //     /* begin transform feedback */
    //     error = beginTransformFeedback(primitiveMode);
    //     if (error) {
    //         setErrorCode(error);
    //         return;
    //     }

    //     replayFunctionCalls(&G.recordedStream, 0);
    //     error = glError();
    //     if (error) {
    //         setErrorCode(error);
    //         return;
    //     }

    //     /* readback feedback buffer */
    //     error = endTransformFeedback(primitiveMode, numFloatsPerVertex, &buffer,
    //                                  &numPrimitives, &numVertices);
    //     if (error) {
    //         setErrorCode(error);
    //     } else {
    //         rec->result = DBG_READBACK_RESULT_VERTEX_DATA;
    //         rec->items[0] = (ALIGNED_DATA) buffer;
    //         rec->items[1] = (ALIGNED_DATA) numVertices;
    //         rec->items[2] = (ALIGNED_DATA) numPrimitives;
    //     }
    // } else if (target == DBG_TARGET_FRAGMENT_SHADER) {
    //     int numComponents = (int) rec->items[4];
    //     int format = (int) rec->items[5];
    //     int width, height;
    //     void *buffer;

    //     /* set debug shader code */
    //     error = loadDbgShader(vshader, gshader, fshader, target, 0);
    //     if (error) {
    //         setErrorCode(error);
    //         return;
    //     }

    //     /* replay recorded drawcall */
    //     error = setSavedGLState(target);
    //     if (error) {
    //         setErrorCode(error);
    //         return;
    //     }
    //     replayFunctionCalls(&G.recordedStream, 0);
    //     error = glError();
    //     if (error) {
    //         setErrorCode(error);
    //         return;
    //     }

    //     /* readback framebuffer */
    //     DMARK
    //     error = readBackRenderBuffer(numComponents, format, &width, &height,
    //                                  &buffer);
    //     DMARK
    //     if (error) {
    //         setErrorCode(error);
    //     } else {
    //         rec->result = DBG_READBACK_RESULT_FRAGMENT_DATA;
    //         rec->items[0] = (ALIGNED_DATA) buffer;
    //         rec->items[1] = (ALIGNED_DATA) width;
    //         rec->items[2] = (ALIGNED_DATA) height;
    //     }
    // } else {
    //     UT_NOTIFY_NO_PRFX("\n");
    //     setErrorCode(DBG_ERROR_INVALID_DBG_TARGET);
    // }
}

enum DBG_OPERATION getDbgOperation(const char* func)
{
    os_pid_t pid = os_getpid();
    thread_state_t *rec = getThreadState(pid);
    enum DBG_OPERATION ret = DBG_CALL_ORIGFUNCTION_AND_PROCEED;
    if (!keepExecuting(func)) {
        stop();
    }
    /* read dbg command from queue and return it */
    UT_NOTIFY(LV_INFO, "OPERATION: %u @ %p", rec->mode, rec);
    // FIXME 
    //return rec->operation;
    return ret;
}

static int isDebuggableDrawCall(const char *name)
{
    int i = 0;
    while (glFunctions[i].name != NULL) {
        if (!strcmp(name, glFunctions[i].name)) {
            return glFunctions[i].is_debuggable;
        }
        i++;
    }
    return 0;
}

static int isShaderSwitch(const char *name)
{
    int i = 0;
    while (glFunctions[i].name != NULL) {
        if (!strcmp(name, glFunctions[i].name)) {
            return glFunctions[i].is_shader_switch;
        }
        i++;
    }
    return 0;
}

int keepExecuting(const char *func)
{
    os_pid_t pid = os_getpid();
    thread_state_t *rec = getThreadState(pid);
    if (rec->mode == EX_MODE_UNATTENDED) {
        return 1;
    } else if (rec->mode == EX_MODE_INTERACTIVE) {
        switch (rec->halt_on) {
        case EX_HALT_ALL:
            return 0;
        case EX_HALT_ON_SHADER_SWITCH:
            return !isShaderSwitch(func);
        case EX_HALT_ON_DRAW_CALL:
            //TODO:  allow also jumps to non-debuggable draw calls
            return !isDebuggableDrawCall(func);
        case EX_HALT_ON_USER_DEFINED:
            // FIXME do a hash comparison here
            return strcmp(rec->halt_on_function, func);
        default:
            break;
        }
        setErrorCode(DBG_ERROR_INVALID_OPERATION);
    }
    return 0;
}

int checkGLErrorInExecution(void)
{
    os_pid_t pid = os_getpid();
    thread_state_t *rec = getThreadState(pid);
    return *rec->current_call->return_data;
}

void setExecuting(void)
{
    // FIXME still usefull?
    // os_pid_t pid = os_getpid();
    // thread_state_t *rec = getThreadState(pid);
    // rec->result = DBG_EXECUTE_IN_PROGRESS;
}

void executeDefaultDbgOperation(enum DBG_OPERATION op)
{
    // switch (op) {
    // /* DBG_CALL_FUNCTION, DBG_RECORD_CALL, and DBG_CALL_ORIGFUNCTION handled
    //  * directly in functionHooks.inc
    //  */
    // case DBG_ALLOC_MEM:
    //         allocMem();
    //     break;
    // case DBG_FREE_MEM:
    //     freeMem();
    //     break;
    // case DBG_READ_RENDER_BUFFER:
    //     if (G.errorCheckAllowed) {
    //         readRenderBuffer();
    //     } else {
    //         setErrorCode(DBG_ERROR_READBACK_NOT_ALLOWED);
    //     }
    //     break;
    // case DBG_CLEAR_RENDER_BUFFER:
    //     if (G.errorCheckAllowed) {
    //         clearRenderBuffer();
    //     } else {
    //         setErrorCode(DBG_ERROR_OPERATION_NOT_ALLOWED);
    //     }
    //     break;
    // case DBG_SET_DBG_TARGET:
    //     setDbgOutputTarget();
    //     break;
    // case DBG_RESTORE_RENDER_TARGET:
    //     restoreOutputTarget();
    //     break;
    // case DBG_START_RECORDING:
    //     startRecording();
    //     break;
    // case DBG_REPLAY:
    //     /* should be obsolete: we use a invalid debug target to avoid
    //      * interference with debug state
    //      */
    //     replayRecording(DBG_TARGET_FRAGMENT_SHADER + 1);
    //     break;
    // case DBG_END_REPLAY:
    //     endReplay();
    //     break;
    // case DBG_STORE_ACTIVE_SHADER:
    //     storeActiveShader();
    //     break;
    // case DBG_RESTORE_ACTIVE_SHADER:
    //     restoreActiveShader();
    //     break;
    // case DBG_SET_DBG_SHADER:
    //     setDbgShader();
    //     break;
    // case DBG_GET_SHADER_CODE:
    //     getShaderCode();
    //     break;
    // case DBG_SHADER_STEP:
    //     shaderStep();
    //     break;
    // case DBG_SAVE_AND_INTERRUPT_QUERIES:
    //     interruptAndSaveQueries();
    //     break;
    // case DBG_RESTART_QUERIES:
    //     restartQueries();
    //     break;
    // default:
    //     UT_NOTIFY(LV_INFO, "UNKNOWN DEBUG OPERATION %i", op);
    //     break;
    // }
}

static void dbgFunctionNOP(void)
{
    setErrorCode(DBG_ERROR_NO_SUCH_DBG_FUNC);
}


void (*getDbgFunction(void))(void)
{
    os_pid_t pid = os_getpid();
    thread_state_t *rec = getThreadState(pid);
    int i;

    for (i = 0; i < g.numDbgFunctions; ++i) {
        /* FIXME do lookup using hash tables */
        if (!strcmp(g.dbgFunctions[i].fname, rec->current_call->name)) {
            UT_NOTIFY(LV_INFO, "found special detour for %s", rec->current_call->name);
            return g.dbgFunctions[i].function;
        }
    }
    return dbgFunctionNOP;
}

/* HAZARD: Windows will never set G.errorCheckAllowed for Begin/End as below!!! */
#ifdef _WIN32
__declspec(dllexport) PROC APIENTRY HookedwglGetProcAddress(LPCSTR arg0);
void (*getOrigFunc(const char *fname))(void)
{
	return (void (*)(void)) HookedwglGetProcAddress(fname);
}
#else /* _WIN32 */
void (*getOrigFunc(const char *fname))(void)
{
	/* glXGetProcAddress and  glXGetProcAddressARB are special cases: we have to
	 * call our version not the original ones
	 */
	if (!strcmp(fname, "glXGetProcAddress") ||
		!strcmp(fname, "glXGetProcAddressARB")) {
		return (void (*)(void))glXGetProcAddressHook;
	} else {
		void *result = hash_find(&g.origFunctions, (void*)fname);

		if (!result) {
#ifdef USE_DLSYM_HARDCODED_LIB
            void *origFunc = g.origdlsym(g.libgl, fname);
#else
            void *origFunc = g.origdlsym(RTLD_NEXT, fname);
#endif
            if (!origFunc) {
                origFunc = G.origGlXGetProcAddress((const GLubyte *)fname);
                if (!origFunc) {
                    UT_NOTIFY(LV_ERROR, "Cannot resolve %s", fname);
                    exit(1); /* TODO: proper error handling */
                }
            }
            hash_insert(&g.origFunctions, (void *)fname, origFunc);
            result = origFunc;
        }

        /* FIXME: Is there a better place for this ??? */
        if (!strcmp(fname, "glBegin")) {
            G.errorCheckAllowed = 0;
        } else if (!strcmp(fname, "glEnd")) {
            G.errorCheckAllowed = 1;
        }
        UT_NOTIFY(LV_INFO, "ORIG_GL: %s (%p)", fname, result);
        return (void (*)(void))result;
    }
}
#endif /* _WIN32 */


/* work-around for external debug functions */
/* TODO: do we need debug functions at all? */
void (*DEBUGLIB_EXTERNAL_getOrigFunc(const char *fname))(void)
{
    return getOrigFunc(fname);
}


int checkGLExtensionSupported(const char *extension)
{
	// statics are set to zero
	static Hash extensions;
	static int dummy = 1;

    if(!extensions.table) {
        int n;
		UT_NOTIFY(LV_DEBUG, "Creating extension hashes");
        hash_create(&extensions, hashString, compString, 512, 0);
        ORIG_GL(glGetIntegerv)(GL_NUM_EXTENSIONS, &n);
		UT_NOTIFY(LV_INFO, "Extensions found %i", n);
        for(int i = 0; i < n; ++i) {
            const GLubyte *name = ORIG_GL(glGetStringi)(GL_EXTENSIONS, i);
            // we don't need to store any relevant data. we just want a quick
            // string lookup.
            if (hash_insert(&extensions, name, &dummy) == 1) {
                UT_NOTIFY(LV_ERROR, "Collision occured while hashing extensions");
                exit(1);
            }
        }
    }

    // check support
    if(!hash_find(&extensions, extension)) {
        UT_NOTIFY(LV_INFO, "not found: %s", extension);
        return 0;
    }
    UT_NOTIFY(LV_INFO, "found: %s", extension);
    return 1;
}

int checkGLVersionSupported(int majorVersion, int minorVersion)
{
	static int major = 0;
	static int minor = 0;

	UT_NOTIFY(LV_INFO, "GL version %i.%i: ", majorVersion, minorVersion);
	if (major == 0) {
		const char *versionString = (char*)ORIG_GL(glGetString)(GL_VERSION);
		const char *rendererString = (char*)ORIG_GL(glGetString)(GL_RENDERER);
		const char *vendorString = (char*)ORIG_GL(glGetString)(GL_VENDOR);
		const char *shadingString = (char*)ORIG_GL(glGetString)(GL_SHADING_LANGUAGE_VERSION);
		char  *dot = NULL;
		major = (int)strtol(versionString, &dot, 10);
		minor = (int)strtol(++dot, NULL, 10);
		UT_NOTIFY(LV_INFO, "GL VENDOR: %s", vendorString);
		UT_NOTIFY(LV_INFO, "GL RENDERER: %s", rendererString);
		UT_NOTIFY(LV_INFO, "GL VERSION: %s", versionString);
		UT_NOTIFY(LV_INFO, "GL SHADING LANGUAGE: %s: %s", shadingString);
	}
	if (majorVersion < major ||
		(majorVersion == major && minorVersion <= minor)) {
		return 1;
	}
	UT_NOTIFY(LV_INFO, "required GL version supported: NO");
	return 0;
}

TFBVersion getTFBVersion()
{
    if (checkGLExtensionSupported("GL_NV_transform_feedback")) {
        return TFBVersion_NV;
    } else if (checkGLExtensionSupported("GL_EXT_transform_feedback")) {
        return TFBVersion_EXT;
    } else {
        return TFBVersion_None;
    }
}


#ifdef RTLD_DEEPBIND
void *dlsym(void *handle, const char *symbol)
{
    char *s;
    void *sym;

    if (!g.origdlsym) {
        void *origDlsymHandle;

        s = getenv("GLSL_DEBUGGER_LIBDLSYM");
        if (!s) {
            UT_NOTIFY(LV_ERROR, "GLSL_DEBUGGER_LIBDLSYM is not set");
            exit(1);
        }

        if (! (origDlsymHandle = dlopen(s, RTLD_LAZY | RTLD_DEEPBIND))) {
            UT_NOTIFY(LV_ERROR, "getting origDlsymHandle failed %s: %s",
                      s, dlerror());
        }
        dlclose(origDlsymHandle);
        s = getenv("GLSL_DEBUGGER_DLSYM");
        if (s) {
            g.origdlsym = (void *(*)(void *, const char *))(intptr_t)strtoll(s, NULL, 16);
        } else {
            UT_NOTIFY(LV_ERROR, "GLSL_DEBUGGER_DLSYM is not set");
            exit(1);
        }
        unsetenv("GLSL_DEBUGGER_DLSYM");
    }

    if (g.initialized) {
        sym = (void *)glXGetProcAddressHook((GLubyte *)symbol);
        if (sym) {
            return sym;
        }
    }

    return g.origdlsym(handle, symbol);
}
#endif
static void *connection_handler(void *args)
{
    handlerstate_t state = HANDLERSTATE_INIT;
    state.sock = (socket_t *)args;
    uint8_t *buf = NULL;
    size_t num_bytes;
    msg_size_t length;
    msg_size_t allocated_length = 0;
    while (!state.end_connection) {
        if (sck_recv(state.sock, &length, sizeof(msg_size_t)) == -1) {
            UT_NOTIFY(LV_ERROR, "Unable to receive message size");
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
            size_t read = sck_recv(state.sock, buf, length);
            if (read < 0) {
                UT_NOTIFY(LV_ERROR, "Receiving request failed");
                goto fail;
            }
            num_bytes += read;
        } while (num_bytes < length);
        state.request = proto__client_request__unpack(NULL, num_bytes, buf);
        if (!state.request) {
            UT_NOTIFY(LV_ERROR, "Unpacking request failed");
            goto fail;
        }

        switch (state.request->type) {
        case PROTO__CLIENT_REQUEST__TYPE__ANNOUNCE:
            UT_NOTIFY(LV_INFO, "announce received");
            handle_request_announce(&state);
            break;
        case PROTO__CLIENT_REQUEST__TYPE__PROCESS_INFO:
            UT_NOTIFY(LV_INFO, "process info request received");
            break;
        case PROTO__CLIENT_REQUEST__TYPE__FUNCTION_CALL:
            UT_NOTIFY(LV_WARN, "current function call requested");
            break;
        case PROTO__CLIENT_REQUEST__TYPE__EXECUTION:
            UT_NOTIFY(LV_INFO, "exec details received");
            handle_request_execution(&state);
            break;
        default:
            UT_NOTIFY(LV_WARN, "Unknown request received");
        }
        proto__client_request__free_unpacked(state.request, NULL);
        if (state.response) {
            send_response(&state);
            free(state.response);
            state.response = NULL;
        }
    }
fail:
    UT_NOTIFY(LV_DEBUG, "connection handler quits");
    free(buf);
    sck_destroy(state.sock);
    return NULL;
}
static int new_connection_callback(socket_t *s)
{
    UT_NOTIFY(LV_INFO, "New connection request");
    pthread_t thread;
    pthread_create(&thread, NULL, connection_handler, s);
    pthread_detach(thread);
    return 0;
}
static void handle_request_announce(handlerstate_t *state)
{
    state->response = malloc(sizeof(Proto__ServerResponse));
    proto__server_response__init(state->response);
    state->response->id = state->request->id;
    Proto__AnnounceRequestDetails *msg = state->request->announce;
    state->response->error_code = PROTO__ERROR_CODE__NONE;
    state->response->message = "Welcome dude!";
    if (msg->id != PROTO_ID) {
        UT_NOTIFY(LV_ERROR, "header mismatch");
        state->response->error_code = PROTO__ERROR_CODE__HEADER_MISMATCH;
        state->response->message = "header mismatch";
        state->end_connection = 1;
    }
    Proto__Version *version = msg->version;
    if (version->major != PROTO_MAJOR) {
        UT_NOTIFY(LV_ERROR, "protocol version mismatch");
        state->response->error_code = PROTO__ERROR_CODE__VERSION_MISMATCH;
        state->response->message = "protocol version mismatch";
        state->end_connection = 1;
    }
}
static void send_response(handlerstate_t *state)
{
    UT_NOTIFY(LV_TRACE, "Sending response");
    msg_size_t len = proto__server_response__get_packed_size(state->response);
    size_t num_bytes = sck_send(state->sock, &len, sizeof(msg_size_t));
    assert(num_bytes == sizeof(msg_size_t));
    uint8_t *buffer = malloc(len);
    assert(buffer);
    proto__server_response__pack(state->response, buffer);
    num_bytes = sck_send(state->sock, buffer, len);
    assert(num_bytes == len);
    free(buffer);
}
<<<<<<< HEAD
=======
static Proto__FunctionCall *fcall_create(const char *funcname, int num_args)
{
    Proto__FunctionCall *fc = malloc(sizeof(Proto__FunctionCall));
    assert(fc);
    fc->n_arguments = num_args;
    fc->arguments = malloc(num_args * sizeof(fc->arguments));
    assert(fc->arguments);
    for (int i = 0; i < num_args; ++i) {
        fc->arguments[i] = malloc(sizeof(Proto__FunctionArgument));
        assert(fc->arguments[i]);
        proto__function_argument__init(fc->arguments[i]);
    }
    return fc;
}
static void fcall_destroy(Proto__FunctionCall *fc)
{
    if (!fc)
        return;
    for (int i = 0; i < fc->n_arguments; ++i) {
        free(fc->arguments[i]);
    }
    free(fc->arguments);
    free(fc);
}
static void handle_request_execution(handlerstate_t *state)
{
    /* create response */
    state->response = malloc(sizeof(Proto__ServerResponse));
    proto__server_response__init(state->response);

    Proto__ExecutionRequestDetails *msg = state->request->execution;
    state->response->error_code = PROTO__ERROR_CODE__NONE;
}
>>>>>>> basic communication should work
