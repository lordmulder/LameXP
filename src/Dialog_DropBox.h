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

#pragma once

#include "../tmp/UIC_DropBox.h"

#include <QPoint>

class QDragEnterEvent;
class QMouseEvent;
class QAbstractItemModel;
class SettingsModel;

////////////////////////////////////////////////////////////
// Splash Frame
////////////////////////////////////////////////////////////

class DropBox: public QDialog, private Ui::DropBox
{
	Q_OBJECT

public:
	DropBox(QWidget *parent = 0, QAbstractItemModel *model = 0, SettingsModel *settings = 0);
	~DropBox(void);
	
private:
	bool m_canClose;
	QPoint m_mouseReferencePoint;
	QPoint m_windowReferencePoint;
	QLabel m_counterLabel;
	QAbstractItemModel *m_model;
	SettingsModel *m_settings;
	bool m_moving;
	bool m_firstShow;

protected:
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
	void closeEvent(QCloseEvent *event);
	void showEvent(QShowEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void changeEvent(QEvent *e);

public slots:
	void modelChanged(void);
	void doRetranslate(void);
	void showToolTip(void);
};
