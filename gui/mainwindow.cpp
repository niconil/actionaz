/*
	Actionaz
	Copyright (C) 2008-2010 Jonathan Mercier-Ganady

	Actionaz is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Actionaz is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.

	Contact : jmgr@jmgr.info
*/

#include "pch.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "actionfactory.h"
#include "actioninterface.h"
#include "actionbuffer.h"
#include "script.h"
#include "scriptmodel.h"
#include "global.h"
#include "actiondialog.h"
#include "codeeditordialog.h"
#include "aboutdialog.h"
#include "settingsdialog.h"
#include "settings.h"
#include "scriptparametersdialog.h"
#include "crossplatform.h"
#include "executer.h"
#include "newactiondialog.h"
#include "updater.h"
#include "scriptcontentdialog.h"
#include "keywords.h"
#include "modeltest.h"

#include <QSystemTrayIcon>
#include <QSplashScreen>
#include <QInputDialog>
#include <QStandardItemModel>

MainWindow::MainWindow(QSplashScreen *splashScreen, QWidget *parent)
	: QMainWindow(parent),
	ui(new Ui::MainWindow),
	mOpacity(0.0f),
	mOpacityTimer(new QTimer(this)),
	mScriptModified(false),
	mActionFactory(new ActionTools::ActionFactory(this)),
	mScript(new ActionTools::Script(mActionFactory)),
	mScriptModel(new ScriptModel(mScript, mActionFactory, this)),
	mSystemTrayIcon(new QSystemTrayIcon(QIcon(":/icons/logo.png"), this)),
	mSplashScreen(splashScreen),
	mExecuter(0),
	mWasNewActionDockShown(false),
	mWasConsoleDockShown(false),
	mUndoGroup(new QUndoGroup(this)),
	mCompletionModel(new QStandardItemModel(this))
{
	ui->setupUi(this);

	mUndoGroup->addStack(mScriptModel->undoStack());
	mScriptModel->undoStack()->setActive(true);

#ifndef Q_WS_WIN
	ui->actionExport_executable->setEnabled(false);
	ui->actionExport_executable->setVisible(false);
#endif

	ui->actionsDockWidget->setWidget(ui->newActionTreeWidget);
	ui->scriptView->setIconSize(QSize(16, 16));

	QItemSelectionModel *oldModel = ui->scriptView->selectionModel();
	ui->scriptView->setModel(mScriptModel);
	delete oldModel;

	mScriptModel->setSelectionModel(ui->scriptView->selectionModel());

	ui->scriptView->horizontalHeader()->setMovable(true);
	ui->actionQuit->setShortcut(QKeySequence(tr("Alt+F4")));
	readSettings();

	updateProxySettings();

	for(int i=0; i<mMaxRecentFiles; ++i)
	{
		QAction *newAction = new QAction(this);
		mRecentFileActs.append(newAction);
		newAction->setVisible(false);
		connect(newAction, SIGNAL(triggered()), this, SLOT(openRecentFile()));

		ui->menuRecent_scripts->addAction(newAction);
	}
	updateRecentFileActions();

	mOpacityTimer->setSingleShot(false);
	mOpacityTimer->start(25);

	connect(mOpacityTimer, SIGNAL(timeout()), this, SLOT(opacityOpenUpdate()));
	connect(ui->deleteDropTarget, SIGNAL(actionsDropped(QList<int>)), mScriptModel, SLOT(removeActions(QList<int>)));
	connect(ui->scriptView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(editAction(QModelIndex)));
	connect(mScriptModel, SIGNAL(wantToAddAction(int, QString)), this, SLOT(wantToAddAction(int, QString)));
	connect(ui->scriptView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(actionSelectionChanged()));
	connect(mScriptModel, SIGNAL(scriptEdited()), this, SLOT(scriptEdited()));
	connect(ui->newActionTreeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(newActionDoubleClicked(QTreeWidgetItem*,int)));
	connect(mSystemTrayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(systemTrayIconActivated(QSystemTrayIcon::ActivationReason)));
	connect(mActionFactory, SIGNAL(packLoadError(QString)), this, SLOT(packLoadError(QString)));
	connect(ui->consoleWidget, SIGNAL(itemDoubleClicked(int)), this, SLOT(logItemDoubleClicked(int)));

	setWindowTitle("Actionaz[*]");//Set this to fix some warnings about the [*] placeholder

	QTimer::singleShot(1, this, SLOT(postInit()));
}

