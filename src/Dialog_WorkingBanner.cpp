///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2025 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Dialog_WorkingBanner.h"
#include "UIC_WorkingBanner.h"

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
#include <QFontMetrics>
#include <QPainter>
#include <QWindowsVistaStyle>
#include <QTimer>

#define EPS (1.0E-5)

/* It can happen that the QThread has just terminated and already emitted the 'terminated' signal, but did NOT change the 'isRunning' flag to FALSE yet. */
/* For this reason the macro will first check the 'isRunning' flag. If (and only if) the flag still returns TRUE, then we will wait() for at most 50 ms. */
/* If, after 50 ms, the wait() function returns with FALSE, then the thread probably is still running and we return TRUE. Otherwise we can return FALSE. */
#define THREAD_RUNNING(THRD) (((THRD)->isRunning()) ? (!((THRD)->wait(1))) : false)

/*Update text color*/
static inline void SET_TEXT_COLOR(QWidget *control, const QColor &color)
{
	QPalette pal = control->palette();
	pal.setColor(QPalette::WindowText, color);
	pal.setColor(QPalette::Text, color);
	control->setPalette(pal);
}

/*Make widget translucent*/
static inline void MAKE_TRANSLUCENT(QWidget *control)
{
	control->setAttribute(Qt::WA_TranslucentBackground);
	control->setAttribute(Qt::WA_NoSystemBackground);
}

/*Update widget margins*/
static inline void UPDATE_MARGINS(QWidget *control, int l = 0, int r = 0, int t = 0, int b = 0)
{
	if(QLayout *layout = control->layout())
	{
		QMargins margins = layout->contentsMargins();
		margins.setLeft(margins.left() + l);
		margins.setRight(margins.right() + r);
		margins.setTop(margins.top() + t);
		margins.setBottom(margins.bottom() + b);
		layout->setContentsMargins(margins);
	}
}

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

WorkingBanner::WorkingBanner(QWidget *parent)
:
	QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint),
	ui(new Ui::WorkingBanner()),
	m_iconHourglass(new QIcon(":/icons/hourglass.png")),
	m_taskbar(new MUtils::Taskbar7(parent)),
	m_metrics(NULL), m_working(NULL), m_style(NULL)
{
	//Init the dialog, from the .ui file
	ui->setupUi(this);
	setModal(true);

	//Enable the "sheet of glass" effect
	if(MUtils::GUI::sheet_of_glass(this))
	{
		m_style.reset(new QWindowsVistaStyle());
		this->setStyle(m_style.data());
		ui->progressBar->setStyle(m_style.data());
		ui->labelStatus->setStyle(m_style.data());
		ui->labelStatus->setStyleSheet("background-color: #FFFFFF;");
	}
	else
	{
		UPDATE_MARGINS(this, 5);
		m_working.reset(new QMovie(":/images/Busy.gif"));
		m_working->setCacheMode(QMovie::CacheAll);
		ui->labelWorking->setMovie(m_working.data());
	}
	
	//Set Opacity
	this->setWindowOpacity(0.9);

	//Set wait cursor
	setCursor(Qt::WaitCursor);

	//Clear label
	ui->labelStatus->clear();
}

////////////////////////////////////////////////////////////
// Destructor
////////////////////////////////////////////////////////////

WorkingBanner::~WorkingBanner(void)
{
	if(!m_working.isNull())
	{
		m_working->stop();
	}

	delete ui;
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

	//Reset progress
	ui->progressBar->setMinimum(0);
	ui->progressBar->setMaximum(0);
	ui->progressBar->setValue(-1);
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
	m_taskbar->setOverlayIcon(m_iconHourglass.data());
	m_taskbar->setTaskbarState(MUtils::Taskbar7::TASKBAR_STATE_INTERMEDIATE);

	//Start the thread
	thread->start();

	//Update cursor
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	//Loop while thread is still running
	while(THREAD_RUNNING(thread))
	{
		loop->exec();
	}

	//Restore cursor
	QApplication::restoreOverrideCursor();

	//Set taskbar state
	m_taskbar->setTaskbarState(MUtils::Taskbar7::TASKBAR_STATE_NONE);
	m_taskbar->setOverlayIcon(NULL);

	//Free memory
	MUTILS_DELETE(loop);

	//Hide splash
	this->close();
}

