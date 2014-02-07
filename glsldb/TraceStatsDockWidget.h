#ifndef TRACESTATSDOCKWIDGET_H
#define TRACESTATSDOCKWIDGET_H 1

#include <QtGui/QDockWidget>

namespace Ui {
	class DockWidget;
}

class GlCallStatistics;
class FunctionCall;

class TraceStatsDockWidget : public QDockWidget
{
	//Q_OBJECT
public:
	TraceStatsDockWidget(QWidget *parent = 0);

	virtual ~TraceStatsDockWidget();

	void setGlStatisticTabs(int n, int m);

	/* FIXME use signals? */
	void addGlTraceItem(const FunctionCall& call);
	void setUpdatesEnabled(bool value);

	void resetStatistics();
	void resetPerFrameStatistics();

private slots:
	/* statistics */
	//void on_cbGlstCallOrigin_currentIndexChanged(int);
	//void on_cbGlstPfMode_currentIndexChanged(int);

private:
	Ui::DockWidget *_ui;

	GlCallStatistics *_glCallStats;
	GlCallStatistics *_glExtSt;
	GlCallStatistics *_glCallPfst;
	GlCallStatistics *_glExtPfst;
	GlCallStatistics *_glxCallSt;
	GlCallStatistics *_glxExtSt;
	GlCallStatistics *_glxCallPfst;
	GlCallStatistics *_glxExtPfst;
	GlCallStatistics *_wglCallSt;
	GlCallStatistics *_wglExtSt;
	GlCallStatistics *_wglCallPfst;
	GlCallStatistics *_wglExtPfst;

};
#endif