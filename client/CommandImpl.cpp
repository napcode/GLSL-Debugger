#include "CommandImpl.h"
#include "Debugger.qt.h"
#include "proto/protocol.h"
#include <exception>

AnnounceCommand::AnnounceCommand(Process &p, const std::string& client_name)
    : Command(p, proto::ClientRequest::ANNOUNCE)
{
    _message.mutable_announce()->set_id(PROTO_ID);
    _message.mutable_announce()->set_client_name(_client_name);
    _message.mutable_announce()->mutable_version()->set_major(PROTO_MAJOR);
    _message.mutable_announce()->mutable_version()->set_minor(PROTO_MINOR);
    _message.mutable_announce()->mutable_version()->set_revision(PROTO_REVISION); 
}
void AnnounceCommand::result(const proto::ServerMessage& response)
{
    ResultPtr res(new Result);
    if(response.error_code() != proto::ErrorCode::NONE) {
        res->ok = false;
        res->message = response.message();
    }
    promise().set_value(res);
}
CallCommand::CallCommand(Process &p)
    : Command(p, proto::ClientRequest::DEBUG_COMMAND)
{
    _message.mutable_command()->set_type(proto::DebugCommand_Type_CALL_ORIGFUNCTION);
    _message.mutable_command()->set_thread_id(0);
    //_rec->operation = DBG_STOP_EXECUTION;
}
void CallCommand::result(const proto::ServerMessage& response)
{
    /* parse server response, create a result and store it in the promise */
    // promise().set_value();
}
DoneCommand::DoneCommand(Process &p)
    : Command(p, proto::ClientRequest::DEBUG_COMMAND)
{
    _message.mutable_command()->set_type(proto::DebugCommand_Type_DONE);
    _message.mutable_command()->set_thread_id(0);
    //_rec->operation = DBG_STOP_EXECUTION;
}
void DoneCommand::result(const proto::ServerMessage& response)
{
    /* parse server response, create a result and store it in the promise */
    // promise().set_value();
}
