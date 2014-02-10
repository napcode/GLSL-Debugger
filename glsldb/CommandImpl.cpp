#include "CommandImpl.h"
#include <exception>

ExecuteCommand::ExecuteCommand(Process &p, bool step, bool stopOnGLError)
    : DebugCommand(p, "DBG_EXECUTE (DBG_EXECUTE_RUN)")
{
    _rec->operation = DBG_EXECUTE;
    _rec->items[0] = step ? DBG_STEP : DBG_EXECUTE_RUN;
    _rec->items[1] = stopOnGLError ? 1 : 0;
}
void ExecuteCommand::operator()()
{
    try {
        DebugCommand::operator()();
        process().advance();
    }
    catch(...) {
        promise().set_exception(std::current_exception());
        return;
    }

    ResultPtr res(new Result(true));
    promise().set_value(res);
}

LaunchCommand::LaunchCommand(Process &p)
    : Command(p, "LAUNCH_CMD")
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

    ResultPtr res(new Result(true));
    promise().set_value(res);
}

CheckTrapCommand::CheckTrapCommand(Process &p, os_pid_t pi, int s)
    : Command(p, "CHECKTRAP_CMD"), _pid(pi), _status(s)
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

    ResultPtr res(new Result(true));
    promise().set_value(res);
}