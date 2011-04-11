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

#include "Dialog_DropBox.h"

#include "Global.h"
#include "Model_Settings.h"

#include <QThread>
#include <QMovie>
#include <QKeyEvent>
#include <QDesktopWidget>
#include <QToolTip>
#include <QTimer>

#define EPS (1.0E-5)
#define SET_FONT_BOLD(WIDGET,BOLD) { QFont _font = WIDGET.font(); _font.setBold(BOLD); WIDGET.setFont(_font); }

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

DropBox::DropBox(QWidget *parent, QAbstractItemModel *model, SettingsModel *settings)
:
	QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowStaysOnTopHint),
	m_counterLabel(this),
	m_model(model),
	m_settings(settings),
	m_moving(false),
	m_firstShow(true)
{
	//Init the dialog, from the .ui file
	setupUi(this);
	
	//Init counter
	m_counterLabel.setParent(dropBoxLabel);
	m_counterLabel.setText("0");
	m_counterLabel.setAlignment(Qt::AlignHCenter | Qt::AlignTop);
	SET_FONT_BOLD(m_counterLabel, true);

	//Prevent close
	m_canClose = false;

	//Make transparent
	setWindowOpacity(0.8);
	
	//Translate UI
	QEvent languageChangeEvent(QEvent::LanguageChange);
	changeEvent(&languageChangeEvent);
}

////////////////////////////////////////////////////////////
// Destructor
////////////////////////////////////////////////////////////

DropBox::~DropBox(void)
{
}

////////////////////////////////////////////////////////////
// PUBLIC SLOTS
////////////////////////////////////////////////////////////

void DropBox::modelChanged(void)
{
	if(m_model)
	{
		m_counterLabel.setText(QString::number(m_model->rowCount()));
	}
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*
 * Re-translate the UI
 */
void DropBox::changeEvent(QEvent *e)
{
	if(e->type() == QEvent::LanguageChange)
	{
		Ui::DropBox::retranslateUi(this);
		dropBoxLabel->setToolTip(QString("<b>%1</b><br><nobr>%2</nobr><br><nobr>%3</nobr>").arg(tr("LameXP DropBox"), tr("You can add files to LameXP via Drag&amp;Drop here!"), tr("(Right-click to close the DropBox)")));
	}
}


void DropBox::showEvent(QShowEvent *event)
{
	QRect screenGeometry = QApplication::desktop()->availableGeometry();

	resize(dropBoxLabel->pixmap()->size());
	setMaximumSize(dropBoxLabel->pixmap()->size());
	
	m_counterLabel.setGeometry(0, dropBoxLabel->height() - 30, dropBoxLabel->width(), 25);

	if(m_firstShow)
	{
		m_firstShow = false;
		int max_x = screenGeometry.width() - frameGeometry().width();
		int max_y = screenGeometry.height() - frameGeometry().height();
		move(max_x, max_y);
		QTimer::singleShot(333, this, SLOT(showToolTip()));
	}

	m_moving = false;
}

void DropBox::keyPressEvent(QKeyEvent *event)
{
	event->ignore();
}

void DropBox::keyReleaseEvent(QKeyEvent *event)
{
	event->ignore();
}

void DropBox::closeEvent(QCloseEvent *event)
{
	if(!m_canClose) event->ignore();
}

void DropBox::mousePressEvent(QMouseEvent *event)
{
	if(m_moving)
	{
		event->ignore();
		return;
	}
	
	if(event->button() == Qt::RightButton)
	{
		hide();
		if(m_settings) m_settings->dropBoxWidgetEnabled(false);
		return;
	}

	QApplication::setOverrideCursor(Qt::SizeAllCursor);
	m_moving = true;
	m_windowReferencePoint = this->pos();
	m_mouseReferencePoint = event->globalPos();
}

void DropBox::mouseReleaseEvent(QMouseEvent *event)
{
	if(m_moving && event->button() != Qt::RightButton)
	{
		QApplication::restoreOverrideCursor();
		m_moving = false;
	}
}

void DropBox::mouseMoveEvent(QMouseEvent *event)
{
	if(!m_moving)
	{
		return;
	}
	
	static const int magnetic = 22;
	QRect screenGeometry = QApplication::desktop()->availableGeometry();
	
	int delta_x = m_mouseReferencePoint.x() - event->globalX();
	int delta_y = m_mouseReferencePoint.y() - event->globalY();
	
	int max_x = screenGeometry.width() - frameGeometry().width();
	int max_y = screenGeometry.height() - frameGeometry().height();

	int new_x = min(max_x, max(0, m_windowReferencePoint.x() - delta_x));
	int new_y = min(max_y, max(0, m_windowReferencePoint.y() - delta_y));

	if(new_x < magnetic)
	{
		new_x = 0;
	}
	else if(max_x - new_x < magnetic)
	{
		new_x = max_x;
	}

	if(new_y < magnetic)
	{
		new_y = 0;
	}
	else if(max_y - new_y < magnetic)
	{
		new_y = max_y;
	}

	move(new_x, new_y);
}

void DropBox::showToolTip(void)
{
	QToolTip::showText(dropBoxLabel->mapToGlobal(dropBoxLabel->pos()), dropBoxLabel->toolTip());
}
