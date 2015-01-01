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

#include "LockedFile.h"
#include "Global.h"

//MUtils
#include <MUtils/KeccakHash.h>
#include <MUtils/OSSupport.h>
#include <MUtils/Exception.h>

//Qt
#include <QResource>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <stdexcept>

//Windows includes
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

///////////////////////////////////////////////////////////////////////////////

// WARNING: Passing file descriptors into Qt does NOT work with dynamically linked CRT!
#ifdef QT_NODLL
	static const bool g_useFileDescrForQFile = true;
#else
	static const bool g_useFileDescrForQFile = false;
#endif

static void CLOSE_HANDLE(HANDLE &h)
{
	if((h != NULL) && (h != INVALID_HANDLE_VALUE))
	{
		CloseHandle(h);
		h = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////

static const char *g_blnk = "deadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeefdeadbeef";
static const char *g_seed = "c375d83b4388329408dfcbb4d9a065b6e06d28272f25ef299c70b506e26600af79fd2f866ae24602daf38f25c9d4b7e1";
static const char *g_salt = "ee9f7bdabc170763d2200a7e3030045aafe380011aefc1730e547e9244c62308aac42a976feeca224ba553de0c4bb883";

QByteArray LockedFile::fileHash(QFile &file)
{
	QByteArray hash = QByteArray::fromHex(g_blnk);

	if(file.isOpen() && file.reset())
	{
		MUtils::KeccakHash keccak;

		const QByteArray data = file.readAll();
		const QByteArray seed = QByteArray::fromHex(g_seed);
		const QByteArray salt = QByteArray::fromHex(g_salt);
	
		if(keccak.init(MUtils::KeccakHash::hb384))
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

///////////////////////////////////////////////////////////////////////////////

LockedFile::LockedFile(QResource *const resource, const QString &outPath, const QByteArray &expectedHash, const bool bOwnsFile)
:
	m_bOwnsFile(bOwnsFile),
	m_filePath(QFileInfo(outPath).absoluteFilePath())
{
	m_fileDescriptor = -1;
	HANDLE fileHandle = NULL;
		
	//Make sure the resource is valid
	if(!(resource->isValid() && resource->data()))
	{
		MUTILS_THROW_FMT("The resource at %p is invalid!", resource);
	}

	QFile outFile(m_filePath);
	
	//Open output file
	for(int i = 0; i < 64; i++)
	{
		if(outFile.open(QIODevice::WriteOnly)) break;
		if(!i) qWarning("Failed to open file on first attemp, retrying...");
		Sleep(100);
	}
	
	//Write data to file
	if(outFile.isOpen() && outFile.isWritable())
	{
		if(outFile.write(reinterpret_cast<const char*>(resource->data()), resource->size()) != resource->size())
		{
			QFile::remove(QFileInfo(outFile).canonicalFilePath());
			MUTILS_THROW_FMT("File '%s' could not be written!", MUTILS_UTF8(QFileInfo(outFile).fileName()));
		}
	}
	else
	{
		MUTILS_THROW_FMT("File '%s' could not be created!", MUTILS_UTF8(QFileInfo(outFile).fileName()));
	}

	//Close file after it has been written
	outFile.close();

	//Now lock the file!
	for(int i = 0; i < 64; i++)
	{
		fileHandle = CreateFileW(MUTILS_WCHR(QDir::toNativeSeparators(m_filePath)), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		if((fileHandle != NULL) && (fileHandle != INVALID_HANDLE_VALUE)) break;
		if(!i) qWarning("Failed to lock file on first attemp, retrying...");
		Sleep(100);
	}
	
	//Locked successfully?
	if((fileHandle == NULL) || (fileHandle == INVALID_HANDLE_VALUE))
	{
		QFile::remove(QFileInfo(outFile).canonicalFilePath());
		MUTILS_THROW_FMT("File '%s' could not be locked!", MUTILS_UTF8(QFileInfo(m_filePath).fileName()));
	}

	//Get file descriptor
	m_fileDescriptor = _open_osfhandle(reinterpret_cast<intptr_t>(fileHandle), _O_RDONLY | _O_BINARY);
	if(m_fileDescriptor < 0)
	{
		MUTILS_THROW_FMT("Failed to obtain C Runtime file descriptor!");
	}

	QFile checkFile;

	//Now re-open the file for reading
	if(g_useFileDescrForQFile)
	{
		checkFile.open(m_fileDescriptor, QIODevice::ReadOnly);
	}
	else
	{
		checkFile.setFileName(m_filePath);
		for(int i = 0; i < 64; i++)
		{
			if(checkFile.open(QIODevice::ReadOnly)) break;
			if(!i) qWarning("Failed to re-open file on first attemp, retrying...");
			Sleep(100);
		}
	}

	//Opened successfully
	if(!checkFile.isOpen())
	{
		QFile::remove(m_filePath);
		MUTILS_THROW_FMT("File '%s' could not be read!", MUTILS_UTF8(QFileInfo(m_filePath).fileName()));
	}

	//Verify file contents
	const QByteArray hash = fileHash(checkFile);
	checkFile.close();

	//Compare hashes
	if(hash.isNull() || _stricmp(hash.constData(), expectedHash.constData()))
	{
		qWarning("\nFile checksum error:\n A = %s\n B = %s\n", expectedHash.constData(), hash.constData());
		CLOSE_HANDLE(fileHandle);
		QFile::remove(m_filePath);
		MUTILS_THROW_FMT("File '%s' is corruputed, take care!", MUTILS_UTF8(QFileInfo(m_filePath).fileName()));
	}
}

LockedFile::LockedFile(const QString &filePath, const bool bOwnsFile)
:
	m_bOwnsFile(bOwnsFile),
	m_filePath(QFileInfo(filePath).canonicalFilePath())
{
	m_fileDescriptor = -1;
	HANDLE fileHandle = NULL;

	QFileInfo existingFileInfo(filePath);
	existingFileInfo.setCaching(false);
	
	//Make sure the file exists, before we try to lock it
	if((!existingFileInfo.exists()) || (!existingFileInfo.isFile()) || m_filePath.isEmpty())
	{
		MUTILS_THROW_FMT("File '%s' does not exist!", MUTILS_UTF8(filePath));
	}
	
	//Now lock the file
	for(int i = 0; i < 64; i++)
	{
		fileHandle = CreateFileW(MUTILS_WCHR(QDir::toNativeSeparators(m_filePath)), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		if((fileHandle != NULL) && (fileHandle != INVALID_HANDLE_VALUE)) break;
		if(!i) qWarning("Failed to lock file on first attemp, retrying...");
		Sleep(100);
	}

	//Locked successfully?
	if((fileHandle == NULL) || (fileHandle == INVALID_HANDLE_VALUE))
	{
		MUTILS_THROW_FMT("File '%s' could not be locked!", MUTILS_UTF8(QFileInfo(m_filePath).fileName()));
	}

	//Get file descriptor
	m_fileDescriptor = _open_osfhandle(reinterpret_cast<intptr_t>(fileHandle), _O_RDONLY | _O_BINARY);
	if(m_fileDescriptor < 0)
	{
		MUTILS_THROW_FMT("Failed to obtain C Runtime file descriptor!");
	}
}

LockedFile::~LockedFile(void)
{
	if(m_fileDescriptor >= 0)
	{
		_close(m_fileDescriptor);
		m_fileDescriptor = -1;
	}
	if(m_bOwnsFile)
	{
		if(QFileInfo(m_filePath).exists())
		{
			for(int i = 0; i < 64; i++)
			{
				if(QFile::remove(m_filePath)) break;
				MUtils::OS::sleep_ms(1);
			}
		}
	}
}

const QString &LockedFile::filePath()
{
	return m_filePath;
}

void LockedFile::selfTest()
{
	if(!MUtils::KeccakHash::selfTest())
	{
		MUTILS_THROW("QKeccakHash self-test has failed!");
	}
}
