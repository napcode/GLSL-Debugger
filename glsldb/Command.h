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
	Command(Process& p);
	virtual ~Command();

	void debugRecord(const debug_record_t *r);
	const debug_record_t& debugRecord() const { return _rec; }

	/* execution low level implementation */
	virtual void operator()();
	const QString& message() const { return *_message; }

	/* these just modified the shared memory */ 
	//void overwriteFuncArguments(FunctionCallPtr call);

	std::future<ResultPtr> result() { return _result.get_future(); }

	/* these methods handle Command results */
	//void queryResult();
	Process& process() const { return _proc; }
protected:
	Process& _proc;
	debug_record_t _rec;
	const QString* _message;
	std::promise<ResultPtr> _result;

protected:
	std::promise<ResultPtr>& promise() { return _result; }
};
typedef QSharedPointer<Command> CommandPtr;
typedef QQueue<CommandPtr> CommandQueue;

#endif