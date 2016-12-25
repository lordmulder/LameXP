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

#include "FileHash.h"

//MUtils
#include <MUtils/Hash.h>
#include <MUtils/Exception.h>

static const char *g_blnk = "deadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef";
static const char *g_seed = "c375d83b4388329408dfcbb4d9a065b6e06d28272f25ef299c70b506e26600af79fd2f866ae24602daf38f25c9d4b7e1";
static const char *g_salt = "ee9f7bdabc170763d2200a7e3030045aafe380011aefc1730e547e9244c62308aac42a976feeca224ba553de0c4bb883";

QByteArray FileHash::computeHash(QFile &file)
{
	QByteArray result;

	if(file.isOpen() && file.reset())
	{
		QScopedPointer<MUtils::Hash::Hash> hash(MUtils::Hash::create(MUtils::Hash::HASH_KECCAK_384));
		const QByteArray data = file.readAll();
		if (data.size() >= 16)
		{
			const QByteArray seed = QByteArray::fromHex(g_seed);
			const QByteArray salt = QByteArray::fromHex(g_salt);

			bool okay[3];
			okay[0] = hash->update(seed);
			okay[1] = hash->update(data);
			okay[2] = hash->update(salt);

			if (okay[0] && okay[1] && okay[2])
			{
				result = hash->digest(true);
			}
		}
	}

	return result.isEmpty() ? QByteArray::fromHex(g_blnk).toHex() : result;
}
