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

#include "Thread_FileAnalyzer.h"

#include "Global.h"
#include "LockedFile.h"
#include "Model_AudioFile.h"
#include "PlaylistImporter.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QDate>
#include <QTime>
#include <QDebug>

#include <math.h>

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

FileAnalyzer::FileAnalyzer(const QStringList &inputFiles)
:
	m_inputFiles(inputFiles),
	m_mediaInfoBin(lamexp_lookup_tool("mediainfo.exe")),
	m_avs2wavBin(lamexp_lookup_tool("avs2wav.exe")),
	m_abortFlag(false)
{
	m_bSuccess = false;
	m_bAborted = false;
		
	if(m_mediaInfoBin.isEmpty())
	{
		qFatal("Invalid path to MediaInfo binary. Tool not initialized properly.");
	}

	m_filesAccepted = 0;
	m_filesRejected = 0;
	m_filesDenied = 0;
	m_filesDummyCDDA = 0;
	m_filesCueSheet = 0;
}

////////////////////////////////////////////////////////////
// Thread Main
////////////////////////////////////////////////////////////

void FileAnalyzer::run()
{
	m_bSuccess = false;
	m_bAborted = false;

	m_filesAccepted = 0;
	m_filesRejected = 0;
	m_filesDenied = 0;
	m_filesDummyCDDA = 0;
	m_filesCueSheet = 0;

	m_inputFiles.sort();

	m_abortFlag = false;

	while(!m_inputFiles.isEmpty())
	{
		int fileType = fileTypeNormal;
		QString currentFile = QDir::fromNativeSeparators(m_inputFiles.takeFirst());
		qDebug64("Analyzing: %1", currentFile);
		emit fileSelected(QFileInfo(currentFile).fileName());
		AudioFileModel file = analyzeFile(currentFile, &fileType);
		
		if(m_abortFlag)
		{
			MessageBeep(MB_ICONERROR);
			m_bAborted = true;
			qWarning("Operation cancelled by user!");
			return;
		}

		if(fileType == fileTypeDenied)
		{
			m_filesDenied++;
			qWarning("Cannot access file for reading, skipping!");
			continue;
		}
		if(fileType == fileTypeCDDA)
		{
			m_filesDummyCDDA++;
			qWarning("Dummy CDDA file detected, skipping!");
			continue;
		}
		
		if(file.fileName().isEmpty() || file.formatContainerType().isEmpty() || file.formatAudioType().isEmpty())
		{
			if(PlaylistImporter::importPlaylist(m_inputFiles, currentFile))
			{
				qDebug("Imported playlist file.");
			}
			else if(!QFileInfo(currentFile).suffix().compare("cue", Qt::CaseInsensitive))
			{
				qWarning("Cue Sheet file detected, skipping!");
				m_filesCueSheet++;
			}
			else if(!QFileInfo(currentFile).suffix().compare("avs", Qt::CaseInsensitive))
			{
				qDebug("Found a potential Avisynth script, investigating...");
				if(analyzeAvisynthFile(currentFile, file))
				{
					m_filesAccepted++;
					emit fileAnalyzed(file);
				}
			}
			else
			{
				qDebug64("Rejected file of unknown type: %1", file.filePath());
				m_filesRejected++;
			}
			continue;
		}

		m_filesAccepted++;
		emit fileAnalyzed(file);
	}

	qDebug("All files added.\n");
	m_bSuccess = true;
}

////////////////////////////////////////////////////////////
// Privtae Functions
////////////////////////////////////////////////////////////

