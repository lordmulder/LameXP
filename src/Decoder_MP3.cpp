///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2017 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Decoder_MP3.h"

//Internal
#include "Global.h"

//MUtils
#include <MUtils/Global.h>
#include <MUtils/Exception.h>

//Qt
#include <QDir>
#include <QProcess>
#include <QRegExp>

MP3Decoder::MP3Decoder(void)
:
	m_binary(lamexp_tools_lookup("mpg123.exe"))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing MPG123 decoder. Tool 'mpg123.exe' is not registred!");
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

	QRegExp regExp("\\b\\d+\\+\\d+\\s+(\\d+):(\\d+)\\.(\\d+)\\+(\\d+):(\\d+)\\.(\\d+)\\b");

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
				quint32 values[6];
				if (MUtils::regexp_parse_uint32(regExp, values, 6))
				{
					const double timeDone = (60.0 * double(values[0])) + double(values[1]) + (double(values[2]) / 100.0);
					const double timeLeft = (60.0 * double(values[3])) + double(values[4]) + (double(values[5]) / 100.0);
					if ((timeDone >= 0.005) || (timeLeft >= 0.005))
					{
						const int newProgress = qRound((static_cast<double>(timeDone) / static_cast<double>(timeDone + timeLeft)) * 100.0);
						if (newProgress > prevProgress)
						{
							emit statusUpdated(newProgress);
							prevProgress = qMin(newProgress + 2, 99);
						}
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

const AbstractDecoder::supportedType_t *MP3Decoder::supportedTypes(void)
{
	static const char *exts[][3] =
	{
		{ "mp3", "mpa", NULL },
		{ "mp2", "mpa", NULL },
		{ "mp1", "mpa", NULL }
	};

	static const supportedType_t s_supportedTypes[] =
	{
		{ "MPEG Audio Layer III", exts[0] },
		{ "MPEG Audio Layer II",  exts[1] },
		{ "MPEG Audio Layer I",   exts[2] },
		{ NULL, NULL }
	};

	return s_supportedTypes;
}
