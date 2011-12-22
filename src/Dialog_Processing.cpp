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
#include "Thread_CPUObserver.h"
#include "Thread_RAMObserver.h"
#include "Thread_DiskObserver.h"
#include "Dialog_LogView.h"
#include "Encoder_AAC.h"
#include "Encoder_AAC_FHG.h"
#include "Encoder_AAC_QAAC.h"
#include "Encoder_AC3.h"
#include "Encoder_DCA.h"
#include "Encoder_FLAC.h"
#include "Encoder_MP3.h"
#include "Encoder_Vorbis.h"
#include "Encoder_Wave.h"
#include "Filter_Downmix.h"
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
#include <QProcess>
#include <QProgressDialog>
#include <QTime>

#include <MMSystem.h>
#include <math.h>
#include <float.h>

////////////////////////////////////////////////////////////

//Maximum number of parallel instances
#define MAX_INSTANCES 16U

//Function to calculate the number of instances
static int cores2instances(int cores);

////////////////////////////////////////////////////////////

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

#define SET_FONT_BOLD(WIDGET,BOLD) { QFont _font = WIDGET->font(); _font.setBold(BOLD); WIDGET->setFont(_font); }
#define UPDATE_MIN_WIDTH(WIDGET) { if(WIDGET->width() > WIDGET->minimumWidth()) WIDGET->setMinimumWidth(WIDGET->width()); }

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

ProcessingDialog::ProcessingDialog(FileListModel *fileListModel, AudioFileModel *metaInfo, SettingsModel *settings, QWidget *parent)
:
	QDialog(parent),
	m_systemTray(new QSystemTrayIcon(QIcon(":/icons/cd_go.png"), this)),
	m_settings(settings),
	m_metaInfo(metaInfo),
	m_shutdownFlag(shutdownFlag_None),
	m_diskObserver(NULL),
	m_cpuObserver(NULL),
	m_ramObserver(NULL)
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
	m_progressIndicator->setCacheMode(QMovie::CacheAll);
	m_progressIndicator->setSpeed(50);
	label_headerWorking->setMovie(m_progressIndicator);
	progressBar->setValue(0);

	//Init progress model
	m_progressModel = new ProgressModel();
	view_log->setModel(m_progressModel);
	view_log->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	view_log->verticalHeader()->hide();
	view_log->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	view_log->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
	view_log->viewport()->installEventFilter(this);
	connect(m_progressModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(progressModelChanged()));
	connect(m_progressModel, SIGNAL(modelReset()), this, SLOT(progressModelChanged()));
	connect(view_log, SIGNAL(activated(QModelIndex)), this, SLOT(logViewDoubleClicked(QModelIndex)));
	connect(view_log->horizontalHeader(), SIGNAL(sectionResized(int,int,int)), this, SLOT(logViewSectionSizeChanged(int,int,int)));

	//Create context menu
	m_contextMenu = new QMenu();
	QAction *contextMenuDetailsAction = m_contextMenu->addAction(QIcon(":/icons/zoom.png"), tr("Show details for selected job"));
	QAction *contextMenuShowFileAction = m_contextMenu->addAction(QIcon(":/icons/folder_go.png"), tr("Browse Output File Location"));

	view_log->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(view_log, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuTriggered(QPoint)));
	connect(contextMenuDetailsAction, SIGNAL(triggered(bool)), this, SLOT(contextMenuDetailsActionTriggered()));
	connect(contextMenuShowFileAction, SIGNAL(triggered(bool)), this, SLOT(contextMenuShowFileActionTriggered()));
	SET_FONT_BOLD(contextMenuDetailsAction, true);

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
	m_timerStart = 0I64;
}

////////////////////////////////////////////////////////////
// Destructor
////////////////////////////////////////////////////////////

