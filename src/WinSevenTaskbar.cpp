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

#ifdef __ITaskbarList3_INTERFACE_DEFINED__

ITaskbarList3 *WinSevenTaskbar::m_ptbl = NULL;

WinSevenTaskbar::WinSevenTaskbar(void)
{
}

WinSevenTaskbar::~WinSevenTaskbar(void)
{
}

void WinSevenTaskbar::initTaskbar(void)
{
	OSVERSIONINFOW version;
	memset(&version, 0, sizeof(OSVERSIONINFOW));
	version.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
	GetVersionEx(&version);
	
	if(version.dwMajorVersion >= 6 && version.dwMinorVersion >= 1)
	{
		if(!m_ptbl)
		{
			ITaskbarList3 *ptbl;
			HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&ptbl));

			if (SUCCEEDED(hr))
			{
				HRESULT hr2 = ptbl->HrInit();
				if(SUCCEEDED(hr2))
				{
					m_ptbl = ptbl;
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
	else
	{
		qWarning("This OS doesn't support the ITaskbarList3 interface (needs NT 6.1 or later)");
	}
}

void WinSevenTaskbar::setTaskbarState(QWidget *window, WinSevenTaskbarState state)
{
	if(m_ptbl && window)
	{
		switch(state)
		{
		case WinSevenTaskbarNoState:
			m_ptbl->SetProgressState(window->winId(), TBPF_NOPROGRESS);
			break;
		case WinSevenTaskbarNormalState:
			m_ptbl->SetProgressState(window->winId(), TBPF_NORMAL);
			break;
		case WinSevenTaskbarIndeterminateState:
			m_ptbl->SetProgressState(window->winId(), TBPF_INDETERMINATE);
			break;
		case WinSevenTaskbarErrorState:
			m_ptbl->SetProgressState(window->winId(), TBPF_ERROR);
			break;
		case WinSevenTaskbarPausedState:
			m_ptbl->SetProgressState(window->winId(), TBPF_PAUSED);
			break;
		}
	}
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

#endif //__ITaskbarList3_INTERFACE_DEFINED__
