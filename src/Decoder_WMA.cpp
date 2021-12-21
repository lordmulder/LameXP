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

#include "Decoder_WMA.h"

//Internal
#include "Global.h"

//MUtils
#include <MUtils/Exception.h>

//Qt
#include <QDir>
#include <QProcess>
#include <QRegExp>
#include <QSystemSemaphore>
#include <QUuid>

WMADecoder::WMADecoder(void)
:
	m_binary(lamexp_tools_lookup("wma2wav.exe"))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing WMA decoder. Tool 'wma2wav.exe' is not registred!");
	}
}

WMADecoder::~WMADecoder(void)
{
}

bool WMADecoder::decode(const QString &sourceFile, const QString &outputFile, QAtomicInt &abortFlag)
{
	QProcess process;
	QStringList args;

	args << "-i" << QDir::toNativeSeparators(sourceFile); 
	args << "-o" << QDir::toNativeSeparators(outputFile) << "-f";

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	int prevProgress = -1;
	QRegExp regExp("\\[(\\d+)\\.(\\d+)%\\]");

	const result_t result = awaitProcess(process, abortFlag, [this, &prevProgress, &regExp](const QString &text)
	{
		if (regExp.lastIndexIn(text) >= 0)
		{
			qint32 newProgress;
			if (MUtils::regexp_parse_int32(regExp, newProgress))
			{
				if (newProgress > prevProgress)
				{
					emit statusUpdated(newProgress);
					prevProgress = NEXT_PROGRESS(newProgress);
				}
			}
			return true;
		}
		return false;
	});
	
	return (result == RESULT_SUCCESS);
}

bool WMADecoder::isFormatSupported(const QString &containerType, const QString& /*containerProfile*/, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	if(containerType.compare(QLatin1String("Windows Media"), Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare(QLatin1String("WMA"), Qt::CaseInsensitive) == 0)
		{
			if((formatVersion.compare(QLatin1String("1"), Qt::CaseInsensitive) == 0) || (formatVersion.compare(QLatin1String("2"), Qt::CaseInsensitive) == 0) || (formatVersion.compare(QLatin1String("3"), Qt::CaseInsensitive) == 0) || (formatProfile.compare(QLatin1String("Pro"), Qt::CaseInsensitive) == 0) || (formatProfile.compare(QLatin1String("Lossless"), Qt::CaseInsensitive) == 0))
			{
				return true;
			}
		}
	}

	return false;
}

const AbstractDecoder::supportedType_t *WMADecoder::supportedTypes(void)
{
	static const char *exts[] =
	{
		"wma", "asf", NULL
	};

	static const supportedType_t s_supportedTypes[] =
	{
		{ "Windows Media Audio", exts },
		{ NULL, NULL }
	};

	return s_supportedTypes;
}
