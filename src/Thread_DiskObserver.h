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

#include <QThread>
#include <QSemaphore>

class DiskObserverThread: public QThread
{
	Q_OBJECT

public:
	DiskObserverThread(const QString &path);
	~DiskObserverThread(void);

	void stop(void) { m_semaphore.release(); }
	
protected:
	void run(void);
	void observe(void);

	static QString makeRootDir(const QString &baseDir);

signals:
	void messageLogged(const QString &text, int type);
	void freeSpaceChanged(const quint64);

private:
	QSemaphore m_semaphore;
	const QString m_path;
};
