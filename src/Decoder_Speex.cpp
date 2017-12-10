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

#include "Decoder_Speex.h"

//Internal
#include "Global.h"

//MUtils
#include <MUtils/Exception.h>

//Qt
#include <QDir>
#include <QProcess>
#include <QRegExp>

SpeexDecoder::SpeexDecoder(void)
:
	m_binary(lamexp_tools_lookup("speexdec.exe"))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing Speex decoder. Tool 'speexdec.exe' is not registred!");
	}
}

SpeexDecoder::~SpeexDecoder(void)
{
}

bool SpeexDecoder::decode(const QString &sourceFile, const QString &outputFile, QAtomicInt &abortFlag)
{
	QProcess process;
	QStringList args;

	args << "-V";
	args << QDir::toNativeSeparators(sourceFile);
	args << QDir::toNativeSeparators(outputFile);

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	QRegExp regExp("Working\\.\\.\\. (.)");

	const result_t result = awaitProcess(process, abortFlag, [this, &regExp](const QString &text)
	{
		if (regExp.lastIndexIn(text) >= 0)
		{
			return true;
		}
		return false;
	});
	
	return (result == RESULT_SUCCESS);
}

bool SpeexDecoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	if(containerType.compare("Speex", Qt::CaseInsensitive) == 0 || containerType.compare("OGG", Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare("Speex", Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}

	return false;
}

const AbstractDecoder::supportedType_t *SpeexDecoder::supportedTypes(void)
{
	static const char *exts[] =
	{
		"spx", "ogg", NULL
	};

	static const supportedType_t s_supportedTypes[] =
	{
		{ "Speex", exts },
		{ NULL, NULL }
	};

	return s_supportedTypes;
}
