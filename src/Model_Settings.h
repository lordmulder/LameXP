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

#pragma once

#include <QMutex>
#include <QScopedPointer>

class QString;
class SettingsCache;

///////////////////////////////////////////////////////////////////////////////

#define LAMEXP_MAKE_OPTION_I(OPT) \
qint32 OPT(void) const; \
void OPT(const qint32 &value); \
qint32 OPT##Default(void);

#define LAMEXP_MAKE_OPTION_S(OPT) \
QString OPT(void) const; \
void OPT(const QString &value); \
QString OPT##Default(void);

#define LAMEXP_MAKE_OPTION_B(OPT) \
bool OPT(void) const; \
void OPT(bool value); \
bool OPT##Default(void);

#define LAMEXP_MAKE_OPTION_U(OPT) \
quint32 OPT(void) const; \
void OPT(const quint32 &value); \
quint32 OPT##Default(void);

///////////////////////////////////////////////////////////////////////////////

class SettingsModel
{
public:
	SettingsModel(void);
	~SettingsModel(void);

	//Enums
	enum Encoder
	{
		MP3Encoder    = 0,
		VorbisEncoder = 1,
		AACEncoder    = 2,
		AC3Encoder    = 3,
		FLACEncoder   = 4,
		OpusEncoder   = 5,
		DCAEncoder    = 6,
		MACEncoder    = 7,
		PCMEncoder    = 8,
		ENCODER_COUNT = 9
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
	
	enum AACEncoderType
	{
		AAC_ENCODER_NONE = 0,
		AAC_ENCODER_NERO = 1,
		AAC_ENCODER_FHG  = 2,
		AAC_ENCODER_FDK  = 3,
		AAC_ENCODER_QAAC = 4,
	};

