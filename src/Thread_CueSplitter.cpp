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
#include "Model_CueSheet.h"
#include "Registry_Decoder.h"
#include "Decoder_Abstract.h"

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

CueSplitter::CueSplitter(const QString &outputDir, const QString &baseName, CueSheetModel *model, const QList<AudioFileModel> &inputFilesInfo)
:
	m_model(model),
	m_outputDir(outputDir),
	m_baseName(baseName),
	m_soxBin(lamexp_lookup_tool("sox.exe"))
{
	if(m_soxBin.isEmpty())
	{
		qFatal("Invalid path to SoX binary. Tool not initialized properly.");
	}

	m_decompressedFiles.clear();
	m_tempFiles.clear();

	qDebug("\n[CueSplitter]");

	int nInputFiles = inputFilesInfo.count();
	for(int i = 0; i < nInputFiles; i++)
	{
		m_inputFilesInfo.insert(inputFilesInfo[i].filePath(), inputFilesInfo[i]);
		qDebug("File %02d: <%s>", i, inputFilesInfo[i].filePath().toUtf8().constData());
	}
	
	qDebug("All input files added.");
	m_bSuccess = false;
}

CueSplitter::~CueSplitter(void)
{
	while(!m_tempFiles.isEmpty())
	{
		lamexp_remove_file(m_tempFiles.takeFirst());
	}
}

////////////////////////////////////////////////////////////
// Thread Main
////////////////////////////////////////////////////////////

void CueSplitter::run()
{
	m_bSuccess = false;
	m_bAborted = false;
	m_abortFlag = false;
	m_nTracksSuccess = 0;
	m_nTracksSkipped = 0;
	m_decompressedFiles.clear();
	m_activeFile.clear();
	
	if(!QDir(m_outputDir).exists())
	{
		qWarning("Output directory \"%s\" does not exist!", m_outputDir.toUtf8().constData());
		return;
	}
	
	QStringList inputFileList = m_inputFilesInfo.keys();
	int nInputFiles = inputFileList.count();
	
	//Decompress all input files
	for(int i = 0; i < nInputFiles; i++)
	{
		AudioFileModel &inputFileInfo = m_inputFilesInfo[inputFileList.at(i)];
		if(inputFileInfo.formatContainerType().compare("Wave", Qt::CaseInsensitive) || inputFileInfo.formatAudioType().compare("PCM", Qt::CaseInsensitive))
		{
			AbstractDecoder *decoder = DecoderRegistry::lookup(inputFileInfo.formatContainerType(), inputFileInfo.formatContainerProfile(), inputFileInfo.formatAudioType(), inputFileInfo.formatAudioProfile(), inputFileInfo.formatAudioVersion());
			if(decoder)
			{
				m_activeFile = shortName(QFileInfo(inputFileList.at(i)).fileName());
				emit fileSelected(m_activeFile);
				QString tempFile = QString("%1/~%2.wav").arg(m_outputDir, lamexp_rand_str());
				connect(decoder, SIGNAL(statusUpdated(int)), this, SLOT(handleUpdate(int)), Qt::DirectConnection);
				if(decoder->decode(inputFileList.at(i), tempFile, &m_abortFlag))
				{
					m_decompressedFiles.insert(inputFileList.at(i), tempFile);
					m_tempFiles.append(tempFile);
				}
				else
				{
					qWarning("Failed to decompress file: <%s>", inputFileList.at(i).toLatin1().constData());
					lamexp_remove_file(tempFile);
				}
				m_activeFile.clear();
				LAMEXP_DELETE(decoder);
			}
			else
			{
				qWarning("Unsupported input file: <%s>", inputFileList.at(i).toLatin1().constData());
			}
		}
		else
		{
			m_decompressedFiles.insert(inputFileList.at(i), inputFileList.at(i));
		}
		if(m_abortFlag)
		{
			m_bAborted = true;
			qWarning("The user has requested to abort the process!");
			return;
		}
	}

	int nFiles = m_model->getFileCount();
	QString albumPerformer = m_model->getAlbumPerformer();
	QString albumTitle = m_model->getAlbumTitle();

	//Now split all files
	for(int i = 0; i < nFiles; i++)
	{
		int nTracks = m_model->getTrackCount(i);
		QString trackFile = m_model->getFileName(i);
		int maxProgress = 0;

		//Process all tracks
		for(int j = 0; j < nTracks; j++)
		{
			int trackNo = m_model->getTrackNo(i, j);
			double trackOffset = std::numeric_limits<double>::quiet_NaN();
			double trackLength = std::numeric_limits<double>::quiet_NaN();
			m_model->getTrackIndex(i, j, &trackOffset, &trackLength);
			
			if((trackNo < 0) || _isnan(trackOffset) || _isnan(trackLength))
			{
				qWarning("Failed to fetch information for track #%d of file #%d!", j, i);
				continue;
			}
			
			//Setup meta info
			AudioFileModel trackMetaInfo(QString().sprintf("cue://File%02d/Track%02d", i, j));
			trackMetaInfo.setFileName(m_model->getTrackTitle(i, j));
			trackMetaInfo.setFileArtist(m_model->getTrackPerformer(i, j));
			trackMetaInfo.setFilePosition(trackNo);

			if(!albumTitle.isEmpty())
			{
				trackMetaInfo.setFileAlbum(albumTitle);
			}
			if(!albumPerformer.isEmpty() && trackMetaInfo.fileArtist().isEmpty())
			{
				trackMetaInfo.setFileArtist(albumPerformer);
			}
			if(_finite(trackLength))
			{
				trackMetaInfo.setFileDuration(static_cast<unsigned int>(abs(trackLength)));
			}

			//Generate output file name
			QString trackTitle = trackMetaInfo.fileName().isEmpty() ? QString().sprintf("Track %02d", trackNo) : trackMetaInfo.fileName();
			QString outputFile = QString("%1/[%2] %3 - %4.wav").arg(m_outputDir, QString().sprintf("%02d", trackNo), m_baseName, trackTitle);
			for(int n = 2; QFileInfo(outputFile).exists(); n++)
			{
				outputFile = QString("%1/[%2] %3 - %4 (%5).wav").arg(m_outputDir, QString().sprintf("%02d", trackNo), m_baseName, trackTitle, QString::number(n));
			}

			//Call split function
			emit fileSelected(shortName(QFileInfo(outputFile).fileName()));
			splitFile(outputFile,  trackNo, trackFile, trackOffset, trackLength, trackMetaInfo, maxProgress);

			if(m_abortFlag)
			{
				m_bAborted = true;
				qWarning("The user has requested to abort the process!");
				return;
			}
		}
	}

	qDebug("All files were split.\n");
	m_bSuccess = true;
}

