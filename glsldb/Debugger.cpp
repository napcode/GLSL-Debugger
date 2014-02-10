#include "Debugger.qt.h"
#include "notify.h"

#include <QtCore/QFileInfo>

#include "build-config.h"
#include "os/os.h"
#ifndef GSLDLB_WIN
#	include <unistd.h>
#	include <sys/shm.h>
#endif

Debugger* Debugger::_instance = nullptr;

Debugger& Debugger::instance()
{
	if(!_instance) {
		_instance = new Debugger();
	}
	return *_instance;
}

void Debugger::release()
{
	delete _instance;
	_instance = nullptr;
}

Debugger::Debugger()
	: _shmID(0),
    _worker(nullptr), 
    _end(true)
{
#ifdef _WIN32
	_hEvtDebuggee = NULL;
	_hEvtDebugger = NULL;
	_hDebuggedProgram = NULL;
	_ai.hDetours = NULL;
	_ai.hLibrary = NULL;
	_ai.hProcess = NULL;
#endif /* _WIN32 */
}

Debugger::~Debugger()
{
	UT_NOTIFY(LV_TRACE, "~Debugger");
	shutdown();
	freeSharedMem();
#ifdef _WIN32
	this->closeEvents();
	if (_hDebuggedProgram != NULL) {
		::CloseHandle(_hDebuggedProgram);
	}
#endif /* _WIN32 */
}

void Debugger::init()
{
	shutdown();
	buildEnvironmentVars();
	initSharedMem();
	clearSharedMem();
    setEnvironmentVars();
    _end = false;
    _worker = new std::thread(&Debugger::run, this);
}
void Debugger::shutdown()
{
    _end = true;
    if(_worker) {
        _workCondition.notify_all();
        _worker->join();
        delete _worker;
        _worker = nullptr;
    }
	_processes.clear();
}

ProcessPtr Debugger::create(const DebugConfig& cfg)
{
	ProcessPtr p(new Process(cfg));
	_processes.push_back(p);
	return p;
}
void Debugger::enqueue(CommandPtr cmd)
{
	{
    	std::lock_guard<std::mutex> lock(_mtxCmd);
		_queue.enqueue(cmd);
		UT_NOTIFY(LV_INFO, "Command enqueued");	
	}
    _workCondition.notify_one();
}
void Debugger::run()
{
    CommandPtr cmd;
    std::unique_lock<std::mutex> lock(_mtxWork, std::defer_lock);
    while(!_end) {
    	lock.lock();
        _workCondition.wait(lock, 
        	[this] { return !_queue.empty() || _end; });
		if(_end)
			break;
        cmd = _queue.dequeue();
    	lock.unlock();
        UT_NOTIFY(LV_INFO, "Command dequeued");
        (*cmd)();
    }
}

void Debugger::setEnvironmentVars(void)
{
#ifndef _WIN32
	if (setenv("LD_PRELOAD", _pathDbgLib.c_str(), 1))
		UT_NOTIFY(LV_ERROR, "setenv LD_PRELOAD failed: " << strerror(errno));
	UT_NOTIFY(LV_INFO, "env dbglib: " << _pathDbgLib);

	{
		std::stringstream s;
		s << _shmID;
		if (setenv("GLSL_DEBUGGER_SHMID", s.str().c_str(), 1))
			UT_NOTIFY(LV_ERROR,
					"setenv GLSL_DEBUGGER_SHMID failed: " << strerror(errno));
		UT_NOTIFY(LV_INFO, "env shmid: " << s.str());
	}

	if (setenv("GLSL_DEBUGGER_DBGFCTNS_PATH", _pathDbgFuncs.c_str(), 1))
		UT_NOTIFY(LV_ERROR,
				"setenv GLSL_DEBUGGER_DBGFCTNS_PATH failed: " << strerror(errno));
	UT_NOTIFY(LV_INFO, "env dbgfctns: " << _pathDbgFuncs);

	if (setenv("GLSL_DEBUGGER_LIBDLSYM", _pathLibDlSym.c_str(), 1))
		UT_NOTIFY(LV_ERROR, "setenv LIBDLSYM failed: " << strerror(errno));
	UT_NOTIFY(LV_DEBUG, "libdlsym: " << _pathLibDlSym);

	{
		std::stringstream s;
		s << utils_notify_level(nullptr);
		if (setenv("GLSL_DEBUGGER_LOGLEVEL", s.str().c_str(), 1))
			UT_NOTIFY(LV_ERROR,
					"setenv GLSL_DEBUGGER_LOGLEVEL failed: " << strerror(errno));
		UT_NOTIFY(LV_INFO, "env dbglvl: " << s.str());
	}

	if (_pathLog.size()
			&& setenv("GLSL_DEBUGGER_LOGDIR", _pathLog.c_str(), 1))
		UT_NOTIFY(LV_ERROR,
				"setenv GLSL_DEBUGGER_LOGDIR failed: " << strerror(errno));
	UT_NOTIFY(LV_INFO, "env dbglogdir: " << _pathLog);

#else /* !_WIN32 */
	{
		std::string s = std::string(GetCurrentProcessId()) + std::string("SHM");
		if (!::SetEnvironmentVariableA("GLSL_DEBUGGER_SHMID", s.c_str()))
		UT_NOTIFY(LV_ERROR, "setenv GLSL_DEBUGGER_SHMID failed: " << ::GetLastError());
		UT_NOTIFY(LV_DEBUG, "env shmid: " << s);
	}

	if (!::SetEnvironmentVariableA("GLSL_DEBUGGER_DBGFCTNS_PATH", _pathDbgFuncs.c_str())) {
		UT_NOTIFY(LV_ERROR, "setenv GLSL_DEBUGGER_DBGFCTNS_PATH failed: " << ::GetLastError());
	}
	UT_NOTIFY(LV_INFO, "env dbgfctns: " << _pathDbgFuncs);

	{
		std::string s(getMaxDebugOutputLevel());
		if (!::SetEnvironmentVariableA("GLSL_DEBUGGER_LOGLEVEL", s.c_str()))
		UT_NOTIFY(LV_ERROR, "setenv GLSL_DEBUGGER_LOGLEVEL failed: " << ::GetLastError());
		UT_NOTIFY(LV_INFO, "env dbglvl: " << s);
	}

	if (!::SetEnvironmentVariableA("GLSL_DEBUGGER_LOGDIR", _pathLog.c_str()))
	UT_NOTIFY(LV_ERROR, "setenv GLSL_DEBUGGER_LOGDIR failed: " << ::GetLastError());
	UT_NOTIFY(LV_INFO, "env dbglogdir: " << _pathLog);
#endif /* !_WIN32 */
}

