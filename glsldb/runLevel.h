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

#ifndef RUNLEVEL_H
#define RUNLEVEL_H

/* Reminder: change getRunLevelInfo() if enum changes */
enum RunLevel {
	RL_INIT,
	RL_SETUP,
	RL_TRACE_EXECUTE,
	RL_TRACE_EXECUTE_NO_DEBUGABLE,
	RL_TRACE_EXECUTE_IS_DEBUGABLE,
	RL_TRACE_EXECUTE_RUN,
	RL_DBG_RECORD_DRAWCALL,
	RL_DBG_VERTEX_SHADER,
	RL_DBG_GEOMETRY_SHADER,
	RL_DBG_FRAGMENT_SHADER,
	RL_DBG_RESTART
};

/* Reminder: change getStateInfo() if enum changes */
enum State {
	ST_INVALID, 
	ST_INIT, 
	ST_WAITING, 
	ST_RUNNING, 
	ST_STOPPED,
	ST_EXITED, 
	ST_KILLED
};
const char* getRunLevelInfo(RunLevel rl);
const char* getStateInfo(State s);

#endif // RUNLEVEL_H
