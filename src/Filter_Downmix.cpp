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

#include "Filter_Downmix.h"

#include "Global.h"

#include <QDir>
#include <QProcess>
#include <QRegExp>

DownmixFilter::DownmixFilter(void)
:
	m_binary(lamexp_lookup_tool("sox.exe"))
{
	if(m_binary.isEmpty())
	{
		throw "Error initializing SoX filter. Tool 'sox.exe' is not registred!";
	}
}

DownmixFilter::~DownmixFilter(void)
{
}

bool DownmixFilter::apply(const QString &sourceFile, const QString &outputFile, volatile bool *abortFlag)
{
	unsigned int channels = detectChannels(sourceFile, abortFlag);
	emit messageLogged(QString().sprintf("--> Number of channels is: %d\n", channels));

	if((channels == 1) || (channels == 2))
	{
		messageLogged("Skipping downmix!");
		qDebug("Dowmmix not required for Mono or Stereo input, skipping!");
		return false;
	}

	QProcess process;
	QStringList args;

	process.setWorkingDirectory(QFileInfo(outputFile).canonicalPath());

	args << "-V3" << "-S";
	args << "--guard" << "--temp" << ".";
	args << QDir::toNativeSeparators(sourceFile);
	args << QDir::toNativeSeparators(outputFile);

	switch(channels)
	{
	case 3:
		args << "remix" << "1,3" << "2,3";
		break;
	case 4:
		args << "remix" << "1,3,4" << "2,3,4";
		break;
	case 6:
		args << "remix" << "1,3,4,5" << "2,3,4,6";
		break;
	case 8:
		args << "remix" << "1,3,4,5,7" << "2,3,4,6,8";
		break;
	case 9:
		args << "remix" << "1,3,4,5,7,9" << "2,3,4,6,8,9";
		break;
	default:
		args << "channels" << "2";
		break;
	}

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	bool bTimeout = false;
	bool bAborted = false;

	QRegExp regExp("In:(\\d+)(\\.\\d+)*%");

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

	if(bTimeout || bAborted || process.exitStatus() != QProcess::NormalExit || QFileInfo(outputFile).size() == 0)
	{
		return false;
	}
	
	return true;
}

unsigned int DownmixFilter::detectChannels(const QString &sourceFile, volatile bool *abortFlag)
{
	unsigned int channels = 0;
	
	QProcess process;
	QStringList args;

	args << "--i" << sourceFile;

	if(!startProcess(process, m_binary, args))
	{
		return channels;
	}

	bool bTimeout = false;
	bool bAborted = false;

	QRegExp regExp("Channels\\s*:\\s*(\\d+)", Qt::CaseInsensitive);

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
			if(regExp.lastIndexIn(text) >= 0)
			{
				bool ok = false;
				unsigned int temp = regExp.cap(1).toUInt(&ok);
				if(ok) channels = temp;
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

	return channels;
}
