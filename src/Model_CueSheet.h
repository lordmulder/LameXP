///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2023 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU GENERAL PUBLIC LICENSE as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version; always including the non-optional
// LAMEXP GNU GENERAL PUBLIC LICENSE ADDENDUM. See "License.txt" file!
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
class QTextCodec;
class QFile;

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
	int getFileCount(void);
	QString getFileName(int fileIndex);
	int getTrackCount(int fileIndex);
	const AudioFileModel_MetaInfo *getTrackInfo(int fileIndex, int trackIndex);
	const AudioFileModel_MetaInfo *getAlbumInfo(void);
	bool getTrackIndex(int fileIndex, int trackIndex, double *startIndex, double *duration);

	//Cue Sheet functions
	int loadCueSheet(const QString &cueFile, QCoreApplication *application = NULL, QTextCodec *forceCodec= NULL);

private:
	int parseCueFile(QFile &cueFile, const QDir &baseDir, QCoreApplication *application, const QTextCodec *codec);
	double parseTimeIndex(const QString &index);
	QString indexToString(const double index) const;
	
	static QMutex m_mutex;

	QList<CueSheetFile*> m_files;
	AudioFileModel_MetaInfo m_albumInfo;

	const QIcon m_fileIcon;
	const QIcon m_trackIcon;
};
