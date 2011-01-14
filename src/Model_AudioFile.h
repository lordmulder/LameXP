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

#pragma once

#include <QObject>
#include <QString>

class AudioFileModel : public QObject
{
	Q_OBJECT

public:
	AudioFileModel(const QString &path = QString(), const QString &name = QString());
	AudioFileModel(const AudioFileModel &model);
	AudioFileModel &operator=(const AudioFileModel &model);
	~AudioFileModel(void);

	//-----------------------
	//Getters
	//-----------------------

	const QString &filePath(void) const;
	const QString &fileName(void) const;
	const QString &fileArtist(void) const;
	const QString &fileAlbum(void) const;
	const QString &fileGenre(void) const;
	const QString &fileComment(void) const;
	unsigned int fileYear(void) const;
	unsigned int filePosition(void) const;
	unsigned int fileDuration(void) const;

	const QString &formatContainerType(void) const;
	const QString &formatContainerProfile(void) const;
	const QString &formatAudioType(void) const;
	const QString &formatAudioProfile(void) const;
	const QString &formatAudioVersion(void) const;
	unsigned int formatAudioSamplerate(void) const;
	unsigned int formatAudioChannels(void) const;
	unsigned int formatAudioBitdepth(void) const;

	const QString fileDurationInfo(void) const;
	const QString formatContainerInfo(void) const;
	const QString formatAudioBaseInfo(void) const;
	const QString formatAudioCompressInfo(void) const;

	//-----------------------
	//Setters
	//-----------------------

	void setFilePath(const QString &path);
	void setFileName(const QString &name);
	void setFileArtist(const QString &artist);
	void setFileAlbum(const QString &album);
	void setFileGenre(const QString &genre);
	void setFileComment(const QString &comment);
	void setFileYear(unsigned int year);
	void setFilePosition(unsigned int position);
	void setFileDuration(unsigned int duration);

	void setFormatContainerType(const QString &type);
	void setFormatContainerProfile(const QString &profile);
	void setFormatAudioType(const QString &type);
	void setFormatAudioProfile(const QString &profile);
	void setFormatAudioVersion(const QString &version);
	void setFormatAudioSamplerate(unsigned int samplerate);
	void setFormatAudioChannels(unsigned int channels);
	void setFormatAudioBitdepth(unsigned int bitdepth);

private:
	QString m_filePath;
	QString m_fileName;
	QString m_fileArtist;
	QString m_fileAlbum;
	QString m_fileGenre;
	QString m_fileComment;
	unsigned int m_fileYear;
	unsigned int m_filePosition;
	unsigned int m_fileDuration;

	QString m_formatContainerType;
	QString m_formatContainerProfile;
	QString m_formatAudioType;
	QString m_formatAudioProfile;
	QString m_formatAudioVersion;
	unsigned int m_formatAudioSamplerate;
	unsigned int m_formatAudioChannels;
	unsigned int m_formatAudioBitdepth;
};
