#include "Process.qt.h"
#include "Debugger.qt.h"
#include "utils/notify.h"
#include "utils/glsldb_ptrace.h"
#include "proto/protocol.h"
#include "CommandImpl.h"

#ifndef GLSLDB_WIN32
#   include <sys/types.h>
#   include <sys/wait.h>
#   include <signal.h>
#endif
#include <cassert>
#include <exception>

Process::Process() :
    _pid(0),
    _state(INIT)
{
}

Process::Process(ConnectionPtr c) :
    _pid(0),
    _state(INIT),
    _connection(c)
{

}
Process::Process(const DebugConfig &cfg, os_pid_t pid) :
    _pid(pid),
    _debugConfig(cfg),
    _state(INIT)
{
}
Process::~Process()
{
    UT_NOTIFY(LV_TRACE, "~Process");
}

void Process::init()
{
    if(!connection()->isEstablished())
        connection()->establish();

    qRegisterMetaType<ServerMessagePtr>("ServerMessagePtr");
    connect(connection().data(), SIGNAL(newServerMessage(ServerMessagePtr)), this, SLOT(newServerMessageSlot(ServerMessagePtr)));
    connect(connection().data(), SIGNAL(error(QString)), this, SLOT(errorSlot(QString)));
    CommandPtr c = announce();
    // throws exception or is just "OK"
    UT_NOTIFY(LV_INFO, "ann");

    Command::ResultPtr r = c->result().get();
    UT_NOTIFY(LV_INFO, r->message);
}

CommandPtr Process::announce()
{
    std::string client = Debugger::instance().clientName();
    CommandPtr cmd(new AnnounceCommand(*this, client));
    _commands.push_back(cmd);
    connection()->send(cmd);
    return cmd;
}
CommandPtr Process::done()
{
    CommandPtr cmd(new DoneCommand(*this));
    _commands.push_back(cmd);
    connection()->send(cmd);
    return cmd;
}
CommandPtr Process::call()
{
    CommandPtr cmd(new CallCommand(*this));
    _commands.push_back(cmd);
    connection()->send(cmd);
    return cmd;
}
CommandPtr Process::kill()
{
    CommandPtr cmd(new CallCommand(*this));
    //CommandPtr cmd(new KillCommand(*this));
    /* store cmd so we can associate received results with it */
    //enqueue(cmd);
    //cmd->send();
    //return cmd;
    return cmd;
}

void Process::advance(void)
{
    if (!isStopped() && !isTrapped())
        return;
#ifdef _WIN32
    if (::SetEvent(_hEvtDebuggee)) {
        state(RUNNING);
        return;
    }
#else /* _WIN32 */
    if (ptrace(PTRACE_CONT, _pid, 0, 0) != -1) {
        state(RUNNING);
        return;
    }
#endif /* _WIN32 */
    throw std::runtime_error(std::string("Continue failed: ") + strerror(errno));
}

void Process::stop(bool immediately)
{
    if (!isRunning())
        return;

    if (immediately) {
        if (::kill(_pid, SIGSTOP) == -1) {
            state(INVALID);
            throw std::runtime_error(std::string("sending SIGSTOP failed") + strerror(errno));
        }
        state(STOPPED);
    } else {
        /* FIXME use CommandFactory */
        //debug_record_t *rec = getThreadRecord(_pid);
        // UT_NOTIFY(LV_INFO, "send: DBG_STOP_EXECUTION");
        // rec->operation = DBG_STOP_EXECUTION;
        // /* FIXME race conditions & use Command? */
        // advance();
        // waitForStatus();
    }
}

void Process::launchLocal()
{
    if (state() != INIT)
        throw std::logic_error("not in launchable state");

    if (!_debugConfig.valid)
        throw std::logic_error("config invalid");

    _pid = fork();
    if (_pid == -1) {
        _pid = 0;
        throw std::runtime_error("forking failed. debuggee not started.");
    }

    if (_pid == 0) {
        /**
         * debuggee path
         * tell parent to trace the child
         * alternative: parent uses PTRACE_ATTACH but that might be racy
         */
        ptrace(PTRACE_TRACEME, 0, 0, 0);

        if (!_debugConfig.workDir.isEmpty() && chdir(_debugConfig.workDir.toLatin1().data()) != 0) {
            UT_NOTIFY(LV_WARN, "changing to working directory failed (" << strerror(errno) << ")");
            UT_NOTIFY(LV_WARN, "debuggee execution might fail now");
        }
        UT_NOTIFY(LV_INFO, "debuggee executes " << _debugConfig.programArgs[0].toLatin1().data());
        setpgid(0, 0);

        char **pargs = static_cast<char **>(malloc((_debugConfig.programArgs.size() + 1) * sizeof(char *)));
        int i = 0;
        for (auto &j : _debugConfig.programArgs) {
            pargs[i] = static_cast<char *>(malloc((j.size() + 1) * sizeof(char)));
            strcpy(pargs[i], j.toLatin1().data());
            i++;
        }
        pargs[_debugConfig.programArgs.size()] = NULL;
        // execv() triggers a SIGTRAP
        // allows parent to set trace options
        execv(pargs[0], pargs);

        /* an error occured, execv should never return */
        UT_NOTIFY(LV_ERROR, "debuggee fork/execution failed: " << strerror(errno));
        UT_NOTIFY(LV_ERROR, "debuggee pid: " << getpid());
        do {
            char *p = *pargs;
            free(p);
            pargs++;
        } while (*pargs != NULL);
        free(pargs);
        _exit(73);
    }
    /* attaching might be racy. we might miss a fork/execv if we
     * don't get a chance to set options prior to that */
    //ptrace(PTRACE_ATTACH, _pid, 0, 0);

    // parent path
    UT_NOTIFY(LV_INFO, "debuggee process pid: " << _pid);
    //waitForStatus(false);
}

void Process::state(State s)
{
    if (s != _state) {
        emit stateChanged(s);
        _state = s;
    }
}

const QString &Process::strState(State s)
{
    switch (s) {
    case INVALID: {
        static const QString m("INVALID");
        return m;
    }
    case INIT: {
        static const QString m("INIT");
        return m;
    }
    case RUNNING: {
        static const QString m("RUNNING");
        return m;
    }
    case STOPPED: {
        static const QString m("STOPPED");
        return m;
    }
    case EXITED: {
        static const QString m("EXITED");
        return m;
    }
    case KILLED: {
        static const QString m("KILLED");
        return m;
    }
    case TRAPPED: {
        static const QString m("TRAPPED");
        return m;
    }
    default: {
        static const QString m("INVALID STATE");
        return m;
    }
    }
    static const QString m("DUMMY");
    return m;
}

void Process::newServerMessageSlot(ServerMessagePtr msg)
{
	UT_NOTIFY(LV_INFO, "server message received: " << msg->message());
}
void Process::errorSlot(QString msg)
{
	UT_NOTIFY(LV_INFO, "error received: " << msg.toStdString());
}