///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2012 LoRd_MuldeR <MuldeR2@GMX.de>
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

//LameXP includes
#include "Resource.h"
#include "LockedFile.h"

//CRT includes
#include <iostream>
#include <fstream>
#include <io.h>
#include <fcntl.h>
#include <intrin.h>
#include <math.h>
#include <time.h>

//COM includes
#include <Objbase.h>
#include <PowrProf.h>

//Debug only includes
#if LAMEXP_DEBUG
#include <Psapi.h>
#endif

//Initialize static Qt plugins
#ifdef QT_NODLL
Q_IMPORT_PLUGIN(qico)
Q_IMPORT_PLUGIN(qsvg)
#endif

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
	char *ver_release_name;
}
g_lamexp_version =
{
	VER_LAMEXP_MAJOR,
	VER_LAMEXP_MINOR,
	VER_LAMEXP_BUILD,
	VER_LAMEXP_RNAME
};

//Build date
static QDate g_lamexp_version_date;
static const char *g_lamexp_months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static const char *g_lamexp_version_raw_date = __DATE__;
static const char *g_lamexp_version_raw_time = __TIME__;

//Console attached flag
static bool g_lamexp_console_attached = false;

//Compiler detection
//The following code was borrowed from MPC-HC project: http://mpc-hc.sf.net/
#if defined(__INTEL_COMPILER)
	#if (__INTEL_COMPILER >= 1200)
		static const char *g_lamexp_version_compiler = "ICL 12.x";
	#elif (__INTEL_COMPILER >= 1100)
		static const char *g_lamexp_version_compiler = = "ICL 11.x";
	#elif (__INTEL_COMPILER >= 1000)
		static const char *g_lamexp_version_compiler = = "ICL 10.x";
	#else
		#error Compiler is not supported!
	#endif
#elif defined(_MSC_VER)
	#if (_MSC_VER == 1600)
		#if (_MSC_FULL_VER >= 160040219)
			static const char *g_lamexp_version_compiler = "MSVC 2010-SP1";
		#else
			static const char *g_lamexp_version_compiler = "MSVC 2010";
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
			LAMEXP_COMPILER_WARNING("SSE2 instruction set is enabled!")
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

//Official web-site URL
static const char *g_lamexp_website_url = "http://lamexp.sourceforge.net/";
static const char *g_lamexp_support_url = "http://forum.doom9.org/showthread.php?t=157726";

//Tool versions (expected versions!)
static const unsigned int g_lamexp_toolver_neroaac = VER_LAMEXP_TOOL_NEROAAC;
static const unsigned int g_lamexp_toolver_fhgaacenc = VER_LAMEXP_TOOL_FHGAACENC;
static const unsigned int g_lamexp_toolver_qaacenc = VER_LAMEXP_TOOL_QAAC;
static const unsigned int g_lamexp_toolver_coreaudio = VER_LAMEXP_TOOL_COREAUDIO;

//Special folders
static QString g_lamexp_temp_folder;

//Tools
static QMap<QString, LockedFile*> g_lamexp_tool_registry;
static QMap<QString, unsigned int> g_lamexp_tool_versions;

//Languages
static struct
{
	QMap<QString, QString> files;
	QMap<QString, QString> names;
	QMap<QString, unsigned int> sysid;
	QMap<QString, unsigned int> cntry;
}
g_lamexp_translation;

//Translator
static QTranslator *g_lamexp_currentTranslator = NULL;

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
}
g_lamexp_ipc_ptr =
{
	NULL, NULL, NULL
};

//Image formats
static const char *g_lamexp_imageformats[] = {"png", "jpg", "gif", "ico", "svg", NULL};

//Global locks
static QMutex g_lamexp_message_mutex;

//Main thread ID
static const DWORD g_main_thread_id = GetCurrentThreadId();

//Log file
static FILE *g_lamexp_log_file = NULL;

///////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

/*
 * Version getters
 */
