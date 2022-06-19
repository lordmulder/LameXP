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

#include "Model_FileList.h"

//Internal
#include "Global.h"

//MUtils
#include <MUtils/Global.h>

//Qt
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QTextCodec>
#include <QTextStream>
#include <QInputDialog>

#define EXPAND(STR) QString(STR).leftJustified(96, ' ')
#define CHECK_HDR(STR,NAM) (!(STR).compare((NAM), Qt::CaseInsensitive))
#define MAKE_KEY(PATH) (QDir::fromNativeSeparators(PATH).toLower())

static inline int LOG10(int x)
{
	int ret = 1;
	while(x >= 10)
	{
		ret++; x /= 10;
	}
	return ret;
}

////////////////////////////////////////////////////////////
// Constructor & Destructor
////////////////////////////////////////////////////////////

FileListModel::FileListModel(void)
:
	m_blockUpdates(false),
	m_fileIcon(":/icons/page_white_cd.png")
{
}

FileListModel::~FileListModel(void)
{
}

////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////

int FileListModel::columnCount(const QModelIndex& /*parent*/) const
{
	return 2;
}

int FileListModel::rowCount(const QModelIndex& /*parent*/) const
{
	return m_fileList.count();
}

QVariant FileListModel::data(const QModelIndex &index, int role) const
{
	if(((role == Qt::DisplayRole) || (role == Qt::ToolTipRole)) && (index.row() < m_fileList.count()) && (index.row() >= 0))
	{
		switch(index.column())
		{
		case 0:
			return m_fileStore.value(m_fileList.at(index.row())).metaInfo().title();
			break;
		case 1:
			return QDir::toNativeSeparators(m_fileStore.value(m_fileList.at(index.row())).filePath());
			break;
		default:
			return QVariant();
			break;
		}		
	}
	else if((role == Qt::DecorationRole) && (index.column() == 0))
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
			return int2str(section + 1);
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
	const QString key = MAKE_KEY(fileInfo.canonicalFilePath()); 
	const bool flag = (!m_blockUpdates);

	if(!m_fileStore.contains(key))
	{
		AudioFileModel audioFile(fileInfo.canonicalFilePath());
		audioFile.metaInfo().setTitle(fileInfo.baseName());
		if(flag) beginInsertRows(QModelIndex(), m_fileList.count(), m_fileList.count());
		m_fileStore.insert(key, audioFile);
		m_fileList.append(key);
		if(flag) endInsertRows();
		emit rowAppended();
	}
}

void FileListModel::addFile(const AudioFileModel &file)
{
	const QString key = MAKE_KEY(file.filePath()); 
	const bool flag = (!m_blockUpdates);

	if(!m_fileStore.contains(key))
	{
		if(flag) beginInsertRows(QModelIndex(), m_fileList.count(), m_fileList.count());
		m_fileStore.insert(key, file);
		m_fileList.append(key);
		if(flag) endInsertRows();
		emit rowAppended();
	}
}

bool FileListModel::removeFile(const QModelIndex &index)
{
	const int row = index.row();
	if(row >= 0 && row < m_fileList.count())
	{
		beginResetModel();
		m_fileStore.remove(m_fileList.at(row));
		m_fileList.removeAt(row);
		endResetModel();
		return true;
	}
	return false;
}

