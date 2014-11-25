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
#include <QPlastiqueStyle>
#include <QProcess>
#include <QReadWriteLock>
#include <QRegExp>
#include <QSysInfo>
#include <QTextCodec>
#include <QTimer>
#include <QTranslator>
#include <QUuid>
#include <QResource>

//LameXP includes
#define LAMEXP_INC_CONFIG
#include "Resource.h"
#include "Config.h"

//MUtils
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>
#include <MUtils/Terminal.h>

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

///////////////////////////////////////////////////////////////////////////////
// GLOBAL VARS
///////////////////////////////////////////////////////////////////////////////

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
	HRESULT (__stdcall *dwmIsCompositionEnabled)(BOOL *bEnabled);
	HRESULT (__stdcall *dwmExtendFrameIntoClientArea)(HWND hWnd, const MARGINS* pMarInset);
	HRESULT (__stdcall *dwmEnableBlurBehindWindow)(HWND hWnd, const DWM_BLURBEHIND* pBlurBehind);
	QLibrary *dwmapi_dll;
	QReadWriteLock lock;
}
g_lamexp_dwmapi;

//Sound file cache
static struct
{
	QHash<const QString, const unsigned char*> *sound_db;
	QReadWriteLock lock;
}
g_lamexp_sounds;

//Image formats
static const char *g_lamexp_imageformats[] = {"bmp", "png", "jpg", "gif", "ico", "xpm", NULL}; //"svg"

//Main thread ID
static const DWORD g_main_thread_id = GetCurrentThreadId();

//Localization
const char* LAMEXP_DEFAULT_LANGID = "en";
const char* LAMEXP_DEFAULT_TRANSLATION = "LameXP_EN.qm";

///////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

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
			if(ntdll.resolve("wine_get_version")          != NULL) g_lamexp_wine.bIsWine = true;
			ntdll.unload();
		}
		g_lamexp_wine.bInitialized = true;
	}

	return g_lamexp_wine.bIsWine;
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

	MUtils::Terminal::write(type, msg);

	if((type == QtCriticalMsg) || (type == QtFatalMsg))
	{
		MUtils::OS::fatal_exit(MUTILS_WCHR(QString::fromUtf8(msg)));
	}
}

/*
 * Invalid parameters handler
 */
static void lamexp_invalid_param_handler(const wchar_t* exp, const wchar_t* fun, const wchar_t* fil, unsigned int, uintptr_t)
{
	MUtils::OS::fatal_exit(L"Invalid parameter handler invoked, application will exit!");
}

/*
 * Signal handler
 */
static void lamexp_signal_handler(int signal_num)
{
	signal(signal_num, lamexp_signal_handler);
	MUtils::OS::fatal_exit(L"Signal handler invoked, application will exit!");
}

/*
 * Global exception handler
 */
static LONG WINAPI lamexp_exception_handler(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	MUtils::OS::fatal_exit(L"Unhandeled exception handler invoked, application will exit!");
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

	BOOL bHaveDebugger = FALSE;
	if(CheckRemoteDebuggerPresent(GetCurrentProcess(), &bHaveDebugger))
	{
		if(bHaveDebugger) return true;
	}

	return IsDebuggerPresent();
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
			MUtils::OS::fatal_exit(L"Not a debug build. Please unload debugger and try again!");
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
		MUtils::OS::fatal_exit(L"Not a debug build. Please unload debugger and try again!");
	}
	const uintptr_t h = _beginthreadex(NULL, 0, lamexp_debug_thread_proc, NULL, 0, NULL);
	return (HANDLE)(h^0xdeadbeef);
}

/*
 * Qt event filter
 */
