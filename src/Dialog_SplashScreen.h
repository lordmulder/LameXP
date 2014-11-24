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

#include "UIC_SplashScreen.h"

////////////////////////////////////////////////////////////
// Splash Frame
////////////////////////////////////////////////////////////

class SplashScreen: public QFrame, private Ui::SplashScreen
{
	Q_OBJECT

public:
	static void showSplash(QThread *thread);

private:
	SplashScreen(QWidget *parent = 0);
	~SplashScreen(void);

	enum
	{
		STATUS_FADE_IN = 0,
		STATUS_WAIT = 1,
		STATUS_FADE_OUT = 2,
		STATUS_DONE = 3
	}
	status_t;

	QMovie *m_working;
	QEventLoop *m_loop;
	QTimer *m_timer;
	
	const unsigned int m_opacitySteps;

	unsigned int m_status;
	unsigned int m_fadeValue;

	volatile bool m_canClose;
	volatile bool m_taskBarInit;

private slots:
	void updateHandler(void);
	void threadComplete(void);

protected:
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
	void closeEvent(QCloseEvent *event);
	bool winEvent(MSG *message, long *result);
};
