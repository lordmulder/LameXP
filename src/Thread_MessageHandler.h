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

class MessageHandlerThread: public QThread
{
	Q_OBJECT

public:
	MessageHandlerThread(void);
	~MessageHandlerThread(void);
	void run();
	void stop(void);

private:
	char *m_parameter;
	volatile bool m_aborted;

signals:
	void otherInstanceDetected(void);
	void fileReceived(const QString &filePath);
	void folderReceived(const QString &filePath, bool recursive);
	void killSignalReceived(void);
};
