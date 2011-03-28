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

#include "Thread_DiskObserver.h"

#include "Global.h"

#include <QDir>
#include <Windows.h>

#define MIN_DISKSPACE 104857600LL //100 MB

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

DiskObserverThread::DiskObserverThread(const QString &path)
:
	m_path(makeRootDir(path))
{
	m_terminated = false;
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
		FatalAppExit(0, L"Unhandeled exception error, application will exit!");
		TerminateProcess(GetCurrentProcess(), -1);
	}
}

void DiskObserverThread::observe(void)
{
	__int64 freeSpace, minimumSpace = MIN_DISKSPACE;

	while(!m_terminated)
	{
		freeSpace = lamexp_free_diskspace(m_path);
		if(freeSpace < minimumSpace)
		{
			qWarning64("Free diskspace on '%1' dropped below %2 MB, only %3 MB free!", m_path, QString::number(minimumSpace / 1048576), QString::number(freeSpace / 1048576));
			emit messageLogged(tr("Low diskspace on drive '%1' detected (only %2 MB are free), problems can occur!").arg(QDir::toNativeSeparators(m_path), QString::number(freeSpace / 1048576)), true);
			minimumSpace = min(freeSpace, (minimumSpace >> 1));
		}
		Sleep(1000);
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
