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

#pragma once

#ifdef _MSC_VER
#define _CRT_RAND_S
#endif

#include <cstdlib>
#include <QtGlobal>

//Forward declarations
class QDate;
class QStringList;
class QIcon;
class LockedFile;

///////////////////////////////////////////////////////////////////////////////
// GLOBAL CONSTANTS
///////////////////////////////////////////////////////////////////////////////

extern const char* LAMEXP_DEFAULT_LANGID;
extern const char* LAMEXP_DEFAULT_TRANSLATION;

///////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

/*
 * Translation Support
 */
QStringList lamexp_query_translations(void);
unsigned int lamexp_translation_country(const QString &langId);
bool lamexp_translation_init(void);
QString lamexp_translation_name(const QString &language);
bool lamexp_translation_register(const QString &langId, const QString &qmFile, const QString &langName, unsigned int &systemId, unsigned int &country);
unsigned int lamexp_translation_sysid(const QString &langId);
bool lamexp_install_translator(const QString &language);
bool lamexp_install_translator_from_file(const QString &qmFile);

/*
 * Tools Support
 */
bool           lamexp_tool_check   (const QString &toolName);
const QString& lamexp_tool_lookup  (const QString &toolName);
void           lamexp_tool_register(const QString &toolName, LockedFile *file, unsigned int version = 0, const QString *tag = NULL);
unsigned int   lamexp_tool_version (const QString &toolName, QString *tag = NULL);

/*
 * Version getters
 */
unsigned int lamexp_version_major    (void);
unsigned int lamexp_version_minor    (void);
unsigned int lamexp_version_build    (void);
unsigned int lamexp_version_confg    (void);
const char*  lamexp_version_release  (void);
bool         lamexp_version_portable (void);
bool         lamexp_version_demo     (void);
QDate&       lamexp_version_expires  (void);
unsigned int lamexp_toolver_neroaac  (void);
unsigned int lamexp_toolver_fhgaacenc(void);
unsigned int lamexp_toolver_qaacenc  (void);
unsigned int lamexp_toolver_coreaudio(void);


/*
 * URL getters
 */
const char *lamexp_website_url(void);
const char *lamexp_mulders_url(void);
const char *lamexp_support_url(void);
const char *lamexp_tracker_url(void);

/*
 * Misc Functions
 */
const QIcon&  lamexp_app_icon(void);
const QString lamexp_version2string(const QString &pattern, unsigned int version, const QString &defaultText, const QString *tag = NULL);

/*
 * Finalization
 */
void lamexp_finalization(void);

///////////////////////////////////////////////////////////////////////////////
// HELPER MACROS
///////////////////////////////////////////////////////////////////////////////

#define NOBR(STR) (QString("<nobr>%1</nobr>").arg((STR)).replace("-", "&minus;"))
