///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2015 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "FileHash.h"

//MUtils
#include <MUtils/Hash_Keccak.h>
#include <MUtils/Exception.h>

static const char *g_blnk = "deadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef";
static const char *g_seed = "c375d83b4388329408dfcbb4d9a065b6e06d28272f25ef299c70b506e26600af79fd2f866ae24602daf38f25c9d4b7e1";
static const char *g_salt = "ee9f7bdabc170763d2200a7e3030045aafe380011aefc1730e547e9244c62308aac42a976feeca224ba553de0c4bb883";

QByteArray FileHash::computeHash(QFile &file)
{
	QByteArray hash = QByteArray::fromHex(g_blnk);

	if(file.isOpen() && file.reset())
	{
		MUtils::Hash::Keccak keccak;

		const QByteArray data = file.readAll();
		const QByteArray seed = QByteArray::fromHex(g_seed);
		const QByteArray salt = QByteArray::fromHex(g_salt);
	
		if(keccak.init(MUtils::Hash::Keccak::hb384))
		{
			bool ok = true;
			ok = ok && keccak.addData(seed);
			ok = ok && keccak.addData(data);
			ok = ok && keccak.addData(salt);
			if(ok)
			{
				const QByteArray digest = keccak.finalize();
				if(!digest.isEmpty()) hash = digest.toHex();
			}
		}
	}

	return hash;
}

void FileHash::selfTest(void)
{
	if(!MUtils::Hash::Keccak::selfTest())
	{
		MUTILS_THROW("QKeccakHash self-test has failed!");
	}
}
