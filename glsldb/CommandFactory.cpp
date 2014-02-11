#include "CommandFactory.h"
#include "Debugger.qt.h"
#include "CommandImpl.h"

CommandPtr CommandFactory::launch()
{
	CommandPtr cmd(new LaunchCommand(_proc));
	Debugger::instance().enqueue(cmd);
	return cmd;
}
CommandPtr CommandFactory::checkTrapEvent(os_pid_t p, int s)
{
	CommandPtr cmd(new CheckTrapCommand(_proc, p, s));
	Debugger::instance().enqueue(cmd);
	return cmd;
}

CommandPtr CommandFactory::execute(bool step, bool stopOnGLError)
{
	CommandPtr cmd(new ExecuteCommand(_proc, step, stopOnGLError));
	Debugger::instance().enqueue(cmd);
	return cmd;
}
