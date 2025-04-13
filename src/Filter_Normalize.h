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

#include "Filter_Abstract.h"

class NormalizeFilter : public AbstractFilter
{
public:
	NormalizeFilter(const int &peakVolume = -50, const bool &dnyAudNorm = false, const bool &channelsCoupled = true, const int &filterSize = 31);
	~NormalizeFilter(void);

	virtual FilterResult apply(const QString &sourceFile, const QString &outputFile, AudioFileModel_TechInfo *const formatInfo, QAtomicInt &abortFlag);

private:
	const QString m_binary;
	const bool m_useDynAudNorm;
	const bool m_channelsCoupled;
	const int m_peakVolume;
	const int m_filterLength;
};
