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
#include <QFileDialog>
#include <QMenu>

#define SET_FONT_BOLD(WIDGET,BOLD) { QFont _font = WIDGET->font(); _font.setBold(BOLD); WIDGET->setFont(_font); }

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

MetaInfoDialog::MetaInfoDialog(QWidget *parent)
:
	QDialog(parent)
{
	//Init the dialog, from the .ui file
	setupUi(this);
	
	//Hide artwork
	frameArtwork->hide();

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

	//Create context menu
	m_contextMenuInfo = new QMenu();
	m_contextMenuArtwork = new QMenu();
	QAction *editMetaInfoAction = m_contextMenuInfo->addAction(QIcon(":/icons/table_edit.png"), tr("Edit this Information"));
	QAction *copyMetaInfoAction = m_contextMenuInfo->addAction(QIcon(":/icons/page_white_copy.png"), tr("Copy everything to Meta Info tab"));
	QAction *clearMetaInfoAction = m_contextMenuInfo->addAction(QIcon(":/icons/bin.png"), tr("Clear all Meta Info"));
	QAction *loadArtworkAction = m_contextMenuArtwork->addAction(QIcon(":/icons/folder_image.png"), tr("Load Artwork From File"));
	QAction *clearArtworkAction = m_contextMenuArtwork->addAction(QIcon(":/icons/bin.png"), tr("Clear Artwork"));
	SET_FONT_BOLD(editMetaInfoAction, true);
	SET_FONT_BOLD(loadArtworkAction, true);
	connect(tableView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(infoContextMenuRequested(QPoint)));
	connect(labelArtwork, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(artworkContextMenuRequested(QPoint)));
	connect(frameArtwork, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(artworkContextMenuRequested(QPoint)));
	connect(editMetaInfoAction, SIGNAL(triggered(bool)), this, SLOT(editButtonClicked()));
	connect(copyMetaInfoAction, SIGNAL(triggered(bool)), this, SLOT(copyMetaInfoActionTriggered()));
	connect(clearMetaInfoAction, SIGNAL(triggered(bool)), this, SLOT(clearMetaInfoActionTriggered()));
	connect(loadArtworkAction, SIGNAL(triggered(bool)), this, SLOT(editButtonClicked()));
	connect(clearArtworkAction, SIGNAL(triggered(bool)), this, SLOT(clearArtworkActionTriggered()));

	//Translate
	labelHeaderText->setText(QString("<b>%1</b><br>%2").arg(tr("Meta Information"), tr("The following meta information have been extracted from the original file.")));
}

MetaInfoDialog::~MetaInfoDialog(void)
{
	LAMEXP_DELETE(m_contextMenuInfo);
	LAMEXP_DELETE(m_contextMenuArtwork);
}

////////////////////////////////////////////////////////////
// Slots
////////////////////////////////////////////////////////////

int MetaInfoDialog::exec(AudioFileModel &audioFile, bool allowUp, bool allowDown)
{
	MetaInfoModel *model = new MetaInfoModel(&audioFile);
	tableView->setModel(model);
	tableView->show();
	frameArtwork->hide();
	setWindowTitle(tr("Meta Information: %1").arg(QFileInfo(audioFile.filePath()).fileName()));
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
	if(!buttonArtwork->isChecked())
	{
		dynamic_cast<MetaInfoModel*>(tableView->model())->editItem(tableView->currentIndex(), this);
		return;
	}

	QString fileName = QFileDialog::getOpenFileName(this, tr("Load Artwork"), QString(), QString::fromLatin1("JPEG (*.jpg);;PNG (*.png);;GIF (*.gif)"));
	if(!fileName.isEmpty())
	{
		QImage artwork;
		if(artwork.load(fileName))
		{
			if((artwork.width() > 256) || (artwork.height() > 256))
			{
				artwork = artwork.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);
			}
			dynamic_cast<MetaInfoModel*>(tableView->model())->editArtwork(fileName);
			labelArtwork->setPixmap(QPixmap::fromImage(artwork));
		}
		else
		{
			qWarning("Error: Failed to load cover art!");
			QMessageBox::warning(this, tr("Artwork Error"), QString("<nobr>%1</nobr><br><br><nobr>%2</nobr>").arg(tr("Sorry, failed to load artwork from selected file!"), QDir::toNativeSeparators(fileName)));
		}
	}
}

void MetaInfoDialog::infoContextMenuRequested(const QPoint &pos)
{
	QAbstractScrollArea *scrollArea = dynamic_cast<QAbstractScrollArea*>(QObject::sender());
	QWidget *sender = scrollArea ? scrollArea->viewport() : dynamic_cast<QWidget*>(QObject::sender());

	if(sender)
	{
		if(pos.x() <= sender->width() && pos.y() <= sender->height() && pos.x() >= 0 && pos.y() >= 0)
		{
			m_contextMenuInfo->popup(sender->mapToGlobal(pos));
		}
	}
}

void MetaInfoDialog::artworkContextMenuRequested(const QPoint &pos)
{
	QAbstractScrollArea *scrollArea = dynamic_cast<QAbstractScrollArea*>(QObject::sender());
	QWidget *sender = scrollArea ? scrollArea->viewport() : dynamic_cast<QWidget*>(QObject::sender());

	if(sender)
	{
		if(pos.x() <= sender->width() && pos.y() <= sender->height() && pos.x() >= 0 && pos.y() >= 0)
		{
			m_contextMenuArtwork->popup(sender->mapToGlobal(pos));
		}
	}
}

void MetaInfoDialog::copyMetaInfoActionTriggered(void)
{
	done(INT_MAX);
}

void MetaInfoDialog::clearMetaInfoActionTriggered(void)
{
	if(MetaInfoModel *model = dynamic_cast<MetaInfoModel*>(tableView->model()))
	{
		model->clearData(true);
	}
}

void MetaInfoDialog::clearArtworkActionTriggered(void)
{
	labelArtwork->setPixmap(QPixmap::fromImage(QImage(":/images/CD.png")));
	dynamic_cast<MetaInfoModel*>(tableView->model())->editArtwork(QString());
}
