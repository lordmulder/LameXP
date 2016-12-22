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
class QUrl;
class QModelIndex;
class QEventLoop;
class SettingsModel;
class WorkingBanner;

//UIC forward declartion
namespace Ui
{
	class MainWindow;
}

//IPC forward declartion
namespace MUtils
{
	class IPCChannel;
}

//MainWindow class
class MainWindow: public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(MUtils::IPCChannel *const ipcChannel, FileListModel *const fileListModel, AudioFileModel_MetaInfo *const metaInfo, SettingsModel *const settingsModel, QWidget *const parent = 0);
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
	void cornerWidgetEventOccurred(QWidget *sender, QEvent *event);
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
	void fileExtAddButtonClicked();
	void fileExtModelChanged();
	void fileExtRemoveButtonClicked();
	void fileUpButtonClicked(void);
	void findFileContextActionTriggered(void);
	void forceStereoDownmixEnabledChanged(bool checked);
	void gotoDesktopButtonClicked(void);
	void gotoFavoriteFolder(void);
	void gotoHomeFolderButtonClicked(void);
	void gotoMusicFolderButtonClicked(void);
	void goUpFolderContextActionTriggered(void);
	void handleDelayedFiles(void);
	void handleDroppedFiles(void);
	void hibernateComputerActionTriggered(bool checked);
	void importCueSheetActionTriggered(bool checked);
	void importCsvContextActionTriggered(void);
	void initOutputFolderModel(void);
	void initOutputFolderModel_doAsync(void);
	void keepOriginalDateTimeChanged(bool checked);
	void languageActionActivated(QAction *action);
	void languageFromFileActionActivated(bool checked);
	void makeFolderButtonClicked(void);
	void metaTagsEnabledChanged(void);
	void neroAAC2PassChanged(bool checked);
	void neroAACProfileChanged(int value);
	void normalizationCoupledChanged(bool checked);
	void normalizationDynamicChanged(bool checked);
	void normalizationEnabledChanged(bool checked);
	void normalizationFilterSizeChanged(int value);
	void normalizationFilterSizeFinished(void);
	void normalizationMaxVolumeChanged(double volume);
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
	void renameButtonClicked(bool checked);
	void renameOutputEnabledChanged(const bool &checked);
	void renameOutputPatternChanged(void);
	void renameOutputPatternChanged(const QString &text, const bool &silent = false);
	void renameRegExpEnabledChanged(const bool &checked);
	void renameRegExpValueChanged(void);
	void renameRegExpSearchChanged (const QString &text, const bool &silent = false);
	void renameRegExpReplaceChanged(const QString &text, const bool &silent = false);
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
	void tabPageChanged(int idx, const bool silent = false);
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
	void addFolder(const QString &path, bool recursive = false, bool delayed = false, QString filter = QString());
	bool checkForUpdates(void);
	void initializeTranslation(void);
	void refreshFavorites(void);
	void openDocumentLink(QAction *const action);
	void moveSelectedFiles(const bool &up);

	void showBanner(const QString &text);
	void showBanner(const QString &text, QThread *const thread);
	void showBanner(const QString &text, QEventLoop *const eventLoop);
	void showBanner(const QString &text, bool &flag, const bool &test);

	bool m_accepted;
	bool m_firstTimeShown;
	uint m_outputFolderViewInitCounter;
	bool m_outputFolderViewCentering;

	FileListModel           *const m_fileListModel;
	AudioFileModel_MetaInfo *const m_metaData;
	SettingsModel           *const m_settings;

	QScopedPointer<WorkingBanner>      m_banner;
	QScopedPointer<MetaInfoModel>      m_metaInfoModel;
	QScopedPointer<QFileSystemModelEx> m_fileSystemModel;
	
	QScopedPointer<QList<QUrl>> m_droppedFileList;
	QScopedPointer<QStringList> m_delayedFileList;
	QScopedPointer<QTimer>      m_delayedFileTimer;
	QScopedPointer<DropBox>     m_dropBox;
	QScopedPointer<QLabel>      m_dropNoteLabel;

	QScopedPointer<MessageHandlerThread> m_messageHandler;
	QScopedPointer<QLabel>               m_outputFolderNoteBox;

	QScopedPointer<QMenu> m_outputFolderContextMenu;
	QScopedPointer<QMenu> m_sourceFilesContextMenu;
	QScopedPointer<QMenu> m_outputFolderFavoritesMenu;

	QScopedPointer<QActionGroup> m_languageActionGroup;
	QScopedPointer<QActionGroup> m_styleActionGroup;
	QScopedPointer<QActionGroup> m_tabActionGroup;
	QScopedPointer<QButtonGroup> m_encoderButtonGroup;
	QScopedPointer<QButtonGroup> m_modeButtonGroup;
	QScopedPointer<QButtonGroup> m_overwriteButtonGroup;

	QAction *m_findFileContextAction;
	QAction *m_previewContextAction;
	QAction *m_showDetailsContextAction;
	QAction *m_showFolderContextAction;
	QAction *m_refreshFolderContextAction;
	QAction *m_goUpFolderContextAction;
	QAction *m_addFavoriteFolderAction;
	QAction *m_exportCsvContextAction;
	QAction *m_importCsvContextAction;

	QScopedPointer<CustomEventFilter> m_evenFilterCornerWidget;
	QScopedPointer<CustomEventFilter> m_evenFilterCustumParamsHelp;
	QScopedPointer<CustomEventFilter> m_evenFilterOutputFolderMouse;
	QScopedPointer<CustomEventFilter> m_evenFilterOutputFolderView;
	QScopedPointer<CustomEventFilter> m_evenFilterCompressionTab;
};
