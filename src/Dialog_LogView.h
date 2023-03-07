///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2023 LoRd_MuldeR <MuldeR2@GMX.de>
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

//Class declarations
class QIcon;

//UIC forward declartion
namespace Ui {
	class LogViewDialog;
}

class LogViewDialog : public QDialog
{
	Q_OBJECT

public:
	LogViewDialog(QWidget *parent = 0);
	~LogViewDialog(void);

	int exec(const QStringList &logData);

public slots:
	void copyButtonClicked(void);
	void saveButtonClicked(void);

private slots:
	void restoreIcon(void);

private:
	Ui::LogViewDialog *ui; //for Qt UIC
	
	bool m_clipboardUsed;
	const QIcon *m_acceptIcon;
	QIcon *m_oldIcon;
};
