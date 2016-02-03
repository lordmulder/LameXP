///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2016 LoRd_MuldeR <MuldeR2@GMX.de>
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

#pragma once

#ifndef LAMEXP_INC_CONFIG
#error Please do *not* include CONFIG.H directly!
#endif

///////////////////////////////////////////////////////////////////////////////
// LameXP Version Info
///////////////////////////////////////////////////////////////////////////////

#define VER_LAMEXP_MAJOR					4
#define VER_LAMEXP_MINOR_HI					1
#define VER_LAMEXP_MINOR_LO					4
#define VER_LAMEXP_TYPE						Alpha
#define VER_LAMEXP_PATCH					3
#define VER_LAMEXP_BUILD					1864
#define VER_LAMEXP_CONFG					1818

///////////////////////////////////////////////////////////////////////////////
// LameXP Build Options
///////////////////////////////////////////////////////////////////////////////

#define VER_LAMEXP_PORTABLE_EDITION			0
#define VER_LAMEXP_CONSOLE_ENABLED			0

///////////////////////////////////////////////////////////////////////////////
// Tool versions (minimum expected versions!)
///////////////////////////////////////////////////////////////////////////////

#define VER_LAMEXP_TOOL_NEROAAC				1540
#define VER_LAMEXP_TOOL_FHGAACENC			20151024
#define VER_LAMEXP_TOOL_FDKAACENC			62
#define VER_LAMEXP_TOOL_QAAC				258

///////////////////////////////////////////////////////////////////////////////
// Helper macros (aka: having fun with the C pre-processor)
///////////////////////////////////////////////////////////////////////////////

#define VER_LAMEXP_SUFFIX_HLP1(X,Y)			X-Y
#define VER_LAMEXP_SUFFIX_HLP2(X,Y)			VER_LAMEXP_SUFFIX_HLP1(X,Y)
#define VER_LAMEXP_SUFFIX					VER_LAMEXP_SUFFIX_HLP2(VER_LAMEXP_TYPE, VER_LAMEXP_PATCH)

#define VER_LAMEXP_STR_HLP1(X)				#X
#define VER_LAMEXP_STR_HLP2(U,V,W,X,Y,Z)	VER_LAMEXP_STR_HLP1(v##U.V##W Y; Build X; Z)
#define VER_LAMEXP_STR_HLP3(U,V,W,X,Y,Z)	VER_LAMEXP_STR_HLP2(U,V,W,X,Y,Z)
#define VER_LAMEXP_STR						VER_LAMEXP_STR_HLP3(VER_LAMEXP_MAJOR,VER_LAMEXP_MINOR_HI,VER_LAMEXP_MINOR_LO,VER_LAMEXP_BUILD,VER_LAMEXP_SUFFIX,_CONFIG_NAME)

#define VER_LAMEXP_RNAME_HLP1(X)			#X
#define VER_LAMEXP_RNAME_HLP2(X)			VER_LAMEXP_RNAME_HLP1(X)
#define VER_LAMEXP_RNAME					VER_LAMEXP_RNAME_HLP2(VER_LAMEXP_SUFFIX)

#define VER_LAMEXP_MINOR_HLP1(X,Y)			X##Y
#define VER_LAMEXP_MINOR_HLP2(X,Y)			VER_LAMEXP_MINOR_HLP1(X,Y)
#define VER_LAMEXP_MINOR					VER_LAMEXP_MINOR_HLP2(VER_LAMEXP_MINOR_HI,VER_LAMEXP_MINOR_LO)
