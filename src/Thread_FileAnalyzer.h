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

#pragma once

//#include "Model_AudioFile.h"

#include <QThread>
#include <QStringList>

class AudioFileModel;
class QFile;
class QDir;
class QFileInfo;

////////////////////////////////////////////////////////////
// Splash Thread
////////////////////////////////////////////////////////////

class FileAnalyzer: public QThread
{
	Q_OBJECT

public:
	FileAnalyzer(const QStringList &inputFiles);
	void run();
	bool getSuccess(void) { return !isRunning() && m_bSuccess; }
	unsigned int filesAccepted(void);
	unsigned int filesRejected(void);
	unsigned int filesDenied(void);
	unsigned int filesDummyCDDA(void);
	unsigned int filesCueSheet(void);

signals:
	void fileSelected(const QString &fileName);
	void fileAnalyzed(const AudioFileModel &file);

public slots:
	void abortProcess(void) { m_abortFlag = true; }

private:
	enum section_t
	{
		sectionGeneral,
		sectionAudio,
		sectionOther
	};
	enum cover_t
	{
		coverNone,
		coverJpeg,
		coverPng,
		coverGif
	};
	enum fileType_t
	{
		fileTypeNormal = 0,
		fileTypeCDDA = 1,
		fileTypeDenied = 2,
		fileTypeSkip = 3
	};

	const AudioFileModel analyzeFile(const QString &filePath, int *type);
	void updateInfo(AudioFileModel &audioFile, const QString &key, const QString &value);
	void updateSection(const QString &section);
	unsigned int parseYear(const QString &str);
	unsigned int parseDuration(const QString &str);
	bool checkFile_CDDA(QFile &file);
	void retrieveCover(AudioFileModel &audioFile, const QString &filePath);
	bool analyzeAvisynthFile(const QString &filePath, AudioFileModel &info);

	const QString m_mediaInfoBin;
	const QString m_avs2wavBin;

	QStringList m_inputFiles;
	QStringList m_recentlyAdded;
	section_t m_currentSection;
	cover_t m_currentCover;
	unsigned int m_filesAccepted;
	unsigned int m_filesRejected;
	unsigned int m_filesDenied;
	unsigned int m_filesDummyCDDA;
	unsigned int m_filesCueSheet;
	
	volatile bool m_abortFlag;

	bool m_bAborted;
	bool m_bSuccess;
};
