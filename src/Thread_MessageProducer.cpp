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

#include "Thread_MessageProducer.h"

//Internal
#include "Global.h"

//MUtils
#include <MUtils/Global.h>
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
	const QStringList &arguments = MUtils::OS::arguments();

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
				lamexp_ipc_send(1, MUTILS_UTF8(file.canonicalFilePath()));
			}
			bSentFiles = true;
		}
		if(!arguments[i].compare("--add-folder", Qt::CaseInsensitive))
		{
			QDir dir = QDir(arguments[++i]);
			if(dir.exists())
			{
				lamexp_ipc_send(2, MUTILS_UTF8(dir.canonicalPath()));
			}
			bSentFiles = true;
		}
		if(!arguments[i].compare("--add-recursive", Qt::CaseInsensitive))
		{
			QDir dir = QDir(arguments[++i]);
			if(dir.exists())
			{
				lamexp_ipc_send(3, MUTILS_UTF8(dir.canonicalPath()));
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