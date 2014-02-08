#include "Process.qt.h"
#include "Debugger.qt.h"
#include "utils/notify.h"
#include "utils/glsldb_ptrace.h"

#ifndef GLSLDB_WIN32
#	include <sys/types.h>
#	include <sys/wait.h>
#	include <signal.h>
#endif
#include <cassert>
#include <exception>

extern "C" {
#include "utils/p2pcopy.h"
}

Process::Process() : 
	_pid(0), 
    _statusHandler(nullptr), 
	_state(INIT)
{
}

Process::Process(const DebugConfig& cfg, PID_T pid) :
	_pid(pid), 
    _statusHandler(nullptr), 
	_debugConfig(cfg), 
	_state(INIT)
{
	/* TODO check if proc is already running */
	/* if (pid != 0) ... */
}
void Process::startStatusHandler()
{
    if(!_statusHandler) {
        _end = false;
        _statusHandler = new std::thread(&Process::handleStatusUpdates, this);
    }
}
void Process::stopStatusHandler()
{
    if(_statusHandler) {
        _end = true;
	    if(_state == RUNNING || _state == STOPPED || _state == TRAPPED)
    		kill();
        if(_statusHandler->joinable())
            _statusHandler->join();
        delete _statusHandler;
        _statusHandler = nullptr;
    }
}

Process::~Process()
{
	UT_NOTIFY(LV_TRACE, "~Process");
    stopStatusHandler();
}
CommandFactory& Process::commandFactory()
{
	if(!_cmdFactory)
		_cmdFactory = CommandFactoryPtr(new CommandFactory(*this));
	return *_cmdFactory;
}
void Process::kill()
{
#ifdef GLSLDB_WIN
	if (_ai.hProcess != NULL) {
		return this->detachFromProgram();
	} else if (_hDebuggedProgram != NULL) {
		ErrorCode retval = ::TerminateProcess(_hDebuggedProgram, 42)
		? NONE : EXIT;
		_hDebuggedProgram = NULL;
		_pid = 0;
		return retval;
	}
#else /* GLSLDB_WIN */
	if (_pid) {
        _end = true;
		if(::kill(_pid, SIGKILL) == -1) {
            state(INVALID);
            throw std::runtime_error("attempt to kill process failed");
        }
        _statusHandler->join();
	}
#endif /* GLSLDB_WIN */
}

void Process::advance(void)
{
    if(!isStopped() && !isTrapped())
        return;
#ifdef _WIN32
	if (::SetEvent(_hEvtDebuggee)) {
        state(RUNNING);
		return;
	}
#else /* _WIN32 */
	if(ptrace(PTRACE_CONT, _pid, 0, 0) != -1) {
        state(RUNNING);
		return;
	}
#endif /* _WIN32 */
	throw std::runtime_error("Continue failed");
}

