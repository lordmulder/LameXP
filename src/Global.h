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

#pragma once

//Target version
#include "Targetver.h"

//inlcude C standard library
#include <stdio.h>
#include <tchar.h>

//Windows includes
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

//Declarations
class QString;
class QStringList;
class QDate;
class QTime;
class QIcon;
class QWidget;
class LockedFile;
enum QtMsgType;

//Types definitions
typedef struct
{
	int family;
	int model;
	int stepping;
	int count;
	bool x64;
	bool mmx;
	bool sse;
	bool sse2;
	bool sse3;
	bool ssse3;
	char vendor[0x40];
	char brand[0x40];
	bool intel;
}
lamexp_cpu_t;

//Known folders
typedef enum
{
	lamexp_folder_localappdata = 0,
	lamexp_folder_programfiles = 2,
	lamexp_folder_systemfolder = 3
}
lamexp_known_folder_t;

//LameXP version info
unsigned int lamexp_version_major(void);
unsigned int lamexp_version_minor(void);
unsigned int lamexp_version_build(void);
const QDate &lamexp_version_date(void);
const char *lamexp_version_time(void);
const char *lamexp_version_release(void);
bool lamexp_version_demo(void);
const char *lamexp_version_compiler(void);
const char *lamexp_version_arch(void);
QDate lamexp_version_expires(void);
unsigned int lamexp_toolver_neroaac(void);
unsigned int lamexp_toolver_fhgaacenc(void);
unsigned int lamexp_toolver_qaacenc(void);
unsigned int lamexp_toolver_coreaudio(void);
const char *lamexp_website_url(void);
const char *lamexp_support_url(void);
DWORD lamexp_get_os_version(void);

//Public functions
void lamexp_init_console(int argc, char* argv[]);
bool lamexp_init_qt(int argc, char* argv[]);
int lamexp_init_ipc(void);
LONG WINAPI lamexp_exception_handler(__in struct _EXCEPTION_POINTERS *ExceptionInfo);
void lamexp_invalid_param_handler(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t);
void lamexp_message_handler(QtMsgType type, const char *msg);
void lamexp_register_tool(const QString &toolName, LockedFile *file, unsigned int version = 0);
bool lamexp_check_tool(const QString &toolName);
const QString lamexp_lookup_tool(const QString &toolName);
unsigned int lamexp_tool_version(const QString &toolName);
void lamexp_finalization(void);
QString lamexp_rand_str(void);
const QString &lamexp_temp_folder2(void);
void lamexp_ipc_read(unsigned int *command, char* message, size_t buffSize);
void lamexp_ipc_send(unsigned int command, const char* message);
lamexp_cpu_t lamexp_detect_cpu_features(int argc = 0, char **argv = NULL);
bool lamexp_portable_mode(void);
bool lamexp_shutdown_computer(const QString &message, const unsigned long timeout = 30, const bool forceShutdown = true, const bool hibernate = false);
bool lamexp_is_hibernation_supported(void);
QIcon lamexp_app_icon(const QDate *date = NULL, const QTime *time = NULL);

//Translation support
QStringList lamexp_query_translations(void);
bool lamexp_translation_register(const QString &langId, const QString &qmFile, const QString &langName, unsigned int &systemId, unsigned int &country);
QString lamexp_translation_name(const QString &language);
unsigned int lamexp_translation_sysid(const QString &langId);
unsigned int lamexp_translation_country(const QString &langId);
bool lamexp_install_translator_from_file(const QString &qmFile);
bool lamexp_install_translator(const QString &language);
QStringList lamexp_available_codepages(bool noAliases = true);
static const char* LAMEXP_DEFAULT_LANGID = "en";

