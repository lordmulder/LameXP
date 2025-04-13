///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2025 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Registry_Encoder.h"

#include "Global.h"
#include "Model_Settings.h"
#include "Encoder_AAC.h"
#include "Encoder_AAC_FHG.h"
#include "Encoder_AAC_FDK.h"
#include "Encoder_AAC_QAAC.h"
#include "Encoder_AC3.h"
#include "Encoder_DCA.h"
#include "Encoder_FLAC.h"
#include "Encoder_MP3.h"
#include "Encoder_Vorbis.h"
#include "Encoder_Opus.h"
#include "Encoder_MAC.h"
#include "Encoder_Wave.h"

#define IS_VBR(RC_MODE) ((RC_MODE) == SettingsModel::VBRMode)
#define IS_ABR(RC_MODE) ((RC_MODE) == SettingsModel::ABRMode)
#define IS_CBR(RC_MODE) ((RC_MODE) == SettingsModel::CBRMode)

////////////////////////////////////////////////////////////
// Create encoder instance
////////////////////////////////////////////////////////////

AbstractEncoder *EncoderRegistry::createInstance(const int encoderId, const SettingsModel *settings)
{
	int rcMode = -1;
	AbstractEncoder *encoder =  NULL;

	//Create new encoder instance and apply encoder-specific settings
	switch(encoderId)
	{
	/*-------- MP3Encoder /*--------*/
	case SettingsModel::MP3Encoder:
		{
			MP3Encoder *const mp3Encoder = new MP3Encoder();
			mp3Encoder->setAlgoQuality(settings->lameAlgoQuality());
			if(settings->bitrateManagementEnabled())
			{
				mp3Encoder->setBitrateLimits(settings->bitrateManagementMinRate(), settings->bitrateManagementMaxRate());
			}
			mp3Encoder->setChannelMode(settings->lameChannelMode());
			encoder = mp3Encoder;
		}
		break;
	/*-------- VorbisEncoder /*--------*/
	case SettingsModel::VorbisEncoder:
		{
			VorbisEncoder *const vorbisEncoder = new VorbisEncoder();
			if(settings->bitrateManagementEnabled())
			{
				vorbisEncoder->setBitrateLimits(settings->bitrateManagementMinRate(), settings->bitrateManagementMaxRate());
			}
			encoder = vorbisEncoder;
		}
		break;
	/*-------- AACEncoder /*--------*/
	case SettingsModel::AACEncoder:
		{
			switch(getAacEncoder())
			{
			case SettingsModel::AAC_ENCODER_QAAC:
				{
					QAACEncoder *const aacEncoder = new QAACEncoder();
					aacEncoder->setProfile(settings->aacEncProfile());
					aacEncoder->setAlgoQuality(settings->lameAlgoQuality());
					encoder = aacEncoder;
				}
				break;
			case SettingsModel::AAC_ENCODER_FHG:
				{
					FHGAACEncoder *const aacEncoder = new FHGAACEncoder();
					aacEncoder->setProfile(settings->aacEncProfile());
					encoder = aacEncoder;
				}
				break;
			case SettingsModel::AAC_ENCODER_FDK:
				{
					FDKAACEncoder *const aacEncoder = new FDKAACEncoder();
					aacEncoder->setProfile(settings->aacEncProfile());
					encoder = aacEncoder;
				}
				break;
			case SettingsModel::AAC_ENCODER_NERO:
				{
					AACEncoder *const aacEncoder = new AACEncoder();
					aacEncoder->setEnable2Pass(settings->neroAACEnable2Pass());
					aacEncoder->setProfile(settings->aacEncProfile());
					encoder = aacEncoder;
				}
				break;
			default:
				MUTILS_THROW("makeEncoder(): Unknown AAC encoder specified!");
				break;
			}
		}
		break;
	/*-------- AC3Encoder /*--------*/
	case SettingsModel::AC3Encoder:
		{
			AC3Encoder *const ac3Encoder = new AC3Encoder();
			ac3Encoder->setAudioCodingMode(settings->aftenAudioCodingMode());
			ac3Encoder->setDynamicRangeCompression(settings->aftenDynamicRangeCompression());
			ac3Encoder->setExponentSearchSize(settings->aftenExponentSearchSize());
			ac3Encoder->setFastBitAllocation(settings->aftenFastBitAllocation());
			encoder = ac3Encoder;
		}
		break;
	/*-------- FLACEncoder /*--------*/
	case SettingsModel::FLACEncoder:
		{
			FLACEncoder *const flacEncoder = new FLACEncoder();
			encoder = flacEncoder;
		}
		break;
	/*-------- OpusEncoder --------*/
	case SettingsModel::OpusEncoder:
		{
			OpusEncoder *const opusEncoder = new OpusEncoder();
			opusEncoder->setOptimizeFor(settings->opusOptimizeFor());
			opusEncoder->setEncodeComplexity(settings->opusComplexity());
			opusEncoder->setFrameSize(settings->opusFramesize());
			encoder = opusEncoder;
		}
		break;
	/*-------- DCAEncoder --------*/
	case SettingsModel::DCAEncoder:
		{
			DCAEncoder *const dcaEncoder = new DCAEncoder();
			encoder = dcaEncoder;
		}
		break;
	/*-------- MACEncoder --------*/
	case SettingsModel::MACEncoder:
		{
			MACEncoder *const macEncoder = new MACEncoder();
			encoder = macEncoder;
		}
		break;
	/*-------- PCMEncoder --------*/
	case SettingsModel::PCMEncoder:
		{
			WaveEncoder *const waveEncoder = new WaveEncoder();
			encoder = waveEncoder;
		}
		break;
	/*-------- default --------*/
	default:
		MUTILS_THROW("Unsupported encoder!");
	}

	//Sanity checking
	if(!encoder)
	{
		MUTILS_THROW("No encoder instance has been assigend!");
	}

	//Apply common settings
	encoder->setRCMode(rcMode = loadEncoderMode(settings, encoderId));
	encoder->setCustomParams(loadEncoderCustomParams(settings, encoderId));
	encoder->setBitrate(loadEncoderValue(settings, encoderId, rcMode));

	return encoder;
}

