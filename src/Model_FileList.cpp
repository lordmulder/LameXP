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

#include "Global.h"

#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QTextCodec>
#include <QTextStream>
#include <QInputDialog>

#define EXPAND(STR) QString(STR).leftJustified(96, ' ')
#define CHECK_HDR(STR,NAM) (!(STR).compare((NAM), Qt::CaseInsensitive))

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

int FileListModel::exportToCsv(const QString &outFile)
{
	const int nFiles = m_fileList.count();
	
	bool havePosition = false, haveTitle = false, haveArtist = false, haveAlbum = false, haveGenre = false, haveYear = false, haveComment = false;
	
	for(int i = 0; i < nFiles; i++)
	{
		if(m_fileList.at(i).filePosition() > 0) havePosition = true;
		if(!m_fileList.at(i).fileName().isEmpty()) haveTitle = true;
		if(!m_fileList.at(i).fileArtist().isEmpty()) haveArtist = true;
		if(!m_fileList.at(i).fileAlbum().isEmpty()) haveAlbum = true;
		if(!m_fileList.at(i).fileGenre().isEmpty()) haveGenre = true;
		if(m_fileList.at(i).fileYear() > 0) haveYear = true;
		if(!m_fileList.at(i).fileComment().isEmpty()) haveComment = true;
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
		
		if(havePosition) line << QString::number(m_fileList.at(i).filePosition());
		if(haveTitle) line << m_fileList.at(i).fileName().trimmed();
		if(haveArtist) line << m_fileList.at(i).fileArtist().trimmed();
		if(haveAlbum) line << m_fileList.at(i).fileAlbum().trimmed();
		if(haveGenre) line << m_fileList.at(i).fileGenre().trimmed();
		if(haveYear) line << QString::number(m_fileList.at(i).fileYear());
		if(haveComment) line << m_fileList.at(i).fileComment().trimmed();

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
		codecList.append(lamexp_available_codepages());

		QInputDialog *input = new QInputDialog(parent);
		input->setLabelText(EXPAND(tr("Select ANSI Codepage for CSV file:")));
		input->setOkButtonText(tr("OK"));
		input->setCancelButtonText(tr("Cancel"));
		input->setTextEchoMode(QLineEdit::Normal);
		input->setComboBoxItems(codecList);
	
		if(input->exec() < 1)
		{
			LAMEXP_DELETE(input);
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

		LAMEXP_DELETE(input);
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
			LAMEXP_DELETE_ARRAY(ignore);
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
				if(ok) m_fileList[i].setFilePosition(temp);
			}
			else if(CHECK_HDR(header.at(j), "TITLE"))
			{
				QString temp = data.at(j).trimmed();
				if(!temp.isEmpty()) m_fileList[i].setFileName(temp);
			}
			else if(CHECK_HDR(header.at(j), "ARTIST"))
			{
				QString temp = data.at(j).trimmed();
				if(!temp.isEmpty()) m_fileList[i].setFileArtist(temp);
			}
			else if(CHECK_HDR(header.at(j), "ALBUM"))
			{
				QString temp = data.at(j).trimmed();
				if(!temp.isEmpty()) m_fileList[i].setFileAlbum(temp);
			}
			else if(CHECK_HDR(header.at(j), "GENRE"))
			{
				QString temp = data.at(j).trimmed();
				if(!temp.isEmpty()) m_fileList[i].setFileGenre(temp);
			}
			else if(CHECK_HDR(header.at(j), "YEAR"))
			{
				bool ok = false;
				unsigned int temp = data.at(j).trimmed().toUInt(&ok);
				if(ok) m_fileList[i].setFileYear(temp);
			}
			else if(CHECK_HDR(header.at(j), "COMMENT"))
			{
				QString temp = data.at(j).trimmed();
				if(!temp.isEmpty()) m_fileList[i].setFileComment(temp);
			}
			else
			{
				qWarning("Unkonw field '%s' will be ignored!", header.at(j).toUtf8().constData());
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

	LAMEXP_DELETE_ARRAY(ignore);
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
