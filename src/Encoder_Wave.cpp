///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2013 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
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

#include "Global.h"
#include "Model_Settings.h"

#include <QDir>
#include <Shellapi.h>

#define FIX_SEPARATORS(STR) for(int i = 0; STR[i]; i++) { if(STR[i] == L'/') STR[i] = L'\\'; }

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
			throw "Bad RC mode specified!";
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
			throw "Bad RC mode specified!";
		}
	}

	virtual int valueAt(int mode, int index) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return -1;
			break;
		default:
			throw "Bad RC mode specified!";
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
			throw "Bad RC mode specified!";
		}
	}

	virtual const char *description(void) const
	{
		static const char* s_description = "Wave Audio (PCM)";
		return s_description;
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

bool WaveEncoder::encode(const QString &sourceFile, const AudioFileModel &metaInfo, const QString &outputFile, volatile bool *abortFlag)
{
	SHFILEOPSTRUCTW fileOperation;
	memset(&fileOperation, 0, sizeof(SHFILEOPSTRUCTW));
	fileOperation.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI | FOF_FILESONLY;

	emit messageLogged(QString("Copy file \"%1\" to \"%2\"").arg(sourceFile, outputFile));
	fileOperation.wFunc = FO_COPY;

	/*
	if(lamexp_temp_folder().compare(QFileInfo(sourceFile).canonicalPath(), Qt::CaseInsensitive) == 0)
	{
		//If the source is in the TEMP folder take shortcut and move the file
		emit messageLogged(QString("Moving file \"%1\" to \"%2\"").arg(sourceFile, outputFile));
		fileOperation.wFunc = FO_MOVE;
	}
	else
	{
		//...otherwise we actually copy the file in order to keep the source
		emit messageLogged(QString("Copy file \"%1\" to \"%2\"").arg(sourceFile, outputFile));
		fileOperation.wFunc = FO_COPY;
	}
	*/
	
	size_t srcLen = wcslen(reinterpret_cast<const wchar_t*>(sourceFile.utf16())) + 3;
	wchar_t *srcBuffer = new wchar_t[srcLen];
	memset(srcBuffer, 0, srcLen * sizeof(wchar_t));
	wcsncpy_s(srcBuffer, srcLen, reinterpret_cast<const wchar_t*>(sourceFile.utf16()), _TRUNCATE);
	FIX_SEPARATORS (srcBuffer);
	fileOperation.pFrom = srcBuffer;

	size_t outLen = wcslen(reinterpret_cast<const wchar_t*>(outputFile.utf16())) + 3;
	wchar_t *outBuffer = new wchar_t[outLen];
	memset(outBuffer, 0, outLen * sizeof(wchar_t));
	wcsncpy_s(outBuffer, outLen, reinterpret_cast<const wchar_t*>(outputFile.utf16()), _TRUNCATE);
	FIX_SEPARATORS (outBuffer);
	fileOperation.pTo = outBuffer;

	emit statusUpdated(0);
	int result = SHFileOperation(&fileOperation);
	emit statusUpdated(100);

	emit messageLogged(QString().sprintf("\nExited with code: 0x%04X", result));

	delete [] srcBuffer;
	delete [] outBuffer;

	return (result == 0 && fileOperation.fAnyOperationsAborted == false);
}

QString WaveEncoder::extension(void)
{
	return "wav";
}

bool WaveEncoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
{
	if(containerType.compare("Wave", Qt::CaseInsensitive) == 0)
	{
		if(formatType.compare("PCM", Qt::CaseInsensitive) == 0)
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
