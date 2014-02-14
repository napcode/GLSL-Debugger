#ifndef CONNECTION_H
#define CONNECTION_H 1

#include <QtNetwork/QTcpSocket>
#include <QtCore/QSharedPointer>
#include "proto/command.h"

typedef QSharedPointer<QTcpSocket> TcpSocketPtr;

class Connection
{
public:
	Connection(QTcpSocket *s);
	~Connection();

	void send(cmd_announce_t& t) {};
 
private:
	TcpSocketPtr _socket;
};

typedef QSharedPointer<Connection> ConnectionPtr;

#endif