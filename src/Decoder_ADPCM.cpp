///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2023 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Decoder_ADPCM.h"

//Internal
#include "Global.h"

//MUtils
#include <MUtils/Exception.h>

//Qt
#include <QDir>
#include <QProcess>
#include <QRegExp>

ADPCMDecoder::ADPCMDecoder(void)
:
	m_binary(lamexp_tools_lookup("sox.exe"))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing Vorbis decoder. Tool 'sox.exe' is not registred!");
	}
}

ADPCMDecoder::~ADPCMDecoder(void)
{
}

bool ADPCMDecoder::decode(const QString &sourceFile, const QString &outputFile, QAtomicInt &abortFlag)
{
	QProcess process;
	QStringList args;

	args << "-V3" << "-S" << "--temp" << ".";
	args << QDir::toNativeSeparators(sourceFile);
	args << "-e" << "signed-integer";
	args << QDir::toNativeSeparators(outputFile);

	if(!startProcess(process, m_binary, args, QFileInfo(outputFile).canonicalPath()))
	{
		return false;
	}

	int prevProgress = -1;
	QRegExp regExp("In:(\\d+)(\\.\\d+)*%");

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

bool ADPCMDecoder::isFormatSupported(const QString &containerType, const QString& /*containerProfile*/, const QString &formatType, const QString& /*formatProfile*/, const QString& /*formatVersion*/)
{
	if(containerType.compare(QLatin1String("Wave"), Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare(QLatin1String("ADPCM"), Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}

	if((containerType.compare(QLatin1String("AIFF"), Qt::CaseInsensitive) == 0) ||( containerType.compare(QLatin1String("AU"), Qt::CaseInsensitive) == 0))
	{
		if((formatType.compare(QLatin1String("PCM"), Qt::CaseInsensitive) == 0) || (formatType.compare(QLatin1String("ADPCM"), Qt::CaseInsensitive) == 0))
		{
			return true;
		}
	}

	return false;
}

const AbstractDecoder::supportedType_t *ADPCMDecoder::supportedTypes(void)
{
	static const char *exts[][3] =
	{
		{ "wav",         NULL },
		{ "aif", "aiff", NULL },
		{ "au" , "snd",  NULL }
	};

	static const supportedType_t s_supportedTypes[] =
	{
		{ "Microsoft ADPCM", exts[0] },
		{ "Apple/SGI AIFF",  exts[1] },
		{ "Sun/NeXT Au",     exts[2] },
		{ NULL, NULL }
	};

	return s_supportedTypes;
}