MainWindow::~MainWindow()
{
	delete mExecuter;

	delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
	QMainWindow::changeEvent(e);
	switch (e->type())
	{
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void MainWindow::postInit()
{
	mPackLoadErrors.clear();

	mActionFactory->loadActionPacks();
	
	//Create one instance of every action to build the completion model
	for(int actionInterfaceIndex = 0; actionInterfaceIndex < mActionFactory->actionCount(); ++actionInterfaceIndex)
	{
		ActionTools::ActionInterface *actionInterface =	mActionFactory->actionInterface(actionInterfaceIndex);
		ActionTools::Action *action = actionInterface->newAction();
		const QMetaObject *actionMetaObject = action->metaObject();
		QStandardItem *actionItem = 0;
		
		//Add all methods
		for(int methodIndex = 0; methodIndex < actionMetaObject->methodCount(); ++methodIndex)
		{
			const QMetaMethod &method = actionMetaObject->method(methodIndex);
			
			if(method.access() != QMetaMethod::Public || method.methodType() != QMetaMethod::Slot)
				continue;
			
			QString signature(method.signature());
			if(signature.startsWith("deleteLater") ||
			   signature.startsWith("startExecution") || 
			   signature.startsWith("stopExecution"))
				continue;
			
			QString methodName(signature.left(signature.indexOf("(")));
			methodName += "(";
			
			bool firstParameter = true;
			foreach(const QByteArray &parameter, method.parameterNames())
			{
				if(firstParameter)
					firstParameter = false;
				else
					methodName += ", ";
				
				methodName += parameter;
			}
			
			methodName += ")";

			if(!actionItem)
				actionItem = new QStandardItem(actionInterface->icon(), actionInterface->id());
			
			QStandardItem *methodItem = new QStandardItem(actionInterface->icon(), methodName);//TODO : Add a smaller icon
			actionItem->appendRow(methodItem);
		}
		
		//Add all enums
		if(actionItem)//Only add enums if a valid method has been added
		{
			for(int enumIndex = 0; enumIndex < actionMetaObject->enumeratorCount(); ++enumIndex)
			{
				const QMetaEnum &enumerator = actionMetaObject->enumerator(enumIndex);
				
				QStandardItem *enumNameItem = new QStandardItem(actionInterface->icon(), enumerator.name());
				
				for(int keyIndex = 0; keyIndex < enumerator.keyCount(); ++keyIndex)
				{
					QStandardItem *enumItem = new QStandardItem(actionInterface->icon(), enumerator.key(keyIndex));//TODO : Add a smaller icon
					enumNameItem->appendRow(enumItem);
				}

				actionItem->appendRow(enumNameItem);
			}
			
			mCompletionModel->appendRow(actionItem);
		}
		
		delete action;
	}
	
	//Add the script keywords
	foreach(const QString &keyword, keywords)
	{
		mCompletionModel->appendRow(new QStandardItem(QIcon(":/icons/keywords.png"), keyword));
	}
	
	//Add our functions
	QStandardItem *scriptItem = new QStandardItem(QIcon(":/icons/keywords.png"), "Script");//TODO : Find an icon to put here (and to the following)
	scriptItem->appendRow(new QStandardItem(QIcon(":/icons/keywords.png"), "nextLine"));
	scriptItem->appendRow(new QStandardItem(QIcon(":/icons/keywords.png"), "stopExecution()"));
	scriptItem->appendRow(new QStandardItem(QIcon(":/icons/keywords.png"), "setVariable(name, value)"));
	scriptItem->appendRow(new QStandardItem(QIcon(":/icons/keywords.png"), "variable(name)"));
	scriptItem->appendRow(new QStandardItem(QIcon(":/icons/keywords.png"), "print(text)"));
	scriptItem->appendRow(new QStandardItem(QIcon(":/icons/keywords.png"), "printWarning(text)"));
	scriptItem->appendRow(new QStandardItem(QIcon(":/icons/keywords.png"), "printError(text)"));
	mCompletionModel->appendRow(scriptItem);
	
	fillNewActionTreeWidget(ui->newActionTreeWidget);

	statusBar()->showMessage(tr("Ready, loaded %1 actions from %2 packs").arg(mActionFactory->actionCount()).arg(mActionFactory->packCount()));

	ui->actionActions_window->setChecked(ui->actionsDockWidget->isVisible());
	ui->actionConsole_window->setChecked(ui->consoleDockWidget->isVisible());
	ui->actionToolbar->setChecked(ui->toolBar->isVisible());

	actionSelectionChanged();
	updateUndoRedoStatus();
	actionCountChanged();

#ifdef Q_WS_X11
	ActionTools::CrossPlatform::setForegroundWindow(this);
#endif

	mSplashScreen->close();
	mSplashScreen->deleteLater();

	if(mPackLoadErrors.count() > 0)
	{
		QString message = tr("<b>Unable to load %n action(s) :</b>\n", "", mPackLoadErrors.count());
		message += "<ul>";

		foreach(const QString &error, mPackLoadErrors)
		{
			message += "<li>" + error + "</li>";
		}

		message += "</ul>";

		QMessageBox::critical(this, tr("Error while loading actions"), message);
	}

	setCurrentFile(QString());

	QSettings settings;
	if(settings.value("general/reopenLastScript", QVariant(false)).toBool())
	{
		QString lastFilename = settings.value("general/lastScript", QString()).toString();

		if(!lastFilename.isEmpty())
		{
			if(!loadFile(lastFilename, false))
			{
				settings.setValue("general/lastScript", QString());

				QMessageBox::warning(this, tr("Open last script"), tr("Unable to open the last script because the file is not readable or you don't have enough rights."));
			}
		}
	}

	if(settings.value("network/updatesCheck", QVariant(ActionTools::Settings::CHECK_FOR_UPDATES_UNKNOWN)) == ActionTools::Settings::CHECK_FOR_UPDATES_UNKNOWN)
	{
		if(QMessageBox::question(this,
								 tr("Automatic updates"),
								 tr("Do you want Actionaz to check once per day if a new version is available ?\nYou can change this setting later in the settings dialog."),
								 QMessageBox::Yes | QMessageBox::No,
								 QMessageBox::Yes) == QMessageBox::Yes)
			settings.setValue("network/updatesCheck", QVariant(ActionTools::Settings::CHECK_FOR_UPDATES_DAY));
		else
			settings.setValue("network/updatesCheck", QVariant(ActionTools::Settings::CHECK_FOR_UPDATES_NEVER));
	}
}

void MainWindow::opacityOpenUpdate()
{
	if(mOpacity < 1.0f)
	{
		mOpacity += 0.04f;
		setWindowOpacity(mOpacity);
	}
	else
	{
		setWindowOpacity(1.0f);
		mOpacityTimer->stop();
		mOpacityTimer->disconnect();
	}
}

void MainWindow::opacityCloseUpdate()
{
	if(mOpacity > 0.0f)
	{
		mOpacity -= 0.06f;
		setWindowOpacity(mOpacity);
	}
	else
		QApplication::quit();
}

void MainWindow::on_actionSave_triggered()
{
	save();
}

void MainWindow::on_actionSave_as_triggered()
{
	saveAs();
}

void MainWindow::on_actionSave_copy_as_triggered()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save copy"), QString(), tr("Actionaz script (*.act)"));
	if(fileName.isEmpty())
		return;

	saveFile(fileName, true);
}

void MainWindow::on_actionOpen_triggered()
{
	if(maybeSave())
	{
		QString fileName = QFileDialog::getOpenFileName(this, tr("Open script"), QString(), tr("Actionaz script (*.act)"));
		if(!fileName.isEmpty())
			loadFile(fileName);
	}
}

void MainWindow::on_actionNew_triggered()
{
	if(maybeSave())
	{
		mScript->removeAll();
		mScript->removeAllParameters();
		setCurrentFile(QString());
		mScriptModel->update();

		actionCountChanged();
	}
}

void MainWindow::on_actionQuit_triggered()
{
	QApplication::quit();
}

void MainWindow::on_deleteDropTarget_clicked()
{
	deleteSelection();
}

void MainWindow::on_actionDelete_action_triggered()
{
	deleteSelection();
}

void MainWindow::on_actionDelete_all_actions_triggered()
{
	if(QMessageBox::question(this, tr("Delete all actions"),
		tr("Do you really want to delete all the actions contained in this script ?"),
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::Yes) != QMessageBox::Yes)
		return;

	scriptWasModified(true);
	mScript->removeAll();
	mScriptModel->update();

	actionSelectionChanged();
	actionCountChanged();
}

void MainWindow::on_actionSelect_all_actions_triggered()
{
	ui->scriptView->selectAll();
}

void MainWindow::on_actionSelect_none_triggered()
{
	ui->scriptView->clearSelection();
}

void MainWindow::on_actionInverse_selection_triggered()
{
	ui->scriptView->selectionModel()->select(
			QItemSelection(mScriptModel->index(0, 0),
						   mScriptModel->index(mScriptModel->rowCount() - 1, 0)),
			QItemSelectionModel::Toggle | QItemSelectionModel::Rows);
}

void MainWindow::on_actionAbout_triggered()
{
	AboutDialog *aboutDialog = new AboutDialog(this);
	aboutDialog->exec();
	delete aboutDialog;
}

void MainWindow::on_actionClear_triggered()
{
	QSettings settings;
	settings.setValue("recentFileList", QStringList());

	updateRecentFileActions();
}

void MainWindow::on_actionExport_executable_triggered()
{
#ifdef Q_WS_WIN
	QString filter(tr("Executable file (*.exe)"));

	QString fileName = QFileDialog::getSaveFileName(this, tr("Save executable script"), QString(), filter);

	//TODO
#endif
}

void MainWindow::on_actionSettings_triggered()
{
	SettingsDialog *settingsDialog = new SettingsDialog(mSystemTrayIcon, this);
	if(settingsDialog->exec() == QDialog::Accepted)
	{
		QSettings settings;

		updateProxySettings();

		int recentFileCount = settings.value("general/maxRecentFiles", 5).toInt();
		if(recentFileCount != mMaxRecentFiles)
		{
			mMaxRecentFiles = recentFileCount;

			updateRecentFileActions();
		}
	}
	delete settingsDialog;
}

void MainWindow::on_actionParameters_triggered()
{
	openParametersDialog();
}

void MainWindow::on_actionMove_up_triggered()
{
	mScriptModel->moveActions(ScriptModel::UP, selectedRows());

	scriptWasModified(true);
}

void MainWindow::on_actionMove_down_triggered()
{
	mScriptModel->moveActions(ScriptModel::DOWN, selectedRows());

	scriptWasModified(true);
}

void MainWindow::on_actionExecute_triggered()
{
	if(mScript->actionCount() == 0 || !mScript->hasEnabledActions())
		return;

	execute(false);
}

void MainWindow::on_actionExecute_selection_triggered()
{
	const QList<int> &selection = selectedRows();
	if(selection.count() == 0)
		return;

	//Unselect all
	for(int actionIndex = 0; actionIndex < mScript->actionCount(); ++actionIndex)
	{
		ActionTools::Action *action = mScript->actionAt(actionIndex);
		if(!action)
			continue;

		action->setSelected(false);
	}

	//Set the current selection
	foreach(int row, selection)
	{
		ActionTools::Action *action = mScript->actionAt(row);
		if(!action)
			continue;

		action->setSelected(true);
	}

	execute(true);
}

void MainWindow::on_actionCut_triggered()
{
	on_actionCopy_triggered();

	mScriptModel->removeActions(selectedRows());
}

void MainWindow::on_actionCopy_triggered()
{
	mScriptModel->copyActions(selectedRows());
}

void MainWindow::on_actionPaste_triggered()
{
	QList<int> selection = selectedRows();
	qSort(selection);
	int destination = mScript->actionCount();

	if(selection.count() > 0)
		destination = selection.at(0);

	mScriptModel->pasteActions(destination);
}

void MainWindow::on_actionUndo_triggered()
{
	mUndoGroup->undo();

	scriptEdited();
}

void MainWindow::on_actionRedo_triggered()
{
	mUndoGroup->redo();

	scriptEdited();
}

void MainWindow::on_actionSet_action_color_triggered()
{
	const QList<int> &selection = selectedRows();
	if(selection.count() == 0)
		return;

	ActionTools::Action *firstAction = mScript->actionAt(selection.at(0));
	if(!firstAction)
		return;

	QColorDialog *colorDialog = new QColorDialog(firstAction->color(), this);
	colorDialog->setOptions(QColorDialog::ShowAlphaChannel | QColorDialog::DontUseNativeDialog);
	colorDialog->setCurrentColor(firstAction->color());

	if(colorDialog->exec() == QDialog::Accepted)
		mScriptModel->setActionsColor(selection, colorDialog->currentColor());

	delete colorDialog;
}

void MainWindow::on_actionClear_selection_color_triggered()
{
	mScriptModel->setActionsColor(selectedRows(), QColor());
}

void MainWindow::on_actionEnable_all_actions_triggered()
{
	mScriptModel->setActionsEnabled(true);
}

void MainWindow::on_actionDisable_all_actions_triggered()
{
	mScriptModel->setActionsEnabled(false);
}

void MainWindow::on_actionEnable_selection_triggered()
{
	mScriptModel->setActionsEnabled(selectedRows(), true);
}

void MainWindow::on_actionDisable_selection_triggered()
{
	mScriptModel->setActionsEnabled(selectedRows(), false);
}

void MainWindow::on_actionNew_action_triggered()
{
	if(mActionFactory->actionCount() == 0)
		return;

	NewActionDialog *dialog = new NewActionDialog(mActionFactory, this);
	fillNewActionTreeWidget(dialog->newActionTreeWidget());
	if(dialog->exec() == QDialog::Accepted)
		wantToAddAction(dialog->selectedAction());
	delete dialog;
}

void MainWindow::on_actionEdit_action_triggered()
{
	const QList<int> &selection = selectedRows();
	if(selection.count() != 1)
		return;

	editAction(mScript->actionAt(selection.at(0)));
}

void MainWindow::on_actionJump_to_line_triggered()
{
	if(mScript->actionCount() == 0)
		return;

	QInputDialog *inputDialog = new QInputDialog(this);
	inputDialog->setWindowTitle(tr("Jump to line"));
	inputDialog->setLabelText(tr("Line :"));
	inputDialog->setInputMode(QInputDialog::IntInput);
	inputDialog->setIntRange(1, mScript->actionCount());

	if(inputDialog->exec() == QDialog::Accepted)
	{
		int line = inputDialog->intValue() - 1;
		if(line >= 0 && line < mScript->actionCount())
		{
			ui->scriptView->setFocus();
			ui->scriptView->selectRow(line);
		}
	}
	delete inputDialog;
}

void MainWindow::on_actionCheck_for_updates_triggered()
{
	Updater *updater = new Updater(this);
	updater->checkNewProgramVersion(true);
}

void MainWindow::on_actionCreate_shortcut_triggered()
{
	if(mCurrentFile.isEmpty())
	{
		if(!saveAs())
			return;
	}

	QString defaultDestination = QDir(QDesktopServices::storageLocation(QDesktopServices::DesktopLocation))
								 .filePath(QFileInfo(mCurrentFile).fileName());

	QString filePath = QFileDialog::getSaveFileName(this, tr("Choose the shortcut destination"), defaultDestination);
	if(filePath.isEmpty())
		return;

#ifdef Q_WS_WIN
	filePath += ".lnk";
#endif

	QFile file(mCurrentFile);
	if(!file.link(filePath))
		QMessageBox::warning(this, tr("Shortcut creation failed"), tr("Unable to create the shortcut."));
}

void MainWindow::on_actionImport_script_content_triggered()
{
	ScriptContentDialog *scriptContentDialog = new ScriptContentDialog(ScriptContentDialog::Write, this);
	if(scriptContentDialog->exec() == QDialog::Accepted)
	{
		QByteArray newContent(scriptContentDialog->text().toUtf8());
		QBuffer buffer(&newContent);

		buffer.open(QIODevice::ReadOnly);

		if(mScript->read(&buffer))
		{
			mScriptModel->update();

			scriptEdited();
		}
	}

	delete scriptContentDialog;
}

void MainWindow::on_actionExport_script_content_triggered()
{
	QBuffer buffer;
	buffer.open(QIODevice::WriteOnly);

	mScript->write(&buffer, Global::ACTIONAZ_VERSION, Global::SCRIPT_VERSION);

	ScriptContentDialog *scriptContentDialog = new ScriptContentDialog(ScriptContentDialog::Read, this);
	scriptContentDialog->setText(buffer.buffer());
	scriptContentDialog->exec();
	delete scriptContentDialog;
}

void MainWindow::on_scriptView_customContextMenuRequested(const QPoint &pos)
{
	ui->menuEdit->exec(ui->scriptView->mapToGlobal(pos));
}

void MainWindow::systemTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
	if(reason == QSystemTrayIcon::DoubleClick)
	{
		switch(windowState())
		{
			case Qt::WindowNoState:
			case Qt::WindowMaximized:
			case Qt::WindowFullScreen:
			case Qt::WindowActive:
				showMinimized();
				break;
			case Qt::WindowMinimized:
				showNormal();
				ActionTools::CrossPlatform::setForegroundWindow(this);
				break;
		}
	}
}

