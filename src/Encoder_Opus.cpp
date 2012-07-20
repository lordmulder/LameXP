///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2012 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Encoder_Opus.h"

#include "Global.h"
#include "Model_Settings.h"

#include <QProcess>
#include <QDir>
#include <QUUid>

OpusEncoder::OpusEncoder(void)
:
	m_binary(lamexp_lookup_tool("opusenc.exe"))
{
	if(m_binary.isEmpty())
	{
		throw "Error initializing Opus encoder. Tool 'opusenc.exe' is not registred!";
	}
}

OpusEncoder::~OpusEncoder(void)
{
}

bool OpusEncoder::encode(const QString &sourceFile, const AudioFileModel &metaInfo, const QString &outputFile, volatile bool *abortFlag)
{
	QProcess process;
	QStringList args;

	args << "--music"; //TODO: Make other optimizations available!

	switch(m_configRCMode)
	{
	case SettingsModel::VBRMode:
		args << "--vbr";
		break;
	case SettingsModel::ABRMode:
		args << "-cvbr";
		break;
	case SettingsModel::CBRMode:
		args << "--hard-cbr";
		break;
	default:
		throw "Bad rate-control mode!";
		break;
	}

	args << QString("--bitrate") << QString::number(qMax(0, qMin(500, m_configBitrate * 8)));

	if(!metaInfo.fileName().isEmpty()) args << "--title" << metaInfo.fileName();
	if(!metaInfo.fileArtist().isEmpty()) args << "--artist" << metaInfo.fileArtist();
	if(!metaInfo.fileAlbum().isEmpty()) args << "--comment" << QString("album=%1").arg(metaInfo.fileAlbum());
	if(!metaInfo.fileGenre().isEmpty()) args << "--comment" << QString("genre=%1").arg(metaInfo.fileGenre());
	if(!metaInfo.fileComment().isEmpty()) args << "--comment" << QString("comment=%1").arg(metaInfo.fileComment());
	if(metaInfo.fileYear()) args << "--comment" << QString("date=%1").arg(QString::number(metaInfo.fileYear()));
	if(metaInfo.filePosition()) args << "--comment" << QString("track=%1").arg(QString::number(metaInfo.filePosition()));
	
	if(!m_configCustomParams.isEmpty()) args << m_configCustomParams.split(" ", QString::SkipEmptyParts);

	args << QDir::toNativeSeparators(sourceFile);
	args << QDir::toNativeSeparators(outputFile);

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	bool bTimeout = false;
	bool bAborted = false;
	int prevProgress = -1;

	QRegExp regExp("\\[(-|\\\\|/|\\|)\\]");

	//The Opus encoder doesn't flus it's status updates :-[
	emit statusUpdated(20 + (QUuid::createUuid().data1 % 60));

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
			qWarning("Opus process timed out <-- killing!");
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
				if(ok && (progress > prevProgress))
				{
					emit statusUpdated(progress);
					prevProgress = qMin(progress + 2, 99);
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

QString OpusEncoder::extension(void)
{
	return "opus";
}

bool OpusEncoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
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

const unsigned int *OpusEncoder::supportedChannelCount(void)
{
	return NULL;
}

const unsigned int *OpusEncoder::supportedBitdepths(void)
{
	return NULL;
}
