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

#include "Thread_FileAnalyzer_Task.h"

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
#include <QImage>
#include <QReadLocker>
#include <QWriteLocker>
#include <QThread>

#include <math.h>
#include <time.h>

#define IS_KEY(KEY) (key.compare(KEY, Qt::CaseInsensitive) == 0)
#define IS_SEC(SEC) (key.startsWith((SEC "_"), Qt::CaseInsensitive))
#define FIRST_TOK(STR) (STR.split(" ", QString::SkipEmptyParts).first())

/* static vars */
QReadWriteLock AnalyzeTask::s_lock;
unsigned __int64 AnalyzeTask::s_threadIdx_created;
unsigned __int64 AnalyzeTask::s_threadIdx_finished;
unsigned int AnalyzeTask::s_filesAccepted;
unsigned int AnalyzeTask::s_filesRejected;
unsigned int AnalyzeTask::s_filesDenied;
unsigned int AnalyzeTask::s_filesDummyCDDA;
unsigned int AnalyzeTask::s_filesCueSheet;
QStringList AnalyzeTask::s_additionalFiles;
QStringList AnalyzeTask::s_recentlyAdded;

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

AnalyzeTask::AnalyzeTask(const QString &inputFile, const QString &templateFile, volatile bool *abortFlag)
:
	m_threadIdx(makeThreadIdx()),
	m_inputFile(inputFile),
	m_templateFile(templateFile),
	m_mediaInfoBin(lamexp_lookup_tool("mediainfo.exe")),
	m_avs2wavBin(lamexp_lookup_tool("avs2wav.exe")),
	m_abortFlag(abortFlag)
{
	if(m_mediaInfoBin.isEmpty() || m_avs2wavBin.isEmpty())
	{
		qFatal("Invalid path to MediaInfo binary. Tool not initialized properly.");
	}
}

AnalyzeTask::~AnalyzeTask(void)
{
	QWriteLocker lock(&s_lock);
	s_threadIdx_finished = qMax(s_threadIdx_finished, m_threadIdx + 1);
}

////////////////////////////////////////////////////////////
// Thread Main
////////////////////////////////////////////////////////////

void AnalyzeTask::run(void)
{
	int fileType = fileTypeNormal;
	QString currentFile = QDir::fromNativeSeparators(m_inputFile);
	qDebug("Analyzing: %s", currentFile.toUtf8().constData());
	emit fileSelected(QFileInfo(currentFile).fileName());
	AudioFileModel file = analyzeFile(currentFile, &fileType);
		
	if(*m_abortFlag)
	{
		qWarning("Operation cancelled by user!");
		return;
	}
	if(fileType == fileTypeSkip)
	{
		qWarning("File was recently added, skipping!");
		return;
	}
	if(fileType == fileTypeDenied)
	{
		QWriteLocker lock(&s_lock);
		s_filesDenied++;
		qWarning("Cannot access file for reading, skipping!");
		return;
	}
	if(fileType == fileTypeCDDA)
	{
		QWriteLocker lock(&s_lock);
		s_filesDummyCDDA++;
		qWarning("Dummy CDDA file detected, skipping!");
		return;
	}
		
	if(file.fileName().isEmpty() || file.formatContainerType().isEmpty() || file.formatAudioType().isEmpty())
	{
		QStringList fileList;
		if(PlaylistImporter::importPlaylist(fileList, currentFile))
		{
			QWriteLocker lock(&s_lock);
			qDebug("Imported playlist file.");
			s_additionalFiles << fileList;
		}
		else if(!QFileInfo(currentFile).suffix().compare("cue", Qt::CaseInsensitive))
		{
			QWriteLocker lock(&s_lock);
			qWarning("Cue Sheet file detected, skipping!");
			s_filesCueSheet++;
		}
		else if(!QFileInfo(currentFile).suffix().compare("avs", Qt::CaseInsensitive))
		{
			qDebug("Found a potential Avisynth script, investigating...");
			if(analyzeAvisynthFile(currentFile, file))
			{
				QWriteLocker lock(&s_lock);
				s_filesAccepted++;
				s_recentlyAdded.append(file.filePath());
				lock.unlock();
				waitForPreviousThreads();
				emit fileAnalyzed(file);
			}
			else
			{
				QWriteLocker lock(&s_lock);
				qDebug("Rejected Avisynth file: %s", file.filePath().toUtf8().constData());
				s_filesRejected++;
			}
		}
		else
		{
			QWriteLocker lock(&s_lock);
			qDebug("Rejected file of unknown type: %s", file.filePath().toUtf8().constData());
			s_filesRejected++;
		}
		return;
	}

	QWriteLocker lock(&s_lock);
	s_filesAccepted++;
	s_recentlyAdded.append(file.filePath());
	lock.unlock();

	waitForPreviousThreads();
	emit fileAnalyzed(file);
}

