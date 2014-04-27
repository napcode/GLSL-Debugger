#include "Connection.qt.h"
#include <iostream>
#include "utils/notify.h"

Connection::Connection()
{}

Connection::~Connection()
{}
TcpConnection::TcpConnection(const QString &path, int port, int timeout_s) :
    _host(path),
    _port(port),
    _timeout(timeout_s * 1000),
    _num_bytes_needed(sizeof(msg_size_t)), 
    _msg_size(0)
{
    QObject::connect(&_socket, SIGNAL(readyRead()), this, SLOT(readyReadSlot()));
    QObject::connect(&_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(errorSlot(QAbstractSocket::SocketError)));
}

void TcpConnection::establish()
{
    _socket.connectToHost(_host, _port);
    _socket.waitForConnected(_timeout);
    if(!isEstablished())
        throw std::runtime_error(_socket.errorString().toStdString());
}
void TcpConnection::send(CommandPtr& cmd)
{
    std::string msg_str = cmd->message().SerializeAsString();
    msg_size_t size = msg_str.size();
    UT_NOTIFY(LV_TRACE, "writing msg size " << size);
    qint64 written = _socket.write((char*)&size, sizeof(msg_size_t));
    assert(written == sizeof(msg_size_t));
    written = _socket.write(msg_str.c_str(), size);
    UT_NOTIFY(LV_TRACE, "msg written" << written);
    assert(written == size);
}
void TcpConnection::readyReadSlot()
{
    if(_socket.bytesAvailable() < _num_bytes_needed)
        return;
    if(haveMessageSize()) {
        QByteArray buf = _socket.read(sizeof(msg_size_t));
        assert(sizeof(unsigned int) == sizeof(msg_size_t));
        _msg_size = *((msg_size_t *)buf.data());
    }
    else {
        QByteArray buf = _socket.read(_msg_size);
        ServerMessagePtr response(new proto::ServerMessage);
        if (!response->ParseFromArray(buf.data(), buf.size())) {
            emit error("erroneous server message received");
        }
        emit newServerMessage(response);
        _msg_size = 0;
    }
}
void TcpConnection::errorSlot(QAbstractSocket::SocketError socketError)
{
    emit error(_socket.errorString());
}