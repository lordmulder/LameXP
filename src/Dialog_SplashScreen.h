///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2018 LoRd_MuldeR <MuldeR2@GMX.de>
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

//MUtils forward declaration
namespace MUtils
{
	class Taskbar7;
}

//UIC forward declartion
namespace Ui
{
	class SplashScreen;
}

////////////////////////////////////////////////////////////
// Splash Frame
////////////////////////////////////////////////////////////

class SplashScreen: public QFrame
{
	Q_OBJECT

public:
	SplashScreen(QWidget *parent = 0);
	~SplashScreen(void);

	static void showSplash(QThread *thread);

private:
	enum
	{
		STATUS_FADE_IN = 0,
		STATUS_WAIT = 1,
		STATUS_FADE_OUT = 2,
		STATUS_DONE = 3
	}
	status_t;

	Ui::SplashScreen *ui; //for Qt UIC

	QScopedPointer<QMovie> m_working;
	QScopedPointer<QEventLoop> m_loop;
	QScopedPointer<QTimer> m_timer;
	QScopedPointer<MUtils::Taskbar7> m_taskbar;
	
	const unsigned int m_opacitySteps;

	unsigned int m_status;
	unsigned int m_fadeValue;

	QAtomicInt m_canClose;
	QAtomicInt m_taskBarFlag;

private slots:
	void updateHandler(void);
	void threadComplete(void);

protected:
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
	void closeEvent(QCloseEvent *event);
};
