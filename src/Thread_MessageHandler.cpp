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

#include "Thread_MessageHandler.h"

#include "Global.h"

#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QMessageBox>

#include <limits.h>

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

MessageHandlerThread::MessageHandlerThread(void)
{
	m_aborted = false;
	m_parameter = new char[4096];
}

MessageHandlerThread::~MessageHandlerThread(void)
{
	delete [] m_parameter;
}

void MessageHandlerThread::run()
{
	m_aborted = false;
	setTerminationEnabled(true);

	while(!m_aborted)
	{
		unsigned int command = 0;
		lamexp_ipc_read(&command, m_parameter, 4096);
		if(!command) continue;

		switch(command)
		{
		case 1:
			emit fileReceived(QString::fromUtf8(m_parameter));
			break;
		case 2:
			emit folderReceived(QString::fromUtf8(m_parameter), false);
			break;
		case 3:
			emit folderReceived(QString::fromUtf8(m_parameter), true);
			break;
		case 666:
			if(!_stricmp(m_parameter, "Force!"))
			{
				_exit(-2);
			}
			else
			{
				emit killSignalReceived();
			}
			break;
		case UINT_MAX:
			emit otherInstanceDetected();
			break;
		default:
			qWarning("Received an unknown IPC message! (command=%u)", command);
			break;
		}
	}
}

void MessageHandlerThread::stop(void)
{
	if(!m_aborted)
	{
		m_aborted = true;
		lamexp_ipc_send(0, NULL);
	}
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/