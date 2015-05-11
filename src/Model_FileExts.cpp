///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2015 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Model_FileExts.h"

//Internal
#include "Global.h"
#include "Registry_Encoder.h"

//MUtils
#include <MUtils/Global.h>
#include <MUtils/Sound.h>

//Qt
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QTextCodec>
#include <QTextStream>
#include <QInputDialog>

static inline int LOG10(int x)
{
	int ret = 1;
	while(x >= 10)
	{
		ret++; x /= 10;
	}
	return ret;
}

static inline QString EXTENSION(const QString &string)
{
	QRegExp regExp("^\\*\\.([A-Za-z0-9]+)$");
	if(regExp.indexIn(string) >= 0)
	{
		return regExp.cap(1).trimmed().toLower();
	}
	return QString();
}

static inline bool VALIDATE(const QString &string)
{
	QRegExp regExp("^[A-Za-z0-9]+$");
	return (regExp.indexIn(string) >= 0);
}

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

FileExtsModel::FileExtsModel(QObject *const parent )
:
	QAbstractItemModel(parent),
	m_label_1(":/icons/tag_blue.png"),
	m_label_2(":/icons/tag_red.png")
{
	//m_fileExts.append("mp4");
	//m_replace.insert(m_fileExts.first(), "m4a");
}

FileExtsModel::~FileExtsModel(void)
{
}

////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////

int FileExtsModel::columnCount(const QModelIndex &parent) const
{
	return 2;
}

int FileExtsModel::rowCount(const QModelIndex &parent) const
{
	return m_fileExt.count();
}

QVariant FileExtsModel::data(const QModelIndex &index, int role) const
{
	if(((role == Qt::DisplayRole) || (role == Qt::ToolTipRole)) && (index.row() < m_fileExt.count()) && (index.row() >= 0))
	{
		switch(index.column())
		{
		case 0:
			return QString("*.%0").arg(m_fileExt.at(index.row()));
		case 1:
			return QString("*.%0").arg(m_replace.value(m_fileExt.at(index.row())));
		default:
			return QVariant();
		}		
	}
	else if((role == Qt::DecorationRole))
	{
		switch(index.column())
		{
		case 0:
			return m_label_1;
		case 1:
			return m_label_2;
		default:
			return QVariant();
		}
	}
	else
	{
		return QVariant();
	}
}

QVariant FileExtsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role == Qt::DisplayRole)
	{
		if(orientation == Qt::Horizontal)
		{
			switch(section)
			{
			case 0:
				return QVariant(tr("File Extension"));
			case 1:
				return QVariant(tr("Replace With"));
			default:
				return QVariant();
			}
		}
		else
		{
			return int2str(section + 1);
		}
	}
	else
	{
		return QVariant();
	}
}

QModelIndex FileExtsModel::index(int row, int column, const QModelIndex & parent) const
{
	return createIndex(row, column, qHash((qint64(row) << 32)| qint64(column)));
}

QModelIndex  FileExtsModel::parent(const QModelIndex & index) const
{
	return QModelIndex();
}

////////////////////////////////////////////////////////////
// Edit Functions
////////////////////////////////////////////////////////////

bool FileExtsModel::addOverwrite(QWidget *const parent)
{
	const QStringList allExts = EncoderRegistry::getOutputFileExtensions();
	QStringList extensions;
	for(QStringList::ConstIterator iter = allExts.constBegin(); iter != allExts.constEnd(); iter++)
	{
		if(!m_fileExt.contains((*iter), Qt::CaseInsensitive))
		{
			extensions << QString("*.%0").arg(*iter);
		}
	}
	if(extensions.isEmpty())
	{
		return false;
	}

	QInputDialog dialog(parent);
	dialog.setLabelText(tr("Select file extensions to overwrite:"));
	dialog.setInputMode(QInputDialog::TextInput);
	dialog.setTextEchoMode(QLineEdit::Normal);
	dialog.setComboBoxEditable(false);
	dialog.setComboBoxItems(extensions);
	
	if(dialog.exec() == 0)
	{
		return false;
	}

	const QString selectedExt = EXTENSION(dialog.textValue());
	if(selectedExt.isEmpty())
	{
		return false;
	}

	dialog.setComboBoxEditable(true);
	dialog.setComboBoxItems(QStringList());
	dialog.setLabelText(tr("Enter the new file extension:"));

	QString replacement;
	while(replacement.isEmpty())
	{
		dialog.setTextValue(QString("*.%0").arg(selectedExt));
		if(dialog.exec() == 0)
		{
			return false;
		}
		replacement = EXTENSION(dialog.textValue());
		if(!replacement.compare(selectedExt, Qt::CaseInsensitive))
		{
			replacement.clear();
		}
		if(replacement.isEmpty())
		{
			MUtils::Sound::beep(MUtils::Sound::BEEP_ERR);
		}
	}

	beginResetModel();
	m_fileExt.append(selectedExt);
	m_fileExt.sort();
	m_replace.insert(selectedExt, replacement);
	endResetModel();
	return true;
}

bool FileExtsModel::removeOverwrite(const QModelIndex &index)
{
	if((index.row() < m_fileExt.count()) && (index.row() >= 0))
	{
		beginResetModel();
		m_replace.remove(m_fileExt.at(index.row()));
		m_fileExt.removeAt(index.row());
		endResetModel();
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////
// Export and Import
////////////////////////////////////////////////////////////

QString FileExtsModel::exportItems(void) const
{
	QString exported;
	for(QStringList::ConstIterator iter = m_fileExt.constBegin(); iter != m_fileExt.constEnd(); iter++)
	{
		if(m_replace.contains(*iter))
		{
			if(!exported.isEmpty()) exported.append('|');
			exported.append(QString("%0>%1").arg(iter->trimmed(), m_replace.value(*iter).trimmed()));
		}
	}
	return exported;
}

void FileExtsModel::importItems(const QString &data)
{
	beginResetModel();
	m_fileExt.clear();
	m_replace.clear();

	const QStringList list = data.split('|', QString::SkipEmptyParts);
	for(QStringList::ConstIterator iter = list.constBegin(); iter != list.constEnd(); iter++)
	{
		const QStringList item = iter->trimmed().split('>');
		if(item.count() >= 2)
		{
			const QString fileExt = item.at(0).simplified().toLower();
			const QString replace = item.at(1).simplified().toLower();
			if(VALIDATE(fileExt) && VALIDATE(replace) && (!m_fileExt.contains(fileExt)))
			{
				m_fileExt.append(fileExt);
				m_replace.insert(fileExt, replace);
			}
		}
	}

	m_fileExt.sort();
	endResetModel();
}

////////////////////////////////////////////////////////////
// Apply Replacement
////////////////////////////////////////////////////////////

QString FileExtsModel::apply(const QString &originalExtension) const
{
	if((!m_replace.isEmpty()) && m_replace.contains(originalExtension.toLower()))
	{
		return m_replace.value(originalExtension);
	}
	return originalExtension;
}


////////////////////////////////////////////////////////////
// Private Functions
////////////////////////////////////////////////////////////

QString FileExtsModel::int2str(const int &value) const
{
	if(m_fileExt.count() < 10)
	{
		return QString().sprintf("%d", value);
	}
	else
	{
		const QString format = QString().sprintf("%%0%dd", LOG10(m_fileExt.count()));
		return QString().sprintf(format.toLatin1().constData(), value);
	}
}
