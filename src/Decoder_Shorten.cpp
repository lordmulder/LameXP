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

#include "Decoder_Shorten.h"

//Internal
#include "Global.h"

//MUtils
#include <MUtils/Exception.h>

//Qt
#include <QDir>
#include <QProcess>
#include <QRegExp>
#include <QUuid>

ShortenDecoder::ShortenDecoder(void)
:
	m_binary(lamexp_tools_lookup("shorten.exe"))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing Shorten decoder. Tool 'shorten.exe' is not registred!");
	}
}

ShortenDecoder::~ShortenDecoder(void)
{
}

bool ShortenDecoder::decode(const QString &sourceFile, const QString &outputFile, QAtomicInt &abortFlag)
{
	QProcess process;
	QStringList args;

	args << "-x";
	args << QDir::toNativeSeparators(sourceFile);
	args << QDir::toNativeSeparators(outputFile);

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	//The Shorten Decoder doesn't actually send any status updates :-[
	emit statusUpdated(20 + (QUuid::createUuid().data1 % 80));

	return (awaitProcess(process, abortFlag) == RESULT_SUCCESS);
}

bool ShortenDecoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	static const QLatin1String shorten("Shorten");
	if(containerType.compare(shorten, Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare(shorten, Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}

	return false;
}

const AbstractDecoder::supportedType_t *ShortenDecoder::supportedTypes(void)
{
	static const char *exts[] =
	{
		"shn", NULL
	};

	static const supportedType_t s_supportedTypes[] =
	{
		{ "Shorten", exts },
		{ NULL, NULL }
	};

	return s_supportedTypes;
}
