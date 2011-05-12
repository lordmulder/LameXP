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

#include "Dialog_CueImport.h"

#include "Global.h"
#include "Model_CueSheet.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QTimer>
#include <QFileDialog>
#include <QMenu>

#define SET_FONT_BOLD(WIDGET,BOLD) { QFont _font = WIDGET->font(); _font.setBold(BOLD); WIDGET->setFont(_font); }

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

CueImportDialog::CueImportDialog(QWidget *parent)
:
	QDialog(parent)
{
	//Init the dialog, from the .ui file
	setupUi(this);

	//Fix size
	setMinimumSize(this->size());
	setMaximumHeight(this->height());

	//Create model
	m_model = new CueSheetModel();
	
	//Setup table view
	treeView->setModel(m_model);
	treeView->header()->setStretchLastSection(false);
	treeView->header()->setResizeMode(QHeaderView::ResizeToContents);
	treeView->header()->setResizeMode(1, QHeaderView::Stretch);
	treeView->header()->setMovable(false);
	treeView->setItemsExpandable(false);
	treeView->expandAll();

	//Enable up/down button
	connect(imprtButton, SIGNAL(clicked()), this, SLOT(importButtonClicked()));

	//Translate
	labelHeaderText->setText(QString("<b>%1</b><br>%2").arg(tr("Import Cue Sheet"), tr("The following Cue Sheet will be split and imported into LameXP.")));
}

CueImportDialog::~CueImportDialog(void)
{
	LAMEXP_DELETE(m_model);
}

////////////////////////////////////////////////////////////
// Slots
////////////////////////////////////////////////////////////

int CueImportDialog::exec(const QString &cueFile)
{
	/*
	MetaInfoModel *model = new MetaInfoModel(&audioFile);
	tableView->setModel(model);
	tableView->show();
	frameArtwork->hide();
	setWindowTitle(QString("Meta Information: %1").arg(QFileInfo(audioFile.filePath()).fileName()));
	editButton->setEnabled(true);
	upButton->setEnabled(allowUp);
	downButton->setEnabled(allowDown);
	buttonArtwork->setChecked(false);

	if(!audioFile.fileCover().isEmpty())
	{
		QImage artwork;
		if(artwork.load(audioFile.fileCover()))
		{
			if((artwork.width() > 256) || (artwork.height() > 256))
			{
				artwork = artwork.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);
			}
			labelArtwork->setPixmap(QPixmap::fromImage(artwork));
		}
		else
		{
			qWarning("Error: Failed to load cover art!");
			labelArtwork->setPixmap(QPixmap::fromImage(QImage(":/images/CD.png")));
		}
	}
	else
	{
		labelArtwork->setPixmap(QPixmap::fromImage(QImage(":/images/CD.png")));
	}

	int iResult = QDialog::exec();
	
	tableView->setModel(NULL);
	LAMEXP_DELETE(model);

	return iResult;*/
	
	return QDialog::exec();
}

void CueImportDialog::importButtonClicked(void)
{
	QMessageBox::information(this, "Not implemenred", "Sorry, not yet. Please try again in a later version!");
}
