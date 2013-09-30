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

AbstractEncoder *EncoderRegistry::createInstance(const int encoderId, const SettingsModel *settings, bool *nativeResampling)
{
	int rcMode = -1;
	AbstractEncoder *encoder =  NULL;
	*nativeResampling = false;

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
