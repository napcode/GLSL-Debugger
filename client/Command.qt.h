#ifndef COMMAND_H
#define COMMAND_H 1

#include <QtCore/QString>
#include <QtCore/QQueue>
#include <QtCore/QSharedPointer>

#include "debuglib.h"
#include "functionCall.h"
#include "notify.h"
#include <future>

class Process;
class Command : public QObject
{
	Q_OBJECT
public:
	/* types */
	struct Result
	{
		Result() {}
		virtual ~Result() {};		
	};
	typedef QSharedPointer<Command::Result> ResultPtr;

public:
	Command(Process& p, const QString& name = QString("DEFAULT")) :
		_proc(p), 
		_name(name)
	{};
	virtual ~Command()
	{
		UT_NOTIFY(LV_TRACE, "~Command " << name().toStdString());
	}

	/* execution low level implementation */
	virtual void operator()() = 0;
	const QString& name() const { return _name; }

	/* these just modified the shared memory */ 
	//void overwriteFuncArguments(FunctionCallPtr call);

	/* retrieve the result. can only be done once */
	std::future<ResultPtr> result() { return _result.get_future(); }

	/* these methods handle Command results */
	//void queryResult();
	Process& process() const { return _proc; }

protected:
	Process& _proc;
	const QString _name;
	std::promise<ResultPtr> _result;

protected:
	std::promise<ResultPtr>& promise() { return _result; }
};
typedef QSharedPointer<Command> CommandPtr;
typedef QQueue<CommandPtr> CommandQueue;
typedef std::future<Command::ResultPtr> FutureResult;

#endif
