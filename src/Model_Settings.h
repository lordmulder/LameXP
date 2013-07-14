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

#pragma once

class QString;
class SettingsCache;

///////////////////////////////////////////////////////////////////////////////

#define LAMEXP_MAKE_OPTION_I(OPT) \
int OPT(void) const; \
void OPT(int value); \
int OPT##Default(void);

#define LAMEXP_MAKE_OPTION_S(OPT) \
QString OPT(void) const; \
void OPT(const QString &value); \
QString OPT##Default(void);

#define LAMEXP_MAKE_OPTION_B(OPT) \
bool OPT(void) const; \
void OPT(bool value); \
bool OPT##Default(void);

#define LAMEXP_MAKE_OPTION_U(OPT) \
unsigned int OPT(void) const; \
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
		PCMEncoder = 7,
		ENCODER_COUNT = 8
	};

	enum RCMode
	{
		VBRMode = 0,
		ABRMode = 1,
		CBRMode = 2,
		RCMODE_COUNT = 3
	};

	enum Overwrite
	{
		Overwrite_KeepBoth = 0,
		Overwrite_SkipFile = 1,
		Overwrite_Replaces = 2
	};
	
	//Consts
	static const int mp3Bitrates[15];
	static const int ac3Bitrates[20];
	static const int samplingRates[8];

	//Getters & setters
	LAMEXP_MAKE_OPTION_I(aacEncProfile)
	LAMEXP_MAKE_OPTION_I(aftenAudioCodingMode)
	LAMEXP_MAKE_OPTION_I(aftenDynamicRangeCompression)
	LAMEXP_MAKE_OPTION_I(aftenExponentSearchSize)
	LAMEXP_MAKE_OPTION_B(aftenFastBitAllocation)
	LAMEXP_MAKE_OPTION_B(antivirNotificationsEnabled)
	LAMEXP_MAKE_OPTION_B(autoUpdateCheckBeta)
	LAMEXP_MAKE_OPTION_B(autoUpdateEnabled)
	LAMEXP_MAKE_OPTION_S(autoUpdateLastCheck)
	LAMEXP_MAKE_OPTION_B(bitrateManagementEnabled)
	LAMEXP_MAKE_OPTION_I(bitrateManagementMaxRate)
	LAMEXP_MAKE_OPTION_I(bitrateManagementMinRate)
	LAMEXP_MAKE_OPTION_I(compressionBitrateAacEnc)
	LAMEXP_MAKE_OPTION_I(compressionBitrateAften)
	LAMEXP_MAKE_OPTION_I(compressionBitrateDcaEnc)
	LAMEXP_MAKE_OPTION_I(compressionBitrateLAME)
	LAMEXP_MAKE_OPTION_I(compressionBitrateOggEnc)
	LAMEXP_MAKE_OPTION_I(compressionBitrateOpusEnc)
	LAMEXP_MAKE_OPTION_I(compressionEncoder)
	LAMEXP_MAKE_OPTION_I(compressionRCModeAacEnc)
	LAMEXP_MAKE_OPTION_I(compressionRCModeAften)
	LAMEXP_MAKE_OPTION_I(compressionRCModeLAME)
	LAMEXP_MAKE_OPTION_I(compressionRCModeOggEnc)
	LAMEXP_MAKE_OPTION_I(compressionRCModeOpusEnc)
	LAMEXP_MAKE_OPTION_I(compressionVbrLevelAacEnc)
	LAMEXP_MAKE_OPTION_I(compressionVbrLevelAften)
	LAMEXP_MAKE_OPTION_I(compressionVbrLevelFLAC)
	LAMEXP_MAKE_OPTION_I(compressionVbrLevelLAME)
	LAMEXP_MAKE_OPTION_I(compressionVbrLevelOggEnc)
	LAMEXP_MAKE_OPTION_B(createPlaylist)
	LAMEXP_MAKE_OPTION_S(currentLanguage)
	LAMEXP_MAKE_OPTION_S(currentLanguageFile)
	LAMEXP_MAKE_OPTION_S(customParametersAacEnc)
	LAMEXP_MAKE_OPTION_S(customParametersAften)
	LAMEXP_MAKE_OPTION_S(customParametersFLAC)
	LAMEXP_MAKE_OPTION_S(customParametersLAME)
	LAMEXP_MAKE_OPTION_S(customParametersOggEnc)
	LAMEXP_MAKE_OPTION_S(customParametersOpus)
	LAMEXP_MAKE_OPTION_S(customTempPath)
	LAMEXP_MAKE_OPTION_B(customTempPathEnabled)
	LAMEXP_MAKE_OPTION_B(dropBoxWidgetEnabled)
	LAMEXP_MAKE_OPTION_S(favoriteOutputFolders)
	LAMEXP_MAKE_OPTION_B(forceStereoDownmix)
	LAMEXP_MAKE_OPTION_B(hibernateComputer)
	LAMEXP_MAKE_OPTION_I(interfaceStyle)
	LAMEXP_MAKE_OPTION_I(lameAlgoQuality)
	LAMEXP_MAKE_OPTION_I(lameChannelMode)
	LAMEXP_MAKE_OPTION_I(licenseAccepted)
	LAMEXP_MAKE_OPTION_U(maximumInstances)
	LAMEXP_MAKE_OPTION_U(metaInfoPosition)
	LAMEXP_MAKE_OPTION_S(mostRecentInputPath)
	LAMEXP_MAKE_OPTION_B(neroAACEnable2Pass)
	LAMEXP_MAKE_OPTION_B(neroAacNotificationsEnabled)
	LAMEXP_MAKE_OPTION_B(normalizationFilterEnabled)
	LAMEXP_MAKE_OPTION_I(normalizationFilterEqualizationMode)
	LAMEXP_MAKE_OPTION_I(normalizationFilterMaxVolume)
	LAMEXP_MAKE_OPTION_I(opusComplexity)
	LAMEXP_MAKE_OPTION_B(opusDisableResample)
	LAMEXP_MAKE_OPTION_I(opusFramesize)
	LAMEXP_MAKE_OPTION_I(opusOptimizeFor)
	LAMEXP_MAKE_OPTION_S(outputDir)
	LAMEXP_MAKE_OPTION_B(outputToSourceDir)
	LAMEXP_MAKE_OPTION_I(overwriteMode)
	LAMEXP_MAKE_OPTION_B(prependRelativeSourcePath)
	LAMEXP_MAKE_OPTION_B(renameOutputFilesEnabled)
	LAMEXP_MAKE_OPTION_S(renameOutputFilesPattern)
	LAMEXP_MAKE_OPTION_I(samplingRate)
	LAMEXP_MAKE_OPTION_B(shellIntegrationEnabled)
	LAMEXP_MAKE_OPTION_B(slowStartup)
	LAMEXP_MAKE_OPTION_B(soundsEnabled)
	LAMEXP_MAKE_OPTION_I(toneAdjustBass)
	LAMEXP_MAKE_OPTION_I(toneAdjustTreble)
	LAMEXP_MAKE_OPTION_B(writeMetaTags)

	//Misc
	void validate(void);
	void syncNow(void);
	
private:
	SettingsCache *m_configCache;

	QString initDirectory(const QString &path) const;
	QString defaultLanguage(void) const;
	QString defaultDirectory(void) const;

	static QString *m_defaultLanguage;
};

///////////////////////////////////////////////////////////////////////////////

#undef LAMEXP_MAKE_OPTION_I
#undef LAMEXP_MAKE_OPTION_S
#undef LAMEXP_MAKE_OPTION_B
#undef LAMEXP_MAKE_OPTION_U
