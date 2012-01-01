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

#include <QObject>

class QMutex;
class QProcess;

class AbstractTool : public QObject
{
	Q_OBJECT

public:
	AbstractTool(void);
	~AbstractTool(void);
	
	bool startProcess(QProcess &process, const QString &program, const QStringList &args);
	static QString commandline2string(const QString &program, const QStringList &arguments);
	static QString AbstractTool::pathToShort(const QString &longPath);

signals:
	void statusUpdated(int progress);
	void messageLogged(const QString &line);

protected:
	static const int m_processTimeoutInterval = 600000;

private:
	static QMutex *m_mutex_startProcess;
	static void *m_handle_jobObject;
	bool m_firstLaunch;
};
