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

#pragma once

#include "../tmp/UIC_MainWindow.h"

//#include "Model_FileList.h"

//Class declarations
class QFileSystemModel;
class WorkingBanner;
class MessageHandlerThread;
class AudioFileModel;
class MetaInfoModel;
class SettingsModel;
class QButtonGroup;
class FileListModel;
class AbstractEncoder;

class MainWindow: public QMainWindow, private Ui::MainWindow
{
	Q_OBJECT

public:
	MainWindow(FileListModel *fileListModel, AudioFileModel *metaInfo, SettingsModel *settingsModel, QWidget *parent = 0);
	~MainWindow(void);

	bool isAccepted() { return m_accepted; }

private slots:
	void windowShown(void);
	void aboutButtonClicked(void);
	void encodeButtonClicked(void);
	void closeButtonClicked(void);
	void addFilesButtonClicked(void);
	void clearFilesButtonClicked(void);
	void removeFileButtonClicked(void);
	void fileDownButtonClicked(void);
	void fileUpButtonClicked(void);
	void showDetailsButtonClicked(void);
	void tabPageChanged(int idx);
	void tabActionActivated(QAction *action);
	void styleActionActivated(QAction *action);
	void outputFolderViewClicked(const QModelIndex &index);
	void makeFolderButtonClicked(void);
	void gotoHomeFolderButtonClicked(void);
	void gotoDesktopButtonClicked(void);
	void gotoMusicFolderButtonClicked(void);
	void checkUpdatesActionActivated(void);
	void visitHomepageActionActivated(void);
	void openFolderActionActivated(void);
	void notifyOtherInstance(void);
	void addFileDelayed(const QString &filePath);
	void handleDelayedFiles(void);
	void editMetaButtonClicked(void);
	void clearMetaButtonClicked(void);
	void updateEncoder(int id);
	void updateRCMode(int id);
	void updateBitrate(int value);
	void rowsChanged(const QModelIndex &parent, int start, int end);
	void modelReset(void);
	void metaTagsEnabledChanged(void);

protected:
	void showEvent(QShowEvent *event);
	void dragEnterEvent(QDragEnterEvent *event);
	void dropEvent(QDropEvent *event);
	void closeEvent(QCloseEvent *event);
	void resizeEvent(QResizeEvent *event);

private:
	void addFiles(const QStringList &files);

	bool m_accepted;
	FileListModel *m_fileListModel;
	QFileSystemModel *m_fileSystemModel;
	QActionGroup *m_tabActionGroup;
	QActionGroup *m_styleActionGroup;
	QButtonGroup *m_encoderButtonGroup;
	QButtonGroup *m_modeButtonGroup;
	WorkingBanner *m_banner;
	MessageHandlerThread *m_messageHandler;
	QStringList *m_delayedFileList;
	QTimer *m_delayedFileTimer;
	AudioFileModel *m_metaData;
	MetaInfoModel *m_metaInfoModel;
	SettingsModel *m_settings;
	QLabel *m_dropNoteLabel;
};
