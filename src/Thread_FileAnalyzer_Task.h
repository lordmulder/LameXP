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

#pragma once

#include <QRunnable>
#include <QReadWriteLock>
#include <QWaitCondition>
#include <QStringList>
#include <QMutex>
#include <QSemaphore>

class AudioFileModel;
class QFile;
class QDir;
class QFileInfo;
class LockedFile;

////////////////////////////////////////////////////////////
// Splash Thread
////////////////////////////////////////////////////////////

class AnalyzeTask: public QObject, public QRunnable
{
	Q_OBJECT

public:
	AnalyzeTask(const int taskId, const QString &inputFile, const QString &templateFile, volatile bool *abortFlag);
	~AnalyzeTask(void);
	
	enum fileType_t
	{
		fileTypeNormal = 0,
		fileTypeCDDA = 1,
		fileTypeDenied = 2,
		fileTypeCueSheet = 3,
		fileTypeUnknown = 4
	};

	//Wait until there is a free slot in the queue
	static bool waitForFreeSlot(volatile bool *abortFlag);

signals:
	void fileAnalyzed(const unsigned int taskId, const int fileType, const AudioFileModel &file);
	void taskCompleted(const unsigned int taskId);

protected:
	void run(void);
	void run_ex(void);

private:
	enum cover_t
	{
		coverNone,
		coverJpeg,
		coverPng,
		coverGif
	};

	const AudioFileModel analyzeFile(const QString &filePath, int *type);
	void updateInfo(AudioFileModel &audioFile, bool *skipNext, unsigned int *id_val, cover_t *coverType, QByteArray *coverData, const QString &key, const QString &value);
	unsigned int parseYear(const QString &str);
	unsigned int parseDuration(const QString &str);
	bool checkFile_CDDA(QFile &file);
	void retrieveCover(AudioFileModel &audioFile, cover_t coverType, const QByteArray &coverData);
	bool analyzeAvisynthFile(const QString &filePath, AudioFileModel &info);

	const unsigned int m_taskId;

	const QString m_mediaInfoBin;
	const QString m_avs2wavBin;
	const QString m_templateFile;
	const QString m_inputFile;
	
	volatile bool *m_abortFlag;
};
