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

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QTime>

#include <float.h>
#include <limits>

////////////////////////////////////////////////////////////
// Helper Classes
////////////////////////////////////////////////////////////

class CueSheetItem
{
public:
	virtual const char* type(void) = 0;
	virtual bool isValid(void) { return false; }
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
		m_duration = std::numeric_limits<double>::infinity();
	}
	int trackNo(void) { return m_trackNo; }
	double startIndex(void) { return m_startIndex; }
	double duration(void) { return m_duration; }
	QString title(void) { return m_title; }
	QString performer(void) { return m_performer; }
	CueSheetFile *parent(void) { return m_parent; }
	void setStartIndex(double startIndex) { m_startIndex = startIndex; }
	void setDuration(double duration) { m_duration = duration; }
	void setTitle(const QString &title, bool update = false) { if(!update || (m_title.isEmpty() && !title.isEmpty())) m_title = title; }
	void setPerformer(const QString &performer, bool update = false) { if(!update || (m_performer.isEmpty() && !performer.isEmpty())) m_performer = performer; }
	virtual bool isValid(void) { return !(_isnan(m_startIndex) || (m_trackNo < 0)); }
	virtual const char* type(void) { return "CueSheetTrack"; }
private:
	int m_trackNo;
	double m_startIndex;
	double m_duration;
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
	virtual bool isValid(void) { return m_tracks.count() > 0; }
	virtual const char* type(void) { return "CueSheetFile"; }
private:
	const QString m_fileName;
	QList<CueSheetTrack*> m_tracks;
};

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

QMutex CueSheetModel::m_mutex(QMutex::Recursive);

CueSheetModel::CueSheetModel()
:
	m_fileIcon(":/icons/music.png"),
	m_trackIcon(":/icons/control_play_blue.png")
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
	QMutexLocker lock(&m_mutex);
	
	if(!parent.isValid())
	{
		return (row < m_files.count()) ? createIndex(row, column, m_files.at(row)) : QModelIndex();
	}

	CueSheetItem *parentItem = static_cast<CueSheetItem*>(parent.internalPointer());
	if(CueSheetFile *filePtr = dynamic_cast<CueSheetFile*>(parentItem))
	{
		return (row < filePtr->trackCount()) ? createIndex(row, column, filePtr->track(row)) : QModelIndex();
	}

	return QModelIndex();
}

int CueSheetModel::columnCount(const QModelIndex &parent) const
{
	QMutexLocker lock(&m_mutex);
	return 4;
}

int CueSheetModel::rowCount(const QModelIndex &parent) const
{
	QMutexLocker lock(&m_mutex);

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
	QMutexLocker lock(&m_mutex);
	
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
	QMutexLocker lock(&m_mutex);
	
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
		case 3:
			return tr("Duration");
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
	QMutexLocker lock(&m_mutex);

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
				return QFileInfo(filePtr->fileName()).fileName();
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
			case 3:
				return indexToString(trackPtr->duration());
				break;
			default:
				return QVariant();
				break;
			}
		}
	}
	else if(role == Qt::ToolTipRole)
	{
		CueSheetItem *item = reinterpret_cast<CueSheetItem*>(index.internalPointer());

		if(CueSheetFile *filePtr = dynamic_cast<CueSheetFile*>(item))
		{
			return QDir::toNativeSeparators(filePtr->fileName());
		}
		else if(CueSheetTrack *trackPtr = dynamic_cast<CueSheetTrack*>(item))
		{
			return QDir::toNativeSeparators(trackPtr->parent()->fileName());
		}
	}
	else if(role == Qt::DecorationRole)
	{
		if(index.column() == 0)
		{
			CueSheetItem *item = reinterpret_cast<CueSheetItem*>(index.internalPointer());

			if(dynamic_cast<CueSheetFile*>(item))
			{
				return m_fileIcon;
			}
			else if(dynamic_cast<CueSheetTrack*>(item))
			{
				return m_trackIcon;
			}
		}
	}
	else if(role == Qt::FontRole)
	{
		QFont font("Monospace");
		font.setStyleHint(QFont::TypeWriter);
		if((index.column() == 1))
		{
			CueSheetItem *item = reinterpret_cast<CueSheetItem*>(index.internalPointer());
			font.setBold(dynamic_cast<CueSheetFile*>(item) != NULL);
		}
		return font;
	}
	else if(role == Qt::ForegroundRole)
	{
		if((index.column() == 1))
		{
			CueSheetItem *item = reinterpret_cast<CueSheetItem*>(index.internalPointer());
			if(CueSheetFile *filePtr = dynamic_cast<CueSheetFile*>(item))
			{
				return (QFileInfo(filePtr->fileName()).size() > 4) ? QColor("mediumblue") : QColor("darkred");
			}
		}
		else if((index.column() == 3))
		{
			CueSheetItem *item = reinterpret_cast<CueSheetItem*>(index.internalPointer());
			if(CueSheetTrack *trackPtr = dynamic_cast<CueSheetTrack*>(item))
			{
				if(trackPtr->duration() == std::numeric_limits<double>::infinity())
				{
					return QColor("dimgrey");
				}
			}
		}
	}

	return QVariant();
}

