#ifndef DebugCommand_H
#define DebugCommand_H 1

#include "Command.qt.h"

class DebugCommand : public Command
{
public:
	DebugCommand(Process& p, const QString& msg = QString("DEFAULT_DEBUGCMD"));
	virtual ~DebugCommand();

	/* execution low level implementation */
	virtual void operator()();

	/* these just modified the shared memory */ 
	//void overwriteFuncArguments(FunctionCallPtr call);
protected:
	debug_record_t *_rec;
};

class SimpleCommand : public DebugCommand
{
public:
	SimpleCommand(Process& p, const QString& msg = QString("SIMPLE_DEBUGCMD"));
	virtual ~SimpleCommand();
	void operator()();
};

#endif
