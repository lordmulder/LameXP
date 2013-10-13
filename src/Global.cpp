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

#include "Global.h"

//Windows includes
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <MMSystem.h>
#include <ShellAPI.h>
#include <WinInet.h>

//Qt includes
#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QUuid>
#include <QMap>
#include <QDate>
#include <QIcon>
#include <QPlastiqueStyle>
#include <QImageReader>
#include <QSharedMemory>
#include <QSysInfo>
#include <QStringList>
#include <QSystemSemaphore>
#include <QMutex>
#include <QTextCodec>
#include <QLibrary>
#include <QRegExp>
#include <QResource>
#include <QTranslator>
#include <QEventLoop>
#include <QTimer>
#include <QLibraryInfo>
#include <QEvent>
#include <QReadWriteLock>
#include <QReadLocker>
#include <QWriteLocker>
#include <QProcess>

//LameXP includes
#define LAMEXP_INC_CONFIG
#include "Resource.h"
#include "Config.h"
#include "LockedFile.h"
#include "strnatcmp.h"

//CRT includes
#include <iostream>
#include <fstream>
#include <io.h>
#include <fcntl.h>
#include <intrin.h>
#include <math.h>
#include <time.h>
#include <process.h>

//Shell API
#include <Shellapi.h>

//COM includes
#include <Objbase.h>
#include <PowrProf.h>

//Process API
#include <Psapi.h>

//Initialize static Qt plugins
#ifdef QT_NODLL
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
Q_IMPORT_PLUGIN(qico)
Q_IMPORT_PLUGIN(qsvg)
#else
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
Q_IMPORT_PLUGIN(QICOPlugin)
#endif
#endif

#define LAMEXP_ZERO_MEMORY(X) SecureZeroMemory(&X, sizeof(X))

//Helper macros
#define _LAMEXP_MAKE_STR(STR) #STR
#define LAMEXP_MAKE_STR(STR) _LAMEXP_MAKE_STR(STR)

///////////////////////////////////////////////////////////////////////////////
// TYPES
///////////////////////////////////////////////////////////////////////////////

static const size_t g_lamexp_ipc_slots = 128;

typedef struct
{
	unsigned int command;
	unsigned int reserved_1;
	unsigned int reserved_2;
	char parameter[4096];
}
lamexp_ipc_data_t;

typedef struct
{
	unsigned int pos_write;
	unsigned int pos_read;
	lamexp_ipc_data_t data[g_lamexp_ipc_slots];
}
lamexp_ipc_t;

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

//Build date
static QDate g_lamexp_version_date;
static const char *g_lamexp_months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static const char *g_lamexp_version_raw_date = __DATE__;
static const char *g_lamexp_version_raw_time = __TIME__;

//Console attached flag
static bool g_lamexp_console_attached = false;

//Official web-site URL
static const char *g_lamexp_website_url = "http://lamexp.sourceforge.net/";
static const char *g_lamexp_support_url = "http://forum.doom9.org/showthread.php?t=157726";
static const char *g_lamexp_mulders_url = "http://muldersoft.com/";

//Tool versions (expected versions!)
static const unsigned int g_lamexp_toolver_neroaac = VER_LAMEXP_TOOL_NEROAAC;
static const unsigned int g_lamexp_toolver_fhgaacenc = VER_LAMEXP_TOOL_FHGAACENC;
static const unsigned int g_lamexp_toolver_qaacenc = VER_LAMEXP_TOOL_QAAC;
static const unsigned int g_lamexp_toolver_coreaudio = VER_LAMEXP_TOOL_COREAUDIO;

//Special folders
static struct
{
	QMap<size_t, QString> *knownFolders;
	QReadWriteLock lock;
}
g_lamexp_known_folder;

//%TEMP% folder
static struct
{
	QString *path;
	QReadWriteLock lock;
}
g_lamexp_temp_folder;

//Tools
static struct
{
	QMap<QString, LockedFile*> *registry;
	QMap<QString, unsigned int> *versions;
	QMap<QString, QString> *tags;
	QReadWriteLock lock;
}
g_lamexp_tools;

//Languages
static struct
{
	QMap<QString, QString> *files;
	QMap<QString, QString> *names;
	QMap<QString, unsigned int> *sysid;
	QMap<QString, unsigned int> *cntry;
	QReadWriteLock lock;
}
g_lamexp_translation;

//Translator
static struct
{
	QTranslator *instance;
	QReadWriteLock lock;
}
g_lamexp_currentTranslator;

//CLI Arguments
static struct
{
	QStringList *list;
	QReadWriteLock lock;
}
g_lamexp_argv;

//OS Version
static struct
{
	bool bInitialized;
	lamexp_os_version_t version;
	QReadWriteLock lock;
}
g_lamexp_os_version;

//Portable Mode
static struct
{
	bool bInitialized;
	bool bPortableModeEnabled;
	QReadWriteLock lock;
}
g_lamexp_portable;

//Win32 Theme support
static struct
{
	bool bInitialized;
	bool bThemesEnabled;
	QReadWriteLock lock;
}
g_lamexp_themes_enabled;

//Shared memory
static const struct
{
	char *sharedmem;
	char *semaphore_read;
	char *semaphore_read_mutex;
	char *semaphore_write;
	char *semaphore_write_mutex;
}
g_lamexp_ipc_uuid =
{
	"{21A68A42-6923-43bb-9CF6-64BF151942EE}",
	"{7A605549-F58C-4d78-B4E5-06EFC34F405B}",
	"{60AA8D04-F6B8-497d-81EB-0F600F4A65B5}",
	"{726061D5-1615-4B82-871C-75FD93458E46}",
	"{1A616023-AA6A-4519-8AF3-F7736E899977}"
};
static struct
{
	QSharedMemory *sharedmem;
	QSystemSemaphore *semaphore_read;
	QSystemSemaphore *semaphore_read_mutex;
	QSystemSemaphore *semaphore_write;
	QSystemSemaphore *semaphore_write_mutex;
	QReadWriteLock lock;
}
g_lamexp_ipc_ptr;

//Image formats
static const char *g_lamexp_imageformats[] = {"bmp", "png", "jpg", "gif", "ico", "xpm", NULL}; //"svg"

//Global locks
static QMutex g_lamexp_message_mutex;

//Main thread ID
static const DWORD g_main_thread_id = GetCurrentThreadId();

//Log file
static FILE *g_lamexp_log_file = NULL;

//Localization
const char* LAMEXP_DEFAULT_LANGID = "en";
const char* LAMEXP_DEFAULT_TRANSLATION = "LameXP_EN.qm";

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
	#if (_MSC_VER == 1700)
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
		#elif (_MSC_FULL_VER == 170060610)
			static const char *g_lamexp_version_compiler = "MSVC 2012.3";
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
unsigned int lamexp_version_major(void) { return g_lamexp_version.ver_major; }
unsigned int lamexp_version_minor(void) { return g_lamexp_version.ver_minor; }
unsigned int lamexp_version_build(void) { return g_lamexp_version.ver_build; }
unsigned int lamexp_version_confg(void) { return g_lamexp_version.ver_confg; }
const char *lamexp_version_release(void) { return g_lamexp_version.ver_release_name; }
const char *lamexp_version_time(void) { return g_lamexp_version_raw_time; }
const char *lamexp_version_compiler(void) { return g_lamexp_version_compiler; }
const char *lamexp_version_arch(void) { return g_lamexp_version_arch; }
unsigned int lamexp_toolver_neroaac(void) { return g_lamexp_toolver_neroaac; }
unsigned int lamexp_toolver_fhgaacenc(void) { return g_lamexp_toolver_fhgaacenc; }
unsigned int lamexp_toolver_qaacenc(void) { return g_lamexp_toolver_qaacenc; }
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
			throw "Internal error: Date format could not be recognized!";
		}
	}

	return g_lamexp_version_date;
}

/*
 * Get the native operating system version
 */
