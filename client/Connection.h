#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H 1

#include <QtNetwork/QTcpSocket>
#include <QtCore/QSharedPointer>
#include "build-config.h"
#include "proto/protocol.h"
#include "Command.qt.h"

typedef QSharedPointer<QTcpSocket> TcpSocketPtr;

class Connection
{
public:
	Connection();
	~Connection();
	virtual void connect() = 0;
	virtual bool connected() const = 0;
	virtual ulong send(CommandPtr& cmd) = 0;
	virtual QByteArray receive(ulong num_bytes) = 0;
private:
};
typedef QSharedPointer<Connection> ConnectionPtr;

class TcpConnection : public Connection
{
public:
	TcpConnection(const QString& host, int port = DEFAULT_SERVER_PORT_TCP, int timeout_s = NETWORK_TIMEOUT);
	void connect();
	bool connected() const { return _socket->state() == QAbstractSocket::ConnectedState; }
	ulong send(CommandPtr& cmd);
	QByteArray receive(ulong num_bytes);
private:
	QString _host;
	int _port;
	int _timeout;
	TcpSocketPtr _socket;
};

#endif
