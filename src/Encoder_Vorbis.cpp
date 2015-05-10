///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2015 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version, but always including the *additional*
// restrictions defined in the "License.txt" file.
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

#include "Encoder_Vorbis.h"

#include "Global.h"
#include "Model_Settings.h"

#include <QProcess>
#include <QDir>

///////////////////////////////////////////////////////////////////////////////
// Encoder Info
///////////////////////////////////////////////////////////////////////////////

class VorbisEncoderInfo : public AbstractEncoderInfo
{
	virtual bool isModeSupported(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
		case SettingsModel::ABRMode:
			return true;
			break;
		case SettingsModel::CBRMode:
			return false;
			break;
		default:
			MUTILS_THROW("Bad RC mode specified!");
		}
	}

	virtual int valueCount(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
			return 12;
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return 60;
			break;
		default:
			MUTILS_THROW("Bad RC mode specified!");
		}
	}

	virtual int valueAt(int mode, int index) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
			return qBound(-2, index - 2, 10);
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return qBound(32, (index + 4) * 8, 500);
			break;
		default:
			MUTILS_THROW("Bad RC mode specified!");
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
			MUTILS_THROW("Bad RC mode specified!");
		}
	}

	virtual const char *description(void) const
	{
		static const char* s_description = "OggEnc2 Vorbis Encoder (aoTuV)";
		return s_description;
	}

	virtual const char *extension(void) const
	{
		static const char* s_extension = "ogg";
		return s_extension;
	}
}
static const g_vorbisEncoderInfo;

///////////////////////////////////////////////////////////////////////////////
// Encoder implementation
///////////////////////////////////////////////////////////////////////////////

VorbisEncoder::VorbisEncoder(void)
:
	m_binary(lamexp_tools_lookup("oggenc2.exe"))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing Vorbis encoder. Tool 'oggenc2.exe' is not registred!");
	}

	m_configBitrateMaximum = 0;
	m_configBitrateMinimum = 0;
	m_configSamplingRate = 0;
}

VorbisEncoder::~VorbisEncoder(void)
{
}

bool VorbisEncoder::encode(const QString &sourceFile, const AudioFileModel_MetaInfo &metaInfo, const unsigned int duration, const QString &outputFile, volatile bool *abortFlag)
{
	QProcess process;
	QStringList args;
	const QString baseName = QFileInfo(outputFile).fileName();

	switch(m_configRCMode)
	{
	case SettingsModel::VBRMode:
		args << "-q" << QString::number(qBound(-2, m_configBitrate - 2, 10));
		break;
	case SettingsModel::ABRMode:
		args << "-b" << QString::number(qBound(32, (m_configBitrate + 4) * 8, 500));
		break;
	default:
		MUTILS_THROW("Bad rate-control mode!");
		break;
	}

	if((m_configBitrateMaximum > 0) && (m_configBitrateMinimum > 0) && (m_configBitrateMinimum <= m_configBitrateMaximum))
	{
		args << "--min-bitrate" << QString::number(qBound(32, m_configBitrateMinimum, 500));
		args << "--max-bitrate" << QString::number(qBound(32, m_configBitrateMaximum, 500));
	}

	if(m_configSamplingRate > 0)
	{
		args << "--resample" << QString::number(m_configSamplingRate) << "--converter" << QString::number(0);
	}

	if(!metaInfo.title().isEmpty()) args << "-t" << cleanTag(metaInfo.title());
	if(!metaInfo.artist().isEmpty()) args << "-a" << cleanTag(metaInfo.artist());
	if(!metaInfo.album().isEmpty()) args << "-l" << cleanTag(metaInfo.album());
	if(!metaInfo.genre().isEmpty()) args << "-G" << cleanTag(metaInfo.genre());
	if(!metaInfo.comment().isEmpty()) args << "-c" << QString("comment=%1").arg(cleanTag(metaInfo.comment()));
	if(metaInfo.year()) args << "-d" << QString::number(metaInfo.year());
	if(metaInfo.position()) args << "-N" << QString::number(metaInfo.position());
	
	//args << "--tv" << QString().sprintf("Encoder=LameXP v%d.%02d.%04d [%s]", lamexp_version_major(), lamexp_version_minor(), lamexp_version_build(), lamexp_version_release());

	if(!m_configCustomParams.isEmpty()) args << m_configCustomParams.split(" ", QString::SkipEmptyParts);

	args << "-o" << QDir::toNativeSeparators(outputFile);
	args << QDir::toNativeSeparators(sourceFile);

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	bool bTimeout = false;
	bool bAborted = false;
	int prevProgress = -1;

	QRegExp regExp("\\[.*(\\d+)[.,](\\d+)%\\]");

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
			qWarning("OggEnc process timed out <-- killing!");
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

bool VorbisEncoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	if(containerType.compare("Wave", Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare("PCM", Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}
	else if(containerType.compare("FLAC", Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare("FLAC", Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}

	return false;
}

void VorbisEncoder::setBitrateLimits(int minimumBitrate, int maximumBitrate)
{
	m_configBitrateMinimum = minimumBitrate;
	m_configBitrateMaximum = maximumBitrate;
}

void VorbisEncoder::setSamplingRate(int value)
{
	m_configSamplingRate = value;
}

const AbstractEncoderInfo *VorbisEncoder::getEncoderInfo(void)
{
	return &g_vorbisEncoderInfo;
}
