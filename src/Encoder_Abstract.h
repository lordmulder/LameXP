///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2010 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Model_AudioFile.h"

#include <QObject>

class QProcess;
class QStringList;
class QMutex;

class AbstractEncoder : public QObject
{
	Q_OBJECT

public:
	AbstractEncoder(void);
	~AbstractEncoder(void);

	//Internal encoder API
	virtual bool encode(const AudioFileModel &sourceFile, const QString &outputFile, volatile bool *abortFlag) = 0;
	virtual bool isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion) = 0;
	virtual QString extension(void) = 0;

	//Common setter methods
	void setBitrate(int bitrate);
	void setRCMode(int mode);

	//Auxiliary functions
	bool startProcess(QProcess &process, const QString &program, const QStringList &args);
	static QString commandline2string(const QString &program, const QStringList &arguments);

signals:
	void statusUpdated(int progress);
	void messageLogged(const QString &line);

protected:
	int m_configBitrate;
	int m_configRCMode;

private:
	static QMutex *m_mutex_startProcess;
	static void *m_handle_jobObject;
};
