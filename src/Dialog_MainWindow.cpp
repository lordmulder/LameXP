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
#include "Model_Settings.h"
#include "Model_FileList.h"
#include "Encoder_MP3.h"

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
#include <QDragEnterEvent>
#include <QWindowsMime>
#include <QProcess>
#include <QRegExp>

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

//Helper class
class Index: public QObjectUserData
{
public:
	Index(int index) { m_index = index; }
	int value(void) { return m_index; }
private:
	int m_index;
};

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

MainWindow::MainWindow(FileListModel *fileListModel, AudioFileModel *metaInfo, SettingsModel *settingsModel, QWidget *parent)
:
	QMainWindow(parent),
	m_fileListModel(fileListModel),
	m_metaData(metaInfo),
	m_settings(settingsModel),
	m_accepted(false)
{
	//Init the dialog, from the .ui file
	setupUi(this);
	setWindowFlags(windowFlags() ^ Qt::WindowMaximizeButtonHint);
	
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
	connect(buttonQuit, SIGNAL(clicked()), this, SLOT(closeButtonClicked()));

	//Setup tab widget
	tabWidget->setCurrentIndex(0);
	connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabPageChanged(int)));

	//Setup "Source" tab
	sourceFileView->setModel(m_fileListModel);
	sourceFileView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	sourceFileView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	m_dropNoteLabel = new QLabel(sourceFileView);
	m_dropNoteLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	m_dropNoteLabel->setText("» You can drop in audio files here! «");
	connect(buttonAddFiles, SIGNAL(clicked()), this, SLOT(addFilesButtonClicked()));
	connect(buttonRemoveFile, SIGNAL(clicked()), this, SLOT(removeFileButtonClicked()));
	connect(buttonClearFiles, SIGNAL(clicked()), this, SLOT(clearFilesButtonClicked()));
	connect(buttonFileUp, SIGNAL(clicked()), this, SLOT(fileUpButtonClicked()));
	connect(buttonFileDown, SIGNAL(clicked()), this, SLOT(fileDownButtonClicked()));
	connect(buttonShowDetails, SIGNAL(clicked()), this, SLOT(showDetailsButtonClicked()));
	connect(m_fileListModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(rowsChanged(QModelIndex,int,int)));
	connect(m_fileListModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(rowsChanged(QModelIndex,int,int)));
	connect(m_fileListModel, SIGNAL(modelReset()), this, SLOT(modelReset()));
	
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
	outputFolderView->setCurrentIndex(m_fileSystemModel->index(m_settings->outputDir()));
	outputFolderViewClicked(outputFolderView->currentIndex());
	connect(buttonMakeFolder, SIGNAL(clicked()), this, SLOT(makeFolderButtonClicked()));
	connect(buttonGotoHome, SIGNAL(clicked()), SLOT(gotoHomeFolderButtonClicked()));
	connect(buttonGotoDesktop, SIGNAL(clicked()), this, SLOT(gotoDesktopButtonClicked()));
	connect(buttonGotoMusic, SIGNAL(clicked()), this, SLOT(gotoMusicFolderButtonClicked()));
	
	//Setup "Meta Data" tab
	m_metaInfoModel = new MetaInfoModel(m_metaData, 6);
	m_metaInfoModel->clearData();
	metaDataView->setModel(m_metaInfoModel);
	metaDataView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	metaDataView->verticalHeader()->hide();
	metaDataView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	connect(buttonEditMeta, SIGNAL(clicked()), this, SLOT(editMetaButtonClicked()));
	connect(buttonClearMeta, SIGNAL(clicked()), this, SLOT(clearMetaButtonClicked()));
	
	//Setup "Compression" tab
	m_encoderButtonGroup = new QButtonGroup(this);
	m_encoderButtonGroup->addButton(radioButtonEncoderMP3, SettingsModel::MP3Encoder);
	m_encoderButtonGroup->addButton(radioButtonEncoderVorbis, SettingsModel::VorbisEncoder);
	m_encoderButtonGroup->addButton(radioButtonEncoderAAC, SettingsModel::AACEncoder);
	m_encoderButtonGroup->addButton(radioButtonEncoderFLAC, SettingsModel::FLACEncoder);
	m_encoderButtonGroup->addButton(radioButtonEncoderPCM, SettingsModel::PCMEncoder);
	m_modeButtonGroup = new QButtonGroup(this);
	m_modeButtonGroup->addButton(radioButtonModeQuality, SettingsModel::VBRMode);
	m_modeButtonGroup->addButton(radioButtonModeAverageBitrate, SettingsModel::ABRMode);
	m_modeButtonGroup->addButton(radioButtonConstBitrate, SettingsModel::CBRMode);
	radioButtonEncoderMP3->setChecked(m_settings->compressionEncoder() == SettingsModel::MP3Encoder);
	radioButtonEncoderVorbis->setChecked(m_settings->compressionEncoder() == SettingsModel::VorbisEncoder);
	radioButtonEncoderAAC->setChecked(m_settings->compressionEncoder() == SettingsModel::AACEncoder);
	radioButtonEncoderFLAC->setChecked(m_settings->compressionEncoder() == SettingsModel::FLACEncoder);
	radioButtonEncoderPCM->setChecked(m_settings->compressionEncoder() == SettingsModel::PCMEncoder);
	radioButtonModeQuality->setChecked(m_settings->compressionRCMode() == SettingsModel::VBRMode);
	radioButtonModeAverageBitrate->setChecked(m_settings->compressionRCMode() == SettingsModel::ABRMode);
	radioButtonConstBitrate->setChecked(m_settings->compressionRCMode() == SettingsModel::CBRMode);
	sliderBitrate->setValue(m_settings->compressionBitrate());
	connect(m_encoderButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(updateEncoder(int)));
	connect(m_modeButtonGroup, SIGNAL(buttonClicked(int)), this, SLOT(updateRCMode(int)));
	connect(sliderBitrate, SIGNAL(valueChanged(int)), this, SLOT(updateBitrate(int)));
	updateEncoder(m_encoderButtonGroup->checkedId());

	//Activate file menu actions
	connect(actionOpenFolder, SIGNAL(triggered()), this, SLOT(openFolderActionActivated()));

	//Activate view menu actions
	m_tabActionGroup = new QActionGroup(this);
	m_tabActionGroup->addAction(actionSourceFiles);
	m_tabActionGroup->addAction(actionOutputDirectory);
	m_tabActionGroup->addAction(actionCompression);
	m_tabActionGroup->addAction(actionMetaData);
	m_tabActionGroup->addAction(actionAdvancedOptions);
	actionSourceFiles->setUserData(0, new Index(0));
	actionOutputDirectory->setUserData(0, new Index(1));
	actionMetaData->setUserData(0, new Index(2));
	actionCompression->setUserData(0, new Index(3));
	actionAdvancedOptions->setUserData(0, new Index(4));
	actionSourceFiles->setChecked(true);
	connect(m_tabActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(tabActionActivated(QAction*)));

	//Activate style menu actions
	m_styleActionGroup = new QActionGroup(this);
	m_styleActionGroup->addAction(actionStylePlastique);
	m_styleActionGroup->addAction(actionStyleCleanlooks);
	m_styleActionGroup->addAction(actionStyleWindowsVista);
	m_styleActionGroup->addAction(actionStyleWindowsXP);
	m_styleActionGroup->addAction(actionStyleWindowsClassic);
	actionStylePlastique->setUserData(0, new Index(0));
	actionStyleCleanlooks->setUserData(0, new Index(1));
	actionStyleWindowsVista->setUserData(0, new Index(2));
	actionStyleWindowsXP->setUserData(0, new Index(3));
	actionStyleWindowsClassic->setUserData(0, new Index(4));
	actionStylePlastique->setChecked(true);
	actionStyleWindowsXP->setEnabled((QSysInfo::windowsVersion() & QSysInfo::WV_NT_based) >= QSysInfo::WV_XP);
	actionStyleWindowsVista->setEnabled((QSysInfo::windowsVersion() & QSysInfo::WV_NT_based) >= QSysInfo::WV_VISTA);
	connect(m_styleActionGroup, SIGNAL(triggered(QAction*)), this, SLOT(styleActionActivated(QAction*)));
	styleActionActivated(NULL);

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

	//Enable Drag & Drop
	this->setAcceptDrops(true);
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
	LAMEXP_DELETE(m_banner);
	LAMEXP_DELETE(m_fileSystemModel);
	LAMEXP_DELETE(m_messageHandler);
	LAMEXP_DELETE(m_delayedFileList);
	LAMEXP_DELETE(m_delayedFileTimer);
	LAMEXP_DELETE(m_metaInfoModel);
	LAMEXP_DELETE(m_encoderButtonGroup);
	LAMEXP_DELETE(m_encoderButtonGroup);
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
	m_accepted = false;
	m_dropNoteLabel->setGeometry(0, 0, sourceFileView->width(), sourceFileView->height());
	modelReset();
	QTimer::singleShot(0, this, SLOT(windowShown()));
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
	QStringList formats = event->mimeData()->formats();
	
	if(formats.contains("application/x-qt-windows-mime;value=\"FileNameW\"", Qt::CaseInsensitive) && formats.contains("text/uri-list", Qt::CaseInsensitive))
	{
		event->acceptProposedAction();
	}
}

