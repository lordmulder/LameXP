///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2022 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include <QAbstractItemModel>
#include <QIcon>

class FileExtsModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	FileExtsModel(QObject *const parent = 0);
	~FileExtsModel(void);

	//Model functions
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex & index) const;

	//Edit functions
	bool addOverwrite(QWidget *const parent);
	bool removeOverwrite(const QModelIndex &index);

	//Export and Import
	QString exportItems(void) const;
	void importItems(const QString &data);

	//Replace extension
	QString apply(const QString &originalExtension) const;

signals:
	void rowAppended(void);

private:
	QString int2str(const int &value) const;

	QStringList             m_fileExt;
	QHash<QString, QString> m_replace;
	const QIcon             m_label_1;
	const QIcon             m_label_2;
};
