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
#include <QReadWriteLock>
#include <QReadLocker>
#include <QWriteLocker>
#include <QHash>
#include <QMutex>
#include <QSet>

////////////////////////////////////////////////////////////
// SettingsCache Class
////////////////////////////////////////////////////////////

class SettingsCache
{
public:
	SettingsCache(QSettings *configFile) : m_configFile(configFile)
	{
		m_cache = new QHash<QString, QVariant>();
		m_cacheLock = new QMutex();
		m_cacheDirty = new QSet<QString>();
	}

	~SettingsCache(void)
	{
		flushValues();

		LAMEXP_DELETE(m_cache);
		LAMEXP_DELETE(m_cacheDirty);
		LAMEXP_DELETE(m_cacheLock);
		LAMEXP_DELETE(m_configFile);
	}

	inline void storeValue(const QString &key, const QVariant &value)
	{
		QMutexLocker lock(m_cacheLock);
	
		if(!m_cache->contains(key))
		{
			m_cache->insert(key, value);
			m_cacheDirty->insert(key);
		}
		else
		{
			if(m_cache->value(key) != value)
			{
				m_cache->insert(key, value);
				m_cacheDirty->insert(key);
			}
		}
	}

	inline QVariant loadValue(const QString &key, const QVariant &defaultValue) const
	{
		QMutexLocker lock(m_cacheLock);

		if(!m_cache->contains(key))
		{
			const QVariant storedValue = m_configFile->value(key, defaultValue);
			m_cache->insert(key, storedValue);
		}

		return m_cache->value(key, defaultValue);
	}

	inline void flushValues(void)
	{
		QMutexLocker lock(m_cacheLock);

		if(!m_cacheDirty->isEmpty())
		{
			QSet<QString>::ConstIterator iter;
			for(iter = m_cacheDirty->constBegin(); iter != m_cacheDirty->constEnd(); iter++)
			{
				if(m_cache->contains(*iter))
				{
					m_configFile->setValue((*iter), m_cache->value(*iter));
				}
				else
				{
					qWarning("Could not find '%s' in cache, but it has been marked as dirty!", (*iter).toUtf8().constData());
				}
			}
			m_configFile->sync();
			m_cacheDirty->clear();
		}
	}

private:
	QSettings *m_configFile;
	QHash<QString, QVariant> *m_cache;
	QSet<QString> *m_cacheDirty;
	QMutex *m_cacheLock;
};

////////////////////////////////////////////////////////////
// Macros
////////////////////////////////////////////////////////////

