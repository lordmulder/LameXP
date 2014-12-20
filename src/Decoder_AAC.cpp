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

#include "Decoder_AAC.h"

//Internal
#include "Global.h"

//MUtils
#include <MUtils/Exception.h>

//Qt
#include <QDir>
#include <QProcess>
#include <QRegExp>

AACDecoder::AACDecoder(void)
:
	m_binary(lamexp_tools_lookup("faad.exe"))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing AAC decoder. Tool 'faad.exe' is not registred!");
	}
}

AACDecoder::~AACDecoder(void)
{
}

bool AACDecoder::decode(const QString &sourceFile, const QString &outputFile, volatile bool *abortFlag)
{
	QProcess process;
	QStringList args;

	args << "-o" << QDir::toNativeSeparators(outputFile);
	args << QDir::toNativeSeparators(sourceFile);

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	bool bTimeout = false;
	bool bAborted = false;

	QRegExp regExp("\\[(\\d+)%\\]\\s+decoding\\s+");

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
			qWarning("FAAD process timed out <-- killing!");
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
	
	return true;
}

bool AACDecoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	if(containerType.compare("ADTS", Qt::CaseInsensitive) == 0 || containerType.compare("MPEG-4", Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare("AAC", Qt::CaseInsensitive) == 0)
		{
			QStringList profileParts = formatProfile.split(" ", QString::SkipEmptyParts);
			if(profileParts.contains("LC", Qt::CaseInsensitive) || profileParts.contains("HE-AAC", Qt::CaseInsensitive) || profileParts.contains("HE-AACv2", Qt::CaseInsensitive))
			{
				if(formatVersion.compare("Version 2", Qt::CaseInsensitive) == 0 || formatVersion.compare("Version 4", Qt::CaseInsensitive) == 0 || formatVersion.isEmpty())
				{
					return true;
				}
			}
		}
	}

	return false;
}

QStringList AACDecoder::supportedTypes(void)
{
	return QStringList() << "Advanced Audio Coding (*.aac *.mp4 *.m4a)";
}
