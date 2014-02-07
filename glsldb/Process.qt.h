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

class Debugger;
class CommandFactory;

class Process : public QObject
{
	Q_OBJECT
public:
	/* types */
	/* Reminder: change strState() if enum changes */
	enum State {
		INVALID, 	/* no debuggee attached */
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
	Process(const DebugConfig& config, PID_T pid = 0);
	~Process();

	const DebugConfig& config() { return _debugConfig; }

	/* launches process, applies options if needed & leaves
	 * process in STOPPED state if everything went well
	 */
	void launch(const DebugConfig* config = nullptr);

	/* continue the process */
	void advance();

	/* continue the process */
	void kill();

	/* halt/stop
	 * @param immediately send SIGSTOP instead of waiting for next GLCall
	 */
	void halt(bool immediately = false);

	/* synchroneously waits for status change */
	void waitForStatus();

	/* check if the child is alive */
	bool isAlive();

	/* get or create a command factory
	 * factory is associated with *this*
	 */
	CommandFactory& commandFactory();

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
signals:
	void stateChanged(State s);
	void newChild(PID_T p);
private:
	/* methods */
	void queryTraceEvent(PID_T pid, int status);
	void updateDebugRecord(PID_T pid);
	void state(State s);
	void config(const DebugConfig& cfg) { _debugConfig = cfg; }

private:
	/* variables */
	PID_T _pid;
    DebugConfig _debugConfig;
    State _state;
    CommandFactoryPtr _cmdFactory;
	debug_record_t *_rec;
};

typedef QSharedPointer<Process> ProcessPtr;
typedef QList<ProcessPtr> ProcessList;

#endif