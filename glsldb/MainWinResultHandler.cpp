#include "MainWinResultHandler.h"
#include "mainWindow.qt.h"
#include "notify.h"

MainWinResultHandler::MainWinResultHandler()
{

}
MainWinResultHandler::~MainWinResultHandler()
{

}
void MainWinResultHandler::handle(FutureResult& result)
{
	UT_NOTIFY(LV_TRACE, "result received");
	try {
		auto res = result.get();
	}
	catch(std::exception& e) {
		UT_NOTIFY(LV_ERROR, e.what());
	}
}