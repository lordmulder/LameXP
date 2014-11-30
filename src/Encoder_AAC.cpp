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

#include "Encoder_AAC.h"

#include "Global.h"
#include "Model_Settings.h"

#include <QProcess>
#include <QDir>

static int index2bitrate(const int index)
{
	return (index < 32) ? ((index + 1) * 8) : ((index - 15) * 16);
}

///////////////////////////////////////////////////////////////////////////////
// Encoder Info
///////////////////////////////////////////////////////////////////////////////

class AACEncoderInfo : public AbstractEncoderInfo
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
			return 21;
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return 41;
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
			return qBound(0, index * 5, 100);
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return qBound(8, index2bitrate(index), 400);
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
			return TYPE_QUALITY_LEVEL_FLT;
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
		static const char* s_description = "Nero AAC Encoder (\x0C2\x0A9 Nero AG)";
		return s_description;
	}
}
static const g_aacEncoderInfo;

///////////////////////////////////////////////////////////////////////////////
// Encoder implementation
///////////////////////////////////////////////////////////////////////////////

AACEncoder::AACEncoder(void)
:
	m_binary_enc(lamexp_lookup_tool("neroAacEnc.exe")),
	m_binary_tag(lamexp_lookup_tool("neroAacTag.exe")),
	m_binary_sox(lamexp_lookup_tool("sox.exe"))
{
	if(m_binary_enc.isEmpty() || m_binary_tag.isEmpty() || m_binary_sox.isEmpty())
	{
		MUTILS_THROW("Error initializing AAC encoder. Tool 'neroAacEnc.exe' is not registred!");
	}

	m_configProfile = 0;
	m_configEnable2Pass = true;
}

AACEncoder::~AACEncoder(void)
{
}

