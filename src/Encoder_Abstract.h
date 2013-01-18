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

#pragma once

#include "Tool_Abstract.h"
#include "Model_AudioFile.h"

class QProcess;
class QStringList;
class QMutex;

class AbstractEncoder : public AbstractTool
{
	Q_OBJECT

public:
	AbstractEncoder(void);
	virtual ~AbstractEncoder(void);

	//Internal encoder API
	virtual bool encode(const QString &sourceFile, const AudioFileModel &metaInfo, const QString &outputFile, volatile bool *abortFlag) = 0;
	virtual bool isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion) = 0;
	virtual QString extension(void) = 0;
	virtual const unsigned int *supportedSamplerates(void);
	virtual const unsigned int *supportedChannelCount(void);
	virtual const unsigned int *supportedBitdepths(void);
	virtual const bool needsTimingInfo(void);

	//Common setter methods
	void setBitrate(int bitrate);
	void setRCMode(int mode);
	void setCustomParams(const QString &customParams);

protected:
	int m_configBitrate;
	int m_configRCMode;
	QString m_configCustomParams;

	//Helper functions
	bool isUnicode(const QString &text);
	QString cleanTag(const QString &text);
};
