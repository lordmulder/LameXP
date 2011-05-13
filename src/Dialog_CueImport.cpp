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
#include "Dialog_WorkingBanner.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QTimer>
#include <QFileDialog>
#include <QProgressDialog>
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
	connect(m_model, SIGNAL(modelReset()), this, SLOT(modelChanged()));
	
	//Setup table view
	treeView->setModel(m_model);
	treeView->header()->setStretchLastSection(false);
	treeView->header()->setResizeMode(QHeaderView::ResizeToContents);
	treeView->header()->setResizeMode(1, QHeaderView::Stretch);
	treeView->header()->setMovable(false);
	treeView->setItemsExpandable(false);

	//Enable up/down button
	connect(imprtButton, SIGNAL(clicked()), this, SLOT(importButtonClicked()));
	connect(browseButton, SIGNAL(clicked()), this, SLOT(importButtonClicked()));

	//Translate
	labelHeaderText->setText(QString("<b>%1</b><br>%2").arg(tr("Import Cue Sheet"), tr("The following Cue Sheet will be split and imported into LameXP.")));
}

CueImportDialog::~CueImportDialog(void)
{
	LAMEXP_DELETE(m_model);
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

void CueImportDialog::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);
	modelChanged();
}

////////////////////////////////////////////////////////////
// Slots
////////////////////////////////////////////////////////////

int CueImportDialog::exec(const QString &cueFile)
{
	WorkingBanner *progress = new WorkingBanner(dynamic_cast<QWidget*>(parent()));
	progress->show(tr("Loading Cue Sheet file, please be patient..."));
	int iResult = m_model->loadCueSheet(cueFile, QApplication::instance());
	progress->close();
	LAMEXP_DELETE(progress);
	
	if(iResult)
	{
		QString errorMsg = tr("An unknown error has occured!");
		
		switch(iResult)
		{
		case 1:
			errorMsg = tr("The file could not be opened for reading!");
			break;
		case 2:
			errorMsg = tr("The file does not look like a valid Cue Sheet disc image file!");
			break;
		case 3:
			errorMsg = tr("Could not find a supported audio track in the Cue Sheet!");
			break;
		}
		
		QString text = QString("<nobr>%1</nobr><br><nobr>%2</nobr><br><br><nobr>%3</nobr>").arg(tr("Failed to load the Cue Sheet file:"), cueFile, errorMsg).replace("-", "&minus;");
		QMessageBox::warning(dynamic_cast<QWidget*>(parent()), tr("Cue Sheet Error"), text);
		return iResult;
	}

	return QDialog::exec();
}

void CueImportDialog::modelChanged(void)
{
	treeView->expandAll();
}

void CueImportDialog::importButtonClicked(void)
{
	QMessageBox::information(this, "Not implemenred", "Sorry, not yet. Please try again in a later version!");
}
