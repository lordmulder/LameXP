///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2023 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU GENERAL PUBLIC LICENSE as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version; always including the non-optional
// LAMEXP GNU GENERAL PUBLIC LICENSE ADDENDUM. See "License.txt" file!
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
class AudioFileModel_MetaInfo;
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
class QThreadPool;
class QElapsedTimer;
class RAMObserverThread;
class SettingsModel;
class FileExtsModel;

enum lamexp_shutdownFlag_t
{
	SHUTDOWN_FLAG_NONE = 0,
	SHUTDOWN_FLAG_POWER_OFF = 1,
	SHUTDOWN_FLAG_HIBERNATE = 2
};

//UIC forward declartion
namespace Ui
{
	class ProcessingDialog;
}

//MUtils forward declartion
namespace MUtils
{
	class Taskbar7;
}

//ProcessingDialog class
class ProcessingDialog : public QDialog
{
	Q_OBJECT

public:
	ProcessingDialog(FileListModel *const fileListModel, const AudioFileModel_MetaInfo *const metaInfo, const SettingsModel *const settings, QWidget *const parent = 0);
	~ProcessingDialog(void);
	
	int getShutdownFlag(void) { return m_shutdownFlag; }

private slots:
	void initEncoding(void);
	void initNextJob(void);
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
	virtual void resizeEvent(QResizeEvent *event);

private:
	Ui::ProcessingDialog *ui; //for Qt UIC

	QThreadPool *createThreadPool(void);
	void updateMetaInfo(AudioFileModel &audioFile);
	void writePlayList(void);
	bool shutdownComputer(void);
	
	QScopedPointer<QThreadPool> m_threadPool;
	QList<AudioFileModel> m_pendingJobs;
	const SettingsModel *const m_settings;
	const AudioFileModel_MetaInfo *const m_metaInfo;
	const QString m_tempFolder;
	QScopedPointer<QMovie> m_progressIndicator;
	QScopedPointer<ProgressModel> m_progressModel;
	QMap<QUuid,QString> m_playList;
	QScopedPointer<QMenu> m_contextMenu;
	QScopedPointer<QActionGroup> m_progressViewFilterGroup;
	QScopedPointer<QLabel> m_filterInfoLabel;
	QScopedPointer<QLabel> m_filterInfoLabelIcon;
	unsigned int m_initThreads;
	unsigned int m_runningThreads;
	unsigned int m_currentFile;
	QList<QUuid> m_allJobs;
	QList<QUuid> m_succeededJobs;
	QList<QUuid> m_failedJobs;
	QList<QUuid> m_skippedJobs;
	bool m_userAborted;
	bool m_forcedAbort;
	bool m_firstShow;
	QScopedPointer<QSystemTrayIcon> m_systemTray;
	int m_shutdownFlag;
	QScopedPointer<CPUObserverThread>  m_cpuObserver;
	QScopedPointer<RAMObserverThread>  m_ramObserver;
	QScopedPointer<DiskObserverThread> m_diskObserver;
	QScopedPointer<QElapsedTimer> m_totalTime;
	int m_progressViewFilter;
	QScopedPointer<QColor> m_defaultColor;
	QScopedPointer<FileExtsModel> m_fileExts;
	QScopedPointer<MUtils::Taskbar7> m_taskbar;
	QScopedPointer<QIcon> m_iconRunning;
	QScopedPointer<QIcon> m_iconError;
	QScopedPointer<QIcon> m_iconWarning;
	QScopedPointer<QIcon> m_iconSuccess;

	static bool isFastSeekingDevice(const QString &path);
	static quint32 cores2instances(const quint32 &cores);
	static QString time2text(const qint64 &msec);
};
