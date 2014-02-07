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

#include <QtGui/QDesktopServices>
#include <QtGui/QTextDocument>
#include <QtGui/QMessageBox>
#include <QtGui/QHeaderView>
#include <QtGui/QGridLayout>
#include <QtGui/QFileDialog>
#include <QtCore/QSettings>
#include <QtGui/QWorkspace>
#include <QtGui/QTabWidget>
#include <QtGui/QTabBar>
#include <QtGui/QColor>
#include <QtCore/QUrl>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif /* _WIN32 */

#include "ui_mainWindow.h"
#include "mainWindow.qt.h"
#include "editCallDialog.qt.h"
#include "selectionDialog.qt.h"
#include "dbgShaderView.qt.h"
#include "watchTable.qt.h"
#include "watchVector.qt.h"
#include "watchGeoDataTree.qt.h"
#include "compilerErrorDialog.qt.h"
#include "attachToProcessDialog.qt.h"
#include "aboutBox.qt.h"
#include "TraceStatsDockWidget.h"

#include "Debugger.qt.h"
#include "CommandImpl.h"

#include "glslSyntaxHighlighter.qt.h"
#include "runLevel.h"
#include "debuglib.h"
#include "utils/dbgprint.h"
#include "utils/notify.h"

#ifdef _WIN32
#define REGISTRY_KEY "Software\\VIS\\GLSL-Debugger"
#endif /* _WIN32 */

MainWindow::MainWindow(const QStringList& args)
	: _ui(new Ui::MainWindow)
{
	int i;

    _debugConfig.programArgs = args;

	/*** Setup GUI ****/
	_ui->setupUi(this);

	_traceStatsWidget = new TraceStatsDockWidget(this);
	addDockWidget(Qt::LeftDockWidgetArea, _traceStatsWidget, Qt::Horizontal);

	_ui->teVertexShader->setTabStopWidth(30);
	_ui->teGeometryShader->setTabStopWidth(30);
	_ui->teFragmentShader->setTabStopWidth(30);

	/* Functionality seems to be obsolete now */
	_ui->tbToggleGuiUpdate->setVisible(false);

	setAttribute(Qt::WA_QuitOnClose, true);

	/*   Status bar    */
	_ui->statusbar->addPermanentWidget(_ui->fSBError);
	_ui->statusbar->addPermanentWidget(_ui->fSBMouseInfo);

	/*   Workspace    */
	_workspace = new QWorkspace;
	setCentralWidget(_workspace);
	connect(_workspace, SIGNAL(windowActivated(QWidget*)), this,
			SLOT(changedActiveWindow(QWidget*)));

	/*   Buffer View    */
	QGridLayout *gridLayout;
	gridLayout = new QGridLayout(_ui->fContent);
	gridLayout->setSpacing(0);
	gridLayout->setMargin(0);
	_sBVArea = new QScrollArea();
	_sBVArea->setBackgroundRole(QPalette::Dark);
	gridLayout->addWidget(_sBVArea);
	_lBVLabel = new QLabel();
	_sBVArea->setWidget(_lBVLabel);

	/* GLTrace Settings */
	_glTraceFilterModel = new GlTraceFilterModel(glFunctions, this);
	_traceSettingsDialog = new GlTraceSettingsDialog(_glTraceFilterModel, this);

	/*   GLTrace View   */
	GlTraceListFilterModel *lvGlTraceFilter = new GlTraceListFilterModel(
			_glTraceFilterModel, this);
	_glTraceModel = new GlTraceListModel(MAX_GLTRACE_ENTRIES,
			_glTraceFilterModel, this);
	lvGlTraceFilter->setSourceModel(_glTraceModel);
	_ui->lvGlTrace->setModel(lvGlTraceFilter);

	/* Action Group for watch window controls */
	_agWatchControl = new QActionGroup(this);
	_agWatchControl->addAction(_ui->aZoom);
	_agWatchControl->addAction(_ui->aSelectPixel);
	_agWatchControl->addAction(_ui->aMinMaxLens);
	_agWatchControl->setEnabled(false);

	/* per frgamnet operations */
	_fragmentTestDialog = new FragmentTestDialog(this);

	Debugger& dbg = Debugger::instance();
	dbg.init();
	//connect(&dbg, SIGNAL(stateChanged(State)), this, SLOT(debuggeeStateChanged(State)));
	//connect(&dbg, SIGNAL(message(QString)), this, SLOT(debugMessage(QString)));
	//connect(&dbg, SIGNAL(error(ErrorCode)), this, SLOT(debugError(ErrorCode)));
	//connect(&dbg, SIGNAL(runLevel(RunLevel)), this, SLOT(debugRunLevel(RunLevel)));

	m_pShVarModel = NULL;

	_inDLCompilation = false;

	/* Prepare debugging */
	ShInitialize();
	m_dShCompiler = 0;

	for (i = 0; i < 3; i++) {
		m_pShaders[i] = NULL;
	}
	m_bHaveValidShaderCode = false;

	m_serializedUniforms.pData = NULL;
	m_serializedUniforms.count = 0;

	m_primitiveMode = GL_NONE;

	m_pGeometryMap = NULL;
	m_pVertexCount = NULL;
	//m_pGeoDataModel = NULL;

	m_pCoverage = NULL;

	_selectedPixel = QPoint(-1, -1);
	_ui->lWatchSelectionPos->setText("No Selection");

#ifdef _WIN32
	// TODO: Only Windows can attach at the moment.
	//this->_ui->aAttach->setEnabled(true);
#endif /* _WIN32 */

	QSettings settings;
	if (settings.contains("MainWinState")) {
		restoreState(settings.value("MainWinState").toByteArray());
	}
}

MainWindow::~MainWindow()
{
	/* Free reachable memory */
	UT_NOTIFY(LV_TRACE, "~MainWindow");
	Debugger::instance().release();

	delete[] m_pCoverage;
	delete m_pGeometryMap;
	delete m_pVertexCount;
	//delete m_pGeoDataModel;

	for (int i = 0; i < 3; i++) {
		delete[] m_pShaders[i];
	}

	delete[] m_serializedUniforms.pData;
	m_serializedUniforms.pData = NULL;
	m_serializedUniforms.count = 0;

	/* clean up compiler stuff */
	ShFinalize();
}
void MainWindow::on_aQuit_triggered()
{
	UT_NOTIFY(LV_TRACE, "quitting application");
	close();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	QSettings settings;
	settings.setValue("MainWinState", this->saveState());
	event->accept();
}

void MainWindow::on_aOpen_triggered()
{
	Dialog_OpenProgram *dOpenProgram = new Dialog_OpenProgram();

	/* Cleanup shader debugging */
	cleanupDBGShader();

    /* Set MRU program, if no old value was available. */
    {
        QSettings s;
    	dOpenProgram->leProgram->setText(s.value("MRU/Program", "").toString());
	    dOpenProgram->leArguments->setText(s.value("MRU/Arguments", "").toString());
    	dOpenProgram->leWorkDir->setText(s.value("MRU/WorkDir", "").toString());
	    dOpenProgram->cbExec->setChecked(s.value("MRU/Exec", "").toBool());
    	dOpenProgram->cbFork->setChecked(s.value("MRU/Fork", "").toBool());
	    dOpenProgram->cbClone->setChecked(s.value("MRU/Clone", "").toBool());
    }

	dOpenProgram->exec();

	if (dOpenProgram->result() == QDialog::Accepted) {
		if (!(dOpenProgram->leProgram->text().isEmpty())) {
			_debugConfig.programArgs.clear();
			_debugConfig.programArgs.append(dOpenProgram->leProgram->text());
			_debugConfig.programArgs += dOpenProgram->leArguments->text().split(
					QRegExp("\\s+"), QString::SkipEmptyParts);
            _debugConfig.traceFork = dOpenProgram->cbFork->isChecked();
            _debugConfig.traceExec = dOpenProgram->cbExec->isChecked();
            _debugConfig.traceClone = dOpenProgram->cbClone->isChecked();
			/* cleanup dbg state */
			leaveDBGState();
			_ui->statusbar->showMessage(
					QString("New program: ") + dOpenProgram->leProgram->text());
			clearGlTraceItemList();
		}

		if (!dOpenProgram->leWorkDir->text().isEmpty()) {
			_debugConfig.workDir = dOpenProgram->leWorkDir->text();
		} else {
			_debugConfig.workDir.clear();
		}

		/* Save MRU program. */
    	QSettings s;
	    s.setValue("MRU/Program", dOpenProgram->leProgram->text());
    	s.setValue("MRU/Arguments", dOpenProgram->leArguments->text());
	    s.setValue("MRU/WorkDir", dOpenProgram->leWorkDir->text());
	    s.setValue("MRU/Exec", dOpenProgram->cbExec->isChecked());
    	s.setValue("MRU/Clone", dOpenProgram->cbClone->isChecked());
	    s.setValue("MRU/Fork", dOpenProgram->cbFork->isChecked());
	    _debugConfig.valid = true;
	}
	delete dOpenProgram;
}

void MainWindow::on_aAttach_triggered()
{
	// UT_NOTIFY(LV_TRACE, "Attaching to debuggee");

	// Dialog_AttachToProcess dlgAttach(this);
	// dlgAttach.exec();

	// if (dlgAttach.result() == QDialog::Accepted) {
	// 	ProcessSnapshotModel::Item *process = dlgAttach.getSelectedItem();

	// 	if ((process != NULL) && process->IsAttachtable()) {
	// 		_debugConfig.programArgs.clear();
	// 		_debugConfig.programArgs.append(QString(process->GetExe()));
	// 		/* cleanup dbg state */
	// 		leaveDBGState();
	// 		/* kill program */
	// 		killDebuggee();
	// 		setRunLevel(RL_SETUP);
	// 		setErrorStatus(EC_NONE);
	// 		_ui->statusbar->showMessage(
	// 				QString("New program: ") + QString(process->GetExe()));
	// 		clearGlTraceItemList();

	// 		/* Attach to programm. */
	// 		if (!_dbg->attachToProgram(process->GetPid())) {
	// 			QMessageBox::critical(this, "Error", "Attaching to process "
	// 					"failed.");
	// 			setRunLevel(RL_INIT);
	// 			setErrorStatus(EC_UNKNOWN_ERROR);
	// 		} else {
	// 			//setRunLevel(RL_SETUP);
	// 			setErrorStatus(EC_NONE);
	// 		}
	// 	}
	// }
}

void MainWindow::on_aOnlineHelp_triggered()
{
	QUrl url("http://www.vis.uni-stuttgart.de/glsldevil/");
	QDesktopServices ds;
	ds.openUrl(url);
}

void MainWindow::on_aAbout_triggered()
{
	Dialog_AboutBox dlg(this);
	dlg.exec();
}

void MainWindow::on_tbBVCapture_clicked()
{
	// int width, height;
	// float *imageData;
	// ErrorCode error;

	// error = _dbg->readBackActiveRenderBuffer(3, &width, &height, &imageData);

	// if (error == EC_NONE) {
	// 	PixelBoxFloat imageBox(width, height, 3, imageData);
	// 	_lBVLabel->setPixmap(
	// 			QPixmap::fromImage(imageBox.getByteImage(PixelBox::FBM_CLAMP)));
	// 	_lBVLabel->resize(width, height);
	// 	_ui->tbBVSave->setEnabled(true);
	// } else {
	// 	setErrorStatus(error);
	// }
}

void MainWindow::on_tbBVCaptureAutomatic_toggled(bool b)
{
	// if (b) {
	// 	_ui->tbBVCapture->setEnabled(false);
	// 	on_tbBVCapture_clicked();
	// } else {
	// 	_ui->tbBVCapture->setEnabled(true);
	// }
}

void MainWindow::on_tbBVSave_clicked()
{
	// static QStringList history;
	// static QDir directory = QDir::current();

	// if (_lBVLabel->pixmap()) {
	// 	QImage img = _lBVLabel->pixmap()->toImage();
	// 	if (!img.isNull()) {
	// 		QFileDialog *sDialog = new QFileDialog(this, QString("Save image"));

	// 		sDialog->setAcceptMode(QFileDialog::AcceptSave);
	// 		sDialog->setFileMode(QFileDialog::AnyFile);
	// 		QStringList formatDesc;
	// 		formatDesc << "Portable Network Graphics (*.png)"
	// 				<< "Windows Bitmap (*.bmp)"
	// 				<< "Joint Photographic Experts Group (*.jpg, *.jepg)"
	// 				<< "Portable Pixmap (*.ppm)"
	// 				<< "Tagged Image File Format (*.tif, *.tiff)"
	// 				<< "X11 Bitmap (*.xbm, *.xpm)";
	// 		sDialog->setFilters(formatDesc);

	// 		if (!(history.isEmpty())) {
	// 			sDialog->setHistory(history);
	// 		}

	// 		sDialog->setDirectory(directory);

	// 		if (sDialog->exec()) {
	// 			QStringList files = sDialog->selectedFiles();
	// 			QString selected;
	// 			if (!files.isEmpty()) {
	// 				selected = files[0];
	// 				if (!(img.save(selected))) {

	// 					QString forceFilter;
	// 					QString filter = sDialog->selectedFilter();
	// 					if (filter
	// 							== QString(
	// 									"Portable Network Graphics (*.png)")) {
	// 						forceFilter.append("png");
	// 					} else if (filter
	// 							== QString("Windows Bitmap (*.bmp)")) {
	// 						forceFilter.append("bmp");
	// 					} else if (filter
	// 							== QString(
	// 									"Joint Photographic Experts Group (*.jpg, *.jepg)")) {
	// 						forceFilter.append("jpg");
	// 					} else if (filter
	// 							== QString("Portable Pixmap (*.ppm)")) {
	// 						forceFilter.append("ppm");
	// 					} else if (filter
	// 							== QString(
	// 									"Tagged Image File Format (*.tif, *.tiff)")) {
	// 						forceFilter.append("tif");
	// 					} else if (filter
	// 							== QString("X11 Bitmap (*.xbm, *.xpm)")) {
	// 						forceFilter.append("xbm");
	// 					}

	// 					img.save(selected, forceFilter.toLatin1().data());
	// 				}
	// 			}
	// 		}

	// 		history = sDialog->history();
	// 		directory = sDialog->directory();

	// 		delete sDialog;
	// 	}
	// }
}

ErrorCode MainWindow::getNextCall()
{
	// _currentCall = _dbg->getCurrentCall();
	// if (!m_bHaveValidShaderCode && _currentCall->isDebuggableDrawCall()) {
	// 	/* current call is a drawcall and we don't have valid shader code;
	// 	 * call debug function that reads back the shader code
	// 	 */
	// 	for (int i = 0; i < 3; i++) {
	// 		delete[] m_pShaders[i];
	// 		m_pShaders[i] = NULL;
	// 	}
	// 	delete[] m_serializedUniforms.pData;
	// 	m_serializedUniforms.pData = NULL;
	// 	m_serializedUniforms.count = 0;

	// 	ErrorCode error = _dbg->getShaderCode(m_pShaders, &m_dShResources,
	// 			&m_serializedUniforms.pData, &m_serializedUniforms.count);
	// 	if (error == EC_NONE) {
	// 		/* show shader code(s) in tabs */
	// 		setShaderCodeText(m_pShaders);
	// 		if (m_pShaders[0] != NULL || m_pShaders[1] != NULL
	// 				|| m_pShaders[2] != NULL) {
	// 			m_bHaveValidShaderCode = true;
	// 		}
	// 	} else if (isErrorCritical(error)) {
	// 		return error;
	// 	}
	// }
	return EC_NONE;
}

void MainWindow::on_tbExecute_clicked()
{
	UT_NOTIFY(LV_INFO, "Executing");

	ProcessPtr p = Debugger::instance().create(_debugConfig);
	try {
		p->launch();
	} 
	catch (std::exception& e) {
		UT_NOTIFY(LV_ERROR, e.what());
	}
	_proc = p;
	CommandFactory& c = _proc->commandFactory();
	auto cmd = c.dbgExecute(false);
	auto a = cmd.get();
	auto x = dynamic_cast<ExecuteCommand::Result*>(a.data());
	UT_NOTIFY(LV_INFO, "result " << x->valid << " " << x->value);
	// /* Cleanup shader debugging */
	// cleanupDBGShader();

	// if (_currentRunLevel == RL_SETUP) {
	// 	int i;

	// 	/* Clear previous status */
	// 	_ui->statusbar->showMessage(QString(""));
	// 	clearGlTraceItemList();
	// 	_traceStatsWidget->resetStatistics();

	// 	m_bHaveValidShaderCode = false;

	// 	setErrorStatus(EC_NONE);

	// 	/* Execute prog */
	// 	if(!_dbg->runProgram(_debugConfig)) {
	// 		setRunLevel(RL_SETUP);
	// 	} else {
	// 		_ui->statusbar->showMessage(QString("Executing " + _debugConfig.programArgs[0]));
	// 		addGlTraceWarningItem("Program Start");
	// 	}
	// } else {
	// 	/* cleanup dbg state */
	// 	leaveDBGState();

	// 	/* Stop already running progs */
	// 	killDebuggee();

	// 	setRunLevel(RL_SETUP);
	// }
}

