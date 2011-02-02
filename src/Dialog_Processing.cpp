///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2011 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Dialog_Processing.h"

#include "Global.h"
#include "Resource.h"
#include "Model_FileList.h"
#include "Model_Progress.h"
#include "Model_Settings.h"
#include "Thread_Process.h"
#include "Dialog_LogView.h"
#include "Encoder_MP3.h"
#include "Encoder_Vorbis.h"
#include "Encoder_AAC.h"
#include "Encoder_FLAC.h"
#include "Encoder_Wave.h"
#include "Filter_Normalize.h"
#include "Filter_Resample.h"
#include "Filter_ToneAdjust.h"
#include "WinSevenTaskbar.h"

#include <QApplication>
#include <QRect>
#include <QDesktopWidget>
#include <QMovie>
#include <QMessageBox>
#include <QTimer>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <QUuid>
#include <QFileInfo>
#include <QDir>
#include <QMenu>
#include <QSystemTrayIcon>

#include <Windows.h>

#define CHANGE_BACKGROUND_COLOR(WIDGET, COLOR) \
{ \
	QPalette palette = WIDGET->palette(); \
	palette.setColor(QPalette::Background, COLOR); \
	WIDGET->setPalette(palette); \
}

#define SET_PROGRESS_TEXT(TXT) \
{ \
	label_progress->setText(TXT); \
	m_systemTray->setToolTip(QString().sprintf("LameXP v%d.%02d\n%ls", lamexp_version_major(), lamexp_version_minor(), QString(TXT).utf16())); \
}

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

ProcessingDialog::ProcessingDialog(FileListModel *fileListModel, AudioFileModel *metaInfo, SettingsModel *settings, QWidget *parent)
:
	QDialog(parent),
	m_systemTray(new QSystemTrayIcon(QIcon(":/icons/cd_go.png"), this)),
	m_settings(settings),
	m_metaInfo(metaInfo)
{
	//Init the dialog, from the .ui file
	setupUi(this);
	setWindowFlags(windowFlags() ^ Qt::WindowContextHelpButtonHint);
	
	//Setup version info
	label_versionInfo->setText(QString().sprintf("v%d.%02d %s (Build %d)", lamexp_version_major(), lamexp_version_minor(), lamexp_version_release(), lamexp_version_build()));
	label_versionInfo->installEventFilter(this);

	//Register meta type
	qRegisterMetaType<QUuid>("QUuid");

	//Center window in screen
	QRect desktopRect = QApplication::desktop()->screenGeometry();
	QRect thisRect = this->geometry();
	move((desktopRect.width() - thisRect.width()) / 2, (desktopRect.height() - thisRect.height()) / 2);
	setMinimumSize(thisRect.width(), thisRect.height());

	//Enable buttons
	connect(button_AbortProcess, SIGNAL(clicked()), this, SLOT(abortEncoding()));
	
	//Init progress indicator
	m_progressIndicator = new QMovie(":/images/Working.gif");
	label_headerWorking->setMovie(m_progressIndicator);
	progressBar->setValue(0);

	//Init progress model
	m_progressModel = new ProgressModel();
	view_log->setModel(m_progressModel);
	view_log->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	view_log->verticalHeader()->hide();
	view_log->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	view_log->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
	connect(m_progressModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(progressModelChanged()));
	connect(m_progressModel, SIGNAL(modelReset()), this, SLOT(progressModelChanged()));
	connect(view_log, SIGNAL(activated(QModelIndex)), this, SLOT(logViewDoubleClicked(QModelIndex)));

	//Create context menu
	m_contextMenu = new QMenu();
	QAction *contextMenuAction = m_contextMenu->addAction(QIcon(":/icons/zoom.png"), tr("Show details for selected job"));
	view_log->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(view_log, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuTriggered(QPoint)));
	connect(contextMenuAction, SIGNAL(triggered(bool)), this, SLOT(contextMenuActionTriggered()));
	
	//Enque jobs
	if(fileListModel)
	{
		for(int i = 0; i < fileListModel->rowCount(); i++)
		{
			m_pendingJobs.append(fileListModel->getFile(fileListModel->index(i,0)));
		}
	}

	//Translate
	label_headerStatus->setText(QString("<b>%1</b><br>%2").arg(tr("Encoding Files"), tr("Your files are being encoded, please be patient...")));
	
	//Enable system tray icon
	connect(m_systemTray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(systemTrayActivated(QSystemTrayIcon::ActivationReason)));

	//Init other vars
	m_runningThreads = 0;
	m_currentFile = 0;
	m_allJobs.clear();
	m_succeededJobs.clear();
	m_failedJobs.clear();
	m_userAborted = false;
}

