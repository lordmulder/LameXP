///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2011 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Thread_MessageProducer.h"

#include "Global.h"

#include <QStringList>
#include <QApplication>
#include <QFileInfo>
#include <QDir>

#include <limits.h>

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

MessageProducerThread::MessageProducerThread(void)
{
}

MessageProducerThread::~MessageProducerThread(void)
{
}

void MessageProducerThread::run()
{
	setTerminationEnabled(true);
	bool bSentFiles = false;
	QStringList arguments = QApplication::arguments();

	for(int i = 0; i < arguments.count(); i++)
	{
		if(!arguments[i].compare("--kill", Qt::CaseInsensitive))
		{
			lamexp_ipc_send(666, NULL);
			return;
		}
		if(!arguments[i].compare("--force-kill", Qt::CaseInsensitive))
		{
			lamexp_ipc_send(666, "Force!");
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
				lamexp_ipc_send(1, file.canonicalFilePath().toUtf8().constData());
			}
			bSentFiles = true;
		}
		if(!arguments[i].compare("--add-folder", Qt::CaseInsensitive))
		{
			QDir dir = QDir(arguments[++i]);
			if(dir.exists())
			{
				lamexp_ipc_send(2, dir.canonicalPath().toUtf8().constData());
			}
			bSentFiles = true;
		}
		if(!arguments[i].compare("--add-recursive", Qt::CaseInsensitive))
		{
			QDir dir = QDir(arguments[++i]);
			if(dir.exists())
			{
				lamexp_ipc_send(3, dir.canonicalPath().toUtf8().constData());
			}
			bSentFiles = true;
		}
	}

	if(!bSentFiles)
	{
		lamexp_ipc_send(UINT_MAX, "Use running instance!");
	}
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/