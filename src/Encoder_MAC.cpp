///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2013 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Encoder_MAC.h"

#include "Global.h"
#include "Model_Settings.h"

#include <QProcess>
#include <QDir>

///////////////////////////////////////////////////////////////////////////////
// Encoder Info
///////////////////////////////////////////////////////////////////////////////

class MACEncoderInfo : public AbstractEncoderInfo
{
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
			return 5;
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return -1;
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
			return qBound(0, index + 1, 8);
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
			break;
		default:
			THROW("Bad RC mode specified!");
		}
	}

	virtual const char *description(void) const
	{
		static const char* s_description = "Monkey's Audio (MAC)";
		return s_description;
	}
}
static const g_macEncoderInfo;

///////////////////////////////////////////////////////////////////////////////
// Encoder implementation
///////////////////////////////////////////////////////////////////////////////

MACEncoder::MACEncoder(void)
:
	m_binary_enc(lamexp_lookup_tool("mac.exe")),
	m_binary_tag(lamexp_lookup_tool("tag.exe"))
{
	if(m_binary_enc.isEmpty() || m_binary_tag.isEmpty())
	{
		THROW("Error initializing MAC encoder. Tool 'mac.exe' or 'tag.exe' is not registred!");
	}
}

MACEncoder::~MACEncoder(void)
{
}

bool MACEncoder::encode(const QString &sourceFile, const AudioFileModel_MetaInfo &metaInfo, const unsigned int duration, const QString &outputFile, volatile bool *abortFlag)
{
	QProcess process;
	QStringList args;
	
	const QString baseName = QFileInfo(outputFile).fileName();

	args << QDir::toNativeSeparators(sourceFile);
	args << QDir::toNativeSeparators(outputFile);

	switch(m_configRCMode)
	{
	case SettingsModel::VBRMode:
		args << QString().sprintf("-c%d", (m_configBitrate + 1) * 1000);
		break;
	default:
		THROW("Bad rate-control mode!");
		break;
	}

	if(!startProcess(process, m_binary_enc, args))
	{
		return false;
	}

	bool bTimeout = false;
	bool bAborted = false;
	int prevProgress = -1;

	QRegExp regExp("Progress: (\\d+).(\\d+)%");

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
			qWarning("MAC process timed out <-- killing!");
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

	emit messageLogged("\n-------------------------------\n");
	
	args.clear();
	args << "APE2" << QDir::toNativeSeparators(outputFile);

	if(!metaInfo.title().isEmpty()) args << QString("Title=%1").arg(cleanTag(metaInfo.title()));
	if(!metaInfo.artist().isEmpty()) args << QString("Artist=%1").arg(cleanTag(metaInfo.artist()));
	if(!metaInfo.album().isEmpty()) args << QString("Album=%1").arg(cleanTag(metaInfo.album()));
	if(!metaInfo.genre().isEmpty()) args << QString("Genre=%1").arg(cleanTag(metaInfo.genre()));
	if(!metaInfo.comment().isEmpty()) args << QString("Comment=%1").arg(cleanTag(metaInfo.comment()));
	if(metaInfo.year()) args << QString("Year=%1").arg(QString::number(metaInfo.year()));
	if(metaInfo.position()) args << QString("Track=%1").arg(QString::number(metaInfo.position()));
	
	//if(!metaInfo.cover().isEmpty()) args << QString("-add-cover:%1:%2").arg("front", metaInfo.cover());
	
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
			qWarning("Tag process timed out <-- killing!");
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

QString MACEncoder::extension(void)
{
	return "ape";
}

bool MACEncoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
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

const AbstractEncoderInfo *MACEncoder::getEncoderInfo(void)
{
	return &g_macEncoderInfo;
}
