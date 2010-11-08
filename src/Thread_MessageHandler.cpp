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

#include "Thread_MessageHandler.h"

#include "Global.h"

#include <QSharedMemory>
#include <QSystemSemaphore>

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

MessageHandlerThread::MessageHandlerThread(void)
{
}

void MessageHandlerThread::run()
{
	unsigned int command = 0;
	char *parameter = new char[4096];
	
	while(true)
	{
		qDebug("MessageHandlerThread: Waiting...");
		lamexp_ipc_read(&command, parameter, 4096);
		qDebug("MessageHandlerThread: command=%u, parameter='%s'", command, parameter);
	}

	delete [] parameter;
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/