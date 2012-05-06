///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2012 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QDate>
#include <QTime>
#include <QDebug>
#include <QImage>
#include <QThreadPool>
#include <QTime>

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

FileAnalyzer::FileAnalyzer(const QStringList &inputFiles)
:
	m_abortFlag(false),
	m_inputFiles(inputFiles),
	m_templateFile(NULL)
{
	m_bSuccess = false;
	m_bAborted = false;
}

FileAnalyzer::~FileAnalyzer(void)
{
	if(m_templateFile)
	{
		QString templatePath = m_templateFile->filePath();
		LAMEXP_DELETE(m_templateFile);
		if(QFile::exists(templatePath)) QFile::remove(templatePath);
	}
	
	AnalyzeTask::reset();
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
	NULL
};

////////////////////////////////////////////////////////////
// Thread Main
////////////////////////////////////////////////////////////

void FileAnalyzer::run()
{
	m_abortFlag = false;

	m_bAborted = false;
	m_bSuccess = false;

	int nFiles = m_inputFiles.count();

	emit progressMaxChanged(nFiles);
	emit progressValChanged(0);

	m_inputFiles.sort();

	if(!m_templateFile)
	{
		if(!createTemplate())
		{
			qWarning("Failed to create template file!");
			return;
		}
	}

	AnalyzeTask::reset();
	QThreadPool *pool = new QThreadPool();
	QThread::msleep(333);

	if(pool->maxThreadCount() < 2)
	{
		pool->setMaxThreadCount(2);
	}

	while(!(m_inputFiles.isEmpty() || m_bAborted))
	{
		while(!(m_inputFiles.isEmpty() || m_bAborted))
		{
			const QString currentFile = QDir::fromNativeSeparators(m_inputFiles.takeFirst());

			AnalyzeTask *task = new AnalyzeTask(currentFile, m_templateFile->filePath(), &m_abortFlag);
			connect(task, SIGNAL(fileSelected(QString)), this, SIGNAL(fileSelected(QString)), Qt::DirectConnection);
			connect(task, SIGNAL(progressValChanged(unsigned int)), this, SIGNAL(progressValChanged(unsigned int)), Qt::DirectConnection);
			connect(task, SIGNAL(progressMaxChanged(unsigned int)), this, SIGNAL(progressMaxChanged(unsigned int)), Qt::DirectConnection);
			connect(task, SIGNAL(fileAnalyzed(AudioFileModel)), this, SIGNAL(fileAnalyzed(AudioFileModel)), Qt::DirectConnection);

			while(!pool->tryStart(task))
			{
				if(!AnalyzeTask::waitForOneThread(1250))
				{
					qWarning("FileAnalyzer: Timeout, retrying!");
				}
				
				if(m_abortFlag)
				{
					LAMEXP_DELETE(task);
					break;
				}

				QThread::yieldCurrentThread();
			}

			if(m_abortFlag)
			{
				MessageBeep(MB_ICONERROR);
				m_bAborted = true;
			}
			else
			{
				QThread::msleep(5);
			}
		}

		//One of the Analyze tasks may have gathered additional files from a playlist!
		if(!m_bAborted)
		{
			pool->waitForDone();
			AnalyzeTask::getAdditionalFiles(m_inputFiles);
			nFiles += m_inputFiles.count();
			emit progressMaxChanged(nFiles);
		}
	}
	
	pool->waitForDone();
	LAMEXP_DELETE(pool);

	if(m_bAborted)
	{
		qWarning("Operation cancelled by user!");
		return;
	}
	
	qDebug("All files added.\n");
	m_bSuccess = true;
}

////////////////////////////////////////////////////////////
// Privtae Functions
////////////////////////////////////////////////////////////

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
// Public Functions
////////////////////////////////////////////////////////////

unsigned int FileAnalyzer::filesAccepted(void)
{
	return AnalyzeTask::filesAccepted();
}

unsigned int FileAnalyzer::filesRejected(void)
{
	return AnalyzeTask::filesRejected();
}

unsigned int FileAnalyzer::filesDenied(void)
{
	return AnalyzeTask::filesDenied();
}

unsigned int FileAnalyzer::filesDummyCDDA(void)
{
	return AnalyzeTask::filesDummyCDDA();
}

unsigned int FileAnalyzer::filesCueSheet(void)
{
	return AnalyzeTask::filesCueSheet();
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/