////////////////////////////////////////////////////////////
// Slots
////////////////////////////////////////////////////////////

void CueSplitter::handleUpdate(int progress)
{
	emit fileSelected(QString("%1 [%2%]").arg(m_activeFile, QString::number(progress)));
}

////////////////////////////////////////////////////////////
// Privtae Functions
////////////////////////////////////////////////////////////

void CueSplitter::splitFile(const QString &output, const int trackNo, const QString &file, const double offset, const double length, const AudioFileModel &metaInfo, int &maxProgress)
{
	qDebug("[Track %02d]", trackNo);
	qDebug("File: <%s>", file.toUtf8().constData());
	qDebug("Offset: <%f> <%s>", offset, indexToString(offset).toLatin1().constData());
	qDebug("Length: <%f> <%s>", length, indexToString(length).toLatin1().constData());
	qDebug("Artist: <%s>", metaInfo.fileArtist().toUtf8().constData());
	qDebug("Title: <%s>", metaInfo.fileName().toUtf8().constData());
	
	if(!m_decompressedFiles.contains(file))
	{
		qWarning("Unknown or unsupported input file, skipping!");
		m_nTracksSkipped++;
		return;
	}

	QString baseName = shortName(QFileInfo(output).fileName());
	QString decompressedInput = m_decompressedFiles[file];
	qDebug("Input: <%s>", decompressedInput.toUtf8().constData());
	
	emit fileSelected(QString("%1 [%2%]").arg(baseName, QString::number(maxProgress)));

	AudioFileModel outFileInfo(metaInfo);
	outFileInfo.setFilePath(output);
	outFileInfo.setFormatContainerType("Wave");
	outFileInfo.setFormatAudioType("PCM");

	QStringList args;
	args << "-S" << "-V3";
	args << "--guard" << "--temp" << ".";
	args << QDir::toNativeSeparators(decompressedInput);
	args << QDir::toNativeSeparators(output);
	
	//Add trim parameters, if needed
	if(_finite(offset))
	{
		args << "trim";
		args << indexToString(offset);
		
		if(_finite(length))
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
			qWarning("Process was aborted on user request!");
			break;
		}
		process.waitForReadyRead(m_processTimeoutInterval);
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
					maxProgress = max(maxProgress, progress);
					emit fileSelected(QString("%1 [%2%]").arg(baseName, QString::number(maxProgress)));
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
						int duration = intputLen - static_cast<int>(floor(offset + 0.5));
						if(duration < 0) qWarning("Track is out of bounds: Track offset exceeds input file duration!");
						outFileInfo.setFileDuration(max(0, duration));
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
	if(!_finite(index) || (index < 0.0) || (index > 86400.0))
	{
		return QString();
	}
	
	QTime time = QTime().addMSecs(static_cast<int>(floor(0.5 + (index * 1000.0))));
	return time.toString(time.hour() ? "H:mm:ss.zzz" : "m:ss.zzz");
}

QString CueSplitter::shortName(const QString &longName) const
{
	static const int maxLen = 54;
	
	if(longName.length() > maxLen)
	{
		return QString("%1...%2").arg(longName.left(maxLen/2).trimmed(), longName.right(maxLen/2).trimmed());
	}

	return longName;
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/
