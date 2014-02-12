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

CommandPtr CommandFactory::execute(bool stopOnGLError)
{
	CommandPtr cmd(new ExecuteCommand(_proc, stopOnGLError));
	Debugger::instance().enqueue(cmd);
	return cmd;
}
CommandPtr CommandFactory::step(bool trace)
{
	CommandPtr cmd(new StepCommand(_proc, trace));
	Debugger::instance().enqueue(cmd);
	return cmd;
}