const AudioFileModel FileAnalyzer::analyzeFile(const QString &filePath, int *type)
{
	*type = fileTypeNormal;
	
	AudioFileModel audioFile(filePath);
	m_currentSection = sectionOther;
	m_currentCover = coverNone;

	QFile readTest(filePath);
	if(!readTest.open(QIODevice::ReadOnly))
	{
		*type = fileTypeDenied;
		return audioFile;
	}
	
	if(checkFile_CDDA(readTest))
	{
		*type = fileTypeCDDA;
		return audioFile;
	}
	
	readTest.close();

	QProcess process;
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.setReadChannel(QProcess::StandardOutput);
	process.start(m_mediaInfoBin, QStringList() << QDir::toNativeSeparators(filePath));
		
	if(!process.waitForStarted())
	{
		qWarning("MediaInfo process failed to create!");
		qWarning("Error message: \"%s\"\n", process.errorString().toLatin1().constData());
		process.kill();
		process.waitForFinished(-1);
		return audioFile;
	}

	while(process.state() != QProcess::NotRunning)
	{
		if(m_abortFlag)
		{
			process.kill();
			qWarning("Process was aborted on user request!");
			break;
		}
		
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

		QByteArray data;

		while(process.canReadLine())
		{
			QString line = QString::fromUtf8(process.readLine().constData()).simplified();
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
	
	process.waitForFinished();
	if(process.state() != QProcess::NotRunning)
	{
		process.kill();
		process.waitForFinished(-1);
	}

	if(m_currentCover != coverNone)
	{
		retrieveCover(audioFile, filePath);
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
		m_currentSection = sectionOther;
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
		else if(!key.compare("Cover", Qt::CaseInsensitive) || !key.compare("Cover type", Qt::CaseInsensitive))
		{
			if(m_currentCover == coverNone) m_currentCover = coverJpeg;
		}
		else if(!key.compare("Cover MIME", Qt::CaseInsensitive))
		{
			QString temp = value.split(" ", QString::SkipEmptyParts, Qt::CaseInsensitive).first();
			if(!temp.compare("image/jpeg", Qt::CaseInsensitive))
			{
				m_currentCover = coverJpeg;
			}
			else if(!temp.compare("image/png", Qt::CaseInsensitive))
			{
				m_currentCover = coverPng;
			}
			else if(!temp.compare("image/gif", Qt::CaseInsensitive))
			{
				m_currentCover = coverGif;
			}
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

	time = QTime::fromString(str, "z'ms'");
	if(time.isValid())
	{
		return max(1, (time.hour() * 60 * 60) + (time.minute() * 60) + time.second());
	}

	time = QTime::fromString(str, "s's 'z'ms'");
	if(time.isValid())
	{
		return max(1, (time.hour() * 60 * 60) + (time.minute() * 60) + time.second());
	}

	time = QTime::fromString(str, "m'mn 's's'");
	if(time.isValid())
	{
		return max(1, (time.hour() * 60 * 60) + (time.minute() * 60) + time.second());
	}

	time = QTime::fromString(str, "h'h 'm'mn'");
	if(time.isValid())
	{
		return max(1, (time.hour() * 60 * 60) + (time.minute() * 60) + time.second());
	}

	return 0;
}

bool FileAnalyzer::checkFile_CDDA(QFile &file)
{
	file.reset();
	QByteArray data = file.read(128);
	
	int i = data.indexOf("RIFF");
	int j = data.indexOf("CDDA");
	int k = data.indexOf("fmt ");

	return ((i >= 0) && (j >= 0) && (k >= 0) && (k > j) && (j > i));
}

void FileAnalyzer::retrieveCover(AudioFileModel &audioFile, const QString &filePath)
{
	qDebug64("Retrieving cover from: %1", filePath);
	QString extension;

	switch(m_currentCover)
	{
	case coverPng:
		extension = QString::fromLatin1("png");
		break;
	case coverGif:
		extension = QString::fromLatin1("gif");
		break;
	default:
		extension = QString::fromLatin1("jpg");
		break;
	}
	
	QProcess process;
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.setReadChannel(QProcess::StandardOutput);
	process.start(m_mediaInfoBin, QStringList() << "-f" << QDir::toNativeSeparators(filePath));
	
	if(!process.waitForStarted())
	{
		qWarning("MediaInfo process failed to create!");
		qWarning("Error message: \"%s\"\n", process.errorString().toLatin1().constData());
		process.kill();
		process.waitForFinished(-1);
		return;
	}

	while(process.state() != QProcess::NotRunning)
	{
		if(m_abortFlag)
		{
			process.kill();
			qWarning("Process was aborted on user request!");
			break;
		}
		
		if(!process.waitForReadyRead())
		{
			if(process.state() == QProcess::Running)
			{
				qWarning("MediaInfo time out. Killing process and skipping file!");
				process.kill();
				process.waitForFinished(-1);
				return;
			}
		}

		while(process.canReadLine())
		{
			QString line = QString::fromUtf8(process.readLine().constData()).simplified();
			if(!line.isEmpty())
			{
				int index = line.indexOf(':');
				if(index > 0)
				{
					QString key = line.left(index-1).trimmed();
					QString val = line.mid(index+1).trimmed();
					if(!key.isEmpty() && !val.isEmpty())
					{
						if(!key.compare("Cover_Data", Qt::CaseInsensitive))
						{
							if(val.indexOf(" ") > 0)
							{
								val = val.split(" ", QString::SkipEmptyParts, Qt::CaseInsensitive).first();
							}
							QByteArray coverData = QByteArray::fromBase64(val.toLatin1());
							QFile coverFile(QString("%1/%2.%3").arg(lamexp_temp_folder2(), lamexp_rand_str(), extension));
							if(coverFile.open(QIODevice::WriteOnly))
							{
								coverFile.write(coverData);
								coverFile.close();
								audioFile.setFileCover(coverFile.fileName(), true);
							}
							break;
						}
					}
				}
			}
		}
	}

	process.waitForFinished();
	if(process.state() != QProcess::NotRunning)
	{
		process.kill();
		process.waitForFinished(-1);
	}
}

bool FileAnalyzer::analyzeAvisynthFile(const QString &filePath, AudioFileModel &info)
{
	QProcess process;
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.setReadChannel(QProcess::StandardOutput);
	process.start(m_avs2wavBin, QStringList() << QDir::toNativeSeparators(filePath) << "?");

	if(!process.waitForStarted())
	{
		qWarning("AVS2WAV process failed to create!");
		qWarning("Error message: \"%s\"\n", process.errorString().toLatin1().constData());
		process.kill();
		process.waitForFinished(-1);
		return false;
	}

	bool bInfoHeaderFound = false;

	while(process.state() != QProcess::NotRunning)
	{
		if(m_abortFlag)
		{
			process.kill();
			qWarning("Process was aborted on user request!");
			break;
		}
		
		if(!process.waitForReadyRead())
		{
			if(process.state() == QProcess::Running)
			{
				qWarning("AVS2WAV time out. Killing process and skipping file!");
				process.kill();
				process.waitForFinished(-1);
				return false;
			}
		}

		QByteArray data;

		while(process.canReadLine())
		{
			QString line = QString::fromUtf8(process.readLine().constData()).simplified();
			if(!line.isEmpty())
			{
				int index = line.indexOf(':');
				if(index > 0)
				{
					QString key = line.left(index).trimmed();
					QString val = line.mid(index+1).trimmed();

					if(bInfoHeaderFound && !key.isEmpty() && !val.isEmpty())
					{
						if(key.compare("TotalSeconds", Qt::CaseInsensitive) == 0)
						{
							bool ok = false;
							unsigned int duration = val.toUInt(&ok);
							if(ok) info.setFileDuration(duration);
						}
						if(key.compare("SamplesPerSec", Qt::CaseInsensitive) == 0)
						{
							bool ok = false;
							unsigned int samplerate = val.toUInt(&ok);
							if(ok) info.setFormatAudioSamplerate (samplerate);
						}
						if(key.compare("Channels", Qt::CaseInsensitive) == 0)
						{
							bool ok = false;
							unsigned int channels = val.toUInt(&ok);
							if(ok) info.setFormatAudioChannels(channels);
						}
						if(key.compare("BitsPerSample", Qt::CaseInsensitive) == 0)
						{
							bool ok = false;
							unsigned int bitdepth = val.toUInt(&ok);
							if(ok) info.setFormatAudioBitdepth(bitdepth);
						}					
					}
				}
				else
				{
					if(line.contains("[Audio Info]", Qt::CaseInsensitive))
					{
						info.setFormatAudioType("Avisynth");
						info.setFormatContainerType("Avisynth");
						bInfoHeaderFound = true;
					}
				}
			}
		}
	}
	
	process.waitForFinished();
	if(process.state() != QProcess::NotRunning)
	{
		process.kill();
		process.waitForFinished(-1);
	}

	//Check exit code
	switch(process.exitCode())
	{
	case 0:
		qDebug("Avisynth script was analyzed successfully.");
		return true;
		break;
	case -5:
		qWarning("It appears that Avisynth is not installed on the system!");
		return false;
		break;
	default:
		qWarning("Failed to open the Avisynth script, bad AVS file?");
		return false;
		break;
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