void CueSheetModel::clearData(void)
{
	QMutexLocker lock(&m_mutex);
	
	beginResetModel();
	while(!m_files.isEmpty()) delete m_files.takeLast();
	endResetModel();
}

////////////////////////////////////////////////////////////
// External API
////////////////////////////////////////////////////////////

int CueSheetModel::getFileCount(void)
{
	QMutexLocker lock(&m_mutex);
	return m_files.count();
}

QString CueSheetModel::getFileName(int fileIndex)
{
	QMutexLocker lock(&m_mutex);
	
	if(fileIndex < 0 || fileIndex >= m_files.count())
	{
		return QString();
	}

	return m_files.at(fileIndex)->fileName();
}

int CueSheetModel::getTrackCount(int fileIndex)
{
	QMutexLocker lock(&m_mutex);

	if(fileIndex < 0 || fileIndex >= m_files.count())
	{
		return -1;
	}

	return m_files.at(fileIndex)->trackCount();
}

int CueSheetModel::getTrackNo(int fileIndex, int trackIndex)
{
	QMutexLocker lock(&m_mutex);
	
	if(fileIndex >= 0 && fileIndex < m_files.count())
	{
		CueSheetFile *currentFile = m_files.at(fileIndex);
		if(trackIndex >= 0 && trackIndex < currentFile->trackCount())
		{
			return currentFile->track(trackIndex)->trackNo();
		}
	}

	return -1;
}

void CueSheetModel::getTrackIndex(int fileIndex, int trackIndex, double *startIndex, double *duration)
{
	QMutexLocker lock(&m_mutex);
	
	*startIndex = std::numeric_limits<double>::quiet_NaN();
	*duration = std::numeric_limits<double>::quiet_NaN();

	if(fileIndex >= 0 && fileIndex < m_files.count())
	{
		CueSheetFile *currentFile = m_files.at(fileIndex);
		if(trackIndex >= 0 && trackIndex < currentFile->trackCount())
		{
			CueSheetTrack *currentTrack = currentFile->track(trackIndex);
			*startIndex = currentTrack->startIndex();
			*duration = currentTrack->duration();
		}
	}
}

QString CueSheetModel::getTrackPerformer(int fileIndex, int trackIndex)
{	
	QMutexLocker lock(&m_mutex);
	
	if(fileIndex >= 0 && fileIndex < m_files.count())
	{
		CueSheetFile *currentFile = m_files.at(fileIndex);
		if(trackIndex >= 0 && trackIndex < currentFile->trackCount())
		{
			CueSheetTrack *currentTrack = currentFile->track(trackIndex);
			return currentTrack->performer();
		}
	}
	
	return QString();
}

QString CueSheetModel::getTrackTitle(int fileIndex, int trackIndex)
{
	QMutexLocker lock(&m_mutex);
	
	if(fileIndex >= 0 && fileIndex < m_files.count())
	{
		CueSheetFile *currentFile = m_files.at(fileIndex);
		if(trackIndex >= 0 && trackIndex < currentFile->trackCount())
		{
			CueSheetTrack *currentTrack = currentFile->track(trackIndex);
			return currentTrack->title();
		}
	}
	
	return QString();
}

QString CueSheetModel::getAlbumPerformer(void)
{
	QMutexLocker lock(&m_mutex);
	return m_albumPerformer;
}

QString CueSheetModel::getAlbumTitle(void)
{
	QMutexLocker lock(&m_mutex);
	return m_albumTitle;
}

