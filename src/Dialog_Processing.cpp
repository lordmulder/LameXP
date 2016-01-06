///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2016 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version, but always including the *additional*
// restrictions defined in the "License.txt" file.
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

//UIC includes
#include "UIC_ProcessingDialog.h"

//Internal
#include "Global.h"
#include "Model_FileList.h"
#include "Model_Progress.h"
#include "Model_Settings.h"
#include "Model_FileExts.h"
#include "Thread_Process.h"
#include "Thread_CPUObserver.h"
#include "Thread_RAMObserver.h"
#include "Thread_DiskObserver.h"
#include "Dialog_LogView.h"
#include "Registry_Decoder.h"
#include "Registry_Encoder.h"
#include "Filter_Downmix.h"
#include "Filter_Normalize.h"
#include "Filter_Resample.h"
#include "Filter_ToneAdjust.h"

//MUtils
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>
#include <MUtils/GUI.h>
#include <MUtils/CPUFeatures.h>
#include <MUtils/Sound.h>
#include <MUtils/Taskbar7.h>

//Qt
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
#include <QResizeEvent>
#include <QTime>
#include <QElapsedTimer>
#include <QThreadPool>

#include <math.h>
#include <float.h>
#include <stdint.h>

////////////////////////////////////////////////////////////

//Maximum number of parallel instances
#define MAX_INSTANCES 32U

//Function to calculate the number of instances
static int cores2instances(int cores);

////////////////////////////////////////////////////////////

#define CHANGE_BACKGROUND_COLOR(WIDGET, COLOR) do \
{ \
	QPalette palette = WIDGET->palette(); \
	palette.setColor(QPalette::Background, COLOR); \
	WIDGET->setPalette(palette); \
} \
while(0)

#define SET_PROGRESS_TEXT(TXT) do \
{ \
	ui->label_progress->setText(TXT); \
	m_systemTray->setToolTip(QString().sprintf("LameXP v%d.%02d\n%ls", lamexp_version_major(), lamexp_version_minor(), QString(TXT).utf16())); \
} \
while(0)

#define SET_FONT_BOLD(WIDGET,BOLD) do \
{ \
	QFont _font = WIDGET->font(); \
	_font.setBold(BOLD); WIDGET->setFont(_font); \
} \
while(0)

#define SET_TEXT_COLOR(WIDGET, COLOR) do \
{ \
	QPalette _palette = WIDGET->palette(); \
	_palette.setColor(QPalette::WindowText, (COLOR)); \
	_palette.setColor(QPalette::Text, (COLOR)); \
	WIDGET->setPalette(_palette); \
} \
while(0)

#define UPDATE_MIN_WIDTH(WIDGET) do \
{ \
	if(WIDGET->width() > WIDGET->minimumWidth()) WIDGET->setMinimumWidth(WIDGET->width()); \
} \
while(0)

#define PLAY_SOUND_OPTIONAL(NAME, ASYNC) do \
{ \
	if(m_settings->soundsEnabled()) MUtils::Sound::play_sound((NAME), (ASYNC)); \
} \
while(0)

#define IS_VBR(RC_MODE) ((RC_MODE) == SettingsModel::VBRMode)

////////////////////////////////////////////////////////////

//Dummy class for UserData
class IntUserData : public QObjectUserData
{
public:
	IntUserData(int value) : m_value(value) {/*NOP*/}
	int value(void) { return m_value; }
	void setValue(int value) { m_value = value; }
private:
	int m_value;
};

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

