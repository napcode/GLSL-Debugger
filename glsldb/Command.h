#ifndef COMMAND_H
#define COMMAND_H 1

#include <QtCore/QString>
#include <QtCore/QQueue>
#include <QtCore/QSharedPointer>

#include "debuglib.h"
#include "functionCall.h"
#include <future>

class Process;
class Command
{
public:
	/* types */
	struct Result
	{
		Result(bool v)
			: valid(v)
		{}
		virtual ~Result() {};
		bool valid;
	};
	typedef QSharedPointer<Command::Result> ResultPtr;

public:
	Command(Process& p, const QString& msg = _dummy) :
		_proc(p), 
		_message(&msg)
	{};
	virtual ~Command()
	{};

	/* execution low level implementation */
	virtual void operator()() = 0;
	const QString& message() const { return *_message; }

	/* these just modified the shared memory */ 
	//void overwriteFuncArguments(FunctionCallPtr call);

	/* retrieve the result. can only be done once */
	std::future<ResultPtr> result() { return _result.get_future(); }

	/* these methods handle Command results */
	//void queryResult();
	Process& process() const { return _proc; }
protected:
	Process& _proc;
	static const QString _dummy;
	const QString* _message;
	std::promise<ResultPtr> _result;

protected:
	std::promise<ResultPtr>& promise() { return _result; }
};
typedef QSharedPointer<Command> CommandPtr;
typedef QQueue<CommandPtr> CommandQueue;
typedef std::future<Command::ResultPtr> FutureResult;

#endif