////////////////////////////////////////////////////////////
// Cue Sheet Parser
////////////////////////////////////////////////////////////

int CueSheetModel::loadCueSheet(const QString &cueFileName, QCoreApplication *application)
{
	QMutexLocker lock(&m_mutex);
	
	QFile cueFile(cueFileName);
	if(!cueFile.open(QIODevice::ReadOnly))
	{
		return ErrorIOFailure;
	}

	clearData();

	beginResetModel();
	int iResult = parseCueFile(cueFile, QDir(QFileInfo(cueFile).canonicalPath()), application);
	endResetModel();

	return iResult;
}

int CueSheetModel::parseCueFile(QFile &cueFile, const QDir &baseDir, QCoreApplication *application)
{
	cueFile.seek(0);
	qDebug("\n[Cue Sheet Import]");

	//Reject very large files, as parsing might take until forever
	if(cueFile.size() >= 10485760i64)
	{
		qWarning("File is very big. Probably not a Cue Sheet. Rejecting...");
		return 2;
	}

	//Check for UTF-8 BOM in order to guess encoding
	QByteArray bomCheck = cueFile.peek(128);
	bool bUTF8 = bomCheck.contains("\xef\xbb\xbf");
	qDebug("Encoding is %s.", (bUTF8 ? "UTF-8" : "Local 8-Bit"));
	bomCheck.clear();

	QRegExp rxFile("^FILE\\s+\"([^\"]+)\"\\s+(\\w+)$", Qt::CaseInsensitive);
	QRegExp rxTrack("^TRACK\\s+(\\d+)\\s(\\w+)$", Qt::CaseInsensitive);
	QRegExp rxIndex("^INDEX\\s+(\\d+)\\s+([0-9:]+)$", Qt::CaseInsensitive);
	QRegExp rxTitle("^TITLE\\s+\"([^\"]+)\"$", Qt::CaseInsensitive);
	QRegExp rxPerformer("^PERFORMER\\s+\"([^\"]+)\"$", Qt::CaseInsensitive);
	
	bool bPreamble = true;
	bool bUnsupportedTrack = false;

	CueSheetFile *currentFile = NULL;
	CueSheetTrack *currentTrack = NULL;

	m_albumTitle.clear();
	m_albumPerformer.clear();

	//Loop over the Cue Sheet until all lines were processed
	for(int lines = 0; lines < INT_MAX; lines++)
	{
		if(application)
		{
			application->processEvents();
			if(lines < 128) Sleep(10);
		}
		
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
			qDebug("%03d File: <%s> <%s>", lines, rxFile.cap(1).toUtf8().constData(), rxFile.cap(2).toUtf8().constData());
			if(currentFile)
			{
				if(currentTrack)
				{
					if(currentTrack->isValid())
					{
						currentFile->addTrack(currentTrack);
						currentTrack = NULL;
					}
					else
					{
						LAMEXP_DELETE(currentTrack);
					}
				}
				if(currentFile->isValid())
				{
					m_files.append(currentFile);
					currentFile = NULL;
				}
				else
				{
					LAMEXP_DELETE(currentFile);
				}
			}
			else
			{
				LAMEXP_DELETE(currentTrack);
			}
			if(!rxFile.cap(2).compare("WAVE", Qt::CaseInsensitive) || !rxFile.cap(2).compare("MP3", Qt::CaseInsensitive) || !rxFile.cap(2).compare("AIFF", Qt::CaseInsensitive))
			{
				currentFile = new CueSheetFile(baseDir.absoluteFilePath(rxFile.cap(1)));
				qDebug("%03d File path: <%s>", lines, currentFile->fileName().toUtf8().constData());
			}
			else
			{
				bUnsupportedTrack = true;
				qWarning("%03d Skipping unsupported file of type '%s'.", lines, rxFile.cap(2).toUtf8().constData());
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
				qDebug("%03d   Track: <%s> <%s>", lines, rxTrack.cap(1).toUtf8().constData(), rxTrack.cap(2).toUtf8().constData());
				if(currentTrack)
				{
					if(currentTrack->isValid())
					{
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
					qWarning("%03d   Skipping unsupported track of type '%s'.", lines, rxTrack.cap(2).toUtf8().constData());
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
				qDebug("%03d     Index: <%s> <%s>", lines, rxIndex.cap(1).toUtf8().constData(), rxIndex.cap(2).toUtf8().constData());
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
				m_albumTitle = rxTitle.cap(1).simplified();
			}
			else if(currentFile && currentTrack)
			{
				qDebug("%03d     Title: <%s>", lines, rxTitle.cap(1).toUtf8().constData());
				currentTrack->setTitle(rxTitle.cap(1));
			}
			continue;
		}

		/* --- PERFORMER --- */
		if(rxPerformer.indexIn(line) >= 0)
		{
			if(bPreamble)
			{
				m_albumPerformer = rxPerformer.cap(1).simplified();
			}
			else if(currentFile && currentTrack)
			{
				qDebug("%03d     Title: <%s>", lines, rxPerformer.cap(1).toUtf8().constData());
				currentTrack->setPerformer(rxPerformer.cap(1));
			}
			continue;
		}
	}

	//Append the very last track/file that is still pending
	if(currentFile)
	{
		if(currentTrack)
		{
			if(currentTrack->isValid())
			{
				currentFile->addTrack(currentTrack);
				currentTrack = NULL;
			}
			else
			{
				LAMEXP_DELETE(currentTrack);
			}
		}
		if(currentFile->isValid())
		{
			m_files.append(currentFile);
			currentFile = NULL;
		}
		else
		{
			LAMEXP_DELETE(currentFile);
		}
	}

	//Finally calculate duration of each track
	int nFiles = m_files.count();
	for(int i = 0; i < nFiles; i++)
	{
		if(application)
		{
			application->processEvents();
			Sleep(10);
		}

		CueSheetFile *currentFile = m_files.at(i);
		int nTracks = currentFile->trackCount();
		if(nTracks > 1)
		{
			for(int j = 1; j < nTracks; j++)
			{
				CueSheetTrack *currentTrack = currentFile->track(j);
				CueSheetTrack *previousTrack = currentFile->track(j-1);
				double duration = currentTrack->startIndex() - previousTrack->startIndex();
				previousTrack->setDuration(max(0.0, duration));
			}
		}
	}
	
	//Sanity check of track numbers
	if(nFiles > 0)
	{
		int previousTrackNo = -1;
		bool trackNo[100];
		for(int i = 0; i < 100; i++)
		{
			trackNo[i] = false;
		}
		for(int i = 0; i < nFiles; i++)
		{
			if(application)
			{
				application->processEvents();
				Sleep(10);
			}
			CueSheetFile *currentFile = m_files.at(i);
			int nTracks = currentFile->trackCount();
			if(nTracks > 1)
			{
				for(int j = 0; j < nTracks; j++)
				{
					int currentTrackNo = currentFile->track(j)->trackNo();
					if(currentTrackNo > 99)
					{
						qWarning("Track #%02d is invalid (maximum is 99), Cue Sheet is inconsistent!", currentTrackNo);
						return ErrorInconsistent;
					}
					if(currentTrackNo <= previousTrackNo)
					{
						qWarning("Non-increasing track numbers, Cue Sheet is inconsistent!", currentTrackNo);
						return ErrorInconsistent;
					}
					if(trackNo[currentTrackNo])
					{
						qWarning("Track #%02d exists multiple times, Cue Sheet is inconsistent!", currentTrackNo);
						return ErrorInconsistent;
					}
					trackNo[currentTrackNo] = true;
					previousTrackNo = currentTrackNo;
				}
			}
		}
		return ErrorSuccess;
	}
	else
	{
		return bUnsupportedTrack ? ErrorUnsupported : ErrorBadFile;
	}
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
			return static_cast<double>(60 * min) + static_cast<double>(sec) + (static_cast<double>(frm) / 75.0);
		}
	}

	qWarning("    Bad time index: '%s'", index.toUtf8().constData());
	return std::numeric_limits<double>::quiet_NaN();
}

QString CueSheetModel::indexToString(const double index) const
{
	if(!_finite(index) || (index < 0.0) || (index > 86400.0))
	{
		return QString("??:??.???");
	}
	
	QTime time = QTime().addMSecs(static_cast<int>(floor(0.5 + (index * 1000.0))));

	if(time.minute() < 100)
	{
		return time.toString("mm:ss.zzz");
	}
	else
	{
		return QString("99:99.999");
	}
}
