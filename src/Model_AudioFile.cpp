///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2012 LoRd_MuldeR <MuldeR2@GMX.de>
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
#include <QObject>
#include <QMutexLocker>
#include <QFile>
#include <limits.h>

#define U16Str(X) QString::fromUtf16(reinterpret_cast<const unsigned short*>(L##X))

const unsigned int AudioFileModel::BITDEPTH_IEEE_FLOAT32 = UINT_MAX-1;

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

AudioFileModel::AudioFileModel(const QString &path, const QString &name)
{
	resetAll();

	m_filePath = path;
	m_fileName = name;
}

AudioFileModel::AudioFileModel(const AudioFileModel &model, bool copyMetaInfo)
{
	resetAll();

	setFilePath(model.m_filePath);
	setFormatContainerType(model.m_formatContainerType);
	setFormatContainerProfile(model.m_formatContainerProfile);
	setFormatAudioType(model.m_formatAudioType);
	setFormatAudioProfile(model.m_formatAudioProfile);
	setFormatAudioVersion(model.m_formatAudioVersion);
	setFormatAudioEncodeLib(model.m_formatAudioEncodeLib);
	setFormatAudioSamplerate(model.m_formatAudioSamplerate);
	setFormatAudioChannels(model.m_formatAudioChannels);
	setFormatAudioBitdepth(model.m_formatAudioBitdepth);
	setFormatAudioBitrate(model.m_formatAudioBitrate);
	setFormatAudioBitrateMode(model.m_formatAudioBitrateMode);
	setFileDuration(model.m_fileDuration);

	if(copyMetaInfo)
	{
		setFileName(model.m_fileName);
		setFileArtist(model.m_fileArtist);
		setFileAlbum(model.m_fileAlbum);
		setFileGenre(model.m_fileGenre);
		setFileComment(model.m_fileComment);
		setFileCover(model.m_fileCover);
		setFileYear(model.m_fileYear);
		setFilePosition(model.m_filePosition);
	}
}

AudioFileModel &AudioFileModel::operator=(const AudioFileModel &model)
{
	setFilePath(model.m_filePath);
	setFileName(model.m_fileName);
	setFileArtist(model.m_fileArtist);
	setFileAlbum(model.m_fileAlbum);
	setFileGenre(model.m_fileGenre);
	setFileComment(model.m_fileComment);
	setFileCover(model.m_fileCover);
	setFileYear(model.m_fileYear);
	setFilePosition(model.m_filePosition);
	setFileDuration(model.m_fileDuration);

	setFormatContainerType(model.m_formatContainerType);
	setFormatContainerProfile(model.m_formatContainerProfile);
	setFormatAudioType(model.m_formatAudioType);
	setFormatAudioProfile(model.m_formatAudioProfile);
	setFormatAudioVersion(model.m_formatAudioVersion);
	setFormatAudioEncodeLib(model.m_formatAudioEncodeLib);
	setFormatAudioSamplerate(model.m_formatAudioSamplerate);
	setFormatAudioChannels(model.m_formatAudioChannels);
	setFormatAudioBitdepth(model.m_formatAudioBitdepth);
	setFormatAudioBitrate(model.m_formatAudioBitrate);
	setFormatAudioBitrateMode(model.m_formatAudioBitrateMode);

	return (*this);
}

AudioFileModel::~AudioFileModel(void)
{
}

////////////////////////////////////////////////////////////
// Private Functions
////////////////////////////////////////////////////////////

