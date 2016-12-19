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

#include "Dialog_CueImport.h"

//UIC includes
#include "UIC_CueSheetImport.h"

//LameXP includes
#include "Global.h"
#include "Model_CueSheet.h"
#include "Model_AudioFile.h"
#include "Model_FileList.h"
#include "Dialog_WorkingBanner.h"
#include "Thread_FileAnalyzer.h"
#include "Thread_CueSplitter.h"
#include "Registry_Decoder.h"
#include "LockedFile.h"

//MUtils
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>
#include <MUtils/GUI.h>

//Qt includes
#include <QFileInfo>
#include <QMessageBox>
#include <QTimer>
#include <QFileDialog>
#include <QProgressDialog>
#include <QMenu>
#include <QTextCodec>
#include <QInputDialog>

#define SET_FONT_BOLD(WIDGET,BOLD) { QFont _font = WIDGET->font(); _font.setBold(BOLD); WIDGET->setFont(_font); }
#define EXPAND(STR) QString(STR).leftJustified(96, ' ')

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

CueImportDialog::CueImportDialog(QWidget *parent, FileListModel *fileList, const QString &cueFile, const SettingsModel *settings)
:
	QDialog(parent),
	ui(new Ui::CueSheetImport),
	m_fileList(fileList),
	m_cueFileName(cueFile),
	m_settings(settings)
{
	//Init the dialog, from the .ui file
	ui->setupUi(this);

	//Fix size
	setMinimumSize(this->size());
	setMaximumHeight(this->height());

	//Create model
	m_model = new CueSheetModel();
	connect(m_model, SIGNAL(modelReset()), this, SLOT(modelChanged()));
	
	//Setup table view
	ui->treeView->setModel(m_model);
	ui->treeView->header()->setStretchLastSection(false);
	ui->treeView->header()->setResizeMode(QHeaderView::ResizeToContents);
	ui->treeView->header()->setResizeMode(1, QHeaderView::Stretch);
	ui->treeView->header()->setMovable(false);
	ui->treeView->setItemsExpandable(false);

	//Enable up/down button
	connect(ui->imprtButton, SIGNAL(clicked()), this, SLOT(importButtonClicked()));
	connect(ui->browseButton, SIGNAL(clicked()), this, SLOT(browseButtonClicked()));
	connect(ui->loadOtherButton, SIGNAL(clicked()), this, SLOT(loadOtherButtonClicked()));

	//Translate
	ui->labelHeaderText->setText(QString("<b>%1</b><br>%2").arg(tr("Import Cue Sheet"), tr("The following Cue Sheet will be split and imported into LameXP.")));
}

CueImportDialog::~CueImportDialog(void)
{
	MUTILS_DELETE(m_model);
	MUTILS_DELETE(ui);
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

void CueImportDialog::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);
	modelChanged();
}

////////////////////////////////////////////////////////////
// Slots
////////////////////////////////////////////////////////////