void MainWindow::scriptEdited()
{
	scriptWasModified(true);
	ui->scriptView->resizeColumnToContents(0);
	ui->scriptView->resizeColumnToContents(1);
	actionCountChanged();
	actionEnabled();

	updateUndoRedoStatus();
}

void MainWindow::actionSelectionChanged()
{
	int columnCount = mScriptModel->columnCount();
	if(columnCount == 0 )
		actionSelectionChanged(0);
	else
		actionSelectionChanged(ui->scriptView->selectionModel()->selectedIndexes().count() / columnCount);
}

void MainWindow::scriptWasModified(bool modified)
{
	mScriptModified = modified;
	setWindowModified(modified);
}

void MainWindow::readSettings()
{
	QSettings settings;

	restoreGeometry(settings.value("geometry").toByteArray());
	restoreState(settings.value("windowState").toByteArray());
	mMaxRecentFiles = settings.value("general/maxRecentFiles", 5).toInt();
	mSystemTrayIcon->setVisible(settings.value("general/showTaskbarIcon", true).toBool());

	QList<QVariant> customColors = settings.value("customColors").toList();
	for(int colorIndex = 0; colorIndex < customColors.count(); ++colorIndex)
		QColorDialog::setCustomColor(colorIndex, customColors.at(colorIndex).value<QRgb>());

	ui->clearBeforeExecutionCheckBox->setChecked(settings.value("general/clearConsoleBeforeExecution", true).toBool());
}