const lamexp_os_version_t *lamexp_get_os_version(void)
{
	QReadLocker readLock(&g_lamexp_os_version.lock);

	//Already initialized?
	if(g_lamexp_os_version.bInitialized)
	{
		return &g_lamexp_os_version.version;
	}
	
	readLock.unlock();
	QWriteLocker writeLock(&g_lamexp_os_version.lock);

	//Detect OS version
	if(!g_lamexp_os_version.bInitialized)
	{
		OSVERSIONINFO osVerInfo;
		memset(&osVerInfo, 0, sizeof(OSVERSIONINFO));
		osVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	
		if(GetVersionEx(&osVerInfo) == TRUE)
		{
			if(osVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
			{
				g_lamexp_os_version.version.versionMajor = osVerInfo.dwMajorVersion;
				g_lamexp_os_version.version.versionMinor = osVerInfo.dwMinorVersion;
			}
			else
			{
				qWarning("lamexp_get_os_version: Not running under Windows NT, this is not supposed to happen!");
				g_lamexp_os_version.version.versionMajor = 0;
				g_lamexp_os_version.version.versionMinor = 0;
			}
		}
		else
		{
			throw "GetVersionEx() has failed. This is not supposed to happen!";
		}
		
		g_lamexp_os_version.bInitialized = true;
	}

	return  &g_lamexp_os_version.version;
}

/*
 * Check if we are running under wine
 */
bool lamexp_detect_wine(void)
{
	static bool isWine = false;
	static bool isWine_initialized = false;

	if(!isWine_initialized)
	{
		QLibrary ntdll("ntdll.dll");
		if(ntdll.load())
		{
			if(ntdll.resolve("wine_nt_to_unix_file_name") != NULL) isWine = true;
			if(ntdll.resolve("wine_get_version") != NULL) isWine = true;
			ntdll.unload();
		}
		isWine_initialized = true;
	}

	return isWine;
}

/*
 * Global exception handler
 */
LONG WINAPI lamexp_exception_handler(__in struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	lamexp_fatal_exit(L"Unhandeled exception handler invoked, application will exit!");
	return LONG_MAX;
}

/*
 * Invalid parameters handler
 */
void lamexp_invalid_param_handler(const wchar_t* exp, const wchar_t* fun, const wchar_t* fil, unsigned int, uintptr_t)
{
	lamexp_fatal_exit(L"Invalid parameter handler invoked, application will exit!");
}

/*
 * Change console text color
 */
static void lamexp_console_color(FILE* file, WORD attributes)
{
	const HANDLE hConsole = (HANDLE)(_get_osfhandle(_fileno(file)));
	if((hConsole != NULL) && (hConsole != INVALID_HANDLE_VALUE))
	{
		SetConsoleTextAttribute(hConsole, attributes);
	}
}

/*
 * Qt message handler
 */
void lamexp_message_handler(QtMsgType type, const char *msg)
{
	static const char *GURU_MEDITATION = "\n\nGURU MEDITATION !!!\n\n";

	if(msg == NULL) return;

	QMutexLocker lock(&g_lamexp_message_mutex);

	if(g_lamexp_log_file)
	{
		static char prefix[] = "DWCF";
		int index = qBound(0, static_cast<int>(type), 3);
		unsigned int timestamp = static_cast<unsigned int>(_time64(NULL) % 3600I64);
		QString str = QString::fromUtf8(msg).trimmed().replace('\n', '\t');
		fprintf(g_lamexp_log_file, "[%c][%04u] %s\r\n", prefix[index], timestamp, str.toUtf8().constData());
		fflush(g_lamexp_log_file);
	}

	if(g_lamexp_console_attached)
	{
		UINT oldOutputCP = GetConsoleOutputCP();
		if(oldOutputCP != CP_UTF8) SetConsoleOutputCP(CP_UTF8);

		switch(type)
		{
		case QtCriticalMsg:
		case QtFatalMsg:
			fflush(stdout);
			fflush(stderr);
			lamexp_console_color(stderr, FOREGROUND_RED | FOREGROUND_INTENSITY);
			fprintf(stderr, GURU_MEDITATION);
			fprintf(stderr, "%s\n", msg);
			fflush(stderr);
			break;
		case QtWarningMsg:
			lamexp_console_color(stderr, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
			fprintf(stderr, "%s\n", msg);
			fflush(stderr);
			break;
		default:
			lamexp_console_color(stderr, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
			fprintf(stderr, "%s\n", msg);
			fflush(stderr);
			break;
		}
	
		lamexp_console_color(stderr, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
		if(oldOutputCP != CP_UTF8) SetConsoleOutputCP(oldOutputCP);
	}
	else
	{
		QString temp("[LameXP][%1] %2");
		
		switch(type)
		{
		case QtCriticalMsg:
		case QtFatalMsg:
			temp = temp.arg("C", QString::fromUtf8(msg));
			break;
		case QtWarningMsg:
			temp = temp.arg("W", QString::fromUtf8(msg));
			break;
		default:
			temp = temp.arg("I", QString::fromUtf8(msg));
			break;
		}

		temp.replace("\n", "\t").append("\n");
		OutputDebugStringA(temp.toLatin1().constData());
	}

	if((type == QtCriticalMsg) || (type == QtFatalMsg))
	{
		lock.unlock();
		lamexp_fatal_exit(L"The application has encountered a critical error and will exit now!", QWCHAR(QString::fromUtf8(msg)));
	}
}

/*
 * Initialize the console
 */
void lamexp_init_console(const QStringList &argv)
{
	bool enableConsole = (LAMEXP_DEBUG) || ((VER_LAMEXP_CONSOLE_ENABLED) && lamexp_version_demo());

	if(_environ)
	{
		wchar_t *logfile = NULL;
		size_t logfile_len = 0;
		if(!_wdupenv_s(&logfile, &logfile_len, L"LAMEXP_LOGFILE"))
		{
			if(logfile && (logfile_len > 0))
			{
				FILE *temp = NULL;
				if(!_wfopen_s(&temp, logfile, L"wb"))
				{
					fprintf(temp, "%c%c%c", 0xEF, 0xBB, 0xBF);
					g_lamexp_log_file = temp;
				}
				free(logfile);
			}
		}
	}

	if(!LAMEXP_DEBUG)
	{
		for(int i = 0; i < argv.count(); i++)
		{
			if(!argv.at(i).compare("--console", Qt::CaseInsensitive))
			{
				enableConsole = true;
			}
			else if(!argv.at(i).compare("--no-console", Qt::CaseInsensitive))
			{
				enableConsole = false;
			}
		}
	}

	if(enableConsole)
	{
		if(!g_lamexp_console_attached)
		{
			if(AllocConsole() != FALSE)
			{
				SetConsoleCtrlHandler(NULL, TRUE);
				SetConsoleTitle(L"LameXP - Audio Encoder Front-End | Debug Console");
				SetConsoleOutputCP(CP_UTF8);
				g_lamexp_console_attached = true;
			}
		}
		
		if(g_lamexp_console_attached)
		{
			//-------------------------------------------------------------------
			//See: http://support.microsoft.com/default.aspx?scid=kb;en-us;105305
			//-------------------------------------------------------------------
			const int flags = _O_WRONLY | _O_U8TEXT;
			int hCrtStdOut = _open_osfhandle((intptr_t) GetStdHandle(STD_OUTPUT_HANDLE), flags);
			int hCrtStdErr = _open_osfhandle((intptr_t) GetStdHandle(STD_ERROR_HANDLE),  flags);
			FILE *hfStdOut = (hCrtStdOut >= 0) ? _fdopen(hCrtStdOut, "wb") : NULL;
			FILE *hfStdErr = (hCrtStdErr >= 0) ? _fdopen(hCrtStdErr, "wb") : NULL;
			if(hfStdOut) { *stdout = *hfStdOut; std::cout.rdbuf(new std::filebuf(hfStdOut)); }
			if(hfStdErr) { *stderr = *hfStdErr; std::cerr.rdbuf(new std::filebuf(hfStdErr)); }
		}

		HWND hwndConsole = GetConsoleWindow();

		if((hwndConsole != NULL) && (hwndConsole != INVALID_HANDLE_VALUE))
		{
			HMENU hMenu = GetSystemMenu(hwndConsole, 0);
			EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
			RemoveMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);

			SetWindowPos(hwndConsole, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
			SetWindowLong(hwndConsole, GWL_STYLE, GetWindowLong(hwndConsole, GWL_STYLE) & (~WS_MAXIMIZEBOX) & (~WS_MINIMIZEBOX));
			SetWindowPos(hwndConsole, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
		}
	}
}

/*
 * Detect CPU features
 */
lamexp_cpu_t lamexp_detect_cpu_features(const QStringList &argv)
{
	typedef BOOL (WINAPI *IsWow64ProcessFun)(__in HANDLE hProcess, __out PBOOL Wow64Process);
	typedef VOID (WINAPI *GetNativeSystemInfoFun)(__out LPSYSTEM_INFO lpSystemInfo);
	
	static IsWow64ProcessFun IsWow64ProcessPtr = NULL;
	static GetNativeSystemInfoFun GetNativeSystemInfoPtr = NULL;

	lamexp_cpu_t features;
	SYSTEM_INFO systemInfo;
	int CPUInfo[4] = {-1};
	char CPUIdentificationString[0x40];
	char CPUBrandString[0x40];

	memset(&features, 0, sizeof(lamexp_cpu_t));
	memset(&systemInfo, 0, sizeof(SYSTEM_INFO));
	memset(CPUIdentificationString, 0, sizeof(CPUIdentificationString));
	memset(CPUBrandString, 0, sizeof(CPUBrandString));
	
	__cpuid(CPUInfo, 0);
	memcpy(CPUIdentificationString, &CPUInfo[1], sizeof(int));
	memcpy(CPUIdentificationString + 4, &CPUInfo[3], sizeof(int));
	memcpy(CPUIdentificationString + 8, &CPUInfo[2], sizeof(int));
	features.intel = (_stricmp(CPUIdentificationString, "GenuineIntel") == 0);
	strncpy_s(features.vendor, 0x40, CPUIdentificationString, _TRUNCATE);

	if(CPUInfo[0] >= 1)
	{
		__cpuid(CPUInfo, 1);
		features.mmx = (CPUInfo[3] & 0x800000) || false;
		features.sse = (CPUInfo[3] & 0x2000000) || false;
		features.sse2 = (CPUInfo[3] & 0x4000000) || false;
		features.ssse3 = (CPUInfo[2] & 0x200) || false;
		features.sse3 = (CPUInfo[2] & 0x1) || false;
		features.ssse3 = (CPUInfo[2] & 0x200) || false;
		features.stepping = CPUInfo[0] & 0xf;
		features.model = ((CPUInfo[0] >> 4) & 0xf) + (((CPUInfo[0] >> 16) & 0xf) << 4);
		features.family = ((CPUInfo[0] >> 8) & 0xf) + ((CPUInfo[0] >> 20) & 0xff);
	}

	__cpuid(CPUInfo, 0x80000000);
	int nExIds = qMax<int>(qMin<int>(CPUInfo[0], 0x80000004), 0x80000000);

	for(int i = 0x80000002; i <= nExIds; ++i)
	{
		__cpuid(CPUInfo, i);
		switch(i)
		{
		case 0x80000002:
			memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
			break;
		case 0x80000003:
			memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
			break;
		case 0x80000004:
			memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
			break;
		}
	}

	strncpy_s(features.brand, 0x40, CPUBrandString, _TRUNCATE);

	if(strlen(features.brand) < 1) strncpy_s(features.brand, 0x40, "Unknown", _TRUNCATE);
	if(strlen(features.vendor) < 1) strncpy_s(features.vendor, 0x40, "Unknown", _TRUNCATE);

#if !defined(_M_X64 ) && !defined(_M_IA64)
	if(!IsWow64ProcessPtr || !GetNativeSystemInfoPtr)
	{
		QLibrary Kernel32Lib("kernel32.dll");
		IsWow64ProcessPtr = (IsWow64ProcessFun) Kernel32Lib.resolve("IsWow64Process");
		GetNativeSystemInfoPtr = (GetNativeSystemInfoFun) Kernel32Lib.resolve("GetNativeSystemInfo");
	}
	if(IsWow64ProcessPtr)
	{
		BOOL x64 = FALSE;
		if(IsWow64ProcessPtr(GetCurrentProcess(), &x64))
		{
			features.x64 = x64;
		}
	}
	if(GetNativeSystemInfoPtr)
	{
		GetNativeSystemInfoPtr(&systemInfo);
	}
	else
	{
		GetSystemInfo(&systemInfo);
	}
	features.count = qBound(1UL, systemInfo.dwNumberOfProcessors, 64UL);
#else
	GetNativeSystemInfo(&systemInfo);
	features.count = systemInfo.dwNumberOfProcessors;
	features.x64 = true;
#endif

	if(argv.count() > 0)
	{
		bool flag = false;
		for(int i = 0; i < argv.count(); i++)
		{
			if(!argv[i].compare("--force-cpu-no-64bit", Qt::CaseInsensitive)) { flag = true; features.x64 = false; }
			if(!argv[i].compare("--force-cpu-no-sse", Qt::CaseInsensitive)) { flag = true; features.sse = features.sse2 = features.sse3 = features.ssse3 = false; }
			if(!argv[i].compare("--force-cpu-no-intel", Qt::CaseInsensitive)) { flag = true; features.intel = false; }
		}
		if(flag) qWarning("CPU flags overwritten by user-defined parameters. Take care!\n");
	}

	return features;
}

/*
 * Check for debugger (detect routine)
 */
static __forceinline bool lamexp_check_for_debugger(void)
{
	if(IsDebuggerPresent())
	{
		return true;
	}
	
	__try
	{
		CloseHandle((HANDLE) 0x7FFFFFFF);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		return true;
	}

	__try 
	{
		DebugBreak();
	}
	__except(EXCEPTION_EXECUTE_HANDLER) 
	{
		return false;
	}
	
	return true;
}

/*
 * Check for debugger (thread proc)
 */
static unsigned int __stdcall lamexp_debug_thread_proc(LPVOID lpParameter)
{
	while(!lamexp_check_for_debugger())
	{
		Sleep(250);
	}
	lamexp_fatal_exit(L"Not a debug build. Please unload debugger and try again!");
	return 666;
}

/*
 * Check for debugger (startup routine)
 */
static HANDLE lamexp_debug_thread_init(void)
{
	if(lamexp_check_for_debugger())
	{
		lamexp_fatal_exit(L"Not a debug build. Please unload debugger and try again!");
	}

	return (HANDLE) _beginthreadex(NULL, 0, lamexp_debug_thread_proc, NULL, 0, NULL);
}

/*
 * Check for compatibility mode
 */
static bool lamexp_check_compatibility_mode(const char *exportName, const QString &executableName)
{
	QLibrary kernel32("kernel32.dll");

	if((exportName != NULL) && kernel32.load())
	{
		if(kernel32.resolve(exportName) != NULL)
		{
			qWarning("Function '%s' exported from 'kernel32.dll' -> Windows compatibility mode!", exportName);
			qFatal("%s", QApplication::tr("Executable '%1' doesn't support Windows compatibility mode.").arg(executableName).toLatin1().constData());
			return false;
		}
	}

	return true;
}

/*
 * Computus according to H. Lichtenberg
 */
static bool lamexp_computus(const QDate &date)
{
	int X = date.year();
	int A = X % 19;
	int K = X / 100;
	int M = 15 + (3*K + 3) / 4 - (8*K + 13) / 25;
	int D = (19*A + M) % 30;
	int S = 2 - (3*K + 3) / 4;
	int R = D / 29 + (D / 28 - D / 29) * (A / 11);
	int OG = 21 + D - R;
	int SZ = 7 - (X + X / 4 + S) % 7;
	int OE = 7 - (OG - SZ) % 7;
	int OS = (OG + OE);

	if(OS > 31)
	{
		return (date.month() == 4) && (date.day() == (OS - 31));
	}
	else
	{
		return (date.month() == 3) && (date.day() == OS);
	}
}

/*
 * Check for Thanksgiving
 */
static bool lamexp_thanksgiving(const QDate &date)
{
	int day = 0;

	switch(QDate(date.year(), 11, 1).dayOfWeek())
	{
		case 1: day = 25; break; 
		case 2: day = 24; break; 
		case 3: day = 23; break; 
		case 4: day = 22; break; 
		case 5: day = 28; break; 
		case 6: day = 27; break; 
		case 7: day = 26; break;
	}

	return (date.month() == 11) && (date.day() == day);
}

/*
 * Initialize app icon
 */
QIcon lamexp_app_icon(const QDate *date, const QTime *time)
{
	QDate currentDate = (date) ? QDate(*date) : QDate::currentDate();
	QTime currentTime = (time) ? QTime(*time) : QTime::currentTime();
	
	if(lamexp_thanksgiving(currentDate))
	{
		return QIcon(":/MainIcon6.png");
	}
	else if(((currentDate.month() == 12) && (currentDate.day() == 31) && (currentTime.hour() >= 20)) || ((currentDate.month() == 1) && (currentDate.day() == 1)  && (currentTime.hour() <= 19)))
	{
		return QIcon(":/MainIcon5.png");
	}
	else if(((currentDate.month() == 10) && (currentDate.day() == 31) && (currentTime.hour() >= 12)) || ((currentDate.month() == 11) && (currentDate.day() == 1)  && (currentTime.hour() <= 11)))
	{
		return QIcon(":/MainIcon4.png");
	}
	else if((currentDate.month() == 12) && (currentDate.day() >= 24) && (currentDate.day() <= 26))
	{
		return QIcon(":/MainIcon3.png");
	}
	else if(lamexp_computus(currentDate))
	{
		return QIcon(":/MainIcon2.png");
	}
	else
	{
		return QIcon(":/MainIcon1.png");
	}
}

/*
 * Broadcast event to all windows
 */
static bool lamexp_broadcast(int eventType, bool onlyToVisible)
{
	if(QApplication *app = dynamic_cast<QApplication*>(QApplication::instance()))
	{
		qDebug("Broadcasting %d", eventType);
		
		bool allOk = true;
		QEvent poEvent(static_cast<QEvent::Type>(eventType));
		QWidgetList list = app->topLevelWidgets();

		while(!list.isEmpty())
		{
			QWidget *widget = list.takeFirst();
			if(!onlyToVisible || widget->isVisible())
			{
				if(!app->sendEvent(widget, &poEvent))
				{
					allOk = false;
				}
			}
		}

		qDebug("Broadcast %d done (%s)", eventType, (allOk ? "OK" : "Stopped"));
		return allOk;
	}
	else
	{
		qWarning("Broadcast failed, could not get QApplication instance!");
		return false;
	}
}

/*
 * Qt event filter
 */
static bool lamexp_event_filter(void *message, long *result)
{
	if((!(LAMEXP_DEBUG)) && lamexp_check_for_debugger())
	{
		lamexp_fatal_exit(L"Not a debug build. Please unload debugger and try again!");
	}
	
	switch(reinterpret_cast<MSG*>(message)->message)
	{
	case WM_QUERYENDSESSION:
		qWarning("WM_QUERYENDSESSION message received!");
		*result = lamexp_broadcast(lamexp_event_queryendsession, false) ? TRUE : FALSE;
		return true;
	case WM_ENDSESSION:
		qWarning("WM_ENDSESSION message received!");
		if(reinterpret_cast<MSG*>(message)->wParam == TRUE)
		{
			lamexp_broadcast(lamexp_event_endsession, false);
			if(QApplication *app = reinterpret_cast<QApplication*>(QApplication::instance()))
			{
				app->closeAllWindows();
				app->quit();
			}
			lamexp_finalization();
			exit(1);
		}
		*result = 0;
		return true;
	default:
		/*ignore this message and let Qt handle it*/
		return false;
	}
}

/*
 * Check for process elevation
 */
static bool lamexp_check_elevation(void)
{
	typedef enum { lamexp_token_elevationType_class = 18, lamexp_token_elevation_class = 20 } LAMEXP_TOKEN_INFORMATION_CLASS;
	typedef enum { lamexp_elevationType_default = 1, lamexp_elevationType_full, lamexp_elevationType_limited } LAMEXP_TOKEN_ELEVATION_TYPE;

	HANDLE hToken = NULL;
	bool bIsProcessElevated = false;
	
	if(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
	{
		LAMEXP_TOKEN_ELEVATION_TYPE tokenElevationType;
		DWORD returnLength;
		if(GetTokenInformation(hToken, (TOKEN_INFORMATION_CLASS) lamexp_token_elevationType_class, &tokenElevationType, sizeof(LAMEXP_TOKEN_ELEVATION_TYPE), &returnLength))
		{
			if(returnLength == sizeof(LAMEXP_TOKEN_ELEVATION_TYPE))
			{
				switch(tokenElevationType)
				{
				case lamexp_elevationType_default:
					qDebug("Process token elevation type: Default -> UAC is disabled.\n");
					break;
				case lamexp_elevationType_full:
					qWarning("Process token elevation type: Full -> potential security risk!\n");
					bIsProcessElevated = true;
					break;
				case lamexp_elevationType_limited:
					qDebug("Process token elevation type: Limited -> not elevated.\n");
					break;
				}
			}
		}
		CloseHandle(hToken);
	}
	else
	{
		qWarning("Failed to open process token!");
	}

	return !bIsProcessElevated;
}

/*
 * Initialize Qt framework
 */
bool lamexp_init_qt(int argc, char* argv[])
{
	static bool qt_initialized = false;
	typedef BOOL (WINAPI *SetDllDirectoryProc)(WCHAR *lpPathName);
	const QStringList &arguments = lamexp_arguments();

	//Don't initialized again, if done already
	if(qt_initialized)
	{
		return true;
	}

	//Secure DLL loading
	QLibrary kernel32("kernel32.dll");
	if(kernel32.load())
	{
		SetDllDirectoryProc pSetDllDirectory = (SetDllDirectoryProc) kernel32.resolve("SetDllDirectoryW");
		if(pSetDllDirectory != NULL) pSetDllDirectory(L"");
		kernel32.unload();
	}

	//Extract executable name from argv[] array
	QString executableName = QLatin1String("LameXP.exe");
	if(arguments.count() > 0)
	{
		static const char *delimiters = "\\/:?";
		executableName = arguments[0].trimmed();
		for(int i = 0; delimiters[i]; i++)
		{
			int temp = executableName.lastIndexOf(QChar(delimiters[i]));
			if(temp >= 0) executableName = executableName.mid(temp + 1);
		}
		executableName = executableName.trimmed();
		if(executableName.isEmpty())
		{
			executableName = QLatin1String("LameXP.exe");
		}
	}

	//Check Qt version
#ifdef QT_BUILD_KEY
	qDebug("Using Qt v%s [%s], %s, %s", qVersion(), QLibraryInfo::buildDate().toString(Qt::ISODate).toLatin1().constData(), (qSharedBuild() ? "DLL" : "Static"), QLibraryInfo::buildKey().toLatin1().constData());
	qDebug("Compiled with Qt v%s [%s], %s\n", QT_VERSION_STR, QT_PACKAGEDATE_STR, QT_BUILD_KEY);
	if(_stricmp(qVersion(), QT_VERSION_STR))
	{
		qFatal("%s", QApplication::tr("Executable '%1' requires Qt v%2, but found Qt v%3.").arg(executableName, QString::fromLatin1(QT_VERSION_STR), QString::fromLatin1(qVersion())).toLatin1().constData());
		return false;
	}
	if(QLibraryInfo::buildKey().compare(QString::fromLatin1(QT_BUILD_KEY), Qt::CaseInsensitive))
	{
		qFatal("%s", QApplication::tr("Executable '%1' was built for Qt '%2', but found Qt '%3'.").arg(executableName, QString::fromLatin1(QT_BUILD_KEY), QLibraryInfo::buildKey()).toLatin1().constData());
		return false;
	}
#else
	qDebug("Using Qt v%s [%s], %s", qVersion(), QLibraryInfo::buildDate().toString(Qt::ISODate).toLatin1().constData(), (qSharedBuild() ? "DLL" : "Static"));
	qDebug("Compiled with Qt v%s [%s]\n", QT_VERSION_STR, QT_PACKAGEDATE_STR);
#endif

	//Check the Windows version
	switch(QSysInfo::windowsVersion() & QSysInfo::WV_NT_based)
	{
	case 0:
	case QSysInfo::WV_NT:
		qFatal("%s", QApplication::tr("Executable '%1' requires Windows 2000 or later.").arg(executableName).toLatin1().constData());
		break;
	case QSysInfo::WV_2000:
		qDebug("Running on Windows 2000 (not officially supported!).\n");
		lamexp_check_compatibility_mode("GetNativeSystemInfo", executableName);
		break;
	case QSysInfo::WV_XP:
		qDebug("Running on Windows XP.\n");
		lamexp_check_compatibility_mode("GetLargePageMinimum", executableName);
		break;
	case QSysInfo::WV_2003:
		qDebug("Running on Windows Server 2003 or Windows XP x64-Edition.\n");
		lamexp_check_compatibility_mode("GetLocaleInfoEx", executableName);
		break;
	case QSysInfo::WV_VISTA:
		qDebug("Running on Windows Vista or Windows Server 2008.\n");
		lamexp_check_compatibility_mode("CreateRemoteThreadEx", executableName);
		break;
	case QSysInfo::WV_WINDOWS7:
		qDebug("Running on Windows 7 or Windows Server 2008 R2.\n");
		lamexp_check_compatibility_mode("CreateFile2", executableName);
		break;
	default:
		{
			const lamexp_os_version_t *osVersionNo = lamexp_get_os_version();
			if(osVersionNo->versionMajor < 5)
			{
				qFatal("%s", QApplication::tr("Executable '%1' requires Windows 2000 or later.").arg(executableName).toLatin1().constData());
			}
			else if(LAMEXP_EQL_OS_VER(osVersionNo, 6, 2))
			{
				qDebug("Running on Windows 8 or Windows Server 2012\n");
				lamexp_check_compatibility_mode(NULL, executableName);
			}
			else
			{
				qWarning("Running on an unknown/untested WinNT-based OS (v%u.%u).\n", osVersionNo->versionMajor, osVersionNo->versionMinor);
			}
		}
		break;
	}

	//Check for Wine
	if(lamexp_detect_wine())
	{
		qWarning("It appears we are running under Wine, unexpected things might happen!\n");
	}

	//Set text Codec for locale
	QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

	//Create Qt application instance
	QApplication *application = new QApplication(argc, argv);

	//Load plugins from application directory
	QCoreApplication::setLibraryPaths(QStringList() << QApplication::applicationDirPath());
	qDebug("Library Path:\n%s\n", QApplication::libraryPaths().first().toUtf8().constData());

	//Set application properties
	application->setApplicationName("LameXP - Audio Encoder Front-End");
	application->setApplicationVersion(QString().sprintf("%d.%02d.%04d", lamexp_version_major(), lamexp_version_minor(), lamexp_version_build())); 
	application->setOrganizationName("LoRd_MuldeR");
	application->setOrganizationDomain("mulder.at.gg");
	application->setWindowIcon(lamexp_app_icon());
	application->setEventFilter(lamexp_event_filter);

	//Check for supported image formats
	QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
	for(int i = 0; g_lamexp_imageformats[i]; i++)
	{
		if(!supportedFormats.contains(g_lamexp_imageformats[i]))
		{
			qFatal("Qt initialization error: QImageIOHandler for '%s' missing!", g_lamexp_imageformats[i]);
			return false;
		}
	}
	
	//Add default translations
	QWriteLocker writeLockTranslations(&g_lamexp_translation.lock);
	if(!g_lamexp_translation.files) g_lamexp_translation.files = new QMap<QString, QString>();
	if(!g_lamexp_translation.names) g_lamexp_translation.names = new QMap<QString, QString>();
	g_lamexp_translation.files->insert(LAMEXP_DEFAULT_LANGID, "");
	g_lamexp_translation.names->insert(LAMEXP_DEFAULT_LANGID, "English");
	writeLockTranslations.unlock();

	//Check for process elevation
	if((!lamexp_check_elevation()) && (!lamexp_detect_wine()))
	{
		QMessageBox messageBox(QMessageBox::Warning, "LameXP", "<nobr>LameXP was started with 'elevated' rights, altough LameXP does not need these rights.<br>Running an applications with unnecessary rights is a potential security risk!</nobr>", QMessageBox::NoButton, NULL, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
		messageBox.addButton("Quit Program (Recommended)", QMessageBox::NoRole);
		messageBox.addButton("Ignore", QMessageBox::NoRole);
		if(messageBox.exec() == 0)
		{
			return false;
		}
	}

	//Update console icon, if a console is attached
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	if(g_lamexp_console_attached && (!lamexp_detect_wine()))
	{
		typedef DWORD (__stdcall *SetConsoleIconFun)(HICON);
		QLibrary kernel32("kernel32.dll");
		if(kernel32.load())
		{
			SetConsoleIconFun SetConsoleIconPtr = (SetConsoleIconFun) kernel32.resolve("SetConsoleIcon");
			if(SetConsoleIconPtr != NULL) SetConsoleIconPtr(QIcon(":/icons/sound.png").pixmap(16, 16).toWinHICON());
			kernel32.unload();
		}
	}
#endif

	//Done
	qt_initialized = true;
	return true;
}

/*
 * Initialize IPC
 */
int lamexp_init_ipc(void)
{
	QWriteLocker writeLock(&g_lamexp_ipc_ptr.lock);
	
	if(g_lamexp_ipc_ptr.sharedmem && g_lamexp_ipc_ptr.semaphore_read && g_lamexp_ipc_ptr.semaphore_write && g_lamexp_ipc_ptr.semaphore_read_mutex && g_lamexp_ipc_ptr.semaphore_write_mutex)
	{
		return 0;
	}

	g_lamexp_ipc_ptr.semaphore_read = new QSystemSemaphore(QString(g_lamexp_ipc_uuid.semaphore_read), 0);
	g_lamexp_ipc_ptr.semaphore_write = new QSystemSemaphore(QString(g_lamexp_ipc_uuid.semaphore_write), 0);
	g_lamexp_ipc_ptr.semaphore_read_mutex = new QSystemSemaphore(QString(g_lamexp_ipc_uuid.semaphore_read_mutex), 0);
	g_lamexp_ipc_ptr.semaphore_write_mutex = new QSystemSemaphore(QString(g_lamexp_ipc_uuid.semaphore_write_mutex), 0);

	if(g_lamexp_ipc_ptr.semaphore_read->error() != QSystemSemaphore::NoError)
	{
		QString errorMessage = g_lamexp_ipc_ptr.semaphore_read->errorString();
		LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_read);
		LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_write);
		LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_read_mutex);
		LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_write_mutex);
		qFatal("Failed to create system smaphore: %s", errorMessage.toUtf8().constData());
		return -1;
	}
	if(g_lamexp_ipc_ptr.semaphore_write->error() != QSystemSemaphore::NoError)
	{
		QString errorMessage = g_lamexp_ipc_ptr.semaphore_write->errorString();
		LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_read);
		LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_write);
		LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_read_mutex);
		LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_write_mutex);
		qFatal("Failed to create system smaphore: %s", errorMessage.toUtf8().constData());
		return -1;
	}
	if(g_lamexp_ipc_ptr.semaphore_read_mutex->error() != QSystemSemaphore::NoError)
	{
		QString errorMessage = g_lamexp_ipc_ptr.semaphore_read_mutex->errorString();
		LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_read);
		LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_write);
		LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_read_mutex);
		LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_write_mutex);
		qFatal("Failed to create system smaphore: %s", errorMessage.toUtf8().constData());
		return -1;
	}
	if(g_lamexp_ipc_ptr.semaphore_write_mutex->error() != QSystemSemaphore::NoError)
	{
		QString errorMessage = g_lamexp_ipc_ptr.semaphore_write_mutex->errorString();
		LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_read);
		LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_write);
		LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_read_mutex);
		LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_write_mutex);
		qFatal("Failed to create system smaphore: %s", errorMessage.toUtf8().constData());
		return -1;
	}

	g_lamexp_ipc_ptr.sharedmem = new QSharedMemory(QString(g_lamexp_ipc_uuid.sharedmem), NULL);
	
	if(!g_lamexp_ipc_ptr.sharedmem->create(sizeof(lamexp_ipc_t)))
	{
		if(g_lamexp_ipc_ptr.sharedmem->error() == QSharedMemory::AlreadyExists)
		{
			g_lamexp_ipc_ptr.sharedmem->attach();
			if(g_lamexp_ipc_ptr.sharedmem->error() == QSharedMemory::NoError)
			{
				return 1;
			}
			else
			{
				QString errorMessage = g_lamexp_ipc_ptr.sharedmem->errorString();
				LAMEXP_DELETE(g_lamexp_ipc_ptr.sharedmem);
				qFatal("Failed to attach to shared memory: %s", errorMessage.toUtf8().constData());
				return -1;
			}
		}
		else
		{
			QString errorMessage = g_lamexp_ipc_ptr.sharedmem->errorString();
			LAMEXP_DELETE(g_lamexp_ipc_ptr.sharedmem);
			qFatal("Failed to create shared memory: %s", errorMessage.toUtf8().constData());
			return -1;
		}
	}

	memset(g_lamexp_ipc_ptr.sharedmem->data(), 0, sizeof(lamexp_ipc_t));
	g_lamexp_ipc_ptr.semaphore_write->release(g_lamexp_ipc_slots);
	g_lamexp_ipc_ptr.semaphore_read_mutex->release();
	g_lamexp_ipc_ptr.semaphore_write_mutex->release();

	return 0;
}

