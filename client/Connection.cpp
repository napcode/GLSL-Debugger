#include "Connection.h"
#include <iostream>

Connection::Connection(QTcpSocket *s) :
	_socket(s)
{}

Connection::~Connection()
{}
/*
void Connection::send(msg_request_t& t)
{
}
*/
