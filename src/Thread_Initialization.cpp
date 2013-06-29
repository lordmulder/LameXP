///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2013 LoRd_MuldeR <MuldeR2@GMX.de>
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
#include "Tools.h"

#include <QFileInfo>
#include <QCoreApplication>
#include <QProcess>
#include <QMap>
#include <QDir>
#include <QLibrary>
#include <QResource>
#include <QTime>
#include <QTextStream>
#include <QRunnable>
#include <QThreadPool>
#include <QMutex>

/* helper macros */
#define PRINT_CPU_TYPE(X) case X: qDebug("Selected CPU is: " #X)
static const double g_allowedExtractDelay = 12.0;

////////////////////////////////////////////////////////////
// ExtractorTask class
////////////////////////////////////////////////////////////

class ExtractorTask : public QRunnable
{
public:
	ExtractorTask(const QDir &appDir, const QString &toolName, const QString &toolShortName, const QByteArray &toolHash, const unsigned int toolVersion, const QString &toolTag)
	:
		QRunnable(), m_appDir(appDir), m_toolName(toolName), m_toolShortName(toolShortName), m_toolHash(toolHash), m_toolVersion(toolVersion), m_toolTag(toolTag)
	{
		/* Nothing to do */
	}

	static void clearFlags(void)
	{
		s_bAbort = s_bCustom = false;
		s_errMsg[0] = '\0';
	}

	static bool getAbort(void) { return s_bAbort; }
	static bool getCustom(void) { return s_bCustom; }
	static char *const getError(void) { return s_errMsg; }

protected:
	void run(void)
	{
		try
		{
			LockedFile *lockedFile = NULL;
			unsigned int version = m_toolVersion;
			
			if(!s_bAbort)
			{
				QFileInfo customTool(QString("%1/tools/%2/%3").arg(m_appDir.canonicalPath(), QString::number(lamexp_version_build()), m_toolShortName));
				if(customTool.exists() && customTool.isFile())
				{
					qDebug("Setting up file: %s <- %s", m_toolShortName.toLatin1().constData(), m_appDir.relativeFilePath(customTool.canonicalFilePath()).toLatin1().constData());
					lockedFile = new LockedFile(customTool.canonicalFilePath()); version = UINT_MAX; s_bCustom = true;
				}
				else
				{
					qDebug("Extracting file: %s -> %s", m_toolName.toLatin1().constData(), m_toolShortName.toLatin1().constData());
					lockedFile = new LockedFile(QString(":/tools/%1").arg(m_toolName), QString("%1/lxp_%2").arg(lamexp_temp_folder2(), m_toolShortName), m_toolHash);
				}

				if(lockedFile)
				{
					QMutexLocker lock(&s_mutex);
					lamexp_register_tool(m_toolShortName, lockedFile, version, &m_toolTag);
				}
			}
		}
		catch(char *errorMsg)
		{
			qWarning("At least one of the required tools could not be initialized:\n%s", errorMsg);
			if(s_mutex.tryLock())
			{
				if(!s_bAbort) { s_bAbort = true; strncpy_s(s_errMsg, 1024, errorMsg, _TRUNCATE); }
				s_mutex.unlock();
			}
		}
	}

private:
	const QDir m_appDir;
	const QString m_toolName;
	const QString m_toolShortName;
	const QString m_toolTag;
	const QByteArray m_toolHash;
	const unsigned int m_toolVersion;

	static volatile bool s_bAbort;
	static volatile bool s_bCustom;
	static QMutex s_mutex;
	static char s_errMsg[1024];
};