void MainWindow::writeSettings()
{
	QSettings settings;

	settings.setValue("geometry", saveGeometry());
	settings.setValue("windowState", saveState());
	settings.setValue("general/maxRecentFiles", mMaxRecentFiles);
	settings.setValue("general/showTaskbarIcon", mSystemTrayIcon->isVisible());

	QList<QVariant> customColors;
	for(int colorIndex = 0; colorIndex < QColorDialog::customCount(); ++colorIndex)
		customColors << QColorDialog::customColor(colorIndex);
	settings.setValue("customColors", customColors);

	settings.setValue("general/clearConsoleBeforeExecution", ui->clearBeforeExecutionCheckBox->isChecked());
}

void MainWindow::updateRecentFileActions()
{
	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();
	files.removeAll(QString());

	int numRecentFiles = qMin(files.size(), mMaxRecentFiles);

	for(int i=0; i<numRecentFiles; ++i)
	{
		QString text = tr("&%1 %2").arg(i + 1).arg(QFileInfo(files.at(i)).fileName());
		mRecentFileActs.at(i)->setText(text);
		mRecentFileActs.at(i)->setData(files.at(i));
		mRecentFileActs.at(i)->setVisible(true);
	}

	for(int j = numRecentFiles; j < mMaxRecentFiles; ++j)
		mRecentFileActs.at(j)->setVisible(false);

	ui->menuRecent_scripts->setEnabled(numRecentFiles > 0);
	ui->actionClear->setEnabled(numRecentFiles > 0);
}

