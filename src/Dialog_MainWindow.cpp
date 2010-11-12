///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2010 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// http://www.gnu.org/licenses/gpl-2.0.txt
///////////////////////////////////////////////////////////////////////////////

#include "Dialog_MainWindow.h"

//LameXP includes
#include "Global.h"
#include "Resource.h"
#include "Dialog_WorkingBanner.h"
#include "Dialog_MetaInfo.h"
#include "Dialog_About.h"
#include "Thread_FileAnalyzer.h"
#include "Thread_MessageHandler.h"
#include "Model_MetaInfo.h"

//Qt includes
#include <QMessageBox>
#include <QTimer>
#include <QDesktopWidget>
#include <QDate>
#include <QFileDialog>
#include <QInputDialog>
#include <QFileSystemModel>
#include <QDesktopServices>
#include <QUrl>
#include <QPlastiqueStyle>
#include <QCleanlooksStyle>
#include <QWindowsVistaStyle>
#include <QWindowsStyle>
#include <QSysInfo>

//Win32 includes
#include <Windows.h>

//Helper macros
#define ABORT_IF_BUSY \
if(m_banner->isVisible() || m_delayedFileTimer->isActive()) \
{ \
	MessageBeep(MB_ICONEXCLAMATION); \
	return; \
}
#define LINK(URL) QString("<a href=\"%1\">%2</a>").arg(URL).arg(URL)

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent)
{
	//Init the dialog, from the .ui file
	setupUi(this);
	
	//Register meta types
	qRegisterMetaType<AudioFileModel>("AudioFileModel");

	//Update window title
	if(lamexp_version_demo())
	{
		setWindowTitle(windowTitle().append(" [DEMO VERSION]"));
	}

	//Enabled main buttons
	connect(buttonAbout, SIGNAL(clicked()), this, SLOT(aboutButtonClicked()));
	connect(buttonStart, SIGNAL(clicked()), this, SLOT(encodeButtonClicked()));

	//Setup tab widget
	tabWidget->setCurrentIndex(0);
	connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabPageChanged(int)));

	//Setup "Source" tab
	m_fileListModel = new FileListModel();
	sourceFileView->setModel(m_fileListModel);
	sourceFileView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	sourceFileView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	connect(buttonAddFiles, SIGNAL(clicked()), this, SLOT(addFilesButtonClicked()));
	connect(buttonRemoveFile, SIGNAL(clicked()), this, SLOT(removeFileButtonClicked()));
	connect(buttonClearFiles, SIGNAL(clicked()), this, SLOT(clearFilesButtonClicked()));
	connect(buttonFileUp, SIGNAL(clicked()), this, SLOT(fileUpButtonClicked()));
	connect(buttonFileDown, SIGNAL(clicked()), this, SLOT(fileDownButtonClicked()));
	connect(buttonShowDetails, SIGNAL(clicked()), this, SLOT(showDetailsButtonClicked()));
	
	//Setup "Output" tab
	m_fileSystemModel = new QFileSystemModel();
	m_fileSystemModel->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
	m_fileSystemModel->setRootPath(m_fileSystemModel->rootPath());
	outputFolderView->setModel(m_fileSystemModel);
	outputFolderView->header()->setStretchLastSection(true);
	outputFolderView->header()->hideSection(1);
	outputFolderView->header()->hideSection(2);
	outputFolderView->header()->hideSection(3);
	outputFolderView->setHeaderHidden(true);
	outputFolderView->setAnimated(true);
	connect(outputFolderView, SIGNAL(clicked(QModelIndex)), this, SLOT(outputFolderViewClicked(QModelIndex)));
	outputFolderView->setCurrentIndex(m_fileSystemModel->index(QDesktopServices::storageLocation(QDesktopServices::MusicLocation)));
	outputFolderViewClicked(outputFolderView->currentIndex());
	connect(buttonMakeFolder, SIGNAL(clicked()), this, SLOT(makeFolderButtonClicked()));
	connect(buttonGotoHome, SIGNAL(clicked()), SLOT(gotoHomeFolderButtonClicked()));
	connect(buttonGotoDesktop, SIGNAL(clicked()), this, SLOT(gotoDesktopButtonClicked()));
	connect(buttonGotoMusic, SIGNAL(clicked()), this, SLOT(gotoMusicFolderButtonClicked()));
	
	//Setup "Meta Data" tab
	m_metaData = new AudioFileModel();
	m_metaInfoModel = new MetaInfoModel(m_metaData, 6);
	m_metaInfoModel->clearData();
	metaDataView->setModel(m_metaInfoModel);
	metaDataView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	metaDataView->verticalHeader()->hide();
	metaDataView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	connect(buttonEditMeta, SIGNAL(clicked()), this, SLOT(editMetaButtonClicked()));
	connect(buttonClearMeta, SIGNAL(clicked()), this, SLOT(clearMetaButtonClicked()));
	
	//Setup "Compression" tab
	sliderBitrate->setValue(24);
	connect(sliderBitrate, SIGNAL(valueChanged(int)), this, SLOT(updateBitrate(int)));
	updateBitrate(sliderBitrate->value());

	//Activate file menu actions
	connect(actionOpenFolder, SIGNAL(triggered()), this, SLOT(openFolderActionActivated()));

	//Activate view menu actions
	m_tabActionGroup = new QActionGroup(this);
	m_tabActionGroup->addAction(actionSourceFiles);
	m_tabActionGroup->addAction(actionOutputDirectory);
	m_tabActionGroup->addAction(actionCompression);
	m_tabActionGroup->addAction(actionMetaData);
	m_tabActionGroup->addAction(actionAdvancedOptions);
	actionSourceFiles->setChecked(true);
	connect(m_tabActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(tabActionActivated(QAction*)));

	//Activate style menu actions
	m_styleActionGroup = new QActionGroup(this);
	m_styleActionGroup->addAction(actionStylePlastique);
	m_styleActionGroup->addAction(actionStyleCleanlooks);
	m_styleActionGroup->addAction(actionStyleWindowsVista);
	m_styleActionGroup->addAction(actionStyleWindowsXP);
	m_styleActionGroup->addAction(actionStyleWindowsClassic);
	actionStylePlastique->setChecked(true);
	actionStyleWindowsXP->setEnabled((QSysInfo::windowsVersion() & QSysInfo::WV_NT_based) >= QSysInfo::WV_XP);
	actionStyleWindowsVista->setEnabled((QSysInfo::windowsVersion() & QSysInfo::WV_NT_based) >= QSysInfo::WV_VISTA);
	connect(m_styleActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(styleActionActivated(QAction*)));

	//Activate help menu actions
	connect(actionCheckUpdates, SIGNAL(triggered()), this, SLOT(checkUpdatesActionActivated()));
	connect(actionVisitHomepage, SIGNAL(triggered()), this, SLOT(visitHomepageActionActivated()));
	
	//Center window in screen
	QRect desktopRect = QApplication::desktop()->screenGeometry();
	QRect thisRect = this->geometry();
	move((desktopRect.width() - thisRect.width()) / 2, (desktopRect.height() - thisRect.height()) / 2);
	setMinimumSize(thisRect.width(), thisRect.height());

	//Create banner
	m_banner = new WorkingBanner(this);

	//Create message handler thread
	m_messageHandler = new MessageHandlerThread();
	m_delayedFileList = new QStringList();
	m_delayedFileTimer = new QTimer();
	connect(m_messageHandler, SIGNAL(otherInstanceDetected()), this, SLOT(notifyOtherInstance()), Qt::QueuedConnection);
	connect(m_messageHandler, SIGNAL(fileReceived(QString)), this, SLOT(addFileDelayed(QString)), Qt::QueuedConnection);
	connect(m_messageHandler, SIGNAL(killSignalReceived()), this, SLOT(close()), Qt::QueuedConnection);
	connect(m_delayedFileTimer, SIGNAL(timeout()), this, SLOT(handleDelayedFiles()));
	m_messageHandler->start();
}

