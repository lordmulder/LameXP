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

#include "Encoder_MP3.h"

#include "Global.h"
#include "Model_Settings.h"

#include <QProcess>
#include <QDir>
#include <limits.h>

static const int g_lameAgorithmQualityLUT[] = {9, 7, 5, 2, 0, INT_MAX};

MP3Encoder::MP3Encoder(void)
:
	m_binary(lamexp_lookup_tool("lame.exe"))
{
	if(m_binary.isEmpty())
	{
		throw "Error initializing MP3 encoder. Tool 'lame.exe' is not registred!";
	}
	
	m_algorithmQuality = 3;
	m_configBitrateMaximum = 0;
	m_configBitrateMinimum = 0;
	m_configSamplingRate = 0;
	m_configChannelMode = 0;
}

MP3Encoder::~MP3Encoder(void)
{
}

bool MP3Encoder::encode(const QString &sourceFile, const AudioFileModel &metaInfo, const QString &outputFile, volatile bool *abortFlag)
{
	QProcess process;
	QStringList args;

	args << "--nohist";
	args << "-q" << QString::number(g_lameAgorithmQualityLUT[m_algorithmQuality]);
		
	switch(m_configRCMode)
	{
	case SettingsModel::VBRMode:
		args << "-V" << QString::number(9 - qMin(9, m_configBitrate));
		break;
	case SettingsModel::ABRMode:
		args << "--abr" << QString::number(SettingsModel::mp3Bitrates[qMax(0, qMin(13, m_configBitrate))]);
		break;
	case SettingsModel::CBRMode:
		args << "--cbr";
		args << "-b" << QString::number(SettingsModel::mp3Bitrates[qMax(0, qMin(13, m_configBitrate))]);
		break;
	default:
		throw "Bad rate-control mode!";
		break;
	}

	if((m_configBitrateMaximum > 0) && (m_configBitrateMinimum > 0) && (m_configBitrateMinimum <= m_configBitrateMaximum))
	{
		if(m_configRCMode != SettingsModel::CBRMode)
		{
			args << "-b" << QString::number(clipBitrate(m_configBitrateMinimum));
			args << "-B" << QString::number(clipBitrate(m_configBitrateMaximum));
		}
	}

	if(m_configSamplingRate > 0)
	{
		args << "--resample" << QString::number(m_configSamplingRate);
	}

	switch(m_configChannelMode)
	{
	case 1:
		args << "-m" << "j";
		break;
	case 2:
		args << "-m" << "f";
		break;
	case 3:
		args << "-m" << "s";
		break;
	case 4:
		args << "-m" << "d";
		break;
	case 5:
		args << "-m" << "m";
		break;
	}

	bool bUseUCS2 = false;

	if(!metaInfo.fileName().isEmpty() && isUnicode(metaInfo.fileName())) bUseUCS2 = true;
	if(!metaInfo.fileArtist().isEmpty() && isUnicode(metaInfo.fileArtist())) bUseUCS2 = true;
	if(!metaInfo.fileAlbum().isEmpty() && isUnicode(metaInfo.fileAlbum())) bUseUCS2 = true;
	if(!metaInfo.fileGenre().isEmpty() && isUnicode(metaInfo.fileGenre())) bUseUCS2 = true;
	if(!metaInfo.fileComment().isEmpty() && isUnicode(metaInfo.fileComment())) bUseUCS2 = true;

	if(bUseUCS2) args << "--id3v2-ucs2"; //Must specify this BEFORE "--tt" and friends!

	if(!metaInfo.fileName().isEmpty()) args << "--tt" << metaInfo.fileName();
	if(!metaInfo.fileArtist().isEmpty()) args << "--ta" << metaInfo.fileArtist();
	if(!metaInfo.fileAlbum().isEmpty()) args << "--tl" << metaInfo.fileAlbum();
	if(!metaInfo.fileGenre().isEmpty()) args << "--tg" << metaInfo.fileGenre();
	if(!metaInfo.fileComment().isEmpty()) args << "--tc" << metaInfo.fileComment();
	if(metaInfo.fileYear()) args << "--ty" << QString::number(metaInfo.fileYear());
	if(metaInfo.filePosition()) args << "--tn" << QString::number(metaInfo.filePosition());
	if(!metaInfo.fileCover().isEmpty()) args << "--ti" << QDir::toNativeSeparators(metaInfo.fileCover());

	if(!m_configCustomParams.isEmpty()) args << m_configCustomParams.split(" ", QString::SkipEmptyParts);

	args << QDir::toNativeSeparators(sourceFile);
	args << QDir::toNativeSeparators(outputFile);

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	bool bTimeout = false;
	bool bAborted = false;
	int prevProgress = -1;

	QRegExp regExp("\\(.*(\\d+)%\\)\\|");

	while(process.state() != QProcess::NotRunning)
	{
		if(*abortFlag)
		{
			process.kill();
			bAborted = true;
			emit messageLogged("\nABORTED BY USER !!!");
			break;
		}
		process.waitForReadyRead(m_processTimeoutInterval);
		if(!process.bytesAvailable() && process.state() == QProcess::Running)
		{
			process.kill();
			qWarning("LAME process timed out <-- killing!");
			emit messageLogged("\nPROCESS TIMEOUT !!!");
			bTimeout = true;
			break;
		}
		while(process.bytesAvailable() > 0)
		{
			QByteArray line = process.readLine();
			QString text = QString::fromUtf8(line.constData()).simplified();
			if(regExp.lastIndexIn(text) >= 0)
			{
				bool ok = false;
				int progress = regExp.cap(1).toInt(&ok);
				if(ok && (progress > prevProgress))
				{
					emit statusUpdated(progress);
					prevProgress = qMin(progress + 2, 99);
				}
			}
			else if(!text.isEmpty())
			{
				emit messageLogged(text);
			}
		}
	}

	process.waitForFinished();
	if(process.state() != QProcess::NotRunning)
	{
		process.kill();
		process.waitForFinished(-1);
	}
	
	emit statusUpdated(100);
	emit messageLogged(QString().sprintf("\nExited with code: 0x%04X", process.exitCode()));

	if(bTimeout || bAborted || process.exitStatus() != QProcess::NormalExit)
	{
		return false;
	}
	
	return true;
}

