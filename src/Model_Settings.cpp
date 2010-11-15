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

static const char *g_settingsId_versionNumber = "VersionNumber";
static const char *g_settingsId_licenseAccepted = "LicenseAccepted";
static const char *g_settingsId_interfaceStyle = "InterfaceStyle";
static const char *g_settingsId_compressionEncoder = "Compression/Encoder";
static const char *g_settingsId_compressionRCMode = "Compression/RCMode";
static const char *g_settingsId_compressionBitrate = "Compression/Bitrate";

#define MAKE_GETTER(OPT,DEF) int SettingsModel::OPT(void) { return m_settings->value(g_settingsId_##OPT, DEF).toInt(); }
#define MAKE_SETTER(OPT) void SettingsModel::OPT(int value) { m_settings->setValue(g_settingsId_##OPT, value); }

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
}

////////////////////////////////////////////////////////////
// Getter and Setter
////////////////////////////////////////////////////////////

MAKE_GETTER(licenseAccepted, 0)
MAKE_SETTER(licenseAccepted)

MAKE_GETTER(interfaceStyle, 0)
MAKE_SETTER(interfaceStyle)

MAKE_GETTER(compressionEncoder, 0)
MAKE_SETTER(compressionEncoder)

MAKE_GETTER(compressionRCMode, 0)
MAKE_SETTER(compressionRCMode)

MAKE_GETTER(compressionBitrate, 0)
MAKE_SETTER(compressionBitrate)
