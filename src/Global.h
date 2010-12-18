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

#pragma once

//MSVC
#include "Targetver.h"

//Stdlib
#include <stdio.h>
#include <tchar.h>

//Win32
#include <Windows.h>

//Declarations
class QString;
class LockedFile;
class QDate;
enum QtMsgType;

//Types definitions
typedef struct
{
	int family;
	int model;
	int stepping;
	int count;
	bool x64;
	bool mmx;
	bool sse;
	bool sse2;
	bool sse3;
	bool ssse3;
	char vendor[0x40];
	char brand[0x40];
	bool intel;
}
lamexp_cpu_t;

//Known folders
typedef enum
{
	lamexp_folder_localappdata = 0,
	lamexp_folder_programfiles = 1
}
lamexp_known_folder_t;

//LameXP version info
unsigned int lamexp_version_major(void);
unsigned int lamexp_version_minor(void);
unsigned int lamexp_version_build(void);
const QDate &lamexp_version_date(void);
const char *lamexp_version_release(void);
bool lamexp_version_demo(void);
const char *lamexp_version_compiler(void);
unsigned int lamexp_toolver_neroaac(void);

//Public functions
void lamexp_init_console(int argc, char* argv[]);
bool lamexp_init_qt(int argc, char* argv[]);
int lamexp_init_ipc(void);
void lamexp_message_handler(QtMsgType type, const char *msg);
void lamexp_register_tool(const QString &toolName, LockedFile *file, unsigned int version = 0);
bool lamexp_check_tool(const QString &toolName);
const QString lamexp_lookup_tool(const QString &toolName);
unsigned int lamexp_tool_version(const QString &toolName);
void lamexp_finalization(void);
QString lamexp_rand_str(void);
const QString &lamexp_temp_folder(void);
void lamexp_ipc_read(unsigned int *command, char* message, size_t buffSize);
void lamexp_ipc_send(unsigned int command, const char* message);
lamexp_cpu_t lamexp_detect_cpu_features(void);

//Auxiliary functions
bool lamexp_clean_folder(const QString folderPath);
const QString lamexp_version2string(const QString &pattern, unsigned int version);
QString lamexp_known_folder(lamexp_known_folder_t folder_id);
__int64 lamexp_free_diskspace(const QString &path);

//Debug-only functions
SIZE_T lamexp_dbg_private_bytes(void);

//Helper macros
#define LAMEXP_DELETE(PTR) if(PTR) { delete PTR; PTR = NULL; }
#define LAMEXP_CLOSE(HANDLE) if(HANDLE != NULL && HANDLE != INVALID_HANDLE_VALUE) { CloseHandle(HANDLE); HANDLE = NULL; }
#define QWCHAR(STR) reinterpret_cast<const wchar_t*>(STR.utf16())
#define	LAMEXP_DYNCAST(OUT,CLASS,SRC) try { OUT = dynamic_cast<CLASS>(SRC); } catch(std::bad_cast) { OUT = NULL; }
#define LAMEXP_BOOL(X) (X ? "1" : "0")

//Check for debug build
#if defined(_DEBUG) || defined(QT_DEBUG) || !defined(NDEBUG) || !defined(QT_NO_DEBUG)
#define LAMEXP_DEBUG 1
#define LAMEXP_CHECK_DEBUG_BUILD \
	qWarning("---------------------------------------------------------"); \
	qWarning("DEBUG BUILD: DO NOT RELEASE THIS BINARY TO THE PUBLIC !!!"); \
	qWarning("---------------------------------------------------------\n"); 
#else
#define LAMEXP_DEBUG 0
#define LAMEXP_CHECK_DEBUG_BUILD \
	if(IsDebuggerPresent())	{ \
	FatalAppExit(0, L"Not a debug build. Please unload debugger and try again!"); \
	TerminateProcess(GetCurrentProcess, -1); } \
	CreateThread(NULL, NULL, reinterpret_cast<LPTHREAD_START_ROUTINE>(&debugThreadProc), NULL, NULL, NULL);
	void WINAPI debugThreadProc(__in  LPVOID lpParameter);
#endif

//Memory check
#if defined(_DEBUG)
#define LAMEXP_MEMORY_CHECK(CMD) \
{ \
	SIZE_T _privateBytesBefore = lamexp_dbg_private_bytes(); \
	CMD; \
	SIZE_T _privateBytesLeak = (lamexp_dbg_private_bytes() - _privateBytesBefore) / 1024; \
	if(_privateBytesLeak > 10) { \
		qWarning("Memory leak: Lost %u KiloBytes.", _privateBytesLeak); \
	} \
}
#else
#define LAMEXP_MEMORY_CHECK(CMD) CMD
#endif

//Check for CPU-compatibility options
#if _M_IX86_FP != 0
#error We should not enabled SSE or SSE2 in release builds!
#endif
