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

#include "Thread_RAMObserver.h"
#include "Global.h"

#include <QDir>

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

RAMObserverThread::RAMObserverThread(void)
{
}

RAMObserverThread::~RAMObserverThread(void)
{
}

////////////////////////////////////////////////////////////
// Protected functions
////////////////////////////////////////////////////////////

void RAMObserverThread::run(void)
{
	qDebug("RAM observer started!");

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

	while(m_semaphore.available()) m_semaphore.tryAcquire();
}

void RAMObserverThread::observe(void)
{
	MEMORYSTATUSEX memoryStatus;
	double previous = -1.0;

	forever
	{
		memset(&memoryStatus, 0, sizeof(MEMORYSTATUSEX));
		memoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
		
		if(GlobalMemoryStatusEx(&memoryStatus))
		{
			double current = static_cast<double>(memoryStatus.dwMemoryLoad) / 100.0;
			if(current != previous)
			{
				emit currentUsageChanged(current);
				previous = current;
			}
		}
		if(m_semaphore.tryAcquire(1, 2000)) break;
	}
}

////////////////////////////////////////////////////////////
// SLOTS
////////////////////////////////////////////////////////////

/*NONE*/

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/
