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

#include "Model_AudioFile.h"

#include <QTime>

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

AudioFileModel::AudioFileModel(const QString &path, const QString &name)
{
	m_filePath = path;
	m_fileName = name;
	m_fileYear = 0;
	m_filePosition = 0;
	m_fileDuration = 0;
	m_formatAudioSamplerate = 0;
	m_formatAudioChannels = 0;
	m_formatAudioBitdepth = 0;
}

AudioFileModel::~AudioFileModel(void)
{
}

////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////

// ---------------------------------
// Getter methods
// ---------------------------------

const QString &AudioFileModel::filePath(void) const
{
	return m_filePath;
}

const QString &AudioFileModel::fileName(void) const
{
	return m_fileName;
}

const QString &AudioFileModel::fileArtist(void) const
{
	return m_fileArtist;
}

const QString &AudioFileModel::fileAlbum(void) const
{
	return m_fileAlbum;
}

const QString &AudioFileModel::fileGenre(void) const
{
	return m_fileGenre;
}

const QString &AudioFileModel::fileComment(void) const
{
	return m_fileComment;
}

unsigned int AudioFileModel::fileYear(void) const
{
	return m_fileYear;
}

unsigned int AudioFileModel::filePosition(void) const
{
	return m_filePosition;
}

unsigned int AudioFileModel::fileDuration(void) const
{
	return m_fileDuration;
}

const QString &AudioFileModel::formatContainerType(void) const
{
	return m_formatContainerType;
}

const QString &AudioFileModel::formatContainerProfile(void) const
{
	return m_formatContainerProfile;
}

const QString &AudioFileModel::formatAudioType(void) const
{
	return m_formatAudioType;
}

const QString &AudioFileModel::formatAudioProfile(void) const
{
	return m_formatAudioProfile;
}

const QString &AudioFileModel::formatAudioVersion(void) const
{
	return m_formatAudioVersion;
}

unsigned int AudioFileModel::formatAudioSamplerate(void) const
{
	return m_formatAudioSamplerate;
}

unsigned int AudioFileModel::formatAudioChannels(void) const
{
	return m_formatAudioChannels;
}

unsigned int AudioFileModel::formatAudioBitdepth(void) const
{
	return m_formatAudioBitdepth;
}

const QString AudioFileModel::fileDurationInfo(void) const
{
	if(m_fileDuration)
	{
		QTime time = QTime().addSecs(m_fileDuration);
		return time.toString("hh:mm:ss");
	}
	else
	{
		return QString();
	}
}

const QString AudioFileModel::formatContainerInfo(void) const
{
	if(!m_formatContainerType.isEmpty())
	{
		QString info = m_formatContainerType;
		if(!m_formatContainerProfile.isEmpty()) info.append(" (Profile: ").append(m_formatContainerProfile).append(")");
		return info;
	}
	else
	{
		return QString();
	}
}

const QString AudioFileModel::formatAudioBaseInfo(void) const
{
	if(m_formatAudioSamplerate || m_formatAudioChannels || m_formatAudioBitdepth)
	{
		QString info;
		if(m_formatAudioChannels)
		{
			if(!info.isEmpty()) info.append(", ");
			info.append("Channels: ").append(QString::number(m_formatAudioChannels));
		}
		if(m_formatAudioSamplerate)
		{
			if(!info.isEmpty()) info.append(", ");
			info.append("Samplerate: ").append(QString::number(m_formatAudioSamplerate)).append(" Hz");
		}
		if(m_formatAudioBitdepth)
		{
			if(!info.isEmpty()) info.append(", ");
			info.append("Bitdepth: ").append(QString::number(m_formatAudioBitdepth)).append(" Bit");
		}
		return info;
	}
	else
	{
		return QString();
	}
}

const QString AudioFileModel::formatAudioCompressInfo(void) const
{
	if(!m_formatAudioType.isEmpty())
	{
		QString info;
		if(!m_formatAudioProfile.isEmpty() || !m_formatAudioVersion.isEmpty())
		{
			info.append("Type: ");
		}
		info.append(m_formatAudioType);
		if(!m_formatAudioProfile.isEmpty())
		{
			info.append(", Profile: ").append(m_formatAudioProfile);
		}
		if(!m_formatAudioVersion.isEmpty())
		{
			info.append(", Version: ").append(m_formatAudioVersion);
		}
		return info;
	}
	else
	{
		return QString();
	}
}

// ---------------------------------
// Setter methods
// ---------------------------------

void AudioFileModel::setFilePath(const QString &path)
{
	m_filePath = path;
}

void AudioFileModel::setFileName(const QString &name)
{
	m_fileName = name;
}

void AudioFileModel::setFileArtist(const QString &artist)
{
	m_fileArtist = artist;
}

void AudioFileModel::setFileAlbum(const QString &album)
{
	m_fileAlbum = album;
}

void AudioFileModel::setFileGenre(const QString &genre)
{
	m_fileGenre = genre;
}

void AudioFileModel::setFileComment(const QString &comment)
{
	m_fileComment = comment;
}

void AudioFileModel::setFileYear(unsigned int year)
{
	m_fileYear = year;
}

void AudioFileModel::setFilePosition(unsigned int position)
{
	m_filePosition = position;
}

void AudioFileModel::setFileDuration(unsigned int duration)
{
	m_fileDuration = duration;
}

void AudioFileModel::setFormatContainerType(const QString &type)
{
	m_formatContainerType = type;
}

void AudioFileModel::setFormatContainerProfile(const QString &profile)
{
	m_formatContainerProfile = profile;
}

void AudioFileModel::setFormatAudioType(const QString &type)
{
	m_formatAudioType = type;
}

void AudioFileModel::setFormatAudioProfile(const QString &profile)
{
	m_formatAudioProfile = profile;
}

void AudioFileModel::setFormatAudioVersion(const QString &version)
{
	m_formatAudioVersion = version;
}

void AudioFileModel::setFormatAudioSamplerate(unsigned int samplerate)
{
	m_formatAudioSamplerate = samplerate;
}

void AudioFileModel::setFormatAudioChannels(unsigned int channels)
{
	m_formatAudioChannels = channels;
}

void AudioFileModel::setFormatAudioBitdepth(unsigned int bitdepth)
{
	m_formatAudioBitdepth = bitdepth;
}
