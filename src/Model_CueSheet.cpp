///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2018 LoRd_MuldeR <MuldeR2@GMX.de>
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

//Internal
#include "Global.h"
#include "Model_CueSheet.h"
#include "Model_AudioFile.h"
#include "Genres.h"

//MUtils
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>

//Qt
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QTime>
#include <QTextCodec>
#include <QTextStream>
#include <QMutexLocker>

//CRT
#include <float.h>
#include <limits>

#define UNQUOTE(STR) STR.split("\"",  QString::SkipEmptyParts).first().trimmed()

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
		m_parent(parent)
	{
		m_startIndex = std::numeric_limits<double>::quiet_NaN();
		m_duration = std::numeric_limits<double>::infinity();
		m_metaInfo.setPosition(trackNo);
	}
	
	//Getter
	CueSheetFile *parent(void) { return m_parent; }
	double startIndex(void) { return m_startIndex; }
	double duration(void) { return m_duration; }
	AudioFileModel_MetaInfo &metaInfo(void) { return m_metaInfo; }

	//Setter
	void setStartIndex(double startIndex) { m_startIndex = startIndex; }
	void setDuration(double duration) { m_duration = duration; }
	
	//Misc
	virtual bool isValid(void) { return !(_isnan(m_startIndex) || (m_metaInfo.position() == 0)); }
	virtual const char* type(void) { return "CueSheetTrack"; }

private:
	double m_startIndex;
	double m_duration;
	AudioFileModel_MetaInfo m_metaInfo;
	CueSheetFile *const m_parent;
};

class CueSheetFile : public CueSheetItem
{
public:
	CueSheetFile(const QString &fileName) : m_fileName(fileName) {}
	~CueSheetFile(void) { while(!m_tracks.isEmpty()) delete m_tracks.takeLast(); }
	
	//Getter
	QString fileName(void) { return m_fileName; }
	CueSheetTrack *track(int index) { return m_tracks.at(index); }
	int trackCount(void) { return m_tracks.count(); }

	//Modifier
	void addTrack(CueSheetTrack *track) { m_tracks.append(track); }
	void clearTracks(void) { while(!m_tracks.isEmpty()) delete m_tracks.takeLast(); }

	//Misc
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
	/*nothing to do*/
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
			const AudioFileModel_MetaInfo &trackInfo = trackPtr->metaInfo();
			switch(index.column())
			{
			case 0:
				return tr("Track %1").arg(QString().sprintf("%02d", trackInfo.position())).append(" ");
				break;
			case 1:
				if(!trackInfo.title().isEmpty() && !trackInfo.artist().isEmpty())
				{
					return QString("%1 - %2").arg(trackInfo.artist(), trackInfo.title());
				}
				else if(!trackInfo.title().isEmpty())
				{
					return QString("%1 - %2").arg(tr("Unknown Artist"), trackInfo.title());
				}
				else if(!trackInfo.artist().isEmpty())
				{
					return QString("%1 - %2").arg(trackInfo.artist(), tr("Unknown Title"));
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

const AudioFileModel_MetaInfo *CueSheetModel::getTrackInfo(int fileIndex, int trackIndex)
{
	QMutexLocker lock(&m_mutex);
	
	if(fileIndex >= 0 && fileIndex < m_files.count())
	{
		CueSheetFile *currentFile = m_files.at(fileIndex);
		if(trackIndex >= 0 && trackIndex < currentFile->trackCount())
		{
			return &currentFile->track(trackIndex)->metaInfo();
		}
	}

	return NULL;
}

bool CueSheetModel::getTrackIndex(int fileIndex, int trackIndex, double *startIndex, double *duration)
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
			return true;
		}
	}

	return false;
}

const AudioFileModel_MetaInfo *CueSheetModel::getAlbumInfo(void)
{
	QMutexLocker lock(&m_mutex);
	return &m_albumInfo;
}

////////////////////////////////////////////////////////////
// Cue Sheet Parser
////////////////////////////////////////////////////////////

int CueSheetModel::loadCueSheet(const QString &cueFileName, QCoreApplication *application, QTextCodec *forceCodec)
{
	QMutexLocker lock(&m_mutex);
	const QTextCodec *codec = (forceCodec != NULL) ? forceCodec : QTextCodec::codecForName("System");
	
	QFile cueFile(cueFileName);
	if(!cueFile.open(QIODevice::ReadOnly))
	{
		return ErrorIOFailure;
	}

	clearData();

	beginResetModel();
	int iResult = parseCueFile(cueFile, QDir(QFileInfo(cueFile).canonicalPath()), application, codec);
	endResetModel();

	return iResult;
}

