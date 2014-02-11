#ifndef MAINWINDOWRESULTHANDLER_H
#define MAINWINDOWRESULTHANDLER_H 1

#include "Command.qt.h"

class MainWinResultHandler : QObject
{
	Q_OBJECT
public:
	MainWinResultHandler(QObject *parent);
	~MainWinResultHandler();
public slots:
	void resultAvail(CommandPtr);
	void commandFailed(CommandPtr);
};

#endif