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

#include "Global.h"

//Target version
#include "Targetver.h"

//Visual Leaks Detector
#include <vld.h>

//Windows includes
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <MMSystem.h>
#include <ShellAPI.h>
#include <SensAPI.h>
#include <Objbase.h>
#include <PowrProf.h>
#include <Psapi.h>
#include <dwmapi.h>

//Qt includes
#include <QApplication>
#include <QDate>
#include <QDir>
#include <QEvent>
#include <QIcon>
#include <QImageReader>
#include <QLibrary>
#include <QLibraryInfo>
#include <QMap>
#include <QMessageBox>
#include <QMutex>
#include <QPlastiqueStyle>
#include <QProcess>
#include <QReadWriteLock>
#include <QRegExp>
#include <QSysInfo>
#include <QTextCodec>
#include <QTimer>
#include <QTranslator>
#include <QUuid>

//LameXP includes
#define LAMEXP_INC_CONFIG
#include "Resource.h"
#include "Config.h"

//CRT includes
#include <cstdio>
#include <iostream>
#include <fstream>
#include <io.h>
#include <fcntl.h>
#include <intrin.h>
#include <cmath>
#include <ctime>
#include <process.h>
#include <csignal>

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

///////////////////////////////////////////////////////////////////////////////
// HELPER MACROS
///////////////////////////////////////////////////////////////////////////////

#define _LAMEXP_MAKE_STR(STR) #STR
#define LAMEXP_MAKE_STR(STR) _LAMEXP_MAKE_STR(STR)

//String helper
#define CLEAN_OUTPUT_STRING(STR) do \
{ \
	const char CTRL_CHARS[3] = { '\r', '\n', '\t' }; \
	for(size_t i = 0; i < 3; i++) \
	{ \
		while(char *pos = strchr((STR), CTRL_CHARS[i])) *pos = char(0x20); \
	} \
} \
while(0)

//String helper
#define TRIM_LEFT(STR) do \
{ \
	const char WHITE_SPACE[4] = { char(0x20), '\r', '\n', '\t' }; \
	for(size_t i = 0; i < 4; i++) \
	{ \
		while(*(STR) == WHITE_SPACE[i]) (STR)++; \
	} \
} \
while(0)

///////////////////////////////////////////////////////////////////////////////
// GLOBAL VARS
///////////////////////////////////////////////////////////////////////////////

//Console attached flag
static bool g_lamexp_console_attached = false;

//Special folders
static struct
{
	QMap<size_t, QString> *knownFolders;
	QReadWriteLock lock;
}
g_lamexp_known_folder;

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

//Wine detection
static struct
{
	bool bInitialized;
	bool bIsWine;
	QReadWriteLock lock;
}
g_lamexp_wine;

//Win32 Theme support
static struct
{
	bool bInitialized;
	bool bThemesEnabled;
	QReadWriteLock lock;
}
g_lamexp_themes_enabled;

//Win32 DWM API functions
static struct
{
	bool bInitialized;
	QLibrary *dwmapi_dll;
	HRESULT (__stdcall *dwmExtendFrameIntoClientArea)(HWND hWnd, const MARGINS* pMarInset);
	HRESULT (__stdcall *dwmEnableBlurBehindWindow)(HWND hWnd, const DWM_BLURBEHIND* pBlurBehind);
	QReadWriteLock lock;
}
g_lamexp_dwmapi;

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

//Known Windows versions - maps marketing names to the actual Windows NT versions
const lamexp_os_version_t lamexp_winver_win2k = {5,0};
const lamexp_os_version_t lamexp_winver_winxp = {5,1};
const lamexp_os_version_t lamexp_winver_xpx64 = {5,2};
const lamexp_os_version_t lamexp_winver_vista = {6,0};
const lamexp_os_version_t lamexp_winver_win70 = {6,1};
const lamexp_os_version_t lamexp_winver_win80 = {6,2};
const lamexp_os_version_t lamexp_winver_win81 = {6,3};

//GURU MEDITATION
static const char *GURU_MEDITATION = "\n\nGURU MEDITATION !!!\n\n";

///////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

/*
 * Verify a specific Windows version
 */