//Auxiliary functions
bool lamexp_clean_folder(const QString &folderPath);
const QString lamexp_version2string(const QString &pattern, unsigned int version, const QString &defaultText);
QString lamexp_known_folder(lamexp_known_folder_t folder_id);
unsigned __int64 lamexp_free_diskspace(const QString &path, bool *ok = NULL);
bool lamexp_remove_file(const QString &filename);
bool lamexp_themes_enabled(void);
void lamexp_blink_window(QWidget *poWindow, unsigned int count = 10, unsigned int delay = 150);
const QString lamexp_clean_filename(const QString &str);
const QString lamexp_clean_filepath(const QString &str);

//Debug-only functions
SIZE_T lamexp_dbg_private_bytes(void);

//Helper macros
#define LAMEXP_DELETE(PTR) if(PTR) { delete PTR; PTR = NULL; }
#define LAMEXP_DELETE_ARRAY(PTR) if(PTR) { delete [] PTR; PTR = NULL; }
#define LAMEXP_SAFE_FREE(PTR) if(PTR) { free((void*) PTR); PTR = NULL; }
#define LAMEXP_CLOSE(HANDLE) if(HANDLE != NULL && HANDLE != INVALID_HANDLE_VALUE) { CloseHandle(HANDLE); HANDLE = NULL; }
#define QWCHAR(STR) reinterpret_cast<const wchar_t*>(STR.utf16())
#define WCHAR2QSTR(STR) QString::fromUtf16(reinterpret_cast<const unsigned short*>(STR))
#define	LAMEXP_DYNCAST(OUT,CLASS,SRC) try { OUT = dynamic_cast<CLASS>(SRC); } catch(std::bad_cast) { OUT = NULL; }
#define LAMEXP_BOOL(X) (X ? "1" : "0")
#define LAMEXP_MAKE_STRING_EX(X) #X
#define LAMEXP_MAKE_STRING(X) LAMEXP_MAKE_STRING_EX(X)
#define LAMEXP_COMPILER_WARNING(TXT) __pragma(message(__FILE__ "(" LAMEXP_MAKE_STRING(__LINE__) ") : warning: " TXT))
#define NOBR(STR) QString("<nobr>%1</nobr>").arg(STR).replace("-", "&minus;")

//Output Qt debug message (Unicode-safe versions)
//#define qDebug64(FORMAT, ...) qDebug("@BASE64@%s", QString(FORMAT).arg(__VA_ARGS__).toUtf8().toBase64().constData());
//#define qWarning64(FORMAT, ...) qWarning("@BASE64@%s", QString(FORMAT).arg(__VA_ARGS__).toUtf8().toBase64().constData());
//#define qFatal64(FORMAT, ...) qFatal("@BASE64@%s", QString(FORMAT).arg(__VA_ARGS__).toUtf8().toBase64().constData());

//Check for debug build
#if defined(_DEBUG) && defined(QT_DEBUG) && !defined(NDEBUG) && !defined(QT_NO_DEBUG)
	#define LAMEXP_DEBUG (1)
#elif defined(NDEBUG) && defined(QT_NO_DEBUG) && !defined(_DEBUG) && !defined(QT_DEBUG)
	#define LAMEXP_DEBUG (0)
#else
	#error Inconsistent debug defines detected!
#endif

//Memory check
#if defined(_DEBUG)
#define LAMEXP_MEMORY_CHECK(CMD) \
{ \
	SIZE_T _privateBytesBefore = lamexp_dbg_private_bytes(); \
	CMD; \
	SIZE_T _privateBytesLeak = (lamexp_dbg_private_bytes() - _privateBytesBefore) / 1024; \
	if(_privateBytesLeak > 10) { \
		qWarning("Memory leak: Lost %u KiloBytes.", _privateBytesLeak); \
	} \
}
#else
#define LAMEXP_MEMORY_CHECK(CMD) CMD
#endif

//Check for CPU-compatibility options
#if !defined(_M_X64) && defined(_M_IX86_FP)
	#if (_M_IX86_FP != 0)
		#error We should not enabled SSE or SSE2 in release builds!
	#endif
#endif