void WorkingBanner::show(const QString &text, QEventLoop *loop)
{
	//Show splash
	this->show(text);

	//Set taskbar state
	m_taskbar->setOverlayIcon(m_iconHourglass.data());
	m_taskbar->setTaskbarState(MUtils::Taskbar7::TASKBAR_STATE_INTERMEDIATE);

	//Update cursor
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

	//Loop while thread is running
	loop->exec(QEventLoop::ExcludeUserInputEvents);

	//Restore cursor
	QApplication::restoreOverrideCursor();

	//Set taskbar state
	m_taskbar->setTaskbarState(MUtils::Taskbar7::TASKBAR_STATE_NONE);
	m_taskbar->setOverlayIcon(NULL);

	//Hide splash
	this->close();
}

bool WorkingBanner::close(void)
{
	m_canClose = true;
	emit userAbort();
	return QDialog::close();
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
	else if(event->key() == Qt::Key_M)
	{
		QTimer::singleShot(0, parent(), SLOT(showMinimized()));
	}
	
	QDialog::keyPressEvent(event);
}

void WorkingBanner::keyReleaseEvent(QKeyEvent *event)
{
	QDialog::keyReleaseEvent(event);
}

void WorkingBanner::closeEvent(QCloseEvent *event)
{
	if(!m_canClose) event->ignore();
}

void WorkingBanner::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);
	if(!event->spontaneous())
	{
		if(m_working)
		{
			m_working->start();
		}
	}
	QTimer::singleShot(25, this, SLOT(windowShown()));
}

void WorkingBanner::hideEvent(QHideEvent *event)
{
	QDialog::hideEvent(event);
	if(!event->spontaneous())
	{
		if(m_working)
		{
			m_working->stop();
		}
	}
}

////////////////////////////////////////////////////////////
// SLOTS
////////////////////////////////////////////////////////////

void WorkingBanner::windowShown(void)
{
	MUtils::GUI::bring_to_front(this);
}

void WorkingBanner::setText(const QString &text)
{
	if(m_metrics.isNull())
	{
		 m_metrics.reset(new QFontMetrics(ui->labelStatus->font()));
	}

	if(m_metrics->width(text) <= ui->labelStatus->width() - 16)
	{
		ui->labelStatus->setText(text);
	}
	else
	{
		QString choppedText = text.simplified().append("...");
		while((m_metrics->width(choppedText) > ui->labelStatus->width() - 16) && (choppedText.length() > 8))
		{
			choppedText.chop(4);
			choppedText = choppedText.trimmed();
			choppedText.append("...");
		}
		ui->labelStatus->setText(choppedText);
	}
}

void WorkingBanner::setProgressMax(unsigned int max)
{
	ui->progressBar->setMaximum(max);
	if(ui->progressBar->maximum() > ui->progressBar->minimum())
	{
		m_taskbar->setTaskbarState(MUtils::Taskbar7::TASKBAR_STATE_NONE);
		m_taskbar->setTaskbarProgress(ui->progressBar->value(), ui->progressBar->maximum());
	}
	else
	{
		m_taskbar->setTaskbarState(MUtils::Taskbar7::TASKBAR_STATE_INTERMEDIATE);
	}
}

void WorkingBanner::setProgressVal(unsigned int val)
{
	ui->progressBar->setValue(val);
	if(ui->progressBar->maximum() > ui->progressBar->minimum())
	{
		m_taskbar->setTaskbarProgress(ui->progressBar->value(), ui->progressBar->maximum());
	}
}

////////////////////////////////////////////////////////////
// Private
////////////////////////////////////////////////////////////

/*
void WorkingBanner::updateProgress(void)
{
	if(m_progressMax > 0)
	{
		int newProgress = qRound(qBound(0.0, static_cast<double>(m_progressVal) / static_cast<double>(m_progressMax), 1.0) * 100.0);
		if(m_progressInt != newProgress)
		{
			m_progressInt = newProgress;
			m_progress->setText(QString::number(m_progressInt));
			if(this->isVisible())
			{
				labelStatus->repaint();
			}
		}
	}
}
*/
