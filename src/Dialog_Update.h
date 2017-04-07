///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2017 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include <QDialog>

class SettingsModel;
class QMovie;

//MUtils forward declaration
namespace MUtils
{
	class UpdateChecker;
	class Taskbar7;
}

//UIC forward declartion
namespace Ui
{
	class UpdateDialog;
}

//UpdateDialog class
class UpdateDialog : public QDialog
{
	Q_OBJECT

public:
	UpdateDialog(const SettingsModel *const settings, QWidget *parent = 0);
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

	void threadStatusChanged(const int status);
	void threadProgressChanged(const int progress);
	void threadMessageLogged(const QString &message);
	void threadFinished(void);

protected:
	virtual void showEvent(QShowEvent *event);
	virtual void closeEvent(QCloseEvent *event);
	virtual void keyPressEvent(QKeyEvent *e);
	virtual bool event(QEvent *e);

	const bool m_betaUpdates;

private:
	Ui::UpdateDialog *const ui; //for Qt UIC
	const SettingsModel *const m_settings;

	const QString m_binaryWGet;
	const QString m_binaryMCat;
	const QString m_binaryGnuPG;
	const QString m_binaryKeys;
	const QString m_binaryUpdater;

	QScopedPointer<MUtils::Taskbar7> m_taskbar;
	QScopedPointer<MUtils::UpdateChecker> m_thread;
	QScopedPointer<QStringList> m_logFile;
	QScopedPointer<QMovie> m_animator;

	unsigned long m_updaterProcess;

	bool m_success;
	bool m_updateReadyToInstall;
	bool m_firstShow;
	
	void testKnownHosts(void);

};
