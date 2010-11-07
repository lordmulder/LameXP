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

#include "LockedFile.h"
#include "Global.h"

#include <QResource>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>

LockedFile::LockedFile(const QString &resourcePath, const QString &outPath, const QByteArray &expectedHash)
{
	m_fileHandle = NULL;
	
	QResource resource(resourcePath);
	QFile outFile(outPath);
	
	m_filePath = QFileInfo(outFile).absoluteFilePath();
	outFile.open(QIODevice::WriteOnly);
	
	//Write data to file
	if(outFile.isOpen() && outFile.isWritable() && resource.isValid())
	{
		if(outFile.write(reinterpret_cast<const char*>(resource.data()), resource.size()) != resource.size())
		{
			QFile::remove(QFileInfo(outFile).absoluteFilePath());
			char error_msg[256];
			strcpy_s(error_msg, 256, QString("File '%1' could not be written!").arg(QFileInfo(outFile).fileName()).toUtf8().constData());
			throw error_msg;
		}
		outFile.close();
	}
	else
	{
		char error_msg[256];
		strcpy_s(error_msg, 256, QString("File '%1' could not be created!").arg(QFileInfo(outFile).fileName()).toUtf8().constData());
		throw error_msg;
	}

	//Now lock the file
	m_fileHandle = CreateFileW(QWCHAR(QDir::toNativeSeparators(m_filePath)), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);

	if(m_fileHandle == INVALID_HANDLE_VALUE)
	{
		QFile::remove(QFileInfo(outFile).absoluteFilePath());
		char error_msg[256];
		strcpy_s(error_msg, 256, QString("File '%1' could not be locked!").arg(QFileInfo(outFile).fileName()).toLatin1().constData());
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

	if(_stricmp(fileHash.result().toHex().constData(), expectedHash.constData()))
	{
		qWarning("\nFile checksum error:\n Expected = %040s\n Detected = %040s\n", expectedHash.constData(), fileHash.result().toHex().constData());
		LAMEXP_CLOSE(m_fileHandle);
		QFile::remove(QFileInfo(outFile).absoluteFilePath());
		char error_msg[256];
		strcpy_s(error_msg, 256, QString("File '%1' is corruputed, take care!").arg(QFileInfo(outFile).fileName()).toLatin1().constData());
		throw error_msg;
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