void Process::stop(bool immediately)
{
    if(!isRunning())
        return;

	if(immediately) {
		if(::kill(_pid, SIGSTOP) == -1) {
            state(INVALID);
			throw std::runtime_error("halt failed");
        }
        state(STOPPED);
	}
	else {
		/* FIXME use CommandFactory */
		//debug_record_t *rec = getThreadRecord(_pid);
		// UT_NOTIFY(LV_INFO, "send: DBG_STOP_EXECUTION");
		// rec->operation = DBG_STOP_EXECUTION;
		// /* FIXME race conditions & use Command? */
		// advance();
		// waitForStatus();
	}
}
void Process::handleStatusUpdates()
{
    std::unique_lock<std::mutex> lock(_mtx, std::defer_lock);
    while(!_end) {
        waitForStatus();
    }
}
void Process::waitForStatus(void)
{
#ifndef _WIN32
	int status = 15;
	pid_t pid = -1;
	int sig;
	UT_NOTIFY(LV_DEBUG, "waiting for debuggee(" << _pid <<  ") status...");

    pid = waitpid(_pid, &status, 0);

    /* no status available. leave unchanged */
	if(pid == 0) {
		/* should not happen unlesse WNOHANG is specified */
 		throw std::runtime_error("waitpid failed");
    }
    else if(pid == -1) {
        if(errno == EINTR) {
		    UT_NOTIFY(LV_WARN, "waitpid got interrupted");
        }
        else if(errno == ECHILD) {
		    UT_NOTIFY(LV_ERROR, "no such debuggee");
        }
		state(INVALID);
        _end = true;
        return;
	}

	UT_NOTIFY(LV_INFO, "received status (" << status << ") from " << pid);
    if(pid != _pid)
	    UT_NOTIFY(LV_INFO, "FROM CHILD!");

	if(WIFEXITED(status) != 0) {
		sig = WEXITSTATUS(status);
		if(sig != 0) {
			UT_NOTIFY(LV_INFO, "exited with status " << sig << "(" << strsignal(sig) << ")");
		}
        state(EXITED);
        _end = true;
        //return;
	}
    else if(WIFSIGNALED(status) != 0) {
		sig = WTERMSIG(status);
		if(sig != 0) {
			UT_NOTIFY(LV_INFO, "signal was " << sig << "(" << strsignal(sig) << ")");
		}
        state(KILLED);
        _end = true;
	}
	/* this is the interesting case */
	else if(WIFSTOPPED(status) != 0) {
		/* debuggee stopped due to signal-delivery-stops, group-stops, PTRACE_EVENT stops
		 * or syscall-stops.
		 */
		sig = WSTOPSIG(status);
		UT_NOTIFY(LV_INFO, "signal was " << sig << "(" << strsignal(sig) << ")");
        switch(sig) {
            case SIGTRAP:
			    checkTrapEvent(pid, status);
    			state(TRAPPED);
                break;
            case SIGSTOP:
			    state(STOPPED);
    			/* update thread record ptr */
	    	 	_rec = Debugger::instance().debugRecord(_pid);
                break;
            case SIGHUP:
            case SIGILL:
            case SIGFPE:
            case SIGSEGV:
                UT_NOTIFY(LV_WARN, "debuggee misbehaved.");
			    state(KILLED);
                _end = true;
                break;
            default:
                UT_NOTIFY(LV_WARN, "unimplemented stop signal.");
			    state(INVALID);
                _end = true;
        }
	}
    else if(WIFCONTINUED(status) != 0) {
        state(RUNNING);
		//return;
	}
    else {
		UT_NOTIFY(LV_ERROR, "debugee in unknown state");
    }
    //UT_NOTIFY(LV_DEBUG, "REACHED END");
    //state(ST_INVALID);
    return;
	
#else /* !_WIN32 */
	DWORD exitCode = STILL_ACTIVE;
	ErrorCode retval = EC_NONE;

	QApplication::setOverrideCursor(Qt::BusyCursor);
	while (exitCode == STILL_ACTIVE) {
		switch (::WaitForSingleObject(_hEvtDebugger, 2000)) {
			case WAIT_OBJECT_0:
			/* Event was signaled. */
			QApplication::restoreOverrideCursor();
			return retval;

			case WAIT_TIMEOUT:
			/* Wait timeouted, check whether debuggee is alive. */
			break;

			default:
			/* Wait failed. */
			QApplication::restoreOverrideCursor();
			return EC_UNKNOWN_ERROR;
		}

		// TODO: This is unsafe. Consider rewriting it using the signaled
		// state of the debuggee process.
		if (::GetExitCodeProcess(_hDebuggedProgram, &exitCode)) {
			if (exitCode != STILL_ACTIVE) {
				retval = EC_EXIT;
			}
		} else {
			dbgPrint(DBGLVL_WARNING, "retrieving process exit code failed: %u\n", ::GetLastError());
			retval = EC_UNKNOWN_ERROR;
			break;
		}
	} /* end while (exitCode == STILL_ACTIVE) */

	QApplication::restoreOverrideCursor();
	return retval;
#endif /* !_WIN32 */
}

bool Process::checkTrapEvent(pid_t pid, int status)
{
    pid_t newpid;
    unsigned long msg;
    if ((status & (PTRACE_EVENT_FORK << 8)) || (status & (PTRACE_EVENT_VFORK))) {
        UT_NOTIFY(LV_INFO, "debuggee calls (v)fork()");
    }
    else if (status & (PTRACE_EVENT_CLONE << 8)) {
        UT_NOTIFY(LV_INFO, "debuggee calls clone()");
    }
    else if (status & (PTRACE_EVENT_VFORK_DONE << 8)) {
        UT_NOTIFY(LV_INFO, "debuggee signals vfork() done");
    }
    else if (status & (PTRACE_EVENT_EXEC << 8)) {
        UT_NOTIFY(LV_INFO, "debuggee calls exec()");
        return false;
    }
    else if (status & (PTRACE_EVENT_EXIT << 8)) {
        UT_NOTIFY(LV_INFO, "debuggee calls exit()");
        return false;
    }
    else if (status & (PTRACE_EVENT_STOP << 8)) {
        UT_NOTIFY(LV_INFO, "debuggee calls stop()");
        return false;
    }
    else if (status & (PTRACE_EVENT_SECCOMP << 8)) {
        UT_NOTIFY(LV_INFO, "seccomp");
        return false;
    }
    else {
        UT_NOTIFY(LV_INFO, "unknown ptrace event " << status);
        return false;
    }
    UT_NOTIFY(LV_INFO, "retrieving new pid for "<<pid);
    if(ptrace((__ptrace_request)PTRACE_GETEVENTMSG, pid, 0, &msg) == -1) {
        UT_NOTIFY(LV_ERROR, "unable to retrieve new pid: " << strerror(errno));
        return false;
    }
    newpid = msg;
    if(pid == newpid || newpid == 0)
        return false;
    UT_NOTIFY(LV_INFO, "new child has pid " << newpid);
    emit newChild(newpid);
    return true;
}

