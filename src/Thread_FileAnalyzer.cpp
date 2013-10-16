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

#include "Thread_FileAnalyzer.h"

#include "Global.h"
#include "LockedFile.h"
#include "Model_AudioFile.h"
#include "Thread_FileAnalyzer_Task.h"
#include "PlaylistImporter.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QDate>
#include <QTime>
#include <QDebug>
#include <QImage>
#include <QThreadPool>
#include <QTime>
#include <QElapsedTimer>
#include <QTimer>

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

FileAnalyzer::FileAnalyzer(const QStringList &inputFiles)
:
	m_tasksCounterNext(0),
	m_tasksCounterDone(0),
	m_inputFiles(inputFiles),
	m_templateFile(NULL),
	m_pool(NULL)
{
	m_bSuccess = false;
	m_bAborted = false;

	m_filesAccepted = 0;
	m_filesRejected = 0;
	m_filesDenied = 0;
	m_filesDummyCDDA = 0;
	m_filesCueSheet = 0;

	m_timer = new QElapsedTimer;
}

FileAnalyzer::~FileAnalyzer(void)
{
	if(m_templateFile)
	{
		QString templatePath = m_templateFile->filePath();
		LAMEXP_DELETE(m_templateFile);
		if(QFile::exists(templatePath)) QFile::remove(templatePath);
	}
	
	if(!m_pool->waitForDone(2500))
	{
		qWarning("There are still running tasks in the thread pool!");
	}

	LAMEXP_DELETE(m_pool);
	LAMEXP_DELETE(m_timer);
}

////////////////////////////////////////////////////////////
// Static data
////////////////////////////////////////////////////////////

const char *FileAnalyzer::g_tags_gen[] =
{
	"ID",
	"Format",
	"Format_Profile",
	"Format_Version",
	"Duration",
	"Title", "Track",
	"Track/Position",
	"Artist", "Performer",
	"Album",
	"Genre",
	"Released_Date", "Recorded_Date",
	"Comment",
	"Cover",
	"Cover_Type",
	"Cover_Mime",
	"Cover_Data",
	NULL
};

const char *FileAnalyzer::g_tags_aud[] =
{
	"ID",
	"Source",
	"Format",
	"Format_Profile",
	"Format_Version",
	"Channel(s)",
	"SamplingRate",
	"BitDepth",
	"BitRate",
	"BitRate_Mode",
	"Encoded_Library",
	NULL
};

////////////////////////////////////////////////////////////
// Thread Main
////////////////////////////////////////////////////////////

void FileAnalyzer::run()
{
	m_bSuccess = false;

	m_tasksCounterNext = 0;
	m_tasksCounterDone = 0;
	m_completedCounter = 0;

	m_completedFiles.clear();
	m_completedTaskIds.clear();
	m_runningTaskIds.clear();

	m_filesAccepted = 0;
	m_filesRejected = 0;
	m_filesDenied = 0;
	m_filesDummyCDDA = 0;
	m_filesCueSheet = 0;

	m_timer->invalidate();

	//Update progress
	emit progressMaxChanged(m_inputFiles.count());
	emit progressValChanged(0);

	//Create MediaInfo template file
	if(!m_templateFile)
	{
		if(!createTemplate())
		{
			qWarning("Failed to create template file!");
			return;
		}
	}

	//Handle playlist files
	lamexp_natural_string_sort(m_inputFiles, true);
	handlePlaylistFiles();
	lamexp_natural_string_sort(m_inputFiles, true);

	const unsigned int nFiles = m_inputFiles.count();

	//Update progress
	emit progressMaxChanged(nFiles);

	//Create thread pool
	if(!m_pool) m_pool = new QThreadPool();
	const int idealThreadCount = QThread::idealThreadCount();
	if(idealThreadCount > 0)
	{
		m_pool->setMaxThreadCount(qBound(2, ((idealThreadCount * 3) / 2), 12));
	}

	//Start first N threads
	QTimer::singleShot(0, this, SLOT(initializeTasks()));

	//Start event processing
	this->exec();

	//Wait for pending tasks to complete
	m_pool->waitForDone();

	//Was opertaion aborted?
	if(m_bAborted)
	{
		qWarning("Operation cancelled by user!");
		return;
	}
	
	//Update progress
	emit progressValChanged(nFiles);

	//Emit pending files (this should not be required though!)
	if(!m_completedFiles.isEmpty())
	{
		qWarning("FileAnalyzer: Pending file information found after last thread terminated!");
		QList<unsigned int> keys = m_completedFiles.keys(); qSort(keys);
		while(!keys.isEmpty())
		{
			emit fileAnalyzed(m_completedFiles.take(keys.takeFirst()));
		}
	}

	qDebug("All files added.\n");
	m_bSuccess = true;

	QThread::msleep(333);
}

////////////////////////////////////////////////////////////
// Privtae Functions
////////////////////////////////////////////////////////////