volatile bool ExtractorTask::s_bAbort = false;
volatile bool ExtractorTask::s_bCustom = false;
char ExtractorTask::s_errMsg[1024] = {'\0'};
QMutex ExtractorTask::s_mutex;

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
	delay();

	//CPU type selection
	unsigned int cpuSupport = 0;
	if(m_cpuFeatures.sse && m_cpuFeatures.sse2 && m_cpuFeatures.intel)
	{
		cpuSupport = m_cpuFeatures.x64 ? CPU_TYPE_X64_SSE : CPU_TYPE_X86_SSE;
	}
	else
	{
		cpuSupport = m_cpuFeatures.x64 ? CPU_TYPE_X64_GEN : CPU_TYPE_X86_GEN;
	}

	//Hack to disable x64 on Wine, as x64 binaries won't run under Wine (tested with Wine 1.4 under Ubuntu 12.04 x64)
	if(cpuSupport & CPU_TYPE_X64_ALL)
	{
		//DWORD osVerNo = lamexp_get_os_version();
		//if((HIWORD(osVerNo) == 6) && (LOWORD(osVerNo) == 2))
		if(lamexp_detect_wine())
		{
			qWarning("Running under Wine on a 64-Bit system. Going to disable all x64 support!\n");
			cpuSupport = (cpuSupport == CPU_TYPE_X64_SSE) ? CPU_TYPE_X86_SSE : CPU_TYPE_X86_GEN;
		}
	}

	//Print selected CPU type
	switch(cpuSupport)
	{
		PRINT_CPU_TYPE(CPU_TYPE_X86_GEN); break;
		PRINT_CPU_TYPE(CPU_TYPE_X86_SSE); break;
		PRINT_CPU_TYPE(CPU_TYPE_X64_GEN); break;
		PRINT_CPU_TYPE(CPU_TYPE_X64_SSE); break;
		default: throw "CPU support undefined!";
	}

	//Allocate maps
	QMap<QString, QString> mapChecksum;
	QMap<QString, unsigned int> mapVersion;
	QMap<QString, unsigned int> mapCpuType;
	QMap<QString, QString> mapVersTag;

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
			mapVersTag.insert(currentTool, g_lamexp_tools[i].pcVersTag);
		}
		else
		{
			qFatal("Inconsistent checksum data detected. Take care!");
		}
	}

	QDir toolsDir(":/tools/");
	QList<QFileInfo> toolsList = toolsDir.entryInfoList(QStringList("*.*"), QDir::Files, QDir::Name);
	QDir appDir = QDir(QCoreApplication::applicationDirPath()).canonicalPath();

	QThreadPool *pool = new QThreadPool();
	int idealThreadCount = QThread::idealThreadCount();
	if(idealThreadCount > 0)
	{
		pool->setMaxThreadCount(idealThreadCount * 2);
	}
	
	LockedFile::selfTest();
	ExtractorTask::clearFlags();

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
			QString toolVersTag = mapVersTag.take(toolName);
			
			if(toolHash.size() != 96)
			{
				throw "The required checksum is missing, take care!";
			}
			
			if(toolCpuType & cpuSupport)
			{
				pool->start(new ExtractorTask(appDir, toolName, toolShortName, toolHash, toolVersion, toolVersTag));
				QThread::yieldCurrentThread();
			}
		}
		catch(char *errorMsg)
		{
			qFatal("At least one of the required tools could not be initialized:\n%s", errorMsg);
			return;
		}
	}

	//Wait for extrator threads to finish
	pool->waitForDone();
	LAMEXP_DELETE(pool);

	//Make sure all files were extracted correctly
	if(ExtractorTask::getAbort())
	{
		qFatal("At least one of the required tools could not be initialized:\n%s", ExtractorTask::getError());
		return;
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
	if(ExtractorTask::getCustom())
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
	else
	{
		qDebug("Extracting the tools took %.3f seconds (OK).\n", delayExtract);
	}

	//Register all translations
	initTranslations();

	//Look for AAC encoders
	initNeroAac();
	initFhgAac();
	initQAac();

	delay();
	m_bSuccess = true;
}

////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
////////////////////////////////////////////////////////////

