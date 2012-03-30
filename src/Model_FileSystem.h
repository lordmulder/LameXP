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

#pragma once

#include <QFileSystemModel>
#include <QMutex>

class QFileIconProviderEx;

class QFileSystemModelEx : public QFileSystemModel
{
public:
	QFileSystemModelEx();
	~QFileSystemModelEx();

	virtual bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual void fetchMore(const QModelIndex &parent);

private:
	QFileIconProviderEx *m_myIconProvider;
	
	static QHash<const QString, bool> s_hasFolderCache;
	static QMutex s_hasFolderMutex;

	static bool hasSubfolders(const QString &path);
	static bool hasSubfoldersCached(const QString &path);
	static void removeFromCache(const QString &path);
};
