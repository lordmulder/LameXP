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

//Internal
#include "Global.h"
#include "FileHash.h"

//MUtils
#include <MUtils/OSSupport.h>
#include <MUtils/Exception.h>

//Qt
#include <QResource>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>

//CRT
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

#define VALID_HANDLE(H) (((H) != NULL) && ((H) != INVALID_HANDLE_VALUE))

static void CLOSE_HANDLE(HANDLE &h)
{
	if(VALID_HANDLE(h))
	{
		CloseHandle(h);
		h = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////

static __forceinline void doWriteOutput(QFile &outFile, const QResource *const resource)
{
	for(int i = 0; i < 64; i++)
	{
		if(outFile.open(QIODevice::WriteOnly))
		{
			break;
		}
		if(i == 0)
		{
			qWarning("Failed to open file on first attemp, retrying...");
		}
		Sleep(25);
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
}

static __forceinline void doValidateFileExists(const QString &filePath)
{
	QFileInfo existingFileInfo(filePath);
	existingFileInfo.setCaching(false);
	
	//Make sure the file exists, before we try to lock it
	if((!existingFileInfo.exists()) || (!existingFileInfo.isFile()) || filePath.isEmpty())
	{
		MUTILS_THROW_FMT("File '%s' does not exist!", MUTILS_UTF8(filePath));
	}
}

static __forceinline void doLockFile(HANDLE &fileHandle, const QString &filePath, QFile *const outFile)
{
	bool success = false;
	fileHandle = INVALID_HANDLE_VALUE;

	//Try to open the file!
	for(int i = 0; i < 64; i++)
	{
		const HANDLE hTemp = CreateFileW(MUTILS_WCHR(QDir::toNativeSeparators(filePath)), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		if(VALID_HANDLE(hTemp))
		{
			fileHandle = hTemp;
			break; /*file opened successfully*/
		}
		if(i == 0)
		{
			qWarning("Failed to open file on first attemp, retrying...");
		}
		Sleep(1);
	}
	
	//Now try to actually lock the file!
	if (VALID_HANDLE(fileHandle))
	{
		for (int i = 0; i < 64; i++)
		{
			LARGE_INTEGER fileSize;
			if (GetFileSizeEx(fileHandle, &fileSize))
			{
				OVERLAPPED overlapped = { 0U, 0U, 0U, 0U, 0U };
				if (LockFileEx(fileHandle, LOCKFILE_FAIL_IMMEDIATELY, 0, fileSize.LowPart, fileSize.HighPart, &overlapped))
				{
					success = true;
					break; /*file locked successfully*/
				}
				Sleep(1);
			}
			if (i == 0)
			{
				qWarning("Failed to lock file on first attemp, retrying...");
			}
		}
	}

	//Locked successfully?
	if(!success)
	{
		CLOSE_HANDLE(fileHandle);
		if(outFile)
		{
			QFile::remove(QFileInfo(*outFile).canonicalFilePath());
		}
		MUTILS_THROW_FMT("File '%s' could not be locked!", MUTILS_UTF8(QFileInfo(filePath).fileName()));
	}
}

static __forceinline void doInitFileDescriptor(const HANDLE &fileHandle, int &fileDescriptor)
{
	fileDescriptor = _open_osfhandle(reinterpret_cast<intptr_t>(fileHandle), _O_RDONLY | _O_BINARY);
	if(fileDescriptor < 0)
	{
		MUTILS_THROW_FMT("Failed to obtain C Runtime file descriptor!");
	}
}

static __forceinline void doValidateHash(HANDLE &fileHandle, const int &fileDescriptor, const QByteArray &expectedHash, const QString &filePath)
{
	QFile checkFile;

	//Now re-open the file for reading
	if(g_useFileDescrForQFile)
	{
		checkFile.open(fileDescriptor, QIODevice::ReadOnly);
	}
	else
	{
		checkFile.setFileName(filePath);
		for(int i = 0; i < 64; i++)
		{
			if(checkFile.open(QIODevice::ReadOnly)) break;
			if(!i) qWarning("Failed to re-open file on first attemp, retrying...");
			Sleep(100);
		}
	}

	//Opened successfully
	if((!checkFile.isOpen()) || checkFile.peek(1).isEmpty())
	{
		QFile::remove(filePath);
		MUTILS_THROW_FMT("File '%s' could not be read!", MUTILS_UTF8(QFileInfo(filePath).fileName()));
	}

	//Verify file contents
	const QByteArray hash = FileHash::computeHash(checkFile);
	checkFile.close();

	//Compare hashes
	if(hash.isNull() || _stricmp(hash.constData(), expectedHash.constData()))
	{
		qWarning("\nFile checksum error:\n A = %s\n B = %s\n", expectedHash.constData(), hash.constData());
		CLOSE_HANDLE(fileHandle);
		QFile::remove(filePath);
		MUTILS_THROW_FMT("File '%s' is corruputed, take care!", MUTILS_UTF8(QFileInfo(filePath).fileName()));
	}
}

static __forceinline bool doRemoveFile(const QString &filePath)
{
	for(int i = 0; i < 32; i++)
	{
		if(MUtils::remove_file(filePath))
		{
			return true;
		}
		MUtils::OS::sleep_ms(1);
	}
	return false;
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

	//Write data to output file
	QFile outFile(m_filePath);
	doWriteOutput(outFile, resource);

	//Now lock the file!
	doLockFile(fileHandle, m_filePath, &outFile);

	//Get file descriptor
	doInitFileDescriptor(fileHandle, m_fileDescriptor);

	//Validate file hash
	doValidateHash(fileHandle, m_fileDescriptor, expectedHash, m_filePath);
}

LockedFile::LockedFile(const QString &filePath, const QByteArray &expectedHash, const bool bOwnsFile)
:
	m_bOwnsFile(bOwnsFile),
	m_filePath(QFileInfo(filePath).absoluteFilePath())
{
	m_fileDescriptor = -1;
	HANDLE fileHandle = NULL;
	
	//Make sure the file exists, before we try to lock it
	doValidateFileExists(m_filePath);

	//Now lock the file!
	doLockFile(fileHandle, m_filePath, NULL);

	//Get file descriptor
	doInitFileDescriptor(fileHandle, m_fileDescriptor);

	//Validate file hash
	doValidateHash(fileHandle, m_fileDescriptor, expectedHash, m_filePath);
}

LockedFile::LockedFile(const QString &filePath, const bool bOwnsFile)
:
	m_bOwnsFile(bOwnsFile),
	m_filePath(QFileInfo(filePath).canonicalFilePath())
{
	m_fileDescriptor = -1;
	HANDLE fileHandle = NULL;
	
	//Make sure the file exists, before we try to lock it
	doValidateFileExists(m_filePath);

	//Now lock the file!
	doLockFile(fileHandle, m_filePath, NULL);

	//Get file descriptor
	doInitFileDescriptor(fileHandle, m_fileDescriptor);
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
		doRemoveFile(m_filePath);
	}
}

const QString LockedFile::filePath(void)
{
	if (m_fileDescriptor >= 0)
	{
		const QString path = MUtils::OS::get_file_path(m_fileDescriptor);
		if (!path.isEmpty())
		{
			return path;
		}
		MUTILS_THROW_FMT("Failed to determine file path!");
	}
	return m_filePath;
}
