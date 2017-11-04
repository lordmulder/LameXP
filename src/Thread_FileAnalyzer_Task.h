///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2017 LoRd_MuldeR <MuldeR2@GMX.de>
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

#pragma once

#include <QRunnable>
#include <QReadWriteLock>
#include <QWaitCondition>
#include <QStringList>
#include <QMutex>
#include <QSemaphore>
#include <QPair>

class AudioFileModel;
class QFile;
class QXmlStreamReader;
class QXmlStreamAttributes;

////////////////////////////////////////////////////////////
// Splash Thread
////////////////////////////////////////////////////////////

class AnalyzeTask: public QObject, public QRunnable
{
	Q_OBJECT

public:
	AnalyzeTask(const int taskId, const QString &inputFile, QAtomicInt &abortFlag);
	~AnalyzeTask(void);
	
	typedef enum
	{
		fileTypeNormal   = 0,
		fileTypeCDDA     = 1,
		fileTypeDenied   = 2,
		fileTypeCueSheet = 3,
		fileTypeUnknown  = 4
	}
	fileType_t;

	typedef enum
	{
		trackType_non = 0,
		trackType_gen = 1,
		trackType_aud = 2
	}
	MI_trackType_t;

	typedef enum
	{
		propertyId_container,
		propertyId_container_profile,
		propertyId_duration,
		propertyId_title,
		propertyId_artist,
		propertyId_album,
		propertyId_genre,
		propertyId_released_date,
		propertyId_track_position,
		propertyId_comment,
		propertyId_format,
		propertyId_format_version,
		propertyId_format_profile,
		propertyId_channel_s_,
		propertyId_samplingrate,
		propertyId_bitdepth,
		propertyId_bitrate,
		propertyId_bitrate_mode,
		propertyId_encoded_library,
		propertyId_cover_mime,
		propertyId_cover_data
	}
	MI_propertyId_t;

signals:
	void fileAnalyzed(const unsigned int taskId, const int fileType, const AudioFileModel &file);
	void taskCompleted(const unsigned int taskId);

protected:
	void run(void);
	void run_ex(void);

private:
	const AudioFileModel& analyzeFile(const QString &filePath, AudioFileModel &audioFile, int *const type);
	const AudioFileModel& analyzeMediaFile(const QString &filePath, AudioFileModel &audioFile);
	const AudioFileModel& parseMediaInfo(const QByteArray &data, AudioFileModel &audioFile);
	void parseFileInfo(QXmlStreamReader &xmlStream, AudioFileModel &audioFile);
	void parseTrackInfo(QXmlStreamReader &xmlStream, const MI_trackType_t trackType, AudioFileModel &audioFile);
	void parseProperty(const QString &value, const MI_propertyId_t propertyIdx, AudioFileModel &audioFile, QString &coverMimeType);
	void retrieveCover(AudioFileModel &audioFile, const QString &coverType, const QString &coverData);
	bool checkFile_CDDA(QFile &file);
	bool analyzeAvisynthFile(const QString &filePath, AudioFileModel &info);

	static QString decodeStr(const QString &str, const QString &encoding);
	static bool parseUnsigned(const QString &str, quint32 &value);
	static bool parseFloat(const QString &str, double &value);
	static bool parseYear(const QString &st, quint32 &valuer);
	static bool parseRCMode(const QString &str, quint32 &value);
	static QString cleanAsciiStr(const QString &str);
	static bool findNextElement(const QString &name, QXmlStreamReader &xmlStream);
	static QString findAttribute(const QString &name, const QXmlStreamAttributes &xmlAttributes);
	static bool checkVersionStr(const QString &str, const quint32 expectedMajor, const quint32 expectedMinor);

	const QMap<QPair<MI_trackType_t, QString>, MI_propertyId_t> &m_mediaInfoIdx;
	const QMap<QString, MI_propertyId_t> &m_avisynthIdx;
	const QMap<QString, QString> &m_mimeTypes;
	const QMap<QString, MI_trackType_t> &m_trackTypes;

	const unsigned int m_taskId;
	const QString m_mediaInfoBin;
	const quint32 m_mediaInfoVer;
	const QString m_avs2wavBin;
	const QString m_inputFile;

	QAtomicInt &m_abortFlag;
};
