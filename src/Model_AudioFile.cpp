///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2018 LoRd_MuldeR <MuldeR2@GMX.de>
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

//Internal
#include "Global.h"

//MUtils
#include <MUtils/Global.h>

//Qt
#include <QTime>
#include <QObject>
#include <QMutexLocker>
#include <QFile>

//CRT
#include <limits.h>

const unsigned int AudioFileModel::BITDEPTH_IEEE_FLOAT32 = UINT_MAX-1;

#define PRINT_S(VAR) do \
{ \
	if((VAR).isEmpty()) qDebug(#VAR " = N/A"); else qDebug(#VAR " = \"%s\"", MUTILS_UTF8((VAR))); \
} \
while(0)

#define PRINT_U(VAR) do \
{ \
	if((VAR) < 1) qDebug(#VAR " = N/A"); else qDebug(#VAR " = %u", (VAR)); \
} \
while(0)

//if((!(OTHER.NAME.isEmpty())) && ((FORCE) || (this.NAME.isEmpty()))) /*this.NAME = OTHER.NAME;*/ \

#define UPDATE_STR(OTHER, FORCE, NAME) do \
{ \
	if(!(((OTHER).NAME).isEmpty())) \
	{ \
		if((FORCE) || ((this->NAME).isEmpty())) (this->NAME) = ((OTHER).NAME); \
	} \
} \
while(0)

#define UPDATE_INT(OTHER, FORCE, NAME) do \
{ \
	if(((OTHER).NAME) > 0) \
	{ \
		if((FORCE) || ((this->NAME) == 0)) (this->NAME) = ((OTHER).NAME); \
	} \
} \
while(0)

#define ASSIGN_VAL(OTHER, NAME)  do \
{ \
	(this->NAME) = ((OTHER).NAME); \
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
	ASSIGN_VAL(model, m_titel);
	ASSIGN_VAL(model, m_artist);
	ASSIGN_VAL(model, m_album);
	ASSIGN_VAL(model, m_genre);
	ASSIGN_VAL(model, m_comment);
	ASSIGN_VAL(model, m_cover);
	ASSIGN_VAL(model, m_year);
	ASSIGN_VAL(model, m_position);
}

AudioFileModel_MetaInfo &AudioFileModel_MetaInfo::operator=(const AudioFileModel_MetaInfo &model)
{
	ASSIGN_VAL(model, m_titel);
	ASSIGN_VAL(model, m_artist);
	ASSIGN_VAL(model, m_album);
	ASSIGN_VAL(model, m_genre);
	ASSIGN_VAL(model, m_comment);
	ASSIGN_VAL(model, m_cover);
	ASSIGN_VAL(model, m_year);
	ASSIGN_VAL(model, m_position);

	return (*this);
}

#define IS_EMPTY(X) ((X).isEmpty() ? "YES" : "NO")

void AudioFileModel_MetaInfo::update(const AudioFileModel_MetaInfo &model, const bool replace)
{
	UPDATE_STR(model, replace, m_titel);
	UPDATE_STR(model, replace, m_artist);
	UPDATE_STR(model, replace, m_album);
	UPDATE_STR(model, replace, m_genre);
	UPDATE_STR(model, replace, m_comment);
	UPDATE_STR(model, replace, m_cover);
	UPDATE_INT(model, replace, m_year);
	UPDATE_INT(model, replace, m_position);
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

bool  AudioFileModel_MetaInfo::empty(const bool &ignoreArtwork) const
{
	bool isEmpty = true;

	if(!m_titel.isEmpty())   isEmpty = false;
	if(!m_artist.isEmpty())  isEmpty = false;
	if(!m_album.isEmpty())   isEmpty = false;
	if(!m_genre.isEmpty())   isEmpty = false;
	if(!m_comment.isEmpty()) isEmpty = false;
	if(m_year)               isEmpty = false;
	if(m_position)           isEmpty = false;

	if(!ignoreArtwork)
	{
		if(!m_cover.isEmpty())
		{
			isEmpty = false;
		}
	}

	return isEmpty;
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
	ASSIGN_VAL(model, m_containerType);
	ASSIGN_VAL(model, m_containerProfile);
	ASSIGN_VAL(model, m_audioType);
	ASSIGN_VAL(model, m_audioProfile);
	ASSIGN_VAL(model, m_audioVersion);
	ASSIGN_VAL(model, m_audioEncodeLib);
	ASSIGN_VAL(model, m_audioSamplerate);
	ASSIGN_VAL(model, m_audioChannels);
	ASSIGN_VAL(model, m_audioBitdepth);
	ASSIGN_VAL(model, m_audioBitrate);
	ASSIGN_VAL(model, m_audioBitrateMode);
	ASSIGN_VAL(model, m_duration);
}

AudioFileModel_TechInfo &AudioFileModel_TechInfo::operator=(const AudioFileModel_TechInfo &model)
{
	ASSIGN_VAL(model, m_containerType);
	ASSIGN_VAL(model, m_containerProfile);
	ASSIGN_VAL(model, m_audioType);
	ASSIGN_VAL(model, m_audioProfile);
	ASSIGN_VAL(model, m_audioVersion);
	ASSIGN_VAL(model, m_audioEncodeLib);
	ASSIGN_VAL(model, m_audioSamplerate);
	ASSIGN_VAL(model, m_audioChannels);
	ASSIGN_VAL(model, m_audioBitdepth);
	ASSIGN_VAL(model, m_audioBitrate);
	ASSIGN_VAL(model, m_audioBitrateMode);
	ASSIGN_VAL(model, m_duration);

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
	ASSIGN_VAL(model, m_filePath);
	ASSIGN_VAL(model, m_metaInfo);
	ASSIGN_VAL(model, m_techInfo);
}

AudioFileModel &AudioFileModel::operator=(const AudioFileModel &model)
{
	ASSIGN_VAL(model, m_filePath);
	ASSIGN_VAL(model, m_metaInfo);
	ASSIGN_VAL(model, m_techInfo);

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
				info.append(MUTILS_QSTR(L", %1: \u2248%2 kbps (%3)").arg(tr("Bitrate"), QString::number(m_techInfo.audioBitrate()), tr("Variable")));
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
