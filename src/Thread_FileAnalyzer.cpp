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

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QDate>
#include <QTime>
#include <QDebug>
#include <QMessageBox>

#include <math.h>

//Un-escape XML characters
#define XML_DECODE replace("&amp;", "&").replace("&apos;", "'").replace("&nbsp;", " ").replace("&quot;", "\"").replace("&lt;", "<").replace("&gt;", ">")

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

FileAnalyzer::FileAnalyzer(const QStringList &inputFiles)
:
	m_inputFiles(inputFiles),
	m_mediaInfoBin_x86(lamexp_lookup_tool("mediainfo_i386.exe")),
	m_mediaInfoBin_x64(lamexp_lookup_tool("mediainfo_x64.exe"))
{
	m_bSuccess = false;
	
	if(m_mediaInfoBin_x86.isEmpty() || m_mediaInfoBin_x64.isEmpty())
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
			if(!importPlaylist(m_inputFiles, currentFile))
			{
				m_filesRejected++;
				qDebug("Skipped: %s", file.filePath().toUtf8().constData());
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

const AudioFileModel FileAnalyzer::analyzeFile(const QString &filePath)
{
	lamexp_cpu_t cpuInfo = lamexp_detect_cpu_features();
	const QString mediaInfoBin = cpuInfo.x64 ? m_mediaInfoBin_x64 : m_mediaInfoBin_x86;
	
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
	process.start(mediaInfoBin, QStringList() << QDir::toNativeSeparators(filePath));
	
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


bool FileAnalyzer::importPlaylist(QStringList &fileList, const QString &playlistFile)
{
	QFileInfo file(playlistFile);
	QDir baseDir(file.canonicalPath());

	QDir rootDir(baseDir);
	while(rootDir.cdUp());

	//Sanity check
	if(file.size() < 3 || file.size() > 512000)
	{
		return false;
	}
	
	//Detect playlist type
	playlist_t playlistType = isPlaylist(file.canonicalFilePath());

	//Exit if not a playlist
	if(playlistType == noPlaylist)
	{
		return false;
	}
	
	QFile data(playlistFile);

	//Open file for reading
	if(!data.open(QIODevice::ReadOnly))
	{
		return false;
	}

	//Parse playlist depending on type
	switch(playlistType)
	{
	case m3uPlaylist:
		return parsePlaylist_m3u(data, fileList, baseDir, rootDir);
		break;
	case plsPlaylist:
		return parsePlaylist_pls(data, fileList, baseDir, rootDir);
		break;
	case wplPlaylist:
		return parsePlaylist_wpl(data, fileList, baseDir, rootDir);
		break;
	default:
		return false;
		break;
	}
}

bool FileAnalyzer::parsePlaylist_m3u(QFile &data, QStringList &fileList, const QDir &baseDir, const QDir &rootDir)
{
	QByteArray line = data.readLine();
	
	while(line.size() > 0)
	{
		QFileInfo filename1(QDir::fromNativeSeparators(QString::fromUtf8(line.constData(), line.size()).trimmed()));
		QFileInfo filename2(QDir::fromNativeSeparators(QString::fromLatin1(line.constData(), line.size()).trimmed()));

		filename1.setCaching(false);
		filename2.setCaching(false);

		if(!(filename1.filePath().startsWith("#") || filename2.filePath().startsWith("#")))
		{
			fixFilePath(filename1, baseDir, rootDir);
			fixFilePath(filename2, baseDir, rootDir);

			if(filename1.exists())
			{
				if(isPlaylist(filename1.canonicalFilePath()) == noPlaylist)
				{
					fileList << filename1.canonicalFilePath();
				}
			}
			else if(filename2.exists())
			{
				if(isPlaylist(filename2.canonicalFilePath()) == noPlaylist)
				{
					fileList << filename2.canonicalFilePath();
				}
			}
		}

		line = data.readLine();
	}

	return true;
}

bool FileAnalyzer::parsePlaylist_pls(QFile &data, QStringList &fileList, const QDir &baseDir, const QDir &rootDir)
{
	QRegExp plsEntry("File(\\d+)=(.+)", Qt::CaseInsensitive);
	QByteArray line = data.readLine();
	
	while(line.size() > 0)
	{
		bool flag = false;
		
		QString temp1(QDir::fromNativeSeparators(QString::fromUtf8(line.constData(), line.size()).trimmed()));
		QString temp2(QDir::fromNativeSeparators(QString::fromLatin1(line.constData(), line.size()).trimmed()));

		if(!flag && plsEntry.indexIn(temp1) >= 0)
		{
			QFileInfo filename(QDir::fromNativeSeparators(plsEntry.cap(2)).trimmed());
			filename.setCaching(false);
			fixFilePath(filename, baseDir, rootDir);

			if(filename.exists())
			{
				if(isPlaylist(filename.canonicalFilePath()) == noPlaylist)
				{
					fileList << filename.canonicalFilePath();
					flag = true;
				}
			}
		}
		
		if(!flag && plsEntry.indexIn(temp2) >= 0)
		{
			QFileInfo filename(QDir::fromNativeSeparators(plsEntry.cap(2)).trimmed());
			filename.setCaching(false);
			fixFilePath(filename, baseDir, rootDir);

			if(filename.exists())
			{
				if(isPlaylist(filename.canonicalFilePath()) == noPlaylist)
				{
					fileList << filename.canonicalFilePath();
					flag = true;
				}
			}
		}

		line = data.readLine();
	}

	return true;
}

bool FileAnalyzer::parsePlaylist_wpl(QFile &data, QStringList &fileList, const QDir &baseDir, const QDir &rootDir)
{
	QRegExp skipData("<!--(.+)-->", Qt::CaseInsensitive);
	QRegExp wplEntry("<(media|ref)[^<>]*(src|href)=\"([^\"]+)\"[^<>]*>", Qt::CaseInsensitive);
	
	skipData.setMinimal(true);

	QByteArray buffer = data.readAll();
	QString line = QString::fromUtf8(buffer.constData(), buffer.size()).simplified();
	buffer.clear();

	int index = 0;

	while((index = skipData.indexIn(line)) >= 0)
	{
		line.remove(index, skipData.matchedLength());
	}

	int offset = 0;

	while((offset = wplEntry.indexIn(line, offset) + 1) > 0)
	{
		QFileInfo filename(QDir::fromNativeSeparators(wplEntry.cap(3).XML_DECODE.trimmed()));
		filename.setCaching(false);
		fixFilePath(filename, baseDir, rootDir);

		if(filename.exists())
		{
			if(isPlaylist(filename.canonicalFilePath()) == noPlaylist)
			{
				fileList << filename.canonicalFilePath();
			}
		}
	}

	return true;
}

FileAnalyzer::playlist_t FileAnalyzer::isPlaylist(const QString &fileName)
{
	QFileInfo file (fileName);
	
	if(file.suffix().compare("m3u", Qt::CaseInsensitive) == 0)
	{
		return m3uPlaylist;
	}
	else if(file.suffix().compare("m3u8", Qt::CaseInsensitive) == 0)
	{
		return m3uPlaylist;
	}
	else if(file.suffix().compare("pls", Qt::CaseInsensitive) == 0)
	{
		return  plsPlaylist;
	}
	else if(file.suffix().compare("asx", Qt::CaseInsensitive) == 0)
	{
		return  wplPlaylist;
	}
	else if(file.suffix().compare("wpl", Qt::CaseInsensitive) == 0)
	{
		return  wplPlaylist;
	}
	else
	{
		return noPlaylist;
	}
}

void FileAnalyzer::fixFilePath(QFileInfo &filename, const QDir &baseDir, const QDir &rootDir)
{
	if(filename.filePath().startsWith("/"))
	{
		while(filename.filePath().startsWith("/"))
		{
			filename.setFile(filename.filePath().mid(1));
		}
		filename.setFile(rootDir.filePath(filename.filePath()));
	}
	
	if(!filename.isAbsolute())
	{
		filename.setFile(baseDir.filePath(filename.filePath()));
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
