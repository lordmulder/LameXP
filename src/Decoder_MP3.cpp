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

#include "Decoder_MP3.h"

#include "Global.h"

#include <QDir>
#include <QProcess>
#include <QRegExp>

MP3Decoder::MP3Decoder(void)
:
	m_binary(lamexp_lookup_tool("mpg123.exe"))
{
	if(m_binary.isEmpty())
	{
		throw "Error initializing MPG123 decoder. Tool 'mpg123.exe' is not registred!";
	}
}

MP3Decoder::~MP3Decoder(void)
{
}

bool MP3Decoder::decode(const QString &sourceFile, const QString &outputFile, volatile bool *abortFlag)
{
	QProcess process;
	QStringList args;

	args << "-v" << "--utf8" << "-w" << QDir::toNativeSeparators(outputFile);
	args << QDir::toNativeSeparators(sourceFile);

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	bool bTimeout = false;
	bool bAborted = false;
	int prevProgress = -1;

	QRegExp regExp("\\s+Time:\\s+(\\d+):(\\d+)\\.(\\d+)\\s+\\[(\\d+):(\\d+)\\.(\\d+)\\],");

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
			qWarning("mpg123 process timed out <-- killing!");
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
				int values[6];
				for(int i = 0; i < 6; i++)
				{
					bool ok = false;
					int temp = regExp.cap(i+1).toInt(&ok);
					values[i] = (ok ? temp : 0);
				}
				int timeDone = (60 * values[0]) + values[1];
				int timeLeft = (60 * values[3]) + values[4];
				if(timeDone > 0 || timeLeft > 0)
				{
					int newProgress = qRound((static_cast<double>(timeDone) / static_cast<double>(timeDone + timeLeft)) * 100.0);
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

	if(bTimeout || bAborted || process.exitStatus() != QProcess::NormalExit)
	{
		return false;
	}
	
	return true;
}

bool MP3Decoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	if(containerType.compare("MPEG Audio", Qt::CaseInsensitive) == 0 || containerType.compare("Wave", Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare("MPEG Audio", Qt::CaseInsensitive) == 0)
		{
			if(formatProfile.compare("Layer 3", Qt::CaseInsensitive) == 0 || formatProfile.compare("Layer 2", Qt::CaseInsensitive) == 0)
			{
				if(formatVersion.compare("Version 1", Qt::CaseInsensitive) == 0 || formatVersion.compare("Version 2", Qt::CaseInsensitive) == 0)
				{
					return true;
				}
			}
		}
	}

	return false;
}

QStringList MP3Decoder::supportedTypes(void)
{
	return QStringList() << "MPEG Audio Layer III (*.mp3 *.mpa)" << "MPEG Audio Layer II (*.mp2 *.mpa)" << "MPEG Audio Layer I ( *.mp1 *.mpa)";
}