QString MP3Encoder::extension(void)
{
	return "mp3";
}

bool MP3Encoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	if(containerType.compare("Wave", Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare("PCM", Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}
	else if(containerType.compare("MPEG Audio", Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare("MPEG Audio", Qt::CaseInsensitive) == 0)
		{
			if(formatProfile.compare("Layer 3", Qt::CaseInsensitive) == 0 || formatProfile.compare("Layer 2", Qt::CaseInsensitive) == 0)
			{
				if(formatVersion.compare("Version 1", Qt::CaseInsensitive) == 0 || formatVersion.compare("Version 2", Qt::CaseInsensitive) == 0)
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool MP3Encoder::requiresDownmix(void)
{
	return true;
}

void MP3Encoder::setAlgoQuality(int value)
{
	m_algorithmQuality = value;
}

void MP3Encoder::setBitrateLimits(int minimumBitrate, int maximumBitrate)
{
	m_configBitrateMinimum = minimumBitrate;
	m_configBitrateMaximum = maximumBitrate;
}

void MP3Encoder::setSamplingRate(int value)
{
	m_configSamplingRate = value;
}

void MP3Encoder::setChannelMode(int value)
{
	m_configChannelMode = value;
}

int MP3Encoder::clipBitrate(int bitrate)
{
	int targetBitrate = qMin(qMax(bitrate, 32), 320);
	
	int minDiff = INT_MAX;
	int minIndx = -1;

	for(int i = 0; SettingsModel::mp3Bitrates[i] > 0; i++)
	{
		int currentDiff = abs(targetBitrate - SettingsModel::mp3Bitrates[i]);
		if(currentDiff < minDiff)
		{
			minDiff = currentDiff;
			minIndx = i;
		}
	}

	if(minIndx >= 0)
	{
		return SettingsModel::mp3Bitrates[minIndx];
	}

	return targetBitrate;
}
