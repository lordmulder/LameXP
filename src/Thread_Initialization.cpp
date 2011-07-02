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
	{"3b4106d22e34df6327bef222622eb327ea067a40", CPU_TYPE_X86, "aften.i386.exe", 8},
	{"f424e28296851d86a85b3863923009549efaf0f2", CPU_TYPE_SSE, "aften.sse2.exe", 8},
	{"abd48661d8990c63178cd639451d6dfdbd1ef28a", CPU_TYPE_X64, "aften.x64.exe",  8},
	{"30cc614bb576c91eb8439d47596e6dbdc088919c", CPU_TYPE_ALL, "alac.exe", 20},
	{"6a7b9db0cc3e3fc3c87643aaafc8ddd9fef928d7", CPU_TYPE_ALL, "avs2wav.exe", 12},
	{"f4c0a20fbcb6e848207c53e1e29c467d4dc70757", CPU_TYPE_ALL, "elevator.exe", UINT_MAX},
	{"81f0ff6b9b1f906d7c0004cbbddf4dc539903591", CPU_TYPE_ALL, "faad.exe", 27},
	{"a2c85f2d9368097207c2c0faf133152e846e7d4b", CPU_TYPE_ALL, "flac.exe", 121},
	{"ee40b5026935126f596351fbaf1dfada8ef06591", CPU_TYPE_ALL, "gpgv.exe", 1411},
	{"8a7d6de45f589588f9210d2cc988b3108a306b84", CPU_TYPE_ALL, "gpgv.gpg", UINT_MAX},
	{"feba4ae2c50a540652b978aa862cf5c02f1da7c4", CPU_TYPE_ALL, "lame.exe", 3990},
	{"70147c86858b5daea33fa7274f70753d6901b061", CPU_TYPE_ALL, "mac.exe", 406},
	{"7f52d7e5f5136f1a24bec7a0664c6ade6f914a47", CPU_TYPE_GEN, "mediainfo.i386.exe", 745},
	{"a532b2661bfecec44f3ad6354639e2e245beab4f", CPU_TYPE_X64, "mediainfo.x64.exe",  745},
	{"e1d675ae5cdd8fff486a14d6a481b6d5a4fb90e7", CPU_TYPE_ALL, "mpcdec.exe", 435},
	{"d444f2ef6bf94ab3a46ee854934b722bf3d4d16a", CPU_TYPE_ALL, "mpg123.exe", 1133},
	{"1b51ad3b25e6d7bcec5491c948bfacec2b132204", CPU_TYPE_ALL, "oggdec.exe", UINT_MAX},
	{"634cbe6ddb151a1803541664bb6a7089eb8c30ac", CPU_TYPE_X86, "oggenc2.i386.exe", 287603},
	{"33ad609d0b1edf6d7726d85ea930b5c435a39d6a", CPU_TYPE_SSE, "oggenc2.sse2.exe", 287603},
	{"6d50367552ad2a015133f5a83e307bb80b811c06", CPU_TYPE_X64, "oggenc2.x64.exe",  287603},
	{"1f073cc23f7321beafcfc5a8f0ce775b2a01b943", CPU_TYPE_ALL, "shorten.exe", 361},
	{"945a4b40743f4fd39a687986ec02a871d5a658db", CPU_TYPE_ALL, "sox.exe", 1432},
	{"55b6e03238ed7da0b169ff5a4e957a8c0e193cec", CPU_TYPE_ALL, "speexdec.exe", 12},
	{"8b587006515085574c5a9de1efc060018414d481", CPU_TYPE_ALL, "tta.exe", 21},
	{"e8578f1ed9cf6cb636befae2bb3ca5b5fd71bc01", CPU_TYPE_ALL, "valdec.exe", 31},
	{"9440b64b7772078d391a014d7e0cca8a7e58e8c7", CPU_TYPE_ALL, "wget.exe", 1114},
	{"bb5f3f72c927fb26910b22676d68fc08960868dc", CPU_TYPE_ALL, "wupdate.exe", UINT_MAX},
	{"972930436457f0b939223d4d3cc51d1535df6c3d", CPU_TYPE_ALL, "wvunpack.exe", 4601},
	{NULL, NULL, NULL, NULL}
};

static const double g_allowedExtractDelay = 10.0;

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

InitializationThread::InitializationThread(const lamexp_cpu_t *cpuFeatures)
{
	m_bSuccess = false;
	memset(&m_cpuFeatures, 0, sizeof(lamexp_cpu_t));
	m_slowIndicator = false;
	
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
	if(delayExtract > g_allowedExtractDelay)
	{
		m_slowIndicator = true;
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