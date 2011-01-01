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

#include "Dialog_SplashScreen.h"

#include "Global.h"

#include <QThread>
#include <QMovie>
#include <QKeyEvent>
#include <Windows.h>

#define EPS (1.0E-5)

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

SplashScreen::SplashScreen(QWidget *parent)
	: QFrame(parent, Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint)
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
	SplashScreen *splashScreen = new SplashScreen();
	
	//Show splash
	splashScreen->m_canClose = false;
	splashScreen->setWindowOpacity(0.0);
	splashScreen->show();
	QApplication::processEvents();

	//Start the thread
	thread->start();
	
	//Fade in
	for(double d = 0.0; d <= 1.0 + EPS; d = d + 0.01)
	{
		splashScreen->setWindowOpacity(d);
		QApplication::processEvents();
		Sleep(6);
	}

	//Loop while thread is running
	while(thread->isRunning())
	{
		QApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents);
	}
	
	//Fade out
	for(double d = 1.0; d >= 0.0; d = d - 0.01)
	{
		splashScreen->setWindowOpacity(d);
		QApplication::processEvents();
		Sleep(6);
	}

	//Hide splash
	splashScreen->m_canClose = true;
	splashScreen->close();

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
