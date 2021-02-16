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

#include "Decoder_ALAC.h"

//Internal
#include "Global.h"

//MUtils
#include <MUtils/Exception.h>

//Qt
#include <QDir>
#include <QProcess>
#include <QRegExp>
#include <QUuid>

ALACDecoder::ALACDecoder(void)
:
	m_binary(lamexp_tools_lookup("refalac.exe"))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing ALAC decoder. Tool 'refalac.exe' is not registred!");
	}
}

ALACDecoder::~ALACDecoder(void)
{
}

bool ALACDecoder::decode(const QString &sourceFile, const QString &outputFile, QAtomicInt &abortFlag)
{
	QProcess process;
	QStringList args;

	args << "--decode";
	args << "-o" << QDir::toNativeSeparators(outputFile);
	args << QDir::toNativeSeparators(sourceFile);

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	int prevProgress = -1;
	QRegExp regExp("\\[(\\d+)\\.(\\d)%\\]");

	const result_t result = awaitProcess(process, abortFlag, [this, &prevProgress, &regExp](const QString &text)
	{
		if (regExp.lastIndexIn(text) >= 0)
		{
			qint32 intVal[2];
			if (MUtils::regexp_parse_int32(regExp, intVal, 2))
			{
				const int newProgress = qRound(static_cast<double>(intVal[0]) + (static_cast<double>(intVal[1]) / 10.0));
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

bool ALACDecoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	if(containerType.compare(QLatin1String("MPEG-4"), Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare(QLatin1String("ALAC"), Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}

	return false;
}

const AbstractDecoder::supportedType_t *ALACDecoder::supportedTypes(void)
{
	static const char *exts[] =
	{
		"mp4", "m4a", NULL
	};

	static const supportedType_t s_supportedTypes[] =
	{
		{ "Apple Lossless", exts },
		{ NULL, NULL }
	};

	return s_supportedTypes;
}