// void Process::attach()
// {
// 	   // if(ptrace(PTRACE_ATTACH, newpid, 0, 0) == -1) {
//    //     UT_NOTIFY(LV_ERROR, "unable to attach " << newpid);
//    // }
//     /* FIXME 
//      * we need a mechanism to handle forking and stuff like that */
//     int opts = 0;
//     if(_debugConfig.traceFork) {
//         opts |= PTRACE_O_TRACEFORK;
//         opts |= PTRACE_O_TRACEVFORK;
// 		opts |= PTRACE_O_TRACEVFORKDONE;
//         UT_NOTIFY(LV_INFO, "Tracing fork()");
//     }
//     if(_debugConfig.traceExec) {
// 		opts |= PTRACE_O_TRACEEXEC;
//         UT_NOTIFY(LV_INFO, "Tracing exec()");
//     }
//     if(_debugConfig.traceClone) {
// 		opts |= PTRACE_O_TRACECLONE;
//         UT_NOTIFY(LV_INFO, "Tracing clone()");
//     }
// #ifdef PTRACE_O_EXITKILL
//     opts |= PTRACE_O_EXITKILL;
// #endif

// 	if(ptrace((__ptrace_request ) PTRACE_SETOPTIONS, newpid, 0, opts) == -1) {
// 		UT_NOTIFY(LV_ERROR, "unable to set PTRACE options: " << strerror(errno));		
// 	}
    
//     /* FIXME not sure whether we want to do that */
//     _pid = newpid;
//     if(ptrace(PTRACE_DETACH, pid, 0, 0) == -1) {
//         UT_NOTIFY(LV_ERROR, "unable to detach from " << pid);
//     }
// 	if(ptrace(PTRACE_CONT, newpid, 0, 0) == -1) {
//         UT_NOTIFY(LV_ERROR, "unable to msg " << newpid);
//     }
// 	if(ptrace(PTRACE_CONT, pid, 0, 0) == -1) {
//         UT_NOTIFY(LV_ERROR, "unable to msg " << _pid);
//     }
// }