void MainWindow::addGlTraceItem(ProcessPtr p)
{
	// QIcon icon;
	// QString iconText;
	// GlTraceListItem::IconType iconType;

	// if (_currentRunLevel == RL_TRACE_EXECUTE_NO_DEBUGABLE
	// 		|| _currentRunLevel == RL_TRACE_EXECUTE_IS_DEBUGABLE) {
	// 	iconType = GlTraceListItem::IT_ACTUAL;
	// } else if (_currentRunLevel == RL_TRACE_EXECUTE_RUN) {
	// 	iconType = GlTraceListItem::IT_OK;
	// } else {
	// 	iconType = GlTraceListItem::IT_EMPTY;
	// }
	// FunctionCallPtr func = p->getCurrentCall();
	// _traceStatsWidget->addGlTraceItem(*func);

	// if (_glTraceModel) {
	// 	_glTraceModel->addGlTraceItem(iconType, func->asString());
	// }
	// _ui->lvGlTrace->scrollToBottom();
}

void MainWindow::addGlTraceErrorItem(const QString& text)
{
	_glTraceModel->addGlTraceItem(GlTraceListItem::IT_ERROR, text);
	_ui->lvGlTrace->scrollToBottom();
}

void MainWindow::addGlTraceWarningItem(const QString& text)
{
	_glTraceModel->addGlTraceItem(GlTraceListItem::IT_WARNING, text);
	_ui->lvGlTrace->scrollToBottom();
}

void MainWindow::setGlTraceItemIconType(const GlTraceListItem::IconType type)
{
	_glTraceModel->setCurrentGlTraceIconType(type, -1);
}

void MainWindow::setGlTraceItemText(const QString& text)
{
	_glTraceModel->setCurrentGlTraceText(text, -1);
}

void MainWindow::clearGlTraceItemList(void)
{
	_glTraceModel->clear();
}

void MainWindow::setShaderCodeText(char *shaders[3])
{
	// if (shaders && shaders[0]) {
	// 	/* make a new document, the old one get's deleted by qt */
	// 	QTextDocument *newDoc = new QTextDocument(QString(shaders[0]),
	// 			_ui->teVertexShader);
	// 	/* the document becomes owner of the highlighter, so it get's freed */
	// 	GlslSyntaxHighlighter *highlighter;
	// 	highlighter = new GlslSyntaxHighlighter(newDoc);
	// 	_ui->teVertexShader->setDocument(newDoc);
	// 	_ui->teVertexShader->setTabStopWidth(30);
	// } else {
	// 	/* make a new empty document, the old one get's deleted by qt */
	// 	QTextDocument *newDoc = new QTextDocument(QString(""), _ui->teVertexShader);
	// 	_ui->teVertexShader->setDocument(newDoc);
	// }
	// if (shaders && shaders[1]) {
	// 	/* make a new document, the old one get's deleted by qt */
	// 	QTextDocument *newDoc = new QTextDocument(QString(shaders[1]),
	// 			_ui->teGeometryShader);
	// 	/* the document becomes owner of the highlighter, so it get's freed */
	// 	GlslSyntaxHighlighter *highlighter;
	// 	highlighter = new GlslSyntaxHighlighter(newDoc);
	// 	_ui->teGeometryShader->setDocument(newDoc);
	// 	_ui->teGeometryShader->setTabStopWidth(30);
	// } else {
	// 	/* make a new empty document, the old one get's deleted by qt */
	// 	QTextDocument *newDoc = new QTextDocument(QString(""),
	// 			_ui->teGeometryShader);
	// 	_ui->teGeometryShader->setDocument(newDoc);
	// }
	// if (shaders && shaders[2]) {
	// 	/* make a new document, the old one get's deleted by qt */
	// 	QTextDocument *newDoc = new QTextDocument(QString(shaders[2]),
	// 			_ui->teFragmentShader);
	// 	/* the document becomes owner of the highlighter, so it get's freed */
	// 	GlslSyntaxHighlighter *highlighter;
	// 	highlighter = new GlslSyntaxHighlighter(newDoc);
	// 	_ui->teFragmentShader->setDocument(newDoc);
	// 	_ui->teFragmentShader->setTabStopWidth(30);
	// } else {
	// 	/* make a new empty document, the old one get's deleted by qt */
	// 	QTextDocument *newDoc = new QTextDocument(QString(""),
	// 			_ui->teFragmentShader);
	// 	_ui->teFragmentShader->setDocument(newDoc);
	// }
}

ErrorCode MainWindow::nextStep()
{
	ErrorCode error;

	// if (_currentCall && _currentCall->isShaderSwitch()) {
	// 	/* current call is a glsl shader switch */

	// 	/* call shader switch */
	// 	error = _dbg->callOrigFunc(_currentCall);

	// 	if (error != EC_NONE) {
	// 		if (isErrorCritical(error)) {
	// 			return error;
	// 		}
	// 	} else {
	// 		/* call debug function that reads back the shader code */
	// 		for (int i = 0; i < 3; i++) {
	// 			delete[] m_pShaders[i];
	// 			m_pShaders[i] = NULL;
	// 		}
	// 		delete[] m_serializedUniforms.pData;
	// 		m_serializedUniforms.pData = NULL;
	// 		m_serializedUniforms.count = 0;
	// 		error = _dbg->getShaderCode(m_pShaders, &m_dShResources,
	// 				&m_serializedUniforms.pData, &m_serializedUniforms.count);
	// 		if (error == EC_NONE) {
	// 			/* show shader code(s) in tabs */
	// 			setShaderCodeText(m_pShaders);
	// 			if (m_pShaders[0] != NULL || m_pShaders[1] != NULL
	// 					|| m_pShaders[2] != NULL) {
	// 				m_bHaveValidShaderCode = true;
	// 			} else {
	// 				m_bHaveValidShaderCode = false;
	// 			}
	// 		} else if (isErrorCritical(error)) {
	// 			return error;
	// 		}
	// 	}
	// } else {
	// 	/* current call is a "normal" function call */
	// 	error = _dbg->callOrigFunc(_currentCall);
	// 	if (isErrorCritical(error)) {
	// 		return error;
	// 	}
	// }
	// /* Readback image if requested by user */
	// if (_ui->tbBVCaptureAutomatic->isChecked()
	// 		&& _currentCall->isFramebufferChange()) {
	// 	on_tbBVCapture_clicked();
	// }
	// if (error == EC_NONE) {
	// 	error = _dbg->callDone();
	// } else {
	// 	/* TODO: what about the error code ??? */
	// 	_dbg->callDone();
	// }
	return error;
}

void MainWindow::on_tbStep_clicked()
{
	// leaveDBGState();

	// setRunLevel(RL_TRACE_EXECUTE_RUN);

	// singleStep();

	// if (_currentRunLevel != RL_TRACE_EXECUTE_RUN) {
	// 	// a critical error occured in step */
	// 	return;
	// }

	// while (_currentRunLevel == RL_TRACE_EXECUTE_RUN
	// 		&& !_glTraceFilterModel->isFunctionVisible(
	// 				_currentCall->name())) {
	// 	singleStep();
	// 	qApp->processEvents(QEventLoop::AllEvents);
	// 	if (_currentRunLevel == RL_SETUP) {
	// 		/* something was wrong in step */
	// 		return;
	// 	}
	// }

	// setRunLevel(RL_TRACE_EXECUTE);
	// setGlTraceItemIconType(GlTraceListItem::IT_ACTUAL);
}

void MainWindow::singleStep()
{
	// if (_currentCall && _currentCall->isFrameEnd()) {
	// 	_traceStatsWidget->resetPerFrameStatistics();
	// }
	// /* cleanup dbg state */
	// leaveDBGState();

	// setGlTraceItemIconType(GlTraceListItem::IT_OK);

	// ErrorCode error = nextStep();

	// /* Error handling */
	// setErrorStatus(error);
	// if (isErrorCritical(error)) {
	// 	killDebuggee();
	// 	setRunLevel(RL_SETUP);
	// 	return;
	// } else {
	// 	if (_currentRunLevel == RL_TRACE_EXECUTE_RUN && isOpenGLError(error)
	// 			&& _ui->tbToggleHaltOnError->isChecked()) {
	// 		setRunLevel(RL_TRACE_EXECUTE);
	// 	}
	// 	error = getNextCall();
	// 	setErrorStatus(error);
	// 	if (isErrorCritical(error)) {
	// 		killDebuggee();
	// 		setRunLevel(RL_SETUP);
	// 		return;
	// 	}
	// 	if (_currentRunLevel != RL_TRACE_EXECUTE_RUN) {
	// 		setRunLevel(RL_TRACE_EXECUTE);
	// 	}
	// 	addGlTraceItem();
	// }
}

void MainWindow::on_tbSkip_clicked()
{
	// if (_currentCall && _currentCall->isFrameEnd()) {
	// 	_traceStatsWidget->resetPerFrameStatistics();
	// }
	// /* cleanup dbg state */
	// leaveDBGState();

	// setGlTraceItemIconType(GlTraceListItem::IT_ERROR);

	// ErrorCode error = _dbg->callDone();

	// /* Error handling */
	// setErrorStatus(error);
	// if (isErrorCritical(error)) {
	// 	killDebuggee();
	// 	setRunLevel(RL_SETUP);
	// } else {
	// 	error = getNextCall();
	// 	setErrorStatus(error);
	// 	if (isErrorCritical(error)) {
	// 		killDebuggee();
	// 		setRunLevel(RL_SETUP);
	// 		return;
	// 	}
	// 	setRunLevel(RL_TRACE_EXECUTE);
	// 	addGlTraceItem();
	// }
}

void MainWindow::on_tbEdit_clicked()
{
	// EditCallDialog *dialog = new EditCallDialog(*_currentCall);
	// dialog->exec();

	// if (dialog->result() == QDialog::Accepted) {
	// 	FunctionCallPtr editCall = dialog->editedFunctionCall();

	// 	if (_currentCall && _currentCall->isFrameEnd()) {
	// 		_traceStatsWidget->resetPerFrameStatistics();
	// 	}		

	// 	if (*editCall != *_currentCall) {
	// 		setGlTraceItemText(editCall->asString());
	// 		setGlTraceItemIconType(GlTraceListItem::IT_IMPORTANT);

	// 		/* Send data to debug library */
	// 		_dbg->overwriteFuncArguments(editCall);

	// 		/* Replace current call by edited call */
	// 		_currentCall = editCall;
	// 	}
	// }
	// delete dialog;
}


void MainWindow::on_tbJumpToDrawCall_clicked()
{
	// /* cleanup dbg state */
	// leaveDBGState();

	// setRunLevel(RL_TRACE_EXECUTE_RUN);

	// if (_ui->tbToggleNoTrace->isChecked()) {
	// 	setShaderCodeText(NULL);
	// 	_traceStatsWidget->resetStatistics();
	// 	_ui->statusbar->showMessage(QString("Running program without tracing"));
	// 	clearGlTraceItemList();
	// 	addGlTraceWarningItem("Running program without call tracing");
	// 	addGlTraceWarningItem("Statistics reset!");
	// 	qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

	// 	m_bHaveValidShaderCode = false;

	// 	ErrorCode error = _dbg->executeToDrawCall(
	// 			_ui->tbToggleHaltOnError->isChecked());
	// 	setErrorStatus(error);
	// 	if (error != EC_NONE) {
	// 		killDebuggee();
	// 		setRunLevel(RL_SETUP);
	// 		return;
	// 	}
	// 	waitForEndOfExecution(); /* last statement!! */
	// } else {
	// 	if (_currentCall && _currentCall->isDebuggableDrawCall()) {
	// 		singleStep();
	// 		if (_currentRunLevel == RL_SETUP) {
	// 			/* something was wrong in step */
	// 			return;
	// 		}
	// 	}

	// 	while (_currentRunLevel == RL_TRACE_EXECUTE_RUN
	// 			&& (!_currentCall
	// 					|| (_currentCall
	// 							&& !_currentCall->isDebuggableDrawCall()))) {
	// 		singleStep();
	// 		if (_currentRunLevel == RL_SETUP) {
	// 			/* something was wrong in step */
	// 			return;
	// 		}
	// 		qApp->processEvents(QEventLoop::AllEvents);
	// 	}
	// }

	// setRunLevel(RL_TRACE_EXECUTE);
	// setGlTraceItemIconType(GlTraceListItem::IT_ACTUAL);
}

void MainWindow::on_tbJumpToShader_clicked()
{
	// /* cleanup dbg state */
	// leaveDBGState();

	// setRunLevel(RL_TRACE_EXECUTE_RUN);

	// if (_ui->tbToggleNoTrace->isChecked()) {
	// 	setShaderCodeText(NULL);
	// 	_traceStatsWidget->resetStatistics();
	// 	_ui->statusbar->showMessage(QString("Running program without tracing"));
	// 	clearGlTraceItemList();
	// 	addGlTraceWarningItem("Running program without call tracing");
	// 	addGlTraceWarningItem("Statistics reset!");
	// 	qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

	// 	m_bHaveValidShaderCode = false;

	// 	ErrorCode error = _dbg->executeToShaderSwitch(
	// 			_ui->tbToggleHaltOnError->isChecked());
	// 	setErrorStatus(error);
	// 	if (error != EC_NONE) {
	// 		killDebuggee();
	// 		setRunLevel(RL_SETUP);
	// 		return;
	// 	}
	// 	waitForEndOfExecution(); /* last statement!! */
	// } else {
	// 	if (_currentCall && _currentCall->isShaderSwitch()) {
	// 		singleStep();
	// 		if (_currentRunLevel == RL_SETUP) {
	// 			/* something was wrong in step */
	// 			return;
	// 		}
	// 	}

	// 	while (_currentRunLevel == RL_TRACE_EXECUTE_RUN
	// 			&& (!_currentCall
	// 					|| (_currentCall && !_currentCall->isShaderSwitch()))) {
	// 		singleStep();
	// 		if (_currentRunLevel == RL_SETUP) {
	// 			/* something was wrong in step */
	// 			return;
	// 		}
	// 		qApp->processEvents(QEventLoop::AllEvents);
	// 	}
	// }

	// setRunLevel(RL_TRACE_EXECUTE);
	// setGlTraceItemIconType(GlTraceListItem::IT_ACTUAL);
}

void MainWindow::on_tbJumpToUserDef_clicked()
{
	// static QString targetName;
	// JumpToDialog *pJumpToDialog = new JumpToDialog(targetName);

	// pJumpToDialog->exec();

	// if (pJumpToDialog->result() == QDialog::Accepted) {
	// 	/* cleanup dbg state */
	// 	leaveDBGState();

	// 	setRunLevel(RL_TRACE_EXECUTE_RUN);
	// 	targetName = pJumpToDialog->getTargetFuncName();

	// 	if (_ui->tbToggleNoTrace->isChecked()) {
	// 		setShaderCodeText(NULL);
	// 		_traceStatsWidget->resetStatistics();
	// 		_ui->statusbar->showMessage(QString("Running program without tracing"));
	// 		clearGlTraceItemList();
	// 		addGlTraceWarningItem("Running program without call tracing");
	// 		addGlTraceWarningItem("Statistics reset!");
	// 		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

	// 		m_bHaveValidShaderCode = false;

	// 		ErrorCode error = _dbg->executeToUserDefined(
	// 				targetName.toAscii().data(),
	// 				_ui->tbToggleHaltOnError->isChecked());
	// 		setErrorStatus(error);
	// 		if (error != EC_NONE) {
	// 			killDebuggee();
	// 			setRunLevel(RL_SETUP);
	// 			return;
	// 		}
	// 		waitForEndOfExecution(); /* last statement!! */
	// 	} else {

	// 		singleStep();
	// 		if (_currentRunLevel == RL_SETUP) {
	// 			/* something was wrong in step */
	// 			return;
	// 		}

	// 		while (_currentRunLevel == RL_TRACE_EXECUTE_RUN && 
	// 			(!_currentCall || 
	// 				(_currentCall && targetName != _currentCall->name()))) {
	// 			singleStep();
	// 			if (_currentRunLevel == RL_SETUP) {
	// 				/* something was wrong in step */
	// 				return;
	// 			}
	// 			qApp->processEvents(QEventLoop::AllEvents);
	// 		}
	// 	}

	// 	setRunLevel(RL_TRACE_EXECUTE);
	// 	setGlTraceItemIconType(GlTraceListItem::IT_ACTUAL);
	// }

	// delete pJumpToDialog;
}

