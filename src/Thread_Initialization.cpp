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

#include "LockedFile.h"

#include <QFileInfo>
#include <QCoreApplication>
#include <QProcess>
#include <QMap>
#include <QDir>
#include <QLibrary>
#include <QResource>
#include <QTime>

////////////////////////////////////////////////////////////
// TOOLS
////////////////////////////////////////////////////////////

#define CPU_TYPE_X86 0x000001 //x86-32
#define CPU_TYPE_SSE 0x000002 //x86-32 + SSE2 (Intel only!)
#define CPU_TYPE_X64 0x000004 //x86-64

#define CPU_TYPE_GEN (CPU_TYPE_X86|CPU_TYPE_SSE)              //Use for all CPU's, except for x86-64
#define CPU_TYPE_ALL (CPU_TYPE_X86|CPU_TYPE_SSE|CPU_TYPE_X64) //Use for all CPU's, x86-32 and x86-64

static const struct
{
	char *pcHash;
	unsigned int uiCpuType;
	char *pcName;
	unsigned int uiVersion;
}
g_lamexp_tools[] =
{
	{"0a6252606c1ceda7b8973e5935ef72d60b8fd64d", CPU_TYPE_X86, "aften.i386.exe", 8},
	{"22253052acba92a0088bbf0aa82a8c505c07b854", CPU_TYPE_SSE, "aften.sse2.exe", 8},
	{"2996a48b01b65a2c1806482654beeea7ffcf1f80", CPU_TYPE_X64, "aften.x64.exe",  8},
	{"3b41f85dde8d4a5a0f4cd5f461099d0db24610ba", CPU_TYPE_ALL, "alac.exe", 20},
	{"3e2cce2e090bab7c7d7d54c21c1fe96917698960", CPU_TYPE_ALL, "avs2wav.exe", 12},
	{"fb74ac8b73ad8cba2c3b4e6e61f23401d630dc22", CPU_TYPE_ALL, "elevator.exe", UINT_MAX},
	{"80e372d8b20be24102c18284286fcdf5fa14bd86", CPU_TYPE_ALL, "faad.exe", 27},
	{"d33cd86f04bd4067e244d2804466583c7b90a4e2", CPU_TYPE_ALL, "flac.exe", 121},
	{"9328a50e89b54ec065637496d9681a7e3eebf915", CPU_TYPE_ALL, "gpgv.exe", 1411},
	{"d837bf6ee4dab557d8b02d46c75a24e58980fffa", CPU_TYPE_ALL, "gpgv.gpg", UINT_MAX},
	{"d5b3b80220d85a9fd2f486e37c1fb6511f3c2d72", CPU_TYPE_ALL, "lame.exe", 3990},
	{"a4e929cfaa42fa2e61a3d0c6434c77a06d45aef3", CPU_TYPE_ALL, "mac.exe", 406},
	{"a9aa99209fb9ad6ceb97b7a46774fc97141d3dce", CPU_TYPE_GEN, "mediainfo.i386.exe", 745},
	{"9f6b81378e4c408fe5ded8a26ddd0d6c705d81cb", CPU_TYPE_X64, "mediainfo.x64.exe",  745},
	{"aa89763a5ba4d1a5986549b9ee53e005c51940c1", CPU_TYPE_ALL, "mpcdec.exe", 435},
	{"c327400fcee268f581d8c03e2a5cbbe8031abb6f", CPU_TYPE_ALL, "mpg123.exe", 1133},
	{"8dd7138714c3bcb39f5a3213413addba13d06f1e", CPU_TYPE_ALL, "oggdec.exe", UINT_MAX},
	{"97fed9ab0657baa36b6c7764461d66318654dbe6", CPU_TYPE_X86, "oggenc2.i386.exe", 287603},
	{"e6dc1f31ce822588fa9cf52a341cebffd526546c", CPU_TYPE_SSE, "oggenc2.sse2.exe", 287603},
	{"79aa9da24b2a0728cdf0a6927be316b864595b83", CPU_TYPE_X64, "oggenc2.x64.exe",  287603},
	{"0d9035bb62bdf46a2785261f8be5a4a0972abd15", CPU_TYPE_ALL, "shorten.exe", 361},
	{"50ead3b852cbfc067a402e6c2d0d0d8879663dec", CPU_TYPE_ALL, "sox.exe", 1432},
	{"8671e16497a2d217d3707d4aa418678d02b16bcc", CPU_TYPE_ALL, "speexdec.exe", 12},
	{"093bfdec22872ca99e40183937c88785468be989", CPU_TYPE_ALL, "tta.exe", 21},
	{"8c842eef65248b46fa6cb9a9e5714f575672d999", CPU_TYPE_ALL, "valdec.exe", 31},
	{"62e2805d1b2eb2a4d86a5ca6e6ea58010d05d2a7", CPU_TYPE_ALL, "wget.exe", 1114},
	{"a7e8aad52213e339ad985829722f35eab62be182", CPU_TYPE_ALL, "wupdate.exe", UINT_MAX},
	{"b7d14b3540d24df13119a55d97623a61412de6e3", CPU_TYPE_ALL, "wvunpack.exe", 4601},
	{NULL, NULL, NULL, NULL}
};

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

