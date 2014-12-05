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

#include "Tool_Abstract.h"

//Internal
#include "Global.h"
#include "JobObject.h"

//MUtils
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>

//Qt
#include <QProcess>
#include <QMutex>
#include <QMutexLocker>
#include <QLibrary>
#include <QProcessEnvironment>
#include <QDir>

/*
 * Job Object
 */
QScopedPointer<JobObject> AbstractTool::s_jobObject;
QMutex                    AbstractTool::s_jobObjMtx;

/*
 * Process Timer
 */
quint64 AbstractTool::s_startProcessTimer = 0ui64;
QMutex  AbstractTool::s_startProcessMutex;

/*
 * Const
 */
static const unsigned int START_DELAY = 50U;								//in milliseconds
static const quint64 START_DELAY_NANO = quint64(START_DELAY) * 10000ui64;	//in 100-nanosecond intervals

/*
 * Constructor
 */
AbstractTool::AbstractTool(void)
{
	QMutexLocker lock(&s_jobObjMtx);

	if(s_jobObject.isNull())
	{
		s_jobObject.reset(new JobObject());
	}

	m_firstLaunch = true;
}

/*
 * Destructor
 */
AbstractTool::~AbstractTool(void)
{
}

/*
 * Initialize and launch process object
 */
bool AbstractTool::startProcess(QProcess &process, const QString &program, const QStringList &args)
{
	QMutexLocker lock(&s_startProcessMutex);

	while(MUtils::OS::current_file_time() <= s_startProcessTimer)
	{
		lock.unlock();
		MUtils::OS::sleep_ms(START_DELAY);
		lock.relock();
	}

	emit messageLogged(commandline2string(program, args) + "\n");
	MUtils::init_process(process, QFileInfo(program).absolutePath());

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

		MUtils::OS::change_process_priority(&process, -1);
		
		if(m_firstLaunch)
		{
			emit statusUpdated(0);
			m_firstLaunch = false;
		}
		
		s_startProcessTimer = MUtils::OS::current_file_time() + START_DELAY_NANO;
		return true;
	}

	emit messageLogged("Process creation has failed :-(");
	QString errorMsg= process.errorString().trimmed();
	if(!errorMsg.isEmpty()) emit messageLogged(errorMsg);

	process.kill();
	process.waitForFinished(-1);

	s_startProcessTimer = MUtils::OS::current_file_time() + START_DELAY_NANO;
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


