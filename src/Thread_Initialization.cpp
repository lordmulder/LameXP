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

#include "Thread_Initialization.h"

#include "Global.h"
#include "LockedFile.h"

#include <QFileInfo>
#include <QCoreApplication>
#include <QProcess>
#include <QMap>
#include <QDir>
#include <QLibrary>

#include <Windows.h>

////////////////////////////////////////////////////////////
// TOOLS
////////////////////////////////////////////////////////////

struct lamexp_tool_t
{
	char *pcHash;
	char *pcName;
	unsigned int uiVersion;
};

static const struct lamexp_tool_t g_lamexp_tools[] =
{
	{"153f4274702f3629093b561a31dbf50e2c146305", "alac.exe", UINT_MAX},
	{"4ecc017a66fe43092110f11494f384e57d99280d", "elevator.exe", UINT_MAX},
	{"097dd004f44dbda57dbaeb5f15b34a220724ad60", "faad.exe", UINT_MAX},
	{"133171ac21f083d565ee9c33ef9cd92fc966811e", "flac.exe", 121},
	{"cf379081035ae6bfb6f7bc22f13bfb7ac6302ac5", "gpgv.exe", UINT_MAX},
	{"d837bf6ee4dab557d8b02d46c75a24e58980fffa", "gpgv.gpg", UINT_MAX},
	{"143fc001a2f6c56fe1b9e6f8a2eb2b53b9e1e504", "lame.exe", 39910},
	{"775b260b3f64101beaeb317b74746f9bccdab842", "MAC.exe", UINT_MAX},
	{"c7aa8ca8e05a5083d0ef961c1224ac3037718d4f", "mediainfo_i386.exe", 738},
	{"74ca0c218eab75393a45fa8f3b53b7f2852fd28f", "mediainfo_x64.exe", 738},
	{"55c293a80475f7aeccf449ac9487a4626e5139cb", "mpcdec.exe", UINT_MAX},
	{"8bbf4a3fffe2ff143eb5ba2cf82ca16d676e865d", "mpg123.exe", UINT_MAX},
	{"380c734e3c3948a844b9fae213d53a93ab20beba", "oggdec.exe", UINT_MAX},
	{"ecd15abe103184aca96e406f5f1c82c6fb2e665d", "oggenc2_i386.exe", 287},
	{"ffe0fbd73352396dc3752ac9d484dbfc754a226d", "oggenc2_sse2.exe", 287},
	{"a8c50872e544a55495a824426e9378984f2ae01d", "oggenc2_x64.exe", 287},
	{"cd95369051f96b9ca3a997658771c5ea52bc874d", "selfdelete.exe", UINT_MAX},
	{"ffeaa70bd6321185eafcb067ab2dc441650038bf", "shorten.exe", UINT_MAX},
	{"346ce516281c97e92e1b8957ddeca52edcf2d056", "speexdec.exe", UINT_MAX},
	{"8a74b767cfe88bf88c068fdae0de02d65589d25e", "takc.exe", UINT_MAX},
	{"1c5cedb56358a0e8c4590a863a97c94d7d7e98b2", "ttaenc.exe", UINT_MAX},
	{"7dcf6517aa90ed15737ee8ea50ea00a6dece2d27", "valdec.exe", UINT_MAX},
	{"8159f4e824b3e343ece95ba6dbb5e16da9c4866e", "volumax.exe", UINT_MAX},
	{"62e2805d1b2eb2a4d86a5ca6e6ea58010d05d2a7", "wget.exe", UINT_MAX},
	{"a17011961aa8696bc935e097b3242d33c38a9842", "wupdate.exe", UINT_MAX},
	{"4d018ac7f6a42abd53faacfae5055c2a3c176430", "wvunpack.exe", UINT_MAX},
	{NULL, NULL, NULL}
};

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

InitializationThread::InitializationThread(void)
{
	m_bSuccess = false;
}

////////////////////////////////////////////////////////////
// Thread Main
////////////////////////////////////////////////////////////