unsigned int lamexp_version_major(void) { return g_lamexp_version.ver_major; }
unsigned int lamexp_version_minor(void) { return g_lamexp_version.ver_minor; }
unsigned int lamexp_version_build(void) { return g_lamexp_version.ver_build; }
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
	return LAMEXP_DEBUG || (!releaseVersion);
}

/*
 * Calculate expiration date
 */
QDate lamexp_version_expires(void)
{
	return lamexp_version_date().addDays(LAMEXP_DEBUG ? 2 : 30);
}

/*
 * Get build date date
 */
const QDate &lamexp_version_date(void)
{
	if(!g_lamexp_version_date.isValid())
	{
		int date[3] = {0, 0, 0}; char temp[12] = {'\0'};
		strncpy_s(temp, 12, g_lamexp_version_raw_date, _TRUNCATE);

		if(strlen(temp) == 11)
		{
			temp[3] = temp[6] = '\0';
			date[2] = atoi(&temp[4]);
			date[0] = atoi(&temp[7]);
			
			for(int j = 0; j < 12; j++)
			{
				if(!_strcmpi(&temp[0], g_lamexp_months[j]))
				{
					date[1] = j+1;
					break;
				}
			}

			g_lamexp_version_date = QDate(date[0], date[1], date[2]);
		}

		if(!g_lamexp_version_date.isValid())
		{
			qFatal("Internal error: Date format could not be recognized!");
		}
	}

	return g_lamexp_version_date;
}

/*
 * Get the native operating system version
 */
DWORD lamexp_get_os_version(void)
{
	OSVERSIONINFO osVerInfo;
	memset(&osVerInfo, 0, sizeof(OSVERSIONINFO));
	osVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	DWORD version = 0;
	
	if(GetVersionEx(&osVerInfo) == TRUE)
	{
		if(osVerInfo.dwPlatformId != VER_PLATFORM_WIN32_NT)
		{
			throw "Ouuups: Not running under Windows NT. This is not supposed to happen!";
		}
		version = (DWORD)((osVerInfo.dwMajorVersion << 16) | (osVerInfo.dwMinorVersion & 0xffff));
	}

	return version;
}

/*
 * Global exception handler
 */
LONG WINAPI lamexp_exception_handler(__in struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	if(GetCurrentThreadId() != g_main_thread_id)
	{
		HANDLE mainThread = OpenThread(THREAD_TERMINATE, FALSE, g_main_thread_id);
		if(mainThread) TerminateThread(mainThread, ULONG_MAX);
	}
	
	FatalAppExit(0, L"Unhandeled exception handler invoked, application will exit!");
	TerminateProcess(GetCurrentProcess(), -1);
	return LONG_MAX;
}

/*
 * Invalid parameters handler
 */
void lamexp_invalid_param_handler(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t)
{
	if(GetCurrentThreadId() != g_main_thread_id)
	{
		HANDLE mainThread = OpenThread(THREAD_TERMINATE, FALSE, g_main_thread_id);
		if(mainThread) TerminateThread(mainThread, ULONG_MAX);
		
	}
	
	FatalAppExit(0, L"Invalid parameter handler invoked, application will exit!");
	TerminateProcess(GetCurrentProcess(), -1);
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

	if(type == QtCriticalMsg || type == QtFatalMsg)
	{
		lock.unlock();
		MessageBoxW(NULL, QWCHAR(QString::fromUtf8(msg)), L"LameXP - GURU MEDITATION", MB_ICONERROR | MB_TOPMOST | MB_TASKMODAL);
		FatalAppExit(0, L"The application has encountered a critical error and will exit now!");
		TerminateProcess(GetCurrentProcess(), -1);
	}
}

/*
 * Initialize the console
 */
