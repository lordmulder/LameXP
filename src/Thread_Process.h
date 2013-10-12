///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2013 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include <QRunnable>
#include <QUuid>
#include <QStringList>

#include "Model_AudioFile.h"
#include "Encoder_Abstract.h"

class AbstractFilter;
class WaveProperties;
class QThreadPool;
class QCoreApplication;

class ProcessThread: public QObject, public QRunnable
{
	Q_OBJECT

public:
	ProcessThread(const AudioFileModel &audioFile, const QString &outputDirectory, const QString &tempDirectory, AbstractEncoder *encoder, const bool prependRelativeSourcePath);
	~ProcessThread(void);
	
	bool init(void);
	bool start(QThreadPool *pool);
	
	QUuid getId(void) { return m_jobId; }
	void setRenamePattern(const QString &pattern);
	void setOverwriteMode(const bool bSkipExistingFile, const bool ReplacesExisting = false);
	void addFilter(AbstractFilter *filter);

public slots:
	void abort(void) { m_aborted = true; }

private slots:
	void handleUpdate(int progress);
	void handleMessage(const QString &line);

signals:
	void processStateInitialized(const QUuid &jobId, const QString &jobName, const QString &jobInitialStatus, int jobInitialState);
	void processStateChanged(const QUuid &jobId, const QString &newStatus, int newState);
	void processStateFinished(const QUuid &jobId, const QString &outFileName, int success);
	void processMessageLogged(const QUuid &jobId, const QString &line);
	void processFinished(void);

protected:
	virtual void run(void);

private:
	enum ProcessStep
	{
		DecodingStep = 0,
		AnalyzeStep = 1,
		FilteringStep = 2,
		EncodingStep = 3,
		UnknownStep = 4
	};
	
	void processFile();
	int generateOutFileName(QString &outFileName);
	QString applyRenamePattern(const QString &baseName, const AudioFileModel_MetaInfo &metaInfo);
	QString generateTempFileName(void);
	void insertDownmixFilter(void);
	void insertDownsampleFilter(void);

	volatile bool m_aborted;
	volatile int m_initialized;

	const QUuid m_jobId;
	AudioFileModel m_audioFile;
	AbstractEncoder *m_encoder;
	const QString m_outputDirectory;
	const QString m_tempDirectory;
	ProcessStep m_currentStep;
	QStringList m_tempFiles;
	const bool m_prependRelativeSourcePath;
	QList<AbstractFilter*> m_filters;
	QString m_renamePattern;
	bool m_overwriteSkipExistingFile;
	bool m_overwriteReplacesExisting;
	WaveProperties *m_propDetect;
	QString m_outFileName;
};