int CueImportDialog::exec(void)
{
	WorkingBanner *progress = new WorkingBanner(dynamic_cast<QWidget*>(parent()));
	progress->show(tr("Loading Cue Sheet file, please be patient..."));

	QFileInfo cueFileInfo(m_cueFileName);
	if(!cueFileInfo.exists() || !cueFileInfo.isFile())
	{
		QString text = QString("<nobr>%1</nobr><br><nobr>%2</nobr><br><br><nobr>%3</nobr>").arg(tr("Failed to load the Cue Sheet file:"), QDir::toNativeSeparators(m_cueFileName), tr("The specified file could not be found!")).replace("-", "&minus;");
		QMessageBox::warning(progress, tr("Cue Sheet Error"), text);
		progress->close();
		MUTILS_DELETE(progress);
		return CueSheetModel::ErrorIOFailure;
	}

	//----------------------//

	QTextCodec *codec = NULL;

	QFile cueFile(cueFileInfo.canonicalFilePath());
	cueFile.open(QIODevice::ReadOnly);
	QByteArray bomCheck = cueFile.isOpen() ? cueFile.peek(16) : QByteArray();

	if((!bomCheck.isEmpty()) && bomCheck.startsWith("\xef\xbb\xbf"))
	{
		codec = QTextCodec::codecForName("UTF-8");
	}
	else if((!bomCheck.isEmpty()) && bomCheck.startsWith("\xff\xfe"))
	{
		codec = QTextCodec::codecForName("UTF-16LE");
	}
	else if((!bomCheck.isEmpty()) && bomCheck.startsWith("\xfe\xff"))
	{
		codec = QTextCodec::codecForName("UTF-16BE");
	}
	else
	{
		const QString systemDefault = tr("(System Default)");

		QStringList codecList;
		codecList.append(systemDefault);
		codecList.append(MUtils::available_codepages());

		QInputDialog *input = new QInputDialog(progress);
		input->setLabelText(EXPAND(tr("Select ANSI Codepage for Cue Sheet file:")));
		input->setOkButtonText(tr("OK"));
		input->setCancelButtonText(tr("Cancel"));
		input->setTextEchoMode(QLineEdit::Normal);
		input->setComboBoxItems(codecList);
	
		if(input->exec() < 1)
		{
			progress->close();
			MUTILS_DELETE(input);
			MUTILS_DELETE(progress);
			return Rejected;
		}
	
		if(input->textValue().compare(systemDefault, Qt::CaseInsensitive))
		{
			qDebug("User-selected codec is: %s", input->textValue().toLatin1().constData());
			codec = QTextCodec::codecForName(input->textValue().toLatin1().constData());
		}
		else
		{
			qDebug("Going to use the system's default codec!");
			codec = QTextCodec::codecForName("System");
		}

		MUTILS_DELETE(input);
	}

	bomCheck.clear();

	//----------------------//

	QString baseName = cueFileInfo.completeBaseName().simplified();
	while(baseName.endsWith(".") || baseName.endsWith(" ")) baseName.chop(1);
	if(baseName.isEmpty()) baseName = tr("New Folder");

	m_outputDir = QString("%1/%2").arg(cueFileInfo.canonicalPath(), baseName);
	for(int n = 2; QDir(m_outputDir).exists() || QFileInfo(m_outputDir).exists(); n++)
	{
		m_outputDir = QString("%1/%2 (%3)").arg(cueFileInfo.canonicalPath(), baseName, QString::number(n));
	}

	setWindowTitle(QString("%1: %2").arg(windowTitle().split(":", QString::SkipEmptyParts).first().trimmed(), cueFileInfo.fileName()));

	int iResult = m_model->loadCueSheet(m_cueFileName, QApplication::instance(), codec);
	if(iResult != CueSheetModel::ErrorSuccess)
	{
		QString errorMsg = tr("An unknown error has occured!");
		
		switch(iResult)
		{
		case CueSheetModel::ErrorIOFailure:
			errorMsg = tr("The file could not be opened for reading. Make sure you have the required rights!");
			break;
		case CueSheetModel::ErrorBadFile:
			errorMsg = tr("The provided file does not look like a valid Cue Sheet disc image file!");
			break;
		case CueSheetModel::ErrorUnsupported:
			errorMsg = QString("%1<br>%2").arg(tr("Could not find any supported audio track in the Cue Sheet image!"), tr("Note that LameXP can not handle \"binary\" Cue Sheet images."));
			break;
		case CueSheetModel::ErrorInconsistent:
			errorMsg = tr("The selected Cue Sheet file contains inconsistent information. Take care!");
			break;
		}
		
		QString text = QString("<nobr>%1</nobr><br><nobr>%2</nobr><br><br><nobr>%3</nobr>").arg(tr("Failed to load the Cue Sheet file:"), QDir::toNativeSeparators(m_cueFileName), errorMsg).replace("-", "&minus;");
		QMessageBox::warning(progress, tr("Cue Sheet Error"), text);
		progress->close();
		MUTILS_DELETE(progress);
		return iResult;
	}
	
	progress->close();
	MUTILS_DELETE(progress);
	return QDialog::exec();
}

