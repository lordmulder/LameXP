///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2021 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Dialog_SplashScreen.h"

//UIC includes
#include "UIC_SplashScreen.h"

//Internal
#include "Global.h"

//MUtils
#include <MUtils/Global.h>
#include <MUtils/GUI.h>
#include <MUtils/Taskbar7.h>

//Qt
#include <QThread>
#include <QMovie>
#include <QKeyEvent>
#include <QTimer>

#define FADE_DELAY 16
#define OPACITY_DELTA 0.04

//Setup taskbar indicator
#define SET_TASKBAR_STATE(WIDGET,FLAG) do \
{ \
	const int _oldFlag = (WIDGET)->m_taskBarFlag.fetchAndStoreOrdered((FLAG) ? 1 : 0); \
	if(_oldFlag != ((FLAG) ? 1 : 0)) \
	{ \
		(WIDGET)->m_taskbar->setTaskbarState((FLAG) ? MUtils::Taskbar7::TASKBAR_STATE_INTERMEDIATE : MUtils::Taskbar7::TASKBAR_STATE_NONE); \
	} \
} \
while(0)

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

SplashScreen::SplashScreen(QWidget *parent)
:
	QFrame(parent, Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint),
	ui(new Ui::SplashScreen),
	m_opacitySteps(qRound(1.0 / OPACITY_DELTA)),
	m_taskbar(new MUtils::Taskbar7(this))
{
	//Init the dialog, from the .ui file
	ui->setupUi(this);

	//Make size fixed
	setFixedSize(this->size());
	
	//Create event loop
	m_loop.reset(new QEventLoop(this));

	//Create timer
	m_timer.reset(new QTimer(this));
	m_timer->setInterval(FADE_DELAY);
	m_timer->setSingleShot(false);

	//Connect timer to slot
	connect(m_timer.data(), SIGNAL(timeout()), this, SLOT(updateHandler()));

	//Enable "sheet of glass" effect on splash screen
	if(!MUtils::GUI::sheet_of_glass(this))
	{
		setStyleSheet("background-image: url(:/images/Background.jpg)");
	}

	//Start animation
	m_working.reset(new QMovie(":/images/Loading4.gif"));
	m_working->setCacheMode(QMovie::CacheAll);
	ui->labelLoading->setMovie(m_working.data());
	m_working->start();

	//Init status
	m_status = STATUS_FADE_IN;
	m_fadeValue = 0;
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

	delete ui;
}

////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
////////////////////////////////////////////////////////////

void SplashScreen::showSplash(QThread *thread)
{
	QScopedPointer<SplashScreen> splashScreen(new SplashScreen());

	//Show splash
	splashScreen->setWindowOpacity(OPACITY_DELTA);
	splashScreen->show();

	//Set wait cursor
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	//Wait for window to show
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	splashScreen->repaint(); MUtils::GUI::bring_to_front(splashScreen.data());
	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

	//Connect thread signals
	connect(thread, SIGNAL(terminated()), splashScreen.data(), SLOT(threadComplete()), Qt::QueuedConnection);
	connect(thread, SIGNAL(finished()),   splashScreen.data(), SLOT(threadComplete()), Qt::QueuedConnection);

	//Init taskbar
	SET_TASKBAR_STATE(splashScreen, true);

	//Start the thread
	splashScreen->m_timer->start(FADE_DELAY);
	QTimer::singleShot(8*60*1000, splashScreen->m_loop.data(), SLOT(quit()));
	QTimer::singleShot(333, thread, SLOT(start()));

	//Start event handling!
	const int ret = splashScreen->m_loop->exec(QEventLoop::ExcludeUserInputEvents);

	//Check for timeout
	if(ret != 42)
	{
		thread->terminate();
		qFatal("Deadlock in initialization thread encountered!");
	}

	//Restore taskbar
	SET_TASKBAR_STATE(splashScreen, false);

	//Restore cursor
	QApplication::restoreOverrideCursor();

	//Hide splash
	splashScreen->m_canClose.ref();
	splashScreen->close();
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
			SET_TASKBAR_STATE(this, true);
		}
		else
		{
			setWindowOpacity(1.0);
			MUtils::GUI::bring_to_front(this);
			m_timer->stop();
			m_status = STATUS_WAIT;
		}
	}
	else if(m_status == STATUS_FADE_OUT)
	{
		if(m_fadeValue > 0)
		{
			setWindowOpacity(OPACITY_DELTA * static_cast<double>(--m_fadeValue));
			SET_TASKBAR_STATE(this, true);
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
	MUtils::GUI::bring_to_front(this);
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
