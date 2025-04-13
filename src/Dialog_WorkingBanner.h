///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2025 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU GENERAL PUBLIC LICENSE as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version; always including the non-optional
// LAMEXP GNU GENERAL PUBLIC LICENSE ADDENDUM. See "License.txt" file!
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

namespace MUtils
{
	class Taskbar7;
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

	QScopedPointer<MUtils::Taskbar7> m_taskbar;
	QScopedPointer<QMovie> m_working;

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
	virtual void showEvent(QShowEvent *event);
	virtual void hideEvent(QHideEvent *event);

	QScopedPointer<QFontMetrics> m_metrics;
	QScopedPointer<QStyle> m_style;
	QScopedPointer<QIcon> m_iconHourglass;
};
