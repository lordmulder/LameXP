///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2025 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Dialog_MainWindow.h"

//UIC includes
#include "UIC_MainWindow.h"

//LameXP includes
#include "Global.h"
#include "Dialog_WorkingBanner.h"
#include "Dialog_MetaInfo.h"
#include "Dialog_About.h"
#include "Dialog_Update.h"
#include "Dialog_DropBox.h"
#include "Dialog_CueImport.h"
#include "Dialog_LogView.h"
#include "Thread_FileAnalyzer.h"
#include "Thread_MessageHandler.h"
#include "Model_MetaInfo.h"
#include "Model_Settings.h"
#include "Model_FileList.h"
#include "Model_FileSystem.h"
#include "Model_FileExts.h"
#include "Registry_Encoder.h"
#include "Registry_Decoder.h"
#include "Encoder_Abstract.h"
#include "ShellIntegration.h"
#include "CustomEventFilter.h"

//Mutils includes
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>
#include <MUtils/GUI.h>
#include <MUtils/Exception.h>
#include <MUtils/Sound.h>
#include <MUtils/Translation.h>
#include <MUtils/Version.h>

//Qt includes
#include <QMessageBox>
#include <QTimer>
#include <QDesktopWidget>
#include <QDate>
#include <QFileDialog>
#include <QInputDialog>
#include <QFileSystemModel>
#include <QDesktopServices>
#include <QUrl>
#include <QPlastiqueStyle>
#include <QCleanlooksStyle>
#include <QWindowsVistaStyle>
#include <QWindowsStyle>
#include <QSysInfo>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QProcess>
#include <QUuid>
#include <QProcessEnvironment>
#include <QCryptographicHash>
#include <QTranslator>
#include <QResource>
#include <QScrollBar>

////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////

static const unsigned int IDM_ABOUTBOX = 0xEFF0;
static const char *g_hydrogen_audio_url = "http://wiki.hydrogenaud.io/index.php?title=Main_Page";
static const char *g_documents_base_url = "http://lamexp.sourceforge.net/doc";

////////////////////////////////////////////////////////////
// Helper Macros
////////////////////////////////////////////////////////////

#define BANNER_VISIBLE ((!m_banner.isNull()) && m_banner->isVisible())

#define INIT_BANNER() do \
{ \
	if(m_banner.isNull()) \
	{ \
		m_banner.reset(new WorkingBanner(this)); \
	} \
} \
while(0)

#define ABORT_IF_BUSY do \
{ \
	if(BANNER_VISIBLE || m_delayedFileTimer->isActive() || (QApplication::activeModalWidget() != NULL)) \
	{ \
		MUtils::Sound::beep(MUtils::Sound::BEEP_WRN); \
		return; \
	} \
} \
while(0)

#define PLAY_SOUND_OPTIONAL(NAME, ASYNC) do \
{ \
	if(m_settings->soundsEnabled()) MUtils::Sound::play_sound((NAME), (ASYNC)); \
} \
while(0)

#define SHOW_CORNER_WIDGET(FLAG) do \
{ \
	if(QWidget *cornerWidget = ui->menubar->cornerWidget()) \
	{ \
		cornerWidget->setVisible((FLAG)); \
	} \
} \
while(0)

#define LINK(URL) \
	(QString("<a href=\"%1\">%2</a>").arg(URL).arg(QString(URL).replace("-", "&minus;")))

#define LINK_EX(URL, NAME) \
	(QString("<a href=\"%1\">%2</a>").arg(URL).arg(QString(NAME).replace("-", "&minus;")))

#define FSLINK(PATH) \
	(QString("<a href=\"file:///%1\">%2</a>").arg(PATH).arg(QString(PATH).replace("-", "&minus;")))

#define CENTER_CURRENT_OUTPUT_FOLDER_DELAYED() \
	QTimer::singleShot(125, this, SLOT(centerOutputFolderModel()))

////////////////////////////////////////////////////////////
// Static Functions
////////////////////////////////////////////////////////////

static inline void SET_TEXT_COLOR(QWidget *const widget, const QColor &color)
{
	QPalette _palette = widget->palette();
	_palette.setColor(QPalette::WindowText, (color));
	_palette.setColor(QPalette::Text, (color));
	widget->setPalette(_palette);
}

static inline void SET_FONT_BOLD(QWidget *const widget, const bool &bold)
{ 
	QFont _font = widget->font();
	_font.setBold(bold);
	widget->setFont(_font);
}

static inline void SET_FONT_BOLD(QAction *const widget, const bool &bold)
{ 
	QFont _font = widget->font();
	_font.setBold(bold);
	widget->setFont(_font);
}

static inline void SET_MODEL(QAbstractItemView *const view, QAbstractItemModel *const model)
{
	QItemSelectionModel *_tmp = view->selectionModel();
	view->setModel(model);
	MUTILS_DELETE(_tmp);
}

static inline void SET_CHECKBOX_STATE(QCheckBox *const chckbx, const bool &state)
{
	const bool isDisabled = (!chckbx->isEnabled());
	if(isDisabled)
	{
		chckbx->setEnabled(true);
	}
	if(chckbx->isChecked() != state)
	{
		chckbx->click();
	}
	if(chckbx->isChecked() != state)
	{
		qWarning("Warning: Failed to set checkbox %p state!", chckbx);
	}
	if(isDisabled)
	{
		chckbx->setEnabled(false);
	}
}

static inline void TRIM_STRING_RIGHT(QString &str)
{
	while((str.length() > 0) && str[str.length()-1].isSpace())
	{
		str.chop(1);
	}
}

static inline void MAKE_TRANSPARENT(QWidget *const widget, const bool &flag)
{
	QPalette _p = widget->palette(); \
	_p.setColor(QPalette::Background, Qt::transparent);
	widget->setPalette(flag ? _p : QPalette());
}

template <typename T>
static QList<T> INVERT_LIST(QList<T> list)
{
	if(!list.isEmpty())
	{
		const int limit = list.size() / 2, maxIdx = list.size() - 1;
		for(int k = 0; k < limit; k++) list.swap(k, maxIdx - k);
	}
	return list;
}

static quint32 encodeInstances(quint32 instances)
{
	if (instances > 16U)
	{
		instances -= (instances - 16U) / 2U;
		if (instances > 24U)
		{
			instances -= (instances - 24U) / 2U;
		}
	}
	return instances;
}

static quint32 decodeInstances(quint32 instances)
{
	if (instances > 16U)
	{
		instances += instances - 16U;
		if (instances > 32U)
		{
			instances += instances - 32U;
		}
	}
	return instances;
}

////////////////////////////////////////////////////////////
// Helper Classes
////////////////////////////////////////////////////////////

class WidgetHideHelper
{
public:
	WidgetHideHelper(QWidget *const widget) : m_widget(widget), m_visible(widget && widget->isVisible()) { if(m_widget && m_visible) m_widget->hide(); }
	~WidgetHideHelper(void)                                                                              { if(m_widget && m_visible) m_widget->show(); }
private:
	QWidget *const m_widget;
	const bool m_visible;
};

class SignalBlockHelper
{
public:
	SignalBlockHelper(QObject *const object) : m_object(object), m_flag(object && object->blockSignals(true)) {}
	~SignalBlockHelper(void) { if(m_object && (!m_flag)) m_object->blockSignals(false); }
private:
	QObject *const m_object;
	const bool m_flag;
};

class FileListBlockHelper
{
public:
	FileListBlockHelper(FileListModel *const fileList) : m_fileList(fileList) { if(m_fileList) m_fileList->setBlockUpdates(true);  }
	~FileListBlockHelper(void)                                                { if(m_fileList) m_fileList->setBlockUpdates(false); }
private:
	FileListModel *const m_fileList;
};

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

