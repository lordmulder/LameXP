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

#include "Dialog_SplashScreen.h"

#include "Global.h"

#include <QThread>
#include <QMovie>
#include <QKeyEvent>
#include <QTimer>

#include "WinSevenTaskbar.h"

#define FADE_DELAY 16
#define OPACITY_DELTA 0.04

//Setup taskbar indicator
#define SET_TASKBAR_STATE(WIDGET,VAR,FLAG) do \
{ \
	if(FLAG) \
	{ \
		if(!(VAR)) (VAR) = WinSevenTaskbar::setTaskbarState((WIDGET), WinSevenTaskbar::WinSevenTaskbarIndeterminateState); \
	} \
	else \
	{ \
		if((VAR)) (VAR) = (!WinSevenTaskbar::setTaskbarState((WIDGET), WinSevenTaskbar::WinSevenTaskbarNoState)); \
	} \
} \
while(0)

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

SplashScreen::SplashScreen(QWidget *parent)
:
	m_opacitySteps(qRound(1.0 / OPACITY_DELTA)),
	QFrame(parent, Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint)
{
	//Init the dialog, from the .ui file
	setupUi(this);

	//Make size fixed
	setFixedSize(this->size());

	//Create event loop
	m_loop = new QEventLoop(this);

	//Create timer
	m_timer = new QTimer(this);
	m_timer->setInterval(FADE_DELAY);
	m_timer->setSingleShot(false);

	//Connect timer to slot
	connect(m_timer, SIGNAL(timeout()), this, SLOT(updateHandler()));

	//Start animation
	m_working = new QMovie(":/images/Loading.gif");
	labelLoading->setMovie(m_working);
	m_working->start();

	//Set wait cursor
	setCursor(Qt::WaitCursor);

	//Init status
	m_canClose = false;
	m_status = STATUS_FADE_IN;
	m_fadeValue = 0;
	m_taskBarInit = false;
}

////////////////////////////////////////////////////////////
// Destructor
////////////////////////////////////////////////////////////

SplashScreen::~SplashScreen(void)
{
	if(m_working)
	{
		m_working->stop();
	}

	LAMEXP_DELETE(m_working);
	LAMEXP_DELETE(m_loop);
	LAMEXP_DELETE(m_timer);
}

////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
////////////////////////////////////////////////////////////

void SplashScreen::showSplash(QThread *thread)
{
	SplashScreen *splashScreen = new SplashScreen();
	
	//Show splash
	splashScreen->setWindowOpacity(OPACITY_DELTA);
	splashScreen->show();

	//Wait for window to show
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	splashScreen->repaint();
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

	//Connect thread signals
	connect(thread, SIGNAL(terminated()), splashScreen, SLOT(threadComplete()), Qt::QueuedConnection);
	connect(thread, SIGNAL(finished()),   splashScreen, SLOT(threadComplete()), Qt::QueuedConnection);

	//Init taskbar
	SET_TASKBAR_STATE(splashScreen, splashScreen->m_taskBarInit, true);

	//Start the thread
	splashScreen->m_timer->start(FADE_DELAY);
	QTimer::singleShot(8*60*1000, splashScreen->m_loop, SLOT(quit()));
	QTimer::singleShot(0, thread, SLOT(start()));

	//Start event handling!
	const int ret = splashScreen->m_loop->exec(QEventLoop::ExcludeUserInputEvents);

	//Check for timeout
	if(ret != 42)
	{
		thread->terminate();
		qFatal("Deadlock in initialization thread encountered!");
	}

	//Restore taskbar
	SET_TASKBAR_STATE(splashScreen, splashScreen->m_taskBarInit, false);

	//Hide splash
	splashScreen->m_canClose = true;
	splashScreen->close();

	//Free
	LAMEXP_DELETE(splashScreen);
}

////////////////////////////////////////////////////////////
// SLOTS
////////////////////////////////////////////////////////////

void SplashScreen::updateHandler(void)
{
	if(m_status == STATUS_FADE_IN)
	{
		if(m_fadeValue < m_opacitySteps)
		{
			setWindowOpacity(OPACITY_DELTA * static_cast<double>(++m_fadeValue));
			SET_TASKBAR_STATE(this, m_taskBarInit, true);
		}
		else
		{
			setWindowOpacity(1.0);
			m_timer->stop();
			m_status = STATUS_WAIT;
		}
	}
	else if(m_status == STATUS_FADE_OUT)
	{
		if(m_fadeValue > 0)
		{
			setWindowOpacity(OPACITY_DELTA * static_cast<double>(--m_fadeValue));
			SET_TASKBAR_STATE(this, m_taskBarInit, true);
		}
		else
		{
			setWindowOpacity(0.0);
			m_timer->stop();
			m_status = STATUS_DONE;
			m_loop->exit(42);
		}
	}
}

void SplashScreen::threadComplete(void)
{
	m_status = STATUS_FADE_OUT;
	if(!m_timer->isActive())
	{
		m_timer->start(FADE_DELAY);
	}
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

void SplashScreen::keyPressEvent(QKeyEvent *event)
{
	event->ignore();
}

void SplashScreen::keyReleaseEvent(QKeyEvent *event)
{
	event->ignore();
}

void SplashScreen::closeEvent(QCloseEvent *event)
{
	if(!m_canClose) event->ignore();
}

bool SplashScreen::winEvent(MSG *message, long *result)
{
	return WinSevenTaskbar::handleWinEvent(message, result);
}