#define LAMEXP_MAKE_OPTION_I(OPT,DEF) \
int SettingsModel::OPT(void) const { return m_configCache->loadValue(g_settingsId_##OPT, (DEF)).toInt(); } \
void SettingsModel::OPT(int value) { m_configCache->storeValue(g_settingsId_##OPT, value); } \
int SettingsModel::OPT##Default(void) { return (DEF); }

#define LAMEXP_MAKE_OPTION_S(OPT,DEF) \
QString SettingsModel::OPT(void) const { return m_configCache->loadValue(g_settingsId_##OPT, (DEF)).toString().trimmed(); } \
void SettingsModel::OPT(const QString &value) { m_configCache->storeValue(g_settingsId_##OPT, value); } \
QString SettingsModel::OPT##Default(void) { return (DEF); }

#define LAMEXP_MAKE_OPTION_B(OPT,DEF) \
bool SettingsModel::OPT(void) const { return m_configCache->loadValue(g_settingsId_##OPT, (DEF)).toBool(); } \
void SettingsModel::OPT(bool value) { m_configCache->storeValue(g_settingsId_##OPT, value); } \
bool SettingsModel::OPT##Default(void) { return (DEF); }

#define LAMEXP_MAKE_OPTION_U(OPT,DEF) \
unsigned int SettingsModel::OPT(void) const { return m_configCache->loadValue(g_settingsId_##OPT, (DEF)).toUInt(); } \
void SettingsModel::OPT(unsigned int value) { m_configCache->storeValue(g_settingsId_##OPT, value); } \
unsigned int SettingsModel::OPT##Default(void) { return (DEF); }

#define LAMEXP_MAKE_ID(DEC,STR) static const char *g_settingsId_##DEC = STR

#define REMOVE_GROUP(OBJ,ID) do \
{ \
	OBJ->beginGroup(ID); \
	OBJ->remove(""); \
	OBJ->endGroup(); \
} \
while(0)

#define DIR_EXISTS(PATH) (QFileInfo(PATH).exists() && QFileInfo(PATH).isDir())

////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////

//Setting ID's
LAMEXP_MAKE_ID(aacEncProfile, "AdvancedOptions/AACEnc/ForceProfile");
LAMEXP_MAKE_ID(aftenAudioCodingMode, "AdvancedOptions/Aften/AudioCodingMode");
LAMEXP_MAKE_ID(aftenDynamicRangeCompression, "AdvancedOptions/Aften/DynamicRangeCompression");
LAMEXP_MAKE_ID(aftenExponentSearchSize, "AdvancedOptions/Aften/ExponentSearchSize");
LAMEXP_MAKE_ID(aftenFastBitAllocation, "AdvancedOptions/Aften/FastBitAllocation");
LAMEXP_MAKE_ID(antivirNotificationsEnabled, "Flags/EnableAntivirusNotifications");
LAMEXP_MAKE_ID(autoUpdateCheckBeta, "AutoUpdate/CheckForBetaVersions");
LAMEXP_MAKE_ID(autoUpdateEnabled, "AutoUpdate/Enabled");
LAMEXP_MAKE_ID(autoUpdateLastCheck, "AutoUpdate/LastCheck");
LAMEXP_MAKE_ID(bitrateManagementEnabled, "AdvancedOptions/BitrateManagement/Enabled");
LAMEXP_MAKE_ID(bitrateManagementMaxRate, "AdvancedOptions/BitrateManagement/MaxRate");
LAMEXP_MAKE_ID(bitrateManagementMinRate, "AdvancedOptions/BitrateManagement/MinRate");
LAMEXP_MAKE_ID(compressionBitrateAacEnc, "Compression/Bitrate/AacEnc");
LAMEXP_MAKE_ID(compressionBitrateAften, "Compression/Bitrate/Aften");
LAMEXP_MAKE_ID(compressionBitrateDcaEnc, "Compression/Bitrate/DcaEnc");
LAMEXP_MAKE_ID(compressionBitrateLAME, "Compression/Bitrate/LAME");
LAMEXP_MAKE_ID(compressionBitrateOggEnc, "Compression/Bitrate/OggEnc");
LAMEXP_MAKE_ID(compressionBitrateOpusEnc, "Compression/Bitrate/OpusEnc");
LAMEXP_MAKE_ID(compressionEncoder, "Compression/Encoder");
LAMEXP_MAKE_ID(compressionRCModeAacEnc, "Compression/RCMode/AacEnc");
LAMEXP_MAKE_ID(compressionRCModeAften, "Compression/RCMode/Aften");
LAMEXP_MAKE_ID(compressionRCModeLAME, "Compression/RCMode/LAME");
LAMEXP_MAKE_ID(compressionRCModeOggEnc, "Compression/RCMode/OggEnc");
LAMEXP_MAKE_ID(compressionRCModeOpusEnc, "Compression/RCMode/OpusEnc");
LAMEXP_MAKE_ID(compressionVbrLevelAacEnc, "Compression/VbrLevel/AacEnc");
LAMEXP_MAKE_ID(compressionVbrLevelAften, "Compression/VbrLevel/Aften");
LAMEXP_MAKE_ID(compressionVbrLevelFLAC, "Compression/VbrLevel/FLAC");
LAMEXP_MAKE_ID(compressionVbrLevelLAME, "Compression/VbrLevel/LAME");
LAMEXP_MAKE_ID(compressionVbrLevelOggEnc, "Compression/VbrLevel/OggEnc");
LAMEXP_MAKE_ID(createPlaylist, "Flags/AutoCreatePlaylist");
LAMEXP_MAKE_ID(currentLanguage, "Localization/Language");
LAMEXP_MAKE_ID(currentLanguageFile, "Localization/UseQMFile");
LAMEXP_MAKE_ID(customParametersAacEnc, "AdvancedOptions/CustomParameters/AacEnc");
LAMEXP_MAKE_ID(customParametersAften, "AdvancedOptions/CustomParameters/Aften");
LAMEXP_MAKE_ID(customParametersFLAC, "AdvancedOptions/CustomParameters/FLAC");
LAMEXP_MAKE_ID(customParametersLAME, "AdvancedOptions/CustomParameters/LAME");
LAMEXP_MAKE_ID(customParametersOggEnc, "AdvancedOptions/CustomParameters/OggEnc");
LAMEXP_MAKE_ID(customParametersOpus, "AdvancedOptions/CustomParameters/OpusEnc");
LAMEXP_MAKE_ID(customTempPath, "AdvancedOptions/TempDirectory/CustomPath");
LAMEXP_MAKE_ID(customTempPathEnabled, "AdvancedOptions/TempDirectory/UseCustomPath");
LAMEXP_MAKE_ID(dropBoxWidgetEnabled, "Flags/EnableDropBoxWidget");
LAMEXP_MAKE_ID(favoriteOutputFolders, "OutputDirectory/Favorites");
LAMEXP_MAKE_ID(forceStereoDownmix, "AdvancedOptions/StereoDownmix/Force");
LAMEXP_MAKE_ID(hibernateComputer, "AdvancedOptions/HibernateComputerOnShutdown");
LAMEXP_MAKE_ID(interfaceStyle, "InterfaceStyle");
LAMEXP_MAKE_ID(lameAlgoQuality, "AdvancedOptions/LAME/AlgorithmQuality");
LAMEXP_MAKE_ID(lameChannelMode, "AdvancedOptions/LAME/ChannelMode");
LAMEXP_MAKE_ID(licenseAccepted, "LicenseAccepted");
LAMEXP_MAKE_ID(maximumInstances, "AdvancedOptions/Threading/MaximumInstances");
LAMEXP_MAKE_ID(metaInfoPosition, "MetaInformation/PlaylistPosition");
LAMEXP_MAKE_ID(mostRecentInputPath, "InputDirectory/MostRecentPath");
LAMEXP_MAKE_ID(neroAACEnable2Pass, "AdvancedOptions/AACEnc/Enable2Pass");
LAMEXP_MAKE_ID(neroAacNotificationsEnabled, "Flags/EnableNeroAacNotifications");
LAMEXP_MAKE_ID(normalizationFilterEnabled, "AdvancedOptions/VolumeNormalization/Enabled");
LAMEXP_MAKE_ID(normalizationFilterEqualizationMode, "AdvancedOptions/VolumeNormalization/EqualizationMode");
LAMEXP_MAKE_ID(normalizationFilterMaxVolume, "AdvancedOptions/VolumeNormalization/MaxVolume");
LAMEXP_MAKE_ID(opusComplexity, "AdvancedOptions/Opus/EncodingComplexity");
LAMEXP_MAKE_ID(opusDisableResample, "AdvancedOptions/Opus/DisableResample");
LAMEXP_MAKE_ID(opusFramesize, "AdvancedOptions/Opus/FrameSize");
LAMEXP_MAKE_ID(opusOptimizeFor, "AdvancedOptions/Opus/OptimizeForSignalType");
LAMEXP_MAKE_ID(outputDir, "OutputDirectory/SelectedPath");
LAMEXP_MAKE_ID(outputToSourceDir, "OutputDirectory/OutputToSourceFolder");
LAMEXP_MAKE_ID(overwriteMode, "AdvancedOptions/OverwriteMode");
LAMEXP_MAKE_ID(prependRelativeSourcePath, "OutputDirectory/PrependRelativeSourcePath");
LAMEXP_MAKE_ID(renameOutputFilesEnabled, "AdvancedOptions/RenameOutputFiles/Enabled");
LAMEXP_MAKE_ID(renameOutputFilesPattern, "AdvancedOptions/RenameOutputFiles/Pattern");
LAMEXP_MAKE_ID(samplingRate, "AdvancedOptions/Common/Resampling");
LAMEXP_MAKE_ID(shellIntegrationEnabled, "Flags/EnableShellIntegration");
LAMEXP_MAKE_ID(slowStartup, "Flags/SlowStartupDetected");
LAMEXP_MAKE_ID(soundsEnabled, "Flags/EnableSounds");
LAMEXP_MAKE_ID(toneAdjustBass, "AdvancedOptions/ToneAdjustment/Bass");
LAMEXP_MAKE_ID(toneAdjustTreble, "AdvancedOptions/ToneAdjustment/Treble");
LAMEXP_MAKE_ID(versionNumber, "VersionNumber");
LAMEXP_MAKE_ID(writeMetaTags, "Flags/WriteMetaTags");

//LUT
const int SettingsModel::mp3Bitrates[15] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1};
const int SettingsModel::ac3Bitrates[20] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 448, 512, 576, 640, -1};
const int SettingsModel::samplingRates[8] = {0, 16000, 22050, 24000, 32000, 44100, 48000, -1};

static QReadWriteLock s_lock;

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

SettingsModel::SettingsModel(void)
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

	//Create settings
	QSettings *configFile = new QSettings(configPath, QSettings::IniFormat);
	const QString groupKey = QString().sprintf("LameXP_%u%02u%05u", lamexp_version_major(), lamexp_version_minor(), lamexp_version_confg());
	QStringList childGroups =configFile->childGroups();

	//Clean-up settings
	while(!childGroups.isEmpty())
	{
		QString current = childGroups.takeFirst();
		QRegExp filter("^LameXP_(\\d+)(\\d\\d)(\\d\\d\\d\\d\\d)$");
		if(filter.indexIn(current) >= 0)
		{
			bool ok = false;
			unsigned int temp = filter.cap(3).toUInt(&ok) + 10;
			if(ok && (temp >= lamexp_version_confg()))
			{
				continue;
			}
		}
		qWarning("Deleting obsolete group from config: %s", current.toUtf8().constData());
		REMOVE_GROUP(configFile, current);
	}

	//Setup settings
	configFile->beginGroup(groupKey);
	configFile->setValue(g_settingsId_versionNumber, QApplication::applicationVersion());
	configFile->sync();

	//Create the cache
	m_configCache = new SettingsCache(configFile);
}

////////////////////////////////////////////////////////////
// Destructor
////////////////////////////////////////////////////////////

SettingsModel::~SettingsModel(void)
{
	LAMEXP_DELETE(m_configCache);
	LAMEXP_DELETE(m_defaultLanguage);
}

////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////

#define CHECK_RCMODE(NAME) do\
{ \
	if(this->compressionRCMode##NAME() < SettingsModel::VBRMode || this->compressionRCMode##NAME() >= SettingsModel::RCMODE_COUNT) \
	{ \
		this->compressionRCMode##NAME(SettingsModel::VBRMode); \
	} \
} \
while(0)

void SettingsModel::validate(void)
{
	if(this->compressionEncoder() < SettingsModel::MP3Encoder || this->compressionEncoder() >= SettingsModel::ENCODER_COUNT)
	{
		this->compressionEncoder(SettingsModel::MP3Encoder);
	}
	
	CHECK_RCMODE(LAME);
	CHECK_RCMODE(OggEnc);
	CHECK_RCMODE(AacEnc);
	CHECK_RCMODE(Aften);
	CHECK_RCMODE(OpusEnc);
	
	if(!(lamexp_check_tool("neroAacEnc.exe") && lamexp_check_tool("neroAacDec.exe") && lamexp_check_tool("neroAacTag.exe")))
	{
		if(!(lamexp_check_tool("fhgaacenc.exe") && lamexp_check_tool("enc_fhgaac.dll")))
		{
			if(!(lamexp_check_tool("qaac.exe") && lamexp_check_tool("libsoxrate.dll")))
			{
				if(this->compressionEncoder() == SettingsModel::AACEncoder)
				{
					qWarning("AAC encoder selected, but not available any more. Reverting to MP3!");
					this->compressionEncoder(SettingsModel::MP3Encoder);
				}
			}
		}
	}
	
	if(this->outputDir().isEmpty() || (!DIR_EXISTS(this->outputDir())))
	{
		qWarning("Output directory not set yet or does NOT exist anymore -> Resetting");
		this->outputDir(defaultDirectory());
	}

	if(this->mostRecentInputPath().isEmpty() || (!DIR_EXISTS(this->mostRecentInputPath())))
	{
		qWarning("Most recent input directory not set yet or does NOT exist anymore -> Resetting");
		this->mostRecentInputPath(defaultDirectory());
	}

	if(!this->currentLanguageFile().isEmpty())
	{
		const QString qmPath = QFileInfo(this->currentLanguageFile()).canonicalFilePath();
		if(qmPath.isEmpty() || (!(QFileInfo(qmPath).exists() && QFileInfo(qmPath).isFile() && (QFileInfo(qmPath).suffix().compare("qm", Qt::CaseInsensitive) == 0))))
		{
			qWarning("Current language file missing, reverting to built-in translator!");
			this->currentLanguageFile(QString());
		}
	}

	if(!lamexp_query_translations().contains(this->currentLanguage(), Qt::CaseInsensitive))
	{
		qWarning("Current language \"%s\" is unknown, reverting to default language!", this->currentLanguage().toLatin1().constData());
		this->currentLanguage(defaultLanguage());
	}

	if(this->hibernateComputer())
	{
		if(!lamexp_is_hibernation_supported())
		{
			this->hibernateComputer(false);
		}
	}

	if(this->overwriteMode() < SettingsModel::Overwrite_KeepBoth || this->overwriteMode() > SettingsModel::Overwrite_Replaces)
	{
		this->overwriteMode(SettingsModel::Overwrite_KeepBoth);
	}

}

void SettingsModel::syncNow(void)
{
	m_configCache->flushValues();
}

////////////////////////////////////////////////////////////
// Static Functions
////////////////////////////////////////////////////////////

int SettingsModel::getAacEncoder(void)
{
	if(lamexp_check_tool("qaac.exe") && lamexp_check_tool("libsoxrate.dll"))
	{
		return AAC_ENCODER_QAAC;
	}
	else if(lamexp_check_tool("fhgaacenc.exe") && lamexp_check_tool("enc_fhgaac.dll") && lamexp_check_tool("nsutil.dll") && lamexp_check_tool("libmp4v2.dll"))
	{
		return AAC_ENCODER_FHG;
	}
	else if(lamexp_check_tool("neroAacEnc.exe") && lamexp_check_tool("neroAacDec.exe") && lamexp_check_tool("neroAacTag.exe"))
	{
		return AAC_ENCODER_NERO;
	}
	else
	{
		return AAC_ENCODER_NONE;
	}
}

////////////////////////////////////////////////////////////
// Private Functions
////////////////////////////////////////////////////////////

QString *SettingsModel::m_defaultLanguage = NULL;

QString SettingsModel::defaultLanguage(void) const
{
	QReadLocker readLock(&s_lock);

	//Default already initialized?
	if(m_defaultLanguage)
	{
		return *m_defaultLanguage;
	}
	
	//Acquire write lock now
	readLock.unlock();
	QWriteLocker writeLock(&s_lock);
	
	//Default still not initialized?
	if(m_defaultLanguage)
	{
		return *m_defaultLanguage;
	}

	//Detect system langauge
	QLocale systemLanguage= QLocale::system();
	qDebug("[Locale]");
	qDebug("Language: %s (%d)", QLocale::languageToString(systemLanguage.language()).toUtf8().constData(), systemLanguage.language());
	qDebug("Country is: %s (%d)", QLocale::countryToString(systemLanguage.country()).toUtf8().constData(), systemLanguage.country());
	qDebug("Script is: %s (%d)\n", QLocale::scriptToString(systemLanguage.script()).toUtf8().constData(), systemLanguage.script());

	//Check if we can use the default translation
	if(systemLanguage.language() == QLocale::English /*|| systemLanguage.language() == QLocale::C*/)
	{
		m_defaultLanguage = new QString(LAMEXP_DEFAULT_LANGID);
		return LAMEXP_DEFAULT_LANGID;
	}

	//Try to find a suitable translation for the user's system language *and* country
	QStringList languages = lamexp_query_translations();
	while(!languages.isEmpty())
	{
		QString currentLangId = languages.takeFirst();
		if(lamexp_translation_sysid(currentLangId) == systemLanguage.language())
		{
			if(lamexp_translation_country(currentLangId) == systemLanguage.country())
			{
				m_defaultLanguage = new QString(currentLangId);
				return currentLangId;
			}
		}
	}

	//Try to find a suitable translation for the user's system language
	languages = lamexp_query_translations();
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

QString SettingsModel::defaultDirectory(void) const
{
	QString defaultLocation = initDirectory(QDesktopServices::storageLocation(QDesktopServices::MusicLocation));

	if(defaultLocation.isEmpty())
	{
		defaultLocation = initDirectory(QDesktopServices::storageLocation(QDesktopServices::HomeLocation));

		if(defaultLocation.isEmpty())
		{
			defaultLocation = initDirectory(QDir::currentPath());
		}
	}

	return defaultLocation;
}

QString SettingsModel::initDirectory(const QString &path) const
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

LAMEXP_MAKE_OPTION_I(aacEncProfile, 0)
LAMEXP_MAKE_OPTION_I(aftenAudioCodingMode, 0)
LAMEXP_MAKE_OPTION_I(aftenDynamicRangeCompression, 5)
LAMEXP_MAKE_OPTION_I(aftenExponentSearchSize, 8)
LAMEXP_MAKE_OPTION_B(aftenFastBitAllocation, false)
LAMEXP_MAKE_OPTION_B(antivirNotificationsEnabled, true)
LAMEXP_MAKE_OPTION_B(autoUpdateCheckBeta, false)
LAMEXP_MAKE_OPTION_B(autoUpdateEnabled, true)
LAMEXP_MAKE_OPTION_S(autoUpdateLastCheck, "Never")
LAMEXP_MAKE_OPTION_B(bitrateManagementEnabled, false)
LAMEXP_MAKE_OPTION_I(bitrateManagementMaxRate, 500)
LAMEXP_MAKE_OPTION_I(bitrateManagementMinRate, 32)
LAMEXP_MAKE_OPTION_I(compressionBitrateAacEnc, 20)
LAMEXP_MAKE_OPTION_I(compressionBitrateAften, 15)
LAMEXP_MAKE_OPTION_I(compressionBitrateDcaEnc, 47)
LAMEXP_MAKE_OPTION_I(compressionBitrateLAME, 10)
LAMEXP_MAKE_OPTION_I(compressionBitrateOggEnc, 20)
LAMEXP_MAKE_OPTION_I(compressionBitrateOpusEnc, 16)
LAMEXP_MAKE_OPTION_I(compressionEncoder, 0)
LAMEXP_MAKE_OPTION_I(compressionRCModeAacEnc, 0)
LAMEXP_MAKE_OPTION_I(compressionRCModeAften, 0)
LAMEXP_MAKE_OPTION_I(compressionRCModeLAME, 0)
LAMEXP_MAKE_OPTION_I(compressionRCModeOggEnc, 0)
LAMEXP_MAKE_OPTION_I(compressionRCModeOpusEnc, 0)
LAMEXP_MAKE_OPTION_I(compressionVbrLevelAacEnc, 10)
LAMEXP_MAKE_OPTION_I(compressionVbrLevelAften, 8)
LAMEXP_MAKE_OPTION_I(compressionVbrLevelFLAC, 8)
LAMEXP_MAKE_OPTION_I(compressionVbrLevelLAME, 7)
LAMEXP_MAKE_OPTION_I(compressionVbrLevelOggEnc, 5)
LAMEXP_MAKE_OPTION_B(createPlaylist, true)
LAMEXP_MAKE_OPTION_S(currentLanguage, defaultLanguage())
LAMEXP_MAKE_OPTION_S(currentLanguageFile, QString())
LAMEXP_MAKE_OPTION_S(customParametersAacEnc, QString())
LAMEXP_MAKE_OPTION_S(customParametersAften, QString())
LAMEXP_MAKE_OPTION_S(customParametersFLAC, QString())
LAMEXP_MAKE_OPTION_S(customParametersLAME, QString())
LAMEXP_MAKE_OPTION_S(customParametersOggEnc, QString())
LAMEXP_MAKE_OPTION_S(customParametersOpus, QString())
LAMEXP_MAKE_OPTION_S(customTempPath, QDesktopServices::storageLocation(QDesktopServices::TempLocation))
LAMEXP_MAKE_OPTION_B(customTempPathEnabled, false)
LAMEXP_MAKE_OPTION_B(dropBoxWidgetEnabled, true)
LAMEXP_MAKE_OPTION_S(favoriteOutputFolders, QString())
LAMEXP_MAKE_OPTION_B(forceStereoDownmix, false)
LAMEXP_MAKE_OPTION_B(hibernateComputer, false)
LAMEXP_MAKE_OPTION_I(interfaceStyle, 0)
LAMEXP_MAKE_OPTION_I(lameAlgoQuality, 2)
LAMEXP_MAKE_OPTION_I(lameChannelMode, 0)
LAMEXP_MAKE_OPTION_I(licenseAccepted, 0)
LAMEXP_MAKE_OPTION_U(maximumInstances, 0)
LAMEXP_MAKE_OPTION_U(metaInfoPosition, UINT_MAX)
LAMEXP_MAKE_OPTION_S(mostRecentInputPath, defaultDirectory())
LAMEXP_MAKE_OPTION_B(neroAACEnable2Pass, true)
LAMEXP_MAKE_OPTION_B(neroAacNotificationsEnabled, true)
LAMEXP_MAKE_OPTION_B(normalizationFilterEnabled, false)
LAMEXP_MAKE_OPTION_I(normalizationFilterEqualizationMode, 0)
LAMEXP_MAKE_OPTION_I(normalizationFilterMaxVolume, -50)
LAMEXP_MAKE_OPTION_I(opusComplexity, 10)
LAMEXP_MAKE_OPTION_B(opusDisableResample, false)
LAMEXP_MAKE_OPTION_I(opusFramesize, 3)
LAMEXP_MAKE_OPTION_I(opusOptimizeFor, 0)
LAMEXP_MAKE_OPTION_S(outputDir, defaultDirectory())
LAMEXP_MAKE_OPTION_B(outputToSourceDir, false)
LAMEXP_MAKE_OPTION_I(overwriteMode, Overwrite_KeepBoth)
LAMEXP_MAKE_OPTION_B(prependRelativeSourcePath, false)
LAMEXP_MAKE_OPTION_B(renameOutputFilesEnabled, false)
LAMEXP_MAKE_OPTION_S(renameOutputFilesPattern, "[<TrackNo>] <Artist> - <Title>")
LAMEXP_MAKE_OPTION_I(samplingRate, 0)
LAMEXP_MAKE_OPTION_B(shellIntegrationEnabled, !lamexp_portable_mode())
LAMEXP_MAKE_OPTION_B(slowStartup, false)
LAMEXP_MAKE_OPTION_B(soundsEnabled, true)
LAMEXP_MAKE_OPTION_I(toneAdjustBass, 0)
LAMEXP_MAKE_OPTION_I(toneAdjustTreble, 0)
LAMEXP_MAKE_OPTION_B(writeMetaTags, true)