ProcessingDialog::ProcessingDialog(FileListModel *const fileListModel, const AudioFileModel_MetaInfo *const metaInfo, const SettingsModel *const settings, QWidget *const parent)
:
	QDialog(parent),
	ui(new Ui::ProcessingDialog),
	m_systemTray(new QSystemTrayIcon(QIcon(":/icons/cd_go.png"), this)),
	m_taskbar(new MUtils::Taskbar7(this)),
	m_settings(settings),
	m_metaInfo(metaInfo),
	m_shutdownFlag(SHUTDOWN_FLAG_NONE),
	m_progressViewFilter(-1),
	m_initThreads(0),
	m_defaultColor(new QColor()),
	m_firstShow(true)
{
	//Init the dialog, from the .ui file
	ui->setupUi(this);
	setWindowFlags(windowFlags() ^ Qt::WindowContextHelpButtonHint);
	
	//Update the window icon
	MUtils::GUI::set_window_icon(this, lamexp_app_icon(), true);

	//Update header icon
	ui->label_headerIcon->setPixmap(lamexp_app_icon().pixmap(ui->label_headerIcon->size()));
	
	//Setup version info
	ui->label_versionInfo->setText(QString().sprintf("v%d.%02d %s (Build %d)", lamexp_version_major(), lamexp_version_minor(), lamexp_version_release(), lamexp_version_build()));
	ui->label_versionInfo->installEventFilter(this);

	//Register meta type
	qRegisterMetaType<QUuid>("QUuid");

	//Center window in screen
	QRect desktopRect = QApplication::desktop()->screenGeometry();
	QRect thisRect = this->geometry();
	move((desktopRect.width() - thisRect.width()) / 2, (desktopRect.height() - thisRect.height()) / 2);
	setMinimumSize(thisRect.width(), thisRect.height());

	//Enable buttons
	connect(ui->button_AbortProcess, SIGNAL(clicked()), this, SLOT(abortEncoding()));
	
	//Init progress indicator
	m_progressIndicator.reset(new QMovie(":/images/Working.gif"));
	m_progressIndicator->setCacheMode(QMovie::CacheAll);
	ui->label_headerWorking->setMovie(m_progressIndicator.data());
	ui->progressBar->setValue(0);

	//Init progress model
	m_progressModel.reset(new ProgressModel());
	ui->view_log->setModel(m_progressModel.data());
	ui->view_log->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	ui->view_log->verticalHeader()->hide();
	ui->view_log->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	ui->view_log->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
	ui->view_log->viewport()->installEventFilter(this);
	connect(m_progressModel.data(), SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(progressModelChanged()));
	connect(m_progressModel.data(), SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)), this, SLOT(progressModelChanged()));
	connect(m_progressModel.data(), SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(progressModelChanged()));
	connect(m_progressModel.data(), SIGNAL(modelReset()), this, SLOT(progressModelChanged()));
	connect(ui->view_log, SIGNAL(activated(QModelIndex)), this, SLOT(logViewDoubleClicked(QModelIndex)));
	connect(ui->view_log->horizontalHeader(), SIGNAL(sectionResized(int,int,int)), this, SLOT(logViewSectionSizeChanged(int,int,int)));

	//Create context menu
	m_contextMenu.reset(new QMenu());
	QAction *contextMenuDetailsAction  = m_contextMenu->addAction(QIcon(":/icons/zoom.png"), tr("Show details for selected job"));
	QAction *contextMenuShowFileAction = m_contextMenu->addAction(QIcon(":/icons/folder_go.png"), tr("Browse Output File Location"));
	m_contextMenu->addSeparator();

	//Create "filter" context menu
	m_progressViewFilterGroup.reset(new QActionGroup(this));
	QAction *contextMenuFilterAction[5] = {NULL, NULL, NULL, NULL, NULL};
	if(QMenu *filterMenu = m_contextMenu->addMenu(QIcon(":/icons/filter.png"), tr("Filter Log Items")))
	{
		contextMenuFilterAction[0] = filterMenu->addAction(m_progressModel->getIcon(ProgressModel::JobRunning), tr("Show Running Only"));
		contextMenuFilterAction[1] = filterMenu->addAction(m_progressModel->getIcon(ProgressModel::JobComplete), tr("Show Succeeded Only"));
		contextMenuFilterAction[2] = filterMenu->addAction(m_progressModel->getIcon(ProgressModel::JobFailed), tr("Show Failed Only"));
		contextMenuFilterAction[3] = filterMenu->addAction(m_progressModel->getIcon(ProgressModel::JobSkipped), tr("Show Skipped Only"));
		contextMenuFilterAction[4] = filterMenu->addAction(m_progressModel->getIcon(ProgressModel::JobState(-1)), tr("Show All Items"));
		if(QAction *act = contextMenuFilterAction[0]) { m_progressViewFilterGroup->addAction(act); act->setCheckable(true); act->setData(ProgressModel::JobRunning); }
		if(QAction *act = contextMenuFilterAction[1]) { m_progressViewFilterGroup->addAction(act); act->setCheckable(true); act->setData(ProgressModel::JobComplete); }
		if(QAction *act = contextMenuFilterAction[2]) { m_progressViewFilterGroup->addAction(act); act->setCheckable(true); act->setData(ProgressModel::JobFailed); }
		if(QAction *act = contextMenuFilterAction[3]) { m_progressViewFilterGroup->addAction(act); act->setCheckable(true); act->setData(ProgressModel::JobSkipped); }
		if(QAction *act = contextMenuFilterAction[4]) { m_progressViewFilterGroup->addAction(act); act->setCheckable(true); act->setData(-1); act->setChecked(true); }
	}

	//Create info label
	m_filterInfoLabel.reset(new QLabel(ui->view_log));
	m_filterInfoLabel->setFrameShape(QFrame::NoFrame);
	m_filterInfoLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	m_filterInfoLabel->setUserData(0, new IntUserData(-1));
	SET_FONT_BOLD(m_filterInfoLabel, true);
	SET_TEXT_COLOR(m_filterInfoLabel, Qt::darkGray);
	m_filterInfoLabel->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(m_filterInfoLabel.data(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuTriggered(QPoint)));
	m_filterInfoLabel->hide();

	m_filterInfoLabelIcon .reset(new QLabel(ui->view_log));
	m_filterInfoLabelIcon->setFrameShape(QFrame::NoFrame);
	m_filterInfoLabelIcon->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
	m_filterInfoLabelIcon->setContextMenuPolicy(Qt::CustomContextMenu);
	const QIcon &ico = m_progressModel->getIcon(ProgressModel::JobState(-1));
	m_filterInfoLabelIcon->setPixmap(ico.pixmap(16, 16));
	connect(m_filterInfoLabelIcon.data(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuTriggered(QPoint)));
	m_filterInfoLabelIcon->hide();

	//Connect context menu
	ui->view_log->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->view_log, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuTriggered(QPoint)));
	connect(contextMenuDetailsAction, SIGNAL(triggered(bool)), this, SLOT(contextMenuDetailsActionTriggered()));
	connect(contextMenuShowFileAction, SIGNAL(triggered(bool)), this, SLOT(contextMenuShowFileActionTriggered()));
	for(size_t i = 0; i < 5; i++)
	{
		if(contextMenuFilterAction[i]) connect(contextMenuFilterAction[i], SIGNAL(triggered(bool)), this, SLOT(contextMenuFilterActionTriggered()));
	}
	SET_FONT_BOLD(contextMenuDetailsAction, true);

	//Setup file extensions
	if(!m_settings->renameFiles_fileExtension().isEmpty())
	{
		m_fileExts.reset(new FileExtsModel());
		m_fileExts->importItems(m_settings->renameFiles_fileExtension());
	}

	//Enque jobs
	if(fileListModel)
	{
		for(int i = 0; i < fileListModel->rowCount(); i++)
		{
			m_pendingJobs.append(fileListModel->getFile(fileListModel->index(i,0)));
		}
	}

	//Translate
	ui->label_headerStatus->setText(QString("<b>%1</b><br>%2").arg(tr("Encoding Files"), tr("Your files are being encoded, please be patient...")));
	
	//Enable system tray icon
	connect(m_systemTray.data(), SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(systemTrayActivated(QSystemTrayIcon::ActivationReason)));

	//Init other vars
	m_runningThreads = 0;
	m_currentFile = 0;
	m_allJobs.clear();
	m_succeededJobs.clear();
	m_failedJobs.clear();
	m_skippedJobs.clear();
	m_userAborted = false;
	m_forcedAbort = false;
}

