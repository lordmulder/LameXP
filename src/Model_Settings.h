///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2011 LoRd_MuldeR <MuldeR2@GMX.de>
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

#pragma once

class QSettings;
class QString;

#define MAKE_OPTION_DEC1(OPT) \
int OPT(void); \
void OPT(int value); \
int OPT##Default(void);

#define MAKE_OPTION_DEC2(OPT) \
QString OPT(void); \
void OPT(const QString &value); \
QString OPT##Default(void);

#define MAKE_OPTION_DEC3(OPT) \
bool OPT(void); \
void OPT(bool value); \
bool OPT##Default(void);

class SettingsModel
{
public:
	SettingsModel(void);
	~SettingsModel(void);

	//Enums
	enum Encoder
	{
		MP3Encoder = 0,
		VorbisEncoder = 1,
		AACEncoder = 2,
		FLACEncoder = 3,
		PCMEncoder = 4
	};
	enum RCMode
	{
		VBRMode = 0,
		ABRMode = 1,
		CBRMode = 2
	};
	
	//Consts
	static const int mp3Bitrates[15];
	static const int samplingRates[8];

	//Getters & setters
	MAKE_OPTION_DEC1(licenseAccepted);
	MAKE_OPTION_DEC1(interfaceStyle);
	MAKE_OPTION_DEC1(compressionEncoder);
	MAKE_OPTION_DEC1(compressionRCMode);
	MAKE_OPTION_DEC1(compressionBitrate);
	MAKE_OPTION_DEC2(outputDir);
	MAKE_OPTION_DEC3(outputToSourceDir);
	MAKE_OPTION_DEC3(prependRelativeSourcePath);
	MAKE_OPTION_DEC3(writeMetaTags);
	MAKE_OPTION_DEC3(createPlaylist);
	MAKE_OPTION_DEC2(autoUpdateLastCheck);
	MAKE_OPTION_DEC3(autoUpdateEnabled);
	MAKE_OPTION_DEC3(soundsEnabled);
	MAKE_OPTION_DEC3(neroAacNotificationsEnabled);
	MAKE_OPTION_DEC3(wmaDecoderNotificationsEnabled);
	MAKE_OPTION_DEC3(dropBoxWidgetEnabled);
	MAKE_OPTION_DEC3(shellIntegrationEnabled);
	MAKE_OPTION_DEC2(currentLanguage);
	MAKE_OPTION_DEC1(lameAlgoQuality);
	MAKE_OPTION_DEC1(lameChannelMode);
	MAKE_OPTION_DEC3(bitrateManagementEnabled);
	MAKE_OPTION_DEC1(bitrateManagementMinRate);
	MAKE_OPTION_DEC1(bitrateManagementMaxRate);
	MAKE_OPTION_DEC1(samplingRate);
	MAKE_OPTION_DEC3(neroAACEnable2Pass);
	MAKE_OPTION_DEC1(neroAACProfile);
	MAKE_OPTION_DEC3(normalizationFilterEnabled);
	MAKE_OPTION_DEC1(normalizationFilterMaxVolume);
	MAKE_OPTION_DEC1(toneAdjustBass);
	MAKE_OPTION_DEC1(toneAdjustTreble);

	//Misc
	void validate(void);

private:
	QSettings *m_settings;
	QString *m_defaultLanguage;
	QString defaultLanguage(void);
};

#undef MAKE_OPTION_DEC1
#undef MAKE_OPTION_DEC2
#undef MAKE_OPTION_DEC3
