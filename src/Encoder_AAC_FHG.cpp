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

#include "Encoder_AAC_FHG.h"

#include "Global.h"
#include "Model_Settings.h"

#include <math.h>
#include <QProcess>
#include <QDir>

static int index2bitrate(const int index)
{
	return (index < 32) ? ((index + 1) * 8) : ((index - 15) * 16);
}

///////////////////////////////////////////////////////////////////////////////
// Encoder Info
///////////////////////////////////////////////////////////////////////////////

class FHGAACEncoderInfo : public AbstractEncoderInfo
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
			return 6;
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return 52;
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
			return qBound(1, index + 1, 6);
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return qBound(8, index2bitrate(index), 576);
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
			return TYPE_APPROX_BITRATE;
			break;
		case SettingsModel::CBRMode:
			return TYPE_BITRATE;
			break;
		default:
			throw "Bad RC mode specified!";
		}
	}

	virtual const char *description(void) const
	{
		static const char* s_description = "fhgaacenc/Winamp (\x0C2\x0A9 Nullsoft)";
		return s_description;
	}
}
static const g_fhgAacEncoderInfo;

///////////////////////////////////////////////////////////////////////////////
// Encoder implementation
///////////////////////////////////////////////////////////////////////////////

FHGAACEncoder::FHGAACEncoder(void)
:
	m_binary_enc(lamexp_lookup_tool("fhgaacenc.exe")),
	m_binary_dll(lamexp_lookup_tool("enc_fhgaac.dll"))
{
	if(m_binary_enc.isEmpty() || m_binary_dll.isEmpty())
	{
		throw "Error initializing FhgAacEnc. Tool 'fhgaacenc.exe' is not registred!";
	}

	m_configProfile = 0;
}

FHGAACEncoder::~FHGAACEncoder(void)
{
}

bool FHGAACEncoder::encode(const QString &sourceFile, const AudioFileModel &metaInfo, const QString &outputFile, volatile bool *abortFlag)
{
	QProcess process;
	QStringList args;
	
	int maxBitrate = 576;

	if(m_configRCMode == SettingsModel::CBRMode)
	{
		switch(m_configProfile)
		{
		case 1:
			args << "--profile" << "lc"; //Forces use of LC AAC profile
			break;
		case 2:
			maxBitrate = 128;
			args << "--profile" << "he"; //Forces use of HE AAC profile
			break;
		case 3:
			maxBitrate = 56;
			args << "--profile" << "hev2"; //Forces use of HEv2 AAC profile
			break;
		}
	}

	switch(m_configRCMode)
	{
	case SettingsModel::CBRMode:
		args << "--cbr" << QString::number(qBound(8, index2bitrate(m_configBitrate), maxBitrate));
		break;
	case SettingsModel::VBRMode:
		args << "--vbr" << QString::number(qBound(1, m_configBitrate + 1, 6));
		break;
	default:
		throw "Bad rate-control mode!";
		break;
	}

	args << "--dll" << m_binary_dll;

	if(!m_configCustomParams.isEmpty()) args << m_configCustomParams.split(" ", QString::SkipEmptyParts);

	args << QDir::toNativeSeparators(sourceFile);
	args << QDir::toNativeSeparators(outputFile);

	if(!startProcess(process, m_binary_enc, args))
	{
		return false;
	}

	bool bTimeout = false;
	bool bAborted = false;
	int prevProgress = -1;

	QRegExp regExp("Progress:\\s*(\\d+)%");

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
			qWarning("FhgAacEnc process timed out <-- killing!");
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

	if(bTimeout || bAborted || process.exitCode() != EXIT_SUCCESS)
	{
		return false;
	}

	return true;
}

QString FHGAACEncoder::extension(void)
{
	return "mp4";
}

bool FHGAACEncoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
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

const unsigned int *FHGAACEncoder::supportedChannelCount(void)
{
	static const unsigned int supportedChannels[] = {1, 2, 4, 5, 6, NULL};
	return supportedChannels;
}

const unsigned int *FHGAACEncoder::supportedSamplerates(void)
{
	static const unsigned int supportedRates[] = {192000, 96000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 6000, NULL};
	return supportedRates;
}

const unsigned int *FHGAACEncoder::supportedBitdepths(void)
{
	static const unsigned int supportedBPS[] = {16, 24, NULL};
	return supportedBPS;
}

void FHGAACEncoder::setProfile(int profile)
{
	m_configProfile = profile;
}

const AbstractEncoderInfo *FHGAACEncoder::getEncoderInfo(void)
{
	return &g_fhgAacEncoderInfo;
}
