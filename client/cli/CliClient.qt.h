#ifndef CLI_CLIENT_QT_H
#define CLI_CLIENT_QT_H 1

#include <QtCore/QCoreApplication>
#include <QtCore/QString>
#include <QtCore/QThread>
#include <QtCore/QTextStream>
#include <iostream>
#include <string>
#include "utils/notify.h"
#include "client/Debugger.qt.h"
#include "proto/protocol.h"
#include "client/MessageImpl.h"

class CliReader : public QObject {
    Q_OBJECT
public:
    CliReader()
    {}
public slots:
    void readLine()
    {
        QTextStream strin(stdin);
        QString line = strin.readLine();
        emit newInput(line);
    }
signals:
    void newInput(QString line);
private:
};

class CliClient : public QCoreApplication {
    Q_OBJECT
public:
    CliClient(int argc, char **argv) :
        QCoreApplication(argc, argv)
    {
        UTILS_NOTIFY_STARTUP();
        severity_t lv = LV_TRACE;
        UTILS_NOTIFY_LEVEL(&lv);
    }
    ~CliClient()
    {
        UTILS_NOTIFY_SHUTDOWN();
        _thread.quit();
        _thread.wait();
    }
    void init()
    {
        Debugger &dbg = Debugger::instance();
        dbg.init();
        _reader.moveToThread(&_thread);
        //connect(&_thread, SIGNAL(finished()), &_reader, SLOT(deleteLater()));
        connect(&_reader, SIGNAL(newInput(QString)), this, SLOT(handleCommand(QString)));
        connect(this, SIGNAL(requestInput()), &_reader, SLOT(readLine()));
        _thread.start();

        QTextStream strout(stdout);
        strout << "Current commands: connect, func, quit\n";
        emit requestInput();
    }
public slots:
    void handleCommand(QString line)
    {
        UT_NOTIFY(LV_INFO, "Command: " << line.toStdString());
        try {
            if (line == QString("done"))  {
                msg::MessagePtr c = _proc->done();
                c->response().get();
            } else if (line == QString("call"))  {
                msg::MessagePtr c = _proc->call();
                c->response().get();
            } else if (line == QString("connect") || line == QString("c") )  {
                Debugger &dbg = Debugger::instance();
                QString path("127.0.0.1");
                _proc = dbg.connectTo(path);
                UT_NOTIFY(LV_INFO, "Connection established");
            } else if (line == QString("launch"))  {
                //request.type = RQ_STEP;
                //cn_send(&c, request);
            } else if (line == QString("func") || line == QString("f"))  {
                msg::MessagePtr c = _proc->currentCall();
                msg::ResponsePtr p = c->response().get();
                msg::FunctionCall::Response& fc = dynamic_cast<msg::FunctionCall::Response&>(*p);
                for(auto &i : fc.calls())
                    UT_NOTIFY(LV_INFO, "Current call: " << i->asString().toStdString());
                //request.type = RQ_STEP;
                //cn_send(&c, request);
            } else if (line == QString("step") || line == QString("s")) {
                msg::MessagePtr c = _proc->execute(proto::ExecutionDetails_Operation_STEP);
                msg::ResponsePtr p = c->response().get();
            } else if (line == QString("continue") || line == QString("c")) {
                msg::MessagePtr c = _proc->execute(proto::ExecutionDetails_Operation_CONTINUE);
                msg::ResponsePtr p = c->response().get();
            } else if (line == QString("stop") || line == QString("halt")) {
                msg::MessagePtr c = _proc->execute(proto::ExecutionDetails_Operation_HALT);
                msg::ResponsePtr p = c->response().get();
            } else if (line == QString("quit") || line == QString("q"))  {
                /* TODO do disconnect */
                quit();
                return;
            } else {
                std::cerr << "Unkown command" << std::endl;
            }
        } catch (std::exception &e) {
            UT_NOTIFY(LV_ERROR, e.what());
        }
        emit requestInput();
    }
signals:
    void requestInput();
private:
    CliReader _reader;
    QThread _thread;
    ProcessPtr _proc;
};

#endif
