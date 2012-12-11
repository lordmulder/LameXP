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

#include "Thread_DiskObserver.h"

#include "Global.h"
#include "Model_Progress.h"

#include <QDir>

#define MIN_DISKSPACE 104857600ui64 //100 MB

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

DiskObserverThread::DiskObserverThread(const QString &path)
:
	m_path(makeRootDir(path))
{
}

DiskObserverThread::~DiskObserverThread(void)
{
}

////////////////////////////////////////////////////////////
// Protected functions
////////////////////////////////////////////////////////////

void DiskObserverThread::run(void)
{
	qDebug("DiskSpace observer started!");

	try
	{
		observe();
	}
	catch(...)
	{
		fflush(stdout);
		fflush(stderr);
		fprintf(stderr, "\nGURU MEDITATION !!!\n");
		lamexp_fatal_exit(L"Unhandeled exception error, application will exit!");
	}

	while(m_semaphore.available()) m_semaphore.tryAcquire();
}

void DiskObserverThread::observe(void)
{
	unsigned __int64 minimumSpace = MIN_DISKSPACE;
	unsigned __int64 freeSpace, previousSpace = 0ui64;
	bool ok = false;

	forever
	{
		freeSpace = lamexp_free_diskspace(m_path, &ok);
		if(ok)
		{
			if(freeSpace < minimumSpace)
			{
				qWarning("Free diskspace on '%s' dropped below %s MB, only %s MB free!", m_path.toUtf8().constData(), QString::number(minimumSpace / 1048576ui64).toUtf8().constData(), QString::number(freeSpace / 1048576ui64).toUtf8().constData());
				emit messageLogged(tr("Low diskspace on drive '%1' detected (only %2 MB are free), problems can occur!").arg(QDir::toNativeSeparators(m_path), QString::number(freeSpace / 1048576ui64)), ProgressModel::SysMsg_Warning);
				minimumSpace = qMin(freeSpace, (minimumSpace >> 1));
			}
			if(freeSpace != previousSpace)
			{
				emit freeSpaceChanged(freeSpace);
				previousSpace = freeSpace;
			}
		}
		if(m_semaphore.tryAcquire(1, 2000)) break;
	}
}

QString DiskObserverThread::makeRootDir(const QString &baseDir)
{
	QDir dir(baseDir);
	
	if(!dir.exists())
	{
		return baseDir;
	}
	
	bool success = true;
	
	while(success)
	{
		success = dir.cdUp();
	}

	return dir.canonicalPath();
}

////////////////////////////////////////////////////////////
// SLOTS
////////////////////////////////////////////////////////////

/*NONE*/

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/
