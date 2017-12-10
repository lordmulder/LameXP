///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2017 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Decoder_AC3.h"

//Internal
#include "Global.h"

//MUtils
#include <MUtils/Exception.h>

//Qt
#include <QDir>
#include <QProcess>
#include <QRegExp>

AC3Decoder::AC3Decoder(void)
:
	m_binary(lamexp_tools_lookup("valdec.exe"))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing Valib decoder. Tool 'valdec.exe' is not registred!");
	}
}

AC3Decoder::~AC3Decoder(void)
{
}

bool AC3Decoder::decode(const QString &sourceFile, const QString &outputFile, QAtomicInt &abortFlag)
{
	QProcess process;
	QStringList args;

	args << QDir::toNativeSeparators(sourceFile);
	args << "-w" << QDir::toNativeSeparators(outputFile);

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	int prevProgress = -1;
	QRegExp regExp("\\b\\s*(\\d+)\\.(\\d+)?%(\\s+)Frames", Qt::CaseInsensitive);

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
					prevProgress = (newProgress < 99) ? (newProgress + 1) : newProgress;
				}
			}
			return true;
		}
		return false;
	});

	return (result == RESULT_SUCCESS);
}

bool AC3Decoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	if(containerType.compare("AC-3", Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare("AC-3", Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}
	if(containerType.compare("E-AC-3", Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare("E-AC-3", Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}
	else if(containerType.compare("DTS", Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare("DTS", Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}
	else if(containerType.compare("Wave", Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare("AC-3", Qt::CaseInsensitive) == 0 || formatType.compare("E-AC-3", Qt::CaseInsensitive) == 0 || formatType.compare("DTS", Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}

	return false;
}

const AbstractDecoder::supportedType_t *AC3Decoder::supportedTypes(void)
{
	static const char *exts[][4] =
	{
		{ "ac3", "eac3", "wav", NULL },
		{ "dts", "wav",         NULL }
	};

	static const supportedType_t s_supportedTypes[] =
	{
		{ "AC-3 / ATSC A/52",       exts[0] },
		{ "Digital Theater System", exts[1] },
		{ NULL, NULL }
	};

	return s_supportedTypes;
}
