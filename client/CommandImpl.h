#ifndef COMMANDIMPL_H
#define COMMANDIMPL_H 1

#include "Command.qt.h"
#include "DebugCommand.h"
#include "Process.qt.h"
#include "FunctionCall.h"

struct AnnounceCommand : public Command
{
	AnnounceCommand(Process& p, const std::string& client_name );
	void result(const proto::ServerResponse& response);
	std::string _client_name;
};
struct StepCommand : public Command
{
	StepCommand(Process& p, bool trace = true, bool stopOnGlError = true);
	void result(const proto::ServerResponse& response);
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
	void result(const proto::ServerResponse& response);
};


#endif