MainWindow::MainWindow(MUtils::IPCChannel *const ipcChannel, FileListModel *const fileListModel, AudioFileModel_MetaInfo *const metaInfo, SettingsModel *const settingsModel, QWidget *const parent)
:
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	m_fileListModel(fileListModel),
	m_metaData(metaInfo),
	m_settings(settingsModel),
	m_accepted(false),
	m_firstTimeShown(true),
	m_outputFolderViewCentering(false),
	m_outputFolderViewInitCounter(0)
{
	//Init the dialog, from the .ui file
	ui->setupUi(this);
	setWindowFlags(windowFlags() ^ Qt::WindowMaximizeButtonHint);
	setMinimumSize(this->size());

	//Create window icon
	MUtils::GUI::set_window_icon(this, lamexp_app_icon(), true);

	//Register meta types
	qRegisterMetaType<AudioFileModel>("AudioFileModel");

	//Enabled main buttons
	connect(ui->buttonAbout, SIGNAL(clicked()), this, SLOT(aboutButtonClicked()));
	connect(ui->buttonStart, SIGNAL(clicked()), this, SLOT(encodeButtonClicked()));
	connect(ui->buttonQuit,  SIGNAL(clicked()), this, SLOT(closeButtonClicked()));

	//Setup tab widget
	ui->tabWidget->setCurrentIndex(0);
	connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabPageChanged(int)));

	//Add system menu
	MUtils::GUI::sysmenu_append(this, IDM_ABOUTBOX, "About...");

	//Setup corner widget
	QLabel *cornerWidget = new QLabel(ui->menubar);
	m_evenFilterCornerWidget.reset(new CustomEventFilter);
	cornerWidget->setText("N/A");
	cornerWidget->setFixedHeight(ui->menubar->height());
	cornerWidget->setCursor(QCursor(Qt::PointingHandCursor));
	cornerWidget->hide();
	cornerWidget->installEventFilter(m_evenFilterCornerWidget.data());
	connect(m_evenFilterCornerWidget.data(), SIGNAL(eventOccurred(QWidget*, QEvent*)), this, SLOT(cornerWidgetEventOccurred(QWidget*, QEvent*)));
	ui->menubar->setCornerWidget(cornerWidget);

	//--------------------------------
	// Setup "Source" tab
	//--------------------------------

	ui->sourceFileView->setModel(m_fileListModel);
	ui->sourceFileView->verticalHeader()  ->setResizeMode(QHeaderView::ResizeToContents);
	ui->sourceFileView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	ui->sourceFileView->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->sourceFileView->viewport()->installEventFilter(this);
	m_dropNoteLabel.reset(new QLabel(ui->sourceFileView));
	m_dropNoteLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	SET_FONT_BOLD(m_dropNoteLabel.data(), true);
	SET_TEXT_COLOR(m_dropNoteLabel.data(), Qt::darkGray);
	m_sourceFilesContextMenu.reset(new QMenu());
	m_showDetailsContextAction = m_sourceFilesContextMenu->addAction(QIcon(":/icons/zoom.png"),         "N/A");
	m_previewContextAction     = m_sourceFilesContextMenu->addAction(QIcon(":/icons/sound.png"),        "N/A");
	m_findFileContextAction    = m_sourceFilesContextMenu->addAction(QIcon(":/icons/folder_go.png"),    "N/A");
	m_sourceFilesContextMenu->addSeparator();
	m_exportCsvContextAction   = m_sourceFilesContextMenu->addAction(QIcon(":/icons/table_save.png"),   "N/A");
	m_importCsvContextAction   = m_sourceFilesContextMenu->addAction(QIcon(":/icons/folder_table.png"), "N/A");
	SET_FONT_BOLD(m_showDetailsContextAction, true);

	connect(ui->buttonAddFiles,                      SIGNAL(clicked()),                          this, SLOT(addFilesButtonClicked()));
	connect(ui->buttonRemoveFile,                    SIGNAL(clicked()),                          this, SLOT(removeFileButtonClicked()));
	connect(ui->buttonClearFiles,                    SIGNAL(clicked()),                          this, SLOT(clearFilesButtonClicked()));
	connect(ui->buttonFileUp,                        SIGNAL(clicked()),                          this, SLOT(fileUpButtonClicked()));
	connect(ui->buttonFileDown,                      SIGNAL(clicked()),                          this, SLOT(fileDownButtonClicked()));
	connect(ui->buttonShowDetails,                   SIGNAL(clicked()),                          this, SLOT(showDetailsButtonClicked()));
	connect(m_fileListModel,                         SIGNAL(rowsInserted(QModelIndex,int,int)),  this, SLOT(sourceModelChanged()));
	connect(m_fileListModel,                         SIGNAL(rowsRemoved(QModelIndex,int,int)),   this, SLOT(sourceModelChanged()));
	connect(m_fileListModel,                         SIGNAL(modelReset()),                       this, SLOT(sourceModelChanged()));
	connect(ui->sourceFileView,                      SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(sourceFilesContextMenu(QPoint)));
	connect(ui->sourceFileView->verticalScrollBar(), SIGNAL(sliderMoved(int)),                   this, SLOT(sourceFilesScrollbarMoved(int)));
	connect(ui->sourceFileView->verticalScrollBar(), SIGNAL(valueChanged(int)),                  this, SLOT(sourceFilesScrollbarMoved(int)));
	connect(m_showDetailsContextAction,              SIGNAL(triggered(bool)),                    this, SLOT(showDetailsButtonClicked()));
	connect(m_previewContextAction,                  SIGNAL(triggered(bool)),                    this, SLOT(previewContextActionTriggered()));
	connect(m_findFileContextAction,                 SIGNAL(triggered(bool)),                    this, SLOT(findFileContextActionTriggered()));
	connect(m_exportCsvContextAction,                SIGNAL(triggered(bool)),                    this, SLOT(exportCsvContextActionTriggered()));
	connect(m_importCsvContextAction,                SIGNAL(triggered(bool)),                    this, SLOT(importCsvContextActionTriggered()));

	//--------------------------------
	// Setup "Output" tab
	//--------------------------------

	ui->outputFolderView->setHeaderHidden(true);
	ui->outputFolderView->setAnimated(false);
	ui->outputFolderView->setMouseTracking(false);
	ui->outputFolderView->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->outputFolderView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	m_evenFilterOutputFolderMouse.reset(new CustomEventFilter);
	ui->outputFoldersGoUpLabel     ->installEventFilter(m_evenFilterOutputFolderMouse.data());
	ui->outputFoldersEditorLabel   ->installEventFilter(m_evenFilterOutputFolderMouse.data());
	ui->outputFoldersFovoritesLabel->installEventFilter(m_evenFilterOutputFolderMouse.data());
	ui->outputFolderLabel          ->installEventFilter(m_evenFilterOutputFolderMouse.data());

	m_evenFilterOutputFolderView.reset(new CustomEventFilter);
	ui->outputFolderView->installEventFilter(m_evenFilterOutputFolderView.data());

	SET_CHECKBOX_STATE(ui->saveToSourceFolderCheckBox, m_settings->outputToSourceDir());
	ui->prependRelativePathCheckBox->setChecked(m_settings->prependRelativeSourcePath());
	
	connect(ui->outputFolderView,                 SIGNAL(clicked(QModelIndex)),             this, SLOT(outputFolderViewClicked(QModelIndex)));
	connect(ui->outputFolderView,                 SIGNAL(activated(QModelIndex)),           this, SLOT(outputFolderViewClicked(QModelIndex)));
	connect(ui->outputFolderView,                 SIGNAL(pressed(QModelIndex)),             this, SLOT(outputFolderViewClicked(QModelIndex)));
	connect(ui->outputFolderView,                 SIGNAL(entered(QModelIndex)),             this, SLOT(outputFolderViewMoved(QModelIndex)));
	connect(ui->outputFolderView,                 SIGNAL(expanded(QModelIndex)),            this, SLOT(outputFolderItemExpanded(QModelIndex)));
	connect(ui->buttonMakeFolder,                 SIGNAL(clicked()),                        this, SLOT(makeFolderButtonClicked()));
	connect(ui->buttonGotoHome,                   SIGNAL(clicked()),                        this, SLOT(gotoHomeFolderButtonClicked()));
	connect(ui->buttonGotoDesktop,                SIGNAL(clicked()),                        this, SLOT(gotoDesktopButtonClicked()));
	connect(ui->buttonGotoMusic,                  SIGNAL(clicked()),                        this, SLOT(gotoMusicFolderButtonClicked()));
	connect(ui->saveToSourceFolderCheckBox,       SIGNAL(clicked()),                        this, SLOT(saveToSourceFolderChanged()));
	connect(ui->prependRelativePathCheckBox,      SIGNAL(clicked()),                        this, SLOT(prependRelativePathChanged()));
	connect(ui->outputFolderEdit,                 SIGNAL(editingFinished()),                this, SLOT(outputFolderEditFinished()));
	connect(m_evenFilterOutputFolderMouse.data(), SIGNAL(eventOccurred(QWidget*, QEvent*)), this, SLOT(outputFolderMouseEventOccurred(QWidget*, QEvent*)));
	connect(m_evenFilterOutputFolderView.data(),  SIGNAL(eventOccurred(QWidget*, QEvent*)), this, SLOT(outputFolderViewEventOccurred(QWidget*, QEvent*)));

	m_outputFolderContextMenu.reset(new QMenu());
	m_showFolderContextAction    = m_outputFolderContextMenu->addAction(QIcon(":/icons/zoom.png"),          "N/A");
	m_goUpFolderContextAction    = m_outputFolderContextMenu->addAction(QIcon(":/icons/folder_up.png"),     "N/A");
	m_outputFolderContextMenu->addSeparator();
	m_refreshFolderContextAction = m_outputFolderContextMenu->addAction(QIcon(":/icons/arrow_refresh.png"), "N/A");
	m_outputFolderContextMenu->setDefaultAction(m_showFolderContextAction);
	connect(ui->outputFolderView,         SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(outputFolderContextMenu(QPoint)));
	connect(m_showFolderContextAction,    SIGNAL(triggered(bool)),                    this, SLOT(showFolderContextActionTriggered()));
	connect(m_refreshFolderContextAction, SIGNAL(triggered(bool)),                    this, SLOT(refreshFolderContextActionTriggered()));
	connect(m_goUpFolderContextAction,    SIGNAL(triggered(bool)),                    this, SLOT(goUpFolderContextActionTriggered()));

	m_outputFolderFavoritesMenu.reset(new QMenu());
	m_addFavoriteFolderAction = m_outputFolderFavoritesMenu->addAction(QIcon(":/icons/add.png"), "N/A");
	m_outputFolderFavoritesMenu->insertSeparator(m_addFavoriteFolderAction);
	connect(m_addFavoriteFolderAction, SIGNAL(triggered(bool)), this, SLOT(addFavoriteFolderActionTriggered()));

	ui->outputFolderEdit->setVisible(false);
	m_outputFolderNoteBox.reset(new QLabel(ui->outputFolderView));
	m_outputFolderNoteBox->setAutoFillBackground(true);
	m_outputFolderNoteBox->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	m_outputFolderNoteBox->setFrameShape(QFrame::StyledPanel);
	SET_FONT_BOLD(m_outputFolderNoteBox.data(), true);
	m_outputFolderNoteBox->hide();

	outputFolderViewClicked(QModelIndex());
	refreshFavorites();
	
	//--------------------------------
	// Setup "Meta Data" tab
	//--------------------------------

	m_metaInfoModel.reset(new MetaInfoModel(m_metaData));
	m_metaInfoModel->clearData();
	m_metaInfoModel->setData(m_metaInfoModel->index(4, 1), m_settings->metaInfoPosition());
	ui->metaDataView->setModel(m_metaInfoModel.data());
	ui->metaDataView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	ui->metaDataView->verticalHeader()->hide();
	ui->metaDataView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	SET_CHECKBOX_STATE(ui->writeMetaDataCheckBox, m_settings->writeMetaTags());
	ui->generatePlaylistCheckBox->setChecked(m_settings->createPlaylist());
	connect(ui->buttonEditMeta,           SIGNAL(clicked()), this, SLOT(editMetaButtonClicked()));
	connect(ui->buttonClearMeta,          SIGNAL(clicked()), this, SLOT(clearMetaButtonClicked()));
	connect(ui->writeMetaDataCheckBox,    SIGNAL(clicked()), this, SLOT(metaTagsEnabledChanged()));
	connect(ui->generatePlaylistCheckBox, SIGNAL(clicked()), this, SLOT(playlistEnabledChanged()));

	//--------------------------------
	//Setup "Compression" tab
	//--------------------------------

	m_encoderButtonGroup.reset(new QButtonGroup(this));
	m_encoderButtonGroup->addButton(ui->radioButtonEncoderMP3,    SettingsModel::MP3Encoder);
	m_encoderButtonGroup->addButton(ui->radioButtonEncoderVorbis, SettingsModel::VorbisEncoder);
	m_encoderButtonGroup->addButton(ui->radioButtonEncoderAAC,    SettingsModel::AACEncoder);
	m_encoderButtonGroup->addButton(ui->radioButtonEncoderAC3,    SettingsModel::AC3Encoder);
	m_encoderButtonGroup->addButton(ui->radioButtonEncoderFLAC,   SettingsModel::FLACEncoder);
	m_encoderButtonGroup->addButton(ui->radioButtonEncoderAPE,    SettingsModel::MACEncoder);
	m_encoderButtonGroup->addButton(ui->radioButtonEncoderOpus,   SettingsModel::OpusEncoder);
	m_encoderButtonGroup->addButton(ui->radioButtonEncoderDCA,    SettingsModel::DCAEncoder);
	m_encoderButtonGroup->addButton(ui->radioButtonEncoderPCM,    SettingsModel::PCMEncoder);

	const int aacEncoder = EncoderRegistry::getAacEncoder();
	ui->radioButtonEncoderAAC->setEnabled(aacEncoder > SettingsModel::AAC_ENCODER_NONE);

	m_modeButtonGroup.reset(new QButtonGroup(this));
	m_modeButtonGroup->addButton(ui->radioButtonModeQuality,        SettingsModel::VBRMode);
	m_modeButtonGroup->addButton(ui->radioButtonModeAverageBitrate, SettingsModel::ABRMode);
	m_modeButtonGroup->addButton(ui->radioButtonConstBitrate,       SettingsModel::CBRMode);

	ui->radioButtonEncoderMP3->setChecked(true);
	foreach(QAbstractButton *currentButton, m_encoderButtonGroup->buttons())
	{
		if(currentButton->isEnabled() && (m_encoderButtonGroup->id(currentButton) == m_settings->compressionEncoder()))
		{
			currentButton->setChecked(true);
			break;
		}
	}

	m_evenFilterCompressionTab.reset(new CustomEventFilter());
	ui->labelCompressionHelp->installEventFilter(m_evenFilterCompressionTab.data());
	ui->labelResetEncoders  ->installEventFilter(m_evenFilterCompressionTab.data());

	connect(m_encoderButtonGroup.data(),       SIGNAL(buttonClicked(int)),               this, SLOT(updateEncoder(int)));
	connect(m_modeButtonGroup.data(),          SIGNAL(buttonClicked(int)),               this, SLOT(updateRCMode(int)));
	connect(m_evenFilterCompressionTab.data(), SIGNAL(eventOccurred(QWidget*, QEvent*)), this, SLOT(compressionTabEventOccurred(QWidget*, QEvent*)));
	connect(ui->sliderBitrate,                 SIGNAL(valueChanged(int)),                this, SLOT(updateBitrate(int)));

	updateEncoder(m_encoderButtonGroup->checkedId());

	//--------------------------------
	//Setup "Advanced Options" tab
	//--------------------------------

	ui->sliderLameAlgoQuality->setValue(m_settings->lameAlgoQuality());
	if (m_settings->maximumInstances() > 0U)
	{
		ui->sliderMaxInstances->setValue(static_cast<int>(encodeInstances(m_settings->maximumInstances())));
	}

	ui->spinBoxBitrateManagementMin   ->setValue(m_settings->bitrateManagementMinRate());
	ui->spinBoxBitrateManagementMax   ->setValue(m_settings->bitrateManagementMaxRate());
	ui->spinBoxNormalizationFilterPeak->setValue(static_cast<double>(m_settings->normalizationFilterMaxVolume()) / 100.0);
	ui->spinBoxNormalizationFilterSize->setValue(m_settings->normalizationFilterSize());
	ui->spinBoxToneAdjustBass         ->setValue(static_cast<double>(m_settings->toneAdjustBass()) / 100.0);
	ui->spinBoxToneAdjustTreble       ->setValue(static_cast<double>(m_settings->toneAdjustTreble()) / 100.0);
	ui->spinBoxAftenSearchSize        ->setValue(m_settings->aftenExponentSearchSize());
	ui->spinBoxOpusComplexity         ->setValue(m_settings->opusComplexity());
	
	ui->comboBoxMP3ChannelMode   ->setCurrentIndex(m_settings->lameChannelMode());
	ui->comboBoxSamplingRate     ->setCurrentIndex(m_settings->samplingRate());
	ui->comboBoxAACProfile       ->setCurrentIndex(m_settings->aacEncProfile());
	ui->comboBoxAftenCodingMode  ->setCurrentIndex(m_settings->aftenAudioCodingMode());
	ui->comboBoxAftenDRCMode     ->setCurrentIndex(m_settings->aftenDynamicRangeCompression());
	ui->comboBoxOpusFramesize    ->setCurrentIndex(m_settings->opusFramesize());
	
	SET_CHECKBOX_STATE(ui->checkBoxBitrateManagement,          m_settings->bitrateManagementEnabled());
	SET_CHECKBOX_STATE(ui->checkBoxNeroAAC2PassMode,           m_settings->neroAACEnable2Pass());
	SET_CHECKBOX_STATE(ui->checkBoxAftenFastAllocation,        m_settings->aftenFastBitAllocation());
	SET_CHECKBOX_STATE(ui->checkBoxNormalizationFilterEnabled, m_settings->normalizationFilterEnabled());
	SET_CHECKBOX_STATE(ui->checkBoxNormalizationFilterDynamic, m_settings->normalizationFilterDynamic());
	SET_CHECKBOX_STATE(ui->checkBoxNormalizationFilterCoupled, m_settings->normalizationFilterCoupled());
	SET_CHECKBOX_STATE(ui->checkBoxAutoDetectInstances,        (m_settings->maximumInstances() < 1));
	SET_CHECKBOX_STATE(ui->checkBoxUseSystemTempFolder,        (!m_settings->customTempPathEnabled()));
	SET_CHECKBOX_STATE(ui->checkBoxRename_Rename,              m_settings->renameFiles_renameEnabled());
	SET_CHECKBOX_STATE(ui->checkBoxRename_RegExp,              m_settings->renameFiles_regExpEnabled());
	SET_CHECKBOX_STATE(ui->checkBoxForceStereoDownmix,         m_settings->forceStereoDownmix());
	SET_CHECKBOX_STATE(ui->checkBoxOpusDisableResample,        m_settings->opusDisableResample());
	SET_CHECKBOX_STATE(ui->checkBoxKeepOriginalDateTime,       m_settings->keepOriginalDataTime());

	ui->checkBoxNeroAAC2PassMode->setEnabled(aacEncoder == SettingsModel::AAC_ENCODER_NERO);
	
	ui->lineEditCustomParamLAME     ->setText(EncoderRegistry::loadEncoderCustomParams(m_settings, SettingsModel::MP3Encoder));
	ui->lineEditCustomParamOggEnc   ->setText(EncoderRegistry::loadEncoderCustomParams(m_settings, SettingsModel::VorbisEncoder));
	ui->lineEditCustomParamNeroAAC  ->setText(EncoderRegistry::loadEncoderCustomParams(m_settings, SettingsModel::AACEncoder));
	ui->lineEditCustomParamFLAC     ->setText(EncoderRegistry::loadEncoderCustomParams(m_settings, SettingsModel::FLACEncoder));
	ui->lineEditCustomParamAften    ->setText(EncoderRegistry::loadEncoderCustomParams(m_settings, SettingsModel::AC3Encoder));
	ui->lineEditCustomParamOpus     ->setText(EncoderRegistry::loadEncoderCustomParams(m_settings, SettingsModel::OpusEncoder));
	ui->lineEditCustomTempFolder    ->setText(QDir::toNativeSeparators(m_settings->customTempPath()));
	ui->lineEditRenamePattern       ->setText(m_settings->renameFiles_renamePattern());
	ui->lineEditRenameRegExp_Search ->setText(m_settings->renameFiles_regExpSearch ());
	ui->lineEditRenameRegExp_Replace->setText(m_settings->renameFiles_regExpReplace());
	
	m_evenFilterCustumParamsHelp.reset(new CustomEventFilter());
	ui->helpCustomParamLAME   ->installEventFilter(m_evenFilterCustumParamsHelp.data());
	ui->helpCustomParamOggEnc ->installEventFilter(m_evenFilterCustumParamsHelp.data());
	ui->helpCustomParamNeroAAC->installEventFilter(m_evenFilterCustumParamsHelp.data());
	ui->helpCustomParamFLAC   ->installEventFilter(m_evenFilterCustumParamsHelp.data());
	ui->helpCustomParamAften  ->installEventFilter(m_evenFilterCustumParamsHelp.data());
	ui->helpCustomParamOpus   ->installEventFilter(m_evenFilterCustumParamsHelp.data());
	
	m_overwriteButtonGroup.reset(new QButtonGroup(this));
	m_overwriteButtonGroup->addButton(ui->radioButtonOverwriteModeKeepBoth, SettingsModel::Overwrite_KeepBoth);
	m_overwriteButtonGroup->addButton(ui->radioButtonOverwriteModeSkipFile, SettingsModel::Overwrite_SkipFile);
	m_overwriteButtonGroup->addButton(ui->radioButtonOverwriteModeReplaces, SettingsModel::Overwrite_Replaces);

	ui->radioButtonOverwriteModeKeepBoth->setChecked(m_settings->overwriteMode() == SettingsModel::Overwrite_KeepBoth);
	ui->radioButtonOverwriteModeSkipFile->setChecked(m_settings->overwriteMode() == SettingsModel::Overwrite_SkipFile);
	ui->radioButtonOverwriteModeReplaces->setChecked(m_settings->overwriteMode() == SettingsModel::Overwrite_Replaces);

	FileExtsModel *fileExtModel = new FileExtsModel(this);
	fileExtModel->importItems(m_settings->renameFiles_fileExtension());
	ui->tableViewFileExts->setModel(fileExtModel);
	ui->tableViewFileExts->verticalHeader()  ->setResizeMode(QHeaderView::ResizeToContents);
	ui->tableViewFileExts->horizontalHeader()->setResizeMode(QHeaderView::Stretch);

	connect(ui->sliderLameAlgoQuality,              SIGNAL(valueChanged(int)),                this, SLOT(updateLameAlgoQuality(int)));
	connect(ui->checkBoxBitrateManagement,          SIGNAL(clicked(bool)),                    this, SLOT(bitrateManagementEnabledChanged(bool)));
	connect(ui->spinBoxBitrateManagementMin,        SIGNAL(valueChanged(int)),                this, SLOT(bitrateManagementMinChanged(int)));
	connect(ui->spinBoxBitrateManagementMax,        SIGNAL(valueChanged(int)),                this, SLOT(bitrateManagementMaxChanged(int)));
	connect(ui->comboBoxMP3ChannelMode,             SIGNAL(currentIndexChanged(int)),         this, SLOT(channelModeChanged(int)));
	connect(ui->comboBoxSamplingRate,               SIGNAL(currentIndexChanged(int)),         this, SLOT(samplingRateChanged(int)));
	connect(ui->checkBoxNeroAAC2PassMode,           SIGNAL(clicked(bool)),                    this, SLOT(neroAAC2PassChanged(bool)));
	connect(ui->comboBoxAACProfile,                 SIGNAL(currentIndexChanged(int)),         this, SLOT(neroAACProfileChanged(int)));
	connect(ui->checkBoxNormalizationFilterEnabled, SIGNAL(clicked(bool)),                    this, SLOT(normalizationEnabledChanged(bool)));
	connect(ui->checkBoxNormalizationFilterDynamic, SIGNAL(clicked(bool)),                    this, SLOT(normalizationDynamicChanged(bool)));
	connect(ui->checkBoxNormalizationFilterCoupled, SIGNAL(clicked(bool)),                    this, SLOT(normalizationCoupledChanged(bool)));
	connect(ui->comboBoxAftenCodingMode,            SIGNAL(currentIndexChanged(int)),         this, SLOT(aftenCodingModeChanged(int)));
	connect(ui->comboBoxAftenDRCMode,               SIGNAL(currentIndexChanged(int)),         this, SLOT(aftenDRCModeChanged(int)));
	connect(ui->spinBoxAftenSearchSize,             SIGNAL(valueChanged(int)),                this, SLOT(aftenSearchSizeChanged(int)));
	connect(ui->checkBoxAftenFastAllocation,        SIGNAL(clicked(bool)),                    this, SLOT(aftenFastAllocationChanged(bool)));
	connect(ui->spinBoxNormalizationFilterPeak,     SIGNAL(valueChanged(double)),             this, SLOT(normalizationMaxVolumeChanged(double)));
	connect(ui->spinBoxNormalizationFilterSize,     SIGNAL(valueChanged(int)),                this, SLOT(normalizationFilterSizeChanged(int)));
	connect(ui->spinBoxNormalizationFilterSize,     SIGNAL(editingFinished()),                this, SLOT(normalizationFilterSizeFinished()));
	connect(ui->spinBoxToneAdjustBass,              SIGNAL(valueChanged(double)),             this, SLOT(toneAdjustBassChanged(double)));
	connect(ui->spinBoxToneAdjustTreble,            SIGNAL(valueChanged(double)),             this, SLOT(toneAdjustTrebleChanged(double)));
	connect(ui->buttonToneAdjustReset,              SIGNAL(clicked()),                        this, SLOT(toneAdjustTrebleReset()));
	connect(ui->lineEditCustomParamLAME,            SIGNAL(editingFinished()),                this, SLOT(customParamsChanged()));
	connect(ui->lineEditCustomParamOggEnc,          SIGNAL(editingFinished()),                this, SLOT(customParamsChanged()));
	connect(ui->lineEditCustomParamNeroAAC,         SIGNAL(editingFinished()),                this, SLOT(customParamsChanged()));
	connect(ui->lineEditCustomParamFLAC,            SIGNAL(editingFinished()),                this, SLOT(customParamsChanged()));
	connect(ui->lineEditCustomParamAften,           SIGNAL(editingFinished()),                this, SLOT(customParamsChanged()));
	connect(ui->lineEditCustomParamOpus,            SIGNAL(editingFinished()),                this, SLOT(customParamsChanged()));
	connect(ui->sliderMaxInstances,                 SIGNAL(valueChanged(int)),                this, SLOT(updateMaximumInstances(int)));
	connect(ui->checkBoxAutoDetectInstances,        SIGNAL(clicked(bool)),                    this, SLOT(autoDetectInstancesChanged(bool)));
	connect(ui->buttonBrowseCustomTempFolder,       SIGNAL(clicked()),                        this, SLOT(browseCustomTempFolderButtonClicked()));
	connect(ui->lineEditCustomTempFolder,           SIGNAL(textChanged(QString)),             this, SLOT(customTempFolderChanged(QString)));
	connect(ui->checkBoxUseSystemTempFolder,        SIGNAL(clicked(bool)),                    this, SLOT(useCustomTempFolderChanged(bool)));
	connect(ui->buttonResetAdvancedOptions,         SIGNAL(clicked()),                        this, SLOT(resetAdvancedOptionsButtonClicked()));
	connect(ui->checkBoxRename_Rename,              SIGNAL(clicked(bool)),                    this, SLOT(renameOutputEnabledChanged(bool)));
	connect(ui->checkBoxRename_RegExp,              SIGNAL(clicked(bool)),                    this, SLOT(renameRegExpEnabledChanged(bool)));
	connect(ui->lineEditRenamePattern,              SIGNAL(editingFinished()),                this, SLOT(renameOutputPatternChanged()));
	connect(ui->lineEditRenamePattern,              SIGNAL(textChanged(QString)),             this, SLOT(renameOutputPatternChanged(QString)));
	connect(ui->lineEditRenameRegExp_Search,        SIGNAL(editingFinished()),                this, SLOT(renameRegExpValueChanged()));
	connect(ui->lineEditRenameRegExp_Search,        SIGNAL(textChanged(QString)),             this, SLOT(renameRegExpSearchChanged(QString)));
	connect(ui->lineEditRenameRegExp_Replace,       SIGNAL(editingFinished()),                this, SLOT(renameRegExpValueChanged()));
	connect(ui->lineEditRenameRegExp_Replace,       SIGNAL(textChanged(QString)),             this, SLOT(renameRegExpReplaceChanged(QString)));
	connect(ui->labelShowRenameMacros,              SIGNAL(linkActivated(QString)),           this, SLOT(showRenameMacros(QString)));
	connect(ui->labelShowRegExpHelp,                SIGNAL(linkActivated(QString)),           this, SLOT(showRenameMacros(QString)));
	connect(ui->checkBoxForceStereoDownmix,         SIGNAL(clicked(bool)),                    this, SLOT(forceStereoDownmixEnabledChanged(bool)));
	connect(ui->comboBoxOpusFramesize,              SIGNAL(currentIndexChanged(int)),         this, SLOT(opusSettingsChanged()));
	connect(ui->spinBoxOpusComplexity,              SIGNAL(valueChanged(int)),                this, SLOT(opusSettingsChanged()));
	connect(ui->checkBoxOpusDisableResample,        SIGNAL(clicked(bool)),                    this, SLOT(opusSettingsChanged()));
	connect(ui->buttonRename_Rename,                SIGNAL(clicked(bool)),                    this, SLOT(renameButtonClicked(bool)));
	connect(ui->buttonRename_RegExp,                SIGNAL(clicked(bool)),                    this, SLOT(renameButtonClicked(bool)));
	connect(ui->buttonRename_FileEx,                SIGNAL(clicked(bool)),                    this, SLOT(renameButtonClicked(bool)));
	connect(ui->buttonFileExts_Add,                 SIGNAL(clicked()),                        this, SLOT(fileExtAddButtonClicked()));
	connect(ui->buttonFileExts_Remove,              SIGNAL(clicked()),                        this, SLOT(fileExtRemoveButtonClicked()));
	connect(ui->checkBoxKeepOriginalDateTime,       SIGNAL(clicked(bool)),                    this, SLOT(keepOriginalDateTimeChanged(bool)));
	connect(m_overwriteButtonGroup.data(),          SIGNAL(buttonClicked(int)),               this, SLOT(overwriteModeChanged(int)));
	connect(m_evenFilterCustumParamsHelp.data(),    SIGNAL(eventOccurred(QWidget*, QEvent*)), this, SLOT(customParamsHelpRequested(QWidget*, QEvent*)));
	connect(fileExtModel,                           SIGNAL(modelReset()),                     this, SLOT(fileExtModelChanged()));

	//--------------------------------
	// Force initial GUI update
	//--------------------------------

	updateLameAlgoQuality(ui->sliderLameAlgoQuality->value());
	updateMaximumInstances(ui->sliderMaxInstances->value());
	toneAdjustTrebleChanged(ui->spinBoxToneAdjustTreble->value());
	toneAdjustBassChanged(ui->spinBoxToneAdjustBass->value());
	normalizationEnabledChanged(ui->checkBoxNormalizationFilterEnabled->isChecked());
	customParamsChanged();
	
	//--------------------------------
	// Initialize actions
	//--------------------------------

	//Activate file menu actions
	ui->actionOpenFolder           ->setData(QVariant::fromValue<bool>(false));
	ui->actionOpenFolderRecursively->setData(QVariant::fromValue<bool>(true));
	connect(ui->actionOpenFolder,            SIGNAL(triggered()), this, SLOT(openFolderActionActivated()));
	connect(ui->actionOpenFolderRecursively, SIGNAL(triggered()), this, SLOT(openFolderActionActivated()));

	//Activate view menu actions
	m_tabActionGroup.reset(new QActionGroup(this));
	m_tabActionGroup->addAction(ui->actionSourceFiles);
	m_tabActionGroup->addAction(ui->actionOutputDirectory);
	m_tabActionGroup->addAction(ui->actionCompression);
	m_tabActionGroup->addAction(ui->actionMetaData);
	m_tabActionGroup->addAction(ui->actionAdvancedOptions);
	ui->actionSourceFiles->setData(0);
	ui->actionOutputDirectory->setData(1);
	ui->actionMetaData->setData(2);
	ui->actionCompression->setData(3);
	ui->actionAdvancedOptions->setData(4);
	ui->actionSourceFiles->setChecked(true);
	connect(m_tabActionGroup.data(), SIGNAL(triggered(QAction*)), this, SLOT(tabActionActivated(QAction*)));

	//Activate style menu actions
	m_styleActionGroup .reset(new QActionGroup(this));
	m_styleActionGroup->addAction(ui->actionStylePlastique);
	m_styleActionGroup->addAction(ui->actionStyleCleanlooks);
	m_styleActionGroup->addAction(ui->actionStyleWindowsVista);
	m_styleActionGroup->addAction(ui->actionStyleWindowsXP);
	m_styleActionGroup->addAction(ui->actionStyleWindowsClassic);
	ui->actionStylePlastique->setData(0);
	ui->actionStyleCleanlooks->setData(1);
	ui->actionStyleWindowsVista->setData(2);
	ui->actionStyleWindowsXP->setData(3);
	ui->actionStyleWindowsClassic->setData(4);
	ui->actionStylePlastique->setChecked(true);
	ui->actionStyleWindowsXP->setEnabled((QSysInfo::windowsVersion() & QSysInfo::WV_NT_based) >= QSysInfo::WV_XP && MUtils::GUI::themes_enabled());
	ui->actionStyleWindowsVista->setEnabled((QSysInfo::windowsVersion() & QSysInfo::WV_NT_based) >= QSysInfo::WV_VISTA && MUtils::GUI::themes_enabled());
	connect(m_styleActionGroup.data(), SIGNAL(triggered(QAction*)), this, SLOT(styleActionActivated(QAction*)));
	styleActionActivated(NULL);

	//Populate the language menu
	m_languageActionGroup.reset(new QActionGroup(this));
	QStringList translations;
	if(MUtils::Translation::enumerate(translations) > 0)
	{
		for(QStringList::ConstIterator iter = translations.constBegin(); iter != translations.constEnd(); iter++)
		{
			QAction *currentLanguage = new QAction(this);
			currentLanguage->setData(*iter);
			currentLanguage->setText(MUtils::Translation::get_name(*iter));
			currentLanguage->setIcon(QIcon(QString(":/flags/%1.png").arg(*iter)));
			currentLanguage->setCheckable(true);
			currentLanguage->setChecked(false);
			m_languageActionGroup->addAction(currentLanguage);
			ui->menuLanguage->insertAction(ui->actionLoadTranslationFromFile, currentLanguage);
		}
	}
	ui->menuLanguage->insertSeparator(ui->actionLoadTranslationFromFile);
	connect(ui->actionLoadTranslationFromFile, SIGNAL(triggered(bool)),     this, SLOT(languageFromFileActionActivated(bool)));
	connect(m_languageActionGroup.data(),      SIGNAL(triggered(QAction*)), this, SLOT(languageActionActivated(QAction*)));
	ui->actionLoadTranslationFromFile->setChecked(false);

	//Activate tools menu actions
	ui->actionDisableUpdateReminder->setChecked(!m_settings->autoUpdateEnabled());
	ui->actionDisableSounds->setChecked(!m_settings->soundsEnabled());
	ui->actionDisableNeroAacNotifications->setChecked(!m_settings->neroAacNotificationsEnabled());
	ui->actionDisableSlowStartupNotifications->setChecked(!m_settings->antivirNotificationsEnabled());
	ui->actionDisableShellIntegration->setChecked(!m_settings->shellIntegrationEnabled());
	ui->actionDisableShellIntegration->setDisabled(lamexp_version_portable() && ui->actionDisableShellIntegration->isChecked());
	ui->actionDisableTrayIcon->setChecked(m_settings->disableTrayIcon());
	ui->actionCheckForBetaUpdates->setChecked(m_settings->autoUpdateCheckBeta() || lamexp_version_test());
	ui->actionCheckForBetaUpdates->setEnabled(!lamexp_version_test());
	ui->actionHibernateComputer->setChecked(m_settings->hibernateComputer());
	ui->actionHibernateComputer->setEnabled(MUtils::OS::is_hibernation_supported());
	connect(ui->actionDisableUpdateReminder, SIGNAL(triggered(bool)), this, SLOT(disableUpdateReminderActionTriggered(bool)));
	connect(ui->actionDisableSounds, SIGNAL(triggered(bool)), this, SLOT(disableSoundsActionTriggered(bool)));
	connect(ui->actionDisableNeroAacNotifications, SIGNAL(triggered(bool)), this, SLOT(disableNeroAacNotificationsActionTriggered(bool)));
	connect(ui->actionDisableSlowStartupNotifications, SIGNAL(triggered(bool)), this, SLOT(disableSlowStartupNotificationsActionTriggered(bool)));
	connect(ui->actionDisableShellIntegration, SIGNAL(triggered(bool)), this, SLOT(disableShellIntegrationActionTriggered(bool)));
	connect(ui->actionDisableTrayIcon, SIGNAL(triggered(bool)), this, SLOT(disableTrayIconActionTriggered(bool)));
	connect(ui->actionShowDropBoxWidget, SIGNAL(triggered(bool)), this, SLOT(showDropBoxWidgetActionTriggered(bool)));
	connect(ui->actionHibernateComputer, SIGNAL(triggered(bool)), this, SLOT(hibernateComputerActionTriggered(bool)));
	connect(ui->actionCheckForBetaUpdates, SIGNAL(triggered(bool)), this, SLOT(checkForBetaUpdatesActionTriggered(bool)));
	connect(ui->actionImportCueSheet, SIGNAL(triggered(bool)), this, SLOT(importCueSheetActionTriggered(bool)));
		
	//Activate help menu actions
	ui->actionVisitHomepage    ->setData(QString::fromLatin1(lamexp_website_url()));
	ui->actionVisitSupport     ->setData(QString::fromLatin1(lamexp_support_url()));
	ui->actionVisitMuldersSite ->setData(QString::fromLatin1(lamexp_mulders_url()));
	ui->actionVisitTracker     ->setData(QString::fromLatin1(lamexp_tracker_url()));
	ui->actionVisitHAK         ->setData(QString::fromLatin1(g_hydrogen_audio_url));
	ui->actionDocumentManual   ->setData(QString("%1/Manual.html")   .arg(QApplication::applicationDirPath()));
	ui->actionDocumentChangelog->setData(QString("%1/Changelog.html").arg(QApplication::applicationDirPath()));
	ui->actionDocumentTranslate->setData(QString("%1/Translate.html").arg(QApplication::applicationDirPath()));
	connect(ui->actionCheckUpdates,      SIGNAL(triggered()), this, SLOT(checkUpdatesActionActivated()));
	connect(ui->actionVisitSupport,      SIGNAL(triggered()), this, SLOT(visitHomepageActionActivated()));
	connect(ui->actionVisitTracker,      SIGNAL(triggered()), this, SLOT(visitHomepageActionActivated()));
	connect(ui->actionVisitHomepage,     SIGNAL(triggered()), this, SLOT(visitHomepageActionActivated()));
	connect(ui->actionVisitMuldersSite,  SIGNAL(triggered()), this, SLOT(visitHomepageActionActivated()));
	connect(ui->actionVisitHAK,          SIGNAL(triggered()), this, SLOT(visitHomepageActionActivated()));
	connect(ui->actionDocumentManual,    SIGNAL(triggered()), this, SLOT(documentActionActivated()));
	connect(ui->actionDocumentChangelog, SIGNAL(triggered()), this, SLOT(documentActionActivated()));
	connect(ui->actionDocumentTranslate, SIGNAL(triggered()), this, SLOT(documentActionActivated()));
	
	//--------------------------------
	// Prepare to show window
	//--------------------------------

	//Adjust size to DPI settings and re-center
	MUtils::GUI::scale_widget(this);
	
	//Create DropBox widget
	m_dropBox.reset(new DropBox(this, m_fileListModel, m_settings));
	connect(m_fileListModel, SIGNAL(modelReset()),                      m_dropBox.data(), SLOT(modelChanged()));
	connect(m_fileListModel, SIGNAL(rowsInserted(QModelIndex,int,int)), m_dropBox.data(), SLOT(modelChanged()));
	connect(m_fileListModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),  m_dropBox.data(), SLOT(modelChanged()));
	connect(m_fileListModel, SIGNAL(rowAppended()),                     m_dropBox.data(), SLOT(modelChanged()));

	//Create message handler thread
	m_messageHandler.reset(new MessageHandlerThread(ipcChannel));
	connect(m_messageHandler.data(), SIGNAL(otherInstanceDetected()),       this, SLOT(notifyOtherInstance()), Qt::QueuedConnection);
	connect(m_messageHandler.data(), SIGNAL(fileReceived(QString)),         this, SLOT(addFileDelayed(QString)), Qt::QueuedConnection);
	connect(m_messageHandler.data(), SIGNAL(folderReceived(QString, bool)), this, SLOT(addFolderDelayed(QString, bool)), Qt::QueuedConnection);
	connect(m_messageHandler.data(), SIGNAL(killSignalReceived()),          this, SLOT(close()), Qt::QueuedConnection);
	m_messageHandler->start();

	//Init delayed file handling
	m_delayedFileList .reset(new QStringList());
	m_delayedFileTimer.reset(new QTimer());
	m_delayedFileTimer->setSingleShot(true);
	m_delayedFileTimer->setInterval(5000);
	connect(m_delayedFileTimer.data(), SIGNAL(timeout()), this, SLOT(handleDelayedFiles()));

	//Load translation
	initializeTranslation();

	//Re-translate (make sure we translate once)
	QEvent languageChangeEvent(QEvent::LanguageChange);
	changeEvent(&languageChangeEvent);

	//Enable Drag & Drop
	m_droppedFileList.reset(new QList<QUrl>());
	this->setAcceptDrops(true);
}

