///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2020 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU GENERAL PUBLIC LICENSE as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version; always including the non-optional
// LAMEXP GNU GENERAL PUBLIC LICENSE ADDENDUM. See "License.txt" file!
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

//MUtils
#include <MUtils/Global.h>

//Internal
#include "Global.h"
#include "Model_Settings.h"
#include "MimeTypes.h"

//Qt
#include <QProcess>
#include <QDir>
#include <QUUid>

static const int g_opusBitrateLUT[30] = { 8, 9, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 144, 160, 176, 192, 208, 224, 240, 256 };

///////////////////////////////////////////////////////////////////////////////
// Encoder Info
///////////////////////////////////////////////////////////////////////////////

class OpusEncoderInfo : public AbstractEncoderInfo
{
	virtual bool isModeSupported(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return true;
			break;
		default:
			MUTILS_THROW("Bad RC mode specified!");
		}
	}

	virtual int valueCount(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return 30;
			break;
		default:
			MUTILS_THROW("Bad RC mode specified!");
		}
	}

	virtual int valueAt(int mode, int index) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return g_opusBitrateLUT[qBound(0, index, 29)];
			break;
		default:
			MUTILS_THROW("Bad RC mode specified!");
		}
	}

	virtual int valueType(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
		case SettingsModel::ABRMode:
			return TYPE_APPROX_BITRATE;
			break;
		case SettingsModel::CBRMode:
			return TYPE_BITRATE;
			break;
		default:
			MUTILS_THROW("Bad RC mode specified!");
		}
	}

	virtual const char *description(void) const
	{
		static const char* s_description = "Opus-Tools OpusEnc (libopus)";
		return s_description;
	}

	virtual const char *extension(void) const
	{
		static const char* s_extension = "opus";
		return s_extension;
	}

	virtual bool isResamplingSupported(void) const
	{
		return false;
	}
}
static const g_opusEncoderInfo;

///////////////////////////////////////////////////////////////////////////////
// Encoder implementation
///////////////////////////////////////////////////////////////////////////////

OpusEncoder::OpusEncoder(void)
:
	m_binary(lamexp_tools_lookup(L1S("opusenc.exe")))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing Opus encoder. Tool 'opusenc.exe' is not registred!");
	}

	m_configOptimizeFor = 0;
	m_configEncodeComplexity = 10;
	m_configFrameSize = 3;
}

OpusEncoder::~OpusEncoder(void)
{
}

bool OpusEncoder::encode(const QString &sourceFile, const AudioFileModel_MetaInfo &metaInfo, const unsigned int duration, const unsigned int channels, const QString &outputFile, QAtomicInt &abortFlag)
{
	QProcess process;
	QStringList args;

	switch(m_configRCMode)
	{
	case SettingsModel::VBRMode:
		args << L1S("--vbr");
		break;
	case SettingsModel::ABRMode:
		args << L1S("--cvbr");
		break;
	case SettingsModel::CBRMode:
		args << L1S("--hard-cbr");
		break;
	default:
		MUTILS_THROW("Bad rate-control mode!");
		break;
	}

	args << "--comp" << QString::number(m_configEncodeComplexity);

	switch(m_configFrameSize)
	{
	case 0:
		args << L1S("--framesize") << L1S("2.5");
		break;
	case 1:
		args << L1S("--framesize") << L1S("5");
		break;
	case 2:
		args << L1S("--framesize") << L1S("10");
		break;
	case 3:
		args << L1S("--framesize") << L1S("20");
		break;
	case 4:
		args << L1S("--framesize") << L1S("40");
		break;
	case 5:
		args << L1S("--framesize") << L1S("60");
		break;
	}
	
	args << L1S("--bitrate") << QString::number(g_opusBitrateLUT[qBound(0, m_configBitrate, 29)]);

	if(!metaInfo.title().isEmpty())   args << L1S("--title")   << cleanTag(metaInfo.title());
	if(!metaInfo.artist().isEmpty())  args << L1S("--artist")  << cleanTag(metaInfo.artist());
	if(!metaInfo.album().isEmpty())   args << L1S("--album")   << cleanTag(metaInfo.album());
	if(!metaInfo.genre().isEmpty())   args << L1S("--genre")   << cleanTag(metaInfo.genre());
	if(metaInfo.year())               args << L1S("--date")    << QString::number(metaInfo.year());
	if(metaInfo.position())           args << L1S("--comment") << QString("tracknumber=%1").arg(QString::number(metaInfo.position()));
	if(!metaInfo.comment().isEmpty()) args << L1S("--comment") << QString("comment=%1").arg(cleanTag(metaInfo.comment()));
	if(!metaInfo.cover().isEmpty())   args << L1S("--picture") << makeCoverParam(metaInfo.cover());

	if(!m_configCustomParams.isEmpty()) args << m_configCustomParams.split(" ", QString::SkipEmptyParts);

	args << QDir::toNativeSeparators(sourceFile);
	args << QDir::toNativeSeparators(outputFile);

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	int prevProgress = -1;
	QRegExp regExp(L1S("\\[.\\]\\s+(\\d+)%"));

	const result_t result = awaitProcess(process, abortFlag, [this, &prevProgress, &regExp](const QString &text)
	{
		if (regExp.lastIndexIn(text) >= 0)
		{
			qint32 newProgress;
			if (MUtils::regexp_parse_int32(regExp, newProgress))
			{
				if (newProgress > prevProgress)
				{
					emit statusUpdated(newProgress);
					prevProgress = NEXT_PROGRESS(newProgress);
				}
			}
			return true;
		}
		return false;
	});
	
	return (result == RESULT_SUCCESS);
}

QString OpusEncoder::detectMimeType(const QString &coverFile)
{
	const QString suffix = QFileInfo(coverFile).suffix();
	for (size_t i = 0; MIME_TYPES[i].type; i++)
	{
		for (size_t k = 0; MIME_TYPES[i].ext[k]; k++)
		{
			if (suffix.compare(QString::fromLatin1(MIME_TYPES[i].ext[k]), Qt::CaseInsensitive) == 0)
			{
				return QString::fromLatin1(MIME_TYPES[i].type);
			}
		}
	}

	qWarning("Unknown MIME type for extension '%s' -> using default!", MUTILS_UTF8(coverFile));
	return QString::fromLatin1(MIME_TYPES[0].type);
}

QString OpusEncoder::makeCoverParam(const QString &coverFile)
{
	return QString("3|%1|||%2").arg(detectMimeType(coverFile), QDir::toNativeSeparators(coverFile));
}

void OpusEncoder::setOptimizeFor(int optimizeFor)
{
	m_configOptimizeFor = qBound(0, optimizeFor, 2);
}

void OpusEncoder::setEncodeComplexity(int complexity)
{
	m_configEncodeComplexity = qBound(0, complexity, 10);
}

void OpusEncoder::setFrameSize(int frameSize)
{
	m_configFrameSize = qBound(0, frameSize, 5);
}

bool OpusEncoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	if(containerType.compare(L1S("Wave"), Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare(L1S("PCM"), Qt::CaseInsensitive) == 0)
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
	static const unsigned int supportedBPS[] = {8, 16, 24, AudioFileModel::BITDEPTH_IEEE_FLOAT32, NULL};
	return supportedBPS;
}

const bool OpusEncoder::needsTimingInfo(void)
{
	return true;
}

const AbstractEncoderInfo *OpusEncoder::getEncoderInfo(void)
{
	return &g_opusEncoderInfo;
}
