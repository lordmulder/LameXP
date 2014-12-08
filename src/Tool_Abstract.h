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

#pragma once

#include <QObject>

class QMutex;
class QProcess;
class QElapsedTimer;

namespace MUtils
{
	class JobObject;
}

class AbstractTool : public QObject
{
	Q_OBJECT

public:
	AbstractTool(void);
	~AbstractTool(void);
	
	bool startProcess(QProcess &process, const QString &program, const QStringList &args);
	static QString commandline2string(const QString &program, const QStringList &arguments);

signals:
	void statusUpdated(int progress);
	void messageLogged(const QString &line);

protected:
	static const int m_processTimeoutInterval = 600000;

private:
	static QScopedPointer<MUtils::JobObject> s_jobObjectInstance;
	static QScopedPointer<QElapsedTimer>     s_startProcessTimer;

	static QMutex s_startProcessMutex;
	static QMutex s_createObjectMutex;

	static quint64 s_referenceCounter;

	bool m_firstLaunch;
};