////////////////////////////////////////////////////////////
// Destructor
////////////////////////////////////////////////////////////

ProcessingDialog::~ProcessingDialog(void)
{
	view_log->setModel(NULL);
	if(m_progressIndicator) m_progressIndicator->stop();
	LAMEXP_DELETE(m_progressIndicator);
	LAMEXP_DELETE(m_progressModel);
	LAMEXP_DELETE(m_contextMenu);
	LAMEXP_DELETE(m_systemTray);

	WinSevenTaskbar::setOverlayIcon(this, NULL);
	WinSevenTaskbar::setTaskbarState(this, WinSevenTaskbar::WinSevenTaskbarNoState);

	while(!m_threadList.isEmpty())
	{
		ProcessThread *thread = m_threadList.takeFirst();
		thread->terminate();
		thread->wait(15000);
		delete thread;
	}
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

void ProcessingDialog::showEvent(QShowEvent *event)
{
	setCloseButtonEnabled(false);
	button_closeDialog->setEnabled(false);
	button_AbortProcess->setEnabled(false);
	m_systemTray->setVisible(true);
	
	if(!SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS))
	{
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	}

	QTimer::singleShot(1000, this, SLOT(initEncoding()));
}

void ProcessingDialog::closeEvent(QCloseEvent *event)
{
	if(!button_closeDialog->isEnabled())
	{
		event->ignore();
	}
	else
	{
		m_systemTray->setVisible(false);
	}
}

bool ProcessingDialog::eventFilter(QObject *obj, QEvent *event)
{
	static QColor defaultColor = QColor();

	if(obj == label_versionInfo)
	{
		if(event->type() == QEvent::Enter)
		{
			QPalette palette = label_versionInfo->palette();
			defaultColor = palette.color(QPalette::Normal, QPalette::WindowText);
			palette.setColor(QPalette::Normal, QPalette::WindowText, Qt::red);
			label_versionInfo->setPalette(palette);
		}
		else if(event->type() == QEvent::Leave)
		{
			QPalette palette = label_versionInfo->palette();
			palette.setColor(QPalette::Normal, QPalette::WindowText, defaultColor);
			label_versionInfo->setPalette(palette);
		}
		else if(event->type() == QEvent::MouseButtonPress)
		{
			QUrl url("http://mulder.dummwiedeutsch.de/");
			QDesktopServices::openUrl(url);
		}
	}

	return false;
}

////////////////////////////////////////////////////////////
// SLOTS
////////////////////////////////////////////////////////////

