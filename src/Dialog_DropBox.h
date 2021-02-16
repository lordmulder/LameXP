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

#pragma once

#include <QDialog>

//UIC forward declartion
namespace Ui {
	class DropBox;
}

//Class declarations
class QAbstractItemModel;
class QDragEnterEvent;
class QLabel;
class QMouseEvent;
class QPoint;
class SettingsModel;

//DropBox class
class DropBox: public QDialog
{
	Q_OBJECT

public:
	DropBox(QWidget *parent = 0, QAbstractItemModel *model = 0, SettingsModel *settings = 0);
	~DropBox(void);
	
private:
	Ui::DropBox *ui; //for Qt UIC

	bool m_canClose;
	QPoint *m_mouseReferencePoint;
	QPoint *m_windowReferencePoint;
	QLabel *m_counterLabel;
	QAbstractItemModel *const m_model;
	SettingsModel *const m_settings;
	bool m_moving;
	bool m_firstShow;
	QRect m_screenGeometry;

	static void boundWidget(QWidget *widget);

protected:
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
	void closeEvent(QCloseEvent *event);
	void showEvent(QShowEvent *event);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void changeEvent(QEvent *e);
	bool event(QEvent *event);

public slots:
	void modelChanged(void);
	void showToolTip(void);
};