////////////////////////////////////////////////////////////
// Destructor
////////////////////////////////////////////////////////////

MainWindow::~MainWindow(void)
{
	//Stop message handler thread
	if(m_messageHandler && m_messageHandler->isRunning())
	{
		m_messageHandler->stop();
		if(!m_messageHandler->wait(2500))
		{
			m_messageHandler->terminate();
			m_messageHandler->wait();
		}
	}

	//Unset models
	SET_MODEL(ui->sourceFileView,   NULL);
	SET_MODEL(ui->outputFolderView, NULL);
	SET_MODEL(ui->metaDataView,     NULL);

	//Un-initialize the dialog
	MUTILS_DELETE(ui);
}

////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
////////////////////////////////////////////////////////////

/*
 * Add file to source list
 */
void MainWindow::addFiles(const QStringList &files)
{
	if(files.isEmpty())
	{
		return;
	}

	if(ui->tabWidget->currentIndex() != 0)
	{
		SignalBlockHelper signalBlockHelper(ui->tabWidget);
		ui->tabWidget->setCurrentIndex(0);
		tabPageChanged(ui->tabWidget->currentIndex(), true);
	}

	INIT_BANNER();
	QScopedPointer<FileAnalyzer> analyzer(new FileAnalyzer(files));

	connect(analyzer.data(), SIGNAL(fileSelected(QString)),            m_banner.data(), SLOT(setText(QString)),             Qt::QueuedConnection);
	connect(analyzer.data(), SIGNAL(progressValChanged(unsigned int)), m_banner.data(), SLOT(setProgressVal(unsigned int)), Qt::QueuedConnection);
	connect(analyzer.data(), SIGNAL(progressMaxChanged(unsigned int)), m_banner.data(), SLOT(setProgressMax(unsigned int)), Qt::QueuedConnection);
	connect(analyzer.data(), SIGNAL(fileAnalyzed(AudioFileModel)),     m_fileListModel, SLOT(addFile(AudioFileModel)),      Qt::QueuedConnection);
	connect(m_banner.data(), SIGNAL(userAbort()),                      analyzer.data(), SLOT(abortProcess()),               Qt::DirectConnection);

	if(!analyzer.isNull())
	{
		FileListBlockHelper fileListBlocker(m_fileListModel);
		m_banner->show(tr("Adding file(s), please wait..."), analyzer.data());
	}

	qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
	ui->sourceFileView->update();
	qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
	ui->sourceFileView->scrollToBottom();
	qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

	if(analyzer->filesDenied())
	{
		QMessageBox::warning(this, tr("Access Denied"), NOBREAK(QString("%1<br>%2").arg(tr("%n file(s) have been rejected, because read access was not granted!", "", analyzer->filesDenied()), tr("This usually means the file is locked by another process."))));
	}
	if(analyzer->filesDummyCDDA())
	{
		QMessageBox::warning(this, tr("CDDA Files"), NOBREAK(QString("%1<br><br>%2<br>%3").arg(tr("%n file(s) have been rejected, because they are dummy CDDA files!", "", analyzer->filesDummyCDDA()), tr("Sorry, LameXP cannot extract audio tracks from an Audio-CD at present."), tr("We recommend using %1 for that purpose.").arg("<a href=\"http://www.exactaudiocopy.de/\">Exact Audio Copy</a>"))));
	}
	if(analyzer->filesCueSheet())
	{
		QMessageBox::warning(this, tr("Cue Sheet"), NOBREAK(QString("%1<br>%2").arg(tr("%n file(s) have been rejected, because they appear to be Cue Sheet images!", "",analyzer->filesCueSheet()), tr("Please use LameXP's Cue Sheet wizard for importing Cue Sheet files."))));
	}
	if(analyzer->filesRejected())
	{
		QMessageBox::warning(this, tr("Files Rejected"), NOBREAK(QString("%1<br>%2").arg(tr("%n file(s) have been rejected, because the file format could not be recognized!", "", analyzer->filesRejected()), tr("This usually means the file is damaged or the file format is not supported."))));
	}

	m_banner->close();
}

/*
 * Add folder to source list
 */
void MainWindow::addFolder(const QString &path, bool recursive, bool delayed, QString filter)
{
	QFileInfoList folderInfoList;
	folderInfoList << QFileInfo(path);
	QStringList fileList;
	
	showBanner(tr("Scanning folder(s) for files, please wait..."));
	
	QApplication::processEvents();
	MUtils::OS::check_key_state_esc();

	while(!folderInfoList.isEmpty())
	{
		if(MUtils::OS::check_key_state_esc())
		{
			qWarning("Operation cancelled by user!");
			MUtils::Sound::beep(MUtils::Sound::BEEP_ERR);
			fileList.clear();
			break;
		}
		
		QDir currentDir(folderInfoList.takeFirst().canonicalFilePath());
		QFileInfoList fileInfoList = currentDir.entryInfoList(QDir::Files | QDir::NoSymLinks);

		for(QFileInfoList::ConstIterator iter = fileInfoList.constBegin(); iter != fileInfoList.constEnd(); iter++)
		{
			if(filter.isEmpty() || (iter->suffix().compare(filter, Qt::CaseInsensitive) == 0))
			{
				fileList << iter->canonicalFilePath();
			}
		}

		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

		if(recursive)
		{
			folderInfoList.append(currentDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks));
			QApplication::processEvents();
		}
	}
	
	m_banner->close();
	QApplication::processEvents();

	if(!fileList.isEmpty())
	{
		if(delayed)
		{
			addFilesDelayed(fileList);
		}
		else
		{
			addFiles(fileList);
		}
	}
}

/*
 * Check for updates
 */
bool MainWindow::checkForUpdates(bool *const haveNewVersion)
{
	bool bReadyToInstall = false;
	if (haveNewVersion)
	{
		*haveNewVersion = false;
	}

	UpdateDialog *updateDialog = new UpdateDialog(m_settings, this);
	updateDialog->exec();

	if(updateDialog->getSuccess())
	{
		SHOW_CORNER_WIDGET(false);
		m_settings->autoUpdateLastCheck(QDate::currentDate().toString(Qt::ISODate));
		bReadyToInstall = updateDialog->updateReadyToInstall();
		if (haveNewVersion)
		{
			*haveNewVersion = updateDialog->haveNewVersion();
		}
	}

	MUTILS_DELETE(updateDialog);
	return bReadyToInstall;
}

/*
 * Refresh list of favorites
 */
void MainWindow::refreshFavorites(void)
{
	QList<QAction*> folderList = m_outputFolderFavoritesMenu->actions();
	QStringList favorites = m_settings->favoriteOutputFolders().split("|", QString::SkipEmptyParts);
	while(favorites.count() > 6) favorites.removeFirst();

	while(!folderList.isEmpty())
	{
		QAction *currentItem = folderList.takeFirst();
		if(currentItem->isSeparator()) break;
		m_outputFolderFavoritesMenu->removeAction(currentItem);
		MUTILS_DELETE(currentItem);
	}

	QAction *lastItem = m_outputFolderFavoritesMenu->actions().first();

	while(!favorites.isEmpty())
	{
		QString path = favorites.takeLast();
		if(QDir(path).exists())
		{
			QAction *action = new QAction(QIcon(":/icons/folder_go.png"), QDir::toNativeSeparators(path), this);
			action->setData(path);
			m_outputFolderFavoritesMenu->insertAction(lastItem, action);
			connect(action, SIGNAL(triggered(bool)), this, SLOT(gotoFavoriteFolder()));
			lastItem = action;
		}
	}
}

/*
 * Initilaize translation
 */
void MainWindow::initializeTranslation(void)
{
	bool translationLoaded = false;

	//Try to load "external" translation file
	if(!m_settings->currentLanguageFile().isEmpty())
	{
		const QString qmFilePath = QFileInfo(m_settings->currentLanguageFile()).canonicalFilePath();
		if((!qmFilePath.isEmpty()) && QFileInfo(qmFilePath).exists() && QFileInfo(qmFilePath).isFile() && (QFileInfo(qmFilePath).suffix().compare("qm", Qt::CaseInsensitive) == 0))
		{
			if(MUtils::Translation::install_translator_from_file(qmFilePath))
			{
				QList<QAction*> actions = m_languageActionGroup->actions();
				while(!actions.isEmpty()) actions.takeFirst()->setChecked(false);
				ui->actionLoadTranslationFromFile->setChecked(true);
				translationLoaded = true;
			}
		}
	}

	//Try to load "built-in" translation file
	if(!translationLoaded)
	{
		QList<QAction*> languageActions = m_languageActionGroup->actions();
		while(!languageActions.isEmpty())
		{
			QAction *currentLanguage = languageActions.takeFirst();
			if(currentLanguage->data().toString().compare(m_settings->currentLanguage(), Qt::CaseInsensitive) == 0)
			{
				currentLanguage->setChecked(true);
				languageActionActivated(currentLanguage);
				translationLoaded = true;
			}
		}
	}

	//Fallback to default translation
	if(!translationLoaded)
	{
		QList<QAction*> languageActions = m_languageActionGroup->actions();
		while(!languageActions.isEmpty())
		{
			QAction *currentLanguage = languageActions.takeFirst();
			if(currentLanguage->data().toString().compare(MUtils::Translation::DEFAULT_LANGID, Qt::CaseInsensitive) == 0)
			{
				currentLanguage->setChecked(true);
				languageActionActivated(currentLanguage);
				translationLoaded = true;
			}
		}
	}

	//Make sure we loaded some translation
	if(!translationLoaded)
	{
		qFatal("Failed to load any translation, this is NOT supposed to happen!");
	}
}

/*
 * Open a document link
 */
void MainWindow::openDocumentLink(QAction *const action)
{
	if(!(action->data().isValid() && (action->data().type() == QVariant::String)))
	{
		qWarning("Cannot open document for this QAction!");
		return;
	}

	//Try to open exitsing document file
	const QFileInfo document(action->data().toString());
	if(document.exists() && document.isFile() && (document.size() >= 1024))
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(document.canonicalFilePath()));
		return;
	}

	//Document not found -> fallback mode!
	qWarning("Document '%s' not found -> redirecting to the website!", MUTILS_UTF8(document.fileName()));
	const QUrl url(QString("%1/%2").arg(QString::fromLatin1(g_documents_base_url), document.fileName()));
	QDesktopServices::openUrl(url);
}

/*
 * Move selected files up/down
 */
void MainWindow::moveSelectedFiles(const bool &up)
{
	QItemSelectionModel *const selection = ui->sourceFileView->selectionModel();
	if(selection && selection->hasSelection())
	{
		const QModelIndexList selectedRows = up ? selection->selectedRows() : INVERT_LIST(selection->selectedRows());
		if((up && (selectedRows.first().row() > 0)) || ((!up) && (selectedRows.first().row() < m_fileListModel->rowCount() - 1)))
		{
			const int delta = up ? (-1) : 1;
			const int firstIndex = (up ? selectedRows.first() : selectedRows.last()).row() + delta;
			const int selectionCount = selectedRows.count();
			if(abs(delta) > 0)
			{
				FileListBlockHelper fileListBlocker(m_fileListModel);
				for(QModelIndexList::ConstIterator iter = selectedRows.constBegin(); iter != selectedRows.constEnd(); iter++)
				{
					if(!m_fileListModel->moveFile((*iter), delta))
					{
						break;
					}
				}
			}
			selection->clearSelection();
			for(int i = 0; i < selectionCount; i++)
			{
				const QModelIndex item = m_fileListModel->index(firstIndex + i, 0);
				selection->select(QItemSelection(item, item), QItemSelectionModel::Select | QItemSelectionModel::Rows);
			}
			ui->sourceFileView->scrollTo(m_fileListModel->index((up ? firstIndex : firstIndex + selectionCount - 1), 0), QAbstractItemView::PositionAtCenter);
			return;
		}
	}
	MUtils::Sound::beep(MUtils::Sound::BEEP_WRN);
}

/*
 * Show banner popup dialog
 */
void MainWindow::showBanner(const QString &text)
{
	INIT_BANNER();
	m_banner->show(text);
}

/*
 * Show banner popup dialog
 */
void MainWindow::showBanner(const QString &text, QThread *const thread)
{
	INIT_BANNER();
	m_banner->show(text, thread);
}

/*
 * Show banner popup dialog
 */
void MainWindow::showBanner(const QString &text, QEventLoop *const eventLoop)
{
	INIT_BANNER();
	m_banner->show(text, eventLoop);
}

/*
 * Show banner popup dialog
 */
void MainWindow::showBanner(const QString &text, bool &flag, const bool &test)
{
	if((!flag) && (test))
	{
		INIT_BANNER();
		m_banner->show(text);
		flag = true;
	}
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*
 * Window is about to be shown
 */
void MainWindow::showEvent(QShowEvent *event)
{
	m_accepted = false;
	resizeEvent(NULL);
	sourceModelChanged();

	if(!event->spontaneous())
	{
		SignalBlockHelper signalBlockHelper(ui->tabWidget);
		ui->tabWidget->setCurrentIndex(0);
		tabPageChanged(ui->tabWidget->currentIndex(), true);
	}

	if(m_firstTimeShown)
	{
		m_firstTimeShown = false;
		QTimer::singleShot(0, this, SLOT(windowShown()));
	}
	else
	{
		if(m_settings->dropBoxWidgetEnabled())
		{
			m_dropBox->setVisible(true);
		}
	}
}

/*
 * Re-translate the UI
 */
void MainWindow::changeEvent(QEvent *e)
{
	QMainWindow::changeEvent(e);
	if(e->type() != QEvent::LanguageChange)
	{
		return;
	}

	int comboBoxIndex[6];
		
	//Backup combobox indices, as retranslateUi() resets
	comboBoxIndex[0] = ui->comboBoxMP3ChannelMode->currentIndex();
	comboBoxIndex[1] = ui->comboBoxSamplingRate->currentIndex();
	comboBoxIndex[2] = ui->comboBoxAACProfile->currentIndex();
	comboBoxIndex[3] = ui->comboBoxAftenCodingMode->currentIndex();
	comboBoxIndex[4] = ui->comboBoxAftenDRCMode->currentIndex();
	comboBoxIndex[5] = ui->comboBoxOpusFramesize->currentIndex();
		
	//Re-translate from UIC
	ui->retranslateUi(this);

	//Restore combobox indices
	ui->comboBoxMP3ChannelMode->setCurrentIndex(comboBoxIndex[0]);
	ui->comboBoxSamplingRate->setCurrentIndex(comboBoxIndex[1]);
	ui->comboBoxAACProfile->setCurrentIndex(comboBoxIndex[2]);
	ui->comboBoxAftenCodingMode->setCurrentIndex(comboBoxIndex[3]);
	ui->comboBoxAftenDRCMode->setCurrentIndex(comboBoxIndex[4]);
	ui->comboBoxOpusFramesize->setCurrentIndex(comboBoxIndex[5]);

	//Update the window title
	if(MUTILS_DEBUG)
	{
		setWindowTitle(QString("%1 [!!! DEBUG BUILD !!!]").arg(windowTitle()));
	}
	else if(lamexp_version_test())
	{
		setWindowTitle(QString("%1 [%2]").arg(windowTitle(), tr("DEMO VERSION")));
	}

	//Manually re-translate widgets that UIC doesn't handle
	m_outputFolderNoteBox->setText(tr("Initializing directory outline, please be patient..."));
	m_dropNoteLabel->setText(QString("<br><img src=\":/images/DropZone.png\"><br><br>%1").arg(tr("You can drop in audio files here!")));
	if(QLabel *cornerWidget = dynamic_cast<QLabel*>(ui->menubar->cornerWidget()))
	{
		cornerWidget->setText(QString("<nobr><img src=\":/icons/exclamation_small.png\">&nbsp;<b style=\"color:darkred\">%1</b>&nbsp;&nbsp;&nbsp;</nobr>").arg(tr("Check for Updates")));
	}
	m_showDetailsContextAction->setText(tr("Show Details"));
	m_previewContextAction->setText(tr("Open File in External Application"));
	m_findFileContextAction->setText(tr("Browse File Location"));
	m_showFolderContextAction->setText(tr("Browse Selected Folder"));
	m_refreshFolderContextAction->setText(tr("Refresh Directory Outline"));
	m_goUpFolderContextAction->setText(tr("Go To Parent Directory"));
	m_addFavoriteFolderAction->setText(tr("Bookmark Current Output Folder"));
	m_exportCsvContextAction->setText(tr("Export Meta Tags to CSV File"));
	m_importCsvContextAction->setText(tr("Import Meta Tags from CSV File"));

	//Force GUI update
	m_metaInfoModel->clearData();
	m_metaInfoModel->setData(m_metaInfoModel->index(4, 1), m_settings->metaInfoPosition());
	updateEncoder(m_settings->compressionEncoder());
	updateLameAlgoQuality(ui->sliderLameAlgoQuality->value());
	updateMaximumInstances(ui->sliderMaxInstances->value());
	renameOutputPatternChanged(ui->lineEditRenamePattern->text(), true);
	renameRegExpSearchChanged (ui->lineEditRenameRegExp_Search ->text(), true);
	renameRegExpReplaceChanged(ui->lineEditRenameRegExp_Replace->text(), true);

	//Re-install shell integration
	if(m_settings->shellIntegrationEnabled())
	{
		ShellIntegration::install();
	}

	//Translate system menu
	MUtils::GUI::sysmenu_update(this, IDM_ABOUTBOX, ui->buttonAbout->text());
	
	//Force resize event
	QApplication::postEvent(this, new QResizeEvent(this->size(), QSize()));
	for(QObjectList::ConstIterator iter = this->children().constBegin(); iter != this->children().constEnd(); iter++)
	{
		if(QWidget *child = dynamic_cast<QWidget*>(*iter))
		{
			QApplication::postEvent(child, new QResizeEvent(child->size(), QSize()));
		}
	}

	//Force tabe page change
	tabPageChanged(ui->tabWidget->currentIndex(), true);
}

/*
 * File dragged over window
 */
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
	QStringList formats = event->mimeData()->formats();
	
	if(formats.contains("application/x-qt-windows-mime;value=\"FileNameW\"", Qt::CaseInsensitive) && formats.contains("text/uri-list", Qt::CaseInsensitive))
	{
		event->acceptProposedAction();
	}
}

