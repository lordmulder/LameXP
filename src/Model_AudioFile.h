///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2020 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU GENERAL PUBLIC LICENSE as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version; always including the non-optional
// LAMEXP GNU GENERAL PUBLIC LICENSE ADDENDUM. See "License.txt" file!
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

#pragma once

#include <QObject>
#include <QString>

#include "Model_Artwork.h"

///////////////////////////////////////////////////////////////////////////////
// Audio File - Meta Info
///////////////////////////////////////////////////////////////////////////////

class AudioFileModel_MetaInfo : public QObject
{
	Q_OBJECT

public:
	//Constructors & Destructor
	AudioFileModel_MetaInfo(void);
	AudioFileModel_MetaInfo(const AudioFileModel_MetaInfo &model);
	AudioFileModel_MetaInfo &operator=(const AudioFileModel_MetaInfo &model);
	~AudioFileModel_MetaInfo(void);

	//Getter
	inline const QString &title(void)   const { return m_titel; }
	inline const QString &artist(void)  const { return m_artist; }
	inline const QString &album(void)   const { return m_album; }
	inline const QString &genre(void)   const { return m_genre; }
	inline const QString &comment(void) const { return m_comment; }
	inline const QString &cover(void)   const { return m_cover.filePath(); }
	inline unsigned int year(void)      const { return m_year; }
	inline unsigned int position(void)  const { return m_position; }

	//Setter
	inline void setTitle(const QString &titel)                    { m_titel = titel.trimmed(); }
	inline void setArtist(const QString &artist)                  { m_artist = artist.trimmed(); }
	inline void setAlbum(const QString &album)                    { m_album = album.trimmed(); }
	inline void setGenre(const QString &genre)                    { m_genre = genre.trimmed(); }
	inline void setComment(const QString &comment)                { m_comment = comment.trimmed(); }
	inline void setCover(const QString &path, const bool isOwner) { m_cover.setFilePath(path, isOwner); }
	inline void setYear(const unsigned int year)                  { m_year = year; }
	inline void setPosition(const unsigned int position)          { m_position = position; }

	//Is empty?
	bool empty(const bool &ignoreArtwork) const;

	//Reset
	void reset(void);

	//Update
	void update(const AudioFileModel_MetaInfo &model, const bool replace);

	//Debug
	void print(void) const;

private:
	QString      m_titel;
	QString      m_artist;
	QString      m_album;
	QString      m_genre;
	QString      m_comment;
	ArtworkModel m_cover;
	unsigned int m_year;
	unsigned int m_position;
};

///////////////////////////////////////////////////////////////////////////////
// Audio File - Technical Info
///////////////////////////////////////////////////////////////////////////////

class AudioFileModel_TechInfo : public QObject
{
	Q_OBJECT

public:
	//Constructors & Destructor
	AudioFileModel_TechInfo(void);
	AudioFileModel_TechInfo(const AudioFileModel_TechInfo &model);
	AudioFileModel_TechInfo &operator=(const AudioFileModel_TechInfo &model);
	~AudioFileModel_TechInfo(void);

	//Getter
	inline const QString &containerType(void)    const { return m_containerType; }
	inline const QString &containerProfile(void) const { return m_containerProfile; }
	inline const QString &audioType(void)        const { return m_audioType; }
	inline const QString &audioProfile(void)     const { return m_audioProfile; }
	inline const QString &audioVersion(void)     const { return m_audioVersion; }
	inline const QString &audioEncodeLib(void)   const { return m_audioEncodeLib; }
	inline unsigned int audioSamplerate(void)    const { return m_audioSamplerate; }
	inline unsigned int audioChannels(void)      const { return m_audioChannels; }
	inline unsigned int audioBitdepth(void)      const { return m_audioBitdepth; }
	inline unsigned int audioBitrate(void)       const { return m_audioBitrate; }
	inline unsigned int audioBitrateMode(void)   const { return m_audioBitrateMode; }
	inline unsigned int duration(void)           const { return m_duration; }

	//Setter
	inline void setContainerType(const QString &containerType)           { m_containerType = containerType.trimmed(); }
	inline void setContainerProfile(const QString &containerProfile)     { m_containerProfile = containerProfile.trimmed(); }
	inline void setAudioType(const QString &audioType)                   { m_audioType = audioType.trimmed(); }
	inline void setAudioProfile(const QString &audioProfile)             { m_audioProfile = audioProfile.trimmed(); }
	inline void setAudioVersion(const QString &audioVersion)             { m_audioVersion = audioVersion.trimmed(); }
	inline void setAudioEncodeLib(const QString &audioEncodeLib)         { m_audioEncodeLib = audioEncodeLib.trimmed(); }
	inline void setAudioSamplerate(const unsigned int audioSamplerate)   { m_audioSamplerate = audioSamplerate; }
	inline void setAudioChannels(const unsigned int audioChannels)       { m_audioChannels = audioChannels; }
	inline void setAudioBitdepth(const unsigned int audioBitdepth)       { m_audioBitdepth = audioBitdepth; }
	inline void setAudioBitrate(const unsigned int audioBitrate)         { m_audioBitrate = audioBitrate; }
	inline void setAudioBitrateMode(const unsigned int audioBitrateMode) { m_audioBitrateMode = audioBitrateMode; }
	inline void setDuration(const unsigned int duration)                 { m_duration = duration; }

	//Reset
	void reset(void);

private:
	QString m_containerType;
	QString m_containerProfile;
	QString m_audioType;
	QString m_audioProfile;
	QString m_audioVersion;
	QString m_audioEncodeLib;
	unsigned int m_audioSamplerate;
	unsigned int m_audioChannels;
	unsigned int m_audioBitdepth;
	unsigned int m_audioBitrate;
	unsigned int m_audioBitrateMode;
	unsigned int m_duration;
};

///////////////////////////////////////////////////////////////////////////////
// Audio File Model
///////////////////////////////////////////////////////////////////////////////

class AudioFileModel : public QObject
{
	Q_OBJECT

public:
	//Types
	enum BitrateMode
	{
		BitrateModeUndefined = 0,
		BitrateModeConstant = 1,
		BitrateModeVariable = 2,
	};

	//Constants
	static const unsigned int BITDEPTH_IEEE_FLOAT32;

	//Constructors & Destructor
	AudioFileModel(const QString &path = QString());
	AudioFileModel(const AudioFileModel &model);
	AudioFileModel &operator=(const AudioFileModel &model);
	~AudioFileModel(void);

	//Getter
	inline const QString &filePath(void)                 const { return m_filePath; }
	inline const AudioFileModel_MetaInfo &metaInfo(void) const { return m_metaInfo; }
	inline const AudioFileModel_TechInfo &techInfo(void) const { return m_techInfo; }
	inline AudioFileModel_MetaInfo &metaInfo(void)             { return m_metaInfo; }
	inline AudioFileModel_TechInfo &techInfo(void)             { return m_techInfo; }

	//Setter
	inline void setFilePath(const QString &filePath)                 { m_filePath = filePath; }
	inline void setMetaInfo(const AudioFileModel_MetaInfo &metaInfo) { m_metaInfo = metaInfo; }
	inline void setTechInfo(const AudioFileModel_TechInfo &techInfo) { m_techInfo = techInfo; }

	//Helpers
	const QString durationInfo(void) const;
	const QString containerInfo(void) const;
	const QString audioBaseInfo(void) const;
	const QString audioCompressInfo(void) const;

	//Reset
	void reset(void);

private:
	QString m_filePath;
	AudioFileModel_MetaInfo m_metaInfo;
	AudioFileModel_TechInfo m_techInfo;
};
