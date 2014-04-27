#ifndef MESSAGEHANDLER_H
#define MESSAGEHANDLER_H 1

#include <thread>
#include <deque>
#include <QtCore/QSharedPointer>

#include "Command.qt.h"

class MessageHandler
{
public:
	virtual ~MessageHandler() {};
	virtual void handle(CommandPtr c) = 0;
};

class AsyncMessageHandler
{
public:
	AsyncMessageHandler();
	virtual ~AsyncMessageHandler();
	virtual void handle(CommandPtr c);	
	virtual void handleSingleMessage(CommandPtr c) = 0; 
protected:
	virtual void run();
protected:
	CommandQueue _results;
private:
	bool _end;
	std::thread *_handler;
	std::mutex _mtx;
	std::condition_variable _messageCondition;
};
typedef QSharedPointer<MessageHandler> MessageHandlerPtr;
#endif
