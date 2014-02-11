#include "MainWinResultHandler.qt.h"
#include "mainWindow.qt.h"
#include "notify.h"
#include "Command.qt.h"
#include "CommandImpl.h"
#include "Debugger.qt.h"

MainWinResultHandler::MainWinResultHandler(MainWindow& win, QObject *parent) :
	QObject(parent),
	_win(win)
{
	Debugger& dbg = Debugger::instance();
	connect(&dbg, SIGNAL(resultAvailable(CommandPtr)), this, SLOT(resultAvailable(CommandPtr)));
}
MainWinResultHandler::~MainWinResultHandler()
{

}
void MainWinResultHandler::resultAvailable(CommandPtr c)
{
	Command::ResultPtr result;
	try {
		result = std::move(c->result().get());
		UT_NOTIFY(LV_INFO, c->name().toStdString() << " succeeded");
	}
	catch (std::exception& e) {
		UT_NOTIFY(LV_ERROR, c->name().toStdString() << " failed");
		return;
	}

	QSharedPointer<StepCommand::Result> r = qSharedPointerDynamicCast<StepCommand::Result>(result);
	if(r) {
		UT_NOTIFY(LV_INFO, "Function call received");
		_win.addGlTraceItem(r->functionCall);
	}
}
