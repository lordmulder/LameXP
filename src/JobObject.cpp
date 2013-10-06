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

#include "JobObject.h"

#include "Global.h"

#include <QProcess>

//Windows includes
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <MMSystem.h>
#include <ShellAPI.h>
#include <WinInet.h>

JobObject::JobObject(void)
:
	m_hJobObject(NULL)
{
	HANDLE jobObject = CreateJobObject(NULL, NULL);
	if((jobObject != NULL) && (jobObject != INVALID_HANDLE_VALUE))
	{
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobExtendedLimitInfo;
		memset(&jobExtendedLimitInfo, 0, sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));
		memset(&jobExtendedLimitInfo.BasicLimitInformation, 0, sizeof(JOBOBJECT_BASIC_LIMIT_INFORMATION));
		jobExtendedLimitInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;
		if(SetInformationJobObject(jobObject, JobObjectExtendedLimitInformation, &jobExtendedLimitInfo, sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION)))
		{
			m_hJobObject = jobObject;
		}
		else
		{
			qWarning("Failed to set job object information!");
			CloseHandle(jobObject);
		}
	}
	else
	{
		qWarning("Failed to create the job object!");
	}
}

JobObject::~JobObject(void)
{
	if(m_hJobObject)
	{
		CloseHandle(m_hJobObject);
		m_hJobObject = NULL;
	}
}

bool JobObject::addProcessToJob(const QProcess *proc)
{
	if(m_hJobObject)
	{
		if(AssignProcessToJobObject(m_hJobObject, proc->pid()->hProcess))
		{
			return true;
		}
		else
		{
			qWarning("Failed to assign process to job object!");
			return false;
		}
	}
	else
	{
		qWarning("Cannot assign process to job: No job bject available!");
		return false;
	}
}
