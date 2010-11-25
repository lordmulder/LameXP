///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2010 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Thread_Process.h"

#include "Global.h"
#include "Model_AudioFile.h"
#include "Model_Progress.h"
#include "Encoder_Abstract.h"

#include "Model_Settings.h"

#include <QUuid>
#include <QFileInfo>
#include <QDir>
#include <QMutex>
#include <QMutexLocker>

#include <limits.h>
#include <time.h>

QMutex *ProcessThread::m_mutex_genFileName = NULL;

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

ProcessThread::ProcessThread(const AudioFileModel &audioFile, const QString &outputDirectory, AbstractEncoder *encoder)
:
	m_audioFile(audioFile),
	m_outputDirectory(outputDirectory),
	m_encoder(encoder),
	m_jobId(QUuid::createUuid()),
	m_aborted(false)
{
	if(m_mutex_genFileName)
	{
		m_mutex_genFileName = new QMutex;
	}

	connect(m_encoder, SIGNAL(statusUpdated(int)), this, SLOT(handleUpdate(int)), Qt::DirectConnection);
	connect(m_encoder, SIGNAL(messageLogged(QString)), this, SLOT(handleMessage(QString)), Qt::DirectConnection);
}

ProcessThread::~ProcessThread(void)
{
	LAMEXP_DELETE(m_encoder);
}

void ProcessThread::run()
{
	try
	{
		processFile();
	}
	catch(...)
	{
		fflush(stdout);
		fflush(stderr);
		fprintf(stderr, "\nEXCEPTION ERROR !!!\n");
		FatalAppExit(0, L"Unhandeled exception error, application will exit!");
		TerminateProcess(GetCurrentProcess(), -1);
	}
}

void ProcessThread::processFile()
{
	m_aborted = false;

	qDebug("Process thread %s has started.", m_jobId.toString().toLatin1().constData());
	emit processStateInitialized(m_jobId, QFileInfo(m_audioFile.filePath()).fileName(), "Starting...", ProgressModel::JobRunning);

	QString outFileName = generateOutFileName();
	if(outFileName.isEmpty())
	{
		emit processStateChanged(m_jobId, "Not found!", ProgressModel::JobFailed);
		emit processStateFinished(m_jobId, outFileName, false);
		return;
	}

	if(!m_encoder->isFormatSupported(m_audioFile.formatContainerType(), m_audioFile.formatContainerProfile(), m_audioFile.formatAudioType(), m_audioFile.formatAudioProfile(), m_audioFile.formatAudioVersion()))
	{
		handleMessage(QString("The format of this file is NOT supported:\n%1\n\nContainer Format:\t%2\nAudio Format:\t%3").arg(m_audioFile.filePath(), m_audioFile.formatContainerInfo(), m_audioFile.formatAudioCompressInfo()));
		emit processStateChanged(m_jobId, "Unsupported!", ProgressModel::JobFailed);
		emit processStateFinished(m_jobId, outFileName, false);
		return;
	}

	bool bSuccess = m_encoder->encode(m_audioFile, outFileName, &m_aborted);

	if(bSuccess)
	{
		QFileInfo fileInfo(outFileName);
		bSuccess = fileInfo.exists() && fileInfo.isFile() && (fileInfo.size() > 0);
	}

	emit processStateChanged(m_jobId, (bSuccess ? "Done." : (m_aborted ? "Aborted!" : "Failed!")), (bSuccess ? ProgressModel::JobComplete : ProgressModel::JobFailed));
	emit processStateFinished(m_jobId, outFileName, bSuccess);

	qDebug("Process thread is done.");
}

////////////////////////////////////////////////////////////
// SLOTS
////////////////////////////////////////////////////////////

void ProcessThread::handleUpdate(int progress)
{
	emit processStateChanged(m_jobId, QString("Encoding (%1%)").arg(QString::number(progress)), ProgressModel::JobRunning);
}

void ProcessThread::handleMessage(const QString &line)
{
	emit processMessageLogged(m_jobId, line);
}

////////////////////////////////////////////////////////////
// PRIVAE FUNCTIONS
////////////////////////////////////////////////////////////

QString ProcessThread::generateOutFileName(void)
{
	QMutexLocker lock(m_mutex_genFileName);
	
	int n = 1;

	QFileInfo sourceFile(m_audioFile.filePath());
	if(!sourceFile.exists() || !sourceFile.isFile())
	{
		handleMessage(QString("The source audio file could not be found:\n%1").arg(sourceFile.absoluteFilePath()));
		return QString();
	}

	QFile readTest(sourceFile.canonicalFilePath());
	if(!readTest.open(QIODevice::ReadOnly))
	{
		handleMessage(QString("The source audio file could not be opened for reading:\n%1").arg(readTest.fileName()));
		return QString();
	}
	else
	{
		readTest.close();
	}

	QString baseName = sourceFile.completeBaseName();
	QDir targetDir(m_outputDirectory.isEmpty() ? sourceFile.canonicalPath() : m_outputDirectory);
	
	if(!targetDir.exists())
	{
		targetDir.mkpath(".");
		if(!targetDir.exists())
		{
			handleMessage(QString("The target output directory doesn't exist and could NOT be created:\n%1").arg(targetDir.absolutePath()));
			return QString();
		}
	}
	
	QFile writeTest(QString("%1/%2").arg(targetDir.canonicalPath(), QUuid::createUuid().toString()));
	if(!writeTest.open(QIODevice::ReadWrite))
	{
		handleMessage(QString("The target output directory is NOT writable:\n%1").arg(targetDir.absolutePath()));
		return QString();
	}
	else
	{
		writeTest.close();
		writeTest.remove();
	}

	QString outFileName = QString("%1/%2.%3").arg(targetDir.canonicalPath(), baseName, m_encoder->extension());
	while(QFileInfo(outFileName).exists())
	{
		outFileName = QString("%1/%2 (%3).%4").arg(targetDir.canonicalPath(), baseName, QString::number(++n), m_encoder->extension());
	}

	QFile placeholder(outFileName);
	if(placeholder.open(QIODevice::WriteOnly))
	{
		placeholder.close();
	}

	return outFileName;
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/
