///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2021 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU GENERAL PUBLIC LICENSE as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version; always including the non-optional
// LAMEXP GNU GENERAL PUBLIC LICENSE ADDENDUM. See "License.txt" file!
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
* Wait for process to terminate while processing its output
*/
AbstractTool::result_t AbstractTool::awaitProcess(QProcess &process, QAtomicInt &abortFlag, int *const exitCode)
{
	return awaitProcess(process, abortFlag, [](const QString &text) { return false; }, exitCode);
}

/*
* Wait for process to terminate while processing its output
*/
AbstractTool::result_t AbstractTool::awaitProcess(QProcess &process, QAtomicInt &abortFlag, std::function<bool(const QString &text)> &&handler, int *const exitCode)
{
	bool bTimeout = false;
	bool bAborted = false;

	QString lastText;

	while (process.state() != QProcess::NotRunning)
	{
		if (CHECK_FLAG(abortFlag))
		{
			process.kill();
			bAborted = true;
			emit messageLogged("\nABORTED BY USER !!!");
			break;
		}

		process.waitForReadyRead(m_processTimeoutInterval);
		if (!process.bytesAvailable() && process.state() == QProcess::Running)
		{
			process.kill();
			qWarning("Tool process timed out <-- killing!");
			emit messageLogged("\nPROCESS TIMEOUT !!!");
			bTimeout = true;
			break;
		}

		while (process.bytesAvailable() > 0)
		{
			QByteArray line = process.readLine();
			if (line.size() > 0)
			{
				line.replace('\r', char(0x20)).replace('\b', char(0x20)).replace('\t', char(0x20));
				const QString text = QString::fromUtf8(line.constData()).simplified();
				if (!text.isEmpty())
				{
					if (!handler(text))
					{
						if (text.compare(lastText, Qt::CaseInsensitive) != 0)
						{
							emit messageLogged(lastText = text);
						}
					}
				}
			}
		}
	}

	process.waitForFinished();
	if (process.state() != QProcess::NotRunning)
	{
		process.kill();
		process.waitForFinished(-1);
	}

	if (exitCode)
	{
		*exitCode = process.exitCode();
	}

	emit messageLogged(QString().sprintf("\nExited with code: 0x%04X", process.exitCode()));
	if (!(bAborted || bTimeout)) emit statusUpdated(100);

	if (bAborted || bTimeout || (process.exitCode() != EXIT_SUCCESS))
	{
		return bAborted ? RESULT_ABORTED : (bTimeout ? RESULT_TIMEOUT : RESULT_FAILURE);
	}

	return RESULT_SUCCESS;
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


