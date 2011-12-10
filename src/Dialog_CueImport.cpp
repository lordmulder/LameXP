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

#include "Dialog_CueImport.h"

#include "Global.h"
#include "Model_CueSheet.h"
#include "Model_AudioFile.h"
#include "Model_FileList.h"
#include "Dialog_WorkingBanner.h"
#include "Thread_FileAnalyzer.h"
#include "Thread_CueSplitter.h"
#include "LockedFile.h"

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

CueImportDialog::CueImportDialog(QWidget *parent, FileListModel *fileList, const QString &cueFile)
:
	QDialog(parent),
	m_cueFileName(cueFile),
	m_fileList(fileList)
{
	//Init the dialog, from the .ui file
	setupUi(this);

	//Fix size
	setMinimumSize(this->size());
	setMaximumHeight(this->height());

	//Create model
	m_model = new CueSheetModel();
	connect(m_model, SIGNAL(modelReset()), this, SLOT(modelChanged()));
	
	//Setup table view
	treeView->setModel(m_model);
	treeView->header()->setStretchLastSection(false);
	treeView->header()->setResizeMode(QHeaderView::ResizeToContents);
	treeView->header()->setResizeMode(1, QHeaderView::Stretch);
	treeView->header()->setMovable(false);
	treeView->setItemsExpandable(false);

	//Enable up/down button
	connect(imprtButton, SIGNAL(clicked()), this, SLOT(importButtonClicked()));
	connect(browseButton, SIGNAL(clicked()), this, SLOT(browseButtonClicked()));
	connect(loadOtherButton, SIGNAL(clicked()), this, SLOT(loadOtherButtonClicked()));

	//Translate
	labelHeaderText->setText(QString("<b>%1</b><br>%2").arg(tr("Import Cue Sheet"), tr("The following Cue Sheet will be split and imported into LameXP.")));
}

CueImportDialog::~CueImportDialog(void)
{
	LAMEXP_DELETE(m_model);
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
		LAMEXP_DELETE(progress);
		return CueSheetModel::ErrorIOFailure;
	}

	//----------------------//

	QTextCodec *codec = NULL;

	QFile cueFile(cueFileInfo.canonicalFilePath());
	cueFile.open(QIODevice::ReadOnly);
	QByteArray bomCheck = cueFile.isOpen() ? cueFile.peek(128) : QByteArray();

	if((!bomCheck.isEmpty()) && bomCheck.contains("\xef\xbb\xbf"))
	{
		codec = QTextCodec::codecForName("UTF-8");
	}
	else
	{
		const QString systemDefault = tr("(System Default)");

		QStringList codecList; codecList << systemDefault;
		QList<QByteArray> availableCodecs = QTextCodec::availableCodecs();
		while(!availableCodecs.isEmpty())
		{
			QByteArray current = availableCodecs.takeFirst();
			if(!(current.startsWith("system") || current.startsWith("System")))
			{
				codecList << QString::fromLatin1(current.constData(), current.size());
			}
		}

		QInputDialog *input = new QInputDialog(progress);
		input->setLabelText(EXPAND(tr("Select ANSI Codepage for Cue Sheet file:")));
		input->setOkButtonText(tr("OK"));
		input->setCancelButtonText(tr("Cancel"));
		input->setTextEchoMode(QLineEdit::Normal);
		input->setComboBoxItems(codecList);
	
		if(input->exec() > 0)
		{
			qDebug("User-selected codec is: %s", input->textValue().toLatin1().constData());
			if(input->textValue().compare(systemDefault, Qt::CaseInsensitive))
			{
				qDebug("Going to use a user-selected codec!");
				codec = QTextCodec::codecForName(input->textValue().toLatin1().constData());
			}
		}

		if(!codec)
		{
			qDebug("Going to use the system's default codec!");
			codec = QTextCodec::codecForName("System");
		}

		LAMEXP_DELETE(input);
	}

	bomCheck.clear();

	//----------------------//

	m_outputDir = QString("%1/%2").arg(cueFileInfo.canonicalPath(), cueFileInfo.completeBaseName());
	for(int n = 2; QDir(m_outputDir).exists(); n++)
	{
		m_outputDir = QString("%1/%2 (%3)").arg(cueFileInfo.canonicalPath(), cueFileInfo.completeBaseName(), QString::number(n));
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
		LAMEXP_DELETE(progress);
		return iResult;
	}
	
	progress->close();
	LAMEXP_DELETE(progress);
	return QDialog::exec();
}