void ProcessingDialog::initEncoding(void)
{
	m_runningThreads = 0;
	m_currentFile = 0;
	m_allJobs.clear();
	m_succeededJobs.clear();
	m_failedJobs.clear();
	m_userAborted = false;
	m_playList.clear();
	
	CHANGE_BACKGROUND_COLOR(frame_header, QColor(Qt::white));
	SET_PROGRESS_TEXT(tr("Encoding files, please wait..."));
	m_progressIndicator->start();
	
	button_closeDialog->setEnabled(false);
	button_AbortProcess->setEnabled(true);
	progressBar->setRange(0, m_pendingJobs.count());

	WinSevenTaskbar::initTaskbar();
	WinSevenTaskbar::setTaskbarState(this, WinSevenTaskbar::WinSevenTaskbarNormalState);
	WinSevenTaskbar::setTaskbarProgress(this, 0, m_pendingJobs.count());
	WinSevenTaskbar::setOverlayIcon(this, &QIcon(":/icons/control_play_blue.png"));

	lamexp_cpu_t cpuFeatures = lamexp_detect_cpu_features();
	int parallelThreadCount = max(min(min(cpuFeatures.count, m_pendingJobs.count()), 4), 1);

	if(parallelThreadCount > 1)
	{
		m_progressModel->addSystemMessage(tr("Multi-threading enabled: Running %1 instances in parallel!").arg(QString::number(parallelThreadCount)));
	}

	for(int i = 0; i < parallelThreadCount; i++)
	{
		startNextJob();
	}
}

void ProcessingDialog::abortEncoding(void)
{
	m_userAborted = true;
	button_AbortProcess->setEnabled(false);
	
	SET_PROGRESS_TEXT(tr("Aborted! Waiting for running jobs to terminate..."));

	for(int i = 0; i < m_threadList.count(); i++)
	{
		m_threadList.at(i)->abort();
	}
}

void ProcessingDialog::doneEncoding(void)
{
	m_runningThreads--;
	progressBar->setValue(progressBar->value() + 1);
	
	if(!m_userAborted)
	{
		SET_PROGRESS_TEXT(tr("Encoding: %1 files of %2 completed so far, please wait...").arg(QString::number(progressBar->value()), QString::number(progressBar->maximum())));
		WinSevenTaskbar::setTaskbarProgress(this, progressBar->value(), progressBar->maximum());
	}
	
	int index = m_threadList.indexOf(dynamic_cast<ProcessThread*>(QWidget::sender()));
	if(index >= 0)
	{
		m_threadList.takeAt(index)->deleteLater();
	}

	if(!m_pendingJobs.isEmpty() && !m_userAborted)
	{
		startNextJob();
		qDebug("Running jobs: %u", m_runningThreads);
		return;
	}
	
	if(m_runningThreads > 0)
	{
		qDebug("Running jobs: %u", m_runningThreads);
		return;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);
	qDebug("Running jobs: %u", m_runningThreads);

	if(!m_userAborted && m_settings->createPlaylist() && !m_settings->outputToSourceDir())
	{
		SET_PROGRESS_TEXT(tr("Creating the playlist file, please wait..."));
		QApplication::processEvents();
		writePlayList();
	}
	
	if(m_userAborted)
	{
		CHANGE_BACKGROUND_COLOR(frame_header, QColor("#FFF3BA"));
		WinSevenTaskbar::setTaskbarState(this, WinSevenTaskbar::WinSevenTaskbarErrorState);
		WinSevenTaskbar::setOverlayIcon(this, &QIcon(":/icons/error.png"));
		SET_PROGRESS_TEXT((m_succeededJobs.count() > 0) ? tr("Process was aborted by the user after %1 file(s)!").arg(QString::number(m_succeededJobs.count())) : tr("Process was aborted prematurely by the user!"));
		m_systemTray->showMessage(tr("LameXP - Aborted"), tr("Process was aborted by the user."), QSystemTrayIcon::Warning);
		m_systemTray->setIcon(QIcon(":/icons/cd_delete.png"));
		QApplication::processEvents();
		if(m_settings->soundsEnabled()) PlaySound(MAKEINTRESOURCE(IDR_WAVE_ABORTED), GetModuleHandle(NULL), SND_RESOURCE | SND_SYNC);
	}
	else
	{
		if(m_failedJobs.count() > 0)
		{
			CHANGE_BACKGROUND_COLOR(frame_header, QColor("#FFBABA"));
			WinSevenTaskbar::setTaskbarState(this, WinSevenTaskbar::WinSevenTaskbarErrorState);
			WinSevenTaskbar::setOverlayIcon(this, &QIcon(":/icons/exclamation.png"));
			SET_PROGRESS_TEXT(tr("Error: %1 of %2 files failed. Double-click failed items for detailed information!").arg(QString::number(m_failedJobs.count()), QString::number(m_failedJobs.count() + m_succeededJobs.count())));
			m_systemTray->showMessage(tr("LameXP - Error"), tr("At least one file has failed!"), QSystemTrayIcon::Critical);
			m_systemTray->setIcon(QIcon(":/icons/cd_delete.png"));
			QApplication::processEvents();
			if(m_settings->soundsEnabled()) PlaySound(MAKEINTRESOURCE(IDR_WAVE_ERROR), GetModuleHandle(NULL), SND_RESOURCE | SND_SYNC);
		}
		else
		{
			CHANGE_BACKGROUND_COLOR(frame_header, QColor("#E0FFE2"));
			WinSevenTaskbar::setTaskbarState(this, WinSevenTaskbar::WinSevenTaskbarNormalState);
			WinSevenTaskbar::setOverlayIcon(this, &QIcon(":/icons/accept.png"));
			SET_PROGRESS_TEXT(tr("Alle files completed successfully."));
			m_systemTray->showMessage(tr("LameXP - Done"), tr("All files completed successfully."), QSystemTrayIcon::Information);
			m_systemTray->setIcon(QIcon(":/icons/cd_add.png"));
			QApplication::processEvents();
			if(m_settings->soundsEnabled()) PlaySound(MAKEINTRESOURCE(IDR_WAVE_SUCCESS), GetModuleHandle(NULL), SND_RESOURCE | SND_SYNC);
		}
	}
	
	setCloseButtonEnabled(true);
	button_closeDialog->setEnabled(true);
	button_AbortProcess->setEnabled(false);

	view_log->scrollToBottom();
	m_progressIndicator->stop();
	progressBar->setValue(progressBar->maximum());
	WinSevenTaskbar::setTaskbarProgress(this, progressBar->value(), progressBar->maximum());

	QApplication::restoreOverrideCursor();
}

