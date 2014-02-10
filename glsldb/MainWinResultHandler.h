#ifndef MAINWINDOWRESULTHANDLER_H
#define MAINWINDOWRESULTHANDLER_H 1

#include "ResultHandler.h"

class MainWinResultHandler : public ResultHandler
{
public:
	MainWinResultHandler();
	~MainWinResultHandler();
	void handle(FutureResult &res);
};

#endif