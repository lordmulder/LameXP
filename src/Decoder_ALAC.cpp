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

#include "Decoder_ALAC.h"

#include "Global.h"

#include <QDir>
#include <QProcess>
#include <QRegExp>
#include <QUuid>

ALACDecoder::ALACDecoder(void)
:
	m_binary(lamexp_lookup_tool("refalac.exe"))
{
	if(m_binary.isEmpty())
	{
		THROW("Error initializing ALAC decoder. Tool 'refalac.exe' is not registred!");
	}
}

ALACDecoder::~ALACDecoder(void)
{
}

bool ALACDecoder::decode(const QString &sourceFile, const QString &outputFile, volatile bool *abortFlag)
{
	QProcess process;
	QStringList args;

	args << "--decode";
	args << "-o" << QDir::toNativeSeparators(outputFile);
	args << QDir::toNativeSeparators(sourceFile);

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	bool bTimeout = false;
	bool bAborted = false;
	int prevProgress = -1;

	//The ALAC Decoder doesn't actually send any status updates :-[
	//emit statusUpdated(20 + (QUuid::createUuid().data1 % 60));
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
			qWarning("ALAC process timed out <-- killing!");
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
				bool ok[2] = {false, false};
				int intVal[2] = {0, 0};
				intVal[0] = regExp.cap(1).toInt(&ok[0]);
				intVal[1] = regExp.cap(2).toInt(&ok[1]);
				if(ok[0] && ok[1])
				{
					int progress = qRound(static_cast<double>(intVal[0]) + (static_cast<double>(intVal[1]) / 10.0));
					if(progress > prevProgress)
					{
						emit statusUpdated(progress);
						prevProgress = qMin(progress + 2, 99);
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

	if(bTimeout || bAborted || process.exitCode() != EXIT_SUCCESS || QFileInfo(outputFile).size() == 0)
	{
		return false;
	}
	
	return true;
}

bool ALACDecoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	if(containerType.compare("MPEG-4", Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare("ALAC", Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}

	return false;
}

QStringList ALACDecoder::supportedTypes(void)
{
	return QStringList() << "Apple Lossless (*.mp4 *.m4a)";
}
