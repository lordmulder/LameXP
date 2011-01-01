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

#include "Dialog_MetaInfo.h"

#include "Global.h"
#include "Model_MetaInfo.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QTimer>

MetaInfoDialog::MetaInfoDialog(QWidget *parent)
	: QDialog(parent)
{
	//Init the dialog, from the .ui file
	setupUi(this);

	//Fix size
	setMinimumSize(this->size());
	setMaximumHeight(this->height());

	//Setup table view
	tableView->verticalHeader()->setVisible(false);
	tableView->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	tableView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
	tableView->horizontalHeader()->setStretchLastSection(true);

	//Enable up/down button
	connect(upButton, SIGNAL(clicked()), this, SLOT(upButtonClicked()));
	connect(downButton, SIGNAL(clicked()), this, SLOT(downButtonClicked()));
	connect(editButton, SIGNAL(clicked()), this, SLOT(editButtonClicked()));
}

MetaInfoDialog::~MetaInfoDialog(void)
{
}

int MetaInfoDialog::exec(AudioFileModel &audioFile, bool allowUp, bool allowDown)
{
	MetaInfoModel *model = new MetaInfoModel(&audioFile);
	tableView->setModel(model);
	setWindowTitle(QString("Meta Information: %1").arg(QFileInfo(audioFile.filePath()).fileName()));
	upButton->setEnabled(allowUp);
	downButton->setEnabled(allowDown);

	int iResult = QDialog::exec();
	
	tableView->setModel(NULL);
	LAMEXP_DELETE(model);

	return iResult;
}

void MetaInfoDialog::upButtonClicked(void)
{
	done(-1);
}

void MetaInfoDialog::downButtonClicked(void)
{
	done(+1);
}

void MetaInfoDialog::editButtonClicked(void)
{
	dynamic_cast<MetaInfoModel*>(tableView->model())->editItem(tableView->currentIndex(), this);
}
