#include "Connection.h"

Connection::Connection(QTcpSocket *s) :
	_socket(s)
{}

Connection::~Connection()
{}

