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

#include "Global.h"

//MUtils
#include <MUtils/Global.h>
#include <MUtils/Exception.h>

//Qt includes
#include <QSharedMemory>
#include <QSystemSemaphore>
#include <QWriteLocker>

///////////////////////////////////////////////////////////////////////////////
// TYPES
///////////////////////////////////////////////////////////////////////////////

static const size_t g_lamexp_ipc_slots = 128;

typedef struct
{
	unsigned int command;
	unsigned int reserved_1;
	unsigned int reserved_2;
	char parameter[4096];
}
lamexp_ipc_data_t;

typedef struct
{
	unsigned int pos_write;
	unsigned int pos_read;
	lamexp_ipc_data_t data[g_lamexp_ipc_slots];
}
lamexp_ipc_t;

///////////////////////////////////////////////////////////////////////////////
// GLOBAL VARS
///////////////////////////////////////////////////////////////////////////////

//Shared memory
static const struct
{
	char *sharedmem;
	char *semaphore_read;
	char *semaphore_read_mutex;
	char *semaphore_write;
	char *semaphore_write_mutex;
}
g_lamexp_ipc_uuid =
{
	"{21A68A42-6923-43bb-9CF6-64BF151942EE}",
	"{7A605549-F58C-4d78-B4E5-06EFC34F405B}",
	"{60AA8D04-F6B8-497d-81EB-0F600F4A65B5}",
	"{726061D5-1615-4B82-871C-75FD93458E46}",
	"{1A616023-AA6A-4519-8AF3-F7736E899977}"
};
static struct
{
	QSharedMemory *sharedmem;
	QSystemSemaphore *semaphore_read;
	QSystemSemaphore *semaphore_read_mutex;
	QSystemSemaphore *semaphore_write;
	QSystemSemaphore *semaphore_write_mutex;
	QReadWriteLock lock;
}
g_lamexp_ipc_ptr;

///////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

/*
 * Initialize IPC
 */
