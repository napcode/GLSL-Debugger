#ifndef DEBUGGER_H
#define DEBUGGER_H 1

#include <QtCore/QObject>

#include "Process.qt.h"
#include "build-config.h"
#include <thread>

class Debugger : public QObject
{
	Q_OBJECT
public:
	/* types */
	enum RunLevel {
	INIT,
	SETUP,
	TRACE_EXECUTE,
	TRACE_EXECUTE_NO_DEBUGABLE,
	TRACE_EXECUTE_IS_DEBUGABLE,
	TRACE_EXECUTE_RUN,
	DBG_RECORD_DRAWCALL,
	DBG_VERTEX_SHADER,
	DBG_GEOMETRY_SHADER,
	DBG_FRAGMENT_SHADER,
	DBG_RESTART
	};
public:
	/* methods */
	static Debugger& instance();
	static void release();

	/* (re-)initialize the debugger, kills running debuggees */
	void init();
	/* end all debugging sessions and wait for them to finish */
	void shutdown();
	/* create a local debuggee. does not launch or anything */
	ProcessPtr create(const DebugConfig& cfg);

	/* connect to a running debuggee */
	ProcessPtr connectTo(QString& path, int port = DEFAULT_SERVER_PORT_TCP, int timeout_s = NETWORK_TIMEOUT);

	void waitForEndOfExecution();

	const ProcessList& processes() const { return _processes; }

	static const QString& strRunLevel(RunLevel rl);
	RunLevel runLevel() const { return _runLevel; }

	const std::string& clientName() { return _client_name; }
	void clientName(const std::string& client_name ) { _client_name = client_name; }
signals:
	void message(const QString& msg);
	void runLevelChanged(RunLevel rl);
	void newChild();
private:
	/* variables */
	static Debugger *_instance;
	std::string _client_name;
	ProcessList _processes;
	RunLevel _runLevel;
	std::string _pathDbgLib;
	std::string _pathDbgFuncs;
	std::string _pathLibDlSym;
	std::string _pathLog;
    std::mutex _mtxCmd;
    bool _end;
#ifdef GLSLDB_WIN
	HANDLE _hEvtDebuggee;
	HANDLE _hEvtDebugger;
	HANDLE _hDebuggedProgram;
	ATTACHMENT_INFORMATION _ai;
	HANDLE _hShMem;
#endif

private:
	/* methods */
	Debugger();
	~Debugger();

	void setEnvironmentVars();
	void buildEnvironmentVars();

	void initSharedMem();
	void freeSharedMem();
	void clearSharedMem();
    void run();
};

#endif //DEBUGGER_H
