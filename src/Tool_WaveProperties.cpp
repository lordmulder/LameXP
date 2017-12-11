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

#include "Tool_WaveProperties.h"

//Internal
#include "Global.h"
#include "Model_AudioFile.h"

//MUtils
#include <MUtils/Exception.h>

//Qt
#include <QDir>
#include <QProcess>

WaveProperties::WaveProperties(void)
:
	m_binary(lamexp_tools_lookup("sox.exe"))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing MP3 encoder. Tool 'lame.exe' is not registred!");
	}
}

WaveProperties::~WaveProperties(void)
{
}

bool WaveProperties::detect(const QString &sourceFile, AudioFileModel_TechInfo *info, QAtomicInt &abortFlag)
{
	QProcess process;
	QStringList args;

	args << "--i" << QDir::toNativeSeparators(sourceFile);

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	int progress = 0;

	QRegExp regExp_prc("Precision\\s*:\\s*(\\d+)-bit", Qt::CaseInsensitive);
	QRegExp regExp_srt("Sample Rate\\s*:\\s*(\\d+)", Qt::CaseInsensitive);
	QRegExp regExp_drt("Duration\\s*:\\s*(\\d\\d):(\\d\\d):(\\d\\d)\\.(\\d\\d)", Qt::CaseInsensitive);
	QRegExp regExp_chl("Channels\\s*:\\s*(\\d+)", Qt::CaseInsensitive);
	QRegExp regExp_enc("Sample Encoding\\s*:\\s*(\\d+)-bit\\s*Float", Qt::CaseInsensitive); //SoX returns a precision of 24-Bit for 32-Bit Float data, so we detect it this way!

	const result_t result = awaitProcess(process, abortFlag, [this, &info, &progress, &regExp_prc, &regExp_srt, &regExp_drt, &regExp_chl, &regExp_enc](const QString &text)
	{
		if (regExp_prc.lastIndexIn(text) >= 0)
		{
			quint32 tmp;
			if (MUtils::regexp_parse_uint32(regExp_prc, tmp))
			{
				info->setAudioBitdepth(tmp);
				emit statusUpdated(qMin(progress += 25, 100));
			}
			return true;
		}
		if (regExp_enc.lastIndexIn(text) >= 0)
		{
			quint32 tmp;
			if (MUtils::regexp_parse_uint32(regExp_enc, tmp))
			{
				info->setAudioBitdepth((tmp == 32) ? AudioFileModel::BITDEPTH_IEEE_FLOAT32 : tmp);
				emit statusUpdated(qMin(progress += 25, 100));
			}
			return true;
		}
		if (regExp_srt.lastIndexIn(text) >= 0)
		{
			quint32 tmp;
			if (MUtils::regexp_parse_uint32(regExp_srt, tmp))
			{
				info->setAudioSamplerate(tmp);
				emit statusUpdated(qMin(progress += 25, 100));
			}
			return true;
		}
		if (regExp_drt.lastIndexIn(text) >= 0)
		{
			quint32 tmp[4];
			if (MUtils::regexp_parse_uint32(regExp_drt, tmp, 4))
			{
				info->setDuration((tmp[0] * 3600) + (tmp[1] * 60) + tmp[2] + qRound(static_cast<double>(tmp[3]) / 100.0));
				emit statusUpdated(qMin(progress += 25, 100));
			}
			return true;
		}
		if (regExp_chl.lastIndexIn(text) >= 0)
		{
			quint32 tmp;
			if (MUtils::regexp_parse_uint32(regExp_chl, tmp))
			{
				info->setAudioChannels(tmp);
				emit statusUpdated(qMin(progress += 25, 100));
			}
			return true;
		}
		return false;
	});
	
	return (result == RESULT_SUCCESS);
}