int lamexp_init_ipc(void)
{
	QWriteLocker writeLock(&g_lamexp_ipc_ptr.lock);
	
	if(g_lamexp_ipc_ptr.sharedmem && g_lamexp_ipc_ptr.semaphore_read && g_lamexp_ipc_ptr.semaphore_write && g_lamexp_ipc_ptr.semaphore_read_mutex && g_lamexp_ipc_ptr.semaphore_write_mutex)
	{
		return 0;
	}

	g_lamexp_ipc_ptr.semaphore_read = new QSystemSemaphore(QString(g_lamexp_ipc_uuid.semaphore_read), 0);
	g_lamexp_ipc_ptr.semaphore_write = new QSystemSemaphore(QString(g_lamexp_ipc_uuid.semaphore_write), 0);
	g_lamexp_ipc_ptr.semaphore_read_mutex = new QSystemSemaphore(QString(g_lamexp_ipc_uuid.semaphore_read_mutex), 0);
	g_lamexp_ipc_ptr.semaphore_write_mutex = new QSystemSemaphore(QString(g_lamexp_ipc_uuid.semaphore_write_mutex), 0);

	if(g_lamexp_ipc_ptr.semaphore_read->error() != QSystemSemaphore::NoError)
	{
		QString errorMessage = g_lamexp_ipc_ptr.semaphore_read->errorString();
		MUTILS_DELETE(g_lamexp_ipc_ptr.semaphore_read);
		MUTILS_DELETE(g_lamexp_ipc_ptr.semaphore_write);
		MUTILS_DELETE(g_lamexp_ipc_ptr.semaphore_read_mutex);
		MUTILS_DELETE(g_lamexp_ipc_ptr.semaphore_write_mutex);
		qFatal("Failed to create system smaphore: %s", MUTILS_UTF8(errorMessage));
		return -1;
	}
	if(g_lamexp_ipc_ptr.semaphore_write->error() != QSystemSemaphore::NoError)
	{
		QString errorMessage = g_lamexp_ipc_ptr.semaphore_write->errorString();
		MUTILS_DELETE(g_lamexp_ipc_ptr.semaphore_read);
		MUTILS_DELETE(g_lamexp_ipc_ptr.semaphore_write);
		MUTILS_DELETE(g_lamexp_ipc_ptr.semaphore_read_mutex);
		MUTILS_DELETE(g_lamexp_ipc_ptr.semaphore_write_mutex);
		qFatal("Failed to create system smaphore: %s", MUTILS_UTF8(errorMessage));
		return -1;
	}
	if(g_lamexp_ipc_ptr.semaphore_read_mutex->error() != QSystemSemaphore::NoError)
	{
		QString errorMessage = g_lamexp_ipc_ptr.semaphore_read_mutex->errorString();
		MUTILS_DELETE(g_lamexp_ipc_ptr.semaphore_read);
		MUTILS_DELETE(g_lamexp_ipc_ptr.semaphore_write);
		MUTILS_DELETE(g_lamexp_ipc_ptr.semaphore_read_mutex);
		MUTILS_DELETE(g_lamexp_ipc_ptr.semaphore_write_mutex);
		qFatal("Failed to create system smaphore: %s", MUTILS_UTF8(errorMessage));
		return -1;
	}
	if(g_lamexp_ipc_ptr.semaphore_write_mutex->error() != QSystemSemaphore::NoError)
	{
		QString errorMessage = g_lamexp_ipc_ptr.semaphore_write_mutex->errorString();
		MUTILS_DELETE(g_lamexp_ipc_ptr.semaphore_read);
		MUTILS_DELETE(g_lamexp_ipc_ptr.semaphore_write);
		MUTILS_DELETE(g_lamexp_ipc_ptr.semaphore_read_mutex);
		MUTILS_DELETE(g_lamexp_ipc_ptr.semaphore_write_mutex);
		qFatal("Failed to create system smaphore: %s", MUTILS_UTF8(errorMessage));
		return -1;
	}

	g_lamexp_ipc_ptr.sharedmem = new QSharedMemory(QString(g_lamexp_ipc_uuid.sharedmem), NULL);
	
	if(!g_lamexp_ipc_ptr.sharedmem->create(sizeof(lamexp_ipc_t)))
	{
		if(g_lamexp_ipc_ptr.sharedmem->error() == QSharedMemory::AlreadyExists)
		{
			g_lamexp_ipc_ptr.sharedmem->attach();
			if(g_lamexp_ipc_ptr.sharedmem->error() == QSharedMemory::NoError)
			{
				return 1;
			}
			else
			{
				QString errorMessage = g_lamexp_ipc_ptr.sharedmem->errorString();
				MUTILS_DELETE(g_lamexp_ipc_ptr.sharedmem);
				qFatal("Failed to attach to shared memory: %s", MUTILS_UTF8(errorMessage));
				return -1;
			}
		}
		else
		{
			QString errorMessage = g_lamexp_ipc_ptr.sharedmem->errorString();
			MUTILS_DELETE(g_lamexp_ipc_ptr.sharedmem);
			qFatal("Failed to create shared memory: %s", MUTILS_UTF8(errorMessage));
			return -1;
		}
	}

	memset(g_lamexp_ipc_ptr.sharedmem->data(), 0, sizeof(lamexp_ipc_t));
	g_lamexp_ipc_ptr.semaphore_write->release(g_lamexp_ipc_slots);
	g_lamexp_ipc_ptr.semaphore_read_mutex->release();
	g_lamexp_ipc_ptr.semaphore_write_mutex->release();

	return 0;
}

/*
 * IPC send message
 */
