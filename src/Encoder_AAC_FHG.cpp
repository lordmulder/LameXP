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

#include "Encoder_AAC_FHG.h"

#include "Global.h"
#include "Model_Settings.h"

#include <QProcess>
#include <QDir>

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

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

	switch(m_configRCMode)
	{
	case SettingsModel::VBRMode:
		args << "--vbr" << QString::number(max(1, min(5, m_configBitrate)));
		break;
	case SettingsModel::CBRMode:
		args << "--cbr" << QString::number(max(32, min(500, (m_configBitrate * 8))));
		break;
	case SettingsModel::ABRMode:
		qWarning("FhgAacEnc does not support ABR mode -> failure!");
		emit messageLogged("\nTHIS ENCODER DOES NOT SUPPORT AN \"ABR\" MODE !!!");
		return false;
		break;
	default:
		throw "Bad rate-control mode!";
		break;
	}
	
	switch(m_configProfile)
	{
	case 1:
		args << "--profile" << "lc"; //Forces use of LC AAC profile
		break;
	case 2:
		args << "--profile" << "he"; //Forces use of HE AAC profile
		break;
	case 3:
		args << "--profile" << "hev2"; //Forces use of HEv2 AAC profile
		break;
	}

	args << "--dll" << m_binary_dll;

	args << QDir::toNativeSeparators(sourceFile);
	args << QDir::toNativeSeparators(outputFile);

	if(!startProcess(process, m_binary_enc, args))
	{
		return false;
	}

	bool bTimeout = false;
	bool bAborted = false;

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
				if(ok) emit statusUpdated(progress);
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


void FHGAACEncoder::setProfile(int profile)
{
	m_configProfile = profile;
}
