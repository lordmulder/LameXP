///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2022 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU GENERAL PUBLIC LICENSE as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version; always including the non-optional
// LAMEXP GNU GENERAL PUBLIC LICENSE ADDENDUM. See "License.txt" file!
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

//Internal
#include "Global.h"
#include "Model_Progress.h"

//MUtils
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>
#include <MUtils/Exception.h>

//Qt
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
	catch(const std::exception &error)
	{
		MUTILS_PRINT_ERROR("\nGURU MEDITATION !!!\n\nException error:\n%s\n", error.what());
		MUtils::OS::fatal_exit(L"Unhandeled C++ exception error, application will exit!");
	}
	catch(...)
	{
		MUTILS_PRINT_ERROR("\nGURU MEDITATION !!!\n\nUnknown exception error!\n");
		MUtils::OS::fatal_exit(L"Unhandeled C++ exception error, application will exit!");
	}

	while(m_semaphore.available()) m_semaphore.tryAcquire();
}

void DiskObserverThread::observe(void)
{
	quint64 minimumSpace = MIN_DISKSPACE;
	quint64 previousSpace = quint64(-1);

	forever
	{
		quint64 freeSpace = 0ui64;
		if(MUtils::OS::free_diskspace(m_path, freeSpace))
		{
			if(freeSpace < minimumSpace)
			{
				qWarning("Free diskspace on '%s' dropped below %s MB, only %s MB free!", MUTILS_UTF8(m_path), MUTILS_UTF8(QString::number(minimumSpace / 1048576ui64)), MUTILS_UTF8(QString::number(freeSpace / 1048576ui64)));
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
