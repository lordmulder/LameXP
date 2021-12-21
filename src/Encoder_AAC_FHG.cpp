///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2021 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Encoder_AAC_FHG.h"

#include "Global.h"
#include "Model_Settings.h"

#include <math.h>
#include <QProcess>
#include <QDir>

static int index2bitrate(const int index)
{
	return (index < 32) ? ((index + 1) * 8) : ((index - 15) * 16);
}

///////////////////////////////////////////////////////////////////////////////
// Encoder Info
///////////////////////////////////////////////////////////////////////////////

class FHGAACEncoderInfo : public AbstractEncoderInfo
{
	virtual bool isModeSupported(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
		case SettingsModel::CBRMode:
			return true;
			break;
		case SettingsModel::ABRMode:
			return false;
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
			return 6;
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return 52;
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
			return qBound(1, index + 1, 6);
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return qBound(8, index2bitrate(index), 576);
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
		static const char* s_description = "fhgaacenc/Winamp (\x0C2\x0A9 Nullsoft)";
		return s_description;
	}

	virtual const char *extension(void) const
	{
		static const char* s_extension = "mp4";
		return s_extension;
	}

	virtual bool isResamplingSupported(void) const
	{
		return false;
	}
}
static const g_fhgAacEncoderInfo;

///////////////////////////////////////////////////////////////////////////////
// Encoder implementation
///////////////////////////////////////////////////////////////////////////////

FHGAACEncoder::FHGAACEncoder(void)
:
	m_binary_enc(lamexp_tools_lookup(L1S("fhgaacenc.exe"))),
	m_binary_dll(lamexp_tools_lookup(L1S("enc_fhgaac.dll")))
{
	if(m_binary_enc.isEmpty() || m_binary_dll.isEmpty())
	{
		MUTILS_THROW("Error initializing FhgAacEnc. Tool 'fhgaacenc.exe' is not registred!");
	}

	m_configProfile = 0;
}

FHGAACEncoder::~FHGAACEncoder(void)
{
}

bool FHGAACEncoder::encode(const QString &sourceFile, const AudioFileModel_MetaInfo& /*metaInfo*/, const unsigned int /*duration*/, const unsigned int /*channels*/, const QString &outputFile, QAtomicInt &abortFlag)
{
	QProcess process;
	QStringList args;
	
	int maxBitrate = 576;

	if(m_configRCMode == SettingsModel::CBRMode)
	{
		switch(m_configProfile)
		{
		case 1:
			args << L1S("--profile") << L1S("lc"); //Forces use of LC AAC profile
			break;
		case 2:
			maxBitrate = 128;
			args << L1S("--profile") << L1S("he"); //Forces use of HE AAC profile
			break;
		case 3:
			maxBitrate = 56;
			args << L1S("--profile") << L1S("hev2"); //Forces use of HEv2 AAC profile
			break;
		}
	}

	switch(m_configRCMode)
	{
	case SettingsModel::CBRMode:
		args << L1S("--cbr") << QString::number(qBound(8, index2bitrate(m_configBitrate), maxBitrate));
		break;
	case SettingsModel::VBRMode:
		args << L1S("--vbr") << QString::number(qBound(1, m_configBitrate + 1, 6));
		break;
	default:
		MUTILS_THROW("Bad rate-control mode!");
		break;
	}

	//args << "--dll" << m_binary_dll;

	if(!m_configCustomParams.isEmpty()) args << m_configCustomParams.split(" ", QString::SkipEmptyParts);

	args << QDir::toNativeSeparators(sourceFile);
	args << QDir::toNativeSeparators(outputFile);

	if(!startProcess(process, m_binary_enc, args))
	{
		return false;
	}

	int prevProgress = -1;
	QRegExp regExp(L1S("Progress:\\s*(\\d+)%"));

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

bool FHGAACEncoder::isFormatSupported(const QString &containerType, const QString& /*containerProfile*/, const QString &formatType, const QString& /*formatProfile*/, const QString& /*formatVersion*/)
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

const unsigned int *FHGAACEncoder::supportedChannelCount(void)
{
	static const unsigned int supportedChannels[] = {1, 2, 4, 5, 6, NULL};
	return supportedChannels;
}

const unsigned int *FHGAACEncoder::supportedSamplerates(void)
{
	static const unsigned int supportedRates[] = {192000, 96000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 6000, NULL};
	return supportedRates;
}

const unsigned int *FHGAACEncoder::supportedBitdepths(void)
{
	static const unsigned int supportedBPS[] = {16, 24, NULL};
	return supportedBPS;
}

void FHGAACEncoder::setProfile(int profile)
{
	m_configProfile = profile;
}

const AbstractEncoderInfo *FHGAACEncoder::getEncoderInfo(void)
{
	return &g_fhgAacEncoderInfo;
}
