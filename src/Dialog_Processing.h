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

#pragma once

#include "../tmp/UIC_ProcessingDialog.h"

#include <QUuid>
#include <QSystemTrayIcon>

class QMovie;
class QMenu;
class ProgressModel;
class ProcessThread;
class FileListModel;
class AudioFileModel;
class SettingsModel;

class ProcessingDialog : public QDialog, private Ui::ProcessingDialog
{
	Q_OBJECT

public:
	ProcessingDialog(FileListModel *fileListModel, AudioFileModel *metaInfo, SettingsModel *settings, QWidget *parent = 0);
	~ProcessingDialog(void);
	
	bool getShutdownFlag(void) { return m_shutdownFlag; }

private slots:
	void initEncoding(void);
	void doneEncoding(void);
	void abortEncoding(void);
	void processFinished(const QUuid &jobId, const QString &outFileName, bool success);
	void progressModelChanged(void);
	void logViewDoubleClicked(const QModelIndex &index);
	void contextMenuTriggered(const QPoint &pos);
	void contextMenuDetailsActionTriggered(void);
	void contextMenuShowFileActionTriggered(void);
	void systemTrayActivated(QSystemTrayIcon::ActivationReason reason);

protected:
	void showEvent(QShowEvent *event);
	void closeEvent(QCloseEvent *event);
	bool eventFilter(QObject *obj, QEvent *event);

private:
	void setCloseButtonEnabled(bool enabled);
	void startNextJob(void);
	AudioFileModel updateMetaInfo(const AudioFileModel &audioFile);
	void writePlayList(void);
	void shutdownComputer(void);
	
	QList<AudioFileModel> m_pendingJobs;
	SettingsModel *m_settings;
	AudioFileModel *m_metaInfo;
	QList<ProcessThread*> m_threadList;
	QMovie *m_progressIndicator;
	ProgressModel *m_progressModel;
	QMap<QUuid,QString> m_playList;
	QMenu *m_contextMenu;
	unsigned int m_runningThreads;
	unsigned int m_currentFile;
	QList<QUuid> m_allJobs;
	QList<QUuid> m_succeededJobs;
	QList<QUuid> m_failedJobs;
	bool m_userAborted;
	QSystemTrayIcon *m_systemTray;
	bool m_shutdownFlag;
};
