#ifndef RESULTHANDLER_H
#define RESULTHANDLER_H 1

#include <thread>
#include <deque>

#include "Command.h"

typedef std::deque<FutureResult> ResultQueue;
class ResultHandler
{
public:
	ResultHandler();
	virtual ~ResultHandler();
	void addFutureResult(FutureResult& f);
protected:
	virtual void run();
	virtual void handle(FutureResult &f) = 0;	
protected:
	ResultQueue _results;
private:
	bool _end;
	std::thread *_handler;
	std::mutex _mtx;
	std::condition_variable _resultCondition;
};
#endif