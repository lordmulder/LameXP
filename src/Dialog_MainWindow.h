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

#include <QMainWindow>

//Class declarations
class AbstractEncoder;
class AudioFileModel;
class AudioFileModel_MetaInfo;
class CustomEventFilter;
class DropBox;
class FileListModel;
class MessageHandlerThread;
class MetaInfoModel;
class QActionGroup;
class QButtonGroup;
class QFileSystemModelEx;
class QLabel;
class QMenu;
class QModelIndex;
class SettingsModel;
class WorkingBanner;

//UIC forward declartion
namespace Ui {
	class MainWindow;
}

//MainWindow class
class MainWindow: public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(FileListModel *fileListModel, AudioFileModel_MetaInfo *metaInfo, SettingsModel *settingsModel, QWidget *parent = 0);
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
	void centerOutputFolderModel(void);
	void centerOutputFolderModel_doAsync(void);
	void channelModeChanged(int value);
	void checkForBetaUpdatesActionTriggered(bool checked);
	void checkUpdatesActionActivated(void);
	void clearFilesButtonClicked(void);
	void clearMetaButtonClicked(void);
	void closeButtonClicked(void);
	void compressionTabEventOccurred(QWidget*, QEvent*);
	void customParamsChanged(void);
	void customParamsHelpRequested(QWidget *obj, QEvent *event);
	void customTempFolderChanged(const QString &text);
	void disableNeroAacNotificationsActionTriggered(bool checked);
	void disableShellIntegrationActionTriggered(bool);
	void disableSlowStartupNotificationsActionTriggered(bool checked);
	void disableSoundsActionTriggered(bool checked);
	void disableUpdateReminderActionTriggered(bool checked);
	void documentActionActivated(void);
	void editMetaButtonClicked(void);
	void encodeButtonClicked(void);
	void exportCsvContextActionTriggered(void);
	void fileDownButtonClicked(void);
	void fileUpButtonClicked(void);
	void findFileContextActionTriggered(void);
	void forceStereoDownmixEnabledChanged(bool checked);
	void gotoDesktopButtonClicked(void);
	void gotoFavoriteFolder(void);
	void gotoHomeFolderButtonClicked(void);
	void gotoMusicFolderButtonClicked(void);
	void goUpFolderContextActionTriggered(void);
	void handleDelayedFiles(void);
	void hibernateComputerActionTriggered(bool checked);
	void importCueSheetActionTriggered(bool checked);
	void importCsvContextActionTriggered(void);
	void initOutputFolderModel(void);
	void initOutputFolderModel_doAsync(void);
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
	void opusSettingsChanged(void);
	void outputFolderContextMenu(const QPoint &pos);
	void outputFolderDirectoryLoaded(const QString &path);
	void outputFolderEditFinished(void);
	void outputFolderItemExpanded(const QModelIndex &item);
	void outputFolderMouseEventOccurred(QWidget *sender, QEvent *event);
	void outputFolderViewEventOccurred(QWidget *sender, QEvent *event);
	void outputFolderRowsInserted(const QModelIndex &parent, int start, int end);
	void outputFolderViewClicked(const QModelIndex &index);
	void outputFolderViewMoved(const QModelIndex &index);
	void overwriteModeChanged(int id);
	void playlistEnabledChanged(void);
	void prependRelativePathChanged(void);
	void previewContextActionTriggered(void);
	void refreshFolderContextActionTriggered(void);
	void removeFileButtonClicked(void);
	void renameOutputEnabledChanged(bool checked);
	void renameOutputPatternChanged(void);
	void renameOutputPatternChanged(const QString &text, bool silent = false);
	void resetAdvancedOptionsButtonClicked(void);
	void restoreCursor(void);
	void samplingRateChanged(int value);
	void saveToSourceFolderChanged(void);
	void showAnnounceBox(void);
	void showCustomParamsHelpScreen(const QString &toolName, const QString &command);
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
	void updateEncoder(int id = 0);
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
	virtual void keyPressEvent(QKeyEvent *e);
	virtual bool event(QEvent *e);
	virtual bool winEvent(MSG *message, long *result);

private:
	Ui::MainWindow *ui; //for Qt UIC

	void addFiles(const QStringList &files);
	void addFolder(const QString &path, bool recursive = false, bool delayed = false);
	bool checkForUpdates(void);
	void initializeTranslation(void);
	void refreshFavorites(void);
	
	bool m_accepted;
	bool m_firstTimeShown;
	uint m_outputFolderViewInitCounter;
	bool m_outputFolderViewCentering;

	WorkingBanner *m_banner;
	QStringList *m_delayedFileList;
	QTimer *m_delayedFileTimer;
	DropBox *m_dropBox;
	QLabel *m_dropNoteLabel;
	FileListModel *m_fileListModel;
	QFileSystemModelEx *m_fileSystemModel;
	MessageHandlerThread *m_messageHandler;
	AudioFileModel_MetaInfo *const m_metaData;
	MetaInfoModel *m_metaInfoModel;
	QMenu *m_outputFolderContextMenu;
	SettingsModel *m_settings;
	QMenu *m_sourceFilesContextMenu;
	QMenu *m_outputFolderFavoritesMenu;
	QLabel *m_outputFolderNoteBox;

	QAction *m_findFileContextAction;
	QAction *m_previewContextAction;
	QAction *m_showDetailsContextAction;
	QAction *m_showFolderContextAction;
	QAction *m_refreshFolderContextAction;
	QAction *m_goUpFolderContextAction;
	QAction *m_addFavoriteFolderAction;
	QAction *m_exportCsvContextAction;
	QAction *m_importCsvContextAction;
	QActionGroup *m_languageActionGroup;
	QActionGroup *m_styleActionGroup;
	QActionGroup *m_tabActionGroup;
	QButtonGroup *m_encoderButtonGroup;
	QButtonGroup *m_modeButtonGroup;
	QButtonGroup *m_overwriteButtonGroup;

	CustomEventFilter *m_evenFilterCustumParamsHelp;
	CustomEventFilter *m_evenFilterOutputFolderMouse;
	CustomEventFilter *m_evenFilterOutputFolderView;
	CustomEventFilter *m_evenFilterCompressionTab;
};