/*
 * File dropped onto window
 */
void MainWindow::dropEvent(QDropEvent *event)
{
	m_droppedFileList->clear();
	(*m_droppedFileList) << event->mimeData()->urls();
	if(!m_droppedFileList->isEmpty())
	{
		PLAY_SOUND_OPTIONAL("drop", true);
		QTimer::singleShot(0, this, SLOT(handleDroppedFiles()));
	}
}

/*
 * Window tries to close
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
	if(BANNER_VISIBLE || m_delayedFileTimer->isActive())
	{
		MUtils::Sound::beep(MUtils::Sound::BEEP_WRN);
		event->ignore();
	}
	
	if(m_dropBox)
	{
		m_dropBox->hide();
	}
}

/*
 * Window was resized
 */
void MainWindow::resizeEvent(QResizeEvent *event)
{
	if(event) QMainWindow::resizeEvent(event);

	if(QWidget *port = ui->sourceFileView->viewport())
	{
		m_dropNoteLabel->setGeometry(port->geometry());
	}

	if(QWidget *port = ui->outputFolderView->viewport())
	{
		m_outputFolderNoteBox->setGeometry(16, (port->height() - 64) / 2, port->width() - 32, 64);
	}
}

/*
 * Key press event filter
 */
void MainWindow::keyPressEvent(QKeyEvent *e)
{
	if(e->key() == Qt::Key_Delete)
	{
		if(ui->sourceFileView->isVisible())
		{
			QTimer::singleShot(0, this, SLOT(removeFileButtonClicked()));
			return;
		}
	}

	if(e->modifiers().testFlag(Qt::ControlModifier) && (e->key() == Qt::Key_F5))
	{
		initializeTranslation();
		MUtils::Sound::beep(MUtils::Sound::BEEP_NFO);
		return;
	}

	if(e->key() == Qt::Key_F5)
	{
		if(ui->outputFolderView->isVisible())
		{
			QTimer::singleShot(0, this, SLOT(refreshFolderContextActionTriggered()));
			return;
		}
	}

	QMainWindow::keyPressEvent(e);
}

/*
 * Event filter
 */
bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
	if(obj == m_fileSystemModel.data())
	{
		if(QApplication::overrideCursor() == NULL)
		{
			QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
			QTimer::singleShot(250, this, SLOT(restoreCursor()));
		}
	}

	return QMainWindow::eventFilter(obj, event);
}

bool MainWindow::event(QEvent *e)
{
	switch(static_cast<qint32>(e->type()))
	{
	case MUtils::GUI::USER_EVENT_QUERYENDSESSION:
		qWarning("System is shutting down, main window prepares to close...");
		if(BANNER_VISIBLE) m_banner->close();
		if(m_delayedFileTimer->isActive()) m_delayedFileTimer->stop();
		return true;
	case MUtils::GUI::USER_EVENT_ENDSESSION:
		qWarning("System is shutting down, main window will close now...");
		if(isVisible())
		{
			while(!close())
			{
				QApplication::processEvents(QEventLoop::WaitForMoreEvents & QEventLoop::ExcludeUserInputEvents);
			}
		}
		m_fileListModel->clearFiles();
		return true;
	case QEvent::MouseButtonPress:
		if(ui->outputFolderEdit->isVisible())
		{
			QTimer::singleShot(0, this, SLOT(outputFolderEditFinished()));
		}
	default:
		return QMainWindow::event(e);
	}
}

