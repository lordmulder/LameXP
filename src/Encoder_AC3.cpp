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

#include "Encoder_AC3.h"

#include "Global.h"
#include "Model_Settings.h"

#include <QProcess>
#include <QDir>

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

bool AC3Encoder::encode(const QString &sourceFile, const AudioFileModel &metaInfo, const QString &outputFile, volatile bool *abortFlag)
{
	QProcess process;
	QStringList args;

	switch(m_configRCMode)
	{
	case SettingsModel::VBRMode:
		args << "-q" << QString::number(qMax(0, qMin(1023, m_configBitrate * 64)));
		break;
	case SettingsModel::CBRMode:
		args << "-b" << QString::number(SettingsModel::ac3Bitrates[qMax(0, qMin(18, m_configBitrate))]);
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

const unsigned int *AC3Encoder::requiresDownsample(void)
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
