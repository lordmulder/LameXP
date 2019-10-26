///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2019 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version, but always including the *additional*
// restrictions defined in the "License.txt" file.
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

//Internal
#include "Global.h"
#include "Registry_Encoder.h"

//MUtils
#include <MUtils/Global.h>
#include <MUtils/Translation.h>
#include <MUtils/OSSupport.h>

//Qt
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
	SettingsCache(QSettings *configFile)
	:
		m_configFile(configFile),
		m_cache(new cache_data_t()),
		m_cacheDirty(new string_set_t())
	{
	}

	~SettingsCache(void)
	{
		flushValues();
	}

	inline void storeValue(const QString &key, const QVariant &value)
	{
		QWriteLocker writeLock(&m_cacheLock);
	
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
		QReadLocker readLock(&m_cacheLock);

		if(m_cache->contains(key))
		{
			return m_cache->value(key, defaultValue);
		}

		readLock.unlock();
		QWriteLocker writeLock(&m_cacheLock);

		if(!m_cache->contains(key))
		{
			const QVariant storedValue = m_configFile->value(key, defaultValue);
			m_cache->insert(key, storedValue);
		}

		return m_cache->value(key, defaultValue);
	}

	inline void flushValues(void)
	{
		QWriteLocker writeLock(&m_cacheLock);

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
					qWarning("Could not find '%s' in cache, but it has been marked as dirty!", MUTILS_UTF8(*iter));
				}
			}
			m_configFile->sync();
			m_cacheDirty->clear();
		}
	}

private:
	typedef QSet<QString>            string_set_t;
	typedef QHash<QString, QVariant> cache_data_t;

	QScopedPointer<QSettings>    m_configFile;
	QScopedPointer<cache_data_t> m_cache;
	QScopedPointer<string_set_t> m_cacheDirty;
	
	mutable QReadWriteLock m_cacheLock;
};

////////////////////////////////////////////////////////////
// Macros
////////////////////////////////////////////////////////////

