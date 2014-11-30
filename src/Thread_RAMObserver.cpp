///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2014 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Thread_RAMObserver.h"
#include "Global.h"

//MUtils
#include <MUtils/OSSupport.h>
#include <MUtils/Exception.h>

//Qt
#include <QDir>

//Windows includes
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

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
