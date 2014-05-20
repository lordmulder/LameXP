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

//Build date
static QDate g_lamexp_version_date;
static const char *g_lamexp_months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static const char *g_lamexp_version_raw_date = __DATE__;
static const char *g_lamexp_version_raw_time = __TIME__;

//Official web-site URL
static const char *g_lamexp_website_url = "http://lamexp.sourceforge.net/";
static const char *g_lamexp_support_url = "http://forum.doom9.org/showthread.php?t=157726";
static const char *g_lamexp_mulders_url = "http://muldersoft.com/";

//Tool versions (expected versions!)
static const unsigned int g_lamexp_toolver_neroaac = VER_LAMEXP_TOOL_NEROAAC;
static const unsigned int g_lamexp_toolver_fhgaacenc = VER_LAMEXP_TOOL_FHGAACENC;
static const unsigned int g_lamexp_toolver_qaacenc = VER_LAMEXP_TOOL_QAAC;
static const unsigned int g_lamexp_toolver_coreaudio = VER_LAMEXP_TOOL_COREAUDIO;

///////////////////////////////////////////////////////////////////////////////
// COMPILER INFO
///////////////////////////////////////////////////////////////////////////////

/*
 * Disclaimer: Parts of the following code were borrowed from MPC-HC project: http://mpc-hc.sf.net/
 */

//Compiler detection
#if defined(__INTEL_COMPILER)
	#if (__INTEL_COMPILER >= 1300)
		static const char *g_lamexp_version_compiler = "ICL 13." LAMEXP_MAKE_STR(__INTEL_COMPILER_BUILD_DATE);
	#elif (__INTEL_COMPILER >= 1200)
		static const char *g_lamexp_version_compiler = "ICL 12." LAMEXP_MAKE_STR(__INTEL_COMPILER_BUILD_DATE);
	#elif (__INTEL_COMPILER >= 1100)
		static const char *g_lamexp_version_compiler = "ICL 11.x";
	#elif (__INTEL_COMPILER >= 1000)
		static const char *g_lamexp_version_compiler = "ICL 10.x";
	#else
		#error Compiler is not supported!
	#endif
#elif defined(_MSC_VER)
	#if (_MSC_VER == 1800)
		#if (_MSC_FULL_VER < 180021005)
			static const char *g_lamexp_version_compiler = "MSVC 2013-Beta";
		#elif (_MSC_FULL_VER < 180030501)
			static const char *g_lamexp_version_compiler = "MSVC 2013";
		#elif (_MSC_FULL_VER == 180030501)
			static const char *g_lamexp_version_compiler = "MSVC 2013.2";
		#else
			#error Compiler version is not supported yet!
		#endif
	#elif (_MSC_VER == 1700)
		#if (_MSC_FULL_VER < 170050727)
			static const char *g_lamexp_version_compiler = "MSVC 2012-Beta";
		#elif (_MSC_FULL_VER < 170051020)
			static const char *g_lamexp_version_compiler = "MSVC 2012";
		#elif (_MSC_FULL_VER < 170051106)
			static const char *g_lamexp_version_compiler = "MSVC 2012.1-CTP";
		#elif (_MSC_FULL_VER < 170060315)
			static const char *g_lamexp_version_compiler = "MSVC 2012.1";
		#elif (_MSC_FULL_VER < 170060610)
			static const char *g_lamexp_version_compiler = "MSVC 2012.2";
		#elif (_MSC_FULL_VER < 170061030)
			static const char *g_lamexp_version_compiler = "MSVC 2012.3";
		#elif (_MSC_FULL_VER == 170061030)
			static const char *g_lamexp_version_compiler = "MSVC 2012.4";
		#else
			#error Compiler version is not supported yet!
		#endif
	#elif (_MSC_VER == 1600)
		#if (_MSC_FULL_VER < 160040219)
			static const char *g_lamexp_version_compiler = "MSVC 2010";
		#elif (_MSC_FULL_VER == 160040219)
			static const char *g_lamexp_version_compiler = "MSVC 2010-SP1";
		#else
			#error Compiler version is not supported yet!
		#endif
	#elif (_MSC_VER == 1500)
		#if (_MSC_FULL_VER >= 150030729)
			static const char *g_lamexp_version_compiler = "MSVC 2008-SP1";
		#else
			static const char *g_lamexp_version_compiler = "MSVC 2008";
		#endif
	#else
		#error Compiler is not supported!
	#endif

	// Note: /arch:SSE and /arch:SSE2 are only available for the x86 platform
	#if !defined(_M_X64) && defined(_M_IX86_FP)
		#if (_M_IX86_FP == 1)
			LAMEXP_COMPILER_WARNING("SSE instruction set is enabled!")
		#elif (_M_IX86_FP == 2)
			LAMEXP_COMPILER_WARNING("SSE2 (or higher) instruction set is enabled!")
		#endif
	#endif
#else
	#error Compiler is not supported!
#endif

//Architecture detection
#if defined(_M_X64)
	static const char *g_lamexp_version_arch = "x64";
#elif defined(_M_IX86)
	static const char *g_lamexp_version_arch = "x86";
#else
	#error Architecture is not supported!
#endif

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
const char *lamexp_version_release(void)    { return g_lamexp_version.ver_release_name; }
const char *lamexp_version_time(void)       { return g_lamexp_version_raw_time; }
const char *lamexp_version_compiler(void)   { return g_lamexp_version_compiler; }
const char *lamexp_version_arch(void)       { return g_lamexp_version_arch; }
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
	return lamexp_version_date().addDays(LAMEXP_DEBUG ? 7 : 30);
}

/*
 * Convert month string to integer value
 */
static int lamexp_month2int(const char *str)
{
	int ret = 0;

	for(int j = 0; j < 12; j++)
	{
		if(!_strcmpi(str, g_lamexp_months[j]))
		{
			ret = j+1;
			break;
		}
	}

	return ret;
}

/*
 * Get build date date
 */
const QDate &lamexp_version_date(void)
{
	//Format of __DATE__ is defined as: "MMM DD YYYY"
	if(!g_lamexp_version_date.isValid())
	{
		int date[3] = {0, 0, 0};
		char temp_m[4], temp_d[3], temp_y[5];

		temp_m[0] = g_lamexp_version_raw_date[0x0];
		temp_m[1] = g_lamexp_version_raw_date[0x1];
		temp_m[2] = g_lamexp_version_raw_date[0x2];
		temp_m[3] = 0x00;

		temp_d[0] = g_lamexp_version_raw_date[0x4];
		temp_d[1] = g_lamexp_version_raw_date[0x5];
		temp_d[2] = 0x00;

		temp_y[0] = g_lamexp_version_raw_date[0x7];
		temp_y[1] = g_lamexp_version_raw_date[0x8];
		temp_y[2] = g_lamexp_version_raw_date[0x9];
		temp_y[3] = g_lamexp_version_raw_date[0xA];
		temp_y[4] = 0x00;
		
		date[0] = atoi(temp_y);
		date[1] = lamexp_month2int(temp_m);
		date[2] = atoi(temp_d);

		if((date[0] > 0) && (date[1] > 0) && (date[2] > 0))
		{

			g_lamexp_version_date = QDate(date[0], date[1], date[2]);
		}
		else
		{
			THROW("Internal error: Date format could not be recognized!");
		}
	}

	return g_lamexp_version_date;
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
