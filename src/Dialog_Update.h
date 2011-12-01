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

#pragma once

#include "..\tmp\UIC_UpdateDialog.h"

#include <QDialog>

class UpdateInfo;
class SettingsModel;
class QMovie;

class UpdateDialog : public QDialog, private Ui::UpdateDialog
{
	Q_OBJECT

public:
	UpdateDialog(SettingsModel *settings, QWidget *parent = 0);
	~UpdateDialog(void);

	bool getSuccess(void) { return m_success; }
	bool updateReadyToInstall(void) { return m_updateReadyToInstall; }

private slots:
	void updateInit(void);
	void checkForUpdates(void);
	void linkActivated(const QString &link);
	void applyUpdate(void);
	void logButtonClicked(void);
	void progressBarValueChanged(int value);

protected:
	void showEvent(QShowEvent *event);
	void closeEvent(QCloseEvent *event);
	void keyPressEvent(QKeyEvent *e);
	bool winEvent(MSG *message, long *result);

	const bool m_betaUpdates;

private:
	bool tryUpdateMirror(UpdateInfo *updateInfo, const QString &url);
	bool getFile(const QString &url, const QString &outFile, unsigned int maxRedir = 5);
	bool checkSignature(const QString &file, const QString &signature);
	bool parseVersionInfo(const QString &file, UpdateInfo *updateInfo);
	void testKnownWebSites(void);

	UpdateInfo *m_updateInfo;
	QStringList *m_logFile;
	SettingsModel *m_settings;
	QMovie *m_animator;
	
	const QString m_binaryWGet;
	const QString m_binaryGnuPG;
	const QString m_binaryUpdater;
	const QString m_binaryKeys;

	bool m_success;
	bool m_updateReadyToInstall;
};
