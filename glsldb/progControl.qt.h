/******************************************************************************

Copyright (C) 2006-2009 Institute for Visualization and Interactive Systems
(VIS), Universität Stuttgart.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice, this
    list of conditions and the following disclaimer in the documentation and/or
    other materials provided with the distribution.

  * Neither the name of the name of VIS, Universität Stuttgart nor the names
    of its contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#ifndef _PROG_CONTROL_H_
#define _PROG_CONTROL_H_

#ifdef _WIN32
#include <windows.h>
#else /* _WIN32 */
#include <sys/wait.h>
#endif /* _WIN32 */
#include "errorCodes.h"
#include "runLevel.h"
#include "debugConfig.h"
#include "functionCall.h"
#include "ResourceLimits.h"
#include "attachToProcess.qt.h"

#include <QtCore/QStringList>

extern "C" {
#include "GL/gl.h"
#include "GL/glext.h"
#include "debuglib.h"
#include "DebugLib/glenumerants.h"
#include "utils/p2pcopy.h"
}

#ifdef GLSLDB_WIN32
	typedef DWORD PID_T;
#else
	typedef pid_t PID_T;
#endif

class ProgramControl :public QObject 
{
	Q_OBJECT

public:
	ProgramControl();
	~ProgramControl();

	/* remote program control */
#ifdef _WIN32
    // We need inherited class, not that
    ErrorCode runProgramWin(char **debuggedProgramArgs, char *workDir = NULL);
#endif
	bool runProgram(const DebugConfig& config);
	bool attachToProgram(const PID_T pid);
	bool killDebuggee(void);
	bool detachFromProgram(void);
	bool childAlive(void);
	void checkChildStatus(void);
	void queryTraceEvent(pid_t pid, int status);
	FunctionCall* getCurrentCall(void);

	ErrorCode execute(bool stopOnGLError);
	ErrorCode executeToShaderSwitch(bool stopOnGLError);
	ErrorCode executeToDrawCall(bool stopOnGLError);
	ErrorCode executeToUserDefined(const char *fname, bool stopOnGLError);
	ErrorCode checkExecuteState(int *state);
	ErrorCode executeContinueOnError(void);
	ErrorCode stop(void);

	ErrorCode callOrigFunc(const FunctionCall *fCall = 0);
	ErrorCode callDone(void);
	ErrorCode overwriteFuncArguments(const FunctionCall *fCall);

	ErrorCode restoreRenderTarget(int target);
	ErrorCode setDbgTarget(int target, int alphaTestOption,
			int depthTestOption, int stencilTestOption, int blendingOption);

	ErrorCode saveAndInterruptQueries(void);
	ErrorCode restartQueries(void);

	ErrorCode initRecording(void);
	ErrorCode recordCall(void);
	ErrorCode replay(int target);
	ErrorCode endReplay(void);

	ErrorCode getShaderCode(char *shaders[3], TBuiltInResource *resource,
			char **serializedUniforms, int *numUniforms);

	ErrorCode saveActiveShader(void);
	ErrorCode restoreActiveShader(void);

	ErrorCode shaderStepFragment(char *shaders[3], int numComponents,
			int format, int *width, int *heigh, void **image);
	ErrorCode shaderStepVertex(char *shaders[3], int target,
			int primitiveMode, int forcePointPrimitiveMode,
			int numFloatsPerVertex, int *numPrimitives, int *numVertices,
			float **vertexData);

	/* obsolete? */
	ErrorCode setDbgShaderCode(char *shaders[3], int target);

	ErrorCode initializeRenderBuffer(bool copyRGB, bool copyAlpha,
			bool copyDepth, bool copyStencil, float red, float green,
			float blue, float alpha, float depth, int stencil);
	ErrorCode readBackActiveRenderBuffer(int numComponents, int *width,
			int *heigh, float **image);

	ErrorCode insertGlEnd(void);