////////////////////////////////////////////////////////////
// Destructor
////////////////////////////////////////////////////////////

ProcessingDialog::~ProcessingDialog(void)
{
	ui->view_log->setModel(NULL);

	if(!m_progressIndicator.isNull())
	{
		m_progressIndicator->stop();
	}

	if(!m_diskObserver.isNull())
	{
		m_diskObserver->stop();
		if(!m_diskObserver->wait(15000))
		{
			m_diskObserver->terminate();
			m_diskObserver->wait();
		}
	}

	if(!m_cpuObserver.isNull())
	{
		m_cpuObserver->stop();
		if(!m_cpuObserver->wait(15000))
		{
			m_cpuObserver->terminate();
			m_cpuObserver->wait();
		}
	}

	if(!m_ramObserver.isNull())
	{
		m_ramObserver->stop();
		if(!m_ramObserver->wait(15000))
		{
			m_ramObserver->terminate();
			m_ramObserver->wait();
		}
	}

	if(!m_threadPool.isNull())
	{
		if(!m_threadPool->waitForDone(100))
		{
			emit abortRunningTasks();
			m_threadPool->waitForDone();
		}
	}

	m_taskbar->setOverlayIcon(NULL);
	m_taskbar->setTaskbarState(MUtils::Taskbar7::TASKBAR_STATE_NONE);
	
	MUTILS_DELETE(ui);
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

void ProcessingDialog::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);

	if(m_firstShow)
	{
		static const char *NA = " N/A";

		MUtils::GUI::enable_close_button(this, false);
		ui->button_closeDialog->setEnabled(false);
		ui->button_AbortProcess->setEnabled(false);
		m_progressIndicator->start();
		m_systemTray->setVisible(true);
		
		MUtils::OS::change_process_priority(1);
		
		ui->label_cpu->setText(NA);
		ui->label_disk->setText(NA);
		ui->label_ram->setText(NA);

		QTimer::singleShot(500, this, SLOT(initEncoding()));
		m_firstShow = false;
	}

	//Force update geometry
	resizeEvent(NULL);
}

