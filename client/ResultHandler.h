#ifndef RESULTHANDLER_H
#define RESULTHANDLER_H 1

#include <thread>
#include <deque>
#include <QtCore/QSharedPointer>

#include "Command.qt.h"

class ResultHandler
{
public:
	virtual ~ResultHandler() {};
	virtual void handle(CommandPtr c) = 0;
};

class AsyncResultHandler
{
public:
	AsyncResultHandler();
	virtual ~AsyncResultHandler();
	virtual void handle(CommandPtr c);	
	virtual void handleSingleResult(CommandPtr c) = 0; 
protected:
	virtual void run();
protected:
	CommandQueue _results;
private:
	bool _end;
	std::thread *_handler;
	std::mutex _mtx;
	std::condition_variable _resultCondition;
};
typedef QSharedPointer<ResultHandler> ResultHandlerPtr;
#endif