void MainWindow::updateProxySettings()
{
	QSettings settings;

	if(settings.value("network/useAProxy", false).toBool())
	{
		int type = settings.value("network/proxyType", ActionTools::Settings::PROXY_TYPE_HTTP).toInt();
		QNetworkProxy proxy;

		if(type == ActionTools::Settings::PROXY_TYPE_HTTP)
			proxy.setType(QNetworkProxy::HttpProxy);
		else
			proxy.setType(QNetworkProxy::Socks5Proxy);

		proxy.setHostName(settings.value("network/proxyHost", QString("0.0.0.0")).toString());
		proxy.setPort(settings.value("network/proxyPort", 0).toInt());
		proxy.setUser(settings.value("network/proxyUser", QString()).toString());
		proxy.setPassword(settings.value("network/proxyPassword", QString()).toString());
		QNetworkProxy::setApplicationProxy(proxy);
	}
	else
		QNetworkProxy::setApplicationProxy(QNetworkProxy());
}

void MainWindow::updateUndoRedoStatus()
{
	ui->actionUndo->setEnabled(mUndoGroup->canUndo());
	ui->actionRedo->setEnabled(mUndoGroup->canRedo());
}

void MainWindow::execute(bool onlySelection)
{
	if(ui->clearBeforeExecutionCheckBox->isChecked())
		ui->consoleWidget->clear();

	QSettings settings;

	if(settings.value("general/addConsoleStartEndSeparators", QVariant(true)).toBool())
		ui->consoleWidget->addStartSeparator();

	QKeySequence stopExecutionHotkey = QKeySequence(settings.value("actions/stopExecutionHotkey", QKeySequence("Ctrl+Alt+Q")).toString());
	bool showExecutionWindow = settings.value("actions/showExecutionWindow", QVariant(true)).toBool();
	int executionWindowPosition = settings.value("actions/executionWindowPosition", QVariant(0)).toInt();
	int executionWindowScreen = settings.value("actions/executionWindowScreen", QVariant(0)).toInt();
	bool showConsoleWindow = settings.value("actions/showConsoleWindow", QVariant(true)).toBool();
	int consoleWindowPosition = settings.value("actions/consoleWindowPosition", QVariant(1)).toInt();
	int consoleWindowScreen = settings.value("actions/consoleWindowScreen", QVariant(0)).toInt();

	delete mExecuter;

	mExecuter = new Executer(mScript,
									  mActionFactory,
									  showExecutionWindow,
									  executionWindowPosition,
									  executionWindowScreen,
									  showConsoleWindow,
									  consoleWindowPosition,
									  consoleWindowScreen,
									  stopExecutionHotkey,
									  ui->consoleWidget->model(),
									  this);

	connect(mExecuter, SIGNAL(executionStopped()), this, SLOT(scriptExecutionStopped()));

	if(mExecuter->startExecution(onlySelection))
	{
		mPreviousWindowPosition = pos();
		hide();
		mWasNewActionDockShown = !ui->actionsDockWidget->isHidden();
		mWasConsoleDockShown = !ui->consoleDockWidget->isHidden();
		ui->actionsDockWidget->hide();
		ui->consoleDockWidget->hide();
	}
	else
	{
		ui->consoleWidget->updateClearButton();

		mExecuter->deleteLater();
		mExecuter = 0;

		if(settings.value("general/addConsoleStartEndSeparators", QVariant(true)).toBool())
			ui->consoleWidget->addEndSeparator();
	}
}