void lamexp_ipc_send(unsigned int command, const char* message)
{
	QReadLocker readLock(&g_lamexp_ipc_ptr.lock);

	if(!g_lamexp_ipc_ptr.sharedmem || !g_lamexp_ipc_ptr.semaphore_read || !g_lamexp_ipc_ptr.semaphore_write || !g_lamexp_ipc_ptr.semaphore_read_mutex || !g_lamexp_ipc_ptr.semaphore_write_mutex)
	{
		MUTILS_THROW("Shared memory for IPC not initialized yet.");
	}

	lamexp_ipc_data_t ipc_data;
	memset(&ipc_data, 0, sizeof(lamexp_ipc_data_t));
	ipc_data.command = command;
	
	if(message)
	{
		strncpy_s(ipc_data.parameter, 4096, message, _TRUNCATE);
	}

	if(g_lamexp_ipc_ptr.semaphore_write->acquire())
	{
		if(g_lamexp_ipc_ptr.semaphore_write_mutex->acquire())
		{
			lamexp_ipc_t *ptr = reinterpret_cast<lamexp_ipc_t*>(g_lamexp_ipc_ptr.sharedmem->data());
			memcpy(&ptr->data[ptr->pos_write], &ipc_data, sizeof(lamexp_ipc_data_t));
			ptr->pos_write = (ptr->pos_write + 1) % g_lamexp_ipc_slots;
			g_lamexp_ipc_ptr.semaphore_read->release();
			g_lamexp_ipc_ptr.semaphore_write_mutex->release();
		}
	}
}

/*
 * IPC read message
 */
void lamexp_ipc_read(unsigned int *command, char* message, size_t buffSize)
{
	QReadLocker readLock(&g_lamexp_ipc_ptr.lock);
	
	*command = 0;
	message[0] = '\0';
	
	if(!g_lamexp_ipc_ptr.sharedmem || !g_lamexp_ipc_ptr.semaphore_read || !g_lamexp_ipc_ptr.semaphore_write || !g_lamexp_ipc_ptr.semaphore_read_mutex || !g_lamexp_ipc_ptr.semaphore_write_mutex)
	{
		MUTILS_THROW("Shared memory for IPC not initialized yet.");
	}

	lamexp_ipc_data_t ipc_data;
	memset(&ipc_data, 0, sizeof(lamexp_ipc_data_t));

	if(g_lamexp_ipc_ptr.semaphore_read->acquire())
	{
		if(g_lamexp_ipc_ptr.semaphore_read_mutex->acquire())
		{
			lamexp_ipc_t *ptr = reinterpret_cast<lamexp_ipc_t*>(g_lamexp_ipc_ptr.sharedmem->data());
			memcpy(&ipc_data, &ptr->data[ptr->pos_read], sizeof(lamexp_ipc_data_t));
			ptr->pos_read = (ptr->pos_read + 1) % g_lamexp_ipc_slots;
			g_lamexp_ipc_ptr.semaphore_write->release();
			g_lamexp_ipc_ptr.semaphore_read_mutex->release();

			if(!(ipc_data.reserved_1 || ipc_data.reserved_2))
			{
				*command = ipc_data.command;
				strncpy_s(message, buffSize, ipc_data.parameter, _TRUNCATE);
			}
			else
			{
				qWarning("Malformed IPC message, will be ignored");
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// INITIALIZATION
///////////////////////////////////////////////////////////////////////////////

extern "C" void _lamexp_global_init_ipcom(void)
{
	MUTILS_ZERO_MEMORY(g_lamexp_ipc_ptr);
}

///////////////////////////////////////////////////////////////////////////////
// FINALIZATION
///////////////////////////////////////////////////////////////////////////////

extern "C" void _lamexp_global_free_ipcom(void)
{
	if(g_lamexp_ipc_ptr.sharedmem)
	{
		g_lamexp_ipc_ptr.sharedmem->detach();
	}

	MUTILS_DELETE(g_lamexp_ipc_ptr.sharedmem);
	MUTILS_DELETE(g_lamexp_ipc_ptr.semaphore_read);
	MUTILS_DELETE(g_lamexp_ipc_ptr.semaphore_write);
	MUTILS_DELETE(g_lamexp_ipc_ptr.semaphore_read_mutex);
	MUTILS_DELETE(g_lamexp_ipc_ptr.semaphore_write_mutex);
}
