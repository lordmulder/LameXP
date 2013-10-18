///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2013 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Global.h"
#include "Tool_Abstract.h"
#include "Model_AudioFile.h"

class QProcess;
class QStringList;
class QMutex;

class AbstractEncoderInfo
{
public:
	typedef enum
	{
		TYPE_BITRATE = 0,
		TYPE_APPROX_BITRATE = 1,
		TYPE_QUALITY_LEVEL_INT = 2,
		TYPE_QUALITY_LEVEL_FLT = 3,
		TYPE_COMPRESSION_LEVEL = 4,
		TYPE_UNCOMPRESSED = 5
	}
	value_type_t;

	virtual bool isModeSupported(int mode) const = 0;	//Returns whether the encoder does support the current RC mode
	virtual int valueCount(int mode) const = 0;			//The number of bitrate/quality values for current RC mode
	virtual int valueAt(int mode, int index) const = 0;	//The bitrate/quality value at 'index' for the current RC mode
	virtual int valueType(int mode) const = 0;			//The display type of the values for the current RC mode
	virtual const char* description(void) const = 0;	//Description of the encoder that can be displayed to the user
};

class AbstractEncoder : public AbstractTool
{
	Q_OBJECT

public:
	AbstractEncoder(void);
	virtual ~AbstractEncoder(void);

	//Internal encoder API
	virtual bool encode(const QString &sourceFile, const AudioFileModel_MetaInfo &metaInfo, const unsigned int duration, const QString &outputFile, volatile bool *abortFlag) = 0;
	virtual bool isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion) = 0;
	virtual QString extension(void) = 0;
	virtual const unsigned int *supportedSamplerates(void);
	virtual const unsigned int *supportedChannelCount(void);
	virtual const unsigned int *supportedBitdepths(void);
	virtual const bool needsTimingInfo(void);

	//Common setter methods
	virtual void setBitrate(int bitrate);
	virtual void setRCMode(int mode);
	virtual void setCustomParams(const QString &customParams);

	//Encoder info
	static const AbstractEncoderInfo *getEncoderInfo(void)
	{
		THROW("This method shall be re-implemented in derived classes!");
		return NULL;
	}

protected:
	int m_configBitrate;			//Bitrate *or* VBR-quality-level
	int m_configRCMode;				//Rate-control mode
	QString m_configCustomParams;	//Custom parameters, if any

	//Helper functions
	bool isUnicode(const QString &text);
	QString cleanTag(const QString &text);
};
