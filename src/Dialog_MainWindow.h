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

#include "../tmp/UIC_MainWindow.h"

//Class declarations
class QFileSystemModelEx;
class WorkingBanner;
class MessageHandlerThread;
class AudioFileModel;
class MetaInfoModel;
class SettingsModel;
class QButtonGroup;
class FileListModel;
class AbstractEncoder;
class QMenu;
class DropBox;

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
	void languageActionActivated(QAction *action);
	void languageFromFileActionActivated(bool checked);
	void outputFolderViewClicked(const QModelIndex &index);
	void outputFolderViewMoved(const QModelIndex &index);
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
	void sourceModelChanged(void);
	void metaTagsEnabledChanged(void);
	void playlistEnabledChanged(void);
	void saveToSourceFolderChanged(void);
	void prependRelativePathChanged(void);
	void restoreCursor(void);
	void sourceFilesContextMenu(const QPoint &pos);
	void previewContextActionTriggered(void);
	void findFileContextActionTriggered(void);
	void disableUpdateReminderActionTriggered(bool checked);
	void disableSoundsActionTriggered(bool checked);
	void outputFolderContextMenu(const QPoint &pos);
	void showFolderContextActionTriggered(void);
	void installWMADecoderActionTriggered(bool checked);
	void disableNeroAacNotificationsActionTriggered(bool checked);
	void disableWmaDecoderNotificationsActionTriggered(bool checked);
	void showDropBoxWidgetActionTriggered(bool checked);

protected:
	void showEvent(QShowEvent *event);
	void dragEnterEvent(QDragEnterEvent *event);
	void dropEvent(QDropEvent *event);
	void closeEvent(QCloseEvent *event);
	void resizeEvent(QResizeEvent *event);
	bool eventFilter(QObject *obj, QEvent *event);
	void changeEvent(QEvent *e);

private:
	void addFiles(const QStringList &files);

	bool m_accepted;
	bool m_firstTimeShown;
	FileListModel *m_fileListModel;
	QFileSystemModelEx *m_fileSystemModel;
	QActionGroup *m_tabActionGroup;
	QActionGroup *m_styleActionGroup;
	QActionGroup *m_languageActionGroup;
	QButtonGroup *m_encoderButtonGroup;
	QButtonGroup *m_modeButtonGroup;
	QAction *m_showDetailsContextAction;
	QAction *m_previewContextAction;
	QAction *m_findFileContextAction;
	QAction *m_showFolderContextAction;
	WorkingBanner *m_banner;
	MessageHandlerThread *m_messageHandler;
	QStringList *m_delayedFileList;
	QTimer *m_delayedFileTimer;
	AudioFileModel *m_metaData;
	MetaInfoModel *m_metaInfoModel;
	SettingsModel *m_settings;
	QLabel *m_dropNoteLabel;
	QMenu *m_sourceFilesContextMenu;
	QMenu *m_outputFolderContextMenu;
	DropBox *m_dropBox;
};
