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

#include "Encoder_Vorbis.h"

#include "Global.h"
#include "Model_Settings.h"

#include <QProcess>
#include <QDir>

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define IS_UNICODE(STR) (qstricmp(STR.toUtf8().constData(), QString::fromLocal8Bit(STR.toLocal8Bit()).toUtf8().constData()))

VorbisEncoder::VorbisEncoder(void)
:
	m_binary_i386(lamexp_lookup_tool("oggenc2_i386.exe")),
	m_binary_sse2(lamexp_lookup_tool("oggenc2_sse2.exe")),
	m_binary_x64(lamexp_lookup_tool("oggenc2_x64.exe"))
{
	if(m_binary_i386.isEmpty() || m_binary_sse2.isEmpty() || m_binary_x64.isEmpty())
	{
		throw "Error initializing Vorbis encoder. Tool 'oggenc2.exe' is not registred!";
	}

	m_configBitrateMaximum = 0;
	m_configBitrateMinimum = 0;
	m_configSamplingRate = 0;
}

VorbisEncoder::~VorbisEncoder(void)
{
}

bool VorbisEncoder::encode(const QString &sourceFile, const AudioFileModel &metaInfo, const QString &outputFile, volatile bool *abortFlag)
{
	QProcess process;
	QStringList args;
	const QString baseName = QFileInfo(outputFile).fileName();
	lamexp_cpu_t cpuFeatures = lamexp_detect_cpu_features();
	const QString &binary = (cpuFeatures.x64 ? m_binary_x64 : ((cpuFeatures.intel && cpuFeatures.sse && cpuFeatures.sse2) ? m_binary_sse2 : m_binary_i386));

	switch(m_configRCMode)
	{
	case SettingsModel::VBRMode:
		args << "-q" << QString::number(max(-2, min(10, m_configBitrate)));
		break;
	case SettingsModel::ABRMode:
		args << "-b" << QString::number(max(32, min(500, (m_configBitrate * 8))));
		break;
	default:
		throw "Bad rate-control mode!";
		break;
	}

	if((m_configBitrateMaximum > 0) && (m_configBitrateMinimum > 0) && (m_configBitrateMinimum <= m_configBitrateMaximum))
	{
		args << "--min-bitrate" << QString::number(min(max(m_configBitrateMinimum, 32), 500));
		args << "--max-bitrate" << QString::number(min(max(m_configBitrateMaximum, 32), 500));
	}

	if(m_configSamplingRate > 0)
	{
		args << "--resample" << QString::number(m_configSamplingRate) << "--converter" << QString::number(0);
	}

	if(!metaInfo.fileName().isEmpty()) args << "-t" << metaInfo.fileName();
	if(!metaInfo.fileArtist().isEmpty()) args << "-a" << metaInfo.fileArtist();
	if(!metaInfo.fileAlbum().isEmpty()) args << "-l" << metaInfo.fileAlbum();
	if(!metaInfo.fileGenre().isEmpty()) args << "-G" << metaInfo.fileGenre();
	if(!metaInfo.fileComment().isEmpty()) args << "-c" << QString("comment=%1").arg(metaInfo.fileComment());
	if(metaInfo.fileYear()) args << "-d" << QString::number(metaInfo.fileYear());
	if(metaInfo.filePosition()) args << "-N" << QString::number(metaInfo.filePosition());
	
	//args << "--tv" << QString().sprintf("Encoder=LameXP v%d.%02d.%04d [%s]", lamexp_version_major(), lamexp_version_minor(), lamexp_version_build(), lamexp_version_release());

	if(!m_configCustomParams.isEmpty()) args << m_configCustomParams.split(" ", QString::SkipEmptyParts);

	args << "-o" << QDir::toNativeSeparators(outputFile);
	args << QDir::toNativeSeparators(sourceFile);

	if(!startProcess(process, binary, args))
	{
		return false;
	}

	bool bTimeout = false;
	bool bAborted = false;

	QRegExp regExp("\\[.*(\\d+)[.,](\\d+)%\\]");

	while(process.state() != QProcess::NotRunning)
	{
		if(*abortFlag)
		{
			process.kill();
			bAborted = true;
			emit messageLogged("\nABORTED BY USER !!!");
			break;
		}
		process.waitForReadyRead();
		if(!process.bytesAvailable() && process.state() == QProcess::Running)
		{
			process.kill();
			qWarning("OggEnc process timed out <-- killing!");
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

	if(bTimeout || bAborted || process.exitStatus() != QProcess::NormalExit)
	{
		return false;
	}
	
	return true;
}

QString VorbisEncoder::extension(void)
{
	return "ogg";
}

bool VorbisEncoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	if(containerType.compare("Wave", Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare("PCM", Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}
	else if(containerType.compare("FLAC", Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare("FLAC", Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}

	return false;
}

void VorbisEncoder::setBitrateLimits(int minimumBitrate, int maximumBitrate)
{
	m_configBitrateMinimum = minimumBitrate;
	m_configBitrateMaximum = maximumBitrate;
}

void VorbisEncoder::setSamplingRate(int value)
{
	m_configSamplingRate = value;
}
