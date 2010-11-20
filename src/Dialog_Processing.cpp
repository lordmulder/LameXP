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

#include "Dialog_Processing.h"

#include "Global.h"
#include "Model_FileList.h"
#include "Model_Progress.h"
#include "Model_Settings.h"
#include "Thread_Process.h"
#include "Encoder_MP3.h"

#include <QApplication>
#include <QRect>
#include <QDesktopWidget>
#include <QMovie>
#include <QMessageBox>
#include <QTimer>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QUrl>
#include <Windows.h>

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

ProcessingDialog::ProcessingDialog(FileListModel *fileListModel, AudioFileModel *metaInfo, SettingsModel *settings, QWidget *parent)
:
	QDialog(parent),
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

	//Enque jobs
	if(fileListModel)
	{
		for(int i = 0; i < fileListModel->rowCount(); i++)
		{
			m_pendingJobs.append(fileListModel->getFile(fileListModel->index(i,0)));
		}
	}

	//Init other vars
	m_runningThreads = 0;
	m_currentFile = 0;
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

	QTimer::singleShot(1000, this, SLOT(initEncoding()));
}

void ProcessingDialog::closeEvent(QCloseEvent *event)
{
	if(!button_closeDialog->isEnabled()) event->ignore();
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
	m_userAborted = false;
	
	label_progress->setText("Encoding files, please wait...");
	m_progressIndicator->start();
	
	button_closeDialog->setEnabled(false);
	button_AbortProcess->setEnabled(true);
	progressBar->setRange(0, m_pendingJobs.count());

	startNextJob(); //TODO: Start as many jobs in parallel as processors available
	startNextJob();
}

void ProcessingDialog::abortEncoding(void)
{
	m_userAborted = true;
	button_AbortProcess->setEnabled(false);
	
	for(int i = 0; i < m_threadList.count(); i++)
	{
		m_threadList.at(i)->abort();
	}
}

void ProcessingDialog::doneEncoding(void)
{
	m_runningThreads--;

	progressBar->setValue(progressBar->value() + 1);
	label_progress->setText(QString("Encoding: %1 files of %2 completed so far, please wait...").arg(QString::number(progressBar->value()), QString::number(progressBar->maximum())));
	
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

	qDebug("Running jobs: %u", m_runningThreads);

	label_progress->setText(m_userAborted ? "Process was aborted by the user!" : "Alle files completed successfully.");
	m_progressIndicator->stop();

	setCloseButtonEnabled(true);
	button_closeDialog->setEnabled(true);
	button_AbortProcess->setEnabled(false);

	progressBar->setValue(100);
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
	
	switch(m_settings->compressionEncoder())
	{
	case SettingsModel::MP3Encoder:
		{
			MP3Encoder *mp3Encoder = new MP3Encoder();
			mp3Encoder->setBitrate(m_settings->compressionBitrate());
			mp3Encoder->setRCMode(m_settings->compressionRCMode());
			encoder = mp3Encoder;
		}
		break;
	default:
		throw "Unsupported encoder!";
	}
	
	ProcessThread *thread = new ProcessThread(currentFile, m_settings->outputDir(), encoder);
	m_threadList.append(thread);
	connect(thread, SIGNAL(finished()), this, SLOT(doneEncoding()), Qt::QueuedConnection);
	connect(thread, SIGNAL(processStateInitialized(QUuid,QString,QString,int)), m_progressModel, SLOT(addJob(QUuid,QString,QString,int)), Qt::QueuedConnection);
	connect(thread, SIGNAL(processStateChanged(QUuid,QString,int)), m_progressModel, SLOT(updateJob(QUuid,QString,int)), Qt::QueuedConnection);
	m_runningThreads++;
	thread->start();
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
	if(m_metaInfo->filePosition()) result.setFileYear(m_metaInfo->filePosition() != UINT_MAX ? m_metaInfo->filePosition() : m_currentFile);
	if(!m_metaInfo->fileComment().isEmpty()) result.setFileComment(m_metaInfo->fileComment());

	return result;
}

void ProcessingDialog::setCloseButtonEnabled(bool enabled)
{
	HMENU hMenu = GetSystemMenu((HWND) winId(), FALSE);
	EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | (enabled ? MF_ENABLED : MF_GRAYED));
}
