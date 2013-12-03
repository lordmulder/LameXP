///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2013 LoRd_MuldeR <MuldeR2@GMX.de>
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

namespace Ui
{
	class WorkingBanner;
}

class QEventLoop;

////////////////////////////////////////////////////////////
// Splash Frame
////////////////////////////////////////////////////////////

class WorkingBanner: public QDialog
{
	Q_OBJECT

public:
	WorkingBanner(QWidget *parent);
	~WorkingBanner(void);
	
	void show(const QString &text);
	void show(const QString &text, QThread *thread);
	void show(const QString &text, QEventLoop *loop);

private:
	Ui::WorkingBanner *const ui;

	QMovie *m_working;
	bool m_canClose;

public slots:
	void windowShown(void);
	void setText(const QString &text);
	void setProgressMax(unsigned int max);
	void setProgressVal(unsigned int val);
	bool close(void);

signals:
	void userAbort(void);

protected:
	virtual void keyPressEvent(QKeyEvent *event);
	virtual void keyReleaseEvent(QKeyEvent *event);
	virtual void closeEvent(QCloseEvent *event);
	virtual bool winEvent(MSG *message, long *result);
	virtual void showEvent(QShowEvent *event);
	virtual void hideEvent(QHideEvent *event);

	QFontMetrics *m_metrics;
	QStyle *m_style;
};