void Process::launch(const DebugConfig* cfg)
{
    if(state() != INIT)
        return;
	if(cfg)
		config(*cfg);

	if(!_debugConfig.valid)
		throw std::logic_error("config invalid");

#ifndef _WIN32
	_pid = fork();
	if (_pid == -1) {
		_pid = 0;
		throw std::runtime_error("forking failed. debuggee not started.");
	}

	if (_pid == 0) {
		/**
		 * debuggee path
		 * tell parent to trace the child 
		 * alternative: parent uses PTRACE_ATTACH but that might be racy
		 */
		ptrace(PTRACE_TRACEME, 0, 0, 0);

		if(!_debugConfig.workDir.isEmpty() && chdir(_debugConfig.workDir.toLatin1().data()) != 0) {
			UT_NOTIFY(LV_WARN, "changing to working directory failed (" << strerror(errno) << ")");
			UT_NOTIFY(LV_WARN, "debuggee execution might fail now");
		}
		UT_NOTIFY(LV_INFO, "debuggee executes " << _debugConfig.programArgs[0].toLatin1().data());
		setpgid(0, 0);

		char **pargs = static_cast<char**>(malloc((_debugConfig.programArgs.size() + 1) * sizeof(char*)));
		int i = 0;
		for(auto &j : _debugConfig.programArgs) {
			pargs[i] = static_cast<char*>(malloc((j.size() + 1) * sizeof(char)));
			strcpy(pargs[i], j.toLatin1().data());
			i++;
		}
		pargs[_debugConfig.programArgs.size()] = NULL;
		// execv() triggers a SIGTRAP
		// allows parent to set trace options
		execv(pargs[0], pargs);

		/* an error occured, execv should never return */
		UT_NOTIFY(LV_ERROR, "debuggee fork/execution failed: " << strerror(errno));
		UT_NOTIFY(LV_ERROR, "debuggee pid: " << getpid());
		do {
			char *p = *pargs;
			free(p);
			pargs++;
		} while(*pargs != NULL);
		free(pargs);
		_exit(73);
	}
    /* attaching might be racy. we might miss a fork/execv if we 
     * don't get a chance to set options prior to that */
    //ptrace(PTRACE_ATTACH, _pid, 0, 0);

	// parent path
	UT_NOTIFY(LV_INFO, "debuggee process pid: " << _pid);
    waitForStatus();
	if(!isStopped() && !isTrapped()) {
		::kill(_pid, SIGKILL);
		_pid = 0;
		state(INVALID);
		throw std::runtime_error("Child didn't stop");
	}
	// set options, child needs to be in STOPPED
    int opts = 0;
    if(_debugConfig.traceFork) {
        opts |= PTRACE_O_TRACEFORK;
        opts |= PTRACE_O_TRACEVFORK;
		opts |= PTRACE_O_TRACEVFORKDONE;
        UT_NOTIFY(LV_INFO, "Tracing fork()");
    }
    if(_debugConfig.traceExec) {
		opts |= PTRACE_O_TRACEEXEC;
        UT_NOTIFY(LV_INFO, "Tracing exec()");
    }
    if(_debugConfig.traceClone) {
		opts |= PTRACE_O_TRACECLONE;
        UT_NOTIFY(LV_INFO, "Tracing clone()");
    }
#ifdef PTRACE_O_EXITKILL
    opts |= PTRACE_O_EXITKILL;
#endif

    if(ptrace((__ptrace_request ) PTRACE_SETOPTIONS, _pid, 0, opts) == -1) 
		UT_NOTIFY(LV_ERROR, "unable to set PTRACE options: " << strerror(errno));		
	else 
		UT_NOTIFY(LV_INFO, "Options have been applied. Continuing child ");

    /* don't really know why we have to do that */
    startStatusHandler();
	return;
#else /* _WIN32 */
	STARTUPINFOA startupInfo;
	PROCESS_INFORMATION processInfo;
	char dllPath[_MAX_PATH];			// Path to debugger preload DLL.
	char detouredDllPath[_MAX_PATH];// Path to marker DLL.
	char *cmdLine = NULL;// Command line to execute.
	size_t cntCmdLine = 0;// Size of command line.

	/* Only size must be initialised. */
	::ZeroMemory(&startupInfo, sizeof(STARTUPINFOA));
	startupInfo.cb = sizeof(STARTUPINFOA);

	/* Get path to detours marker DLL. */
	HMODULE hDetouredDll = ::DetourGetDetouredMarker();
	::GetModuleFileNameA(hDetouredDll, detouredDllPath, _MAX_PATH);
	// Note: Do not close handle. See MSDN: "The GetModuleHandle function
	// returns a handle to a mapped module without incrementing its
	// reference count."

	/* Concatenate the command line. */
	for (int i = 0; debuggedProgramArgs[i] != 0; i++) {
		cntCmdLine += ::strlen(debuggedProgramArgs[i]) + 1;
	}
	cmdLine = new char[cntCmdLine + 1];
	*cmdLine = 0;
	for (int i = 0; debuggedProgramArgs[i] != 0; i++) {
		::strcat(cmdLine, debuggedProgramArgs[i]);
		::strcat(cmdLine, " ");
	}

	this->setDebugEnvVars();	// TODO dirty hack.
	LPVOID newEnv = ::GetEnvironmentStrings();

	HMODULE hThis = GetModuleHandleW(NULL);
	GetModuleFileNameA(hThis, dllPath, _MAX_PATH);
	char *insPos = strrchr(dllPath, '\\');
	if (insPos != NULL) {
		strcpy(insPos + 1, DEBUGLIB);
	} else {
		strcpy(dllPath, DEBUGLIB);
	}

	if (::DetourCreateProcessWithDllA(NULL, cmdLine, NULL, NULL, TRUE,
					CREATE_SUSPENDED | CREATE_DEFAULT_ERROR_MODE
					/*| CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS*/, NULL,
					workDir, &startupInfo, &processInfo, detouredDllPath, dllPath,
					NULL)) {
		this->createEvents(processInfo.dwProcessId);
		_hDebuggedProgram = processInfo.hProcess;
		_pid = processInfo.dwProcessId;
		//::DebugActiveProcess(processInfo.dwProcessId);
		//::DebugBreakProcess(processInfo.hProcess);
		::ResumeThread(processInfo.hThread);
		::CloseHandle(processInfo.hThread);
		error = EC_NONE;
	} else {
		error = EC_EXEC;
	}

	delete[] cmdLine;
	//return error;

	dbgPrint(DBGLVL_INFO, "wait for debuggee\n");
	error = waitForChildStatus();
	if (error != EC_NONE) {
		killDebuggee();
		//_pid = 0;
		return error;
	}

	dbgPrint(DBGLVL_INFO, "send continue\n");
	error = executeDbgCommand();
	//error = dbgCommandCallOrig();
	if (error != EC_NONE) {
		this->killDebuggee();
		//_pid = 0;
		return error;
	}
#ifdef DEBUG
	printCall();
#endif
	return error;
#endif /* _WIN32 */
}

