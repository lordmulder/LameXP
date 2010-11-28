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

#include "..\tmp\UIC_UpdateDialog.h"

#include <QDialog>

class UpdateInfo;

class UpdateDialog : public QDialog, private Ui::UpdateDialog
{
	Q_OBJECT

public:
	UpdateDialog(QWidget *parent = 0);
	~UpdateDialog(void);

private slots:
	void updateInit(void);
	void checkForUpdates(void);
	void linkActivated(const QString &link);
	void applyUpdate(void);

protected:
	void showEvent(QShowEvent *event);
	void closeEvent(QCloseEvent *event);

private:
	bool tryUpdateMirror(UpdateInfo *updateInfo, const QString &url);
	bool getFile(const QString &url, const QString &outFile);
	bool checkSignature(const QString &file, const QString &signature);
	bool parseVersionInfo(const QString &file, UpdateInfo *updateInfo);

	UpdateInfo *m_updateInfo;
	
	const QString m_binaryWGet;
	const QString m_binaryGnuPG;
	const QString m_binaryUpdater;
	const QString m_binaryKeys;
};
