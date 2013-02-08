///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2013 LoRd_MuldeR <MuldeR2@GMX.de>
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
#include "Tool_WaveProperties.h"
#include "Model_AudioFile.h"

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

bool DownmixFilter::apply(const QString &sourceFile, const QString &outputFile, AudioFileModel *formatInfo, volatile bool *abortFlag)
{
	unsigned int channels = formatInfo->formatAudioChannels(); //detectChannels(sourceFile, abortFlag);
	emit messageLogged(QString().sprintf("--> Number of channels is: %d\n", channels));

	if(channels == 2)
	{
		messageLogged("Skipping downmix!");
		qDebug("Dowmmix not required for Stereo input, skipping!");
		return true;
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
	case 2: //Unknown
		qWarning("Downmixer: Nothing to do!");
	case 3: //3.0 (L/R/C)
		args << "remix" << "1v0.66,3v0.34" << "2v0.66,3v0.34";
		break;
	case 4: //3.1 (L/R/C/LFE)
		args << "remix" << "1v0.5,3v0.25,4v0.25" << "2v0.5,3v0.25,4v0.25";
		break;
	case 5: //5.0 (L/R/C/BL/BR)
		args << "remix" << "1v0.5,3v0.25,4v0.25" << "2v0.5,3v0.25,5v0.25";
		break;
	case 6: //5.1 (L/R/C/LFE/BL/BR)
		args << "remix" << "1v0.4,3v0.2,4v0.2,5v0.2" << "2v0.4,3v0.2,4v0.2,6v0.2";
		break;
	case 7: //7.0 (L/R/C/BL/BR/SL/SR)
		args << "remix" << "1v0.4,3v0.2,4v0.2,6v0.2" << "2v0.4,3v0.2,5v0.2,7v0.2";
		break;
	case 8: //7.1 (L/R/C/LFE/BL/BR/SL/SR)
		args << "remix" << "1v0.36,3v0.16,4v0.16,5v0.16,7v0.16" << "2v0.36,3v0.16,4v0.16,6v0.16,8v0.16";
		break;
	case 9: //8.1 (L/R/C/LFE/BL/BR/SL/SR/BC)
		args << "remix" << "1v0.308,3v0.154,4v0.154,5v0.154,7v0.154,9v0.076" << "2v0.308,3v0.154,4v0.154,6v0.154,8v0.154,9v0.076";
		break;
	default: //Unknown
		qWarning("Downmixer: Unknown channel configuration!");
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

	if(bTimeout || bAborted || process.exitCode() != EXIT_SUCCESS || QFileInfo(outputFile).size() == 0)
	{
		return false;
	}
	
	formatInfo->setFormatAudioChannels(2);
	return true;
}
