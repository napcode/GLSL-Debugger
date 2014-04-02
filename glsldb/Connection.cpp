#include "Connection.h"
#include <iostream>
#include "tpl/src/tpl.h"

Connection::Connection(QTcpSocket *s) :
	_socket(s)
{}

Connection::~Connection()
{}
void Connection::send(msg_request_t& t)
{
	tpl_node *n;
	size_t len;
	void *buffer;
	n = tpl_map("S(i)", &t);
	tpl_pack(n, 0);
	tpl_dump(n, TPL_FD, TPL_MEM, &buffer, &len);
	_socket->write((const char*)buffer, len);
	free(buffer);
	tpl_free(n);
}