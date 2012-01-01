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

#include "Model_FileList.h"

#include <QFileInfo>
#include <QDir>

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

FileListModel::FileListModel(void)
:
	m_fileIcon(":/icons/page_white_cd.png")
{
}

FileListModel::~FileListModel(void)
{
}

////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////

int FileListModel::columnCount(const QModelIndex &parent) const
{
	return 2;
}

int FileListModel::rowCount(const QModelIndex &parent) const
{
	return m_fileList.count();
}

QVariant FileListModel::data(const QModelIndex &index, int role) const
{
	if((role == Qt::DisplayRole || role == Qt::ToolTipRole) && index.row() < m_fileList.count() && index.row() >= 0)
	{
		switch(index.column())
		{
		case 0:
			return m_fileList.at(index.row()).fileName();
			break;
		case 1:
			return QDir::toNativeSeparators(m_fileList.at(index.row()).filePath());
			break;
		default:
			return QVariant();
			break;
		}		
	}
	else if(role == Qt::DecorationRole && index.column() == 0)
	{
		return m_fileIcon;
	}
	else
	{
		return QVariant();
	}
}

QVariant FileListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role == Qt::DisplayRole)
	{
		if(orientation == Qt::Horizontal)
		{
			switch(section)
			{
			case 0:
				return QVariant(tr("Title"));
				break;
			case 1:
				return QVariant(tr("Full Path"));
				break;
			default:
				return QVariant();
				break;
			}
		}
		else
		{
			if(m_fileList.count() < 10)
			{
				return QVariant(QString().sprintf("%d", section + 1));
			}
			else if(m_fileList.count() < 100)
			{
				return QVariant(QString().sprintf("%02d", section + 1));
			}
			else if(m_fileList.count() < 1000)
			{
				return QVariant(QString().sprintf("%03d", section + 1));
			}
			else
			{
				return QVariant(QString().sprintf("%04d", section + 1));
			}
		}
	}
	else
	{
		return QVariant();
	}
}

void FileListModel::addFile(const QString &filePath)
{
	QFileInfo fileInfo(filePath);

	for(int i = 0; i < m_fileList.count(); i++)
	{
		if(m_fileList.at(i).filePath().compare(fileInfo.canonicalFilePath(), Qt::CaseInsensitive) == 0)
		{
			return;
		}
	}
	
	beginResetModel();
	m_fileList.append(AudioFileModel(fileInfo.canonicalFilePath(), fileInfo.baseName()));
	endResetModel();
}

void FileListModel::addFile(const AudioFileModel &file)
{
	for(int i = 0; i < m_fileList.count(); i++)
	{
		if(m_fileList.at(i).filePath().compare(file.filePath(), Qt::CaseInsensitive) == 0)
		{
			return;
		}
	}
	
	beginResetModel();
	m_fileList.append(file);
	endResetModel();
}

bool FileListModel::removeFile(const QModelIndex &index)
{
	if(index.row() >= 0 && index.row() < m_fileList.count())
	{
		beginResetModel();
		m_fileList.removeAt(index.row());
		endResetModel();
		return true;
	}
	else
	{
		return false;
	}
}

void FileListModel::clearFiles(void)
{
	beginResetModel();
	m_fileList.clear();
	endResetModel();
}

bool FileListModel::moveFile(const QModelIndex &index, int delta)
{
	if(delta != 0 && index.row() >= 0 && index.row() < m_fileList.count() && index.row() + delta >= 0 && index.row() + delta < m_fileList.count())
	{
		beginResetModel();
		m_fileList.move(index.row(), index.row() + delta);
		endResetModel();
		return true;
	}
	else
	{
		return false;
	}
}

AudioFileModel FileListModel::getFile(const QModelIndex &index)
{
	if(index.row() >= 0 && index.row() < m_fileList.count())
	{
		return m_fileList.at(index.row());
	}
	else
	{
		return AudioFileModel();
	}
}

AudioFileModel &FileListModel::operator[] (const QModelIndex &index)
{
	return m_fileList[index.row()];
}

bool FileListModel::setFile(const QModelIndex &index, const AudioFileModel &audioFile)
{
	if(index.row() >= 0 && index.row() < m_fileList.count())
	{
		beginResetModel();
		m_fileList.replace(index.row(), audioFile);
		endResetModel();
		return true;
	}
	else
	{
		return false;
	}
}
