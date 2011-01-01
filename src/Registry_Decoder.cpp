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

#include "Registry_Decoder.h"

#include "Decoder_AAC.h"
#include "Decoder_MP3.h"
#include "Decoder_Vorbis.h"
#include "Decoder_FLAC.h"
#include "Decoder_AC3.h"
#include "Decoder_WMA.h"
#include "Decoder_Wave.h"

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
	PROBE_DECODER(WMADecoder);
	PROBE_DECODER(WaveDecoder);
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
	types << GET_FILETYPES(WMADecoder);

	QStringList extensions;
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
		types.prepend(QString("All supported types (%1)").arg(extensions.join(" ")));
	}
	
	types << "All files (*.*)";
	return types;
}
