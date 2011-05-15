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

#include "Thread_CueSplitter.h"

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

CueSplitter::CueSplitter(const QString &outputDir, const QString &baseName, const QList<AudioFileModel> &inputFiles)
:
	m_outputDir(outputDir),
	m_baseName(baseName),
	m_soxBin(lamexp_lookup_tool("sox.exe"))
{
	if(m_soxBin.isEmpty())
	{
		qFatal("Invalid path to SoX binary. Tool not initialized properly.");
	}

	m_albumPerformer.clear();
	m_albumTitle.clear();

	qDebug("\n[CueSplitter::CueSplitter]");

	int nInputFiles = inputFiles.count();
	for(int i = 0; i < nInputFiles; i++)
	{
		m_inputFiles.insert(inputFiles[i].filePath(), inputFiles[i]);
		qDebug("%02d <%s>", i, inputFiles[i].filePath());
	}
	
	qDebug("All input files added.");
	m_bSuccess = false;
}

////////////////////////////////////////////////////////////
// Thread Main
////////////////////////////////////////////////////////////

void CueSplitter::run()
{
	m_bSuccess = false;
	m_nTracksSuccess = 0;
	m_nTracksSkipped = 0;
	m_abortFlag = false;

	int nTracks =  min(min(min(m_trackFile.count(), m_trackNo.count()), min(m_trackOffset.count(), m_trackLength.count())), m_trackMetaInfo.count());
	
	if(!QDir(m_outputDir).exists())
	{
		qWarning("Output directory \"%s\" does not exist!", m_outputDir.toUtf8().constData());
		return;
	}

	for(int i = 0; i < nTracks; i++)
	{
		QString outputFile = QString("%1/%2 - Track %3.wav").arg(m_outputDir, m_baseName, QString().sprintf("%02d", m_trackNo.at(i)));
		for(int n = 2; QFileInfo(outputFile).exists(); n++)
		{
			outputFile = QString("%1/%2 - Track %3 (%4).wav").arg(m_outputDir, m_baseName, QString().sprintf("%02d", m_trackNo.at(i)), QString::number(n));
		}

		emit fileSelected(QFileInfo(outputFile).fileName());
		splitFile(outputFile,  m_trackNo.at(i), m_trackFile.at(i), m_trackOffset.at(i), m_trackLength.at(i), m_trackMetaInfo.at(i));
	}

	qDebug("All files were split.\n");
	m_bSuccess = true;
}

void CueSplitter::addTrack(const int trackNo, const QString &file, const double offset, const double length, const AudioFileModel &metaInfo)
{
	
	if(m_inputFiles.contains(file))
	{
		m_trackFile << file;
		m_trackNo << trackNo;
		m_trackOffset << offset;
		m_trackLength << length;
		m_trackMetaInfo << metaInfo;
	}
	else
	{
		throw "Unknown input file!";
	}
}

void CueSplitter::setAlbumInfo(const QString &performer, const QString &title)
{
	if(!performer.isEmpty()) m_albumPerformer = performer;
	if(!title.isEmpty()) m_albumTitle = title;
}

////////////////////////////////////////////////////////////
// Privtae Functions
////////////////////////////////////////////////////////////

