///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2013 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Model_AudioFile.h"

#include "Global.h"

#include <QTime>
#include <QObject>
#include <QMutexLocker>
#include <QFile>

#include <limits.h>

const unsigned int AudioFileModel::BITDEPTH_IEEE_FLOAT32 = UINT_MAX-1;

#define PRINT_S(VAR) do \
{ \
	if((VAR).isEmpty()) qDebug(#VAR " = N/A"); else qDebug(#VAR " = \"%s\"", QUTF8((VAR))); \
} \
while(0)

#define PRINT_U(VAR) do \
{ \
	if((VAR) < 1) qDebug(#VAR " = N/A"); else qDebug(#VAR " = %u", (VAR)); \
} \
while(0)

///////////////////////////////////////////////////////////////////////////////
// Audio File - Meta Info
///////////////////////////////////////////////////////////////////////////////

AudioFileModel_MetaInfo::AudioFileModel_MetaInfo(void)
{
	reset();
}

AudioFileModel_MetaInfo::AudioFileModel_MetaInfo(const AudioFileModel_MetaInfo &model)
{
	m_titel =    model.m_titel;
	m_artist =   model.m_artist;
	m_album =    model.m_album;
	m_genre =    model.m_genre;
	m_comment =  model.m_comment;
	m_cover =    model.m_cover;
	m_year =     model.m_year;
	m_position = model.m_position;
}

AudioFileModel_MetaInfo &AudioFileModel_MetaInfo::operator=(const AudioFileModel_MetaInfo &model)
{
	m_titel =    model.m_titel;
	m_artist =   model.m_artist;
	m_album =    model.m_album;
	m_genre =    model.m_genre;
	m_comment =  model.m_comment;
	m_cover =    model.m_cover;
	m_year =     model.m_year;
	m_position = model.m_position;

	return (*this);
}

void AudioFileModel_MetaInfo::update(const AudioFileModel_MetaInfo &model, const bool replace)
{
	if((!model.m_titel.isEmpty())   && (replace || m_titel.isEmpty()))   m_titel    = model.m_titel;
	if((!model.m_artist.isEmpty())  && (replace || m_artist.isEmpty()))  m_artist   = model.m_artist;
	if((!model.m_album.isEmpty())   && (replace || m_album.isEmpty()))   m_album    = model.m_album;
	if((!model.m_genre.isEmpty())   && (replace || m_genre.isEmpty()))   m_genre    = model.m_genre;
	if((!model.m_comment.isEmpty()) && (replace || m_comment.isEmpty())) m_comment  = model.m_comment;
	if((!model.m_cover.isEmpty())   && (replace || m_cover.isEmpty()))   m_cover    = model.m_cover;
	if((model.m_year > 0)           && (replace || (m_year == 0)))       m_year     = model.m_year;
	if((model.m_position > 0)       && (replace || (m_position == 0)))   m_position = model.m_position;
}

AudioFileModel_MetaInfo::~AudioFileModel_MetaInfo(void)
{
	/*nothing to do*/
}

void AudioFileModel_MetaInfo::reset(void)
{
	m_titel.clear();
	m_artist.clear();
	m_album.clear();
	m_genre.clear();
	m_comment.clear();
	m_cover.clear();
	m_year = 0;
	m_position = 0;
}

void AudioFileModel_MetaInfo::print(void) const
{
	PRINT_S(m_titel);
	PRINT_S(m_artist);
	PRINT_S(m_album);
	PRINT_S(m_genre);
	PRINT_S(m_comment);
	PRINT_S(m_cover.filePath());
	PRINT_U(m_year);
	PRINT_U(m_position);
}

///////////////////////////////////////////////////////////////////////////////
// Audio File - Technical Info
///////////////////////////////////////////////////////////////////////////////

AudioFileModel_TechInfo::AudioFileModel_TechInfo(void)
{
	reset();
}

AudioFileModel_TechInfo::AudioFileModel_TechInfo(const AudioFileModel_TechInfo &model)
{
	m_containerType =    model.m_containerType;
	m_containerProfile = model.m_containerProfile;
	m_audioType =        model.m_audioType;
	m_audioProfile =     model.m_audioProfile;
	m_audioVersion =     model.m_audioVersion;
	m_audioEncodeLib =   model.m_audioEncodeLib;
	m_audioSamplerate =  model.m_audioSamplerate;
	m_audioChannels =    model.m_audioChannels;
	m_audioBitdepth =    model.m_audioBitdepth;
	m_audioBitrate =     model.m_audioBitrate;
	m_audioBitrateMode = model.m_audioBitrateMode;
	m_duration =         model.m_duration;
}

AudioFileModel_TechInfo &AudioFileModel_TechInfo::operator=(const AudioFileModel_TechInfo &model)
{
	m_containerType =    model.m_containerType;
	m_containerProfile = model.m_containerProfile;
	m_audioType =        model.m_audioType;
	m_audioProfile =     model.m_audioProfile;
	m_audioVersion =     model.m_audioVersion;
	m_audioEncodeLib =   model.m_audioEncodeLib;
	m_audioSamplerate =  model.m_audioSamplerate;
	m_audioChannels =    model.m_audioChannels;
	m_audioBitdepth =    model.m_audioBitdepth;
	m_audioBitrate =     model.m_audioBitrate;
	m_audioBitrateMode = model.m_audioBitrateMode;
	m_duration =         model.m_duration;

	return (*this);
}

AudioFileModel_TechInfo::~AudioFileModel_TechInfo(void)
{
	/*nothing to do*/
}

void AudioFileModel_TechInfo::reset(void)
{
	m_containerType.clear();
	m_containerProfile.clear();
	m_audioType.clear();
	m_audioProfile.clear();
	m_audioVersion.clear();
	m_audioEncodeLib.clear();
	m_audioSamplerate = 0;
	m_audioChannels = 0;
	m_audioBitdepth = 0;
	m_audioBitrate = 0;
	m_audioBitrateMode = 0;
	m_duration = 0;
}

////////////////////////////////////////////////////////////
// Audio File Model
////////////////////////////////////////////////////////////

AudioFileModel::AudioFileModel(const QString &path)
:
	m_filePath(path)
{
	m_metaInfo.reset();
	m_techInfo.reset();
}

AudioFileModel::AudioFileModel(const AudioFileModel &model)
{
	m_filePath = model.m_filePath;
	m_metaInfo = model.m_metaInfo;
	m_techInfo = model.m_techInfo;
}

AudioFileModel &AudioFileModel::operator=(const AudioFileModel &model)
{
	m_filePath = model.m_filePath;
	m_metaInfo = model.m_metaInfo;
	m_techInfo = model.m_techInfo;

	return (*this);
}

AudioFileModel::~AudioFileModel(void)
{
	/*nothing to do*/
}


void AudioFileModel::reset(void)
{
	m_filePath.clear();
	m_metaInfo.reset();
	m_techInfo.reset();
}

/*------------------------------------*/
/* Helper functions
/*------------------------------------*/

const QString AudioFileModel::durationInfo(void) const
{
	if(m_techInfo.duration())
	{
		QTime time = QTime().addSecs(m_techInfo.duration());
		return time.toString("hh:mm:ss");
	}
	else
	{
		return QString();
	}
}

const QString AudioFileModel::containerInfo(void) const
{
	if(!m_techInfo.containerType().isEmpty())
	{
		QString info = m_techInfo.containerType();
		if(!m_techInfo.containerProfile().isEmpty()) info.append(QString(" (%1: %2)").arg(tr("Profile"), m_techInfo.containerProfile()));
		return info;
	}
	else
	{
		return QString();
	}
}

const QString AudioFileModel::audioBaseInfo(void) const
{
	if(m_techInfo.audioSamplerate() || m_techInfo.audioChannels() || m_techInfo.audioBitdepth())
	{
		QString info;
		if(m_techInfo.audioChannels())
		{
			if(!info.isEmpty()) info.append(", ");
			info.append(QString("%1: %2").arg(tr("Channels"), QString::number(m_techInfo.audioChannels())));
		}
		if(m_techInfo.audioSamplerate())
		{
			if(!info.isEmpty()) info.append(", ");
			info.append(QString("%1: %2 Hz").arg(tr("Samplerate"), QString::number(m_techInfo.audioSamplerate())));
		}
		if(m_techInfo.audioBitdepth())
		{
			if(!info.isEmpty()) info.append(", ");
			if(m_techInfo.audioBitdepth() == BITDEPTH_IEEE_FLOAT32)
			{
				info.append(QString("%1: %2 Bit (IEEE Float)").arg(tr("Bitdepth"), QString::number(32)));
			}
			else
			{
				info.append(QString("%1: %2 Bit").arg(tr("Bitdepth"), QString::number(m_techInfo.audioBitdepth())));
			}
		}
		return info;
	}
	else
	{
		return QString();
	}
}

const QString AudioFileModel::audioCompressInfo(void) const
{
	if(!m_techInfo.audioType().isEmpty())
	{
		QString info;
		if(!m_techInfo.audioProfile().isEmpty() || !m_techInfo.audioVersion().isEmpty())
		{
			info.append(QString("%1: ").arg(tr("Type")));
		}
		info.append(m_techInfo.audioType());
		if(!m_techInfo.audioProfile().isEmpty())
		{
			info.append(QString(", %1: %2").arg(tr("Profile"), m_techInfo.audioProfile()));
		}
		if(!m_techInfo.audioVersion().isEmpty())
		{
			info.append(QString(", %1: %2").arg(tr("Version"), m_techInfo.audioVersion()));
		}
		if(m_techInfo.audioBitrate() > 0)
		{
			switch(m_techInfo.audioBitrateMode())
			{
			case BitrateModeConstant:
				info.append(QString(", %1: %2 kbps (%3)").arg(tr("Bitrate"), QString::number(m_techInfo.audioBitrate()), tr("Constant")));
				break;
			case BitrateModeVariable:
				info.append(WCHAR2QSTR(L", %1: \u2248%2 kbps (%3)").arg(tr("Bitrate"), QString::number(m_techInfo.audioBitrate()), tr("Variable")));
				break;
			default:
				info.append(QString(", %1: %2 kbps").arg(tr("Bitrate"), QString::number(m_techInfo.audioBitrate())));
				break;
			}
		}
		if(!m_techInfo.audioEncodeLib().isEmpty())
		{
			info.append(QString(", %1: %2").arg(tr("Encoder"), m_techInfo.audioEncodeLib()));
		}
		return info;
	}
	else
	{
		return QString();
	}
}
