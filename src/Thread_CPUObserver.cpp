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

#include "Thread_CPUObserver.h"
#include "Global.h"

#include <QDir>
#include <QLibrary>

//Windows includes
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

////////////////////////////////////////////////////////////

typedef enum { SystemProcInfo = 8 } SYSTEM_INFO_CLASS;

typedef struct
{
	LARGE_INTEGER IdleTime;
	LARGE_INTEGER KrnlTime;
	LARGE_INTEGER UserTime;
	LARGE_INTEGER Reserved[2];
	ULONG Reserved2;
}
SYSTEM_PROC_INFO;

typedef BOOL (WINAPI *GetSystemTimesPtr)(LPFILETIME lpIdleTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime);
typedef LONG (WINAPI *NtQuerySystemInformationPtr)(SYSTEM_INFO_CLASS SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);

#define IS_OK(X) (((LONG)(X)) == ((LONG)0x00000000L))

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
	QLibrary kernel32("kernel32.dll"), ntdll("ntdll.dll");

	ULONG performanceInfoSize = 0;
	BYTE *performanceInfoBuffer = NULL;
	NtQuerySystemInformationPtr querySysInfo = NULL;
	GetSystemTimesPtr getSystemTimes = NULL;

	if(kernel32.load())
	{
		getSystemTimes = reinterpret_cast<GetSystemTimesPtr>(kernel32.resolve("GetSystemTimes"));
	}

	if(!getSystemTimes)
	{
		qWarning("GetSystemTimes() not found, falling back to NtQueryInformationProcess().");
		if(ntdll.load())
		{
			querySysInfo = reinterpret_cast<NtQuerySystemInformationPtr>(ntdll.resolve("NtQuerySystemInformation"));
			if(querySysInfo)
			{
				querySysInfo(SystemProcInfo, &performanceInfoBuffer, 0, &performanceInfoSize);
				if(performanceInfoSize < sizeof(SYSTEM_PROC_INFO)) performanceInfoSize = sizeof(SYSTEM_PROC_INFO);
				performanceInfoBuffer = new BYTE[performanceInfoSize];
			}
		}
	}

	if(getSystemTimes || (querySysInfo && performanceInfoBuffer))
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
			bool ok = false;
			
			if(getSystemTimes)
			{
				if(ok = getSystemTimes(&idlTime, &sysTime, &usrTime))
				{
					sys[1] = sys[0]; sys[0] = filetime2ulonglong(&sysTime);
					usr[1] = usr[0]; usr[0] = filetime2ulonglong(&usrTime);
					idl[1] = idl[0]; idl[0] = filetime2ulonglong(&idlTime);
				}
			}
			else
			{
				if(ok = IS_OK(querySysInfo(SystemProcInfo, performanceInfoBuffer, performanceInfoSize, NULL)))
				{
					sys[1] = sys[0]; sys[0] = reinterpret_cast<SYSTEM_PROC_INFO*>(performanceInfoBuffer)[0].KrnlTime.QuadPart;
					usr[1] = usr[0]; usr[0] = reinterpret_cast<SYSTEM_PROC_INFO*>(performanceInfoBuffer)[0].UserTime.QuadPart;
					idl[1] = idl[0]; idl[0] = reinterpret_cast<SYSTEM_PROC_INFO*>(performanceInfoBuffer)[0].IdleTime.QuadPart;
				}
			}

			if(ok)
			{
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
	else
	{
		qWarning("NtQueryInformationProcess() not available, giving up!");
	}

	LAMEXP_DELETE_ARRAY(performanceInfoBuffer);
}

////////////////////////////////////////////////////////////
// SLOTS
////////////////////////////////////////////////////////////

/*NONE*/

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/
