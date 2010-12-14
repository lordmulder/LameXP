///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2010 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Model_MetaInfo.h"
#include "Genres.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QFileInfo>

#define MODEL_ROW_COUNT 12

#define CHECK1(STR) (STR.isEmpty() ? (m_offset ? "(Not Specified)" : "(Unknown)") : STR)
#define CHECK2(VAL) ((VAL > 0) ? QString::number(VAL) : (m_offset ? "(Not Specified)" : "(Unknown)"))
#define CHECK3(STR) (STR.isEmpty() ? Qt::darkGray : QVariant())
#define CHECK4(VAL) ((VAL == 0) ? Qt::darkGray : QVariant())

#define EXPAND(STR) QString(STR).leftJustified(96, ' ')

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

MetaInfoModel::MetaInfoModel(AudioFileModel *file, unsigned int offset)
{
	if(offset >= MODEL_ROW_COUNT)
	{
		throw "Offset is out of range!";
	}

	m_audioFile = file;
	m_offset = offset;
}

MetaInfoModel::~MetaInfoModel(void)
{
}

////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////

int MetaInfoModel::columnCount(const QModelIndex &parent) const
{
	return 2;
}

int MetaInfoModel::rowCount(const QModelIndex &parent) const
{
	return MODEL_ROW_COUNT - m_offset;
}

