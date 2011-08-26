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
#include <QRegExp>


////////////////////////////////////////////////////////////
//Macros
////////////////////////////////////////////////////////////

#define LAMEXP_MAKE_OPTION_I(OPT,DEF) \
int SettingsModel::OPT(void) { return m_settings->value(g_settingsId_##OPT, DEF).toInt(); } \
void SettingsModel::OPT(int value) { m_settings->setValue(g_settingsId_##OPT, value); } \
int SettingsModel::OPT##Default(void) { return DEF; }

#define LAMEXP_MAKE_OPTION_S(OPT,DEF) \
QString SettingsModel::OPT(void) { return m_settings->value(g_settingsId_##OPT, DEF).toString().trimmed(); } \
void SettingsModel::OPT(const QString &value) { m_settings->setValue(g_settingsId_##OPT, value); } \
QString SettingsModel::OPT##Default(void) { return DEF; }

#define LAMEXP_MAKE_OPTION_B(OPT,DEF) \
bool SettingsModel::OPT(void) { return m_settings->value(g_settingsId_##OPT, DEF).toBool(); } \
void SettingsModel::OPT(bool value) { m_settings->setValue(g_settingsId_##OPT, value); } \
bool SettingsModel::OPT##Default(void) { return DEF; }

#define LAMEXP_MAKE_OPTION_U(OPT,DEF) \
unsigned int SettingsModel::OPT(void) { return m_settings->value(g_settingsId_##OPT, DEF).toUInt(); } \
void SettingsModel::OPT(unsigned int value) { m_settings->setValue(g_settingsId_##OPT, value); } \
unsigned int SettingsModel::OPT##Default(void) { return DEF; }

#define LAMEXP_MAKE_ID(DEC,STR) static const char *g_settingsId_##DEC = STR
#define REMOVE_GROUP(OBJ,ID) OBJ->beginGroup(ID); OBJ->remove(""); OBJ->endGroup();

////////////////////////////////////////////////////////////
//Constants
////////////////////////////////////////////////////////////

//Setting ID's
LAMEXP_MAKE_ID(versionNumber, "VersionNumber");
LAMEXP_MAKE_ID(licenseAccepted, "LicenseAccepted");
LAMEXP_MAKE_ID(interfaceStyle, "InterfaceStyle");
LAMEXP_MAKE_ID(compressionEncoder, "Compression/Encoder");
LAMEXP_MAKE_ID(compressionRCMode, "Compression/RCMode");
LAMEXP_MAKE_ID(compressionBitrate, "Compression/Bitrate");
LAMEXP_MAKE_ID(outputDir, "OutputDirectory/SelectedPath");
LAMEXP_MAKE_ID(outputToSourceDir, "OutputDirectory/OutputToSourceFolder");
LAMEXP_MAKE_ID(prependRelativeSourcePath, "OutputDirectory/PrependRelativeSourcePath");
LAMEXP_MAKE_ID(favoriteOutputFolders, "OutputDirectory/Favorites");
LAMEXP_MAKE_ID(mostRecentInputPath, "InputDirectory/MostRecentPath");
LAMEXP_MAKE_ID(writeMetaTags, "Flags/WriteMetaTags");
LAMEXP_MAKE_ID(createPlaylist, "Flags/AutoCreatePlaylist");
LAMEXP_MAKE_ID(autoUpdateLastCheck, "AutoUpdate/LastCheck");
LAMEXP_MAKE_ID(autoUpdateEnabled, "AutoUpdate/Enabled");
LAMEXP_MAKE_ID(autoUpdateCheckBeta, "AutoUpdate/CheckForBetaVersions");
LAMEXP_MAKE_ID(soundsEnabled, "Flags/EnableSounds");
LAMEXP_MAKE_ID(neroAacNotificationsEnabled, "Flags/EnableNeroAacNotifications");
LAMEXP_MAKE_ID(antivirNotificationsEnabled, "Flags/EnableAntivirusNotifications");
LAMEXP_MAKE_ID(dropBoxWidgetEnabled, "Flags/EnableDropBoxWidget");
LAMEXP_MAKE_ID(shellIntegrationEnabled, "Flags/EnableShellIntegration");
LAMEXP_MAKE_ID(currentLanguage, "Localization/Language");
LAMEXP_MAKE_ID(lameAlgoQuality, "AdvancedOptions/LAME/AlgorithmQuality");
LAMEXP_MAKE_ID(lameChannelMode, "AdvancedOptions/LAME/ChannelMode");
LAMEXP_MAKE_ID(forceStereoDownmix, "AdvancedOptions/StereoDownmix/Force");
LAMEXP_MAKE_ID(bitrateManagementEnabled, "AdvancedOptions/BitrateManagement/Enabled");
LAMEXP_MAKE_ID(bitrateManagementMinRate, "AdvancedOptions/BitrateManagement/MinRate");
LAMEXP_MAKE_ID(bitrateManagementMaxRate, "AdvancedOptions/BitrateManagement/MaxRate");
LAMEXP_MAKE_ID(aftenAudioCodingMode, "AdvancedOptions/Aften/AudioCodingMode");
LAMEXP_MAKE_ID(aftenDynamicRangeCompression, "AdvancedOptions/Aften/DynamicRangeCompression");
LAMEXP_MAKE_ID(aftenFastBitAllocation, "AdvancedOptions/Aften/FastBitAllocation");
LAMEXP_MAKE_ID(aftenExponentSearchSize, "AdvancedOptions/Aften/ExponentSearchSize");
LAMEXP_MAKE_ID(samplingRate, "AdvancedOptions/Common/Resampling");
LAMEXP_MAKE_ID(neroAACEnable2Pass, "AdvancedOptions/AACEnc/Enable2Pass");
LAMEXP_MAKE_ID(aacEncProfile, "AdvancedOptions/AACEnc/ForceProfile");
LAMEXP_MAKE_ID(normalizationFilterEnabled, "AdvancedOptions/VolumeNormalization/Enabled");
LAMEXP_MAKE_ID(normalizationFilterMaxVolume, "AdvancedOptions/VolumeNormalization/MaxVolume");
LAMEXP_MAKE_ID(toneAdjustBass, "AdvancedOptions/ToneAdjustment/Bass");
LAMEXP_MAKE_ID(toneAdjustTreble, "AdvancedOptions/ToneAdjustment/Treble");
LAMEXP_MAKE_ID(customParametersLAME, "AdvancedOptions/CustomParameters/LAME");
LAMEXP_MAKE_ID(customParametersOggEnc, "AdvancedOptions/CustomParameters/OggEnc");
LAMEXP_MAKE_ID(customParametersAacEnc, "AdvancedOptions/CustomParameters/AacEnc");
LAMEXP_MAKE_ID(customParametersAften, "AdvancedOptions/CustomParameters/Aften");
LAMEXP_MAKE_ID(customParametersFLAC, "AdvancedOptions/CustomParameters/FLAC");
LAMEXP_MAKE_ID(renameOutputFilesEnabled, "AdvancedOptions/RenameOutputFiles/Enabled");
LAMEXP_MAKE_ID(renameOutputFilesPattern, "AdvancedOptions/RenameOutputFiles/Pattern");
LAMEXP_MAKE_ID(metaInfoPosition, "MetaInformation/PlaylistPosition");
LAMEXP_MAKE_ID(maximumInstances, "AdvancedOptions/Threading/MaximumInstances");
LAMEXP_MAKE_ID(customTempPath, "AdvancedOptions/TempDirectory/CustomPath");
LAMEXP_MAKE_ID(customTempPathEnabled, "AdvancedOptions/TempDirectory/UseCustomPath");
LAMEXP_MAKE_ID(slowStartup, "Flags/SlowStartupDetected");

//LUT
const int SettingsModel::mp3Bitrates[15] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1};
const int SettingsModel::ac3Bitrates[20] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640, -1};
const int SettingsModel::samplingRates[8] = {0, 16000, 22050, 24000, 32000, 44100, 48000, -1};

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

SettingsModel::SettingsModel(void)
:
	m_defaultLanguage(NULL)
{
	QString configPath = "LameXP.ini";
	
	if(!lamexp_portable_mode())
	{
		QString dataPath = initDirectory(QDesktopServices::storageLocation(QDesktopServices::DataLocation));
		if(!dataPath.isEmpty())
		{
			configPath = QString("%1/config.ini").arg(QDir(dataPath).canonicalPath());
		}
		else
		{
			qWarning("SettingsModel: DataLocation could not be initialized!");
			dataPath = initDirectory(QDesktopServices::storageLocation(QDesktopServices::HomeLocation));
			if(!dataPath.isEmpty())
			{
				configPath = QString("%1/LameXP.ini").arg(QDir(dataPath).canonicalPath());
			}
		}
	}
	else
	{
		qDebug("LameXP is running in \"portable\" mode -> config in application dir!\n");
		QString appPath = QFileInfo(QApplication::applicationFilePath()).canonicalFilePath();
		if(appPath.isEmpty())
		{
			appPath = QFileInfo(QApplication::applicationFilePath()).absoluteFilePath();
		}
		if(QFileInfo(appPath).exists() && QFileInfo(appPath).isFile())
		{
			configPath = QString("%1/%2.ini").arg(QFileInfo(appPath).absolutePath(), QFileInfo(appPath).completeBaseName());
		}
	}

	m_settings = new QSettings(configPath, QSettings::IniFormat);
	const QString groupKey = QString().sprintf("LameXP_%u%02u%05u", lamexp_version_major(), lamexp_version_minor(), lamexp_version_build());
	QStringList childGroups = m_settings->childGroups();

	while(!childGroups.isEmpty())
	{
		QString current = childGroups.takeFirst();
		QRegExp filter("^LameXP_(\\d+)(\\d\\d)(\\d\\d\\d\\d\\d)$");
		if(filter.indexIn(current) >= 0)
		{
			bool ok = false;
			unsigned int temp = filter.cap(3).toUInt(&ok) + 10;
			if(ok && (temp >= lamexp_version_build()))
			{
				continue;
			}
		}
		qWarning("Deleting obsolete group from config: %s", current.toUtf8().constData());
		REMOVE_GROUP(m_settings, current);
	}

	m_settings->beginGroup(groupKey);
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
		if(!(lamexp_check_tool("fhgaacenc.exe") && lamexp_check_tool("enc_fhgaac.dll")))
		{
			if(this->compressionEncoder() == SettingsModel::AACEncoder)
			{
				qWarning("AAC encoder selected, but not available any more. Reverting to MP3!");
				this->compressionEncoder(SettingsModel::MP3Encoder);
			}
		}
	}
	
	if(this->outputDir().isEmpty() || !QFileInfo(this->outputDir()).isDir())
	{
		QString musicLocation = QDesktopServices::storageLocation(QDesktopServices::MusicLocation);
		this->outputDir(musicLocation.isEmpty() ? QDesktopServices::storageLocation(QDesktopServices::HomeLocation) : musicLocation);
	}

	if(this->mostRecentInputPath().isEmpty() || !QFileInfo(this->mostRecentInputPath()).isDir())
	{
		QString musicLocation = QDesktopServices::storageLocation(QDesktopServices::MusicLocation);
		this->outputDir(musicLocation.isEmpty() ? QDesktopServices::storageLocation(QDesktopServices::HomeLocation) : musicLocation);
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

QString SettingsModel::initDirectory(const QString &path)
{
	if(path.isEmpty())
	{
		return QString();
	}

	if(!QDir(path).exists())
	{
		for(int i = 0; i < 32; i++)
		{
			if(QDir(path).mkpath(".")) break;
			Sleep(1);
		}
	}

	if(!QDir(path).exists())
	{
		return QString();
	}
	
	return QDir(path).canonicalPath();
}

////////////////////////////////////////////////////////////
// Getter and Setter
////////////////////////////////////////////////////////////

LAMEXP_MAKE_OPTION_I(licenseAccepted, 0)
LAMEXP_MAKE_OPTION_I(interfaceStyle, 0)
LAMEXP_MAKE_OPTION_I(compressionEncoder, 0)
LAMEXP_MAKE_OPTION_I(compressionRCMode, 0)
LAMEXP_MAKE_OPTION_I(compressionBitrate, 7)
LAMEXP_MAKE_OPTION_S(outputDir, QDesktopServices::storageLocation(QDesktopServices::MusicLocation))
LAMEXP_MAKE_OPTION_B(outputToSourceDir, false)
LAMEXP_MAKE_OPTION_B(prependRelativeSourcePath, false)
LAMEXP_MAKE_OPTION_S(favoriteOutputFolders, QString());
LAMEXP_MAKE_OPTION_B(writeMetaTags, true)
LAMEXP_MAKE_OPTION_B(createPlaylist, true)
LAMEXP_MAKE_OPTION_S(autoUpdateLastCheck, "Never")
LAMEXP_MAKE_OPTION_B(autoUpdateEnabled, true)
LAMEXP_MAKE_OPTION_B(autoUpdateCheckBeta, false)
LAMEXP_MAKE_OPTION_B(soundsEnabled, true)
LAMEXP_MAKE_OPTION_B(neroAacNotificationsEnabled, true)
LAMEXP_MAKE_OPTION_B(antivirNotificationsEnabled, true)
LAMEXP_MAKE_OPTION_B(dropBoxWidgetEnabled, true)
LAMEXP_MAKE_OPTION_B(shellIntegrationEnabled, !lamexp_portable_mode())
LAMEXP_MAKE_OPTION_S(currentLanguage, defaultLanguage())
LAMEXP_MAKE_OPTION_I(lameAlgoQuality, 3)
LAMEXP_MAKE_OPTION_I(lameChannelMode, 0)
LAMEXP_MAKE_OPTION_B(forceStereoDownmix, false)
LAMEXP_MAKE_OPTION_B(bitrateManagementEnabled, false)
LAMEXP_MAKE_OPTION_I(bitrateManagementMinRate, 32)
LAMEXP_MAKE_OPTION_I(bitrateManagementMaxRate, 500)
LAMEXP_MAKE_OPTION_I(samplingRate, 0)
LAMEXP_MAKE_OPTION_B(neroAACEnable2Pass, true)
LAMEXP_MAKE_OPTION_I(aacEncProfile, 0)
LAMEXP_MAKE_OPTION_I(aftenAudioCodingMode, 0);
LAMEXP_MAKE_OPTION_I(aftenDynamicRangeCompression, 5);
LAMEXP_MAKE_OPTION_B(aftenFastBitAllocation, false);
LAMEXP_MAKE_OPTION_I(aftenExponentSearchSize, 8);
LAMEXP_MAKE_OPTION_B(normalizationFilterEnabled, false)
LAMEXP_MAKE_OPTION_I(normalizationFilterMaxVolume, -50)
LAMEXP_MAKE_OPTION_I(toneAdjustBass, 0)
LAMEXP_MAKE_OPTION_I(toneAdjustTreble, 0)
LAMEXP_MAKE_OPTION_S(customParametersLAME, QString());
LAMEXP_MAKE_OPTION_S(customParametersOggEnc, QString());
LAMEXP_MAKE_OPTION_S(customParametersAacEnc, QString());
LAMEXP_MAKE_OPTION_S(customParametersAften, QString());
LAMEXP_MAKE_OPTION_S(customParametersFLAC, QString());
LAMEXP_MAKE_OPTION_B(renameOutputFilesEnabled, false);
LAMEXP_MAKE_OPTION_S(renameOutputFilesPattern, "[<TrackNo>] <Artist> - <Title>");
LAMEXP_MAKE_OPTION_U(metaInfoPosition, UINT_MAX);
LAMEXP_MAKE_OPTION_U(maximumInstances, 0);
LAMEXP_MAKE_OPTION_S(customTempPath, QDesktopServices::storageLocation(QDesktopServices::TempLocation));
LAMEXP_MAKE_OPTION_B(customTempPathEnabled, false);
LAMEXP_MAKE_OPTION_B(slowStartup, false);
LAMEXP_MAKE_OPTION_S(mostRecentInputPath, QDesktopServices::storageLocation(QDesktopServices::MusicLocation));