ProcessingDialog::~ProcessingDialog(void)
{
	view_log->setModel(NULL);

	if(m_progressIndicator)
	{
		m_progressIndicator->stop();
	}

	if(m_diskObserver)
	{
		m_diskObserver->stop();
		if(!m_diskObserver->wait(15000))
		{
			m_diskObserver->terminate();
			m_diskObserver->wait();
		}
	}
	if(m_cpuObserver)
	{
		m_cpuObserver->stop();
		if(!m_cpuObserver->wait(15000))
		{
			m_cpuObserver->terminate();
			m_cpuObserver->wait();
		}
	}
	if(m_ramObserver)
	{
		m_ramObserver->stop();
		if(!m_ramObserver->wait(15000))
		{
			m_ramObserver->terminate();
			m_ramObserver->wait();
		}
	}

	LAMEXP_DELETE(m_progressIndicator);
	LAMEXP_DELETE(m_progressModel);
	LAMEXP_DELETE(m_contextMenu);
	LAMEXP_DELETE(m_systemTray);
	LAMEXP_DELETE(m_diskObserver);
	LAMEXP_DELETE(m_cpuObserver);
	LAMEXP_DELETE(m_ramObserver);

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
	static const char *NA = " N/A";

	setCloseButtonEnabled(false);
	button_closeDialog->setEnabled(false);
	button_AbortProcess->setEnabled(false);
	m_systemTray->setVisible(true);
	
	if(!SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS))
	{
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	}

	label_cpu->setText(NA);
	label_disk->setText(NA);
	label_ram->setText(NA);

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
			QUrl url(lamexp_website_url());
			QDesktopServices::openUrl(url);
		}
	}

	return false;
}

bool ProcessingDialog::winEvent(MSG *message, long *result)
{
	return WinSevenTaskbar::handleWinEvent(message, result);
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
	checkBox_shutdownComputer->setEnabled(true);
	checkBox_shutdownComputer->setChecked(false);

	WinSevenTaskbar::setTaskbarState(this, WinSevenTaskbar::WinSevenTaskbarNormalState);
	WinSevenTaskbar::setTaskbarProgress(this, 0, m_pendingJobs.count());
	WinSevenTaskbar::setOverlayIcon(this, &QIcon(":/icons/control_play_blue.png"));

	if(!m_diskObserver)
	{
		m_diskObserver = new DiskObserverThread(m_settings->customTempPathEnabled() ? m_settings->customTempPath() : lamexp_temp_folder2());
		connect(m_diskObserver, SIGNAL(messageLogged(QString,int)), m_progressModel, SLOT(addSystemMessage(QString,int)), Qt::QueuedConnection);
		connect(m_diskObserver, SIGNAL(freeSpaceChanged(quint64)), this, SLOT(diskUsageHasChanged(quint64)), Qt::QueuedConnection);
		m_diskObserver->start();
	}
	if(!m_cpuObserver)
	{
		m_cpuObserver = new CPUObserverThread();
		connect(m_cpuObserver, SIGNAL(currentUsageChanged(double)), this, SLOT(cpuUsageHasChanged(double)), Qt::QueuedConnection);
		m_cpuObserver->start();
	}
	if(!m_ramObserver)
	{
		m_ramObserver = new RAMObserverThread();
		connect(m_ramObserver, SIGNAL(currentUsageChanged(double)), this, SLOT(ramUsageHasChanged(double)), Qt::QueuedConnection);
		m_ramObserver->start();
	}
	
	unsigned int maximumInstances = qBound(0U, m_settings->maximumInstances(), MAX_INSTANCES);
	if(maximumInstances < 1)
	{
		lamexp_cpu_t cpuFeatures = lamexp_detect_cpu_features();
		maximumInstances = cores2instances(qBound(1, cpuFeatures.count, 64));
	}

	maximumInstances = qBound(1U, maximumInstances, static_cast<unsigned int>(m_pendingJobs.count()));
	if(maximumInstances > 1)
	{
		m_progressModel->addSystemMessage(tr("Multi-threading enabled: Running %1 instances in parallel!").arg(QString::number(maximumInstances)));
	}

	for(unsigned int i = 0; i < maximumInstances; i++)
	{
		startNextJob();
	}

	LARGE_INTEGER counter;
	if(QueryPerformanceCounter(&counter))
	{
		m_timerStart = counter.QuadPart;
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
		LARGE_INTEGER counter, frequency;
		if(QueryPerformanceCounter(&counter) && QueryPerformanceFrequency(&frequency))
		{
			if((m_timerStart > 0I64) && (frequency.QuadPart > 0I64) && (m_timerStart < counter.QuadPart))
			{
				double timeElapsed = static_cast<double>(counter.QuadPart - m_timerStart) / static_cast<double>(frequency.QuadPart);
				m_progressModel->addSystemMessage(tr("Process finished after %1.").arg(time2text(timeElapsed)), ProgressModel::SysMsg_Performance);
			}
		}

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
			SET_PROGRESS_TEXT(tr("All files completed successfully."));
			m_systemTray->showMessage(tr("LameXP - Done"), tr("All files completed successfully."), QSystemTrayIcon::Information);
			m_systemTray->setIcon(QIcon(":/icons/cd_add.png"));
			QApplication::processEvents();
			if(m_settings->soundsEnabled()) PlaySound(MAKEINTRESOURCE(IDR_WAVE_SUCCESS), GetModuleHandle(NULL), SND_RESOURCE | SND_SYNC);
		}
	}
	
	setCloseButtonEnabled(true);
	button_closeDialog->setEnabled(true);
	button_AbortProcess->setEnabled(false);
	checkBox_shutdownComputer->setEnabled(false);

	m_progressModel->restoreHiddenItems();
	view_log->scrollToBottom();
	m_progressIndicator->stop();
	progressBar->setValue(progressBar->maximum());
	WinSevenTaskbar::setTaskbarProgress(this, progressBar->value(), progressBar->maximum());

	QApplication::restoreOverrideCursor();

	if(!m_userAborted && checkBox_shutdownComputer->isChecked())
	{
		if(shutdownComputer())
		{
			m_shutdownFlag = m_settings->hibernateComputer() ? shutdownFlag_Hibernate : shutdownFlag_TurnPowerOff;
			accept();
		}
	}
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

