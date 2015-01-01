///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2015 LoRd_MuldeR <MuldeR2@GMX.de>
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
	m_aborted = false;
	m_parameter = new char[MUtils::IPCChannel::MAX_MESSAGE_LEN];
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
		unsigned int command = 0, flags = 0;
		if(!m_ipcChannel->read(command, flags, m_parameter, MUtils::IPCChannel::MAX_MESSAGE_LEN))
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
			emit fileReceived(QString::fromUtf8(m_parameter));
			break;
		case IPC_CMD_ADD_FOLDER:
			emit folderReceived(QString::fromUtf8(m_parameter), TEST_FLAG(IPC_FLAG_ADD_RECURSIVE));
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
		m_aborted = true;
		m_ipcChannel->send(0, 0, NULL);
	}
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/
