/******************************************************************************

Copyright (C) 2006-2009 Institute for Visualization and Interactive Systems
(VIS), Universität Stuttgart.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice, this
	list of conditions and the following disclaimer in the documentation and/or
	other materials provided with the distribution.

  * Neither the name of the name of VIS, Universität Stuttgart nor the names
	of its contributors may be used to endorse or promote products derived from
	this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#ifndef MAINWINDOW_QT_H
#define MAINWINDOW_QT_H

#ifdef _WIN32
#include <windows.h>
#endif /* _WIN32 */

#include <QtCore/QTextStream>

#include "globaldefines.h"
#include "ui_mainWindow.h"
#include "openProgramDialog.qt.h"
#include "jumpToDialog.qt.h"

//#include "ShHandle.h"
#include "ShaderLang.h"

#include "shVarModel.qt.h"
#include "errorCodes.h"
#include "runLevel.h"
#include "functionCall.h"
#include "pixelBox.qt.h"
#include "loopDialog.qt.h"
#include "loopData.qt.h"
#include "watchView.qt.h"
#include "glTraceFilterModel.qt.h"
#include "geoShaderDataModel.qt.h"
#include "fragmentTestDialog.qt.h"
#include "glTraceListModel.qt.h"
#include "glTraceSettingsDialog.qt.h"

#include "DebugConfig.h"
#include "Process.qt.h"

#include <QtGui/QScrollArea>
#include <QtCore/QStack>

class QWorkspace;
class TraceStatsDockWidget;
class MainWinResultHandler;

namespace Ui {
	class MainWindow;
}
 
class MainWindow: public QMainWindow
{
	Q_OBJECT
	friend class MainWinResultHandler;
public:
	MainWindow(const QStringList& args);
	~MainWindow();

public slots:
	void debuggeeStateChanged(Process::State s);
	void debugMessage(const QString& msg);
	void updateGuiToRunLevel(RunLevel);
	void debugError(ErrorCode);
    void debuggeeForked(os_pid_t pid);

private slots:

	/****************
	 * auto connect *
	 ****************/

	/* general */
	void on_aQuit_triggered();
	void on_aOpen_triggered();
	void on_aAttach_triggered();
	void on_aOnlineHelp_triggered();
	void on_aAbout_triggered();

	/* buffer view */
	void on_tbBVCapture_clicked();
	void on_tbBVCaptureAutomatic_toggled(bool);
	void on_tbBVSave_clicked();

	/* glTrace */
	void on_tbExecute_clicked();
	void on_tbStep_clicked();
	void on_tbSkip_clicked();
	void on_tbEdit_clicked();
	void on_tbJumpToDrawCall_clicked();
	void on_tbJumpToShader_clicked();
	void on_tbJumpToUserDef_clicked();
	void on_tbRun_clicked();
	void on_tbPause_clicked();
	void on_tbToggleGuiUpdate_clicked(bool);
	void on_tbToggleNoTrace_clicked(bool);
	void on_tbToggleHaltOnError_clicked(bool);
	void on_tbGlTraceSettings_clicked();
	void on_tbSave_clicked();

	/* shader */
	void on_twShader_currentChanged(int selection);
	void on_tbShaderExecute_clicked();
	void on_tbShaderReset_clicked();
	void on_tbShaderStep_clicked();
	void on_tbShaderStepOver_clicked();
	void on_tbShaderFragmentOptions_clicked();

	/* watch */
	void on_tbWatchWindow_clicked();
	void on_tbWatchWindowAdd_clicked();
	void on_tbWatchDelete_clicked();
	void watchWindowClosed();

	/****************
	 * self connect *
	 ****************/

	void updateWatchItemData(ShVarItem*);
	void watchSelectionChanged(const QItemSelection&, const QItemSelection&);
	void setMouseOverValues(int x, int y, const bool *active,
			const QVariant *values);
	void newSelectedPixel(int x, int y);
	void newSelectedVertex(int n);
	void newSelectedPrimitive(int dataIdx);
	void changedActiveWindow(QWidget *w);
	void shaderStep(int action, bool updateData = true, bool updateCovermap =
			true);

