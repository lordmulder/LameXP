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

#include "Thread_FileAnalyzer.h"

#include "Global.h"
#include "LockedFile.h"
#include "Model_AudioFile.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QDate>
#include <QTime>

#include <math.h>

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

FileAnalyzer::FileAnalyzer(const QStringList &inputFiles)
	: m_inputFiles(inputFiles)
{
	m_bSuccess = false;
	m_mediaInfoBin = lamexp_lookup_tool("mediainfo_icl11.exe");
	
	if(m_mediaInfoBin.isEmpty())
	{
		qFatal("Invalid path to MediaInfo binary. Tool not initialized properly.");
	}

	m_filesAccepted = 0;
	m_filesRejected = 0;
	m_filesDenied = 0;
}

////////////////////////////////////////////////////////////
// Thread Main
////////////////////////////////////////////////////////////

void FileAnalyzer::run()
{
	m_bSuccess = false;

	m_filesAccepted = 0;
	m_filesRejected = 0;
	m_filesDenied = 0;

	m_inputFiles.sort();

	while(!m_inputFiles.isEmpty())
	{
		QString currentFile = QDir::fromNativeSeparators(m_inputFiles.takeFirst());
		qDebug("Analyzing: %s", currentFile.toUtf8().constData());
		emit fileSelected(QFileInfo(currentFile).fileName());
		AudioFileModel file = analyzeFile(currentFile);
		if(file.fileName().isEmpty() || file.formatContainerType().isEmpty() || file.formatAudioType().isEmpty())
		{
			m_filesRejected++;
			qDebug("Skipped: %s", file.filePath().toUtf8().constData());
			continue;
		}
		m_filesAccepted++;
		emit fileAnalyzed(file);
	}

	qDebug("All files added.\n");
	m_bSuccess = true;
}

////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////

const AudioFileModel FileAnalyzer::analyzeFile(const QString &filePath)
{
	AudioFileModel audioFile(filePath);
	m_currentSection = sectionOther;

	QFile readTest(filePath);
	if(!readTest.open(QIODevice::ReadOnly))
	{
		qWarning("Cannot access file for reading, skipping!");
		m_filesDenied++;
		return audioFile;
	}
	else
	{
		readTest.close();
	}

	QProcess process;
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.setReadChannel(QProcess::StandardOutput);
	process.start(m_mediaInfoBin, QStringList() << QDir::toNativeSeparators(filePath));
	process.waitForStarted();

	while(process.state() != QProcess::NotRunning)
	{
		if(!process.waitForReadyRead())
		{
			if(process.state() == QProcess::Running)
			{
				qWarning("MediaInfo time out. Killing process and skipping file!");
				process.kill();
				process.waitForFinished(-1);
				return audioFile;
			}
		}

		QByteArray data = process.readLine().constData();
		while(data.size() > 0)
		{
			QString line = QString::fromUtf8(data).simplified();
			if(!line.isEmpty())
			{
				int index = line.indexOf(':');
				if(index > 0)
				{
					QString key = line.left(index-1).trimmed();
					QString val = line.mid(index+1).trimmed();
					if(!key.isEmpty() && !val.isEmpty())
					{
						updateInfo(audioFile, key, val);
					}
				}
				else
				{
					updateSection(line);
				}
			}
			data = process.readLine().constData();
		}
	}

	if(audioFile.fileName().isEmpty())
	{
		QString baseName = QFileInfo(filePath).fileName();
		int index = baseName.lastIndexOf(".");

		if(index >= 0)
		{
			baseName = baseName.left(index);
		}

		baseName = baseName.replace("_", " ").simplified();
		index = baseName.lastIndexOf(" - ");

		if(index >= 0)
		{
			baseName = baseName.mid(index + 3).trimmed();
		}

		audioFile.setFileName(baseName);
	}
	
	return audioFile;
}

void FileAnalyzer::updateSection(const QString &section)
{
	if(section.startsWith("General", Qt::CaseInsensitive))
	{
		m_currentSection = sectionGeneral;
	}
	else if(!section.compare("Audio", Qt::CaseInsensitive) || section.startsWith("Audio #1", Qt::CaseInsensitive))
	{
		m_currentSection = sectionAudio;
	}
	else if(section.startsWith("Audio", Qt::CaseInsensitive) || section.startsWith("Video", Qt::CaseInsensitive) || section.startsWith("Text", Qt::CaseInsensitive) ||
		section.startsWith("Menu", Qt::CaseInsensitive) || section.startsWith("Image", Qt::CaseInsensitive) || section.startsWith("Chapters", Qt::CaseInsensitive))
	{
		m_currentSection = sectionOther;
	}
	else
	{
		qWarning("Unknown section: %s", section.toUtf8().constData());
	}
}

