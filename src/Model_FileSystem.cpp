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

#include "Model_FileSystem.h"
#include "Global.h"

#include <QApplication>
#include <QFileIconProvider>
#include <QDesktopServices>
#include <QLibrary>

#define IS_DIR(ATTR) (((ATTR) & FILE_ATTRIBUTE_DIRECTORY) && (!((ATTR) & FILE_ATTRIBUTE_HIDDEN)))
#define NO_DOT_OR_DOTDOT(STR) (wcscmp((STR), L".") && wcscmp((STR), L".."))

typedef HANDLE (WINAPI *FindFirstFileExFun)(LPCWSTR lpFileName, FINDEX_INFO_LEVELS fInfoLevelId, LPVOID lpFindFileData, FINDEX_SEARCH_OPS fSearchOp, LPVOID lpSearchFilter, DWORD dwAdditionalFlags);

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
	const QIcon m_homeIcon;
	const QIcon m_desktopIcon;
	const QIcon m_musicIcon;
	const QIcon m_moviesIcon;
	const QIcon m_picturesIcon;
	const QIcon m_emptyIcon;
	const QString m_folderType;
	const QString m_emptyType;
	const QString m_homeDir;
	const QString m_desktopDir;
	const QString m_musicDir;
	const QString m_moviesDir;
	const QString m_picturesDir;
};

QFileIconProviderEx::QFileIconProviderEx()
:
	m_folderIcon(":/icons/folder.png"),
	m_driveIcon(":/icons/drive.png"),
	m_cdromIcon(":/icons/drive_cd.png"),
	m_networkIcon(":/icons/drive_link.png"),
	m_floppyIcon(":/icons/drive_disk.png"),
	m_homeIcon(":/icons/house.png"),
	m_desktopIcon(":/icons/monitor.png"),
	m_musicIcon(":/icons/music.png"),
	m_moviesIcon(":/icons/film.png"),
	m_picturesIcon(":/icons/picture.png"),
	m_homeDir(QDir::fromNativeSeparators(QDesktopServices::storageLocation(QDesktopServices::HomeLocation))),
	m_desktopDir(QDir::fromNativeSeparators(QDesktopServices::storageLocation(QDesktopServices::DesktopLocation))),
	m_musicDir(QDir::fromNativeSeparators(QDesktopServices::storageLocation(QDesktopServices::MusicLocation))),
	m_moviesDir(QDir::fromNativeSeparators(QDesktopServices::storageLocation(QDesktopServices::MoviesLocation))),
	m_picturesDir(QDir::fromNativeSeparators(QDesktopServices::storageLocation(QDesktopServices::PicturesLocation))),
	m_folderType("Folder")
{
	/* Nothing to do! */
}

QIcon QFileIconProviderEx::icon(const QFileInfo &info) const
{
	if(info.isFile())
	{
		return m_emptyIcon;
	}
	else if(info.isRoot())
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
	else if(!info.filePath().compare(m_homeDir, Qt::CaseInsensitive))
	{
		return m_homeIcon;
	}
	else if(!info.filePath().compare(m_desktopDir, Qt::CaseInsensitive))
	{
		return m_desktopIcon;
	}
	else if(!info.filePath().compare(m_musicDir, Qt::CaseInsensitive))
	{
		return m_musicIcon;
	}
	else if(!info.filePath().compare(m_moviesDir, Qt::CaseInsensitive))
	{
		return m_moviesIcon;
	}
	else if(!info.filePath().compare(m_picturesDir, Qt::CaseInsensitive))
	{
		return m_picturesIcon;
	}
	else
	{
		return  m_folderIcon;
	}
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
		return /*(QFileSystemModel::rowCount(parent) > 0) ||*/ hasSubfoldersCached(filePath(parent));
	}
	
	return true;
}

int QFileSystemModelEx::rowCount(const QModelIndex &parent) const
{
	if(parent.isValid())
	{
		removeFromCache(filePath(parent));
	}

	return QFileSystemModel::rowCount(parent);
}

void QFileSystemModelEx::fetchMore(const QModelIndex &parent)
{
	if(parent.isValid())
	{
		removeFromCache(filePath(parent));
	}

	QFileSystemModel::fetchMore(parent);
}

QModelIndex QFileSystemModelEx::index(const QString &path, int column) const
{
	QFileInfo info(path);
	if(info.exists() && info.isDir())
	{
		QStringList parts = QDir::fromNativeSeparators(info.canonicalFilePath()).split('/', QString::SkipEmptyParts);
		for(int i = 2; i <= parts.count(); i++)
		{
			QFileInfo currentPath(((QStringList) parts.mid(0, i)).join("/"));
			if((!currentPath.exists()) || (!currentPath.isDir()) || currentPath.isHidden())
			{
				return QModelIndex();
			}
		}
		return QFileSystemModel::index(path, column);
	}
	return QModelIndex();
}

/* ------------------------ */
/*  STATIC FUNCTIONS BELOW  */
/* ------------------------ */

QHash<const QString, bool> QFileSystemModelEx::s_hasSubfolderCache;
QMutex QFileSystemModelEx::s_hasSubfolderMutex;

bool QFileSystemModelEx::hasSubfoldersCached(const QString &path)
{
	QMutexLocker lock(&s_hasSubfolderMutex);
	
	if(s_hasSubfolderCache.contains(path))
	{
		return s_hasSubfolderCache.value(path);
	}
	
	bool bChildren = hasSubfolders(path);
	s_hasSubfolderCache.insert(path, bChildren);
	return bChildren;
}

void QFileSystemModelEx::removeFromCache(const QString &path)
{
	QMutexLocker lock(&s_hasSubfolderMutex);
	s_hasSubfolderCache.remove(path);
}

bool QFileSystemModelEx::hasSubfolders(const QString &path)
{
	static bool FindFirstFileExInitialized = false;
	static FindFirstFileExFun FindFirstFileExPtr = NULL;

	if(!FindFirstFileExInitialized)
	{
		QLibrary Kernel32Lib("kernel32.dll");
		FindFirstFileExPtr = (FindFirstFileExFun) Kernel32Lib.resolve("FindFirstFileExW");
		FindFirstFileExInitialized = true;
	}
		
	WIN32_FIND_DATAW findData;
	bool bChildren = false;

	HANDLE h = (FindFirstFileExPtr)
		? FindFirstFileExPtr(QWCHAR(QDir::toNativeSeparators(path + "/*")), FindExInfoStandard, &findData, FindExSearchLimitToDirectories, NULL, 0)
		: FindFirstFileW(QWCHAR(QDir::toNativeSeparators(path + "/*")), &findData);
	
	if(h != INVALID_HANDLE_VALUE)
	{
		if(NO_DOT_OR_DOTDOT(findData.cFileName))
		{
			bChildren = IS_DIR(findData.dwFileAttributes);
		}
		while((!bChildren) && FindNextFile(h, &findData))
		{
			if(NO_DOT_OR_DOTDOT(findData.cFileName))
			{
				bChildren = IS_DIR(findData.dwFileAttributes);
			}
		}
		FindClose(h);
	}
	return bChildren;
}
