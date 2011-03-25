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

#include "Global.h"
#include <ShObjIdl.h>

class QWidget;
class QIcon;

#ifdef __ITaskbarList3_INTERFACE_DEFINED__

class WinSevenTaskbar
{
public:
	WinSevenTaskbar(void);
	~WinSevenTaskbar(void);

	//Taskbar states
	enum WinSevenTaskbarState
	{
		WinSevenTaskbarNoState = 0,
		WinSevenTaskbarNormalState = 1,
		WinSevenTaskbarIndeterminateState = 2,
		WinSevenTaskbarPausedState = 3,
		WinSevenTaskbarErrorState = 4
	};
	
	//Public interface
	static void initTaskbar(void);
	static void setTaskbarState(QWidget *window, WinSevenTaskbarState state);
	static void setTaskbarProgress(QWidget *window, unsigned __int64 currentValue, unsigned __int64 maximumValue);
	static void setOverlayIcon(QWidget *window, QIcon *icon);

private:
	static ITaskbarList3 *m_ptbl;
};

#else //__ITaskbarList3_INTERFACE_DEFINED__

LAMEXP_COMPILER_WARNING("ITaskbarList3 not defined. Compiling *without* support for Win7 taskbar!")

class WinSevenTaskbar
{
public:
	WinSevenTaskbar(void);
	~WinSevenTaskbar(void);

	//Taskbar states
	enum WinSevenTaskbarState
	{
		WinSevenTaskbarNoState = 0,
		WinSevenTaskbarNormalState = 1,
		WinSevenTaskbarIndeterminateState = 2,
		WinSevenTaskbarPausedState = 3,
		WinSevenTaskbarErrorState = 4
	};
	
	//Public interface
	static void initTaskbar(void) {}
	static void setTaskbarState(QWidget *window, WinSevenTaskbarState state) {}
	static void setTaskbarProgress(QWidget *window, unsigned __int64 currentValue, unsigned __int64 maximumValue) {}
	static void setOverlayIcon(QWidget *window, QIcon *icon) {}
};

#endif //__ITaskbarList3_INTERFACE_DEFINED__
