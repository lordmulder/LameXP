///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2013 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Dialog_SplashScreen.h"

#include "Global.h"

#include <QThread>
#include <QMovie>
#include <QKeyEvent>
#include <QTimer>

#include "WinSevenTaskbar.h"

#define FADE_DELAY 16
#define OPACITY_DELTA 0.02

/* It can happen that the QThread has just terminated and already emitted the 'terminated' signal, but did NOT change the 'isRunning' flag to FALSE yet. */
/* For this reason the macro will first check the 'isRunning' flag. If (and only if) the flag still returns TRUE, then we will wait() for at most 50 ms. */
/* If, after 50 ms, the wait() function returns with FALSE, then the thread probably is still running and we return TRUE. Otherwise we can return FALSE. */
#define THREAD_RUNNING(THRD) (((THRD)->isRunning()) ? (!((THRD)->wait(50))) : false)

#define SET_TASKBAR_STATE(FLAG) do \
{ \
	if(FLAG) \
	{ \
		if(!bTaskBar) bTaskBar = WinSevenTaskbar::setTaskbarState(splashScreen, WinSevenTaskbar::WinSevenTaskbarIndeterminateState); \
	} \
	else \
	{ \
		if(bTaskBar) bTaskBar = (!WinSevenTaskbar::setTaskbarState(splashScreen, WinSevenTaskbar::WinSevenTaskbarNoState)); \
	} \
} \
while(0)

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

SplashScreen::SplashScreen(QWidget *parent)
:
	QFrame(parent, Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint)
{
	//Init the dialog, from the .ui file
	setupUi(this);

	//Start animation
	m_working = new QMovie(":/images/Loading.gif");
	labelLoading->setMovie(m_working);
	m_working->start();

	//Set wait cursor
	setCursor(Qt::WaitCursor);

	//Prevent close
	m_canClose = false;
}

////////////////////////////////////////////////////////////
// Destructor
////////////////////////////////////////////////////////////

SplashScreen::~SplashScreen(void)
{
	if(m_working)
	{
		m_working->stop();
		delete m_working;
		m_working = NULL;
	}
}

////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
////////////////////////////////////////////////////////////

void SplashScreen::showSplash(QThread *thread)
{
	double opacity = OPACITY_DELTA;
	const int opacitySteps = qRound(1.0 / OPACITY_DELTA);
	SplashScreen *splashScreen = new SplashScreen();
	bool bTaskBar = false;
	
	//Show splash
	splashScreen->m_canClose = false;
	splashScreen->setWindowOpacity(opacity);
	splashScreen->setFixedSize(splashScreen->size());
	splashScreen->show();

	//Wait for window to show
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	splashScreen->repaint();
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

	//Setup the event loop
	QEventLoop *loop = new QEventLoop(splashScreen);
	connect(thread, SIGNAL(terminated()), loop, SLOT(quit()), Qt::QueuedConnection);
	connect(thread, SIGNAL(finished()), loop, SLOT(quit()), Qt::QueuedConnection);

	//Create timer
	QTimer *timer = new QTimer();
	connect(timer, SIGNAL(timeout()), loop, SLOT(quit()));

	//Start thread
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	thread->start();
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

	//Init taskbar
	SET_TASKBAR_STATE(true);

	//Fade in
	for(int i = 1; i <= opacitySteps; i++)
	{
		opacity = (i < opacitySteps) ? (OPACITY_DELTA * static_cast<double>(i)) : 1.0;
		splashScreen->setWindowOpacity(opacity);
		splashScreen->update();
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, FADE_DELAY);
		SET_TASKBAR_STATE(true);
		Sleep(FADE_DELAY);
	}

	//Start the timer
	timer->start(30720);

	//Loop while thread is still running
	if(bool bIsRunning = THREAD_RUNNING(thread))
	{
		int deadlockCounter = 0;
		while(bIsRunning)
		{
			loop->exec();
			if(bIsRunning = THREAD_RUNNING(thread))
			{
				qWarning("Potential deadlock in initialization thread!");
				if(++deadlockCounter >= 10) qFatal("Deadlock in initialization thread!");
			}
		}
	}

	//Stop the timer
	timer->stop();

	//Fade out
	for(int i = opacitySteps; i >= 0; i--)
	{
		opacity = OPACITY_DELTA * static_cast<double>(i);
		splashScreen->setWindowOpacity(opacity);
		splashScreen->update();
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, FADE_DELAY);
		Sleep(FADE_DELAY);
	}

	//Restore taskbar
	SET_TASKBAR_STATE(false);

	//Hide splash
	splashScreen->m_canClose = true;
	splashScreen->close();

	//Free
	LAMEXP_DELETE(loop);
	LAMEXP_DELETE(timer);
	LAMEXP_DELETE(splashScreen);
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