bool Process::isAlive(void)
{
#ifndef _WIN32
	int status = 15;
	pid_t pid = -1;

	UT_NOTIFY(LV_TRACE, "getting debuggee status...");
	pid = waitpid(_pid, &status, WUNTRACED | WNOHANG);

	if (pid == -1) {
		UT_NOTIFY(LV_WARN, "waitpid failed (" << strerror(errno) << ")");
		return false;
	}
	else if(pid == 0) {
		UT_NOTIFY(LV_WARN, "waitpid succeeded but no debuggee status available");		
	}
	return true;
#else /* !_WIN32 */
	/* TODO: check this code!! */
	DWORD exitCode = STILL_ACTIVE;

	switch (::WaitForSingleObject(_hEvtDebugger, 2000)) {
		case WAIT_OBJECT_0:
		/* Event was signaled. */
		return true;

		case WAIT_TIMEOUT:
		/* Wait timeouted, check whether child is alive. */
		break;

		default:
		/* Wait failed. */
		return false;
	}

	// TODO: This is unsafe. Consider rewriting it using the signaled
	// state of the child process.
	if (::GetExitCodeProcess(_hDebuggedProgram, &exitCode)) {
		if (exitCode != STILL_ACTIVE) {
			return false;
		}
	} else {
		dbgPrint(DBGLVL_WARNING, "Retrieving process exit code failed: %u\n",
				::GetLastError());
		return false;
	}
	return true;
#endif /* !_WIN32 */
}


void* Process::copyArgumentFrom(void *addr, DBG_TYPE type)
{
	void *r = NULL;
	r = malloc(sizeof_dbg_type(type));
	cpyFromProcess(_pid, r, addr, sizeof_dbg_type(type));
	return r;
}

void Process::copyArgumentTo(void *dst, void *src, DBG_TYPE type)
{
	cpyToProcess(_pid, dst, src, sizeof_dbg_type(type));
}

void Process::state(State s)
{
	if(s != _state) {
		emit stateChanged(s);
		_state = s;		
	}
}

const QString& Process::strState(State s)
{
	switch(s) {
		case INVALID:
			{ static const QString m("INVALID"); return m; }
		case INIT:
			{ static const QString m("INIT"); return m; }
		case RUNNING:
			{ static const QString m("RUNNING"); return m; }
		case STOPPED:
			{ static const QString m("STOPPED"); return m; }
		case EXITED:
			{ static const QString m("EXITED"); return m; }
		case KILLED:
			{ static const QString m("KILLED"); return m; }
		case TRAPPED:
			{ static const QString m("TRAPPED"); return m; }
		default:
			{ static const QString m("INVALID STATE"); return m; }
	}
	static const QString m("DUMMY");
	return m;
}

FunctionCallPtr Process::getCurrentCall(void)
{
	FunctionCallPtr call(new FunctionCall(_rec->fname));

	for (int i = 0; i < (int) _rec->numItems; ++i) {
		FunctionArgumentPtr arg(new FunctionArgument(static_cast<DBG_TYPE>(_rec->items[2 * i + 1]), 
			copyArgumentFrom((void*) _rec->items[2 * i], static_cast<DBG_TYPE>(_rec->items[2 * i + 1])), 
			(void*) _rec->items[2 * i]));
		call->addArgument(arg);
	}

	return call;
}

