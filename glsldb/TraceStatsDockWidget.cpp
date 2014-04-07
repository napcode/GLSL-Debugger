#include "TraceStatsDockWidget.h"
#include "glCallStatistics.qt.h"
#include "client/FunctionCall.h"
#include "ui_TraceStatsDockWidget.h"

#include <iostream>

TraceStatsDockWidget::TraceStatsDockWidget(QWidget *parent)
	: _ui(new Ui::DockWidget())
{
	_ui->setupUi(this);

	_glCallStats = new GlCallStatistics(_ui->tvGlCalls);
	_glExtSt = new GlCallStatistics(_ui->tvGlExt);
	_glCallPfst = new GlCallStatistics(_ui->tvGlCallsPf);
	_glExtPfst = new GlCallStatistics(_ui->tvGlExtPf);
	_glxCallSt = new GlCallStatistics(_ui->tvGlxCalls);
	_glxExtSt = new GlCallStatistics(_ui->tvGlxExt);
	_glxCallPfst = new GlCallStatistics(_ui->tvGlxCallsPf);
	_glxExtPfst = new GlCallStatistics(_ui->tvGlxExtPf);
	_wglCallSt = new GlCallStatistics(_ui->tvWglCalls);
	_wglExtSt = new GlCallStatistics(_ui->tvWglExt);
	_wglCallPfst = new GlCallStatistics(_ui->tvWglCallsPf);
	_wglExtPfst = new GlCallStatistics(_ui->tvWglExtPf);

	while (_ui->tabWidget->count() > 0) {
		_ui->tabWidget->removeTab(0);
	}
	_ui->tabWidget->insertTab(0, _ui->tabGlCalls, QString("GL Calls"));
	_ui->tabWidget->insertTab(1, _ui->tabGlExt, QString("GL Extensions"));

	setAutoFillBackground(true);
	setFeatures(QDockWidget::AllDockWidgetFeatures);
	setFloating(false);
	setAllowedAreas(Qt::AllDockWidgetAreas);

}

TraceStatsDockWidget::~TraceStatsDockWidget()
{
	delete _glCallStats;
	delete _glExtSt;
	delete _glCallPfst;
	delete _glExtPfst;
	delete _glxCallSt;
	delete _glxExtSt;
	delete _glxCallPfst;
	delete _glxExtPfst;
	delete _wglCallSt;
	delete _wglExtSt;
	delete _wglCallPfst;
	delete _wglExtPfst;

}
void TraceStatsDockWidget::setGlStatisticTabs(int n, int m)
{
	while (_ui->tabWidget->count() > 0) {
		_ui->tabWidget->removeTab(0);
	}

	switch (n) {
	case 0:
		switch (m) {
		case 0:
			_ui->tabWidget->insertTab(0, _ui->tabGlCalls, QString("GL Calls"));
			_ui->tabWidget->insertTab(1, _ui->tabGlExt, QString("GL Extensions"));
			break;
		case 1:
			_ui->tabWidget->insertTab(0, _ui->tabGlCallsPf, QString("GL Calls"));
			_ui->tabWidget->insertTab(1, _ui->tabGlExtPf, QString("GL Extensions"));
			break;
		}
		break;
	case 1:
		switch (m) {
		case 0:
			_ui->tabWidget->insertTab(0, _ui->tabGlxCalls, QString("GLX Calls"));
			_ui->tabWidget->insertTab(1, _ui->tabGlxExt, QString("GLX Extensions"));
			break;
		case 1:
			_ui->tabWidget->insertTab(0, _ui->tabGlxCallsPf, QString("GLX Calls"));
			_ui->tabWidget->insertTab(1, _ui->tabGlxExtPf, QString("GLX Extensions"));
			break;
		}
		break;
	case 2:
		switch (m) {
		case 0:
			_ui->tabWidget->insertTab(0, _ui->tabWglCalls, QString("WGL Calls"));
			_ui->tabWidget->insertTab(1, _ui->tabWglExt, QString("WGL Extensions"));
			break;
		case 1:
			_ui->tabWidget->insertTab(0, _ui->tabWglCallsPf, QString("WGL Calls"));
			_ui->tabWidget->insertTab(1, _ui->tabWglExtPf, QString("WGL Extensions"));
			break;
		}
		break;
	default:
		break;
	}
}
void TraceStatsDockWidget::addGlTraceItem(const FunctionCall& call)
{

	if (call.isGlFunc()) {
		_glCallStats->incCallStatistic(call.name());
		_glExtSt->incCallStatistic(call.extension());
		_glCallPfst->incCallStatistic(call.name());
		_glExtPfst->incCallStatistic(call.extension());
	} else if (call.isGlxFunc()) {
		_glxCallSt->incCallStatistic(call.name());
		_glxExtSt->incCallStatistic(call.extension());
		_glxCallPfst->incCallStatistic(call.name());
		_glxExtPfst->incCallStatistic(call.extension());
	} else if (call.isWglFunc()) {
		_wglCallSt->incCallStatistic(call.name());
		_wglExtSt->incCallStatistic(call.extension());
		_wglCallPfst->incCallStatistic(call.name());
		_wglExtPfst->incCallStatistic(call.extension());
	}
}
void TraceStatsDockWidget::setUpdatesEnabled(bool value)
{
	_ui->tvGlCalls->setUpdatesEnabled(value);
	_ui->tvGlCallsPf->setUpdatesEnabled(value);
	_ui->tvGlExt->setUpdatesEnabled(value);
	_ui->tvGlExtPf->setUpdatesEnabled(value);
	_ui->tvGlxCalls->setUpdatesEnabled(value);
	_ui->tvGlxCallsPf->setUpdatesEnabled(value);
	_ui->tvGlxExt->setUpdatesEnabled(value);
	_ui->tvGlxExtPf->setUpdatesEnabled(value);
	_ui->tvWglCalls->setUpdatesEnabled(value);
	_ui->tvWglCallsPf->setUpdatesEnabled(value);
	_ui->tvWglExt->setUpdatesEnabled(value);
	_ui->tvWglExtPf->setUpdatesEnabled(value);
	QDockWidget::setUpdatesEnabled(value);
}
void TraceStatsDockWidget::resetStatistics()
{
	_glCallStats->resetStatistic();
	_glExtSt->resetStatistic();
	_glxCallSt->resetStatistic();
	_glxExtSt->resetStatistic();
	_wglCallSt->resetStatistic();
	_wglExtSt->resetStatistic();
	_wglExtPfst->resetStatistic();
	resetPerFrameStatistics();
}

void TraceStatsDockWidget::resetPerFrameStatistics()
{
	_glCallPfst->resetStatistic();
	_glExtPfst->resetStatistic();
	_glxCallPfst->resetStatistic();
	_glxExtPfst->resetStatistic();
	_wglCallPfst->resetStatistic();
	_wglExtPfst->resetStatistic();
}
/*
void TraceStatsDockWidget::on_cbGlstCallOrigin_currentIndexChanged(int n)
{
	setGlStatisticTabs(n, cbGlstPfMode->currentIndex());
}

void TraceStatsDockWidget::on_cbGlstPfMode_currentIndexChanged(int m)
{
	setGlStatisticTabs(cbGlstCallOrigin->currentIndex(), m);
}
*/