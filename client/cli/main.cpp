#include <iostream>
#include <string>
#include "utils/notify.h"
#include "client/Debugger.qt.h"
#include <QtCore/QString>

ProcessPtr _proc;

void create_debuggee(int argc, char** argv)
{
    Debugger& dbg = Debugger::instance();
    dbg.init();
    DebugConfig cfg;
    //cfg.programArgs += QString(argv[1]);
    cfg.programArgs += QString("/usr/bin/glxgears");
    cfg.traceFork = true;
    cfg.traceExec = true;
    cfg.traceClone =  true;
    cfg.valid = true;
    /* cleanup dbg state */
    cfg.workDir = QString("./");
    _proc = Debugger::instance().create(cfg);
}


int main(int argc, char** argv)
{
    UTILS_NOTIFY_STARTUP();
    Debugger& dbg = Debugger::instance();
    dbg.init();

    //create_debuggee(argc, argv);
    QString path;
    if(argc > 1)
        path = QString(argv[1]);
    else 
        path = QString("127.0.0.1");
    _proc = dbg.connect(path);

    int read;
    /* needs to be done first */
    //if(cn_create_tcp(&c, "127.0.0.1", 12345) != 0)
    //if(cn_create_unix(&c, "/tmp/glsldb-unix-sck") != 0)
    //    return EXIT_FAILURE;

    CommandFactory& cf = _proc->commandFactory();
    std::string line;
    do {
        std::getline(std::cin, line);
        if(line == std::string("step"))  {
            FutureResult fr = cf.launch()->result();
            fr.get();            
        }
        else if(line == std::string("launch"))  {
            //request.type = RQ_STEP;
            //cn_send(&c, request);
        }
        else if(line == std::string("continue")) {
            //request.type = RQ_CONTINUE;
            //cn_send(&c, request);
        }
        else if(line == std::string("stop")) {
            //request.type = RQ_STOP;
            //cn_send(&c, request);
        }
        else if(line == std::string("quit"))  {
            //request.type = RQ_DISCONNECT;
            //cn_send(&c, request);
            //cn_destroy(&c);
            break;
        }
        else {
            std::cerr << "Unkown command" << std::endl;
        }
    } while(1);

    UTILS_NOTIFY_SHUTDOWN();
    return 0;
}