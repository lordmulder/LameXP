///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2013 LoRd_MuldeR <MuldeR2@GMX.de>
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

#define _CRT_RAND_S
#include <cstdlib>

//Forward declarations
class QString;
class QStringList;
class QDate;
class QTime;
class QIcon;
class QWidget;
class QProcess;
class QColor;
class LockedFile;
enum QtMsgType;

//Variables
extern const char* LAMEXP_DEFAULT_LANGID;
extern const char* LAMEXP_DEFAULT_TRANSLATION;

///////////////////////////////////////////////////////////////////////////////
// TYPE DEFINITIONS
///////////////////////////////////////////////////////////////////////////////

//CPU features
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
	lamexp_folder_systemfolder = 3,
	lamexp_folder_systroot_dir = 4
}
lamexp_known_folder_t;

//LameXP user-defined events
typedef enum
{
	lamexp_event = 1000,           /*QEvent::User*/
	lamexp_event_queryendsession = lamexp_event + 666,
	lamexp_event_endsession      = lamexp_event + 667
}
lamexp_event_t;

//OS version number
typedef struct _lamexp_os_version_t
{
	unsigned int versionMajor;
	unsigned int versionMinor;
	bool overrideFlag;

	//comparision operators
	inline bool operator== (const _lamexp_os_version_t &rhs) const { return (versionMajor == rhs.versionMajor) && (versionMinor == rhs.versionMinor); }
	inline bool operator!= (const _lamexp_os_version_t &rhs) const { return (versionMajor != rhs.versionMajor) || (versionMinor != rhs.versionMinor); }
	inline bool operator>  (const _lamexp_os_version_t &rhs) const { return (versionMajor > rhs.versionMajor) || ((versionMajor == rhs.versionMajor) && (versionMinor >  rhs.versionMinor)); }
	inline bool operator>= (const _lamexp_os_version_t &rhs) const { return (versionMajor > rhs.versionMajor) || ((versionMajor == rhs.versionMajor) && (versionMinor >= rhs.versionMinor)); }
	inline bool operator<  (const _lamexp_os_version_t &rhs) const { return (versionMajor < rhs.versionMajor) || ((versionMajor == rhs.versionMajor) && (versionMinor <  rhs.versionMinor)); }
	inline bool operator<= (const _lamexp_os_version_t &rhs) const { return (versionMajor < rhs.versionMajor) || ((versionMajor == rhs.versionMajor) && (versionMinor <= rhs.versionMinor)); }
}
lamexp_os_version_t;

//Known Windows versions
extern const lamexp_os_version_t lamexp_winver_win2k;
extern const lamexp_os_version_t lamexp_winver_winxp;
extern const lamexp_os_version_t lamexp_winver_xpx64;
extern const lamexp_os_version_t lamexp_winver_vista;
extern const lamexp_os_version_t lamexp_winver_win70;
extern const lamexp_os_version_t lamexp_winver_win80;
extern const lamexp_os_version_t lamexp_winver_win81;

//Beep types
typedef enum
{
	lamexp_beep_info = 0,
	lamexp_beep_warning = 1,
	lamexp_beep_error = 2
}
lamexp_beep_t;

//Network connection types
typedef enum
{
	lamexp_network_err = 0,	/*unknown*/
	lamexp_network_non = 1,	/*not connected*/
	lamexp_network_yes = 2	/*connected*/
}
lamexp_network_t;

//System color types
typedef enum
{
	lamexp_syscolor_text = 0,
	lamexp_syscolor_background = 1,
	lamexp_syscolor_caption = 2
}
lamexp_syscolor_t;

//Icon type
class lamexp_icon_t;

///////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

