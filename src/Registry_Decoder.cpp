///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2014 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version, but always including the *additional*
// restrictions defined in the "License.txt" file.
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

#include "Registry_Decoder.h"

#include "Decoder_AAC.h"
#include "Decoder_AC3.h"
#include "Decoder_ADPCM.h"
#include "Decoder_ALAC.h"
#include "Decoder_Avisynth.h"
#include "Decoder_FLAC.h"
#include "Decoder_MAC.h"
#include "Decoder_MP3.h"
#include "Decoder_Musepack.h"
#include "Decoder_Shorten.h"
#include "Decoder_Speex.h"
#include "Decoder_TTA.h"
#include "Decoder_Vorbis.h"
#include "Decoder_Wave.h"
#include "Decoder_WavPack.h"
#include "Decoder_Opus.h"
#include "Decoder_WMA.h"
#include "PlaylistImporter.h"
#include "Model_Settings.h"

#include <QString>
#include <QStringList>
#include <QRegExp>

#define PROBE_DECODER(DEC) if(DEC::isDecoderAvailable() && DEC::isFormatSupported(containerType, containerProfile, formatType, formatProfile, formatVersion)) { return new DEC(); }
#define GET_FILETYPES(DEC) (DEC::isDecoderAvailable() ? DEC::supportedTypes() : QStringList())

AbstractDecoder *DecoderRegistry::lookup(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	PROBE_DECODER(MP3Decoder);
	PROBE_DECODER(VorbisDecoder);
	PROBE_DECODER(AACDecoder);
	PROBE_DECODER(AC3Decoder);
	PROBE_DECODER(FLACDecoder);
	PROBE_DECODER(WavPackDecoder);
	PROBE_DECODER(MusepackDecoder);
	PROBE_DECODER(ShortenDecoder);
	PROBE_DECODER(MACDecoder);
	PROBE_DECODER(TTADecoder);
	PROBE_DECODER(SpeexDecoder);
	PROBE_DECODER(ALACDecoder);
	PROBE_DECODER(WMADecoder);
	PROBE_DECODER(ADPCMDecoder);
	PROBE_DECODER(WaveDecoder);
	PROBE_DECODER(OpusDecoder);
	PROBE_DECODER(AvisynthDecoder);
	
	return NULL;
}

QStringList DecoderRegistry::getSupportedTypes(void)
{
	QStringList types;

	types << GET_FILETYPES(WaveDecoder);
	types << GET_FILETYPES(MP3Decoder);
	types << GET_FILETYPES(VorbisDecoder);
	types << GET_FILETYPES(AACDecoder);
	types << GET_FILETYPES(AC3Decoder);
	types << GET_FILETYPES(FLACDecoder);
	types << GET_FILETYPES(WavPackDecoder);
	types << GET_FILETYPES(MusepackDecoder);
	types << GET_FILETYPES(ShortenDecoder);
	types << GET_FILETYPES(MACDecoder);
	types << GET_FILETYPES(TTADecoder);
	types << GET_FILETYPES(SpeexDecoder);
	types << GET_FILETYPES(ALACDecoder);
	types << GET_FILETYPES(WMADecoder);
	types << GET_FILETYPES(ADPCMDecoder);
	types << GET_FILETYPES(OpusDecoder);
	types << GET_FILETYPES(AvisynthDecoder);

	QStringList extensions;
	extensions << QString(PlaylistImporter::supportedExtensions).split(" ", QString::SkipEmptyParts);
	QRegExp regExp("\\((.+)\\)", Qt::CaseInsensitive);

	for(int i = 0; i < types.count(); i++)
	{
		if(regExp.lastIndexIn(types.at(i)) >= 0)
		{
			extensions << regExp.cap(1).split(" ", QString::SkipEmptyParts);
		}
	}

	if(!extensions.empty())
	{
		extensions.removeDuplicates();
		extensions.sort();
		types.prepend(QString("%1 (%2)").arg(tr("All supported types"), extensions.join(" ")));
	}
	
	types << QString("%1 (%2)").arg(tr("Playlists"), PlaylistImporter::supportedExtensions);
	types << QString("%1 (*.*)").arg(tr("All files"));

	return types;
}

void DecoderRegistry::configureDecoders(const SettingsModel *settings)
{
	OpusDecoder::setDisableResampling(settings->opusDisableResample());
}