void ProcessingDialog::logViewSectionSizeChanged(int logicalIndex, int oldSize, int newSize)
{
	if(logicalIndex == 1)
	{
		if(QHeaderView *hdr = view_log->horizontalHeader())
		{
			hdr->setMinimumSectionSize(qMax(hdr->minimumSectionSize(), hdr->sectionSize(1)));
		}
	}
}

void ProcessingDialog::contextMenuTriggered(const QPoint &pos)
{
	QAbstractScrollArea *scrollArea = dynamic_cast<QAbstractScrollArea*>(QObject::sender());
	QWidget *sender = scrollArea ? scrollArea->viewport() : dynamic_cast<QWidget*>(QObject::sender());	

	if(pos.x() <= sender->width() && pos.y() <= sender->height() && pos.x() >= 0 && pos.y() >= 0)
	{
		m_contextMenu->popup(sender->mapToGlobal(pos));
	}
}

void ProcessingDialog::contextMenuDetailsActionTriggered(void)
{
	QModelIndex index = view_log->indexAt(view_log->viewport()->mapFromGlobal(m_contextMenu->pos()));
	logViewDoubleClicked(index.isValid() ? index : view_log->currentIndex());
}

void ProcessingDialog::contextMenuShowFileActionTriggered(void)
{
	QModelIndex index = view_log->indexAt(view_log->viewport()->mapFromGlobal(m_contextMenu->pos()));
	const QUuid &jobId = m_progressModel->getJobId(index.isValid() ? index : view_log->currentIndex());
	QString filePath = m_playList.value(jobId, QString());

	if(filePath.isEmpty())
	{
		MessageBeep(MB_ICONWARNING);
		return;
	}

	if(QFileInfo(filePath).exists())
	{
		QString systemRootPath;

		QDir systemRoot(lamexp_known_folder(lamexp_folder_systemfolder));
		if(systemRoot.exists() && systemRoot.cdUp())
		{
			systemRootPath = systemRoot.canonicalPath();
		}

		if(!systemRootPath.isEmpty())
		{
			QFileInfo explorer(QString("%1/explorer.exe").arg(systemRootPath));
			if(explorer.exists() && explorer.isFile())
			{
				QProcess::execute(explorer.canonicalFilePath(), QStringList() << "/select," << QDir::toNativeSeparators(QFileInfo(filePath).canonicalFilePath()));
				return;
			}
		}
		else
		{
			qWarning("SystemRoot directory could not be detected!");
		}
	}
	else
	{
		qWarning("File not found: %s", filePath.toLatin1().constData());
		MessageBeep(MB_ICONERROR);
	}
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
	bool nativeResampling = false;

	//Create encoder instance
	AbstractEncoder *encoder = makeEncoder(&nativeResampling);

	//Create processing thread
	ProcessThread *thread = new ProcessThread
	(
		currentFile,
		(m_settings->outputToSourceDir() ? QFileInfo(currentFile.filePath()).absolutePath() : m_settings->outputDir()),
		(m_settings->customTempPathEnabled() ? m_settings->customTempPath() : lamexp_temp_folder2()),
		encoder,
		m_settings->prependRelativeSourcePath() && (!m_settings->outputToSourceDir())
	);

	//Add audio filters
	if(m_settings->forceStereoDownmix())
	{
		thread->addFilter(new DownmixFilter());
	}
	if((m_settings->samplingRate() > 0) && !nativeResampling)
	{
		if(SettingsModel::samplingRates[m_settings->samplingRate()] != currentFile.formatAudioSamplerate() || currentFile.formatAudioSamplerate() == 0)
		{
			thread->addFilter(new ResampleFilter(SettingsModel::samplingRates[m_settings->samplingRate()]));
		}
	}
	if((m_settings->toneAdjustBass() != 0) || (m_settings->toneAdjustTreble() != 0))
	{
		thread->addFilter(new ToneAdjustFilter(m_settings->toneAdjustBass(), m_settings->toneAdjustTreble()));
	}
	if(m_settings->normalizationFilterEnabled())
	{
		thread->addFilter(new NormalizeFilter(m_settings->normalizationFilterMaxVolume(), m_settings->normalizationFilterEqualizationMode()));
	}
	if(m_settings->renameOutputFilesEnabled() && (!m_settings->renameOutputFilesPattern().simplified().isEmpty()))
	{
		thread->setRenamePattern(m_settings->renameOutputFilesPattern());
	}

	m_threadList.append(thread);
	m_allJobs.append(thread->getId());
	
	//Connect thread signals
	connect(thread, SIGNAL(finished()), this, SLOT(doneEncoding()), Qt::QueuedConnection);
	connect(thread, SIGNAL(processStateInitialized(QUuid,QString,QString,int)), m_progressModel, SLOT(addJob(QUuid,QString,QString,int)), Qt::QueuedConnection);
	connect(thread, SIGNAL(processStateChanged(QUuid,QString,int)), m_progressModel, SLOT(updateJob(QUuid,QString,int)), Qt::QueuedConnection);
	connect(thread, SIGNAL(processStateFinished(QUuid,QString,bool)), this, SLOT(processFinished(QUuid,QString,bool)), Qt::QueuedConnection);
	connect(thread, SIGNAL(processMessageLogged(QUuid,QString)), m_progressModel, SLOT(appendToLog(QUuid,QString)), Qt::QueuedConnection);
	
	//Give it a go!
	m_runningThreads++;
	thread->start();
}

