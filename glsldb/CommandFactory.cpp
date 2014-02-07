#include "CommandFactory.h"
#include "Debugger.qt.h"
#include "CommandImpl.h"

std::future<Command::ResultPtr> CommandFactory::dbgExecute(bool stopOnGLError)
{
	CommandPtr cmd(new ExecuteCommand(_proc, stopOnGLError));
	Debugger::instance().enqueue(cmd);
	return cmd->result();
}