void MainWindow::on_tbRun_clicked()
{
	UT_NOTIFY(LV_TRACE, "Run clicked");
	// /* cleanup dbg state */
	// leaveDBGState();

	// setRunLevel(RL_TRACE_EXECUTE_RUN);
	// if (_ui->tbToggleNoTrace->isChecked()) {
	// 	setShaderCodeText(NULL);
	// 	_traceStatsWidget->resetStatistics();
	// 	_ui->statusbar->showMessage(QString("Running program without tracing"));
	// 	clearGlTraceItemList();
	// 	addGlTraceWarningItem("Running program without call tracing");
	// 	addGlTraceWarningItem("Statistics reset!");
	// 	qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

	// 	m_bHaveValidShaderCode = false;

	// 	ErrorCode error = _dbg->execute(_ui->tbToggleHaltOnError->isChecked());
	// 	setErrorStatus(error);
	// 	if (isErrorCritical(error)) {
	// 		killDebuggee();
	// 		setRunLevel(RL_SETUP);
	// 		return;
	// 	}
	// 	waitForEndOfExecution(); /* last statement !! */
	// } else {
	// 	while (_currentRunLevel == RL_TRACE_EXECUTE_RUN) {
	// 		singleStep();
	// 		if (_currentRunLevel == RL_SETUP) {
	// 			/* something was wrong in step */
	// 			return;
	// 		}
	// 		qApp->processEvents(QEventLoop::AllEvents);
	// 	}
	// 	setGlTraceItemIconType(GlTraceListItem::IT_ACTUAL);
	// }
}

void MainWindow::on_tbPause_clicked()
{
	UT_NOTIFY(LV_TRACE, "pause clicked");
	// if (_ui->tbToggleNoTrace->isChecked()) {
	// 	_dbg->stop();
	// } else {
	// 	setRunLevel(RL_TRACE_EXECUTE);
	// }
}

void MainWindow::setGuiUpdates(bool value)
{
	_traceStatsWidget->setUpdatesEnabled(value);
	_ui->lvGlTrace->setUpdatesEnabled(value);
	_ui->teFragmentShader->setUpdatesEnabled(value);
	_ui->teGeometryShader->setUpdatesEnabled(value);
	_ui->teVertexShader->setUpdatesEnabled(value);
}

void MainWindow::on_tbToggleGuiUpdate_clicked(bool value)
{
	setGuiUpdates(!value);
}

void MainWindow::on_tbToggleNoTrace_clicked(bool)
{
}

void MainWindow::on_tbToggleHaltOnError_clicked(bool)
{
}

void MainWindow::on_tbGlTraceSettings_clicked()
{
	_traceSettingsDialog->exec();
	_glTraceModel->resetLayout();
}

void MainWindow::on_tbSave_clicked()
{
	static QStringList history;
	static QDir directory = QDir::current();

	QFileDialog *sDialog = new QFileDialog(this, QString("Save GL Trace as"));

	sDialog->setAcceptMode(QFileDialog::AcceptSave);
	sDialog->setFileMode(QFileDialog::AnyFile);
	QStringList formatDesc;
	formatDesc << "Plain Text (*.txt)";
	sDialog->setFilters(formatDesc);

	if (!(history.isEmpty())) {
		sDialog->setHistory(history);
	}

	sDialog->setDirectory(directory);

	if (sDialog->exec()) {
		QStringList files = sDialog->selectedFiles();
		QString selected;
		if (!files.isEmpty()) {
			QString filter = sDialog->selectedFilter();
			if (filter == QString("Plain Text (*.txt)")) {
				QFile file(files[0]);
				if (file.open(QIODevice::WriteOnly)) {

					QTextStream out(&file);
					_glTraceModel->traverse(out, &GlTraceListItem::outputTXT);
				}
			}
		}
	}

	history = sDialog->history();
	directory = sDialog->directory();

	delete sDialog;
}

bool MainWindow::getDebugVertexData(DbgCgOptions option, ShChangeableList *cl,
		bool *coverage, VertexBox *vdata)
{
	// int target, elementsPerVertex, numVertices, numPrimitives,
	// 		forcePointPrimitiveMode;
	// float *data = NULL;
	// ErrorCode error;

	// char *shaders[] = {
	// 	m_pShaders[0],
	// 	m_pShaders[1],
	// 	m_pShaders[2] };

	// char *debugCode = NULL;
	// debugCode = ShDebugGetProg(m_dShCompiler, cl, &m_dShVariableList, option);
	// switch (_currentRunLevel) {
	// case RL_DBG_VERTEX_SHADER:
	// 	shaders[0] = debugCode;
	// 	shaders[1] = NULL;
	// 	target = DBG_TARGET_VERTEX_SHADER;
	// 	break;
	// case RL_DBG_GEOMETRY_SHADER:
	// 	shaders[1] = debugCode;
	// 	target = DBG_TARGET_GEOMETRY_SHADER;
	// 	break;
	// default:
	// 	QMessageBox::critical(this, "Internal Error",
	// 			"MainWindow::getDebugVertexData called when debugging "
	// 					"non-vertex/geometry shader<br>Please report this probem to "
	// 					"<A HREF=\"mailto:glsldevil@vis.uni-stuttgart.de\">"
	// 					"glsldevil@vis.uni-stuttgart.de</A>.", QMessageBox::Ok);
	// 	free(debugCode);
	// 	return false;
	// }

	// switch (option) {
	// case DBG_CG_GEOMETRY_MAP:
	// 	elementsPerVertex = 3;
	// 	forcePointPrimitiveMode = 0;
	// 	break;
	// case DBG_CG_VERTEX_COUNT:
	// 	elementsPerVertex = 3;
	// 	forcePointPrimitiveMode = 1;
	// 	break;
	// case DBG_CG_GEOMETRY_CHANGEABLE:
	// 	elementsPerVertex = 2;
	// 	forcePointPrimitiveMode = 0;
	// 	break;
	// case DBG_CG_CHANGEABLE:
	// case DBG_CG_COVERAGE:
	// case DBG_CG_SELECTION_CONDITIONAL:
	// case DBG_CG_LOOP_CONDITIONAL:
	// 	if (target == DBG_TARGET_GEOMETRY_SHADER) {
	// 		elementsPerVertex = 1;
	// 		forcePointPrimitiveMode = 1;
	// 	} else {
	// 		elementsPerVertex = 1;
	// 		forcePointPrimitiveMode = 0;
	// 	}
	// 	break;
	// default:
	// 	elementsPerVertex = 1;
	// 	forcePointPrimitiveMode = 0;
	// 	break;
	// }

	// error = _dbg->shaderStepVertex(shaders, target, m_primitiveMode,
	// 		forcePointPrimitiveMode, elementsPerVertex, &numPrimitives,
	// 		&numVertices, &data);

	// /////// DEBUG
	// UT_NOTIFY(LV_DEBUG, ">>>>> DEBUG CG: ");
	// switch (option) {
	// case DBG_CG_GEOMETRY_MAP:
	// 	dbgPrintNoPrefix(DBGLVL_COMPILERINFO, "DBG_CG_GEOMETRY_MAP\n");
	// 	break;
	// case DBG_CG_VERTEX_COUNT:
	// 	dbgPrintNoPrefix(DBGLVL_COMPILERINFO, "DBG_CG_VERTEX_COUNT\n");
	// 	break;
	// case DBG_CG_GEOMETRY_CHANGEABLE:
	// 	dbgPrintNoPrefix(DBGLVL_COMPILERINFO, "DBG_CG_GEOMETRY_CHANGEABLE\n");
	// 	break;
	// case DBG_CG_CHANGEABLE:
	// 	dbgPrintNoPrefix(DBGLVL_COMPILERINFO, "DBG_CG_CHANGEABLE\n");
	// 	break;
	// case DBG_CG_COVERAGE:
	// 	dbgPrintNoPrefix(DBGLVL_COMPILERINFO, "DBG_CG_COVERAGE\n");
	// 	break;
	// case DBG_CG_SELECTION_CONDITIONAL:
	// 	dbgPrintNoPrefix(DBGLVL_COMPILERINFO, "DBG_CG_SELECTION_CONDITIONAL\n");
	// 	break;
	// case DBG_CG_LOOP_CONDITIONAL:
	// 	dbgPrintNoPrefix(DBGLVL_COMPILERINFO, "DBG_CG_LOOP_CONDITIONAL\n");
	// 	break;
	// default:
	// 	dbgPrintNoPrefix(DBGLVL_COMPILERINFO, "XXXXXX FIXME XXXXX\n");
	// 	break;
	// }
	// if (_currentRunLevel == RL_DBG_VERTEX_SHADER) {
	// 	dbgPrint(DBGLVL_COMPILERINFO,
	// 			">>>>> DEBUG VERTEX SHADER:\n %s\n", shaders[0]);
	// } else {
	// 	dbgPrint(DBGLVL_COMPILERINFO,
	// 			">>>>> DEBUG GEOMETRY SHADER:\n %s\n", shaders[1]);
	// }
	// dbgPrint(DBGLVL_INFO,
	// 		"getDebugVertexData: numPrimitives=%i numVertices=%i\n", numPrimitives, numVertices);
	// ////////////////

	// free(debugCode);
	// if (error != EC_NONE) {
	// 	setErrorStatus(error);
	// 	if (isErrorCritical(error)) {
	// 		cleanupDBGShader();
	// 		setRunLevel(RL_SETUP);
	// 		QMessageBox::critical(this, "Critical Error", "Could not debug "
	// 				"shader. An error occured!", QMessageBox::Ok);
	// 		UT_NOTIFY(LV_ERROR,
	// 				"Critical Error in getDebugVertexData: " << getErrorDescription(error));
	// 		killDebuggee();
	// 		return false;
	// 	}
	// 	QMessageBox::critical(this, "Error", "Could not debug "
	// 			"shader. An error occured!", QMessageBox::Ok);
	// 	UT_NOTIFY(LV_WARN,
	// 			"Error in getDebugVertexData: " << getErrorDescription(error));
	// 	return false;
	// }

	// vdata->setData(data, elementsPerVertex, numVertices, numPrimitives,
	// 		coverage);
	// free(data);
	// UT_NOTIFY(LV_TRACE, "getDebugVertexData done");
	return true;
}

bool MainWindow::getDebugImage(DbgCgOptions option, ShChangeableList *cl,
		int rbFormat, bool *coverage, PixelBox **fbData)
{
	// int width, height, channels;
	// void *imageData;
	// ErrorCode error;

	// char *shaders[] = {
	// 	m_pShaders[0],
	// 	m_pShaders[1],
	// 	m_pShaders[2] };

	// if (_currentRunLevel != RL_DBG_FRAGMENT_SHADER) {
	// 	QMessageBox::critical(this, "Internal Error",
	// 			"MainWindow::getDebugImage called when debugging "
	// 					"debugging non-fragment shader<br>Please report this probem to "
	// 					"<A HREF=\"mailto:glsldevil@vis.uni-stuttgart.de\">"
	// 					"glsldevil@vis.uni-stuttgart.de</A>.", QMessageBox::Ok);
	// 	return false;
	// }

	// char *debugCode = NULL;
	// debugCode = ShDebugGetProg(m_dShCompiler, cl, &m_dShVariableList, option);
	// shaders[2] = debugCode;

	// switch (option) {
	// case DBG_CG_CHANGEABLE:
	// case DBG_CG_COVERAGE:
	// 	channels = 1;
	// 	break;
	// case DBG_CG_LOOP_CONDITIONAL:
	// case DBG_CG_SELECTION_CONDITIONAL:
	// 	if (_currentRunLevel == RL_DBG_FRAGMENT_SHADER) {
	// 		channels = 1;
	// 	} else {
	// 		channels = 3;
	// 	}
	// 	break;
	// default:
	// 	channels = 3;
	// }

	// UT_NOTIFY(LV_TRACE, "Init buffers...");
	// switch (option) {
	// case DBG_CG_ORIGINAL_SRC:
	// 	error = _dbg->initializeRenderBuffer(true, true, true, true, 0.0, 0.0,
	// 			0.0, 0.0, 0.0, 0);
	// 	break;
	// case DBG_CG_COVERAGE:
	// case DBG_CG_SELECTION_CONDITIONAL:
	// case DBG_CG_LOOP_CONDITIONAL:
	// case DBG_CG_CHANGEABLE:
	// 	error = _dbg->initializeRenderBuffer(false, _fragmentTestDialog->copyAlpha(),
	// 			_fragmentTestDialog->copyDepth(), _fragmentTestDialog->copyStencil(), 0.0, 0.0,
	// 			0.0, _fragmentTestDialog->alphaValue(), _fragmentTestDialog->depthValue(),
	// 			_fragmentTestDialog->stencilValue());
	// 	break;
	// default: {
	// 	QString msg;
	// 	msg.append("Unhandled DbgCgCoption ");
	// 	msg.append(QString::number(option));
	// 	msg.append("<BR>Please report this probem to "
	// 			"<A HREF=\"mailto:glsldevil@vis.uni-stuttgart.de\">"
	// 			"glsldevil@vis.uni-stuttgart.de</A>.");
	// 	QMessageBox::critical(this, "Internal Error", msg, QMessageBox::Ok);
	// 	return false;
	// }
	// }
	// setErrorStatus(error);
	// if (isErrorCritical(error)) {
	// 	cleanupDBGShader();
	// 	setRunLevel(RL_SETUP);
	// 	QMessageBox::critical(this, "Error", "Could not initialize buffers for "
	// 			"fragment program debugging.", QMessageBox::Ok);
	// 	killDebuggee();
	// 	return false;
	// }

	// error = _dbg->shaderStepFragment(shaders, channels, rbFormat, &width, &height,
	// 		&imageData);
	// free(debugCode);
	// if (error != EC_NONE) {
	// 	setErrorStatus(error);
	// 	if (isErrorCritical(error)) {
	// 		cleanupDBGShader();
	// 		setRunLevel(RL_SETUP);
	// 		QMessageBox::critical(this, "Error", "Could not debug fragment "
	// 				"shader. An error occured!", QMessageBox::Ok);
	// 		killDebuggee();
	// 		return false;
	// 	}
	// 	QMessageBox::critical(this, "Error", "Could not debug fragment "
	// 			"shader. An error occured!", QMessageBox::Ok);
	// 	return false;
	// }

	// if (rbFormat == GL_FLOAT) {
	// 	PixelBoxFloat *fb = new PixelBoxFloat(width, height, channels,
	// 			(float*) imageData, coverage);
	// 	if (*fbData) {
	// 		PixelBoxFloat *pfbData = dynamic_cast<PixelBoxFloat*>(*fbData);
	// 		pfbData->addPixelBox(fb);
	// 		delete fb;
	// 	} else {
	// 		*fbData = fb;
	// 	}
	// } else if (rbFormat == GL_INT) {
	// 	PixelBoxInt *fb = new PixelBoxInt(width, height, channels,
	// 			(int*) imageData, coverage);
	// 	if (*fbData) {
	// 		PixelBoxInt *pfbData = dynamic_cast<PixelBoxInt*>(*fbData);
	// 		pfbData->addPixelBox(fb);
	// 		delete fb;
	// 	} else {
	// 		*fbData = fb;
	// 	}
	// } else if (rbFormat == GL_UNSIGNED_INT) {
	// 	PixelBoxUInt *fb = new PixelBoxUInt(width, height, channels,
	// 			(unsigned int*) imageData, coverage);
	// 	if (*fbData) {
	// 		PixelBoxUInt *pfbData = dynamic_cast<PixelBoxUInt*>(*fbData);
	// 		pfbData->addPixelBox(fb);
	// 		delete fb;
	// 	} else {
	// 		*fbData = fb;
	// 	}
	// } else {
	// 	UT_NOTIFY(LV_ERROR, "Invalid image data format");
	// }

	// free(imageData);
	// UT_NOTIFY(LV_TRACE, "getDebugImage done.");
	return true;
}