	void singleStep();

private: /* types */
	enum CoverageMapStatus {
		COVERAGEMAP_UNCHANGED,
		COVERAGEMAP_GROWN,
		COVERAGEMAP_SHRINKED
	};


private: /* methods */
	void run();
	void closeEvent(QCloseEvent *event);


	void setShaderCodeText(char *shaders[3]);
	void leaveDBGState();
	void cleanupDBGShader();
	bool getDebugImage(DbgCgOptions option, ShChangeableList *cl, int rbFormat,
			bool *coverage, PixelBox **fbData);
	bool getDebugVertexData(DbgCgOptions option, ShChangeableList *cl,
			bool *coverage, VertexBox *vdata);

	/* Gui update handling */
	void setGuiUpdates(bool);
	void updateWatchGui(int s);

	void addGlTraceItem(ProcessPtr p);
	void addGlTraceErrorItem(const QString& text);
	void addGlTraceWarningItem(const QString& text);
	void setGlTraceItemText(const QString& text);
	void setGlTraceItemIconType(const GlTraceListItem::IconType type);
	void clearGlTraceItemList(void);

	ErrorCode getNextCall();
	ErrorCode nextStep();
	ErrorCode recordCall();
	void recordDrawCall();
	void updateWatchListData(CoverageMapStatus cmstatus, bool forceUpdate);
	void updateWatchItemsCoverage(bool *coverage);
	void resetWatchListData(void);
	void updateSelectedPixelValues(void);
	QModelIndexList cleanupSelectionList(QModelIndexList input);

	WatchView* newWatchWindowGeoDataTree(QModelIndexList &list);
	WatchView* newWatchWindowVertexTable(QModelIndexList &list);
	WatchView* newWatchWindowFragment(QModelIndexList &list);
	void addToWatchWindowGeoDataTree(WatchView *watchView,
			QModelIndexList &list);
	void addToWatchWindowVertexTable(WatchView *watchView,
			QModelIndexList &list);
	void addToWatchWindowFragment(WatchView *watchView, QModelIndexList &list);

	/* MRU program. */
	bool loadMruProgram(QString& outProgram, QString& outArguments,
			QString& outWorkDir);
	bool saveMruProgram(const QString& program, const QString& arguments,
			const QString& workDir);

private: /* variables */
	Ui::MainWindow *_ui;

	/* Workspace */
	QWorkspace *_workspace;

	/* GLTrace Model */
	GlTraceListModel *_glTraceModel;
	GlTraceFilterModel *_glTraceFilterModel;

	/* watch window controls */
	QActionGroup *_agWatchControl;

	/* Dock Widget: GL Buffer View */
	QScrollArea *_sBVArea;
	QLabel *_lBVLabel;

	/* glTrace settings dialog */
	GlTraceSettingsDialog *_traceSettingsDialog;

	/* per fragment tests */
	FragmentTestDialog *_fragmentTestDialog;

    DebugConfig _debugConfig;
    ProcessPtr _proc;
    MainWinResultHandler* _resultHandler;

	FunctionCallPtr _currentCall;

	/* == 1 if we are inside a glNewList-glEndList block */
	bool _inDLCompilation;

	TraceStatsDockWidget *_traceStatsWidget;

	ShHandle m_dShCompiler;
	TBuiltInResource m_dShResources;
	ShVariableList m_dShVariableList;

	ShVarModel *m_pShVarModel;
	QStack<LoopData*> m_qLoopData;

	char *m_pShaders[3];
	bool m_bHaveValidShaderCode;

	struct {
		char *pData;
		int count;
	} m_serializedUniforms;

	int m_primitiveMode;
	VertexBox *m_pGeometryMap;
	VertexBox *m_pVertexCount;
	//GeoShaderDataModel *m_pGeoDataModel;

	bool *m_pCoverage;

	QPoint _selectedPixel;
};

#endif