void MainWindow::fillNewActionTreeWidget(NewActionTreeWidget *widget)
{
	for(int i = 0; i < ActionTools::ActionInterface::CategoryCount; ++i)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem(QStringList(ActionTools::ActionInterface::CategoryName[i]));

		widget->addTopLevelItem(item);

		if(mActionFactory->actionCount(static_cast<ActionTools::ActionInterface::Category>(i)) == 0)
			item->setFlags(Qt::NoItemFlags);
		else
			item->setFlags(Qt::ItemIsEnabled);
	}

	for(int i = 0; i < mActionFactory->actionCount(); ++i)
	{
		ActionTools::ActionInterface *actionInterface = mActionFactory->actionInterface(i);

		QTreeWidgetItem *parentItem = widget->topLevelItem(actionInterface->category());
		QTreeWidgetItem *item = new QTreeWidgetItem(parentItem, QStringList(actionInterface->name()));

		item->setIcon(0, actionInterface->icon());
		item->setToolTip(0, actionInterface->description());
		item->setData(0, NewActionTreeWidget::ActionIdRole, actionInterface->id());
	}

	widget->expandAll();
}

void MainWindow::editAction(const QModelIndex &index)
{
	if(index.column() != ScriptModel::ColumnActionName)
		return;

	editAction(mScript->actionAt(index.row()));
}

void MainWindow::wantToAddAction(const QString &actionId)
{
	wantToAddAction(mScript->actionCount(), actionId);
}

void MainWindow::wantToAddAction(int row, const QString &actionId)
{
	ActionTools::ActionInterface *interface = mActionFactory->actionInterface(actionId);
	if(!interface)
		return;

	ActionTools::Action *action = interface->newAction();
	if(!action)
		return;

	if(editAction(action))
	{
		if(row == mScript->actionCount())
			ui->scriptView->scrollToBottom();

		mScriptModel->insertAction(row, ActionTools::ActionBuffer(actionId, *action));
	}

	delete action;//Always delete it, since the model creates a copy of it...
}

