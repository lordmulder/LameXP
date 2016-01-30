///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2016 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Encoder_Abstract.h"

//Internal
#include "Global.h"

//MUtils
#include <MUtils/Global.h>

AbstractEncoder::AbstractEncoder(void)
{
	m_configBitrate = 0;
	m_configRCMode = 0;
	m_configCustomParams.clear();
	m_configSamplingRate = 0;
}

AbstractEncoder::~AbstractEncoder(void)
{
}

/*
 * Setters
 */

void AbstractEncoder::setRCMode(const int &mode)
{
	if (!toEncoderInfo()->isModeSupported(qMax(0, mode)))
	{
		MUTILS_THROW("This RC mode is not supported by the encoder!");
	}
	m_configRCMode = qMax(0, mode);
}

void AbstractEncoder::setBitrate(const int &bitrate)
{
	if (qMax(0, bitrate) >= toEncoderInfo()->valueCount(m_configRCMode))
	{
		MUTILS_THROW("The specified bitrate/quality is out of range!");
	}
	m_configBitrate = qMax(0, bitrate);
}

void AbstractEncoder::setCustomParams(const QString &customParams)
{
	m_configCustomParams = customParams.trimmed();
}

void AbstractEncoder::setSamplingRate(const int &value)
{
	if (!toEncoderInfo()->isResamplingSupported())
	{
		MUTILS_THROW("This encoder does *not* support native resampling!");
	}
	m_configSamplingRate = qBound(0, value, 48000);
};


/*
 * Default implementation
 */

// Does encoder require the input to be downmixed to stereo?
const unsigned int *AbstractEncoder::supportedChannelCount(void)
{
	return NULL;
}

// Does encoder require the input to be downsampled? (NULL-terminated array of supported sampling rates)
const unsigned int *AbstractEncoder::supportedSamplerates(void)
{
	return NULL;
}

// What bitdepths does the encoder support as input? (NULL-terminated array of supported bits per sample)
const unsigned int *AbstractEncoder::supportedBitdepths(void)
{
	return NULL;
}

//Does the encoder need the exact duration of the source?
const bool AbstractEncoder::needsTimingInfo(void)
{
	return false;
}


/*
 * Helper functions
 */

//Does this text contain Non-ASCII characters?
bool AbstractEncoder::isUnicode(const QString &original)
{
	if(!original.isEmpty())
	{
		QString asLatin1 = QString::fromLatin1(original.toLatin1().constData());
		return (wcscmp(MUTILS_WCHR(original), MUTILS_WCHR(asLatin1)) != 0);
	}
	return false;
}

//Remove "problematic" characters from tag
QString AbstractEncoder::cleanTag(const QString &text)
{
	QString result(text);
	result.replace(QChar('"'), "'");
	result.replace(QChar('\\'), "/");
	return result;
}
