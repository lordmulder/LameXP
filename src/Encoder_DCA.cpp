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

#include "Encoder_DCA.h"

#include "Global.h"
#include "Model_Settings.h"

#include <QProcess>
#include <QDir>
#include <limits.h>

static int index2bitrate(const int index)
{
	return (index < 8) ? ((index + 1) * 32) : ((index < 12) ? ((index - 3) * 64) : ((index < 24) ? (index - 7) * 128 : (index - 15) * 256));
}

///////////////////////////////////////////////////////////////////////////////
// Encoder Info
///////////////////////////////////////////////////////////////////////////////

class DCAEncoderInfo : public AbstractEncoderInfo
{
	virtual bool isModeSupported(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
		case SettingsModel::ABRMode:
			return false;
			break;
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
			return 0;
			break;
		case SettingsModel::CBRMode:
			return 32;
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
			return 0;
			break;
		case SettingsModel::CBRMode:
			return qBound(32, index2bitrate(index), 4096);
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
			return TYPE_QUALITY_LEVEL_INT;
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return TYPE_BITRATE;
			break;
		default:
			MUTILS_THROW("Bad RC mode specified!");
		}
	}

	virtual const char *description(void) const
	{
		static const char* s_description = "dcaenc-2 by Alexander E. Patrakov";
		return s_description;
	}

	virtual const char *extension(void) const
	{
		static const char* s_extension = "dts";
		return s_extension;
	}

	virtual bool isResamplingSupported(void) const
	{
		return false;
	}
}
static const g_dcaEncoderInfo;

///////////////////////////////////////////////////////////////////////////////
// Encoder implementation
///////////////////////////////////////////////////////////////////////////////

DCAEncoder::DCAEncoder(void)
:
	m_binary(lamexp_tools_lookup(L1S("dcaenc.exe")))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing DCA encoder. Tool 'dcaenc.exe' is not registred!");
	}
}

DCAEncoder::~DCAEncoder(void)
{
}

bool DCAEncoder::encode(const QString &sourceFile, const AudioFileModel_MetaInfo &metaInfo, const unsigned int duration, const unsigned int channels, const QString &outputFile, QAtomicInt &abortFlag)
{
	QProcess process;
	QStringList args;

	args << L1S("-i") << QDir::toNativeSeparators(sourceFile);
	args << L1S("-o") << QDir::toNativeSeparators(outputFile);
	args << L1S("-b") << QString::number(qBound(32, index2bitrate(m_configBitrate), 4096));

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	int prevProgress = -1;
	QRegExp regExp(L1S("\\[(\\d+)\\.(\\d+)%\\]"));

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

bool DCAEncoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
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

const unsigned int *DCAEncoder::supportedChannelCount(void)
{
	static const unsigned int supportedChannels[] = {1, 2, 4, 5, 6, NULL};
	return supportedChannels;
}

const unsigned int *DCAEncoder::supportedSamplerates(void)
{
	static const unsigned int supportedRates[] = {48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, NULL};
	return supportedRates;
}

const unsigned int *DCAEncoder::supportedBitdepths(void)
{
	static const unsigned int supportedBPS[] = {16, 32, NULL};
	return supportedBPS;
}

const AbstractEncoderInfo *DCAEncoder::getEncoderInfo(void)
{
	return &g_dcaEncoderInfo;
}
