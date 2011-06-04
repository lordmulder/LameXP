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

#include "Model_FileSystem.h"
#include "Global.h"

#include <QApplication>
#include <QFileIconProvider>

///////////////////////////////////////////////////////////////////////////////
// Dummy QFileIconProvider class
///////////////////////////////////////////////////////////////////////////////

class QFileIconProviderEx : public QFileIconProvider
{
public:
	QFileIconProviderEx();
	virtual QIcon icon(IconType type) const { return (type == Drive) ? m_driveIcon : m_folderIcon; }
	virtual QIcon icon(const QFileInfo &info) const;
	virtual QString	type (const QFileInfo &info) const { return info.isDir() ? m_folderType : m_emptyType; }

private:
	const QIcon m_driveIcon;
	const QIcon m_cdromIcon;
	const QIcon m_networkIcon;
	const QIcon m_floppyIcon;
	const QIcon m_folderIcon;
	const QIcon m_emptyIcon;
	const QString m_folderType;
	const QString m_emptyType;
};

QFileIconProviderEx::QFileIconProviderEx()
:
	m_folderIcon(":/icons/folder.png"),
	m_driveIcon(":/icons/drive.png"),
	m_cdromIcon(":/icons/drive_cd.png"),
	m_networkIcon(":/icons/drive_link.png"),
	m_floppyIcon(":/icons/drive_disk.png"),
	m_folderType("Folder")
{
	/* Nothing to do! */
}

QIcon QFileIconProviderEx::icon(const QFileInfo &info) const
{
	if(info.isRoot())
	{
		switch(GetDriveType(QWCHAR(QDir::toNativeSeparators(info.absoluteFilePath()))))
		{
		case DRIVE_CDROM:
			return m_cdromIcon;
			break;
		case DRIVE_REMOVABLE:
			return m_floppyIcon;
			break;
		case DRIVE_REMOTE:
			return m_networkIcon;
			break;
		default:
			return m_driveIcon;
			break;
		}
	}
	
	return info.isFile() ? m_emptyIcon : m_folderIcon;
}

///////////////////////////////////////////////////////////////////////////////
// Modified QFileSystemModel class
///////////////////////////////////////////////////////////////////////////////

QFileSystemModelEx::QFileSystemModelEx()
:
	QFileSystemModel()
{
	this->m_myIconProvider = new QFileIconProviderEx();
	this->setIconProvider(m_myIconProvider);
	this->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
	this->setNameFilterDisables(false);
}

QFileSystemModelEx::~QFileSystemModelEx()
{
	LAMEXP_DELETE(m_myIconProvider);
}

bool QFileSystemModelEx::hasChildren(const QModelIndex &parent) const
{
	if(parent.isValid())
	{
		QDir dir = QDir(QFileSystemModel::filePath(parent));
		return dir.exists() && (dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot).count() > 0);
	}
	
	return true;
}
