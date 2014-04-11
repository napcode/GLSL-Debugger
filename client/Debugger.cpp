#include "Debugger.qt.h"
#include "utils/notify.h"

#include <QtCore/QFileInfo>
#include <QtNetwork/QTcpSocket>

#include "proto/protocol.h"
#include "build-config.h"
#include "utils/os/os.h"
#ifndef GSLDLB_WIN
#	include <unistd.h>
#	include <sys/shm.h>
#endif

Debugger* Debugger::_instance = nullptr;

Debugger& Debugger::instance()
{
	GOOGLE_PROTOBUF_VERIFY_VERSION;

	if(!_instance) {
		_instance = new Debugger();
	}
	return *_instance;
}

void Debugger::release()
{
	delete _instance;
	_instance = nullptr;
	google::protobuf::ShutdownProtobufLibrary();
}

Debugger::Debugger() : 
	_client_name("unknown"), 
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
    setEnvironmentVars();
    _end = false;
  //  if(!_server.listen(QHostAddress::Any, DEFAULT_SERVER_PORT_TCP))
   // 	throw std::runtime_error("Unable to start server on port " + SERVER_PORT);
  //  connect(&_server, SIGNAL(newConnection()), this, SLOT(newDebuggeeConnection()));
}
ProcessPtr Debugger::connectTo(QString& path, int port, int timeout)
{
    ConnectionPtr con(new TcpConnection(path, port, timeout));
	ProcessPtr p(new Process(con));
	p->init();
	_processes.push_back(p);
	return p;
}
void Debugger::shutdown()
{
	_processes.clear();
}

ProcessPtr Debugger::create(const DebugConfig& cfg)
{
	ProcessPtr p(new Process(cfg));
	_processes.push_back(p);
	return p;
}

void Debugger::setEnvironmentVars(void)
{
#ifndef _WIN32
	if (setenv("LD_PRELOAD", _pathDbgLib.c_str(), 1))
		UT_NOTIFY(LV_ERROR, "setenv LD_PRELOAD failed: " << strerror(errno));
	UT_NOTIFY(LV_INFO, "env dbglib: " << _pathDbgLib);


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
