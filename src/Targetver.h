///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2021 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU GENERAL PUBLIC LICENSE as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version; always including the non-optional
// LAMEXP GNU GENERAL PUBLIC LICENSE ADDENDUM. See "License.txt" file!
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

// Check Windows SDK version
// We currently do NOT support the Windows SDK 8.0, because it broke Windows XP support!
// Also the minimum required Windows SDK version to build LameXP currently is 6.0A
#include <ntverp.h>
#if !defined(VER_PRODUCTMAJORVERSION) || !defined(VER_PRODUCTMINORVERSION)
#error Windows SDK version is NOT defined !!!
#endif
#if (VER_PRODUCTMAJORVERSION < 6)
#error Your Windows SDK is too old (unsupported), please build LameXP with Windows SDK version 7.1! (6.0A or 7.0A should work too)
#endif
#if (VER_PRODUCTMAJORVERSION > 6) || ((VER_PRODUCTMAJORVERSION == 6) && (VER_PRODUCTMINORVERSION > 1))
#error Your Windows SDK is too new (unsupported), please build LameXP with Windows SDK version 7.1! (6.0A or 7.0A should work too)
#endif


// The following macros define the minimum required platform.  The minimum required platform
// is the earliest version of Windows, Internet Explorer etc. that has the necessary features to run 
// your application.  The macros work by enabling all features available on platform versions up to and 
// including the version specified.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.
//#include <WinSDKVer.h>

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.

#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows 2000.
#define _WIN32_WINNT 0x0501     // Change this to the appropriate value to target other versions of Windows.
#endif

#include <SDKDDKVer.h>