void ProcessingDialog::processFinished(const QUuid &jobId, const QString &outFileName, bool success)
{
	if(success)
	{
		m_playList.insert(jobId, outFileName);
		m_succeededJobs.append(jobId);
	}
	else
	{
		m_failedJobs.append(jobId);
	}
}

void ProcessingDialog::progressModelChanged(void)
{
	view_log->scrollToBottom();
}

void ProcessingDialog::logViewDoubleClicked(const QModelIndex &index)
{
	if(m_runningThreads == 0)
	{
		const QStringList &logFile = m_progressModel->getLogFile(index);
		
		if(!logFile.isEmpty())
		{
			LogViewDialog *logView = new LogViewDialog(this);
			logView->setWindowTitle(QString("LameXP - [%1]").arg(m_progressModel->data(index, Qt::DisplayRole).toString()));
			logView->exec(logFile);
			LAMEXP_DELETE(logView);
		}
		else
		{
			MessageBeep(MB_ICONWARNING);
		}
	}
	else
	{
		MessageBeep(MB_ICONWARNING);
	}
}

void ProcessingDialog::contextMenuTriggered(const QPoint &pos)
{
	if(pos.x() <= view_log->width() && pos.y() <= view_log->height() && pos.x() >= 0 && pos.y() >= 0)
	{
		m_contextMenu->popup(view_log->mapToGlobal(pos));
	}
}

void ProcessingDialog::contextMenuActionTriggered(void)
{
	QModelIndex index = view_log->indexAt(view_log->mapFromGlobal(m_contextMenu->pos()));
	logViewDoubleClicked(index.isValid() ? index : view_log->currentIndex());
}

////////////////////////////////////////////////////////////
// Private Functions
////////////////////////////////////////////////////////////