void CueImportDialog::modelChanged(void)
{
	ui->treeView->expandAll();
	ui->editOutputDir->setText(QDir::toNativeSeparators(m_outputDir));
	if(const AudioFileModel_MetaInfo *albumInfo = m_model->getAlbumInfo())
	{
		ui->labelArtist->setText(albumInfo->artist().isEmpty() ? tr("Unknown Artist") : albumInfo->artist());
		ui->labelAlbum->setText(albumInfo->album().isEmpty() ? tr("Unknown Album") : albumInfo->album());
	}
}

void CueImportDialog::browseButtonClicked(void)
{
	QString currentDir = QDir::fromNativeSeparators(m_outputDir);
	while(!QDir(currentDir).exists())
	{
		const int pos = currentDir.lastIndexOf(QChar('/'));
		if(pos > 2)
		{
			currentDir = currentDir.left(pos);
			continue;
		}
		break;
	}

	QString newOutDir;
	if(MUtils::GUI::themes_enabled())
	{
		newOutDir = QFileDialog::getExistingDirectory(this, tr("Choose Output Directory"), currentDir);
	}
	else
	{
		QFileDialog dialog(this, tr("Choose Output Directory"));
		dialog.setFileMode(QFileDialog::DirectoryOnly);
		dialog.setDirectory(currentDir);
		if(dialog.exec())
		{
			newOutDir = dialog.selectedFiles().first();
		}
	}

	if(!newOutDir.isEmpty())
	{
		m_outputDir = newOutDir;
		modelChanged();
	}
}

void CueImportDialog::importButtonClicked(void)
{
	static const unsigned __int64 oneGigabyte = 1073741824ui64; 
	static const unsigned __int64 minimumFreeDiskspaceMultiplier = 2ui64;
	static const char *writeTestBuffer = "LAMEXP_WRITE_TEST";
	
	QDir outputDir(m_outputDir);
	outputDir.mkpath(".");
	if(!(outputDir.exists() && outputDir.isReadable()))
	{
		QMessageBox::warning(this, tr("LameXP"), QString("<nobr>%2</nobr>").arg(tr("Error: The selected output directory could not be created!")));
		return;
	}

	QFile writeTest(QString("%1/~%2.txt").arg(m_outputDir, MUtils::next_rand_str()));
	if(!(writeTest.open(QIODevice::ReadWrite) && (writeTest.write(writeTestBuffer) == strlen(writeTestBuffer))))
	{
		QMessageBox::warning(this, tr("LameXP"), QString("<nobr>%2</nobr>").arg(tr("Error: The selected output directory is not writable!")));
		return;
	}
	else
	{
		writeTest.close();
		writeTest.remove();
	}

	quint64 currentFreeDiskspace = 0;
	if(MUtils::OS::free_diskspace(m_outputDir, currentFreeDiskspace))
	{
		if(currentFreeDiskspace < (oneGigabyte * minimumFreeDiskspaceMultiplier))
		{
			QMessageBox::warning(this, tr("Low Diskspace Warning"), QString("<nobr>%1</nobr><br><nobr>%2</nobr>").arg(tr("There are less than %1 GB of free diskspace available in the selected output directory.").arg(QString::number(minimumFreeDiskspaceMultiplier)), tr("It is highly recommend to free up more diskspace before proceeding with the import!")));
			return;
		}
	}

	importCueSheet();
	accept();
}

void CueImportDialog::loadOtherButtonClicked(void)
{
	done(-1);
}

void CueImportDialog::analyzedFile(const AudioFileModel &file)
{
	qDebug("Received result: <%s> <%s/%s>", file.filePath().toLatin1().constData(), file.techInfo().containerType().toLatin1().constData(), file.techInfo().audioType().toLatin1().constData());
	m_fileInfo << file;
}

////////////////////////////////////////////////////////////
// Private Functions
////////////////////////////////////////////////////////////

