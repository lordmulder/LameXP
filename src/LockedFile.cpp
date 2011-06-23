///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2011 LoRd_MuldeR <MuldeR2@GMX.de>
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

#define THROW(STR)					\
{									\
	char error_msg[512];			\
	strcpy_s(error_msg, 512, STR);	\
	throw error_msg;				\
}

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

	//Verify file contents
	outFile.open(QIODevice::ReadOnly);
	QCryptographicHash fileHash(QCryptographicHash::Sha1);
	if(outFile.isOpen() && outFile.isReadable())
	{
		fileHash.addData(outFile.readAll());
		outFile.close();
	}
	else
	{
		QFile::remove(QFileInfo(outFile).canonicalFilePath());
		THROW(QString("File '%1' could not be read!").arg(QFileInfo(outFile).fileName()).toLatin1().constData());
	}

	//Compare hashes
	if(_stricmp(fileHash.result().toHex().constData(), expectedHash.constData()))
	{
		qWarning("\nFile checksum error:\n Expected = %040s\n Detected = %040s\n", expectedHash.constData(), fileHash.result().toHex().constData());
		LAMEXP_CLOSE(m_fileHandle);
		QFile::remove(QFileInfo(outFile).canonicalFilePath());
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