////////////////////////////////////////////////////////////
// Privtae Functions
////////////////////////////////////////////////////////////

const AudioFileModel AnalyzeTask::analyzeFile(const QString &filePath, int *type)
{
	*type = fileTypeNormal;
	AudioFileModel audioFile(filePath);

	QReadLocker readLock(&s_lock);
	if(s_recentlyAdded.contains(filePath, Qt::CaseInsensitive))
	{
		*type = fileTypeSkip;
		return audioFile;
	}
	readLock.unlock();

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

	bool skipNext = false;
	unsigned int id_val[2] = {UINT_MAX, UINT_MAX};
	cover_t coverType = coverNone;
	QByteArray coverData;

	QStringList params;
	params << QString("--Inform=file://%1").arg(QDir::toNativeSeparators(m_templateFile));
	params << QDir::toNativeSeparators(filePath);
	
	QProcess process;
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.setReadChannel(QProcess::StandardOutput);
	process.start(m_mediaInfoBin, params);
		
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
		if(*m_abortFlag)
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
				//qDebug("Line:%s", line.toUtf8().constData());
				
				int index = line.indexOf('=');
				if(index > 0)
				{
					QString key = line.left(index).trimmed();
					QString val = line.mid(index+1).trimmed();
					if(!key.isEmpty())
					{
						updateInfo(audioFile, &skipNext, id_val, &coverType, &coverData, key, val);
					}
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

	if((coverType != coverNone) && (!coverData.isEmpty()))
	{
		retrieveCover(audioFile, coverType, coverData);
	}

	return audioFile;
}

void AnalyzeTask::updateInfo(AudioFileModel &audioFile, bool *skipNext, unsigned int *id_val, cover_t *coverType, QByteArray *coverData, const QString &key, const QString &value)
{
	//qWarning("'%s' -> '%s'", key.toUtf8().constData(), value.toUtf8().constData());
	
	/*New Stream*/
	if(IS_KEY("Gen_ID") || IS_KEY("Aud_ID"))
	{
		if(value.isEmpty())
		{
			*skipNext = false;
		}
		else
		{
			//We ignore all ID's, except for the lowest one!
			bool ok = false;
			unsigned int id = value.toUInt(&ok);
			if(ok)
			{
				if(IS_KEY("Gen_ID")) { id_val[0] = qMin(id_val[0], id); *skipNext = (id > id_val[0]); }
				if(IS_KEY("Aud_ID")) { id_val[1] = qMin(id_val[1], id); *skipNext = (id > id_val[1]); }
			}
			else
			{
				*skipNext = true;
			}
		}
		if(*skipNext)
		{
			qWarning("Skipping info for non-primary stream!");
		}
		return;
	}

	/*Skip or empty?*/
	if((*skipNext) || value.isEmpty())
	{
		return;
	}

	/*Playlist file?*/
	if(IS_KEY("Aud_Source"))
	{
		*skipNext = true;
		audioFile.setFormatContainerType(QString());
		audioFile.setFormatAudioType(QString());
		qWarning("Skipping info for playlist file!");
		return;
	}

	/*General Section*/
	if(IS_SEC("Gen"))
	{
		if(IS_KEY("Gen_Format"))
		{
			audioFile.setFormatContainerType(value);
		}
		else if(IS_KEY("Gen_Format_Profile"))
		{
			audioFile.setFormatContainerProfile(value);
		}
		else if(IS_KEY("Gen_Title") || IS_KEY("Gen_Track"))
		{
			audioFile.setFileName(value);
		}
		else if(IS_KEY("Gen_Duration"))
		{
			unsigned int tmp = parseDuration(value);
			if(tmp > 0) audioFile.setFileDuration(tmp);
		}
		else if(IS_KEY("Gen_Artist") || IS_KEY("Gen_Performer"))
		{
			audioFile.setFileArtist(value);
		}
		else if(IS_KEY("Gen_Album"))
		{
			audioFile.setFileAlbum(value);
		}
		else if(IS_KEY("Gen_Genre"))
		{
			audioFile.setFileGenre(value);
		}
		else if(IS_KEY("Gen_Released_Date") || IS_KEY("Gen_Recorded_Date"))
		{
			unsigned int tmp = parseYear(value);
			if(tmp > 0) audioFile.setFileYear(tmp);
		}
		else if(IS_KEY("Gen_Comment"))
		{
			audioFile.setFileComment(value);
		}
		else if(IS_KEY("Gen_Track/Position"))
		{
			bool ok = false;
			unsigned int tmp = value.toUInt(&ok);
			if(ok) audioFile.setFilePosition(tmp);
		}
		else if(IS_KEY("Gen_Cover") || IS_KEY("Gen_Cover_Type"))
		{
			if(*coverType == coverNone)
			{
				*coverType = coverJpeg;
			}
		}
		else if(IS_KEY("Gen_Cover_Mime"))
		{
			QString temp = FIRST_TOK(value);
			if(!temp.compare("image/jpeg", Qt::CaseInsensitive)) *coverType = coverJpeg;
			else if(!temp.compare("image/png", Qt::CaseInsensitive)) *coverType = coverPng;
			else if(!temp.compare("image/gif", Qt::CaseInsensitive)) *coverType = coverGif;
		}
		else if(IS_KEY("Gen_Cover_Data"))
		{
			if(!coverData->isEmpty()) coverData->clear();
			coverData->append(QByteArray::fromBase64(FIRST_TOK(value).toLatin1()));
		}
		else
		{
			qWarning("Unknown key '%s' with value '%s' found!", key.toUtf8().constData(), value.toUtf8().constData());
		}
		return;
	}

	/*Audio Section*/
	if(IS_SEC("Aud"))
	{

		if(IS_KEY("Aud_Format"))
		{
			audioFile.setFormatAudioType(value);
		}
		else if(IS_KEY("Aud_Format_Profile"))
		{
			audioFile.setFormatAudioProfile(value);
		}
		else if(IS_KEY("Aud_Format_Version"))
		{
			audioFile.setFormatAudioVersion(value);
		}
		else if(IS_KEY("Aud_Channel(s)"))
		{
			bool ok = false;
			unsigned int tmp = value.toUInt(&ok);
			if(ok) audioFile.setFormatAudioChannels(tmp);
		}
		else if(IS_KEY("Aud_SamplingRate"))
		{
			bool ok = false;
			unsigned int tmp = value.toUInt(&ok);
			if(ok) audioFile.setFormatAudioSamplerate(tmp);
		}
		else if(IS_KEY("Aud_BitDepth"))
		{
			bool ok = false;
			unsigned int tmp = value.toUInt(&ok);
			if(ok) audioFile.setFormatAudioBitdepth(tmp);
		}
		else if(IS_KEY("Aud_Duration"))
		{
			unsigned int tmp = parseDuration(value);
			if(tmp > 0) audioFile.setFileDuration(tmp);
		}
		else if(IS_KEY("Aud_BitRate"))
		{
			bool ok = false;
			unsigned int tmp = value.toUInt(&ok);
			if(ok) audioFile.setFormatAudioBitrate(tmp/1000);
		}
		else if(IS_KEY("Aud_BitRate_Mode"))
		{
			if(!value.compare("CBR", Qt::CaseInsensitive)) audioFile.setFormatAudioBitrateMode(AudioFileModel::BitrateModeConstant);
			if(!value.compare("VBR", Qt::CaseInsensitive)) audioFile.setFormatAudioBitrateMode(AudioFileModel::BitrateModeVariable);
		}
		else
		{
			qWarning("Unknown key '%s' with value '%s' found!", key.toUtf8().constData(), value.toUtf8().constData());
		}
		return;
	}

	/*Section not recognized*/
	qWarning("Unknown section: %s", key.toUtf8().constData());
}

bool AnalyzeTask::checkFile_CDDA(QFile &file)
{
	file.reset();
	QByteArray data = file.read(128);
	
	int i = data.indexOf("RIFF");
	int j = data.indexOf("CDDA");
	int k = data.indexOf("fmt ");

	return ((i >= 0) && (j >= 0) && (k >= 0) && (k > j) && (j > i));
}

void AnalyzeTask::retrieveCover(AudioFileModel &audioFile, cover_t coverType, const QByteArray &coverData)
{
	qDebug("Retrieving cover!");
	QString extension;

	switch(coverType)
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
	
	if(!(QImage::fromData(coverData, extension.toUpper().toLatin1().constData()).isNull()))
	{
		QFile coverFile(QString("%1/%2.%3").arg(lamexp_temp_folder2(), lamexp_rand_str(), extension));
		if(coverFile.open(QIODevice::WriteOnly))
		{
			coverFile.write(coverData);
			coverFile.close();
			audioFile.setFileCover(coverFile.fileName(), true);
		}
	}
	else
	{
		qWarning("Image data seems to be invalid :-(");
	}
}

bool AnalyzeTask::analyzeAvisynthFile(const QString &filePath, AudioFileModel &info)
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
		if(*m_abortFlag)
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

unsigned int AnalyzeTask::parseYear(const QString &str)
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

unsigned int AnalyzeTask::parseDuration(const QString &str)
{
	bool ok = false;
	unsigned int value = str.toUInt(&ok);
	return ok ? (value/1000) : 0;
}

unsigned __int64 AnalyzeTask::makeThreadIdx(void)
{
	QWriteLocker lock(&s_lock);
	return s_threadIdx_created++;
}

void AnalyzeTask::waitForPreviousThreads(void)
{
	for(int i = 0; i < 240; i++) 
	{
		QReadLocker lock(&s_lock);
		if((s_threadIdx_finished >= m_threadIdx) || *m_abortFlag)
		{
			break;
		}
		lock.unlock();
		Sleep(125);
	}
}

////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////

unsigned int AnalyzeTask::filesAccepted(void)
{
	QReadLocker lock(&s_lock);
	return s_filesAccepted;
}

unsigned int AnalyzeTask::filesRejected(void)
{
	QReadLocker lock(&s_lock);
	return s_filesRejected;
}

unsigned int AnalyzeTask::filesDenied(void)
{
	QReadLocker lock(&s_lock);
	return s_filesDenied;
}

unsigned int AnalyzeTask::filesDummyCDDA(void)
{
	QReadLocker lock(&s_lock);
	return s_filesDummyCDDA;
}

unsigned int AnalyzeTask::filesCueSheet(void)
{
	QReadLocker lock(&s_lock);
	return s_filesCueSheet;
}

void AnalyzeTask::getAdditionalFiles(QStringList &fileList)
{
	QReadLocker lock(&s_lock);
	fileList << s_additionalFiles;
	s_additionalFiles.clear();
}

void AnalyzeTask::reset(void)
{
	QWriteLocker lock(&s_lock);

	s_threadIdx_created = 0;
	s_threadIdx_finished = 0;
	s_filesAccepted = 0;
	s_filesRejected = 0;
	s_filesDenied = 0;
	s_filesDummyCDDA = 0;
	s_filesCueSheet = 0;
	s_additionalFiles.clear();
	s_recentlyAdded.clear();
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/
