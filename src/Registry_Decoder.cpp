///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2015 LoRd_MuldeR <MuldeR2@GMX.de>
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

//Internal
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

//MUtils
#include <MUtils/Exception.h>

//Qt
#include <QString>
#include <QStringList>
#include <QMutex>
#include <QRegExp>

#define PROBE_DECODER(DEC) if(DEC::isDecoderAvailable() && DEC::isFormatSupported(containerType, containerProfile, formatType, formatProfile, formatVersion)) { return new DEC(); }

#define GET_FILETYPES(LIST, DEC) do \
{ \
	if(DEC::isDecoderAvailable()) (LIST) << DEC::supportedTypes(); \
} \
while(0)

QMutex DecoderRegistry::m_lock(QMutex::Recursive);
QScopedPointer<QStringList> DecoderRegistry::m_supportedExts;
QScopedPointer<QStringList> DecoderRegistry::m_supportedTypes;
QScopedPointer<DecoderRegistry::typeList_t> DecoderRegistry::m_availableTypes;

////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////

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

const QStringList &DecoderRegistry::getSupportedExts(void)
{
	QMutexLocker locker(&m_lock);

	if(!m_supportedExts.isNull())
	{
		return (*m_supportedExts);
	}

	m_supportedExts.reset(new QStringList());

	const typeList_t &types = getAvailableDecoderTypes();
	for(QList<const AbstractDecoder::supportedType_t*>::ConstIterator iter = types.constBegin(); iter != types.constEnd(); iter++)
	{
		for(size_t i = 0; (*iter)[i].name; i++)
		{
			for(size_t j = 0; (*iter)[i].exts[j]; j++)
			{
				const QString ext = QString().sprintf("*.%s", (*iter)[i].exts[j]);
				if(!m_supportedExts->contains(ext, Qt::CaseInsensitive))
				{
					(*m_supportedExts) << ext;
				}
			}
		}
	}

	m_supportedExts->sort();
	return (*m_supportedExts);
}

const QStringList &DecoderRegistry::getSupportedTypes(void)
{
	QMutexLocker locker(&m_lock);

	if(!m_supportedTypes.isNull())
	{
		return (*m_supportedTypes);
	}

	m_supportedTypes.reset(new QStringList());

	const typeList_t &types = getAvailableDecoderTypes();
	for(QList<const AbstractDecoder::supportedType_t*>::ConstIterator iter = types.constBegin(); iter != types.constEnd(); iter++)
	{
		for(size_t i = 0; (*iter)[i].name; i++)
		{
			QStringList extList;
			for(size_t j = 0; (*iter)[i].exts[j]; j++)
			{
				extList << QString().sprintf("*.%s", (*iter)[i].exts[j]);
			}
			if(!extList.isEmpty())
			{
				(*m_supportedTypes) << QString("%1 (%2)").arg(QString::fromLatin1((*iter)[i].name), extList.join(" "));
			}
		}
	}

	QStringList playlistExts;
	const char *const *const playlistPtr = PlaylistImporter::getSupportedExtensions();
	for(size_t i = 0; playlistPtr[i]; i++)
	{
		playlistExts << QString().sprintf("*.%s", playlistPtr[i]);
	}

	(*m_supportedTypes) << QString("%1 (%2)").arg(tr("Playlists"), playlistExts.join(" "));
	(*m_supportedTypes) << QString("%1 (*.*)").arg(tr("All files"));
	m_supportedTypes->prepend(QString("%1 (%2 %3)").arg(tr("All supported types"), getSupportedExts().join(" "), playlistExts.join(" ")));

	return (*m_supportedTypes);
}

void DecoderRegistry::configureDecoders(const SettingsModel *settings)
{
	OpusDecoder::setDisableResampling(settings->opusDisableResample());
}

////////////////////////////////////////////////////////////
// Private Functions
////////////////////////////////////////////////////////////

const DecoderRegistry::typeList_t &DecoderRegistry::getAvailableDecoderTypes(void)
{
	if(!m_availableTypes.isNull())
	{
		return (*m_availableTypes);
	}

	m_availableTypes.reset(new typeList_t());

	GET_FILETYPES(*m_availableTypes, WaveDecoder);
	GET_FILETYPES(*m_availableTypes, MP3Decoder);
	GET_FILETYPES(*m_availableTypes, VorbisDecoder);
	GET_FILETYPES(*m_availableTypes, AACDecoder);
	GET_FILETYPES(*m_availableTypes, AC3Decoder);
	GET_FILETYPES(*m_availableTypes, FLACDecoder);
	GET_FILETYPES(*m_availableTypes, WavPackDecoder);
	GET_FILETYPES(*m_availableTypes, MusepackDecoder);
	GET_FILETYPES(*m_availableTypes, ShortenDecoder);
	GET_FILETYPES(*m_availableTypes, MACDecoder);
	GET_FILETYPES(*m_availableTypes, TTADecoder);
	GET_FILETYPES(*m_availableTypes, SpeexDecoder);
	GET_FILETYPES(*m_availableTypes, ALACDecoder);
	GET_FILETYPES(*m_availableTypes, WMADecoder);
	GET_FILETYPES(*m_availableTypes, ADPCMDecoder);
	GET_FILETYPES(*m_availableTypes, OpusDecoder);
	GET_FILETYPES(*m_availableTypes, AvisynthDecoder);

	return (*m_availableTypes);
}
