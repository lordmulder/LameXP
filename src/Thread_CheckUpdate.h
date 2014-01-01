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

#include <QThread>
#include <QDate>

///////////////////////////////////////////////////////////////////////////////

class UpdateInfo
{
public:
	UpdateInfo(void);
	void resetInfo(void);

	unsigned int m_buildNo;
	QDate m_buildDate;
	QString m_downloadSite;
	QString m_downloadAddress;
	QString m_downloadFilename;
	QString m_downloadFilecode;
};

///////////////////////////////////////////////////////////////////////////////

class UpdateCheckThread : public QThread
{
	Q_OBJECT

public:
	enum
	{
		UpdateStatus_NotStartedYet             = 0,
		UpdateStatus_CheckingConnection        = 1,
		UpdateStatus_FetchingUpdates           = 2,
		UpdateStatus_CompletedUpdateAvailable  = 3,
		UpdateStatus_CompletedNoUpdates        = 4,
		UpdateStatus_CompletedNewVersionOlder  = 5,
		UpdateStatus_ErrorNoConnection         = 6,
		UpdateStatus_ErrorConnectionTestFailed = 7,
		UpdateStatus_ErrorFetchUpdateInfo      = 8
	}
	update_status_t;

	UpdateCheckThread(const QString &binWGet, const QString &binGnuPG, const QString &binKeys, const bool betaUpdates, const bool testMode = false);
	~UpdateCheckThread(void);

	const int getUpdateStatus(void) const { return m_status; }
	const bool getSuccess(void) const { return m_success; };
	const int getMaximumProgress(void) const { return m_maxProgress; };
	const int getCurrentProgress(void) const { return m_progress; };
	const UpdateInfo *getUpdateInfo(void) const { return m_updateInfo; }

protected:
	void run(void);
	void checkForUpdates(void);
	void testKnownHosts(void);

signals:
	void statusChanged(const int status);
	void progressChanged(const int progress);
	void messageLogged(const QString &text);

private:
	const int m_maxProgress;
	UpdateInfo *const m_updateInfo;
	
	const bool m_betaUpdates;
	const bool m_testMode;

	const QString m_binaryWGet;
	const QString m_binaryGnuPG;
	const QString m_binaryKeys;

	volatile bool m_success;

	int m_status;
	int m_progress;

	inline void setStatus(const int status);
	inline void setProgress(const int progress);
	inline void log(const QString &str1, const QString &str2 = QString(), const QString &str3 = QString(), const QString &str4 = QString());

	bool getFile(const QString &url, const QString &outFile, unsigned int maxRedir = 5, bool *httpOk = NULL);
	bool tryUpdateMirror(UpdateInfo *updateInfo, const QString &url);
	bool checkSignature(const QString &file, const QString &signature);
	bool parseVersionInfo(const QString &file, UpdateInfo *updateInfo);
};
