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

#include <MUtils/Global.h>

#include <QThread>
#include <QStringList>
#include <QHash>
#include <QSet>

class AudioFileModel;
class QFile;
class QDir;
class QFileInfo;
class LockedFile;
class QThreadPool;
class QElapsedTimer;

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
	bool getSuccess(void) { return (!isRunning()) && (!m_bAborted) && MUTILS_BOOLIFY(m_bSuccess); }

	unsigned int filesAccepted(void);
	unsigned int filesRejected(void);
	unsigned int filesDenied(void);
	unsigned int filesDummyCDDA(void);
	unsigned int filesCueSheet(void);

signals:
	void fileSelected(const QString &fileName);
	void fileAnalyzed(const AudioFileModel &file);
	void progressValChanged(unsigned int);
	void progressMaxChanged(unsigned int);

public slots:
	void abortProcess(void) { m_bAborted.ref(); exit(-1); }

private slots:
	void initializeTasks(void);
	void taskFileAnalyzed(const unsigned int taskId, const int fileType, const AudioFileModel &file);
	void taskThreadFinish(const unsigned int);

private:
	bool analyzeNextFile(void);
	void handlePlaylistFiles(void);
	bool createTemplate(void);

	QThreadPool *m_pool;
	QElapsedTimer *m_timer;

	unsigned int m_tasksCounterNext;
	unsigned int m_tasksCounterDone;
	unsigned int m_completedCounter;

	unsigned int m_filesAccepted;
	unsigned int m_filesRejected;
	unsigned int m_filesDenied;
	unsigned int m_filesDummyCDDA;
	unsigned int m_filesCueSheet;

	QStringList m_inputFiles;
	LockedFile *m_templateFile;

	QSet<unsigned int> m_completedTaskIds;
	QSet<unsigned int> m_runningTaskIds;
	QHash<unsigned int, AudioFileModel> m_completedFiles;

	static const char *g_tags_gen[];
	static const char *g_tags_aud[];

	QAtomicInt m_bAborted;
	QAtomicInt m_bSuccess;
};