void ProcessingDialog::closeEvent(QCloseEvent *event)
{
	if(!ui->button_closeDialog->isEnabled())
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
	if(obj == ui->label_versionInfo)
	{
		if(event->type() == QEvent::Enter)
		{
			QPalette palette = ui->label_versionInfo->palette();
			*m_defaultColor = palette.color(QPalette::Normal, QPalette::WindowText);
			palette.setColor(QPalette::Normal, QPalette::WindowText, Qt::red);
			ui->label_versionInfo->setPalette(palette);
		}
		else if(event->type() == QEvent::Leave)
		{
			QPalette palette = ui->label_versionInfo->palette();
			palette.setColor(QPalette::Normal, QPalette::WindowText, *m_defaultColor);
			ui->label_versionInfo->setPalette(palette);
		}
		else if(event->type() == QEvent::MouseButtonPress)
		{
			QUrl url(lamexp_website_url());
			QDesktopServices::openUrl(url);
		}
	}

	return false;
}

bool ProcessingDialog::event(QEvent *e)
{
	switch(e->type())
	{
	case MUtils::GUI::USER_EVENT_QUERYENDSESSION:
		qWarning("System is shutting down, preparing to abort...");
		if(!m_userAborted) abortEncoding(true);
		return true;
	case MUtils::GUI::USER_EVENT_ENDSESSION:
		qWarning("System is shutting down, encoding will be aborted now...");
		if(isVisible())
		{
			while(!close())
			{
				if(!m_userAborted) abortEncoding(true);
				qApp->processEvents(QEventLoop::WaitForMoreEvents | QEventLoop::ExcludeUserInputEvents);
			}
		}
		m_pendingJobs.clear();
		return true;
	default:
		return QDialog::event(e);
	}
}

/*
 * Window was resized
 */
void ProcessingDialog::resizeEvent(QResizeEvent *event)
{
	if(event) QDialog::resizeEvent(event);

	if(QWidget *port = ui->view_log->viewport())
	{
		QRect geom = port->geometry();
		m_filterInfoLabel->setGeometry(geom.left() + 16, geom.top() + 16, geom.width() - 32, 48);
		m_filterInfoLabelIcon->setGeometry(geom.left() + 16, geom.top() + 64, geom.width() - 32, geom.height() - 80);
	}
}

////////////////////////////////////////////////////////////
// SLOTS
////////////////////////////////////////////////////////////

void ProcessingDialog::initEncoding(void)
{
	qDebug("Initializing encoding process...");
	
	m_runningThreads = 0;
	m_currentFile = 0;
	m_allJobs.clear();
	m_succeededJobs.clear();
	m_failedJobs.clear();
	m_skippedJobs.clear();
	m_userAborted = false;
	m_forcedAbort = false;
	m_playList.clear();

	DecoderRegistry::configureDecoders(m_settings);

	CHANGE_BACKGROUND_COLOR(ui->frame_header, QColor(Qt::white));
	SET_PROGRESS_TEXT(tr("Encoding files, please wait..."));
	
	ui->button_closeDialog->setEnabled(false);
	ui->button_AbortProcess->setEnabled(true);
	ui->progressBar->setRange(0, m_pendingJobs.count());
	ui->checkBox_shutdownComputer->setEnabled(true);
	ui->checkBox_shutdownComputer->setChecked(false);

	m_taskbar->setTaskbarState(MUtils::Taskbar7::TASKBAR_STATE_NORMAL);
	m_taskbar->setTaskbarProgress(0, m_pendingJobs.count());
	m_taskbar->setOverlayIcon(&QIcon(":/icons/control_play_blue.png"));

	if(!m_diskObserver)
	{
		m_diskObserver.reset(new DiskObserverThread(m_settings->customTempPathEnabled() ? m_settings->customTempPath() : MUtils::temp_folder()));
		connect(m_diskObserver.data(), SIGNAL(messageLogged(QString,int)), m_progressModel.data(), SLOT(addSystemMessage(QString,int)), Qt::QueuedConnection);
		connect(m_diskObserver.data(), SIGNAL(freeSpaceChanged(quint64)), this, SLOT(diskUsageHasChanged(quint64)), Qt::QueuedConnection);
		m_diskObserver->start();
	}
	if(!m_cpuObserver)
	{
		m_cpuObserver.reset(new CPUObserverThread());
		connect(m_cpuObserver.data(), SIGNAL(currentUsageChanged(double)), this, SLOT(cpuUsageHasChanged(double)), Qt::QueuedConnection);
		m_cpuObserver->start();
	}
	if(!m_ramObserver)
	{
		m_ramObserver.reset(new RAMObserverThread());
		connect(m_ramObserver.data(), SIGNAL(currentUsageChanged(double)), this, SLOT(ramUsageHasChanged(double)), Qt::QueuedConnection);
		m_ramObserver->start();
	}

	if(m_threadPool.isNull())
	{
		unsigned int maximumInstances = qBound(0U, m_settings->maximumInstances(), MAX_INSTANCES);
		if(maximumInstances < 1)
		{
			const MUtils::CPUFetaures::cpu_info_t cpuFeatures = MUtils::CPUFetaures::detect();
			maximumInstances = cores2instances(qBound(1U, cpuFeatures.count, 64U));
		}

		maximumInstances = qBound(1U, maximumInstances, static_cast<unsigned int>(m_pendingJobs.count()));
		if(maximumInstances > 1)
		{
			m_progressModel->addSystemMessage(tr("Multi-threading enabled: Running %1 instances in parallel!").arg(QString::number(maximumInstances)));
		}

		m_threadPool.reset(new QThreadPool());
		m_threadPool->setMaxThreadCount(maximumInstances);
	}

	m_initThreads = m_threadPool->maxThreadCount();
	QTimer::singleShot(100, this, SLOT(initNextJob()));
	
	m_totalTime.reset(new QElapsedTimer());
	m_totalTime->start();
}

