///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2017 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Dialog_LogView.h"

//UIC includes
#include "UIC_LogViewDialog.h"

//Internal
#include "Global.h"

//MUtils
#include <MUtils/Global.h>
#include <MUtils/Sound.h>

//Qt includes
#include <QClipboard>
#include <QFileDialog>
#include <QMimeData>
#include <QTimer>

LogViewDialog::LogViewDialog(QWidget *parent)
:
	QDialog(parent),
	m_acceptIcon(new QIcon(":/icons/accept.png")),
	m_oldIcon(new QIcon()),
	ui(new Ui::LogViewDialog)
{
	//Init the dialog, from the .ui file
	ui->setupUi(this);
	setWindowFlags(windowFlags() ^ Qt::WindowContextHelpButtonHint);
	
	//Translate
	ui->headerText->setText(QString("<b>%1</b><br>%2").arg(tr("Log File"), tr("The log file shows detailed information about the selected job.")));

	//Clear
	ui->textEdit->clear();

	//Enable buttons
	connect(ui->buttonCopy, SIGNAL(clicked()), this, SLOT(copyButtonClicked()));
	connect(ui->buttonSave, SIGNAL(clicked()), this, SLOT(saveButtonClicked()));

	//Init flags
	m_clipboardUsed = false;
}

LogViewDialog::~LogViewDialog(void)
{
	if(m_clipboardUsed)
	{
		QApplication::clipboard()->clear();
	}

	MUTILS_DELETE(m_oldIcon);
	MUTILS_DELETE(m_acceptIcon);
	MUTILS_DELETE(ui);
}

int LogViewDialog::exec(const QStringList &logData)
{
	ui->textEdit->setPlainText(logData.join("\n"));
	return QDialog::exec();
}

void LogViewDialog::copyButtonClicked(void)
{
	QMimeData *mime = new QMimeData();
	mime->setData("text/plain", MUTILS_UTF8(ui->textEdit->toPlainText()));
	QApplication::clipboard()->setMimeData(mime);
	m_clipboardUsed = true;
	m_oldIcon->swap(ui->buttonCopy->icon());
	ui->buttonCopy->setIcon(*m_acceptIcon);
	ui->buttonCopy->blockSignals(true);
	QTimer::singleShot(1250, this, SLOT(restoreIcon()));
	MUtils::Sound::beep(MUtils::Sound::BEEP_NFO);
}

void LogViewDialog::saveButtonClicked(void)
{
	QString fileName = QFileDialog::getSaveFileName(this, "Save Log File", QDir::homePath(), "Log Files (*.txt)");
	
	if(!fileName.isEmpty())
	{
		QFile file(fileName);
		if(file.open(QIODevice::WriteOnly))
		{
			QByteArray data = ui->textEdit->toPlainText().toUtf8();
			data.replace("\n", "\r\n");
			file.write(data.constData(), data.size());
			file.close();
		}
	}
}

void LogViewDialog::restoreIcon(void)
{
	if(!m_oldIcon->isNull())
	{
		ui->buttonCopy->setIcon(*m_oldIcon);
		ui->buttonCopy->blockSignals(false);
		m_oldIcon->swap(QIcon());
	}
}
