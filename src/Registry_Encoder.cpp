///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2013 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Registry_Encoder.h"

#include "Model_Settings.h"
#include "Encoder_AAC.h"
#include "Encoder_AAC_FHG.h"
#include "Encoder_AAC_QAAC.h"
#include "Encoder_AC3.h"
#include "Encoder_DCA.h"
#include "Encoder_FLAC.h"
#include "Encoder_MP3.h"
#include "Encoder_Vorbis.h"
#include "Encoder_Opus.h"
#include "Encoder_Wave.h"

#define IS_VBR(RC_MODE) ((RC_MODE) == SettingsModel::VBRMode)

////////////////////////////////////////////////////////////
// Create encoder instance
////////////////////////////////////////////////////////////

AbstractEncoder *EncoderRegistry::createInstance(const int encoderId, const SettingsModel *settings, bool *nativeResampling)
{
	int rcMode = -1;
	AbstractEncoder *encoder =  NULL;
	*nativeResampling = false;

	//Sanity checking
	if((rcMode < SettingsModel::VBRMode) || (rcMode > SettingsModel::CBRMode))
	{
		throw "Unknown rate-control mode!";
	}

	//Create new encoder instance
	switch(encoderId)
	{
	/*-------- MP3Encoder /*--------*/
	case SettingsModel::MP3Encoder:
		{
			MP3Encoder *mp3Encoder = new MP3Encoder();
			mp3Encoder->setRCMode(rcMode = settings->compressionRCModeLAME());
			mp3Encoder->setBitrate(IS_VBR(rcMode) ? settings->compressionVbrLevelLAME() : settings->compressionBitrateLAME());
			mp3Encoder->setAlgoQuality(settings->lameAlgoQuality());
			if(settings->bitrateManagementEnabled())
			{
				mp3Encoder->setBitrateLimits(settings->bitrateManagementMinRate(), settings->bitrateManagementMaxRate());
			}
			if(settings->samplingRate() > 0)
			{
				mp3Encoder->setSamplingRate(SettingsModel::samplingRates[settings->samplingRate()]);
				*nativeResampling = true;
			}
			mp3Encoder->setChannelMode(settings->lameChannelMode());
			mp3Encoder->setCustomParams(settings->customParametersLAME());
			encoder = mp3Encoder;
		}
		break;
	/*-------- VorbisEncoder /*--------*/
	case SettingsModel::VorbisEncoder:
		{
			VorbisEncoder *vorbisEncoder = new VorbisEncoder();
			vorbisEncoder->setRCMode(rcMode = settings->compressionRCModeOggEnc());
			vorbisEncoder->setBitrate(IS_VBR(rcMode) ? settings->compressionVbrLevelOggEnc() : settings->compressionBitrateOggEnc());
			if(settings->bitrateManagementEnabled())
			{
				vorbisEncoder->setBitrateLimits(settings->bitrateManagementMinRate(), settings->bitrateManagementMaxRate());
			}
			if(settings->samplingRate() > 0)
			{
				vorbisEncoder->setSamplingRate(SettingsModel::samplingRates[settings->samplingRate()]);
				*nativeResampling = true;
			}
			vorbisEncoder->setCustomParams(settings->customParametersOggEnc());
			encoder = vorbisEncoder;
		}
		break;
	/*-------- AACEncoder /*--------*/
	case SettingsModel::AACEncoder:
		{
			switch(SettingsModel::getAacEncoder())
			{
			case SettingsModel::AAC_ENCODER_QAAC:
				{
					QAACEncoder *aacEncoder = new QAACEncoder();
					aacEncoder->setRCMode(rcMode = settings->compressionRCModeAacEnc());
					aacEncoder->setBitrate(IS_VBR(rcMode) ? settings->compressionVbrLevelAacEnc() : settings->compressionBitrateAacEnc());
					aacEncoder->setProfile(settings->aacEncProfile());
					aacEncoder->setCustomParams(settings->customParametersAacEnc());
					encoder = aacEncoder;
				}
				break;
			case SettingsModel::AAC_ENCODER_FHG:
				{
					FHGAACEncoder *aacEncoder = new FHGAACEncoder();
					aacEncoder->setRCMode(rcMode = settings->compressionRCModeAacEnc());
					aacEncoder->setBitrate(IS_VBR(rcMode) ? settings->compressionVbrLevelAacEnc() : settings->compressionBitrateAacEnc());
					aacEncoder->setProfile(settings->aacEncProfile());
					aacEncoder->setCustomParams(settings->customParametersAacEnc());
					encoder = aacEncoder;
				}
				break;
			case SettingsModel::AAC_ENCODER_NERO:
				{
					AACEncoder *aacEncoder = new AACEncoder();
					aacEncoder->setRCMode(rcMode = settings->compressionRCModeAacEnc());
					aacEncoder->setBitrate(IS_VBR(rcMode) ? settings->compressionVbrLevelAacEnc() : settings->compressionBitrateAacEnc());
					aacEncoder->setEnable2Pass(settings->neroAACEnable2Pass());
					aacEncoder->setProfile(settings->aacEncProfile());
					aacEncoder->setCustomParams(settings->customParametersAacEnc());
					encoder = aacEncoder;
				}
				break;
			default:
				throw "makeEncoder(): Unknown AAC encoder specified!";
				break;
			}
		}
		break;
	/*-------- AC3Encoder /*--------*/
	case SettingsModel::AC3Encoder:
		{
			AC3Encoder *ac3Encoder = new AC3Encoder();
			ac3Encoder->setRCMode(rcMode = settings->compressionRCModeAften());
			ac3Encoder->setBitrate(IS_VBR(rcMode) ? settings->compressionVbrLevelAften() : settings->compressionBitrateAften());
			ac3Encoder->setCustomParams(settings->customParametersAften());
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
			FLACEncoder *flacEncoder = new FLACEncoder();
			flacEncoder->setBitrate(settings->compressionVbrLevelFLAC());
			flacEncoder->setRCMode(SettingsModel::VBRMode);
			flacEncoder->setCustomParams(settings->customParametersFLAC());
			encoder = flacEncoder;
		}
		break;
	/*-------- OpusEncoder --------*/
	case SettingsModel::OpusEncoder:
		{
			OpusEncoder *opusEncoder = new OpusEncoder();
			opusEncoder->setRCMode(rcMode = settings->compressionRCModeOpusEnc());
			opusEncoder->setBitrate(settings->compressionBitrateOpusEnc()); /*Opus always uses bitrate*/
			opusEncoder->setOptimizeFor(settings->opusOptimizeFor());
			opusEncoder->setEncodeComplexity(settings->opusComplexity());
			opusEncoder->setFrameSize(settings->opusFramesize());
			opusEncoder->setCustomParams(settings->customParametersOpus());
			encoder = opusEncoder;
		}
		break;
	/*-------- DCAEncoder --------*/
	case SettingsModel::DCAEncoder:
		{
			DCAEncoder *dcaEncoder = new DCAEncoder();
			dcaEncoder->setRCMode(SettingsModel::CBRMode);
			dcaEncoder->setBitrate(IS_VBR(rcMode) ? 0 : settings->compressionBitrateDcaEnc());
			encoder = dcaEncoder;
		}
		break;
	/*-------- PCMEncoder --------*/
	case SettingsModel::PCMEncoder:
		{
			WaveEncoder *waveEncoder = new WaveEncoder();
			waveEncoder->setBitrate(0); /*does NOT apply to PCM output*/
			waveEncoder->setRCMode(0); /*does NOT apply to PCM output*/
			encoder = waveEncoder;
		}
		break;
	/*-------- default --------*/
	default:
		throw "Unsupported encoder!";
	}

	//Sanity checking
	if(!encoder)
	{
		throw "No encoder instance has been assigend!";
	}

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
	/*-------- MP3Encoder /*--------*/
	case SettingsModel::MP3Encoder:
		info = MP3Encoder::getEncoderInfo();
		break;
	/*-------- VorbisEncoder /*--------*/
	case SettingsModel::VorbisEncoder:
		info = VorbisEncoder::getEncoderInfo();
		break;
	/*-------- AACEncoder /*--------*/
	case SettingsModel::AACEncoder:
		{
			switch(SettingsModel::getAacEncoder())
			{
			case SettingsModel::AAC_ENCODER_QAAC:
				info = QAACEncoder::getEncoderInfo();
				break;
			case SettingsModel::AAC_ENCODER_FHG:
				info = FHGAACEncoder::getEncoderInfo();
				break;
			case SettingsModel::AAC_ENCODER_NERO:
				info = AACEncoder::getEncoderInfo();
				break;
			default:
				throw "Unknown AAC encoder specified!";
				break;
			}
		}
		break;
	/*-------- AC3Encoder /*--------*/
	case SettingsModel::AC3Encoder:
		info = AC3Encoder::getEncoderInfo();
		break;
	/*-------- FLACEncoder /*--------*/
	case SettingsModel::FLACEncoder:
		info = FLACEncoder::getEncoderInfo();
		break;
	/*-------- OpusEncoder --------*/
	case SettingsModel::OpusEncoder:
		info = OpusEncoder::getEncoderInfo();
		break;
	/*-------- DCAEncoder --------*/
	case SettingsModel::DCAEncoder:
		info = DCAEncoder::getEncoderInfo();
		break;
	/*-------- PCMEncoder --------*/
	case SettingsModel::PCMEncoder:
		info = WaveEncoder::getEncoderInfo();
		break;
	/*-------- default --------*/
	default:
		throw "Unsupported encoder!";
	}

	//Sanity checking
	if(!info)
	{
		throw "No encoder instance has been assigend!";
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
		throw "Unknown rate-control mode!";
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
		case SettingsModel::PCMEncoder:    STORE_MODE(Wave,    rcMode); break;
		default: throw "Unsupported encoder!";
	}
}

