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

#include "Dialog_WorkingBanner.h"

#include "Global.h"
#include "WinSevenTaskbar.h"

#include <QThread>
#include <QMovie>
#include <QKeyEvent>
#include <QFontMetrics>

#define EPS (1.0E-5)

/* It can happen that the QThread has just terminated and already emitted the 'terminated' signal, but did NOT change the 'isRunning' flag to FALSE yet. */
/* For this reason the macro will first check the 'isRunning' flag. If (and only if) the flag still returns TRUE, then we will wait() for at most 50 ms. */
/* If, after 50 ms, the wait() function returns with FALSE, then the thread probably is still running and we return TRUE. Otherwise we can return FALSE. */
#define THREAD_RUNNING(THRD) (((THRD)->isRunning()) ? (!((THRD)->wait(50))) : false)

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

WorkingBanner::WorkingBanner(QWidget *parent)
:
	QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint)
{
	//Init the dialog, from the .ui file
	setupUi(this);
	setModal(true);

	//Start animation
	m_working = new QMovie(":/images/Busy.gif");
	m_working->setSpeed(25);
	labelWorking->setMovie(m_working);
	m_working->start();

	//Set wait cursor
	setCursor(Qt::WaitCursor);
}

////////////////////////////////////////////////////////////
// Destructor
////////////////////////////////////////////////////////////

WorkingBanner::~WorkingBanner(void)
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

void WorkingBanner::show(const QString &text)
{
	m_canClose = false;
	QDialog::show();
	setFixedSize(size());
	setText(text);
	QApplication::processEvents();
}

bool WorkingBanner::close(void)
{
	m_canClose = true;
	emit userAbort();
	return QDialog::close();
}

void WorkingBanner::show(const QString &text, QThread *thread)
{
	//Show splash
	this->show(text);

	//Create event loop
	QEventLoop *loop = new QEventLoop(this);
	connect(thread, SIGNAL(finished()), loop, SLOT(quit()));
	connect(thread, SIGNAL(terminated()), loop, SLOT(quit()));

	//Set taskbar state
	WinSevenTaskbar::setOverlayIcon(dynamic_cast<QWidget*>(this->parent()), &QIcon(":/icons/hourglass.png"));
	WinSevenTaskbar::setTaskbarState(dynamic_cast<QWidget*>(this->parent()), WinSevenTaskbar::WinSevenTaskbarIndeterminateState);

	//Start the thread
	thread->start();

	//Loop while thread is still running
	while(THREAD_RUNNING(thread))
	{
		loop->exec();
	}

	//Set taskbar state
	WinSevenTaskbar::setTaskbarState(dynamic_cast<QWidget*>(this->parent()), WinSevenTaskbar::WinSevenTaskbarNoState);
	WinSevenTaskbar::setOverlayIcon(dynamic_cast<QWidget*>(this->parent()), NULL);

	//Free memory
	LAMEXP_DELETE(loop);

	//Hide splash
	this->close();
}

void WorkingBanner::show(const QString &text, QEventLoop *loop)
{
	//Show splash
	this->show(text);

	//Set taskbar state
	WinSevenTaskbar::setOverlayIcon(dynamic_cast<QWidget*>(this->parent()), &QIcon(":/icons/hourglass.png"));
	WinSevenTaskbar::setTaskbarState(dynamic_cast<QWidget*>(this->parent()), WinSevenTaskbar::WinSevenTaskbarIndeterminateState);

	//Loop while thread is running
	loop->exec(QEventLoop::ExcludeUserInputEvents);

	//Set taskbar state
	WinSevenTaskbar::setTaskbarState(dynamic_cast<QWidget*>(this->parent()), WinSevenTaskbar::WinSevenTaskbarNoState);
	WinSevenTaskbar::setOverlayIcon(dynamic_cast<QWidget*>(this->parent()), NULL);

	//Hide splash
	this->close();
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

void WorkingBanner::keyPressEvent(QKeyEvent *event)
{
	if(event->key() == Qt::Key_Escape)
	{
		qDebug("QT::KEY_ESCAPE pressed!");
		emit userAbort();
	}
	
	event->ignore();
}

void WorkingBanner::keyReleaseEvent(QKeyEvent *event)
{
	event->ignore();
}

void WorkingBanner::closeEvent(QCloseEvent *event)
{
	if(!m_canClose) event->ignore();
}

bool WorkingBanner::winEvent(MSG *message, long *result)
{
	return WinSevenTaskbar::handleWinEvent(message, result);
}

////////////////////////////////////////////////////////////
// SLOTS
////////////////////////////////////////////////////////////

void WorkingBanner::setText(const QString &text)
{
	QFontMetrics metrics(labelStatus->font());
	if(metrics.width(text) <= labelStatus->width())
	{
		labelStatus->setText(text);
	}
	else
	{
		QString choppedText = text.simplified().append("...");
		while(metrics.width(choppedText) > labelStatus->width() && choppedText.length() > 8)
		{
			choppedText.chop(4);
			choppedText = choppedText.trimmed();
			choppedText.append("...");
		}
		labelStatus->setText(choppedText);
	}
	/*
	if(this->isVisible())
	{
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}
	*/
}
