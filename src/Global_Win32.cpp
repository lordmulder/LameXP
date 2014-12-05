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

//Main thread ID
static const DWORD g_main_thread_id = GetCurrentThreadId();

//Localization
const char* LAMEXP_DEFAULT_LANGID = "en";
const char* LAMEXP_DEFAULT_TRANSLATION = "LameXP_EN.qm";

///////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////
// INITIALIZATION
///////////////////////////////////////////////////////////////////////////////

extern "C" void _lamexp_global_init_win32(void)
{
	//Zero *before* constructors are called
	MUTILS_ZERO_MEMORY(g_lamexp_wine);
	MUTILS_ZERO_MEMORY(g_lamexp_themes_enabled);
	MUTILS_ZERO_MEMORY(g_lamexp_dwmapi);
	MUTILS_ZERO_MEMORY(g_lamexp_sounds);
}

///////////////////////////////////////////////////////////////////////////////
// FINALIZATION
///////////////////////////////////////////////////////////////////////////////

extern "C" void _lamexp_global_free_win32(void)
{
	//Release DWM API
	g_lamexp_dwmapi.dwmIsCompositionEnabled = NULL;
	g_lamexp_dwmapi.dwmExtendFrameIntoClientArea = NULL;
	g_lamexp_dwmapi.dwmEnableBlurBehindWindow = NULL;
	MUTILS_DELETE(g_lamexp_dwmapi.dwmapi_dll);

	//Clear sound cache
	MUTILS_DELETE(g_lamexp_sounds.sound_db);
}