void ProcessingDialog::startNextJob(void)
{
	if(m_pendingJobs.isEmpty())
	{
		return;
	}
	
	m_currentFile++;
	AudioFileModel currentFile = updateMetaInfo(m_pendingJobs.takeFirst());
	AbstractEncoder *encoder = NULL;
	bool nativeResampling = false;

	switch(m_settings->compressionEncoder())
	{
	case SettingsModel::MP3Encoder:
		{
			MP3Encoder *mp3Encoder = new MP3Encoder();
			mp3Encoder->setBitrate(m_settings->compressionBitrate());
			mp3Encoder->setRCMode(m_settings->compressionRCMode());
			mp3Encoder->setAlgoQuality(m_settings->lameAlgoQuality());
			if(m_settings->bitrateManagementEnabled())
			{
				mp3Encoder->setBitrateLimits(m_settings->bitrateManagementMinRate(), m_settings->bitrateManagementMaxRate());
			}
			if(m_settings->samplingRate() > 0)
			{
				mp3Encoder->setSamplingRate(SettingsModel::samplingRates[m_settings->samplingRate()]);
				nativeResampling = true;
			}
			mp3Encoder->setChannelMode(m_settings->lameChannelMode());
			encoder = mp3Encoder;
		}
		break;
	case SettingsModel::VorbisEncoder:
		{
			VorbisEncoder *vorbisEncoder = new VorbisEncoder();
			vorbisEncoder->setBitrate(m_settings->compressionBitrate());
			vorbisEncoder->setRCMode(m_settings->compressionRCMode());
			if(m_settings->bitrateManagementEnabled())
			{
				vorbisEncoder->setBitrateLimits(m_settings->bitrateManagementMinRate(), m_settings->bitrateManagementMaxRate());
			}
			if(m_settings->samplingRate() > 0)
			{
				vorbisEncoder->setSamplingRate(SettingsModel::samplingRates[m_settings->samplingRate()]);
				nativeResampling = true;
			}
			encoder = vorbisEncoder;
		}
		break;
	case SettingsModel::AACEncoder:
		{
			AACEncoder *aacEncoder = new AACEncoder();
			aacEncoder->setBitrate(m_settings->compressionBitrate());
			aacEncoder->setRCMode(m_settings->compressionRCMode());
			aacEncoder->setEnable2Pass(m_settings->neroAACEnable2Pass());
			aacEncoder->setProfile(m_settings->neroAACProfile());
			encoder = aacEncoder;
		}
		break;
	case SettingsModel::FLACEncoder:
		{
			FLACEncoder *flacEncoder = new FLACEncoder();
			flacEncoder->setBitrate(m_settings->compressionBitrate());
			flacEncoder->setRCMode(m_settings->compressionRCMode());
			encoder = flacEncoder;
		}
		break;
	case SettingsModel::PCMEncoder:
		{
			WaveEncoder *waveEncoder = new WaveEncoder();
			waveEncoder->setBitrate(m_settings->compressionBitrate());
			waveEncoder->setRCMode(m_settings->compressionRCMode());
			encoder = waveEncoder;
		}
		break;
	default:
		throw "Unsupported encoder!";
	}

	ProcessThread *thread = new ProcessThread
	(
		currentFile,
		(m_settings->outputToSourceDir() ? QFileInfo(currentFile.filePath()).absolutePath(): m_settings->outputDir()),
		encoder,
		m_settings->prependRelativeSourcePath()
	);

	if((m_settings->samplingRate() > 0) && !nativeResampling)
	{
		thread->addFilter(new ResampleFilter(SettingsModel::samplingRates[m_settings->samplingRate()]));
	}
	if((m_settings->toneAdjustBass() != 0) || (m_settings->toneAdjustTreble() != 0))
	{
		thread->addFilter(new ToneAdjustFilter(m_settings->toneAdjustBass(), m_settings->toneAdjustTreble()));
	}
	if(m_settings->normalizationFilterEnabled())
	{
		thread->addFilter(new NormalizeFilter(m_settings->normalizationFilterMaxVolume()));
	}

	m_threadList.append(thread);
	m_allJobs.append(thread->getId());
	
	connect(thread, SIGNAL(finished()), this, SLOT(doneEncoding()), Qt::QueuedConnection);
	connect(thread, SIGNAL(processStateInitialized(QUuid,QString,QString,int)), m_progressModel, SLOT(addJob(QUuid,QString,QString,int)), Qt::QueuedConnection);
	connect(thread, SIGNAL(processStateChanged(QUuid,QString,int)), m_progressModel, SLOT(updateJob(QUuid,QString,int)), Qt::QueuedConnection);
	connect(thread, SIGNAL(processStateFinished(QUuid,QString,bool)), this, SLOT(processFinished(QUuid,QString,bool)), Qt::QueuedConnection);
	connect(thread, SIGNAL(processMessageLogged(QUuid,QString)), m_progressModel, SLOT(appendToLog(QUuid,QString)), Qt::QueuedConnection);
	
	m_runningThreads++;
	thread->start();
}