static bool lamexp_event_filter(void *message, long *result)
{ 
	if((!(MUTILS_DEBUG)) && lamexp_check_for_debugger())
	{
		MUtils::OS::fatal_exit(L"Not a debug build. Please unload debugger and try again!");
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
static bool lamexp_process_is_elevated(bool *bIsUacEnabled = NULL)
{
	bool bIsProcessElevated = false;
	if(bIsUacEnabled) *bIsUacEnabled = false;
	HANDLE hToken = NULL;
	
	if(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
	{
		TOKEN_ELEVATION_TYPE tokenElevationType;
		DWORD returnLength;
		if(GetTokenInformation(hToken, TokenElevationType, &tokenElevationType, sizeof(TOKEN_ELEVATION_TYPE), &returnLength))
		{
			if(returnLength == sizeof(TOKEN_ELEVATION_TYPE))
			{
				switch(tokenElevationType)
				{
				case TokenElevationTypeDefault:
					qDebug("Process token elevation type: Default -> UAC is disabled.\n");
					break;
				case TokenElevationTypeFull:
					qWarning("Process token elevation type: Full -> potential security risk!\n");
					bIsProcessElevated = true;
					if(bIsUacEnabled) *bIsUacEnabled = true;
					break;
				case TokenElevationTypeLimited:
					qDebug("Process token elevation type: Limited -> not elevated.\n");
					if(bIsUacEnabled) *bIsUacEnabled = true;
					break;
				default:
					qWarning("Unknown tokenElevationType value: %d", tokenElevationType);
					break;
				}
			}
			else
			{
				qWarning("GetTokenInformation() return an unexpected size!");
			}
		}
		CloseHandle(hToken);
	}
	else
	{
		qWarning("Failed to open process token!");
	}

	return bIsProcessElevated;
}

/*
 * Convert QIcon to HICON -> caller is responsible for destroying the HICON!
 */
static HICON lamexp_qicon2hicon(const QIcon &icon, const int w, const int h)
{
	if(!icon.isNull())
	{
		QPixmap pixmap = icon.pixmap(w, h);
		if(!pixmap.isNull())
		{
			return pixmap.toWinHICON();
		}
	}
	return NULL;
}

/*
 * Initialize Qt framework
 */
bool lamexp_init_qt(int argc, char* argv[])
{
	static bool qt_initialized = false;
	typedef BOOL (WINAPI *SetDllDirectoryProc)(WCHAR *lpPathName);
	const QStringList &arguments = MUtils::OS::arguments();

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
	
	const MUtils::OS::Version::os_version_t &osVersion = MUtils::OS::os_version();
	if((osVersion.type != MUtils::OS::Version::OS_WINDOWS) || (osVersion < MUtils::OS::Version::WINDOWS_WINXP))
	{
		qFatal("%s", QApplication::tr("Executable '%1' requires Windows XP or later.").arg(executableName).toLatin1().constData());
	}

	//Check whether we are running on a supported Windows version
	if(const char *const friendlyName = MUtils::OS::os_friendly_name(osVersion))
	{
		qDebug("Running on %s (NT v%u.%u).\n", friendlyName, osVersion.versionMajor, osVersion.versionMinor);
	}
	else
	{
		const QString message = QString().sprintf("Running on an unknown WindowsNT-based system (v%u.%u).", osVersion.versionMajor, osVersion.versionMinor);
		qWarning("%s\n", MUTILS_UTF8(message));
		MUtils::OS::system_message_wrn(MUTILS_WCHR(message), L"LameXP");
	}

	//Check for compat mode
	if(osVersion.overrideFlag && (osVersion <= MUtils::OS::Version::WINDOWS_WN100))
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
	qDebug("Library Path:\n%s\n", MUTILS_UTF8(QApplication::libraryPaths().first()));

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
	
	//Enable larger/smaller font size
	double fontScaleFactor = 1.0;
	if(arguments.contains("--huge-font",  Qt::CaseInsensitive)) fontScaleFactor = 1.500;
	if(arguments.contains("--big-font",   Qt::CaseInsensitive)) fontScaleFactor = 1.250;
	if(arguments.contains("--small-font", Qt::CaseInsensitive)) fontScaleFactor = 0.875;
	if(arguments.contains("--tiny-font",  Qt::CaseInsensitive)) fontScaleFactor = 0.750;
	if(!qFuzzyCompare(fontScaleFactor, 1.0))
	{
		qWarning("Application font scale factor set to: %.3f\n", fontScaleFactor);
		QFont appFont = application->font();
		appFont.setPointSizeF(appFont.pointSizeF() * fontScaleFactor);
		application->setFont(appFont);
	}

	//Add the default translations
	lamexp_translation_init();

	//Check for process elevation
	if(lamexp_process_is_elevated() && (!lamexp_detect_wine()))
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
#if 0 //FIXME !!!! FIXME !!!! FIXME !!!! FIXME !!!! FIXME !!!! FIXME !!!! FIXME !!!! FIXME !!!!
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	if(g_lamexp_console_attached && (!lamexp_detect_wine()))
	{
		QLibrary kernel32("kernel32.dll");
		if(kernel32.load())
		{
			typedef DWORD (__stdcall *SetConsoleIconFun)(HICON);
			if(SetConsoleIconFun SetConsoleIconPtr = (SetConsoleIconFun) kernel32.resolve("SetConsoleIcon"))
			{
				if(HICON hIcon = lamexp_qicon2hicon(QIcon(":/icons/sound.png"), 16, 16))
				{
					SetConsoleIconPtr(hIcon);
					DestroyIcon(hIcon);
				}
			}
		}
	}
#endif
#endif

	//Done
	qt_initialized = true;
	return true;
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
		const MUtils::OS::Version::os_version_t &osVersion = MUtils::OS::os_version();
		if(osVersion >= MUtils::OS::Version::WINDOWS_WINXP)
		{
			IsAppThemedFun IsAppThemedPtr = NULL;
			QLibrary uxTheme(QString("%1/UxTheme.dll").arg(MUtils::OS::known_folder(MUtils::OS::FOLDER_SYSTEMFOLDER)));
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
				return InitiateSystemShutdownEx(NULL, const_cast<wchar_t*>(MUTILS_WCHR(message)), timeout, forceShutdown ? TRUE : FALSE, FALSE, reason);
			}
		}
	}
	
	return false;
}

/*
 * Block window "move" message
 */
bool lamexp_block_window_move(void *message)
{
	if(message)
	{
		MSG *msg = reinterpret_cast<MSG*>(message);
		if((msg->message == WM_SYSCOMMAND) && (msg->wParam == SC_MOVE))
		{
			return true;
		}
		if((msg->message == WM_NCLBUTTONDOWN) && (msg->wParam == HTCAPTION))
		{
			return true;
		}
	}
	return false;
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
bool lamexp_play_sound(const QString &name, const bool bAsync)
{
	const unsigned char *data = NULL;
	
	//Try to look-up the sound in the cache first
	if(!name.isEmpty())
	{
		QReadLocker readLock(&g_lamexp_sounds.lock);
		if(g_lamexp_sounds.sound_db && g_lamexp_sounds.sound_db->contains(name))
		{
			data = g_lamexp_sounds.sound_db->value(name);
		}
	}
	
	//If data not found in cache, try to load from resource!
	if((!data) && (!name.isEmpty()))
	{
		QResource resource(QString(":/sounds/%1.wav").arg(name));
		if(resource.isValid() && (data = resource.data()))
		{
			QWriteLocker writeLock(&g_lamexp_sounds.lock);
			if(!g_lamexp_sounds.sound_db)
			{
				g_lamexp_sounds.sound_db = new QHash<const QString, const unsigned char*>();
			}
			g_lamexp_sounds.sound_db->insert(name, data);
		}
		else
		{
			qWarning("Sound effect \"%s\" not found!", MUTILS_UTF8(name));
		}
	}

	//Play the sound, if availbale
	if(data)
	{
		return PlaySound(LPCWSTR(data), NULL, (SND_MEMORY | (bAsync ? SND_ASYNC : SND_SYNC))) != FALSE;
	}

	return false;
}

/*
 * Play a sound (system alias)
 */
bool lamexp_play_sound_alias(const QString &alias, const bool bAsync)
{
	return PlaySound(MUTILS_WCHR(alias), GetModuleHandle(NULL), (SND_ALIAS | (bAsync ? SND_ASYNC : SND_SYNC))) != FALSE;
}

/*
 * Play a sound (from external DLL)
 */
bool lamexp_play_sound_file(const QString &library, const unsigned short uiSoundIdx, const bool bAsync)
{
	bool result = false;

	QFileInfo libraryFile(library);
	if(!libraryFile.isAbsolute())
	{
		const QString &systemDir = MUtils::OS::known_folder(MUtils::OS::FOLDER_SYSTEMFOLDER);
		if(!systemDir.isEmpty())
		{
			libraryFile.setFile(QDir(systemDir), libraryFile.fileName());
		}
	}

	if(libraryFile.exists() && libraryFile.isFile())
	{
		if(HMODULE module = LoadLibraryW(MUTILS_WCHR(QDir::toNativeSeparators(libraryFile.canonicalFilePath()))))
		{
			result = (PlaySound(MAKEINTRESOURCE(uiSoundIdx), module, (SND_RESOURCE | (bAsync ? SND_ASYNC : SND_SYNC))) != FALSE);
			FreeLibrary(module);
		}
	}
	else
	{
		qWarning("PlaySound: File \"%s\" could not be found!", MUTILS_UTF8(libraryFile.absoluteFilePath()));
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
	return ((int) ShellExecuteW(((win) ? win->winId() : NULL), (explore ? L"explore" : L"open"), MUTILS_WCHR(url), ((!parameters.isEmpty()) ? MUTILS_WCHR(parameters) : NULL), ((!directory.isEmpty()) ? MUTILS_WCHR(directory) : NULL), SW_SHOW)) > 32;
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
		ok = (AppendMenuW(hMenu, MF_STRING, identifier, MUTILS_WCHR(text)) == TRUE);
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
		ok = (ModifyMenu(hMenu, identifier, MF_STRING | MF_BYCOMMAND, identifier, MUTILS_WCHR(text)) == TRUE);
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
bool lamexp_bring_to_front(const QWidget *window)
{
	bool ret = false;
	
	if(window)
	{
		for(int i = 0; (i < 5) && (!ret); i++)
		{
			ret = (SetForegroundWindow(window->winId()) != FALSE);
			SwitchToThisWindow(window->winId(), TRUE);
		}
		LockSetForegroundWindow(LSFW_LOCK);
	}

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
 * Check the network connectivity status
 */
int lamexp_network_status(void)
{
	DWORD dwFlags;
	const BOOL ret = IsNetworkAlive(&dwFlags);
	if(GetLastError() == 0)
	{
		return (ret != FALSE) ? lamexp_network_yes : lamexp_network_non;
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

			const QString currentKey = MUTILS_QSTR(registryPrefix[j]).append(MUTILS_QSTR(registryKeys[i]));
			if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, MUTILS_WCHR(currentKey), 0, KEY_READ, &registryKeyHandle) == ERROR_SUCCESS)
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
							mplayerPath = MUTILS_QSTR(Buffer);
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
						if(mplayerDir.exists(MUTILS_QSTR(appNames[k])))
						{
							qDebug("Player found at:\n%s\n", MUTILS_UTF8(mplayerDir.absoluteFilePath(MUTILS_QSTR(appNames[k]))));
							QProcess::startDetached(mplayerDir.absoluteFilePath(MUTILS_QSTR(appNames[k])), QStringList() << QDir::toNativeSeparators(mediaFilePath));
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
	
	//Reset function pointers
	g_lamexp_dwmapi.dwmIsCompositionEnabled = NULL;
	g_lamexp_dwmapi.dwmExtendFrameIntoClientArea = NULL;
	g_lamexp_dwmapi.dwmEnableBlurBehindWindow = NULL;
			
	//Does OS support DWM?
	const MUtils::OS::Version::os_version_t &osVersion = MUtils::OS::os_version();
	if(osVersion >= MUtils::OS::Version::WINDOWS_VISTA)
	{
		//Load DWMAPI.DLL
		g_lamexp_dwmapi.dwmapi_dll = new QLibrary("dwmapi.dll");
		if(g_lamexp_dwmapi.dwmapi_dll->load())
		{
			//Initialize function pointers
			g_lamexp_dwmapi.dwmIsCompositionEnabled      = (HRESULT (__stdcall*)(BOOL*))                       g_lamexp_dwmapi.dwmapi_dll->resolve("DwmIsCompositionEnabled");
			g_lamexp_dwmapi.dwmExtendFrameIntoClientArea = (HRESULT (__stdcall*)(HWND, const MARGINS*))        g_lamexp_dwmapi.dwmapi_dll->resolve("DwmExtendFrameIntoClientArea");
			g_lamexp_dwmapi.dwmEnableBlurBehindWindow    = (HRESULT (__stdcall*)(HWND, const DWM_BLURBEHIND*)) g_lamexp_dwmapi.dwmapi_dll->resolve("DwmEnableBlurBehindWindow");
		}
		else
		{
			MUTILS_DELETE(g_lamexp_dwmapi.dwmapi_dll);
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

	BOOL bCompositionEnabled = FALSE;

	//Required functions available?
	if((g_lamexp_dwmapi.dwmIsCompositionEnabled != NULL) && (g_lamexp_dwmapi.dwmExtendFrameIntoClientArea != NULL) && (g_lamexp_dwmapi.dwmEnableBlurBehindWindow != NULL))
	{
		//Check if composition is currently enabled
		if(HRESULT hr = g_lamexp_dwmapi.dwmIsCompositionEnabled(&bCompositionEnabled))
		{
			qWarning("DwmIsCompositionEnabled function has failed! (error %d)", hr);
			return false;
		}
	}
	
	//All functions available *and* composition enabled?
	if(!bCompositionEnabled)
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
	window->setAutoFillBackground(false);
	window->setAttribute(Qt::WA_TranslucentBackground);
	window->setAttribute(Qt::WA_NoSystemBackground);

	return true;
}

/*
 * Update "sheet of glass" effect on the given Window
 */
bool lamexp_sheet_of_glass_update(QWidget *window)
{
	QReadLocker readLock(&g_lamexp_dwmapi.lock);

	//Initialize the DWM API
	while(!g_lamexp_dwmapi.bInitialized)
	{
		readLock.unlock();
		lamexp_init_dwmapi();
		readLock.relock();
	}

	BOOL bCompositionEnabled = FALSE;

	//Required functions available?
	if((g_lamexp_dwmapi.dwmIsCompositionEnabled != NULL) && (g_lamexp_dwmapi.dwmEnableBlurBehindWindow != NULL))
	{
		//Check if composition is currently enabled
		if(HRESULT hr = g_lamexp_dwmapi.dwmIsCompositionEnabled(&bCompositionEnabled))
		{
			qWarning("DwmIsCompositionEnabled function has failed! (error %d)", hr);
			return false;
		}
	}
	
	//All functions available *and* composition enabled?
	if(!bCompositionEnabled)
	{
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

	return true;
}

/*
 * Update the window icon
 */
lamexp_icon_t *lamexp_set_window_icon(QWidget *window, const QIcon &icon, const bool bIsBigIcon)
{
	if(!icon.isNull())
	{
		const int extend = (bIsBigIcon ? 32 : 16);
		if(HICON hIcon = lamexp_qicon2hicon(icon, extend, extend))
		{
			SendMessage(window->winId(), WM_SETICON, (bIsBigIcon ? ICON_BIG : ICON_SMALL), LPARAM(hIcon));
			return reinterpret_cast<lamexp_icon_t*>(hIcon);
		}
	}
	return NULL;
}

/*
 * Free window icon
 */
void lamexp_free_window_icon(lamexp_icon_t *icon)
{
	if(HICON hIcon = reinterpret_cast<HICON>(icon))
	{
		DestroyIcon(hIcon);
	}
}

/*
 * Get system color info
 */
QColor lamexp_system_color(const int color_id)
{
	int nIndex = -1;

	switch(color_id)
	{
	case lamexp_syscolor_text:
		nIndex = COLOR_WINDOWTEXT;       /*Text in windows*/
		break;
	case lamexp_syscolor_background:
		nIndex = COLOR_WINDOW;           /*Window background*/
		break;
	case lamexp_syscolor_caption:
		nIndex = COLOR_CAPTIONTEXT;      /*Text in caption, size box, and scroll bar arrow box*/
		break;
	default:
		qWarning("Unknown system color id (%d) specified!", color_id);
		nIndex = COLOR_WINDOWTEXT;
	}
	
	const DWORD rgb = GetSysColor(nIndex);
	QColor color(GetRValue(rgb), GetGValue(rgb), GetBValue(rgb));
	return color;
}

/*
 * Check if the current user is an administartor (helper function)
 */
static bool lamexp_user_is_admin_helper(void)
{
	HANDLE hToken = NULL;
	if(!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
	{
		return false;
	}

	DWORD dwSize = 0;
	if(!GetTokenInformation(hToken, TokenGroups, NULL, 0, &dwSize))
	{
		if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			CloseHandle(hToken);
			return false;
		}
	}

	PTOKEN_GROUPS lpGroups = (PTOKEN_GROUPS) malloc(dwSize);
	if(!lpGroups)
	{
		CloseHandle(hToken);
		return false;
	}

	if(!GetTokenInformation(hToken, TokenGroups, lpGroups, dwSize, &dwSize))
	{
		free(lpGroups);
		CloseHandle(hToken);
		return false;
	}

	PSID lpSid = NULL; SID_IDENTIFIER_AUTHORITY Authority = {SECURITY_NT_AUTHORITY};
	if(!AllocateAndInitializeSid(&Authority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &lpSid))
	{
		free(lpGroups);
		CloseHandle(hToken);
		return false;
	}

	bool bResult = false;
	for(DWORD i = 0; i < lpGroups->GroupCount; i++)
	{
		if(EqualSid(lpSid, lpGroups->Groups[i].Sid))
		{
			bResult = true;
			break;
		}
	}

	FreeSid(lpSid);
	free(lpGroups);
	CloseHandle(hToken);
	return bResult;
}

/*
 * Check if the current user is an administartor
 */
bool lamexp_user_is_admin(void)
{
	bool isAdmin = false;

	//Check for process elevation and UAC support first!
	if(lamexp_process_is_elevated(&isAdmin))
	{
		qWarning("Process is elevated -> user is admin!");
		return true;
	}
	
	//If not elevated and UAC is not available -> user must be in admin group!
	if(!isAdmin)
	{
		qDebug("UAC is disabled/unavailable -> checking for Administrators group");
		isAdmin = lamexp_user_is_admin_helper();
	}

	return isAdmin;
}

/*
 * Check if file is a valid Win32/Win64 executable
 */
bool lamexp_is_executable(const QString &path)
{
	bool bIsExecutable = false;
	DWORD binaryType;
	if(GetBinaryType(MUTILS_WCHR(QDir::toNativeSeparators(path)), &binaryType))
	{
		bIsExecutable = (binaryType == SCS_32BIT_BINARY || binaryType == SCS_64BIT_BINARY);
	}
	return bIsExecutable;
}

/*
 * Fatal application exit - helper
 */
static DWORD WINAPI lamexp_fatal_exit_helper(LPVOID lpParameter)
{
	MessageBoxA(NULL, ((LPCSTR) lpParameter), "LameXP - Guru Meditation", MB_OK | MB_ICONERROR | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND);
	return 0;
}

/*
 * Initialize debug thread
 */
static const HANDLE g_debug_thread1 = MUTILS_DEBUG ? NULL : lamexp_debug_thread_init();

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
	if((!MUTILS_DEBUG) && lamexp_check_for_debugger())
	{
		MUtils::OS::fatal_exit(L"Not a debug build. Please unload debugger and try again!");
	}

	//Zero *before* constructors are called
	LAMEXP_ZERO_MEMORY(g_lamexp_wine);
	LAMEXP_ZERO_MEMORY(g_lamexp_themes_enabled);
	LAMEXP_ZERO_MEMORY(g_lamexp_dwmapi);
	LAMEXP_ZERO_MEMORY(g_lamexp_sounds);
}

///////////////////////////////////////////////////////////////////////////////
// FINALIZATION
///////////////////////////////////////////////////////////////////////////////

extern "C" void _lamexp_global_free_win32(void)
{
	//Destroy Qt application object
	QApplication *application = dynamic_cast<QApplication*>(QApplication::instance());
	MUTILS_DELETE(application);

	//Release DWM API
	g_lamexp_dwmapi.dwmIsCompositionEnabled = NULL;
	g_lamexp_dwmapi.dwmExtendFrameIntoClientArea = NULL;
	g_lamexp_dwmapi.dwmEnableBlurBehindWindow = NULL;
	MUTILS_DELETE(g_lamexp_dwmapi.dwmapi_dll);

	//Clear sound cache
	MUTILS_DELETE(g_lamexp_sounds.sound_db);
}
