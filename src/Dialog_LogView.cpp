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

#include "Dialog_LogView.h"

#include <QClipboard>
#include <QFileDialog>

LogViewDialog::LogViewDialog(QWidget *parent)
:
	QDialog(parent)
{
	//Init the dialog, from the .ui file
	setupUi(this);
	setWindowFlags(windowFlags() ^ Qt::WindowContextHelpButtonHint);
	//setWindowFlags(windowFlags() & (~Qt::WindowMaximizeButtonHint));

	//Clear
	textEdit->clear();

	//Enable buttons
	connect(buttonCopy, SIGNAL(clicked()), this, SLOT(copyButtonClicked()));
	connect(buttonSave, SIGNAL(clicked()), this, SLOT(saveButtonClicked()));

	//Init flags
	m_clipboardUsed = false;
}

LogViewDialog::~LogViewDialog(void)
{
	if(m_clipboardUsed)
	{
		QApplication::clipboard()->clear();
	}
}

int LogViewDialog::exec(const QStringList &logData)
{
	textEdit->setPlainText(logData.join("\n"));
	return QDialog::exec();
}

void LogViewDialog::copyButtonClicked(void)
{
	QMimeData *mime = new QMimeData();
	mime->setData("text/plain", textEdit->toPlainText().toUtf8().constData());
	QApplication::clipboard()->setMimeData(mime);
	m_clipboardUsed = true;
}

void LogViewDialog::saveButtonClicked(void)
{
	QString fileName = QFileDialog::getSaveFileName(this, "Save Log File", QDir::homePath(), "Log Files (*.txt)");
	
	if(!fileName.isEmpty())
	{
		QFile file(fileName);
		if(file.open(QIODevice::WriteOnly))
		{
			QByteArray data = textEdit->toPlainText().toUtf8();
			data.replace("\n", "\r\n");
			file.write(data.constData(), data.size());
			file.close();
		}
	}
}
