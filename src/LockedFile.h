///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2019 LoRd_MuldeR <MuldeR2@GMX.de>
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

#pragma once

#include <QString>

class QResource;
class QFile;

class LockedFile
{
public:
	LockedFile(QResource *const resource, const QString &outPath, const QByteArray &expectedHash = QByteArray(), const bool bOwnsFile = true);
	LockedFile(const QString &filePath, const QByteArray &expectedHash, const bool bOwnsFile = false);
	LockedFile(const QString &filePath, const bool bOwnsFile = false);
	~LockedFile(void);

	const QString filePath(void);

private:
	const bool m_bOwnsFile;
	const QString m_filePath;
	int m_fileDescriptor;
};