void Debugger::buildEnvironmentVars()
{
	std::string progpath;
	/* Determine absolute program path. */
#ifdef _WIN32
	{
		HMODULE hModule = ::GetModuleHandle(NULL);
		char *tmp = new char[_MAX_PATH];
		//progpath = static_cast<char *>(::malloc(_MAX_PATH));
		GetModuleFileNameA(hModule, tmp, _MAX_PATH);
		progpath.append(tmp);
	}
#else /* _WIN32 */

	long pathmax = pathconf("/", _PC_PATH_MAX);
	if (pathmax <= 0) {
		UT_NOTIFY(LV_ERROR, "Cannot get max. path length!");
		if (errno != 0)
			UT_NOTIFY(LV_ERROR, strerror(errno));
		else
			UT_NOTIFY(LV_ERROR, "No value set!");
		exit(1);
	}

	{
		std::string s;
		ssize_t progPathLen;
		s += "/proc/" + std::to_string(os_getpid()) + "/exe";
		char *tmp = new char[pathmax];
		progPathLen = readlink(s.c_str(), tmp, pathmax);
		if (progPathLen != -1) {
			tmp[progPathLen] = '\0';
			progpath.append(tmp);
		} else {
			UT_NOTIFY(LV_ERROR, "Unable to retrieve absolute path.");
			exit(1);
		}
		delete[] tmp;
	}

#endif /* _WIN32 */
	UT_NOTIFY(LV_INFO, "Absolute path to debugger executable is: " << progpath);

	/* Remove executable name from path. */
	size_t pos = progpath.find_last_of(OS_PATH_SEPARATOR);
	if (pos != std::string::npos) {
		progpath = progpath.substr(0, pos);
	}

	/* preload library */
	_pathDbgLib.append(progpath);
	_pathDbgLib.append(DEBUGLIB);
	UT_NOTIFY(LV_INFO, "Path to debug library is: " << _pathDbgLib);

	/* path to debug SOs */
	_pathDbgFuncs.append(progpath);
	_pathDbgFuncs.append(DBG_FUNCTIONS_PATH);
	UT_NOTIFY(LV_INFO, "Path to debug functions is: " << _pathDbgFuncs);

	/* dlsym helper library */
	_pathLibDlSym.append(progpath);
	_pathLibDlSym.append(LIBDLSYM);
	UT_NOTIFY(LV_INFO, "Path to libdlsym is: " << _pathLibDlSym);

	/* log dir */
	if (utils_notify_filename(nullptr)) {
		QFileInfo fio(utils_notify_filename(nullptr));
		std::string str = fio.absoluteFilePath().toStdString();
		UT_NOTIFY(LV_INFO, "Log path " << str);
	}
}