int EncoderRegistry::loadEncoderMode(SettingsModel *settings, const int encoderId)
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
		case SettingsModel::PCMEncoder:    LOAD_MODE(rcMode, Wave);    break;
		default: throw "Unsupported encoder!";
	}

	return rcMode;
}

////////////////////////////////////////////////////////////
// Load/store encoder bitrate/quality value
////////////////////////////////////////////////////////////

#define STORE_VALUE(ENCODER_ID, RC_MODE, VALUE) do \
{ \
	if(IS_VBR(RC_MODE)) \
	{ \
		settings->compressionVbrLevel##ENCODER_ID(VALUE); \
	} \
	else \
	{ \
		settings->compressionBitrate##ENCODER_ID(VALUE); \
	} \
} \
while(0)

#define LOAD_VALUE(VALUE, ENCODER_ID, RC_MODE) do \
{ \
	if(IS_VBR(RC_MODE)) \
	{ \
		(VALUE) = settings->compressionVbrLevel##ENCODER_ID(); \
	} \
	else \
	{ \
		(VALUE) = settings->compressionBitrate##ENCODER_ID(); \
	} \
} \
while(0)

void EncoderRegistry::saveEncoderValue(SettingsModel *settings, const int encoderId, const int rcMode, const int value)
{
	//Sanity checking
	if((rcMode < SettingsModel::VBRMode) || (rcMode > SettingsModel::CBRMode))
	{
		throw "Unknown rate-control mode!";
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
		case SettingsModel::PCMEncoder:    STORE_VALUE(Wave,    rcMode, value); break;
		default: throw "Unsupported encoder!";
	}
}

int EncoderRegistry::loadEncoderValue(const SettingsModel *settings, const int encoderId, const int rcMode)
{
	int value = INT_MAX;
	
	//Sanity checking
	if((rcMode < SettingsModel::VBRMode) || (rcMode > SettingsModel::CBRMode))
	{
		throw "Unknown rate-control mode!";
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
		case SettingsModel::PCMEncoder:    LOAD_VALUE(value, Wave,    rcMode); break;
		default: throw "Unsupported encoder!";
	}

	return value;
}
