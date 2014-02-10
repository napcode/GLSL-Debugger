#ifndef DebugCommand_H
#define DebugCommand_H 1

#include "Command.h"

class DebugCommand : public Command
{
public:
	DebugCommand(Process& p, const QString& msg = _dummy);
	virtual ~DebugCommand();

	void debugRecord(const debug_record_t *r);
	const debug_record_t& debugRecord() const { return *_rec; }

	/* execution low level implementation */
	virtual void operator()();

	/* these just modified the shared memory */ 
	//void overwriteFuncArguments(FunctionCallPtr call);
protected:
	debug_record_t *_rec;

};

#endif
