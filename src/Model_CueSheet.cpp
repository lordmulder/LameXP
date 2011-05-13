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

#include "Global.h"
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
				return tr("Track %1").arg(QString().sprintf("%02d", trackPtr->trackNo())).append(" ");
				break;
			case 1:
				if(!trackPtr->title().isEmpty() && !trackPtr->performer().isEmpty())
				{
					return QString("%1 - %2").arg(trackPtr->performer(), trackPtr->title());
				}
				else if(!trackPtr->title().isEmpty())
				{
					return QString("%1 - %2").arg(tr("Unknown Artist"), trackPtr->title());
				}
				else if(!trackPtr->performer().isEmpty())
				{
					return QString("%1 - %2").arg(trackPtr->performer(), tr("Unknown Title"));
				}
				else
				{
					return QString("%1 - %2").arg(tr("Unknown Artist"), tr("Unknown Title"));
				}
				break;
			case 2:
				return indexToString(trackPtr->startIndex());
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

////////////////////////////////////////////////////////////
// Cue Sheet Parser
////////////////////////////////////////////////////////////

int CueSheetModel::loadCueSheet(const QString &cueFileName)
{
	QFile cueFile(cueFileName);
	if(!cueFile.open(QIODevice::ReadOnly))
	{
		return 1;
	}

	clearData();

	beginResetModel();
	int iResult = parseCueFile(cueFile);
	endResetModel();

	return iResult;
}

