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

#include "Thread_CPUObserver.h"
#include "Global.h"

#include <QDir>

//Windows includes
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

CPUObserverThread::CPUObserverThread(void)
{
}

CPUObserverThread::~CPUObserverThread(void)
{
}

////////////////////////////////////////////////////////////
// Protected functions
////////////////////////////////////////////////////////////

void CPUObserverThread::run(void)
{
	qDebug("CPU observer started!");

	try
	{
		observe();
	}
	catch(const std::exception &error)
	{
		fflush(stdout); fflush(stderr);
		fprintf(stderr, "\nGURU MEDITATION !!!\n\nException error:\n%s\n", error.what());
		lamexp_fatal_exit(L"Unhandeled C++ exception error, application will exit!");
	}
	catch(...)
	{
		fflush(stdout); fflush(stderr);
		fprintf(stderr, "\nGURU MEDITATION !!!\n\nUnknown exception error!\n");
		lamexp_fatal_exit(L"Unhandeled C++ exception error, application will exit!");
	}

	while(m_semaphore.available()) m_semaphore.tryAcquire();
}

ULONGLONG CPUObserverThread::filetime2ulonglong(const void *ftime)
{
	ULARGE_INTEGER tmp; tmp.QuadPart = 0UI64;
	const FILETIME* fileTime = reinterpret_cast<const FILETIME*>(ftime);
	tmp.LowPart = fileTime->dwLowDateTime;
	tmp.HighPart = fileTime->dwHighDateTime;
	return tmp.QuadPart;
}

void CPUObserverThread::observe(void)
{
	bool first = true;
	double previous = -1.0;
	FILETIME sysTime, usrTime, idlTime;
	ULONGLONG sys[2], usr[2], idl[2];

	for(size_t i = 0; i < 2; i++)
	{
		sys[i] = 0; usr[i] = 0; idl[i] = 0;
	}

	forever
	{
		if(GetSystemTimes(&idlTime, &sysTime, &usrTime))
		{
			sys[1] = sys[0]; sys[0] = filetime2ulonglong(&sysTime);
			usr[1] = usr[0]; usr[0] = filetime2ulonglong(&usrTime);
			idl[1] = idl[0]; idl[0] = filetime2ulonglong(&idlTime);

			if(first)
			{
				first = false;
				emit currentUsageChanged(1.0);
				msleep(250);
				continue;
			}

			ULONGLONG timeIdl = (idl[0] - idl[1]); //Idle time only
			ULONGLONG timeSys = (sys[0] - sys[1]); //Kernel mode time (incl. Idle time!)
			ULONGLONG timeUsr = (usr[0] - usr[1]); //User mode time only
				
			ULONGLONG timeSum = timeUsr + timeSys; //Overall CPU time that has elapsed
			ULONGLONG timeWrk = timeSum - timeIdl; //Time the CPU spent working

			if(timeSum > 0)
			{
				double current = static_cast<double>(timeWrk) / static_cast<double>(timeSum);
				if(current != previous)
				{
					emit currentUsageChanged(current);
					previous = current;
				}
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