void MainWindow::openRecentFile()
{
	QAction *action = qobject_cast<QAction *>(sender());

	if(action && maybeSave())
	{
		if(!loadFile(action->data().toString()))
		{
			QSettings settings;
			QStringList files = settings.value("recentFileList").toStringList();

			files.removeAll(action->data().toString());
			settings.setValue("recentFileList", files);

			updateRecentFileActions();
		}
	}
}

void MainWindow::newActionDoubleClicked(QTreeWidgetItem *item, int column)
{
	if(column != ScriptModel::ColumnLabel)
		return;

	wantToAddAction(item->data(0, NewActionTreeWidget::ActionIdRole).toString());
}

void MainWindow::actionEnabled()
{
	enabledActionsCountChanged(mScript->hasEnabledActions());
}

void MainWindow::packLoadError(const QString &error)
{
	mPackLoadErrors << error;
}

void MainWindow::scriptExecutionStopped()
{
	mExecuter->deleteLater();
	mExecuter = 0;

	QSettings settings;

	if(settings.value("general/addConsoleStartEndSeparators", QVariant(true)).toBool())
		ui->consoleWidget->addEndSeparator();

	ui->consoleWidget->updateClearButton();

	show();

	ui->actionsDockWidget->setVisible(mWasNewActionDockShown);
	ui->consoleDockWidget->setVisible(mWasConsoleDockShown);

	QTimer::singleShot(50, this, SLOT(postExecution()));
}

void MainWindow::postExecution()
{
	move(mPreviousWindowPosition);
}

void MainWindow::logItemDoubleClicked(int itemRow)
{
	QStandardItemModel *model = qobject_cast<QStandardItemModel *>(ui->consoleWidget->model());
	if(!model)
		return;

	QStandardItem *item = model->item(itemRow, 0);
	if(!item)
		return;

	switch(item->data(ActionTools::ConsoleWidget::SourceRole).value<ActionTools::ConsoleWidget::Source>())
	{
	case ActionTools::ConsoleWidget::Parameters:
		{
			int parameter = item->data(ActionTools::ConsoleWidget::ParameterRole).toInt();
			int line = item->data(ActionTools::ConsoleWidget::LineRole).toInt();
			int column = item->data(ActionTools::ConsoleWidget::ColumnRole).toInt();
			openParametersDialog(parameter, line, column);
		}
		break;
	case ActionTools::ConsoleWidget::Action:
	case ActionTools::ConsoleWidget::User:
		{
			int action = item->data(ActionTools::ConsoleWidget::ActionRole).toInt();
			QString field = item->data(ActionTools::ConsoleWidget::FieldRole).toString();
			QString subField = item->data(ActionTools::ConsoleWidget::SubFieldRole).toString();
			int line = item->data(ActionTools::ConsoleWidget::LineRole).toInt();
			int column = item->data(ActionTools::ConsoleWidget::ColumnRole).toInt();
			editAction(mScript->actionAt(action), field, subField, line, column);
		}
		break;
	}
}

bool MainWindow::editAction(ActionTools::Action *action, const QString &field, const QString &subField, int line, int column)
{
	if(!action)
		return false;

	if(action->interface()->elements().count() == 0)
		return true;

	ActionTools::ParametersData previousData = action->parametersData();
	ActionDialog *dialog = new ActionDialog(mCompletionModel, mScript, action, this);
	dialog->setCurrentField(field, subField);
	dialog->setCurrentLine(line);
	dialog->setCurrentColumn(column);
	int result = dialog->exec();

	if(result == QDialog::Accepted)
	{
		if(previousData != action->parametersData())
			scriptEdited();
	}

	delete dialog;

	return (result == QDialog::Accepted);
}

void MainWindow::openParametersDialog(int parameter, int line, int column)
{
	ScriptParametersDialog *scriptParametersDialog = new ScriptParametersDialog(mCompletionModel, mScript, this);
	QList<ActionTools::ScriptParameter> parameters = mScript->parameters();
	scriptParametersDialog->setCurrentParameter(parameter);
	scriptParametersDialog->setCurrentLine(line);
	scriptParametersDialog->setCurrentColumn(column);
	if(scriptParametersDialog->exec() == QDialog::Accepted && mScript->parameters() != parameters)
		scriptWasModified(true);
	delete scriptParametersDialog;
}

QList<int> MainWindow::selectedRows() const
{
	QModelIndexList selectedIndexes = ui->scriptView->selectionModel()->selectedIndexes();

	if(selectedIndexes.count() == 0)
		return QList<int>();

	QList<int> selectedRows;

	foreach(const QModelIndex &index, selectedIndexes)
	{
		if(index.column() == ScriptModel::ColumnLabel)
			selectedRows << index.row();
	}

	return selectedRows;
}