void MainWindow::dropEvent(QDropEvent *event)
{
	ABORT_IF_BUSY;

	QStringList droppedFiles;
	const QList<QUrl> urls = event->mimeData()->urls();

	for(int i = 0; i < urls.count(); i++)
	{
		QFileInfo file(urls.at(i).toLocalFile());
		if(!file.exists())
		{
			continue;
		}
		if(file.isFile())
		{
			droppedFiles << file.canonicalFilePath();
			continue;
		}
		if(file.isDir())
		{
			QList<QFileInfo> list = QDir(file.canonicalFilePath()).entryInfoList(QDir::Files);
			for(int j = 0; j < list.count(); j++)
			{
				droppedFiles << list.at(j).canonicalFilePath();
			}
		}
	}
	
	addFiles(droppedFiles);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if(m_banner->isVisible() || m_delayedFileTimer->isActive())
	{
		MessageBeep(MB_ICONEXCLAMATION);
		event->ignore();
	}
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
	QMainWindow::resizeEvent(event);
	m_dropNoteLabel->setGeometry(0, 0, sourceFileView->width(), sourceFileView->height());
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

	if(m_settings->licenseAccepted() <= 0)
	{
		int iAccepted = -1;

		if(m_settings->licenseAccepted() == 0)
		{
			AboutDialog *about = new AboutDialog(this, true);
			iAccepted = about->exec();
			LAMEXP_DELETE(about);
		}

		if(iAccepted <= 0)
		{
			m_settings->licenseAccepted(-1);
			QMessageBox::critical(this, "License Declined", "You have declined the license. Consequently the application will exit now!");
			QApplication::quit();
			return;
		}

		m_settings->licenseAccepted(1);
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
	}
	else
	{
		radioButtonEncoderAAC->setEnabled(false);
		QString messageText;
		messageText += "<nobr>The Nero AAC encoder could not be found. AAC encoding support will be disabled.<br>";
		messageText += "Please put 'neroAacEnc.exe', 'neroAacDec.exe' and 'neroAacTag.exe' into the LameXP directory!<br><br>";
		messageText += "You can download the Nero AAC encoder for free from the official Nero website at:<br>";
		messageText += "<b>" + LINK(AboutDialog::neroAacUrl) + "</b><br></nobr>";
		QMessageBox::information(this, "AAC Support Disabled", messageText);
	}
	
	//Add files from the command-line
	for(int i = 0; i < arguments.count() - 1; i++)
	{
		if(!arguments[i].compare("--add", Qt::CaseInsensitive))
		{
			QFileInfo currentFile(arguments[++i].trimmed());
			qDebug("Adding file from CLI: %s", currentFile.canonicalFilePath().toUtf8().constData());
			m_delayedFileList->append(currentFile.canonicalFilePath());
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
	
	if(m_fileListModel->rowCount() < 1)
	{
		QMessageBox::warning(this, "LameXP", "You must add at least one file to the list before proceeding!");
		tabWidget->setCurrentIndex(0);
		return;
	}
	
	if(m_settings->compressionEncoder() != SettingsModel::MP3Encoder)
	{
		QMessageBox::warning(this, "LameXP", "Sorry, only Lame MP3 encoding is supported at the moment!");
		tabWidget->setCurrentIndex(3);
		return;
	}
		
	m_accepted = true;
	close();
}

/*
 * Close button
 */
void MainWindow::closeButtonClicked(void)
{
	ABORT_IF_BUSY;
	close();
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
			fileList << fileInfoList.takeFirst().canonicalFilePath();
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
	QList<QAction*> actions = m_tabActionGroup->actions();
	for(int i = 0; i < actions.count(); i++)
	{
		if(actions.at(i)->userData(0) && dynamic_cast<Index*>(actions.at(i)->userData(0))->value() == idx)
		{
			actions.at(i)->setChecked(true);
		}
	}
}

/*
 * Tab action triggered
 */
void MainWindow::tabActionActivated(QAction *action)
{
	if(action && action->userData(0))
	{
		int index = dynamic_cast<Index*>(action->userData(0))->value();
		tabWidget->setCurrentIndex(index);
	}
}

/*
 * Style action triggered
 */
void MainWindow::styleActionActivated(QAction *action)
{
	if(action && action->userData(0))
	{
		m_settings->interfaceStyle(dynamic_cast<Index*>(action->userData(0))->value());
	}

	switch(m_settings->interfaceStyle())
	{
	case 1:
		actionStyleCleanlooks->setChecked(true);
		QApplication::setStyle(new QCleanlooksStyle());
		break;
	case 2:
		actionStyleWindowsVista->setChecked(true);
		QApplication::setStyle(new QWindowsVistaStyle());
		break;
	case 3:
		actionStyleWindowsXP->setChecked(true);
		QApplication::setStyle(new QWindowsXPStyle());
		break;
	case 4:
		actionStyleWindowsClassic->setChecked(true);
		QApplication::setStyle(new QWindowsStyle());
		break;
	default:
		actionStylePlastique->setChecked(true);
		QApplication::setStyle(new QPlastiqueStyle());
		break;
	}
}

/*
 * Output folder changed
 */
void MainWindow::outputFolderViewClicked(const QModelIndex &index)
{
	QString selectedDir = m_fileSystemModel->filePath(index);
	if(selectedDir.length() < 3) selectedDir.append(QDir::separator());
	outputFolderLabel->setText(selectedDir);
	m_settings->outputDir(selectedDir);
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
				outputFolderView->setCurrentIndex(m_fileSystemModel->index(createdDir.canonicalPath()));
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
		QFileInfo currentFile = QFileInfo(m_delayedFileList->takeFirst());
		if(!currentFile.exists())
		{
			continue;
		}
		if(currentFile.isFile())
		{
			selectedFiles << currentFile.canonicalFilePath();
			continue;
		}
		if(currentFile.isDir())
		{
			QList<QFileInfo> list = QDir(currentFile.canonicalFilePath()).entryInfoList(QDir::Files);
			for(int j = 0; j < list.count(); j++)
			{
				selectedFiles << list.at(j).canonicalFilePath();
			}
		}
	}
	
	addFiles(selectedFiles);
}

/*
 * Update encoder
 */
void MainWindow::updateEncoder(int id)
{
	m_settings->compressionEncoder(id);

	switch(m_settings->compressionEncoder())
	{
	case SettingsModel::VorbisEncoder:
		radioButtonModeQuality->setEnabled(true);
		radioButtonModeAverageBitrate->setEnabled(true);
		radioButtonConstBitrate->setEnabled(false);
		if(radioButtonConstBitrate->isChecked()) radioButtonModeQuality->setChecked(true);
		sliderBitrate->setEnabled(true);
		break;
	case SettingsModel::FLACEncoder:
		radioButtonModeQuality->setEnabled(false);
		radioButtonModeQuality->setChecked(true);
		radioButtonModeAverageBitrate->setEnabled(false);
		radioButtonConstBitrate->setEnabled(false);
		sliderBitrate->setEnabled(true);
		break;
	case SettingsModel::PCMEncoder:
		radioButtonModeQuality->setEnabled(false);
		radioButtonModeQuality->setChecked(true);
		radioButtonModeAverageBitrate->setEnabled(false);
		radioButtonConstBitrate->setEnabled(false);
		sliderBitrate->setEnabled(false);
		break;
	default:
		radioButtonModeQuality->setEnabled(true);
		radioButtonModeAverageBitrate->setEnabled(true);
		radioButtonConstBitrate->setEnabled(true);
		sliderBitrate->setEnabled(true);
		break;
	}

	updateRCMode(m_modeButtonGroup->checkedId());
}

/*
 * Update rate-control mode
 */
void MainWindow::updateRCMode(int id)
{
	m_settings->compressionRCMode(id);

	switch(m_settings->compressionEncoder())
	{
	case SettingsModel::MP3Encoder:
		switch(m_settings->compressionRCMode())
		{
		case SettingsModel::VBRMode:
			sliderBitrate->setMinimum(0);
			sliderBitrate->setMaximum(9);
			break;
		default:
			sliderBitrate->setMinimum(0);
			sliderBitrate->setMaximum(13);
			break;
		}
		break;
	case SettingsModel::VorbisEncoder:
		switch(m_settings->compressionRCMode())
		{
		case SettingsModel::VBRMode:
			sliderBitrate->setMinimum(-2);
			sliderBitrate->setMaximum(10);
			break;
		default:
			sliderBitrate->setMinimum(4);
			sliderBitrate->setMaximum(63);
			break;
		}
		break;
	case SettingsModel::AACEncoder:
		switch(m_settings->compressionRCMode())
		{
		case SettingsModel::VBRMode:
			sliderBitrate->setMinimum(0);
			sliderBitrate->setMaximum(20);
			break;
		default:
			sliderBitrate->setMinimum(4);
			sliderBitrate->setMaximum(63);
			break;
		}
		break;
	case SettingsModel::FLACEncoder:
		sliderBitrate->setMinimum(0);
		sliderBitrate->setMaximum(8);
		break;
	case SettingsModel::PCMEncoder:
		sliderBitrate->setMinimum(0);
		sliderBitrate->setMaximum(2);
		sliderBitrate->setValue(1);
		break;
	default:
		sliderBitrate->setMinimum(0);
		sliderBitrate->setMaximum(0);
		break;
	}

	updateBitrate(sliderBitrate->value());
}

/*
 * Update bitrate
 */
void MainWindow::updateBitrate(int value)
{
	m_settings->compressionBitrate(value);
	
	switch(m_settings->compressionRCMode())
	{
	case SettingsModel::VBRMode:
		switch(m_settings->compressionEncoder())
		{
		case SettingsModel::MP3Encoder:
			labelBitrate->setText(QString("Quality Level %1").arg(9 - value));
			break;
		case SettingsModel::VorbisEncoder:
			labelBitrate->setText(QString("Quality Level %1").arg(value));
			break;
		case SettingsModel::AACEncoder:
			labelBitrate->setText(QString("Quality Level %1").arg(QString().sprintf("%.2f", static_cast<double>(value * 5) / 100.0)));
			break;
		case SettingsModel::FLACEncoder:
			labelBitrate->setText(QString("Compression %1").arg(value));
			break;
		case SettingsModel::PCMEncoder:
			labelBitrate->setText("Uncompressed");
			break;
		default:
			labelBitrate->setText(QString::number(value));
			break;
		}
		break;
	case SettingsModel::ABRMode:
		switch(m_settings->compressionEncoder())
		{
		case SettingsModel::MP3Encoder:
			labelBitrate->setText(QString("&asymp; %1 kbps").arg(SettingsModel::mp3Bitrates[value]));
			break;
		case SettingsModel::FLACEncoder:
			labelBitrate->setText(QString("Compression %1").arg(value));
			break;
		case SettingsModel::PCMEncoder:
			labelBitrate->setText("Uncompressed");
			break;
		default:
			labelBitrate->setText(QString("&asymp; %1 kbps").arg(min(500, value * 8)));
			break;
		}
		break;
	default:
		switch(m_settings->compressionEncoder())
		{
		case SettingsModel::MP3Encoder:
			labelBitrate->setText(QString("%1 kbps").arg(SettingsModel::mp3Bitrates[value]));
			break;
		case SettingsModel::FLACEncoder:
			labelBitrate->setText(QString("Compression %1").arg(value));
			break;
		case SettingsModel::PCMEncoder:
			labelBitrate->setText("Uncompressed");
			break;
		default:
			labelBitrate->setText(QString("%1 kbps").arg(min(500, value * 8)));
			break;
		}
		break;
	}
}

/*
 * Rows changed
 */
void MainWindow::rowsChanged(const QModelIndex &parent, int start, int end)
{
	m_dropNoteLabel->setVisible(m_fileListModel->rowCount() <= 0);
}

/*
 * Model reset
 */
void MainWindow::modelReset(void)
{
	m_dropNoteLabel->setVisible(m_fileListModel->rowCount() <= 0);
}
