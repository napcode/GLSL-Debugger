#ifndef COMMANDIMPL_H
#define COMMANDIMPL_H 1

#include "Command.h"
#include "Process.qt.h"

struct ExecuteCommand : public Command
{
	struct Result : public Command::Result
	{
		Result(bool v, int i)
			: Command::Result(v), value(i)
		{}
		int value;
	};
	ExecuteCommand(Process& p, bool stopOnGLError);
	void operator()();
};
#endif