///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2025 LoRd_MuldeR <MuldeR2@GMX.de>
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
	const MUtils::OS::ArgumentMap &arguments = MUtils::OS::arguments();

	//Kill application?
	if(arguments.contains("kill"))
	{
		if(!m_ipcChannel->send(IPC_CMD_TERMINATE, IPC_FLAG_NONE))
		{
			qWarning("Failed to send IPC message!");
		}
		return;
	}
	if(arguments.contains("force-kill"))
	{
		if(!m_ipcChannel->send(IPC_CMD_TERMINATE, IPC_FLAG_FORCE))
		{
			qWarning("Failed to send IPC message!");
		}
		return;
	}

	//Send file to "matser" instance
	foreach(const QString &value, arguments.values("add"))
	{
		if(!value.isEmpty())
		{
			const QFileInfo file = QFileInfo(value);
			if(file.exists() && file.isFile())
			{
				if(!m_ipcChannel->send(IPC_CMD_ADD_FILE, IPC_FLAG_NONE, QStringList() << file.canonicalFilePath()))
				{
					qWarning("Failed to send IPC message!");
				}
			}
			bSentFiles = true;
		}
	}
	foreach(const QString &value, arguments.values("add-folder"))
	{
		if(!value.isEmpty())
		{
			const QDir dir = QDir(value);
			if(dir.exists())
			{
				if(!m_ipcChannel->send(IPC_CMD_ADD_FOLDER, IPC_FLAG_NONE, QStringList() << dir.canonicalPath()))
				{
					qWarning("Failed to send IPC message!");
				}
			}
			bSentFiles = true;
		}
	}
	foreach(const QString &value, arguments.values("add-recursive"))
	{
		if(!value.isEmpty())
		{
			const QDir dir = QDir(value);
			if(dir.exists())
			{
				if(!m_ipcChannel->send(IPC_CMD_ADD_FOLDER, IPC_FLAG_ADD_RECURSIVE, QStringList() << dir.canonicalPath()))
				{
					qWarning("Failed to send IPC message!");
				}
			}
			bSentFiles = true;
		}
	}

	if(!bSentFiles)
	{
		if(!m_ipcChannel->send(IPC_CMD_PING, IPC_FLAG_NONE, QStringList() << QLatin1String("Use running instance!")))
		{
			qWarning("Failed to send IPC message!");
		}
	}
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/