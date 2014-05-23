#ifndef MSG_MESSAGEIMPL_H
#define MSG_MESSAGEIMPL_H 1

#include "MessageBase.qt.h"
#include "FunctionCall.h"

namespace msg
{
class Announce : public MessageBase
{
public:
	Announce(const std::string& client_name);
	void response(const msg::ServerMessagePtr& response);
	std::string _client_name;
};
class FunctionCall : public MessageBase
{
public:
	class Response : public ResponseBase
	{
	public:
		std::list<::FunctionCallPtr>& calls() { return _calls; }
	private:
		std::list<::FunctionCallPtr> _calls;
	};
	FunctionCall(uint64_t thread_id = 0);
	void response(const msg::ServerMessagePtr& response);
};
class Call : public DebugCommand
{
public:
	Call(uint64_t thread_id);
	void response(const msg::ServerMessagePtr& response);
};
class Done : public DebugCommand
{
public:
	Done(uint64_t thread_id);
	void response(const msg::ServerMessagePtr& response);
};
class Next : public DebugCommand
{
public:
	Next(uint64_t thread_id);
	void response(const msg::ServerMessagePtr& response);
};

}
#endif