AbstractEncoder *ProcessingDialog::makeEncoder(bool *nativeResampling)
{
	AbstractEncoder *encoder =  NULL;
	*nativeResampling = false;
	
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
				*nativeResampling = true;
			}
			mp3Encoder->setChannelMode(m_settings->lameChannelMode());
			mp3Encoder->setCustomParams(m_settings->customParametersLAME());
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
				*nativeResampling = true;
			}
			vorbisEncoder->setCustomParams(m_settings->customParametersOggEnc());
			encoder = vorbisEncoder;
		}
		break;
	case SettingsModel::AACEncoder:
		{
			if(lamexp_check_tool("qaac.exe") && lamexp_check_tool("libsoxrate.dll"))
			{
				QAACEncoder *aacEncoder = new QAACEncoder();
				aacEncoder->setBitrate(m_settings->compressionBitrate());
				aacEncoder->setRCMode(m_settings->compressionRCMode());
				aacEncoder->setProfile(m_settings->aacEncProfile());
				aacEncoder->setCustomParams(m_settings->customParametersAacEnc());
				encoder = aacEncoder;
			}
			else if(lamexp_check_tool("fhgaacenc.exe") && lamexp_check_tool("enc_fhgaac.dll"))
			{
				FHGAACEncoder *aacEncoder = new FHGAACEncoder();
				aacEncoder->setBitrate(m_settings->compressionBitrate());
				aacEncoder->setRCMode(m_settings->compressionRCMode());
				aacEncoder->setProfile(m_settings->aacEncProfile());
				aacEncoder->setCustomParams(m_settings->customParametersAacEnc());
				encoder = aacEncoder;
			}
			else
			{
				AACEncoder *aacEncoder = new AACEncoder();
				aacEncoder->setBitrate(m_settings->compressionBitrate());
				aacEncoder->setRCMode(m_settings->compressionRCMode());
				aacEncoder->setEnable2Pass(m_settings->neroAACEnable2Pass());
				aacEncoder->setProfile(m_settings->aacEncProfile());
				aacEncoder->setCustomParams(m_settings->customParametersAacEnc());
				encoder = aacEncoder;
			}
		}
		break;
	case SettingsModel::AC3Encoder:
		{
			AC3Encoder *ac3Encoder = new AC3Encoder();
			ac3Encoder->setBitrate(m_settings->compressionBitrate());
			ac3Encoder->setRCMode(m_settings->compressionRCMode());
			ac3Encoder->setCustomParams(m_settings->customParametersAften());
			ac3Encoder->setAudioCodingMode(m_settings->aftenAudioCodingMode());
			ac3Encoder->setDynamicRangeCompression(m_settings->aftenDynamicRangeCompression());
			ac3Encoder->setExponentSearchSize(m_settings->aftenExponentSearchSize());
			ac3Encoder->setFastBitAllocation(m_settings->aftenFastBitAllocation());
			encoder = ac3Encoder;
		}
		break;
	case SettingsModel::FLACEncoder:
		{
			FLACEncoder *flacEncoder = new FLACEncoder();
			flacEncoder->setBitrate(m_settings->compressionBitrate());
			flacEncoder->setRCMode(m_settings->compressionRCMode());
			flacEncoder->setCustomParams(m_settings->customParametersFLAC());
			encoder = flacEncoder;
		}
		break;
	case SettingsModel::DCAEncoder:
		{
			DCAEncoder *dcaEncoder = new DCAEncoder();
			dcaEncoder->setBitrate(m_settings->compressionBitrate());
			dcaEncoder->setRCMode(m_settings->compressionRCMode());
			encoder = dcaEncoder;
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

	return encoder;
}

