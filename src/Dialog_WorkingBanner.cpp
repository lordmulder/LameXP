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

#include "Dialog_WorkingBanner.h"

#include "Global.h"
#include "WinSevenTaskbar.h"

#include <QThread>
#include <QMovie>
#include <QKeyEvent>
#include <QFontMetrics>
#include <Windows.h>

#define EPS (1.0E-5)

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
	labelWorking->setMovie(m_working);
	m_working->start();

	//Set wait cursor
	setCursor(Qt::WaitCursor);

	//Init taskbar
	WinSevenTaskbar::initTaskbar();
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
	setText(text);
	QApplication::processEvents();
}

void WorkingBanner::close(void)
{
	m_canClose = true;
	QDialog::close();
}

void WorkingBanner::show(const QString &text, QThread *thread)
{
	//Show splash
	this->show(text);

	//Start the thread
	thread->start();

	//Set taskbar state
	WinSevenTaskbar::setOverlayIcon(dynamic_cast<QWidget*>(this->parent()), &QIcon(":/icons/hourglass.png"));
	WinSevenTaskbar::setTaskbarState(dynamic_cast<QWidget*>(this->parent()), WinSevenTaskbar::WinSevenTaskbarIndeterminateState);

	//Loop while thread is running
	while(thread->isRunning())
	{
		QApplication::processEvents(QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents);
	}

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
	if(this->isVisible())
	{
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	}
}
