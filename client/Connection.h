#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H 1

#include <QtNetwork/QTcpSocket>
#include <QtCore/QSharedPointer>

typedef QSharedPointer<QTcpSocket> TcpSocketPtr;

class Connection
{
public:
	Connection(QTcpSocket *s);
	~Connection();
	//void send(msg_request_t& t);
 
private:
	TcpSocketPtr _socket;
};

typedef QSharedPointer<Connection> ConnectionPtr;

#endif
