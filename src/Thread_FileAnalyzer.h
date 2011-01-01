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

//#include "Model_AudioFile.h"

#include <QThread>
#include <QStringList>

class AudioFileModel;

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

signals:
	void fileSelected(const QString &fileName);
	void fileAnalyzed(const AudioFileModel &file);

private:
	enum section_t
	{
		sectionGeneral,
		sectionAudio,
		sectionOther
	};

	const AudioFileModel analyzeFile(const QString &filePath);
	void updateInfo(AudioFileModel &audioFile, const QString &key, const QString &value);
	void updateSection(const QString &section);
	unsigned int parseYear(const QString &str);
	unsigned int parseDuration(const QString &str);

	QStringList m_inputFiles;
	const QString m_mediaInfoBin_x86;
	const QString m_mediaInfoBin_x64;
	section_t m_currentSection;
	unsigned int m_filesAccepted;
	unsigned int m_filesRejected;
	unsigned int m_filesDenied;
	bool m_bSuccess;
};