void MainWindow::updateWatchItemData(ShVarItem *watchItem)
{
	// ShChangeableList cl;

	// cl.numChangeables = 0;
	// cl.changeables = NULL;

	// ShChangeable *watchItemCgbl = watchItem->getShChangeable();
	// addShChangeable(&cl, watchItemCgbl);

	// int rbFormat = watchItem->getReadbackFormat();

	// if (_currentRunLevel == RL_DBG_FRAGMENT_SHADER) {
	// 	PixelBox *fb = watchItem->getPixelBoxPointer();
	// 	if (fb) {
	// 		if (getDebugImage(DBG_CG_CHANGEABLE, &cl, rbFormat, m_pCoverage,
	// 				&fb)) {
	// 			watchItem->setCurrentValue(_selectedPixel.x, _selectedPixel.y);
	// 		} else {
	// 			QMessageBox::warning(this, "Warning",
	// 					"The requested data could "
	// 							"not be retrieved.");
	// 		}
	// 	} else {
	// 		if (getDebugImage(DBG_CG_CHANGEABLE, &cl, rbFormat, m_pCoverage,
	// 				&fb)) {
	// 			watchItem->setPixelBoxPointer(fb);
	// 			watchItem->setCurrentValue(_selectedPixel.x, _selectedPixel.y);
	// 		} else {
	// 			QMessageBox::warning(this, "Warning",
	// 					"The requested data could "
	// 							"not be retrieved.");
	// 		}
	// 	}
	// } else if (_currentRunLevel == RL_DBG_VERTEX_SHADER) {
	// 	VertexBox *data = new VertexBox();
	// 	if (getDebugVertexData(DBG_CG_CHANGEABLE, &cl, m_pCoverage, data)) {
	// 		VertexBox *vb = watchItem->getVertexBoxPointer();
	// 		if (vb) {
	// 			vb->addVertexBox(data);
	// 			delete data;
	// 		} else {
	// 			watchItem->setVertexBoxPointer(data);
	// 		}
	// 		watchItem->setCurrentValue(_selectedPixel.x);
	// 	} else {
	// 		QMessageBox::warning(this, "Warning", "The requested data could "
	// 				"not be retrieved.");
	// 	}
	// } else if (_currentRunLevel == RL_DBG_GEOMETRY_SHADER) {
	// 	VertexBox *currentData = new VertexBox();

	// 	UT_NOTIFY(LV_TRACE, "Get CHANGEABLE:");
	// 	if (getDebugVertexData(DBG_CG_CHANGEABLE, &cl, m_pCoverage,
	// 			currentData)) {
	// 		VertexBox *vb = watchItem->getCurrentPointer();
	// 		if (vb) {
	// 			vb->addVertexBox(currentData);
	// 			delete currentData;
	// 		} else {
	// 			watchItem->setCurrentPointer(currentData);
	// 		}

	// 		VertexBox *vertexData = new VertexBox();

	// 		UT_NOTIFY(LV_TRACE, "Get GEOMETRY_CHANGABLE:");
	// 		if (getDebugVertexData(DBG_CG_GEOMETRY_CHANGEABLE, &cl, NULL,
	// 				vertexData)) {
	// 			VertexBox *vb = watchItem->getVertexBoxPointer();
	// 			if (vb) {
	// 				vb->addVertexBox(vertexData);
	// 				delete vertexData;
	// 			} else {
	// 				watchItem->setVertexBoxPointer(vertexData);
	// 			}
	// 		} else {
	// 			QMessageBox::warning(this, "Warning",
	// 					"The requested data could "
	// 							"not be retrieved.");
	// 		}
	// 		watchItem->setCurrentValue(_selectedPixel.x);
	// 	} else {
	// 		QMessageBox::warning(this, "Warning", "The requested data could "
	// 				"not be retrieved.");
	// 		return;
	// 	}
	// }
	// freeShChangeable(&watchItemCgbl);
}

static void invalidateWatchItemData(ShVarItem *item)
{
	if (item->getPixelBoxPointer()) {
		item->getPixelBoxPointer()->invalidateData();
		item->resetCurrentValue();
	}
	if (item->getCurrentPointer()) {
		item->getCurrentPointer()->invalidateData();
	}
	if (item->getVertexBoxPointer()) {
		item->getVertexBoxPointer()->invalidateData();
	}
}

void MainWindow::updateWatchListData(CoverageMapStatus cmstatus,
		bool forceUpdate)
{
	// QList<ShVarItem*> watchItems;
	// int i;

	// if (m_pShVarModel) {
	// 	watchItems = m_pShVarModel->getAllWatchItemPointers();
	// }

	// for (i = 0; i < watchItems.count(); i++) {
	// 	ShVarItem *item = watchItems[i];

	// 	UT_NOTIFY(LV_TRACE, ">>>>>>>>>>>>>>updateWatchListData: "
 //                << qPrintable(item->getFullName()) << " ("
 //                << item->isChanged() << ", "
 //                << item->hasEnteredScope() << ", "
 //                << item->isInScope() << ", "
 //                << item->isInScopeStack() << ")");

	// 	if (forceUpdate) {
	// 		if (item->isInScope() || item->isBuildIn()
	// 				|| item->isInScopeStack()) {
	// 			updateWatchItemData(item);
	// 		} else {
	// 			invalidateWatchItemData(item);
	// 		}
	// 	} else if ((item->isChanged() || item->hasEnteredScope())
	// 			&& (item->isInScope() || item->isInScopeStack())) {
	// 		updateWatchItemData(item);
	// 	} else if (item->hasLeftScope()) {
	// 		invalidateWatchItemData(item);
	// 	} else {
	// 		/* If covermap grows larger, more readbacks could become possible */
	// 		if (cmstatus == COVERAGEMAP_GROWN) {
	// 			if (item->isInScope() || item->isBuildIn()
	// 					|| item->isInScopeStack()) {
	// 				if (_currentRunLevel == RL_DBG_FRAGMENT_SHADER) {
	// 					PixelBox *dataBox = item->getPixelBoxPointer();
	// 					if (!(dataBox->isAllDataAvailable())) {
	// 						updateWatchItemData(item);
	// 					}
	// 				} else {
	// 					updateWatchItemData(item);
	// 				}
	// 			} else {
	// 				invalidateWatchItemData(item);
	// 			}
	// 		}
	// 	}
	// 	/* HACK: when an error occurs in shader debugging the runlevel
	// 	 * might change to RL_SETUP and all shader debugging data will
	// 	 * be invalid; so we have to check it here
	// 	 */
	// 	if (_currentRunLevel == RL_SETUP) {
	// 		return;
	// 	}
	// }

	// /* Now update all windows to update themselves if necessary */
	// QWidgetList windowList = _workspace->windowList();

	// for (i = 0; i < windowList.count(); i++) {
	// 	WatchView *wv = static_cast<WatchView*>(windowList[i]);
	// 	wv->updateView(cmstatus != COVERAGEMAP_UNCHANGED);
	// }
	// /* update view */
	// m_pShVarModel->currentValuesChanged();
}

void MainWindow::updateWatchItemsCoverage(bool *coverage)
{
	// QList<ShVarItem*> watchItems;
	// int i;

	// if (m_pShVarModel) {
	// 	watchItems = m_pShVarModel->getAllWatchItemPointers();
	// }

	// for (i = 0; i < watchItems.count(); i++) {
	// 	ShVarItem *item = watchItems[i];
	// 	if (_currentRunLevel == RL_DBG_FRAGMENT_SHADER) {
	// 		PixelBox *fb = item->getPixelBoxPointer();
	// 		fb->setNewCoverage(coverage);
	// 		item->setCurrentValue(_selectedPixel.x, _selectedPixel.y);
	// 	} else if (_currentRunLevel == RL_DBG_VERTEX_SHADER) {
	// 		VertexBox *vb = item->getVertexBoxPointer();
	// 		vb->setNewCoverage(coverage);
	// 		item->setCurrentValue(_selectedPixel.x);
	// 	} else if (_currentRunLevel == RL_DBG_GEOMETRY_SHADER) {
	// 		VertexBox *cb = item->getCurrentPointer();
	// 		cb->setNewCoverage(coverage);
	// 		item->setCurrentValue(_selectedPixel.x);
	// 	}
	// }
	// /* update view */
	// m_pShVarModel->currentValuesChanged();
}

void MainWindow::resetWatchListData(void)
{
	// QList<ShVarItem*> watchItems;
	// int i;

	// if (m_pShVarModel) {
	// 	watchItems = m_pShVarModel->getAllWatchItemPointers();
	// 	for (i = 0; i < watchItems.count(); i++) {
	// 		ShVarItem *item = watchItems[i];
	// 		if (item->isInScope() || item->isBuildIn()) {
	// 			updateWatchItemData(item);
	// 			 HACK: when an error occurs in shader debugging the runlevel
	// 			 * might change to RL_SETUP and all shader debugging data will
	// 			 * be invalid; so we have to check it here
				 
	// 			if (_currentRunLevel == RL_SETUP) {
	// 				return;
	// 			}
	// 		} else {
	// 			invalidateWatchItemData(item);
	// 		}
	// 	}
	// 	/* Now notify all windows to update themselves if necessary */
	// 	QWidgetList windowList = _workspace->windowList();
	// 	for (i = 0; i < windowList.count(); i++) {
	// 		WatchView *wv = static_cast<WatchView*>(windowList[i]);
	// 		wv->updateView(true);
	// 	}
	// 	/* update view */
	// 	m_pShVarModel->currentValuesChanged();
	// }
}

