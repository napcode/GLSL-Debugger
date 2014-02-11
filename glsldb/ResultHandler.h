#ifndef RESULTHANDLER_H
#define RESULTHANDLER_H 1

#include <thread>
#include <deque>
#include <QSharedPointer>

#include "Command.h"

class ResultHandler
{
public:
	virtual ~ResultHandler() {};
	virtual void handle(FutureResult &f) = 0;
};

typedef std::deque<FutureResult> ResultQueue;
class AsyncResultHandler
{
public:
	AsyncResultHandler();
	virtual ~AsyncResultHandler();
	virtual void handle(FutureResult &f);	
	virtual void handleSingleResult(FutureResult &f) = 0; 
protected:
	virtual void run();
protected:
	ResultQueue _results;
private:
	bool _end;
	std::thread *_handler;
	std::mutex _mtx;
	std::condition_variable _resultCondition;
};
typedef QSharedPointer<ResultHandler> ResultHandlerPtr;
#endif