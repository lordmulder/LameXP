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

#include "Tool_WaveProperties.h"

//Internal
#include "Global.h"
#include "Model_AudioFile.h"

//MUtils
#include <MUtils/Exception.h>

//Qt
#include <QDir>
#include <QProcess>

WaveProperties::WaveProperties(void)
:
	m_binary(lamexp_tools_lookup("sox.exe"))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing MP3 encoder. Tool 'lame.exe' is not registred!");
	}
}

WaveProperties::~WaveProperties(void)
{
}

bool WaveProperties::detect(const QString &sourceFile, AudioFileModel_TechInfo *info, volatile bool *abortFlag)
{
	QProcess process;
	QStringList args;

	args << "--i" << QDir::toNativeSeparators(sourceFile);

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	bool bTimeout = false;
	bool bAborted = false;

	int progress = 0;

	QRegExp regExp_precision("Precision\\s*:\\s*(\\d+)-bit", Qt::CaseInsensitive);
	QRegExp regExp_samplerate("Sample Rate\\s*:\\s*(\\d+)", Qt::CaseInsensitive);
	QRegExp regExp_duration("Duration\\s*:\\s*(\\d\\d):(\\d\\d):(\\d\\d)\\.(\\d\\d)", Qt::CaseInsensitive);
	QRegExp regExp_channels("Channels\\s*:\\s*(\\d+)", Qt::CaseInsensitive);
	QRegExp regExp_encoding("Sample Encoding\\s*:\\s*(\\d+)-bit\\s*Float", Qt::CaseInsensitive); //SoX returns a precision of 24-Bit for 32-Bit Float data, so we detect it this way!

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
			qWarning("SoX process timed out <-- killing!");
			emit messageLogged("\nPROCESS TIMEOUT !!!");
			bTimeout = true;
			break;
		}
		while(process.bytesAvailable() > 0)
		{
			QByteArray line = process.readLine();
			QString text = QString::fromUtf8(line.constData()).simplified();
			if(regExp_precision.lastIndexIn(text) >= 0)
			{
				bool ok = false;
				unsigned int tmp = regExp_precision.cap(1).toUInt(&ok);
				if(ok) info->setAudioBitdepth(tmp);
				emit statusUpdated(qMin(progress += 25, 100));
			}
			if(regExp_encoding.lastIndexIn(text) >= 0)
			{
				bool ok = false;
				unsigned int tmp = regExp_encoding.cap(1).toUInt(&ok);
				if(ok) info->setAudioBitdepth((tmp == 32) ? AudioFileModel::BITDEPTH_IEEE_FLOAT32 : tmp);
				emit statusUpdated(qMin(progress += 25, 100));
			}
			if(regExp_samplerate.lastIndexIn(text) >= 0)
			{
				bool ok = false;
				unsigned int tmp = regExp_samplerate.cap(1).toUInt(&ok);
				if(ok) info->setAudioSamplerate(tmp);
				emit statusUpdated(qMin(progress += 25, 100));
			}
			if(regExp_duration.lastIndexIn(text) >= 0)
			{
				bool ok[4] = {false, false, false, false};
				unsigned int tmp1 = regExp_duration.cap(1).toUInt(&ok[0]);
				unsigned int tmp2 = regExp_duration.cap(2).toUInt(&ok[1]);
				unsigned int tmp3 = regExp_duration.cap(3).toUInt(&ok[2]);
				unsigned int tmp4 = regExp_duration.cap(4).toUInt(&ok[3]);
				if(ok[0] && ok[1] && ok[2] && ok[3])
				{
					info->setDuration((tmp1 * 3600) + (tmp2 * 60) + tmp3 + qRound(static_cast<double>(tmp4) / 100.0));
				}
				emit statusUpdated(qMin(progress += 25, 100));
			}
			if(regExp_channels.lastIndexIn(text) >= 0)
			{
				bool ok = false;
				unsigned int tmp = regExp_channels.cap(1).toUInt(&ok);
				if(ok) info->setAudioChannels(tmp);
				emit statusUpdated(qMin(progress += 25, 100));
			}
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

	emit statusUpdated(100);
	emit messageLogged(QString().sprintf("\nExited with code: 0x%04X", process.exitCode()));

	if(bTimeout || bAborted || process.exitCode() != EXIT_SUCCESS)
	{
		return false;
	}
	
	return true;
}