void MainWindow::shaderStep(int action, bool updateWatchData,
		bool updateCovermap)
{
	// bool updateGUI = true;

	// switch (action) {
	// case DBG_BH_RESET:
	// case DBG_BH_JUMPINTO:
	// case DBG_BH_FOLLOW_ELSE:
	// case DBG_BH_SELECTION_JUMP_OVER:
	// case DBG_BH_LOOP_CONTINUE:
	// 	updateGUI = true;
	// 	break;
	// case DBG_BH_LOOP_NEXT_ITER:
	// 	updateGUI = false;
	// 	break;
	// }

	// int debugOptions = EDebugOpIntermediate;
	// DbgResult *dr = NULL;
	// static int nOldCoverageMap = 0;
	// CoverageMapStatus cmstatus = COVERAGEMAP_UNCHANGED;

	// dr = ShDebugJumpToNext(m_dShCompiler, debugOptions, action);

	// if (dr) {
	// 	switch (dr->status) {
	// 	case DBG_RS_STATUS_OK: {
	// 		/* Update scope list and mark changed variables */
	// 		m_pShVarModel->setChangedAndScope(dr->cgbls, dr->scope,
	// 				dr->scopeStack);

	// 		if (_currentRunLevel == RL_DBG_FRAGMENT_SHADER && updateCovermap) {
	// 			/* Read cover map */
	// 			PixelBoxFloat *pCoverageBox = NULL;
	// 			if (!(getDebugImage(DBG_CG_COVERAGE, NULL, GL_FLOAT, NULL,
	// 					(PixelBox**) &pCoverageBox))) {
	// 				QMessageBox::warning(this, "Warning", "An error "
	// 						"occurred while reading coverage.");
	// 				return;
	// 			}

	// 			/* Retrieve covermap from CoverageBox */
	// 			int nNewCoverageMap;
	// 			delete[] m_pCoverage;
	// 			m_pCoverage = pCoverageBox->getCoverageFromData(
	// 					&nNewCoverageMap);
	// 			updateWatchItemsCoverage(m_pCoverage);

	// 			if (nNewCoverageMap == nOldCoverageMap) {
	// 				cmstatus = COVERAGEMAP_UNCHANGED;
	// 			} else if (nNewCoverageMap > nOldCoverageMap) {
	// 				cmstatus = COVERAGEMAP_GROWN;
	// 			} else {
	// 				cmstatus = COVERAGEMAP_SHRINKED;
	// 			}
	// 			nOldCoverageMap = nNewCoverageMap;

	// 			delete pCoverageBox;
	// 		} else if ((_currentRunLevel == RL_DBG_GEOMETRY_SHADER
	// 				| _currentRunLevel == RL_DBG_VERTEX_SHADER)
	// 				&& updateCovermap) {
	// 			/* Retrieve cover map (one render pass 'DBG_CG_COVERAGE') */
	// 			VertexBox *pCoverageBox = new VertexBox(NULL);
	// 			if (!(getDebugVertexData(DBG_CG_COVERAGE, NULL, NULL,
	// 					pCoverageBox))) {
	// 				QMessageBox::warning(this, "Warning", "An error "
	// 						"occurred while reading vertex coverage.");
	// 				/* TODO: error handling */
	// 				UT_NOTIFY(LV_WARN, "Error reading vertex coverage!");
	// 				delete pCoverageBox;
	// 				cleanupDBGShader();
	// 				setRunLevel(RL_DBG_RESTART);
	// 				return;
	// 			}

	// 			/* Convert data to bool map: VertexBox -> bool
	// 			 * Check for change of covermap */
	// 			bool *newCoverage;
	// 			bool coverageChanged;
	// 			newCoverage = pCoverageBox->getCoverageFromData(m_pCoverage,
	// 					&coverageChanged);
	// 			delete[] m_pCoverage;
	// 			m_pCoverage = newCoverage;
	// 			updateWatchItemsCoverage(m_pCoverage);

	// 			if (coverageChanged) {
	// 				UT_NOTIFY(LV_INFO, "cmstatus = COVERAGEMAP_GROWN");
	// 				cmstatus = COVERAGEMAP_GROWN;
	// 			} else {
	// 				UT_NOTIFY(LV_INFO, "cmstatus = COVERAGEMAP_UNCHANGED");
	// 				cmstatus = COVERAGEMAP_UNCHANGED;
	// 			}

	// 			delete pCoverageBox;
	// 		}
	// 	}
	// 		break;
	// 	case DBG_RS_STATUS_FINISHED:
	// 		_ui->tbShaderStep->setEnabled(false);
	// 		_ui->tbShaderStepOver->setEnabled(false);
	// 		break;
	// 	default: {
	// 		QString msg;
	// 		msg.append("An unhandled debug result (");
	// 		msg.append(QString::number(dr->status));
	// 		msg.append(") occurred.");
	// 		msg.append("<br>Please report this probem to "
	// 				"<A HREF=\"mailto:glsldevil@vis.uni-stuttgart.de\">"
	// 				"glsldevil@vis.uni-stuttgart.de</A>.");
	// 		QMessageBox::critical(this, "Internal Error", msg, QMessageBox::Ok);
	// 		return;
	// 	}
	// 		break;
	// 	}

	// 	if (dr->position == DBG_RS_POSITION_DUMMY) {
	// 		_ui->tbShaderStep->setEnabled(false);
	// 		_ui->tbShaderStepOver->setEnabled(false);
	// 	}

	// 	/* Process watch list */
	// 	if (updateWatchData) {
	// 		UT_NOTIFY(LV_INFO,
	// 				"updateWatchData " << cmstatus << " emitVertex: " << dr->passedEmitVertex << " discard: " << dr->passedDiscard);
	// 		updateWatchListData(cmstatus,
	// 				dr->passedEmitVertex || dr->passedDiscard);
	// 	}

	// 	VertexBox vbCondition;

	// 	/* Process position dependent requests */
	// 	switch (dr->position) {
	// 	case DBG_RS_POSITION_SELECTION_IF_CHOOSE:
	// 	case DBG_RS_POSITION_SELECTION_IF_ELSE_CHOOSE: {
	// 		SelectionDialog *sDialog = NULL;
	// 		switch (_currentRunLevel) {
	// 		case RL_DBG_FRAGMENT_SHADER: {
	// 			PixelBoxFloat *imageBox = NULL;
	// 			if (getDebugImage(DBG_CG_SELECTION_CONDITIONAL, NULL, GL_FLOAT,
	// 					m_pCoverage, (PixelBox**) &imageBox)) {
	// 			} else {
	// 				QMessageBox::warning(this, "Warning",
	// 						"An error occurred while retrieving "
	// 								"the selection image.");
	// 				delete imageBox;
	// 				return;
	// 			}
	// 			sDialog = new SelectionDialog(imageBox,
	// 					dr->position
	// 							== DBG_RS_POSITION_SELECTION_IF_ELSE_CHOOSE,
	// 					this);
	// 			delete imageBox;
	// 		}
	// 			break;
	// 		case RL_DBG_GEOMETRY_SHADER: {
	// 			if (getDebugVertexData(DBG_CG_SELECTION_CONDITIONAL, NULL,
	// 					m_pCoverage, &vbCondition)) {
	// 			} else {
	// 				QMessageBox::warning(this, "Warning",
	// 						"An error occurred while retrieving "
	// 								"the selection condition.");
	// 				cleanupDBGShader();
	// 				setRunLevel(RL_DBG_RESTART);
	// 				return;
	// 			}

	// 			/* Create list of all watch item boxes */
	// 			QList<ShVarItem*> watchItems;
	// 			if (m_pShVarModel) {
	// 				watchItems = m_pShVarModel->getAllWatchItemPointers();
	// 			}

	// 			sDialog = new SelectionDialog(&vbCondition, watchItems,
	// 					m_primitiveMode, m_dShResources.geoOutputType,
	// 					m_pGeometryMap, m_pVertexCount,
	// 					dr->position
	// 							== DBG_RS_POSITION_SELECTION_IF_ELSE_CHOOSE,
	// 					this);
	// 		}
	// 			break;
	// 		case RL_DBG_VERTEX_SHADER: {
	// 			/* Get condition for each vertex */
	// 			if (getDebugVertexData(DBG_CG_SELECTION_CONDITIONAL, NULL,
	// 					m_pCoverage, &vbCondition)) {
	// 			} else {
	// 				QMessageBox::warning(this, "Warning",
	// 						"An error occurred while retrieving "
	// 								"the selection condition.");
	// 				cleanupDBGShader();
	// 				setRunLevel(RL_DBG_RESTART);
	// 				return;
	// 			}

	// 			/* Create list of all watch item boxes */
	// 			QList<ShVarItem*> watchItems;
	// 			if (m_pShVarModel) {
	// 				watchItems = m_pShVarModel->getAllWatchItemPointers();
	// 			}

	// 			sDialog = new SelectionDialog(&vbCondition, watchItems,
	// 					dr->position
	// 							== DBG_RS_POSITION_SELECTION_IF_ELSE_CHOOSE,
	// 					this);
	// 		}
	// 			break;
	// 		default:
	// 			// TODO: Is this an internal error?
	// 			QMessageBox::warning(this, "Warning",
	// 					"The current run level is invalid for "
	// 							"SelectionDialog.");
	// 		}
	// 		switch (sDialog->exec()) {
	// 		case SelectionDialog::SB_SKIP:
	// 			ShaderStep (DBG_BH_SELECTION_JUMP_OVER);
	// 			break;
	// 		case SelectionDialog::SB_IF:
	// 			ShaderStep (DBG_BH_JUMPINTO);
	// 			break;
	// 		case SelectionDialog::SB_ELSE:
	// 			ShaderStep (DBG_BH_FOLLOW_ELSE);
	// 			break;
	// 		}
	// 		delete sDialog;
	// 	}
	// 		break;
	// 	case DBG_RS_POSITION_LOOP_CHOOSE: {
	// 		LoopData *lData = NULL;
	// 		switch (_currentRunLevel) {
	// 		case RL_DBG_FRAGMENT_SHADER: {
	// 			PixelBoxFloat *loopCondition = NULL;

	// 			if (updateCovermap) {
	// 				/* First get image of loop condition */
	// 				if (!getDebugImage(DBG_CG_LOOP_CONDITIONAL, NULL, GL_FLOAT,
	// 						m_pCoverage, (PixelBox**) &loopCondition)) {
	// 					QMessageBox::warning(this, "Warning",
	// 							"An error occurred while retrieving "
	// 									"the loop image.");
	// 					delete loopCondition;
	// 					return;
	// 				}
	// 			}

	// 			/* Add data to the loop storage */
	// 			if (dr->loopIteration == 0) {
	// 				UT_NOTIFY(LV_INFO, "==> new loop encountered");
	// 				lData = new LoopData(loopCondition, this);
	// 				m_qLoopData.push(lData);
	// 			} else {
	// 				UT_NOTIFY(LV_INFO,
	// 						"==> known loop at " << dr->loopIteration);
	// 				if (!m_qLoopData.isEmpty()) {
	// 					lData = m_qLoopData.top();
	// 					if (updateCovermap) {
	// 						lData->addLoopIteration(loopCondition,
	// 								dr->loopIteration);
	// 					}
	// 				} else {
	// 					/* TODO error handling */
	// 					QMessageBox::warning(this, "Warning",
	// 							"An error occurred while trying to "
	// 									"get loop count data.");
	// 					ShaderStep (DBG_BH_SELECTION_JUMP_OVER);
	// 					delete loopCondition;
	// 					return;
	// 				}
	// 			}
	// 			delete loopCondition;
	// 		}
	// 			break;
	// 		case RL_DBG_VERTEX_SHADER:
	// 		case RL_DBG_GEOMETRY_SHADER: {
	// 			VertexBox loopCondition;
	// 			if (!(getDebugVertexData(DBG_CG_LOOP_CONDITIONAL, NULL,
	// 					m_pCoverage, &loopCondition))) {
	// 				QMessageBox::warning(this, "Warning",
	// 						"An error occurred while trying to "
	// 								"get the loop count condition.");
	// 				cleanupDBGShader();
	// 				setRunLevel(RL_DBG_RESTART);
	// 				return;
	// 			}

	// 			/* Add data to the loop storage */
	// 			if (dr->loopIteration == 0) {
	// 				UT_NOTIFY(LV_INFO, "==> new loop encountered\n");
	// 				lData = new LoopData(&loopCondition, this);
	// 				m_qLoopData.push(lData);
	// 			} else {
	// 				UT_NOTIFY(LV_INFO,
	// 						"==> known loop at " << dr->loopIteration);
	// 				if (!m_qLoopData.isEmpty()) {
	// 					lData = m_qLoopData.top();
	// 					if (updateCovermap) {
	// 						lData->addLoopIteration(&loopCondition,
	// 								dr->loopIteration);
	// 					}
	// 				} else {
	// 					/* TODO error handling */
	// 					QMessageBox::warning(this, "Warning",
	// 							"An error occurred while trying to "
	// 									"get the loop data.");
	// 					ShaderStep (DBG_BH_SELECTION_JUMP_OVER);
	// 					return;
	// 				}
	// 			}
	// 		}
	// 			break;
	// 		}

	// 		if (updateGUI) {
	// 			LoopDialog *lDialog;
	// 			switch (_currentRunLevel) {
	// 			case RL_DBG_FRAGMENT_SHADER:
	// 				lDialog = new LoopDialog(lData, this);
	// 				break;
	// 			case RL_DBG_GEOMETRY_SHADER: {
	// 				/* Create list of all watch item boxes */
	// 				QList<ShVarItem*> watchItems;
	// 				if (m_pShVarModel) {
	// 					watchItems = m_pShVarModel->getAllWatchItemPointers();
	// 				}
	// 				lDialog = new LoopDialog(lData, watchItems, m_primitiveMode,
	// 						m_dShResources.geoOutputType, m_pGeometryMap,
	// 						m_pVertexCount, this);
	// 			}
	// 				break;
	// 			case RL_DBG_VERTEX_SHADER: {
	// 				/* Create list of all watch item boxes */
	// 				QList<ShVarItem*> watchItems;
	// 				if (m_pShVarModel) {
	// 					watchItems = m_pShVarModel->getAllWatchItemPointers();
	// 				}
	// 				lDialog = new LoopDialog(lData, watchItems, this);
	// 			}
	// 				break;
	// 			default:
	// 				lDialog = NULL;
	// 				QMessageBox::warning(this, "Warning", "The "
	// 						"current run level does not match the request.");
	// 			}
	// 			if (lDialog) {
	// 				connect(lDialog, SIGNAL(doShaderStep(int, bool, bool)),
	// 						this, SLOT(ShaderStep(int, bool, bool)));
	// 				switch (lDialog->exec()) {
	// 				case LoopDialog::SA_NEXT:
	// 					ShaderStep (DBG_BH_JUMPINTO);
	// 					break;
	// 				case LoopDialog::SA_BREAK:
	// 					ShaderStep (DBG_BH_SELECTION_JUMP_OVER);
	// 					break;
	// 				case LoopDialog::SA_JUMP:
	// 					/* Force update of all changed items */
	// 					updateWatchListData(COVERAGEMAP_GROWN, false);
	// 					ShaderStep(DBG_BH_JUMPINTO);
	// 					break;
	// 				}
	// 				disconnect(lDialog, 0, 0, 0);
	// 				delete lDialog;
	// 			} else {
	// 				ShaderStep (DBG_BH_SELECTION_JUMP_OVER);
	// 			}
	// 		}
	// 	}
	// 		break;
	// 	default:
	// 		break;
	// 	}

	// 	/* Update GUI shader text windows */
	// 	if (updateGUI) {
	// 		QTextDocument *document = NULL;
	// 		QTextEdit *edit = NULL;
	// 		switch (_currentRunLevel) {
	// 		case RL_DBG_VERTEX_SHADER:
	// 			document = _ui->teVertexShader->document();
	// 			edit = _ui->teVertexShader;
	// 			break;
	// 		case RL_DBG_GEOMETRY_SHADER:
	// 			document = _ui->teGeometryShader->document();
	// 			edit = _ui->teGeometryShader;
	// 			break;
	// 		case RL_DBG_FRAGMENT_SHADER:
	// 			document = _ui->teFragmentShader->document();
	// 			edit = _ui->teFragmentShader;
	// 			break;
	// 		default:
	// 			QMessageBox::warning(this, "Warning", "The "
	// 					"current run level does not allow for shader "
	// 					"stepping.");
	// 		}

	// 		/* Mark actual debug position */
	// 		if (document && edit) {
	// 			QTextCharFormat highlight;
	// 			QTextCursor cursor(document);

	// 			cursor.setPosition(0, QTextCursor::MoveAnchor);
	// 			cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor,
	// 					1);
	// 			highlight.setBackground(Qt::white);
	// 			cursor.mergeCharFormat(highlight);

	// 			/* Highlight the actual statement */
	// 			cursor.setPosition(0, QTextCursor::MoveAnchor);
	// 			cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor,
	// 					dr->range.left.line - 1);
	// 			cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor,
	// 					dr->range.left.colum - 1);
	// 			cursor.setPosition(0, QTextCursor::KeepAnchor);
	// 			cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor,
	// 					dr->range.right.line - 1);
	// 			cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor,
	// 					dr->range.right.colum);
	// 			highlight.setBackground(Qt::yellow);
	// 			cursor.mergeCharFormat(highlight);

	// 			/* Ensure the highlighted line is visible */
	// 			QTextCursor cursorVisible = edit->textCursor();
	// 			cursorVisible.setPosition(0, QTextCursor::MoveAnchor);
	// 			cursorVisible.movePosition(QTextCursor::Down,
	// 					QTextCursor::MoveAnchor, MAX(dr->range.left.line-3, 0));
	// 			edit->setTextCursor(cursorVisible);
	// 			edit->ensureCursorVisible();
	// 			cursorVisible.setPosition(0, QTextCursor::KeepAnchor);
	// 			cursorVisible.movePosition(QTextCursor::Down,
	// 					QTextCursor::KeepAnchor, dr->range.right.line + 1);
	// 			edit->setTextCursor(cursorVisible);
	// 			edit->ensureCursorVisible();

	// 			/* Unselect visible cursor */
	// 			QTextCursor cursorSet = edit->textCursor();
	// 			cursorSet.setPosition(0, QTextCursor::MoveAnchor);
	// 			cursorSet.movePosition(QTextCursor::Down,
	// 					QTextCursor::MoveAnchor, dr->range.left.line - 1);
	// 			edit->setTextCursor(cursorSet);
	// 			qApp->processEvents();
	// 		}
	// 	}
	// } else {
	// 	/* TODO: error */
	// }

	// /* TODO free debug result */
}

ErrorCode MainWindow::recordCall()
{
	// if (_currentCall && _currentCall->isFrameEnd()) {
	// 	_traceStatsWidget->resetPerFrameStatistics();
	// }
	// setGlTraceItemIconType(GlTraceListItem::IT_RECORD);

	// ErrorCode error = _dbg->recordCall();
	// /* TODO: error handling!!!!!! */

	// error = _dbg->callDone();

	// /* Error handling */
	// setErrorStatus(error);
	// if (isErrorCritical(error)) {
	// 	killDebuggee();
	// 	setRunLevel(RL_SETUP);
	// } else {
	// 	error = getNextCall();
	// 	setErrorStatus(error);
	// 	if (isErrorCritical(error)) {
	// 		killDebuggee();
	// 		setRunLevel(RL_SETUP);
	// 	} else {
	// 		addGlTraceItem();
	// 	}
	// }
	// return error;
}

void MainWindow::recordDrawCall()
{
	// _dbg->initRecording();
	// if (_currentCall->name() == "glBegin") {
	// 	while (_currentRunLevel == RL_DBG_RECORD_DRAWCALL
	// 			&& (!_currentCall
	// 					|| (_currentCall
	// 							&& _currentCall->name() != "glEnd"))) {
	// 		if (recordCall() != EC_NONE) {
	// 			return;
	// 		}
	// 		qApp->processEvents(QEventLoop::AllEvents);
	// 	}
	// 	if (!_currentCall || _currentCall->name() != "glEnd"
	// 			|| recordCall() != EC_NONE) {
	// 		if (_currentRunLevel == RL_DBG_RECORD_DRAWCALL) {
	// 			/* TODO: error handling */
	// 			UT_NOTIFY(LV_WARN, "recordDrawCall: begin without end????");
	// 			return;
	// 		} else {
	// 			/* draw call recording stopped by user interaction */
	// 			ErrorCode error = _dbg->insertGlEnd();
	// 			if (error == EC_NONE) {
	// 				switch (_ui->twShader->currentIndex()) {
	// 				case 0:
	// 					error = _dbg->restoreRenderTarget(
	// 							DBG_TARGET_VERTEX_SHADER);
	// 					break;
	// 				case 1:
	// 					error = _dbg->restoreRenderTarget(
	// 							DBG_TARGET_GEOMETRY_SHADER);
	// 					break;
	// 				case 2:
	// 					error = _dbg->restoreRenderTarget(
	// 							DBG_TARGET_FRAGMENT_SHADER);
	// 					break;
	// 				}
	// 			}
	// 			if (error == EC_NONE) {
	// 				error = _dbg->restoreActiveShader();
	// 			}
	// 			if (error == EC_NONE) {
	// 				error = _dbg->endReplay();
	// 			}
	// 			setErrorStatus(error);
	// 			if (isErrorCritical(error)) {
	// 				killDebuggee();
	// 				setRunLevel(RL_SETUP);
	// 				return;
	// 			}
	// 			setGlTraceItemIconType(GlTraceListItem::IT_ACTUAL);
	// 			return;
	// 		}
	// 	}
	// 	setGlTraceItemIconType(GlTraceListItem::IT_ACTUAL);
	// } else {
	// 	if (recordCall() != EC_NONE) {
	// 		return;
	// 	}
	// }
}