bool AACEncoder::encode(const QString &sourceFile, const AudioFileModel_MetaInfo &metaInfo, const unsigned int duration, const QString &outputFile, volatile bool *abortFlag)
{
	QProcess process;
	QStringList args;
	const QString baseName = QFileInfo(outputFile).fileName();

	switch(m_configRCMode)
	{
	case SettingsModel::VBRMode:
		args << "-q" << QString().sprintf("%.2f", double(qBound(0, m_configBitrate * 5, 100)) / 100.0);
		break;
	case SettingsModel::ABRMode:
		args << "-br" << QString::number(qBound(8, index2bitrate(m_configBitrate), 400) * 1000);
		break;
	case SettingsModel::CBRMode:
		args << "-cbr" << QString::number(qBound(8, index2bitrate(m_configBitrate), 400) * 1000);
		break;
	default:
		MUTILS_THROW("Bad rate-control mode!");
		break;
	}

	if(m_configEnable2Pass && (m_configRCMode == SettingsModel::ABRMode))
	{
		args << "-2pass";
	}
	
	switch(m_configProfile)
	{
	case 1:
		args << "-lc"; //Forces use of LC AAC profile
		break;
	case 2:
		args << "-he"; //Forces use of HE AAC profile
		break;
	case 3:
		args << "-hev2"; //Forces use of HEv2 AAC profile
		break;
	}

	if(!m_configCustomParams.isEmpty()) args << m_configCustomParams.split(" ", QString::SkipEmptyParts);

	args << "-if" << QDir::toNativeSeparators(sourceFile);
	args << "-of" << QDir::toNativeSeparators(outputFile);

	if(!startProcess(process, m_binary_enc, args))
	{
		return false;
	}

	bool bTimeout = false;
	bool bAborted = false;
	int prevProgress = -1;


	QRegExp regExp("Processed\\s+(\\d+)\\s+seconds");
	QRegExp regExp_pass1("First\\s+pass:\\s+processed\\s+(\\d+)\\s+seconds");
	QRegExp regExp_pass2("Second\\s+pass:\\s+processed\\s+(\\d+)\\s+seconds");

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
			qWarning("NeroAacEnc process timed out <-- killing!");
			emit messageLogged("\nPROCESS TIMEOUT !!!");
			bTimeout = true;
			break;
		}
		while(process.bytesAvailable() > 0)
		{
			QByteArray line = process.readLine();
			QString text = QString::fromUtf8(line.constData()).simplified();
			if(regExp_pass1.lastIndexIn(text) >= 0)
			{
				bool ok = false;
				int progress = regExp_pass1.cap(1).toInt(&ok);
				if(ok && (duration > 0))
				{
					int newProgress = qRound((static_cast<double>(progress) / static_cast<double>(duration)) * 50.0);
					if(newProgress > prevProgress)
					{
						emit statusUpdated(newProgress);
						prevProgress = qMin(newProgress + 2, 99);
					}
				}
			}
			else if(regExp_pass2.lastIndexIn(text) >= 0)
			{
				bool ok = false;
				int progress = regExp_pass2.cap(1).toInt(&ok);
				if(ok && (duration > 0))
				{
					int newProgress = qRound((static_cast<double>(progress) / static_cast<double>(duration)) * 50.0) + 50;
					if(newProgress > prevProgress)
					{
						emit statusUpdated(newProgress);
						prevProgress = qMin(newProgress + 2, 99);
					}
				}
			}
			else if(regExp.lastIndexIn(text) >= 0)
			{
				bool ok = false;
				int progress = regExp.cap(1).toInt(&ok);
				if(ok && (duration > 0))
				{
					int newProgress = qRound((static_cast<double>(progress) / static_cast<double>(duration)) * 100.0);
					if(newProgress > prevProgress)
					{
						emit statusUpdated(newProgress);
						prevProgress = qMin(newProgress + 2, 99);
					}
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

	emit messageLogged("\n-------------------------------\n");
	
	args.clear();
	args << QDir::toNativeSeparators(outputFile);

	if(!metaInfo.title().isEmpty()) args << QString("-meta:title=%1").arg(cleanTag(metaInfo.title()));
	if(!metaInfo.artist().isEmpty()) args << QString("-meta:artist=%1").arg(cleanTag(metaInfo.artist()));
	if(!metaInfo.album().isEmpty()) args << QString("-meta:album=%1").arg(cleanTag(metaInfo.album()));
	if(!metaInfo.genre().isEmpty()) args << QString("-meta:genre=%1").arg(cleanTag(metaInfo.genre()));
	if(!metaInfo.comment().isEmpty()) args << QString("-meta:comment=%1").arg(cleanTag(metaInfo.comment()));
	if(metaInfo.year()) args << QString("-meta:year=%1").arg(QString::number(metaInfo.year()));
	if(metaInfo.position()) args << QString("-meta:track=%1").arg(QString::number(metaInfo.position()));
	if(!metaInfo.cover().isEmpty()) args << QString("-add-cover:%1:%2").arg("front", metaInfo.cover());
	
	if(!startProcess(process, m_binary_tag, args))
	{
		return false;
	}

	bTimeout = false;

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
			qWarning("NeroAacTag process timed out <-- killing!");
			emit messageLogged("\nPROCESS TIMEOUT !!!");
			bTimeout = true;
			break;
		}
		while(process.bytesAvailable() > 0)
		{
			QByteArray line = process.readLine();
			QString text = QString::fromUtf8(line.constData()).simplified();
			if(!text.isEmpty())
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
		
	emit messageLogged(QString().sprintf("\nExited with code: 0x%04X", process.exitCode()));

	if(bTimeout || bAborted || process.exitCode() != EXIT_SUCCESS)
	{
		return false;
	}

	return true;
}

QString AACEncoder::extension(void)
{
	return "mp4";
}

bool AACEncoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
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

void AACEncoder::setProfile(int profile)
{
	m_configProfile = profile;
}

void AACEncoder::setEnable2Pass(bool enabled)
{
	m_configEnable2Pass = enabled;
}

const bool AACEncoder::needsTimingInfo(void)
{
	return true;
}

const AbstractEncoderInfo *AACEncoder::getEncoderInfo(void)
{
	return &g_aacEncoderInfo;
}