bool MainWindow::winEvent(MSG *message, long *result)
{
	if(MUtils::GUI::sysmenu_check_msg(message, IDM_ABOUTBOX))
	{
		QTimer::singleShot(0, ui->buttonAbout, SLOT(click()));
		*result = 0;
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////
// Slots
////////////////////////////////////////////////////////////

// =========================================================
// Show window slots
// =========================================================

/*
 * Window shown
 */
void MainWindow::windowShown(void)
{
	const MUtils::OS::ArgumentMap &arguments = MUtils::OS::arguments(); //QApplication::arguments();

	//Force resize event
	resizeEvent(NULL);

	//First run?
	const bool firstRun = arguments.contains("first-run");
	if (firstRun)
	{
		if (m_settings->licenseAccepted() < 0)
		{
			m_settings->licenseAccepted(0);
		}
		m_settings->autoUpdateCheckBeta(false);
		m_settings->syncNow();
	}

	//Check license
	if (m_settings->licenseAccepted() == 0)
	{
		QScopedPointer<AboutDialog> about(new AboutDialog(m_settings, this, true));
		if (about->exec() > 0)
		{
			m_settings->licenseAccepted(1);
			m_settings->syncNow();
			PLAY_SOUND_OPTIONAL("woohoo", false);
			if (lamexp_version_test())
			{
				showAnnounceBox();
			}
		}
		else
		{
			m_settings->licenseAccepted(-1);
			m_settings->syncNow();
		}
	}

	//License declined?
	if(m_settings->licenseAccepted() <= 0)
	{
		QApplication::processEvents();
		PLAY_SOUND_OPTIONAL("whammy", false);
		QMessageBox::critical(this, tr("License Declined"), tr("You have declined the license. Consequently the application will exit now!"), tr("Goodbye!"));
		QFileInfo uninstallerInfo = QFileInfo(QString("%1/Uninstall.exe").arg(QApplication::applicationDirPath()));
		if(uninstallerInfo.exists())
		{
			QString uninstallerDir = uninstallerInfo.canonicalPath();
			QString uninstallerPath = uninstallerInfo.canonicalFilePath();
			for(int i = 0; i < 3; i++)
			{
				if(MUtils::OS::shell_open(this, QDir::toNativeSeparators(uninstallerPath), "/Force", QDir::toNativeSeparators(uninstallerDir))) break;
			}
		}
		QApplication::quit();
		return;
	}
	
	//Check for expiration of "pre-release" version
	if(lamexp_version_test())
	{
		if(MUtils::OS::current_date() >= lamexp_version_expires())
		{
			qWarning("Binary expired !!!");
			PLAY_SOUND_OPTIONAL("whammy", false);
			bool haveNewVersion = true;
			if(QMessageBox::warning(this, tr("LameXP - Expired"), NOBREAK(QString("%1<br>%2").arg(tr("This demo (pre-release) version of LameXP has expired at %1.").arg(lamexp_version_expires().toString(Qt::ISODate)), tr("LameXP is free software and release versions won't expire."))), tr("Check for Updates"), tr("Exit Program")) == 0)
			{
				if (checkForUpdates(&haveNewVersion))
				{
					QApplication::quit();
					return;
				}
			}
			if(haveNewVersion)
			{
				QApplication::quit();
				return;
			}
		}
	}

	//Slow startup indicator
	if((!firstRun) && m_settings->slowStartup() && m_settings->antivirNotificationsEnabled())
	{
		QString message;
		message += tr("It seems that a bogus anti-virus software is slowing down the startup of LameXP.").append("<br>");
		message += tr("Please refer to the %1 document for details and solutions!").arg(LINK_EX(QString("%1/Manual.html#performance-issues").arg(g_documents_base_url), tr("Manual"))).append("<br>");
		if(QMessageBox::warning(this, tr("Slow Startup"), NOBREAK(message), tr("Discard"), tr("Don't Show Again")) == 1)
		{
			m_settings->antivirNotificationsEnabled(false);
			ui->actionDisableSlowStartupNotifications->setChecked(!m_settings->antivirNotificationsEnabled());
		}
	}

	//Update reminder
	if (MUtils::OS::current_date() >= MUtils::Version::app_build_date().addMonths(18))
	{
		qWarning("Binary is more than a year old, time to update!");
		SHOW_CORNER_WIDGET(true);
		int ret = QMessageBox::warning(this, tr("Urgent Update"), NOBREAK(tr("Your version of LameXP is more than a year old. Time for an update!")), tr("Check for Updates"), tr("Exit Program"), tr("Ignore"));
		switch(ret)
		{
		case 0:
			if(checkForUpdates())
			{
				QApplication::quit();
				return;
			}
			break;
		case 1:
			QApplication::quit();
			return;
		default:
			QEventLoop loop; QTimer::singleShot(7000, &loop, SLOT(quit()));
			PLAY_SOUND_OPTIONAL("waiting", true);
			showBanner(tr("Skipping update check this time, please be patient..."), &loop);
			break;
		}
	}
	else
	{
		QDate lastUpdateCheck = QDate::fromString(m_settings->autoUpdateLastCheck(), Qt::ISODate);
		if((!firstRun) && ((!lastUpdateCheck.isValid()) || (MUtils::OS::current_date() >= lastUpdateCheck.addDays(14))))
		{
			SHOW_CORNER_WIDGET(true);
			if(m_settings->autoUpdateEnabled())
			{
				if(QMessageBox::information(this, tr("Update Reminder"), NOBREAK(lastUpdateCheck.isValid() ? tr("Your last update check was more than 14 days ago. Check for updates now?") : tr("Your did not check for LameXP updates yet. Check for updates now?")), tr("Check for Updates"), tr("Postpone")) == 0)
				{
					if(checkForUpdates(NULL))
					{
						QApplication::quit();
						return;
					}
				}
			}
		}
	}

	//Check for AAC support
	if(m_settings->neroAacNotificationsEnabled() && (EncoderRegistry::getAacEncoder() <= SettingsModel::AAC_ENCODER_NONE))
	{
		QString appPath = QDir(QCoreApplication::applicationDirPath()).canonicalPath();
		if(appPath.isEmpty()) appPath = QCoreApplication::applicationDirPath();
		QString messageText;
		messageText += tr("The Nero AAC encoder could not be found. AAC encoding support will be disabled.").append("<br>");
		messageText += tr("Please put 'neroAacEnc.exe', 'neroAacDec.exe' and 'neroAacTag.exe' into the LameXP directory!").append("<br><br>");
		messageText += QString("<b>").append(tr("Your LameXP install directory is located here:")).append("</b><br>");
		messageText += QString("<tt>%1</tt><br><br>").arg(FSLINK(QDir::toNativeSeparators(appPath)));
		messageText += QString("<b>").append(tr("You can download the Nero AAC encoder for free from this website:")).append("</b><br>");
		messageText += QString("<tt>").append(LINK(AboutDialog::neroAacUrl)).append("</tt><br><br>");
		messageText += QString("<i>").append(tr("Note: Nero AAC encoder version %1 or newer is required to enable AAC encoding support!").arg(lamexp_version2string("v?.?.?.?", lamexp_toolver_neroaac(), "n/a"))).append("</i><br>");
		messageText += QString("<hr><br>");
		messageText += QString(tr("Alternatively, LameXP supports using QAAC, i.e. the Apple QuickTime/iTunes AAC encoder.")).append("<br>");
		messageText += QString(tr("Please refer to the manual for instructions on how to set up QAAC with LameXP!")).append("<br>");
		if(QMessageBox::information(this, tr("AAC Support Disabled"), NOBREAK(messageText), tr("Discard"), tr("Don't Show Again")) == 1)
		{
			m_settings->neroAacNotificationsEnabled(false);
			ui->actionDisableNeroAacNotifications->setChecked(!m_settings->neroAacNotificationsEnabled());
		}
	}

	//Add files from the command-line
	QStringList addedFiles;
	foreach(const QString &value, arguments.values("add"))
	{
		if(!value.isEmpty())
		{
			QFileInfo currentFile(value);
			qDebug("Adding file from CLI: %s", MUTILS_UTF8(currentFile.absoluteFilePath()));
			addedFiles.append(currentFile.absoluteFilePath());
		}
	}
	if(!addedFiles.isEmpty())
	{
		addFilesDelayed(addedFiles);
	}

	//Add folders from the command-line
	foreach(const QString &value, arguments.values("add-folder"))
	{
		if(!value.isEmpty())
		{
			const QFileInfo currentFile(value);
			qDebug("Adding folder from CLI: %s", MUTILS_UTF8(currentFile.absoluteFilePath()));
			addFolder(currentFile.absoluteFilePath(), false, true);
		}
	}
	foreach(const QString &value, arguments.values("add-recursive"))
	{
		if(!value.isEmpty())
		{
			const QFileInfo currentFile(value);
			qDebug("Adding folder recursively from CLI: %s", MUTILS_UTF8(currentFile.absoluteFilePath()));
			addFolder(currentFile.absoluteFilePath(), true, true);
		}
	}

	//Enable shell integration
	if(m_settings->shellIntegrationEnabled())
	{
		ShellIntegration::install();
	}

	//Make DropBox visible
	if(m_settings->dropBoxWidgetEnabled())
	{
		m_dropBox->setVisible(true);
	}
}

/*
 * Show announce box
 */
void MainWindow::showAnnounceBox(void)
{
	const unsigned int timeout = 8U;

	const QString announceText = QString("%1<br><br>%2<br><tt>%3</tt><br>").arg
	(
		"We are still looking for LameXP translators!",
		"If you are willing to translate LameXP to your language or to complete an existing translation, please refer to:",
		LINK("http://lamexp.sourceforge.net/doc/Translate.html")
	);

	QMessageBox *announceBox = new QMessageBox(QMessageBox::Warning, "We want you!", NOBREAK(announceText), QMessageBox::NoButton, this);
	announceBox->setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
	announceBox->setIconPixmap(QIcon(":/images/Announcement.png").pixmap(64,79));
	
	QTimer *timers[timeout+1];
	QPushButton *buttons[timeout+1];

	for(unsigned int i = 0; i <= timeout; i++)
	{
		QString text = (i > 0) ? QString("%1 (%2)").arg(tr("Discard"), QString::number(i)) : tr("Discard");
		buttons[i] = announceBox->addButton(text, (i > 0) ? QMessageBox::NoRole : QMessageBox::AcceptRole);
	}

	for(unsigned int i = 0; i <= timeout; i++)
	{
		buttons[i]->setEnabled(i == 0);
		buttons[i]->setVisible(i == timeout);
	}

	for(unsigned int i = 0; i < timeout; i++)
	{
		timers[i] = new QTimer(this);
		timers[i]->setSingleShot(true);
		timers[i]->setInterval(1000);
		connect(timers[i], SIGNAL(timeout()), buttons[i+1], SLOT(hide()));
		connect(timers[i], SIGNAL(timeout()), buttons[i], SLOT(show()));
		if(i > 0)
		{
			connect(timers[i], SIGNAL(timeout()), timers[i-1], SLOT(start()));
		}
	}

	timers[timeout-1]->start();
	announceBox->exec();

	for(unsigned int i = 0; i < timeout; i++)
	{
		timers[i]->stop();
		MUTILS_DELETE(timers[i]);
	}

	MUTILS_DELETE(announceBox);
}

// =========================================================
// Main button solots
// =========================================================

/*
 * Encode button
 */
void MainWindow::encodeButtonClicked(void)
{
	static const unsigned __int64 oneGigabyte = 1073741824ui64; 
	static const unsigned __int64 minimumFreeDiskspaceMultiplier = 2ui64;
	static const char *writeTestBuffer = "LAMEXP_WRITE_TEST";
	
	ABORT_IF_BUSY;

	if(m_fileListModel->rowCount() < 1)
	{
		QMessageBox::warning(this, tr("LameXP"), NOBREAK(tr("You must add at least one file to the list before proceeding!")));
		ui->tabWidget->setCurrentIndex(0);
		return;
	}
	
	QString tempFolder = m_settings->customTempPathEnabled() ? m_settings->customTempPath() : MUtils::temp_folder();
	if(!QFileInfo(tempFolder).exists() || !QFileInfo(tempFolder).isDir())
	{
		if(QMessageBox::warning(this, tr("Not Found"), NOBREAK(QString("%1<br><tt>%2</tt>").arg(tr("Your currently selected TEMP folder does not exist anymore:"), QDir::toNativeSeparators(tempFolder))), tr("Restore Default"), tr("Cancel")) == 0)
		{
			SET_CHECKBOX_STATE(ui->checkBoxUseSystemTempFolder, (!m_settings->customTempPathEnabledDefault()));
		}
		return;
	}

	quint64 currentFreeDiskspace = 0;
	if(MUtils::OS::free_diskspace(tempFolder, currentFreeDiskspace))
	{
		if(currentFreeDiskspace < (oneGigabyte * minimumFreeDiskspaceMultiplier))
		{
			QStringList tempFolderParts = tempFolder.split("/", QString::SkipEmptyParts, Qt::CaseInsensitive);
			tempFolderParts.takeLast();
			PLAY_SOUND_OPTIONAL("whammy", false);
			QString lowDiskspaceMsg = QString("%1<br>%2<br><br>%3<br>%4<br>").arg
			(
				tr("There are less than %1 GB of free diskspace available on your system's TEMP folder.").arg(QString::number(minimumFreeDiskspaceMultiplier)),
				tr("It is highly recommend to free up more diskspace before proceeding with the encode!"),
				tr("Your TEMP folder is located at:"),
				QString("<tt>%1</tt>").arg(FSLINK(tempFolderParts.join("\\")))
			);
			switch(QMessageBox::warning(this, tr("Low Diskspace Warning"), NOBREAK(lowDiskspaceMsg), tr("Abort Encoding Process"), tr("Clean Disk Now"), tr("Ignore")))
			{
			case 1:
				QProcess::startDetached(QString("%1/cleanmgr.exe").arg(MUtils::OS::known_folder(MUtils::OS::FOLDER_SYSTEM_DEF)), QStringList() << "/D" << tempFolderParts.first());
			case 0:
				return;
				break;
			default:
				QMessageBox::warning(this, tr("Low Diskspace"), NOBREAK(tr("You are proceeding with low diskspace. Problems might occur!")));
				break;
			}
		}
	}

	switch(m_settings->compressionEncoder())
	{
	case SettingsModel::MP3Encoder:
	case SettingsModel::VorbisEncoder:
	case SettingsModel::AACEncoder:
	case SettingsModel::AC3Encoder:
	case SettingsModel::FLACEncoder:
	case SettingsModel::OpusEncoder:
	case SettingsModel::DCAEncoder:
	case SettingsModel::MACEncoder:
	case SettingsModel::PCMEncoder:
		break;
	default:
		QMessageBox::warning(this, tr("LameXP"), tr("Sorry, an unsupported encoder has been chosen!"));
		ui->tabWidget->setCurrentIndex(3);
		return;
	}

	if(!m_settings->outputToSourceDir())
	{
		QFile writeTest(QString("%1/~%2.txt").arg(m_settings->outputDir(), MUtils::next_rand_str()));
		if(!(writeTest.open(QIODevice::ReadWrite) && (writeTest.write(writeTestBuffer) == strlen(writeTestBuffer))))
		{
			QMessageBox::warning(this, tr("LameXP"), NOBREAK(QString("%1<br>%2<br><br>%3").arg(tr("Cannot write to the selected output directory."), m_settings->outputDir(), tr("Please choose a different directory!"))));
			ui->tabWidget->setCurrentIndex(1);
			return;
		}
		else
		{
			writeTest.close();
			writeTest.remove();
		}
	}

	m_accepted = true;
	close();
}

/*
 * About button
 */
void MainWindow::aboutButtonClicked(void)
{
	ABORT_IF_BUSY;
	WidgetHideHelper hiderHelper(m_dropBox.data());
	QScopedPointer<AboutDialog> aboutBox(new AboutDialog(m_settings, this));
	aboutBox->exec();
}

/*
 * Close button
 */
void MainWindow::closeButtonClicked(void)
{
	ABORT_IF_BUSY;
	close();
}

// =========================================================
// Tab widget slots
// =========================================================

/*
 * Tab page changed
 */
void MainWindow::tabPageChanged(int idx, const bool silent)
{
	resizeEvent(NULL);
	
	//Update "view" menu
	QList<QAction*> actions = m_tabActionGroup->actions();
	for(int i = 0; i < actions.count(); i++)
	{
		bool ok = false;
		int actionIndex = actions.at(i)->data().toInt(&ok);
		if(ok && actionIndex == idx)
		{
			actions.at(i)->setChecked(true);
		}
	}

	//Play tick sound
	if(!silent)
	{
		PLAY_SOUND_OPTIONAL("tick", true);
	}

	int initialWidth = this->width();
	int maximumWidth = QApplication::desktop()->availableGeometry().width();

	//Make sure all tab headers are fully visible
	if(this->isVisible())
	{
		int delta = ui->tabWidget->sizeHint().width() - ui->tabWidget->width();
		if(delta > 0)
		{
			this->resize(qMin(this->width() + delta, maximumWidth), this->height());
		}
	}

	//Tab specific operations
	if(idx == ui->tabWidget->indexOf(ui->tabOptions) && ui->scrollArea->widget() && this->isVisible())
	{
		ui->scrollArea->widget()->updateGeometry();
		ui->scrollArea->viewport()->updateGeometry();
		qApp->processEvents();
		int delta = ui->scrollArea->widget()->width() - ui->scrollArea->viewport()->width();
		if(delta > 0)
		{
			this->resize(qMin(this->width() + delta, maximumWidth), this->height());
		}
	}
	else if(idx == ui->tabWidget->indexOf(ui->tabSourceFiles))
	{
		m_dropNoteLabel->setGeometry(0, 0, ui->sourceFileView->width(), ui->sourceFileView->height());
	}
	else if(idx == ui->tabWidget->indexOf(ui->tabOutputDir))
	{
		if(!m_fileSystemModel)
		{
			QTimer::singleShot(125, this, SLOT(initOutputFolderModel()));
		}
		else
		{
			CENTER_CURRENT_OUTPUT_FOLDER_DELAYED();
		}
	}

	//Center window around previous position
	if(initialWidth < this->width())
	{
		QPoint prevPos = this->pos();
		int delta = (this->width() - initialWidth) >> 2;
		move(prevPos.x() - delta, prevPos.y());
	}
}

/*
 * Tab action triggered
 */
void MainWindow::tabActionActivated(QAction *action)
{
	if(action && action->data().isValid())
	{
		bool ok = false;
		int index = action->data().toInt(&ok);
		if(ok)
		{
			ui->tabWidget->setCurrentIndex(index);
		}
	}
}

// =========================================================
// Menubar slots
// =========================================================

/*
 * Handle corner widget Event
 */
void MainWindow::cornerWidgetEventOccurred(QWidget* /*sender*/, QEvent *const event)
{
	if(event->type() == QEvent::MouseButtonPress)
	{
		QTimer::singleShot(0, this, SLOT(checkUpdatesActionActivated()));
	}
}

// =========================================================
// View menu slots
// =========================================================

/*
 * Style action triggered
 */
void MainWindow::styleActionActivated(QAction *const action)
{
	//Change style setting
	if(action && action->data().isValid())
	{
		bool ok = false;
		int actionIndex = action->data().toInt(&ok);
		if(ok)
		{
			m_settings->interfaceStyle(actionIndex);
		}
	}

	//Set up the new style
	switch(m_settings->interfaceStyle())
	{
	case 1:
		if(ui->actionStyleCleanlooks->isEnabled())
		{
			ui->actionStyleCleanlooks->setChecked(true);
			QApplication::setStyle(new QCleanlooksStyle());
			break;
		}
	case 2:
		if(ui->actionStyleWindowsVista->isEnabled())
		{
			ui->actionStyleWindowsVista->setChecked(true);
			QApplication::setStyle(new QWindowsVistaStyle());
			break;
		}
	case 3:
		if(ui->actionStyleWindowsXP->isEnabled())
		{
			ui->actionStyleWindowsXP->setChecked(true);
			QApplication::setStyle(new QWindowsXPStyle());
			break;
		}
	case 4:
		if(ui->actionStyleWindowsClassic->isEnabled())
		{
			ui->actionStyleWindowsClassic->setChecked(true);
			QApplication::setStyle(new QWindowsStyle());
			break;
		}
	default:
		ui->actionStylePlastique->setChecked(true);
		QApplication::setStyle(new QPlastiqueStyle());
		break;
	}

	//Force re-translate after style change
	if(QEvent *e = new QEvent(QEvent::LanguageChange))
	{
		changeEvent(e);
		MUTILS_DELETE(e);
	}

	//Make transparent
	const type_info &styleType = typeid(*qApp->style());
	const bool bTransparent = ((typeid(QWindowsVistaStyle) == styleType) || (typeid(QWindowsXPStyle) == styleType));
	MAKE_TRANSPARENT(ui->scrollArea, bTransparent);

	//Also force a re-size event
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	resizeEvent(NULL);
}

/*
 * Language action triggered
 */
void MainWindow::languageActionActivated(QAction *action)
{
	if(action->data().type() == QVariant::String)
	{
		QString langId = action->data().toString();

		if(MUtils::Translation::install_translator(langId))
		{
			action->setChecked(true);
			ui->actionLoadTranslationFromFile->setChecked(false);
			m_settings->currentLanguage(langId);
			m_settings->currentLanguageFile(QString());
		}
	}
}

/*
 * Load language from file action triggered
 */
void MainWindow::languageFromFileActionActivated(bool /*checked*/)
{
	QFileDialog dialog(this, tr("Load Translation"));
	dialog.setFileMode(QFileDialog::ExistingFile);
	dialog.setNameFilter(QString("%1 (*.qm)").arg(tr("Translation Files")));

	if(dialog.exec())
	{
		QStringList selectedFiles = dialog.selectedFiles();
		const QString qmFile = QFileInfo(selectedFiles.first()).canonicalFilePath();
		if(MUtils::Translation::install_translator_from_file(qmFile))
		{
			QList<QAction*> actions = m_languageActionGroup->actions();
			while(!actions.isEmpty())
			{
				actions.takeFirst()->setChecked(false);
			}
			ui->actionLoadTranslationFromFile->setChecked(true);
			m_settings->currentLanguageFile(qmFile);
		}
		else
		{
			languageActionActivated(m_languageActionGroup->actions().first());
		}
	}
}

// =========================================================
// Tools menu slots
// =========================================================

/*
 * Disable update reminder action
 */
void MainWindow::disableUpdateReminderActionTriggered(const bool checked)
{
	if(checked)
	{
		if(0 == QMessageBox::question(this, tr("Disable Update Reminder"), NOBREAK(tr("Do you really want to disable the update reminder?")), tr("Yes"), tr("No"), QString(), 1))
		{
			QMessageBox::information(this, tr("Update Reminder"), NOBREAK(QString("%1<br>%2").arg(tr("The update reminder has been disabled."), tr("Please remember to check for updates at regular intervals!"))));
			m_settings->autoUpdateEnabled(false);
		}
		else
		{
			m_settings->autoUpdateEnabled(true);
		}
	}
	else
	{
			QMessageBox::information(this, tr("Update Reminder"), NOBREAK(tr("The update reminder has been re-enabled.")));
			m_settings->autoUpdateEnabled(true);
	}

	ui->actionDisableUpdateReminder->setChecked(!m_settings->autoUpdateEnabled());
}

/*
 * Disable sound effects action
 */
void MainWindow::disableSoundsActionTriggered(const bool checked)
{
	if(checked)
	{
		if(0 == QMessageBox::question(this, tr("Disable Sound Effects"), NOBREAK(tr("Do you really want to disable all sound effects?")), tr("Yes"), tr("No"), QString(), 1))
		{
			QMessageBox::information(this, tr("Sound Effects"), NOBREAK(tr("All sound effects have been disabled.")));
			m_settings->soundsEnabled(false);
		}
		else
		{
			m_settings->soundsEnabled(true);
		}
	}
	else
	{
			QMessageBox::information(this, tr("Sound Effects"), NOBREAK(tr("The sound effects have been re-enabled.")));
			m_settings->soundsEnabled(true);
	}

	ui->actionDisableSounds->setChecked(!m_settings->soundsEnabled());
}

/*
 * Disable Nero AAC encoder action
 */
void MainWindow::disableNeroAacNotificationsActionTriggered(const bool checked)
{
	if(checked)
	{
		if(0 == QMessageBox::question(this, tr("Nero AAC Notifications"), NOBREAK(tr("Do you really want to disable all Nero AAC Encoder notifications?")), tr("Yes"), tr("No"), QString(), 1))
		{
			QMessageBox::information(this, tr("Nero AAC Notifications"), NOBREAK(tr("All Nero AAC Encoder notifications have been disabled.")));
			m_settings->neroAacNotificationsEnabled(false);
		}
		else
		{
			m_settings->neroAacNotificationsEnabled(true);
		}
	}
	else
	{
		QMessageBox::information(this, tr("Nero AAC Notifications"), NOBREAK(tr("The Nero AAC Encoder notifications have been re-enabled.")));
		m_settings->neroAacNotificationsEnabled(true);
	}

	ui->actionDisableNeroAacNotifications->setChecked(!m_settings->neroAacNotificationsEnabled());
}

/*
 * Disable slow startup action
 */
void MainWindow::disableSlowStartupNotificationsActionTriggered(const bool checked)
{
	if(checked)
	{
		if(0 == QMessageBox::question(this, tr("Slow Startup Notifications"), NOBREAK(tr("Do you really want to disable the slow startup notifications?")), tr("Yes"), tr("No"), QString(), 1))
		{
			QMessageBox::information(this, tr("Slow Startup Notifications"), NOBREAK(tr("The slow startup notifications have been disabled.")));
			m_settings->antivirNotificationsEnabled(false);
		}
		else
		{
			m_settings->antivirNotificationsEnabled(true);
		}
	}
	else
	{
			QMessageBox::information(this, tr("Slow Startup Notifications"), NOBREAK(tr("The slow startup notifications have been re-enabled.")));
			m_settings->antivirNotificationsEnabled(true);
	}

	ui->actionDisableSlowStartupNotifications->setChecked(!m_settings->antivirNotificationsEnabled());
}

/*
 * Import a Cue Sheet file
 */
void MainWindow::importCueSheetActionTriggered(bool /*checked*/)
{
	ABORT_IF_BUSY;
	WidgetHideHelper hiderHelper(m_dropBox.data());

	while(true)
	{
		int result = 0;
		QString selectedCueFile;

		if(MUtils::GUI::themes_enabled())
		{
			selectedCueFile = QFileDialog::getOpenFileName(this, tr("Open Cue Sheet"), m_settings->mostRecentInputPath(), QString("%1 (*.cue)").arg(tr("Cue Sheet File")));
		}
		else
		{
			QFileDialog dialog(this, tr("Open Cue Sheet"));
			dialog.setFileMode(QFileDialog::ExistingFile);
			dialog.setNameFilter(QString("%1 (*.cue)").arg(tr("Cue Sheet File")));
			dialog.setDirectory(m_settings->mostRecentInputPath());
			if(dialog.exec())
			{
				selectedCueFile = dialog.selectedFiles().first();
			}
		}

		if(!selectedCueFile.isEmpty())
		{
			m_settings->mostRecentInputPath(QFileInfo(selectedCueFile).canonicalPath());
			FileListBlockHelper fileListBlocker(m_fileListModel);
			QScopedPointer<CueImportDialog> cueImporter(new CueImportDialog(this, m_fileListModel, selectedCueFile, m_settings));
			result = cueImporter->exec();
		}

		if(result != (-1))
		{
			qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
			ui->sourceFileView->update();
			qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
			ui->sourceFileView->scrollToBottom();
			qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
			break;
		}
	}
}

/*
 * Show the "drop box" widget
 */
void MainWindow::showDropBoxWidgetActionTriggered(bool /*checked*/)
{
	m_settings->dropBoxWidgetEnabled(true);
	
	if(!m_dropBox->isVisible())
	{
		m_dropBox->show();
		QTimer::singleShot(2500, m_dropBox.data(), SLOT(showToolTip()));
	}
	
	MUtils::GUI::blink_window(m_dropBox.data());
}

/*
 * Check for beta (pre-release) updates
 */
void MainWindow::checkForBetaUpdatesActionTriggered(const bool checked)
{	
	bool checkUpdatesNow = false;
	
	if(checked)
	{
		if(0 == QMessageBox::question(this, tr("Beta Updates"), NOBREAK(tr("Do you really want LameXP to check for Beta (pre-release) updates?")), tr("Yes"), tr("No"), QString(), 1))
		{
			if(0 == QMessageBox::information(this, tr("Beta Updates"), NOBREAK(tr("LameXP will check for Beta (pre-release) updates from now on.")), tr("Check Now"), tr("Discard")))
			{
				checkUpdatesNow = true;
			}
			m_settings->autoUpdateCheckBeta(true);
		}
		else
		{
			m_settings->autoUpdateCheckBeta(false);
		}
	}
	else
	{
		QMessageBox::information(this, tr("Beta Updates"), NOBREAK(tr("LameXP will <i>not</i> check for Beta (pre-release) updates from now on.")));
		m_settings->autoUpdateCheckBeta(false);
	}

	ui->actionCheckForBetaUpdates->setChecked(m_settings->autoUpdateCheckBeta());

	if(checkUpdatesNow)
	{
		if(checkForUpdates())
		{
			QApplication::quit();
		}
	}
}

/*
 * Hibernate computer action
 */
void MainWindow::hibernateComputerActionTriggered(const bool checked)
{
	if(checked)
	{
		if(0 == QMessageBox::question(this, tr("Hibernate Computer"), NOBREAK(tr("Do you really want the computer to be hibernated on shutdown?")), tr("Yes"), tr("No"), QString(), 1))
		{
			QMessageBox::information(this, tr("Hibernate Computer"), NOBREAK(tr("LameXP will hibernate the computer on shutdown from now on.")));
			m_settings->hibernateComputer(true);
		}
		else
		{
			m_settings->hibernateComputer(false);
		}
	}
	else
	{
			QMessageBox::information(this, tr("Hibernate Computer"), NOBREAK(tr("LameXP will <i>not</i> hibernate the computer on shutdown from now on.")));
			m_settings->hibernateComputer(false);
	}

	ui->actionHibernateComputer->setChecked(m_settings->hibernateComputer());
}

/*
 * Disable shell integration action
 */
void MainWindow::disableShellIntegrationActionTriggered(const bool checked)
{
	if(checked)
	{
		if(0 == QMessageBox::question(this, tr("Shell Integration"), NOBREAK(tr("Do you really want to disable the LameXP shell integration?")), tr("Yes"), tr("No"), QString(), 1))
		{
			ShellIntegration::remove();
			QMessageBox::information(this, tr("Shell Integration"), NOBREAK(tr("The LameXP shell integration has been disabled.")));
			m_settings->shellIntegrationEnabled(false);
		}
		else
		{
			m_settings->shellIntegrationEnabled(true);
		}
	}
	else
	{
			ShellIntegration::install();
			QMessageBox::information(this, tr("Shell Integration"), NOBREAK(tr("The LameXP shell integration has been re-enabled.")));
			m_settings->shellIntegrationEnabled(true);
	}

	ui->actionDisableShellIntegration->setChecked(!m_settings->shellIntegrationEnabled());
	
	if(lamexp_version_portable() && ui->actionDisableShellIntegration->isChecked())
	{
		ui->actionDisableShellIntegration->setEnabled(false);
	}
}

void MainWindow::disableTrayIconActionTriggered(const bool checked)
{
	if (checked)
	{
		QMessageBox::information(this, tr("Notification Icon"), NOBREAK(tr("The notification icon has been disabled.")));
	}
	else
	{
		QMessageBox::information(this, tr("Notification Icon"), NOBREAK(tr("The notification icon has been re-enabled.")));
	}

	m_settings->disableTrayIcon(checked);
}

// =========================================================
// Help menu slots
// =========================================================

/*
 * Visit homepage action
 */
void MainWindow::visitHomepageActionActivated(void)
{
	if(QAction *action = dynamic_cast<QAction*>(QObject::sender()))
	{
		if(action->data().isValid() && (action->data().type() == QVariant::String))
		{
			QDesktopServices::openUrl(QUrl(action->data().toString()));
		}
	}
}

/*
 * Show document
 */
void MainWindow::documentActionActivated(void)
{
	if(QAction *action = dynamic_cast<QAction*>(QObject::sender()))
	{
		openDocumentLink(action);
	}
}

/*
 * Check for updates action
 */
void MainWindow::checkUpdatesActionActivated(void)
{
	ABORT_IF_BUSY;
	WidgetHideHelper hiderHelper(m_dropBox.data());
	
	if(checkForUpdates())
	{
		QApplication::quit();
	}
}

// =========================================================
// Source file slots
// =========================================================

/*
 * Add file(s) button
 */
void MainWindow::addFilesButtonClicked(void)
{
	ABORT_IF_BUSY;
	WidgetHideHelper hiderHelper(m_dropBox.data());

	if(MUtils::GUI::themes_enabled() && (!MUTILS_DEBUG))
	{
		QStringList fileTypeFilters = DecoderRegistry::getSupportedTypes();
		QStringList selectedFiles = QFileDialog::getOpenFileNames(this, tr("Add file(s)"), m_settings->mostRecentInputPath(), fileTypeFilters.join(";;"));
		if(!selectedFiles.isEmpty())
		{
			m_settings->mostRecentInputPath(QFileInfo(selectedFiles.first()).canonicalPath());
			addFiles(selectedFiles);
		}
	}
	else
	{
		QFileDialog dialog(this, tr("Add file(s)"));
		QStringList fileTypeFilters = DecoderRegistry::getSupportedTypes();
		dialog.setFileMode(QFileDialog::ExistingFiles);
		dialog.setNameFilter(fileTypeFilters.join(";;"));
		dialog.setDirectory(m_settings->mostRecentInputPath());
		if(dialog.exec())
		{
			QStringList selectedFiles = dialog.selectedFiles();
			if(!selectedFiles.isEmpty())
			{
				m_settings->mostRecentInputPath(QFileInfo(selectedFiles.first()).canonicalPath());
				addFiles(selectedFiles);
			}
		}
	}
}

/*
 * Open folder action
 */
void MainWindow::openFolderActionActivated(void)
{
	ABORT_IF_BUSY;
	QString selectedFolder;
	
	if(QAction *action = dynamic_cast<QAction*>(QObject::sender()))
	{
		WidgetHideHelper hiderHelper(m_dropBox.data());
		if(MUtils::GUI::themes_enabled())
		{
			selectedFolder = QFileDialog::getExistingDirectory(this, tr("Add Folder"), m_settings->mostRecentInputPath());
		}
		else
		{
			QFileDialog dialog(this, tr("Add Folder"));
			dialog.setFileMode(QFileDialog::DirectoryOnly);
			dialog.setDirectory(m_settings->mostRecentInputPath());
			if(dialog.exec())
			{
				selectedFolder = dialog.selectedFiles().first();
			}
		}

		if(selectedFolder.isEmpty())
		{
			return;
		}

		QStringList filterItems = DecoderRegistry::getSupportedExts();
		filterItems.prepend("*.*");

		bool okay;
		QString filterStr = QInputDialog::getItem(this, tr("Filter Files"), tr("Select filename filter:"), filterItems, 0, false, &okay).trimmed();
		if(!okay)
		{
			return;
		}

		QRegExp regExp("\\*\\.([A-Za-z0-9]+)", Qt::CaseInsensitive);
		if(regExp.lastIndexIn(filterStr) >= 0)
		{
			filterStr = regExp.cap(1).trimmed();
		}
		else
		{
			filterStr.clear();
		}

		m_settings->mostRecentInputPath(QDir(selectedFolder).canonicalPath());
		addFolder(selectedFolder, action->data().toBool(), false, filterStr);
	}
}

/*
 * Remove file button
 */
void MainWindow::removeFileButtonClicked(void)
{
	const QItemSelectionModel *const selection = ui->sourceFileView->selectionModel();
	if(selection && selection->hasSelection())
	{
		int firstRow = -1;
		const QModelIndexList selectedRows = INVERT_LIST(selection->selectedRows());
		if(!selectedRows.isEmpty())
		{
			FileListBlockHelper fileListBlocker(m_fileListModel);
			firstRow = selectedRows.last().row();
			for(QModelIndexList::ConstIterator iter = selectedRows.constBegin(); iter != selectedRows.constEnd(); iter++)
			{
				if(!m_fileListModel->removeFile(*iter))
				{
					break;
				}
			}
		}
		if(m_fileListModel->rowCount() > 0)
		{
			const QModelIndex position = m_fileListModel->index(((firstRow >= 0) && (firstRow < m_fileListModel->rowCount())) ? firstRow : (m_fileListModel->rowCount() - 1), 0);
			ui->sourceFileView->selectRow(position.row());
			ui->sourceFileView->scrollTo(position, QAbstractItemView::PositionAtCenter);
		}
	}
	else
	{
		MUtils::Sound::beep(MUtils::Sound::BEEP_WRN);
	}
}

/*
 * Clear files button
 */
void MainWindow::clearFilesButtonClicked(void)
{
	if(m_fileListModel->rowCount() > 0)
	{
		m_fileListModel->clearFiles();
	}
	else
	{
		MUtils::Sound::beep(MUtils::Sound::BEEP_WRN);
	}
}

/*
 * Move file up button
 */
void MainWindow::fileUpButtonClicked(void)
{
	moveSelectedFiles(true);
}

/*
 * Move file down button
 */
void MainWindow::fileDownButtonClicked(void)
{
	moveSelectedFiles(false);
}

/*
 * Show details button
 */
void MainWindow::showDetailsButtonClicked(void)
{
	ABORT_IF_BUSY;

	int iResult = 0;
	QModelIndex index = ui->sourceFileView->currentIndex();
	
	if(index.isValid())
	{
		ui->sourceFileView->selectRow(index.row());
		QScopedPointer<MetaInfoDialog> metaInfoDialog(new MetaInfoDialog(this));
		forever
		{
			AudioFileModel &file = (*m_fileListModel)[index];
			WidgetHideHelper hiderHelper(m_dropBox.data());
			iResult = metaInfoDialog->exec(file, index.row() > 0, index.row() < m_fileListModel->rowCount() - 1);
		
			//Copy all info to Meta Info tab
			if(iResult == INT_MAX)
			{
				m_metaInfoModel->assignInfoFrom(file);
				ui->tabWidget->setCurrentIndex(ui->tabWidget->indexOf(ui->tabMetaData));
				break;
			}

			if(iResult > 0)
			{
				index = m_fileListModel->index(index.row() + 1, index.column()); 
				ui->sourceFileView->selectRow(index.row());
				continue;
			}
			else if(iResult < 0)
			{
				index = m_fileListModel->index(index.row() - 1, index.column()); 
				ui->sourceFileView->selectRow(index.row());
				continue;
			}		

			break; /*close dilalog now*/
		}
	}
	else
	{
		MUtils::Sound::beep(MUtils::Sound::BEEP_WRN);
	}

	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	sourceFilesScrollbarMoved(0);
}

/*
 * Show context menu for source files
 */
void MainWindow::sourceFilesContextMenu(const QPoint &pos)
{
	QAbstractScrollArea *scrollArea = dynamic_cast<QAbstractScrollArea*>(QObject::sender());
	QWidget *sender = scrollArea ? scrollArea->viewport() : dynamic_cast<QWidget*>(QObject::sender());

	if(sender)
	{
		if(pos.x() <= sender->width() && pos.y() <= sender->height() && pos.x() >= 0 && pos.y() >= 0)
		{
			m_sourceFilesContextMenu->popup(sender->mapToGlobal(pos));
		}
	}
}

/*
 * Scrollbar of source files moved
 */
void MainWindow::sourceFilesScrollbarMoved(int)
{
	ui->sourceFileView->resizeColumnToContents(0);
}

/*
 * Open selected file in external player
 */
void MainWindow::previewContextActionTriggered(void)
{
	QModelIndex index = ui->sourceFileView->currentIndex();
	if(!index.isValid())
	{
		return;
	}

	if(!MUtils::OS::open_media_file(m_fileListModel->getFile(index).filePath()))
	{
		qDebug("Player not found, falling back to default application...");
		QDesktopServices::openUrl(QString("file:///").append(m_fileListModel->getFile(index).filePath()));
	}
}

/*
 * Find selected file in explorer
 */
void MainWindow::findFileContextActionTriggered(void)
{
	QModelIndex index = ui->sourceFileView->currentIndex();
	if(index.isValid())
	{
		const QString systemToolsPath = MUtils::OS::known_folder(MUtils::OS::FOLDER_SYSROOT);
		if(!systemToolsPath.isEmpty())
		{
			QFileInfo explorer(QString("%1/explorer.exe").arg(systemToolsPath));
			if(explorer.exists() && explorer.isFile())
			{
				QProcess::execute(explorer.canonicalFilePath(), QStringList() << "/select," << QDir::toNativeSeparators(m_fileListModel->getFile(index).filePath()));
				return;
			}
		}
		else
		{
			qWarning("System tools directory could not be detected!");
		}
	}
}

/*
 * Add all dropped files
 */
void MainWindow::handleDroppedFiles(void)
{
	ABORT_IF_BUSY;

	static const int MIN_COUNT = 16;
	const QString bannerText = tr("Loading dropped files or folders, please wait...");
	bool bUseBanner = false;

	showBanner(bannerText, bUseBanner, (m_droppedFileList->count() >= MIN_COUNT));

	QStringList droppedFiles;
	while(!m_droppedFileList->isEmpty())
	{
		QFileInfo file(m_droppedFileList->takeFirst().toLocalFile());
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

		if(file.exists())
		{
			if(file.isFile())
			{
				qDebug("Dropped File: %s", MUTILS_UTF8(file.canonicalFilePath()));
				droppedFiles << file.canonicalFilePath();
				continue;
			}
			else if(file.isDir())
			{
				qDebug("Dropped Folder: %s", MUTILS_UTF8(file.canonicalFilePath()));
				QFileInfoList list = QDir(file.canonicalFilePath()).entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);
				if(list.count() > 0)
				{
					showBanner(bannerText, bUseBanner, (list.count() >= MIN_COUNT));
					for(QFileInfoList::ConstIterator iter = list.constBegin(); iter != list.constEnd(); iter++)
					{
						droppedFiles << (*iter).canonicalFilePath();
					}
				}
			}
		}
	}
	
	if(bUseBanner)
	{
		m_banner->close();
	}

	if(!droppedFiles.isEmpty())
	{
		addFiles(droppedFiles);
	}
}

/*
 * Add all pending files
 */
void MainWindow::handleDelayedFiles(void)
{
	m_delayedFileTimer->stop();

	if(m_delayedFileList->isEmpty())
	{
		return;
	}

	if(BANNER_VISIBLE)
	{
		m_delayedFileTimer->start(5000);
		return;
	}
	
	if(ui->tabWidget->currentIndex() != 0)
	{
		SignalBlockHelper signalBlockHelper(ui->tabWidget);
		ui->tabWidget->setCurrentIndex(0);
		tabPageChanged(ui->tabWidget->currentIndex(), true);
	}
	
	QStringList selectedFiles;
	while(!m_delayedFileList->isEmpty())
	{
		QFileInfo currentFile = QFileInfo(m_delayedFileList->takeFirst());
		if(!currentFile.exists() || !currentFile.isFile())
		{
			continue;
		}
		selectedFiles << currentFile.canonicalFilePath();
	}
	
	addFiles(selectedFiles);
}

/*
 * Export Meta tags to CSV file
 */
void MainWindow::exportCsvContextActionTriggered(void)
{
	ABORT_IF_BUSY;
	WidgetHideHelper hiderHelper(m_dropBox.data());

	QString selectedCsvFile;
	if(MUtils::GUI::themes_enabled())
	{
		selectedCsvFile = QFileDialog::getSaveFileName(this, tr("Save CSV file"), m_settings->mostRecentInputPath(), QString("%1 (*.csv)").arg(tr("CSV File")));
	}
	else
	{
		QFileDialog dialog(this, tr("Save CSV file"));
		dialog.setFileMode(QFileDialog::AnyFile);
		dialog.setAcceptMode(QFileDialog::AcceptSave);
		dialog.setNameFilter(QString("%1 (*.csv)").arg(tr("CSV File")));
		dialog.setDirectory(m_settings->mostRecentInputPath());
		if(dialog.exec())
		{
			selectedCsvFile = dialog.selectedFiles().first();
		}
	}

	if(!selectedCsvFile.isEmpty())
	{
		m_settings->mostRecentInputPath(QFileInfo(selectedCsvFile).canonicalPath());
		switch(m_fileListModel->exportToCsv(selectedCsvFile))
		{
		case FileListModel::CsvError_NoTags:
			QMessageBox::critical(this, tr("CSV Export"), NOBREAK(tr("Sorry, there are no meta tags that can be exported!")));
			break;
		case FileListModel::CsvError_FileOpen:
			QMessageBox::critical(this, tr("CSV Export"), NOBREAK(tr("Sorry, failed to open CSV file for writing!")));
			break;
		case FileListModel::CsvError_FileWrite:
			QMessageBox::critical(this, tr("CSV Export"), NOBREAK(tr("Sorry, failed to write to the CSV file!")));
			break;
		case FileListModel::CsvError_OK:
			QMessageBox::information(this, tr("CSV Export"), NOBREAK(tr("The CSV files was created successfully!")));
			break;
		default:
			qWarning("exportToCsv: Unknown return code!");
		}
	}
}


/*
 * Import Meta tags from CSV file
 */
void MainWindow::importCsvContextActionTriggered(void)
{
	ABORT_IF_BUSY;
	WidgetHideHelper hiderHelper(m_dropBox.data());

	QString selectedCsvFile;
	if(MUtils::GUI::themes_enabled())
	{
		selectedCsvFile = QFileDialog::getOpenFileName(this, tr("Open CSV file"), m_settings->mostRecentInputPath(), QString("%1 (*.csv)").arg(tr("CSV File")));
	}
	else
	{
		QFileDialog dialog(this, tr("Open CSV file"));
		dialog.setFileMode(QFileDialog::ExistingFile);
		dialog.setNameFilter(QString("%1 (*.csv)").arg(tr("CSV File")));
		dialog.setDirectory(m_settings->mostRecentInputPath());
		if(dialog.exec())
		{
			selectedCsvFile = dialog.selectedFiles().first();
		}
	}

	if(!selectedCsvFile.isEmpty())
	{
		m_settings->mostRecentInputPath(QFileInfo(selectedCsvFile).canonicalPath());
		switch(m_fileListModel->importFromCsv(this, selectedCsvFile))
		{
		case FileListModel::CsvError_FileOpen:
			QMessageBox::critical(this, tr("CSV Import"), NOBREAK(tr("Sorry, failed to open CSV file for reading!")));
			break;
		case FileListModel::CsvError_FileRead:
			QMessageBox::critical(this, tr("CSV Import"), NOBREAK(tr("Sorry, failed to read from the CSV file!")));
			break;
		case FileListModel::CsvError_NoTags:
			QMessageBox::critical(this, tr("CSV Import"), NOBREAK(tr("Sorry, the CSV file does not contain any known fields!")));
			break;
		case FileListModel::CsvError_Incomplete:
			QMessageBox::warning(this, tr("CSV Import"), NOBREAK(tr("CSV file is incomplete. Not all files were updated!")));
			break;
		case FileListModel::CsvError_OK:
			QMessageBox::information(this, tr("CSV Import"), NOBREAK(tr("The CSV files was imported successfully!")));
			break;
		case FileListModel::CsvError_Aborted:
			/* User aborted, ignore! */
			break;
		default:
			qWarning("exportToCsv: Unknown return code!");
		}
	}
}

/*
 * Show or hide Drag'n'Drop notice after model reset
 */
void MainWindow::sourceModelChanged(void)
{
	m_dropNoteLabel->setVisible(m_fileListModel->rowCount() <= 0);
}

// =========================================================
// Output folder slots
// =========================================================

/*
 * Output folder changed (mouse clicked)
 */
void MainWindow::outputFolderViewClicked(const QModelIndex &index)
{
	if(index.isValid() && (ui->outputFolderView->currentIndex() != index))
	{
		ui->outputFolderView->setCurrentIndex(index);
	}
	
	if(m_fileSystemModel && index.isValid())
	{
		QString selectedDir = m_fileSystemModel->filePath(index);
		if(selectedDir.length() < 3) selectedDir.append(QDir::separator());
		ui->outputFolderLabel->setText(QDir::toNativeSeparators(selectedDir));
		ui->outputFolderLabel->setToolTip(ui->outputFolderLabel->text());
		m_settings->outputDir(selectedDir);
	}
	else
	{
		ui->outputFolderLabel->setText(QDir::toNativeSeparators(m_settings->outputDir()));
		ui->outputFolderLabel->setToolTip(ui->outputFolderLabel->text());
	}
}

/*
 * Output folder changed (mouse moved)
 */
void MainWindow::outputFolderViewMoved(const QModelIndex &index)
{
	if(QApplication::mouseButtons() & Qt::LeftButton)
	{
		outputFolderViewClicked(index);
	}
}

/*
 * Goto desktop button
 */
void MainWindow::gotoDesktopButtonClicked(void)
{
	if(!m_fileSystemModel)
	{
		qWarning("File system model not initialized yet!");
		return;
	}
	
	const QString desktopPath = MUtils::OS::known_folder(MUtils::OS::FOLDER_DESKTOP_USER);
	
	if(!desktopPath.isEmpty() && QDir(desktopPath).exists())
	{
		ui->outputFolderView->setCurrentIndex(m_fileSystemModel->index(desktopPath));
		outputFolderViewClicked(ui->outputFolderView->currentIndex());
		CENTER_CURRENT_OUTPUT_FOLDER_DELAYED();
	}
	else
	{
		ui->buttonGotoDesktop->setEnabled(false);
	}
}

/*
 * Goto home folder button
 */
void MainWindow::gotoHomeFolderButtonClicked(void)
{
	if(!m_fileSystemModel)
	{
		qWarning("File system model not initialized yet!");
		return;
	}

	const QString homePath = MUtils::OS::known_folder(MUtils::OS::FOLDER_PROFILE_USER);

	if(!homePath.isEmpty() && QDir(homePath).exists())
	{
		ui->outputFolderView->setCurrentIndex(m_fileSystemModel->index(homePath));
		outputFolderViewClicked(ui->outputFolderView->currentIndex());
		CENTER_CURRENT_OUTPUT_FOLDER_DELAYED();
	}
	else
	{
		ui->buttonGotoHome->setEnabled(false);
	}
}

/*
 * Goto music folder button
 */
void MainWindow::gotoMusicFolderButtonClicked(void)
{
	if(!m_fileSystemModel)
	{
		qWarning("File system model not initialized yet!");
		return;
	}

	const QString musicPath = MUtils::OS::known_folder(MUtils::OS::FOLDER_MUSIC_USER);
	
	if(!musicPath.isEmpty() && QDir(musicPath).exists())
	{
		ui->outputFolderView->setCurrentIndex(m_fileSystemModel->index(musicPath));
		outputFolderViewClicked(ui->outputFolderView->currentIndex());
		CENTER_CURRENT_OUTPUT_FOLDER_DELAYED();
	}
	else
	{
		ui->buttonGotoMusic->setEnabled(false);
	}
}

/*
 * Goto music favorite output folder
 */
void MainWindow::gotoFavoriteFolder(void)
{
	if(!m_fileSystemModel)
	{
		qWarning("File system model not initialized yet!");
		return;
	}

	QAction *item = dynamic_cast<QAction*>(QObject::sender());
	
	if(item)
	{
		QDir path(item->data().toString());
		if(path.exists())
		{
			ui->outputFolderView->setCurrentIndex(m_fileSystemModel->index(path.canonicalPath()));
			outputFolderViewClicked(ui->outputFolderView->currentIndex());
			CENTER_CURRENT_OUTPUT_FOLDER_DELAYED();
		}
		else
		{
			MUtils::Sound::beep(MUtils::Sound::BEEP_ERR);
			m_outputFolderFavoritesMenu->removeAction(item);
			item->deleteLater();
		}
	}
}

/*
 * Make folder button
 */
void MainWindow::makeFolderButtonClicked(void)
{
	ABORT_IF_BUSY;

	if(m_fileSystemModel.isNull())
	{
		qWarning("File system model not initialized yet!");
		return;
	}

	QDir basePath(m_fileSystemModel->fileInfo(ui->outputFolderView->currentIndex()).absoluteFilePath());
	QString suggestedName = tr("New Folder");

	if(!m_metaData->artist().isEmpty() && !m_metaData->album().isEmpty())
	{
		suggestedName = QString("%1 - %2").arg(m_metaData->artist(),m_metaData->album());
	}
	else if(!m_metaData->artist().isEmpty())
	{
		suggestedName = m_metaData->artist();
	}
	else if(!m_metaData->album().isEmpty())
	{
		suggestedName = m_metaData->album();
	}
	else
	{
		for(int i = 0; i < m_fileListModel->rowCount(); i++)
		{
			const AudioFileModel &audioFile = m_fileListModel->getFile(m_fileListModel->index(i, 0));
			const AudioFileModel_MetaInfo &fileMetaInfo = audioFile.metaInfo();

			if(!fileMetaInfo.album().isEmpty() || !fileMetaInfo.artist().isEmpty())
			{
				if(!fileMetaInfo.artist().isEmpty() && !fileMetaInfo.album().isEmpty())
				{
					suggestedName = QString("%1 - %2").arg(fileMetaInfo.artist(), fileMetaInfo.album());
				}
				else if(!fileMetaInfo.artist().isEmpty())
				{
					suggestedName = fileMetaInfo.artist();
				}
				else if(!fileMetaInfo.album().isEmpty())
				{
					suggestedName = fileMetaInfo.album();
				}
				break;
			}
		}
	}
	
	suggestedName = MUtils::clean_file_name(suggestedName, true);

	while(true)
	{
		bool bApplied = false;
		QString folderName = QInputDialog::getText(this, tr("New Folder"), tr("Enter the name of the new folder:").leftJustified(96, ' '), QLineEdit::Normal, suggestedName, &bApplied, Qt::WindowStaysOnTopHint).simplified();

		if(bApplied)
		{
			folderName = MUtils::clean_file_path(folderName.simplified(), true);

			if(folderName.isEmpty())
			{
				MUtils::Sound::beep(MUtils::Sound::BEEP_ERR);
				continue;
			}

			int i = 1;
			QString newFolder = folderName;

			while(basePath.exists(newFolder))
			{
				newFolder = QString(folderName).append(QString().sprintf(" (%d)", ++i));
			}
			
			if(basePath.mkpath(newFolder))
			{
				QDir createdDir = basePath;
				if(createdDir.cd(newFolder))
				{
					QModelIndex newIndex = m_fileSystemModel->index(createdDir.canonicalPath());
					ui->outputFolderView->setCurrentIndex(newIndex);
					outputFolderViewClicked(newIndex);
					CENTER_CURRENT_OUTPUT_FOLDER_DELAYED();
				}
			}
			else
			{
				QMessageBox::warning(this, tr("Failed to create folder"), NOBREAK(QString("%1<br>%2<br><br>%3").arg(tr("The new folder could not be created:"), basePath.absoluteFilePath(newFolder), tr("Drive is read-only or insufficient access rights!"))));
			}
		}
		break;
	}
}

/*
 * Output to source dir changed
 */
void MainWindow::saveToSourceFolderChanged(void)
{
	m_settings->outputToSourceDir(ui->saveToSourceFolderCheckBox->isChecked());
}

/*
 * Prepend relative source file path to output file name changed
 */
void MainWindow::prependRelativePathChanged(void)
{
	m_settings->prependRelativeSourcePath(ui->prependRelativePathCheckBox->isChecked());
}

/*
 * Show context menu for output folder
 */
void MainWindow::outputFolderContextMenu(const QPoint &pos)
{
	QAbstractScrollArea *scrollArea = dynamic_cast<QAbstractScrollArea*>(QObject::sender());
	QWidget *sender = scrollArea ? scrollArea->viewport() : dynamic_cast<QWidget*>(QObject::sender());	

	if(pos.x() <= sender->width() && pos.y() <= sender->height() && pos.x() >= 0 && pos.y() >= 0)
	{
		m_outputFolderContextMenu->popup(sender->mapToGlobal(pos));
	}
}

/*
 * Show selected folder in explorer
 */
void MainWindow::showFolderContextActionTriggered(void)
{
	if(!m_fileSystemModel)
	{
		qWarning("File system model not initialized yet!");
		return;
	}

	QString path = QDir::toNativeSeparators(m_fileSystemModel->filePath(ui->outputFolderView->currentIndex()));
	if(!path.endsWith(QDir::separator())) path.append(QDir::separator());
	MUtils::OS::shell_open(this, path, true);
}

/*
 * Refresh the directory outline
 */
void MainWindow::refreshFolderContextActionTriggered(void)
{
	//force re-initialization
	QTimer::singleShot(0, this, SLOT(initOutputFolderModel()));
}

/*
 * Go one directory up
 */
void MainWindow::goUpFolderContextActionTriggered(void)
{
	QModelIndex current = ui->outputFolderView->currentIndex();
	if(current.isValid())
	{
		QModelIndex parent = current.parent();
		if(parent.isValid())
		{
			
			ui->outputFolderView->setCurrentIndex(parent);
			outputFolderViewClicked(parent);
		}
		else
		{
			MUtils::Sound::beep(MUtils::Sound::BEEP_WRN);
		}
		CENTER_CURRENT_OUTPUT_FOLDER_DELAYED();
	}
}

/*
 * Add current folder to favorites
 */
void MainWindow::addFavoriteFolderActionTriggered(void)
{
	QString path = m_fileSystemModel->filePath(ui->outputFolderView->currentIndex());
	QStringList favorites = m_settings->favoriteOutputFolders().split("|", QString::SkipEmptyParts);

	if(!favorites.contains(path, Qt::CaseInsensitive))
	{
		favorites.append(path);
		while(favorites.count() > 6) favorites.removeFirst();
	}
	else
	{
		MUtils::Sound::beep(MUtils::Sound::BEEP_WRN);
	}

	m_settings->favoriteOutputFolders(favorites.join("|"));
	refreshFavorites();
}

/*
 * Output folder edit finished
 */
void MainWindow::outputFolderEditFinished(void)
{
	if(ui->outputFolderEdit->isHidden())
	{
		return; //Not currently in edit mode!
	}
	
	bool ok = false;
	
	QString text = QDir::fromNativeSeparators(ui->outputFolderEdit->text().trimmed());
	while(text.startsWith('"') || text.startsWith('/')) text = text.right(text.length() - 1).trimmed();
	while(text.endsWith('"') || text.endsWith('/')) text = text.left(text.length() - 1).trimmed();

	static const char *str = "?*<>|\"";
	for(size_t i = 0; str[i]; i++) text.replace(str[i], "_");

	if(!((text.length() >= 2) && text.at(0).isLetter() && text.at(1) == QChar(':')))
	{
		text = QString("%1/%2").arg(QDir::fromNativeSeparators(ui->outputFolderLabel->text()), text);
	}

	if(text.length() == 2) text += "/"; /* "X:" => "X:/" */

	while(text.length() > 2)
	{
		QFileInfo info(text);
		if(info.exists() && info.isDir())
		{
			QModelIndex index = m_fileSystemModel->index(QFileInfo(info.canonicalFilePath()).absoluteFilePath());
			if(index.isValid())
			{
				ok = true;
				ui->outputFolderView->setCurrentIndex(index);
				outputFolderViewClicked(index);
				break;
			}
		}
		else if(info.exists() && info.isFile())
		{
			QModelIndex index = m_fileSystemModel->index(QFileInfo(info.canonicalPath()).absoluteFilePath());
			if(index.isValid())
			{
				ok = true;
				ui->outputFolderView->setCurrentIndex(index);
				outputFolderViewClicked(index);
				break;
			}
		}

		text = text.left(text.length() - 1).trimmed();
	}

	ui->outputFolderEdit->setVisible(false);
	ui->outputFolderLabel->setVisible(true);
	ui->outputFolderView->setEnabled(true);

	if(!ok) MUtils::Sound::beep(MUtils::Sound::BEEP_ERR);
	CENTER_CURRENT_OUTPUT_FOLDER_DELAYED();
}

/*
 * Initialize file system model
 */
void MainWindow::initOutputFolderModel(void)
{
	if(m_outputFolderNoteBox->isHidden())
	{
		m_outputFolderNoteBox->show();
		m_outputFolderNoteBox->repaint();
		m_outputFolderViewInitCounter = 4;

		if(m_fileSystemModel)
		{
			SET_MODEL(ui->outputFolderView, NULL);
			ui->outputFolderView->repaint();
		}

		m_fileSystemModel.reset(new QFileSystemModelEx());
		if(!m_fileSystemModel.isNull())
		{
			m_fileSystemModel->installEventFilter(this);
			connect(m_fileSystemModel.data(), SIGNAL(directoryLoaded(QString)),          this, SLOT(outputFolderDirectoryLoaded(QString)));
			connect(m_fileSystemModel.data(), SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(outputFolderRowsInserted(QModelIndex,int,int)));

			SET_MODEL(ui->outputFolderView, m_fileSystemModel.data());
			ui->outputFolderView->header()->setStretchLastSection(true);
			ui->outputFolderView->header()->hideSection(1);
			ui->outputFolderView->header()->hideSection(2);
			ui->outputFolderView->header()->hideSection(3);
		
			m_fileSystemModel->setRootPath("");
			QModelIndex index = m_fileSystemModel->index(m_settings->outputDir());
			if(index.isValid()) ui->outputFolderView->setCurrentIndex(index);
			outputFolderViewClicked(ui->outputFolderView->currentIndex());
		}

		CENTER_CURRENT_OUTPUT_FOLDER_DELAYED();
		QTimer::singleShot(125, this, SLOT(initOutputFolderModel_doAsync()));
	}
}

/*
 * Initialize file system model (do NOT call this one directly!)
 */
void MainWindow::initOutputFolderModel_doAsync(void)
{
	if(m_outputFolderViewInitCounter > 0)
	{
		m_outputFolderViewInitCounter--;
		QTimer::singleShot(125, this, SLOT(initOutputFolderModel_doAsync()));
	}
	else
	{
		QTimer::singleShot(125, m_outputFolderNoteBox.data(), SLOT(hide()));
		ui->outputFolderView->setFocus();
	}
}

/*
 * Center current folder in view
 */
void MainWindow::centerOutputFolderModel(void)
{
	if(ui->outputFolderView->isVisible())
	{
		centerOutputFolderModel_doAsync();
		QTimer::singleShot(125, this, SLOT(centerOutputFolderModel_doAsync()));
	}
}

/*
 * Center current folder in view (do NOT call this one directly!)
 */
void MainWindow::centerOutputFolderModel_doAsync(void)
{
	if(ui->outputFolderView->isVisible())
	{
		m_outputFolderViewCentering = true;
		const QModelIndex index = ui->outputFolderView->currentIndex();
		ui->outputFolderView->scrollTo(index, QAbstractItemView::PositionAtCenter);
		ui->outputFolderView->setFocus();
	}
}

/*
 * File system model asynchronously loaded a dir
 */
void MainWindow::outputFolderDirectoryLoaded(const QString& /*path*/)
{
	if(m_outputFolderViewCentering)
	{
		CENTER_CURRENT_OUTPUT_FOLDER_DELAYED();
	}
}

/*
 * File system model inserted new items
 */
void MainWindow::outputFolderRowsInserted(const QModelIndex& /*parent*/, int /*start*/, int /*end*/)
{
	if(m_outputFolderViewCentering)
	{
		CENTER_CURRENT_OUTPUT_FOLDER_DELAYED();
	}
}

/*
 * Directory view item was expanded by user
 */
void MainWindow::outputFolderItemExpanded(const QModelIndex& /*item*/)
{
	//We need to stop centering as soon as the user has expanded an item manually!
	m_outputFolderViewCentering = false;
}

/*
 * View event for output folder control occurred
 */
void MainWindow::outputFolderViewEventOccurred(QWidget* /*sender*/, QEvent *event)
{
	switch(event->type())
	{
	case QEvent::Enter:
	case QEvent::Leave:
	case QEvent::KeyPress:
	case QEvent::KeyRelease:
	case QEvent::FocusIn:
	case QEvent::FocusOut:
	case QEvent::TouchEnd:
		outputFolderViewClicked(ui->outputFolderView->currentIndex());
		break;
	}
}

/*
 * Mouse event for output folder control occurred
 */
void MainWindow::outputFolderMouseEventOccurred(QWidget *sender, QEvent *event)
{
	QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent*>(event);
	QPoint pos = (mouseEvent) ? mouseEvent->pos() : QPoint();

	if(sender == ui->outputFolderLabel)
	{
		switch(event->type())
		{
		case QEvent::MouseButtonPress:
			if(mouseEvent && (mouseEvent->button() == Qt::LeftButton))
			{
				QString path = ui->outputFolderLabel->text();
				if(!path.endsWith(QDir::separator())) path.append(QDir::separator());
				MUtils::OS::shell_open(this, path, true);
			}
			break;
		case QEvent::Enter:
			ui->outputFolderLabel->setForegroundRole(QPalette::Link);
			break;
		case QEvent::Leave:
			ui->outputFolderLabel->setForegroundRole(QPalette::WindowText);
			break;
		}
	}

	if((sender == ui->outputFoldersFovoritesLabel) || (sender == ui->outputFoldersEditorLabel) || (sender == ui->outputFoldersGoUpLabel))
	{
		const type_info &styleType = typeid(*qApp->style());
		if((typeid(QPlastiqueStyle) == styleType) || (typeid(QWindowsStyle) == styleType))
		{
			switch(event->type())
			{
			case QEvent::Enter:
				dynamic_cast<QLabel*>(sender)->setFrameShadow(ui->outputFolderView->isEnabled() ? QFrame::Raised : QFrame::Plain);
				break;
			case QEvent::MouseButtonPress:
				dynamic_cast<QLabel*>(sender)->setFrameShadow(ui->outputFolderView->isEnabled() ? QFrame::Sunken : QFrame::Plain);
				break;
			case QEvent::MouseButtonRelease:
				dynamic_cast<QLabel*>(sender)->setFrameShadow(ui->outputFolderView->isEnabled() ? QFrame::Raised : QFrame::Plain);
				break;
			case QEvent::Leave:
				dynamic_cast<QLabel*>(sender)->setFrameShadow(ui->outputFolderView->isEnabled() ? QFrame::Plain : QFrame::Plain);
				break;
			}
		}
		else
		{
			dynamic_cast<QLabel*>(sender)->setFrameShadow(QFrame::Plain);
		}

		if((event->type() == QEvent::MouseButtonRelease) && ui->outputFolderView->isEnabled() && (mouseEvent))
		{
			if(pos.x() <= sender->width() && pos.y() <= sender->height() && pos.x() >= 0 && pos.y() >= 0 && mouseEvent->button() != Qt::MidButton)
			{
				if(sender == ui->outputFoldersFovoritesLabel)
				{
					m_outputFolderFavoritesMenu->popup(sender->mapToGlobal(pos));
				}
				else if(sender == ui->outputFoldersEditorLabel)
				{
					ui->outputFolderView->setEnabled(false);
					ui->outputFolderLabel->setVisible(false);
					ui->outputFolderEdit->setVisible(true);
					ui->outputFolderEdit->setText(ui->outputFolderLabel->text());
					ui->outputFolderEdit->selectAll();
					ui->outputFolderEdit->setFocus();
				}
				else if(sender == ui->outputFoldersGoUpLabel)
				{
					QTimer::singleShot(0, this, SLOT(goUpFolderContextActionTriggered()));
				}
				else
				{
					MUTILS_THROW("Oups, this is not supposed to happen!");
				}
			}
		}
	}
}

// =========================================================
// Metadata tab slots
// =========================================================

/*
 * Edit meta button clicked
 */
void MainWindow::editMetaButtonClicked(void)
{
	ABORT_IF_BUSY;

	const QModelIndex index = ui->metaDataView->currentIndex();

	if(index.isValid())
	{
		m_metaInfoModel->editItem(index, this);
	
		if(index.row() == 4)
		{
			m_settings->metaInfoPosition(m_metaData->position());
		}
	}
}

/*
 * Reset meta button clicked
 */
void MainWindow::clearMetaButtonClicked(void)
{
	ABORT_IF_BUSY;
	m_metaInfoModel->clearData();
}

/*
 * Meta tags enabled changed
 */
void MainWindow::metaTagsEnabledChanged(void)
{
	m_settings->writeMetaTags(ui->writeMetaDataCheckBox->isChecked());
}

/*
 * Playlist enabled changed
 */
void MainWindow::playlistEnabledChanged(void)
{
	m_settings->createPlaylist(ui->generatePlaylistCheckBox->isChecked());
}

// =========================================================
// Compression tab slots
// =========================================================

/*
 * Update encoder
 */
void MainWindow::updateEncoder(int id)
{
	/*qWarning("\nupdateEncoder(%d)", id);*/

	m_settings->compressionEncoder(id);
	const AbstractEncoderInfo *info = EncoderRegistry::getEncoderInfo(id);

	//Update UI controls
	ui->radioButtonModeQuality       ->setEnabled(info->isModeSupported(SettingsModel::VBRMode));
	ui->radioButtonModeAverageBitrate->setEnabled(info->isModeSupported(SettingsModel::ABRMode));
	ui->radioButtonConstBitrate      ->setEnabled(info->isModeSupported(SettingsModel::CBRMode));
	
	//Initialize checkbox state
	if(ui->radioButtonModeQuality->isEnabled())             ui->radioButtonModeQuality->setChecked(true);
	else if(ui->radioButtonModeAverageBitrate->isEnabled()) ui->radioButtonModeAverageBitrate->setChecked(true);
	else if(ui->radioButtonConstBitrate->isEnabled())       ui->radioButtonConstBitrate->setChecked(true);
	else MUTILS_THROW("It appears that the encoder does not support *any* RC mode!");

	//Apply current RC mode
	const int currentRCMode = EncoderRegistry::loadEncoderMode(m_settings, id);
	switch(currentRCMode)
	{
		case SettingsModel::VBRMode: if(ui->radioButtonModeQuality->isEnabled())        ui->radioButtonModeQuality->setChecked(true);        break;
		case SettingsModel::ABRMode: if(ui->radioButtonModeAverageBitrate->isEnabled()) ui->radioButtonModeAverageBitrate->setChecked(true); break;
		case SettingsModel::CBRMode: if(ui->radioButtonConstBitrate->isEnabled())       ui->radioButtonConstBitrate->setChecked(true);       break;
		default: MUTILS_THROW("updateEncoder(): Unknown rc-mode encountered!");
	}

	//Display encoder description
	if(const char* description = info->description())
	{
		ui->labelEncoderInfo->setVisible(true);
		ui->labelEncoderInfo->setText(tr("Current Encoder: %1").arg(QString::fromUtf8(description)));
	}
	else
	{
		ui->labelEncoderInfo->setVisible(false);
	}

	//Update RC mode!
	updateRCMode(m_modeButtonGroup->checkedId());
}

/*
 * Update rate-control mode
 */
void MainWindow::updateRCMode(int id)
{
	/*qWarning("updateRCMode(%d)", id);*/

	//Store new RC mode
	const int currentEncoder = m_encoderButtonGroup->checkedId();
	EncoderRegistry::saveEncoderMode(m_settings, currentEncoder, id);

	//Fetch encoder info
	const AbstractEncoderInfo *info = EncoderRegistry::getEncoderInfo(currentEncoder);
	const int valueCount = info->valueCount(id);

	//Sanity check
	if(!info->isModeSupported(id))
	{
		qWarning("Attempting to use an unsupported RC mode (%d) with current encoder (%d)!", id, currentEncoder);
		ui->labelBitrate->setText("(ERROR)");
		return;
	}

	//Update slider min/max values
	if(valueCount > 0)
	{
		SignalBlockHelper signalBlockHelper(ui->sliderBitrate);
		ui->sliderBitrate->setEnabled(true);
		ui->sliderBitrate->setMinimum(0);
		ui->sliderBitrate->setMaximum(valueCount-1);
	}
	else
	{
		SignalBlockHelper signalBlockHelper(ui->sliderBitrate);
		ui->sliderBitrate->setEnabled(false);
		ui->sliderBitrate->setMinimum(0);
		ui->sliderBitrate->setMaximum(2);
	}

	//Now update bitrate/quality value!
	if(valueCount > 0)
	{
		const int currentValue = EncoderRegistry::loadEncoderValue(m_settings, currentEncoder, id);
		ui->sliderBitrate->setValue(qBound(0, currentValue, valueCount-1));
		updateBitrate(qBound(0, currentValue, valueCount-1));
	}
	else
	{
		ui->sliderBitrate->setValue(1);
		updateBitrate(0);
	}
}

/*
 * Update bitrate
 */
void MainWindow::updateBitrate(int value)
{
	/*qWarning("updateBitrate(%d)", value);*/

	//Load current encoder and RC mode
	const int currentEncoder = m_encoderButtonGroup->checkedId();
	const int currentRCMode = m_modeButtonGroup->checkedId();

	//Fetch encoder info
	const AbstractEncoderInfo *info = EncoderRegistry::getEncoderInfo(currentEncoder);
	const int valueCount = info->valueCount(currentRCMode);

	//Sanity check
	if(!info->isModeSupported(currentRCMode))
	{
		qWarning("Attempting to use an unsupported RC mode (%d) with current encoder (%d)!", currentRCMode, currentEncoder);
		ui->labelBitrate->setText("(ERROR)");
		return;
	}

	//Store new bitrate value
	if(valueCount > 0)
	{
		EncoderRegistry::saveEncoderValue(m_settings, currentEncoder, currentRCMode, qBound(0, value, valueCount-1));
	}

	//Update bitrate value
	const int displayValue = (valueCount > 0) ? info->valueAt(currentRCMode, qBound(0, value, valueCount-1)) : INT_MAX;
	switch(info->valueType(currentRCMode))
	{
	case AbstractEncoderInfo::TYPE_BITRATE:
		ui->labelBitrate->setText(QString("%1 kbps").arg(QString::number(displayValue)));
		break;
	case AbstractEncoderInfo::TYPE_APPROX_BITRATE:
		ui->labelBitrate->setText(QString("&asymp; %1 kbps").arg(QString::number(displayValue)));
		break;
	case AbstractEncoderInfo::TYPE_QUALITY_LEVEL_INT:
		ui->labelBitrate->setText(tr("Quality Level %1").arg(QString::number(displayValue)));
		break;
	case AbstractEncoderInfo::TYPE_QUALITY_LEVEL_FLT:
		ui->labelBitrate->setText(tr("Quality Level %1").arg(QString().sprintf("%.2f", double(displayValue)/100.0)));
		break;
	case AbstractEncoderInfo::TYPE_COMPRESSION_LEVEL:
		ui->labelBitrate->setText(tr("Compression %1").arg(QString::number(displayValue)));
		break;
	case AbstractEncoderInfo::TYPE_UNCOMPRESSED:
		ui->labelBitrate->setText(tr("Uncompressed"));
		break;
	default:
		MUTILS_THROW("Unknown display value type encountered!");
		break;
	}
}

/*
 * Event for compression tab occurred
 */
void MainWindow::compressionTabEventOccurred(QWidget *sender, QEvent *event)
{
	static const QUrl helpUrl("http://lamexp.sourceforge.net/doc/FAQ.html#054010d9");
	
	if((sender == ui->labelCompressionHelp) && (event->type() == QEvent::MouseButtonPress))
	{
		QDesktopServices::openUrl(helpUrl);
	}
	else if((sender == ui->labelResetEncoders) && (event->type() == QEvent::MouseButtonPress))
	{
		PLAY_SOUND_OPTIONAL("blast", true);
		EncoderRegistry::resetAllEncoders(m_settings);
		m_settings->compressionEncoder(SettingsModel::MP3Encoder);
		ui->radioButtonEncoderMP3->setChecked(true);
		QTimer::singleShot(0, this, SLOT(updateEncoder()));
	}
}

// =========================================================
// Advanced option slots
// =========================================================

/*
 * Lame algorithm quality changed
 */
void MainWindow::updateLameAlgoQuality(int value)
{
	QString text;

	switch(value)
	{
	case 3:
		text = tr("Best Quality (Slow)");
		break;
	case 2:
		text = tr("High Quality (Recommended)");
		break;
	case 1:
		text = tr("Acceptable Quality (Fast)");
		break;
	case 0:
		text = tr("Poor Quality (Very Fast)");
		break;
	}

	if(!text.isEmpty())
	{
		m_settings->lameAlgoQuality(value);
		ui->labelLameAlgoQuality->setText(text);
	}

	bool warning = (value == 0), notice = (value == 3);
	ui->labelLameAlgoQualityWarning->setVisible(warning);
	ui->labelLameAlgoQualityWarningIcon->setVisible(warning);
	ui->labelLameAlgoQualityNotice->setVisible(notice);
	ui->labelLameAlgoQualityNoticeIcon->setVisible(notice);
	ui->labelLameAlgoQualitySpacer->setVisible(warning || notice);
}

/*
 * Bitrate management endabled/disabled
 */
void MainWindow::bitrateManagementEnabledChanged(bool checked)
{
	m_settings->bitrateManagementEnabled(checked);
}

/*
 * Minimum bitrate has changed
 */
void MainWindow::bitrateManagementMinChanged(int value)
{
	if(value > ui->spinBoxBitrateManagementMax->value())
	{
		ui->spinBoxBitrateManagementMin->setValue(ui->spinBoxBitrateManagementMax->value());
		m_settings->bitrateManagementMinRate(ui->spinBoxBitrateManagementMax->value());
	}
	else
	{
		m_settings->bitrateManagementMinRate(value);
	}
}

/*
 * Maximum bitrate has changed
 */
void MainWindow::bitrateManagementMaxChanged(int value)
{
	if(value < ui->spinBoxBitrateManagementMin->value())
	{
		ui->spinBoxBitrateManagementMax->setValue(ui->spinBoxBitrateManagementMin->value());
		m_settings->bitrateManagementMaxRate(ui->spinBoxBitrateManagementMin->value());
	}
	else
	{
		m_settings->bitrateManagementMaxRate(value);
	}
}

/*
 * Channel mode has changed
 */
void MainWindow::channelModeChanged(int value)
{
	if(value >= 0) m_settings->lameChannelMode(value);
}

/*
 * Sampling rate has changed
 */
void MainWindow::samplingRateChanged(int value)
{
	if(value >= 0) m_settings->samplingRate(value);
}

/*
 * Nero AAC 2-Pass mode changed
 */
void MainWindow::neroAAC2PassChanged(bool checked)
{
	m_settings->neroAACEnable2Pass(checked);
}

/*
 * Nero AAC profile mode changed
 */
void MainWindow::neroAACProfileChanged(int value)
{
	if(value >= 0) m_settings->aacEncProfile(value);
}

/*
 * Aften audio coding mode changed
 */
void MainWindow::aftenCodingModeChanged(int value)
{
	if(value >= 0) m_settings->aftenAudioCodingMode(value);
}

/*
 * Aften DRC mode changed
 */
void MainWindow::aftenDRCModeChanged(int value)
{
	if(value >= 0) m_settings->aftenDynamicRangeCompression(value);
}

/*
 * Aften exponent search size changed
 */
void MainWindow::aftenSearchSizeChanged(int value)
{
	if(value >= 0) m_settings->aftenExponentSearchSize(value);
}

/*
 * Aften fast bit allocation changed
 */
void MainWindow::aftenFastAllocationChanged(bool checked)
{
	m_settings->aftenFastBitAllocation(checked);
}


/*
 * Opus encoder settings changed
 */
void MainWindow::opusSettingsChanged(void)
{
	m_settings->opusFramesize(ui->comboBoxOpusFramesize->currentIndex());
	m_settings->opusComplexity(ui->spinBoxOpusComplexity->value());
	m_settings->opusDisableResample(ui->checkBoxOpusDisableResample->isChecked());
}

/*
 * Normalization filter enabled changed
 */
void MainWindow::normalizationEnabledChanged(bool checked)
{
	m_settings->normalizationFilterEnabled(checked);
	normalizationDynamicChanged(ui->checkBoxNormalizationFilterDynamic->isChecked());
}

/*
 * Dynamic normalization enabled changed
 */
void MainWindow::normalizationDynamicChanged(bool checked)
{
	ui->spinBoxNormalizationFilterSize->setEnabled(ui->checkBoxNormalizationFilterEnabled->isChecked() && checked);
	m_settings->normalizationFilterDynamic(checked);
}

/*
 * Normalization max. volume changed
 */
void MainWindow::normalizationMaxVolumeChanged(double value)
{
	m_settings->normalizationFilterMaxVolume(static_cast<int>(value * 100.0));
}

/*
 * Normalization equalization mode changed
 */
void MainWindow::normalizationCoupledChanged(bool checked)
{
	m_settings->normalizationFilterCoupled(checked);
}

/*
 * Normalization filter size changed
 */
void MainWindow::normalizationFilterSizeChanged(int value)
{
	m_settings->normalizationFilterSize(value);
}

/*
 * Normalization filter size editing finished
 */
void MainWindow::normalizationFilterSizeFinished(void)
{
	const int value = ui->spinBoxNormalizationFilterSize->value();
	if((value % 2) != 1)
	{
		bool rnd = MUtils::parity(MUtils::next_rand_u32());
		ui->spinBoxNormalizationFilterSize->setValue(rnd ? value+1 : value-1);
	}
}

/*
 * Tone adjustment has changed (Bass)
 */
void MainWindow::toneAdjustBassChanged(double value)
{
	m_settings->toneAdjustBass(static_cast<int>(value * 100.0));
	ui->spinBoxToneAdjustBass->setPrefix((value > 0) ? "+" : QString());
}

/*
 * Tone adjustment has changed (Treble)
 */
void MainWindow::toneAdjustTrebleChanged(double value)
{
	m_settings->toneAdjustTreble(static_cast<int>(value * 100.0));
	ui->spinBoxToneAdjustTreble->setPrefix((value > 0) ? "+" : QString());
}

/*
 * Tone adjustment has been reset
 */
void MainWindow::toneAdjustTrebleReset(void)
{
	ui->spinBoxToneAdjustBass->setValue(m_settings->toneAdjustBassDefault());
	ui->spinBoxToneAdjustTreble->setValue(m_settings->toneAdjustTrebleDefault());
	toneAdjustBassChanged(ui->spinBoxToneAdjustBass->value());
	toneAdjustTrebleChanged(ui->spinBoxToneAdjustTreble->value());
}

/*
 * Custom encoder parameters changed
 */
void MainWindow::customParamsChanged(void)
{
	ui->lineEditCustomParamLAME->setText(ui->lineEditCustomParamLAME->text().simplified());
	ui->lineEditCustomParamOggEnc->setText(ui->lineEditCustomParamOggEnc->text().simplified());
	ui->lineEditCustomParamNeroAAC->setText(ui->lineEditCustomParamNeroAAC->text().simplified());
	ui->lineEditCustomParamFLAC->setText(ui->lineEditCustomParamFLAC->text().simplified());
	ui->lineEditCustomParamAften->setText(ui->lineEditCustomParamAften->text().simplified());
	ui->lineEditCustomParamOpus->setText(ui->lineEditCustomParamOpus->text().simplified());

	bool customParamsUsed = false;
	if(!ui->lineEditCustomParamLAME->text().isEmpty()) customParamsUsed = true;
	if(!ui->lineEditCustomParamOggEnc->text().isEmpty()) customParamsUsed = true;
	if(!ui->lineEditCustomParamNeroAAC->text().isEmpty()) customParamsUsed = true;
	if(!ui->lineEditCustomParamFLAC->text().isEmpty()) customParamsUsed = true;
	if(!ui->lineEditCustomParamAften->text().isEmpty()) customParamsUsed = true;
	if(!ui->lineEditCustomParamOpus->text().isEmpty()) customParamsUsed = true;

	ui->labelCustomParamsIcon->setVisible(customParamsUsed);
	ui->labelCustomParamsText->setVisible(customParamsUsed);
	ui->labelCustomParamsSpacer->setVisible(customParamsUsed);

	EncoderRegistry::saveEncoderCustomParams(m_settings, SettingsModel::MP3Encoder,    ui->lineEditCustomParamLAME->text());
	EncoderRegistry::saveEncoderCustomParams(m_settings, SettingsModel::VorbisEncoder, ui->lineEditCustomParamOggEnc->text());
	EncoderRegistry::saveEncoderCustomParams(m_settings, SettingsModel::AACEncoder,    ui->lineEditCustomParamNeroAAC->text());
	EncoderRegistry::saveEncoderCustomParams(m_settings, SettingsModel::FLACEncoder,   ui->lineEditCustomParamFLAC->text());
	EncoderRegistry::saveEncoderCustomParams(m_settings, SettingsModel::AC3Encoder,    ui->lineEditCustomParamAften->text());
	EncoderRegistry::saveEncoderCustomParams(m_settings, SettingsModel::OpusEncoder,   ui->lineEditCustomParamOpus->text());
}

/*
 * One of the rename buttons has been clicked
 */
void MainWindow::renameButtonClicked(bool checked)
{
	if(QPushButton *const button  = dynamic_cast<QPushButton*>(QObject::sender()))
	{
		QWidget *pages[]       = { ui->pageRename_Rename,   ui->pageRename_RegExp,   ui->pageRename_FileEx   };
		QPushButton *buttons[] = { ui->buttonRename_Rename, ui->buttonRename_RegExp, ui->buttonRename_FileEx };
		for(int i = 0; i < 3; i++)
		{
			const bool match = (button == buttons[i]);
			buttons[i]->setChecked(match);
			if(match && checked) ui->stackedWidget->setCurrentWidget(pages[i]);
		}
	}
}

/*
 * Rename output files enabled changed
 */
void MainWindow::renameOutputEnabledChanged(const bool &checked)
{
	m_settings->renameFiles_renameEnabled(checked);
}

/*
 * Rename output files patterm changed
 */
void MainWindow::renameOutputPatternChanged(void)
{
	QString temp = ui->lineEditRenamePattern->text().simplified();
	ui->lineEditRenamePattern->setText(temp.isEmpty() ? m_settings->renameFiles_renamePatternDefault() : temp);
	m_settings->renameFiles_renamePattern(ui->lineEditRenamePattern->text());
}

/*
 * Rename output files patterm changed
 */
void MainWindow::renameOutputPatternChanged(const QString &text, const bool &silent)
{
	QString pattern(text.simplified());
	
	pattern.replace("<BaseName>", "The_White_Stripes_-_Fell_In_Love_With_A_Girl", Qt::CaseInsensitive);
	pattern.replace("<TrackNo>", "04", Qt::CaseInsensitive);
	pattern.replace("<Title>", "Fell In Love With A Girl", Qt::CaseInsensitive);
	pattern.replace("<Artist>", "The White Stripes", Qt::CaseInsensitive);
	pattern.replace("<Album>", "White Blood Cells", Qt::CaseInsensitive);
	pattern.replace("<Year>", "2001", Qt::CaseInsensitive);
	pattern.replace("<Comment>", "Encoded by LameXP", Qt::CaseInsensitive);

	const QString patternClean = MUtils::clean_file_name(pattern, false);

	if(pattern.compare(patternClean))
	{
		if(ui->lineEditRenamePattern->palette().color(QPalette::Text) != Qt::red)
		{
			if(!silent) MUtils::Sound::beep(MUtils::Sound::BEEP_ERR);
			SET_TEXT_COLOR(ui->lineEditRenamePattern, Qt::red);
		}
	}
	else
	{
		if(ui->lineEditRenamePattern->palette() != QPalette())
		{
			if(!silent) MUtils::Sound::beep(MUtils::Sound::BEEP_NFO);
			ui->lineEditRenamePattern->setPalette(QPalette());
		}
	}

	ui->labelRanameExample->setText(patternClean);
}

/*
 * Regular expression enabled changed
 */
void MainWindow::renameRegExpEnabledChanged(const bool &checked)
{
	m_settings->renameFiles_regExpEnabled(checked);
}

/*
 * Regular expression value has changed
 */
void  MainWindow::renameRegExpValueChanged(void)
{
	const QString search  = ui->lineEditRenameRegExp_Search->text() .trimmed();
	const QString replace = ui->lineEditRenameRegExp_Replace->text().simplified();
	ui->lineEditRenameRegExp_Search ->setText(search.isEmpty()  ? m_settings->renameFiles_regExpSearchDefault()  : search);
	ui->lineEditRenameRegExp_Replace->setText(replace.isEmpty() ? m_settings->renameFiles_regExpReplaceDefault() : replace);
	m_settings->renameFiles_regExpSearch (ui->lineEditRenameRegExp_Search ->text());
	m_settings->renameFiles_regExpReplace(ui->lineEditRenameRegExp_Replace->text());
}

/*
 * Regular expression search pattern has changed
 */
void  MainWindow::renameRegExpSearchChanged(const QString &text, const bool &silent)
{
	const QString pattern(text.trimmed());

	if((!pattern.isEmpty()) && (!QRegExp(pattern.trimmed()).isValid()))
	{
		if(ui->lineEditRenameRegExp_Search->palette().color(QPalette::Text) != Qt::red)
		{
			if(!silent) MUtils::Sound::beep(MUtils::Sound::BEEP_ERR);
			SET_TEXT_COLOR(ui->lineEditRenameRegExp_Search, Qt::red);
		}
	}
	else
	{
		if(ui->lineEditRenameRegExp_Search->palette() != QPalette())
		{
			if(!silent) MUtils::Sound::beep(MUtils::Sound::BEEP_NFO);
			ui->lineEditRenameRegExp_Search->setPalette(QPalette());
		}
	}

	renameRegExpReplaceChanged(ui->lineEditRenameRegExp_Replace->text(), silent);
}

/*
 * Regular expression replacement string changed
 */
void  MainWindow::renameRegExpReplaceChanged(const QString &text, const bool &silent)
{
	QString replacement(text.simplified());
	const QString search(ui->lineEditRenameRegExp_Search->text().trimmed());

	if(!search.isEmpty())
	{
		const QRegExp regexp(search);
		if(regexp.isValid())
		{
			const int count = regexp.captureCount();
			const QString blank; 
			for(int i = 0; i < count; i++)
			{
				replacement.replace(QString("\\%0").arg(QString::number(i+1)), blank);
			}
		}
	}

	if(replacement.compare(MUtils::clean_file_name(replacement, false)))
	{
		if(ui->lineEditRenameRegExp_Replace->palette().color(QPalette::Text) != Qt::red)
		{
			if(!silent) MUtils::Sound::beep(MUtils::Sound::BEEP_ERR);
			SET_TEXT_COLOR(ui->lineEditRenameRegExp_Replace, Qt::red);
		}
	}
	else
	{
		if(ui->lineEditRenameRegExp_Replace->palette() != QPalette())
		{
			if(!silent) MUtils::Sound::beep(MUtils::Sound::BEEP_NFO);
			ui->lineEditRenameRegExp_Replace->setPalette(QPalette());
		}
	}
}

/*
 * Show list of rename macros
 */
void MainWindow::showRenameMacros(const QString &text)
{
	if(text.compare("reset", Qt::CaseInsensitive) == 0)
	{
		ui->lineEditRenamePattern->setText(m_settings->renameFiles_renamePatternDefault());
		return;
	}

	if(text.compare("regexp", Qt::CaseInsensitive) == 0)
	{
		MUtils::OS::shell_open(this, "http://www.regular-expressions.info/quickstart.html");
		return;
	}

	const QString format = QString("<tr><td><tt>&lt;%1&gt;</tt></td><td>&nbsp;&nbsp;</td><td>%2</td></tr>");

	QString message = QString("<table>");
	message += QString(format).arg("BaseName", tr("File name without extension"));
	message += QString(format).arg("TrackNo", tr("Track number with leading zero"));
	message += QString(format).arg("Title", tr("Track title"));
	message += QString(format).arg("Artist", tr("Artist name"));
	message += QString(format).arg("Album", tr("Album name"));
	message += QString(format).arg("Year", tr("Year with (at least) four digits"));
	message += QString(format).arg("Comment", tr("Comment"));
	message += "</table><br><br>";
	message += QString("%1<br>").arg(tr("Characters forbidden in file names:"));
	message += "<b><tt>\\ / : * ? &lt; &gt; |<br>";
	
	QMessageBox::information(this, tr("Rename Macros"), message, tr("Discard"));
}

void MainWindow::fileExtAddButtonClicked(void)
{
	if(FileExtsModel *const model = dynamic_cast<FileExtsModel*>(ui->tableViewFileExts->model()))
	{
		model->addOverwrite(this);
	}
}

void MainWindow::fileExtRemoveButtonClicked(void)
{
	if(FileExtsModel *const model = dynamic_cast<FileExtsModel*>(ui->tableViewFileExts->model()))
	{
		const QModelIndex selected = ui->tableViewFileExts->currentIndex();
		if(selected.isValid())
		{
			model->removeOverwrite(selected);
		}
		else
		{
			MUtils::Sound::beep(MUtils::Sound::BEEP_ERR);
		}
	}
}

void MainWindow::fileExtModelChanged(void)
{
	if(FileExtsModel *const model = dynamic_cast<FileExtsModel*>(ui->tableViewFileExts->model()))
	{
		m_settings->renameFiles_fileExtension(model->exportItems());
	}
}

void MainWindow::forceStereoDownmixEnabledChanged(bool checked)
{
	m_settings->forceStereoDownmix(checked);
}

/*
 * Maximum number of instances changed
 */
void MainWindow::updateMaximumInstances(const int value)
{
	const quint32 instances = decodeInstances(qBound(1U, static_cast<quint32>(value), 32U));
	m_settings->maximumInstances(ui->checkBoxAutoDetectInstances->isChecked() ? 0U : instances);
	ui->labelMaxInstances->setText(tr("%n Instance(s)", "", static_cast<int>(instances)));
}

/*
 * Auto-detect number of instances
 */
void MainWindow::autoDetectInstancesChanged(const bool checked)
{
	m_settings->maximumInstances(checked ? 0U : decodeInstances(qBound(1U, static_cast<quint32>(ui->sliderMaxInstances->value()), 32U)));
}

/*
 * Browse for custom TEMP folder button clicked
 */
void MainWindow::browseCustomTempFolderButtonClicked(void)
{
	QString newTempFolder;

	if(MUtils::GUI::themes_enabled())
	{
		newTempFolder = QFileDialog::getExistingDirectory(this, QString(), m_settings->customTempPath());
	}
	else
	{
		QFileDialog dialog(this);
		dialog.setFileMode(QFileDialog::DirectoryOnly);
		dialog.setDirectory(m_settings->customTempPath());
		if(dialog.exec())
		{
			newTempFolder = dialog.selectedFiles().first();
		}
	}

	if(!newTempFolder.isEmpty())
	{
		QFile writeTest(QString("%1/~%2.tmp").arg(newTempFolder, MUtils::next_rand_str()));
		if(writeTest.open(QIODevice::ReadWrite))
		{
			writeTest.remove();
			ui->lineEditCustomTempFolder->setText(QDir::toNativeSeparators(newTempFolder));
		}
		else
		{
			QMessageBox::warning(this, tr("Access Denied"), tr("Cannot write to the selected directory. Please choose another directory!"));
		}
	}
}

/*
 * Custom TEMP folder changed
 */
void MainWindow::customTempFolderChanged(const QString &text)
{
	m_settings->customTempPath(QDir::fromNativeSeparators(text));
}

/*
 * Use custom TEMP folder option changed
 */
void MainWindow::useCustomTempFolderChanged(bool checked)
{
	m_settings->customTempPathEnabled(!checked);
}

/*
 * Help for custom parameters was requested
 */
void MainWindow::customParamsHelpRequested(QWidget *obj, QEvent *event)
{
	if(event->type() != QEvent::MouseButtonRelease)
	{
		return;
	}

	if(QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent*>(event))
	{
		QPoint pos = mouseEvent->pos();
		if(!(pos.x() <= obj->width() && pos.y() <= obj->height() && pos.x() >= 0 && pos.y() >= 0 && mouseEvent->button() != Qt::MidButton))
		{
			return;
		}
	}

	if(obj == ui->helpCustomParamLAME)         showCustomParamsHelpScreen("lame.exe",    "--longhelp");
	else if(obj == ui->helpCustomParamOggEnc)  showCustomParamsHelpScreen("oggenc2.exe", "--help");
	else if(obj == ui->helpCustomParamNeroAAC)
	{
		switch(EncoderRegistry::getAacEncoder())
		{
			case SettingsModel::AAC_ENCODER_QAAC: showCustomParamsHelpScreen("qaac64.exe|qaac.exe", "--help"); break;
			case SettingsModel::AAC_ENCODER_FHG : showCustomParamsHelpScreen("fhgaacenc.exe",       ""      ); break;
			case SettingsModel::AAC_ENCODER_FDK : showCustomParamsHelpScreen("fdkaac.exe",          "--help"); break;
			case SettingsModel::AAC_ENCODER_NERO: showCustomParamsHelpScreen("neroAacEnc.exe",      "-help" ); break;
			default: MUtils::Sound::beep(MUtils::Sound::BEEP_ERR); break;
		}
	}
	else if(obj == ui->helpCustomParamFLAC)    showCustomParamsHelpScreen("flac.exe",    "--help");
	else if(obj == ui->helpCustomParamAften)   showCustomParamsHelpScreen("aften.exe",   "-h"    );
	else if(obj == ui->helpCustomParamOpus)    showCustomParamsHelpScreen("opusenc.exe", "--help");
	else MUtils::Sound::beep(MUtils::Sound::BEEP_ERR);
}

/*
 * Show help for custom parameters
 */
void MainWindow::showCustomParamsHelpScreen(const QString &toolName, const QString &command)
{
	const QStringList toolNames = toolName.split('|', QString::SkipEmptyParts);
	QString binary;
	for(QStringList::ConstIterator iter = toolNames.constBegin(); iter != toolNames.constEnd(); iter++)
	{
		if(lamexp_tools_check(*iter))
		{
			binary = lamexp_tools_lookup(*iter);
			break;
		}
	}

	if(binary.isEmpty())
	{
		MUtils::Sound::beep(MUtils::Sound::BEEP_ERR);
		qWarning("customParamsHelpRequested: Binary could not be found!");
		return;
	}

	QProcess process;
	MUtils::init_process(process, QFileInfo(binary).absolutePath());

	process.start(binary, command.isEmpty() ? QStringList() : QStringList() << command);

	qApp->setOverrideCursor(QCursor(Qt::WaitCursor));

	if(process.waitForStarted(15000))
	{
		qApp->processEvents();
		process.waitForFinished(15000);
	}
	
	if(process.state() != QProcess::NotRunning)
	{
		process.kill();
		process.waitForFinished(-1);
	}

	qApp->restoreOverrideCursor();
	QStringList output; bool spaceFlag = true;

	while(process.canReadLine())
	{
		QString temp = QString::fromUtf8(process.readLine());
		TRIM_STRING_RIGHT(temp);
		if(temp.isEmpty())
		{
			if(!spaceFlag) { output << temp; spaceFlag = true; }
		}
		else
		{
			output << temp; spaceFlag = false;
		}
	}

	if(output.count() < 1)
	{
		qWarning("Empty output, cannot show help screen!");
		MUtils::Sound::beep(MUtils::Sound::BEEP_ERR);
	}

	WidgetHideHelper hiderHelper(m_dropBox.data());
	QScopedPointer<LogViewDialog> dialog(new LogViewDialog(this));
	dialog->exec(output);
}

/*
* File overwrite mode has changed
*/

void MainWindow::overwriteModeChanged(int id)
{
	if((id == SettingsModel::Overwrite_Replaces) && (m_settings->overwriteMode() != SettingsModel::Overwrite_Replaces))
	{
		if(QMessageBox::warning(this, tr("Overwrite Mode"), tr("Warning: This mode may overwrite existing files with no way to revert!"), tr("Continue"), tr("Revert"), QString(), 1) != 0)
		{
			ui->radioButtonOverwriteModeKeepBoth->setChecked(m_settings->overwriteMode() == SettingsModel::Overwrite_KeepBoth);
			ui->radioButtonOverwriteModeSkipFile->setChecked(m_settings->overwriteMode() == SettingsModel::Overwrite_SkipFile);
			return;
		}
	}

	m_settings->overwriteMode(id);
}

/*
* Keep original date/time opertion changed
*/
void MainWindow::keepOriginalDateTimeChanged(bool checked)
{
	m_settings->keepOriginalDataTime(checked);
}

/*
 * Reset all advanced options to their defaults
 */
void MainWindow::resetAdvancedOptionsButtonClicked(void)
{
	PLAY_SOUND_OPTIONAL("blast", true);

	ui->sliderLameAlgoQuality         ->setValue(m_settings->lameAlgoQualityDefault());
	ui->spinBoxBitrateManagementMin   ->setValue(m_settings->bitrateManagementMinRateDefault());
	ui->spinBoxBitrateManagementMax   ->setValue(m_settings->bitrateManagementMaxRateDefault());
	ui->spinBoxNormalizationFilterPeak->setValue(static_cast<double>(m_settings->normalizationFilterMaxVolumeDefault()) / 100.0);
	ui->spinBoxNormalizationFilterSize->setValue(m_settings->normalizationFilterSizeDefault());
	ui->spinBoxToneAdjustBass         ->setValue(static_cast<double>(m_settings->toneAdjustBassDefault()) / 100.0);
	ui->spinBoxToneAdjustTreble       ->setValue(static_cast<double>(m_settings->toneAdjustTrebleDefault()) / 100.0);
	ui->spinBoxAftenSearchSize        ->setValue(m_settings->aftenExponentSearchSizeDefault());
	ui->spinBoxOpusComplexity         ->setValue(m_settings->opusComplexityDefault());
	ui->comboBoxMP3ChannelMode        ->setCurrentIndex(m_settings->lameChannelModeDefault());
	ui->comboBoxSamplingRate          ->setCurrentIndex(m_settings->samplingRateDefault());
	ui->comboBoxAACProfile            ->setCurrentIndex(m_settings->aacEncProfileDefault());
	ui->comboBoxAftenCodingMode       ->setCurrentIndex(m_settings->aftenAudioCodingModeDefault());
	ui->comboBoxAftenDRCMode          ->setCurrentIndex(m_settings->aftenDynamicRangeCompressionDefault());
	ui->comboBoxOpusFramesize         ->setCurrentIndex(m_settings->opusFramesizeDefault());

	SET_CHECKBOX_STATE(ui->checkBoxBitrateManagement,          m_settings->bitrateManagementEnabledDefault());
	SET_CHECKBOX_STATE(ui->checkBoxNeroAAC2PassMode,           m_settings->neroAACEnable2PassDefault());
	SET_CHECKBOX_STATE(ui->checkBoxNormalizationFilterEnabled, m_settings->normalizationFilterEnabledDefault());
	SET_CHECKBOX_STATE(ui->checkBoxNormalizationFilterDynamic, m_settings->normalizationFilterDynamicDefault());
	SET_CHECKBOX_STATE(ui->checkBoxNormalizationFilterCoupled, m_settings->normalizationFilterCoupledDefault());
	SET_CHECKBOX_STATE(ui->checkBoxAutoDetectInstances,        (m_settings->maximumInstancesDefault() < 1));
	SET_CHECKBOX_STATE(ui->checkBoxUseSystemTempFolder,        (!m_settings->customTempPathEnabledDefault()));
	SET_CHECKBOX_STATE(ui->checkBoxAftenFastAllocation,        m_settings->aftenFastBitAllocationDefault());
	SET_CHECKBOX_STATE(ui->checkBoxRename_Rename,              m_settings->renameFiles_renameEnabledDefault());
	SET_CHECKBOX_STATE(ui->checkBoxRename_RegExp,              m_settings->renameFiles_regExpEnabledDefault());
	SET_CHECKBOX_STATE(ui->checkBoxForceStereoDownmix,         m_settings->forceStereoDownmixDefault());
	SET_CHECKBOX_STATE(ui->checkBoxOpusDisableResample,        m_settings->opusDisableResampleDefault());
	SET_CHECKBOX_STATE(ui->checkBoxKeepOriginalDateTime,       m_settings->keepOriginalDataTimeDefault());

	ui->lineEditCustomParamLAME     ->setText(m_settings->customParametersLAMEDefault());
	ui->lineEditCustomParamOggEnc   ->setText(m_settings->customParametersOggEncDefault());
	ui->lineEditCustomParamNeroAAC  ->setText(m_settings->customParametersAacEncDefault());
	ui->lineEditCustomParamFLAC     ->setText(m_settings->customParametersFLACDefault());
	ui->lineEditCustomParamOpus     ->setText(m_settings->customParametersOpusEncDefault());
	ui->lineEditCustomTempFolder    ->setText(QDir::toNativeSeparators(m_settings->customTempPathDefault()));
	ui->lineEditRenamePattern       ->setText(m_settings->renameFiles_renamePatternDefault());
	ui->lineEditRenameRegExp_Search ->setText(m_settings->renameFiles_regExpSearchDefault());
	ui->lineEditRenameRegExp_Replace->setText(m_settings->renameFiles_regExpReplaceDefault());

	if(m_settings->overwriteModeDefault() == SettingsModel::Overwrite_KeepBoth) ui->radioButtonOverwriteModeKeepBoth->click();
	if(m_settings->overwriteModeDefault() == SettingsModel::Overwrite_SkipFile) ui->radioButtonOverwriteModeSkipFile->click();
	if(m_settings->overwriteModeDefault() == SettingsModel::Overwrite_Replaces) ui->radioButtonOverwriteModeReplaces->click();

	if(FileExtsModel *const model = dynamic_cast<FileExtsModel*>(ui->tableViewFileExts->model()))
	{
		model->importItems(m_settings->renameFiles_fileExtensionDefault());
	}

	ui->scrollArea->verticalScrollBar()->setValue(0);
	ui->buttonRename_Rename->click();
	customParamsChanged();
	renameOutputPatternChanged();
	renameRegExpValueChanged();
}

// =========================================================
// Multi-instance handling slots
// =========================================================

/*
 * Other instance detected
 */
void MainWindow::notifyOtherInstance(void)
{
	if(!(BANNER_VISIBLE))
	{
		QMessageBox msgBox(QMessageBox::Warning, tr("Already Running"), tr("LameXP is already running, please use the running instance!"), QMessageBox::NoButton, this, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
		msgBox.exec();
	}
}

/*
 * Add file from another instance
 */
void MainWindow::addFileDelayed(const QString &filePath, bool tryASAP)
{
	if(tryASAP && !m_delayedFileTimer->isActive())
	{
		qDebug("Received file: %s", MUTILS_UTF8(filePath));
		m_delayedFileList->append(filePath);
		QTimer::singleShot(0, this, SLOT(handleDelayedFiles()));
	}
	
	m_delayedFileTimer->stop();
	qDebug("Received file: %s", MUTILS_UTF8(filePath));
	m_delayedFileList->append(filePath);
	m_delayedFileTimer->start(5000);
}

/*
 * Add files from another instance
 */
void MainWindow::addFilesDelayed(const QStringList &filePaths, bool tryASAP)
{
	if(tryASAP && (!m_delayedFileTimer->isActive()))
	{
		qDebug("Received %d file(s).", filePaths.count());
		m_delayedFileList->append(filePaths);
		QTimer::singleShot(0, this, SLOT(handleDelayedFiles()));
	}
	else
	{
		m_delayedFileTimer->stop();
		qDebug("Received %d file(s).", filePaths.count());
		m_delayedFileList->append(filePaths);
		m_delayedFileTimer->start(5000);
	}
}

/*
 * Add folder from another instance
 */
void MainWindow::addFolderDelayed(const QString &folderPath, bool recursive)
{
	if(!(BANNER_VISIBLE))
	{
		addFolder(folderPath, recursive, true);
	}
}

// =========================================================
// Misc slots
// =========================================================

/*
 * Restore the override cursor
 */
void MainWindow::restoreCursor(void)
{
	QApplication::restoreOverrideCursor();
}