/*
 * IPC send message
 */
void lamexp_ipc_send(unsigned int command, const char* message)
{
	QReadLocker readLock(&g_lamexp_ipc_ptr.lock);

	if(!g_lamexp_ipc_ptr.sharedmem || !g_lamexp_ipc_ptr.semaphore_read || !g_lamexp_ipc_ptr.semaphore_write || !g_lamexp_ipc_ptr.semaphore_read_mutex || !g_lamexp_ipc_ptr.semaphore_write_mutex)
	{
		throw "Shared memory for IPC not initialized yet.";
	}

	lamexp_ipc_data_t ipc_data;
	memset(&ipc_data, 0, sizeof(lamexp_ipc_data_t));
	ipc_data.command = command;
	
	if(message)
	{
		strncpy_s(ipc_data.parameter, 4096, message, _TRUNCATE);
	}

	if(g_lamexp_ipc_ptr.semaphore_write->acquire())
	{
		if(g_lamexp_ipc_ptr.semaphore_write_mutex->acquire())
		{
			lamexp_ipc_t *ptr = reinterpret_cast<lamexp_ipc_t*>(g_lamexp_ipc_ptr.sharedmem->data());
			memcpy(&ptr->data[ptr->pos_write], &ipc_data, sizeof(lamexp_ipc_data_t));
			ptr->pos_write = (ptr->pos_write + 1) % g_lamexp_ipc_slots;
			g_lamexp_ipc_ptr.semaphore_read->release();
			g_lamexp_ipc_ptr.semaphore_write_mutex->release();
		}
	}
}

