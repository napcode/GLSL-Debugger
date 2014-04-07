#include "client/ResultHandler.h"
#include "utils/notify.h"

AsyncResultHandler::AsyncResultHandler()
    : _end(false)
{
	_handler = new std::thread(&AsyncResultHandler::run, this);
}
AsyncResultHandler::~AsyncResultHandler()
{
    UT_NOTIFY(LV_TRACE, "~AsyncResultHandler");
	_end = true;
    _resultCondition.notify_all();
	_handler->join();
	delete _handler;
}
void AsyncResultHandler::handle(CommandPtr f)
{
	{
        std::lock_guard<std::mutex> lock(_mtx);
	   _results.push_back(f);
        UT_NOTIFY(LV_TRACE, "Result enqueued");
    }
    _resultCondition.notify_one();
}
void AsyncResultHandler::run()
{
	CommandPtr c;
    std::unique_lock<std::mutex> lock(_mtx);
    while(!_end) {
        {
            _resultCondition.wait(lock, [this] { return !_results.empty() || _end; });
            if(_end)
                break;
            c = _results.front();
            UT_NOTIFY(LV_TRACE, "Result dequeued");
            _results.pop_front();
            handleSingleResult(c);
        }
    }
}
