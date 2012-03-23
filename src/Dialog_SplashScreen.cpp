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

#include "Dialog_SplashScreen.h"

#include "Global.h"

#include <QThread>
#include <QMovie>
#include <QKeyEvent>
#include <QTimer>

#include "WinSevenTaskbar.h"

#define EPS (1.0E-5)
#define FADE_DELAY 4

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
	double opacity = 0.0;
	SplashScreen *splashScreen = new SplashScreen();
	
	//Show splash
	splashScreen->m_canClose = false;
	splashScreen->setWindowOpacity(opacity);
	splashScreen->show();

	//Wait for window to show
	QApplication::processEvents();
	Sleep(100);
	QApplication::processEvents();

	//Setup the event loop
	QEventLoop *loop = new QEventLoop(splashScreen);
	connect(thread, SIGNAL(terminated()), loop, SLOT(quit()), Qt::QueuedConnection);
	connect(thread, SIGNAL(finished()), loop, SLOT(quit()), Qt::QueuedConnection);

	//Create timer
	QTimer *timer = new QTimer();
	connect(timer, SIGNAL(timeout()), loop, SLOT(quit()));

	//Start thread
	QApplication::processEvents();
	thread->start();
	QApplication::processEvents();

	//Init taskbar
	WinSevenTaskbar::setTaskbarState(splashScreen, WinSevenTaskbar::WinSevenTaskbarIndeterminateState);

	//Fade in
	for(int i = 0; i <= 100; i++)
	{
		opacity = 0.01 * static_cast<double>(i);
		splashScreen->setWindowOpacity(opacity);
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, FADE_DELAY);
		Sleep(FADE_DELAY);
	}

	//Start the timer
	timer->start(30720);

	//Loop while thread is still running
	bool bStillRunning = threadStillRunning(thread);
	while(bStillRunning)
	{
		loop->exec();
		if(bStillRunning = threadStillRunning(thread))
		{
			qWarning("Potential deadlock in initialization thread!");
		}
	}

	//Stop the timer
	timer->stop();

	//Fade out
	for(int i = 100; i >= 0; i--)
	{
		opacity = 0.01 * static_cast<double>(i);
		splashScreen->setWindowOpacity(opacity);
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, FADE_DELAY);
		Sleep(FADE_DELAY);
	}

	//Restore taskbar
	WinSevenTaskbar::setTaskbarState(splashScreen, WinSevenTaskbar::WinSevenTaskbarNoState);

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

////////////////////////////////////////////////////////////
// HELPER FUNCTIONS
////////////////////////////////////////////////////////////

bool SplashScreen::threadStillRunning(const QThread *thread)
{
	for(int i = 0; i < 128; i++)
	{
		if(!(thread->isRunning()))
		{
			return false;
		}
		QThread::yieldCurrentThread();
	}

	return thread->isRunning();
}