const QIcon &lamexp_app_icon(void);
bool lamexp_append_sysmenu(const QWidget *win, const unsigned int identifier, const QString &text);
const QStringList &lamexp_arguments(void);
QStringList lamexp_available_codepages(bool noAliases = true);
bool lamexp_beep(int beepType);
void lamexp_blink_window(QWidget *poWindow, unsigned int count = 10, unsigned int delay = 150);
bool lamexp_bring_process_to_front(const unsigned long pid);
bool lamexp_bring_to_front(const QWidget *win);
bool lamexp_broadcast(int eventType, bool onlyToVisible);
bool lamexp_change_process_priority(const int priority);
bool lamexp_change_process_priority(void *hProcess, const int priority);
bool lamexp_change_process_priority(const QProcess *proc, const int priority);
bool lamexp_check_escape_state(void);
bool lamexp_check_sysmenu_msg(void *message, const unsigned int identifier);
bool lamexp_check_tool(const QString &toolName);
const QString lamexp_clean_filename(const QString &str);
const QString lamexp_clean_filepath(const QString &str);
bool lamexp_clean_folder(const QString &folderPath);
QDate lamexp_current_date_safe(void);
unsigned __int64 lamexp_current_file_time(void);
void lamexp_dbg_dbg_output_string(const char* format, ...);
unsigned long lamexp_dbg_private_bytes(void);
lamexp_cpu_t lamexp_detect_cpu_features(const QStringList &argv);
bool lamexp_detect_wine(void);
bool lamexp_enable_close_button(const QWidget *win, const bool bEnable = true);
bool lamexp_exec_shell(const QWidget *win, const QString &url, const bool explore = false);
bool lamexp_exec_shell(const QWidget *win, const QString &url, const QString &parameters, const QString &directory, const bool explore = false);
void lamexp_fatal_exit(const wchar_t* exitMessage, const wchar_t* errorBoxMessage = NULL);
void lamexp_finalization(void);
unsigned __int64 lamexp_free_diskspace(const QString &path, bool *ok = NULL);
void lamexp_free_window_icon(lamexp_icon_t *icon);
const lamexp_os_version_t &lamexp_get_os_version(void);
void lamexp_init_console(const QStringList &argv);
void lamexp_init_error_handlers(void);
int lamexp_init_ipc(void);
void lamexp_init_process(QProcess &process, const QString &wokringDir, const bool bReplaceTempDir = true);
bool lamexp_init_qt(int argc, char* argv[]);
bool lamexp_install_translator(const QString &language);
bool lamexp_install_translator_from_file(const QString &qmFile);
void lamexp_invalid_param_handler(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t);
void lamexp_ipc_read(unsigned int *command, char* message, size_t buffSize);
void lamexp_ipc_send(unsigned int command, const char* message);
bool lamexp_is_hibernation_supported(void);
const QString &lamexp_known_folder(lamexp_known_folder_t folder_id);
const QString lamexp_lookup_tool(const QString &toolName);
void lamexp_message_handler(QtMsgType type, const char *msg);
const char *lamexp_mulders_url(void);
void lamexp_natural_string_sort(QStringList &list, const bool bIgnoreCase);
int lamexp_network_status(void);
bool lamexp_open_media_file(const QString &mediaFilePath);
QString lamexp_path_to_short(const QString &longPath);
__int64 lamexp_perfcounter_frequ(void);
__int64 lamexp_perfcounter_value(void);
bool lamexp_play_sound(const unsigned short uiSoundIdx, const bool bAsync, const wchar_t *alias = NULL);
bool lamexp_play_sound_file(const QString &library, const unsigned short uiSoundIdx, const bool bAsync);
bool lamexp_portable_mode(void);
unsigned long lamexp_process_id(const QProcess *proc);
QStringList lamexp_query_translations(void);
unsigned int lamexp_rand(void);
QString lamexp_rand_str(const bool bLong = false);
void lamexp_register_tool(const QString &toolName, LockedFile *file, unsigned int version = 0, const QString *tag = NULL);
bool lamexp_remove_file(const QString &filename);
void lamexp_seed_rand(void);
lamexp_icon_t *lamexp_set_window_icon(QWidget *window, const QIcon &icon, const bool bIsBigIcon);
bool lamexp_sheet_of_glass(QWidget *window);
bool lamexp_sheet_of_glass_update(QWidget *window);
bool lamexp_shutdown_computer(const QString &message, const unsigned long timeout = 30, const bool forceShutdown = true, const bool hibernate = false);
void lamexp_sleep(const unsigned int delay);
QColor lamexp_system_color(const int color_id);
int lamexp_system_message(const wchar_t *text, int beepType);
const char *lamexp_support_url(void);
const QString &lamexp_temp_folder2(void);
bool lamexp_themes_enabled(void);
unsigned int lamexp_tool_version(const QString &toolName, QString *tag = NULL);
unsigned int lamexp_toolver_coreaudio(void);
unsigned int lamexp_toolver_fhgaacenc(void);
unsigned int lamexp_toolver_neroaac(void);
unsigned int lamexp_toolver_qaacenc(void);
unsigned int lamexp_translation_country(const QString &langId);
bool lamexp_translation_init(void);
QString lamexp_translation_name(const QString &language);
bool lamexp_translation_register(const QString &langId, const QString &qmFile, const QString &langName, unsigned int &systemId, unsigned int &country);
unsigned int lamexp_translation_sysid(const QString &langId);
bool lamexp_update_sysmenu(const QWidget *win, const unsigned int identifier, const QString &text);
const QString lamexp_version2string(const QString &pattern, unsigned int version, const QString &defaultText, const QString *tag = NULL);
const char *lamexp_version_arch(void);
unsigned int lamexp_version_build(void);
const char *lamexp_version_compiler(void);
unsigned int lamexp_version_confg(void);
const QDate &lamexp_version_date(void);
bool lamexp_version_demo(void);
QDate lamexp_version_expires(void);
unsigned int lamexp_version_major(void);
unsigned int lamexp_version_minor(void);
const char *lamexp_version_release(void);
const char *lamexp_version_time(void);
const char *lamexp_website_url(void);

