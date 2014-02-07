#include "runLevel.h"

const char* getRunLevelString(RunLevel rl)
{
	switch(rl) {
		case RL_INIT:
			return "init";
		case RL_SETUP:
			return "setup";
		case RL_TRACE_EXECUTE:
			return "execute";
		case RL_TRACE_EXECUTE_NO_DEBUGABLE:
			return "execute_no_debugable";
		case RL_TRACE_EXECUTE_IS_DEBUGABLE:
			return "execute_is_debugable";
		case RL_TRACE_EXECUTE_RUN:
			return "execute_run";
		case RL_DBG_RECORD_DRAWCALL:
			return "record drawcall";
		case RL_DBG_VERTEX_SHADER:
			return "vertex shader";
		case RL_DBG_GEOMETRY_SHADER:
			return "geometry shader";
		case RL_DBG_FRAGMENT_SHADER:
			return "fragment shader";
		case RL_DBG_RESTART:
			return "restart";
		default:
			return "unknown runlevel";
	}
}