InitializationThread::InitializationThread(const lamexp_cpu_t *cpuFeatures)
{
	m_bSuccess = false;
	memset(&m_cpuFeatures, 0, sizeof(lamexp_cpu_t));
	
	if(cpuFeatures)
	{
		memcpy(&m_cpuFeatures, cpuFeatures, sizeof(lamexp_cpu_t));
	}
}

////////////////////////////////////////////////////////////
// Thread Main
////////////////////////////////////////////////////////////

void InitializationThread::run()
{
	m_bSuccess = false;
	bool bCustom = false;
	delay();

	//CPU type selection
	unsigned int cpuSupport = m_cpuFeatures.x64 ? CPU_TYPE_X64 : ((m_cpuFeatures.intel && m_cpuFeatures.sse && m_cpuFeatures.sse2) ? CPU_TYPE_SSE : CPU_TYPE_X86);
	
	//Allocate maps
	QMap<QString, QString> mapChecksum;
	QMap<QString, unsigned int> mapVersion;
	QMap<QString, unsigned int> mapCpuType;

	//Init properties
	for(int i = 0; i < INT_MAX; i++)
	{
		if(!g_lamexp_tools[i].pcName && !g_lamexp_tools[i].pcHash && !g_lamexp_tools[i].uiVersion)
		{
			break;
		}
		else if(g_lamexp_tools[i].pcName && g_lamexp_tools[i].pcHash && g_lamexp_tools[i].uiVersion)
		{
			const QString currentTool = QString::fromLatin1(g_lamexp_tools[i].pcName);
			mapChecksum.insert(currentTool, QString::fromLatin1(g_lamexp_tools[i].pcHash));
			mapCpuType.insert(currentTool, g_lamexp_tools[i].uiCpuType);
			mapVersion.insert(currentTool, g_lamexp_tools[i].uiVersion);
		}
		else
		{
			qFatal("Inconsistent checksum data detected. Take care!");
		}
	}

	QDir toolsDir(":/tools/");
	QList<QFileInfo> toolsList = toolsDir.entryInfoList(QStringList("*.*"), QDir::Files, QDir::Name);
	QDir appDir = QDir(QCoreApplication::applicationDirPath()).canonicalPath();

	QTime timer;
	timer.start();
	
	//Extract all files
	while(!toolsList.isEmpty())
	{
		try
		{
			QFileInfo currentTool = toolsList.takeFirst();
			QString toolName = currentTool.fileName().toLower();
			QString toolShortName = QString("%1.%2").arg(currentTool.baseName().toLower(), currentTool.suffix().toLower());
			
			QByteArray toolHash = mapChecksum.take(toolName).toLatin1();
			unsigned int toolCpuType = mapCpuType.take(toolName);
			unsigned int toolVersion = mapVersion.take(toolName);
			
			if(toolHash.size() != 40)
			{
				throw "The required checksum is missing, take care!";
			}
			
			if(toolCpuType & cpuSupport)
			{
				QFileInfo customTool(QString("%1/tools/%2/%3").arg(appDir.canonicalPath(), QString::number(lamexp_version_build()), toolShortName));
				if(customTool.exists() && customTool.isFile())
				{
					bCustom = true;
					qDebug("Setting up file: %s <- %s", toolShortName.toLatin1().constData(), appDir.relativeFilePath(customTool.canonicalFilePath()).toLatin1().constData());
					LockedFile *lockedFile = new LockedFile(customTool.canonicalFilePath());
					lamexp_register_tool(toolShortName, lockedFile, UINT_MAX);
				}
				else
				{
					qDebug("Extracting file: %s -> %s", toolName.toLatin1().constData(),  toolShortName.toLatin1().constData());
					LockedFile *lockedFile = new LockedFile(QString(":/tools/%1").arg(toolName), QString("%1/tool_%2").arg(lamexp_temp_folder2(), toolShortName), toolHash);
					lamexp_register_tool(toolShortName, lockedFile, toolVersion);
				}
			}
		}
		catch(char *errorMsg)
		{
			qFatal("At least one of the required tools could not be initialized:\n%s", errorMsg);
			return;
		}
	}
	
	//Make sure all files were extracted
	if(!mapChecksum.isEmpty())
	{
		qFatal("At least one required tool could not be found:\n%s", toolsDir.filePath(mapChecksum.keys().first()).toLatin1().constData());
		return;
	}
	
	qDebug("All extracted.\n");

	//Clean-up
	mapChecksum.clear();
	mapVersion.clear();
	mapCpuType.clear();
	
	//Using any custom tools?
	if(bCustom)
	{
		qWarning("Warning: Using custom tools, you might encounter unexpected problems!\n");
	}

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
	const QString appPath = QDir(QCoreApplication::applicationDirPath()).canonicalPath();
	
	QFileInfo neroFileInfo[3];
	neroFileInfo[0] = QFileInfo(QString("%1/neroAacEnc.exe").arg(appPath));
	neroFileInfo[1] = QFileInfo(QString("%1/neroAacDec.exe").arg(appPath));
	neroFileInfo[2] = QFileInfo(QString("%1/neroAacTag.exe").arg(appPath));
	
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

	if(!(wmaFileInfo.exists() && wmaFileInfo.isFile()))
	{
		wmaFileInfo.setFile(QString("%1/%2").arg(QDir(QCoreApplication::applicationDirPath()).canonicalPath(), "wmawav.exe"));
	}
	if(!(wmaFileInfo.exists() && wmaFileInfo.isFile()))
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