void ProcessingDialog::initNextJob(void)
{
	if((m_initThreads > 0) && (!m_userAborted))
	{
		startNextJob();
		if(--m_initThreads > 0)
		{
			QTimer::singleShot(32, this, SLOT(initNextJob()));
		}
	}
}

void ProcessingDialog::startNextJob(void)
{
	if(m_pendingJobs.isEmpty())
	{
		qWarning("No more files left, unable to start another job!");
		return;
	}
	
	m_currentFile++;
	m_runningThreads++;

	AudioFileModel currentFile = updateMetaInfo(m_pendingJobs.takeFirst());
	bool nativeResampling = false;

	//Create encoder instance
	AbstractEncoder *encoder = EncoderRegistry::createInstance(m_settings->compressionEncoder(), m_settings, &nativeResampling);

	//Create processing thread
	ProcessThread *thread = new ProcessThread
	(
		currentFile,
		(m_settings->outputToSourceDir() ? QFileInfo(currentFile.filePath()).absolutePath() : m_settings->outputDir()),
		(m_settings->customTempPathEnabled() ? m_settings->customTempPath() : MUtils::temp_folder()),
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
		if(SettingsModel::samplingRates[m_settings->samplingRate()] != currentFile.techInfo().audioSamplerate() || currentFile.techInfo().audioSamplerate() == 0)
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
		thread->addFilter(new NormalizeFilter(m_settings->normalizationFilterMaxVolume(), m_settings->normalizationFilterDynamic(), m_settings->normalizationFilterCoupled(), m_settings->normalizationFilterSize()));
	}
	if(m_settings->renameFiles_renameEnabled() && (!m_settings->renameFiles_renamePattern().simplified().isEmpty()))
	{
		thread->setRenamePattern(m_settings->renameFiles_renamePattern());
	}
	if(m_settings->renameFiles_regExpEnabled() && (!m_settings->renameFiles_regExpSearch().trimmed().isEmpty()) && (!m_settings->renameFiles_regExpReplace().simplified().isEmpty()))
	{
		thread->setRenameRegExp(m_settings->renameFiles_regExpSearch(), m_settings->renameFiles_regExpReplace());
	}
	if(!m_fileExts.isNull())
	{
		thread->setRenameFileExt(m_fileExts->apply(QString::fromUtf8(EncoderRegistry::getEncoderInfo(m_settings->compressionEncoder())->extension())));
	}
	if(m_settings->overwriteMode() != SettingsModel::Overwrite_KeepBoth)
	{
		thread->setOverwriteMode((m_settings->overwriteMode() == SettingsModel::Overwrite_SkipFile), (m_settings->overwriteMode() == SettingsModel::Overwrite_Replaces));
	}
	if (m_settings->keepOriginalDataTime())
	{
		thread->setKeepDateTime(m_settings->keepOriginalDataTime());
	}

	m_allJobs.append(thread->getId());
	
	//Connect thread signals
	connect(thread, SIGNAL(processFinished()), this, SLOT(doneEncoding()), Qt::QueuedConnection);
	connect(thread, SIGNAL(processStateInitialized(QUuid,QString,QString,int)), m_progressModel.data(), SLOT(addJob(QUuid,QString,QString,int)), Qt::QueuedConnection);
	connect(thread, SIGNAL(processStateChanged(QUuid,QString,int)), m_progressModel.data(), SLOT(updateJob(QUuid,QString,int)), Qt::QueuedConnection);
	connect(thread, SIGNAL(processStateFinished(QUuid,QString,int)), this, SLOT(processFinished(QUuid,QString,int)), Qt::QueuedConnection);
	connect(thread, SIGNAL(processMessageLogged(QUuid,QString)), m_progressModel.data(), SLOT(appendToLog(QUuid,QString)), Qt::QueuedConnection);
	connect(this, SIGNAL(abortRunningTasks()), thread, SLOT(abort()), Qt::DirectConnection);

	//Initialize thread object
	if(!thread->init())
	{
		qFatal("Fatal Error: Thread initialization has failed!");
	}

	//Give it a go!
	if(!thread->start(m_threadPool.data()))
	{
		qWarning("Job failed to start or file was skipped!");
	}
}

