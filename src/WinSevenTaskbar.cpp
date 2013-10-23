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

#include "WinSevenTaskbar.h"

#include <QWidget>
#include <QIcon>

//Windows includes
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShObjIdl.h>

static UINT s_winMsg = 0;
static ITaskbarList3 *s_ptbl = NULL;

WinSevenTaskbar::WinSevenTaskbar(void)
{
	THROW("Cannot create instance of this class!");
}

WinSevenTaskbar::~WinSevenTaskbar(void)
{
}

////////////////////////////////////////////////////////////

#ifdef __ITaskbarList3_INTERFACE_DEFINED__	

void WinSevenTaskbar::init(void)
{
	s_winMsg = RegisterWindowMessageW(L"TaskbarButtonCreated");
	s_ptbl = NULL;
}
	
void WinSevenTaskbar::uninit(void)
{
	if(s_ptbl)
	{
		s_ptbl->Release();
		s_ptbl = NULL;
	}
}

bool WinSevenTaskbar::handleWinEvent(void *message, long *result)
{
	bool stopEvent = false;

	if(((MSG*)message)->message == s_winMsg)
	{
		if(!s_ptbl) createInterface();
		*result = (s_ptbl) ? S_OK : S_FALSE;
		stopEvent = true;
	}

	return stopEvent;
}

void WinSevenTaskbar::createInterface(void)
{
	if(!s_ptbl)
	{
		ITaskbarList3 *ptbl = NULL;
		HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&ptbl));

		if (SUCCEEDED(hr))
		{
			HRESULT hr2 = ptbl->HrInit();
			if(SUCCEEDED(hr2))
			{
				s_ptbl = ptbl;
				/*qDebug("ITaskbarList3::HrInit() succeeded.");*/
			}
			else
			{
				ptbl->Release();
				qWarning("ITaskbarList3::HrInit() has failed.");
			}
		}
		else
		{
			qWarning("ITaskbarList3 could not be created.");
		}
	}
}

bool WinSevenTaskbar::setTaskbarState(QWidget *window, WinSevenTaskbarState state)
{
	bool result = false;
	
	if(s_ptbl && window)
	{
		HRESULT hr = HRESULT(-1);

		switch(state)
		{
		case WinSevenTaskbarNoState:
			hr = s_ptbl->SetProgressState(reinterpret_cast<HWND>(window->winId()), TBPF_NOPROGRESS);
			break;
		case WinSevenTaskbarNormalState:
			hr = s_ptbl->SetProgressState(reinterpret_cast<HWND>(window->winId()), TBPF_NORMAL);
			break;
		case WinSevenTaskbarIndeterminateState:
			hr = s_ptbl->SetProgressState(reinterpret_cast<HWND>(window->winId()), TBPF_INDETERMINATE);
			break;
		case WinSevenTaskbarErrorState:
			hr = s_ptbl->SetProgressState(reinterpret_cast<HWND>(window->winId()), TBPF_ERROR);
			break;
		case WinSevenTaskbarPausedState:
			hr = s_ptbl->SetProgressState(reinterpret_cast<HWND>(window->winId()), TBPF_PAUSED);
			break;
		}

		result = SUCCEEDED(hr);
	}

	return result;
}

void WinSevenTaskbar::setTaskbarProgress(QWidget *window, unsigned __int64 currentValue, unsigned __int64 maximumValue)
{
	if(s_ptbl && window)
	{
		s_ptbl->SetProgressValue(reinterpret_cast<HWND>(window->winId()), currentValue, maximumValue);
	}
}

void WinSevenTaskbar::setOverlayIcon(QWidget *window, QIcon *icon)
{
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	if(s_ptbl && window)
	{
		s_ptbl->SetOverlayIcon(window->winId(), (icon ? icon->pixmap(16,16).toWinHICON() : NULL), L"LameXP");
	}
#endif
}

#else //__ITaskbarList3_INTERFACE_DEFINED__

LAMEXP_COMPILER_WARNING("ITaskbarList3 not defined. Compiling *without* support for Win7 taskbar!")
void WinSevenTaskbar::init(void) {}
void WinSevenTaskbar::uninit(void) {}
bool WinSevenTaskbar::handleWinEvent(MSG *message, long *result) { return false; }
void WinSevenTaskbar::createInterface(void) {}
bool WinSevenTaskbar::setTaskbarState(QWidget *window, WinSevenTaskbarState state) { return false; }
void WinSevenTaskbar::setTaskbarProgress(QWidget *window, unsigned __int64 currentValue, unsigned __int64 maximumValue) {}
void WinSevenTaskbar::setOverlayIcon(QWidget *window, QIcon *icon) {}

#endif //__ITaskbarList3_INTERFACE_DEFINED__