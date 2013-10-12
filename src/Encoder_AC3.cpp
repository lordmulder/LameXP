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

#include "Encoder_AC3.h"

#include "Global.h"
#include "Model_Settings.h"

#include <QProcess>
#include <QDir>

static const int g_ac3BitratesLUT[20] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640, -1};

///////////////////////////////////////////////////////////////////////////////
// Encoder Info
///////////////////////////////////////////////////////////////////////////////

class AC3EncoderInfo : public AbstractEncoderInfo
{
	virtual bool isModeSupported(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
		case SettingsModel::CBRMode:
			return true;
			break;
		case SettingsModel::ABRMode:
			return false;
			break;
		default:
			throw "Bad RC mode specified!";
		}
	}

	virtual int valueCount(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
			return 65;
			break;
		case SettingsModel::ABRMode:
			return 0;
			break;
		case SettingsModel::CBRMode:
			return 19;
			break;
		default:
			throw "Bad RC mode specified!";
		}
	}

	virtual int valueAt(int mode, int index) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
			return qBound(0, index * 16, 1023);
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return g_ac3BitratesLUT[qBound(0, index, 18)];
			break;
		default:
			throw "Bad RC mode specified!";
		}
	}

	virtual int valueType(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
			return TYPE_QUALITY_LEVEL_INT;
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return TYPE_BITRATE;
			break;
		default:
			throw "Bad RC mode specified!";
		}
	}

	virtual const char *description(void) const
	{
		static const char* s_description = "Aften: A/52 Audio Encoder";
		return s_description;
	}
}
static const g_aftenEncoderInfo;

///////////////////////////////////////////////////////////////////////////////
// Encoder implementation
///////////////////////////////////////////////////////////////////////////////

AC3Encoder::AC3Encoder(void)
:
	m_binary(lamexp_lookup_tool("aften.exe"))
{
	if(m_binary.isEmpty())
	{
		throw "Error initializing FLAC encoder. Tool 'aften.exe' is not registred!";
	}

	m_configAudioCodingMode = 0;
	m_configDynamicRangeCompression = 5;
	m_configExponentSearchSize = 8;
	m_configFastBitAllocation = false;
}

AC3Encoder::~AC3Encoder(void)
{
}

bool AC3Encoder::encode(const QString &sourceFile, const AudioFileModel_MetaInfo &metaInfo, const unsigned int duration, const QString &outputFile, volatile bool *abortFlag)
{
	QProcess process;
	QStringList args;

	switch(m_configRCMode)
	{
	case SettingsModel::VBRMode:
		args << "-q" << QString::number(qBound(0, m_configBitrate * 16, 1023));
		break;
	case SettingsModel::CBRMode:
		args << "-b" << QString::number(g_ac3BitratesLUT[qBound(0, m_configBitrate, 18)]);
		break;
	default:
		throw "Bad rate-control mode!";
		break;
	}

	if(m_configAudioCodingMode >= 1)
	{
		args << "-acmod" << QString::number(m_configAudioCodingMode - 1);
	}
	if(m_configDynamicRangeCompression != 5)
	{
		args << "-dynrng" << QString::number(m_configDynamicRangeCompression);
	}
	if(m_configExponentSearchSize != 8)
	{
		args << "-exps" << QString::number(m_configExponentSearchSize);
	}
	if(m_configFastBitAllocation)
	{
		args << "-fba" << QString::number(1);
	}

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

	QRegExp regExp("progress:(\\s+)(\\d+)%(\\s+)\\|");

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
			qWarning("Aften process timed out <-- killing!");
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
				int progress = regExp.cap(2).toInt(&ok);
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

	if(bTimeout || bAborted || process.exitCode() != EXIT_SUCCESS)
	{
		return false;
	}
	
	return true;
}

void AC3Encoder::setAudioCodingMode(int value)
{
	m_configAudioCodingMode = qMin(8, qMax(0, value));
}

void AC3Encoder::setDynamicRangeCompression(int value)
{
	m_configDynamicRangeCompression = qMin(5, qMax(0, value));
}

void AC3Encoder::setExponentSearchSize(int value)
{
	m_configExponentSearchSize = qMin(32, qMax(1, value));
}

void AC3Encoder::setFastBitAllocation(bool value)
{
	m_configFastBitAllocation = value;
}

QString AC3Encoder::extension(void)
{
	return "ac3";
}

const unsigned int *AC3Encoder::supportedChannelCount(void)
{
	static const unsigned int supportedChannels[] = {1, 2, 3, 4, 5, 6, NULL};
	return supportedChannels;
}

const unsigned int *AC3Encoder::supportedSamplerates(void)
{
	static const unsigned int supportedRates[] = {48000, 44100, 32000, NULL};
	return supportedRates;
}

bool AC3Encoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	if(containerType.compare("Wave", Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare("PCM", Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}

	return false;
}

const AbstractEncoderInfo *AC3Encoder::getEncoderInfo(void)
{
	return &g_aftenEncoderInfo;
}