////////////////////////////////////////////////////////////
// Get encoder info
////////////////////////////////////////////////////////////

const AbstractEncoderInfo *EncoderRegistry::getEncoderInfo(const int encoderId)
{
	const AbstractEncoderInfo *info = NULL;

	switch(encoderId)
	{
		case SettingsModel::MP3Encoder:    info = MP3Encoder   ::getEncoderInfo(); break;
		case SettingsModel::VorbisEncoder: info = VorbisEncoder::getEncoderInfo(); break;
		case SettingsModel::AC3Encoder:    info = AC3Encoder   ::getEncoderInfo(); break;
		case SettingsModel::FLACEncoder:   info = FLACEncoder  ::getEncoderInfo(); break;
		case SettingsModel::OpusEncoder:   info = OpusEncoder  ::getEncoderInfo(); break;
		case SettingsModel::DCAEncoder:    info = DCAEncoder   ::getEncoderInfo(); break;
		case SettingsModel::MACEncoder:    info = MACEncoder   ::getEncoderInfo(); break;
		case SettingsModel::PCMEncoder:    info = WaveEncoder  ::getEncoderInfo(); break;
		case SettingsModel::AACEncoder:
			switch(getAacEncoder())
			{
				case SettingsModel::AAC_ENCODER_QAAC: info = QAACEncoder  ::getEncoderInfo(); break;
				case SettingsModel::AAC_ENCODER_FHG:  info = FHGAACEncoder::getEncoderInfo(); break;
				case SettingsModel::AAC_ENCODER_FDK:  info = FDKAACEncoder::getEncoderInfo(); break;
				case SettingsModel::AAC_ENCODER_NERO: info = AACEncoder   ::getEncoderInfo(); break;
				default: MUTILS_THROW("Unknown AAC encoder specified!");
			}
			break;
		default: MUTILS_THROW("Unsupported encoder!");
	}

	//Sanity checking
	if(!info)
	{
		MUTILS_THROW("No encoder instance has been assigend!");
	}

	return info;
}

