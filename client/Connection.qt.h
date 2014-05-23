#ifndef CLIENT_CONNECTION_QT_H
#define CLIENT_CONNECTION_QT_H 1

#include <thread>
#include <QtCore/QSharedPointer>
#include "build-config.h"
#include "utils/socket.h"
#include "proto/protocol.h"
#include "MessageBase.qt.h"

class IPCConnection : public QObject
{
	Q_OBJECT
public:
	IPCConnection();
	~IPCConnection();
	bool isValid() const { return _verified; };
	virtual void establish() = 0;
	virtual void send(msg::MessagePtr& cmd) = 0;
signals:
	void newResponseAvailable(msg::MessagePtr& cmd);
	void error(QString message);
protected:
protected:
	bool _verified;
};
typedef QSharedPointer<IPCConnection> IPCConnectionPtr;

class TcpConnection : public IPCConnection
{
	Q_OBJECT
public:
	TcpConnection(const QString& host, int port = DEFAULT_SERVER_PORT_TCP, int timeout_s = NETWORK_TIMEOUT);
	~TcpConnection();
	void send(msg::MessagePtr& cmd);
	void establish();
protected:
	void doSend(msg::MessagePtr& cmd);
	void receive();
private:
	QString _host;
	int _port;
	socket_t *_socket;
	msg::MessageList _msg_list;
	std::thread *_th_receiver;
	std::mutex _mtx;
};

#endif
