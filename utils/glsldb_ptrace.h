#ifndef GLSLDB_PTRACE_H
#define GLSLDB_PTRACE_H

#ifndef GLSLDB_WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/shm.h>
#include <sched.h>
#endif /* !GLSLDB_WIN32 */

#ifdef GLSLDB_OSX
#  include <signal.h>
#  include "utils/osx_ptrace_defs.h"
#  define __ptrace_request int
#endif

#ifndef PTRACE_SETOPTIONS
/* from linux/ptrace.h */
#  define PTRACE_SETOPTIONS   0x4200
#  define PTRACE_GETEVENTMSG  0x4201
#  define PTRACE_GETSIGINFO   0x4202
#  define PTRACE_SETSIGINFO   0x4203

/* Wait extended result codes for the above trace options.  */
#define PTRACE_EVENT_FORK	1
#define PTRACE_EVENT_VFORK	2
#define PTRACE_EVENT_CLONE	3
#define PTRACE_EVENT_EXEC	4
#define PTRACE_EVENT_VFORK_DONE	5
#define PTRACE_EVENT_EXIT	6
#define PTRACE_EVENT_SECCOMP	7
/* Extended result codes which enabled by means other than options.  */
#define PTRACE_EVENT_STOP	128

/* Options set using PTRACE_SETOPTIONS or using PTRACE_SEIZE @data param */
#define PTRACE_O_TRACESYSGOOD	1
#define PTRACE_O_TRACEFORK	(1 << PTRACE_EVENT_FORK)
#define PTRACE_O_TRACEVFORK	(1 << PTRACE_EVENT_VFORK)
#define PTRACE_O_TRACECLONE	(1 << PTRACE_EVENT_CLONE)
#define PTRACE_O_TRACEEXEC	(1 << PTRACE_EVENT_EXEC)
#define PTRACE_O_TRACEVFORKDONE	(1 << PTRACE_EVENT_VFORK_DONE)
#define PTRACE_O_TRACEEXIT	(1 << PTRACE_EVENT_EXIT)
#define PTRACE_O_TRACESECCOMP	(1 << PTRACE_EVENT_SECCOMP)

/* eventless options */
#define PTRACE_O_EXITKILL	(1 << 20)
#endif

#endif