/*
 * IPC read message
 */
void lamexp_ipc_read(unsigned int *command, char* message, size_t buffSize)
{
	QReadLocker readLock(&g_lamexp_ipc_ptr.lock);
	
	*command = 0;
	message[0] = '\0';
	
	if(!g_lamexp_ipc_ptr.sharedmem || !g_lamexp_ipc_ptr.semaphore_read || !g_lamexp_ipc_ptr.semaphore_write || !g_lamexp_ipc_ptr.semaphore_read_mutex || !g_lamexp_ipc_ptr.semaphore_write_mutex)
	{
		throw "Shared memory for IPC not initialized yet.";
	}

	lamexp_ipc_data_t ipc_data;
	memset(&ipc_data, 0, sizeof(lamexp_ipc_data_t));

	if(g_lamexp_ipc_ptr.semaphore_read->acquire())
	{
		if(g_lamexp_ipc_ptr.semaphore_read_mutex->acquire())
		{
			lamexp_ipc_t *ptr = reinterpret_cast<lamexp_ipc_t*>(g_lamexp_ipc_ptr.sharedmem->data());
			memcpy(&ipc_data, &ptr->data[ptr->pos_read], sizeof(lamexp_ipc_data_t));
			ptr->pos_read = (ptr->pos_read + 1) % g_lamexp_ipc_slots;
			g_lamexp_ipc_ptr.semaphore_write->release();
			g_lamexp_ipc_ptr.semaphore_read_mutex->release();

			if(!(ipc_data.reserved_1 || ipc_data.reserved_2))
			{
				*command = ipc_data.command;
				strncpy_s(message, buffSize, ipc_data.parameter, _TRUNCATE);
			}
			else
			{
				qWarning("Malformed IPC message, will be ignored");
			}
		}
	}
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
			g_lamexp_portable.bPortableModeEnabled = (idx1 >= 0) && (idx2 >= 0) && (idx1 < idx2);
		}
		g_lamexp_portable.bInitialized = true;
	}
	
	return g_lamexp_portable.bPortableModeEnabled;
}

/*
 * Get a random string
 */
QString lamexp_rand_str(const bool bLong)
{
	const QUuid uuid = QUuid::createUuid().toString();

	const unsigned int u1 = uuid.data1;
	const unsigned int u2 = (((unsigned int)(uuid.data2)) << 16) | ((unsigned int)(uuid.data3));
	const unsigned int u3 = (((unsigned int)(uuid.data4[0])) << 24) | (((unsigned int)(uuid.data4[1])) << 16) | (((unsigned int)(uuid.data4[2])) << 8) | ((unsigned int)(uuid.data4[3]));
	const unsigned int u4 = (((unsigned int)(uuid.data4[4])) << 24) | (((unsigned int)(uuid.data4[5])) << 16) | (((unsigned int)(uuid.data4[6])) << 8) | ((unsigned int)(uuid.data4[7]));

	return bLong ? QString().sprintf("%08x%08x%08x%08x", u1, u2, u3, u4) : QString().sprintf("%08x%08x", (u1 ^ u2), (u3 ^ u4));
}


/*
 * Try to initialize the folder (with *write* access)
 */
static QString lamexp_try_init_folder(const QString &folderPath)
{
	bool success = false;

	const QFileInfo folderInfo(folderPath);
	const QDir folderDir(folderInfo.absoluteFilePath());

	//Create folder, if it does *not* exist yet
	if(!folderDir.exists())
	{
		folderDir.mkpath(".");
	}

	//Make sure folder exists now *and* is writable
	if(folderDir.exists())
	{
		QFile testFile(folderDir.absoluteFilePath(QString("~%1.tmp").arg(lamexp_rand_str())));
		if(testFile.open(QIODevice::ReadWrite))
		{
			const QByteArray testData = QByteArray("Lorem ipsum dolor sit amet, consectetur, adipisci velit!");
			if(testFile.write(testData) >= strlen(testData))
			{
				success = true;
				testFile.remove();
			}
			testFile.close();
		}
	}

	return (success ? folderDir.canonicalPath() : QString());
}

/*
 * Initialize LameXP temp folder
 */
#define INIT_TEMP_FOLDER(OUT,TMP) do \
{ \
	(OUT) = lamexp_try_init_folder(QString("%1/%2").arg((TMP), lamexp_rand_str())); \
} \
while(0)

/*
 * Get LameXP temp folder
 */
