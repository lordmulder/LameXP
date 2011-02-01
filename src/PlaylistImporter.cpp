///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2011 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "PlaylistImporter.h"

#include <QString>
#include <QStringList>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QDate>
#include <QTime>
#include <QDebug>

//Un-escape XML characters
static const struct
{
	char *escape;
	char *output;
}
g_xmlEscapeSequence[] =
{
	{"&amp;", "&"},
	{"&lt;", "<"},
	{"&gt;", ">"},
	{"&apos;", "'"},
	{"&nbsp;", " "},
	{"&quot;", "\""},
	{NULL, NULL}
};

const char *PlaylistImporter::supportedExtensions = "*.m3u *.m3u8 *.pls *.asx *.wpl";


////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////

bool PlaylistImporter::importPlaylist(QStringList &fileList, const QString &playlistFile)
{
	QFileInfo file(playlistFile);
	QDir baseDir(file.canonicalPath());

	QDir rootDir(baseDir);
	while(rootDir.cdUp());

	//Sanity check
	if(file.size() < 3 || file.size() > 512000)
	{
		return false;
	}
	
	//Detect playlist type
	playlist_t playlistType = isPlaylist(file.canonicalFilePath());

	//Exit if not a playlist
	if(playlistType == notPlaylist)
	{
		return false;
	}
	
	QFile data(playlistFile);

	//Open file for reading
	if(!data.open(QIODevice::ReadOnly))
	{
		return false;
	}

	//Parse playlist depending on type
	switch(playlistType)
	{
	case m3uPlaylist:
		return parsePlaylist_m3u(data, fileList, baseDir, rootDir);
		break;
	case plsPlaylist:
		return parsePlaylist_pls(data, fileList, baseDir, rootDir);
		break;
	case wplPlaylist:
		return parsePlaylist_wpl(data, fileList, baseDir, rootDir);
		break;
	default:
		return false;
		break;
	}
}

////////////////////////////////////////////////////////////
// Private Functions
////////////////////////////////////////////////////////////

PlaylistImporter::playlist_t PlaylistImporter::isPlaylist(const QString &fileName)
{
	QFileInfo file (fileName);
	
	if(file.suffix().compare("m3u", Qt::CaseInsensitive) == 0 || file.suffix().compare("m3u8", Qt::CaseInsensitive) == 0)
	{
		return m3uPlaylist;
	}
	else if(file.suffix().compare("pls", Qt::CaseInsensitive) == 0)
	{
		return  plsPlaylist;
	}
	else if(file.suffix().compare("asx", Qt::CaseInsensitive) == 0 || file.suffix().compare("wpl", Qt::CaseInsensitive) == 0)
	{
		return  wplPlaylist;
	}
	else
	{
		return notPlaylist;
	}
}

bool PlaylistImporter::parsePlaylist_m3u(QFile &data, QStringList &fileList, const QDir &baseDir, const QDir &rootDir)
{
	QByteArray line = data.readLine();
	
	while(line.size() > 0)
	{
		QFileInfo filename1(QDir::fromNativeSeparators(QString::fromUtf8(line.constData(), line.size()).trimmed()));
		QFileInfo filename2(QDir::fromNativeSeparators(QString::fromLatin1(line.constData(), line.size()).trimmed()));

		filename1.setCaching(false);
		filename2.setCaching(false);

		if(!(filename1.filePath().startsWith("#") || filename2.filePath().startsWith("#")))
		{
			fixFilePath(filename1, baseDir, rootDir);
			fixFilePath(filename2, baseDir, rootDir);

			if(filename1.exists())
			{
				if(isPlaylist(filename1.canonicalFilePath()) == notPlaylist)
				{
					fileList << filename1.canonicalFilePath();
				}
			}
			else if(filename2.exists())
			{
				if(isPlaylist(filename2.canonicalFilePath()) == notPlaylist)
				{
					fileList << filename2.canonicalFilePath();
				}
			}
		}

		line = data.readLine();
	}

	return true;
}

bool PlaylistImporter::parsePlaylist_pls(QFile &data, QStringList &fileList, const QDir &baseDir, const QDir &rootDir)
{
	QRegExp plsEntry("File(\\d+)=(.+)", Qt::CaseInsensitive);
	QByteArray line = data.readLine();
	
	while(line.size() > 0)
	{
		bool flag = false;
		
		QString temp1(QDir::fromNativeSeparators(QString::fromUtf8(line.constData(), line.size()).trimmed()));
		QString temp2(QDir::fromNativeSeparators(QString::fromLatin1(line.constData(), line.size()).trimmed()));

		if(!flag && plsEntry.indexIn(temp1) >= 0)
		{
			QFileInfo filename(QDir::fromNativeSeparators(plsEntry.cap(2)).trimmed());
			filename.setCaching(false);
			fixFilePath(filename, baseDir, rootDir);

			if(filename.exists())
			{
				if(isPlaylist(filename.canonicalFilePath()) == notPlaylist)
				{
					fileList << filename.canonicalFilePath();
				}
				flag = true;
			}
		}
		
		if(!flag && plsEntry.indexIn(temp2) >= 0)
		{
			QFileInfo filename(QDir::fromNativeSeparators(plsEntry.cap(2)).trimmed());
			filename.setCaching(false);
			fixFilePath(filename, baseDir, rootDir);

			if(filename.exists())
			{
				if(isPlaylist(filename.canonicalFilePath()) == notPlaylist)
				{
					fileList << filename.canonicalFilePath();
				}
				flag = true;
			}
		}

		line = data.readLine();
	}

	return true;
}

bool PlaylistImporter::parsePlaylist_wpl(QFile &data, QStringList &fileList, const QDir &baseDir, const QDir &rootDir)
{
	QRegExp skipData("<!--(.+)-->", Qt::CaseInsensitive);
	QRegExp wplEntry("<(media|ref)[^<>]*(src|href)=\"([^\"]+)\"[^<>]*>", Qt::CaseInsensitive);
	
	skipData.setMinimal(true);

	QByteArray buffer = data.readAll();
	QString line = QString::fromUtf8(buffer.constData(), buffer.size()).simplified();
	buffer.clear();

	int index = 0;

	while((index = skipData.indexIn(line)) >= 0)
	{
		line.remove(index, skipData.matchedLength());
	}

	int offset = 0;

	while((offset = wplEntry.indexIn(line, offset) + 1) > 0)
	{
		QFileInfo filename(QDir::fromNativeSeparators(unescapeXml(wplEntry.cap(3)).trimmed()));
		filename.setCaching(false);
		fixFilePath(filename, baseDir, rootDir);

		if(filename.exists())
		{
			if(isPlaylist(filename.canonicalFilePath()) == notPlaylist)
			{
				fileList << filename.canonicalFilePath();
			}
		}
	}

	return true;
}

void PlaylistImporter::fixFilePath(QFileInfo &filename, const QDir &baseDir, const QDir &rootDir)
{
	if(filename.filePath().startsWith("/"))
	{
		while(filename.filePath().startsWith("/"))
		{
			filename.setFile(filename.filePath().mid(1));
		}
		filename.setFile(rootDir.filePath(filename.filePath()));
	}
	
	if(!filename.isAbsolute())
	{
		filename.setFile(baseDir.filePath(filename.filePath()));
	}
}

QString &PlaylistImporter::unescapeXml(QString &str)
{
	for(int i = 0; (g_xmlEscapeSequence[i].escape && g_xmlEscapeSequence[i].output); i++)
	{
		str.replace(g_xmlEscapeSequence[i].escape, g_xmlEscapeSequence[i].output);
	}
	
	return str;
}
