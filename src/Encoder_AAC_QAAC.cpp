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

#include "Encoder_AAC_QAAC.h"

//Internal
#include "Global.h"
#include "Model_Settings.h"

//MUtils
#include <MUtils/Global.h>

//StdLib
#include <math.h>

//Qt
#include <QProcess>
#include <QDir>
#include <QCoreApplication>

static int index2bitrate(const int index)
{
	return (index < 32) ? ((index + 1) * 8) : ((index - 15) * 16);
}

static const int g_qaacVBRQualityLUT[16] = {0 ,9, 18, 27, 36, 45, 54, 63, 73, 82, 91, 100, 109, 118, 127, INT_MAX};

///////////////////////////////////////////////////////////////////////////////
// Encoder Info
///////////////////////////////////////////////////////////////////////////////

class QAACEncoderInfo : public AbstractEncoderInfo
{
	virtual bool isModeSupported(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
		case SettingsModel::CBRMode:
		case SettingsModel::ABRMode:
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
			return 15;
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return 52;
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
			return g_qaacVBRQualityLUT[qBound(0, index , 14)];
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return qBound(8, index2bitrate(index), 576);
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
		static const char* s_description = "QAAC/QuickTime (\x0C2\x0A9 Apple Inc.)";
		return s_description;
	}
}
static const g_qaacEncoderInfo;

///////////////////////////////////////////////////////////////////////////////
// Encoder implementation
///////////////////////////////////////////////////////////////////////////////

QAACEncoder::QAACEncoder(void)
:
	m_binary_qaac(lamexp_tools_lookup("qaac.exe")),
	m_binary_soxr(lamexp_tools_lookup("libsoxr.dll")),
	m_binary_soxc(lamexp_tools_lookup("libsoxconvolver.dll"))
{
	if(m_binary_qaac.isEmpty() || m_binary_soxr.isEmpty() || m_binary_soxc.isEmpty())
	{
		MUTILS_THROW("Error initializing QAAC. Tool 'qaac.exe' is not registred!");
	}

	m_configProfile = 0;
}

QAACEncoder::~QAACEncoder(void)
{
}

bool QAACEncoder::encode(const QString &sourceFile, const AudioFileModel_MetaInfo &metaInfo, const unsigned int duration, const QString &outputFile, volatile bool *abortFlag)
{
	QProcess process;
	QStringList args;

	process.setWorkingDirectory(QFileInfo(outputFile).canonicalPath());

	QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
	env.insert("PATH", QDir::toNativeSeparators(QString("%1;%1/QTfiles;%2").arg(QDir(QCoreApplication::applicationDirPath()).canonicalPath(), MUtils::temp_folder())));
	process.setProcessEnvironment(env);

	if(m_configRCMode != SettingsModel::VBRMode)
	{
		switch(m_configProfile)
		{
		case 2:
		case 3:
			args << "--he"; //Forces use of HE AAC profile (there is no explicit HEv2 switch for QAAC)
			break;
		}
	}

	switch(m_configRCMode)
	{
	case SettingsModel::CBRMode:
		args << "--cbr" << QString::number(qBound(8, index2bitrate(m_configBitrate), 576));
		break;
	case SettingsModel::ABRMode:
		args << "--abr" << QString::number(qBound(8, index2bitrate(m_configBitrate), 576));
		break;
	case SettingsModel::VBRMode:
		args << "--tvbr" << QString::number(g_qaacVBRQualityLUT[qBound(0, m_configBitrate , 14)]);
		break;
	default:
		MUTILS_THROW("Bad rate-control mode!");
		break;
	}

	if(!m_configCustomParams.isEmpty()) args << m_configCustomParams.split(" ", QString::SkipEmptyParts);

	if(!metaInfo.title().isEmpty())   args << "--title"   << cleanTag(metaInfo.title());
	if(!metaInfo.artist().isEmpty())  args << "--artist"  << cleanTag(metaInfo.artist());
	if(!metaInfo.album().isEmpty())   args << "--album"   << cleanTag(metaInfo.album());
	if(!metaInfo.genre().isEmpty())   args << "--genre"   << cleanTag(metaInfo.genre());
	if(!metaInfo.comment().isEmpty()) args << "--comment" << cleanTag( metaInfo.comment());
	if(metaInfo.year())               args << "--date"    << QString::number(metaInfo.year());
	if(metaInfo.position())           args << "--track"   << QString::number(metaInfo.position());
	if(!metaInfo.cover().isEmpty())   args << "--artwork" << metaInfo.cover();

	args << "-d" << ".";
	args << "-o" << QDir::toNativeSeparators(outputFile);
	args << QDir::toNativeSeparators(sourceFile);

	if(!startProcess(process, m_binary_qaac, args))
	{
		return false;
	}

	bool bTimeout = false;
	bool bAborted = false;
	int prevProgress = -1;

	QRegExp regExp("\\[(\\d+)\\.(\\d)%\\]");

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
			qWarning("QAAC process timed out <-- killing!");
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

QString QAACEncoder::extension(void)
{
	return "mp4";
}

bool QAACEncoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
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

void QAACEncoder::setProfile(int profile)
{
	m_configProfile = profile;
}

const AbstractEncoderInfo *QAACEncoder::getEncoderInfo(void)
{
	return &g_qaacEncoderInfo;
}