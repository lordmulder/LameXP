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

#include "Decoder_WMA.h"

#include "Global.h"

#include <QDir>
#include <QProcess>
#include <QRegExp>
#include <QSystemSemaphore>
#include <QUuid>

WMADecoder::WMADecoder(void)
:
	m_binary(lamexp_lookup_tool("wmawav.exe")),
	m_semaphore(new QSystemSemaphore("{84BB780D-67D3-49DC-ADF0-97C795A55D5C}", 1))
{
	if(m_binary.isEmpty())
	{
		throw "Error initializing WMA decoder. Tool 'wmawav.exe' is not registred!";
	}
}

WMADecoder::~WMADecoder(void)
{
}

bool WMADecoder::decode(const QString &sourceFile, const QString &outputFile, volatile bool *abortFlag)
{
	QProcess process;
	QStringList args;

	args << pathToShort(QDir::toNativeSeparators(sourceFile)); 
	args << pathToShort(QDir::toNativeSeparators(outputFile));

	if(!m_semaphore->acquire())
	{
		emit messageLogged("Failed to acquire the semaphore!");
		return false;
	}

	if(!startProcess(process, m_binary, args))
	{
		m_semaphore->release();
		return false;
	}

	bool bTimeout = false;
	bool bAborted = false;

	//The WMA Decoder doesn't actually send any status updates :-[
	emit statusUpdated(20 + (QUuid::createUuid().data1 % 80));

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
			qWarning("WmaWav process timed out <-- killing!");
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
	
	emit statusUpdated(100);
	emit messageLogged(QString().sprintf("\nExited with code: 0x%04X", process.exitCode()));
	m_semaphore->release();

	if(bTimeout || bAborted || process.exitStatus() != QProcess::NormalExit || QFileInfo(outputFile).size() == 0)
	{
		return false;
	}
	
	return true;
}

bool WMADecoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	if(containerType.compare("Windows Media", Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare("WMA", Qt::CaseInsensitive) == 0)
		{
			if(formatVersion.compare("Version 1", Qt::CaseInsensitive) == 0 || formatVersion.compare("Version 2", Qt::CaseInsensitive) == 0 || formatVersion.compare("Version 3", Qt::CaseInsensitive) == 0 || formatProfile.compare("Pro", Qt::CaseInsensitive) == 0 || formatProfile.compare("Lossless", Qt::CaseInsensitive) == 0)
			{
				return true;
			}
		}
	}

	return false;
}

bool WMADecoder::isDecoderAvailable(void)
{
	return lamexp_check_tool("wmawav.exe");
}

QStringList WMADecoder::supportedTypes(void)
{
	return QStringList() << "Windows Media Audio (*.wma)";
}
