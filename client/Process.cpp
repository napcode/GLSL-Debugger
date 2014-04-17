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
    _responseReceiver(nullptr),
    _state(INIT)
{
}

Process::Process(ConnectionPtr c) :
    _pid(0),
    _responseReceiver(nullptr),
    _state(INIT),
    _connection(c)
{

}
Process::Process(const DebugConfig &cfg, os_pid_t pid) :
    _pid(pid),
    _responseReceiver(nullptr),
    _debugConfig(cfg),
    _state(INIT)
{
}
Process::~Process()
{
    UT_NOTIFY(LV_TRACE, "~Process");
    stopReceiver();
}
void Process::startReceiver()
{
    if (!_responseReceiver) {
        _end = false;
        _responseReceiver = new std::thread(&Process::receiveMessages, this);
    }
}
void Process::stopReceiver()
{
    if (_responseReceiver) {
        _end = true;
        if (_state == RUNNING || _state == STOPPED || _state == TRAPPED)
            kill();
        if (_responseReceiver->joinable())
            _responseReceiver->join();
        delete _responseReceiver;
        _responseReceiver = nullptr;
    }
}
bool Process::init()
{
    if (!connection()->connected())
        connection()->connect();
    startReceiver();
    //announce();
    //proto::ClientRequest rq;
    //rq.set_type(proto::ClientRequest::PROCESS_INFO);
    //_connection->write(rq);
    return true;
}
void Process::storeCommand(CommandPtr &cmd)
{
    {
        std::lock_guard<std::mutex> lock(_mtxWork);
        _commands.push_back(cmd);
    }
    _workCondition.notify_one();
}
CommandPtr Process::announce()
{
    std::string client = Debugger::instance().clientName();
    CommandPtr cmd(new AnnounceCommand(*this, client));
    UT_NOTIFY(LV_INFO, "sending request");
    connection()->send(cmd);
    storeCommand(cmd);
    return cmd;
}
CommandPtr Process::done()
{
    CommandPtr cmd(new DoneCommand(*this));
    storeCommand(cmd);
    connection()->send(cmd);
    return cmd;
}
CommandPtr Process::call()
{
    CommandPtr cmd(new CallCommand(*this));
    storeCommand(cmd);
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

void Process::receiveMessages()
{
    std::unique_lock<std::mutex> lock(_mtxWork, std::defer_lock);
    QByteArray ba;
    msg_size_t length;
    ServerMessagePtr response;
    while (!_end) {
        lock.lock();
        _workCondition.wait(lock,
                            [this] { return !_commands.empty() || _end; });
        lock.unlock();
        if (_end)
            break;
        /* 1) receive size of next message (msg_size_t)
         * 2) receive buffer with previously received size
         * 3) parse received buffer (usually a ServerMessage)
         * 4) back to 1)
         */
        try {
            QByteArray bas = connection()->receive(sizeof(msg_size_t));
            assert(sizeof(unsigned int) == sizeof(msg_size_t));
            length = *((msg_size_t *)bas.data());
            ba = connection()->receive(length);
        } catch (std::exception &e) {
            /* TODO decide what to do on timeout */
            UT_NOTIFY(LV_ERROR, "Exception caught: " << e.what());
            break;
        }
        try {
            ServerMessagePtr response(new proto::ServerMessage);
            if (!response->ParseFromArray(ba.data(), ba.size())) 
                throw std::logic_error("Not a valid server response");
            std::unique_lock<std::mutex> lock(_mtxWork);
            /* a message might be the result of a previously sent command */
            /* meh. std:find/qFind won't work with SharedPtrs/Qt stuff */
            UT_NOTIFY(LV_INFO, "cmd size " << _commands.size() );
            CommandList::iterator it = _commands.begin();
            while (it != _commands.end()) {
                if ((*it)->id() == response->id()) {
                    (*it)->result(*response);
                    _commands.erase(it);
                    break;
                }
                ++it;
            }
            /* this message is not the result of a command */
            if (it != _commands.end())
                throw std::logic_error("We should have search the list by now...");
            if (messageHandler())
                messageHandler()->handle(*it);
        } catch (std::exception &e) {
            UT_NOTIFY(LV_ERROR, "Exception caught: " << e.what());
        }
    }
}