void Debugger::initSharedMem(void)
{
#ifdef _WIN32
#define SHMEM_NAME_LEN 64
	char shmemName[SHMEM_NAME_LEN];

	_snprintf(shmemName, SHMEM_NAME_LEN, "%uSHM", GetCurrentProcessId());
	/* this creates a non-inheritable shared memory mapping! */
	_hShMem = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SHM_SIZE, shmemName);
	if (_hShMem == NULL || _hShMem == INVALID_HANDLE_VALUE) {
		UT_NOTIFY(LV_ERROR, "Creation of shared mem segment " << shmemName << " failed: " << GetLastError());
		exit(1);
	}
	/* FILE_MAP_WRITE implies read */
	_records = (debug_record_t*)MapViewOfFile(_hShMem, FILE_MAP_WRITE, 0, 0, SHM_SIZE);
	if (_records == NULL) {
		UT_NOTIFY(LV_ERROR, "View mapping of shared mem segment " << shmemName << " failed: " << GetLastError());
		exit(1);
	}
#undef SHMEM_NAME_LEN
#else /* _WIN32 */
	_shmID = shmget(IPC_PRIVATE, SHM_SIZE, SHM_R | SHM_W);

	if (_shmID == -1) {
		UT_NOTIFY(LV_ERROR,
				"Creation of shared mem segment failed: " << strerror(errno));
		exit(1);
	}

	_records = (debug_record_t*) shmat(_shmID, NULL, 0);

	if ((void*) _records == (void*) -1) {
		UT_NOTIFY(LV_ERROR,
				"Attaching to shared mem segment failed: " << strerror(errno));
		exit(1);
	}
#endif /* _WIN32 */
}

void Debugger::clearSharedMem(void)
{
	memset(_records, 0, SHM_SIZE);
}

void Debugger::freeSharedMem(void)
{
#ifdef _WIN32
	if (_records != NULL) {
		if (!UnmapViewOfFile(_records)) {
			UT_NOTIFY(LV_ERROR, "View unmapping of shared mem segment failed: " << GetLastError());
			exit(1);
		}
		fcalls = NULL;
	}
	if (_hShMem != INVALID_HANDLE_VALUE && _hShMem != NULL) {
		if (CloseHandle(_hShMem) == 0) {
			UT_NOTIFY(LV_ERROR, "Closing handle of shared mem segment failed: " << GetLastError());
			exit(1);
		}
		_hShMem = INVALID_HANDLE_VALUE;
	}
#else /* _WIN32 */
	shmctl(_shmID, IPC_RMID, 0);

	if (shmdt(_records) == -1) {
		UT_NOTIFY(LV_ERROR,
				"Deleting shared mem segment failed: " << strerror(errno));
	}
#endif /* _WIN32 */
}

#ifdef _WIN32
void Debugger::createEvents(const DWORD processId) {
#define EVENT_NAME_LEN (32)
	wchar_t eventName[EVENT_NAME_LEN];

	this->closeEvents();

	_snwprintf(eventName, EVENT_NAME_LEN, L"%udbgee", processId);
	_hEvtDebuggee = ::CreateEventW(NULL, FALSE, FALSE, eventName);
	if (_hEvtDebuggee == NULL) {
		dbgPrint(DBGLVL_ERROR, "creating %s failed\n", eventName);
		::exit(1);
	}

	_snwprintf(eventName, EVENT_NAME_LEN, L"%udbgr", processId);
	_hEvtDebugger = ::CreateEventW(NULL, FALSE, FALSE, eventName);
	if (_hEvtDebugger == NULL) {
		dbgPrint(DBGLVL_ERROR, "creating %s failed\n", eventName);
		::exit(1);
	}
}

void Debugger::closeEvents(void) {
	dbgPrint(DBGLVL_INFO, "Closing events ...\n");

	if (_hEvtDebugger != NULL) {
		::CloseHandle(_hEvtDebugger);
		_hEvtDebugger = NULL;
	}

	if (_hEvtDebuggee != NULL) {
		::CloseHandle(_hEvtDebuggee);
		_hEvtDebuggee = NULL;
	}

	dbgPrint(DBGLVL_INFO, "Events closed.\n");
}

#endif /* _WIN32 */

const QString& Debugger::strRunLevel(RunLevel rl)
{
	switch(rl) {
		case INIT: 
		{ static const QString m("init"); return m; }
		case SETUP: 
		{ static const QString m("setup"); return m; }
		case TRACE_EXECUTE: 
		{ static const QString m("execute"); return m; }
		case TRACE_EXECUTE_NO_DEBUGABLE: 
		{ static const QString m("execute_no_debugable"); return m; }
		case TRACE_EXECUTE_IS_DEBUGABLE: 
		{ static const QString m("execute_is_debugable"); return m; }
		case TRACE_EXECUTE_RUN: 
		{ static const QString m("execute_run"); return m; }
		case DBG_RECORD_DRAWCALL: 
		{ static const QString m("record drawcall"); return m; }
		case DBG_VERTEX_SHADER: 
		{ static const QString m("vertex shader"); return m; }
		case DBG_GEOMETRY_SHADER: 
		{ static const QString m("geometry shader"); return m; }
		case DBG_FRAGMENT_SHADER: 
		{ static const QString m("fragment shader"); return m; }
		case DBG_RESTART: 
		{ static const QString m("restart"); return m; }
		default:
		{ static const QString m("unknown runlevel"); return m; }
	}
	static const QString dummy;
	return dummy;
}