void ProcessingDialog::writePlayList(void)
{
	if(m_succeededJobs.count() <= 0 || m_allJobs.count() <= 0)
	{
		qWarning("WritePlayList: Nothing to do!");
		return;
	}
	
	QString playListName = (m_metaInfo->fileAlbum().isEmpty() ? "Playlist" : m_metaInfo->fileAlbum());

	const static char *invalidChars = "\\/:*?\"<>|";
	for(int i = 0; invalidChars[i]; i++)
	{
		playListName.replace(invalidChars[i], ' ');
		playListName = playListName.simplified();
	}
	
	QString playListFile = QString("%1/%2.m3u").arg(m_settings->outputDir(), playListName);

	int counter = 1;
	while(QFileInfo(playListFile).exists())
	{
		playListFile = QString("%1/%2 (%3).m3u").arg(m_settings->outputDir(), playListName, QString::number(++counter));
	}

	QFile playList(playListFile);
	if(playList.open(QIODevice::WriteOnly))
	{
		playList.write("#EXTM3U\r\n");
		for(int i = 0; i < m_allJobs.count(); i++)
		{
			
			if(!m_succeededJobs.contains(m_allJobs.at(i))) continue;
			playList.write(QDir::toNativeSeparators(QDir(m_settings->outputDir()).relativeFilePath(m_playList.value(m_allJobs.at(i), "N/A"))).toUtf8().constData());
			playList.write("\r\n");
		}
		playList.close();
	}
	else
	{
		QMessageBox::warning(this, tr("Playlist creation failed"), QString("%1<br><nobr>%2</nobr>").arg(tr("The playlist file could not be created:"), playListFile));
	}
}

AudioFileModel ProcessingDialog::updateMetaInfo(const AudioFileModel &audioFile)
{
	if(!m_settings->writeMetaTags())
	{
		return AudioFileModel(audioFile.filePath());
	}
	
	AudioFileModel result = audioFile;

	if(!m_metaInfo->fileArtist().isEmpty()) result.setFileArtist(m_metaInfo->fileArtist());
	if(!m_metaInfo->fileAlbum().isEmpty()) result.setFileAlbum(m_metaInfo->fileAlbum());
	if(!m_metaInfo->fileGenre().isEmpty()) result.setFileGenre(m_metaInfo->fileGenre());
	if(m_metaInfo->fileYear()) result.setFileYear(m_metaInfo->fileYear());
	if(m_metaInfo->filePosition() == UINT_MAX) result.setFilePosition(m_currentFile);
	if(!m_metaInfo->fileComment().isEmpty()) result.setFileComment(m_metaInfo->fileComment());

	return result;
}

void ProcessingDialog::setCloseButtonEnabled(bool enabled)
{
	HMENU hMenu = GetSystemMenu((HWND) winId(), FALSE);
	EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | (enabled ? MF_ENABLED : MF_GRAYED));
}

void ProcessingDialog::systemTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
	if(reason == QSystemTrayIcon::DoubleClick)
	{
		SetForegroundWindow(this->winId());
	}
}