int CueSheetModel::parseCueFile(QFile &cueFile)
{
	cueFile.seek(0);

	//Check for UTF-8 BOM to guess encoding
	bool bUTF8 = false;
	QByteArray bomCheck = cueFile.peek(3);
	if(bomCheck.size() == 3)
	{
		bUTF8 = ((bomCheck.at(0) == '\xef') && (bomCheck.at(1) == '\xbb') && (bomCheck.at(2) == '\xbf'));
		qDebug("Encoding is %s.", (bUTF8 ? "UTF-8" : "Local 8-Bit"));
	}

	QRegExp rxFile("^FILE\\s+\"([^\"]+)\"\\s+(\\w+)$", Qt::CaseInsensitive);
	QRegExp rxTrack("^TRACK\\s+(\\d+)\\s(\\w+)$", Qt::CaseInsensitive);
	QRegExp rxIndex("^INDEX\\s+(\\d+)\\s+([0-9:]+)$", Qt::CaseInsensitive);
	QRegExp rxTitle("^TITLE\\s+\"([^\"]+)\"$", Qt::CaseInsensitive);
	QRegExp rxPerformer("^PERFORMER\\s+\"([^\"]+)\"$", Qt::CaseInsensitive);
	
	CueSheetFile *currentFile = NULL;
	CueSheetTrack *currentTrack = NULL;
	
	bool bPreamble = true;
	bool bUnsupportedTrack = false;

	QString albumTitle;
	QString albumPerformer;

	//Loop over the Cue Sheet until all lines were processed
	while(true)
	{
		QByteArray lineData = cueFile.readLine();
		if(lineData.size() <= 0)
		{
			qDebug("End of Cue Sheet file.");
			break;
		}

		QString line = bUTF8 ? QString::fromUtf8(lineData.constData(), lineData.size()).trimmed() : QString::fromLocal8Bit(lineData.constData(), lineData.size()).trimmed();
		
		/* --- FILE --- */
		if(rxFile.indexIn(line) >= 0)
		{
			qDebug("File: <%s> <%s>", rxFile.cap(1).toUtf8().constData(), rxFile.cap(2).toUtf8().constData());
			if(currentFile)
			{
				if(currentTrack)
				{
					if(currentTrack->isValid())
					{
						if(currentTrack->title().isEmpty() && !albumTitle.isEmpty())
						{
							currentTrack->setTitle(albumTitle);
						}
						if(currentTrack->performer().isEmpty() && !albumPerformer.isEmpty())
						{
							currentTrack->setPerformer(albumPerformer);
						}
						currentFile->addTrack(currentTrack);
						currentTrack = NULL;
					}
					else
					{
						LAMEXP_DELETE(currentTrack);
					}
				}
				if(currentFile->trackCount() > 0)
				{
					m_files.append(currentFile);
					currentFile = NULL;
				}
				else
				{
					LAMEXP_DELETE(currentFile);
				}
			}
			if(!rxFile.cap(2).compare("WAVE", Qt::CaseInsensitive) || !rxFile.cap(2).compare("MP3", Qt::CaseInsensitive) || !rxFile.cap(2).compare("AIFF", Qt::CaseInsensitive))
			{
				currentFile = new CueSheetFile(rxFile.cap(1));
			}
			else
			{
				bUnsupportedTrack = true;
				qWarning("Skipping unsupported file of type '%s'.", rxFile.cap(2).toUtf8().constData());
				currentFile = NULL;
			}
			bPreamble = false;
			currentTrack = NULL;
			continue;
		}
		
		/* --- TRACK --- */
		if(rxTrack.indexIn(line) >= 0)
		{
			if(currentFile)
			{
				qDebug("  Track: <%s> <%s>", rxTrack.cap(1).toUtf8().constData(), rxTrack.cap(2).toUtf8().constData());
				if(currentTrack)
				{
					if(currentTrack->isValid())
					{
						if(currentTrack->title().isEmpty() && !albumTitle.isEmpty())
						{
							currentTrack->setTitle(albumTitle);
						}
						if(currentTrack->performer().isEmpty() && !albumPerformer.isEmpty())
						{
							currentTrack->setPerformer(albumPerformer);
						}
						currentFile->addTrack(currentTrack);
						currentTrack = NULL;
					}
					else
					{
						LAMEXP_DELETE(currentTrack);
					}
				}
				if(!rxTrack.cap(2).compare("AUDIO", Qt::CaseInsensitive))
				{
					currentTrack = new CueSheetTrack(currentFile, rxTrack.cap(1).toInt());
				}
				else
				{
					bUnsupportedTrack = true;
					qWarning("  Skipping unsupported track of type '%s'.", rxTrack.cap(2).toUtf8().constData());
					currentTrack = NULL;
				}
			}
			else
			{
				LAMEXP_DELETE(currentTrack);
			}
			bPreamble = false;
			continue;
		}
		
		/* --- INDEX --- */
		if(rxIndex.indexIn(line) >= 0)
		{
			if(currentFile && currentTrack)
			{
				qDebug("    Index: <%s> <%s>", rxIndex.cap(1).toUtf8().constData(), rxIndex.cap(2).toUtf8().constData());
				if(rxIndex.cap(1).toInt() == 1)
				{
					currentTrack->setStartIndex(parseTimeIndex(rxIndex.cap(2)));
				}
			}
			continue;
		}

		/* --- TITLE --- */
		if(rxTitle.indexIn(line) >= 0)
		{
			if(bPreamble)
			{
				albumTitle = rxTitle.cap(1);
			}
			else if(currentFile && currentTrack)
			{
				qDebug("    Title: <%s>", rxTitle.cap(1).toUtf8().constData());
				currentTrack->setTitle(rxTitle.cap(1));
			}
			continue;
		}

		/* --- PERFORMER --- */
		if(rxPerformer.indexIn(line) >= 0)
		{
			if(bPreamble)
			{
				albumPerformer = rxPerformer.cap(1);
			}
			else if(currentFile && currentTrack)
			{
				qDebug("    Title: <%s>", rxPerformer.cap(1).toUtf8().constData());
				currentTrack->setPerformer(rxPerformer.cap(1));
			}
			continue;
		}
	}

	//Finally append the very last track/file
	if(currentFile)
	{
		if(currentTrack)
		{
			if(currentTrack->isValid())
			{
				if(currentTrack->title().isEmpty() && !albumTitle.isEmpty())
				{
					currentTrack->setTitle(albumTitle);
				}
				if(currentTrack->performer().isEmpty() && !albumPerformer.isEmpty())
				{
					currentTrack->setPerformer(albumPerformer);
				}
				currentFile->addTrack(currentTrack);
				currentTrack = NULL;
			}
			else
			{
				LAMEXP_DELETE(currentTrack);
			}
		}
		if(currentFile->trackCount() > 0)
		{
			m_files.append(currentFile);
			currentFile = NULL;
		}
		else
		{
			LAMEXP_DELETE(currentFile);
		}
	}

	return (m_files.count() > 0) ? 0 : (bUnsupportedTrack ? 3 : 2);
}

double CueSheetModel::parseTimeIndex(const QString &index)
{
	QRegExp rxTimeIndex("\\s*(\\d+)\\s*:\\s*(\\d+)\\s*:\\s*(\\d+)\\s*");
	
	if(rxTimeIndex.indexIn(index) >= 0)
	{
		int min, sec, frm;
		bool minOK, secOK, frmOK;

		min = rxTimeIndex.cap(1).toInt(&minOK);
		sec = rxTimeIndex.cap(2).toInt(&secOK);
		frm = rxTimeIndex.cap(3).toInt(&frmOK);

		if(minOK && secOK && frmOK)
		{
			return static_cast<double>(60 * min) + static_cast<double>(sec) + ((1.0/75.0) * static_cast<double>(frm));
		}
	}

	qWarning("    Bad time index: '%s'", index.toUtf8().constData());
	return std::numeric_limits<double>::quiet_NaN();
}

QString CueSheetModel::indexToString(const double index) const
{
	int temp = static_cast<int>(index * 100.0);

	int msec = temp % 100;
	int secs = temp / 100;

	return QString().sprintf("%02d:%02d.%02d", (secs / 60), (secs % 60), msec);
}