static bool lamexp_verify_os_version(const DWORD major, const DWORD minor)
{
	OSVERSIONINFOEXW osvi;
	DWORDLONG dwlConditionMask = 0;

	//Initialize the OSVERSIONINFOEX structure
	memset(&osvi, 0, sizeof(OSVERSIONINFOEXW));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
	osvi.dwMajorVersion = major;
	osvi.dwMinorVersion = minor;
	osvi.dwPlatformId = VER_PLATFORM_WIN32_NT;

	//Initialize the condition mask
	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_PLATFORMID, VER_EQUAL);

	// Perform the test
	const BOOL ret = VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_PLATFORMID, dwlConditionMask);

	//Error checking
	if(!ret)
	{
		if(GetLastError() != ERROR_OLD_WIN_VERSION)
		{
			qWarning("VerifyVersionInfo() system call has failed!");
		}
	}

	return (ret != FALSE);
}

/*
 * Determine the *real* Windows version
 */
static bool lamexp_get_real_os_version(unsigned int *major, unsigned int *minor, bool *pbOverride)
{
	*major = *minor = 0;
	*pbOverride = false;
	
	//Initialize local variables
	OSVERSIONINFOEXW osvi;
	memset(&osvi, 0, sizeof(OSVERSIONINFOEXW));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

	//Try GetVersionEx() first
	if(GetVersionExW((LPOSVERSIONINFOW)&osvi) == FALSE)
	{
		qWarning("GetVersionEx() has failed, cannot detect Windows version!");
		return false;
	}

	//Make sure we are running on NT
	if(osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{
		*major = osvi.dwMajorVersion;
		*minor = osvi.dwMinorVersion;
	}
	else
	{
		qWarning("Not running on Windows NT, unsupported operating system!");
		return false;
	}

	//Determine the real *major* version first
	forever
	{
		const DWORD nextMajor = (*major) + 1;
		if(lamexp_verify_os_version(nextMajor, 0))
		{
			*pbOverride = true;
			*major = nextMajor;
			*minor = 0;
			continue;
		}
		break;
	}

	//Now also determine the real *minor* version
	forever
	{
		const DWORD nextMinor = (*minor) + 1;
		if(lamexp_verify_os_version((*major), nextMinor))
		{
			*pbOverride = true;
			*minor = nextMinor;
			continue;
		}
		break;
	}

	return true;
}

/*
 * Get the native operating system version
 */
const lamexp_os_version_t &lamexp_get_os_version(void)
{
	QReadLocker readLock(&g_lamexp_os_version.lock);

	//Already initialized?
	if(g_lamexp_os_version.bInitialized)
	{
		return g_lamexp_os_version.version;
	}
	
	readLock.unlock();
	QWriteLocker writeLock(&g_lamexp_os_version.lock);

	//Detect OS version
	if(!g_lamexp_os_version.bInitialized)
	{
		unsigned int major, minor; bool oflag;
		if(lamexp_get_real_os_version(&major, &minor, &oflag))
		{
			g_lamexp_os_version.version.versionMajor = major;
			g_lamexp_os_version.version.versionMinor = minor;
			g_lamexp_os_version.version.overrideFlag = oflag;
			g_lamexp_os_version.bInitialized = true;
		}
		else
		{
			qWarning("Failed to determin the operating system version!");
		}
	}

	return g_lamexp_os_version.version;
}

/*
 * Check if we are running under wine
 */
bool lamexp_detect_wine(void)
{
	QReadLocker readLock(&g_lamexp_wine.lock);

	//Already initialized?
	if(g_lamexp_wine.bInitialized)
	{
		return g_lamexp_wine.bIsWine;
	}
	
	readLock.unlock();
	QWriteLocker writeLock(&g_lamexp_wine.lock);

	if(!g_lamexp_wine.bInitialized)
	{
		g_lamexp_wine.bIsWine = false;
		QLibrary ntdll("ntdll.dll");
		if(ntdll.load())
		{
			if(ntdll.resolve("wine_nt_to_unix_file_name") != NULL) g_lamexp_wine.bIsWine = true;
			if(ntdll.resolve("wine_get_version") != NULL) g_lamexp_wine.bIsWine = true;
			ntdll.unload();
		}
		g_lamexp_wine.bInitialized = true;
	}

	return g_lamexp_wine.bIsWine;
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
 * Output logging message to console
 */
static void lamexp_write_console(const int type, const char *msg)
{	
	__try
	{
		if(_isatty(_fileno(stderr)))
		{
			UINT oldOutputCP = GetConsoleOutputCP();
			if(oldOutputCP != CP_UTF8) SetConsoleOutputCP(CP_UTF8);

			switch(type)
			{
			case QtCriticalMsg:
			case QtFatalMsg:
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
	}
	__except(1)
	{
		/*ignore any exception that might occur here!*/
	}
}

/*
 * Output logging message to debugger
 */
static void lamexp_write_dbg_out(const int type, const char *msg)
{	
	const char *FORMAT = "[LameXP][%c] %s\n";

	__try
	{
		char buffer[512];
		const char* input = msg;
		TRIM_LEFT(input);
		
		switch(type)
		{
		case QtCriticalMsg:
		case QtFatalMsg:
			_snprintf_s(buffer, 512, _TRUNCATE, FORMAT, 'C', input);
			break;
		case QtWarningMsg:
			_snprintf_s(buffer, 512, _TRUNCATE, FORMAT, 'W', input);
			break;
		default:
			_snprintf_s(buffer, 512, _TRUNCATE, FORMAT, 'I', input);
			break;
		}

		char *temp = &buffer[0];
		CLEAN_OUTPUT_STRING(temp);
		OutputDebugStringA(temp);
	}
	__except(1)
	{
		/*ignore any exception that might occur here!*/
	}
}

/*
 * Output logging message to logfile
 */
static void lamexp_write_logfile(const int type, const char *msg)
{	
	const char *FORMAT = "[%c][%04u] %s\r\n";

	__try
	{
		if(g_lamexp_log_file)
		{
			char buffer[512];
			strncpy_s(buffer, 512, msg, _TRUNCATE);

			char *temp = &buffer[0];
			TRIM_LEFT(temp);
			CLEAN_OUTPUT_STRING(temp);
			
			const unsigned int timestamp = static_cast<unsigned int>(_time64(NULL) % 3600I64);

			switch(type)
			{
			case QtCriticalMsg:
			case QtFatalMsg:
				fprintf(g_lamexp_log_file, FORMAT, 'C', timestamp, temp);
				break;
			case QtWarningMsg:
				fprintf(g_lamexp_log_file, FORMAT, 'W', timestamp, temp);
				break;
			default:
				fprintf(g_lamexp_log_file, FORMAT, 'I', timestamp, temp);
				break;
			}

			fflush(g_lamexp_log_file);
		}
	}
	__except(1)
	{
		/*ignore any exception that might occur here!*/
	}
}

/*
 * Qt message handler
 */
void lamexp_message_handler(QtMsgType type, const char *msg)
{
	if((!msg) || (!(msg[0])))
	{
		return;
	}

	QMutexLocker lock(&g_lamexp_message_mutex);

	if(g_lamexp_log_file)
	{
		lamexp_write_logfile(type, msg);
	}

	if(g_lamexp_console_attached)
	{
		lamexp_write_console(type, msg);
	}
	else
	{
		lamexp_write_dbg_out(type, msg);
	}

	if((type == QtCriticalMsg) || (type == QtFatalMsg))
	{
		lock.unlock();
		lamexp_fatal_exit(L"The application has encountered a critical error and will exit now!", QWCHAR(QString::fromUtf8(msg)));
	}
}

/*
 * Invalid parameters handler
 */
static void lamexp_invalid_param_handler(const wchar_t* exp, const wchar_t* fun, const wchar_t* fil, unsigned int, uintptr_t)
{
	lamexp_fatal_exit(L"Invalid parameter handler invoked, application will exit!");
}

/*
 * Signal handler
 */
static void lamexp_signal_handler(int signal_num)
{
	signal(signal_num, lamexp_signal_handler);
	lamexp_fatal_exit(L"Signal handler invoked, application will exit!");
}

/*
 * Global exception handler
 */
static LONG WINAPI lamexp_exception_handler(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	lamexp_fatal_exit(L"Unhandeled exception handler invoked, application will exit!");
	return LONG_MAX;
}

/*
 * Initialize error handlers
 */
void lamexp_init_error_handlers(void)
{
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
	SetUnhandledExceptionFilter(lamexp_exception_handler);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	_set_invalid_parameter_handler(lamexp_invalid_param_handler);
	
	static const int signal_num[6] = { SIGABRT, SIGFPE, SIGILL, SIGINT, SIGSEGV, SIGTERM };

	for(size_t i = 0; i < 6; i++)
	{
		signal(signal_num[i], lamexp_signal_handler);
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
					fprintf(temp, "%c%c%c", char(0xEF), char(0xBB), char(0xBF));
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

#if (!(defined(_M_X64) || defined(_M_IA64)))
	QLibrary Kernel32Lib("kernel32.dll");
	if(IsWow64ProcessFun IsWow64ProcessPtr = (IsWow64ProcessFun) Kernel32Lib.resolve("IsWow64Process"))
	{
		BOOL x64flag = FALSE;
		if(IsWow64ProcessPtr(GetCurrentProcess(), &x64flag))
		{
			features.x64 = (x64flag == TRUE);
		}
	}
#else
	features.x64 = true;
#endif

	DWORD_PTR procAffinity, sysAffinity;
	if(GetProcessAffinityMask(GetCurrentProcess(), &procAffinity, &sysAffinity))
	{
		for(DWORD_PTR mask = 1; mask; mask <<= 1)
		{
			features.count += ((sysAffinity & mask) ? (1) : (0));
		}
	}
	if(features.count < 1)
	{
		GetNativeSystemInfo(&systemInfo);
		features.count = qBound(1UL, systemInfo.dwNumberOfProcessors, 64UL);
	}

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
	__try
	{
		CloseHandle((HANDLE)((DWORD_PTR)-3));
	}
	__except(1)
	{
		return true;
	}
	__try 
	{
		__debugbreak();
	}
	__except(1) 
	{
		return IsDebuggerPresent();
	}
	return true;
}

/*
 * Check for debugger (thread proc)
 */
static unsigned int __stdcall lamexp_debug_thread_proc(LPVOID lpParameter)
{
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
	forever
	{
		if(lamexp_check_for_debugger())
		{
			lamexp_fatal_exit(L"Not a debug build. Please unload debugger and try again!");
			return 666;
		}
		lamexp_sleep(100);
	}
}

/*
 * Check for debugger (startup routine)
 */
static HANDLE lamexp_debug_thread_init()
{
	if(lamexp_check_for_debugger())
	{
		lamexp_fatal_exit(L"Not a debug build. Please unload debugger and try again!");
	}
	const uintptr_t h = _beginthreadex(NULL, 0, lamexp_debug_thread_proc, NULL, 0, NULL);
	return (HANDLE)(h^0xdeadbeef);
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
	const lamexp_os_version_t &osVersionNo = lamexp_get_os_version();
	if(osVersionNo < lamexp_winver_winxp)
	{
		qFatal("%s", QApplication::tr("Executable '%1' requires Windows XP or later.").arg(executableName).toLatin1().constData());
	}

	//Supported Windows version?
	if(osVersionNo == lamexp_winver_winxp)
	{
		qDebug("Running on Windows XP or Windows XP Media Center Edition.\n");						//lamexp_check_compatibility_mode("GetLargePageMinimum", executableName);
	}
	else if(osVersionNo == lamexp_winver_xpx64)
	{
		qDebug("Running on Windows Server 2003, Windows Server 2003 R2 or Windows XP x64.\n");		//lamexp_check_compatibility_mode("GetLocaleInfoEx", executableName);
	}
	else if(osVersionNo == lamexp_winver_vista)
	{
		qDebug("Running on Windows Vista or Windows Server 2008.\n");								//lamexp_check_compatibility_mode("CreateRemoteThreadEx", executableName*/);
	}
	else if(osVersionNo == lamexp_winver_win70)
	{
		qDebug("Running on Windows 7 or Windows Server 2008 R2.\n");								//lamexp_check_compatibility_mode("CreateFile2", executableName);
	}
	else if(osVersionNo == lamexp_winver_win80)
	{
		qDebug("Running on Windows 8 or Windows Server 2012.\n");									//lamexp_check_compatibility_mode("FindPackagesByPackageFamily", executableName);
	}
	else if(osVersionNo == lamexp_winver_win81)
	{
		qDebug("Running on Windows 8.1 or Windows Server 2012 R2.\n");								//lamexp_check_compatibility_mode(NULL, executableName);
	}
	else
	{
		const QString message = QString().sprintf("Running on an unknown WindowsNT-based system (v%u.%u).", osVersionNo.versionMajor, osVersionNo.versionMinor);
		qWarning("%s\n", QUTF8(message));
		MessageBoxW(NULL, QWCHAR(message), L"LameXP", MB_OK | MB_TOPMOST | MB_ICONWARNING);
	}

	//Check for compat mode
	if(osVersionNo.overrideFlag && (osVersionNo <= lamexp_winver_win81))
	{
		qWarning("Windows compatibility mode detected!");
		if(!arguments.contains("--ignore-compat-mode", Qt::CaseInsensitive))
		{
			qFatal("%s", QApplication::tr("Executable '%1' doesn't support Windows compatibility mode.").arg(executableName).toLatin1().constData());
			return false;
		}
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
	qDebug("Library Path:\n%s\n", QUTF8(QApplication::libraryPaths().first()));

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
	
	//Add the default translations
	lamexp_translation_init();

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
			QPixmap pixmap = QIcon(":/icons/sound.png").pixmap(16, 16);
			if((SetConsoleIconPtr != NULL) && (!pixmap.isNull())) SetConsoleIconPtr(pixmap.toWinHICON());
			kernel32.unload();
		}
	}
#endif

	//Done
	qt_initialized = true;
	return true;
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
			static const DWORD attrMask = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
			const DWORD attributes = GetFileAttributesW(QWCHAR(filename));
			if(attributes & attrMask)
			{
				SetFileAttributesW(QWCHAR(filename), FILE_ATTRIBUTE_NORMAL);
			}
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
		const lamexp_os_version_t &osVersion = lamexp_get_os_version();
		if(osVersion >= lamexp_winver_winxp)
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
 * Suspend calling thread for N milliseconds
 */
inline void lamexp_sleep(const unsigned int delay)
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
	if(Q_PID qPid = proc->pid())
	{
		return lamexp_change_process_priority(qPid->hProcess, priority);
	}
	else
	{
		return false;
	}
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
 * Check the network connection status
 */
int lamexp_network_status(void)
{
	DWORD dwFlags;
	const BOOL ret = (IsNetworkAlive(&dwFlags) == TRUE);
	if(GetLastError() == 0)
	{
		return (ret == TRUE) ? lamexp_network_yes : lamexp_network_non;
	}
	return lamexp_network_err;
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
							qDebug("Player found at:\n%s\n", QUTF8(mplayerDir.absoluteFilePath(WCHAR2QSTR(appNames[k]))));
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

static void lamexp_init_dwmapi(void)
{
	QReadLocker writeLock(&g_lamexp_dwmapi.lock);

	//Not initialized yet?
	if(g_lamexp_dwmapi.bInitialized)
	{
		return;
	}
	
	//Does OS support DWM?
	if(lamexp_get_os_version() >= lamexp_winver_vista)
	{
		//Load DWMAPI.DLL
		g_lamexp_dwmapi.dwmapi_dll = new QLibrary("dwmapi.dll");
		if(g_lamexp_dwmapi.dwmapi_dll->load())
		{
			//Lookup required functions
			g_lamexp_dwmapi.dwmExtendFrameIntoClientArea = (HRESULT (__stdcall*)(HWND, const MARGINS*))        g_lamexp_dwmapi.dwmapi_dll->resolve("DwmExtendFrameIntoClientArea");
			g_lamexp_dwmapi.dwmEnableBlurBehindWindow    = (HRESULT (__stdcall*)(HWND, const DWM_BLURBEHIND*)) g_lamexp_dwmapi.dwmapi_dll->resolve("DwmEnableBlurBehindWindow");
		}
		else
		{
			LAMEXP_DELETE(g_lamexp_dwmapi.dwmapi_dll);
			qWarning("Failed to load DWMAPI.DLL on a DWM-enabled system!");
		}
	}

	g_lamexp_dwmapi.bInitialized = true;
}

/*
 * Enable "sheet of glass" effect on the given Window
 */
bool lamexp_sheet_of_glass(QWidget *window)
{
	QReadLocker readLock(&g_lamexp_dwmapi.lock);

	//Initialize the DWM API
	while(!g_lamexp_dwmapi.bInitialized)
	{
		readLock.unlock();
		lamexp_init_dwmapi();
		readLock.relock();
	}
	
	//Required functions available?
	if((g_lamexp_dwmapi.dwmExtendFrameIntoClientArea == NULL) || (g_lamexp_dwmapi.dwmEnableBlurBehindWindow == NULL))
	{
		return false;
	}

	//Enable the "sheet of glass" effect on this window
	MARGINS margins = {-1, -1, -1, -1};
	if(HRESULT hr = g_lamexp_dwmapi.dwmExtendFrameIntoClientArea(window->winId(), &margins))
	{
		qWarning("DwmExtendFrameIntoClientArea function has failed! (error %d)", hr);
		return false;
	}

	//Create and populate the Blur Behind structure
	DWM_BLURBEHIND bb;
	memset(&bb, 0, sizeof(DWM_BLURBEHIND));
	bb.fEnable = TRUE;
	bb.dwFlags = DWM_BB_ENABLE;
	if(HRESULT hr = g_lamexp_dwmapi.dwmEnableBlurBehindWindow(window->winId(), &bb))
	{
		qWarning("DwmEnableBlurBehindWindow function has failed! (error %d)", hr);
		return false;
	}

	//Required for Qt
	window->setAttribute(Qt::WA_TranslucentBackground);
	window->setAttribute(Qt::WA_NoSystemBackground);

	return true;
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
	lamexp_clean_all_tools();
	
	//Delete temporary files
	const QString &tempFolder = lamexp_temp_folder2();
	if(!tempFolder.isEmpty())
	{
		bool success = false;
		for(int i = 0; i < 100; i++)
		{
			if(lamexp_clean_folder(tempFolder))
			{
				success = true;
				break;
			}
			lamexp_sleep(100);
		}
		if(!success)
		{
			MessageBoxW(NULL, L"Sorry, LameXP was unable to clean up all temporary files. Some residual files in your TEMP directory may require manual deletion!", L"LameXP", MB_ICONEXCLAMATION|MB_TOPMOST);
			lamexp_exec_shell(NULL, tempFolder, QString(), QString(), true);
		}
	}

	//Clear folder cache
	LAMEXP_DELETE(g_lamexp_known_folder.knownFolders);

	//Clear languages
	lamexp_clean_all_translations();

	//Destroy Qt application object
	QApplication *application = dynamic_cast<QApplication*>(QApplication::instance());
	LAMEXP_DELETE(application);

	//Release DWM API
	g_lamexp_dwmapi.dwmEnableBlurBehindWindow = NULL;
	g_lamexp_dwmapi.dwmExtendFrameIntoClientArea = NULL;
	LAMEXP_DELETE(g_lamexp_dwmapi.dwmapi_dll);

	//Detach from shared memory
	lamexp_ipc_exit();

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

	//Free TEMP folder
	lamexp_temp_folder_clear();
}

/*
 * Initialize debug thread
 */
static const HANDLE g_debug_thread1 = LAMEXP_DEBUG ? NULL : lamexp_debug_thread_init();

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
	THROW("Cannot call this function in a non-debug build!");
#endif //LAMEXP_DEBUG
}

/*
 * Output string to debugger [debug only]
 */
void lamexp_dbg_dbg_output_string(const char* format, ...)
{
#if LAMEXP_DEBUG
	char buffer[256];
	va_list args;
	va_start (args, format);
	vsnprintf_s(buffer, 256, _TRUNCATE, format, args);
	OutputDebugStringA(buffer);
	va_end(args);
#else
	THROW("Cannot call this function in a non-debug build!");
#endif //LAMEXP_DEBUG
}

///////////////////////////////////////////////////////////////////////////////
// INITIALIZATION
///////////////////////////////////////////////////////////////////////////////

extern "C" void _lamexp_global_init_win32(void)
{
	if((!LAMEXP_DEBUG) && lamexp_check_for_debugger())
	{
		lamexp_fatal_exit(L"Not a debug build. Please unload debugger and try again!");
	}

	//Zero *before* constructors are called
	LAMEXP_ZERO_MEMORY(g_lamexp_argv);
	LAMEXP_ZERO_MEMORY(g_lamexp_known_folder);
	LAMEXP_ZERO_MEMORY(g_lamexp_os_version);
	LAMEXP_ZERO_MEMORY(g_lamexp_wine);
	LAMEXP_ZERO_MEMORY(g_lamexp_themes_enabled);
	LAMEXP_ZERO_MEMORY(g_lamexp_dwmapi);
}
