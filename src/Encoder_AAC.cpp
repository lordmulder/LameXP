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

#include "Encoder_AAC.h"

#include "Global.h"
#include "Model_Settings.h"

#include <QProcess>
#include <QDir>

static int index2bitrate(const int index)
{
	return (index < 32) ? ((index + 1) * 8) : ((index - 15) * 16);
}

#define IS_VALID(X) (((X) != 0U) && ((X) != UINT_MAX))

///////////////////////////////////////////////////////////////////////////////
// Encoder Info
///////////////////////////////////////////////////////////////////////////////

class AACEncoderInfo : public AbstractEncoderInfo
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
			return 21;
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return 41;
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
			return qBound(0, index * 5, 100);
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return qBound(8, index2bitrate(index), 400);
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
			return TYPE_QUALITY_LEVEL_FLT;
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
		static const char* s_description = "Nero AAC Encoder (\x0C2\x0A9 Nero AG)";
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
static const g_aacEncoderInfo;

///////////////////////////////////////////////////////////////////////////////
// Encoder implementation
///////////////////////////////////////////////////////////////////////////////

AACEncoder::AACEncoder(void)
:
	m_binary_enc(lamexp_tools_lookup(L1S("neroAacEnc.exe"))),
	m_binary_tag(lamexp_tools_lookup(L1S("neroAacTag.exe")))
{
	if(m_binary_enc.isEmpty() || m_binary_tag.isEmpty())
	{
		MUTILS_THROW("Error initializing AAC encoder. Tool 'neroAacEnc.exe' is not registred!");
	}

	m_configProfile = 0;
	m_configEnable2Pass = true;
}

AACEncoder::~AACEncoder(void)
{
}