void InitializationThread::run()
{
	m_bSuccess = false;
	delay();
	
	QMap<QString, QString> checksum;
	QMap<QString, unsigned int> version;

	//Init checksums
	for(int i = 0; i < INT_MAX; i++)
	{
		if(!g_lamexp_tools[i].pcName && !g_lamexp_tools[i].pcHash && !g_lamexp_tools[i].uiVersion)
		{
			break;
		}
		else if(g_lamexp_tools[i].pcName && g_lamexp_tools[i].pcHash && g_lamexp_tools[i].uiVersion)
		{
			const QString currentTool = QString::fromLatin1(g_lamexp_tools[i].pcName);
			checksum.insert(currentTool, QString::fromLatin1(g_lamexp_tools[i].pcHash));
			version.insert(currentTool, g_lamexp_tools[i].uiVersion);
		}
		else
		{
			qFatal("Inconsistent checksum data detected. Take care!");
		}
	}

	QDir toolsDir(":/tools/");
	QList<QFileInfo> toolsList = toolsDir.entryInfoList(QStringList("*.*"), QDir::Files, QDir::Name);

	//Extract all files
	for(int i = 0; i < toolsList.count(); i++)
	{
		try
		{
			qDebug("Extracting file: %s", g_lamexp_tools[i].pcName);
			QString toolName = toolsList.at(i).fileName();
			QByteArray toolHash = checksum.take(toolName).toLatin1();
			unsigned int toolVersion = version.take(toolName);
			if(toolHash.size() != 40)
			{
				throw "The required checksum is missing, take care!";
			}
			LockedFile *lockedFile = new LockedFile(QString(":/tools/%1").arg(toolName), QString(lamexp_temp_folder()).append(QString("/tool_%1").arg(toolName)), toolHash);
			lamexp_register_tool(toolName, lockedFile, toolVersion);
		}
		catch(char *errorMsg)
		{
			qFatal("At least one required tool could not be extracted:\n%s", errorMsg);
			return;
		}
	}
	
	if(!checksum.isEmpty())
	{
		qFatal("At least one required tool could not be found:\n%s", toolsDir.filePath(checksum.keys().first()).toLatin1().constData());
		return;
	}
	
	qDebug("All extracted.\n");

	//Look for Nero encoder
	initNeroAac();
	
	//Look for WMA File decoder
	initWmaDec();

	delay();
	m_bSuccess = true;
}

////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
////////////////////////////////////////////////////////////

void InitializationThread::delay(void)
{
	const char *temp = "|/-\\";
	printf("Thread is doing something important... ?\b", temp[4]);

	for(int i = 0; i < 20; i++)
	{
		printf("%c\b", temp[i%4]);
		msleep(100);
	}

	printf("Done\n\n");
}

