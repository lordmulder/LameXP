///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2018 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Encoder_Abstract.h"

#include <QObject>

class FLACEncoder : public AbstractEncoder
{
	Q_OBJECT

public:
	FLACEncoder(void);
	~FLACEncoder(void);

	virtual bool encode(const QString &sourceFile, const AudioFileModel_MetaInfo &metaInfo, const unsigned int duration, const unsigned int channels, const QString &outputFile, QAtomicInt &abortFlag);
	virtual bool isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion);
	virtual const unsigned int *supportedChannelCount(void);
	virtual const unsigned int *supportedBitdepths(void);

	//Encoder info
	virtual const AbstractEncoderInfo *toEncoderInfo(void) const { return getEncoderInfo(); }
	static const AbstractEncoderInfo *getEncoderInfo(void);

private:
	const QString m_binary;
};
