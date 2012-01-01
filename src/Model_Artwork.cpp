///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2012 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Model_Artwork.h"

#include "Global.h"

#include <QFile>
#include <QMutexLocker>

////////////////////////////////////////////////////////////

QMutex ArtworkModel::m_mutex;
QMap<QString, unsigned int> ArtworkModel::m_refCount;
QMap<QString, QFile*> ArtworkModel::m_fileHandle;

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

ArtworkModel::ArtworkModel(void)
{
	m_isOwner = false;
}

ArtworkModel::ArtworkModel(const QString &fileName, bool isOwner)
{
	m_isOwner = false;
	setFilePath(fileName, isOwner);
}

ArtworkModel::ArtworkModel(const ArtworkModel &model)
{
	m_isOwner = false;
	setFilePath(model.m_filePath, model.m_isOwner);
}

ArtworkModel &ArtworkModel::operator=(const ArtworkModel &model)
{
	setFilePath(model.m_filePath, model.m_isOwner);
	return (*this);
}

ArtworkModel::~ArtworkModel(void)
{
	clear();
}


////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////

const QString &ArtworkModel::filePath(void) const
{
	return m_filePath;
}

bool ArtworkModel::isOwner(void) const
{
	return m_isOwner;
}

void ArtworkModel::setFilePath(const QString &newPath, bool isOwner)
{
	if(newPath.isEmpty() || m_filePath.isEmpty() || QString::compare(m_filePath, newPath,Qt::CaseInsensitive))
	{
		clear();
	
		if(!newPath.isEmpty())
		{
			QMutexLocker lock(&m_mutex);

			if(!m_refCount.contains(newPath))
			{
				m_refCount.insert(newPath, 0);
				m_fileHandle.insert(newPath, new QFile(newPath));
				m_fileHandle[newPath]->open(QIODevice::ReadOnly);
			}

			m_refCount[newPath]++;
		}

		m_filePath = newPath;
		m_isOwner = isOwner;
	}
}

void ArtworkModel:: clear(void)
{
	if(!m_filePath.isEmpty())
	{
		QMutexLocker lock(&m_mutex);

		if(m_refCount.contains(m_filePath))
		{
			if(--m_refCount[m_filePath] < 1)
			{
				m_refCount.remove(m_filePath);

				if(m_fileHandle.contains(m_filePath))
				{
					if(QFile *fileHandle = m_fileHandle.take(m_filePath))
					{
						if(m_isOwner)
						{
							fileHandle->remove();
						}
						else
						{
							fileHandle->close();
						}
						LAMEXP_DELETE(fileHandle);
					}
				}

				if(m_isOwner)
				{
					QFile::remove(m_filePath);
				}
			}
		}

		m_filePath.clear();
	}
}
