///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2014 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Global.h"

//LameXP includes
#define LAMEXP_INC_CONFIG
#include "Resource.h"
#include "Config.h"

//MUtils
#include <MUtils/Version.h>

//Qt includes
#include <QApplication>
#include <QDate>
#include <QFileInfo>
#include <QReadWriteLock>

///////////////////////////////////////////////////////////////////////////////
// GLOBAL VARS
///////////////////////////////////////////////////////////////////////////////

//Build version
static const struct
{
	unsigned int ver_major;
	unsigned int ver_minor;
	unsigned int ver_build;
	unsigned int ver_confg;
	char *ver_release_name;
}
g_lamexp_version =
{
	VER_LAMEXP_MAJOR,
	(10 * VER_LAMEXP_MINOR_HI) + VER_LAMEXP_MINOR_LO,
	VER_LAMEXP_BUILD,
	VER_LAMEXP_CONFG,
	VER_LAMEXP_RNAME,
};

//Portable Mode
static struct
{
	bool bInitialized;
	bool bPortableModeEnabled;
	QReadWriteLock lock;
}
g_lamexp_portable;

//Official web-site URL
static const char *g_lamexp_website_url = "http://lamexp.sourceforge.net/";
static const char *g_lamexp_mulders_url = "http://muldersoft.com/";
static const char *g_lamexp_support_url = "http://forum.doom9.org/showthread.php?t=157726";
static const char *g_lamexp_tracker_url = "https://github.com/lordmulder/LameXP/issues";

//Tool versions (expected versions!)
static const unsigned int g_lamexp_toolver_neroaac   = VER_LAMEXP_TOOL_NEROAAC;
static const unsigned int g_lamexp_toolver_fhgaacenc = VER_LAMEXP_TOOL_FHGAACENC;
static const unsigned int g_lamexp_toolver_qaacenc   = VER_LAMEXP_TOOL_QAAC;
static const unsigned int g_lamexp_toolver_coreaudio = VER_LAMEXP_TOOL_COREAUDIO;

///////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

/*
 * Version getters
 */
unsigned int lamexp_version_major(void)     { return g_lamexp_version.ver_major; }
unsigned int lamexp_version_minor(void)     { return g_lamexp_version.ver_minor; }
unsigned int lamexp_version_build(void)     { return g_lamexp_version.ver_build; }
unsigned int lamexp_version_confg(void)     { return g_lamexp_version.ver_confg; }
const char*  lamexp_version_release(void)   { return g_lamexp_version.ver_release_name; }
unsigned int lamexp_toolver_neroaac(void)   { return g_lamexp_toolver_neroaac; }
unsigned int lamexp_toolver_fhgaacenc(void) { return g_lamexp_toolver_fhgaacenc; }
unsigned int lamexp_toolver_qaacenc(void)   { return g_lamexp_toolver_qaacenc; }
unsigned int lamexp_toolver_coreaudio(void) { return g_lamexp_toolver_coreaudio; }

/*
 * URL getters
 */
const char *lamexp_website_url(void) { return g_lamexp_website_url; }
const char *lamexp_mulders_url(void) { return g_lamexp_mulders_url; }
const char *lamexp_support_url(void) { return g_lamexp_support_url; }
const char *lamexp_tracker_url(void) { return g_lamexp_tracker_url; }

/*
 * Check for Demo (pre-release) version
 */
bool lamexp_version_demo(void)
{
	char buffer[128];
	bool releaseVersion = false;
	if(!strncpy_s(buffer, 128, g_lamexp_version.ver_release_name, _TRUNCATE))
	{
		char *context, *prefix = strtok_s(buffer, "-,; ", &context);
		if(prefix)
		{
			releaseVersion = (!_stricmp(prefix, "Final")) || (!_stricmp(prefix, "Hotfix"));
		}
	}
	return (!releaseVersion);
}

/*
 * Calculate expiration date
 */
QDate lamexp_version_expires(void)
{
	return MUtils::Version::app_build_date().addDays(MUTILS_DEBUG ? 7 : 30);
}

/*
 * Check for LameXP "portable" mode
 */
bool lamexp_portable_mode(void)
{
	QReadLocker readLock(&g_lamexp_portable.lock);

	if(g_lamexp_portable.bInitialized)
	{
		return g_lamexp_portable.bPortableModeEnabled;
	}
	
	readLock.unlock();
	QWriteLocker writeLock(&g_lamexp_portable.lock);

	if(!g_lamexp_portable.bInitialized)
	{
		if(VER_LAMEXP_PORTABLE_EDITION)
		{
			qWarning("LameXP portable edition!\n");
			g_lamexp_portable.bPortableModeEnabled = true;
		}
		else
		{
			QString baseName = QFileInfo(QApplication::applicationFilePath()).completeBaseName();
			int idx1 = baseName.indexOf("lamexp", 0, Qt::CaseInsensitive);
			int idx2 = baseName.lastIndexOf("portable", -1, Qt::CaseInsensitive);
			g_lamexp_portable.bPortableModeEnabled = ((idx1 >= 0) && (idx2 >= 0) && (idx1 < idx2));
		}
		g_lamexp_portable.bInitialized = true;
	}
	
	return g_lamexp_portable.bPortableModeEnabled;
}

///////////////////////////////////////////////////////////////////////////////
// INITIALIZATION
///////////////////////////////////////////////////////////////////////////////

extern "C" void _lamexp_global_init_versn(void)
{
	LAMEXP_ZERO_MEMORY(g_lamexp_portable);
}

///////////////////////////////////////////////////////////////////////////////
// FINALIZATION
///////////////////////////////////////////////////////////////////////////////

extern "C" void _lamexp_global_free_versn(void)
{
	/*nothing to do here*/
}
