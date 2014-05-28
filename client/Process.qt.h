#ifndef PROCESS_H
#define PROCESS_H 1

#include <QtCore/QStringList>
#include <QtCore/QList>
#include <QtCore/QSharedPointer>

#include "utils/os/os.h"
#include "Connection.qt.h"
#include "DebugConfig.h"
#include "FunctionCall.h"
#include "MessageHandler.h"

class Debugger;
class CommandFactory;

class Process : public QObject
{
	Q_OBJECT
public:
	/* types */
	/* Reminder: change strState() if enum changes */
	enum State {
		INVALID, 	/* no valid debuggee attached */
		INIT,  		/* initialized but not yet executing */
		RUNNING, 	/* running */
		STOPPED,		/* stopped (by SIG***) */
		TRAPPED,		/* stopped (by SIGTRAP), fork & friends */
		EXITED, 		/* exited normally */
		KILLED		/* killed explicitely or by signal */
	};

public:
	/* methods */
	Process();
	Process(IPCConnectionPtr c);
	Process(const DebugConfig& config, os_pid_t pid = 0);
	~Process();

	const DebugConfig& config() { return _debugConfig; }
    void init();

	/* check if the child is alive */
	bool isAlive();
	/* continue the process */
	void advance();

	msg::MessagePtr done(void);
	msg::MessagePtr call(void);
	/* kill the process */
	msg::MessagePtr kill(void);
	/* retrieve the current/last call */
	msg::MessagePtr currentCall(void);
	msg::MessagePtr execute(proto::ExecutionDetails_Operation op = proto::ExecutionDetails_Operation_STEP, const QString& param = QString() );
	/* launches process, applies options if needed & leaves
	 * process in STOPPED state if everything went well
	 */
	void launchLocal();

	/* synchroneously waits for status change */
	void waitForStatus(bool checkTrap = false);

	/* halt/stop
	 * @param immediately send SIGSTOP instead of waiting for next GLCall
	 */
	void stop(bool immediately = false);



	bool connected() const { return !_connection ? false : true; }
	void connection(IPCConnectionPtr c) { _connection = c; }
	IPCConnectionPtr connection() const { return _connection; }

	//void copyArgumentTo(void *dst, void *src, DBG_TYPE type);
	//void* copyArgumentFrom(void *addr, DBG_TYPE type);

	/* get a string representation of the state */
	static const QString& strState(State s);

	State state() const { return _state; }

    bool isInit() const { return _state == INIT; }
    bool isInvalid() const { return _state == INVALID; }
    bool isRunning() const { return _state == RUNNING; }
    bool isTrapped() const { return _state == TRAPPED; }
    bool isStopped() const { return _state == STOPPED; }
    bool isKilled() const { return _state == KILLED; }
    bool hasExited() const { return _state == EXITED; }

	msg::MessageHandlerPtr messageHandler() const { return _messageHandler; }
	/* set a ResultHandler. Process takes ownership */
	void messageHandler(msg::MessageHandler* res) { _messageHandler = msg::MessageHandlerPtr(res); }

signals:
	void stateChanged(Process::State s);
	void newChild(os_pid_t p);
private:
	/* methods */
	void state(State s);
	void config(const DebugConfig& cfg) { _debugConfig = cfg; }
private slots:
	void newResponseSlot(msg::MessagePtr&);
	void errorSlot(QString);
private:
	/* variables */
	os_pid_t _pid;

    DebugConfig _debugConfig;
    State _state;
	IPCConnectionPtr _connection;
    bool _end;
   	msg::MessageHandlerPtr _messageHandler;
};

typedef QSharedPointer<Process> ProcessPtr;
typedef QList<ProcessPtr> ProcessList;

#endif