	State state() const { return _state; }
	void state(State s);
	ErrorCode error() const { return _error; }
	void error(ErrorCode s);
signals:
	void stateChanged(State s);
	void errorOccured(ErrorCode ec);
	void errorMessage(const QString& msg);
private:
	unsigned int getArgumentSize(int type);

	/* process environment communication */
	void buildEnvVars();
	void setDebugEnvVars(void);

	/* remote program control */
	PID_T _debuggeePID;
	ErrorCode dbgCommandExecute(void);
	ErrorCode dbgCommandExecuteToDrawCall(void);
	ErrorCode dbgCommandExecuteToShaderSwitch(void);
	ErrorCode dbgCommandExecuteToUserDefined(const char *fname);
	ErrorCode dbgCommandExecute(bool stopOnGLError);
	ErrorCode dbgCommandExecuteToDrawCall(bool stopOnGLError);
	ErrorCode dbgCommandExecuteToShaderSwitch(bool stopOnGLError);
	ErrorCode dbgCommandExecuteToUserDefined(const char *fname,
			bool stopOnGLError);
	ErrorCode dbgCommandStopExecution(void);
	ErrorCode dbgCommandCallOrig(void);
	ErrorCode dbgCommandCallOrig(const FunctionCall *fCall);
	ErrorCode dbgCommandCallDBGFunction(const char* = 0);
	ErrorCode dbgCommandAllocMem(unsigned int numBlocks, unsigned int *sizes,
			void **addresses);
	ErrorCode dbgCommandFreeMem(unsigned int numBlocks, void **addresses);
	ErrorCode dbgCommandStartRecording(void);
	ErrorCode dbgCommandRecord(void);
	ErrorCode dbgCommandReplay(int target);
	ErrorCode dbgCommandEndReplay(void);
	ErrorCode dbgCommandSetDbgTarget(int target, int alphaTestOption,
			int depthTestOption, int stencilTestOption, int blendingOption);
	ErrorCode dbgCommandRestoreRenderTarget(int target);
	ErrorCode dbgCommandCopyToRenderBuffer(void);
	ErrorCode dbgCommandClearRenderBuffer(int mode, float r, float g, float b,
			float a, float f, int s);
	ErrorCode dbgCommandSaveAndInterruptQueries(void);
	ErrorCode dbgCommandRestartQueries(void);
	ErrorCode dbgCommandShaderStepFragment(void *shaders[3],
			int numComponents, int format, int *width, int *height,
			void **image);
	ErrorCode dbgCommandShaderStepVertex(void *shaders[3], int target,
			int primitiveMode, int forcePointPrimitiveMode,
			int numFloatsPerVertex, int *numPrimitives, int *numVertices,
			float **vertexData);
	ErrorCode dbgCommandReadRenderBuffer(int numComponents, int *width,
			int *height, float **image);
	ErrorCode dbgCommandDone(void);
	void* copyArgumentFromProcess(void *addr, int type);
	void copyArgumentToProcess(void *dst, void *src, int type);
	char* printArgument(void *addr, int type);
	void printCall(void);
	void printResult(void);

	/* dbg command execution and error checking */
	ErrorCode executeDbgCommand(void);
	ErrorCode checkError(void);

	/* Shared memory handling */
	void initShmem(void);
	void clearShmem(void);
	void freeShmem(void);
	DbgRec* getThreadRecord(PID_T pid);

    DebugConfig _debugConfig;
	State _state;
	ErrorCode _error;

	int shmid;
	DbgRec *_fcalls;
	std::string _path_dbglib;
	std::string _path_dbgfuncs;
	std::string _path_libdlsym;
	std::string _path_log;

#ifdef _WIN32
	void createEvents(const DWORD processId);
	void closeEvents(void);

	/* Signal debugee. */
	HANDLE _hEvtDebuggee;

	/* Wait for debugee signaling me. */
	HANDLE _hEvtDebugger;

	/* Process handle of the debugged program. */
	HANDLE _hDebuggedProgram;

	/** Handles for process we attached to, but did not create ourself. */
	ATTACHMENT_INFORMATION _ai;

	HANDLE _hShMem;

#endif /* _WIN32 */
};

#endif