///////////////////////////////////////////////////////////////////////////////
// HELPER MACROS
///////////////////////////////////////////////////////////////////////////////

#define LAMEXP_BOOL2STR(X) ((X) ? "1" : "0")
#define LAMEXP_COMPILER_WARNING(TXT) __pragma(message(__FILE__ "(" LAMEXP_MAKE_STRING(__LINE__) ") : warning: " TXT))
#define LAMEXP_CLOSE(HANDLE) do { if(HANDLE != NULL && HANDLE != INVALID_HANDLE_VALUE) { CloseHandle(HANDLE); HANDLE = NULL; } } while(0)
#define LAMEXP_DELETE(PTR) do { if(PTR) { delete PTR; PTR = NULL; } } while(0)
#define LAMEXP_DELETE_ARRAY(PTR) do { if(PTR) { delete [] PTR; PTR = NULL; } } while(0)
#define LAMEXP_MAKE_STRING_EX(X) #X
#define LAMEXP_MAKE_STRING(X) LAMEXP_MAKE_STRING_EX(X)
#define LAMEXP_SAFE_FREE(PTR) do { if(PTR) { free((void*) PTR); PTR = NULL; } } while(0)
#define LAMEXP_ZERO_MEMORY(X) memset(&(X), 0, sizeof((X)))
#define NOBR(STR) (QString("<nobr>%1</nobr>").arg((STR)).replace("-", "&minus;"))
#define QUTF8(STR) ((STR).toUtf8().constData())
#define QWCHAR(STR) (reinterpret_cast<const wchar_t*>((STR).utf16()))
#define WCHAR2QSTR(STR) (QString::fromUtf16(reinterpret_cast<const unsigned short*>((STR))))

//Check for debug build
#if defined(_DEBUG) && defined(QT_DEBUG) && !defined(NDEBUG) && !defined(QT_NO_DEBUG)
	#define LAMEXP_DEBUG (1)
#elif defined(NDEBUG) && defined(QT_NO_DEBUG) && !defined(_DEBUG) && !defined(QT_DEBUG)
	#define LAMEXP_DEBUG (0)
#else
	#error Inconsistent debug defines detected!
#endif

//Check for CPU-compatibility options
#if !defined(_M_X64) && defined(_MSC_VER) && defined(_M_IX86_FP)
	#if (_M_IX86_FP != 0)
		#error We should not enabled SSE or SSE2 in release builds!
	#endif
#endif

//Helper macro for throwing exceptions
#define THROW(MESSAGE) do \
{ \
	throw std::runtime_error((MESSAGE)); \
} \
while(0)
#define THROW_FMT(FORMAT, ...) do \
{ \
	char _error_msg[512]; \
	_snprintf_s(_error_msg, 512, _TRUNCATE, (FORMAT), __VA_ARGS__); \
	throw std::runtime_error(_error_msg); \
} \
while(0)

//Memory check
#if LAMEXP_DEBUG
	#define LAMEXP_MEMORY_CHECK(FUNC, RETV,  ...) do \
	{ \
		size_t _privateBytesBefore = lamexp_dbg_private_bytes(); \
		RETV = FUNC(__VA_ARGS__); \
		size_t _privateBytesLeak = (lamexp_dbg_private_bytes() - _privateBytesBefore) / 1024; \
		if(_privateBytesLeak > 0) { \
			lamexp_dbg_dbg_output_string("\nMemory leak: Lost %u KiloBytes of PrivateUsage memory!\n\n", _privateBytesLeak); \
		} \
	} \
	while(0)
#else
	#define LAMEXP_MEMORY_CHECK(FUNC, RETV,  ...) do \
	{ \
		RETV = __noop(__VA_ARGS__); \
	} \
	while(0)
#endif
