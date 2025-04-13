///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2025 LoRd_MuldeR <MuldeR2@GMX.de>
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

class QFile;
class QDir;
class QFileInfo;
class QStringList;
class QString;

class PlaylistImporter
{
public:
	enum playlist_t
	{
		notPlaylist,
		m3uPlaylist,
		plsPlaylist,
		wplPlaylist
	};

	static bool importPlaylist(QStringList &fileList, const QString &playlistFile);
	static const char *const *const getSupportedExtensions(void);

private:
	PlaylistImporter(void) {}
	~PlaylistImporter(void) {}

	static bool parsePlaylist_m3u(QFile &data, QStringList &fileList, const QDir &baseDir, const QDir &rootDir);
	static bool parsePlaylist_pls(QFile &data, QStringList &fileList, const QDir &baseDir, const QDir &rootDir);
	static bool parsePlaylist_wpl(QFile &data, QStringList &fileList, const QDir &baseDir, const QDir &rootDir);

	static playlist_t isPlaylist(const QString &fileName);
	static void fixFilePath(QFileInfo &filename, const QDir &baseDir, const QDir &rootDir);
	static QString unescapeXml(QString str);
};
