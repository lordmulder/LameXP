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
	void aboutButtonClicked(void);
	void addFileDelayed(const QString &filePath, bool tryASAP = false);
	void addFilesButtonClicked(void);
	void addFilesDelayed(const QStringList &filePaths, bool tryASAP = false);
	void addFavoriteFolderActionTriggered(void);
	void addFolderDelayed(const QString &folderPath, bool recursive);
	void aftenCodingModeChanged(int value);
	void aftenDRCModeChanged(int value);
	void aftenFastAllocationChanged(bool checked);
	void aftenSearchSizeChanged(int value);
	void autoDetectInstancesChanged(bool checked);
	void bitrateManagementEnabledChanged(bool checked);
	void bitrateManagementMaxChanged(int value);
	void bitrateManagementMinChanged(int value);
	void browseCustomTempFolderButtonClicked(void);
	void channelModeChanged(int value);
	void checkForBetaUpdatesActionTriggered(bool checked);
	void checkUpdatesActionActivated(void);
	void clearFilesButtonClicked(void);
	void clearMetaButtonClicked(void);
	void closeButtonClicked(void);
	void customParamsChanged(void);
	void customTempFolderChanged(const QString &text);
	void disableNeroAacNotificationsActionTriggered(bool checked);
	void disableShellIntegrationActionTriggered(bool);
	void disableSlowStartupNotificationsActionTriggered(bool checked);
	void disableSoundsActionTriggered(bool checked);
	void disableUpdateReminderActionTriggered(bool checked);
	void documentActionActivated(void);
	void editMetaButtonClicked(void);
	void encodeButtonClicked(void);
	void fileDownButtonClicked(void);
	void fileUpButtonClicked(void);
	void findFileContextActionTriggered(void);
	void forceStereoDownmixEnabledChanged(bool checked);
	void gotoDesktopButtonClicked(void);
	void gotoFavoriteFolder(void);
	void gotoHomeFolderButtonClicked(void);
	void gotoMusicFolderButtonClicked(void);
	void handleDelayedFiles(void);
	void hibernateComputerActionTriggered(bool checked);
	void importCueSheetActionTriggered(bool checked);
	void initOutputFolderModel(void);
	void languageActionActivated(QAction *action);
	void languageFromFileActionActivated(bool checked);
	void makeFolderButtonClicked(void);
	void metaTagsEnabledChanged(void);
	void neroAAC2PassChanged(bool checked);
	void neroAACProfileChanged(int value);
	void normalizationEnabledChanged(bool checked);
	void normalizationMaxVolumeChanged(double volume);
	void normalizationModeChanged(int mode);
	void notifyOtherInstance(void);
	void openFolderActionActivated(void);
	void outputFolderContextMenu(const QPoint &pos);
	void outputFolderViewClicked(const QModelIndex &index);
	void outputFolderViewMoved(const QModelIndex &index);
	void playlistEnabledChanged(void);
	void prependRelativePathChanged(void);
	void previewContextActionTriggered(void);
	void removeFileButtonClicked(void);
	void renameOutputEnabledChanged(bool checked);
	void renameOutputPatternChanged(void);
	void renameOutputPatternChanged(const QString &text);
	void resetAdvancedOptionsButtonClicked(void);
	void restoreCursor(void);
	void samplingRateChanged(int value);
	void saveToSourceFolderChanged(void);
	void showAnnounceBox(void);
	void showDetailsButtonClicked(void);
	void showDropBoxWidgetActionTriggered(bool checked);
	void showFolderContextActionTriggered(void);
	void showRenameMacros(const QString &text);
	void sourceFilesContextMenu(const QPoint &pos);
	void sourceFilesScrollbarMoved(int);
	void sourceModelChanged(void);
	void styleActionActivated(QAction *action);
	void tabActionActivated(QAction *action);
	void tabPageChanged(int idx);
	void toneAdjustBassChanged(double value);
	void toneAdjustTrebleChanged(double value);
	void toneAdjustTrebleReset(void);
	void updateBitrate(int value);
	void updateEncoder(int id);
	void updateLameAlgoQuality(int value);
	void updateMaximumInstances(int value);
	void updateRCMode(int id);
	void useCustomTempFolderChanged(bool checked);
	void visitHomepageActionActivated(void);
	void windowShown(void);

protected:
	virtual void changeEvent(QEvent *e);
	virtual void closeEvent(QCloseEvent *event);
	virtual void dragEnterEvent(QDragEnterEvent *event);
	virtual void dropEvent(QDropEvent *event);
	virtual bool eventFilter(QObject *obj, QEvent *event);
	virtual void resizeEvent(QResizeEvent *event);
	virtual void showEvent(QShowEvent *event);
	virtual bool event(QEvent *e);
	virtual bool winEvent(MSG *message, long *result);

private:
	void addFiles(const QStringList &files);
	void addFolder(const QString &path, bool recursive = false, bool delayed = false);
	bool checkForUpdates(void);
	void refreshFavorites(void);
	
	bool m_accepted;
	bool m_firstTimeShown;
	bool m_OutputFolderViewInitialized;

	const bool m_neroEncoderAvailable;
	const bool m_fhgEncoderAvailable;
	const bool m_qaacEncoderAvailable;

	WorkingBanner *m_banner;
	QStringList *m_delayedFileList;
	QTimer *m_delayedFileTimer;
	DropBox *m_dropBox;
	QLabel *m_dropNoteLabel;
	FileListModel *m_fileListModel;
	QFileSystemModelEx *m_fileSystemModel;
	MessageHandlerThread *m_messageHandler;
	AudioFileModel *m_metaData;
	MetaInfoModel *m_metaInfoModel;
	QMenu *m_outputFolderContextMenu;
	SettingsModel *m_settings;
	QMenu *m_sourceFilesContextMenu;
	QMenu *m_outputFolderFavoritesMenu;

	QAction *m_findFileContextAction;
	QAction *m_previewContextAction;
	QAction *m_showDetailsContextAction;
	QAction *m_showFolderContextAction;
	QAction *m_addFavoriteFolderAction;
	QActionGroup *m_languageActionGroup;
	QActionGroup *m_styleActionGroup;
	QActionGroup *m_tabActionGroup;
	QButtonGroup *m_encoderButtonGroup;
	QButtonGroup *m_modeButtonGroup;
};