	//Consts
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
	LAMEXP_MAKE_OPTION_I(compressionAbrBitrateAacEnc)
	LAMEXP_MAKE_OPTION_I(compressionAbrBitrateAften)
	LAMEXP_MAKE_OPTION_I(compressionAbrBitrateDcaEnc)
	LAMEXP_MAKE_OPTION_I(compressionAbrBitrateFLAC)
	LAMEXP_MAKE_OPTION_I(compressionAbrBitrateLAME)
	LAMEXP_MAKE_OPTION_I(compressionAbrBitrateMacEnc)
	LAMEXP_MAKE_OPTION_I(compressionAbrBitrateOggEnc)
	LAMEXP_MAKE_OPTION_I(compressionAbrBitrateOpusEnc)
	LAMEXP_MAKE_OPTION_I(compressionAbrBitrateWave)
	LAMEXP_MAKE_OPTION_I(compressionCbrBitrateAacEnc)
	LAMEXP_MAKE_OPTION_I(compressionCbrBitrateAften)
	LAMEXP_MAKE_OPTION_I(compressionCbrBitrateDcaEnc)
	LAMEXP_MAKE_OPTION_I(compressionCbrBitrateFLAC)
	LAMEXP_MAKE_OPTION_I(compressionCbrBitrateLAME)
	LAMEXP_MAKE_OPTION_I(compressionCbrBitrateMacEnc)
	LAMEXP_MAKE_OPTION_I(compressionCbrBitrateOggEnc)
	LAMEXP_MAKE_OPTION_I(compressionCbrBitrateOpusEnc)
	LAMEXP_MAKE_OPTION_I(compressionCbrBitrateWave)
	LAMEXP_MAKE_OPTION_I(compressionEncoder)
	LAMEXP_MAKE_OPTION_I(compressionRCModeAacEnc)
	LAMEXP_MAKE_OPTION_I(compressionRCModeAften)
	LAMEXP_MAKE_OPTION_I(compressionRCModeDcaEnc)
	LAMEXP_MAKE_OPTION_I(compressionRCModeFLAC)
	LAMEXP_MAKE_OPTION_I(compressionRCModeLAME)
	LAMEXP_MAKE_OPTION_I(compressionRCModeMacEnc)
	LAMEXP_MAKE_OPTION_I(compressionRCModeOggEnc)
	LAMEXP_MAKE_OPTION_I(compressionRCModeOpusEnc)
	LAMEXP_MAKE_OPTION_I(compressionRCModeWave)
	LAMEXP_MAKE_OPTION_I(compressionVbrQualityAacEnc)
	LAMEXP_MAKE_OPTION_I(compressionVbrQualityAften)
	LAMEXP_MAKE_OPTION_I(compressionVbrQualityDcaEnc)
	LAMEXP_MAKE_OPTION_I(compressionVbrQualityFLAC)
	LAMEXP_MAKE_OPTION_I(compressionVbrQualityLAME)
	LAMEXP_MAKE_OPTION_I(compressionVbrQualityMacEnc)
	LAMEXP_MAKE_OPTION_I(compressionVbrQualityOggEnc)
	LAMEXP_MAKE_OPTION_I(compressionVbrQualityOpusEnc)
	LAMEXP_MAKE_OPTION_I(compressionVbrQualityWave)
	LAMEXP_MAKE_OPTION_B(createPlaylist)
	LAMEXP_MAKE_OPTION_S(currentLanguage)
	LAMEXP_MAKE_OPTION_S(currentLanguageFile)
	LAMEXP_MAKE_OPTION_S(customParametersAacEnc)
	LAMEXP_MAKE_OPTION_S(customParametersAften)
	LAMEXP_MAKE_OPTION_S(customParametersDcaEnc)
	LAMEXP_MAKE_OPTION_S(customParametersFLAC)
	LAMEXP_MAKE_OPTION_S(customParametersLAME)
	LAMEXP_MAKE_OPTION_S(customParametersMacEnc)
	LAMEXP_MAKE_OPTION_S(customParametersOggEnc)
	LAMEXP_MAKE_OPTION_S(customParametersOpusEnc)
	LAMEXP_MAKE_OPTION_S(customParametersWave)
	LAMEXP_MAKE_OPTION_S(customTempPath)
	LAMEXP_MAKE_OPTION_B(customTempPathEnabled)
	LAMEXP_MAKE_OPTION_B(dropBoxWidgetEnabled)
	LAMEXP_MAKE_OPTION_I(dropBoxWidgetPositionX)
	LAMEXP_MAKE_OPTION_I(dropBoxWidgetPositionY)
	LAMEXP_MAKE_OPTION_S(favoriteOutputFolders)
	LAMEXP_MAKE_OPTION_B(forceStereoDownmix)
	LAMEXP_MAKE_OPTION_B(hibernateComputer)
	LAMEXP_MAKE_OPTION_I(interfaceStyle)
	LAMEXP_MAKE_OPTION_B(keepOriginalDataTime)
	LAMEXP_MAKE_OPTION_I(lameAlgoQuality)
	LAMEXP_MAKE_OPTION_I(lameChannelMode)
	LAMEXP_MAKE_OPTION_I(licenseAccepted)
	LAMEXP_MAKE_OPTION_U(maximumInstances)
	LAMEXP_MAKE_OPTION_U(metaInfoPosition)
	LAMEXP_MAKE_OPTION_S(mostRecentInputPath)
	LAMEXP_MAKE_OPTION_B(neroAACEnable2Pass)
	LAMEXP_MAKE_OPTION_B(neroAacNotificationsEnabled)
	LAMEXP_MAKE_OPTION_B(normalizationFilterEnabled)
	LAMEXP_MAKE_OPTION_B(normalizationFilterDynamic)
	LAMEXP_MAKE_OPTION_B(normalizationFilterCoupled)
	LAMEXP_MAKE_OPTION_I(normalizationFilterMaxVolume)
	LAMEXP_MAKE_OPTION_I(normalizationFilterSize)
	LAMEXP_MAKE_OPTION_I(opusComplexity)
	LAMEXP_MAKE_OPTION_B(opusDisableResample)
	LAMEXP_MAKE_OPTION_I(opusFramesize)
	LAMEXP_MAKE_OPTION_I(opusOptimizeFor)
	LAMEXP_MAKE_OPTION_S(outputDir)
	LAMEXP_MAKE_OPTION_B(outputToSourceDir)
	LAMEXP_MAKE_OPTION_I(overwriteMode)
	LAMEXP_MAKE_OPTION_B(prependRelativeSourcePath)
	LAMEXP_MAKE_OPTION_B(renameFiles_regExpEnabled)
	LAMEXP_MAKE_OPTION_S(renameFiles_regExpSearch)
	LAMEXP_MAKE_OPTION_S(renameFiles_regExpReplace)
	LAMEXP_MAKE_OPTION_B(renameFiles_renameEnabled)
	LAMEXP_MAKE_OPTION_S(renameFiles_renamePattern)
	LAMEXP_MAKE_OPTION_S(renameFiles_fileExtension)
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
	SettingsModel(const SettingsModel &other) {}
	SettingsModel &operator=(const SettingsModel &other) { return *this; }

	QString initDirectory(const QString &path) const;
	QString defaultLanguage(void) const;
	QString defaultDirectory(void) const;

	SettingsCache *m_configCache;

	mutable QMutex                  m_defaultLangLock;
	mutable QScopedPointer<QString> m_defaultLanguage;
};

///////////////////////////////////////////////////////////////////////////////

#undef LAMEXP_MAKE_OPTION_I
#undef LAMEXP_MAKE_OPTION_S
#undef LAMEXP_MAKE_OPTION_B
#undef LAMEXP_MAKE_OPTION_U
