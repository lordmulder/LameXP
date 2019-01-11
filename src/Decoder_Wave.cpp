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

//Internal
#include "Decoder_Wave.h"
#include "Global.h"

//MUtils
#include <MUtils/OSSupport.h>

//Qt
#include <QDir>

//Type
typedef struct _callback_t
{
	WaveDecoder *const pInstance;
	QAtomicInt  *const abortFlag;
}
callback_t;

WaveDecoder::WaveDecoder(void)
{
}

WaveDecoder::~WaveDecoder(void)
{
}

bool WaveDecoder::decode(const QString &sourceFile, const QString &outputFile, QAtomicInt &abortFlag)
{
	emit messageLogged(QString("Copy file \"%1\" to \"%2\"").arg(QDir::toNativeSeparators(sourceFile), QDir::toNativeSeparators(outputFile)));
	emit statusUpdated(0);

	callback_t progressData = { this, &abortFlag };
	const bool okay = MUtils::OS::copy_file(sourceFile, outputFile, true, progressHandler, &progressData);
	
	emit statusUpdated(100);

	if(okay)
	{
		emit messageLogged("File copied successfully.");
	}
	else
	{
		emit messageLogged("Failed to copy file!");
	}

	return okay;
}

bool WaveDecoder::progressHandler(const double &progress, void *const userData)
{
	if(const callback_t *const ptr = reinterpret_cast<callback_t*>(userData))
	{
		ptr->pInstance->updateProgress(progress);
		return ptr->abortFlag->operator!();
	}
	return true;
}

void WaveDecoder::updateProgress(const double &progress)
{
	emit statusUpdated(qBound(0, qRound(progress * 100.0), 100));
}

bool WaveDecoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	if(containerType.compare(QLatin1String("Wave"), Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare(QLatin1String("PCM"), Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}

	return false;
}

const AbstractDecoder::supportedType_t *WaveDecoder::supportedTypes(void)
{
	static const char *exts[] =
	{
		"wav", NULL
	};

	static const supportedType_t s_supportedTypes[] =
	{
		{ "Waveform Audio File", exts },
		{ NULL, NULL }
	};

	return s_supportedTypes;
}
