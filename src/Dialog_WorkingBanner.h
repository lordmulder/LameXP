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

#include "../tmp/UIC_WorkingBanner.h"

////////////////////////////////////////////////////////////
// Splash Frame
////////////////////////////////////////////////////////////

class WorkingBanner: public QDialog, private Ui::WorkingBanner
{
	Q_OBJECT

public:
	WorkingBanner(QWidget *parent = 0);
	~WorkingBanner(void);
	
	void show(const QString &text);
	void show(const QString &text, QThread *thread);
	void close(void);

private:
	QMovie *m_working;
	bool m_canClose;

public slots:
	void setText(const QString &text);

protected:
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
	void closeEvent(QCloseEvent *event);
};