void InitializationThread::delay(void)
{
	const char *temp = "|/-\\";
	printf("Thread is doing something important... ?\b");

	for(int i = 0; i < 20; i++)
	{
		printf("%c\b", temp[i%4]);
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
		unsigned int systemId = 0, country = 0;
		QString qmFile = qmFiles.takeFirst();
		
		QRegExp langIdExp("LameXP_(\\w\\w)\\.qm", Qt::CaseInsensitive);
		if(langIdExp.indexIn(qmFile) >= 0)
		{
			langId = langIdExp.cap(1).toLower();
			QResource langRes = QResource(QString(":/localization/%1.txt").arg(qmFile));
			if(langRes.isValid() && langRes.size() > 0)
			{
				QByteArray data = QByteArray::fromRawData(reinterpret_cast<const char*>(langRes.data()), langRes.size());
				QTextStream stream(&data, QIODevice::ReadOnly);
				stream.setAutoDetectUnicode(false); stream.setCodec("UTF-8");
				while(!stream.atEnd())
				{
					QStringList langInfo = stream.readLine().simplified().split(",", QString::SkipEmptyParts);
					if(langInfo.count() == 3)
					{
						systemId = langInfo.at(0).trimmed().toUInt();
						country  = langInfo.at(1).trimmed().toUInt();
						langName = langInfo.at(2).trimmed();
						break;
					}
				}
			}
		}

		if(!(langId.isEmpty() || langName.isEmpty() || systemId == 0))
		{
			if(lamexp_translation_register(langId, qmFile, langName, systemId, country))
			{
				qDebug("Registering translation: %s = %s (%u) [%u]", qmFile.toUtf8().constData(), langName.toUtf8().constData(), systemId, country);
			}
			else
			{
				qWarning("Failed to register: %s", qmFile.toLatin1().constData());
			}
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
					neroVersion += qMin(9, qMax(0, versionTokens.at(3).toInt()));
					neroVersion += qMin(9, qMax(0, versionTokens.at(2).toInt())) * 10;
					neroVersion += qMin(9, qMax(0, versionTokens.at(1).toInt())) * 100;
					neroVersion += qMin(9, qMax(0, versionTokens.at(0).toInt())) * 1000;
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

void InitializationThread::initFhgAac(void)
{
	const QString appPath = QDir(QCoreApplication::applicationDirPath()).canonicalPath();
	
	QFileInfo fhgFileInfo[5];
	fhgFileInfo[0] = QFileInfo(QString("%1/fhgaacenc.exe").arg(appPath));
	fhgFileInfo[1] = QFileInfo(QString("%1/enc_fhgaac.dll").arg(appPath));
	fhgFileInfo[2] = QFileInfo(QString("%1/nsutil.dll").arg(appPath));
	fhgFileInfo[3] = QFileInfo(QString("%1/libmp4v2.dll").arg(appPath));
	fhgFileInfo[4] = QFileInfo(QString("%1/libsndfile-1.dll").arg(appPath));
	
	bool fhgFilesFound = true;
	for(int i = 0; i < 5; i++)	{ if(!fhgFileInfo[i].exists()) fhgFilesFound = false; }

	//Lock the FhgAacEnc binaries
	if(!fhgFilesFound)
	{
		qDebug("FhgAacEnc binaries not found -> FhgAacEnc support will be disabled!\n");
		return;
	}

	qDebug("Found FhgAacEnc cli_exe:\n%s\n", fhgFileInfo[0].canonicalFilePath().toUtf8().constData());
	qDebug("Found FhgAacEnc enc_dll:\n%s\n", fhgFileInfo[1].canonicalFilePath().toUtf8().constData());

	LockedFile *fhgBin[5];
	for(int i = 0; i < 5; i++) fhgBin[i] = NULL;

	try
	{
		for(int i = 0; i < 5; i++)
		{
			fhgBin[i] = new LockedFile(fhgFileInfo[i].canonicalFilePath());
		}
	}
	catch(...)
	{
		for(int i = 0; i < 5; i++) LAMEXP_DELETE(fhgBin[i]);
		qWarning("Failed to get excluive lock to FhgAacEnc binary -> FhgAacEnc support will be disabled!");
		return;
	}

	QProcess process;
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.setReadChannel(QProcess::StandardOutput);
	process.start(fhgFileInfo[0].canonicalFilePath(), QStringList() << "--version");

	if(!process.waitForStarted())
	{
		qWarning("FhgAacEnc process failed to create!");
		qWarning("Error message: \"%s\"\n", process.errorString().toLatin1().constData());
		process.kill();
		process.waitForFinished(-1);
		for(int i = 0; i < 5; i++) LAMEXP_DELETE(fhgBin[i]);
		return;
	}

	QRegExp fhgAacEncSig("fhgaacenc version (\\d+) by tmkk", Qt::CaseInsensitive);
	unsigned int fhgVersion = 0;

	while(process.state() != QProcess::NotRunning)
	{
		process.waitForReadyRead();
		if(!process.bytesAvailable() && process.state() == QProcess::Running)
		{
			qWarning("FhgAacEnc process time out -> killing!");
			process.kill();
			process.waitForFinished(-1);
			for(int i = 0; i < 5; i++) LAMEXP_DELETE(fhgBin[i]);
			return;
		}
		while(process.bytesAvailable() > 0)
		{
			QString line = QString::fromUtf8(process.readLine().constData()).simplified();
			if(fhgAacEncSig.lastIndexIn(line) >= 0)
			{
				bool ok = false;
				unsigned int temp = fhgAacEncSig.cap(1).toUInt(&ok);
				if(ok) fhgVersion = temp;
			}
		}
	}

	if(!(fhgVersion > 0))
	{
		qWarning("FhgAacEnc version couldn't be determined -> FhgAacEnc support will be disabled!");
		for(int i = 0; i < 5; i++) LAMEXP_DELETE(fhgBin[i]);
		return;
	}
	else if(fhgVersion < lamexp_toolver_fhgaacenc())
	{
		qWarning("FhgAacEnc version is too much outdated (%s) -> FhgAacEnc support will be disabled!", lamexp_version2string("????-??-??", fhgVersion, "N/A").toLatin1().constData());
		qWarning("Minimum required FhgAacEnc version currently is: %s\n", lamexp_version2string("????-??-??", lamexp_toolver_fhgaacenc(), "N/A").toLatin1().constData());
		for(int i = 0; i < 5; i++) LAMEXP_DELETE(fhgBin[i]);
		return;
	}
	
	for(int i = 0; i < 5; i++)
	{
		lamexp_register_tool(fhgFileInfo[i].fileName(), fhgBin[i], fhgVersion);
	}
}

void InitializationThread::initQAac(void)
{
	const QString appPath = QDir(QCoreApplication::applicationDirPath()).canonicalPath();

	QFileInfo qaacFileInfo[2];
	qaacFileInfo[0] = QFileInfo(QString("%1/qaac.exe").arg(appPath));
	qaacFileInfo[1] = QFileInfo(QString("%1/libsoxrate.dll").arg(appPath));
	
	bool qaacFilesFound = true;
	for(int i = 0; i < 2; i++)	{ if(!qaacFileInfo[i].exists()) qaacFilesFound = false; }

	//Lock the QAAC binaries
	if(!qaacFilesFound)
	{
		qDebug("QAAC binaries not found -> QAAC support will be disabled!\n");
		return;
	}

	qDebug("Found QAAC encoder:\n%s\n", qaacFileInfo[0].canonicalFilePath().toUtf8().constData());

	LockedFile *qaacBin[2];
	for(int i = 0; i < 2; i++) qaacBin[i] = NULL;

	try
	{
		for(int i = 0; i < 2; i++)
		{
			qaacBin[i] = new LockedFile(qaacFileInfo[i].canonicalFilePath());
		}
	}
	catch(...)
	{
		for(int i = 0; i < 2; i++) LAMEXP_DELETE(qaacBin[i]);
		qWarning("Failed to get excluive lock to QAAC binary -> QAAC support will be disabled!");
		return;
	}

	QProcess process;
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.setReadChannel(QProcess::StandardOutput);
	process.start(qaacFileInfo[0].canonicalFilePath(), QStringList() << "--check");

	if(!process.waitForStarted())
	{
		qWarning("QAAC process failed to create!");
		qWarning("Error message: \"%s\"\n", process.errorString().toLatin1().constData());
		process.kill();
		process.waitForFinished(-1);
		for(int i = 0; i < 2; i++) LAMEXP_DELETE(qaacBin[i]);
		return;
	}

	QRegExp qaacEncSig("qaac (\\d)\\.(\\d)(\\d)", Qt::CaseInsensitive);
	QRegExp coreEncSig("CoreAudioToolbox (\\d)\\.(\\d)\\.(\\d)\\.(\\d)", Qt::CaseInsensitive);
	unsigned int qaacVersion = 0;
	unsigned int coreVersion = 0;

	while(process.state() != QProcess::NotRunning)
	{
		process.waitForReadyRead();
		if(!process.bytesAvailable() && process.state() == QProcess::Running)
		{
			qWarning("QAAC process time out -> killing!");
			process.kill();
			process.waitForFinished(-1);
		for(int i = 0; i < 2; i++) LAMEXP_DELETE(qaacBin[i]);
			return;
		}
		while(process.bytesAvailable() > 0)
		{
			QString line = QString::fromUtf8(process.readLine().constData()).simplified();
			if(qaacEncSig.lastIndexIn(line) >= 0)
			{
				unsigned int tmp[3] = {0, 0, 0};
				bool ok[3] = {false, false, false};
				tmp[0] = qaacEncSig.cap(1).toUInt(&ok[0]);
				tmp[1] = qaacEncSig.cap(2).toUInt(&ok[1]);
				tmp[2] = qaacEncSig.cap(3).toUInt(&ok[2]);
				if(ok[0] && ok[1] && ok[2])
				{
					qaacVersion = (qBound(0U, tmp[0], 9U) * 100) + (qBound(0U, tmp[1], 9U) * 10) + qBound(0U, tmp[2], 9U);
				}
			}
			if(coreEncSig.lastIndexIn(line) >= 0)
			{
				unsigned int tmp[4] = {0, 0, 0, 0};
				bool ok[4] = {false, false, false, false};
				tmp[0] = coreEncSig.cap(1).toUInt(&ok[0]);
				tmp[1] = coreEncSig.cap(2).toUInt(&ok[1]);
				tmp[2] = coreEncSig.cap(3).toUInt(&ok[2]);
				tmp[3] = coreEncSig.cap(4).toUInt(&ok[3]);
				if(ok[0] && ok[1] && ok[2] && ok[3])
				{
					coreVersion = (qBound(0U, tmp[0], 9U) * 1000) + (qBound(0U, tmp[1], 9U) * 100) + (qBound(0U, tmp[2], 9U) * 10) + qBound(0U, tmp[3], 9U);
				}
			}
		}
	}

	//qDebug("qaac %d, CoreAudioToolbox %d", qaacVersion, coreVersion);

	if(!(qaacVersion > 0))
	{
		qWarning("QAAC version couldn't be determined -> QAAC support will be disabled!");
		for(int i = 0; i < 2; i++) LAMEXP_DELETE(qaacBin[i]);
		return;
	}
	else if(qaacVersion < lamexp_toolver_qaacenc())
	{
		qWarning("QAAC version is too much outdated (%s) -> QAAC support will be disabled!", lamexp_version2string("v?.??", qaacVersion, "N/A").toLatin1().constData());
		qWarning("Minimum required QAAC version currently is: %s.\n", lamexp_version2string("v?.??", lamexp_toolver_qaacenc(), "N/A").toLatin1().constData());
		for(int i = 0; i < 2; i++) LAMEXP_DELETE(qaacBin[i]);
		return;
	}

	if(!(coreVersion > 0))
	{
		qWarning("CoreAudioToolbox version couldn't be determined -> QAAC support will be disabled!");
		for(int i = 0; i < 2; i++) LAMEXP_DELETE(qaacBin[i]);
		return;
	}
	else if(coreVersion < lamexp_toolver_coreaudio())
	{
		qWarning("CoreAudioToolbox version is too much outdated (%s) -> QAAC support will be disabled!", lamexp_version2string("v?.?.?.?", coreVersion, "N/A").toLatin1().constData());
		qWarning("Minimum required CoreAudioToolbox version currently is: %s.\n", lamexp_version2string("v?.??", lamexp_toolver_coreaudio(), "N/A").toLatin1().constData());
		for(int i = 0; i < 2; i++) LAMEXP_DELETE(qaacBin[i]);
		return;
	}

	lamexp_register_tool(qaacFileInfo[0].fileName(), qaacBin[0], qaacVersion);
	lamexp_register_tool(qaacFileInfo[1].fileName(), qaacBin[1], qaacVersion);
}

void InitializationThread::selfTest(void)
{
	const unsigned int cpu[4] = {CPU_TYPE_X86_GEN, CPU_TYPE_X86_SSE, CPU_TYPE_X64_GEN, CPU_TYPE_X64_SSE};

	for(size_t k = 0; k < 4; k++)
	{
		qDebug("[TEST]");
		switch(cpu[k])
		{
			PRINT_CPU_TYPE(CPU_TYPE_X86_GEN); break;
			PRINT_CPU_TYPE(CPU_TYPE_X86_SSE); break;
			PRINT_CPU_TYPE(CPU_TYPE_X64_GEN); break;
			PRINT_CPU_TYPE(CPU_TYPE_X64_SSE); break;
			default: throw "CPU support undefined!";
		}
		int n = 0;
		for(int i = 0; i < INT_MAX; i++)
		{
			if(!g_lamexp_tools[i].pcName && !g_lamexp_tools[i].pcHash && !g_lamexp_tools[i].uiVersion)
			{
				break;
			}
			if(g_lamexp_tools[i].uiCpuType & cpu[k])
			{
				qDebug("%02i -> %s", ++n, g_lamexp_tools[i].pcName);
			}
		}
		if(n != 27)
		{
			qFatal("Tool count mismatch !!!");
		}
		qDebug("Done.\n");
	}
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/