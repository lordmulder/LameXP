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

#pragma once

#include "Model_AudioFile.h"
#include <QAbstractItemModel>
#include <QIcon>

class CueSheetFile;
class QApplication;
class QDir;

class CueSheetModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	CueSheetModel();
	~CueSheetModel(void);

	//Error codes
	enum ErrorCode
	{
		ErrorSuccess = 0,
		ErrorIOFailure = 1,
		ErrorBadFile = 2,
		ErrorUnsupported = 3,
		ErrorInconsistent = 4,
		ErrorUnknown = 9
	};
	
	//Model functions
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	QModelIndex parent(const QModelIndex &child) const;
	void clearData(void);

	//External API
	int CueSheetModel::getFileCount(void);
	QString getFileName(int fileIndex);
	int getTrackCount(int fileIndex);
	int getTrackNo(int fileIndex, int trackIndex);
	void getTrackIndex(int fileIndex, int trackIndex, double *startIndex, double *duration);
	QString getTrackPerformer(int fileIndex, int trackIndex);
	QString getTrackTitle(int fileIndex, int trackIndex);
	QString getTrackGenre(int fileIndex, int trackIndex);
	unsigned int getTrackYear(int fileIndex, int trackIndex);
	QString getAlbumPerformer(void);
	QString getAlbumTitle(void);
	QString getAlbumGenre(void);
	unsigned int getAlbumYear(void);

	//Cue Sheet functions
	int loadCueSheet(const QString &cueFile, QCoreApplication *application = NULL);

private:
	int parseCueFile(QFile &cueFile, const QDir &baseDir, QCoreApplication *application);
	double parseTimeIndex(const QString &index);
	QString indexToString(const double index) const;
	
	static QMutex m_mutex;

	QList<CueSheetFile*> m_files;
	QString m_albumTitle;
	QString m_albumPerformer;
	QString m_albumGenre;
	unsigned int m_albumYear;

	const QIcon m_fileIcon;
	const QIcon m_trackIcon;
};