int CueSheetModel::parseCueFile(QFile &cueFile, const QDir &baseDir, QCoreApplication *application, const QTextCodec *codec)
{
	cueFile.reset();
	qDebug("\n[Cue Sheet Import]");
	bool bForceLatin1 = false;

	//Reject very large files, as parsing might take until forever
	if(cueFile.size() >= 10485760i64)
	{
		qWarning("File is very big. Probably not a Cue Sheet. Rejecting...");
		return 2;
	}

	//Test selected Codepage for decoding errors
	qDebug("Character encoding is: %s.", codec->name().constData());
	const QString replacementSymbol = QString(QChar(QChar::ReplacementCharacter));
	QByteArray testData = cueFile.peek(1048576);
	if((!testData.isEmpty()) && codec->toUnicode(testData.constData(), testData.size()).contains(replacementSymbol))
	{
		qWarning("Decoding error using selected codepage (%s). Enforcing Latin-1.", codec->name().constData());
		bForceLatin1 = true;
	}
	testData.clear();

	//Init text stream
	QTextStream cueStream(&cueFile);
	cueStream.setAutoDetectUnicode(false);
	cueStream.setCodec(bForceLatin1 ? "latin1" : codec->name());
	cueStream.seek(0i64);

	//Create regular expressions
	QRegExp rxFile("^FILE\\s+(\"[^\"]+\"|\\S+)\\s+(\\w+)$", Qt::CaseInsensitive);
	QRegExp rxTrack("^TRACK\\s+(\\d+)\\s(\\w+)$", Qt::CaseInsensitive);
	QRegExp rxIndex("^INDEX\\s+(\\d+)\\s+([0-9:]+)$", Qt::CaseInsensitive);
	QRegExp rxTitle("^TITLE\\s+(\"[^\"]+\"|\\S+)$", Qt::CaseInsensitive);
	QRegExp rxPerformer("^PERFORMER\\s+(\"[^\"]+\"|\\S+)$", Qt::CaseInsensitive);
	QRegExp rxGenre("^REM\\s+GENRE\\s+(\"[^\"]+\"|\\S+)$", Qt::CaseInsensitive);
	QRegExp rxYear("^REM\\s+DATE\\s+(\\d+)$", Qt::CaseInsensitive);
	
	bool bPreamble = true;
	bool bUnsupportedTrack = false;

	CueSheetFile *currentFile = NULL;
	CueSheetTrack *currentTrack = NULL;

	m_albumInfo.reset();

	//Loop over the Cue Sheet until all lines were processed
	for(int lines = 0; lines < INT_MAX; lines++)
	{
		if(application)
		{
			application->processEvents();
			if(lines < 128) MUtils::OS::sleep_ms(10);
		}
		
		if(cueStream.atEnd())
		{
			qDebug("End of Cue Sheet file.");
			break;
		}

		QString line = cueStream.readLine().trimmed();
		
		/* --- FILE --- */
		if(rxFile.indexIn(line) >= 0)
		{
			qDebug("%03d File: <%s> <%s>", lines, MUTILS_UTF8(rxFile.cap(1)), MUTILS_UTF8(rxFile.cap(2)));
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
						MUTILS_DELETE(currentTrack);
					}
				}
				if(currentFile->isValid())
				{
					m_files.append(currentFile);
					currentFile = NULL;
				}
				else
				{
					MUTILS_DELETE(currentFile);
				}
			}
			else
			{
				MUTILS_DELETE(currentTrack);
			}
			if(!rxFile.cap(2).compare("WAVE", Qt::CaseInsensitive) || !rxFile.cap(2).compare("MP3", Qt::CaseInsensitive) || !rxFile.cap(2).compare("AIFF", Qt::CaseInsensitive))
			{
				currentFile = new CueSheetFile(baseDir.absoluteFilePath(UNQUOTE(rxFile.cap(1))));
				qDebug("%03d File path: <%s>", lines, currentFile->fileName().toUtf8().constData());
			}
			else
			{
				bUnsupportedTrack = true;
				qWarning("%03d Skipping unsupported file of type '%s'.", lines, MUTILS_UTF8(rxFile.cap(2)));
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
				qDebug("%03d   Track: <%s> <%s>", lines, MUTILS_UTF8(rxTrack.cap(1)), MUTILS_UTF8(rxTrack.cap(2)));
				if(currentTrack)
				{
					if(currentTrack->isValid())
					{
						currentFile->addTrack(currentTrack);
						currentTrack = NULL;
					}
					else
					{
						MUTILS_DELETE(currentTrack);
					}
				}
				if(!rxTrack.cap(2).compare("AUDIO", Qt::CaseInsensitive))
				{
					currentTrack = new CueSheetTrack(currentFile, rxTrack.cap(1).toInt());
				}
				else
				{
					bUnsupportedTrack = true;
					qWarning("%03d   Skipping unsupported track of type '%s'.", lines, MUTILS_UTF8(rxTrack.cap(2)));
					currentTrack = NULL;
				}
			}
			else
			{
				MUTILS_DELETE(currentTrack);
			}
			bPreamble = false;
			continue;
		}
		
		/* --- INDEX --- */
		if(rxIndex.indexIn(line) >= 0)
		{
			if(currentFile && currentTrack)
			{
				qDebug("%03d     Index: <%s> <%s>", lines, MUTILS_UTF8(rxIndex.cap(1)), MUTILS_UTF8(rxIndex.cap(2)));
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
				m_albumInfo.setAlbum(UNQUOTE(rxTitle.cap(1)).simplified());
			}
			else if(currentFile && currentTrack)
			{
				qDebug("%03d     Title: <%s>", lines, MUTILS_UTF8(rxTitle.cap(1)));
				currentTrack->metaInfo().setTitle(UNQUOTE(rxTitle.cap(1)).simplified());
			}
			continue;
		}

		/* --- PERFORMER --- */
		if(rxPerformer.indexIn(line) >= 0)
		{
			if(bPreamble)
			{
				m_albumInfo.setArtist(UNQUOTE(rxPerformer.cap(1)).simplified());
			}
			else if(currentFile && currentTrack)
			{
				qDebug("%03d     Title: <%s>", lines, MUTILS_UTF8(rxPerformer.cap(1)));
				currentTrack->metaInfo().setArtist(UNQUOTE(rxPerformer.cap(1)).simplified());
			}
			continue;
		}

		/* --- GENRE --- */
		if(rxGenre.indexIn(line) >= 0)
		{
			if(bPreamble)
			{
				QString temp = UNQUOTE(rxGenre.cap(1)).simplified();
				for(int i = 0; g_lamexp_generes[i]; i++)
				{
					if(temp.compare(g_lamexp_generes[i], Qt::CaseInsensitive) == 0)
					{
						m_albumInfo.setGenre(QString(g_lamexp_generes[i]));
						break;
					}
				}
			}
			else if(currentFile && currentTrack)
			{
				qDebug("%03d     Genre: <%s>", lines, MUTILS_UTF8(rxGenre.cap(1)));
				QString temp = UNQUOTE(rxGenre.cap(1).simplified());
				for(int i = 0; g_lamexp_generes[i]; i++)
				{
					if(temp.compare(g_lamexp_generes[i], Qt::CaseInsensitive) == 0)
					{
						currentTrack->metaInfo().setGenre(QString(g_lamexp_generes[i]));
						break;
					}
				}
			}
			continue;
		}

		/* --- YEAR --- */
		if(rxYear.indexIn(line) >= 0)
		{
			if(bPreamble)
			{
				bool ok = false;
				unsigned int temp = rxYear.cap(1).toUInt(&ok);
				if(ok) m_albumInfo.setYear(temp);
			}
			else if(currentFile && currentTrack)
			{
				qDebug("%03d     Year: <%s>", lines, MUTILS_UTF8(rxPerformer.cap(1)));
				bool ok = false;
				unsigned int temp = rxYear.cap(1).toUInt(&ok);
				if(ok) currentTrack->metaInfo().setYear(temp);
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
				MUTILS_DELETE(currentTrack);
			}
		}
		if(currentFile->isValid())
		{
			m_files.append(currentFile);
			currentFile = NULL;
		}
		else
		{
			MUTILS_DELETE(currentFile);
		}
	}

	//Finally calculate duration of each track
	int nFiles = m_files.count();
	for(int i = 0; i < nFiles; i++)
	{
		if(application)
		{
			application->processEvents();
			MUtils::OS::sleep_ms(10);
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
				previousTrack->setDuration(qMax(0.0, duration));
			}
		}
	}
	
	//Sanity check of track numbers
	if(nFiles > 0)
	{
		bool hasTracks = false;
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
				MUtils::OS::sleep_ms(10);
			}
			CueSheetFile *currentFile = m_files.at(i);
			int nTracks = currentFile->trackCount();
			if(nTracks > 1)
			{
				for(int j = 0; j < nTracks; j++)
				{
					int currentTrackNo = currentFile->track(j)->metaInfo().position();
					if(currentTrackNo > 99)
					{
						qWarning("Track #%02d is invalid (maximum is 99), Cue Sheet is inconsistent!", currentTrackNo);
						return ErrorInconsistent;
					}
					if(currentTrackNo <= previousTrackNo)
					{
						qWarning("Non-increasing track numbers (%02d -> %02d), Cue Sheet is inconsistent!", previousTrackNo, currentTrackNo);
						return ErrorInconsistent;
					}
					if(trackNo[currentTrackNo])
					{
						qWarning("Track #%02d exists multiple times, Cue Sheet is inconsistent!", currentTrackNo);
						return ErrorInconsistent;
					}
					trackNo[currentTrackNo] = true;
					previousTrackNo = currentTrackNo;
					hasTracks = true;
				}
			}
		}
		
		if(!hasTracks)
		{
			qWarning("Could not find at least one valid track in the Cue Sheet!");
			return ErrorInconsistent;
		}

		return ErrorSuccess;
	}
	else
	{
		qWarning("Could not find at least one valid input file in the Cue Sheet!");
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

	qWarning("    Bad time index: '%s'", MUTILS_UTF8(index));
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