const QString &lamexp_temp_folder2(void)
{
	QReadLocker readLock(&g_lamexp_temp_folder.lock);

	//Already initialized?
	if(g_lamexp_temp_folder.path && (!g_lamexp_temp_folder.path->isEmpty()))
	{
		if(QDir(*g_lamexp_temp_folder.path).exists())
		{
			return *g_lamexp_temp_folder.path;
		}
	}

	//Obtain the write lock to initilaize
	readLock.unlock();
	QWriteLocker writeLock(&g_lamexp_temp_folder.lock);
	
	//Still uninitilaized?
	if(g_lamexp_temp_folder.path && (!g_lamexp_temp_folder.path->isEmpty()))
	{
		if(QDir(*g_lamexp_temp_folder.path).exists())
		{
			return *g_lamexp_temp_folder.path;
		}
	}

	//Create the string, if not done yet
	if(!g_lamexp_temp_folder.path)
	{
		g_lamexp_temp_folder.path = new QString();
	}
	
	g_lamexp_temp_folder.path->clear();

	//Try the %TMP% or %TEMP% directory first
	QString tempPath = lamexp_try_init_folder(QDir::temp().absolutePath());
	if(!tempPath.isEmpty())
	{
		INIT_TEMP_FOLDER(*g_lamexp_temp_folder.path, tempPath);
	}

	//Otherwise create TEMP folder in %LOCALAPPDATA%
	if(g_lamexp_temp_folder.path->isEmpty())
	{
		tempPath = lamexp_try_init_folder(QString("%1/Temp").arg(lamexp_known_folder(lamexp_folder_localappdata)));
		if(!tempPath.isEmpty())
		{
			INIT_TEMP_FOLDER(*g_lamexp_temp_folder.path, tempPath);
		}
	}

	//Failed to create TEMP folder?
	if(g_lamexp_temp_folder.path->isEmpty())
	{
		qFatal("Temporary directory could not be initialized !!!");
	}
	
	return *g_lamexp_temp_folder.path;
}

/*
 * Clean folder
 */
bool lamexp_clean_folder(const QString &folderPath)
{
	QDir tempFolder(folderPath);
	if(tempFolder.exists())
	{
		QFileInfoList entryList = tempFolder.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot);

		for(int i = 0; i < entryList.count(); i++)
		{
			if(entryList.at(i).isDir())
			{
				lamexp_clean_folder(entryList.at(i).canonicalFilePath());
			}
			else
			{
				for(int j = 0; j < 3; j++)
				{
					if(lamexp_remove_file(entryList.at(i).canonicalFilePath()))
					{
						break;
					}
				}
			}
		}
		return tempFolder.rmdir(".");
	}
	return true;
}

/*
 * Register tool
 */
void lamexp_register_tool(const QString &toolName, LockedFile *file, unsigned int version, const QString *tag)
{
	QWriteLocker writeLock(&g_lamexp_tools.lock);
	
	if(!g_lamexp_tools.registry) g_lamexp_tools.registry = new QMap<QString, LockedFile*>();
	if(!g_lamexp_tools.versions) g_lamexp_tools.versions = new QMap<QString, unsigned int>();
	if(!g_lamexp_tools.tags) g_lamexp_tools.tags = new QMap<QString, QString>();

	if(g_lamexp_tools.registry->contains(toolName.toLower()))
	{
		throw "lamexp_register_tool: Tool is already registered!";
	}

	g_lamexp_tools.registry->insert(toolName.toLower(), file);
	g_lamexp_tools.versions->insert(toolName.toLower(), version);
	g_lamexp_tools.tags->insert(toolName.toLower(), (tag) ? (*tag) : QString());
}

/*
 * Check for tool
 */
bool lamexp_check_tool(const QString &toolName)
{
	QReadLocker readLock(&g_lamexp_tools.lock);
	return (g_lamexp_tools.registry) ? g_lamexp_tools.registry->contains(toolName.toLower()) : false;
}

/*
 * Lookup tool path
 */
const QString lamexp_lookup_tool(const QString &toolName)
{
	QReadLocker readLock(&g_lamexp_tools.lock);

	if(g_lamexp_tools.registry)
	{
		if(g_lamexp_tools.registry->contains(toolName.toLower()))
		{
			return g_lamexp_tools.registry->value(toolName.toLower())->filePath();
		}
		else
		{
			return QString();
		}
	}
	else
	{
		return QString();
	}
}

/*
 * Lookup tool version
 */
unsigned int lamexp_tool_version(const QString &toolName, QString *tag)
{
	QReadLocker readLock(&g_lamexp_tools.lock);
	if(tag) tag->clear();

	if(g_lamexp_tools.versions)
	{
		if(g_lamexp_tools.versions->contains(toolName.toLower()))
		{
			if(tag)
			{
				if(g_lamexp_tools.tags->contains(toolName.toLower())) *tag = g_lamexp_tools.tags->value(toolName.toLower());
			}
			return g_lamexp_tools.versions->value(toolName.toLower());
		}
		else
		{
			return UINT_MAX;
		}
	}
	else
	{
		return UINT_MAX;
	}
}

/*
 * Version number to human-readable string
 */
const QString lamexp_version2string(const QString &pattern, unsigned int version, const QString &defaultText, const QString *tag)
{
	if(version == UINT_MAX)
	{
		return defaultText;
	}
	
	QString result = pattern;
	int digits = result.count("?", Qt::CaseInsensitive);
	
	if(digits < 1)
	{
		return result;
	}
	
	int pos = 0;
	QString versionStr = QString().sprintf(QString().sprintf("%%0%du", digits).toLatin1().constData(), version);
	int index = result.indexOf("?", Qt::CaseInsensitive);
	
	while(index >= 0 && pos < versionStr.length())
	{
		result[index] = versionStr[pos++];
		index = result.indexOf("?", Qt::CaseInsensitive);
	}

	if(tag)
	{
		result.replace(QChar('#'), *tag, Qt::CaseInsensitive);
	}

	return result;
}

/*
 * Register a new translation
 */
bool lamexp_translation_register(const QString &langId, const QString &qmFile, const QString &langName, unsigned int &systemId, unsigned int &country)
{
	QWriteLocker writeLockTranslations(&g_lamexp_translation.lock);

	if(qmFile.isEmpty() || langName.isEmpty() || systemId < 1)
	{
		return false;
	}

	if(!g_lamexp_translation.files) g_lamexp_translation.files = new QMap<QString, QString>();
	if(!g_lamexp_translation.names) g_lamexp_translation.names = new QMap<QString, QString>();
	if(!g_lamexp_translation.sysid) g_lamexp_translation.sysid = new QMap<QString, unsigned int>();
	if(!g_lamexp_translation.cntry) g_lamexp_translation.cntry = new QMap<QString, unsigned int>();

	g_lamexp_translation.files->insert(langId, qmFile);
	g_lamexp_translation.names->insert(langId, langName);
	g_lamexp_translation.sysid->insert(langId, systemId);
	g_lamexp_translation.cntry->insert(langId, country);

	return true;
}

/*
 * Get list of all translations
 */
QStringList lamexp_query_translations(void)
{
	QReadLocker readLockTranslations(&g_lamexp_translation.lock);
	return (g_lamexp_translation.files) ? g_lamexp_translation.files->keys() : QStringList();
}

/*
 * Get translation name
 */
QString lamexp_translation_name(const QString &langId)
{
	QReadLocker readLockTranslations(&g_lamexp_translation.lock);
	return (g_lamexp_translation.names) ? g_lamexp_translation.names->value(langId.toLower(), QString()) : QString();
}

/*
 * Get translation system id
 */
unsigned int lamexp_translation_sysid(const QString &langId)
{
	QReadLocker readLockTranslations(&g_lamexp_translation.lock);
	return (g_lamexp_translation.sysid) ? g_lamexp_translation.sysid->value(langId.toLower(), 0) : 0;
}

/*
 * Get translation script id
 */
unsigned int lamexp_translation_country(const QString &langId)
{
	QReadLocker readLockTranslations(&g_lamexp_translation.lock);
	return (g_lamexp_translation.cntry) ? g_lamexp_translation.cntry->value(langId.toLower(), 0) : 0;
}

/*
 * Install a new translator
 */
bool lamexp_install_translator(const QString &langId)
{
	bool success = false;
	const QString qmFileToPath(":/localization/%1");

	if(langId.isEmpty() || langId.toLower().compare(LAMEXP_DEFAULT_LANGID) == 0)
	{
		success = lamexp_install_translator_from_file(qmFileToPath.arg(LAMEXP_DEFAULT_TRANSLATION));
	}
	else
	{
		QReadLocker readLock(&g_lamexp_translation.lock);
		QString qmFile = (g_lamexp_translation.files) ? g_lamexp_translation.files->value(langId.toLower(), QString()) : QString();
		readLock.unlock();

		if(!qmFile.isEmpty())
		{
			success = lamexp_install_translator_from_file(qmFileToPath.arg(qmFile));
		}
		else
		{
			qWarning("Translation '%s' not available!", langId.toLatin1().constData());
		}
	}

	return success;
}

/*
 * Install a new translator from file
 */
bool lamexp_install_translator_from_file(const QString &qmFile)
{
	QWriteLocker writeLock(&g_lamexp_currentTranslator.lock);
	bool success = false;

	if(!g_lamexp_currentTranslator.instance)
	{
		g_lamexp_currentTranslator.instance = new QTranslator();
	}

	if(!qmFile.isEmpty())
	{
		QString qmPath = QFileInfo(qmFile).canonicalFilePath();
		QApplication::removeTranslator(g_lamexp_currentTranslator.instance);
		if(success = g_lamexp_currentTranslator.instance->load(qmPath))
		{
			QApplication::installTranslator(g_lamexp_currentTranslator.instance);
		}
		else
		{
			qWarning("Failed to load translation:\n\"%s\"", qmPath.toLatin1().constData());
		}
	}
	else
	{
		QApplication::removeTranslator(g_lamexp_currentTranslator.instance);
		success = true;
	}

	return success;
}

const QStringList &lamexp_arguments(void)
{
	QReadLocker readLock(&g_lamexp_argv.lock);

	if(!g_lamexp_argv.list)
	{
		readLock.unlock();
		QWriteLocker writeLock(&g_lamexp_argv.lock);

		g_lamexp_argv.list = new QStringList;

		int nArgs = 0;
		LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

		if(NULL != szArglist)
		{
			for(int i = 0; i < nArgs; i++)
			{
				(*g_lamexp_argv.list) << WCHAR2QSTR(szArglist[i]);
			}
			LocalFree(szArglist);
		}
		else
		{
			qWarning("CommandLineToArgvW() has failed !!!");
		}
	}

	return (*g_lamexp_argv.list);
}

/*
 * Locate known folder on local system
 */
