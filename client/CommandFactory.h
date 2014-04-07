#ifndef COMMANDFACTORY_H
#define COMMANDFACTORY_H 1

#include <future>
#include <QtCore/QSharedPointer>

#include "Command.qt.h"
#include "FunctionCall.h"

class DebugConfig;
class CommandFactory
{
public:
	CommandFactory(Process& p) : 
		_proc(p)
	{}
	/* Commands */
	CommandPtr checkTrapEvent(os_pid_t p, int s);
	CommandPtr launch();

	/* DebugCommands */
	CommandPtr execute(bool stopOnGLError);
	CommandPtr step(bool trace = true);
	CommandPtr stepToDrawCall(bool trace = true, bool stopOnGlError = true);
	CommandPtr stepToShaderSwitch(bool trace = true, bool stopOnGlError = true);
	CommandPtr stepToUserDefined(FunctionCallPtr func, bool trace = true, bool stopOnGlError = true);
	
		/* these commands need to CONT the process in order to be effectual*/
	/*void dbgExecute(bool stopOnGLError);
	void dbgExecuteToDrawCall(bool stopOnGLError);
	void dbgExecuteToShaderSwitch(bool stopOnGLError);
	void dbgExecuteToUserDefined(const QString& fname, bool stopOnGLError);
	void dbgDone();
	void dbgSetDbgTarget(int target, int alphaTestOption, 
		int depthTestOption, int stencilTestOption,	int blendingOption);
	void dbgRestoreRenderTarget(int target);
	void dbgSaveAndInterruptQueries(void);
	void dbgRestartQueries(void);
	void dbgStartRecording();
	void dbgReplay(int);
	void dbgEndReplay();
	void dbgRecord();
	void dbgCallOrig(FunctionCallPtr call);
	void dbgCallDBGFunction(const QString& fname);
	void dbgFreeMem(unsigned int numBlocks, void **addresses);
	void dbgAllocMem(unsigned int numBlocks, unsigned int *sizes, 
		void **addresses);
	void dbgClearRenderBuffer(int mode, float r, float g, float b, 
		float a, float f, int s);
	void dbgReadRenderBuffer(int numComponents, int *width, int *height, 
		float **image);
		*/
private:
	Process &_proc;
};
typedef QSharedPointer<CommandFactory> CommandFactoryPtr;
#endif
