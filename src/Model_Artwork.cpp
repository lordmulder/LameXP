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

#include "Model_Artwork.h"

//Internal
#include "Global.h"

//MUtils
#include <MUtils/Global.h>

//Qt
#include <QFile>
#include <QMutex>
#include <QMutexLocker>

////////////////////////////////////////////////////////////
// Shared data class
////////////////////////////////////////////////////////////

class ArtworkModel_SharedData
{
	friend ArtworkModel;

protected:
	ArtworkModel_SharedData(const QString &filePath, const bool isOwner)
	:
		m_isOwner(isOwner),
		m_filePath(filePath),
		m_fileHandle(NULL)
	{
		m_referenceCounter = 1;

		if(!m_filePath.isEmpty())
		{
			QFile *file = new QFile(m_filePath);
			if(file->open(QIODevice::ReadOnly))
			{
				m_fileHandle = file;
			}
			else
			{
				qWarning("[ArtworkModel] Failed to open artwork file!");
				MUTILS_DELETE(file);
			}
		}
	}

	~ArtworkModel_SharedData(void)
	{
		if(m_fileHandle)
		{
			if(m_isOwner)
			{
				m_fileHandle->remove();
			}
			m_fileHandle->close();
			MUTILS_DELETE(m_fileHandle);
		}
	}

	static ArtworkModel_SharedData *attach(ArtworkModel_SharedData *ptr)
	{
		if(ptr)
		{
			QMutexLocker lock(&s_mutex);
			ptr->m_referenceCounter = ptr->m_referenceCounter + 1;
			return ptr;
		}
		return NULL;
	}

	static void detach(ArtworkModel_SharedData **ptr)
	{
		if(*ptr)
		{
			QMutexLocker lock(&s_mutex);
			if((*ptr)->m_referenceCounter > 0)
			{
				(*ptr)->m_referenceCounter = (*ptr)->m_referenceCounter - 1;
				if((*ptr)->m_referenceCounter < 1)
				{
					delete (*ptr);
				}
			}
			else
			{
				qWarning("[ArtworkModel::detach] Ref counter already zero!");
			}
			*ptr = NULL;
		}
	}

	const QString m_filePath;
	const bool m_isOwner;
	QFile *m_fileHandle;
	unsigned int m_referenceCounter;

	static QMutex s_mutex;
};

QMutex ArtworkModel_SharedData::s_mutex;

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

ArtworkModel::ArtworkModel(void)
:
	m_mutex(new QMutex)
{
	m_data = NULL;
}

ArtworkModel::ArtworkModel(const QString &fileName, bool isOwner)
:
	m_mutex(new QMutex)
{
	m_data = new ArtworkModel_SharedData(fileName, isOwner);
}

ArtworkModel::ArtworkModel(const ArtworkModel &model)
:
	m_mutex(new QMutex)
{
	m_data = ArtworkModel_SharedData::attach(model.m_data);
}

ArtworkModel &ArtworkModel::operator=(const ArtworkModel &model)
{
	QMutexLocker lock(m_mutex);
	if(m_data != model.m_data)
	{
		ArtworkModel_SharedData::detach(&m_data);
		m_data = ArtworkModel_SharedData::attach(model.m_data);
	}
	return (*this);
}

ArtworkModel::~ArtworkModel(void)
{
	QMutexLocker lock(m_mutex);
	ArtworkModel_SharedData::detach(&m_data);
	lock.unlock();
	MUTILS_DELETE(m_mutex);
}

////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////

const QString &ArtworkModel::filePath(void) const
{
	QMutexLocker lock(m_mutex);
	return (m_data) ? m_data->m_filePath : m_nullString;
}

bool ArtworkModel::isOwner(void) const
{
	QMutexLocker lock(m_mutex);
	return (m_data) ? m_data->m_isOwner : false;
}

void ArtworkModel::setFilePath(const QString &newPath, bool isOwner)
{
	QMutexLocker lock(m_mutex);
	ArtworkModel_SharedData::detach(&m_data);
	if(!newPath.isEmpty())
	{
		m_data = new ArtworkModel_SharedData(newPath, isOwner);
	}
}

void ArtworkModel::clear(void)
{
	QMutexLocker lock(m_mutex);
	ArtworkModel_SharedData::detach(&m_data);
}
