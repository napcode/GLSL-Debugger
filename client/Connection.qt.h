#ifndef CLIENT_CONNECTION_QT_H
#define CLIENT_CONNECTION_QT_H 1

#include <QtNetwork/QTcpSocket>
#include <QtCore/QSharedPointer>
#include "build-config.h"
#include "proto/protocol.h"
#include "Command.qt.h"

typedef QSharedPointer<QTcpSocket> TcpSocketPtr;
typedef QSharedPointer<proto::ServerMessage> ServerMessagePtr;

class Connection : public QObject
{
	Q_OBJECT
public:
	Connection();
	~Connection();
	virtual void establish() = 0;
	virtual bool isEstablished() const = 0;
	virtual void send(CommandPtr& cmd) = 0;
signals:
	void newServerMessage(ServerMessagePtr p);
	void error(QString message);
private:
};
typedef QSharedPointer<Connection> ConnectionPtr;

class TcpConnection : public Connection
{
	Q_OBJECT
public:
	TcpConnection(const QString& host, int port = DEFAULT_SERVER_PORT_TCP, int timeout_s = NETWORK_TIMEOUT);
	void establish();
	bool isEstablished() const { return _socket.state() == QAbstractSocket::ConnectedState; }
	void send(CommandPtr& cmd);
protected:
	bool haveMessageSize() const { return _msg_size > 0; } 
private:
	QString _host;
	int _port;
	int _timeout;
	QTcpSocket _socket;
	int _num_bytes_needed;
	int _msg_size;
private slots:
	void readyReadSlot();
	void errorSlot(QAbstractSocket::SocketError socketError);
};

#endif
