#ifndef COMMANDIMPL_H
#define COMMANDIMPL_H 1

#include "Command.qt.h"
#include "DebugCommand.h"
#include "Process.qt.h"

struct ExecuteCommand : public SimpleCommand
{
	ExecuteCommand(Process& p, bool stopOnGLError);
};
struct StepCommand : public DebugCommand
{
	StepCommand(Process& p);
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