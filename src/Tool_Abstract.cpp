///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2017 LoRd_MuldeR <MuldeR2@GMX.de>
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

//MUtils
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>
#include <MUtils/JobObject.h>

//Qt
#include <QProcess>
#include <QMutex>
#include <QMutexLocker>
#include <QProcessEnvironment>
#include <QDir>
#include <QElapsedTimer>

/*
 * Static Objects
 */
QScopedPointer<MUtils::JobObject> AbstractTool::s_jobObjectInstance;
QScopedPointer<QElapsedTimer>     AbstractTool::s_startProcessTimer;

/*
 * Synchronization
 */
QMutex AbstractTool::s_startProcessMutex;
QMutex AbstractTool::s_createObjectMutex;

/*
 * Ref Counter
 */
quint64 AbstractTool::s_referenceCounter = 0ui64;

/*
 * Const
 */
static const qint64 START_DELAY = 64i64;	//in milliseconds

/*
 * Constructor
 */
AbstractTool::AbstractTool(void)
:
	m_firstLaunch(true)

{
	QMutexLocker lock(&s_createObjectMutex);

	if(s_referenceCounter++ == 0)
	{
		s_jobObjectInstance.reset(new MUtils::JobObject());
		s_startProcessTimer.reset(new QElapsedTimer());
		if(!MUtils::OS::setup_timer_resolution())
		{
			qWarning("Failed to setup system timer resolution!");
		}
	}
}

/*
 * Destructor
 */
AbstractTool::~AbstractTool(void)
{
	QMutexLocker lock(&s_createObjectMutex);

	if(--s_referenceCounter == 0)
	{
		s_jobObjectInstance.reset(NULL);
		s_startProcessTimer.reset(NULL);
		if(!MUtils::OS::reset_timer_resolution())
		{
			qWarning("Failed to reset system timer resolution!");
		}
	}
}

/*
 * Initialize and launch process object
 */
bool AbstractTool::startProcess(QProcess &process, const QString &program, const QStringList &args, const QString &workingDir)
{
	QMutexLocker lock(&s_startProcessMutex);
	
	if((!s_startProcessTimer.isNull()) && s_startProcessTimer->isValid())
	{
		qint64 elapsed = s_startProcessTimer->elapsed();
		while(elapsed < START_DELAY)
		{
			lock.unlock();
			MUtils::OS::sleep_ms((size_t)(START_DELAY - elapsed));
			lock.relock();
			elapsed = s_startProcessTimer->elapsed();
		}
	}

	emit messageLogged(commandline2string(program, args) + "\n");
	MUtils::init_process(process, workingDir.isEmpty() ? QFileInfo(program).absolutePath() : workingDir);

	process.start(program, args);
	
	if(process.waitForStarted())
	{
		if(!s_jobObjectInstance.isNull())
		{
			if(!s_jobObjectInstance->addProcessToJob(&process))
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
		
		s_startProcessTimer->start();
		return true;
	}

	emit messageLogged("Process creation has failed :-(");
	QString errorMsg= process.errorString().trimmed();
	if(!errorMsg.isEmpty()) emit messageLogged(errorMsg);

	process.kill();
	process.waitForFinished(-1);

	s_startProcessTimer->start();
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


