#include "client/MessageHandler.h"
#include "utils/notify.h"

AsyncMessageHandler::AsyncMessageHandler()
    : _end(false)
{
	_handler = new std::thread(&AsyncMessageHandler::run, this);
}
AsyncMessageHandler::~AsyncMessageHandler()
{
    UT_NOTIFY(LV_TRACE, "~AsyncMessageHandler");
	_end = true;
    _messageCondition.notify_all();
	_handler->join();
	delete _handler;
}
void AsyncMessageHandler::handle(CommandPtr f)
{
	{
        std::lock_guard<std::mutex> lock(_mtx);
	   _results.push_back(f);
        UT_NOTIFY(LV_TRACE, "Message enqueued");
    }
    _messageCondition.notify_one();
}
void AsyncMessageHandler::run()
{
	CommandPtr c;
    std::unique_lock<std::mutex> lock(_mtx);
    while(!_end) {
        {
            _messageCondition.wait(lock, [this] { return !_results.empty() || _end; });
            if(_end)
                break;
            c = _results.front();
            UT_NOTIFY(LV_TRACE, "Message dequeued");
            _results.pop_front();
            handleSingleMessage(c);
        }
    }
}
