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

#include "Encoder_AAC.h"

#include "Global.h"
#include "Model_Settings.h"

#include <QProcess>
#include <QDir>

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

AACEncoder::AACEncoder(void)
:
	m_binary_enc(lamexp_lookup_tool("neroAacEnc.exe")),
	m_binary_tag(lamexp_lookup_tool("neroAacTag.exe"))
{
	if(m_binary_enc.isEmpty() || m_binary_tag.isEmpty())
	{
		throw "Error initializing AAC encoder. Tool 'neroAacEnc.exe' is not registred!";
	}

	m_configProfile = 0;
	m_configEnable2Pass = true;
}

AACEncoder::~AACEncoder(void)
{
}

bool AACEncoder::encode(const QString &sourceFile, const AudioFileModel &metaInfo, const QString &outputFile, volatile bool *abortFlag)
{
	QProcess process;
	QStringList args;
	const QString baseName = QFileInfo(outputFile).fileName();

	switch(m_configRCMode)
	{
	case SettingsModel::VBRMode:
		args << "-q" << QString().sprintf("%.2f", min(1.0, max(0.0, static_cast<double>(m_configBitrate * 5) / 100.0)));
		break;
	case SettingsModel::ABRMode:
		args << "-br" << QString::number(max(32, min(500, (m_configBitrate * 8))) * 1000);
		break;
	case SettingsModel::CBRMode:
		args << "-cbr" << QString::number(max(32, min(500, (m_configBitrate * 8))) * 1000);
		break;
	default:
		throw "Bad rate-control mode!";
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
		process.waitForReadyRead();
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
				if(ok && metaInfo.fileDuration() > 0)
				{
					emit statusUpdated(static_cast<int>((static_cast<double>(progress) / static_cast<double>(metaInfo.fileDuration())) * 50.0));
				}
			}
			else if(regExp_pass2.lastIndexIn(text) >= 0)
			{
				bool ok = false;
				int progress = regExp_pass2.cap(1).toInt(&ok);
				if(ok && metaInfo.fileDuration() > 0)
				{
					emit statusUpdated(static_cast<int>((static_cast<double>(progress) / static_cast<double>(metaInfo.fileDuration())) * 50.0) + 50);
				}
			}
			else if(regExp.lastIndexIn(text) >= 0)
			{
				bool ok = false;
				int progress = regExp.cap(1).toInt(&ok);
				if(ok && metaInfo.fileDuration() > 0)
				{
					emit statusUpdated(static_cast<int>((static_cast<double>(progress) / static_cast<double>(metaInfo.fileDuration())) * 100.0));
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

	emit messageLogged("\n-------------------------------\n");
	
	args.clear();
	args << QDir::toNativeSeparators(outputFile);

	if(!metaInfo.fileName().isEmpty()) args << QString("-meta:title=%1").arg(metaInfo.fileName());
	if(!metaInfo.fileArtist().isEmpty()) args << QString("-meta:artist=%1").arg(metaInfo.fileArtist());
	if(!metaInfo.fileAlbum().isEmpty()) args << QString("-meta:album=%1").arg(metaInfo.fileAlbum());
	if(!metaInfo.fileGenre().isEmpty()) args << QString("-meta:genre=%1").arg(metaInfo.fileGenre());
	if(!metaInfo.fileComment().isEmpty()) args << QString("-meta:comment=%1").arg(metaInfo.fileComment());
	if(metaInfo.fileYear()) args << QString("-meta:year=%1").arg(QString::number(metaInfo.fileYear()));
	if(metaInfo.filePosition()) args << QString("-meta:track=%1").arg(QString::number(metaInfo.filePosition()));
	
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
		process.waitForReadyRead();
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

	if(bTimeout || bAborted || process.exitStatus() != QProcess::NormalExit)
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
