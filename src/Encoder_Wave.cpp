///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2010 LoRd_MuldeR <MuldeR2@GMX.de>
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

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define IS_UNICODE(STR) (qstricmp(STR.toUtf8().constData(), QString::fromLocal8Bit(STR.toLocal8Bit()).toUtf8().constData()))
#define FIX_SEPARATORS(STR) for(int i = 0; STR[i]; i++) { if(STR[i] == L'/') STR[i] = L'\\'; }

WaveEncoder::WaveEncoder(void)
{
}

WaveEncoder::~WaveEncoder(void)
{
}

bool WaveEncoder::encode(const QString &sourceFile, const AudioFileModel &metaInfo, const QString &outputFile, volatile bool *abortFlag)
{
	emit messageLogged(QString("Moving file \"%1\" to \"%2\"").arg(sourceFile, outputFile));
	
	SHFILEOPSTRUCTW fileOperation;
	memset(&fileOperation, 0, sizeof(SHFILEOPSTRUCTW));
	fileOperation.wFunc = FO_MOVE;
	fileOperation.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR | FOF_NOERRORUI | FOF_FILESONLY;

	size_t srcLen = wcslen(reinterpret_cast<const wchar_t*>(sourceFile.utf16())) + 3;
	wchar_t *srcBuffer = new wchar_t[srcLen];
	memset(srcBuffer, 0, srcLen * sizeof(wchar_t));
	wcscpy_s(srcBuffer, srcLen, reinterpret_cast<const wchar_t*>(sourceFile.utf16()));
	FIX_SEPARATORS (srcBuffer);
	fileOperation.pFrom = srcBuffer;

	size_t outLen = wcslen(reinterpret_cast<const wchar_t*>(outputFile.utf16())) + 3;
	wchar_t *outBuffer = new wchar_t[outLen];
	memset(outBuffer, 0, outLen * sizeof(wchar_t));
	wcscpy_s(outBuffer, outLen, reinterpret_cast<const wchar_t*>(outputFile.utf16()));
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
