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

#include "Decoder_WavPack.h"

//Internal
#include "Global.h"

//MUtils
#include <MUtils/Exception.h>

//Qt
#include <QDir>
#include <QProcess>
#include <QRegExp>

WavPackDecoder::WavPackDecoder(void)
:
	m_binary(lamexp_tools_lookup("wvunpack.exe"))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing WavPack decoder. Tool 'wvunpack.exe' is not registred!");
	}
}

WavPackDecoder::~WavPackDecoder(void)
{
}

bool WavPackDecoder::decode(const QString &sourceFile, const QString &outputFile, QAtomicInt &abortFlag)
{
	QProcess process;
	QStringList args;

	args << "-y" << "-w";
	args << QDir::toNativeSeparators(sourceFile);
	args << QDir::toNativeSeparators(outputFile);

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	int prevProgress = -1;
	QRegExp regExp("(\\s|\b)(\\d+)%\\s+done");

	const result_t result = awaitProcess(process, abortFlag, [this, &prevProgress, &regExp](const QString &text)
	{
		if (regExp.lastIndexIn(text) >= 0)
		{
			qint32 newProgress;
			if (MUtils::regexp_parse_int32(regExp, newProgress, 2U))
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

bool WavPackDecoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	if(containerType.compare("WavPack", Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare("WavPack", Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}

	return false;
}

const AbstractDecoder::supportedType_t *WavPackDecoder::supportedTypes(void)
{
	static const char *exts[] =
	{
		"wv", NULL
	};

	static const supportedType_t s_supportedTypes[] =
	{
		{ "WavPack Hybrid Lossless Audio", exts },
		{ NULL, NULL }
	};

	return s_supportedTypes;
}