void FileListModel::clearFiles(void)
{
	beginResetModel();
	m_fileList.clear();
	m_fileStore.clear();
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

const AudioFileModel &FileListModel::getFile(const QModelIndex &index)
{
	if(index.row() >= 0 && index.row() < m_fileList.count())
	{
		return m_fileStore[m_fileList.at(index.row())];		//return m_fileStore.value(m_fileList.at(index.row()));
	}
	else
	{
		return m_nullAudioFile;
	}
}

AudioFileModel &FileListModel::operator[] (const QModelIndex &index)
{
	const QString key = m_fileList.at(index.row());
	return m_fileStore[key];
}

bool FileListModel::setFile(const QModelIndex &index, const AudioFileModel &audioFile)
{
	if(index.row() >= 0 && index.row() < m_fileList.count())
	{
		const QString oldKey = m_fileList.at(index.row());
		const QString newKey = MAKE_KEY(audioFile.filePath());
		
		beginResetModel();
		m_fileList.replace(index.row(), newKey);
		m_fileStore.remove(oldKey);
		m_fileStore.insert(newKey, audioFile);
		endResetModel();
		return true;
	}
	else
	{
		return false;
	}
}

int FileListModel::exportToCsv(const QString &outFile)
{
	const int nFiles = m_fileList.count();
	
	bool havePosition = false, haveTitle = false, haveArtist = false, haveAlbum = false, haveGenre = false, haveYear = false, haveComment = false;
	
	for(int i = 0; i < nFiles; i++)
	{
		const AudioFileModel &current = m_fileStore.value(m_fileList.at(i));
		const AudioFileModel_MetaInfo &metaInfo = current.metaInfo();
		
		if(metaInfo.position() > 0) havePosition = true;
		if(!metaInfo.title().isEmpty()) haveTitle = true;
		if(!metaInfo.artist().isEmpty()) haveArtist = true;
		if(!metaInfo.album().isEmpty()) haveAlbum = true;
		if(!metaInfo.genre().isEmpty()) haveGenre = true;
		if(metaInfo.year() > 0) haveYear = true;
		if(!metaInfo.comment().isEmpty()) haveComment = true;
	}

	if(!(haveTitle || haveArtist || haveAlbum || haveGenre || haveYear || haveComment))
	{
		return CsvError_NoTags;
	}

	QFile file(outFile);

	if(!file.open(QIODevice::WriteOnly))
	{
		return CsvError_FileOpen;
	}
	else
	{
		QStringList line;
		
		if(havePosition) line << "POSITION";
		if(haveTitle) line << "TITLE";
		if(haveArtist) line << "ARTIST";
		if(haveAlbum) line << "ALBUM";
		if(haveGenre) line << "GENRE";
		if(haveYear) line << "YEAR";
		if(haveComment) line << "COMMENT";

		if(file.write(line.join(";").append("\r\n").toUtf8().prepend("\xef\xbb\xbf")) < 1)
		{
			file.close();
			return CsvError_FileWrite;
		}
	}

	for(int i = 0; i < nFiles; i++)
	{
		QStringList line;
		const AudioFileModel &current = m_fileStore.value(m_fileList.at(i));
		const AudioFileModel_MetaInfo &metaInfo = current.metaInfo();
		
		if(havePosition) line << QString::number(metaInfo.position());
		if(haveTitle) line << metaInfo.title().trimmed();
		if(haveArtist) line << metaInfo.artist().trimmed();
		if(haveAlbum) line << metaInfo.album().trimmed();
		if(haveGenre) line << metaInfo.genre().trimmed();
		if(haveYear) line << QString::number(metaInfo.year());
		if(haveComment) line << metaInfo.comment().trimmed();

		if(file.write(line.replaceInStrings(";", ",").join(";").append("\r\n").toUtf8()) < 1)
		{
			file.close();
			return CsvError_FileWrite;
		}
	}

	file.close();
	return CsvError_OK;
}

int FileListModel::importFromCsv(QWidget *parent, const QString &inFile)
{
	QFile file(inFile);
	if(!file.open(QIODevice::ReadOnly))
	{
		return CsvError_FileOpen;
	}

	QTextCodec *codec = NULL;
	QByteArray bomCheck = file.peek(16);

	if((!bomCheck.isEmpty()) && bomCheck.startsWith("\xef\xbb\xbf"))
	{
		codec = QTextCodec::codecForName("UTF-8");
	}
	else if((!bomCheck.isEmpty()) && bomCheck.startsWith("\xff\xfe"))
	{
		codec = QTextCodec::codecForName("UTF-16LE");
	}
	else if((!bomCheck.isEmpty()) && bomCheck.startsWith("\xfe\xff"))
	{
		codec = QTextCodec::codecForName("UTF-16BE");
	}
	else
	{
		const QString systemDefault = tr("(System Default)");

		QStringList codecList;
		codecList.append(systemDefault);
		codecList.append(MUtils::available_codepages());

		QInputDialog *input = new QInputDialog(parent);
		input->setLabelText(EXPAND(tr("Select ANSI Codepage for CSV file:")));
		input->setOkButtonText(tr("OK"));
		input->setCancelButtonText(tr("Cancel"));
		input->setTextEchoMode(QLineEdit::Normal);
		input->setComboBoxItems(codecList);
	
		if(input->exec() < 1)
		{
			MUTILS_DELETE(input);
			return CsvError_Aborted;
		}
	
		if(input->textValue().compare(systemDefault, Qt::CaseInsensitive))
		{
			qDebug("User-selected codec is: %s", input->textValue().toLatin1().constData());
			codec = QTextCodec::codecForName(input->textValue().toLatin1().constData());
		}
		else
		{
			qDebug("Going to use the system's default codec!");
			codec = QTextCodec::codecForName("System");
		}

		MUTILS_DELETE(input);
	}

	bomCheck.clear();

	//----------------------//

	QTextStream stream(&file);
	stream.setAutoDetectUnicode(false);
	stream.setCodec(codec);

	QString headerLine = stream.readLine().simplified();

	while(headerLine.isEmpty())
	{
		if(stream.atEnd())
		{
			qWarning("The file appears to be empty!");
			return CsvError_FileRead;
		}
		qWarning("Skipping a blank line at beginning of CSV file!");
		headerLine = stream.readLine().simplified();
	}

	QStringList header = headerLine.split(";", QString::KeepEmptyParts);

	const int nCols = header.count();
	const int nFiles = m_fileList.count();

	if(nCols < 1)
	{
		qWarning("Header appears to be empty!");
		return CsvError_FileRead;
	}

	bool *ignore = new bool[nCols];
	memset(ignore, 0, sizeof(bool) * nCols);

	for(int i = 0; i < nCols; i++)
	{
		if((header[i] = header[i].trimmed()).isEmpty())
		{
			ignore[i] = true;
		}
	}

	//----------------------//

	for(int i = 0; i < nFiles; i++)
	{
		if(stream.atEnd())
		{
			MUTILS_DELETE_ARRAY(ignore);
			return CsvError_Incomplete;
		}
		
		QString line = stream.readLine().simplified();
		
		if(line.isEmpty())
		{
			qWarning("Skipping a blank line in CSV file!");
			continue;
		}
		
		QStringList data = line.split(";", QString::KeepEmptyParts);

		if(data.count() < header.count())
		{
			qWarning("Skipping an incomplete line in CSV file!");
			continue;
		}

		const QString key = m_fileList[i];

		for(int j = 0; j < nCols; j++)
		{
			if(ignore[j])
			{
				continue;
			}
			else if(CHECK_HDR(header.at(j), "POSITION"))
			{
				bool ok = false;
				unsigned int temp = data.at(j).trimmed().toUInt(&ok);
				if(ok) m_fileStore[key].metaInfo().setPosition(temp);
			}
			else if(CHECK_HDR(header.at(j), "TITLE"))
			{
				QString temp = data.at(j).trimmed();
				if(!temp.isEmpty()) m_fileStore[key].metaInfo().setTitle(temp);
			}
			else if(CHECK_HDR(header.at(j), "ARTIST"))
			{
				QString temp = data.at(j).trimmed();
				if(!temp.isEmpty()) m_fileStore[key].metaInfo().setArtist(temp);
			}
			else if(CHECK_HDR(header.at(j), "ALBUM"))
			{
				QString temp = data.at(j).trimmed();
				if(!temp.isEmpty()) m_fileStore[key].metaInfo().setAlbum(temp);
			}
			else if(CHECK_HDR(header.at(j), "GENRE"))
			{
				QString temp = data.at(j).trimmed();
				if(!temp.isEmpty()) m_fileStore[key].metaInfo().setGenre(temp);
			}
			else if(CHECK_HDR(header.at(j), "YEAR"))
			{
				bool ok = false;
				unsigned int temp = data.at(j).trimmed().toUInt(&ok);
				if(ok) m_fileStore[key].metaInfo().setYear(temp);
			}
			else if(CHECK_HDR(header.at(j), "COMMENT"))
			{
				QString temp = data.at(j).trimmed();
				if(!temp.isEmpty()) m_fileStore[key].metaInfo().setComment(temp);
			}
			else
			{
				qWarning("Unkonw field '%s' will be ignored!", MUTILS_UTF8(header.at(j)));
				ignore[j] = true;
				
				if(!checkArray(ignore, false, nCols))
				{
					qWarning("No known fields left, aborting!");
					return CsvError_NoTags;
				}
			}
		}
	}

	//----------------------//

	MUTILS_DELETE_ARRAY(ignore);
	return CsvError_OK;
}

bool FileListModel::checkArray(const bool *a, const bool val, size_t len)
{
	for(size_t i = 0; i < len; i++)
	{
		if(a[i] == val) return true;
	}

	return false;
}

QString FileListModel::int2str(const int &value) const
{
	if(m_fileList.count() < 10)
	{
		return QString().sprintf("%d", value);
	}
	else
	{
		const QString format = QString().sprintf("%%0%dd", LOG10(m_fileList.count()));
		return QString().sprintf(format.toLatin1().constData(), value);
	}
}
