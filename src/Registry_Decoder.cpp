///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2010 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Registry_Decoder.h"

#include "Decoder_AAC.h"
#include "Decoder_MP3.h"
#include "Decoder_Vorbis.h"
#include "Decoder_FLAC.h"
#include "Decoder_WMA.h"
#include "Decoder_Wave.h"

#include <QString>

#define PROBE_DECODER(DEC) if(DEC::isDecoderAvailable() && DEC::isFormatSupported(containerType, containerProfile, formatType, formatProfile, formatVersion)) { return new DEC(); }

AbstractDecoder *DecoderRegistry::lookup(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	PROBE_DECODER(MP3Decoder);
	PROBE_DECODER(VorbisDecoder);
	PROBE_DECODER(AACDecoder);
	PROBE_DECODER(FLACDecoder);
	PROBE_DECODER(WMADecoder);
	PROBE_DECODER(WaveDecoder);
	return NULL;
}

