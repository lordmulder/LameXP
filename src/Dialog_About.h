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

#include <QDialog>

//UIC forward declartion
namespace Ui {
	class AboutDialog;
}

//Class declarations
class QLabel;
class QPixmap;
class QTimer;
class SettingsModel;

//AboutDialog class
class AboutDialog : public QDialog
{
	Q_OBJECT

public:
	AboutDialog(SettingsModel *settings, QWidget *parent = 0, bool firstStart = false);
	~AboutDialog(void);

	static const char *neroAacUrl, *disqueUrl;

public slots:
	int exec();
	void enableButtons(void);
	void openURL(const QString &url);
	void gotoLicenseTab(void);
	void showAboutQt(void);
	void moveDisque(void);
	void tabChanged(int index);
	void adjustSize(void);

protected:
	virtual void showEvent(QShowEvent *e);
	virtual void closeEvent(QCloseEvent *e);
	bool eventFilter(QObject *obj, QEvent *event);

private:
	Ui::AboutDialog *ui; //for Qt UIC

	bool m_firstShow;
	SettingsModel *m_settings;
	QMap<QWidget*,bool> *m_initFlags;
	int m_lastTab;
	
	QLabel *m_disque;
	QTimer * m_disqueTimer;
	bool m_disqueFlags[2];
	QPixmap *m_cartoon[4];
	bool m_rotateNext;
	__int64 m_disqueDelay;

	void initInformationTab(void);
	void initContributorsTab(void);
	void initSoftwareTab(void);
	void initLicenseTab(void);

	QString makeToolText(const QString &toolName, const QString &toolBin, const QString &toolVerFmt, const QString &toolLicense, const QString &toolWebsite, const QString &extraInfo = QString());
	bool playResoureSound(const QString &library, const unsigned long soundId, const bool async);
};
