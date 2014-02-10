#ifndef COMMANDIMPL_H
#define COMMANDIMPL_H 1

#include "Command.h"
#include "DebugCommand.h"
#include "Process.qt.h"

struct ExecuteCommand : public DebugCommand
{
	ExecuteCommand(Process& p, bool step, bool stopOnGLError);
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
struct AdvanceCommand : public Command
{
	AdvanceCommand(Process& p);
	void operator()();
};
#endif