void InitializationThread::initNeroAac(void)
{
	QFileInfo neroFileInfo[3];
	neroFileInfo[0] = QFileInfo(QString("%1/neroAacEnc.exe").arg(QCoreApplication::applicationDirPath()));
	neroFileInfo[1] = QFileInfo(QString("%1/neroAacDec.exe").arg(QCoreApplication::applicationDirPath()));
	neroFileInfo[2] = QFileInfo(QString("%1/neroAacTag.exe").arg(QCoreApplication::applicationDirPath()));
	
	bool neroFilesFound = true;
	for(int i = 0; i < 3; i++)	{ if(!neroFileInfo[i].exists()) neroFilesFound = false; }

	//Lock the Nero binaries
	if(!neroFilesFound)
	{
		qDebug("Nero encoder binaries not found -> AAC encoding support will be disabled!\n");
		return;
	}

	qDebug("Found Nero AAC encoder binary:\n%s\n", neroFileInfo[0].canonicalFilePath().toUtf8().constData());

	LockedFile *neroBin[3];
	for(int i = 0; i < 3; i++) neroBin[i] = NULL;

	try
	{
		for(int i = 0; i < 3; i++)
		{
			neroBin[i] = new LockedFile(neroFileInfo[i].canonicalFilePath());
		}
	}
	catch(...)
	{
		for(int i = 0; i < 3; i++) LAMEXP_DELETE(neroBin[i]);
		qWarning("Failed to get excluive lock to Nero encoder binary -> AAC encoding support will be disabled!");
		return;
	}

	QProcess process;
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.setReadChannel(QProcess::StandardOutput);
	process.start(neroFileInfo[0].canonicalFilePath(), QStringList() << "-help");

	if(!process.waitForStarted())
	{
		qWarning("Nero process failed to create!");
		qWarning("Error message: \"%s\"\n", process.errorString().toLatin1().constData());
		process.kill();
		process.waitForFinished(-1);
		for(int i = 0; i < 3; i++) LAMEXP_DELETE(neroBin[i]);
		return;
	}

	unsigned int neroVersion = 0;

	while(process.state() != QProcess::NotRunning)
	{
		if(!process.waitForReadyRead())
		{
			if(process.state() == QProcess::Running)
			{
				qWarning("Nero process time out -> killing!");
				process.kill();
				process.waitForFinished(-1);
				for(int i = 0; i < 3; i++) LAMEXP_DELETE(neroBin[i]);
				return;
			}
		}

		while(process.canReadLine())
		{
			QString line = QString::fromUtf8(process.readLine().constData()).simplified();
			QStringList tokens = line.split(" ", QString::SkipEmptyParts, Qt::CaseInsensitive);
			int index1 = tokens.indexOf("Package");
			int index2 = tokens.indexOf("version:");
			if(index1 >= 0 && index2 >= 0 && index1 + 1 == index2 && index2 < tokens.count() - 1)
			{
				QStringList versionTokens = tokens.at(index2 + 1).split(".", QString::SkipEmptyParts, Qt::CaseInsensitive);
				if(versionTokens.count() == 4)
				{
					neroVersion = 0;
					neroVersion += min(9, max(0, versionTokens.at(3).toInt()));
					neroVersion += min(9, max(0, versionTokens.at(2).toInt())) * 10;
					neroVersion += min(9, max(0, versionTokens.at(1).toInt())) * 100;
					neroVersion += min(9, max(0, versionTokens.at(0).toInt())) * 1000;
				}
			}
		}
	}

	if(!(neroVersion > 0))
	{
		qWarning("Nero AAC version could not be determined -> AAC encoding support will be disabled!");
		for(int i = 0; i < 3; i++) LAMEXP_DELETE(neroBin[i]);
		return;
	}
	
	for(int i = 0; i < 3; i++)
	{
		lamexp_register_tool(neroFileInfo[i].fileName(), neroBin[i], neroVersion);
	}
}


void InitializationThread::initWmaDec(void)
{
	static const char* wmaDecoderComponentPath = "NCH Software/Components/wmawav/wmawav.exe";

	LockedFile *wmaFileBin = NULL;
	QFileInfo wmaFileInfo = QFileInfo(QString("%1/%2").arg(lamexp_known_folder(lamexp_folder_programfiles), wmaDecoderComponentPath));

	if(!wmaFileInfo.exists())
	{
		qDebug("WMA File Decoder not found -> WMA decoding support will be disabled!\n");
		return;
	}

	try
	{
		wmaFileBin = new LockedFile(wmaFileInfo.canonicalFilePath());
	}
	catch(...)
	{
		qWarning("Failed to get excluive lock to WMA File Decoder binary -> WMA decoding support will be disabled!");
		return;
	}

	QProcess process;
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.setReadChannel(QProcess::StandardOutput);
	process.start(wmaFileInfo.canonicalFilePath(), QStringList());

	if(!process.waitForStarted())
	{
		qWarning("WmaWav process failed to create!");
		qWarning("Error message: \"%s\"\n", process.errorString().toLatin1().constData());
		process.kill();
		process.waitForFinished(-1);
		return;
	}

	bool b_wmaWavFound = false;

	while(process.state() != QProcess::NotRunning)
	{
		if(!process.waitForReadyRead())
		{
			if(process.state() == QProcess::Running)
			{
				qWarning("WmaWav process time out -> killing!");
				process.kill();
				process.waitForFinished(-1);
				return;
			}
		}
		while(process.canReadLine())
		{
			QString line = QString::fromUtf8(process.readLine().constData()).simplified();
			if(line.contains("Usage: wmatowav.exe WMAFileSpec WAVFileSpec", Qt::CaseInsensitive))
			{
				b_wmaWavFound = true;
			}
		}
	}

	if(!b_wmaWavFound)
	{
		qWarning("WmaWav could not be identified -> WMA decoding support will be disabled!\n");
		LAMEXP_DELETE(wmaFileBin);
		return;
	}

	qDebug("Found WMA File Decoder binary:\n%s\n", wmaFileInfo.canonicalFilePath().toUtf8().constData());

	if(wmaFileBin)
	{
		lamexp_register_tool(wmaFileInfo.fileName(), wmaFileBin);
	}
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/