const QString &lamexp_known_folder(lamexp_known_folder_t folder_id)
{
	typedef HRESULT (WINAPI *SHGetKnownFolderPathFun)(__in const GUID &rfid, __in DWORD dwFlags, __in HANDLE hToken, __out PWSTR *ppszPath);
	typedef HRESULT (WINAPI *SHGetFolderPathFun)(__in HWND hwndOwner, __in int nFolder, __in HANDLE hToken, __in DWORD dwFlags, __out LPWSTR pszPath);

	static const int CSIDL_LOCAL_APPDATA = 0x001c;
	static const int CSIDL_PROGRAM_FILES = 0x0026;
	static const int CSIDL_SYSTEM_FOLDER = 0x0025;
	static const GUID GUID_LOCAL_APPDATA = {0xF1B32785,0x6FBA,0x4FCF,{0x9D,0x55,0x7B,0x8E,0x7F,0x15,0x70,0x91}};
	static const GUID GUID_LOCAL_APPDATA_LOW = {0xA520A1A4,0x1780,0x4FF6,{0xBD,0x18,0x16,0x73,0x43,0xC5,0xAF,0x16}};
	static const GUID GUID_PROGRAM_FILES = {0x905e63b6,0xc1bf,0x494e,{0xb2,0x9c,0x65,0xb7,0x32,0xd3,0xd2,0x1a}};
	static const GUID GUID_SYSTEM_FOLDER = {0x1AC14E77,0x02E7,0x4E5D,{0xB7,0x44,0x2E,0xB1,0xAE,0x51,0x98,0xB7}};

	QReadLocker readLock(&g_lamexp_known_folder.lock);

	int folderCSIDL = -1;
	GUID folderGUID = {0x0000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}};
	size_t folderCacheId = size_t(-1);

	switch(folder_id)
	{
	case lamexp_folder_localappdata:
		folderCacheId = 0;
		folderCSIDL = CSIDL_LOCAL_APPDATA;
		folderGUID = GUID_LOCAL_APPDATA;
		break;
	case lamexp_folder_programfiles:
		folderCacheId = 1;
		folderCSIDL = CSIDL_PROGRAM_FILES;
		folderGUID = GUID_PROGRAM_FILES;
		break;
	case lamexp_folder_systemfolder:
		folderCacheId = 2;
		folderCSIDL = CSIDL_SYSTEM_FOLDER;
		folderGUID = GUID_SYSTEM_FOLDER;
		break;
	default:
		qWarning("Invalid 'known' folder was requested!");
		return *reinterpret_cast<QString*>(NULL);
		break;
	}

	//Already in cache?
	if(g_lamexp_known_folder.knownFolders)
	{
		if(g_lamexp_known_folder.knownFolders->contains(folderCacheId))
		{
			return (*g_lamexp_known_folder.knownFolders)[folderCacheId];
		}
	}

	//Obtain write lock to initialize
	readLock.unlock();
	QWriteLocker writeLock(&g_lamexp_known_folder.lock);

	//Still not in cache?
	if(g_lamexp_known_folder.knownFolders)
	{
		if(g_lamexp_known_folder.knownFolders->contains(folderCacheId))
		{
			return (*g_lamexp_known_folder.knownFolders)[folderCacheId];
		}
	}

	static SHGetKnownFolderPathFun SHGetKnownFolderPathPtr = NULL;
	static SHGetFolderPathFun SHGetFolderPathPtr = NULL;

	//Lookup functions
	if((!SHGetKnownFolderPathPtr) && (!SHGetFolderPathPtr))
	{
		QLibrary kernel32Lib("shell32.dll");
		if(kernel32Lib.load())
		{
			SHGetKnownFolderPathPtr = (SHGetKnownFolderPathFun) kernel32Lib.resolve("SHGetKnownFolderPath");
			SHGetFolderPathPtr = (SHGetFolderPathFun) kernel32Lib.resolve("SHGetFolderPathW");
		}
	}

	QString folder;

	//Now try to get the folder path!
	if(SHGetKnownFolderPathPtr)
	{
		WCHAR *path = NULL;
		if(SHGetKnownFolderPathPtr(folderGUID, 0x00008000, NULL, &path) == S_OK)
		{
			//MessageBoxW(0, path, L"SHGetKnownFolderPath", MB_TOPMOST);
			QDir folderTemp = QDir(QDir::fromNativeSeparators(QString::fromUtf16(reinterpret_cast<const unsigned short*>(path))));
			if(!folderTemp.exists())
			{
				folderTemp.mkpath(".");
			}
			if(folderTemp.exists())
			{
				folder = folderTemp.canonicalPath();
			}
			CoTaskMemFree(path);
		}
	}
	else if(SHGetFolderPathPtr)
	{
		WCHAR *path = new WCHAR[4096];
		if(SHGetFolderPathPtr(NULL, folderCSIDL, NULL, NULL, path) == S_OK)
		{
			//MessageBoxW(0, path, L"SHGetFolderPathW", MB_TOPMOST);
			QDir folderTemp = QDir(QDir::fromNativeSeparators(QString::fromUtf16(reinterpret_cast<const unsigned short*>(path))));
			if(!folderTemp.exists())
			{
				folderTemp.mkpath(".");
			}
			if(folderTemp.exists())
			{
				folder = folderTemp.canonicalPath();
			}
		}
		delete [] path;
	}

	//Create cache
	if(!g_lamexp_known_folder.knownFolders)
	{
		g_lamexp_known_folder.knownFolders = new QMap<size_t, QString>();
	}

	//Update cache
	g_lamexp_known_folder.knownFolders->insert(folderCacheId, folder);
	return (*g_lamexp_known_folder.knownFolders)[folderCacheId];
}

/*
 * Safely remove a file
 */
bool lamexp_remove_file(const QString &filename)
{
	if(!QFileInfo(filename).exists() || !QFileInfo(filename).isFile())
	{
		return true;
	}
	else
	{
		if(!QFile::remove(filename))
		{
			DWORD attributes = GetFileAttributesW(QWCHAR(filename));
			SetFileAttributesW(QWCHAR(filename), (attributes & (~FILE_ATTRIBUTE_READONLY)));
			if(!QFile::remove(filename))
			{
				qWarning("Could not delete \"%s\"", filename.toLatin1().constData());
				return false;
			}
			else
			{
				return true;
			}
		}
		else
		{
			return true;
		}
	}
}

/*
 * Check if visual themes are enabled (WinXP and later)
 */
bool lamexp_themes_enabled(void)
{
	typedef int (WINAPI *IsAppThemedFun)(void);
	
	QReadLocker readLock(&g_lamexp_themes_enabled.lock);
	if(g_lamexp_themes_enabled.bInitialized)
	{
		return g_lamexp_themes_enabled.bThemesEnabled;
	}

	readLock.unlock();
	QWriteLocker writeLock(&g_lamexp_themes_enabled.lock);

	if(!g_lamexp_themes_enabled.bInitialized)
	{
		g_lamexp_themes_enabled.bThemesEnabled = false;
		const lamexp_os_version_t * osVersion = lamexp_get_os_version();
		if(LAMEXP_MIN_OS_VER(osVersion, 5, 1))
		{
			IsAppThemedFun IsAppThemedPtr = NULL;
			QLibrary uxTheme(QString("%1/UxTheme.dll").arg(lamexp_known_folder(lamexp_folder_systemfolder)));
			if(uxTheme.load())
			{
				IsAppThemedPtr = (IsAppThemedFun) uxTheme.resolve("IsAppThemed");
			}
			if(IsAppThemedPtr)
			{
				g_lamexp_themes_enabled.bThemesEnabled = IsAppThemedPtr();
				if(!g_lamexp_themes_enabled.bThemesEnabled)
				{
					qWarning("Theme support is disabled for this process!");
				}
			}
		}
		g_lamexp_themes_enabled.bInitialized = true;
	}

	return g_lamexp_themes_enabled.bThemesEnabled;
}

/*
 * Get number of free bytes on disk
 */
unsigned __int64 lamexp_free_diskspace(const QString &path, bool *ok)
{
	ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
	if(GetDiskFreeSpaceExW(reinterpret_cast<const wchar_t*>(QDir::toNativeSeparators(path).utf16()), &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes))
	{
		if(ok) *ok = true;
		return freeBytesAvailable.QuadPart;
	}
	else
	{
		if(ok) *ok = false;
		return 0;
	}
}

/*
 * Check if computer does support hibernation
 */
bool lamexp_is_hibernation_supported(void)
{
	bool hibernationSupported = false;

	SYSTEM_POWER_CAPABILITIES pwrCaps;
	SecureZeroMemory(&pwrCaps, sizeof(SYSTEM_POWER_CAPABILITIES));
	
	if(GetPwrCapabilities(&pwrCaps))
	{
		hibernationSupported = pwrCaps.SystemS4 && pwrCaps.HiberFilePresent;
	}

	return hibernationSupported;
}

/*
 * Shutdown the computer
 */
bool lamexp_shutdown_computer(const QString &message, const unsigned long timeout, const bool forceShutdown, const bool hibernate)
{
	HANDLE hToken = NULL;

	if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		TOKEN_PRIVILEGES privileges;
		memset(&privileges, 0, sizeof(TOKEN_PRIVILEGES));
		privileges.PrivilegeCount = 1;
		privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		
		if(LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &privileges.Privileges[0].Luid))
		{
			if(AdjustTokenPrivileges(hToken, FALSE, &privileges, NULL, NULL, NULL))
			{
				if(hibernate)
				{
					if(SetSuspendState(TRUE, TRUE, TRUE))
					{
						return true;
					}
				}
				const DWORD reason = SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_FLAG_PLANNED;
				return InitiateSystemShutdownEx(NULL, const_cast<wchar_t*>(QWCHAR(message)), timeout, forceShutdown ? TRUE : FALSE, FALSE, reason);
			}
		}
	}
	
	return false;
}

/*
 * Make a window blink (to draw user's attention)
 */
void lamexp_blink_window(QWidget *poWindow, unsigned int count, unsigned int delay)
{
	static QMutex blinkMutex;

	const double maxOpac = 1.0;
	const double minOpac = 0.3;
	const double delOpac = 0.1;

	if(!blinkMutex.tryLock())
	{
		qWarning("Blinking is already in progress, skipping!");
		return;
	}
	
	try
	{
		const int steps = static_cast<int>(ceil(maxOpac - minOpac) / delOpac);
		const int sleep = static_cast<int>(floor(static_cast<double>(delay) / static_cast<double>(steps)));
		const double opacity = poWindow->windowOpacity();
	
		for(unsigned int i = 0; i < count; i++)
		{
			for(double x = maxOpac; x >= minOpac; x -= delOpac)
			{
				poWindow->setWindowOpacity(x);
				QApplication::processEvents();
				Sleep(sleep);
			}

			for(double x = minOpac; x <= maxOpac; x += delOpac)
			{
				poWindow->setWindowOpacity(x);
				QApplication::processEvents();
				Sleep(sleep);
			}
		}

		poWindow->setWindowOpacity(opacity);
		QApplication::processEvents();
		blinkMutex.unlock();
	}
	catch (...)
	{
		blinkMutex.unlock();
		qWarning("Exception error while blinking!");
	}
}

/*
 * Remove forbidden characters from a filename
 */
const QString lamexp_clean_filename(const QString &str)
{
	QString newStr(str);
	QRegExp rx("\"(.+)\"");
	rx.setMinimal(true);

	newStr.replace("\\", "-");
	newStr.replace(" / ", ", ");
	newStr.replace("/", ",");
	newStr.replace(":", "-");
	newStr.replace("*", "x");
	newStr.replace("?", "");
	newStr.replace("<", "[");
	newStr.replace(">", "]");
	newStr.replace("|", "!");
	newStr.replace(rx, "`\\1´");
	newStr.replace("\"", "'");
	
	return newStr.simplified();
}

/*
 * Remove forbidden characters from a file path
 */
const QString lamexp_clean_filepath(const QString &str)
{
	QStringList parts = QString(str).replace("\\", "/").split("/");

	for(int i = 0; i < parts.count(); i++)
	{
		parts[i] = lamexp_clean_filename(parts[i]);
	}

	return parts.join("/");
}

/*
 * Get a list of all available Qt Text Codecs
 */
QStringList lamexp_available_codepages(bool noAliases)
{
	QStringList codecList;
	
	QList<QByteArray> availableCodecs = QTextCodec::availableCodecs();
	while(!availableCodecs.isEmpty())
	{
		QByteArray current = availableCodecs.takeFirst();
		if(!(current.startsWith("system") || current.startsWith("System")))
		{
			codecList << QString::fromLatin1(current.constData(), current.size());
			if(noAliases)
			{
				if(QTextCodec *currentCodec = QTextCodec::codecForName(current.constData()))
				{
					
					QList<QByteArray> aliases = currentCodec->aliases();
					while(!aliases.isEmpty()) availableCodecs.removeAll(aliases.takeFirst());
				}
			}
		}
	}

	return codecList;
}

/*
 * Robert Jenkins' 96 bit Mix Function
 * Source: http://www.concentric.net/~Ttwang/tech/inthash.htm
 */
