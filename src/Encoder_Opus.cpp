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

#include "Encoder_Opus.h"

#include "Global.h"
#include "Model_Settings.h"

#include <QProcess>
#include <QDir>
#include <QUUid>

///////////////////////////////////////////////////////////////////////////////
// Encoder Info
///////////////////////////////////////////////////////////////////////////////

class OpusEncoderInfo : public AbstractEncoderInfo
{
	virtual bool isModeSupported(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return true;
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
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return 32;
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
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return qBound(8, (index + 1) * 8, 256);
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
		static const char* s_description = "Opus-Tools OpusEnc (libopus)";
		return s_description;
	}
}
static const g_opusEncoderInfo;

///////////////////////////////////////////////////////////////////////////////
// Encoder implementation
///////////////////////////////////////////////////////////////////////////////

OpusEncoder::OpusEncoder(void)
:
	m_binary(lamexp_tools_lookup("opusenc.exe"))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing Opus encoder. Tool 'opusenc.exe' is not registred!");
	}

	m_configOptimizeFor = 0;
	m_configEncodeComplexity = 10;
	m_configFrameSize = 3;
}

OpusEncoder::~OpusEncoder(void)
{
}

bool OpusEncoder::encode(const QString &sourceFile, const AudioFileModel_MetaInfo &metaInfo, const unsigned int duration, const QString &outputFile, volatile bool *abortFlag)
{
	QProcess process;
	QStringList args;

	switch(m_configRCMode)
	{
	case SettingsModel::VBRMode:
		args << "--vbr";
		break;
	case SettingsModel::ABRMode:
		args << "--cvbr";
		break;
	case SettingsModel::CBRMode:
		args << "--hard-cbr";
		break;
	default:
		MUTILS_THROW("Bad rate-control mode!");
		break;
	}

	args << "--comp" << QString::number(m_configEncodeComplexity);

	switch(m_configFrameSize)
	{
	case 0:
		args << "--framesize" << "2.5";
		break;
	case 1:
		args << "--framesize" << "5";
		break;
	case 2:
		args << "--framesize" << "10";
		break;
	case 3:
		args << "--framesize" << "20";
		break;
	case 4:
		args << "--framesize" << "40";
		break;
	case 5:
		args << "--framesize" << "60";
		break;
	}

	args << QString("--bitrate") << QString::number(qBound(8, (m_configBitrate + 1) * 8, 256));

	if(!metaInfo.title().isEmpty()) args << "--title" << cleanTag(metaInfo.title());
	if(!metaInfo.artist().isEmpty()) args << "--artist" << cleanTag(metaInfo.artist());
	if(!metaInfo.album().isEmpty()) args << "--album" << cleanTag(metaInfo.album());
	if(!metaInfo.genre().isEmpty()) args << "--genre" << cleanTag(metaInfo.genre());
	if(metaInfo.year()) args << "--date" << QString::number(metaInfo.year());
	if(metaInfo.position()) args << "--comment" << QString("tracknumber=%1").arg(QString::number(metaInfo.position()));
	if(!metaInfo.comment().isEmpty()) args << "--comment" << QString("comment=%1").arg(cleanTag(metaInfo.comment()));
	
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

	QRegExp regExp("\\((\\d+)%\\)");

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
			qWarning("Opus process timed out <-- killing!");
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

void OpusEncoder::setOptimizeFor(int optimizeFor)
{
	m_configOptimizeFor = qBound(0, optimizeFor, 2);
}

void OpusEncoder::setEncodeComplexity(int complexity)
{
	m_configEncodeComplexity = qBound(0, complexity, 10);
}

void OpusEncoder::setFrameSize(int frameSize)
{
	m_configFrameSize = qBound(0, frameSize, 5);
}

QString OpusEncoder::extension(void)
{
	return "opus";
}

bool OpusEncoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
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

const unsigned int *OpusEncoder::supportedChannelCount(void)
{
	return NULL;
}

const unsigned int *OpusEncoder::supportedBitdepths(void)
{
	static const unsigned int supportedBPS[] = {8, 16, 24, AudioFileModel::BITDEPTH_IEEE_FLOAT32, NULL};
	return supportedBPS;
}

const bool OpusEncoder::needsTimingInfo(void)
{
	return true;
}

const AbstractEncoderInfo *OpusEncoder::getEncoderInfo(void)
{
	return &g_opusEncoderInfo;
}