#define LAMEXP_MAKE_OPTION_I(OPT,DEF) \
qint32 SettingsModel::OPT(void) const { return m_configCache->loadValue(g_settingsId_##OPT, (DEF)).toInt(); } \
void SettingsModel::OPT(const qint32 &value) { m_configCache->storeValue(g_settingsId_##OPT, value); } \
qint32 SettingsModel::OPT##Default(void) { return (DEF); }

#define LAMEXP_MAKE_OPTION_S(OPT,DEF) \
QString SettingsModel::OPT(void) const { return m_configCache->loadValue(g_settingsId_##OPT, (DEF)).toString().trimmed(); } \
void SettingsModel::OPT(const QString &value) { m_configCache->storeValue(g_settingsId_##OPT, value); } \
QString SettingsModel::OPT##Default(void) { return (DEF); }

#define LAMEXP_MAKE_OPTION_B(OPT,DEF) \
bool SettingsModel::OPT(void) const { return m_configCache->loadValue(g_settingsId_##OPT, (DEF)).toBool(); } \
void SettingsModel::OPT(bool value) { m_configCache->storeValue(g_settingsId_##OPT, value); } \
bool SettingsModel::OPT##Default(void) { return (DEF); }

#define LAMEXP_MAKE_OPTION_U(OPT,DEF) \
quint32 SettingsModel::OPT(void) const { return m_configCache->loadValue(g_settingsId_##OPT, (DEF)).toUInt(); } \
void SettingsModel::OPT(const quint32 &value) { m_configCache->storeValue(g_settingsId_##OPT, value); } \
quint32 SettingsModel::OPT##Default(void) { return (DEF); }

#define LAMEXP_MAKE_ID(DEC,STR) static const char *g_settingsId_##DEC = STR

#define REMOVE_GROUP(OBJ,ID) do \
{ \
	OBJ->beginGroup(ID); \
	OBJ->remove(""); \
	OBJ->endGroup(); \
} \
while(0)

////////////////////////////////////////////////////////////
// Utility functions
////////////////////////////////////////////////////////////

static bool dir_exists(const QString &path)
{
	const QFileInfo info(path);
	return info.exists() && info.isDir();
}

static QString find_existing_ancestor(const QString &path)
{
	for (QString parentPath = path; !parentPath.isEmpty(); parentPath = MUtils::parent_path(parentPath))
	{
		if (dir_exists(parentPath))
		{
			return parentPath; /*existing parent found*/
		}
	}
	return QString();
}

////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////

//Setting ID's
LAMEXP_MAKE_ID(aacEncProfile,                "AdvancedOptions/AACEnc/ForceProfile");
LAMEXP_MAKE_ID(aftenAudioCodingMode,         "AdvancedOptions/Aften/AudioCodingMode");
LAMEXP_MAKE_ID(aftenDynamicRangeCompression, "AdvancedOptions/Aften/DynamicRangeCompression");
LAMEXP_MAKE_ID(aftenExponentSearchSize,      "AdvancedOptions/Aften/ExponentSearchSize");
LAMEXP_MAKE_ID(aftenFastBitAllocation,       "AdvancedOptions/Aften/FastBitAllocation");
LAMEXP_MAKE_ID(antivirNotificationsEnabled,  "Flags/EnableAntivirusNotifications");
LAMEXP_MAKE_ID(autoUpdateCheckBeta,          "AutoUpdate/CheckForBetaVersions");
LAMEXP_MAKE_ID(autoUpdateEnabled,            "AutoUpdate/Enabled");
LAMEXP_MAKE_ID(autoUpdateLastCheck,          "AutoUpdate/LastCheck");
LAMEXP_MAKE_ID(bitrateManagementEnabled,     "AdvancedOptions/BitrateManagement/Enabled");
LAMEXP_MAKE_ID(bitrateManagementMaxRate,     "AdvancedOptions/BitrateManagement/MaxRate");
LAMEXP_MAKE_ID(bitrateManagementMinRate,     "AdvancedOptions/BitrateManagement/MinRate");
LAMEXP_MAKE_ID(compressionAbrBitrateAacEnc,  "Compression/AbrTaretBitrate/AacEnc");
LAMEXP_MAKE_ID(compressionAbrBitrateAften,   "Compression/AbrTaretBitrate/Aften");
LAMEXP_MAKE_ID(compressionAbrBitrateDcaEnc,  "Compression/AbrTaretBitrate/DcaEnc");
LAMEXP_MAKE_ID(compressionAbrBitrateFLAC,    "Compression/AbrTaretBitrate/FLAC");
LAMEXP_MAKE_ID(compressionAbrBitrateLAME,    "Compression/AbrTaretBitrate/LAME");
LAMEXP_MAKE_ID(compressionAbrBitrateMacEnc,  "Compression/AbrTaretBitrate/MacEnc");
LAMEXP_MAKE_ID(compressionAbrBitrateOggEnc,  "Compression/AbrTaretBitrate/OggEnc");
LAMEXP_MAKE_ID(compressionAbrBitrateOpusEnc, "Compression/AbrTaretBitrate/OpusEnc");
LAMEXP_MAKE_ID(compressionAbrBitrateWave,    "Compression/AbrTaretBitrate/Wave");
LAMEXP_MAKE_ID(compressionCbrBitrateAacEnc,  "Compression/CbrTaretBitrate/AacEnc");
LAMEXP_MAKE_ID(compressionCbrBitrateAften,   "Compression/CbrTaretBitrate/Aften");
LAMEXP_MAKE_ID(compressionCbrBitrateDcaEnc,  "Compression/CbrTaretBitrate/DcaEnc");
LAMEXP_MAKE_ID(compressionCbrBitrateFLAC,    "Compression/CbrTaretBitrate/FLAC");
LAMEXP_MAKE_ID(compressionCbrBitrateLAME,    "Compression/CbrTaretBitrate/LAME");
LAMEXP_MAKE_ID(compressionCbrBitrateMacEnc,  "Compression/CbrTaretBitrate/MacEnc");
LAMEXP_MAKE_ID(compressionCbrBitrateOggEnc,  "Compression/CbrTaretBitrate/OggEnc");
LAMEXP_MAKE_ID(compressionCbrBitrateOpusEnc, "Compression/CbrTaretBitrate/OpusEnc");
LAMEXP_MAKE_ID(compressionCbrBitrateWave,    "Compression/CbrTaretBitrate/Wave");
LAMEXP_MAKE_ID(compressionEncoder,           "Compression/Encoder");
LAMEXP_MAKE_ID(compressionRCModeAacEnc,      "Compression/RCMode/AacEnc");
LAMEXP_MAKE_ID(compressionRCModeAften,       "Compression/RCMode/Aften");
LAMEXP_MAKE_ID(compressionRCModeDcaEnc,      "Compression/RCMode/DcaEnc");
LAMEXP_MAKE_ID(compressionRCModeFLAC,        "Compression/RCMode/FLAC");
LAMEXP_MAKE_ID(compressionRCModeLAME,        "Compression/RCMode/LAME");
LAMEXP_MAKE_ID(compressionRCModeMacEnc,      "Compression/RCMode/MacEnc");
LAMEXP_MAKE_ID(compressionRCModeOggEnc,      "Compression/RCMode/OggEnc");
LAMEXP_MAKE_ID(compressionRCModeOpusEnc,     "Compression/RCMode/OpusEnc");
LAMEXP_MAKE_ID(compressionRCModeWave,        "Compression/RCMode/Wave");
LAMEXP_MAKE_ID(compressionVbrQualityAacEnc,  "Compression/VbrQualityLevel/AacEnc");
LAMEXP_MAKE_ID(compressionVbrQualityAften,   "Compression/VbrQualityLevel/Aften");
LAMEXP_MAKE_ID(compressionVbrQualityDcaEnc,  "Compression/VbrQualityLevel/DcaEnc");
LAMEXP_MAKE_ID(compressionVbrQualityFLAC,    "Compression/VbrQualityLevel/FLAC");
LAMEXP_MAKE_ID(compressionVbrQualityLAME,    "Compression/VbrQualityLevel/LAME");
LAMEXP_MAKE_ID(compressionVbrQualityMacEnc,  "Compression/VbrQualityLevel/MacEnc");
LAMEXP_MAKE_ID(compressionVbrQualityOggEnc,  "Compression/VbrQualityLevel/OggEnc");
LAMEXP_MAKE_ID(compressionVbrQualityOpusEnc, "Compression/VbrQualityLevel/OpusEnc");
LAMEXP_MAKE_ID(compressionVbrQualityWave,    "Compression/VbrQualityLevel/Wave");
LAMEXP_MAKE_ID(createPlaylist,               "Flags/AutoCreatePlaylist");
LAMEXP_MAKE_ID(currentLanguage,              "Localization/Language");
LAMEXP_MAKE_ID(currentLanguageFile,          "Localization/UseQMFile");
LAMEXP_MAKE_ID(customParametersAacEnc,       "AdvancedOptions/CustomParameters/AacEnc");
LAMEXP_MAKE_ID(customParametersAften,        "AdvancedOptions/CustomParameters/Aften");
LAMEXP_MAKE_ID(customParametersDcaEnc,       "AdvancedOptions/CustomParameters/DcaEnc");
LAMEXP_MAKE_ID(customParametersFLAC,         "AdvancedOptions/CustomParameters/FLAC");
LAMEXP_MAKE_ID(customParametersLAME,         "AdvancedOptions/CustomParameters/LAME");
LAMEXP_MAKE_ID(customParametersMacEnc,       "AdvancedOptions/CustomParameters/MacEnc");
LAMEXP_MAKE_ID(customParametersOggEnc,       "AdvancedOptions/CustomParameters/OggEnc");
LAMEXP_MAKE_ID(customParametersOpusEnc,      "AdvancedOptions/CustomParameters/OpusEnc");
LAMEXP_MAKE_ID(customParametersWave,         "AdvancedOptions/CustomParameters/Wave");
LAMEXP_MAKE_ID(customTempPath,               "AdvancedOptions/TempDirectory/CustomPath");
LAMEXP_MAKE_ID(customTempPathEnabled,        "AdvancedOptions/TempDirectory/UseCustomPath");
LAMEXP_MAKE_ID(dropBoxWidgetEnabled,         "DropBoxWidget/Enabled");
LAMEXP_MAKE_ID(dropBoxWidgetPositionX,       "DropBoxWidget/Position/X");
LAMEXP_MAKE_ID(dropBoxWidgetPositionY,       "DropBoxWidget/Position/Y");
LAMEXP_MAKE_ID(favoriteOutputFolders,        "OutputDirectory/Favorites");
LAMEXP_MAKE_ID(forceStereoDownmix,           "AdvancedOptions/StereoDownmix/Force");
LAMEXP_MAKE_ID(hibernateComputer,            "AdvancedOptions/HibernateComputerOnShutdown");
LAMEXP_MAKE_ID(interfaceStyle,               "InterfaceStyle");
LAMEXP_MAKE_ID(keepOriginalDataTime,         "AdvancedOptions/FileOperations/KeepOriginalDataTime");
LAMEXP_MAKE_ID(lameAlgoQuality,              "AdvancedOptions/LAME/AlgorithmQuality");
LAMEXP_MAKE_ID(lameChannelMode,              "AdvancedOptions/LAME/ChannelMode");
LAMEXP_MAKE_ID(licenseAccepted,              "LicenseAccepted");
LAMEXP_MAKE_ID(maximumInstances,             "AdvancedOptions/Threading/MaximumInstances");
LAMEXP_MAKE_ID(metaInfoPosition,             "MetaInformation/PlaylistPosition");
LAMEXP_MAKE_ID(mostRecentInputPath,          "InputDirectory/MostRecentPath");
LAMEXP_MAKE_ID(neroAACEnable2Pass,           "AdvancedOptions/AACEnc/Enable2Pass");
LAMEXP_MAKE_ID(neroAacNotificationsEnabled,  "Flags/EnableNeroAacNotifications");
LAMEXP_MAKE_ID(normalizationFilterEnabled,   "AdvancedOptions/VolumeNormalization/Enabled");
LAMEXP_MAKE_ID(normalizationFilterDynamic,   "AdvancedOptions/VolumeNormalization/UseDynAudNorm");
LAMEXP_MAKE_ID(normalizationFilterCoupled,    "AdvancedOptions/VolumeNormalization/ChannelCoupling");
LAMEXP_MAKE_ID(normalizationFilterMaxVolume, "AdvancedOptions/VolumeNormalization/MaxVolume");
LAMEXP_MAKE_ID(normalizationFilterSize,      "AdvancedOptions/VolumeNormalization/FilterLength");
LAMEXP_MAKE_ID(opusComplexity,               "AdvancedOptions/Opus/EncodingComplexity");
LAMEXP_MAKE_ID(opusDisableResample,          "AdvancedOptions/Opus/DisableResample");
LAMEXP_MAKE_ID(opusFramesize,                "AdvancedOptions/Opus/FrameSize");
LAMEXP_MAKE_ID(opusOptimizeFor,              "AdvancedOptions/Opus/OptimizeForSignalType");
LAMEXP_MAKE_ID(outputDir,                    "OutputDirectory/SelectedPath");
LAMEXP_MAKE_ID(outputToSourceDir,            "OutputDirectory/OutputToSourceFolder");
LAMEXP_MAKE_ID(overwriteMode,                "AdvancedOptions/FileOperations/OverwriteMode");
LAMEXP_MAKE_ID(prependRelativeSourcePath,    "OutputDirectory/PrependRelativeSourcePath");
LAMEXP_MAKE_ID(renameFiles_regExpEnabled,    "AdvancedOptions/RenameOutputFiles/RegExp/Enabled");
LAMEXP_MAKE_ID(renameFiles_regExpSearch,     "AdvancedOptions/RenameOutputFiles/RegExp/SearchPattern");
LAMEXP_MAKE_ID(renameFiles_regExpReplace,    "AdvancedOptions/RenameOutputFiles/RegExp/Replacement");
LAMEXP_MAKE_ID(renameFiles_renameEnabled,    "AdvancedOptions/RenameOutputFiles/Rename/Enabled");
LAMEXP_MAKE_ID(renameFiles_renamePattern,    "AdvancedOptions/RenameOutputFiles/Rename/Pattern");
LAMEXP_MAKE_ID(renameFiles_fileExtension,    "AdvancedOptions/RenameOutputFiles/FileExtensions/Overwrite");
LAMEXP_MAKE_ID(samplingRate,                 "AdvancedOptions/Common/Resampling");
LAMEXP_MAKE_ID(shellIntegrationEnabled,      "Flags/EnableShellIntegration");
LAMEXP_MAKE_ID(slowStartup,                  "Flags/SlowStartupDetected");
LAMEXP_MAKE_ID(soundsEnabled,                "Flags/EnableSounds");
LAMEXP_MAKE_ID(toneAdjustBass,               "AdvancedOptions/ToneAdjustment/Bass");
LAMEXP_MAKE_ID(toneAdjustTreble,             "AdvancedOptions/ToneAdjustment/Treble");
LAMEXP_MAKE_ID(versionNumber,                "VersionNumber");
LAMEXP_MAKE_ID(writeMetaTags,                "Flags/WriteMetaTags");

//LUT
const int SettingsModel::samplingRates[8] = {0, 16000, 22050, 24000, 32000, 44100, 48000, -1};

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

SettingsModel::SettingsModel(void)
:
	m_configCache(NULL)
{
	QString configPath = "LameXP.ini";
	
	if(!lamexp_version_portable())
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
	const QStringList childGroups = configFile->childGroups();

	//Clean-up settings
	if (!childGroups.empty())
	{
		static const int MAX_GROUPS = 3;
		QRegExp filter("^LameXP_(\\d+)(\\d\\d)(\\d\\d\\d\\d\\d)$");
		QStringList obsoleteGroups;
		for (QStringList::ConstIterator iter = childGroups.constBegin(); iter != childGroups.constEnd(); iter++)
		{
			if (filter.indexIn(*iter) >= 0)
			{
				quint32 temp[3] = { 0, 0, 0 };
				if (MUtils::regexp_parse_uint32(filter, temp, 3))
				{
					if ((temp[0] < lamexp_version_major()) || ((temp[0] == lamexp_version_major()) && ((temp[1] < lamexp_version_minor()) || ((temp[1] == lamexp_version_minor()) && (temp[2] < lamexp_version_confg())))))
					{
						obsoleteGroups.append(*iter);
					}
				}
			}
		}
		if (obsoleteGroups.count() > MAX_GROUPS)
		{
			qSort(obsoleteGroups);
			for (int i = 0; i < MAX_GROUPS; i++)
			{
				obsoleteGroups.removeLast();
			}
			for (QStringList::ConstIterator iter = obsoleteGroups.constBegin(); iter != obsoleteGroups.constEnd(); iter++)
			{
				qWarning("Deleting obsolete group from config: %s", MUTILS_UTF8(*iter));
				REMOVE_GROUP(configFile, (*iter));
			}
		}
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
	MUTILS_DELETE(m_configCache);
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
	
	if(EncoderRegistry::getAacEncoder() == AAC_ENCODER_NONE)
	{
		if(this->compressionEncoder() == SettingsModel::AACEncoder)
		{
			qWarning("AAC encoder selected, but not available any more. Reverting to MP3!");
			this->compressionEncoder(SettingsModel::MP3Encoder);
		}
	}
	
	if(this->outputDir().isEmpty() || (!dir_exists(this->outputDir())))
	{
		qWarning("Output directory not set yet or does NOT exist anymore -> resetting!");
		const QString outputDir = find_existing_ancestor(this->outputDir());
		this->outputDir((!outputDir.isEmpty()) ? outputDir : defaultDirectory());
	}

	if(this->mostRecentInputPath().isEmpty() || (!dir_exists(this->mostRecentInputPath())))
	{
		qWarning("Most recent input directory not set yet or does NOT exist anymore -> resetting!");
		const QString inputPath = find_existing_ancestor(this->mostRecentInputPath());
		this->mostRecentInputPath((!inputPath.isEmpty()) ? inputPath : defaultDirectory());
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

	QStringList translations;
	if(MUtils::Translation::enumerate(translations) > 0)
	{
		if(!translations.contains(this->currentLanguage(), Qt::CaseInsensitive))
		{
			qWarning("Current language \"%s\" is unknown, reverting to default language!", this->currentLanguage().toLatin1().constData());
			this->currentLanguage(defaultLanguage());
		}
	}

	if(this->hibernateComputer())
	{
		if(!MUtils::OS::is_hibernation_supported())
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
// Private Functions
////////////////////////////////////////////////////////////

QString SettingsModel::defaultLanguage(void) const
{
	QMutexLocker lock(&m_defaultLangLock);

	//Default already initialized?
	if(!m_defaultLanguage.isNull())
	{
		return *m_defaultLanguage;
	}

	//Detect system langauge
	QLocale systemLanguage= QLocale::system();
	qDebug("[Locale]");
	qDebug("Language: %s (%d)", MUTILS_UTF8(QLocale::languageToString(systemLanguage.language())), systemLanguage.language());
	qDebug("Country is: %s (%d)", MUTILS_UTF8(QLocale::countryToString(systemLanguage.country())), systemLanguage.country());
	qDebug("Script is: %s (%d)\n", MUTILS_UTF8(QLocale::scriptToString(systemLanguage.script())), systemLanguage.script());

	//Check if we can use the default translation
	if(systemLanguage.language() == QLocale::English /*|| systemLanguage.language() == QLocale::C*/)
	{
		m_defaultLanguage.reset(new QString(MUtils::Translation::DEFAULT_LANGID));
		return MUtils::Translation::DEFAULT_LANGID;
	}

	QStringList languages;
	if(MUtils::Translation::enumerate(languages) > 0)
	{
		//Try to find a suitable translation for the user's system language *and* country
		for(QStringList::ConstIterator iter = languages.constBegin(); iter != languages.constEnd(); iter++)
		{
			if(MUtils::Translation::get_sysid(*iter) == systemLanguage.language())
			{
				if(MUtils::Translation::get_country(*iter) == systemLanguage.country())
				{
					m_defaultLanguage.reset(new QString(*iter));
					return (*iter);
				}
			}
		}

		//Try to find a suitable translation for the user's system language
		for(QStringList::ConstIterator iter = languages.constBegin(); iter != languages.constEnd(); iter++)
		{
			if(MUtils::Translation::get_sysid(*iter) == systemLanguage.language())
			{
				m_defaultLanguage.reset(new QString(*iter));
				return (*iter);
			}
		}
	}

	//Fall back to the default translation
	m_defaultLanguage.reset(new QString(MUtils::Translation::DEFAULT_LANGID));
	return MUtils::Translation::DEFAULT_LANGID;
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
			MUtils::OS::sleep_ms(1);
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
LAMEXP_MAKE_OPTION_B(autoUpdateEnabled, (!lamexp_version_portable()));
LAMEXP_MAKE_OPTION_S(autoUpdateLastCheck, "Never")
LAMEXP_MAKE_OPTION_B(bitrateManagementEnabled, false)
LAMEXP_MAKE_OPTION_I(bitrateManagementMaxRate, 500)
LAMEXP_MAKE_OPTION_I(bitrateManagementMinRate, 32)
LAMEXP_MAKE_OPTION_I(compressionAbrBitrateAacEnc, 19)
LAMEXP_MAKE_OPTION_I(compressionAbrBitrateAften, 17)
LAMEXP_MAKE_OPTION_I(compressionAbrBitrateDcaEnc, 13)
LAMEXP_MAKE_OPTION_I(compressionAbrBitrateFLAC, 5)
LAMEXP_MAKE_OPTION_I(compressionAbrBitrateLAME, 10)
LAMEXP_MAKE_OPTION_I(compressionAbrBitrateMacEnc, 2)
LAMEXP_MAKE_OPTION_I(compressionAbrBitrateOggEnc, 16)
LAMEXP_MAKE_OPTION_I(compressionAbrBitrateOpusEnc, 11)
LAMEXP_MAKE_OPTION_I(compressionAbrBitrateWave, 0)
LAMEXP_MAKE_OPTION_I(compressionCbrBitrateAacEnc, 19)
LAMEXP_MAKE_OPTION_I(compressionCbrBitrateAften, 17)
LAMEXP_MAKE_OPTION_I(compressionCbrBitrateDcaEnc, 13)
LAMEXP_MAKE_OPTION_I(compressionCbrBitrateFLAC, 5)
LAMEXP_MAKE_OPTION_I(compressionCbrBitrateLAME, 10)
LAMEXP_MAKE_OPTION_I(compressionCbrBitrateMacEnc, 2)
LAMEXP_MAKE_OPTION_I(compressionCbrBitrateOggEnc, 16)
LAMEXP_MAKE_OPTION_I(compressionCbrBitrateOpusEnc, 11)
LAMEXP_MAKE_OPTION_I(compressionCbrBitrateWave, 0)
LAMEXP_MAKE_OPTION_I(compressionEncoder, 0)
LAMEXP_MAKE_OPTION_I(compressionRCModeAacEnc, 0)
LAMEXP_MAKE_OPTION_I(compressionRCModeAften, 0)
LAMEXP_MAKE_OPTION_I(compressionRCModeDcaEnc, 2)
LAMEXP_MAKE_OPTION_I(compressionRCModeFLAC, 0)
LAMEXP_MAKE_OPTION_I(compressionRCModeLAME, 0)
LAMEXP_MAKE_OPTION_I(compressionRCModeMacEnc, 0)
LAMEXP_MAKE_OPTION_I(compressionRCModeOggEnc, 0)
LAMEXP_MAKE_OPTION_I(compressionRCModeOpusEnc, 0)
LAMEXP_MAKE_OPTION_I(compressionRCModeWave, 2)
LAMEXP_MAKE_OPTION_I(compressionVbrQualityAacEnc, 10)
LAMEXP_MAKE_OPTION_I(compressionVbrQualityAften, 15)
LAMEXP_MAKE_OPTION_I(compressionVbrQualityDcaEnc, 13)
LAMEXP_MAKE_OPTION_I(compressionVbrQualityFLAC, 5)
LAMEXP_MAKE_OPTION_I(compressionVbrQualityLAME, 7)
LAMEXP_MAKE_OPTION_I(compressionVbrQualityMacEnc, 2)
LAMEXP_MAKE_OPTION_I(compressionVbrQualityOggEnc, 7)
LAMEXP_MAKE_OPTION_I(compressionVbrQualityOpusEnc, 11)
LAMEXP_MAKE_OPTION_I(compressionVbrQualityWave, 0)
LAMEXP_MAKE_OPTION_B(createPlaylist, true)
LAMEXP_MAKE_OPTION_S(currentLanguage, defaultLanguage())
LAMEXP_MAKE_OPTION_S(currentLanguageFile, QString())
LAMEXP_MAKE_OPTION_S(customParametersAacEnc, QString())
LAMEXP_MAKE_OPTION_S(customParametersAften, QString())
LAMEXP_MAKE_OPTION_S(customParametersDcaEnc, QString())
LAMEXP_MAKE_OPTION_S(customParametersFLAC, QString())
LAMEXP_MAKE_OPTION_S(customParametersLAME, QString())
LAMEXP_MAKE_OPTION_S(customParametersMacEnc, QString())
LAMEXP_MAKE_OPTION_S(customParametersOggEnc, QString())
LAMEXP_MAKE_OPTION_S(customParametersOpusEnc, QString())
LAMEXP_MAKE_OPTION_S(customParametersWave, QString())
LAMEXP_MAKE_OPTION_S(customTempPath, QDesktopServices::storageLocation(QDesktopServices::TempLocation))
LAMEXP_MAKE_OPTION_B(customTempPathEnabled, false)
LAMEXP_MAKE_OPTION_B(dropBoxWidgetEnabled, true)
LAMEXP_MAKE_OPTION_I(dropBoxWidgetPositionX, -1)
LAMEXP_MAKE_OPTION_I(dropBoxWidgetPositionY, -1)
LAMEXP_MAKE_OPTION_S(favoriteOutputFolders, QString())
LAMEXP_MAKE_OPTION_B(forceStereoDownmix, false)
LAMEXP_MAKE_OPTION_B(hibernateComputer, false)
LAMEXP_MAKE_OPTION_I(interfaceStyle, 0)
LAMEXP_MAKE_OPTION_B(keepOriginalDataTime, false)
LAMEXP_MAKE_OPTION_I(lameAlgoQuality, 2)
LAMEXP_MAKE_OPTION_I(lameChannelMode, 0)
LAMEXP_MAKE_OPTION_I(licenseAccepted, 0)
LAMEXP_MAKE_OPTION_U(maximumInstances, 0)
LAMEXP_MAKE_OPTION_U(metaInfoPosition, UINT_MAX)
LAMEXP_MAKE_OPTION_S(mostRecentInputPath, defaultDirectory())
LAMEXP_MAKE_OPTION_B(neroAACEnable2Pass, true)
LAMEXP_MAKE_OPTION_B(neroAacNotificationsEnabled, true)
LAMEXP_MAKE_OPTION_B(normalizationFilterEnabled, false)
LAMEXP_MAKE_OPTION_B(normalizationFilterDynamic, false)
LAMEXP_MAKE_OPTION_B(normalizationFilterCoupled, true)
LAMEXP_MAKE_OPTION_I(normalizationFilterMaxVolume, -50)
LAMEXP_MAKE_OPTION_I(normalizationFilterSize, 31)
LAMEXP_MAKE_OPTION_I(opusComplexity, 10)
LAMEXP_MAKE_OPTION_B(opusDisableResample, false)
LAMEXP_MAKE_OPTION_I(opusFramesize, 3)
LAMEXP_MAKE_OPTION_I(opusOptimizeFor, 0)
LAMEXP_MAKE_OPTION_S(outputDir, defaultDirectory())
LAMEXP_MAKE_OPTION_B(outputToSourceDir, false)
LAMEXP_MAKE_OPTION_I(overwriteMode, Overwrite_KeepBoth)
LAMEXP_MAKE_OPTION_B(prependRelativeSourcePath, false)
LAMEXP_MAKE_OPTION_B(renameFiles_regExpEnabled, false)
LAMEXP_MAKE_OPTION_S(renameFiles_regExpSearch, QString())
LAMEXP_MAKE_OPTION_S(renameFiles_regExpReplace, QString())
LAMEXP_MAKE_OPTION_B(renameFiles_renameEnabled, false)
LAMEXP_MAKE_OPTION_S(renameFiles_renamePattern, "[<TrackNo>] <Artist> - <Title>")
LAMEXP_MAKE_OPTION_S(renameFiles_fileExtension, QString())
LAMEXP_MAKE_OPTION_I(samplingRate, 0)
LAMEXP_MAKE_OPTION_B(shellIntegrationEnabled, !lamexp_version_portable())
LAMEXP_MAKE_OPTION_B(slowStartup, false)
LAMEXP_MAKE_OPTION_B(soundsEnabled, true)
LAMEXP_MAKE_OPTION_I(toneAdjustBass, 0)
LAMEXP_MAKE_OPTION_I(toneAdjustTreble, 0)
LAMEXP_MAKE_OPTION_B(writeMetaTags, true)
