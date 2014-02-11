#ifndef PROCESS_H
#define PROCESS_H 1

#include <QtCore/QStringList>
#include <QtCore/QList>
#include <QSharedPointer>

#include "os/os.h"
#include "DebugLib/debuglib.h"
#include "DebugConfig.h"
#include "functionCall.h"
#include "CommandFactory.h"
#include "ResultHandler.h"


class Debugger;
class CommandFactory;

class Process : public QObject
{
	Q_OBJECT
#	include "ProcessFriends.inc.h"
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
	Process(const DebugConfig& config, os_pid_t pid = 0);
	~Process();

	const DebugConfig& config() { return _debugConfig; }

	/* check if the child is alive */
	bool isAlive();

	/* get or create a command factory
	 * factory is associated with *this*
	 */
	CommandFactory& commandFactory() { return *_cmdFactory; }
	/* get thread record. might be a nullptr
	 * update is done on SIGSTOPs
	 */
	debug_record_t* debugRecord() const { return _rec; }

	/* retrieve the current/last call */
	FunctionCallPtr getCurrentCall(void);

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
private:
	/* methods */
	bool checkTrapEvent(os_pid_t pid, int status);
	void state(State s);
	void config(const DebugConfig& cfg) { _debugConfig = cfg; }
    void init();
    void handleStatusUpdates();
    void startStatusHandler();
    void stopStatusHandler();

	/* continue the process */
	void advance();

	/* kill the process */
	void kill();
	/* launches process, applies options if needed & leaves
	 * process in STOPPED state if everything went well
	 */
	void launch();

	/* synchroneously waits for status change */
	void waitForStatus(bool checkTrap = false);

	/* halt/stop
	 * @param immediately send SIGSTOP instead of waiting for next GLCall
	 */
	void stop(bool immediately = false);

private:
	/* variables */
	os_pid_t _pid;
	debug_record_t *_rec;
    std::thread *_statusHandler;
    DebugConfig _debugConfig;
    State _state;
    CommandFactoryPtr _cmdFactory;
    bool _end;
    ResultHandlerPtr _resultHandler;
};

typedef QSharedPointer<Process> ProcessPtr;
typedef QList<ProcessPtr> ProcessList;

#endif
