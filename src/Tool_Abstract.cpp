///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2013 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Tool_Abstract.h"

#include "Global.h"
#include "JobObject.h"

#include <QProcess>
#include <QMutex>
#include <QMutexLocker>
#include <QLibrary>
#include <QProcessEnvironment>
#include <QDir>

/*
 * Static vars
 */
quint64 AbstractTool::s_lastLaunchTime = 0ui64;
QMutex AbstractTool::s_mutex_startProcess;
JobObject *AbstractTool::s_jobObject = NULL;
unsigned int AbstractTool::s_jobObjRefCount = 0U;

/*
 * Const
 */
static const unsigned int START_DELAY = 333;					//in milliseconds
static const quint64 START_DELAY_NANO = START_DELAY * 1000 * 10; //in 100-nanosecond intervals

/*
 * Constructor
 */
AbstractTool::AbstractTool(void)
{
	QMutexLocker lock(&s_mutex_startProcess);

	if(s_jobObjRefCount < 1U)
	{
		s_jobObject = new JobObject();
		s_jobObjRefCount = 1U;
	}
	else
	{
		s_jobObjRefCount++;
	}

	m_firstLaunch = true;
}

/*
 * Destructor
 */
AbstractTool::~AbstractTool(void)
{
	QMutexLocker lock(&s_mutex_startProcess);

	if(s_jobObjRefCount >= 1U)
	{
		s_jobObjRefCount--;
		if(s_jobObjRefCount < 1U)
		{
			LAMEXP_DELETE(s_jobObject);
		}
	}
}

/*
 * Initialize and launch process object
 */
bool AbstractTool::startProcess(QProcess &process, const QString &program, const QStringList &args)
{
	QMutexLocker lock(&s_mutex_startProcess);

	if(lamexp_current_file_time() <= s_lastLaunchTime)
	{
		lamexp_sleep(START_DELAY);
	}

	emit messageLogged(commandline2string(program, args) + "\n");

	QProcessEnvironment env = process.processEnvironment();
	if(env.isEmpty()) env = QProcessEnvironment::systemEnvironment();
	env.insert("TEMP", QDir::toNativeSeparators(lamexp_temp_folder2()));
	env.insert("TMP", QDir::toNativeSeparators(lamexp_temp_folder2()));
	process.setProcessEnvironment(env);
		
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.setReadChannel(QProcess::StandardOutput);
	process.start(program, args);
	
	if(process.waitForStarted())
	{
		if(s_jobObject)
		{
			if(!s_jobObject->addProcessToJob(&process))
			{
				qWarning("Failed to assign process to job object!");
			}
		}

		lamexp_change_process_priority(&process, -1);
		lock.unlock();
		
		if(m_firstLaunch)
		{
			emit statusUpdated(0);
			m_firstLaunch = false;
		}
		
		s_lastLaunchTime = lamexp_current_file_time() + START_DELAY_NANO;
		return true;
	}

	emit messageLogged("Process creation has failed :-(");
	QString errorMsg= process.errorString().trimmed();
	if(!errorMsg.isEmpty()) emit messageLogged(errorMsg);

	process.kill();
	process.waitForFinished(-1);

	s_lastLaunchTime = lamexp_current_file_time() + START_DELAY_NANO;
	return false;
}

/*
 * Convert program arguments to single string
 */
QString AbstractTool::commandline2string(const QString &program, const QStringList &arguments)
{
	QString commandline = (program.contains(' ') ? QString("\"%1\"").arg(program) : program);
	
	for(int i = 0; i < arguments.count(); i++)
	{
		commandline += (arguments.at(i).contains(' ') ? QString(" \"%1\"").arg(arguments.at(i)) : QString(" %1").arg(arguments.at(i)));
	}

	return commandline;
}


