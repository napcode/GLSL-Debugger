#ifndef COMMANDIMPL_H
#define COMMANDIMPL_H 1

#include "Command.qt.h"
#include "DebugCommand.h"
#include "Process.qt.h"
#include "functionCall.h"

struct ExecuteCommand : public SimpleCommand
{
	ExecuteCommand(Process& p, bool stopOnGLError);
};
struct StepCommand : public DebugCommand
{
	struct Result : public Command::Result
	{
		Result(FunctionCallPtr f)
			: functionCall(f)
		{}
		virtual ~Result() {};
		FunctionCallPtr functionCall;
	};
	StepCommand(Process& p);
	void operator()();
};
struct LaunchCommand : public Command
{
	LaunchCommand(Process& p);
	void operator()();
};
struct CheckTrapCommand : public Command
{
	CheckTrapCommand(Process& p, os_pid_t, int);
	void operator()();
	os_pid_t _pid;
	int _status;
};

#endif
