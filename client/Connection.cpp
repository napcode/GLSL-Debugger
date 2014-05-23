#include "Connection.qt.h"
#include <iostream>
#include "utils/notify.h"
#include "Debugger.qt.h"
#include "MessageImpl.h"

IPCConnection::IPCConnection() :
    _verified(false)
{}

IPCConnection::~IPCConnection()
{}
void IPCConnection::establish()
{
    std::string client = Debugger::instance().clientName();
    msg::MessagePtr cmd(new msg::Announce(client));
    send(cmd);
    /* explicit sync */
    cmd->response().get();
    _verified = true;
}
TcpConnection::TcpConnection(const QString &path, int port, int timeout_s) :
    _host(path),
    _port(port)
{
    sck_begin();
}
TcpConnection::~TcpConnection()
{
    if(_verified && _th_receiver->joinable()) {
        sck_close(_socket);
        _th_receiver->join();
        sck_destroy(_socket);
        delete _th_receiver;
    }
    sck_end();
}

void TcpConnection::establish()
{
    _socket = sck_create(_host.toStdString().c_str(), _port, SCK_INET, SCK_STREAM);
    if (!_socket)
        throw std::runtime_error("unable to create socket");
    if (sck_connect(_socket) != 0)
        throw std::runtime_error("connection failed");

    _th_receiver = new std::thread(&TcpConnection::receive, this);
    std::string client = Debugger::instance().clientName();
    msg::MessagePtr cmd(new msg::Announce(client));
    doSend(cmd);

    /* explicit sync */
    /* throws if an error occured */
    cmd->response().get();
    _verified = true;
}
void TcpConnection::send(msg::MessagePtr &cmd)
{
    if (!isValid())
        throw std::runtime_error("invalid connection state");
    doSend(cmd);
}
void TcpConnection::doSend(msg::MessagePtr &cmd)
{
    _msg_list.push_back(cmd);
    std::string msg_str = cmd->message().SerializeAsString();
    msg_size_t size = msg_str.size();
    qint64 written = sck_send(_socket, (char *)&size, sizeof(msg_size_t));
    assert(written == sizeof(msg_size_t));
    written = sck_send(_socket, msg_str.c_str(), size);
    assert(written == size);
}
void TcpConnection::receive()
{
    static std::vector<char> buffer(1024);
    msg_size_t num_bytes_needed = sizeof(msg_size_t);
    int ret;
    while (true) {
        /* read message size */
        ret = sck_recv(_socket, &num_bytes_needed, sizeof(msg_size_t));
        assert(ret == sizeof(msg_size_t));
        if (num_bytes_needed > buffer.size())
            buffer.resize(num_bytes_needed);

        /* read message with previously read size */
        ret = sck_recv(_socket, &buffer[0], num_bytes_needed);
        assert(ret == num_bytes_needed);
        msg::ServerMessagePtr response(new proto::ServerMessage);
        UT_NOTIFY(LV_DEBUG, "Message received");
        if (!response->ParseFromArray(&buffer[0], num_bytes_needed))
            emit error("erroneous server message received");
        else {
            UT_NOTIFY(LV_DEBUG, "Message parsed");
            auto i = _msg_list.begin();
            while (i != _msg_list.end()) {
                if ((*i)->id() == response->id()) {
                    (*i)->response(response);
                    emit newResponseAvailable((*i));
                    break;
                }
            }
            if (i == _msg_list.end())
                emit error("received a server message but there was no request");
            else
                _msg_list.erase(i);
        }
    }
}