void lamexp_init_console(int argc, char* argv[])
{
	bool enableConsole = lamexp_version_demo();

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
		for(int i = 0; i < argc; i++)
		{
			if(!_stricmp(argv[i], "--console"))
			{
				enableConsole = true;
			}
			else if(!_stricmp(argv[i], "--no-console"))
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
			int hCrtStdErr = _open_osfhandle((intptr_t) GetStdHandle(STD_ERROR_HANDLE), flags);
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
lamexp_cpu_t lamexp_detect_cpu_features(int argc, char **argv)
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

	if((argv != NULL) && (argc > 0))
	{
		bool flag = false;
		for(int i = 0; i < argc; i++)
		{
			if(!_stricmp("--force-cpu-no-64bit", argv[i])) { flag = true; features.x64 = false; }
			if(!_stricmp("--force-cpu-no-sse", argv[i])) { flag = true; features.sse = features.sse2 = features.sse3 = features.ssse3 = false; }
			if(!_stricmp("--force-cpu-no-intel", argv[i])) { flag = true; features.intel = false; }
		}
		if(flag) qWarning("CPU flags overwritten by user-defined parameters. Take care!\n");
	}

	return features;
}

/*
 * Check for debugger (detect routine)
 */
static bool lamexp_check_for_debugger(void)
{
	__try 
	{
		DebugBreak();
	}
	__except(GetExceptionCode() == EXCEPTION_BREAKPOINT ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) 
	{
		return false;
	}
	return true;
}

/*
 * Check for debugger (thread proc)
 */
static void WINAPI lamexp_debug_thread_proc(__in LPVOID lpParameter)
{
	while(!(IsDebuggerPresent() || lamexp_check_for_debugger()))
	{
		Sleep(333);
	}
	TerminateProcess(GetCurrentProcess(), -1);
}

/*
 * Check for debugger (startup routine)
 */
static HANDLE lamexp_debug_thread_init(void)
{
	if(IsDebuggerPresent() || lamexp_check_for_debugger())
	{
		FatalAppExit(0, L"Not a debug build. Please unload debugger and try again!");
		TerminateProcess(GetCurrentProcess(), -1);
	}

	return CreateThread(NULL, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(&lamexp_debug_thread_proc), NULL, NULL, NULL);
}

/*
 * Check for compatibility mode
 */
static bool lamexp_check_compatibility_mode(const char *exportName, const char *executableName)
{
	QLibrary kernel32("kernel32.dll");

	if(exportName != NULL)
	{
		if(kernel32.resolve(exportName) != NULL)
		{
			qWarning("Function '%s' exported from 'kernel32.dll' -> Windows compatibility mode!", exportName);
			qFatal("%s", QApplication::tr("Executable '%1' doesn't support Windows compatibility mode.").arg(QString::fromLatin1(executableName)).toLatin1().constData());
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
	bool isWine = false;
	typedef BOOL (WINAPI *SetDllDirectoryProc)(WCHAR *lpPathName);

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
	char *executableName = argv[0];
	while(char *temp = strpbrk(executableName, "\\/:?"))
	{
		executableName = temp + 1;
	}

	//Check Qt version
	qDebug("Using Qt v%s [%s], %s, %s", qVersion(), QLibraryInfo::buildDate().toString(Qt::ISODate).toLatin1().constData(), (qSharedBuild() ? "DLL" : "Static"), QLibraryInfo::buildKey().toLatin1().constData());
	qDebug("Compiled with Qt v%s [%s], %s\n", QT_VERSION_STR, QT_PACKAGEDATE_STR, QT_BUILD_KEY);
	if(_stricmp(qVersion(), QT_VERSION_STR))
	{
		qFatal("%s", QApplication::tr("Executable '%1' requires Qt v%2, but found Qt v%3.").arg(QString::fromLatin1(executableName), QString::fromLatin1(QT_VERSION_STR), QString::fromLatin1(qVersion())).toLatin1().constData());
		return false;
	}
	if(QLibraryInfo::buildKey().compare(QString::fromLatin1(QT_BUILD_KEY), Qt::CaseInsensitive))
	{
		qFatal("%s", QApplication::tr("Executable '%1' was built for Qt '%2', but found Qt '%3'.").arg(QString::fromLatin1(executableName), QString::fromLatin1(QT_BUILD_KEY), QLibraryInfo::buildKey()).toLatin1().constData());
		return false;
	}

	//Check the Windows version
	switch(QSysInfo::windowsVersion() & QSysInfo::WV_NT_based)
	{
	case 0:
	case QSysInfo::WV_NT:
		qFatal("%s", QApplication::tr("Executable '%1' requires Windows 2000 or later.").arg(QString::fromLatin1(executableName)).toLatin1().constData());
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
		lamexp_check_compatibility_mode(NULL, executableName);
		break;
	default:
		{
			DWORD osVersionNo = lamexp_get_os_version();
			qWarning("Running on an unknown/untested WinNT-based OS (v%u.%u).\n", HIWORD(osVersionNo), LOWORD(osVersionNo));
		}
		break;
	}

	//Check for Wine
	QLibrary ntdll("ntdll.dll");
	if(ntdll.load())
	{
		if(ntdll.resolve("wine_nt_to_unix_file_name") != NULL) isWine = true;
		if(ntdll.resolve("wine_get_version") != NULL) isWine = true;
		if(isWine) qWarning("It appears we are running under Wine, unexpected things might happen!\n");
		ntdll.unload();
	}

	//Create Qt application instance and setup version info
	QApplication *application = new QApplication(argc, argv);
	application->setApplicationName("LameXP - Audio Encoder Front-End");
	application->setApplicationVersion(QString().sprintf("%d.%02d.%04d", lamexp_version_major(), lamexp_version_minor(), lamexp_version_build())); 
	application->setOrganizationName("LoRd_MuldeR");
	application->setOrganizationDomain("mulder.at.gg");
	application->setWindowIcon(lamexp_app_icon());
	application->setEventFilter(lamexp_event_filter);

	//Set text Codec for locale
	QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

	//Load plugins from application directory
	QCoreApplication::setLibraryPaths(QStringList() << QApplication::applicationDirPath());
	qDebug("Library Path:\n%s\n", QApplication::libraryPaths().first().toUtf8().constData());

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
	g_lamexp_translation.files.insert(LAMEXP_DEFAULT_LANGID, "");
	g_lamexp_translation.names.insert(LAMEXP_DEFAULT_LANGID, "English");

	//Check for process elevation
	if(!lamexp_check_elevation())
	{
		if(QMessageBox::warning(NULL, "LameXP", "<nobr>LameXP was started with elevated rights. This is a potential security risk!</nobr>", "Quit Program (Recommended)", "Ignore") == 0)
		{
			return false;
		}
	}

	//Update console icon, if a console is attached
	if(g_lamexp_console_attached && !isWine)
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

	//Done
	qt_initialized = true;
	return true;
}

/*
 * Initialize IPC
 */
int lamexp_init_ipc(void)
{
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
	QString baseName = QFileInfo(QApplication::applicationFilePath()).completeBaseName();
	int idx1 = baseName.indexOf("lamexp", 0, Qt::CaseInsensitive);
	int idx2 = baseName.lastIndexOf("portable", -1, Qt::CaseInsensitive);
	return (idx1 >= 0) && (idx2 >= 0) && (idx1 < idx2);
}

/*
 * Get a random string
 */
QString lamexp_rand_str(void)
{
	QRegExp regExp("\\{(\\w+)-(\\w+)-(\\w+)-(\\w+)-(\\w+)\\}");
	QString uuid = QUuid::createUuid().toString();

	if(regExp.indexIn(uuid) >= 0)
	{
		return QString().append(regExp.cap(1)).append(regExp.cap(2)).append(regExp.cap(3)).append(regExp.cap(4)).append(regExp.cap(5));
	}

	throw "The RegExp didn't match on the UUID string. This shouldn't happen ;-)";
}

/*
 * Get LameXP temp folder
 */
const QString &lamexp_temp_folder2(void)
{
	static const char *TEMP_STR = "Temp";
	const QString WRITE_TEST_DATA = lamexp_rand_str();
	const QString SUB_FOLDER = lamexp_rand_str();

	//Already initialized?
	if(!g_lamexp_temp_folder.isEmpty())
	{
		if(QDir(g_lamexp_temp_folder).exists())
		{
			return g_lamexp_temp_folder;
		}
		else
		{
			g_lamexp_temp_folder.clear();
		}
	}
	
	//Try the %TMP% or %TEMP% directory first
	QDir temp = QDir::temp();
	if(temp.exists())
	{
		temp.mkdir(SUB_FOLDER);
		if(temp.cd(SUB_FOLDER) && temp.exists())
		{
			QFile testFile(QString("%1/~%2.tmp").arg(temp.canonicalPath(), lamexp_rand_str()));
			if(testFile.open(QIODevice::ReadWrite))
			{
				if(testFile.write(WRITE_TEST_DATA.toLatin1().constData()) >= strlen(WRITE_TEST_DATA.toLatin1().constData()))
				{
					g_lamexp_temp_folder = temp.canonicalPath();
				}
				testFile.remove();
			}
		}
		if(!g_lamexp_temp_folder.isEmpty())
		{
			return g_lamexp_temp_folder;
		}
	}

	//Create TEMP folder in %LOCALAPPDATA%
	QDir localAppData = QDir(lamexp_known_folder(lamexp_folder_localappdata));
	if(!localAppData.path().isEmpty())
	{
		if(!localAppData.exists())
		{
			localAppData.mkpath(".");
		}
		if(localAppData.exists())
		{
			if(!localAppData.entryList(QDir::AllDirs).contains(TEMP_STR, Qt::CaseInsensitive))
			{
				localAppData.mkdir(TEMP_STR);
			}
			if(localAppData.cd(TEMP_STR) && localAppData.exists())
			{
				localAppData.mkdir(SUB_FOLDER);
				if(localAppData.cd(SUB_FOLDER) && localAppData.exists())
				{
					QFile testFile(QString("%1/~%2.tmp").arg(localAppData.canonicalPath(), lamexp_rand_str()));
					if(testFile.open(QIODevice::ReadWrite))
					{
						if(testFile.write(WRITE_TEST_DATA.toLatin1().constData()) >= strlen(WRITE_TEST_DATA.toLatin1().constData()))
						{
							g_lamexp_temp_folder = localAppData.canonicalPath();
						}
						testFile.remove();
					}
				}
			}
		}
		if(!g_lamexp_temp_folder.isEmpty())
		{
			return g_lamexp_temp_folder;
		}
	}

	//Failed to create TEMP folder!
	qFatal("Temporary directory could not be initialized!\n\nFirst attempt:\n%s\n\nSecond attempt:\n%s", temp.canonicalPath().toUtf8().constData(), localAppData.canonicalPath().toUtf8().constData());
	return g_lamexp_temp_folder;
}

/*
 * Clean folder
 */
bool lamexp_clean_folder(const QString &folderPath)
{
	QDir tempFolder(folderPath);
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
	
	tempFolder.rmdir(".");
	return !tempFolder.exists();
}

/*
 * Register tool
 */
void lamexp_register_tool(const QString &toolName, LockedFile *file, unsigned int version)
{
	if(g_lamexp_tool_registry.contains(toolName.toLower()))
	{
		throw "lamexp_register_tool: Tool is already registered!";
	}

	g_lamexp_tool_registry.insert(toolName.toLower(), file);
	g_lamexp_tool_versions.insert(toolName.toLower(), version);
}

/*
 * Check for tool
 */
bool lamexp_check_tool(const QString &toolName)
{
	return g_lamexp_tool_registry.contains(toolName.toLower());
}

/*
 * Lookup tool path
 */
const QString lamexp_lookup_tool(const QString &toolName)
{
	if(g_lamexp_tool_registry.contains(toolName.toLower()))
	{
		return g_lamexp_tool_registry.value(toolName.toLower())->filePath();
	}
	else
	{
		return QString();
	}
}

/*
 * Lookup tool version
 */
unsigned int lamexp_tool_version(const QString &toolName)
{
	if(g_lamexp_tool_versions.contains(toolName.toLower()))
	{
		return g_lamexp_tool_versions.value(toolName.toLower());
	}
	else
	{
		return UINT_MAX;
	}
}

/*
 * Version number to human-readable string
 */
const QString lamexp_version2string(const QString &pattern, unsigned int version, const QString &defaultText)
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

	return result;
}

/*
 * Register a new translation
 */
bool lamexp_translation_register(const QString &langId, const QString &qmFile, const QString &langName, unsigned int &systemId, unsigned int &country)
{
	if(qmFile.isEmpty() || langName.isEmpty() || systemId < 1)
	{
		return false;
	}

	g_lamexp_translation.files.insert(langId, qmFile);
	g_lamexp_translation.names.insert(langId, langName);
	g_lamexp_translation.sysid.insert(langId, systemId);
	g_lamexp_translation.cntry.insert(langId, country);

	return true;
}

/*
 * Get list of all translations
 */
QStringList lamexp_query_translations(void)
{
	return g_lamexp_translation.files.keys();
}

/*
 * Get translation name
 */
QString lamexp_translation_name(const QString &langId)
{
	return g_lamexp_translation.names.value(langId.toLower(), QString());
}

/*
 * Get translation system id
 */
unsigned int lamexp_translation_sysid(const QString &langId)
{
	return g_lamexp_translation.sysid.value(langId.toLower(), 0);
}

/*
 * Get translation script id
 */
unsigned int lamexp_translation_country(const QString &langId)
{
	return g_lamexp_translation.cntry.value(langId.toLower(), 0);
}

/*
 * Install a new translator
 */
bool lamexp_install_translator(const QString &langId)
{
	bool success = false;

	if(langId.isEmpty() || langId.toLower().compare(LAMEXP_DEFAULT_LANGID) == 0)
	{
		success = lamexp_install_translator_from_file(QString());
	}
	else
	{
		QString qmFile = g_lamexp_translation.files.value(langId.toLower(), QString());
		if(!qmFile.isEmpty())
		{
			success = lamexp_install_translator_from_file(QString(":/localization/%1").arg(qmFile));
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
	bool success = false;

	if(!g_lamexp_currentTranslator)
	{
		g_lamexp_currentTranslator = new QTranslator();
	}

	if(!qmFile.isEmpty())
	{
		QString qmPath = QFileInfo(qmFile).canonicalFilePath();
		QApplication::removeTranslator(g_lamexp_currentTranslator);
		success = g_lamexp_currentTranslator->load(qmPath);
		QApplication::installTranslator(g_lamexp_currentTranslator);
		if(!success)
		{
			qWarning("Failed to load translation:\n\"%s\"", qmPath.toLatin1().constData());
		}
	}
	else
	{
		QApplication::removeTranslator(g_lamexp_currentTranslator);
		success = true;
	}

	return success;
}

/*
 * Locate known folder on local system
 */
QString lamexp_known_folder(lamexp_known_folder_t folder_id)
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

	static QLibrary *Kernel32Lib = NULL;
	static SHGetKnownFolderPathFun SHGetKnownFolderPathPtr = NULL;
	static SHGetFolderPathFun SHGetFolderPathPtr = NULL;

	if((!SHGetKnownFolderPathPtr) && (!SHGetFolderPathPtr))
	{
		if(!Kernel32Lib) Kernel32Lib = new QLibrary("shell32.dll");
		SHGetKnownFolderPathPtr = (SHGetKnownFolderPathFun) Kernel32Lib->resolve("SHGetKnownFolderPath");
		SHGetFolderPathPtr = (SHGetFolderPathFun) Kernel32Lib->resolve("SHGetFolderPathW");
	}

	int folderCSIDL = -1;
	GUID folderGUID = {0x0000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}};

	switch(folder_id)
	{
	case lamexp_folder_localappdata:
		folderCSIDL = CSIDL_LOCAL_APPDATA;
		folderGUID = GUID_LOCAL_APPDATA;
		break;
	case lamexp_folder_programfiles:
		folderCSIDL = CSIDL_PROGRAM_FILES;
		folderGUID = GUID_PROGRAM_FILES;
		break;
	case lamexp_folder_systemfolder:
		folderCSIDL = CSIDL_SYSTEM_FOLDER;
		folderGUID = GUID_SYSTEM_FOLDER;
		break;
	default:
		return QString();
		break;
	}

	QString folder;

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

	return folder;
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
	
	bool isAppThemed = false;
	QLibrary uxTheme(QString("%1/UxTheme.dll").arg(lamexp_known_folder(lamexp_folder_systemfolder)));
	IsAppThemedFun IsAppThemedPtr = (IsAppThemedFun) uxTheme.resolve("IsAppThemed");

	if(IsAppThemedPtr)
	{
		isAppThemed = IsAppThemedPtr();
		if(!isAppThemed)
		{
			qWarning("Theme support is disabled for this process!");
		}
	}

	return isAppThemed;
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
	
	newStr.replace("\\", "-");
	newStr.replace(" / ", ", ");
	newStr.replace("/", ",");
	newStr.replace(":", "-");
	newStr.replace("*", "x");
	newStr.replace("?", "");
	newStr.replace("<", "[");
	newStr.replace(">", "]");
	newStr.replace("|", "!");
	
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
 * Finalization function (final clean-up)
 */
void lamexp_finalization(void)
{
	qDebug("lamexp_finalization()");
	
	//Free all tools
	if(!g_lamexp_tool_registry.isEmpty())
	{
		QStringList keys = g_lamexp_tool_registry.keys();
		for(int i = 0; i < keys.count(); i++)
		{
			LAMEXP_DELETE(g_lamexp_tool_registry[keys.at(i)]);
		}
		g_lamexp_tool_registry.clear();
		g_lamexp_tool_versions.clear();
	}
	
	//Delete temporary files
	if(!g_lamexp_temp_folder.isEmpty())
	{
		for(int i = 0; i < 100; i++)
		{
			if(lamexp_clean_folder(g_lamexp_temp_folder))
			{
				break;
			}
			Sleep(125);
		}
		g_lamexp_temp_folder.clear();
	}

	//Clear languages
	if(g_lamexp_currentTranslator)
	{
		QApplication::removeTranslator(g_lamexp_currentTranslator);
		LAMEXP_DELETE(g_lamexp_currentTranslator);
	}
	g_lamexp_translation.files.clear();
	g_lamexp_translation.names.clear();

	//Destroy Qt application object
	QApplication *application = dynamic_cast<QApplication*>(QApplication::instance());
	LAMEXP_DELETE(application);

	//Detach from shared memory
	if(g_lamexp_ipc_ptr.sharedmem) g_lamexp_ipc_ptr.sharedmem->detach();
	LAMEXP_DELETE(g_lamexp_ipc_ptr.sharedmem);
	LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_read);
	LAMEXP_DELETE(g_lamexp_ipc_ptr.semaphore_write);

	//Close log file
	if(g_lamexp_log_file)
	{
		fclose(g_lamexp_log_file);
		g_lamexp_log_file = NULL;
	}
}

/*
 * Initialize debug thread
 */
static const HANDLE g_debug_thread = LAMEXP_DEBUG ? NULL : lamexp_debug_thread_init();

/*
 * Get number private bytes [debug only]
 */
SIZE_T lamexp_dbg_private_bytes(void)
{
#if LAMEXP_DEBUG
	PROCESS_MEMORY_COUNTERS_EX memoryCounters;
	memoryCounters.cb = sizeof(PROCESS_MEMORY_COUNTERS_EX);
	GetProcessMemoryInfo(GetCurrentProcess(), (PPROCESS_MEMORY_COUNTERS) &memoryCounters, sizeof(PROCESS_MEMORY_COUNTERS_EX));
	return memoryCounters.PrivateUsage;
#else
	throw "Cannot call this function in a non-debug build!";
#endif //LAMEXP_DEBUG
}
