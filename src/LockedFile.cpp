///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2012 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "LockedFile.h"
#include "Global.h"

#include <QResource>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>

#include <stdio.h>
#include <io.h>
#include <fcntl.h>

///////////////////////////////////////////////////////////////////////////////

#define THROW(STR)					\
{									\
	char error_msg[512];			\
	strcpy_s(error_msg, 512, STR);	\
	throw error_msg;				\
}

// WARNING: Passing file descriptors into Qt does NOT work with dynamically linked CRT!
#ifdef QT_NODLL
	static const bool g_useFileDescr = 1;
#else
	static const bool g_useFileDescr = 0;
#endif

///////////////////////////////////////////////////////////////////////////////

static const char *g_seed = "b14bf0ed8d5a62311b366907a6450ea2d52ad8bbc7d520b70400f1dd8ddfa9339011a473";
static const char *g_salt = "80e0cce7af513880065488568071342e5889d6c5ee96190c303b7260e5c8356c350fbe44";

static QByteArray fileHash(QFile &file)
{
	QByteArray hash = QByteArray::fromHex(g_seed);
	QByteArray salt = QByteArray::fromHex(g_salt);
	
	if(file.isOpen() && file.reset())
	{
		QByteArray data = file.readAll();
		QCryptographicHash sha(QCryptographicHash::Sha1);
		QCryptographicHash md5(QCryptographicHash::Md5);

		for(int r = 0; r < 8; r++)
		{
			sha.reset(); md5.reset();
			sha.addData(hash); md5.addData(hash);
			sha.addData(data); md5.addData(data);
			sha.addData(salt); md5.addData(salt);
			hash = sha.result() + md5.result();
		}
	}

	return hash;
}

///////////////////////////////////////////////////////////////////////////////

LockedFile::LockedFile(const QString &resourcePath, const QString &outPath, const QByteArray &expectedHash)
{
	m_fileHandle = NULL;
	QResource resource(resourcePath);
	
	//Make sure the resource is valid
	if(!resource.isValid())
	{
		THROW(QString("Resource '%1' is invalid!").arg(QFileInfo(resourcePath).absoluteFilePath().replace(QRegExp("^:/"), QString())).toUtf8().constData());
	}

	QFile outFile(outPath);
	m_filePath = QFileInfo(outFile).absoluteFilePath();
	
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
		if(outFile.write(reinterpret_cast<const char*>(resource.data()), resource.size()) != resource.size())
		{
			QFile::remove(QFileInfo(outFile).canonicalFilePath());
			THROW(QString("File '%1' could not be written!").arg(QFileInfo(outFile).fileName()).toUtf8().constData());
		}
		outFile.close();
	}
	else
	{
		THROW(QString("File '%1' could not be created!").arg(QFileInfo(outFile).fileName()).toUtf8().constData());
	}

	//Now lock the file!
	for(int i = 0; i < 64; i++)
	{
		m_fileHandle = CreateFileW(QWCHAR(QDir::toNativeSeparators(m_filePath)), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		if((m_fileHandle != NULL) && (m_fileHandle != INVALID_HANDLE_VALUE)) break;
		if(!i) qWarning("Failed to lock file on first attemp, retrying...");
		Sleep(100);
	}
	
	//Locked successfully?
	if((m_fileHandle == NULL) || (m_fileHandle == INVALID_HANDLE_VALUE))
	{
		QFile::remove(QFileInfo(outFile).canonicalFilePath());
		char error_msg[512];
		strcpy_s(error_msg, 512, QString("File '%1' could not be locked!").arg(QFileInfo(outFile).fileName()).toLatin1().constData());
		throw error_msg;
	}

	//Open file for reading
	if(g_useFileDescr)
	{
		int fd = _open_osfhandle(reinterpret_cast<intptr_t>(m_fileHandle), _O_RDONLY | _O_BINARY);
		if(fd >= 0) outFile.open(fd, QIODevice::ReadOnly);
	}
	else
	{
		for(int i = 0; i < 64; i++)
		{
			if(outFile.open(QIODevice::ReadOnly)) break;
			if(!i) qWarning("Failed to re-open file on first attemp, retrying...");
			Sleep(100);
		}
	}

	//Verify file contents
	QByteArray hash;
	if(outFile.isOpen())
	{
		hash = fileHash(outFile).toHex();
		outFile.close();
	}
	else
	{
		QFile::remove(m_filePath);
		THROW(QString("File '%1' could not be read!").arg(QFileInfo(outFile).fileName()).toLatin1().constData());
	}

	//Compare hashes
	if(hash.isNull() || _stricmp(hash.constData(), expectedHash.constData()))
	{
		qWarning("\nFile checksum error:\n A = %s\n B = %s\n", expectedHash.constData(), hash.constData());
		LAMEXP_CLOSE(m_fileHandle);
		QFile::remove(m_filePath);
		THROW(QString("File '%1' is corruputed, take care!").arg(QFileInfo(resourcePath).absoluteFilePath().replace(QRegExp("^:/"), QString())).toLatin1().constData());
	}
}

LockedFile::LockedFile(const QString &filePath)
{
	m_fileHandle = NULL;
	QFileInfo existingFile(filePath);
	existingFile.setCaching(false);
	
	//Make sure the file exists, before we try to lock it
	if(!existingFile.exists())
	{
		char error_msg[256];
		strcpy_s(error_msg, 256, QString("File '%1' does not exist!").arg(existingFile.fileName()).toLatin1().constData());
		throw error_msg;
	}
	
	//Remember file path
	m_filePath = existingFile.canonicalFilePath();

	//Now lock the file
	for(int i = 0; i < 64; i++)
	{
		m_fileHandle = CreateFileW(QWCHAR(QDir::toNativeSeparators(filePath)), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		if((m_fileHandle != NULL) && (m_fileHandle != INVALID_HANDLE_VALUE)) break;
		if(!i) qWarning("Failed to lock file on first attemp, retrying...");
		Sleep(100);
	}

	//Locked successfully?
	if((m_fileHandle == NULL) || (m_fileHandle == INVALID_HANDLE_VALUE))
	{
		THROW(QString("File '%1' could not be locked!").arg(existingFile.fileName()).toLatin1().constData());
	}
}

LockedFile::~LockedFile(void)
{
	LAMEXP_CLOSE(m_fileHandle);
}

const QString &LockedFile::filePath()
{
	return m_filePath;
}