void AudioFileModel::resetAll(void)
{
	m_filePath.clear();
	m_fileName.clear();
	m_fileArtist.clear();
	m_fileAlbum.clear();
	m_fileGenre.clear();
	m_fileComment.clear();
	m_fileCover.clear();
	
	m_fileYear = 0;
	m_filePosition = 0;
	m_fileDuration = 0;

	m_formatContainerType.clear();
	m_formatContainerProfile.clear();
	m_formatAudioType.clear();
	m_formatAudioProfile.clear();
	m_formatAudioVersion.clear();
	m_formatAudioEncodeLib.clear();
	
	m_formatAudioSamplerate = 0;
	m_formatAudioChannels = 0;
	m_formatAudioBitdepth = 0;
	m_formatAudioBitrate = 0;
	m_formatAudioBitrateMode = BitrateModeUndefined;
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

const QString &AudioFileModel::fileCover(void) const
{
	return m_fileCover.filePath();
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

unsigned int AudioFileModel::formatAudioBitrate(void) const
{
	return m_formatAudioBitrate;
}

unsigned int  AudioFileModel::formatAudioBitrateMode(void) const
{
	return m_formatAudioBitrateMode;
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
		if(!m_formatContainerProfile.isEmpty()) info.append(QString(" (%1: %2)").arg(tr("Profile"), m_formatContainerProfile));
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
			info.append(QString("%1: %2").arg(tr("Channels"), QString::number(m_formatAudioChannels)));
		}
		if(m_formatAudioSamplerate)
		{
			if(!info.isEmpty()) info.append(", ");
			info.append(QString("%1: %2 Hz").arg(tr("Samplerate"), QString::number(m_formatAudioSamplerate)));
		}
		if(m_formatAudioBitdepth)
		{
			if(!info.isEmpty()) info.append(", ");
			if(m_formatAudioBitdepth == BITDEPTH_IEEE_FLOAT32)
			{
				info.append(QString("%1: %2 Bit (IEEE Float)").arg(tr("Bitdepth"), QString::number(32)));
			}
			else
			{
				info.append(QString("%1: %2 Bit").arg(tr("Bitdepth"), QString::number(m_formatAudioBitdepth)));
			}
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
			info.append(QString("%1: ").arg(tr("Type")));
		}
		info.append(m_formatAudioType);
		if(!m_formatAudioProfile.isEmpty())
		{
			info.append(QString(", %1: %2").arg(tr("Profile"), m_formatAudioProfile));
		}
		if(!m_formatAudioVersion.isEmpty())
		{
			info.append(QString(", %1: %2").arg(tr("Version"), m_formatAudioVersion));
		}
		if(m_formatAudioBitrate > 0)
		{
			switch(m_formatAudioBitrateMode)
			{
			case BitrateModeConstant:
				info.append(U16Str(", %1: %2 kbps (%3)").arg(tr("Bitrate"), QString::number(m_formatAudioBitrate), tr("Constant")));
				break;
			case BitrateModeVariable:
				info.append(U16Str(", %1: \u2248%2 kbps (%3)").arg(tr("Bitrate"), QString::number(m_formatAudioBitrate), tr("Variable")));
				break;
			default:
				info.append(U16Str(", %1: %2 kbps").arg(tr("Bitrate"), QString::number(m_formatAudioBitrate)));
				break;
			}
		}
		if(!m_formatAudioEncodeLib.isEmpty())
		{
			info.append(QString(", %1: %2").arg(tr("Encoder"), m_formatAudioEncodeLib));
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

void AudioFileModel::setFileCover(const QString &coverFile, bool owner)
{
	m_fileCover.setFilePath(coverFile, owner);
}

void AudioFileModel::setFileCover(const ArtworkModel &model)
{
	m_fileCover = model;
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

void AudioFileModel::setFormatAudioEncodeLib(const QString &encodeLib)
{
	m_formatAudioEncodeLib = encodeLib;
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

void AudioFileModel::setFormatAudioBitrate(unsigned int bitrate)
{
	m_formatAudioBitrate = bitrate;
}

void AudioFileModel::setFormatAudioBitrateMode(unsigned int bitrateMode)
{
	m_formatAudioBitrateMode = bitrateMode;
}

void AudioFileModel::updateMetaInfo(const AudioFileModel &model)
{
	if(!model.fileArtist().isEmpty()) setFileArtist(model.fileArtist());
	if(!model.fileAlbum().isEmpty()) setFileAlbum(model.fileAlbum());
	if(!model.fileGenre().isEmpty()) setFileGenre(model.fileGenre());
	if(!model.fileComment().isEmpty()) setFileComment(model.fileComment());
	if(model.fileYear()) setFileYear(model.fileYear());
}
