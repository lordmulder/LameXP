///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2016 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include <QEvent>
#include <QWidget>

class CustomEventFilter: public QObject
{
	Q_OBJECT

public:
	CustomEventFilter(void) {}
	~CustomEventFilter(void) {}

	bool eventFilter(QObject *obj, QEvent *event)
	{
		if(QWidget *sender = dynamic_cast<QWidget*>(obj))
		{
			switch(event->type())
			{
			case QEvent::Enter:
			case QEvent::Leave:
			case QEvent::KeyPress:
			case QEvent::KeyRelease:
			case QEvent::MouseButtonPress:
			case QEvent::MouseButtonRelease:
			case QEvent::FocusIn:
			case QEvent::FocusOut:
			case QEvent::TouchEnd:
				eventOccurred(sender, event);
				break;
			}
		}

		return false;
	}

signals:
	void eventOccurred(QWidget *sender, QEvent *event);
};
