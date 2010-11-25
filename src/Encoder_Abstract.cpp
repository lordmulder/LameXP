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

#include "Encoder_Abstract.h"

#include <QStringList>
#include <QProcess>
#include <QMutex>
#include <QMutexLocker>
#include <QLibrary>
#include <Windows.h>

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

QMutex *AbstractEncoder::m_mutex_startProcess = NULL;
HANDLE AbstractEncoder::m_handle_jobObject = NULL;

AbstractEncoder::AbstractEncoder(void)
{
	typedef HANDLE (WINAPI *CreateJobObjectFun)(__in_opt LPSECURITY_ATTRIBUTES lpJobAttributes, __in_opt LPCSTR lpName);
	typedef BOOL (WINAPI *SetInformationJobObjectFun)(__in HANDLE hJob, __in JOBOBJECTINFOCLASS JobObjectInformationClass, __in_bcount(cbJobObjectInformationLength) LPVOID lpJobObjectInformation, __in DWORD cbJobObjectInformationLength);

	static CreateJobObjectFun CreateJobObjectPtr = NULL;
	static SetInformationJobObjectFun SetInformationJobObjectPtr = NULL;

	if(!m_mutex_startProcess)
	{
		m_mutex_startProcess = new QMutex();
	}

	if(!m_handle_jobObject)
	{
		if(!CreateJobObjectPtr || !SetInformationJobObjectPtr)
		{
			QLibrary Kernel32Lib("kernel32.dll");
			CreateJobObjectPtr = (CreateJobObjectFun) Kernel32Lib.resolve("CreateJobObjectA");
			SetInformationJobObjectPtr = (SetInformationJobObjectFun) Kernel32Lib.resolve("SetInformationJobObject");
		}
		if(CreateJobObjectPtr && SetInformationJobObjectPtr)
		{
			m_handle_jobObject = CreateJobObjectPtr(NULL, NULL);
			if(m_handle_jobObject == INVALID_HANDLE_VALUE)
			{
				m_handle_jobObject = NULL;
			}
			if(m_handle_jobObject)
			{
				JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobExtendedLimitInfo;
				memset(&jobExtendedLimitInfo, 0, sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));
				jobExtendedLimitInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;
				SetInformationJobObjectPtr(m_handle_jobObject, JobObjectExtendedLimitInformation, &jobExtendedLimitInfo, sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION));
			}
		}
	}

	m_configBitrate = 0;
	m_configRCMode = 0;
}

AbstractEncoder::~AbstractEncoder(void)
{
}

/*
 * Setters
 */

void AbstractEncoder::setBitrate(int bitrate) { m_configBitrate = max(0, bitrate); }
void AbstractEncoder::setRCMode(int mode) { m_configRCMode = max(0, mode); }

/*
 * Auxiliary functions
 */

bool AbstractEncoder::startProcess(QProcess &process, const QString &program, const QStringList &args)
{
	typedef BOOL (WINAPI *AssignProcessToJobObjectFun)(__in HANDLE hJob, __in HANDLE hProcess);
	static AssignProcessToJobObjectFun AssignProcessToJobObjectPtr = NULL;
	
	QMutexLocker lock(m_mutex_startProcess);
	
	emit messageLogged(commandline2string(program, args) + "\n");

	if(!AssignProcessToJobObjectPtr)
	{
		QLibrary Kernel32Lib("kernel32.dll");
		AssignProcessToJobObjectPtr = (AssignProcessToJobObjectFun) Kernel32Lib.resolve("AssignProcessToJobObject");
	}
	
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.setReadChannel(QProcess::StandardOutput);
	process.start(program, args);
	
	if(process.waitForStarted())
	{
		
		if(AssignProcessToJobObjectPtr)
		{
			AssignProcessToJobObjectPtr(m_handle_jobObject, process.pid()->hProcess);
		}
		if(!SetPriorityClass(process.pid()->hProcess, BELOW_NORMAL_PRIORITY_CLASS))
		{
			SetPriorityClass(process.pid()->hProcess, IDLE_PRIORITY_CLASS);
		}
		lock.unlock();
		emit statusUpdated(0);
		return true;
	}

	return false;
}

QString AbstractEncoder::commandline2string(const QString &program, const QStringList &arguments)
{
	QString commandline = (program.contains(' ') ? QString("\"%1\"").arg(program) : program);
	
	for(int i = 0; i < arguments.count(); i++)
	{
		commandline += (arguments.at(i).contains(' ') ? QString(" \"%1\"").arg(arguments.at(i)) : QString(" %1").arg(arguments.at(i)));
	}

	return commandline;
}
