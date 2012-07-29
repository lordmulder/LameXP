///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2012 LoRd_MuldeR <MuldeR2@GMX.de>
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

///////////////////////////////////////////////////////////////////////////////

#define LAMEXP_MAKE_OPTION_I(OPT) \
int OPT(void); \
void OPT(int value); \
int OPT##Default(void);

#define LAMEXP_MAKE_OPTION_S(OPT) \
QString OPT(void); \
void OPT(const QString &value); \
QString OPT##Default(void);

#define LAMEXP_MAKE_OPTION_B(OPT) \
bool OPT(void); \
void OPT(bool value); \
bool OPT##Default(void);

#define LAMEXP_MAKE_OPTION_U(OPT) \
unsigned int OPT(void); \
void OPT(unsigned int value); \
unsigned int OPT##Default(void);

///////////////////////////////////////////////////////////////////////////////

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
		AC3Encoder = 3,
		FLACEncoder = 4,
		OpusEncoder = 5,
		DCAEncoder = 6,
		PCMEncoder = 7
	};
	enum RCMode
	{
		VBRMode = 0,
		ABRMode = 1,
		CBRMode = 2
	};
	
	//Consts
	static const int mp3Bitrates[15];
	static const int ac3Bitrates[20];
	static const int samplingRates[8];

	//Getters & setters
	LAMEXP_MAKE_OPTION_I(licenseAccepted);
	LAMEXP_MAKE_OPTION_I(interfaceStyle);
	LAMEXP_MAKE_OPTION_I(compressionEncoder);
	LAMEXP_MAKE_OPTION_I(compressionRCMode);
	LAMEXP_MAKE_OPTION_I(compressionBitrate);
	LAMEXP_MAKE_OPTION_S(outputDir);
	LAMEXP_MAKE_OPTION_B(outputToSourceDir);
	LAMEXP_MAKE_OPTION_B(prependRelativeSourcePath);
	LAMEXP_MAKE_OPTION_S(favoriteOutputFolders);
	LAMEXP_MAKE_OPTION_B(writeMetaTags);
	LAMEXP_MAKE_OPTION_B(createPlaylist);
	LAMEXP_MAKE_OPTION_S(autoUpdateLastCheck);
	LAMEXP_MAKE_OPTION_B(autoUpdateEnabled);
	LAMEXP_MAKE_OPTION_B(autoUpdateCheckBeta);
	LAMEXP_MAKE_OPTION_B(soundsEnabled);
	LAMEXP_MAKE_OPTION_B(neroAacNotificationsEnabled);
	LAMEXP_MAKE_OPTION_B(antivirNotificationsEnabled);
	LAMEXP_MAKE_OPTION_B(dropBoxWidgetEnabled);
	LAMEXP_MAKE_OPTION_B(shellIntegrationEnabled);
	LAMEXP_MAKE_OPTION_S(currentLanguage);
	LAMEXP_MAKE_OPTION_I(lameAlgoQuality);
	LAMEXP_MAKE_OPTION_I(lameChannelMode);
	LAMEXP_MAKE_OPTION_B(forceStereoDownmix);
	LAMEXP_MAKE_OPTION_B(bitrateManagementEnabled);
	LAMEXP_MAKE_OPTION_I(bitrateManagementMinRate);
	LAMEXP_MAKE_OPTION_I(bitrateManagementMaxRate);
	LAMEXP_MAKE_OPTION_I(samplingRate);
	LAMEXP_MAKE_OPTION_B(neroAACEnable2Pass);
	LAMEXP_MAKE_OPTION_I(aacEncProfile);
	LAMEXP_MAKE_OPTION_I(aftenAudioCodingMode);
	LAMEXP_MAKE_OPTION_I(aftenDynamicRangeCompression);
	LAMEXP_MAKE_OPTION_B(aftenFastBitAllocation);
	LAMEXP_MAKE_OPTION_I(aftenExponentSearchSize);
	LAMEXP_MAKE_OPTION_I(opusOptimizeFor);
	LAMEXP_MAKE_OPTION_I(opusComplexity);
	LAMEXP_MAKE_OPTION_I(opusFramesize);
	LAMEXP_MAKE_OPTION_B(opusExpAnalysis);
	LAMEXP_MAKE_OPTION_B(normalizationFilterEnabled);
	LAMEXP_MAKE_OPTION_I(normalizationFilterMaxVolume);
	LAMEXP_MAKE_OPTION_I(normalizationFilterEqualizationMode);
	LAMEXP_MAKE_OPTION_I(toneAdjustBass);
	LAMEXP_MAKE_OPTION_I(toneAdjustTreble);
	LAMEXP_MAKE_OPTION_S(customParametersLAME);
	LAMEXP_MAKE_OPTION_S(customParametersOggEnc);
	LAMEXP_MAKE_OPTION_S(customParametersAacEnc);
	LAMEXP_MAKE_OPTION_S(customParametersAften);
	LAMEXP_MAKE_OPTION_S(customParametersFLAC);
	LAMEXP_MAKE_OPTION_S(customParametersOpus);
	LAMEXP_MAKE_OPTION_B(renameOutputFilesEnabled);
	LAMEXP_MAKE_OPTION_S(renameOutputFilesPattern);
	LAMEXP_MAKE_OPTION_U(metaInfoPosition);
	LAMEXP_MAKE_OPTION_U(maximumInstances);
	LAMEXP_MAKE_OPTION_S(customTempPath);
	LAMEXP_MAKE_OPTION_B(customTempPathEnabled);
	LAMEXP_MAKE_OPTION_B(slowStartup);
	LAMEXP_MAKE_OPTION_S(mostRecentInputPath);
	LAMEXP_MAKE_OPTION_B(hibernateComputer);

	//Misc
	void validate(void);
	
private:
	QSettings *m_settings;
	QString *m_defaultLanguage;
	QString defaultLanguage(void);
	QString initDirectory(const QString &path);
};

///////////////////////////////////////////////////////////////////////////////

#undef LAMEXP_MAKE_OPTION_I
#undef LAMEXP_MAKE_OPTION_S
#undef LAMEXP_MAKE_OPTION_B
#undef LAMEXP_MAKE_OPTION_U
