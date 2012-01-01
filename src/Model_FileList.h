///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2012 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include <QAbstractTableModel>
#include <QIcon>

class FileListModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	FileListModel(void);
	~FileListModel(void);

	//Model functions
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	//Edit functions
	bool removeFile(const QModelIndex &index);
	void clearFiles(void);
	bool moveFile(const QModelIndex &index, int delta);
	AudioFileModel getFile(const QModelIndex &index);
	bool setFile(const QModelIndex &index, const AudioFileModel &audioFile);
	AudioFileModel &operator[] (const QModelIndex &index);

public slots:
	void addFile(const QString &filePath);
	void addFile(const AudioFileModel &file);

private:
	QList<AudioFileModel> m_fileList;
	const QIcon m_fileIcon;
};