////////////////////////////////////////////////////////////
// Load/store encoder RC mode
////////////////////////////////////////////////////////////

#define STORE_MODE(ENCODER_ID, RC_MODE) do \
{ \
	settings->compressionRCMode##ENCODER_ID(RC_MODE); \
} \
while(0)

#define LOAD_MODE(RC_MODE, ENCODER_ID) do \
{ \
	(RC_MODE) = settings->compressionRCMode##ENCODER_ID(); \
} \
while(0)

void EncoderRegistry::saveEncoderMode(SettingsModel *settings, const int encoderId, const int rcMode)
{
	//Sanity checking
	if((rcMode < SettingsModel::VBRMode) || (rcMode > SettingsModel::CBRMode))
	{
		MUTILS_THROW("Unknown rate-control mode!");
	}

	//Store the encoder bitrate/quality value
	switch(encoderId)
	{
		case SettingsModel::MP3Encoder:    STORE_MODE(LAME,    rcMode); break;
		case SettingsModel::VorbisEncoder: STORE_MODE(OggEnc,  rcMode); break;
		case SettingsModel::AACEncoder:    STORE_MODE(AacEnc,  rcMode); break;
		case SettingsModel::AC3Encoder:    STORE_MODE(Aften,   rcMode); break;
		case SettingsModel::FLACEncoder:   STORE_MODE(FLAC,    rcMode); break;
		case SettingsModel::OpusEncoder:   STORE_MODE(OpusEnc, rcMode); break;
		case SettingsModel::DCAEncoder:    STORE_MODE(DcaEnc,  rcMode); break;
		case SettingsModel::MACEncoder:    STORE_MODE(MacEnc,  rcMode); break;
		case SettingsModel::PCMEncoder:    STORE_MODE(Wave,    rcMode); break;
		default: MUTILS_THROW("Unsupported encoder!");
	}
}

int EncoderRegistry::loadEncoderMode(const SettingsModel *settings, const int encoderId)
{
	int rcMode = -1;
	
	//Store the encoder bitrate/quality value
	switch(encoderId)
	{
		case SettingsModel::MP3Encoder:    LOAD_MODE(rcMode, LAME);    break;
		case SettingsModel::VorbisEncoder: LOAD_MODE(rcMode, OggEnc);  break;
		case SettingsModel::AACEncoder:    LOAD_MODE(rcMode, AacEnc);  break;
		case SettingsModel::AC3Encoder:    LOAD_MODE(rcMode, Aften);   break;
		case SettingsModel::FLACEncoder:   LOAD_MODE(rcMode, FLAC);    break;
		case SettingsModel::OpusEncoder:   LOAD_MODE(rcMode, OpusEnc); break;
		case SettingsModel::DCAEncoder:    LOAD_MODE(rcMode, DcaEnc);  break;
		case SettingsModel::MACEncoder:    LOAD_MODE(rcMode, MacEnc);  break;
		case SettingsModel::PCMEncoder:    LOAD_MODE(rcMode, Wave);    break;
		default: MUTILS_THROW("Unsupported encoder!");
	}

	return rcMode;
}

////////////////////////////////////////////////////////////
// Load/store encoder bitrate/quality value
////////////////////////////////////////////////////////////

