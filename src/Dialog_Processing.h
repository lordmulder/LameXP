///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2013 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include <QDialog>
#include <QUuid>
#include <QSystemTrayIcon>
#include <QMap>

class AbstractEncoder;
class AudioFileModel;
class CPUObserverThread;
class DiskObserverThread;
class FileListModel;
class ProcessThread;
class ProgressModel;
class QActionGroup;
class QLabel;
class QMenu;
class QModelIndex;
class QMovie;
class RAMObserverThread;
class SettingsModel;
class QThreadPool;

enum shutdownFlag_t
{
	shutdownFlag_None = 0,
	shutdownFlag_TurnPowerOff = 1,
	shutdownFlag_Hibernate = 2
};

//UIC forward declartion
namespace Ui {
	class ProcessingDialog;
}

//ProcessingDialog class
class ProcessingDialog : public QDialog
{
	Q_OBJECT

public:
	ProcessingDialog(FileListModel *fileListModel, AudioFileModel *metaInfo, SettingsModel *settings, QWidget *parent = 0);
	~ProcessingDialog(void);
	
	int getShutdownFlag(void) { return m_shutdownFlag; }

private slots:
	void initEncoding(void);
	void startNextJob(void);
	void doneEncoding(void);
	void abortEncoding(bool force = false);
	void processFinished(const QUuid &jobId, const QString &outFileName, int success);
	void progressModelChanged(void);
	void logViewDoubleClicked(const QModelIndex &index);
	void logViewSectionSizeChanged(int, int, int);
	void contextMenuTriggered(const QPoint &pos);
	void contextMenuDetailsActionTriggered(void);
	void contextMenuShowFileActionTriggered(void);
	void contextMenuFilterActionTriggered(void);
	void systemTrayActivated(QSystemTrayIcon::ActivationReason reason);
	void cpuUsageHasChanged(const double val);
	void ramUsageHasChanged(const double val);
	void diskUsageHasChanged(const quint64 val);
	void progressViewFilterChanged(void);

signals:
	void abortRunningTasks(void);

protected:
	void showEvent(QShowEvent *event);
	void closeEvent(QCloseEvent *event);
	bool eventFilter(QObject *obj, QEvent *event);
	virtual bool event(QEvent *e);
	virtual bool winEvent(MSG *message, long *result);
	virtual void resizeEvent(QResizeEvent *event);

private:
	Ui::ProcessingDialog *ui; //for Qt UIC

	AudioFileModel updateMetaInfo(AudioFileModel &audioFile);
	void writePlayList(void);
	bool shutdownComputer(void);
	QString time2text(const double timeVal) const;
	
	QThreadPool *m_threadPool;
	QList<AudioFileModel> m_pendingJobs;
	SettingsModel *m_settings;
	AudioFileModel *m_metaInfo;
	QMovie *m_progressIndicator;
	ProgressModel *m_progressModel;
	QMap<QUuid,QString> m_playList;
	QMenu *m_contextMenu;
	QActionGroup *m_progressViewFilterGroup;
	QLabel *m_filterInfoLabel;
	QLabel *m_filterInfoLabelIcon;
	unsigned int m_runningThreads;
	unsigned int m_currentFile;
	QList<QUuid> m_allJobs;
	QList<QUuid> m_succeededJobs;
	QList<QUuid> m_failedJobs;
	QList<QUuid> m_skippedJobs;
	bool m_userAborted;
	bool m_forcedAbort;
	bool m_firstShow;
	QSystemTrayIcon *m_systemTray;
	int m_shutdownFlag;
	CPUObserverThread *m_cpuObserver;
	RAMObserverThread *m_ramObserver;
	DiskObserverThread *m_diskObserver;
	qint64 m_timerStart;
	int m_progressViewFilter;
};