static unsigned int lamexp_mix(const unsigned int x, const unsigned int y, const unsigned int z)
{
	unsigned int a = x;
	unsigned int b = y;
	unsigned int c = z;
	
	a=a-b;  a=a-c;  a=a^(c >> 13);
	b=b-c;  b=b-a;  b=b^(a << 8); 
	c=c-a;  c=c-b;  c=c^(b >> 13);
	a=a-b;  a=a-c;  a=a^(c >> 12);
	b=b-c;  b=b-a;  b=b^(a << 16);
	c=c-a;  c=c-b;  c=c^(b >> 5);
	a=a-b;  a=a-c;  a=a^(c >> 3);
	b=b-c;  b=b-a;  b=b^(a << 10);
	c=c-a;  c=c-b;  c=c^(b >> 15);

	return c;
}

/*
 * Seeds the random number generator
 * Note: Altough rand_s() doesn't need a seed, this must be called pripr to lamexp_rand(), just to to be sure!
 */
void lamexp_seed_rand(void)
{
	qsrand(lamexp_mix(clock(), time(NULL), _getpid()));
}

/*
 * Returns a randum number
 * Note: This function uses rand_s() if available, but falls back to qrand() otherwise
 */
unsigned int lamexp_rand(void)
{
	quint32 rnd = 0;

	if(rand_s(&rnd) == 0)
	{
		return rnd;
	}

	for(size_t i = 0; i < sizeof(unsigned int); i++)
	{
		rnd = (rnd << 8) ^ qrand();
	}

	return rnd;
}

/*
 * Determines the current date, resistant against certain manipulations
 */
QDate lamexp_current_date_safe(void)
{
	const DWORD MAX_PROC = 1024;
	DWORD *processes = new DWORD[MAX_PROC];
	DWORD bytesReturned = 0;
	
	if(!EnumProcesses(processes, sizeof(DWORD) * MAX_PROC, &bytesReturned))
	{
		LAMEXP_DELETE_ARRAY(processes);
		return QDate::currentDate();
	}

	const DWORD procCount = bytesReturned / sizeof(DWORD);
	ULARGE_INTEGER lastStartTime;
	memset(&lastStartTime, 0, sizeof(ULARGE_INTEGER));

	for(DWORD i = 0; i < procCount; i++)
	{
		HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processes[i]);
		if(hProc)
		{
			FILETIME processTime[4];
			if(GetProcessTimes(hProc, &processTime[0], &processTime[1], &processTime[2], &processTime[3]))
			{
				ULARGE_INTEGER timeCreation;
				timeCreation.LowPart = processTime[0].dwLowDateTime;
				timeCreation.HighPart = processTime[0].dwHighDateTime;
				if(timeCreation.QuadPart > lastStartTime.QuadPart)
				{
					lastStartTime.QuadPart = timeCreation.QuadPart;
				}
			}
			CloseHandle(hProc);
		}
	}

	LAMEXP_DELETE_ARRAY(processes);
	
	FILETIME lastStartTime_fileTime;
	lastStartTime_fileTime.dwHighDateTime = lastStartTime.HighPart;
	lastStartTime_fileTime.dwLowDateTime = lastStartTime.LowPart;

	FILETIME lastStartTime_localTime;
	if(!FileTimeToLocalFileTime(&lastStartTime_fileTime, &lastStartTime_localTime))
	{
		memcpy(&lastStartTime_localTime, &lastStartTime_fileTime, sizeof(FILETIME));
	}
	
	SYSTEMTIME lastStartTime_system;
	if(!FileTimeToSystemTime(&lastStartTime_localTime, &lastStartTime_system))
	{
		memset(&lastStartTime_system, 0, sizeof(SYSTEMTIME));
		lastStartTime_system.wYear = 1970; lastStartTime_system.wMonth = lastStartTime_system.wDay = 1;
	}

	const QDate currentDate = QDate::currentDate();
	const QDate processDate = QDate(lastStartTime_system.wYear, lastStartTime_system.wMonth, lastStartTime_system.wDay);
	return (currentDate >= processDate) ? currentDate : processDate;
}


/*
 * Natural Order String Comparison - the 'lessThan' helper function
 */
static bool lamexp_natural_string_sort_helper(const QString &str1, const QString &str2)
{
	return (strnatcmp(QWCHAR(str1), QWCHAR(str2)) < 0);
}

/*
 * Natural Order String Comparison - the 'lessThan' helper function *with* case folding
 */
static bool lamexp_natural_string_sort_helper_fold_case(const QString &str1, const QString &str2)
{
	return (strnatcasecmp(QWCHAR(str1), QWCHAR(str2)) < 0);
}

/*
 * Natural Order String Comparison - the main sorting function
 */
void lamexp_natural_string_sort(QStringList &list, const bool bIgnoreCase)
{
	qSort(list.begin(), list.end(), bIgnoreCase ? lamexp_natural_string_sort_helper_fold_case : lamexp_natural_string_sort_helper);
}

/*
 * Suspend calling thread for N milliseconds
 */
void lamexp_sleep(const unsigned int delay)
{
	Sleep(delay);
}

bool lamexp_beep(int beepType)
{
	switch(beepType)
	{
		case lamexp_beep_info:    return MessageBeep(MB_ICONASTERISK) == TRUE;    break;
		case lamexp_beep_warning: return MessageBeep(MB_ICONEXCLAMATION) == TRUE; break;
		case lamexp_beep_error:   return MessageBeep(MB_ICONHAND) == TRUE;        break;
		default: return false;
	}
}

/*
 * Play a sound (from resources)
 */
bool lamexp_play_sound(const unsigned short uiSoundIdx, const bool bAsync, const wchar_t *alias)
{
	if(alias)
	{
		return PlaySound(alias, GetModuleHandle(NULL), (SND_ALIAS | (bAsync ? SND_ASYNC : SND_SYNC))) == TRUE;
	}
	else
	{
		return PlaySound(MAKEINTRESOURCE(uiSoundIdx), GetModuleHandle(NULL), (SND_RESOURCE | (bAsync ? SND_ASYNC : SND_SYNC))) == TRUE;
	}
}

/*
 * Play a sound (from resources)
 */
bool lamexp_play_sound_file(const QString &library, const unsigned short uiSoundIdx, const bool bAsync)
{
	bool result = false;
	HMODULE module = NULL;

	QFileInfo libraryFile(library);
	if(!libraryFile.isAbsolute())
	{
		unsigned int buffSize = GetSystemDirectoryW(NULL, NULL) + 1;
		wchar_t *buffer = (wchar_t*) _malloca(buffSize * sizeof(wchar_t));
		unsigned int result = GetSystemDirectory(buffer, buffSize);
		if(result > 0 && result < buffSize)
		{
			libraryFile.setFile(QString("%1/%2").arg(QDir::fromNativeSeparators(QString::fromUtf16(reinterpret_cast<const unsigned short*>(buffer))), library));
		}
		_freea(buffer);
	}

	module = LoadLibraryW(QWCHAR(QDir::toNativeSeparators(libraryFile.absoluteFilePath())));
	if(module)
	{
		result = (PlaySound(MAKEINTRESOURCE(uiSoundIdx), module, (SND_RESOURCE | (bAsync ? SND_ASYNC : SND_SYNC))) == TRUE);
		FreeLibrary(module);
	}

	return result;
}

/*
 * Open file using the shell
 */
bool lamexp_exec_shell(const QWidget *win, const QString &url, const bool explore)
{
	return lamexp_exec_shell(win, url, QString(), QString(), explore);
}

/*
 * Open file using the shell (with parameters)
 */
bool lamexp_exec_shell(const QWidget *win, const QString &url, const QString &parameters, const QString &directory, const bool explore)
{
	return ((int) ShellExecuteW(((win) ? win->winId() : NULL), (explore ? L"explore" : L"open"), QWCHAR(url), ((!parameters.isEmpty()) ? QWCHAR(parameters) : NULL), ((!directory.isEmpty()) ? QWCHAR(directory) : NULL), SW_SHOW)) > 32;
}

	/*
 * Query value of the performance counter
 */
__int64 lamexp_perfcounter_value(void)
{
	LARGE_INTEGER counter;
	if(QueryPerformanceCounter(&counter) == TRUE)
	{
		return counter.QuadPart;
	}
	return -1;
}

/*
 * Query frequency of the performance counter
 */
__int64 lamexp_perfcounter_frequ(void)
{
	LARGE_INTEGER frequency;
	if(QueryPerformanceFrequency(&frequency) == TRUE)
	{
		return frequency.QuadPart;
	}
	return -1;
}

/*
 * Insert entry to the window's system menu
 */
bool lamexp_append_sysmenu(const QWidget *win, const unsigned int identifier, const QString &text)
{
	bool ok = false;
	
	if(HMENU hMenu = GetSystemMenu(win->winId(), FALSE))
	{
		ok = (AppendMenuW(hMenu, MF_SEPARATOR, 0, 0) == TRUE);
		ok = (AppendMenuW(hMenu, MF_STRING, identifier, QWCHAR(text)) == TRUE);
	}

	return ok;
}

/*
 * Insert entry to the window's system menu
 */
bool lamexp_check_sysmenu_msg(void *message, const unsigned int identifier)
{
	return (((MSG*)message)->message == WM_SYSCOMMAND) && ((((MSG*)message)->wParam & 0xFFF0) == identifier);
}

/*
 * Update system menu entry
 */
bool lamexp_update_sysmenu(const QWidget *win, const unsigned int identifier, const QString &text)
{
	bool ok = false;
	
	if(HMENU hMenu = ::GetSystemMenu(win->winId(), FALSE))
	{
		ok = (ModifyMenu(hMenu, identifier, MF_STRING | MF_BYCOMMAND, identifier, QWCHAR(text)) == TRUE);
	}
	return ok;
}

/*
 * Display the window's close button
 */
bool lamexp_enable_close_button(const QWidget *win, const bool bEnable)
{
	bool ok = false;

	if(HMENU hMenu = GetSystemMenu(win->winId(), FALSE))
	{
		ok = (EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | (bEnable ? MF_ENABLED : MF_GRAYED)) == TRUE);
	}

	return ok;
}

/*
 * Check whether ESC key has been pressed since the previous call to this function
 */
bool lamexp_check_escape_state(void)
{
	return (GetAsyncKeyState(VK_ESCAPE) & 0x0001) != 0;
}

/*
 * Set the process priority class for current process
 */
bool lamexp_change_process_priority(const int priority)
{
	return lamexp_change_process_priority(GetCurrentProcess(), priority);
}

/*
 * Set the process priority class for specified process
 */
bool lamexp_change_process_priority(const QProcess *proc, const int priority)
{
	return lamexp_change_process_priority(proc->pid()->hProcess, priority);
}

/*
 * Set the process priority class for specified process
 */
bool lamexp_change_process_priority(void *hProcess, const int priority)
{
	bool ok = false;

	switch(qBound(-2, priority, 2))
	{
	case 2:
		ok = (SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS) == TRUE);
		break;
	case 1:
		if(!(ok = (SetPriorityClass(hProcess, ABOVE_NORMAL_PRIORITY_CLASS) == TRUE)))
		{
			ok = (SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS) == TRUE);
		}
		break;
	case 0:
		ok = (SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS) == TRUE);
		break;
	case -1:
		if(!(ok = (SetPriorityClass(hProcess, BELOW_NORMAL_PRIORITY_CLASS) == TRUE)))
		{
			ok = (SetPriorityClass(hProcess, IDLE_PRIORITY_CLASS) == TRUE);
		}
		break;
	case -2:
		ok = (SetPriorityClass(hProcess, IDLE_PRIORITY_CLASS) == TRUE);
		break;
	}

	return ok;
}

/*
 * Returns the current file time
 */
unsigned __int64 lamexp_current_file_time(void)
{
	FILETIME fileTime;
	GetSystemTimeAsFileTime(&fileTime);

	ULARGE_INTEGER temp;
	temp.HighPart = fileTime.dwHighDateTime;
	temp.LowPart = fileTime.dwLowDateTime;

	return temp.QuadPart;
}

/*
 * Bring the specifed window to the front
 */
bool lamexp_bring_to_front(const QWidget *win)
{
	const bool ret = (SetForegroundWindow(win->winId()) == TRUE);
	SwitchToThisWindow(win->winId(), TRUE);
	return ret;
}

