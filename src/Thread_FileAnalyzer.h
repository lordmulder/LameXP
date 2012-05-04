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
class LockedFile;

////////////////////////////////////////////////////////////
// Splash Thread
////////////////////////////////////////////////////////////

class FileAnalyzer: public QThread
{
	Q_OBJECT

public:
	FileAnalyzer(const QStringList &inputFiles);
	~FileAnalyzer(void);
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
	const AudioFileModel analyzeFile(const QString &filePath, int *type);
	bool createTemplate(void);

	QStringList m_inputFiles;
	LockedFile *m_templateFile;
	
	volatile bool m_abortFlag;

	static const char *g_tags_gen[];
	static const char *g_tags_aud[];

	bool m_bAborted;
	bool m_bSuccess;
};
