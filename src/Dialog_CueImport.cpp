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
#include "Dialog_WorkingBanner.h"
#include "Thread_FileAnalyzer.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QTimer>
#include <QFileDialog>
#include <QProgressDialog>
#include <QMenu>

#define SET_FONT_BOLD(WIDGET,BOLD) { QFont _font = WIDGET->font(); _font.setBold(BOLD); WIDGET->setFont(_font); }

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

CueImportDialog::CueImportDialog(QWidget *parent)
:
	QDialog(parent)
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

int CueImportDialog::exec(const QString &cueFile)
{
	WorkingBanner *progress = new WorkingBanner(dynamic_cast<QWidget*>(parent()));
	progress->show(tr("Loading Cue Sheet file, please be patient..."));
	
	QFileInfo cueFileInfo(cueFile);
	m_outputDir = QFileInfo(cueFile).canonicalPath();

	setWindowTitle(QString("%1: %2").arg(windowTitle().split(":", QString::SkipEmptyParts).first().trimmed(), cueFileInfo.fileName()));

	if(!cueFileInfo.exists() || !cueFileInfo.isFile() || m_outputDir.isEmpty())
	{
		QString text = QString("<nobr>%1</nobr><br><nobr>%2</nobr><br><br><nobr>%3</nobr>").arg(tr("Failed to load the Cue Sheet file:"), QDir::toNativeSeparators(cueFile), tr("The specified file could not be found!")).replace("-", "&minus;");
		QMessageBox::warning(progress, tr("Cue Sheet Error"), text);
		progress->close();
		LAMEXP_DELETE(progress);
		return CueSheetModel::ErrorIOFailure;
	}

	int iResult = m_model->loadCueSheet(cueFile, QApplication::instance());
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
		
		QString text = QString("<nobr>%1</nobr><br><nobr>%2</nobr><br><br><nobr>%3</nobr>").arg(tr("Failed to load the Cue Sheet file:"), QDir::toNativeSeparators(cueFile), errorMsg).replace("-", "&minus;");
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
}

void CueImportDialog::browseButtonClicked(void)
{
	QString newOutDir = QFileDialog::getExistingDirectory(this, tr("Choose Output Directory"));
	if(!newOutDir.isEmpty())
	{
		m_outputDir = newOutDir;
		modelChanged();
	}
}

void CueImportDialog::importButtonClicked(void)
{
	static const __int64 oneGigabyte = 1073741824i64; 
	static const __int64 minimumFreeDiskspaceMultiplier = 2i64;
	static const char *writeTestBuffer = "LAMEXP_WRITE_TEST";
	
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

	qint64 currentFreeDiskspace = lamexp_free_diskspace(m_outputDir);
	if(currentFreeDiskspace < (oneGigabyte * minimumFreeDiskspaceMultiplier))
	{
		QMessageBox::warning(this, tr("Low Diskspace Warning"), QString("<nobr>%1</nobr><br><nobr>%2</nobr>").arg(tr("There are less than %1 GB of free diskspace available in the selected output directory.").arg(QString::number(minimumFreeDiskspaceMultiplier)), tr("It is highly recommend to free up more diskspace before proceeding with the import!")));
		return;
	}

	importCueSheet();
}

void CueImportDialog::analyzedFile(const AudioFileModel &file)
{
	qWarning("Received results for: %s", file.filePath().toLatin1().constData());
	m_fileInfo.insert(file.filePath(), file);
}

////////////////////////////////////////////////////////////
// Private FUnctions
////////////////////////////////////////////////////////////

void CueImportDialog::importCueSheet(void)
{
	QStringList files;
	int nFiles = m_model->getFileCount();

	//Fetch all files that are referenced in the Cue Sheet
	for(int i = 0; i < nFiles; i++)
	{
		files << m_model->getFileName(i);
	}
	
	//Analyze all source files
	analyzeFiles(files);
}

void CueImportDialog::analyzeFiles(QStringList &files)
{
	m_fileInfo.clear();

	WorkingBanner *progress = new WorkingBanner(dynamic_cast<QWidget*>(parent()));
	FileAnalyzer *analyzer = new FileAnalyzer(files);
	connect(analyzer, SIGNAL(fileSelected(QString)), progress, SLOT(setText(QString)), Qt::QueuedConnection);
	connect(analyzer, SIGNAL(fileAnalyzed(AudioFileModel)), this, SLOT(analyzedFile(AudioFileModel)), Qt::QueuedConnection);

	progress->show(tr("Adding file(s), please wait..."), analyzer);
	progress->close();
	LAMEXP_DELETE(progress);
}
