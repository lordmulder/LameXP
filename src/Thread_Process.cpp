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

#include "Thread_Process.h"

#include "Global.h"
#include "Model_Progress.h"

#include <QUuid.h>
#include <limits.h>
#include <time.h>

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

ProcessThread::ProcessThread(void)
	: m_jobId(QUuid::createUuid().toString()), m_aborted(false)
{
}

ProcessThread::~ProcessThread(void)
{
}

void ProcessThread::run()
{
	m_aborted = false;

	qDebug("Process thread %s has started.", m_jobId.toLatin1().constData());
	emit processStateInitialized(m_jobId, "Slime - Der Tod Ist Ein Meister Aus Deutschland.mp3", "Starting...", ProgressModel::JobRunning);
	
	QUuid uuid = QUuid::createUuid();
	qsrand(uuid.data1 * uuid.data2 * uuid.data3);
	unsigned long delay = 250 + (qrand() % 500);


	for(int i = 1; i <= 100; i++)
	{
		if(m_aborted)
		{
			emit processStateChanged(m_jobId, "Aborted.", ProgressModel::JobFailed);
			return;
		}

		QThread::msleep(delay);
		emit processStateChanged(m_jobId, QString("Encoding (%1%)").arg(i), ProgressModel::JobRunning);
		qDebug("Process thread is alive.");
	}

	emit processStateChanged(m_jobId, "Done (100%)", ProgressModel::JobComplete);
	qDebug("Process thread is about to die...");
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/