void CueSplitter::splitFile(const QString &output, const int trackNo, const QString &file, const double offset, const double length, const AudioFileModel &metaInfo)
{
	qDebug("\n[Track %02d]", trackNo);
	qDebug("File: <%s>", file.toUtf8().constData());
	qDebug("Offset: %f", offset);
	qDebug("Length: %f", length);
	qDebug("Artist: <%s>", metaInfo.fileArtist().toUtf8().constData());
	qDebug("Title: <%s>", metaInfo.fileName().toUtf8().constData());

	QString baseName = QFileInfo(output).fileName();

	if(!m_inputFiles.contains(file))
	{
		qWarning("Unknown input file, skipping!");
		m_nTracksSkipped++;
		return;
	}

	AudioFileModel &inputFileInfo = m_inputFiles[file];
	if(inputFileInfo.formatContainerType().compare("Wave", Qt::CaseInsensitive) || inputFileInfo.formatAudioType().compare("PCM", Qt::CaseInsensitive))
	{
		qWarning("Sorry, only Wave/PCM files are supported at this time!");
		m_nTracksSkipped++;
		return;
	}

	AudioFileModel outFileInfo(metaInfo);
	outFileInfo.setFilePath(output);
	outFileInfo.setFormatContainerType(inputFileInfo.formatContainerType());
	outFileInfo.setFormatAudioType(inputFileInfo.formatAudioType());
	
	if(length != std::numeric_limits<double>::infinity())
	{
		outFileInfo.setFileDuration(static_cast<unsigned int>(abs(length)));
	}
	if(!m_albumTitle.isEmpty())
	{
		outFileInfo.setFileAlbum(m_albumTitle);
	}
	if(!m_albumPerformer.isEmpty() && outFileInfo.fileArtist().isEmpty())
	{
		outFileInfo.setFileArtist(m_albumPerformer);
	}

	QStringList args;
	args << "-S" << "-V3";
	args << "--guard" << "--temp" << ".";
	args << QDir::toNativeSeparators(file);
	args << QDir::toNativeSeparators(output);
	
	if(offset != 0.0 || length != std::numeric_limits<double>::infinity())
	{
		args << "trim";
		args << indexToString(offset);
	
		if((length != std::numeric_limits<double>::quiet_NaN()) && (length != std::numeric_limits<double>::infinity()))
		{
			args << indexToString(length);
		}
	}

	QRegExp rxProgress("In:(\\d+)(\\.\\d+)*%", Qt::CaseInsensitive);
	QRegExp rxChannels("Channels\\s*:\\s*(\\d+)", Qt::CaseInsensitive);
	QRegExp rxSamplerate("Sample Rate\\s*:\\s*(\\d+)", Qt::CaseInsensitive);
	QRegExp rxPrecision("Precision\\s*:\\s*(\\d+)-bit", Qt::CaseInsensitive);
	QRegExp rxDuration("Duration\\s*:\\s*(\\d\\d):(\\d\\d):(\\d\\d).(\\d\\d)", Qt::CaseInsensitive);

	QProcess process;
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.setReadChannel(QProcess::StandardOutput);
	process.setWorkingDirectory(m_outputDir);
	process.start(m_soxBin, args);
		
	if(!process.waitForStarted())
	{
		qWarning("SoX process failed to create!");
		qWarning("Error message: \"%s\"\n", process.errorString().toLatin1().constData());
		process.kill();
		process.waitForFinished(-1);
		m_nTracksSkipped++;
		return;
	}

	while(process.state() != QProcess::NotRunning)
	{
		if(m_abortFlag)
		{
			process.kill();
			break;
		}
		process.waitForReadyRead();
		if(!process.bytesAvailable() && process.state() == QProcess::Running)
		{
			process.kill();
			qWarning("SoX process timed out <-- killing!");
			break;
		}
		while(process.bytesAvailable() > 0)
		{
			QByteArray line = process.readLine();
			QString text = QString::fromUtf8(line.constData()).simplified();
			if(rxProgress.lastIndexIn(text) >= 0)
			{
				bool ok = false;
				int progress = rxProgress.cap(1).toInt(&ok);
				if(ok)
				{
					emit fileSelected(QString("%1 [%2%]").arg(baseName, QString::number(progress)));
				}
			}
			else if(rxChannels.lastIndexIn(text) >= 0)
			{
				bool ok = false;
				unsigned int channels = rxChannels.cap(1).toUInt(&ok);
				if(ok) outFileInfo.setFormatAudioChannels(channels);
			}
			else if(rxSamplerate.lastIndexIn(text) >= 0)
			{
				bool ok = false;
				unsigned int samplerate = rxSamplerate.cap(1).toUInt(&ok);
				if(ok) outFileInfo.setFormatAudioSamplerate(samplerate);
			}
			else if(rxPrecision.lastIndexIn(text) >= 0)
			{
				bool ok = false;
				unsigned int precision = rxPrecision.cap(1).toUInt(&ok);
				if(ok) outFileInfo.setFormatAudioBitdepth(precision);
			}
			else if(rxDuration.lastIndexIn(text) >= 0)
			{
				bool ok1 = false, ok2 = false, ok3 = false;
				unsigned int hh = rxDuration.cap(1).toUInt(&ok1);
				unsigned int mm = rxDuration.cap(2).toUInt(&ok2);
				unsigned int ss = rxDuration.cap(3).toUInt(&ok3);
				if(ok1 && ok2 && ok3)
				{
					unsigned intputLen = (hh * 3600) + (mm * 60) + ss;
					if(length == std::numeric_limits<double>::infinity())
					{
						qDebug("Duration updated from SoX info!");
						outFileInfo.setFileDuration(max(0, intputLen - static_cast<unsigned int>(floor(offset + 0.5))));
					}
					else
					{
						unsigned int trackEnd = static_cast<unsigned int>(floor(offset + 0.5)) + static_cast<unsigned int>(floor(length + 0.5));
						if(trackEnd > intputLen) qWarning("Track is out of bounds: End of track exceeds input file duration!");
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

	if(process.exitStatus() != QProcess::NormalExit || QFileInfo(output).size() == 0)
	{
		qWarning("Splitting has failed !!!");
		m_nTracksSkipped++;
		return;
	}

	emit fileSplit(outFileInfo);
	m_nTracksSuccess++;
}

QString CueSplitter::indexToString(const double index) const
{
	if(index == std::numeric_limits<double>::quiet_NaN() || index == std::numeric_limits<double>::infinity() || index < 0.0)
	{
		return QString();
	}

	int temp = static_cast<int>(floor(0.5 + (index * 1000.0)));

	int msec = temp % 1000;
	int secs = temp / 1000;

	if(secs >= 3600)
	{
		return QString().sprintf("%d:%02d:%02d.%03d", min(99, secs / 3600), min(59, (secs % 3600) / 60), min(59, (secs % 3600) % 60), min(99, msec));
	}
	else if(secs >= 60)
	{
		return QString().sprintf("%d:%02d.%03d", min(99, secs / 60), min(59, secs % 60), min(99, msec));
	}
	else
	{
		return QString().sprintf("%d.%03d", min(59, secs % 60), min(99, msec));
	}
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/
