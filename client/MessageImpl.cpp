#include "MessageImpl.h"
#include "Debugger.qt.h"
#include "proto/protocol.h"
#include <exception>

using namespace msg;

Announce::Announce(const std::string& client_name) :
    MessageBase(proto::ClientMessage::ANNOUNCE), 
    _client_name(client_name)
{
    _message.mutable_announce()->set_id(PROTO_ID);
    _message.mutable_announce()->set_client_name(_client_name);
    _message.mutable_announce()->mutable_version()->set_major(PROTO_MAJOR);
    _message.mutable_announce()->mutable_version()->set_minor(PROTO_MINOR);
    _message.mutable_announce()->mutable_version()->set_revision(PROTO_REVISION);
}
void Announce::response(const msg::ServerMessagePtr& rsp)
{
    UT_NOTIFY(LV_TRACE, "handler");
    if(rsp->error_code() == proto::ErrorCode::NONE) {
        ResponsePtr res(new ResponseBase(rsp->id(), QString::fromStdString(rsp->message())));
        promise().set_value(res);
    }
    else {
        promise().set_exception(make_exception_ptr(std::runtime_error(rsp->message())));
    } 
}
msg::FunctionCall::FunctionCall(ThreadID_t thread_id) :
    MessageBase(proto::ClientMessage::FUNCTION_CALL, thread_id)
{
}
void msg::FunctionCall::response(const msg::ServerMessagePtr& rsp)
{
    UT_NOTIFY(LV_TRACE, "handler");
    
    if(rsp->error_code() == proto::ErrorCode::NONE) {
        UT_NOTIFY(LV_TRACE, "response");
        Response *res = new Response;
        for(int i = 0; i < rsp->function_call_size(); ++i) {
            FunctionCallPtr func(new ::FunctionCall(rsp->function_call(i)));
            res->calls().push_back(func);
        }
        promise().set_value(ResponsePtr(res));
    }
    else {
        UT_NOTIFY(LV_TRACE, "exception");
        promise().set_exception(make_exception_ptr(std::runtime_error(rsp->message())));
    }
}
Execute::Execute(ThreadID_t thread_id, proto::ExecutionDetails_Operation op, const QString& param) :
    MessageBase(proto::ClientMessage::EXECUTION, thread_id)
{
    _message.mutable_execution()->set_operation(op);
    if(param.size())
        _message.mutable_execution()->mutable_call()->set_name(param.toStdString());
}
void Execute::response(const msg::ServerMessagePtr& rsp)
{
    if(rsp->error_code() == proto::ErrorCode::NONE) {
        ResponsePtr res(new ResponseBase);
        promise().set_value(res);
    }
    else {
        promise().set_exception(make_exception_ptr(std::runtime_error(rsp->message())));
    }
}

Call::Call(ThreadID_t thread) : 
    DebugCommand(thread)
{
    _message.mutable_command()->set_type(proto::DebugCommand_Type_CALL_ORIGFUNCTION);
    //_rec->operation = DBG_STOP_EXECUTION;
}
void Call::response(const msg::ServerMessagePtr& rsp)
{
     UT_NOTIFY(LV_TRACE, "handler");
   /* parse server response, create a response and store it in the promise */
    // promise().set_value();
}
Done::Done(ThreadID_t thread)
    : DebugCommand(thread)
{
    _message.mutable_command()->set_type(proto::DebugCommand_Type_DONE);
    //_rec->operation = DBG_STOP_EXECUTION;
}
void Done::response(const msg::ServerMessagePtr& rsp)
{
    UT_NOTIFY(LV_TRACE, "handler");
    /* parse server response, create a response and store it in the promise */
    // promise().set_value();
}
