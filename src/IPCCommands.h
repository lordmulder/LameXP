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

#include <climits>

typedef enum
{
	IPC_CMD_NOOP       = 0x0000,
	IPC_CMD_PING       = 0x0001,
	IPC_CMD_ADD_FILE   = 0x0002,
	IPC_CMD_ADD_FOLDER = 0x0003,
	IPC_CMD_TERMINATE  = 0xFFFF
}
lamexp_ipc_command_t;

typedef enum
{
	IPC_FLAG_NONE          = 0x0000,
	IPC_FLAG_ADD_RECURSIVE = 0x0001,
	IPC_FLAG_FORCE         = 0x8000,
}
lamexp_ipc_flag_t;