void MainWindow::on_tbShaderExecute_clicked()
{
	// int type = _ui->twShader->currentIndex();
	// QString sourceCode;
	// char *shaderCode = nullptr;
	// int debugOptions = EDebugOpIntermediate;
	// EShLanguage language = EShLangFragment;
	// ErrorCode error = EC_NONE;

	// if (_dbg->runLevel() == RL_DBG_VERTEX_SHADER
	// 		|| _dbg->runLevel() == RL_DBG_GEOMETRY_SHADER
	// 		|| _dbg->runLevel() == RL_DBG_FRAGMENT_SHADER) {

	// 	/* clean up debug run */
	// 	cleanupDBGShader();

	// 	setRunLevel(RL_DBG_RESTART);
	// 	return;
	// }

	// if (_dbg->runLevel() == RL_TRACE_EXECUTE_IS_DEBUGABLE) {
	// 	error = _dbg->saveAndInterruptQueries();
	// 	setErrorStatus(error);
	// 	if (isErrorCritical(error)) {
	// 		killDebuggee();
	// 		setRunLevel(RL_SETUP);
	// 		return;
	// 	}
	// }

	// /* setup debug render target */
	// switch (type) {
	// case 0:
	// 	error = _dbg->setDbgTarget(DBG_TARGET_VERTEX_SHADER, DBG_PFT_KEEP,
	// 			DBG_PFT_KEEP, DBG_PFT_KEEP, DBG_PFT_KEEP);
	// 	break;
	// case 1:
	// 	error = _dbg->setDbgTarget(DBG_TARGET_GEOMETRY_SHADER, DBG_PFT_KEEP,
	// 			DBG_PFT_KEEP, DBG_PFT_KEEP, DBG_PFT_KEEP);
	// 	break;
	// case 2:
	// 	error = _dbg->setDbgTarget(DBG_TARGET_FRAGMENT_SHADER,
	// 			_fragmentTestDialog->alphaTestOption(), _fragmentTestDialog->depthTestOption(),
	// 			_fragmentTestDialog->stencilTestOption(),
	// 			_fragmentTestDialog->blendingOption());
	// 	break;
	// }
	// setErrorStatus(error);
	// if (error != EC_NONE) {
	// 	if (isErrorCritical(error)) {
	// 		killDebuggee();
	// 		setRunLevel(RL_SETUP);
	// 		return;
	// 	} else {
	// 		setRunLevel(RL_DBG_RESTART);
	// 		return;
	// 	}
	// }

	// /* record stream, store currently active shader program,
	//  * and enter debug state only when shader is executed the first time
	//  * and not after restart.
	//  */
	// if (_currentRunLevel == RL_TRACE_EXECUTE_IS_DEBUGABLE) {
	// 	setRunLevel(RL_DBG_RECORD_DRAWCALL);
	// 	/* save active shader */
	// 	error = _dbg->saveActiveShader();
	// 	setErrorStatus(error);
	// 	if (isErrorCritical(error)) {
	// 		killDebuggee();
	// 		setRunLevel(RL_SETUP);
	// 		return;
	// 	}
	// 	recordDrawCall();
	// 	/* has the user interrupted the recording? */
	// 	if (_currentRunLevel != RL_DBG_RECORD_DRAWCALL) {
	// 		return;
	// 	}
	// }

	// switch (type) {
	// case 0: /* Vertex shaders */
	// 	if (m_pShaders[0]) {
	// 		setRunLevel(RL_DBG_VERTEX_SHADER);
	// 		language = EShLangVertex;
	// 	} else {
	// 		return;
	// 	}
	// 	break;
	// case 1: /* Geometry shaders */
	// 	if (m_pShaders[1]) {
	// 		language = EShLangGeometry;
	// 		setRunLevel(RL_DBG_GEOMETRY_SHADER);
	// 	} else {
	// 		return;
	// 	}
	// 	break;
	// case 2: /* Fragment shaders */
	// 	if (m_pShaders[2]) {
	// 		language = EShLangFragment;
	// 		setRunLevel(RL_DBG_FRAGMENT_SHADER);
	// 	} else {
	// 		return;
	// 	}
	// 	break;
	// default:
	// 	return;
	// }

	// /* start building the parse tree for this shader */
	// m_dShCompiler = ShConstructCompiler(language, debugOptions);
	// if (m_dShCompiler == 0) {
	// 	setErrorStatus(EC_UNKNOWN_ERROR);
	// 	killDebuggee();
	// 	setRunLevel(RL_SETUP);
	// 	return;
	// }

	// shaderCode = m_pShaders[type];

	// if (!ShCompile(m_dShCompiler, &shaderCode, 1, EShOptNone, &m_dShResources,
	// 		debugOptions, &m_dShVariableList)) {
	// 	const char *err = ShGetInfoLog(m_dShCompiler);
	// 	Dialog_CompilerError dlgCompilerError(this);
	// 	dlgCompilerError.labelMessage->setText(
	// 			"Your shader seems not to be compliant to the official GLSL1.2 
	// 			specification and may rely on vendor specific enhancements.
	// 			<br>If this is not the case, please report this probem to 
	// 			<A HREF=\"mailto:glsldevil@vis.uni-stuttgart.de\">
	// 			glsldevil@vis.uni-stuttgart.de</A>.");
	// 	dlgCompilerError.setDetailedOutput(err);
	// 	dlgCompilerError.exec();
	// 	cleanupDBGShader();
	// 	setRunLevel(RL_DBG_RESTART);
	// 	return;
	// }

	// m_pShVarModel = new ShVarModel(&m_dShVariableList, this, qApp);
	// connect(m_pShVarModel, SIGNAL(newWatchItem(ShVarItem*)), this,
	// 		SLOT(updateWatchItemData(ShVarItem*)));
	// QItemSelectionModel *selectionModel = new QItemSelectionModel(
	// 		m_pShVarModel->getFilterModel(ShVarModel::TV_WATCH_LIST));

	// m_pShVarModel->attach(_ui->tvShVarAll, ShVarModel::TV_ALL);
	// m_pShVarModel->attach(_ui->tvShVarBuiltIn, ShVarModel::TV_BUILTIN);
	// m_pShVarModel->attach(_ui->tvShVarScope, ShVarModel::TV_SCOPE);
	// m_pShVarModel->attach(_ui->tvShVarUniform, ShVarModel::TV_UNIFORM);
	// m_pShVarModel->attach(_ui->tvWatchList, ShVarModel::TV_WATCH_LIST);

	// _ui->tvWatchList->setSelectionModel(selectionModel);

	// /* Watchview feedback for selection tracking */
	// connect(_ui->tvWatchList->selectionModel(),
	// 		SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
	// 		this,
	// 		SLOT(watchSelectionChanged(const QItemSelection &, const QItemSelection &)));

	// /* set uniform values */
	// m_pShVarModel->setUniformValues(m_serializedUniforms.pData,
	// 		m_serializedUniforms.count);

	// ShaderStep (DBG_BH_JUMPINTO);

	// if (_dbg->runLevel() != RL_DBG_VERTEX_SHADER
	// 		&& _dbg->runLevel() != RL_DBG_GEOMETRY_SHADER
	// 		&& _dbg->runLevel() != RL_DBG_FRAGMENT_SHADER) {
	// 	return;
	// }

	// if (language == EShLangGeometry) {
	// 	delete m_pGeometryMap;
	// 	delete m_pVertexCount;
	// 	m_pGeometryMap = new VertexBox();
	// 	m_pVertexCount = new VertexBox();
	// 	UT_NOTIFY(LV_INFO, "Get GEOMETRY_MAP:");
	// 	if (getDebugVertexData(DBG_CG_GEOMETRY_MAP, nullptr, nullptr,
	// 			m_pGeometryMap)) {
	// 		/* TODO: build geometry model */
	// 	} else {
	// 		cleanupDBGShader();
	// 		setRunLevel(RL_DBG_RESTART);
	// 		return;
	// 	}
	// 	UT_NOTIFY(LV_INFO, "Get VERTEX_COUNT:");
	// 	if (getDebugVertexData(DBG_CG_VERTEX_COUNT, nullptr, nullptr,
	// 			m_pVertexCount)) {
	// 		/* TODO: build geometry model */
	// 	} else {
	// 		cleanupDBGShader();
	// 		setRunLevel(RL_DBG_RESTART);
	// 		return;
	// 	}

	// 	//m_pGeoDataModel = new GeoShaderDataModel(m_primitiveMode,
	// 	//			m_dShResources.geoOutputType, m_pGeometryMap, m_pVertexCount);

	// }

}

void MainWindow::on_tbShaderReset_clicked()
{
	shaderStep(DBG_BH_RESET, true);
	shaderStep(DBG_BH_JUMPINTO);

	_ui->tbShaderStep->setEnabled(true);
	_ui->tbShaderStepOver->setEnabled(true);

	resetWatchListData();
}

void MainWindow::on_tbShaderStep_clicked()
{
	shaderStep(DBG_BH_JUMPINTO);
}

void MainWindow::on_tbShaderStepOver_clicked()
{
	shaderStep(DBG_BH_FOLLOW_ELSE);
}

void MainWindow::on_tbShaderFragmentOptions_clicked()
{
	_fragmentTestDialog->show();
}

void MainWindow::on_twShader_currentChanged(int selection)
{
	// if (_dbg->runLevel() == Debugger::DBG_VERTEX_SHADER
	// 		|| _dbg->runLevel() == Debugger::DBG_GEOMETRY_SHADER
	// 		|| _dbg->runLevel() == Debugger::DBG_FRAGMENT_SHADER) {
	// 	if ((_dbg->runLevel() == Debugger::DBG_VERTEX_SHADER && selection == 0)
	// 			|| (_dbg->runLevel() == Debugger::DBG_GEOMETRY_SHADER && selection == 1)
	// 			|| (_dbg->runLevel() == Debugger::DBG_FRAGMENT_SHADER && selection == 2)) {
	// 		_ui->tbShaderExecute->setEnabled(true);
	// 		_ui->tbShaderReset->setEnabled(true);
	// 		_ui->tbShaderStepOver->setEnabled(true);
	// 		_ui->tbShaderStep->setEnabled(true);
	// 	} else {
	// 		_ui->tbShaderExecute->setEnabled(false);
	// 		_ui->tbShaderReset->setEnabled(false);
	// 		_ui->tbShaderStepOver->setEnabled(false);
	// 		_ui->tbShaderStep->setEnabled(false);
	// 		_ui->tbShaderFragmentOptions->setEnabled(false);
	// 	}
	// } else if ((_dbg->runLevel() == Debugger::DBG_RESTART
	// 		|| _dbg->runLevel() == Debugger::TRACE_EXECUTE_IS_DEBUGABLE)
	// 		&& m_bHaveValidShaderCode) {
	// 	switch (_ui->twShader->currentIndex()) {
	// 	case 0:
	// 		if (m_pShaders[0] && m_dShResources.transformFeedbackSupported) {
	// 			_ui->tbShaderExecute->setEnabled(true);
	// 		} else {
	// 			_ui->tbShaderExecute->setEnabled(false);
	// 		}
	// 		_ui->tbShaderFragmentOptions->setEnabled(false);
	// 		break;
	// 	case 1:
	// 		if (m_pShaders[1] && m_dShResources.geoShaderSupported
	// 				&& m_dShResources.transformFeedbackSupported) {
	// 			_ui->tbShaderExecute->setEnabled(true);
	// 		} else {
	// 			_ui->tbShaderExecute->setEnabled(false);
	// 		}
	// 		_ui->tbShaderFragmentOptions->setEnabled(false);
	// 		break;
	// 	case 2:
	// 		if (m_pShaders[2] && m_dShResources.framebufferObjectsSupported) {
	// 			_ui->tbShaderExecute->setEnabled(true);
	// 			_ui->tbShaderFragmentOptions->setEnabled(true);
	// 		} else {
	// 			_ui->tbShaderExecute->setEnabled(false);
	// 		}
	// 		break;
	// 	}
	// } else {
	// 	_ui->tbShaderExecute->setEnabled(false);
	// }
}

QModelIndexList MainWindow::cleanupSelectionList(QModelIndexList input)
{
	QModelIndexList output;         // Resulting filtered list.
	QStack<QModelIndex> stack;      // For iterative tree traversal.

	if (!m_pShVarModel) {
		return output;
	}

	/*
	 * Add directly selected items in reverse order such that getting them
	 * from the stack restores the original order. This is also required
	 * for all following push operations.
	 */
	for (int i = input.count() - 1; i >= 0; i--) {
		stack.push(input[i]);
	}

	while (!stack.isEmpty()) {
		QModelIndex idx = stack.pop();
		ShVarItem *item = this->m_pShVarModel->getWatchItemPointer(idx);

		// Item might be NULL because 'idx' can become invalid during recursion
		// which causes the parent removal to crash.
		if (item != nullptr) {
			for (int c = item->childCount() - 1; c >= 0; c--) {
				stack.push(idx.child(c, idx.column()));
			}

			if ((item->childCount() == 0) && item->isSelectable()
					&& !output.contains(idx)) {
				output << idx;
			}
		}
	}

	return output;
}

WatchView* MainWindow::newWatchWindowFragment(QModelIndexList &list)
{
	WatchVector *window = nullptr;
	for (auto &i : list) {
		ShVarItem *item = m_pShVarModel->getWatchItemPointer(i);
		if (item) {
			if (!window) {
				/* Create window */
				window = new WatchVector(_workspace);
				connect(window, SIGNAL(destroyed()), this,
						SLOT(watchWindowClosed()));
				connect(window,
						SIGNAL(mouseOverValuesChanged(int, int, const bool *, const QVariant *)),
						this,
						SLOT(setMouseOverValues(int, int, const bool *, const QVariant *)));
				connect(window, SIGNAL(selectionChanged(int, int)), this,
						SLOT(newSelectedPixel(int, int)));
				connect(_ui->aZoom, SIGNAL(triggered()), window,
						SLOT(setZoomMode()));
				connect(_ui->aSelectPixel, SIGNAL(triggered()), window,
						SLOT(setPickMode()));
				connect(_ui->aMinMaxLens, SIGNAL(triggered()), window,
						SLOT(setMinMaxMode()));
				/* initialize mouse mode */
				_agWatchControl->setEnabled(true);
				if (_agWatchControl->checkedAction() == _ui->aZoom) {
					window->setZoomMode();
				} else if (_agWatchControl->checkedAction() == _ui->aSelectPixel) {
					window->setPickMode();
				} else if (_agWatchControl->checkedAction() == _ui->aMinMaxLens) {
					window->setMinMaxMode();
				}
				window->setWorkspace(_workspace);
				_workspace->addWindow(window);
			}
			window->attachFpData(item->getPixelBoxPointer(),
					item->getFullName());
		}
	}
	return window;
}

WatchView* MainWindow::newWatchWindowVertexTable(QModelIndexList &list)
{
	WatchTable *window = nullptr;
	for (auto &i : list) {
		ShVarItem *item = m_pShVarModel->getWatchItemPointer(i);
		if (item) {
			if (!window) {
				/* Create window */
				window = new WatchTable(_workspace);
				connect(window, SIGNAL(selectionChanged(int)), this,
						SLOT(newSelectedVertex(int)));
				_workspace->addWindow(window);
			}
			window->attachVpData(item->getVertexBoxPointer(),
					item->getFullName());
		}
	}
	return window;
}

WatchView* MainWindow::newWatchWindowGeoDataTree(QModelIndexList &list)
{
	WatchGeoDataTree *window = nullptr;
	for (auto &i : list) {
		ShVarItem *item = m_pShVarModel->getWatchItemPointer(i);
		if (item) {
			if (!window) {
				/* Create window */
				window = new WatchGeoDataTree(m_primitiveMode,
						m_dShResources.geoOutputType, m_pGeometryMap,
						m_pVertexCount, _workspace);
				connect(window, SIGNAL(selectionChanged(int)), this,
						SLOT(newSelectedPrimitive(int)));
				_workspace->addWindow(window);
			}
			window->attachData(item->getCurrentPointer(),
					item->getVertexBoxPointer(), item->getFullName());
		}
	}
	return window;
}

void MainWindow::on_tbWatchWindow_clicked()
{
	// QModelIndexList list = cleanupSelectionList(
	// 		_ui->tvWatchList->selectionModel()->selectedRows(0));

	// if (m_pShVarModel && !list.isEmpty()) {
	// 	WatchView *window = nullptr;
	// 	switch (_dbg->runLevel()) {
	// 	case RL_DBG_GEOMETRY_SHADER:
	// 		window = newWatchWindowGeoDataTree(list);
	// 		break;
	// 	case RL_DBG_VERTEX_SHADER:
	// 		window = newWatchWindowVertexTable(list);
	// 		break;
	// 	case RL_DBG_FRAGMENT_SHADER:
	// 		window = newWatchWindowFragment(list);
	// 		break;
	// 	default:
	// 		UT_NOTIFY(LV_WARN, "invalid runlevel");
	// 	}
	// 	if (window) {
	// 		window->updateView(true);
	// 		window->show();
	// 	} else {
	// 		// TODO: Should this be an error?
	// 		QMessageBox::warning(this, "Warning", "The "
	// 				"watch window could not be created.");
	// 	}
	// }
}

void MainWindow::addToWatchWindowVertexTable(WatchView *watchView,
		QModelIndexList &list)
{
	WatchTable *window = static_cast<WatchTable*>(watchView);
	for (auto &i : list) {
		ShVarItem *item = m_pShVarModel->getWatchItemPointer(i);
		if (item) {
			window->attachVpData(item->getVertexBoxPointer(),
					item->getFullName());
		}
	}
}

void MainWindow::addToWatchWindowFragment(WatchView *watchView,
		QModelIndexList &list)
{
	WatchVector *window = static_cast<WatchVector*>(watchView);
	for (auto &i : list) {
		ShVarItem *item = m_pShVarModel->getWatchItemPointer(i);
		if (item) {
			window->attachFpData(item->getPixelBoxPointer(),
					item->getFullName());
		}
	}
}

void MainWindow::addToWatchWindowGeoDataTree(WatchView *watchView,
		QModelIndexList &list)
{
	WatchGeoDataTree *w = static_cast<WatchGeoDataTree*>(watchView);
	for (auto &i : list) {
		ShVarItem *item = m_pShVarModel->getWatchItemPointer(i);
		if (item) {
			w->attachData(item->getCurrentPointer(),
					item->getVertexBoxPointer(), item->getFullName());
		}
	}
}

void MainWindow::on_tbWatchWindowAdd_clicked()
{
	/* TODO: do not use static cast! instead watchView should provide functionality */
	// WatchView *window = static_cast<WatchView*>(_workspace->activeWindow());
	// QModelIndexList list = cleanupSelectionList(
	// 		_ui->tvWatchList->selectionModel()->selectedRows(0));

	// if (window && m_pShVarModel && !list.isEmpty()) {
	// 	switch (_dbg->runLevel()) {
	// 	case RL_DBG_GEOMETRY_SHADER:
	// 		addToWatchWindowGeoDataTree(window, list);
	// 		break;
	// 	case RL_DBG_VERTEX_SHADER:
	// 		addToWatchWindowVertexTable(window, list);
	// 		break;
	// 	case RL_DBG_FRAGMENT_SHADER:
	// 		addToWatchWindowFragment(window, list);
	// 		break;
	// 	default:
	// 		QMessageBox::critical(this, "Internal Error",
	// 				"on_tbWatchWindowAdd_clicked was called on an invalid "
	// 						"run level.<br>Please report this probem to "
	// 						"<A HREF=\"mailto:glsldevil@vis.uni-stuttgart.de\">"
	// 						"glsldevil@vis.uni-stuttgart.de</A>.",
	// 				QMessageBox::Ok);
	// 	}
	// 	window->updateView(true);
	// }
}

void MainWindow::watchWindowClosed()
{
	if (_workspace->windowList().count() == 0) {
		_agWatchControl->setEnabled(false);
	}
}

void MainWindow::updateWatchGui(int s)
{
	if (s == 0) {
		_ui->tbWatchWindow->setEnabled(false);
		_ui->tbWatchWindowAdd->setEnabled(false);
		_ui->tbWatchDelete->setEnabled(false);
	} else if (s == 1) {
		_ui->tbWatchWindow->setEnabled(true);
		if (_workspace->activeWindow()) {
			_ui->tbWatchWindowAdd->setEnabled(true);
		} else {
			_ui->tbWatchWindowAdd->setEnabled(false);
		}
		_ui->tbWatchDelete->setEnabled(true);
	} else {
		_ui->tbWatchWindow->setEnabled(true);
		if (_workspace->activeWindow()) {
			_ui->tbWatchWindowAdd->setEnabled(true);
		} else {
			_ui->tbWatchWindowAdd->setEnabled(false);
		}
		_ui->tbWatchDelete->setEnabled(true);
	}
}

