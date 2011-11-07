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

#include "WinSevenTaskbar.h"

#include <QWidget>
#include <QIcon>
#include <ShObjIdl.h>

UINT WinSevenTaskbar::m_winMsg = 0;
ITaskbarList3 *WinSevenTaskbar::m_ptbl = NULL;

WinSevenTaskbar::WinSevenTaskbar(void)
{
	throw "Cannot create instance of this class!";
}

WinSevenTaskbar::~WinSevenTaskbar(void)
{
}

////////////////////////////////////////////////////////////

#ifdef __ITaskbarList3_INTERFACE_DEFINED__	

void WinSevenTaskbar::init(void)
{
	m_winMsg = RegisterWindowMessageW(L"TaskbarButtonCreated");
	m_ptbl = NULL;
}
	
void WinSevenTaskbar::uninit(void)
{
	if(m_ptbl)
	{
		m_ptbl->Release();
		m_ptbl = NULL;
	}
}

bool WinSevenTaskbar::handleWinEvent(MSG *message, long *result)
{
	bool stopEvent = false;

	if(message->message == m_winMsg)
	{
		if(!m_ptbl) createInterface();
		*result = (m_ptbl) ? S_OK : S_FALSE;
		stopEvent = true;
	}

	return stopEvent;
}

void WinSevenTaskbar::createInterface(void)
{
	if(!m_ptbl)
	{
		ITaskbarList3 *ptbl = NULL;
		HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&ptbl));

		if (SUCCEEDED(hr))
		{
			HRESULT hr2 = ptbl->HrInit();
			if(SUCCEEDED(hr2))
			{
				m_ptbl = ptbl;
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
	
	if(m_ptbl && window)
	{
		HRESULT hr = HRESULT(-1);

		switch(state)
		{
		case WinSevenTaskbarNoState:
			hr = m_ptbl->SetProgressState(window->winId(), TBPF_NOPROGRESS);
			break;
		case WinSevenTaskbarNormalState:
			hr = m_ptbl->SetProgressState(window->winId(), TBPF_NORMAL);
			break;
		case WinSevenTaskbarIndeterminateState:
			hr = m_ptbl->SetProgressState(window->winId(), TBPF_INDETERMINATE);
			break;
		case WinSevenTaskbarErrorState:
			hr = m_ptbl->SetProgressState(window->winId(), TBPF_ERROR);
			break;
		case WinSevenTaskbarPausedState:
			hr = m_ptbl->SetProgressState(window->winId(), TBPF_PAUSED);
			break;
		}

		result = SUCCEEDED(hr);
	}

	return result;
}

void WinSevenTaskbar::setTaskbarProgress(QWidget *window, unsigned __int64 currentValue, unsigned __int64 maximumValue)
{
	if(m_ptbl && window)
	{
		m_ptbl->SetProgressValue(window->winId(), currentValue, maximumValue);
	}
}

void WinSevenTaskbar::setOverlayIcon(QWidget *window, QIcon *icon)
{
	if(m_ptbl && window)
	{
		m_ptbl->SetOverlayIcon(window->winId(), (icon ? icon->pixmap(16,16).toWinHICON() : NULL), L"LameXP");
	}
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