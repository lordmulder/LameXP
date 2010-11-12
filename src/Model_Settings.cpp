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

static const char *g_settingsVersionNumber = "VersionNumber";
static const char *g_settingsLicenseAccepted = "LicenseAccepted";
static const char *g_settingsInterfaceStyle = "InterfaceStyle";

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

SettingsModel::SettingsModel(void)
{
	QString appPath = QDesktopServices::storageLocation(QDesktopServices::DataLocation);
	m_settings = new QSettings(appPath.append("/config.ini"), QSettings::IniFormat);
	m_settings->beginGroup(QString().sprintf("LameXP_%u%02u%05u", lamexp_version_major(), lamexp_version_minor(), lamexp_version_build()));
	m_settings->setValue(g_settingsVersionNumber, QApplication::applicationVersion());
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

int SettingsModel::licenseAccepted(void) { return m_settings->value(g_settingsLicenseAccepted, 0).toInt(); }
void SettingsModel::setLicenseAccepted(int value) { m_settings->setValue(g_settingsLicenseAccepted, value); }

int SettingsModel::interfaceStyle(void) { return m_settings->value(g_settingsInterfaceStyle, 0).toInt(); }
void SettingsModel::setInterfaceStyle(int value) { m_settings->setValue(g_settingsInterfaceStyle, value); }
