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

#include "Encoder_MAC.h"

#include "Global.h"
#include "Model_Settings.h"

#include <QProcess>
#include <QDir>

#define APPEND_TAG(NAME, VALUE) do \
{ \
	if (!buffer.isEmpty()) buffer += L1C('|'); \
	buffer += QString("%1=%2").arg((NAME), (VALUE)); \
} \
while(0)

///////////////////////////////////////////////////////////////////////////////
// Encoder Info
///////////////////////////////////////////////////////////////////////////////

class MACEncoderInfo : public AbstractEncoderInfo
{
	virtual bool isModeSupported(int mode) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
			return true;
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return false;
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
			return 5;
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return -1;
			break;
		default:
			MUTILS_THROW("Bad RC mode specified!");
		}
	}

	virtual int valueAt(int mode, int index) const
	{
		switch(mode)
		{
		case SettingsModel::VBRMode:
			return qBound(0, index + 1, 8);
			break;
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
			return TYPE_COMPRESSION_LEVEL;
			break;
		case SettingsModel::ABRMode:
		case SettingsModel::CBRMode:
			return -1;
			break;
		default:
			MUTILS_THROW("Bad RC mode specified!");
		}
	}

	virtual const char *description(void) const
	{
		static const char* s_description = "Monkey's Audio (MAC)";
		return s_description;
	}

	virtual const char *extension(void) const
	{
		static const char* s_extension = "ape";
		return s_extension;
	}

	virtual bool isResamplingSupported(void) const
	{
		return false;
	}
}
static const g_macEncoderInfo;

///////////////////////////////////////////////////////////////////////////////
// Encoder implementation
///////////////////////////////////////////////////////////////////////////////

MACEncoder::MACEncoder(void)
:
	m_binary(lamexp_tools_lookup(L1S("mac.exe")))
{
	if (m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing MAC encoder. Tool 'mac.exe' is not registred!");
	}
}

MACEncoder::~MACEncoder(void)
{
}

bool MACEncoder::encode(const QString &sourceFile, const AudioFileModel_MetaInfo &metaInfo, const unsigned int duration, const unsigned int channels, const QString &outputFile, QAtomicInt &abortFlag)
{
	QProcess process;
	QStringList args;
	
	const QString baseName = QFileInfo(outputFile).fileName();

	args << QDir::toNativeSeparators(sourceFile);
	args << QDir::toNativeSeparators(outputFile);

	switch(m_configRCMode)
	{
	case SettingsModel::VBRMode:
		args << QString().sprintf("-c%d", (m_configBitrate + 1) * 1000);
		break;
	default:
		MUTILS_THROW("Bad rate-control mode!");
		break;
	}

	if (!metaInfo.empty(true))
	{
		const QString apeTagsData = createApeTags(metaInfo);
		if (!apeTagsData.isEmpty())
		{
			args << "-t" << apeTagsData;
		}
	}

	if(!startProcess(process, m_binary, args))
	{
		return false;
	}

	int prevProgress = -1;
	QRegExp regExp(L1S("Progress: (\\d+).(\\d+)%"));

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

bool MACEncoder::isFormatSupported(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion)
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

const AbstractEncoderInfo *MACEncoder::getEncoderInfo(void)
{
	return &g_macEncoderInfo;
}

QString MACEncoder::createApeTags(const AudioFileModel_MetaInfo &metaInfo)
{
	QString buffer;

	if (!metaInfo.title().isEmpty())   APPEND_TAG(L1S("Title"),   cleanTag(metaInfo.title()));
	if (!metaInfo.artist().isEmpty())  APPEND_TAG(L1S("Artist"),  cleanTag(metaInfo.artist()));
	if (!metaInfo.album().isEmpty())   APPEND_TAG(L1S("Album"),   cleanTag(metaInfo.album()));
	if (!metaInfo.genre().isEmpty())   APPEND_TAG(L1S("Genre"),   cleanTag(metaInfo.genre()));
	if (!metaInfo.comment().isEmpty()) APPEND_TAG(L1S("Comment"), cleanTag(metaInfo.comment()));
	if (metaInfo.year())               APPEND_TAG(L1S("Year"),    QString::number(metaInfo.year()));
	if (metaInfo.position())           APPEND_TAG(L1S("Track"),   QString::number(metaInfo.position()));

	return buffer;
}
