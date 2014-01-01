///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2014 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Encoder_FLAC.h"

#include "Global.h"
#include "Model_Settings.h"

#include <QProcess>
#include <QDir>

///////////////////////////////////////////////////////////////////////////////
// Encoder Info
///////////////////////////////////////////////////////////////////////////////

class FLACEncoderInfo : public AbstractEncoderInfo
{
public:
	virtual bool isModeSupported(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
			return true;
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return false;
			break;
		default:
			THROW("Bad RC mode specified!");
		}
	}

	virtual int valueCount(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
			return 9;
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return 0;
			break;
		default:
			THROW("Bad RC mode specified!");
		}
	}

	virtual int valueAt(int mode, int index) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
			return qBound(0, index, 8);
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return -1;
			break;
		default:
			THROW("Bad RC mode specified!");
		}
	}

	virtual int valueType(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
			return TYPE_COMPRESSION_LEVEL;
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return -1;
		default:
			THROW("Bad RC mode specified!");
		}
	}

	virtual const char *description(void) const
	{
		static const char* s_description = "Free Lossless Audio Codec (FLAC)";
		return s_description;
	}
}
static const g_flacEncoderInfo;

///////////////////////////////////////////////////////////////////////////////
// Encoder implementation
///////////////////////////////////////////////////////////////////////////////

FLACEncoder::FLACEncoder(void)
:
	m_binary(lamexp_lookup_tool("flac.exe"))
{
	if(m_binary.isEmpty())
	{
		THROW("Error initializing FLAC encoder. Tool 'flac.exe' is not registred!");
	}
}

FLACEncoder::~FLACEncoder(void)
{
}

bool FLACEncoder::encode(const QString &sourceFile, const AudioFileModel_MetaInfo &metaInfo, const unsigned int duration, const QString &outputFile, volatile bool *abortFlag)
{
	QProcess process;
	QStringList args;

	args << QString("-%1").arg(QString::number(qBound(0, m_configBitrate, 8)));
	args << "--channel-map=none";

	if(!metaInfo.title().isEmpty()) args << "-T" << QString("title=%1").arg(cleanTag(metaInfo.title()));
	if(!metaInfo.artist().isEmpty()) args << "-T" << QString("artist=%1").arg(cleanTag(metaInfo.artist()));
	if(!metaInfo.album().isEmpty()) args << "-T" << QString("album=%1").arg(cleanTag(metaInfo.album()));
	if(!metaInfo.genre().isEmpty()) args << "-T" << QString("genre=%1").arg(cleanTag(metaInfo.genre()));
	if(!metaInfo.comment().isEmpty()) args << "-T" << QString("comment=%1").arg(cleanTag(metaInfo.comment()));
	if(metaInfo.year()) args << "-T" << QString("date=%1").arg(QString::number(metaInfo.year()));
	if(metaInfo.position()) args << "-T" << QString("track=%1").arg(QString::number(metaInfo.position()));
	if(!metaInfo.cover().isEmpty()) args << QString("--picture=%1").arg(metaInfo.cover());

	//args << "--tv" << QString().sprintf("Encoder=LameXP v%d.%02d.%04d [%s]", lamexp_version_major(), lamexp_version_minor(), lamexp_version_build(), lamexp_version_release());

	if(!m_configCustomParams.isEmpty()) args << m_configCustomParams.split(" ", QString::SkipEmptyParts);

	args << "-f" << "-o" << QDir::toNativeSeparators(outputFile);
	args << QDir::toNativeSeparators(sourceFile);

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	bool bTimeout = false;
	bool bAborted = false;
	int prevProgress = -1;

	QRegExp regExp("\\b(\\d+)% complete");

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
			qWarning("FLAC process timed out <-- killing!");
			emit messageLogged("\nPROCESS TIMEOUT !!!");
			bTimeout = true;
			break;
		}
		while(process.bytesAvailable() > 0)
		{
			QByteArray line = process.readLine().replace('\b', char(0x20));
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

QString FLACEncoder::extension(void)
{
	return "flac";
}

bool FLACEncoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
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

const unsigned int *FLACEncoder::supportedChannelCount(void)
{
	static const unsigned int supportedChannels[] = {2, 4, 5, 6, 7, 8, NULL};
	return supportedChannels;
}

const unsigned int *FLACEncoder::supportedBitdepths(void)
{
	static const unsigned int supportedBPS[] = {16, 24, NULL};
	return supportedBPS;
}

const AbstractEncoderInfo *FLACEncoder::getEncoderInfo(void)
{
	return &g_flacEncoderInfo;
}
