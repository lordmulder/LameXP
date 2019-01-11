///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2019 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Decoder_AAC.h"

//Internal
#include "Global.h"

//MUtils
#include <MUtils/Exception.h>

//Qt
#include <QDir>
#include <QProcess>
#include <QRegExp>
#include <QMutexLocker>

//Static
QMutex AACDecoder::m_regexMutex;
MUtils::Lazy<QRegExp> AACDecoder::m_regxFeatures([]
{
	return new QRegExp(L1S("\\bLC\\b"), Qt::CaseInsensitive);
});

AACDecoder::AACDecoder(void)
:
	m_binary(lamexp_tools_lookup("faad.exe"))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing AAC decoder. Tool 'faad.exe' is not registred!");
	}
}

AACDecoder::~AACDecoder(void)
{
}

bool AACDecoder::decode(const QString &sourceFile, const QString &outputFile, QAtomicInt &abortFlag)
{
	QProcess process;
	QStringList args;

	args << "-o" << QDir::toNativeSeparators(outputFile);
	args << QDir::toNativeSeparators(sourceFile);

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	int prevProgress = -1;
	QRegExp regExp("\\[(\\d+)%\\]\\s*decoding", Qt::CaseInsensitive); //regExp("\\b(\\d+)%\\s+decoding", Qt::CaseInsensitive);

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

bool AACDecoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	if((containerType.compare(QLatin1String("ADTS"), Qt::CaseInsensitive) == 0) || (containerType.compare(QLatin1String("MPEG-4"), Qt::CaseInsensitive) == 0))
	{
		if(formatType.compare(QLatin1String("AAC"), Qt::CaseInsensitive) == 0)
		{
			QMutexLocker lock(&m_regexMutex);
			if (m_regxFeatures->indexIn(formatProfile) >= 0)
			{
				if((formatVersion.compare(QLatin1String("2"), Qt::CaseInsensitive) == 0) || (formatVersion.compare(QLatin1String("4"), Qt::CaseInsensitive) == 0) || formatVersion.isEmpty())
				{
					return true;
				}
			}
		}
	}

	return false;
}

const AbstractDecoder::supportedType_t *AACDecoder::supportedTypes(void)
{
	static const char *exts[] =
	{
		"mp4", "m4a", "aac", NULL
	};

	static const supportedType_t s_supportedTypes[] =
	{
		{ "Advanced Audio Coding", exts },
		{ NULL, NULL }
	};

	return s_supportedTypes;
}
