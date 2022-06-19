///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2022 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Thread_MessageHandler.h"

//Internal
#include "Global.h"
#include "IPCCommands.h"

//MUtils
#include <MUtils/IPCChannel.h>

//Qt
#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QMessageBox>

//CRL
#include <limits.h>

#define TEST_FLAG(X) ((flags & (X)) == (X))

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

MessageHandlerThread::MessageHandlerThread(MUtils::IPCChannel *const ipcChannel)
:
	m_ipcChannel(ipcChannel)
{
}

MessageHandlerThread::~MessageHandlerThread(void)
{
}

void MessageHandlerThread::run()
{
	setTerminationEnabled(true);
	QStringList params;
	quint32 command = 0, flags = 0;

	while(!m_aborted)
	{
		if(!m_ipcChannel->read(command, flags, params))
		{
			qWarning("Failed to read next IPC message!");
			break;
		}
		
		if(command == IPC_CMD_NOOP)
		{
			continue;
		}

		switch(command)
		{
		case IPC_CMD_PING:
			emit otherInstanceDetected();
			break;
		case IPC_CMD_ADD_FILE:
			if(params.count() > 0 )
			{
				emit fileReceived(params.first());
			}
			break;
		case IPC_CMD_ADD_FOLDER:
			if(params.count() > 0 )
			{
				emit folderReceived(params.first(), TEST_FLAG(IPC_FLAG_ADD_RECURSIVE));
			}
			break;
		case IPC_CMD_TERMINATE:
			if(TEST_FLAG(IPC_FLAG_FORCE))
			{
				_exit(-2);
			}
			emit killSignalReceived();
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
		m_aborted.ref();
		m_ipcChannel->send(0, 0, QStringList());
	}
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/