void ProcessingDialog::abortEncoding(bool force)
{
	m_userAborted = true;
	if(force) m_forcedAbort = true;
	ui->button_AbortProcess->setEnabled(false);
	SET_PROGRESS_TEXT(tr("Aborted! Waiting for running jobs to terminate..."));
	emit abortRunningTasks();
}

void ProcessingDialog::doneEncoding(void)
{
	m_runningThreads--;
	ui->progressBar->setValue(ui->progressBar->value() + 1);
	
	if(!m_userAborted)
	{
		SET_PROGRESS_TEXT(tr("Encoding: %n file(s) of %1 completed so far, please wait...", "", ui->progressBar->value()).arg(QString::number(ui->progressBar->maximum())));
		m_taskbar->setTaskbarProgress(ui->progressBar->value(), ui->progressBar->maximum());
	}
	
	if((!m_pendingJobs.isEmpty()) && (!m_userAborted))
	{
		QTimer::singleShot(0, this, SLOT(startNextJob()));
		qDebug("%d files left, starting next job...", m_pendingJobs.count());
		return;
	}
	
	if(m_runningThreads > 0)
	{
		qDebug("No files left, but still have %u running jobs.", m_runningThreads);
		return;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);
	qDebug("Running jobs: %u", m_runningThreads);

	if(!m_userAborted && m_settings->createPlaylist() && !m_settings->outputToSourceDir())
	{
		SET_PROGRESS_TEXT(tr("Creating the playlist file, please wait..."));
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
		writePlayList();
	}
	
	if(m_userAborted)
	{
		CHANGE_BACKGROUND_COLOR(ui->frame_header, QColor("#FFFFE0"));
		m_taskbar->setTaskbarState(MUtils::Taskbar7::TASKBAR_STATE_ERROR);
		m_taskbar->setOverlayIcon(&QIcon(":/icons/error.png"));
		SET_PROGRESS_TEXT((m_succeededJobs.count() > 0) ? tr("Process was aborted by the user after %n file(s)!", "", m_succeededJobs.count()) : tr("Process was aborted prematurely by the user!"));
		m_systemTray->showMessage(tr("LameXP - Aborted"), tr("Process was aborted by the user."), QSystemTrayIcon::Warning);
		m_systemTray->setIcon(QIcon(":/icons/cd_delete.png"));
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
		if(!m_forcedAbort) PLAY_SOUND_OPTIONAL("aborted", false);
	}
	else
	{
		if((!m_totalTime.isNull()) && m_totalTime->isValid())
		{
			m_progressModel->addSystemMessage(tr("Process finished after %1.").arg(time2text(m_totalTime->elapsed())), ProgressModel::SysMsg_Performance);
			m_totalTime->invalidate();
		}

		if(m_failedJobs.count() > 0)
		{
			CHANGE_BACKGROUND_COLOR(ui->frame_header, QColor("#FFF0F0"));
			m_taskbar->setTaskbarState(MUtils::Taskbar7::TASKBAR_STATE_ERROR);
			m_taskbar->setOverlayIcon(&QIcon(":/icons/exclamation.png"));
			if(m_skippedJobs.count() > 0)
			{
				SET_PROGRESS_TEXT(tr("Error: %1 of %n file(s) failed (%2). Double-click failed items for detailed information!", "", m_failedJobs.count() + m_succeededJobs.count() + m_skippedJobs.count()).arg(QString::number(m_failedJobs.count()), tr("%n file(s) skipped", "", m_skippedJobs.count())));
			}
			else
			{
				SET_PROGRESS_TEXT(tr("Error: %1 of %n file(s) failed. Double-click failed items for detailed information!", "", m_failedJobs.count() + m_succeededJobs.count()).arg(QString::number(m_failedJobs.count())));
			}
			m_systemTray->showMessage(tr("LameXP - Error"), tr("At least one file has failed!"), QSystemTrayIcon::Critical);
			m_systemTray->setIcon(QIcon(":/icons/cd_delete.png"));
			qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
			PLAY_SOUND_OPTIONAL("error", false);
		}
		else
		{
			CHANGE_BACKGROUND_COLOR(ui->frame_header, QColor("#F0FFF0"));
			m_taskbar->setTaskbarState(MUtils::Taskbar7::TASKBAR_STATE_NORMAL);
			m_taskbar->setOverlayIcon(&QIcon(":/icons/accept.png"));
			if(m_skippedJobs.count() > 0)
			{
				SET_PROGRESS_TEXT(tr("All files completed successfully. Skipped %n file(s).", "", m_skippedJobs.count()));
			}
			else
			{
				SET_PROGRESS_TEXT(tr("All files completed successfully."));
			}
			m_systemTray->showMessage(tr("LameXP - Done"), tr("All files completed successfully."), QSystemTrayIcon::Information);
			m_systemTray->setIcon(QIcon(":/icons/cd_add.png"));
			qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
			PLAY_SOUND_OPTIONAL("success", false);
		}
	}
	
	MUtils::GUI::enable_close_button(this, true);
	ui->button_closeDialog->setEnabled(true);
	ui->button_AbortProcess->setEnabled(false);
	ui->checkBox_shutdownComputer->setEnabled(false);

	m_progressModel->restoreHiddenItems();
	ui->view_log->scrollToBottom();
	m_progressIndicator->stop();
	ui->progressBar->setValue(ui->progressBar->maximum());
	m_taskbar->setTaskbarProgress(ui->progressBar->value(), ui->progressBar->maximum());

	QApplication::restoreOverrideCursor();

	if(!m_userAborted && ui->checkBox_shutdownComputer->isChecked())
	{
		if(shutdownComputer())
		{
			m_shutdownFlag = m_settings->hibernateComputer() ? SHUTDOWN_FLAG_HIBERNATE : SHUTDOWN_FLAG_POWER_OFF;
			accept();
		}
	}
}

