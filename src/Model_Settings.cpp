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

#include "Model_Settings.h"

#include "Global.h"

#include <QSettings>
#include <QDesktopServices>
#include <QApplication>
#include <QString>
#include <QFileInfo>
#include <QDir>
#include <QStringList>
#include <QLocale>


////////////////////////////////////////////////////////////
//Macros
////////////////////////////////////////////////////////////

#define MAKE_OPTION1(OPT,DEF) \
int SettingsModel::OPT(void) { return m_settings->value(g_settingsId_##OPT, DEF).toInt(); } \
void SettingsModel::OPT(int value) { m_settings->setValue(g_settingsId_##OPT, value); } \
int SettingsModel::OPT##Default(void) { return DEF; }

#define MAKE_OPTION2(OPT,DEF) \
QString SettingsModel::OPT(void) { return m_settings->value(g_settingsId_##OPT, DEF).toString().trimmed(); } \
void SettingsModel::OPT(const QString &value) { m_settings->setValue(g_settingsId_##OPT, value); } \
QString SettingsModel::OPT##Default(void) { return DEF; }

#define MAKE_OPTION3(OPT,DEF) \
bool SettingsModel::OPT(void) { return m_settings->value(g_settingsId_##OPT, DEF).toBool(); } \
void SettingsModel::OPT(bool value) { m_settings->setValue(g_settingsId_##OPT, value); } \
bool SettingsModel::OPT##Default(void) { return DEF; }

#define MAKE_OPTION4(OPT,DEF) \
unsigned int SettingsModel::OPT(void) { return m_settings->value(g_settingsId_##OPT, DEF).toUInt(); } \
void SettingsModel::OPT(unsigned int value) { m_settings->setValue(g_settingsId_##OPT, value); } \
unsigned int SettingsModel::OPT##Default(void) { return DEF; }

#define MAKE_ID(DEC,STR) static const char *g_settingsId_##DEC = STR

////////////////////////////////////////////////////////////
//Constants
////////////////////////////////////////////////////////////

//Setting ID's
MAKE_ID(versionNumber, "VersionNumber");
MAKE_ID(licenseAccepted, "LicenseAccepted");
MAKE_ID(interfaceStyle, "InterfaceStyle");
MAKE_ID(compressionEncoder, "Compression/Encoder");
MAKE_ID(compressionRCMode, "Compression/RCMode");
MAKE_ID(compressionBitrate, "Compression/Bitrate");
MAKE_ID(outputDir, "OutputDirectory/SelectedPath");
MAKE_ID(outputToSourceDir, "OutputDirectory/OutputToSourceFolder");
MAKE_ID(prependRelativeSourcePath, "OutputDirectory/PrependRelativeSourcePath");
MAKE_ID(writeMetaTags, "Flags/WriteMetaTags");
MAKE_ID(createPlaylist, "Flags/AutoCreatePlaylist");
MAKE_ID(autoUpdateLastCheck, "AutoUpdate/LastCheck");
MAKE_ID(autoUpdateEnabled, "AutoUpdate/Enabled");
MAKE_ID(soundsEnabled, "Flags/EnableSounds");
MAKE_ID(neroAacNotificationsEnabled, "Flags/EnableNeroAacNotifications");
MAKE_ID(wmaDecoderNotificationsEnabled, "Flags/EnableWmaDecoderNotifications");
MAKE_ID(dropBoxWidgetEnabled, "Flags/EnableDropBoxWidget");
MAKE_ID(shellIntegrationEnabled, "Flags/EnableShellIntegration");
MAKE_ID(currentLanguage, "Localization/Language");
MAKE_ID(lameAlgoQuality, "AdvancedOptions/LAME/AlgorithmQuality");
MAKE_ID(lameChannelMode, "AdvancedOptions/LAME/ChannelMode");
MAKE_ID(bitrateManagementEnabled, "AdvancedOptions/BitrateManagement/Enabled");
MAKE_ID(bitrateManagementMinRate, "AdvancedOptions/BitrateManagement/MinRate");
MAKE_ID(bitrateManagementMaxRate, "AdvancedOptions/BitrateManagement/MaxRate");
MAKE_ID(samplingRate, "AdvancedOptions/Common/Resampling");
MAKE_ID(neroAACEnable2Pass, "AdvancedOptions/NeroAAC/Enable2Pass");
MAKE_ID(neroAACProfile, "AdvancedOptions/NeroAAC/ForceProfile");
MAKE_ID(normalizationFilterEnabled, "AdvancedOptions/VolumeNormalization/Enabled");
MAKE_ID(normalizationFilterMaxVolume, "AdvancedOptions/VolumeNormalization/MaxVolume");
MAKE_ID(toneAdjustBass, "AdvancedOptions/ToneAdjustment/Bass");
MAKE_ID(toneAdjustTreble, "AdvancedOptions/ToneAdjustment/Treble");
MAKE_ID(customParametersLAME, "AdvancedOptions/CustomParameters/LAME");
MAKE_ID(customParametersOggEnc, "AdvancedOptions/CustomParameters/OggEnc");
MAKE_ID(customParametersNeroAAC, "AdvancedOptions/CustomParameters/NeroAAC");
MAKE_ID(customParametersFLAC, "AdvancedOptions/CustomParameters/FLAC");
MAKE_ID(metaInfoPosition, "MetaInformation/PlaylistPosition");
MAKE_ID(maximumInstances, "AdvancedOptions/Threading/MaximumInstances");

//LUT
const int SettingsModel::mp3Bitrates[15] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1};
const int SettingsModel::samplingRates[8] = {0, 16000, 22050, 24000, 32000, 44100, 48000, -1};

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

SettingsModel::SettingsModel(void)
:
	m_defaultLanguage(NULL)
{
	QString configPath;
	
	if(!lamexp_portable_mode())
	{
		QString dataPath = QDir(QDesktopServices::storageLocation(QDesktopServices::DataLocation)).canonicalPath();
		if(dataPath.isEmpty()) dataPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
		QDir(dataPath).mkpath(".");
		configPath = QString("%1/config.ini").arg(dataPath);
	}
	else
	{
		qDebug("LameXP is running in \"portable\" mode -> config in application dir!\n");
		QString appPath = QFileInfo(QApplication::applicationFilePath()).canonicalFilePath();
		if(appPath.isEmpty()) appPath = QApplication::applicationFilePath();
		configPath = QString("%1/%2.ini").arg(QFileInfo(appPath).absolutePath(), QFileInfo(appPath).completeBaseName());
	}

	m_settings = new QSettings(configPath, QSettings::IniFormat);
	m_settings->beginGroup(QString().sprintf("LameXP_%u%02u%05u", lamexp_version_major(), lamexp_version_minor(), lamexp_version_build()));
	m_settings->setValue(g_settingsId_versionNumber, QApplication::applicationVersion());
	m_settings->sync();
}

////////////////////////////////////////////////////////////
// Destructor
////////////////////////////////////////////////////////////

SettingsModel::~SettingsModel(void)
{
	LAMEXP_DELETE(m_settings);
	LAMEXP_DELETE(m_defaultLanguage);
}

////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////

void SettingsModel::validate(void)
{
	if(this->compressionEncoder() < SettingsModel::MP3Encoder || this->compressionEncoder() > SettingsModel::PCMEncoder)
	{
		this->compressionEncoder(SettingsModel::MP3Encoder);
	}
	
	if(this->compressionRCMode() < SettingsModel::VBRMode || this->compressionRCMode() > SettingsModel::CBRMode)
	{
		this->compressionEncoder(SettingsModel::VBRMode);
	}
	
	if(!(lamexp_check_tool("neroAacEnc.exe") && lamexp_check_tool("neroAacDec.exe") && lamexp_check_tool("neroAacTag.exe")))
	{
		if(this->compressionEncoder() == SettingsModel::AACEncoder)
		{
			qWarning("Nero encoder selected, but not available any more. Reverting to MP3!");
			this->compressionEncoder(SettingsModel::MP3Encoder);
		}
	}
	
	if(this->outputDir().isEmpty() || !QFileInfo(this->outputDir()).isDir())
	{
		this->outputDir(QDesktopServices::storageLocation(QDesktopServices::MusicLocation));
	}

	if(!lamexp_query_translations().contains(this->currentLanguage(), Qt::CaseInsensitive))
	{
		qWarning("Current language \"%s\" is unknown, reverting to default language!", this->currentLanguage().toLatin1().constData());
			this->currentLanguage(defaultLanguage());
	}
}

////////////////////////////////////////////////////////////
// Private Functions
////////////////////////////////////////////////////////////

QString SettingsModel::defaultLanguage(void)
{
	if(m_defaultLanguage)
	{
		return *m_defaultLanguage;
	}
	
	//Check if we can use the default translation
	QLocale systemLanguage= QLocale::system();
	if(systemLanguage.language() == QLocale::English || systemLanguage.language() == QLocale::C)
	{
		m_defaultLanguage = new QString(LAMEXP_DEFAULT_LANGID);
		return LAMEXP_DEFAULT_LANGID;
	}

	//Try to find a suitable translation for the user's system language
	QStringList languages = lamexp_query_translations();
	while(!languages.isEmpty())
	{
		QString currentLangId = languages.takeFirst();
		if(lamexp_translation_sysid(currentLangId) == systemLanguage.language())
		{
			m_defaultLanguage = new QString(currentLangId);
			return currentLangId;
		}
	}

	//Fall back to the default translation
	m_defaultLanguage = new QString(LAMEXP_DEFAULT_LANGID);
	return LAMEXP_DEFAULT_LANGID;
}

////////////////////////////////////////////////////////////
// Getter and Setter
////////////////////////////////////////////////////////////

MAKE_OPTION1(licenseAccepted, 0)
MAKE_OPTION1(interfaceStyle, 0)
MAKE_OPTION1(compressionEncoder, 0)
MAKE_OPTION1(compressionRCMode, 0)
MAKE_OPTION1(compressionBitrate, 7)
MAKE_OPTION2(outputDir, QString())
MAKE_OPTION3(outputToSourceDir, false)
MAKE_OPTION3(prependRelativeSourcePath, false)
MAKE_OPTION3(writeMetaTags, true)
MAKE_OPTION3(createPlaylist, true)
MAKE_OPTION2(autoUpdateLastCheck, "Never")
MAKE_OPTION3(autoUpdateEnabled, true)
MAKE_OPTION3(soundsEnabled, true)
MAKE_OPTION3(neroAacNotificationsEnabled, true)
MAKE_OPTION3(wmaDecoderNotificationsEnabled, true)
MAKE_OPTION3(dropBoxWidgetEnabled, true)
MAKE_OPTION3(shellIntegrationEnabled, !lamexp_portable_mode())
MAKE_OPTION2(currentLanguage, defaultLanguage())
MAKE_OPTION1(lameAlgoQuality, 3)
MAKE_OPTION1(lameChannelMode, 0);
MAKE_OPTION3(bitrateManagementEnabled, false)
MAKE_OPTION1(bitrateManagementMinRate, 32)
MAKE_OPTION1(bitrateManagementMaxRate, 500)
MAKE_OPTION1(samplingRate, 0)
MAKE_OPTION3(neroAACEnable2Pass, true)
MAKE_OPTION1(neroAACProfile, 0)
MAKE_OPTION3(normalizationFilterEnabled, false)
MAKE_OPTION1(normalizationFilterMaxVolume, -50)
MAKE_OPTION1(toneAdjustBass, 0)
MAKE_OPTION1(toneAdjustTreble, 0)
MAKE_OPTION2(customParametersLAME, QString());
MAKE_OPTION2(customParametersOggEnc, QString());
MAKE_OPTION2(customParametersNeroAAC, QString());
MAKE_OPTION2(customParametersFLAC, QString());
MAKE_OPTION4(metaInfoPosition, UINT_MAX);
MAKE_OPTION4(maximumInstances, 0);