#define STORE_VALUE(ENCODER_ID, RC_MODE, VALUE) do \
{ \
	if(IS_VBR(RC_MODE)) settings->compressionVbrQuality##ENCODER_ID(VALUE); \
	if(IS_ABR(RC_MODE)) settings->compressionAbrBitrate##ENCODER_ID(VALUE); \
	if(IS_CBR(RC_MODE)) settings->compressionCbrBitrate##ENCODER_ID(VALUE); \
} \
while(0)

#define LOAD_VALUE(VALUE, ENCODER_ID, RC_MODE) do \
{ \
	if(IS_VBR(RC_MODE)) (VALUE) = settings->compressionVbrQuality##ENCODER_ID(); \
	if(IS_ABR(RC_MODE)) (VALUE) = settings->compressionAbrBitrate##ENCODER_ID(); \
	if(IS_CBR(RC_MODE)) (VALUE) = settings->compressionCbrBitrate##ENCODER_ID(); \
} \
while(0)

void EncoderRegistry::saveEncoderValue(SettingsModel *settings, const int encoderId, const int rcMode, const int value)
{
	//Sanity checking
	if((rcMode < SettingsModel::VBRMode) || (rcMode > SettingsModel::CBRMode))
	{
		MUTILS_THROW("Unknown rate-control mode!");
	}

	//Store the encoder bitrate/quality value
	switch(encoderId)
	{
		case SettingsModel::MP3Encoder:    STORE_VALUE(LAME,    rcMode, value); break;
		case SettingsModel::VorbisEncoder: STORE_VALUE(OggEnc,  rcMode, value); break;
		case SettingsModel::AACEncoder:    STORE_VALUE(AacEnc,  rcMode, value); break;
		case SettingsModel::AC3Encoder:    STORE_VALUE(Aften,   rcMode, value); break;
		case SettingsModel::FLACEncoder:   STORE_VALUE(FLAC,    rcMode, value); break;
		case SettingsModel::OpusEncoder:   STORE_VALUE(OpusEnc, rcMode, value); break;
		case SettingsModel::DCAEncoder:    STORE_VALUE(DcaEnc,  rcMode, value); break;
		case SettingsModel::MACEncoder:    STORE_VALUE(MacEnc,  rcMode, value); break;
		case SettingsModel::PCMEncoder:    STORE_VALUE(Wave,    rcMode, value); break;
		default: MUTILS_THROW("Unsupported encoder!");
	}
}

int EncoderRegistry::loadEncoderValue(const SettingsModel *settings, const int encoderId, const int rcMode)
{
	int value = INT_MAX;
	
	//Sanity checking
	if((rcMode < SettingsModel::VBRMode) || (rcMode > SettingsModel::CBRMode))
	{
		MUTILS_THROW("Unknown rate-control mode!");
	}

	//Load the encoder bitrate/quality value
	switch(encoderId)
	{
		case SettingsModel::MP3Encoder:    LOAD_VALUE(value, LAME,    rcMode); break;
		case SettingsModel::VorbisEncoder: LOAD_VALUE(value, OggEnc,  rcMode); break;
		case SettingsModel::AACEncoder:    LOAD_VALUE(value, AacEnc,  rcMode); break;
		case SettingsModel::AC3Encoder:    LOAD_VALUE(value, Aften,   rcMode); break;
		case SettingsModel::FLACEncoder:   LOAD_VALUE(value, FLAC,    rcMode); break;
		case SettingsModel::OpusEncoder:   LOAD_VALUE(value, OpusEnc, rcMode); break;
		case SettingsModel::DCAEncoder:    LOAD_VALUE(value, DcaEnc,  rcMode); break;
		case SettingsModel::MACEncoder:    LOAD_VALUE(value, MacEnc,  rcMode); break;
		case SettingsModel::PCMEncoder:    LOAD_VALUE(value, Wave,    rcMode); break;
		default: MUTILS_THROW("Unsupported encoder!");
	}

	return value;
}

////////////////////////////////////////////////////////////
// Load/store encoder custom parameters
////////////////////////////////////////////////////////////