void ProcessingDialog::processFinished(const QUuid &jobId, const QString &outFileName, int success)
{
	if(success > 0)
	{
		m_playList.insert(jobId, outFileName);
		m_succeededJobs.append(jobId);
	}
	else if(success < 0)
	{
		m_playList.insert(jobId, outFileName);
		m_skippedJobs.append(jobId);
	}
	else
	{
		m_failedJobs.append(jobId);
	}

	//Update filter as soon as a job finished!
	if(m_progressViewFilter >= 0)
	{
		QTimer::singleShot(0, this, SLOT(progressViewFilterChanged()));
	}
}

void ProcessingDialog::progressModelChanged(void)
{
	//Update filter as soon as the model changes!
	if(m_progressViewFilter >= 0)
	{
		QTimer::singleShot(0, this, SLOT(progressViewFilterChanged()));
	}

	QTimer::singleShot(0, ui->view_log, SLOT(scrollToBottom()));
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
			MUTILS_DELETE(logView);
		}
		else
		{
			QMessageBox::information(this, windowTitle(), m_progressModel->data(m_progressModel->index(index.row(), 0)).toString());
		}
	}
	else
	{
		MUtils::Sound::beep(MUtils::Sound::BEEP_WRN);
	}
}

void ProcessingDialog::logViewSectionSizeChanged(int logicalIndex, int oldSize, int newSize)
{
	if(logicalIndex == 1)
	{
		if(QHeaderView *hdr = ui->view_log->horizontalHeader())
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
	QModelIndex index = ui->view_log->indexAt(ui->view_log->viewport()->mapFromGlobal(m_contextMenu->pos()));
	logViewDoubleClicked(index.isValid() ? index : ui->view_log->currentIndex());
}

void ProcessingDialog::contextMenuShowFileActionTriggered(void)
{
	QModelIndex index = ui->view_log->indexAt(ui->view_log->viewport()->mapFromGlobal(m_contextMenu->pos()));
	const QUuid &jobId = m_progressModel->getJobId(index.isValid() ? index : ui->view_log->currentIndex());
	QString filePath = m_playList.value(jobId, QString());

	if(filePath.isEmpty())
	{
		MUtils::Sound::beep(MUtils::Sound::BEEP_WRN);
		return;
	}

	if(QFileInfo(filePath).exists())
	{
		QString systemRootPath;

		QDir systemRoot(MUtils::OS::known_folder(MUtils::OS::FOLDER_SYSTEMFOLDER));
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
		MUtils::Sound::beep(MUtils::Sound::BEEP_ERR);
	}
}

void ProcessingDialog::contextMenuFilterActionTriggered(void)
{
	if(QAction *action = dynamic_cast<QAction*>(QObject::sender()))
	{
		if(action->data().type() == QVariant::Int)
		{
			m_progressViewFilter = action->data().toInt();
			progressViewFilterChanged();
			QTimer::singleShot(0, this, SLOT(progressViewFilterChanged()));
			QTimer::singleShot(0, ui->view_log, SLOT(scrollToBottom()));
			action->setChecked(true);
		}
	}
}

/*
 * Filter progress items
 */
void ProcessingDialog::progressViewFilterChanged(void)
{
	bool matchFound = false;

	for(int i = 0; i < ui->view_log->model()->rowCount(); i++)
	{
		QModelIndex index = (m_progressViewFilter >= 0) ? m_progressModel->index(i, 0) : QModelIndex();
		const bool bHide = index.isValid() ? (m_progressModel->getJobState(index) != m_progressViewFilter) : false;
		ui->view_log->setRowHidden(i, bHide); matchFound = matchFound || (!bHide);
	}

	if((m_progressViewFilter >= 0) && (!matchFound))
	{
		if(m_filterInfoLabel->isHidden() || (dynamic_cast<IntUserData*>(m_filterInfoLabel->userData(0))->value() != m_progressViewFilter))
		{
			dynamic_cast<IntUserData*>(m_filterInfoLabel->userData(0))->setValue(m_progressViewFilter);
			m_filterInfoLabel->setText(QString("<p>&raquo; %1 &laquo;</p>").arg(tr("None of the items matches the current filtering rules")));
			m_filterInfoLabel->show();
			m_filterInfoLabelIcon->setPixmap(m_progressModel->getIcon(static_cast<ProgressModel::JobState>(m_progressViewFilter)).pixmap(16, 16, QIcon::Disabled));
			m_filterInfoLabelIcon->show();
			resizeEvent(NULL);
		}
	}
	else if(!m_filterInfoLabel->isHidden())
	{
		m_filterInfoLabel->hide();
		m_filterInfoLabelIcon->hide();
	}
}

////////////////////////////////////////////////////////////
// Private Functions
////////////////////////////////////////////////////////////

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
	QString playListName = (m_metaInfo->album().isEmpty() ? "Playlist" : m_metaInfo->album());
	if(!m_metaInfo->artist().isEmpty())
	{
		playListName = QString("%1 - %2").arg(m_metaInfo->artist(), playListName);
	}

	//Clean playlist name
	playListName = MUtils::clean_file_name(playListName);

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
		if(wcscmp(MUTILS_WCHR(QString::fromLatin1(list.at(i).toLatin1().constData())), MUTILS_WCHR(list.at(i))))
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
			playList.write(useUtf8 ? MUTILS_UTF8(list.takeFirst()) : list.takeFirst().toLatin1().constData());
			playList.write("\r\n");
		}
		playList.close();
	}
	else
	{
		QMessageBox::warning(this, tr("Playlist creation failed"), QString("%1<br><nobr>%2</nobr>").arg(tr("The playlist file could not be created:"), playListFile));
	}
}