void CueImportDialog::modelChanged(void)
{
	treeView->expandAll();
	editOutputDir->setText(QDir::toNativeSeparators(m_outputDir));
	labelArtist->setText(m_model->getAlbumPerformer().isEmpty() ? tr("Unknown Artist") : m_model->getAlbumPerformer());
	labelAlbum->setText(m_model->getAlbumTitle().isEmpty() ? tr("Unknown Album") : m_model->getAlbumTitle());
}

void CueImportDialog::browseButtonClicked(void)
{
	QString newOutDir, currentDir = m_outputDir;
	
	while(QDir(currentDir).exists())
	{
		int pos = qMax(currentDir.lastIndexOf(QChar('\\')), currentDir.lastIndexOf(QChar('/')));
		if(pos > 0) currentDir.left(pos - 1); else break;
	}

	if(lamexp_themes_enabled() || ((QSysInfo::windowsVersion() & QSysInfo::WV_NT_based) < QSysInfo::WV_XP))
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

	QFile writeTest(QString("%1/~%2.txt").arg(m_outputDir, lamexp_rand_str()));
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

	bool ok = false;
	unsigned __int64 currentFreeDiskspace = lamexp_free_diskspace(m_outputDir, &ok);

	if(ok && (currentFreeDiskspace < (oneGigabyte * minimumFreeDiskspaceMultiplier)))
	{
		QMessageBox::warning(this, tr("Low Diskspace Warning"), QString("<nobr>%1</nobr><br><nobr>%2</nobr>").arg(tr("There are less than %1 GB of free diskspace available in the selected output directory.").arg(QString::number(minimumFreeDiskspaceMultiplier)), tr("It is highly recommend to free up more diskspace before proceeding with the import!")));
		return;
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
	qDebug("Received result: <%s> <%s/%s>", file.filePath().toLatin1().constData(), file.formatContainerType().toLatin1().constData(), file.formatAudioType().toLatin1().constData());
	m_fileInfo << file;
}

////////////////////////////////////////////////////////////
// Private Functions
////////////////////////////////////////////////////////////

void CueImportDialog::importCueSheet(void)
{
	QStringList files;

	//Fetch all files that are referenced in the Cue Sheet and lock them
	int nFiles = m_model->getFileCount();
	for(int i = 0; i < nFiles; i++)
	{
		QString temp = m_model->getFileName(i);
		try
		{
			m_locks << new LockedFile(temp);
		}
		catch(char *err)
		{
			qWarning("Failed to lock file: %s", err);
			continue;
		}
		files << temp;
	}
	
	//Analyze all source files first
	if(analyzeFiles(files))
	{
		//Now split files according to Cue Sheet
		splitFiles();
	}
	
	//Release locks
	while(!m_locks.isEmpty())
	{
		delete m_locks.takeFirst();
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

	LAMEXP_DELETE(progress);
	LAMEXP_DELETE(analyzer);

	return bSuccess;
}

void CueImportDialog::splitFiles(void)
{
	QString baseName = QFileInfo(m_cueFileName).completeBaseName().replace(".", " ").simplified();

	WorkingBanner *progress = new WorkingBanner(this);
	CueSplitter *splitter  = new CueSplitter(m_outputDir, baseName, m_model, m_fileInfo);

	connect(splitter, SIGNAL(fileSelected(QString)), progress, SLOT(setText(QString)), Qt::QueuedConnection);
	connect(splitter, SIGNAL(fileSplit(AudioFileModel)), m_fileList, SLOT(addFile(AudioFileModel)), Qt::QueuedConnection);
	connect(progress, SIGNAL(userAbort()), splitter, SLOT(abortProcess()), Qt::DirectConnection);

	progress->show(tr("Splitting file(s), please wait..."), splitter);
	progress->close();

	if(splitter->getAborted())	
	{
		QMessageBox::warning(this, tr("Cue Sheet Error"), tr("Process was aborted by the user after %1 track(s)!").arg(QString::number(splitter->getTracksSuccess())));
	}
	else if(!splitter->getSuccess())
	{
		QMessageBox::warning(this, tr("Cue Sheet Error"), tr("An unexpected error has occured while splitting the Cue Sheet!"));
	}
	else
	{
		QString text = QString("<nobr>%1</nobr>").arg(tr("Imported %1 track(s) from the Cue Sheet and skipped %2 track(s).").arg(QString::number(splitter->getTracksSuccess()), QString::number(splitter->getTracksSkipped() /*+ nTracksSkipped*/)));
		QMessageBox::information(this, tr("Cue Sheet Completed"), text);
	}

	LAMEXP_DELETE(splitter);
	LAMEXP_DELETE(progress);
}
