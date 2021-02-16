///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2021 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include <QString>

class ArtworkModel_SharedData;
class QMutex;

class ArtworkModel
{
public:
	ArtworkModel(void);
	ArtworkModel(const QString &fileName, bool isOwner = true);
	ArtworkModel(const ArtworkModel &model);
	ArtworkModel &operator=(const ArtworkModel &model);
	~ArtworkModel(void);

	const QString &filePath(void) const;
	bool isOwner(void) const;
	void setFilePath(const QString &newPath, bool isOwner = true);
	void clear(void);
	
	inline bool isEmpty(void) const { return (m_data == NULL); }

private:
	const QString m_nullString;
	ArtworkModel_SharedData *m_data;
	QMutex *m_mutex;
};