void MainWindow::on_tbWatchDelete_clicked()
{
	if (!_ui->tvWatchList)
		return;
	if (!m_pShVarModel)
		return;

	QModelIndexList list;
	do {
		list = _ui->tvWatchList->selectionModel()->selectedIndexes();
		if (list.count() != 0) {
			m_pShVarModel->unsetItemWatched(list[0]);
		}
	} while (list.count() != 0);

	updateWatchGui(
			cleanupSelectionList(_ui->tvWatchList->selectionModel()->selectedRows()).count());

	/* Now update all windows to update themselves if necessary */
	for (auto &i :  _workspace->windowList()) {
		static_cast<WatchView*>(i)->updateView(false);
	}
}

void MainWindow::watchSelectionChanged(const QItemSelection&,
		const QItemSelection&)
{
	updateWatchGui(
			cleanupSelectionList(_ui->tvWatchList->selectionModel()->selectedRows()).count());
}

void MainWindow::changedActiveWindow(QWidget *w)
{
	for (auto &i :  _workspace->windowList()) {
		if (w == i) {
			static_cast<WatchView*>(i)->setActive(true);
		} else {
			static_cast<WatchView*>(i)->setActive(false);
		}
	}
}

void MainWindow::leaveDBGState()
{
	/* FIXME */
	// ErrorCode error = EC_NONE;

	// switch (_currentRunLevel) {
	// case RL_DBG_RESTART:
	// case RL_DBG_RECORD_DRAWCALL:
	// 	/* restore shader program */
	// 	error = _dbg->restoreActiveShader();
	// 	setErrorStatus(error);
	// 	if (isErrorCritical(error)) {
	// 		killDebuggee();
	// 		setRunLevel(RL_SETUP);
	// 		return;
	// 	}
	// 	error = _dbg->restartQueries();
	// 	setErrorStatus(error);
	// 	if (isErrorCritical(error)) {
	// 		killDebuggee();
	// 		setRunLevel(RL_SETUP);
	// 		return;
	// 	}
	// 	error = _dbg->endReplay();
	// 	setErrorStatus(error);
	// 	if (isErrorCritical(error)) {
	// 		killDebuggee();
	// 		setRunLevel(RL_SETUP);
	// 		return;
	// 	}
	// 	/* TODO: close all windows (obsolete?) */
	// 	delete[] m_pCoverage;
	// 	m_pCoverage = NULL;
	// 	break;
	// default:
	// 	break;
	// }
}

void MainWindow::cleanupDBGShader()
{
	// QList<ShVarItem*> watchItems;
	// int i;

	// ErrorCode error = EC_NONE;

	// /* remove debug markers from code display */
	// QTextDocument *document = nullptr;
	// QTextEdit *edit = nullptr;
	// enum DBG_TARGET target;
	// switch (_dbg->runLevel()) {
	// 	case Debugger::DBG_VERTEX_SHADER:
	// 		document = _ui->teVertexShader->document();
	// 		edit = _ui->teVertexShader;
	// 		target = DBG_TARGET_VERTEX_SHADER;
	// 		break;
	// 	case Debugger::DBG_GEOMETRY_SHADER:
	// 		document = _ui->teGeometryShader->document();
	// 		edit = _ui->teGeometryShader;
	// 		target = DBG_TARGET_GEOMETRY_SHADER;
	// 		break;
	// 	case Debugger::DBG_FRAGMENT_SHADER:
	// 		document = _ui->teFragmentShader->document();
	// 		edit = _ui->teFragmentShader;
	// 		target = DBG_TARGET_FRAGMENT_SHADER;
	// 		break;
	// 	default:
	// 		UT_NOTIFY(LV_ERROR, "we shouldn't be here");
	// 		exit(1);
	// }
	// if (document && edit) {
	// 	QTextCharFormat highlight;
	// 	QTextCursor cursor(document);

	// 	cursor.setPosition(0, QTextCursor::MoveAnchor);
	// 	cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor, 1);
	// 	highlight.setBackground(Qt::white);
	// 	cursor.mergeCharFormat(highlight);
	// }

	// _ui->lWatchSelectionPos->setText("No Selection");
	// _selectedPixel.rx() = -1;
	// _selectedPixel.ry() = -1;

	// while (!m_qLoopData.isEmpty()) {
	// 	delete m_qLoopData.pop();
	// }

	// if (m_pShVarModel) {
	// 	/* free data boxes */
	// 	watchItems = m_pShVarModel->getAllWatchItemPointers();
	// 	for (i = 0; i < watchItems.count(); i++) {
	// 		PixelBox *fb;
	// 		VertexBox *vb;
	// 		if ((fb = watchItems[i]->getPixelBoxPointer())) {
	// 			watchItems[i]->setPixelBoxPointer(NULL);
	// 			delete fb;
	// 		}
	// 		if ((vb = watchItems[i]->getCurrentPointer())) {
	// 			watchItems[i]->setCurrentPointer(NULL);
	// 			delete vb;
	// 		}
	// 		if ((vb = watchItems[i]->getVertexBoxPointer())) {
	// 			watchItems[i]->setVertexBoxPointer(NULL);
	// 			delete vb;
	// 		}
	// 	}

	// 	/* Watchview feedback for selection tracking */
	// 	disconnect(_ui->tvWatchList->selectionModel(),
	// 			SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
	// 			this,
	// 			SLOT(watchSelectionChanged(const QItemSelection &, const QItemSelection &)));

	// 	/* reset shader debuging */
	// 	m_pShVarModel->detach(_ui->tvShVarAll);
	// 	m_pShVarModel->detach(_ui->tvShVarBuiltIn);
	// 	m_pShVarModel->detach(_ui->tvShVarScope);
	// 	m_pShVarModel->detach(_ui->tvShVarUniform);
	// 	m_pShVarModel->detach(_ui->tvWatchList);

	// 	disconnect(m_pShVarModel, SIGNAL(newWatchItem(ShVarItem*)), this, 0);
	// 	delete m_pShVarModel;
	// 	m_pShVarModel = NULL;
	// }

	// if (m_dShCompiler) {
	// 	ShDestruct(m_dShCompiler);
	// 	m_dShCompiler = 0;
	// 	freeShVariableList(&m_dShVariableList);
	// }

	// UT_NOTIFY(LV_INFO, "restore render target");
	// /* restore render target */
	// error = _dbg->restoreRenderTarget(target);
}