QVariant MetaInfoModel::data(const QModelIndex &index, int role) const
{
	if(role == Qt::DisplayRole)
	{
		switch(index.row() + m_offset)
		{
		case 0:
			return (!index.column()) ? "Full Path" : CHECK1(m_audioFile->filePath());
			break;
		case 1:
			return (!index.column()) ? "Format" : CHECK1(m_audioFile->formatAudioBaseInfo());
			break;
		case 2:
			return (!index.column()) ? "Container" : CHECK1(m_audioFile->formatContainerInfo());
			break;
		case 3:
			return (!index.column()) ? "Compression" : CHECK1(m_audioFile->formatAudioCompressInfo());
			break;
		case 4:
			return (!index.column()) ? "Duration" : CHECK1(m_audioFile->fileDurationInfo());
			break;
		case 5:
			return (!index.column()) ? "Title" : CHECK1(m_audioFile->fileName());
			break;
		case 6:
			return (!index.column()) ? "Artist" : CHECK1(m_audioFile->fileArtist());
			break;
		case 7:
			return (!index.column()) ? "Album" : CHECK1(m_audioFile->fileAlbum());
			break;
		case 8:
			return (!index.column()) ? "Genre" : CHECK1(m_audioFile->fileGenre());
			break;
		case 9:
			return (!index.column()) ? "Year" : CHECK2(m_audioFile->fileYear());
			break;
		case 10:
			return (!index.column()) ? "Position" : ((m_audioFile->filePosition() == UINT_MAX) ? "Generate from list position" : CHECK2(m_audioFile->filePosition()));
			break;
		case 11:
			return (!index.column()) ? "Comment" : CHECK1(m_audioFile->fileComment());
			break;
		default:
			return QVariant();
			break;
		}
	}
	else if(role == Qt::DecorationRole && index.column() == 0)
	{
		switch(index.row() + m_offset)
		{
		case 0:
			return QIcon(":/icons/folder_page.png");
			break;
		case 1:
			return QIcon(":/icons/sound.png");
			break;
		case 2:
			return QIcon(":/icons/package.png");
			break;
		case 3:
			return QIcon(":/icons/compress.png");
			break;
		case 4:
			return QIcon(":/icons/clock_play.png");
			break;
		case 5:
			return QIcon(":/icons/music.png");
			break;
		case 6:
			return QIcon(":/icons/user.png");
			break;
		case 7:
			return QIcon(":/icons/cd.png");
			break;
		case 8:
			return QIcon(":/icons/star.png");
			break;
		case 9:
			return QIcon(":/icons/date.png");
			break;
		case 10:
			return QIcon(":/icons/timeline_marker.png");
			break;
		case 11:
			return QIcon(":/icons/comment.png");
			break;
		default:
			return QVariant();
			break;
		}
	}
	else if(role == Qt::TextColorRole && index.column() == 1)
	{
		switch(index.row() + m_offset)
		{
		case 0:
			return CHECK3(m_audioFile->filePath());
			break;
		case 1:
			return CHECK3(m_audioFile->formatAudioBaseInfo());
			break;
		case 2:
			return CHECK3(m_audioFile->formatContainerInfo());
			break;
		case 3:
			return CHECK3(m_audioFile->formatAudioCompressInfo());
			break;
		case 4:
			return CHECK4(m_audioFile->fileDurationInfo());
			break;
		case 5:
			return CHECK3(m_audioFile->fileName());
			break;
		case 6:
			return CHECK3(m_audioFile->fileArtist());
			break;
		case 7:
			return CHECK3(m_audioFile->fileAlbum());
			break;
		case 8:
			return CHECK3(m_audioFile->fileGenre());
			break;
		case 9:
			return CHECK4(m_audioFile->fileYear());
			break;
		case 10:
			return CHECK4(m_audioFile->filePosition());
			break;
		case 11:
			return CHECK3(m_audioFile->fileComment());
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

QVariant MetaInfoModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role == Qt::DisplayRole)
	{
		if(orientation == Qt::Horizontal)
		{
			switch(section)
			{
			case 0:
				return QVariant("Property");
				break;
			case 1:
				return QVariant("Value");
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
	else
	{
		return QVariant();
	}
}

void MetaInfoModel::editItem(const QModelIndex &index, QWidget *parent)
{
	bool ok = false;
	int val = -1;
	QStringList generes("(Unspecified)");
	QString temp;
	
	switch(index.row() + m_offset)
	{
	case 5:
		temp = QInputDialog::getText(parent, "Edit Title", EXPAND("Please enter the title for this file:"), QLineEdit::Normal, m_audioFile->fileName(), &ok).simplified();
		if(ok)
		{
			if(temp.isEmpty())
			{
				QMessageBox::warning(parent, "Edit Title", "The title must not be empty. Generating title from file name!");
				temp = QFileInfo(m_audioFile->filePath()).completeBaseName().replace("_", " ").simplified();
				int index = temp.lastIndexOf(" - ");
				if(index >= 0) temp = temp.mid(index + 3).trimmed();
			}
			beginResetModel();
			m_audioFile->setFileName(temp.isEmpty() ? QString() : temp);
			endResetModel();
		}
		break;
	case 6:
		temp = QInputDialog::getText(parent, "Edit Artist", EXPAND("Please enter the artist for this file:"), QLineEdit::Normal, m_audioFile->fileArtist(), &ok).simplified();
		if(ok)
		{
			beginResetModel();
			m_audioFile->setFileArtist(temp.isEmpty() ? QString() : temp);
			endResetModel();
		}
		break;
	case 7:
		temp = QInputDialog::getText(parent, "Edit Album", EXPAND("Please enter the album for this file:"), QLineEdit::Normal, m_audioFile->fileAlbum(), &ok).simplified();
		if(ok)
		{
			beginResetModel();
			m_audioFile->setFileAlbum(temp.isEmpty() ? QString() : temp);
			endResetModel();
		}
		break;
	case 8:
		for(int i = 0; g_lamexp_generes[i]; i++) generes << g_lamexp_generes[i];
		temp = QInputDialog::getItem(parent, "Edit Genre", EXPAND("Please enter the genre for this file:"), generes, (m_audioFile->fileGenre().isEmpty() ? 1 : generes.indexOf(m_audioFile->fileGenre())), false, &ok);
		if(ok)
		{
			beginResetModel();
			m_audioFile->setFileGenre((temp.isEmpty() || !temp.compare(generes.at(0), Qt::CaseInsensitive)) ? QString() : temp);
			endResetModel();
		}
		break;
	case 9:
		val = QInputDialog::getInt(parent, "Edit Year", EXPAND("Please enter the year for this file:"), (m_audioFile->fileYear() ? m_audioFile->fileYear() : 1900), 0, 2100, 1, &ok);
		if(ok)
		{
			beginResetModel();
			m_audioFile->setFileYear(val);
			endResetModel();
		}
		break;
	case 10:
		if(!m_offset)
		{
			val = QInputDialog::getInt(parent, "Edit Position", EXPAND("Please enter the position (track no.) for this file:"), (m_audioFile->filePosition() ? m_audioFile->filePosition() : 1), 0, 99, 1, &ok);
			if(ok)
			{
				beginResetModel();
				m_audioFile->setFilePosition(val);
				endResetModel();
			}
		}
		else
		{
			QStringList options;
			options << "Unspecified (copy from source file)" << "Generate from list position";
			temp = QInputDialog::getItem(parent, "Edit Position", EXPAND("Please enter the position (track no.) for this file:"), options, ((m_audioFile->filePosition() == UINT_MAX) ? 1 : 0), false, &ok);
			if(ok)
			{
				beginResetModel();
				m_audioFile->setFilePosition((options.indexOf(temp) == 1) ? UINT_MAX : 0);
				endResetModel();
			}
		}
		break;
	case 11:
		temp = QInputDialog::getText(parent, "Edit Comment", EXPAND("Please enter the comment for this file:"), QLineEdit::Normal, (m_audioFile->fileComment().isEmpty() ? "Encoded with LameXP" : m_audioFile->fileComment()), &ok).simplified();
		if(ok)
		{
			beginResetModel();
			m_audioFile->setFileComment(temp.isEmpty() ? QString() : temp);
			endResetModel();
		}
		break;
	default:
		QMessageBox::warning(parent, "Not editable", "Sorry, this property of the source file cannot be edited!");
		break;
	}
}

void MetaInfoModel::clearData(void)
{
	beginResetModel();

	m_audioFile->setFilePath(QString());
	m_audioFile->setFileName(QString());
	m_audioFile->setFileArtist(QString());
	m_audioFile->setFileAlbum(QString());
	m_audioFile->setFileGenre(QString());
	m_audioFile->setFileComment("Encoded with LameXP");
	m_audioFile->setFileYear(0);
	m_audioFile->setFilePosition(UINT_MAX);
	m_audioFile->setFileDuration(0);
	m_audioFile->setFormatContainerType(QString());
	m_audioFile->setFormatContainerProfile(QString());
	m_audioFile->setFormatAudioType(QString());
	m_audioFile->setFormatAudioProfile(QString());
	m_audioFile->setFormatAudioVersion(QString());
	m_audioFile->setFormatAudioSamplerate(0);
	m_audioFile->setFormatAudioChannels(0);
	m_audioFile->setFormatAudioBitdepth(0);

	endResetModel();
}

Qt::ItemFlags MetaInfoModel::flags(const QModelIndex &index) const
{
	return QAbstractTableModel::flags(index);
}