#define STORE_PARAMS(ENCODER_ID, PARAMS) do \
{ \
	settings->customParameters##ENCODER_ID(PARAMS); \
} \
while(0)

#define LOAD_PARAMS(PARAMS, ENCODER_ID) do \
{ \
	(PARAMS) = settings->customParameters##ENCODER_ID(); \
} \
while(0)

void EncoderRegistry::saveEncoderCustomParams(SettingsModel *settings, const int encoderId, const QString &params)
{
	//Store the encoder bitrate/quality value
	switch(encoderId)
	{
		case SettingsModel::MP3Encoder:    STORE_PARAMS(LAME,    params.trimmed()); break;
		case SettingsModel::VorbisEncoder: STORE_PARAMS(OggEnc,  params.trimmed()); break;
		case SettingsModel::AACEncoder:    STORE_PARAMS(AacEnc,  params.trimmed()); break;
		case SettingsModel::AC3Encoder:    STORE_PARAMS(Aften,   params.trimmed()); break;
		case SettingsModel::FLACEncoder:   STORE_PARAMS(FLAC,    params.trimmed()); break;
		case SettingsModel::OpusEncoder:   STORE_PARAMS(OpusEnc, params.trimmed()); break;
		case SettingsModel::DCAEncoder:    STORE_PARAMS(DcaEnc,  params.trimmed()); break;
		case SettingsModel::MACEncoder:    STORE_PARAMS(MacEnc,  params.trimmed()); break;
		case SettingsModel::PCMEncoder:    STORE_PARAMS(Wave,    params.trimmed()); break;
		default: MUTILS_THROW("Unsupported encoder!");
	}
}

QString EncoderRegistry::loadEncoderCustomParams(const SettingsModel *settings, const int encoderId)
{
	QString params;
	
	//Load the encoder bitrate/quality value
	switch(encoderId)
	{
		case SettingsModel::MP3Encoder:    LOAD_PARAMS(params, LAME);    break;
		case SettingsModel::VorbisEncoder: LOAD_PARAMS(params, OggEnc);  break;
		case SettingsModel::AACEncoder:    LOAD_PARAMS(params, AacEnc);  break;
		case SettingsModel::AC3Encoder:    LOAD_PARAMS(params, Aften);   break;
		case SettingsModel::FLACEncoder:   LOAD_PARAMS(params, FLAC);    break;
		case SettingsModel::OpusEncoder:   LOAD_PARAMS(params, OpusEnc); break;
		case SettingsModel::DCAEncoder:    LOAD_PARAMS(params, DcaEnc);  break;
		case SettingsModel::MACEncoder:    LOAD_PARAMS(params, MacEnc);  break;
		case SettingsModel::PCMEncoder:    LOAD_PARAMS(params, Wave);    break;
		default: MUTILS_THROW("Unsupported encoder!");
	}

	return params;
}

////////////////////////////////////////////////////////////
// Reset encoder settings
////////////////////////////////////////////////////////////