void CueImportDialog::importCueSheet(void)
{
	QStringList files;
	QList<LockedFile*> locks;

	//Fetch all files that are referenced in the Cue Sheet and lock them
	int nFiles = m_model->getFileCount();
	for(int i = 0; i < nFiles; i++)
	{
		try
		{
			LockedFile *temp = new LockedFile(m_model->getFileName(i));
			locks << temp;
			files << temp->filePath();
		}
		catch(const std::exception &error)
		{
			qWarning("Failed to lock file:\n%s\n", error.what());
		}
		catch(...)
		{
			qWarning("Failed to lock file!");
		}
	}

	//Check if all files could be locked
	if(files.count() < m_model->getFileCount())
	{
		if(QMessageBox::warning(this, tr("Cue Sheet Error"), tr("Warning: Some of the required input files could not be found!"), tr("Continue Anyway"), tr("Abort")) == 1)
		{
			files.clear();
		}
	}

	//Process all avialble input files
	if(files.count() > 0)
	{
		//Analyze all source files first
		if(analyzeFiles(files))
		{
			//Now split files according to Cue Sheet
			splitFiles();
		}
	}
	
	//Release locks
	while(!locks.isEmpty())
	{
		delete locks.takeFirst();
	}
}

bool CueImportDialog::analyzeFiles(QStringList &files)
{
	m_fileInfo.clear();
	bool bSuccess = true;

	WorkingBanner *progress = new WorkingBanner(this);
	FileAnalyzer *analyzer = new FileAnalyzer(files);
	
	connect(analyzer, SIGNAL(fileSelected(QString)), progress, SLOT(setText(QString)), Qt::QueuedConnection);
	connect(analyzer, SIGNAL(fileAnalyzed(AudioFileModel)), this, SLOT(analyzedFile(AudioFileModel)), Qt::QueuedConnection);
	connect(progress, SIGNAL(userAbort()), analyzer, SLOT(abortProcess()), Qt::DirectConnection);

	progress->show(tr("Analyzing file(s), please wait..."), analyzer);
	progress->close();

	if(analyzer->filesAccepted() < static_cast<unsigned int>(files.count()))
	{
		if(QMessageBox::warning(this, tr("Analysis Failed"), tr("Warning: The format of some of the input files could not be determined!"), tr("Continue Anyway"), tr("Abort")) == 1)
		{
			bSuccess = false;
		}
	}

	MUTILS_DELETE(progress);
	MUTILS_DELETE(analyzer);

	return bSuccess;
}

void CueImportDialog::splitFiles(void)
{
	QString baseName = QFileInfo(m_cueFileName).completeBaseName().replace(".", " ").simplified();

	WorkingBanner *progress = new WorkingBanner(this);
	CueSplitter *splitter  = new CueSplitter(m_outputDir, baseName, m_model, m_fileInfo);

	connect(splitter, SIGNAL(fileSelected(QString)), progress, SLOT(setText(QString)), Qt::QueuedConnection);
	connect(splitter, SIGNAL(fileSplit(AudioFileModel)), m_fileList, SLOT(addFile(AudioFileModel)), Qt::QueuedConnection);
	connect(splitter, SIGNAL(progressValChanged(unsigned int)), progress, SLOT(setProgressVal(unsigned int)), Qt::QueuedConnection);
	connect(splitter, SIGNAL(progressMaxChanged(unsigned int)), progress, SLOT(setProgressMax(unsigned int)), Qt::QueuedConnection);
	connect(progress, SIGNAL(userAbort()), splitter, SLOT(abortProcess()), Qt::DirectConnection);

	DecoderRegistry::configureDecoders(m_settings);

	progress->show(tr("Splitting file(s), please wait..."), splitter);
	progress->close();

	if(splitter->getAborted())
	{
		QMessageBox::warning(this, tr("Cue Sheet Error"), tr("Process was aborted by the user after %n track(s)!", "", splitter->getTracksSuccess()));
	}
	else if(!splitter->getSuccess())
	{
		QMessageBox::warning(this, tr("Cue Sheet Error"), tr("An unexpected error has occured while splitting the Cue Sheet!"));
	}
	else
	{
		QString text = QString("<nobr>%1 %2</nobr>").arg(tr("Imported %n track(s) from the Cue Sheet.", "", splitter->getTracksSuccess()), tr("Skipped %n track(s).", "", splitter->getTracksSkipped()));
		QMessageBox::information(this, tr("Cue Sheet Completed"), text);
	}

	MUTILS_DELETE(splitter);
	MUTILS_DELETE(progress);
}
