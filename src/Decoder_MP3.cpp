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

#include "Decoder_MP3.h"

//Internal
#include "Global.h"

//MUtils
#include <MUtils/Global.h>
#include <MUtils/Exception.h>

//Qt
#include <QDir>
#include <QProcess>
#include <QRegExp>
#include <QMutexLocker>

//Static
QMutex MP3Decoder::m_regexMutex;
MUtils::Lazy<QRegExp> MP3Decoder::m_regxLayer([]
{
	return new QRegExp(L1S("^Layer\\s+(1|2|3)\\b"), Qt::CaseInsensitive);
});
MUtils::Lazy<QRegExp> MP3Decoder::m_regxVersion([]
{
	return new QRegExp(L1S("^(Version\\s+)?(1|2|2\\.5)\\b"), Qt::CaseInsensitive);
});

MP3Decoder::MP3Decoder(void)
:
	m_binary(lamexp_tools_lookup("mpg123.exe"))
{
	if (m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing MPG123 decoder. Tool 'mpg123.exe' is not registred!");
	}
}

MP3Decoder::~MP3Decoder(void)
{
}

bool MP3Decoder::decode(const QString &sourceFile, const QString &outputFile, QAtomicInt &abortFlag)
{
	QProcess process;
	QStringList args;

	args << "-v" << "--utf8" << "-w" << QDir::toNativeSeparators(outputFile);
	args << QDir::toNativeSeparators(sourceFile);

	if (!startProcess(process, m_binary, args))
	{
		return false;
	}

	int prevProgress = -1;
	QRegExp regExp("[_=>]\\s+(\\d+)\\+(\\d+)\\s+");

	const result_t result = awaitProcess(process, abortFlag, [this, &prevProgress, &regExp](const QString &text)
	{
		if (regExp.lastIndexIn(text) >= 0)
		{
			quint32 values[2];
			if (MUtils::regexp_parse_uint32(regExp, values, 2))
			{
				const quint32 total = values[0] + values[1];
				if ((total >= 512U) && (values[0] >= 256U))
				{
					const int newProgress = qRound((static_cast<double>(values[0]) / static_cast<double>(total)) * 100.0);
					if (newProgress > prevProgress)
					{
						emit statusUpdated(newProgress);
						prevProgress = NEXT_PROGRESS(newProgress);
					}
				}
			}
			return true;
		}
		return false;
	});

	return (result == RESULT_SUCCESS);
}

bool MP3Decoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	static const QLatin1String mpegAudio("MPEG Audio"), waveAudio("Wave");
	if((containerType.compare(mpegAudio, Qt::CaseInsensitive) == 0) || (containerType.compare(waveAudio, Qt::CaseInsensitive) == 0))
	{
		if(formatType.compare(mpegAudio, Qt::CaseInsensitive) == 0)
		{
			QMutexLocker lock(&m_regexMutex);
			if (m_regxLayer->indexIn(formatProfile) >= 0)
			{
				return (m_regxVersion->indexIn(formatVersion) >= 0);
			}
		}
	}

	return false;
}

const AbstractDecoder::supportedType_t *MP3Decoder::supportedTypes(void)
{
	static const char *exts[][3] =
	{
		{ "mp3", "mpa", NULL },
		{ "mp2", "mpa", NULL },
		{ "mp1", "mpa", NULL }
	};

	static const supportedType_t s_supportedTypes[] =
	{
		{ "MPEG Audio Layer III", exts[0] },
		{ "MPEG Audio Layer II",  exts[1] },
		{ "MPEG Audio Layer I",   exts[2] },
		{ NULL, NULL }
	};

	return s_supportedTypes;
}
