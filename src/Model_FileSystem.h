///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2015 LoRd_MuldeR <MuldeR2@GMX.de>
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
	virtual void fetchMore(const QModelIndex &parent);
	virtual QModelIndex index(const QString &path, int column = 0) const;
	virtual void flushCache(void);

private:
	QFileIconProviderEx *m_myIconProvider;
	
	static QHash<const QString, bool> s_hasSubfolderCache;
	static QMutex s_hasSubfolderMutex;
	static int s_findFirstFileExInfoLevel;

	static bool hasSubfolders(const QString &path);
	static bool hasSubfoldersCached(const QString &path);
	static void removeFromCache(const QString &path);
	static void removeAllFromCache(void);
};
