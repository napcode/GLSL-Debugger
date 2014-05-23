#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H 1

#include <thread>
#include <deque>
#include <QtCore/QSharedPointer>

#include "MessageBase.qt.h"

namespace msg
{

class MessageHandler
{
public:
	virtual ~MessageHandler() {};
	virtual void handle(msg::MessagePtr c) = 0;
};
typedef QSharedPointer<MessageHandler> MessageHandlerPtr;

}
#endif
