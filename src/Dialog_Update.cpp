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

#include "Dialog_Update.h"

#include <QClipboard>
#include <QFileDialog>
#include <QTimer>

#include <Windows.h>

UpdateDialog::UpdateDialog(QWidget *parent)
:
	QDialog(parent)
{
	//Init the dialog, from the .ui file
	setupUi(this);
	setWindowFlags(windowFlags() ^ Qt::WindowContextHelpButtonHint);
	setMinimumSize(size());
	setMaximumHeight(height());

	//Disable "X" button
	HMENU hMenu = GetSystemMenu((HWND) winId(), FALSE);
	EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);

	//Init flags
	m_clipboardUsed = false;
}

UpdateDialog::~UpdateDialog(void)
{
	if(m_clipboardUsed)
	{
		QApplication::clipboard()->clear();
	}
}

void UpdateDialog::showEvent(QShowEvent *event)
{
	statusLabel->setText("Checking for new updates online, please wait...");
	QTimer::singleShot(8000, this, SLOT(updateCompleted()));
	installButton->setEnabled(false);
	closeButton->setEnabled(false);
}

void UpdateDialog::updateCompleted(void)
{
	statusLabel->setText("No new updates avialbale. Your version of LameXP is up-to-date.");
	closeButton->setEnabled(true);
}

