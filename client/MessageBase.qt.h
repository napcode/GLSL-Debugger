#ifndef MESSAGEBASE_H
#define MESSAGEBASE_H 1

#include <QtCore/QString>
#include <QtCore/QQueue>
#include <QtCore/QList>
#include <QtCore/QSharedPointer>

#include "utils/notify.h"
#include "proto/protocol.h"
#include <future>
#include <atomic>

class Process;
namespace msg {
typedef QSharedPointer<proto::ServerMessage> ServerMessagePtr;

class ResponseBase {
public:
    ResponseBase(uint64_t id = 0, const QString &msg = QString()) :
        _id(id), _message(msg)
    {}
    virtual ~ResponseBase() {}

    void id(uint64_t id)
    {
        _id = id;
    }
    uint64_t id() const
    {
        return _id;
    }
    void message(const QString &message)
    {
        _message = message;
    }
    const QString &message() const
    {
        return _message;
    }
protected:
    uint64_t _id;
    QString _message;
};
typedef QSharedPointer<ResponseBase> ResponsePtr;

class MessageBase : public QObject {
    Q_OBJECT

public: // methods
    MessageBase(proto::ClientMessage::Type t, uint64_t thread_id = 0);
    virtual ~MessageBase() {};
    virtual void response(const ServerMessagePtr &response) = 0;

    bool operator==(uint64_t response_id)
    {
        return id() == response_id;
    }

    /* retrieve the response. can only be done once */
    std::future<ResponsePtr> response()
    {
        return _response.get_future();
    }

    /* these methods handle MessageBase results */
    uint64_t id() const
    {
        return _message.id();
    }
    const proto::ClientMessage &message() const
    {
        return _message;
    }
protected: // variables
    proto::ClientMessage _message;
    std::promise<ResponsePtr> _response;
    static std::atomic<uint64_t> _seq_number;

protected: // methods
    std::promise<ResponsePtr> &promise()
    {
        return _response;
    }
};
typedef QSharedPointer<MessageBase> MessagePtr;
typedef QQueue<MessagePtr> MessageQueue;
typedef QList<MessagePtr> MessageList;
typedef std::future<ResponsePtr> FutureResponse;


class DebugCommand : public MessageBase {
public:
    DebugCommand(uint64_t thread_id = 0) :
        MessageBase(proto::ClientMessage::DEBUG_COMMAND, thread_id)
    {
    }
    virtual ~DebugCommand() {};
    virtual void response(const ServerMessagePtr &response) = 0;
protected:
};

}   // namespace msg

#endif
