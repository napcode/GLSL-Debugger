#include "runLevel.h"

const char* getRunLevelInfo(RunLevel rl)
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
const char* getStateInfo(State s)
{
	switch(s) {
		case ST_INVALID:
			return "INVALID";
		case ST_INIT:
			return "INIT";
		case ST_ALIVE:
			return "ALIVE";
		case ST_RUNNING:
			return "RUNNING";
		case ST_STOPPED:
			return "STOPPED";
		case ST_EXITED:
			return "EXITED";
		case ST_KILLED:
			return "KILLED";
		default:
			return "UNDEFINED";
	}
}
