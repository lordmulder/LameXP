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

#include "Model_Settings.h"

#include "Global.h"

#include <QSettings>
#include <QDesktopServices>
#include <QApplication>
#include <QString>
#include <QFileInfo>

//Constants
static const char *g_settingsId_versionNumber = "VersionNumber";
static const char *g_settingsId_licenseAccepted = "LicenseAccepted";
static const char *g_settingsId_interfaceStyle = "InterfaceStyle";
static const char *g_settingsId_compressionEncoder = "Compression/Encoder";
static const char *g_settingsId_compressionRCMode = "Compression/RCMode";
static const char *g_settingsId_compressionBitrate = "Compression/Bitrate";
static const char *g_settingsId_outputDir = "OutputDirectory/SelectedPath";
static const char *g_settingsId_outputToSourceDir = "OutputDirectory/OutputToSourceFolder";
static const char *g_settingsId_writeMetaTags = "Flags/WriteMetaTags";
static const char *g_settingsId_createPlaylist = "Flags/AutoCreatePlaylist";
static const char *g_settingsId_autoUpdateLastCheck = "AutoUpdate/LastCheck";
static const char *g_settingsId_autoUpdateEnabled = "AutoUpdate/Enabled";
static const char *g_settingsId_soundsEnabled = "Flags/EnableSounds";

//Macros
#define MAKE_OPTION1(OPT,DEF) \
int SettingsModel::OPT(void) { return m_settings->value(g_settingsId_##OPT, DEF).toInt(); } \
void SettingsModel::OPT(int value) { m_settings->setValue(g_settingsId_##OPT, value); }

#define MAKE_OPTION2(OPT,DEF) \
QString SettingsModel::OPT(void) { return m_settings->value(g_settingsId_##OPT, DEF).toString().trimmed(); } \
void SettingsModel::OPT(const QString &value) { m_settings->setValue(g_settingsId_##OPT, value); }

#define MAKE_OPTION3(OPT,DEF) \
bool SettingsModel::OPT(void) { return m_settings->value(g_settingsId_##OPT, DEF).toBool(); } \
void SettingsModel::OPT(bool value) { m_settings->setValue(g_settingsId_##OPT, value); }

//LUT
const int SettingsModel::mp3Bitrates[15] = {32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1};

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

SettingsModel::SettingsModel(void)
{
	QString appPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
	m_settings = new QSettings(appPath.append("/config.ini"), QSettings::IniFormat);
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
		if(this->compressionEncoder() == SettingsModel::AACEncoder) this->compressionEncoder(SettingsModel::MP3Encoder);
	}
	if(this->outputDir().isEmpty() || !QFileInfo(this->outputDir()).isDir())
	{
		this->outputDir(QDesktopServices::storageLocation(QDesktopServices::MusicLocation));
	}
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
MAKE_OPTION3(writeMetaTags, true)
MAKE_OPTION3(createPlaylist, true)
MAKE_OPTION2(autoUpdateLastCheck, "Never")
MAKE_OPTION3(autoUpdateEnabled, true)
MAKE_OPTION3(soundsEnabled, true)