void ProcessingDialog::writePlayList(void)
{
	if(m_succeededJobs.count() <= 0 || m_allJobs.count() <= 0)
	{
		qWarning("WritePlayList: Nothing to do!");
		return;
	}
	
	//Init local variables
	QStringList list;
	QRegExp regExp1("\\[\\d\\d\\][^/\\\\]+$", Qt::CaseInsensitive);
	QRegExp regExp2("\\(\\d\\d\\)[^/\\\\]+$", Qt::CaseInsensitive);
	QRegExp regExp3("\\d\\d[^/\\\\]+$", Qt::CaseInsensitive);
	bool usePrefix[3] = {true, true, true};
	bool useUtf8 = false;
	int counter = 1;

	//Generate playlist name
	QString playListName = (m_metaInfo->fileAlbum().isEmpty() ? "Playlist" : m_metaInfo->fileAlbum());
	if(!m_metaInfo->fileArtist().isEmpty())
	{
		playListName = QString("%1 - %2").arg(m_metaInfo->fileArtist(), playListName);
	}

	//Clean playlist name
	playListName = lamexp_clean_filename(playListName);

	//Create list of audio files
	for(int i = 0; i < m_allJobs.count(); i++)
	{
		if(!m_succeededJobs.contains(m_allJobs.at(i))) continue;
		list << QDir::toNativeSeparators(QDir(m_settings->outputDir()).relativeFilePath(m_playList.value(m_allJobs.at(i), "N/A")));
	}

	//Use prefix?
	for(int i = 0; i < list.count(); i++)
	{
		if(regExp1.indexIn(list.at(i)) < 0) usePrefix[0] = false;
		if(regExp2.indexIn(list.at(i)) < 0) usePrefix[1] = false;
		if(regExp3.indexIn(list.at(i)) < 0) usePrefix[2] = false;
	}
	if(usePrefix[0] || usePrefix[1] || usePrefix[2])
	{
		playListName.prepend(usePrefix[0] ? "[00] " : (usePrefix[1] ? "(00) " : "00 "));
	}

	//Do we need an UTF-8 playlist?
	for(int i = 0; i < list.count(); i++)
	{
		if(wcscmp(QWCHAR(QString::fromLatin1(list.at(i).toLatin1().constData())), QWCHAR(list.at(i))))
		{
			useUtf8 = true;
			break;
		}
	}

	//Generate playlist output file
	QString playListFile = QString("%1/%2.%3").arg(m_settings->outputDir(), playListName, (useUtf8 ? "m3u8" : "m3u"));
	while(QFileInfo(playListFile).exists())
	{
		playListFile = QString("%1/%2 (%3).%4").arg(m_settings->outputDir(), playListName, QString::number(++counter), (useUtf8 ? "m3u8" : "m3u"));
	}

	//Now write playlist to output file
	QFile playList(playListFile);
	if(playList.open(QIODevice::WriteOnly))
	{
		if(useUtf8)
		{
			playList.write("\xef\xbb\xbf");
		}
		playList.write("#EXTM3U\r\n");
		while(!list.isEmpty())
		{
			playList.write(useUtf8 ? list.takeFirst().toUtf8().constData() : list.takeFirst().toLatin1().constData());
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
		return AudioFileModel(audioFile, false);
	}
	
	AudioFileModel result = audioFile;
	result.updateMetaInfo(*m_metaInfo);
	
	if(m_metaInfo->filePosition() == UINT_MAX)
	{
		result.setFilePosition(m_currentFile);
	}

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

void ProcessingDialog::cpuUsageHasChanged(const double val)
{
	
	this->label_cpu->setText(QString().sprintf(" %d%%", qRound(val * 100.0)));
	UPDATE_MIN_WIDTH(label_cpu);
}

void ProcessingDialog::ramUsageHasChanged(const double val)
{
	
	this->label_ram->setText(QString().sprintf(" %d%%", qRound(val * 100.0)));
	UPDATE_MIN_WIDTH(label_ram);
}

void ProcessingDialog::diskUsageHasChanged(const quint64 val)
{
	int postfix = 0;
	const char *postfixStr[6] = {"B", "KB", "MB", "GB", "TB", "PB"};
	double space = static_cast<double>(val);

	while((space >= 1000.0) && (postfix < 5))
	{
		space = space / 1024.0;
		postfix++;
	}

	this->label_disk->setText(QString().sprintf(" %3.1f %s", space, postfixStr[postfix]));
	UPDATE_MIN_WIDTH(label_disk);
}

bool ProcessingDialog::shutdownComputer(void)
{
	const int iTimeout = m_settings->hibernateComputer() ? 10 : 30;
	const Qt::WindowFlags flags = Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowSystemMenuHint;
	const QString text = QString("%1%2%1").arg(QString().fill(' ', 18), tr("Warning: Computer will shutdown in %1 seconds..."));
	
	qWarning("Initiating shutdown sequence!");
	
	QProgressDialog progressDialog(text.arg(iTimeout), tr("Cancel Shutdown"), 0, iTimeout + 1, this, flags);
	QPushButton *cancelButton = new QPushButton(tr("Cancel Shutdown"), &progressDialog);
	cancelButton->setIcon(QIcon(":/icons/power_on.png"));
	progressDialog.setModal(true);
	progressDialog.setAutoClose(false);
	progressDialog.setAutoReset(false);
	progressDialog.setWindowIcon(QIcon(":/icons/power_off.png"));
	progressDialog.setCancelButton(cancelButton);
	progressDialog.show();
	
	QApplication::processEvents();

	if(m_settings->soundsEnabled())
	{
		QApplication::setOverrideCursor(Qt::WaitCursor);
		PlaySound(MAKEINTRESOURCE(IDR_WAVE_SHUTDOWN), GetModuleHandle(NULL), SND_RESOURCE | SND_SYNC);
		QApplication::restoreOverrideCursor();
	}

	QTimer timer;
	timer.setInterval(1000);
	timer.start();

	QEventLoop eventLoop(this);
	connect(&timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));
	connect(&progressDialog, SIGNAL(canceled()), &eventLoop, SLOT(quit()));

	for(int i = 1; i <= iTimeout; i++)
	{
		eventLoop.exec();
		if(progressDialog.wasCanceled())
		{
			progressDialog.close();
			return false;
		}
		progressDialog.setValue(i+1);
		progressDialog.setLabelText(text.arg(iTimeout-i));
		if(iTimeout-i == 3) progressDialog.setCancelButton(NULL);
		QApplication::processEvents();
		PlaySound(MAKEINTRESOURCE((i < iTimeout) ? IDR_WAVE_BEEP : IDR_WAVE_BEEP_LONG), GetModuleHandle(NULL), SND_RESOURCE | SND_SYNC);
	}
	
	progressDialog.close();
	return true;
}

QString ProcessingDialog::time2text(const double timeVal) const
{
	double intPart = 0;
	double frcPart = modf(timeVal, &intPart);
	int x = 0, y = 0; QString a, b;

	QTime time = QTime().addSecs(qRound(intPart)).addMSecs(qRound(frcPart * 1000.0));

	if(time.hour() > 0)
	{
		x = time.hour();   a = tr("hour(s)");
		y = time.minute(); b = tr("minute(s)");
	}
	else if(time.minute() > 0)
	{
		x = time.minute(); a = tr("minute(s)");
		y = time.second(); b = tr("second(s)");
	}
	else
	{
		x = time.second(); a = tr("second(s)");
		y = time.msec();   b = tr("millisecond(s)");
	}

	return QString("%1 %2, %3 %4").arg(QString::number(x), a, QString::number(y), b);
}

////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
////////////////////////////////////////////////////////////

static int cores2instances(int cores)
{
	//This function is a "cubic spline" with sampling points at:
	//(1,1); (2,2); (4,4); (8,6); (16,8); (32,11); (64,16)
	static const double LUT[8][5] =
	{
		{ 1.0,  0.014353554, -0.043060662, 1.028707108,  0.000000000},
		{ 2.0, -0.028707108,  0.215303309, 0.511979167,  0.344485294},
		{ 4.0,  0.010016468, -0.249379596, 2.370710784, -2.133823529},
		{ 8.0,  0.000282437, -0.015762868, 0.501776961,  2.850000000},
		{16.0,  0.000033270, -0.003802849, 0.310416667,  3.870588235},
		{32.0,  0.000006343, -0.001217831, 0.227696078,  4.752941176},
		{64.0,  0.000000000,  0.000000000, 0.000000000, 16.000000000},
		{DBL_MAX, 0.0, 0.0, 0.0, 0.0}
	};

	double x = abs(static_cast<double>(cores)), y = 1.0;
	
	for(size_t i = 0; i < 7; i++)
	{
		if((x >= LUT[i][0]) && (x < LUT[i+1][0]))
		{
			y = (((((LUT[i][1] * x) + LUT[i][2]) * x) + LUT[i][3]) * x) + LUT[i][4];
			break;
		}
	}

	return qRound(y);
}
