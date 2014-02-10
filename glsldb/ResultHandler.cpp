#include "ResultHandler.h"
#include "notify.h"

ResultHandler::ResultHandler()
    : _end(false)
{
	_handler = new std::thread(&ResultHandler::run, this);
}
ResultHandler::~ResultHandler()
{
    UT_NOTIFY(LV_TRACE, "~ResultHandler");
	_end = true;
    _resultCondition.notify_all();
	_handler->join();
	delete _handler;
}
void ResultHandler::addFutureResult(FutureResult &f)
{
	{
        std::lock_guard<std::mutex> lock(_mtx);
	   _results.push_back(std::move(f));
        UT_NOTIFY(LV_TRACE, "Result enqueued");
    }
    _resultCondition.notify_one();
}
void ResultHandler::run()
{
	FutureResult res;
    std::unique_lock<std::mutex> lock(_mtx);
    while(!_end) {
        {
            _resultCondition.wait(lock, [this] { return !_results.empty() || _end; });
            if(_end)
                break;
            res = std::move(_results.front());
            UT_NOTIFY(LV_TRACE, "Result dequeued");
            _results.pop_front();
            handle(res);
        }
    }
}
