///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2010 LoRd_MuldeR <MuldeR2@GMX.de>
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

/*
 * LameXP Version Info
 */
#define VER_LAMEXP_MAJOR				4
#define VER_LAMEXP_MINOR_HI				0
#define VER_LAMEXP_MINOR_LO				0
#define VER_LAMEXP_BUILD				8
#define VER_LAMEXP_SUFFIX				TechPreview

/*
 * Resource ID's
 */
#define IDI_ICON1                       106
#define IDR_WAVE_ABOUT                  666

/*
 * Next default values for new objects
 */
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        107
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1001
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif

/*
 * Helper macros (having fun with the C pre-processor)
 */
#define VER_LAMEXP_STR_HLP1(X)			#X
#define VER_LAMEXP_STR_HLP2(V,W,X,Y,Z)	VER_LAMEXP_STR_HLP1(v##V.W##X Z [Build Y])
#define VER_LAMEXP_STR_HLP3(V,W,X,Y,Z)	VER_LAMEXP_STR_HLP2(V,W,X,Y,Z)
#define VER_LAMEXP_STR					VER_LAMEXP_STR_HLP3(VER_LAMEXP_MAJOR,VER_LAMEXP_MINOR_HI,VER_LAMEXP_MINOR_LO,VER_LAMEXP_BUILD,VER_LAMEXP_SUFFIX)
#define VER_LAMEXP_SUFFIX_STR_HLP1(X)	#X		
#define VER_LAMEXP_SUFFIX_STR_HLP2(X)	VER_LAMEXP_SUFFIX_STR_HLP1(X)
#define VER_LAMEXP_SUFFIX_STR			VER_LAMEXP_SUFFIX_STR_HLP2(VER_LAMEXP_SUFFIX)
#define VER_LAMEXP_MINOR_HLP1(X,Y)		X##Y
#define VER_LAMEXP_MINOR_HLP2(X,Y)		VER_LAMEXP_MINOR_HLP1(X,Y)
#define VER_LAMEXP_MINOR				VER_LAMEXP_MINOR_HLP2(VER_LAMEXP_MINOR_HI,VER_LAMEXP_MINOR_LO)