void Debugger::childBecameParent(os_pid_t p)
{
	UT_NOTIFY(LV_INFO, "Child forked!");
}

// void Debugger::setRunLevel(RunLevel rl)
// {
// 	UT_NOTIFY(LV_INFO, "new level " << rl << "(" << strRunLevel(rl) << ") "
// 		<< (_currentCall ? _currentCall->name().toStdString().c_str() : NULL));

// 	switch (rl) {
// 		case RL_INIT: 
// 			break;
// 		case RL_SETUP:
// 			break;
// 		case RL_TRACE_EXECUTE:  // Trace is running in step mode
// 			/* choose sub-level */
// 			if (_currentCall && _currentCall->isDebuggable(&m_primitiveMode)
// 					&& m_bHaveValidShaderCode) {
// 				setRunLevel(RL_TRACE_EXECUTE_IS_DEBUGABLE);
// 			} else {
// 				setRunLevel(RL_TRACE_EXECUTE_NO_DEBUGABLE);
// 			}
// 			break;
// 		case RL_TRACE_EXECUTE_NO_DEBUGABLE:  // sub-level for non debuggable calls
// 			break;
// 		case RL_TRACE_EXECUTE_IS_DEBUGABLE:  // sub-level for debuggable calls
// 			break;
// 		case RL_TRACE_EXECUTE_RUN:
// 			break;
// 		case RL_DBG_RECORD_DRAWCALL:
// 			break;
// 		case RL_DBG_VERTEX_SHADER:
// 		case RL_DBG_GEOMETRY_SHADER:
// 		case RL_DBG_FRAGMENT_SHADER:
// 			break;
// 		case RL_DBG_RESTART:
// 			break;

// 		default:
// 			break;
// 	}
// }

debug_record_t* Debugger::debugRecord(os_pid_t pid)
{
	int i;
	debug_record_t *rec = nullptr;
	for (i = 0; i < SHM_MAX_THREADS; ++i) {
		if (_records[i].threadId == 0 || _records[i].threadId == pid) {
	        rec = &_records[i];
	        break;
		}
	}
	if(i >= SHM_MAX_THREADS) {
		UT_NOTIFY(LV_ERROR, "max. number of debuggable threads exceeded!");
	    exit(1);
	}
	return rec;
}
void Debugger::waitForEndOfExecution()
{
// 	ErrorCode error;
// 	while (_currentRunLevel == RL_TRACE_EXECUTE_RUN) {
// 		qApp->processEvents(QEventLoop::AllEvents);
// #ifndef _WIN32
// 		usleep(1000);
// #else /* !_WIN32 */
// 		Sleep(1);
// #endif /* !_WIN32 */
// 		int state;
// 		error = _dbg->checkExecuteState(&state);
// 		if (isErrorCritical(error)) {
// 			killDebuggee();
// 			setRunLevel(RL_SETUP);
// 			return;
// 		} else if (isOpenGLError(error)) {
// 			/* TODO: error check */
// 			_dbg->executeContinueOnError();
// 			_dbg->waitForChildStatus();
// 			_currentCall = _dbg->getCurrentCall();
// 			addGlTraceItem();
// 			setErrorStatus(error);
// 			_dbg->callDone();
// 			error = getNextCall();
// 			setErrorStatus(error);
// 			if (isErrorCritical(error)) {
// 				killDebuggee();
// 				setRunLevel(RL_SETUP);
// 				return;
// 			} else {
// 				setRunLevel(RL_TRACE_EXECUTE);
// 				addGlTraceItem();
// 			}
// 			break;
// 		}
// 		if (state) {
// 			break;
// 		}
// 		if (!_dbg->childAlive()) {
// 			killDebuggee();
// 			setRunLevel(RL_SETUP);
// 			return;
// 		}
// 	}
// 	if (_currentRunLevel == RL_TRACE_EXECUTE_RUN) {
// 		_dbg->waitForChildStatus();
// 		error = getNextCall();
// 		setRunLevel(RL_TRACE_EXECUTE);
// 		setErrorStatus(error);
// 		if (isErrorCritical(error)) {
// 			killDebuggee();
// 			setRunLevel(RL_SETUP);
// 			return;
// 		} else {
// 			addGlTraceItem();
// 		}
// 	}
}
