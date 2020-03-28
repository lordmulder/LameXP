///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2020 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include <QAbstractTableModel>
#include <QIcon>

class MetaInfoModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	MetaInfoModel(AudioFileModel *file);
	MetaInfoModel(AudioFileModel_MetaInfo *metaInfo);
	~MetaInfoModel(void);

	//Model functions
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	bool setData (const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	Qt::ItemFlags flags(const QModelIndex &index) const;
	void editItem(const QModelIndex &index, QWidget *parent);
	void editArtwork(const QString &imagePath);
	void assignInfoFrom(const AudioFileModel &file);
	void clearData(bool clearMetaOnly = false);

private:
	const unsigned int m_offset;

	AudioFileModel *const m_fullInfo;
	AudioFileModel_MetaInfo *const m_metaInfo;

	QString m_textNotSpecified;
	QString m_textUnknown;
};
