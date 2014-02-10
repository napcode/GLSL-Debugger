#ifndef DEBUGGER_H
#define DEBUGGER_H 1

#include <QtCore/QObject>

#include "Process.qt.h"
#include "Command.h"
#include <thread>

class Error {
	/* Error class describing error msg, error code
	 * and debuggee information (or whether it comes from 
	 * the debugger itself)
	 */
	public:
		//origin();
		//pid();
		//message();
		//code();
};

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
	ProcessPtr create(const DebugConfig& cfg);

	void waitForEndOfExecution();

	const ProcessList& processes() const { return _processes; }

	static const QString& strRunLevel(RunLevel rl);
	RunLevel runLevel() const { return _runLevel; }

	debug_record_t* debugRecord(os_pid_t pid);

	void enqueue(CommandPtr cmd);
public slots:
	void childBecameParent(os_pid_t p);

signals:
	void message(const QString& msg);
	void runLevelChanged(RunLevel rl);
	void error(Error e);
	void newChild();
private:
	/* variables */
	static Debugger *_instance;
	int _shmID;
	ProcessList _processes;
	RunLevel _runLevel;
	debug_record_t *_records;
	CommandQueue _queue;
	std::string _pathDbgLib;
	std::string _pathDbgFuncs;
	std::string _pathLibDlSym;
	std::string _pathLog;
    std::thread *_worker;
    std::mutex _mtxWork;
    std::mutex _mtxCmd;
    std::condition_variable _workCondition;
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
