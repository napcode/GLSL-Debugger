#ifndef PROCESS_H
#define PROCESS_H 1

#include <QtCore/QStringList>
#include <QtCore/QList>
#include <QtCore/QSharedPointer>

#include "utils/os/os.h"
#include "Connection.h"
#include "DebugConfig.h"
#include "FunctionCall.h"
#include "CommandFactory.h"
#include "ResultHandler.h"


class Debugger;
class CommandFactory;
typedef QSharedPointer<proto::ServerResponse> ServerResponsePtr;

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
	Process(ConnectionPtr c);
	Process(const DebugConfig& config, os_pid_t pid = 0);
	~Process();

	const DebugConfig& config() { return _debugConfig; }
    bool init();

	/* check if the child is alive */
	bool isAlive();
	/* continue the process */
	void advance();

	CommandPtr announce();
	CommandPtr step();
	/* kill the process */
	CommandPtr kill();
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

	/* retrieve the current/last call */
	FunctionCallPtr currentCall(void);

	bool connected() const { return !_connection ? false : true; }
	void connection(ConnectionPtr c) { _connection = c; }
	ConnectionPtr connection() const { return _connection; }

	void copyArgumentTo(void *dst, void *src, DBG_TYPE type);
	void* copyArgumentFrom(void *addr, DBG_TYPE type);

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
	ResultHandlerPtr resultHandler() const { return _resultHandler; }
	/* set a ResultHandler. Process takes ownership */
	void resultHandler(ResultHandler* res) { _resultHandler = ResultHandlerPtr(res); }
signals:
	void stateChanged(Process::State s);
	void newChild(os_pid_t p);
	void resultAvailable(CommandPtr);
private:
	/* methods */
	void state(State s);
	void config(const DebugConfig& cfg) { _debugConfig = cfg; }
    void startReceiver();
    void stopReceiver();
    void receiveResults();
    void storeCommand(CommandPtr& cmd);
    ServerResponsePtr readResponse(QByteArray& ba);
    //ServerResponsePtr readEvent(QByteArray& ba);
private:
	/* variables */
	os_pid_t _pid;
    std::thread *_responseReceiver;
    std::mutex _mtxWork;
    std::condition_variable _workCondition;

	CommandList _commands;

    DebugConfig _debugConfig;
    State _state;
	ConnectionPtr _connection;
    bool _end;
    ResultHandlerPtr _resultHandler;
};

typedef QSharedPointer<Process> ProcessPtr;
typedef QList<ProcessPtr> ProcessList;

#endif