void MainWindow::updateGuiToRunLevel(RunLevel rl)
{
	QString title = QString(MAIN_WINDOW_TITLE);

	switch (rl) {
	case RL_INIT:  // Program start
		setWindowTitle(title);
		_ui->aOpen->setEnabled(true);
#ifdef GLSLDB_WIN32
		//_ui->aAttach->setEnabled(true);
		// TODO
#endif /* GLSLDB_WIN32 */
		_ui->tbBVCaptureAutomatic->setEnabled(false);
		_ui->tbBVCaptureAutomatic->setChecked(false);
		_ui->tbBVCapture->setEnabled(false);
		_ui->tbBVSave->setEnabled(false);
		_lBVLabel->setPixmap(QPixmap());
		_ui->tbExecute->setEnabled(false);
		_ui->tbExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/gltrace-execute_32.png")));
		_ui->tbStep->setEnabled(false);
		_ui->tbSkip->setEnabled(false);
		_ui->tbEdit->setEnabled(false);
		_ui->tbJumpToDrawCall->setEnabled(false);
		_ui->tbJumpToShader->setEnabled(false);
		_ui->tbJumpToUserDef->setEnabled(false);
		_ui->tbRun->setEnabled(false);
		_ui->tbPause->setEnabled(false);
		_ui->tbToggleGuiUpdate->setEnabled(false);
		_ui->tbToggleNoTrace->setEnabled(false);
		_ui->tbToggleHaltOnError->setEnabled(false);
		_ui->tbGlTraceSettings->setEnabled(false);
		_ui->tbSave->setEnabled(false);
		_ui->tbShaderExecute->setEnabled(false);
		_ui->tbShaderExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/shader-execute_32.png")));
		_ui->tbShaderReset->setEnabled(false);
		_ui->tbShaderStep->setEnabled(false);
		_ui->tbShaderStepOver->setEnabled(false);
		_ui->tbShaderFragmentOptions->setEnabled(false);
		_ui->twShader->setTabIcon(0, QIcon());
		_ui->twShader->setTabIcon(1, QIcon());
		_ui->twShader->setTabIcon(2, QIcon());
		updateWatchGui(0);
		setShaderCodeText(NULL);
		m_bHaveValidShaderCode = false;
		setGuiUpdates(true);
		break;
	case RL_SETUP:  // User has setup parameters for debugging
		title.append(" - ");
		/* FIXME
		title.append(_debugConfig.programArgs[0]);
		*/
		setWindowTitle(title);
		_ui->aOpen->setEnabled(true);
#ifdef _WIN32
		//_ui->aAttach->setEnabled(true);
		// TODO
#endif /* _WIN32 */
		_ui->tbBVCapture->setEnabled(false);
		_ui->tbBVCaptureAutomatic->setEnabled(false);
		_ui->tbBVCaptureAutomatic->setChecked(false);
		_ui->tbBVSave->setEnabled(false);
		_lBVLabel->setPixmap(QPixmap());
		_ui->tbExecute->setEnabled(true);
		_ui->tbExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/gltrace-execute_32.png")));
		_ui->tbStep->setEnabled(false);
		_ui->tbSkip->setEnabled(false);
		_ui->tbEdit->setEnabled(false);
		_ui->tbJumpToDrawCall->setEnabled(false);
		_ui->tbJumpToShader->setEnabled(false);
		_ui->tbJumpToUserDef->setEnabled(false);
		_ui->tbRun->setEnabled(false);
		_ui->tbPause->setEnabled(false);
		_ui->tbToggleGuiUpdate->setEnabled(false);
		_ui->tbToggleNoTrace->setEnabled(false);
		_ui->tbShaderExecute->setEnabled(false);
		_ui->tbToggleHaltOnError->setEnabled(false);
		_ui->tbGlTraceSettings->setEnabled(true);
		_ui->tbSave->setEnabled(true);
		_ui->tbShaderExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/shader-execute_32.png")));
		_ui->tbShaderReset->setEnabled(false);
		_ui->tbShaderStep->setEnabled(false);
		_ui->tbShaderStepOver->setEnabled(false);
		_ui->tbShaderFragmentOptions->setEnabled(false);
		_ui->twShader->setTabIcon(0, QIcon());
		_ui->twShader->setTabIcon(1, QIcon());
		_ui->twShader->setTabIcon(2, QIcon());
		updateWatchGui(0);
		setShaderCodeText(NULL);
		m_bHaveValidShaderCode = false;
		setGuiUpdates(true);
		break;
	case RL_TRACE_EXECUTE:  
		// Trace is running in step mode
		/* nothing to change */
		break;
	case RL_TRACE_EXECUTE_NO_DEBUGABLE:  // sub-level for non debugable calls
		title.append(" - ");
		/* FIXME
		title.append(_debugConfig.programArgs[0]);
		*/
		setWindowTitle(title);
		_ui->aOpen->setEnabled(true);
#ifdef _WIN32
		//_ui->aAttach->setEnabled(true);
		// TODO
#endif /* _WIN32 */
		if (!_ui->tbBVCaptureAutomatic->isChecked()) {
			_ui->tbBVCapture->setEnabled(true);
		}
		_ui->tbBVCaptureAutomatic->setEnabled(true);
		_ui->tbExecute->setEnabled(true);
		_ui->tbExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/face-devil-green-grin_32.png")));
		_ui->tbStep->setEnabled(true);
		_ui->tbSkip->setEnabled(true);
		if (_currentCall && _currentCall->isEditable()) {
			_ui->tbEdit->setEnabled(true);
		} else {
			_ui->tbEdit->setEnabled(false);
		}
		_ui->tbJumpToDrawCall->setEnabled(true);
		_ui->tbJumpToShader->setEnabled(true);
		_ui->tbJumpToUserDef->setEnabled(true);
		_ui->tbRun->setEnabled(true);
		_ui->tbPause->setEnabled(false);
		_ui->tbToggleGuiUpdate->setEnabled(true);
		_ui->tbToggleNoTrace->setEnabled(true);
		_ui->tbToggleHaltOnError->setEnabled(true);
		_ui->tbGlTraceSettings->setEnabled(true);
		_ui->tbSave->setEnabled(true);
		_ui->tbShaderExecute->setEnabled(false);
		_ui->tbShaderExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/shader-execute_32.png")));
		_ui->tbShaderReset->setEnabled(false);
		_ui->tbShaderStep->setEnabled(false);
		_ui->tbShaderStepOver->setEnabled(false);
		_ui->tbShaderFragmentOptions->setEnabled(false);
		_ui->twShader->setTabIcon(0, QIcon());
		_ui->twShader->setTabIcon(1, QIcon());
		_ui->twShader->setTabIcon(2, QIcon());
		updateWatchGui(0);
		setGuiUpdates(true);
		break;
	case RL_TRACE_EXECUTE_IS_DEBUGABLE:  // sub-level for debugable calls
		title.append(" - ");
		/* FIXME
		title.append(_debugConfig.programArgs[0]);
		*/
		setWindowTitle(title);
		_ui->aOpen->setEnabled(true);
#ifdef _WIN32
		//_ui->aAttach->setEnabled(true);
		// TODO
#endif /* _WIN32 */
		if (!_ui->tbBVCaptureAutomatic->isChecked()) {
			_ui->tbBVCapture->setEnabled(true);
		}
		_ui->tbBVCaptureAutomatic->setEnabled(true);
		_ui->tbExecute->setEnabled(true);
		_ui->tbExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/face-devil-green-grin_32.png")));
		_ui->tbStep->setEnabled(true);
		_ui->tbSkip->setEnabled(true);
		/* FIXME
		if (_currentCall && _currentCall->isEditable()) {
			_ui->tbEdit->setEnabled(true);
		} else {
			_ui->tbEdit->setEnabled(false);
		}
		*/
		_ui->tbJumpToDrawCall->setEnabled(true);
		_ui->tbJumpToShader->setEnabled(true);
		_ui->tbJumpToUserDef->setEnabled(true);
		_ui->tbRun->setEnabled(true);
		_ui->tbPause->setEnabled(false);
		_ui->tbToggleGuiUpdate->setEnabled(true);
		_ui->tbToggleNoTrace->setEnabled(true);
		_ui->tbToggleHaltOnError->setEnabled(true);
		_ui->tbGlTraceSettings->setEnabled(true);
		_ui->tbSave->setEnabled(true);
		if (m_bHaveValidShaderCode) {
			switch (_ui->twShader->currentIndex()) {
			case 0:
				if (m_pShaders[0]
						&& m_dShResources.transformFeedbackSupported) {
					_ui->tbShaderExecute->setEnabled(true);
				} else {
					_ui->tbShaderExecute->setEnabled(false);
				}
				_ui->tbShaderFragmentOptions->setEnabled(false);
				break;
			case 1:
				if (m_pShaders[1] && m_dShResources.geoShaderSupported
						&& m_dShResources.transformFeedbackSupported) {
					_ui->tbShaderExecute->setEnabled(true);
				} else {
					_ui->tbShaderExecute->setEnabled(false);
				}
				_ui->tbShaderFragmentOptions->setEnabled(false);
				break;
			case 2:
				if (m_pShaders[2]
						&& m_dShResources.framebufferObjectsSupported) {
					_ui->tbShaderExecute->setEnabled(true);
					_ui->tbShaderFragmentOptions->setEnabled(true);
				} else {
					_ui->tbShaderExecute->setEnabled(false);
					_ui->tbShaderFragmentOptions->setEnabled(false);
				}
				break;
			}
		} else {
			_ui->tbShaderExecute->setEnabled(false);
			_ui->tbShaderFragmentOptions->setEnabled(false);
		}
		_ui->tbShaderExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/shader-execute_32.png")));
		_ui->tbShaderReset->setEnabled(false);
		_ui->tbShaderStep->setEnabled(false);
		_ui->tbShaderStepOver->setEnabled(false);
		_ui->twShader->setTabIcon(0, QIcon());
		_ui->twShader->setTabIcon(1, QIcon());
		_ui->twShader->setTabIcon(2, QIcon());
		updateWatchGui(0);
		setGuiUpdates(true);
		break;
	case RL_TRACE_EXECUTE_RUN:
		title.append(" - ");
		/* FIXME
		title.append(_debugConfig.programArgs[0]);
		*/
		setWindowTitle(title);
		_ui->aOpen->setEnabled(false);
		_ui->aAttach->setEnabled(false);
		if (!_ui->tbBVCaptureAutomatic->isChecked()) {
			_ui->tbBVCapture->setEnabled(true);
		}
		_ui->tbBVCaptureAutomatic->setEnabled(true);
		_ui->tbExecute->setEnabled(true);
		_ui->tbExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/face-devil-green-grin_32.png")));
		_ui->tbStep->setEnabled(false);
		_ui->tbSkip->setEnabled(false);
		_ui->tbEdit->setEnabled(false);
		_ui->tbJumpToDrawCall->setEnabled(false);
		_ui->tbJumpToShader->setEnabled(false);
		_ui->tbJumpToUserDef->setEnabled(false);
		_ui->tbRun->setEnabled(false);
		_ui->tbPause->setEnabled(true);
		if (_ui->tbToggleNoTrace->isChecked()) {
			_ui->tbToggleGuiUpdate->setEnabled(false);
		} else {
			_ui->tbToggleGuiUpdate->setEnabled(true);
		}
		_ui->tbToggleNoTrace->setEnabled(false);
		_ui->tbToggleHaltOnError->setEnabled(false);
		_ui->tbGlTraceSettings->setEnabled(false);
		_ui->tbSave->setEnabled(false);
		_ui->tbShaderExecute->setEnabled(false);
		_ui->tbShaderExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/shader-execute_32.png")));
		_ui->tbShaderReset->setEnabled(false);
		_ui->tbShaderStep->setEnabled(false);
		_ui->tbShaderStepOver->setEnabled(false);
		_ui->tbShaderFragmentOptions->setEnabled(false);
		_ui->twShader->setTabIcon(0, QIcon());
		_ui->twShader->setTabIcon(1, QIcon());
		_ui->twShader->setTabIcon(2, QIcon());
		updateWatchGui(0);
		/* update handling */
		if (_ui->tbToggleGuiUpdate->isChecked()) {
			setGuiUpdates(!_ui->tbToggleGuiUpdate->isChecked());
		} else {
			setGuiUpdates(true);
		}
		break;
	case RL_DBG_RECORD_DRAWCALL:
		title.append(" - ");
		/* FIXME
		title.append(_debugConfig.programArgs[0]);
		*/
		setWindowTitle(title);
		_ui->aOpen->setEnabled(false);
		_ui->aAttach->setEnabled(false);
		if (!_ui->tbBVCaptureAutomatic->isChecked()) {
			_ui->tbBVCapture->setEnabled(true);
		}
		_ui->tbBVCaptureAutomatic->setEnabled(true);
		_ui->tbExecute->setEnabled(true);
		_ui->tbExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/face-devil-green-grin_32.png")));
		_ui->tbStep->setEnabled(false);
		_ui->tbSkip->setEnabled(false);
		_ui->tbEdit->setEnabled(false);
		_ui->tbJumpToDrawCall->setEnabled(false);
		_ui->tbJumpToShader->setEnabled(false);
		_ui->tbJumpToUserDef->setEnabled(false);
		_ui->tbRun->setEnabled(false);
		_ui->tbPause->setEnabled(true);
		_ui->tbToggleGuiUpdate->setEnabled(true);
		_ui->tbToggleNoTrace->setEnabled(false);
		_ui->tbToggleHaltOnError->setEnabled(false);
		_ui->tbSave->setEnabled(false);
		_ui->tbGlTraceSettings->setEnabled(false);
		_ui->tbShaderExecute->setEnabled(false);
		_ui->tbShaderExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/shader-execute_32.png")));
		_ui->tbShaderReset->setEnabled(false);
		_ui->tbShaderStep->setEnabled(false);
		_ui->tbShaderStepOver->setEnabled(false);
		_ui->tbShaderFragmentOptions->setEnabled(false);
		_ui->twShader->setTabIcon(0, QIcon());
		_ui->twShader->setTabIcon(1, QIcon());
		_ui->twShader->setTabIcon(2, QIcon());
		updateWatchGui(0);
		setGuiUpdates(true);
		break;
	case RL_DBG_VERTEX_SHADER:
	case RL_DBG_GEOMETRY_SHADER:
	case RL_DBG_FRAGMENT_SHADER:
		title.append(" - ");
		/* FIXME
		title.append(_debugConfig.programArgs[0]);
		*/
		setWindowTitle(title);
		_ui->aOpen->setEnabled(true);
#ifdef _WIN32
		//_ui->aAttach->setEnabled(true);
		// TODO
#endif /* _WIN32 */
		if (!_ui->tbBVCaptureAutomatic->isChecked()) {
			_ui->tbBVCapture->setEnabled(true);
		}
		_ui->tbBVCaptureAutomatic->setEnabled(true);
		_ui->tbExecute->setEnabled(true);
		_ui->tbExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/face-devil-green-grin_32.png")));
		_ui->tbStep->setEnabled(false);
		_ui->tbSkip->setEnabled(false);
		_ui->tbEdit->setEnabled(false);
		_ui->tbJumpToDrawCall->setEnabled(false);
		_ui->tbJumpToShader->setEnabled(false);
		_ui->tbJumpToUserDef->setEnabled(false);
		_ui->tbRun->setEnabled(false);
		_ui->tbPause->setEnabled(false);
		_ui->tbToggleGuiUpdate->setEnabled(false);
		_ui->tbToggleNoTrace->setEnabled(false);
		_ui->tbToggleHaltOnError->setEnabled(false);
		_ui->tbGlTraceSettings->setEnabled(false);
		_ui->tbSave->setEnabled(true);

		if (m_bHaveValidShaderCode) {
			switch (_ui->twShader->currentIndex()) {
			case 0:
				if (m_pShaders[0]
						&& m_dShResources.transformFeedbackSupported) {
					_ui->tbShaderExecute->setEnabled(true);
				} else {
					_ui->tbShaderExecute->setEnabled(false);
				}
				break;
			case 1:
				if (m_pShaders[1] && m_dShResources.geoShaderSupported
						&& m_dShResources.transformFeedbackSupported) {
					_ui->tbShaderExecute->setEnabled(true);
				} else {
					_ui->tbShaderExecute->setEnabled(false);
				}
				break;
			case 2:
				if (m_pShaders[2]
						&& m_dShResources.framebufferObjectsSupported) {
					_ui->tbShaderExecute->setEnabled(true);
				} else {
					_ui->tbShaderExecute->setEnabled(false);
				}
				break;
			}
		} else {
			_ui->tbShaderExecute->setEnabled(false);
		}
		_ui->tbShaderExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/face-devil-grin_32.png")));
		_ui->tbShaderReset->setEnabled(true);
		_ui->tbShaderStep->setEnabled(true);
		_ui->tbShaderStepOver->setEnabled(true);
		_ui->tbShaderFragmentOptions->setEnabled(false);
		_ui->twShader->setTabIcon(0, QIcon());
		_ui->twShader->setTabIcon(1, QIcon());
		_ui->twShader->setTabIcon(2, QIcon());
		_ui->twShader->setTabIcon(rl - RL_DBG_VERTEX_SHADER,
				QIcon(
						QString::fromUtf8(
								":/icons/icons/shader-execute_32.png")));
		updateWatchGui(0);
		setGuiUpdates(true);
		break;
	case RL_DBG_RESTART:
		title.append(" - ");
		/* FIXME
		title.append(_debugConfig.programArgs[0]);
		*/
		setWindowTitle(title);
		_ui->aOpen->setEnabled(true);
#ifdef _WIN32
		//_ui->aAttach->setEnabled(true);
		// TODO
#endif /* _WIN32 */
		if (!_ui->tbBVCaptureAutomatic->isChecked()) {
			_ui->tbBVCapture->setEnabled(true);
		}
		_ui->tbBVCaptureAutomatic->setEnabled(true);
		_ui->tbExecute->setEnabled(true);
		_ui->tbExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/face-devil-green-grin_32.png")));
		_ui->tbStep->setEnabled(true);
		_ui->tbSkip->setEnabled(true);
		/* FIXME
		if (_currentCall && _currentCall->isEditable()) {
			_ui->tbEdit->setEnabled(true);
		} else {
			_ui->tbEdit->setEnabled(false);
		}
		*/
		_ui->tbJumpToDrawCall->setEnabled(true);
		_ui->tbJumpToShader->setEnabled(true);
		_ui->tbJumpToUserDef->setEnabled(true);
		_ui->tbRun->setEnabled(true);
		_ui->tbPause->setEnabled(false);
		_ui->tbToggleGuiUpdate->setEnabled(true);
		_ui->tbToggleNoTrace->setEnabled(true);
		_ui->tbToggleHaltOnError->setEnabled(true);
		_ui->tbGlTraceSettings->setEnabled(true);
		_ui->tbSave->setEnabled(true);

		if (m_bHaveValidShaderCode) {
			switch (_ui->twShader->currentIndex()) {
			case 0:
				if (m_pShaders[0]
						&& m_dShResources.transformFeedbackSupported) {
					_ui->tbShaderExecute->setEnabled(true);
				} else {
					_ui->tbShaderExecute->setEnabled(false);
				}
				_ui->tbShaderFragmentOptions->setEnabled(false);
				break;
			case 1:
				if (m_pShaders[1] && m_dShResources.geoShaderSupported
						&& m_dShResources.transformFeedbackSupported) {
					_ui->tbShaderExecute->setEnabled(true);
				} else {
					_ui->tbShaderExecute->setEnabled(false);
				}
				_ui->tbShaderFragmentOptions->setEnabled(false);
				break;
			case 2:
				if (m_pShaders[2]
						&& m_dShResources.framebufferObjectsSupported) {
					_ui->tbShaderExecute->setEnabled(true);
					_ui->tbShaderFragmentOptions->setEnabled(true);
				} else {
					_ui->tbShaderExecute->setEnabled(false);
					_ui->tbShaderFragmentOptions->setEnabled(false);
				}
				break;
			}
		} else {
			_ui->tbShaderExecute->setEnabled(false);
		}
		_ui->tbShaderExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/shader-execute_32.png")));
		_ui->tbShaderReset->setEnabled(false);
		_ui->tbShaderStep->setEnabled(false);
		_ui->tbShaderStepOver->setEnabled(false);
		_ui->twShader->setTabIcon(0, QIcon());
		_ui->twShader->setTabIcon(1, QIcon());
		_ui->twShader->setTabIcon(2, QIcon());
		updateWatchGui(0);
		setGuiUpdates(true);
		break;
	}
}

void MainWindow::debugError(ErrorCode error)
{
	QPalette palette;

	switch (error) {
	/* no error */
	case EC_NONE:
		_ui->lSBErrorIcon->setVisible(false);
		_ui->lSBErrorText->setVisible(false);
		_ui->statusbar->showMessage("");
		break;
		/* gl errors and other non-critical errors */
	case EC_GL_INVALID_ENUM:
	case EC_GL_INVALID_VALUE:
	case EC_GL_INVALID_OPERATION:
	case EC_GL_STACK_OVERFLOW:
	case EC_GL_STACK_UNDERFLOW:
	case EC_GL_OUT_OF_MEMORY:
	case EC_GL_TABLE_TOO_LARGE:
	case EC_DBG_READBACK_NOT_ALLOWED:
		_ui->lSBErrorIcon->setVisible(true);
		_ui->lSBErrorIcon->setPixmap(
				QPixmap(
						QString::fromUtf8(
								":/icons/icons/dialog-error-green.png")));
		_ui->lSBErrorText->setVisible(true);
		palette = _ui->lSBErrorText->palette();
		palette.setColor(QPalette::WindowText, Qt::green);
		_ui->lSBErrorText->setPalette(palette);
		_ui->lSBErrorText->setText(getErrorInfo(error));
		_ui->statusbar->showMessage(getErrorDescription(error));
		addGlTraceWarningItem(getErrorDescription(error));
		break;
		/* all other errors are considered critical errors */
	default:
		_ui->lSBErrorIcon->setVisible(true);
		_ui->lSBErrorIcon->setPixmap(
				QPixmap(
						QString::fromUtf8(
								":/icons/icons/dialog-error-blue_32.png")));
		_ui->lSBErrorText->setVisible(true);
		palette = _ui->lSBErrorText->palette();
		palette.setColor(QPalette::WindowText, Qt::blue);
		_ui->lSBErrorText->setPalette(palette);
		_ui->lSBErrorText->setText(getErrorInfo(error));
		_ui->statusbar->showMessage(getErrorDescription(error));
		addGlTraceErrorItem(getErrorDescription(error));
		break;
	}
}


void MainWindow::setMouseOverValues(int x, int y, const bool *active,
		const QVariant *values)
{
	if (x < 0 || y < 0) {
		_ui->lSBCurrentPosition->clear();
		_ui->lSBCurrentValueR->clear();
		_ui->lSBCurrentValueG->clear();
		_ui->lSBCurrentValueB->clear();
	} else {
		_ui->lSBCurrentPosition->setText(
				QString::number(x) + "," + QString::number(y));
		if (active[0]) {
			_ui->lSBCurrentValueR->setText(values[0].toString());
		} else {
			_ui->lSBCurrentValueR->clear();
		}
		if (active[1]) {
			_ui->lSBCurrentValueG->setText(values[1].toString());
		} else {
			_ui->lSBCurrentValueG->clear();
		}
		if (active[2]) {
			_ui->lSBCurrentValueB->setText(values[2].toString());
		} else {
			_ui->lSBCurrentValueB->clear();
		}
	}
}

void MainWindow::newSelectedPixel(int x, int y)
{
	_selectedPixel.rx() = x;
	_selectedPixel.ry() = y;
	_ui->lWatchSelectionPos->setText(
			"Pixel " + QString::number(x) + ", " + QString::number(y));
	if (m_pShVarModel && _selectedPixel.x() >= 0 && _selectedPixel.y() >= 0) {
		m_pShVarModel->setCurrentValues(_selectedPixel.x(), _selectedPixel.y());
	}
}

void MainWindow::newSelectedVertex(int n)
{
	_selectedPixel.rx() = n;
	_selectedPixel.ry() = -1;
	if (m_pShVarModel && _selectedPixel.x() >= 0) {
		_ui->lWatchSelectionPos->setText("Vertex " + QString::number(n));
		m_pShVarModel->setCurrentValues(_selectedPixel.x());
	}
}

void MainWindow::newSelectedPrimitive(int dataIdx)
{
	_selectedPixel.rx() = dataIdx;
	if (m_pShVarModel && _selectedPixel.x() >= 0) {
		_ui->lWatchSelectionPos->setText("Primitive " + QString::number(dataIdx));
		m_pShVarModel->setCurrentValues(_selectedPixel.x());
	}
}

bool MainWindow::loadMruProgram(QString& outProgram, QString& outArguments,
		QString& outWorkDir)
{
	QSettings settings;
	outProgram = settings.value("MRU/Program", "").toString();
	outArguments = settings.value("MRU/Arguments", "").toString();
	outWorkDir = settings.value("MRU/WorkDir", "").toString();
	return true;
}

bool MainWindow::saveMruProgram(const QString& program,
		const QString& arguments, const QString& workDir)
{
	QSettings settings;
	settings.setValue("MRU/Program", program);
	settings.setValue("MRU/Arguments", arguments);
	settings.setValue("MRU/WorkDir", workDir);
	return true;
}

void MainWindow::debuggeeStateChanged(Process::State s) 
{
	if(s == Process::STOPPED) {
		/* debuggee has stopped and we assume it is a debug trap */
		UT_NOTIFY(LV_INFO, "debuggee state " << Process::strState(s).toStdString());
		//debugRunLevel(RL_TRACE_EXECUTE);
		ProcessPtr *p = dynamic_cast<ProcessPtr*>(sender());
		addGlTraceItem(*p);
	} 
	else if (s == Process::TRAPPED) {
		/* trap happens when child forks */
		UT_NOTIFY(LV_INFO, "debuggee state " << Process::strState(s).toStdString());
		UT_NOTIFY(LV_INFO, "We might want to do somethin' 'bout that");
	}
	else if (s == Process::KILLED) {
		/* child got killed by uncaught signal or whatever */
		leaveDBGState();
		QString msg("debuggee died. What a sad day this is.");
		_ui->statusbar->showMessage(msg);
		addGlTraceWarningItem(msg);
		//debugRunLevel(RL_SETUP);
	
	}
	else if (s == Process::EXITED) {
		/* child exited normally */
		leaveDBGState();
		QString msg("debuggee terminated happily.");
		_ui->statusbar->showMessage(msg);
		addGlTraceWarningItem(msg);
		//debugRunLevel(RL_SETUP);
	}
	else {
		UT_NOTIFY(LV_INFO, "Unimplemented");
	}
}

void MainWindow::debugMessage(const QString& msg)
{
	_ui->statusbar->showMessage(msg);
	addGlTraceWarningItem(msg);
}

