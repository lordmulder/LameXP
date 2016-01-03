///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2016 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Model_MetaInfo.h"
#include "Genres.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QFileInfo>
#include <QDir>

#define MODEL_ROW_COUNT 12

#define CHECK1(STR) (STR.isEmpty() ? (m_offset ? m_textNotSpecified : m_textUnknown) : STR)
#define CHECK2(VAL) ((VAL > 0) ? QString::number(VAL) : (m_offset ? m_textNotSpecified : m_textUnknown))
#define CHECK3(STR) (STR.isEmpty() ? Qt::darkGray : QVariant())
#define CHECK4(VAL) ((VAL == 0) ? Qt::darkGray : QVariant())

#define EXPAND(STR) QString(STR).leftJustified(96, ' ')

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

MetaInfoModel::MetaInfoModel(AudioFileModel *file)
:
	m_fullInfo(file),
	m_metaInfo(&file->metaInfo()),
	m_offset(0)
{
	m_textUnknown = QString("(%1)").arg(tr("Unknown"));
	m_textNotSpecified = QString("(%1)").arg(tr("Not Specified"));
}

MetaInfoModel::MetaInfoModel(AudioFileModel_MetaInfo *metaInfo)
:
	m_fullInfo(NULL),
	m_metaInfo(metaInfo),
	m_offset(6)
{
	m_textUnknown = QString("(%1)").arg(tr("Unknown"));
	m_textNotSpecified = QString("(%1)").arg(tr("Not Specified"));
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
			return (!index.column()) ? tr("Full Path") : CHECK1(QDir::toNativeSeparators(m_fullInfo->filePath()));
			break;
		case 1:
			return (!index.column()) ? tr("Format") : CHECK1(m_fullInfo->audioBaseInfo());
			break;
		case 2:
			return (!index.column()) ? tr("Container") : CHECK1(m_fullInfo->containerInfo());
			break;
		case 3:
			return (!index.column()) ? tr("Compression") : CHECK1(m_fullInfo->audioCompressInfo());
			break;
		case 4:
			return (!index.column()) ? tr("Duration") : CHECK1(m_fullInfo->durationInfo());
			break;
		case 5:
			return (!index.column()) ? tr("Title") : CHECK1(m_metaInfo->title());
			break;
		case 6:
			return (!index.column()) ? tr("Artist") : CHECK1(m_metaInfo->artist());
			break;
		case 7:
			return (!index.column()) ? tr("Album") : CHECK1(m_metaInfo->album());
			break;
		case 8:
			return (!index.column()) ? tr("Genre") : CHECK1(m_metaInfo->genre());
			break;
		case 9:
			return (!index.column()) ? tr("Year") : CHECK2(m_metaInfo->year());
			break;
		case 10:
			return (!index.column()) ? tr("Position") : ((m_metaInfo->position() == UINT_MAX) ? tr("Generate from list position") : CHECK2(m_metaInfo->position()));
			break;
		case 11:
			return (!index.column()) ? tr("Comment") : CHECK1(m_metaInfo->comment());
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
			return CHECK3(m_fullInfo->filePath());
			break;
		case 1:
			return CHECK3(m_fullInfo->audioBaseInfo());
			break;
		case 2:
			return CHECK3(m_fullInfo->containerInfo());
			break;
		case 3:
			return CHECK3(m_fullInfo->audioCompressInfo());
			break;
		case 4:
			return CHECK4(m_fullInfo->durationInfo());
			break;
		case 5:
			return CHECK3(m_metaInfo->title());
			break;
		case 6:
			return CHECK3(m_metaInfo->artist());
			break;
		case 7:
			return CHECK3(m_metaInfo->album());
			break;
		case 8:
			return CHECK3(m_metaInfo->genre());
			break;
		case 9:
			return CHECK4(m_metaInfo->year());
			break;
		case 10:
			return CHECK4(m_metaInfo->position());
			break;
		case 11:
			return CHECK3(m_metaInfo->comment());
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
				return QVariant(tr("Property"));
				break;
			case 1:
				return QVariant(tr("Value"));
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

bool MetaInfoModel::setData (const QModelIndex &index, const QVariant &value, int role)
{
	if((role != Qt::EditRole) || (index.column() != 1) || !value.isValid())
	{
		return false;
	}

	switch(index.row() + m_offset)
	{
	case 0:
		m_fullInfo->setFilePath(value.toString());
		break;
	case 1:
	case 2:
	case 3:
		return false;
		break;
	case 4:
		m_fullInfo->techInfo().setDuration(value.toUInt());
		break;
	case 5:
		m_metaInfo->setTitle(value.toString());
		break;
	case 6:
		m_metaInfo->setArtist(value.toString());
		break;
	case 7:
		m_metaInfo->setAlbum(value.toString());
		break;
	case 8:
		m_metaInfo->setGenre(value.toString());
		break;
	case 9:
		m_metaInfo->setYear(value.toUInt());
		break;
	case 10:
		m_metaInfo->setPosition(value.toUInt());
		break;
	case 11:
		m_metaInfo->setComment(value.toString());
		break;
	default:
		return false;
		break;
	}

	emit dataChanged(index, index);
	return true;
}

void MetaInfoModel::editItem(const QModelIndex &index, QWidget *parent)
{
	bool ok = false;
	int val = -1;
	QStringList generes(QString("(%1)").arg(tr("Unspecified")));
	QString temp;

	QInputDialog input(parent);
	input.setOkButtonText(tr("OK"));
	input.setCancelButtonText(tr("Cancel"));
	input.setTextEchoMode(QLineEdit::Normal);

	switch(index.row() + m_offset)
	{
	case 5:
		input.setWindowTitle(tr("Edit Title"));
		input.setLabelText(EXPAND(tr("Please enter the title for this file:")));
		input.setTextValue(m_metaInfo->title());
		if(input.exec() != 0)
		{
			temp = input.textValue().simplified();
			if(temp.isEmpty())
			{
				QMessageBox::warning(parent, tr("Edit Title"), tr("The title must not be empty. Generating title from file name!"));
				temp = QFileInfo(m_fullInfo->filePath()).completeBaseName().replace("_", " ").simplified();
				int index = temp.lastIndexOf(" - ");
				if(index >= 0) temp = temp.mid(index + 3).trimmed();
			}
			beginResetModel();
			m_metaInfo->setTitle(temp.isEmpty() ? QString() : temp);
			endResetModel();
		}
		break;
	case 6:
		input.setWindowTitle(tr("Edit Artist"));
		input.setLabelText(EXPAND(tr("Please enter the artist for this file:")));
		input.setTextValue(m_metaInfo->artist());
		if(input.exec() != 0)
		{
			temp = input.textValue().simplified();
			beginResetModel();
			m_metaInfo->setArtist(temp.isEmpty() ? QString() : temp);
			endResetModel();
		}
		break;
	case 7:
		input.setWindowTitle(tr("Edit Album"));
		input.setLabelText(EXPAND(tr("Please enter the album for this file:")));
		input.setTextValue(m_metaInfo->album());
		if(input.exec() != 0)
		{
			temp = input.textValue().simplified();
			beginResetModel();
			m_metaInfo->setAlbum(temp.isEmpty() ? QString() : temp);
			endResetModel();
		}
		break;
	case 8:
		input.setWindowTitle(tr("Edit Genre"));
		input.setLabelText(EXPAND(tr("Please enter the genre for this file:")));
		for(int i = 0; g_lamexp_generes[i]; i++) generes << g_lamexp_generes[i];
		input.setComboBoxItems(generes);
		input.setTextValue(m_metaInfo->genre());
		if(input.exec() != 0)
		{
			temp = input.textValue().simplified();
			beginResetModel();
			m_metaInfo->setGenre((temp.isEmpty() || !temp.compare(generes.at(0), Qt::CaseInsensitive)) ? QString() : temp);
			endResetModel();
		}
		break;
	case 9:
		input.setWindowTitle(tr("Edit Year"));
		input.setLabelText(EXPAND(tr("Please enter the year for this file:")));
		input.setIntRange(0, 2100);
		input.setIntValue((m_metaInfo->year() ? m_metaInfo->year() : 1900));
		input.setIntStep(1);
		if(input.exec() != 0)
		{
			val = input.intValue();
			beginResetModel();
			m_metaInfo->setYear(val);
			endResetModel();
		}
		break;
	case 10:
		if(!m_offset)
		{
			input.setWindowTitle(tr("Edit Position"));
			input.setLabelText(EXPAND(tr("Please enter the position (track no.) for this file:")));
			input.setIntRange(0, 99);
			input.setIntValue((m_metaInfo->position() ? m_metaInfo->position() : 1));
			input.setIntStep(1);
			if(input.exec() != 0)
			{
				val = input.intValue();
				beginResetModel();
				m_metaInfo->setPosition(val);
				endResetModel();
			}
		}
		else
		{
			QStringList options;
			options << tr("Unspecified (copy from source file)") << tr("Generate from list position");
			input.setWindowTitle(tr("Edit Position"));
			input.setLabelText(EXPAND(tr("Please enter the position (track no.) for this file:")));
			input.setComboBoxItems(options);
			input.setTextValue(options.value((m_metaInfo->position() == UINT_MAX) ? 1 : 0));
			if(input.exec() != 0)
			{
				temp = input.textValue().simplified();
				beginResetModel();
				m_metaInfo->setPosition((options.indexOf(temp) == 1) ? UINT_MAX : 0);
				endResetModel();
			}
		}
		break;
	case 11:
		input.setWindowTitle(tr("Edit Comment"));
		input.setLabelText(EXPAND(tr("Please enter the comment for this file:")));
		input.setTextValue((m_metaInfo->comment().isEmpty() ? tr("Encoded with LameXP") : m_metaInfo->comment()));
		if(input.exec() != 0)
		{
			temp = input.textValue().simplified();
			beginResetModel();
			m_metaInfo->setComment(temp.isEmpty() ? QString() : temp);
			endResetModel();
		}
		break;
	default:
		QMessageBox::warning(parent, tr("Not editable"), tr("Sorry, this property of the source file cannot be edited!"));
		break;
	}
}

void MetaInfoModel::editArtwork(const QString &imagePath)
{
	m_metaInfo->setCover(imagePath, false);
}

void MetaInfoModel::clearData(bool clearMetaOnly)
{
	beginResetModel();

	m_textUnknown = QString("(%1)").arg(tr("Unknown"));
	m_textNotSpecified = QString("(%1)").arg(tr("Not Specified"));

	if((!clearMetaOnly) && m_fullInfo)
	{
		m_fullInfo->techInfo().reset();
	}

	if(m_metaInfo)
	{
		m_metaInfo->reset();
		m_metaInfo->setComment(tr("Encoded with LameXP"));
		m_metaInfo->setPosition(m_offset ? UINT_MAX : 0);
	}

	if(m_fullInfo)
	{
		QString temp = QFileInfo(m_fullInfo->filePath()).baseName();
		temp = temp.split("-", QString::SkipEmptyParts).last().trimmed();
		m_metaInfo->setTitle(temp);
	}

	endResetModel();
}

Qt::ItemFlags MetaInfoModel::flags(const QModelIndex &index) const
{
	return QAbstractTableModel::flags(index);
}

void MetaInfoModel::assignInfoFrom(const AudioFileModel &file)
{
	beginResetModel();
	const unsigned int position = m_metaInfo->position();
	m_metaInfo->update(file.metaInfo(), true);
	if(m_offset)
	{
		m_metaInfo->setTitle(QString());
		m_metaInfo->setPosition(position ? UINT_MAX : 0);
		m_metaInfo->setCover(QString(), false);
	}
	endResetModel();
}
