///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2022 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Encoder_Wave.h"

#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>

#include "Global.h"
#include "Model_Settings.h"

typedef struct _callback_t
{
	WaveEncoder *const pInstance;
	QAtomicInt  *const abortFlag;
}
callback_t;

///////////////////////////////////////////////////////////////////////////////
// Encoder Info
///////////////////////////////////////////////////////////////////////////////

class WaveEncoderInfo : public AbstractEncoderInfo
{
public:
	virtual bool isModeSupported(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
		case SettingsModel::ABRMode:
			return false;
			break;
		case SettingsModel::CBRMode:
			return true;
			break;
		default:
			MUTILS_THROW("Bad RC mode specified!");
		}
	}

	virtual int valueCount(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return 0;
			break;
		default:
			MUTILS_THROW("Bad RC mode specified!");
		}
	}

	virtual int valueAt(int mode, int /*index*/) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return -1;
			break;
		default:
			MUTILS_THROW("Bad RC mode specified!");
		}
	}

	virtual int valueType(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return TYPE_UNCOMPRESSED;
			break;
		default:
			MUTILS_THROW("Bad RC mode specified!");
		}
	}

	virtual const char *description(void) const
	{
		static const char* s_description = "Wave Audio (PCM)";
		return s_description;
	}

	virtual const char *extension(void) const
	{
		static const char* s_extension = "wav";
		return s_extension;
	}

	virtual bool isResamplingSupported(void) const
	{
		return false;
	}
}
static const g_waveEncoderInfo;

///////////////////////////////////////////////////////////////////////////////
// Encoder implementation
///////////////////////////////////////////////////////////////////////////////


WaveEncoder::WaveEncoder(void)
{
}

WaveEncoder::~WaveEncoder(void)
{
}

bool WaveEncoder::encode(const QString &sourceFile, const AudioFileModel_MetaInfo& /*metaInfo*/, const unsigned int /*duration*/, const unsigned int /*channels*/, const QString &outputFile, QAtomicInt &abortFlag)
{
	emit messageLogged(QString("Copy file \"%1\" to \"%2\"\n").arg(sourceFile, outputFile));

	callback_t callbackData = { this, &abortFlag };

	emit statusUpdated(0);
	const bool success = MUtils::OS::copy_file(sourceFile, outputFile, true, progressCallback, &callbackData);
	emit statusUpdated(100);

	if (success)
	{
		emit messageLogged(L1S("File copied successfully."));
	}
	else
	{
		emit messageLogged(CHECK_FLAG(abortFlag) ? L1S("Operation cancelled by user!")  : L1S("Error: Failed to copy file!"));
	}

	return success;
}

bool WaveEncoder::progressCallback(const double &progress, void *const userData)
{
	if (const callback_t *const ptr = reinterpret_cast<callback_t*>(userData))
	{
		ptr->pInstance->updateProgress(progress);
		return ptr->abortFlag->operator!();
	}
	return true;
}

void WaveEncoder::updateProgress(const double &progress)
{
	emit statusUpdated(qRound(progress * 100.0));
}

bool WaveEncoder::isFormatSupported(const QString &containerType, const QString& /*containerProfile*/, const QString &formatType, const QString& /*formatProfile*/, const QString& /*formatVersion*/)
{
	if(containerType.compare(L1S("Wave"), Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare(L1S("PCM"), Qt::CaseInsensitive) == 0)
		{
			return true;
		}
	}
	return false;
}

const AbstractEncoderInfo *WaveEncoder::getEncoderInfo(void)
{
	return &g_waveEncoderInfo;
}
