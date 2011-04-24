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

#include <QMessageBox>

class SettingsModel;

class AboutDialog : public QMessageBox
{
	Q_OBJECT

public:
	AboutDialog(SettingsModel *settings, QWidget *parent = 0, bool firstStart = false);
	~AboutDialog(void);

	static const char *neroAacUrl, *disqueUrl;

public slots:
	int exec();
	void enableButtons(void);
	void openLicenseText(void);
	void showMoreAbout(void);
	void showAboutQt(void);
	void showAboutContributors(void);
	void moveDisque(void);

protected:
	void showEvent(QShowEvent *e);
	bool eventFilter(QObject *obj, QEvent *event);

private:
	bool m_firstShow;
	SettingsModel *m_settings;
	QLabel *m_disque;
	QTimer * m_disqueTimer;
	bool m_disqueFlags[2];
	QRect m_screenGeometry;

	QString makeToolText(const QString &toolName, const QString &toolBin, const QString &toolVerFmt, const QString &toolLicense, const QString &toolWebsite, const QString &extraInfo = QString());
	bool playResoureSound(const QString &library, const unsigned long soundId, const bool async);
};