AudioFileModel ProcessingDialog::updateMetaInfo(AudioFileModel &audioFile)
{
	if(!m_settings->writeMetaTags())
	{
		audioFile.metaInfo().reset();
		return audioFile;
	}
	
	audioFile.metaInfo().update(*m_metaInfo, true);
	
	if(audioFile.metaInfo().position() == UINT_MAX)
	{
		audioFile.metaInfo().setPosition(m_currentFile);
	}

	return audioFile;
}

void ProcessingDialog::systemTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
	if(reason == QSystemTrayIcon::DoubleClick)
	{
		MUtils::GUI::bring_to_front(this);
	}
}

void ProcessingDialog::cpuUsageHasChanged(const double val)
{
	
	ui->label_cpu->setText(QString().sprintf(" %d%%", qRound(val * 100.0)));
	UPDATE_MIN_WIDTH(ui->label_cpu);
}

void ProcessingDialog::ramUsageHasChanged(const double val)
{
	
	ui->label_ram->setText(QString().sprintf(" %d%%", qRound(val * 100.0)));
	UPDATE_MIN_WIDTH(ui->label_ram);
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

	ui->label_disk->setText(QString().sprintf(" %3.1f %s", space, postfixStr[postfix]));
	UPDATE_MIN_WIDTH(ui->label_disk);
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
	
	qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

	QApplication::setOverrideCursor(Qt::WaitCursor);
	PLAY_SOUND_OPTIONAL("shutdown", false);
	QApplication::restoreOverrideCursor();

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
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
		PLAY_SOUND_OPTIONAL(((i < iTimeout) ? "beep" : "beep2"), false);
	}
	
	progressDialog.close();
	return true;
}

QString ProcessingDialog::time2text(const qint64 &msec) const
{
	const qint64 MILLISECONDS_PER_DAY = 86399999;	//24x60x60x1000 - 1
	const QTime time = QTime().addMSecs(qMin(msec, MILLISECONDS_PER_DAY));

	QString a, b;

	if(time.hour() > 0)
	{
		a = tr("%n hour(s)",   "", time.hour());
		b = tr("%n minute(s)", "", time.minute());
	}
	else if(time.minute() > 0)
	{
		a = tr("%n minute(s)", "", time.minute());
		b = tr("%n second(s)", "", time.second());
	}
	else
	{
		a = tr("%n second(s)",      "", time.second());
		b = tr("%n millisecond(s)", "", time.msec());
	}

	return QString("%1, %2").arg(a, b);
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