void FileAnalyzer::updateInfo(AudioFileModel &audioFile, const QString &key, const QString &value)
{
	switch(m_currentSection)
	{
	case sectionGeneral:
		if(!key.compare("Title", Qt::CaseInsensitive) || !key.compare("Track", Qt::CaseInsensitive) || !key.compare("Track Name", Qt::CaseInsensitive))
		{
			if(audioFile.fileName().isEmpty()) audioFile.setFileName(value);
		}
		else if(!key.compare("Duration", Qt::CaseInsensitive))
		{
			if(!audioFile.fileDuration()) audioFile.setFileDuration(parseDuration(value));
		}
		else if(!key.compare("Artist", Qt::CaseInsensitive) || !key.compare("Performer", Qt::CaseInsensitive))
		{
			if(audioFile.fileArtist().isEmpty()) audioFile.setFileArtist(value);
		}
		else if(!key.compare("Album", Qt::CaseInsensitive))
		{
			if(audioFile.fileAlbum().isEmpty()) audioFile.setFileAlbum(value);
		}
		else if(!key.compare("Genre", Qt::CaseInsensitive))
		{
			if(audioFile.fileGenre().isEmpty()) audioFile.setFileGenre(value);
		}
		else if(!key.compare("Year", Qt::CaseInsensitive) || !key.compare("Recorded Date", Qt::CaseInsensitive) || !key.compare("Encoded Date", Qt::CaseInsensitive))
		{
			if(!audioFile.fileYear()) audioFile.setFileYear(parseYear(value));
		}
		else if(!key.compare("Comment", Qt::CaseInsensitive))
		{
			if(audioFile.fileComment().isEmpty()) audioFile.setFileComment(value);
		}
		else if(!key.compare("Track Name/Position", Qt::CaseInsensitive))
		{
			if(!audioFile.filePosition()) audioFile.setFilePosition(value.toInt());
		}
		else if(!key.compare("Format", Qt::CaseInsensitive))
		{
			if(audioFile.formatContainerType().isEmpty()) audioFile.setFormatContainerType(value);
		}
		else if(!key.compare("Format Profile", Qt::CaseInsensitive))
		{
			if(audioFile.formatContainerProfile().isEmpty()) audioFile.setFormatContainerProfile(value);
		}
		break;

	case sectionAudio:
		if(!key.compare("Year", Qt::CaseInsensitive) || !key.compare("Recorded Date", Qt::CaseInsensitive) || !key.compare("Encoded Date", Qt::CaseInsensitive))
		{
			if(!audioFile.fileYear()) audioFile.setFileYear(parseYear(value));
		}
		else if(!key.compare("Format", Qt::CaseInsensitive))
		{
			if(audioFile.formatAudioType().isEmpty()) audioFile.setFormatAudioType(value);
		}
		else if(!key.compare("Format Profile", Qt::CaseInsensitive))
		{
			if(audioFile.formatAudioProfile().isEmpty()) audioFile.setFormatAudioProfile(value);
		}
		else if(!key.compare("Format Version", Qt::CaseInsensitive))
		{
			if(audioFile.formatAudioVersion().isEmpty()) audioFile.setFormatAudioVersion(value);
		}
		else if(!key.compare("Channel(s)", Qt::CaseInsensitive))
		{
			if(!audioFile.formatAudioChannels()) audioFile.setFormatAudioChannels(value.split(" ", QString::SkipEmptyParts).first().toInt());
		}
		else if(!key.compare("Sampling rate", Qt::CaseInsensitive))
		{
			if(!audioFile.formatAudioSamplerate()) audioFile.setFormatAudioSamplerate(ceil(value.split(" ", QString::SkipEmptyParts).first().toFloat() * 1000.0f));
		}
		else if(!key.compare("Bit depth", Qt::CaseInsensitive))
		{
			if(!audioFile.formatAudioBitdepth()) audioFile.setFormatAudioBitdepth(value.split(" ", QString::SkipEmptyParts).first().toInt());
		}
		else if(!key.compare("Duration", Qt::CaseInsensitive))
		{
			if(!audioFile.fileDuration()) audioFile.setFileDuration(parseDuration(value));
		}
		break;
	}
}

unsigned int FileAnalyzer::parseYear(const QString &str)
{
	if(str.startsWith("UTC", Qt::CaseInsensitive))
	{
		QDate date = QDate::fromString(str.mid(3).trimmed().left(10), "yyyy-MM-dd");
		if(date.isValid())
		{
			return date.year();
		}
		else
		{
			return 0;
		}
	}
	else
	{
		bool ok = false;
		int year = str.toInt(&ok);
		if(ok && year > 0)
		{
			return year;
		}
		else
		{
			return 0;
		}
	}
}

unsigned int FileAnalyzer::parseDuration(const QString &str)
{
	QTime time;

	time = QTime::fromString(str, "s's 'z'ms'");
	if(time.isValid())
	{
		return (time.hour() * 60 * 60) + (time.minute() * 60) + time.second();
	}

	time = QTime::fromString(str, "m'mn 's's'");
	if(time.isValid())
	{
		return (time.hour() * 60 * 60) + (time.minute() * 60) + time.second();
	}

	time = QTime::fromString(str, "h'h 'm'mn'");
	if(time.isValid())
	{
		return (time.hour() * 60 * 60) + (time.minute() * 60) + time.second();
	}

	return 0;
}

unsigned int FileAnalyzer::filesAccepted(void)
{
	return m_filesAccepted;
}

unsigned int FileAnalyzer::filesRejected(void)
{
	return m_filesRejected - m_filesDenied;
}

unsigned int FileAnalyzer::filesDenied(void)
{
	return m_filesDenied;
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/
