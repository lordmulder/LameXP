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

#include "Thread_Initialization.h"

#include "Global.h"
#include "LockedFile.h"

#include <QFileInfo>
#include <QCoreApplication>
#include <QProcess>
#include <QMap>
#include <QDir>
#include <QLibrary>
#include <QResource>
#include <QTime>

#include <Windows.h>

////////////////////////////////////////////////////////////
// TOOLS
////////////////////////////////////////////////////////////

static const struct
{
	char *pcHash;
	char *pcName;
	unsigned int uiVersion;
}
g_lamexp_tools[] =
{
	{"3b41f85dde8d4a5a0f4cd5f461099d0db24610ba", "alac.exe", UINT_MAX},
	{"fb74ac8b73ad8cba2c3b4e6e61f23401d630dc22", "elevator.exe", UINT_MAX},
	{"3c647950bccfcc75d0746c0772e7115684be4dc5", "faad.exe", 27},
	{"d33cd86f04bd4067e244d2804466583c7b90a4e2", "flac.exe", 121},
	{"9328a50e89b54ec065637496d9681a7e3eebf915", "gpgv.exe", 1411},
	{"d837bf6ee4dab557d8b02d46c75a24e58980fffa", "gpgv.gpg", UINT_MAX},
	{"62e301a56be4b56fc053710042d58992f25b1773", "lame.exe", 39914},
	{"a4e929cfaa42fa2e61a3d0c6434c77a06d45aef3", "mac.exe", 406},
	{"e83cad851d1f0d13057736d9133767960b5ca514", "mediainfo_i386.exe", 742},
	{"6fb20ea7492fcf984e99957b7a3c5fe4fb06cca2", "mediainfo_x64.exe", 742},
	{"aa89763a5ba4d1a5986549b9ee53e005c51940c1", "mpcdec.exe", 435},
	{"38f81efca6c1eeab0b9dc39d06c2ac750267217f", "mpg123.exe", 1132},
	{"8dd7138714c3bcb39f5a3213413addba13d06f1e", "oggdec.exe", UINT_MAX},
	{"14a99d3b1f0b166dbd68db45196da871e58e14ec", "oggenc2_i386.exe", 287602},
	{"36f8d93ef3df6a420a73a9b5cf02dafdaf4321f0", "oggenc2_sse2.exe", 287602},
	{"87ad1af73e9b9db3da3db645e5c2253cb0c2a2ea", "oggenc2_x64.exe", 287602},
	{"0d9035bb62bdf46a2785261f8be5a4a0972abd15", "shorten.exe", 361},
	{"50ead3b852cbfc067a402e6c2d0d0d8879663dec", "sox.exe", 1432},
	{"8671e16497a2d217d3707d4aa418678d02b16bcc", "speexdec.exe", 12},
	{"093bfdec22872ca99e40183937c88785468be989", "tta.exe", 21},
	{"8c842eef65248b46fa6cb9a9e5714f575672d999", "valdec.exe", 31},
	{"62e2805d1b2eb2a4d86a5ca6e6ea58010d05d2a7", "wget.exe", 1114},
	{"a7e8aad52213e339ad985829722f35eab62be182", "wupdate.exe", UINT_MAX},
	{"b7d14b3540d24df13119a55d97623a61412de6e3", "wvunpack.exe", 4601},
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
	
	QTime timer;
	timer.start();

	//Extract all files
	for(int i = 0; i < toolsList.count(); i++)
	{
		try
		{
			QString toolName = toolsList.at(i).fileName().toLower();
			qDebug("Extracting file: %s", toolName.toLatin1().constData());
			QByteArray toolHash = checksum.take(toolName).toLatin1();
			unsigned int toolVersion = version.take(toolName);
			if(toolHash.size() != 40)
			{
				throw "The required checksum is missing, take care!";
			}
			LockedFile *lockedFile = new LockedFile(QString(":/tools/%1").arg(toolName), QString(lamexp_temp_folder2()).append(QString("/tool_%1").arg(toolName)), toolHash);
			lamexp_register_tool(toolName, lockedFile, toolVersion);
		}
		catch(char *errorMsg)
		{
			qFatal("At least one of the required tools could not be extracted:\n%s", errorMsg);
			return;
		}
	}
	
	if(!checksum.isEmpty())
	{
		qFatal("At least one required tool could not be found:\n%s", toolsDir.filePath(checksum.keys().first()).toLatin1().constData());
		return;
	}
	
	qDebug("All extracted.\n");

	//Check delay
	double delayExtract = static_cast<double>(timer.elapsed()) / 1000.0;
	if(delayExtract > 8.0)
	{
		qWarning("Extracting tools took %.3f seconds -> probably slow realtime virus scanner.", delayExtract);
		qWarning("Please report performance problems to your anti-virus developer !!!\n");
	}

	//Register all translations
	initTranslations();

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
		msleep(25);
	}

	printf("Done\n\n");
}

void InitializationThread::initTranslations(void)
{
	//Search for language files
	QStringList qmFiles = QDir(":/localization").entryList(QStringList() << "LameXP_??.qm", QDir::Files, QDir::Name);

	//Make sure we found at least one translation
	if(qmFiles.count() < 1)
	{
		qFatal("Could not find any translation files!");
		return;
	}

	//Add all available translations
	while(!qmFiles.isEmpty())
	{
		QString langId, langName;
		unsigned int systemId = 0;
		QString qmFile = qmFiles.takeFirst();
		
		QRegExp langIdExp("LameXP_(\\w\\w)\\.qm", Qt::CaseInsensitive);
		if(langIdExp.indexIn(qmFile) >= 0)
		{
			langId = langIdExp.cap(1).toLower();
		}

		QResource langRes = (QString(":/localization/%1.txt").arg(qmFile));
		if(langRes.isValid() && langRes.size() > 0)
		{
			QStringList langInfo = QString::fromUtf8(reinterpret_cast<const char*>(langRes.data()), langRes.size()).simplified().split(",", QString::SkipEmptyParts);
			if(langInfo.count() == 2)
			{
				systemId = langInfo.at(0).toUInt();
				langName = langInfo.at(1);
			}
		}
		
		if(lamexp_translation_register(langId, qmFile, langName, systemId))
		{
			qDebug64("Registering translation: %1 = %2 (%3)", qmFile, langName, QString::number(systemId));
		}
		else
		{
			qWarning("Failed to register: %s", qmFile.toLatin1().constData());
		}
	}

	qDebug("All registered.\n");
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