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

#include "Decoder_Avisynth.h"


//Internal
#include "Global.h"

//MUtils
#include <MUtils/Exception.h>

//Qt
#include <QDir>
#include <QProcess>
#include <QRegExp>

AvisynthDecoder::AvisynthDecoder(void)
:
	m_binary(lamexp_tools_lookup("avs2wav.exe"))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing Avisynth decoder. Tool 'avs2wav.exe' is not registred!");
	}
}

AvisynthDecoder::~AvisynthDecoder(void)
{
}

bool AvisynthDecoder::decode(const QString &sourceFile, const QString &outputFile, QAtomicInt &abortFlag)
{
	QProcess process;
	QStringList args;

	args << QDir::toNativeSeparators(sourceFile);
	args << QDir::toNativeSeparators(outputFile);

	if(!startProcess(process, m_binary, args, QFileInfo(outputFile).absolutePath()))
	{
		return false;
	}

	int prevProgress = -1;
	QRegExp regExp("\\d+/\\d+ \\[(\\d+)%\\]");

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

bool AvisynthDecoder::isFormatSupported(const QString &containerType, const QString& /*containerProfile*/, const QString &formatType, const QString& /*formatProfile*/, const QString& /*formatVersion*/)
{
	static const QLatin1String avs("Avisynth");
	if(containerType.compare(avs, Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare(avs, Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}

	return false;
}

const AbstractDecoder::supportedType_t *AvisynthDecoder::supportedTypes(void)
{
	static const char *exts[] =
	{
		"avs", NULL
	};

	static const supportedType_t s_supportedTypes[] =
	{
		{ "Avisynth Script", exts },
		{ NULL, NULL }
	};

	return s_supportedTypes;
}
