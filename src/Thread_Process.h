///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2010 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include <QThread>
#include <QUuid>

#include "Model_AudioFile.h"
#include "Encoder_Abstract.h"

class ProcessThread: public QThread
{
	Q_OBJECT

public:
	ProcessThread(const AudioFileModel &audioFile, const QString &outputDirectory, AbstractEncoder *encoder);
	~ProcessThread(void);
	void run();
	void abort() { m_aborted = true; }
	QUuid getId() { return m_jobId; }

private slots:
	void handleUpdate(int progress);

signals:
	void processStateInitialized(const QUuid &jobId, const QString &jobName, const QString &jobInitialStatus, int jobInitialState);
	void processStateChanged(const QUuid &jobId, const QString &newStatus, int newState);

private:
	QString generateOutFileName(void);
	
	const QUuid m_jobId;
	AudioFileModel m_audioFile;
	AbstractEncoder *m_encoder;
	const QString m_outputDirectory;
	volatile bool m_aborted;
};
