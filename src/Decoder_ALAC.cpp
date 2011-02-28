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

#include "Decoder_ALAC.h"

#include "Global.h"

#include <QDir>
#include <QProcess>
#include <QRegExp>
#include <QUuid>

ALACDecoder::ALACDecoder(void)
:
	m_binary(lamexp_lookup_tool("alac.exe"))
{
	if(m_binary.isEmpty())
	{
		throw "Error initializing ALAC decoder. Tool 'alac.exe' is not registred!";
	}
}

ALACDecoder::~ALACDecoder(void)
{
}

bool ALACDecoder::decode(const QString &sourceFile, const QString &outputFile, volatile bool *abortFlag)
{
	QProcess process;
	QStringList args;

	args << "-f" << QDir::toNativeSeparators(outputFile);
	args << QDir::toNativeSeparators(sourceFile);

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	bool bTimeout = false;
	bool bAborted = false;

	//The ALAC Decoder doesn't actually send any status updates :-[
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
		process.waitForReadyRead(180000);
		if(!process.bytesAvailable() && process.state() == QProcess::Running)
		{
			process.kill();
			qWarning("TTAEnc process timed out <-- killing!");
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

	if(bTimeout || bAborted || process.exitStatus() != QProcess::NormalExit || QFileInfo(outputFile).size() == 0)
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
