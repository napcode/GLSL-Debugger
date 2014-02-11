#ifndef MAINWINDOWRESULTHANDLER_H
#define MAINWINDOWRESULTHANDLER_H 1

#include "Command.qt.h"
class MainWindow;

class MainWinResultHandler : QObject
{
	Q_OBJECT
public:
	MainWinResultHandler(MainWindow& win, QObject *parent = nullptr);
	~MainWinResultHandler();
public slots:
	void resultAvailable(CommandPtr);
private:
	MainWindow& _win;
};

#endif