////////////////////////////////////////////////////////////
// Destructor
////////////////////////////////////////////////////////////

MainWindow::~MainWindow(void)
{
	//Stop message handler thread
	if(m_messageHandler && m_messageHandler->isRunning())
	{
		m_messageHandler->stop();
		if(!m_messageHandler->wait(10000))
		{
			m_messageHandler->terminate();
			m_messageHandler->wait();
		}
	}

	//Unset models
	sourceFileView->setModel(NULL);
	metaDataView->setModel(NULL);
	
	//Free memory
	LAMEXP_DELETE(m_tabActionGroup);
	LAMEXP_DELETE(m_styleActionGroup);
	LAMEXP_DELETE(m_fileListModel);
	LAMEXP_DELETE(m_banner);
	LAMEXP_DELETE(m_fileSystemModel);
	LAMEXP_DELETE(m_messageHandler);
	LAMEXP_DELETE(m_delayedFileList);
	LAMEXP_DELETE(m_delayedFileTimer);
	LAMEXP_DELETE(m_metaData);
	LAMEXP_DELETE(m_metaInfoModel);
}

////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
////////////////////////////////////////////////////////////

void MainWindow::addFiles(const QStringList &files)
{
	if(files.isEmpty())
	{
		return;
	}

	tabWidget->setCurrentIndex(0);

	FileAnalyzer *analyzer = new FileAnalyzer(files);
	connect(analyzer, SIGNAL(fileSelected(QString)), m_banner, SLOT(setText(QString)), Qt::QueuedConnection);
	connect(analyzer, SIGNAL(fileAnalyzed(AudioFileModel)), m_fileListModel, SLOT(addFile(AudioFileModel)), Qt::QueuedConnection);

	m_banner->show("Adding file(s), please wait...", analyzer);

	if(analyzer->filesDenied())
	{
		QMessageBox::warning(this, "Access Denied", QString("<nobr>%1 file(s) have been rejected, because read access was not granted!<br>This usually means the file is locked by another process.</nobr>").arg(analyzer->filesDenied()));
	}
	if(analyzer->filesRejected())
	{
		QMessageBox::warning(this, "Files Rejected", QString("<nobr>%1 file(s) have been rejected, because the file format could not be recognized!<br>This usually means the file is damaged or the file format is not supported.</nobr>").arg(analyzer->filesRejected()));
	}

	LAMEXP_DELETE(analyzer);
	sourceFileView->scrollToBottom();
	m_banner->close();
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

void MainWindow::showEvent(QShowEvent *event)
{
	QTimer::singleShot(0, this, SLOT(windowShown()));
}

////////////////////////////////////////////////////////////
// Slots
////////////////////////////////////////////////////////////

/*
 * Window shown
 */
void MainWindow::windowShown(void)
{
	QStringList arguments = QApplication::arguments();

	AboutDialog *about = new AboutDialog(this, true);
	int iAccepted = about->exec();
	LAMEXP_DELETE(about);

	if(iAccepted <= 0)
	{
		QApplication::quit();
		return;
	}
	
	//Check for AAC support
	if(lamexp_check_tool("neroAacEnc.exe") && lamexp_check_tool("neroAacDec.exe") && lamexp_check_tool("neroAacTag.exe"))
	{
		if(lamexp_tool_version("neroAacEnc.exe") < lamexp_toolver_neroaac())
		{
			QString messageText;
			messageText += "<nobr>LameXP detected that your version of the Nero AAC encoder is outdated!<br>";
			messageText += "The current version available is " + lamexp_version2string("?.?.?.?", lamexp_toolver_neroaac()) + " (or later), but you still have version " + lamexp_version2string("?.?.?.?", lamexp_tool_version("neroAacEnc.exe")) + " installed.<br><br>";
			messageText += "You can download the latest version of the Nero AAC encoder from the Nero website at:<br>";
			messageText += "<b>" + LINK(AboutDialog::neroAacUrl) + "</b><br></nobr>";
			QMessageBox::information(this, "AAC Encoder Outdated", messageText);
		}
		radioButtonEncoderAAC->setEnabled(true);
	}
	else
	{
		QString messageText;
		messageText += "<nobr>The Nero AAC encoder could not be found. AAC encoding support will be disabled.<br>";
		messageText += "Please put 'neroAacEnc.exe', 'neroAacDec.exe' and 'neroAacTag.exe' into the LameXP directory!<br><br>";
		messageText += "You can download the Nero AAC encoder for free from the official Nero website at:<br>";
		messageText += "<b>" + LINK(AboutDialog::neroAacUrl) + "</b><br></nobr>";
		QMessageBox::information(this, "AAC Support Disabled", messageText);
		radioButtonEncoderAAC->setEnabled(false);
	}
	
	//Add files from the command-line
	for(int i = 0; i < arguments.count() - 1; i++)
	{
		if(!arguments[i].compare("--add", Qt::CaseInsensitive))
		{
			QFileInfo currentFile(arguments[++i].trimmed());
			qDebug("Adding file from CLI: %s", currentFile.absoluteFilePath().toUtf8().constData());
			if(currentFile.exists())
			{
				m_delayedFileList->append(currentFile.absoluteFilePath());
			}
			else
			{
				qWarning("File doesn't exist: %s", currentFile.absoluteFilePath().toUtf8().constData());
			}
		}
	}

	if(!m_delayedFileList->isEmpty() && !m_delayedFileTimer->isActive())
	{
		m_delayedFileTimer->start(5000);
	}
}

/*
 * About button
 */
void MainWindow::aboutButtonClicked(void)
{
	ABORT_IF_BUSY;
	AboutDialog *aboutBox = new AboutDialog(this);
	aboutBox->exec();
	LAMEXP_DELETE(aboutBox);
}

/*
 * Encode button
 */
void MainWindow::encodeButtonClicked(void)
{
	ABORT_IF_BUSY;
	QMessageBox::warning(this, "LameXP", "Not implemented yet, please try again with a later version!");
}

/*
 * Add file(s) button
 */
void MainWindow::addFilesButtonClicked(void)
{
	ABORT_IF_BUSY;
	QStringList selectedFiles = QFileDialog::getOpenFileNames(this, "Add file(s)", QString(), "All supported files (*.*)");
	addFiles(selectedFiles);
}

/*
 * Open folder action
 */
void MainWindow::openFolderActionActivated(void)
{
	ABORT_IF_BUSY;
	QString selectedFolder = QFileDialog::getExistingDirectory(this, "Add folder", QDesktopServices::storageLocation(QDesktopServices::MusicLocation));
	
	if(!selectedFolder.isEmpty())
	{
		QDir sourceDir(selectedFolder);
		QFileInfoList fileInfoList = sourceDir.entryInfoList(QDir::Files);
		QStringList fileList;

		while(!fileInfoList.isEmpty())
		{
			fileList << fileInfoList.takeFirst().absoluteFilePath();
		}

		addFiles(fileList);
	}
}

/*
 * Remove file button
 */
void MainWindow::removeFileButtonClicked(void)
{
	if(sourceFileView->currentIndex().isValid())
	{
		int iRow = sourceFileView->currentIndex().row();
		m_fileListModel->removeFile(sourceFileView->currentIndex());
		sourceFileView->selectRow(iRow < m_fileListModel->rowCount() ? iRow : m_fileListModel->rowCount()-1);
	}
}

/*
 * Clear files button
 */
void MainWindow::clearFilesButtonClicked(void)
{
	m_fileListModel->clearFiles();
}

/*
 * Move file up button
 */
void MainWindow::fileUpButtonClicked(void)
{
	if(sourceFileView->currentIndex().isValid())
	{
		int iRow = sourceFileView->currentIndex().row() - 1;
		m_fileListModel->moveFile(sourceFileView->currentIndex(), -1);
		sourceFileView->selectRow(iRow >= 0 ? iRow : 0);
	}
}

/*
 * Move file down button
 */
void MainWindow::fileDownButtonClicked(void)
{
	if(sourceFileView->currentIndex().isValid())
	{
		int iRow = sourceFileView->currentIndex().row() + 1;
		m_fileListModel->moveFile(sourceFileView->currentIndex(), 1);
		sourceFileView->selectRow(iRow < m_fileListModel->rowCount() ? iRow : m_fileListModel->rowCount()-1);
	}
}

/*
 * Show details button
 */
void MainWindow::showDetailsButtonClicked(void)
{
	ABORT_IF_BUSY;

	int iResult = 0;
	MetaInfoDialog *metaInfoDialog = new MetaInfoDialog(this);
	QModelIndex index = sourceFileView->currentIndex();

	while(index.isValid())
	{
		if(iResult > 0)
		{
			index = m_fileListModel->index(index.row() + 1, index.column()); 
			sourceFileView->selectRow(index.row());
		}
		if(iResult < 0)
		{
			index = m_fileListModel->index(index.row() - 1, index.column()); 
			sourceFileView->selectRow(index.row());
		}

		AudioFileModel &file = (*m_fileListModel)[index];
		iResult = metaInfoDialog->exec(file, index.row() > 0, index.row() < m_fileListModel->rowCount() - 1);

		if(!iResult) break;
	}

	LAMEXP_DELETE(metaInfoDialog);
}

/*
 * Tab page changed
 */
void MainWindow::tabPageChanged(int idx)
{
	switch(idx)
	{
	case 0:
		actionSourceFiles->setChecked(true);
		break;
	case 1:
		actionOutputDirectory->setChecked(true);
		break;
	case 2:
		actionMetaData->setChecked(true);
		break;
	case 3:
		actionCompression->setChecked(true);
		break;
	case 4:
		actionAdvancedOptions->setChecked(true);
		break;
	}
}

/*
 * Tab action triggered
 */
void MainWindow::tabActionActivated(QAction *action)
{
	int idx = -1;

	if(actionSourceFiles == action) idx = 0;
	else if(actionOutputDirectory == action) idx = 1;
	else if(actionMetaData == action) idx = 2;
	else if(actionCompression == action) idx = 3;
	else if(actionAdvancedOptions == action) idx = 4;

	if(idx >= 0)
	{
		tabWidget->setCurrentIndex(idx);
	}
}

/*
 * Style action triggered
 */
void MainWindow::styleActionActivated(QAction *action)
{
	if(action == actionStylePlastique) QApplication::setStyle(new QPlastiqueStyle());
	else if(action == actionStyleCleanlooks) QApplication::setStyle(new QCleanlooksStyle());
	else if(action == actionStyleWindowsVista) QApplication::setStyle(new QWindowsVistaStyle());
	else if(action == actionStyleWindowsXP) QApplication::setStyle(new QWindowsXPStyle());
	else if(action == actionStyleWindowsClassic) QApplication::setStyle(new QWindowsStyle());
}

/*
 * Output folder changed
 */
void MainWindow::outputFolderViewClicked(const QModelIndex &index)
{
	QString selectedDir = m_fileSystemModel->filePath(index);
	if(selectedDir.length() < 3) selectedDir.append(QDir::separator());
	outputFolderLabel->setText(selectedDir);
}

/*
 * Goto desktop button
 */
void MainWindow::gotoDesktopButtonClicked(void)
{
	outputFolderView->setCurrentIndex(m_fileSystemModel->index(QDesktopServices::storageLocation(QDesktopServices::DesktopLocation)));
	outputFolderViewClicked(outputFolderView->currentIndex());
	outputFolderView->setFocus();
}

/*
 * Goto home folder button
 */
void MainWindow::gotoHomeFolderButtonClicked(void)
{
	outputFolderView->setCurrentIndex(m_fileSystemModel->index(QDesktopServices::storageLocation(QDesktopServices::HomeLocation)));
	outputFolderViewClicked(outputFolderView->currentIndex());
	outputFolderView->setFocus();
}

/*
 * Goto music folder button
 */
void MainWindow::gotoMusicFolderButtonClicked(void)
{
	outputFolderView->setCurrentIndex(m_fileSystemModel->index(QDesktopServices::storageLocation(QDesktopServices::MusicLocation)));
	outputFolderViewClicked(outputFolderView->currentIndex());
	outputFolderView->setFocus();
}

/*
 * Make folder button
 */
void MainWindow::makeFolderButtonClicked(void)
{
	ABORT_IF_BUSY;

	QDir basePath(m_fileSystemModel->filePath(outputFolderView->currentIndex()));
	
	bool bApplied = true;
	QString folderName = QInputDialog::getText(this, "New Folder", "Enter the name of the new folder:", QLineEdit::Normal, "New folder", &bApplied, Qt::WindowStaysOnTopHint).simplified();

	if(bApplied)
	{
		folderName.remove(":", Qt::CaseInsensitive);
		folderName.remove("/", Qt::CaseInsensitive);
		folderName.remove("\\", Qt::CaseInsensitive);
		folderName.remove("?", Qt::CaseInsensitive);
		folderName.remove("*", Qt::CaseInsensitive);
		folderName.remove("<", Qt::CaseInsensitive);
		folderName.remove(">", Qt::CaseInsensitive);
		
		int i = 1;
		QString newFolder = folderName;

		while(basePath.exists(newFolder))
		{
			newFolder = QString(folderName).append(QString().sprintf(" (%d)", ++i));
		}
		
		if(basePath.mkdir(newFolder))
		{
			QDir createdDir = basePath;
			if(createdDir.cd(newFolder))
			{
				outputFolderView->setCurrentIndex(m_fileSystemModel->index(createdDir.absolutePath()));
				outputFolderViewClicked(outputFolderView->currentIndex());
				outputFolderView->setFocus();
			}
		}
		else
		{
			QMessageBox::warning(this, "Failed to create folder", QString("The folder '%1' could not be created!").arg(newFolder));
		}
	}
}

/*
 * Edit meta button clicked
 */
void MainWindow::editMetaButtonClicked(void)
{
	ABORT_IF_BUSY;
	m_metaInfoModel->editItem(metaDataView->currentIndex(), this);
}

/*
 * Reset meta button clicked
 */
void MainWindow::clearMetaButtonClicked(void)
{
	ABORT_IF_BUSY;
	m_metaInfoModel->clearData();
}

/*
 * Visit homepage action
 */
void MainWindow::visitHomepageActionActivated(void)
{
	QDesktopServices::openUrl(QUrl("http://mulder.dummwiedeutsch.de/"));
}

/*
 * Check for updates action
 */
void MainWindow::checkUpdatesActionActivated(void)
{
	ABORT_IF_BUSY;

	m_banner->show("Checking for updates, please be patient...");
	
	for(int i = 0; i < 300; i++)
	{
		QApplication::processEvents();
		Sleep(5);
	}
	
	m_banner->close();
	
	QMessageBox::information(this, "Update Check", "Your version of LameXP is still up-to-date. There are no updates available.\nPlease remember to check for updates at regular intervals!");
}

/*
 * Other instance detected
 */
void MainWindow::notifyOtherInstance(void)
{
	if(!m_banner->isVisible())
	{
		QMessageBox msgBox(QMessageBox::Warning, "Already running", "LameXP is already running, please use the running instance!", QMessageBox::NoButton, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
		msgBox.exec();
	}
}

/*
 * Add file from another instance
 */
void MainWindow::addFileDelayed(const QString &filePath)
{
	m_delayedFileTimer->stop();
	qDebug("Received file: %s", filePath.toUtf8().constData());
	m_delayedFileList->append(filePath);
	m_delayedFileTimer->start(5000);
}

/*
 * Add all pending files
 */
void MainWindow::handleDelayedFiles(void)
{
	if(m_banner->isVisible())
	{
		return;
	}
	
	m_delayedFileTimer->stop();
	if(m_delayedFileList->isEmpty())
	{
		return;
	}

	QStringList selectedFiles;
	tabWidget->setCurrentIndex(0);

	while(!m_delayedFileList->isEmpty())
	{
		selectedFiles << QFileInfo(m_delayedFileList->takeFirst()).absoluteFilePath();
	}
	
	addFiles(selectedFiles);
}

/*
 * Update bitrate
 */
void MainWindow::updateBitrate(int value)
{
	labelBitrate->setText(QString("%1 kbps").arg(value * 8));
}