bool FileAnalyzer::analyzeNextFile(void)
{
	if(!(m_inputFiles.isEmpty() || m_bAborted))
	{
		const unsigned int taskId = m_tasksCounterNext++;
		const QString currentFile = QDir::fromNativeSeparators(m_inputFiles.takeFirst());

		if((!m_timer->isValid()) || (m_timer->elapsed() >= 250))
		{
			emit fileSelected(QFileInfo(currentFile).fileName());
			m_timer->restart();
		}
	
		AnalyzeTask *task = new AnalyzeTask(taskId, currentFile, m_templateFile->filePath(), &m_bAborted);
		connect(task, SIGNAL(fileAnalyzed(const unsigned int, const int, AudioFileModel)), this, SLOT(taskFileAnalyzed(unsigned int, const int, AudioFileModel)), Qt::QueuedConnection);
		connect(task, SIGNAL(taskCompleted(const unsigned int)), this, SLOT(taskThreadFinish(const unsigned int)), Qt::QueuedConnection);
		m_runningTaskIds.insert(taskId); m_pool->start(task);

		return true;
	}

	return false;
}

void FileAnalyzer::handlePlaylistFiles(void)
{
	QStringList importedFiles;
	while(!m_inputFiles.isEmpty())
	{
		const QString currentFile = m_inputFiles.takeFirst();
		if(!PlaylistImporter::importPlaylist(importedFiles, currentFile))
		{
			importedFiles << currentFile;
		}
	}

	while(!importedFiles.isEmpty())
	{
		const QString currentFile = importedFiles.takeFirst();
		if(!m_inputFiles.contains(currentFile, Qt::CaseInsensitive))
		{
			m_inputFiles << currentFile;
		}
	}
}

bool FileAnalyzer::createTemplate(void)
{
	if(m_templateFile)
	{
		qWarning("Template file already exists!");
		return true;
	}
	
	QString templatePath = QString("%1/%2.txt").arg(lamexp_temp_folder2(), lamexp_rand_str());

	QFile templateFile(templatePath);
	if(!templateFile.open(QIODevice::WriteOnly))
	{
		return false;
	}

	templateFile.write("General;");
	for(size_t i = 0; g_tags_gen[i]; i++)
	{
		templateFile.write(QString("Gen_%1=%%1%\\n").arg(g_tags_gen[i]).toLatin1().constData());
	}
	templateFile.write("\\n\r\n");

	templateFile.write("Audio;");
	for(size_t i = 0; g_tags_aud[i]; i++)
	{
		templateFile.write(QString("Aud_%1=%%1%\\n").arg(g_tags_aud[i]).toLatin1().constData());
	}
	templateFile.write("\\n\r\n");

	bool success = (templateFile.error() == QFile::NoError);
	templateFile.close();
	
	if(!success)
	{
		QFile::remove(templatePath);
		return false;
	}

	try
	{
		m_templateFile = new LockedFile(templatePath);
	}
	catch(...)
	{
		qWarning("Failed to lock template file!");
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////
// Slot Functions
////////////////////////////////////////////////////////////

void FileAnalyzer::initializeTasks(void)
{
	for(int i = 0; i < m_pool->maxThreadCount(); i++)
	{
		if(!analyzeNextFile()) break;
	}
}

void FileAnalyzer::taskFileAnalyzed(const unsigned int taskId, const int fileType, const AudioFileModel &file)
{
	m_completedTaskIds.insert(taskId);

	switch(fileType)
	{
	case AnalyzeTask::fileTypeNormal:
		m_filesAccepted++;
		if(m_tasksCounterDone == taskId)
		{
			emit fileAnalyzed(file);
			m_tasksCounterDone++;
		}
		else
		{
			m_completedFiles.insert(taskId, file);
		}
		break;
	case AnalyzeTask::fileTypeCDDA:
		m_filesDummyCDDA++;
		break;
	case AnalyzeTask::fileTypeDenied:
		m_filesDenied++;
		break;
	case AnalyzeTask::fileTypeCueSheet:
		m_filesCueSheet++;
		break;
	case AnalyzeTask::fileTypeUnknown:
		m_filesRejected++;
		break;
	default:
		throw "Unknown file type identifier!";
	}

	//Emit all pending files
	while(m_completedTaskIds.contains(m_tasksCounterDone))
	{
		if(m_completedFiles.contains(m_tasksCounterDone))
		{
			emit fileAnalyzed(m_completedFiles.take(m_tasksCounterDone));
		}
		m_completedTaskIds.remove(m_tasksCounterDone);
		m_tasksCounterDone++;
	}
}

void FileAnalyzer::taskThreadFinish(const unsigned int taskId)
{
	m_runningTaskIds.remove(taskId);
	emit progressValChanged(++m_completedCounter);

	if(!analyzeNextFile())
	{
		if(m_runningTaskIds.empty())
		{
			QTimer::singleShot(0, this, SLOT(quit())); //Stop event processing, if all threads have completed!
		}
	}
}

////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////

unsigned int FileAnalyzer::filesAccepted(void)
{
	return m_filesAccepted;
}

unsigned int FileAnalyzer::filesRejected(void)
{
	return m_filesRejected;
}

unsigned int FileAnalyzer::filesDenied(void)
{
	return m_filesDenied;
}

unsigned int FileAnalyzer::filesDummyCDDA(void)
{
	return m_filesDummyCDDA;
}

unsigned int FileAnalyzer::filesCueSheet(void)
{
	return m_filesCueSheet;
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/
