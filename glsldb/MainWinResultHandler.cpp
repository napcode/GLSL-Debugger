#include "MainWinResultHandler.qt.h"
#include "mainWindow.qt.h"
#include "notify.h"
#include "Command.qt.h"
#include "CommandImpl.h"
#include "Debugger.qt.h"

MainWinResultHandler::MainWinResultHandler(QObject *parent) :
	QObject(parent)
{
	Debugger& dbg = Debugger::instance();
	connect(&dbg, SIGNAL(commandFailed(CommandPtr)), this, SLOT(commandFailed(CommandPtr)));
	connect(&dbg, SIGNAL(resultAvail(CommandPtr)), this, SLOT(resultAvail(CommandPtr)));
}
MainWinResultHandler::~MainWinResultHandler()
{

}
void MainWinResultHandler::resultAvail(CommandPtr c)
{
	UT_NOTIFY(LV_INFO, c->name().toStdString() << " succeeded");
}
void MainWinResultHandler::commandFailed(CommandPtr c)
{
	UT_NOTIFY(LV_ERROR, c->name().toStdString() << " failed");
}