/*
 * Bring window of the specifed process to the front (callback)
 */
static BOOL CALLBACK lamexp_bring_process_to_front_helper(HWND hwnd, LPARAM lParam)
{
	DWORD processId = *reinterpret_cast<WORD*>(lParam);
	DWORD windowProcessId = NULL;
	GetWindowThreadProcessId(hwnd, &windowProcessId);
	if(windowProcessId == processId)
	{
		SwitchToThisWindow(hwnd, TRUE);
		SetForegroundWindow(hwnd);
		return FALSE;
	}

	return TRUE;
}

/*
 * Bring window of the specifed process to the front
 */
bool lamexp_bring_process_to_front(const unsigned long pid)
{
	return EnumWindows(lamexp_bring_process_to_front_helper, reinterpret_cast<LPARAM>(&pid)) == TRUE;
}

/*
 * Check the Internet connection status
 */
bool lamexp_get_connection_state(void)
{
	DWORD lpdwFlags = NULL;
	BOOL result = InternetGetConnectedState(&lpdwFlags, NULL);
	return result == TRUE;
}

/*
 * Retrun the process ID of the given QProcess
 */
unsigned long lamexp_process_id(const QProcess *proc)
{
	PROCESS_INFORMATION *procInf = proc->pid();
	return (procInf) ? procInf->dwProcessId : NULL;
}

/*
 * Convert long path to short path
 */
QString lamexp_path_to_short(const QString &longPath)
{
	QString shortPath;
	DWORD buffSize = GetShortPathNameW(reinterpret_cast<const wchar_t*>(longPath.utf16()), NULL, NULL);
	
	if(buffSize > 0)
	{
		wchar_t *buffer = new wchar_t[buffSize];
		DWORD result = GetShortPathNameW(reinterpret_cast<const wchar_t*>(longPath.utf16()), buffer, buffSize);

		if(result > 0 && result < buffSize)
		{
			shortPath = QString::fromUtf16(reinterpret_cast<const unsigned short*>(buffer));
		}

		delete[] buffer;
	}

	return (shortPath.isEmpty() ? longPath : shortPath);
}

/*
 * Open media file in external player
 */
bool lamexp_open_media_file(const QString &mediaFilePath)
{
	const static wchar_t *registryPrefix[2] = { L"SOFTWARE\\", L"SOFTWARE\\Wow6432Node\\" };
	const static wchar_t *registryKeys[3] = 
	{
		L"Microsoft\\Windows\\CurrentVersion\\Uninstall\\{97D341C8-B0D1-4E4A-A49A-C30B52F168E9}",
		L"Microsoft\\Windows\\CurrentVersion\\Uninstall\\{DB9E4EAB-2717-499F-8D56-4CC8A644AB60}",
		L"foobar2000"
	};
	const static wchar_t *appNames[4] = { L"smplayer_portable.exe", L"smplayer.exe", L"MPUI.exe", L"foobar2000.exe" };
	const static wchar_t *valueNames[2] = { L"InstallLocation", L"InstallDir" };

	for(size_t i = 0; i < 3; i++)
	{
		for(size_t j = 0; j < 2; j++)
		{
			QString mplayerPath;
			HKEY registryKeyHandle = NULL;

			const QString currentKey = WCHAR2QSTR(registryPrefix[j]).append(WCHAR2QSTR(registryKeys[i]));
			if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, QWCHAR(currentKey), 0, KEY_READ, &registryKeyHandle) == ERROR_SUCCESS)
			{
				for(size_t k = 0; k < 2; k++)
				{
					wchar_t Buffer[4096];
					DWORD BuffSize = sizeof(wchar_t*) * 4096;
					DWORD DataType = REG_NONE;
					if(RegQueryValueExW(registryKeyHandle, valueNames[k], 0, &DataType, reinterpret_cast<BYTE*>(Buffer), &BuffSize) == ERROR_SUCCESS)
					{
						if((DataType == REG_SZ) || (DataType == REG_EXPAND_SZ) || (DataType == REG_LINK))
						{
							mplayerPath = WCHAR2QSTR(Buffer);
							break;
						}
					}
				}
				RegCloseKey(registryKeyHandle);
			}

			if(!mplayerPath.isEmpty())
			{
				QDir mplayerDir(mplayerPath);
				if(mplayerDir.exists())
				{
					for(size_t k = 0; k < 4; k++)
					{
						if(mplayerDir.exists(WCHAR2QSTR(appNames[k])))
						{
							qDebug("Player found at:\n%s\n", mplayerDir.absoluteFilePath(WCHAR2QSTR(appNames[k])).toUtf8().constData());
							QProcess::startDetached(mplayerDir.absoluteFilePath(WCHAR2QSTR(appNames[k])), QStringList() << QDir::toNativeSeparators(mediaFilePath));
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

/*
 * Entry point checks
 */
static DWORD lamexp_entry_check(void);
static DWORD g_lamexp_entry_check_result = lamexp_entry_check();
static DWORD g_lamexp_entry_check_flag = 0x789E09B2;
static DWORD lamexp_entry_check(void)
{
	volatile DWORD retVal = 0xA199B5AF;
	if(g_lamexp_entry_check_flag != 0x8761F64D)
	{
		lamexp_fatal_exit(L"Application initialization has failed, take care!");
	}
	return retVal;
}

/*
 * Application entry point (runs before static initializers)
 */
extern "C"
{
	int WinMainCRTStartup(void);
	
	int lamexp_entry_point(void)
	{
		if((!LAMEXP_DEBUG) && lamexp_check_for_debugger())
		{
			lamexp_fatal_exit(L"Not a debug build. Please unload debugger and try again!");
		}
		if(g_lamexp_entry_check_flag != 0x789E09B2)
		{
			lamexp_fatal_exit(L"Application initialization has failed, take care!");
		}

		//Zero *before* constructors are called
		LAMEXP_ZERO_MEMORY(g_lamexp_argv);
		LAMEXP_ZERO_MEMORY(g_lamexp_tools);
		LAMEXP_ZERO_MEMORY(g_lamexp_currentTranslator);
		LAMEXP_ZERO_MEMORY(g_lamexp_translation);
		LAMEXP_ZERO_MEMORY(g_lamexp_known_folder);
		LAMEXP_ZERO_MEMORY(g_lamexp_temp_folder);
		LAMEXP_ZERO_MEMORY(g_lamexp_ipc_ptr);
		LAMEXP_ZERO_MEMORY(g_lamexp_os_version);
		LAMEXP_ZERO_MEMORY(g_lamexp_themes_enabled);
		LAMEXP_ZERO_MEMORY(g_lamexp_portable);

		//Make sure we will pass the check
		g_lamexp_entry_check_flag = ~g_lamexp_entry_check_flag;

		//Now initialize the C Runtime library!
		return WinMainCRTStartup();
	}
}

/*
 * Fatal application exit
 */
#pragma intrinsic(_InterlockedExchange)
void lamexp_fatal_exit(const wchar_t* exitMessage, const wchar_t* errorBoxMessage)
{
	static volatile long bFatalFlag = 0L;

	if(_InterlockedExchange(&bFatalFlag, 1L) == 0L)
	{
		if(GetCurrentThreadId() != g_main_thread_id)
		{
			HANDLE mainThread = OpenThread(THREAD_TERMINATE, FALSE, g_main_thread_id);
			if(mainThread) TerminateThread(mainThread, ULONG_MAX);
		}
	
		if(errorBoxMessage)
		{
			MessageBoxW(NULL, errorBoxMessage, L"LameXP - GURU MEDITATION", MB_ICONERROR | MB_TOPMOST | MB_TASKMODAL);
		}

		for(;;)
		{
			FatalAppExit(0, exitMessage);
			TerminateProcess(GetCurrentProcess(), -1);
		}
	}

	TerminateThread(GetCurrentThread(), -1);
	Sleep(INFINITE);
}

/*
 * Finalization function (final clean-up)
 */
void lamexp_finalization(void)
{
	qDebug("lamexp_finalization()");
	
	//Free all tools
	if(g_lamexp_tools.registry)
	{
		QStringList keys = g_lamexp_tools.registry->keys();
		for(int i = 0; i < keys.count(); i++)
		{
			LAMEXP_DELETE((*g_lamexp_tools.registry)[keys.at(i)]);
		}
		LAMEXP_DELETE(g_lamexp_tools.registry);
		LAMEXP_DELETE(g_lamexp_tools.versions);
		LAMEXP_DELETE(g_lamexp_tools.tags);
	}
	
	//Delete temporary files
	if(g_lamexp_temp_folder.path)
	{
		if(!g_lamexp_temp_folder.path->isEmpty())
		{
			bool success = false;
			for(int i = 0; i < 100; i++)
			{
				if(lamexp_clean_folder(*g_lamexp_temp_folder.path))
				{
					success = true;
					break;
				}
				lamexp_sleep(100);
			}
			if(!success)
			{
				MessageBoxW(NULL, L"Sorry, LameXP was unable to clean up all temporary files. Some residual files in your TEMP directory may require manual deletion!", L"LameXP", MB_ICONEXCLAMATION|MB_TOPMOST);
				lamexp_exec_shell(NULL, *g_lamexp_temp_folder.path, QString(), QString(), true);
			}
		}
		LAMEXP_DELETE(g_lamexp_temp_folder.path);
	}

	//Clear folder cache
	LAMEXP_DELETE(g_lamexp_known_folder.knownFolders);

	//Clear languages
	if(g_lamexp_currentTranslator.instance)
	{
		QApplication::removeTranslator(g_lamexp_currentTranslator.instance);
		LAMEXP_DELETE(g_lamexp_currentTranslator.instance);
	}
	LAMEXP_DELETE(g_lamexp_translation.files);
	LAMEXP_DELETE(g_lamexp_translation.names);
	LAMEXP_DELETE(g_lamexp_translation.cntry);
	LAMEXP_DELETE(g_lamexp_translation.sysid);

	//Destroy Qt application object
	QApplication *application = dynamic_cast<QApplication*>(QApplication::instance());
	LAMEXP_DELETE(application);

	//Detach from shared memory
	if(g_lamexp_ipc_ptr.sharedmem) g_lamexp_ipc_ptr.sharedmem->detach();
	LAMEXP_DELETE(g_lamexp_ipc_ptr.sharedmem);
	LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_read);
	LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_write);
	LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_read_mutex);
	LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_write_mutex);

	//Free STDOUT and STDERR buffers
	if(g_lamexp_console_attached)
	{
		if(std::filebuf *tmp = dynamic_cast<std::filebuf*>(std::cout.rdbuf()))
		{
			std::cout.rdbuf(NULL);
			LAMEXP_DELETE(tmp);
		}
		if(std::filebuf *tmp = dynamic_cast<std::filebuf*>(std::cerr.rdbuf()))
		{
			std::cerr.rdbuf(NULL);
			LAMEXP_DELETE(tmp);
		}
	}

	//Close log file
	if(g_lamexp_log_file)
	{
		fclose(g_lamexp_log_file);
		g_lamexp_log_file = NULL;
	}

	//Free CLI Arguments
	LAMEXP_DELETE(g_lamexp_argv.list);
}

/*
 * Initialize debug thread
 */
static const HANDLE g_debug_thread = LAMEXP_DEBUG ? NULL : lamexp_debug_thread_init();

/*
 * Get number private bytes [debug only]
 */
unsigned long lamexp_dbg_private_bytes(void)
{
#if LAMEXP_DEBUG
	for(int i = 0; i < 8; i++) _heapmin();
	PROCESS_MEMORY_COUNTERS_EX memoryCounters;
	memoryCounters.cb = sizeof(PROCESS_MEMORY_COUNTERS_EX);
	GetProcessMemoryInfo(GetCurrentProcess(), (PPROCESS_MEMORY_COUNTERS) &memoryCounters, sizeof(PROCESS_MEMORY_COUNTERS_EX));
	return memoryCounters.PrivateUsage;
#else
	throw "Cannot call this function in a non-debug build!";
#endif //LAMEXP_DEBUG
}