bool AACEncoder::encode(const QString &sourceFile, const AudioFileModel_MetaInfo &metaInfo, const unsigned int duration, const unsigned int channels, const QString &outputFile, QAtomicInt &abortFlag)
{
	QProcess process;
	QStringList args;
	const QString baseName = QFileInfo(outputFile).fileName();

	switch(m_configRCMode)
	{
	case SettingsModel::VBRMode:
		args << L1S("-q") << QString().sprintf("%.2f", double(qBound(0, m_configBitrate * 5, 100)) / 100.0);
		break;
	case SettingsModel::ABRMode:
		args << L1S("-br") << QString::number(qBound(8, index2bitrate(m_configBitrate), 400) * 1000);
		break;
	case SettingsModel::CBRMode:
		args << L1S("-cbr") << QString::number(qBound(8, index2bitrate(m_configBitrate), 400) * 1000);
		break;
	default:
		MUTILS_THROW("Bad rate-control mode!");
		break;
	}

	if(m_configEnable2Pass && (m_configRCMode == SettingsModel::ABRMode))
	{
		args << L1S("-2pass");
	}
	
	int selectedProfile = m_configProfile;
	if ((selectedProfile == 3) && IS_VALID(channels) && (channels != 2))
	{
		emit messageLogged("WARNING: Cannot use HE-AAC v2 (SBR+PS) with Mono input --> reverting to HE-AAC (SBR)");
		selectedProfile = 2;
	}

	switch(selectedProfile)
	{
	case 0:
		//Do *not* overwrite profile -> let the encoder decide!
		break;
	case 1:
		args << L1S("-lc"); //Forces use of LC AAC profile
		break;
	case 2:
		args << L1S("-he"); //Forces use of HE AAC profile
		break;
	case 3:
		args << L1S("-hev2"); //Forces use of HEv2 AAC profile
		break;
	default:
		MUTILS_THROW("Bad AAC Profile specified!");
	}

	if(!m_configCustomParams.isEmpty()) args << m_configCustomParams.split(" ", QString::SkipEmptyParts);

	args << L1S("-if") << QDir::toNativeSeparators(sourceFile);
	args << L1S("-of") << QDir::toNativeSeparators(outputFile);

	if(!startProcess(process, m_binary_enc, args))
	{
		return false;
	}

	int prevProgress = -1;
	QRegExp regExp_sp(L1S("\\bprocessed\\s+(\\d+)\\s+seconds"), Qt::CaseInsensitive);
	QRegExp regExp_mp(L1S("\\b(\\w+)\\s+pass:\\s+processed\\s+(\\d+)\\s+seconds"), Qt::CaseInsensitive);

	const result_t result = awaitProcess(process, abortFlag, [this, &prevProgress, &duration, &regExp_sp, &regExp_mp](const QString &text)
	{
		if (regExp_mp.lastIndexIn(text) >= 0)
		{
			int timeElapsed;
			if ((duration > 0) && MUtils::regexp_parse_int32(regExp_mp, timeElapsed, 2))
			{
				const bool second_pass = (regExp_mp.cap(1).compare(L1S("second"), Qt::CaseInsensitive) == 0);
				const int newProgress = qRound((second_pass ? 50.0 : 0.0) + ((static_cast<double>(timeElapsed) / static_cast<double>(duration)) * 50.0));
				if (newProgress > prevProgress)
				{
					emit statusUpdated(newProgress);
					prevProgress = NEXT_PROGRESS(newProgress);
				}
			}
			return true;
		}
		if (regExp_sp.lastIndexIn(text) >= 0)
		{
			int timeElapsed;
			if ((duration > 0) && MUtils::regexp_parse_int32(regExp_sp, timeElapsed))
			{
				const int newProgress = qRound((static_cast<double>(timeElapsed) / static_cast<double>(duration)) * 100.0);
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

	if(result != RESULT_SUCCESS)
	{
		return false;
	}

	if(metaInfo.empty(false))
	{
		return true;
	}

	emit messageLogged(L1S("\n-------------------------------\n"));
	
	args.clear();
	args << QDir::toNativeSeparators(outputFile);

	if(!metaInfo.title().isEmpty())   args << QString("-meta:title=%1").arg(cleanTag(metaInfo.title()));
	if(!metaInfo.artist().isEmpty())  args << QString("-meta:artist=%1").arg(cleanTag(metaInfo.artist()));
	if(!metaInfo.album().isEmpty())   args << QString("-meta:album=%1").arg(cleanTag(metaInfo.album()));
	if(!metaInfo.genre().isEmpty())   args << QString("-meta:genre=%1").arg(cleanTag(metaInfo.genre()));
	if(!metaInfo.comment().isEmpty()) args << QString("-meta:comment=%1").arg(cleanTag(metaInfo.comment()));
	if(metaInfo.year())               args << QString("-meta:year=%1").arg(QString::number(metaInfo.year()));
	if(metaInfo.position())           args << QString("-meta:track=%1").arg(QString::number(metaInfo.position()));
	if(!metaInfo.cover().isEmpty())   args << QString("-add-cover:%1:%2").arg("front", metaInfo.cover());
	
	if(!startProcess(process, m_binary_tag, args))
	{
		return false;
	}

	return (awaitProcess(process, abortFlag) == RESULT_SUCCESS);
}

bool AACEncoder::isFormatSupported(const QString &containerType, const QString& /*containerProfile*/, const QString &formatType, const QString& /*formatProfile*/, const QString& /*formatVersion*/)
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

const unsigned int *AACEncoder::supportedSamplerates(void)
{
	static const unsigned int supportedRates[] = { 96000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, NULL};
	return supportedRates;
}

void AACEncoder::setProfile(int profile)
{
	m_configProfile = qBound(0, profile, 3);
}

void AACEncoder::setEnable2Pass(bool enabled)
{
	m_configEnable2Pass = enabled;
}

const bool AACEncoder::needsTimingInfo(void)
{
	return true;
}

const AbstractEncoderInfo *AACEncoder::getEncoderInfo(void)
{
	return &g_aacEncoderInfo;
}
