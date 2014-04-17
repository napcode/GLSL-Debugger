#ifndef COMMANDIMPL_H
#define COMMANDIMPL_H 1

#include "Command.qt.h"
#include "DebugCommand.h"
#include "Process.qt.h"
#include "FunctionCall.h"

struct AnnounceCommand : public Command
{
	AnnounceCommand(Process& p, const std::string& client_name );
	void result(const proto::ServerMessage& response);
	std::string _client_name;
};
struct CallCommand : public Command
{
	CallCommand(Process& p);
	void result(const proto::ServerMessage& response);
};
struct DoneCommand : public Command
{
	DoneCommand(Process& p);
	void result(const proto::ServerMessage& response);
};
struct NextCommand : public Command
{
	NextCommand(Process& p);
	void result(const proto::ServerMessage& response);
};
struct GetCallCommand : public Command
{
	struct Result : public Command::Result
	{
		Result(FunctionCallPtr f)
			: functionCall(f)
		{}
		virtual ~Result() {};
		FunctionCallPtr functionCall;
	};
	void result(const proto::ServerMessage& response);
};


#endif
