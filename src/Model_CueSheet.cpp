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

#include "Model_CueSheet.h"
#include "Genres.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QFileInfo>

#include <float.h>
#include <limits>

////////////////////////////////////////////////////////////
// Helper Classes
////////////////////////////////////////////////////////////

class CueSheetItem
{
public:
	virtual const char* type(void) = 0;
};

class CueSheetTrack : public CueSheetItem
{
public:
	CueSheetTrack(CueSheetFile *parent, int trackNo)
	:
		m_parent(parent),
		m_trackNo(trackNo)
	{
		m_startIndex = std::numeric_limits<double>::quiet_NaN();
	}
	int trackNo(void) { return m_trackNo; }
	double startIndex(void) { return m_startIndex; }
	QString title(void) { return m_title; }
	QString performer(void) { return m_performer; }
	CueSheetFile *parent(void) { return m_parent; }
	void setStartIndex(double startIndex) { m_startIndex = startIndex; }
	void setTitle(const QString &title) { m_title = title; }
	void setPerformer(const QString &performer) { m_performer = performer; }
	bool isValid(void) { return !(_isnan(m_startIndex) || (m_trackNo < 0)); }
	virtual const char* type(void) { return "CueSheetTrack"; }
private:
	int m_trackNo;
	double m_startIndex;
	QString m_title;
	QString m_performer;
	CueSheetFile *m_parent;
};

class CueSheetFile : public CueSheetItem
{
public:
	CueSheetFile(const QString &fileName) : m_fileName(fileName) {}
	~CueSheetFile(void) { while(!m_tracks.isEmpty()) delete m_tracks.takeLast(); }
	QString fileName(void) { return m_fileName; }
	void addTrack(CueSheetTrack *track) { m_tracks.append(track); }
	void clearTracks(void) { while(!m_tracks.isEmpty()) delete m_tracks.takeLast(); }
	CueSheetTrack *track(int index) { return m_tracks.at(index); }
	int trackCount(void) { return m_tracks.count(); }
	virtual const char* type(void) { return "CueSheetFile"; }
private:
	const QString m_fileName;
	QList<CueSheetTrack*> m_tracks;
};

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

CueSheetModel::CueSheetModel()
{
	int trackNo = 0;
	
	for(int i = 0; i < 5; i++)
	{
		CueSheetFile *currentFile = new CueSheetFile(QString().sprintf("File %02d.wav", i+1));
		for(int j = 0; j < 8; j++)
		{
			CueSheetTrack *currentTrack = new CueSheetTrack(currentFile, trackNo++);
			currentTrack->setTitle("ATWA (Air Trees Water Animals)");
			currentTrack->setPerformer("System of a Down");
			currentFile->addTrack(currentTrack);
		}
		m_files.append(currentFile);
	}
}

CueSheetModel::~CueSheetModel(void)
{
	while(!m_files.isEmpty()) delete m_files.takeLast();
}

////////////////////////////////////////////////////////////
// Model Functions
////////////////////////////////////////////////////////////

QModelIndex CueSheetModel::index(int row, int column, const QModelIndex &parent) const
{
	if(!parent.isValid())
	{
		return createIndex(row, column, m_files.at(row));
	}

	CueSheetItem *parentItem = static_cast<CueSheetItem*>(parent.internalPointer());
	if(CueSheetFile *filePtr = dynamic_cast<CueSheetFile*>(parentItem))
	{
		return createIndex(row, column, filePtr->track(row));
	}

	return QModelIndex();
}

int CueSheetModel::columnCount(const QModelIndex &parent) const
{
	return 3;
}

int CueSheetModel::rowCount(const QModelIndex &parent) const
{
	if(!parent.isValid())
	{
		return m_files.count();
	}

	CueSheetItem *parentItem = static_cast<CueSheetItem*>(parent.internalPointer());
	if(CueSheetFile *filePtr = dynamic_cast<CueSheetFile*>(parentItem))
	{
		return filePtr->trackCount();
	}

	return 0;
}

QModelIndex CueSheetModel::parent(const QModelIndex &child) const
{
	if(child.isValid())
	{
		CueSheetItem *childItem = static_cast<CueSheetItem*>(child.internalPointer());
		if(CueSheetTrack *trackPtr = dynamic_cast<CueSheetTrack*>(childItem))
		{
			return createIndex(m_files.indexOf(trackPtr->parent()), 0, trackPtr->parent());
		}
	}

	return QModelIndex();
}

QVariant CueSheetModel::headerData (int section, Qt::Orientation orientation, int role) const
{
	if(role == Qt::DisplayRole)
	{
		switch(section)
		{
		case 0:
			return tr("No.");
			break;
		case 1:
			return tr("File / Track");
			break;
		case 2:
			return tr("Index");
			break;
		default:
			return QVariant();
			break;
		}
	}
	else
	{
		return QVariant();
	}
}

QVariant CueSheetModel::data(const QModelIndex &index, int role) const
{
	if(role == Qt::DisplayRole)
	{
		CueSheetItem *item = reinterpret_cast<CueSheetItem*>(index.internalPointer());

		if(CueSheetFile *filePtr = dynamic_cast<CueSheetFile*>(item))
		{
			switch(index.column())
			{
			case 0:
				return tr("File %1").arg(QString().sprintf("%02d", index.row() + 1)).append(" ");
				break;
			case 1:
				return QString("[%1]").arg(QFileInfo(filePtr->fileName()).fileName());
				break;
			default:
				return QVariant();
				break;
			}
		}
		else if(CueSheetTrack *trackPtr = dynamic_cast<CueSheetTrack*>(item))
		{
			switch(index.column())
			{
			case 0:
				return tr("Track %1").arg(QString().sprintf("%02d", trackPtr->trackNo() + 1)).append(" ");
				break;
			case 1:
				if(!trackPtr->title().isEmpty() && !trackPtr->performer().isEmpty())
				{
					return QString("%1 / %2").arg(trackPtr->performer(), trackPtr->title());
				}
				else if(!trackPtr->title().isEmpty())
				{
					return trackPtr->title();
				}
				else if(!trackPtr->performer().isEmpty())
				{
					return trackPtr->performer();
				}
				return QVariant();
				break;
			case 2:
				return QString().sprintf("%07.2f", trackPtr->startIndex());
				break;
			default:
				return QVariant();
				break;
			}
		}
	}

	return QVariant();
}

void CueSheetModel::clearData(void)
{
	beginResetModel();
	while(!m_files.isEmpty()) delete m_files.takeLast();
	endResetModel();
}