bool MainWindow::loadFile(const QString &fileName, bool verbose)
{
	QFile loadFile(fileName);
	if(!loadFile.open(QIODevice::ReadOnly))
	{
		if(verbose)
			QMessageBox::warning(this, tr("Open script"), tr("Unable to open the script because the file is not readable or you don't have enough rights."));
		return false;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);

	bool result = mScript->read(&loadFile);

	QApplication::restoreOverrideCursor();

	loadFile.close();

	if(result)
	{
		QSettings settings;

		settings.setValue("general/lastScript", fileName);

		mScriptModel->update();

		statusBar()->showMessage(tr("File loaded"), 2000);

		scriptEdited();

		setCurrentFile(fileName);
	}

	return result;
}

bool MainWindow::saveFile(const QString &fileName, bool copy)
{
	QFile saveFile(fileName);
	if(!saveFile.open(QIODevice::WriteOnly))
	{
		QMessageBox::warning(this, tr("Save script"), tr("Unable to save the script because the file is not writeable or you don't have enough rights."));
		return false;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);

	bool result = mScript->write(&saveFile, Global::ACTIONAZ_VERSION, Global::SCRIPT_VERSION);

	QApplication::restoreOverrideCursor();

	saveFile.close();

	if(result)
	{
		if(!copy)
			setCurrentFile(fileName);
		statusBar()->showMessage(tr("File saved"), 2000);
	}

	return result;
}

void MainWindow::setCurrentFile(const QString &fileName)
{
	mCurrentFile = fileName;
	setWindowTitle(QString());
	scriptWasModified(false);

	QString shownName = mCurrentFile;
	if(shownName.isEmpty())
		shownName = tr("untitled.act");
	setWindowFilePath(shownName);

	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();
	files.removeAll(fileName);
	files.prepend(fileName);
	while(files.size() > mMaxRecentFiles)
		files.removeLast();

	settings.setValue("recentFileList", files);

	updateRecentFileActions();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if(maybeSave())
	{
		writeSettings();

		mOpacityTimer->start(25);
		connect(mOpacityTimer, SIGNAL(timeout()), this, SLOT(opacityCloseUpdate()));

		event->ignore();//Ignore, since we have to wait until the fade out is done
	}
	else
		event->ignore();
}

bool MainWindow::maybeSave()
{
	if(mScriptModified)
	{
		QMessageBox::StandardButton result = QMessageBox::warning(this, tr("Save changes"), tr("The script has been modified.\nDo you want to save your changes ?"),
			QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
		if(result == QMessageBox::Save)
			return save();
		else if(result == QMessageBox::Cancel)
			return false;
	}

	return true;
}

bool MainWindow::save()
{
	if(mCurrentFile.isEmpty())
		return saveAs();
	else
		return saveFile(mCurrentFile);
}

bool MainWindow::saveAs()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save script"), QString(), tr("Actionaz script (*.act)"));
	if(fileName.isEmpty())
		return false;

	return saveFile(fileName);
}

void MainWindow::deleteSelection()
{
	mScriptModel->removeActions(selectedRows());

	scriptWasModified(true);
}

void MainWindow::actionSelectionChanged(int selectionCount)
{
	bool hasSelection = (selectionCount > 0);

	bool hasSelectionEnabledActions = false;
	foreach(int row, selectedRows())
	{
		ActionTools::Action *action = mScript->actionAt(row);
		if(!action)
			continue;

		if(action->isEnabled())
		{
			hasSelectionEnabledActions = true;
			break;
		}
	}

	ui->actionDelete_action->setEnabled(hasSelection);
	ui->actionCut->setEnabled(hasSelection);
	ui->actionCopy->setEnabled(hasSelection);
	ui->actionEdit_action->setEnabled(selectionCount == 1);
	ui->actionExecute_selection->setEnabled(hasSelection && hasSelectionEnabledActions);
	ui->actionMove_up->setEnabled(hasSelection);
	ui->actionMove_down->setEnabled(hasSelection);
	ui->actionSet_action_color->setEnabled(hasSelection);
	ui->actionClear_selection_color->setEnabled(hasSelection);
	ui->deleteDropTarget->setEnabled(hasSelection);
	ui->actionEnable_selection->setEnabled(hasSelection);
	ui->actionDisable_selection->setEnabled(hasSelection);
}

void MainWindow::actionCountChanged()
{
	bool hasActions = (mScript->actionCount() > 0);

	ui->actionDelete_all_actions->setEnabled(hasActions);
	ui->actionSelect_all_actions->setEnabled(hasActions);
	ui->actionExecute->setEnabled(hasActions);
	ui->actionSelect_all_actions->setEnabled(hasActions);
	ui->actionSelect_none->setEnabled(hasActions);
	ui->actionEnable_all_actions->setEnabled(hasActions);
	ui->actionDisable_all_actions->setEnabled(hasActions);
	ui->actionInverse_selection->setEnabled(hasActions);
	ui->actionJump_to_line->setEnabled(hasActions);
}

void MainWindow::enabledActionsCountChanged(bool hasEnabledActions)
{
	ui->actionExecute->setEnabled(hasEnabledActions);
	ui->actionExecute_selection->setEnabled(hasEnabledActions);
}