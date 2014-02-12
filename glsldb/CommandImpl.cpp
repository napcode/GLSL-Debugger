#include "CommandImpl.h"
#include "Debugger.qt.h"
#include <exception>

ExecuteCommand::ExecuteCommand(Process &p, bool stopOnGLError)
    : SimpleCommand(p, "DBG_EXECUTE (DBG_EXECUTE_RUN)")
{
    _rec->operation = DBG_EXECUTE;
    _rec->items[0] = DBG_EXECUTE_RUN;
    _rec->items[1] = stopOnGLError ? 1 : 0;
}

StepCommand::StepCommand(Process &p, bool trace, bool stopOnGLError)
    : DebugCommand(p, "DBG_EXECUTE_STOP")
{
    _rec->operation = DBG_STOP_EXECUTION;
}
void StepCommand::operator()()
{
    try {
        DebugCommand::operator()();
        process().advance();
        process().waitForStatus();
        /* FIXME evaluate error code then */
        if(_rec->result != DBG_FUNCTION_CALL)
			throw std::logic_error("Result is not a function call");
        Debugger& dbg = Debugger::instance();
        emit dbg.resultAvailable(CommandPtr(this));
    }
    catch(...) {
        promise().set_exception(std::current_exception());
        return;
    }

    ResultPtr res(new Result(process().currentCall()));
    promise().set_value(res);
}
LaunchCommand::LaunchCommand(Process &p)
    : Command(p, "LAUNCH")
{}
void LaunchCommand::operator()()
{
    try {
        process().launch();
    }
    catch(...) {
        promise().set_exception(std::current_exception());
        return;
    }

    ResultPtr res(new Result);
    promise().set_value(res);
}

CheckTrapCommand::CheckTrapCommand(Process &p, os_pid_t pi, int s)
    : Command(p, "CHECKTRAP"), _pid(pi), _status(s)
{}
void CheckTrapCommand::operator()()
{
    try {
        process().checkTrapEvent(_pid, _status);
    }
    catch(...) {
        promise().set_exception(std::current_exception());
        return;
    }

    ResultPtr res(new Result);
    promise().set_value(res);
}
