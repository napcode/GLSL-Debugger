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
void AnnounceCommand::result(const proto::ServerResponse& response)
{
    ResultPtr res(new Result);
    if(response.error_code() != proto::ErrorCode::NONE) {
        res->ok = false;
        res->message = response.message();
    }
    promise().set_value(res);
}
StepCommand::StepCommand(Process &p, bool trace, bool stopOnGLError)
    : Command(p, proto::ClientRequest::STEP)
{
    //_rec->operation = DBG_STOP_EXECUTION;
}
void StepCommand::result(const proto::ServerResponse& response)
{
    /* parse server response, create a result and store it in the promise */
    // promise().set_value();
}
