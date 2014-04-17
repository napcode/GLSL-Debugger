#ifndef COMMAND_H
#define COMMAND_H 1

#include <QtCore/QString>
#include <QtCore/QQueue>
#include <QtCore/QList>
#include <QtCore/QSharedPointer>

#include "FunctionCall.h"
#include "utils/notify.h"
#include "proto/protocol.h"
#include <future>
#include <atomic>

class Process;
class Command : public QObject
{
	Q_OBJECT
public:
	/* types */
	struct Result
	{
		Result() : ok(true), message(std::string()) {}
		Result(bool done, const std::string& msg = std::string()) : 
			ok(done), 
			message(msg)
		{}
		bool ok;
		std::string message;
		virtual ~Result() {};	
	};
	typedef QSharedPointer<Command::Result> ResultPtr;

public:
	Command(Process& p, proto::ClientRequest::Type t);
	virtual ~Command() {};

	virtual void result(const proto::ServerMessage& response) = 0;

	bool operator==(uint64_t response_id) { return id() == response_id; }

	/* these just modified the shared memory */ 
	//void overwriteFuncArguments(FunctionCallPtr call);

	/* retrieve the result. can only be done once */
	std::future<ResultPtr> result() { return _result.get_future(); }

	/* these methods handle Command results */
	//void queryResult();
	Process& process() const { return _proc; }
	uint64_t id() const { return _message.id(); }
	const proto::ClientRequest& message() const { return _message; }
protected:
	proto::ClientRequest _message;
	Process& _proc;
	std::promise<ResultPtr> _result;
	static std::atomic<uint64_t> _seq_number;
protected:
	std::promise<ResultPtr>& promise() { return _result; }
};
typedef QSharedPointer<Command> CommandPtr;
typedef QQueue<CommandPtr> CommandQueue;
typedef QList<CommandPtr> CommandList;
typedef std::future<Command::ResultPtr> FutureResult;

#endif
