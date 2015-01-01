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

#include "Thread_MessageProducer.h"

//Internal
#include "Global.h"
#include "IPCCommands.h"

//MUtils
#include <MUtils/Global.h>
#include <MUtils/IPCChannel.h>
#include <MUtils/OSSupport.h>

//Qt
#include <QStringList>
#include <QApplication>
#include <QFileInfo>
#include <QDir>

//CRT
#include <limits.h>

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

MessageProducerThread::MessageProducerThread(MUtils::IPCChannel *const ipcChannel)
:
	m_ipcChannel(ipcChannel)
{
}

MessageProducerThread::~MessageProducerThread(void)
{
}

void MessageProducerThread::run()
{
	setTerminationEnabled(true);
	bool bSentFiles = false;
	const QStringList &arguments = MUtils::OS::arguments();

	for(int i = 0; i < arguments.count(); i++)
	{
		if(!arguments[i].compare("--kill", Qt::CaseInsensitive))
		{
			if(!m_ipcChannel->send(IPC_CMD_TERMINATE, IPC_FLAG_NONE, NULL))
			{
				qWarning("Failed to send IPC message!");
			}
			return;
		}
		if(!arguments[i].compare("--force-kill", Qt::CaseInsensitive))
		{
			if(!m_ipcChannel->send(IPC_CMD_TERMINATE, IPC_FLAG_FORCE, NULL))
			{
				qWarning("Failed to send IPC message!");
			}
			return;
		}
	}

	for(int i = 0; i < arguments.count() - 1; i++)
	{
		if(!arguments[i].compare("--add", Qt::CaseInsensitive))
		{
			QFileInfo file = QFileInfo(arguments[++i]);
			if(file.exists() && file.isFile())
			{
				if(!m_ipcChannel->send(IPC_CMD_ADD_FILE, IPC_FLAG_NONE, MUTILS_UTF8(file.canonicalFilePath())))
				{
					qWarning("Failed to send IPC message!");
				}
			}
			bSentFiles = true;
		}
		if(!arguments[i].compare("--add-folder", Qt::CaseInsensitive))
		{
			QDir dir = QDir(arguments[++i]);
			if(dir.exists())
			{
				if(!m_ipcChannel->send(IPC_CMD_ADD_FOLDER, IPC_FLAG_NONE, MUTILS_UTF8(dir.canonicalPath())))
				{
					qWarning("Failed to send IPC message!");
				}
			}
			bSentFiles = true;
		}
		if(!arguments[i].compare("--add-recursive", Qt::CaseInsensitive))
		{
			QDir dir = QDir(arguments[++i]);
			if(dir.exists())
			{
				if(!m_ipcChannel->send(IPC_CMD_ADD_FOLDER, IPC_FLAG_ADD_RECURSIVE, MUTILS_UTF8(dir.canonicalPath())))
				{
					qWarning("Failed to send IPC message!");
				}
			}
			bSentFiles = true;
		}
	}

	if(!bSentFiles)
	{
		if(!m_ipcChannel->send(IPC_CMD_PING, IPC_FLAG_NONE, "Use running instance!"))
		{
			qWarning("Failed to send IPC message!");
		}
	}
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/