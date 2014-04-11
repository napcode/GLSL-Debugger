#include "Connection.h"
#include <iostream>
#include "utils/notify.h"

Connection::Connection()
{}

Connection::~Connection()
{}

/*
void Connection::send(msg_request_t& t)
{
}
*/
TcpConnection::TcpConnection(const QString &path, int port, int timeout_s) :
    _host(path),
    _port(port),
    _timeout(timeout_s * 1000),
    _socket(new QTcpSocket)
{
}
void TcpConnection::connect()
{
    _socket->connectToHost(_host, _port);
    if (!_socket->waitForConnected(_timeout))
        throw std::runtime_error("Unable to connect to debuggee @ " + _host.toStdString());
    UT_NOTIFY(LV_INFO, "Connection established");
}
ulong TcpConnection::send(CommandPtr& cmd)
{
    std::string msg_str = cmd->message().SerializeAsString();
    msg_size_t size = msg_str.size();
    qint64 written = _socket->write((char*)&size, sizeof(msg_size_t));
    assert(written == sizeof(msg_size_t));
    written = _socket->write(msg_str.c_str(), size);
    assert(written == size);
    return written;
}
QByteArray TcpConnection::receive(ulong num_bytes)
{
    while(_socket->bytesAvailable() < (qint64)num_bytes) {
        if(!_socket->waitForReadyRead(_timeout))
            throw std::runtime_error(_socket->errorString().toStdString());
    }
    return _socket->read(num_bytes);
}