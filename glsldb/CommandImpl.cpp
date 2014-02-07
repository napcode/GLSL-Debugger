#include "CommandImpl.h"

ExecuteCommand::ExecuteCommand(Process &p, bool stopOnGLError)
    : Command(p)
{
    static const QString msg("DBG_EXECUTE (DBG_EXECUTE_RUN)");
    _message = &msg;
    _rec.operation = DBG_EXECUTE;
    _rec.items[0] = DBG_EXECUTE_RUN;
    _rec.items[1] = stopOnGLError ? 1 : 0;
}
void ExecuteCommand::operator()()
{
    Command::operator()();
    process().advance();
    ResultPtr res(new Result(true, 666));
    promise().set_value(res);
}