#define RESET_SETTING(OBJ,NAME) do \
{ \
	(OBJ)->NAME((OBJ)->NAME##Default()); \
} \
while(0)

void EncoderRegistry::resetAllEncoders(SettingsModel *settings)
{
	RESET_SETTING(settings, compressionAbrBitrateAacEnc);
	RESET_SETTING(settings, compressionAbrBitrateAften);
	RESET_SETTING(settings, compressionAbrBitrateDcaEnc);
	RESET_SETTING(settings, compressionAbrBitrateFLAC);
	RESET_SETTING(settings, compressionAbrBitrateLAME);
	RESET_SETTING(settings, compressionAbrBitrateMacEnc);
	RESET_SETTING(settings, compressionAbrBitrateOggEnc);
	RESET_SETTING(settings, compressionAbrBitrateOpusEnc);
	RESET_SETTING(settings, compressionAbrBitrateWave);

	RESET_SETTING(settings, compressionCbrBitrateAacEnc);
	RESET_SETTING(settings, compressionCbrBitrateAften);
	RESET_SETTING(settings, compressionCbrBitrateDcaEnc);
	RESET_SETTING(settings, compressionCbrBitrateFLAC);
	RESET_SETTING(settings, compressionCbrBitrateLAME);
	RESET_SETTING(settings, compressionCbrBitrateMacEnc);
	RESET_SETTING(settings, compressionCbrBitrateOggEnc);
	RESET_SETTING(settings, compressionCbrBitrateOpusEnc);
	RESET_SETTING(settings, compressionCbrBitrateWave);

	RESET_SETTING(settings, compressionRCModeAacEnc);
	RESET_SETTING(settings, compressionRCModeAften);
	RESET_SETTING(settings, compressionRCModeDcaEnc);
	RESET_SETTING(settings, compressionRCModeFLAC);
	RESET_SETTING(settings, compressionRCModeLAME);
	RESET_SETTING(settings, compressionRCModeMacEnc);
	RESET_SETTING(settings, compressionRCModeOggEnc);
	RESET_SETTING(settings, compressionRCModeOpusEnc);
	RESET_SETTING(settings, compressionRCModeWave);

	RESET_SETTING(settings, compressionVbrQualityAacEnc);
	RESET_SETTING(settings, compressionVbrQualityAften);
	RESET_SETTING(settings, compressionVbrQualityDcaEnc);
	RESET_SETTING(settings, compressionVbrQualityFLAC);
	RESET_SETTING(settings, compressionVbrQualityLAME);
	RESET_SETTING(settings, compressionVbrQualityMacEnc);
	RESET_SETTING(settings, compressionVbrQualityOggEnc);
	RESET_SETTING(settings, compressionVbrQualityOpusEnc);
	RESET_SETTING(settings, compressionVbrQualityWave);
}

////////////////////////////////////////////////////////////
// Get File Extensions
////////////////////////////////////////////////////////////

QStringList EncoderRegistry::getOutputFileExtensions(void)
{
	QStringList list;
	for(int encoderId = SettingsModel::MP3Encoder; encoderId < SettingsModel::ENCODER_COUNT; encoderId++)
	{
		if((encoderId == SettingsModel::AACEncoder) && (getAacEncoder() == SettingsModel::AAC_ENCODER_NONE))
		{
			continue;
		}
		list << QString::fromLatin1(getEncoderInfo(encoderId)->extension());
	}
	return list;
}

////////////////////////////////////////////////////////////
// Static Functions
////////////////////////////////////////////////////////////

int EncoderRegistry::getAacEncoder(void)
{
	if(lamexp_tools_check("qaac.exe") && lamexp_tools_check("libsoxr.dll") && lamexp_tools_check("libsoxconvolver.dll"))
	{
		return SettingsModel::AAC_ENCODER_QAAC;
	}
	else if(lamexp_tools_check("qaac64.exe") && lamexp_tools_check("libsoxr64.dll") && lamexp_tools_check("libsoxconvolver64.dll"))
	{
		return SettingsModel::AAC_ENCODER_QAAC;
	}
	else if(lamexp_tools_check("fdkaac.exe"))
	{
		return SettingsModel::AAC_ENCODER_FDK;
	}
	else if(lamexp_tools_check("fhgaacenc.exe") && lamexp_tools_check("enc_fhgaac.dll") && lamexp_tools_check("nsutil.dll") && lamexp_tools_check("libmp4v2.dll"))
	{
		return SettingsModel::AAC_ENCODER_FHG;
	}
	else if(lamexp_tools_check("neroAacEnc.exe") && lamexp_tools_check("neroAacDec.exe") && lamexp_tools_check("neroAacTag.exe"))
	{
		return SettingsModel::AAC_ENCODER_NERO;
	}
	else
	{
		return SettingsModel::AAC_ENCODER_NONE;
	}
}
