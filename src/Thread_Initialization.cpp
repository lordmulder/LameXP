///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2014 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Thread_Initialization.h"

//Internal
#define LAMEXP_INC_TOOLS 1
#include "Tools.h"
#include "LockedFile.h"
#include "Tool_Abstract.h"

//MUtils
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>
#include <MUtils/Translation.h>
#include <MUtils/Exception.h>

//Qt
#include <QFileInfo>
#include <QCoreApplication>
#include <QProcess>
#include <QMap>
#include <QDir>
#include <QResource>
#include <QTextStream>
#include <QRunnable>
#include <QThreadPool>
#include <QMutex>
#include <QQueue>
#include <QElapsedTimer>

/* helper macros */
#define PRINT_CPU_TYPE(X) case X: qDebug("Selected CPU is: " #X)

/* constants */
static const double g_allowedExtractDelay = 12.0;
static const size_t BUFF_SIZE = 512;
static const size_t EXPECTED_TOOL_COUNT = 28;

/* number of CPU cores -> number of threads */
static unsigned int cores2threads(const unsigned int cores)
{
	static const size_t LUT_LEN = 4;
	
	static const struct
	{
		const unsigned int upperBound;
		const double coeffs[4];
	}
	LUT[LUT_LEN] =
	{
		{  4, { -0.052695810565,  0.158087431694, 4.982841530055, -1.088233151184 } },
		{  8, {  0.042431693989, -0.983442622951, 9.548961748634, -7.176393442623 } },
		{ 12, { -0.006277322404,  0.185573770492, 0.196830601093, 17.762622950820 } },
		{ 32, {  0.000673497268, -0.064655737705, 3.199584699454,  5.751606557377 } }
	};

	size_t index = 0;
	while((cores > LUT[index].upperBound) && (index < (LUT_LEN-1))) index++;

	const double x = qBound(1.0, double(cores), double(LUT[LUT_LEN-1].upperBound));
	const double y = (LUT[index].coeffs[0] * pow(x, 3.0)) + (LUT[index].coeffs[1] * pow(x, 2.0)) + (LUT[index].coeffs[2] * x) + LUT[index].coeffs[3];

	return qRound(abs(y));
}

////////////////////////////////////////////////////////////
// ExtractorTask class
////////////////////////////////////////////////////////////

class ExtractorTask : public QRunnable
{
public:
	ExtractorTask(QResource *const toolResource, const QDir &appDir, const QString &toolName, const QByteArray &toolHash, const unsigned int toolVersion, const QString &toolTag)
	:
		m_appDir(appDir),
		m_toolName(toolName),
		m_toolHash(toolHash),
		m_toolVersion(toolVersion),
		m_toolTag(toolTag),
		m_toolResource(toolResource)
	{
		/* Nothing to do */
	}

	~ExtractorTask(void)
	{
		delete m_toolResource;
	}

	static void clearFlags(void)
	{
		QMutexLocker lock(&s_mutex);
		s_bExcept = false;
		s_bCustom = false;
		s_errMsg[0] = char(0);
	}

	static bool getExcept(void) { bool ret; QMutexLocker lock(&s_mutex); ret = s_bExcept; return ret; }
	static bool getCustom(void) { bool ret; QMutexLocker lock(&s_mutex); ret = s_bCustom; return ret; }

	static bool getErrMsg(char *buffer, const size_t buffSize)
	{
		QMutexLocker lock(&s_mutex);
		if(s_errMsg[0])
		{
			strncpy_s(buffer, BUFF_SIZE, s_errMsg, _TRUNCATE);
			return true;
		}
		return false;
	}

protected:
	void run(void)
	{
		try
		{
			if(!getExcept()) doExtract();
		}
		catch(const std::exception &e)
		{
			QMutexLocker lock(&s_mutex);
			if(!s_bExcept)
			{
				s_bExcept = true;
				strncpy_s(s_errMsg, BUFF_SIZE, e.what(), _TRUNCATE);
			}
			lock.unlock();
			qWarning("ExtractorTask exception error:\n%s\n\n", e.what());
		}
		catch(...)
		{
			QMutexLocker lock(&s_mutex);
			if(!s_bExcept)
			{
				s_bExcept = true;
				strncpy_s(s_errMsg, BUFF_SIZE, "Unknown exception error!", _TRUNCATE);
			}
			lock.unlock();
			qWarning("ExtractorTask encountered an unknown exception!");
		}
	}

	void doExtract(void)
	{
		LockedFile *lockedFile = NULL;
		unsigned int version = m_toolVersion;

		QFileInfo toolFileInfo(m_toolName);
		const QString toolShortName = QString("%1.%2").arg(toolFileInfo.baseName().toLower(), toolFileInfo.suffix().toLower());

		QFileInfo customTool(QString("%1/tools/%2/%3").arg(m_appDir.canonicalPath(), QString::number(lamexp_version_build()), toolShortName));
		if(customTool.exists() && customTool.isFile())
		{
			qDebug("Setting up file: %s <- %s", toolShortName.toLatin1().constData(), m_appDir.relativeFilePath(customTool.canonicalFilePath()).toLatin1().constData());
			lockedFile = new LockedFile(customTool.canonicalFilePath()); version = UINT_MAX; s_bCustom = true;
		}
		else
		{
			qDebug("Extracting file: %s -> %s", m_toolName.toLatin1().constData(), toolShortName.toLatin1().constData());
			lockedFile = new LockedFile(m_toolResource, QString("%1/lxp_%2").arg(MUtils::temp_folder(), toolShortName), m_toolHash);
		}

		if(lockedFile)
		{
			lamexp_tools_register(toolShortName, lockedFile, version, m_toolTag);
		}
	}

private:
	QResource *const m_toolResource;
	const QDir m_appDir;
	const QString m_toolName;
	const QByteArray m_toolHash;
	const unsigned int m_toolVersion;
	const QString m_toolTag;

	static volatile bool s_bExcept;
	static volatile bool s_bCustom;
	static QMutex s_mutex;
	static char s_errMsg[BUFF_SIZE];
};

QMutex ExtractorTask::s_mutex;
char ExtractorTask::s_errMsg[BUFF_SIZE] = {'\0'};
volatile bool ExtractorTask::s_bExcept = false;
volatile bool ExtractorTask::s_bCustom = false;

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

InitializationThread::InitializationThread(const MUtils::CPUFetaures::cpu_info_t &cpuFeatures)
:
	m_bSuccess(false),
	m_slowIndicator(false)
{

	memcpy(&m_cpuFeatures, &cpuFeatures, sizeof(MUtils::CPUFetaures::cpu_info_t));
}

////////////////////////////////////////////////////////////
// Thread Main
////////////////////////////////////////////////////////////

void InitializationThread::run(void)
{
	try
	{
		doInit();
	}
	catch(const std::exception &error)
	{
		MUTILS_PRINT_ERROR("\nGURU MEDITATION !!!\n\nException error:\n%s\n", error.what());
		MUtils::OS::fatal_exit(L"Unhandeled C++ exception error, application will exit!");
	}
	catch(...)
	{
		MUTILS_PRINT_ERROR("\nGURU MEDITATION !!!\n\nUnknown exception error!\n");
		MUtils::OS::fatal_exit(L"Unhandeled C++ exception error, application will exit!");
	}
}

double InitializationThread::doInit(const size_t threadCount)
{
	m_bSuccess = false;
	delay();

	//CPU type selection
	unsigned int cpuSupport = 0;
	if((m_cpuFeatures.features & MUtils::CPUFetaures::FLAG_SSE) && (m_cpuFeatures.features & MUtils::CPUFetaures::FLAG_SSE2) && m_cpuFeatures.intel)
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
		if(MUtils::OS::running_on_wine())
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
		default: MUTILS_THROW("CPU support undefined!");
	}

	//Allocate queues
	QQueue<QString> queueToolName;
	QQueue<QString> queueChecksum;
	QQueue<QString> queueVersInfo;
	QQueue<unsigned int> queueVersions;
	QQueue<unsigned int> queueCpuTypes;

	//Init properties
	for(int i = 0; true; i++)
	{
		if(!(g_lamexp_tools[i].pcName || g_lamexp_tools[i].pcHash  || g_lamexp_tools[i].uiVersion))
		{
			break;
		}
		else if(g_lamexp_tools[i].pcName && g_lamexp_tools[i].pcHash && g_lamexp_tools[i].uiVersion)
		{
			queueToolName.enqueue(QString::fromLatin1(g_lamexp_tools[i].pcName));
			queueChecksum.enqueue(QString::fromLatin1(g_lamexp_tools[i].pcHash));
			queueVersInfo.enqueue(QString::fromLatin1(g_lamexp_tools[i].pcVersTag));
			queueCpuTypes.enqueue(g_lamexp_tools[i].uiCpuType);
			queueVersions.enqueue(g_lamexp_tools[i].uiVersion);
		}
		else
		{
			qFatal("Inconsistent checksum data detected. Take care!");
		}
	}

	QDir appDir = QDir(QCoreApplication::applicationDirPath()).canonicalPath();

	QThreadPool *pool = new QThreadPool();
	pool->setMaxThreadCount((threadCount > 0) ? threadCount : qBound(2U, cores2threads(m_cpuFeatures.count), EXPECTED_TOOL_COUNT));
	/* qWarning("Using %u threads for extraction.", pool->maxThreadCount()); */

	LockedFile::selfTest();
	ExtractorTask::clearFlags();

	//Start the timer
	QElapsedTimer timeExtractStart;
	timeExtractStart.start();
	
	//Extract all files
	while(!(queueToolName.isEmpty() || queueChecksum.isEmpty() || queueVersInfo.isEmpty() || queueCpuTypes.isEmpty() || queueVersions.isEmpty()))
	{
		const QString toolName = queueToolName.dequeue();
		const QString checksum = queueChecksum.dequeue();
		const QString versInfo = queueVersInfo.dequeue();
		const unsigned int cpuType = queueCpuTypes.dequeue();
		const unsigned int version = queueVersions.dequeue();
			
		const QByteArray toolHash(checksum.toLatin1());
		if(toolHash.size() != 96)
		{
			qFatal("The checksum for \"%s\" has an invalid size!", MUTILS_UTF8(toolName));
			return -1.0;
		}
			
		QResource *resource = new QResource(QString(":/tools/%1").arg(toolName));
		if(!(resource->isValid() && resource->data()))
		{
			MUTILS_DELETE(resource);
			qFatal("The resource for \"%s\" could not be found!", MUTILS_UTF8(toolName));
			return -1.0;
		}
			
		if(cpuType & cpuSupport)
		{
			pool->start(new ExtractorTask(resource, appDir, toolName, toolHash, version, versInfo));
			continue;
		}

		MUTILS_DELETE(resource);
	}

	//Sanity Check
	if(!(queueToolName.isEmpty() && queueChecksum.isEmpty() && queueVersInfo.isEmpty() && queueCpuTypes.isEmpty() && queueVersions.isEmpty()))
	{
		qFatal("Checksum queues *not* empty fater verification completed. Take care!");
	}

	//Wait for extrator threads to finish
	pool->waitForDone();
	MUTILS_DELETE(pool);

	//Performance measure
	const double delayExtract = double(timeExtractStart.elapsed()) / 1000.0;
	timeExtractStart.invalidate();

	//Make sure all files were extracted correctly
	if(ExtractorTask::getExcept())
	{
		char errorMsg[BUFF_SIZE];
		if(ExtractorTask::getErrMsg(errorMsg, BUFF_SIZE))
		{
			qFatal("At least one of the required tools could not be initialized:\n%s", errorMsg);
			return -1.0;
		}
		qFatal("At least one of the required tools could not be initialized!");
		return -1.0;
	}

	qDebug("All extracted.\n");

	//Using any custom tools?
	if(ExtractorTask::getCustom())
	{
		qWarning("Warning: Using custom tools, you might encounter unexpected problems!\n");
	}

	//Check delay
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
	initAacEnc_Nero();
	initAacEnc_FHG();
	initAacEnc_QAAC();

	m_bSuccess = true;
	delay();

	return delayExtract;
}

////////////////////////////////////////////////////////////
// INTERNAL FUNCTIONS
////////////////////////////////////////////////////////////

void InitializationThread::delay(void)
{
	MUtils::OS::sleep_ms(333);
}

////////////////////////////////////////////////////////////
// Translation Support
////////////////////////////////////////////////////////////

void InitializationThread::initTranslations(void)
{
	//Search for language files
	const QDir qmDirectory(":/localization");
	const QStringList qmFiles = qmDirectory.entryList(QStringList() << "LameXP_??.qm", QDir::Files, QDir::Name);

	//Make sure we found at least one translation
	if(qmFiles.count() < 1)
	{
		qFatal("Could not find any translation files!");
		return;
	}

	//Initialize variables
	const QString langResTemplate(":/localization/%1.txt");
	QRegExp langIdExp("^LameXP_(\\w\\w)\\.qm$", Qt::CaseInsensitive);

	//Add all available translations
	for(QStringList::ConstIterator iter = qmFiles.constBegin(); iter != qmFiles.constEnd(); iter++)
	{
		const QString langFile = qmDirectory.absoluteFilePath(*iter);
		QString langId, langName;
		unsigned int systemId = 0, country = 0;
		
		if(QFileInfo(langFile).isFile() && (langIdExp.indexIn(*iter) >= 0))
		{
			langId = langIdExp.cap(1).toLower();
			QResource langRes = QResource(langResTemplate.arg(*iter));
			if(langRes.isValid() && langRes.size() > 0)
			{
				QByteArray data = QByteArray::fromRawData(reinterpret_cast<const char*>(langRes.data()), langRes.size());
				QTextStream stream(&data, QIODevice::ReadOnly);
				stream.setAutoDetectUnicode(false); stream.setCodec("UTF-8");

				while(!(stream.atEnd() || (stream.status() != QTextStream::Ok)))
				{
					QStringList langInfo = stream.readLine().simplified().split(",", QString::SkipEmptyParts);
					if(langInfo.count() >= 3)
					{
						systemId = langInfo.at(0).trimmed().toUInt();
						country  = langInfo.at(1).trimmed().toUInt();
						langName = langInfo.at(2).trimmed();
						break;
					}
				}
			}
		}

		if(!(langId.isEmpty() || langName.isEmpty() || (systemId == 0)))
		{
			if(MUtils::Translation::insert(langId, langFile, langName, systemId, country))
			{
				qDebug("Registering translation: %s = %s (%u) [%u]", MUTILS_UTF8(*iter), MUTILS_UTF8(langName), systemId, country);
			}
			else
			{
				qWarning("Failed to register: %s", langFile.toLatin1().constData());
			}
		}
	}

	qDebug("All registered.\n");
}

////////////////////////////////////////////////////////////
// AAC Encoder Detection
////////////////////////////////////////////////////////////

void InitializationThread::initAacEnc_Nero(void)
{
	const QString appPath = QDir(QCoreApplication::applicationDirPath()).canonicalPath();
	
	const QFileInfo neroFileInfo[3] =
	{
		QFileInfo(QString("%1/neroAacEnc.exe").arg(appPath)),
		QFileInfo(QString("%1/neroAacDec.exe").arg(appPath)),
		QFileInfo(QString("%1/neroAacTag.exe").arg(appPath))
	};
	
	bool neroFilesFound = true;
	for(int i = 0; i < 3; i++)
	{
		if(!(neroFileInfo[i].exists() && neroFileInfo[i].isFile()))
		{
			neroFilesFound = false;
		}
	}

	if(!neroFilesFound)
	{
		qDebug("Nero encoder binaries not found -> NeroAAC encoding support will be disabled!\n");
		return;
	}

	for(int i = 0; i < 3; i++)
	{
		if(!MUtils::OS::is_executable_file(neroFileInfo[i].canonicalFilePath()))
		{
			qDebug("%s executbale is invalid -> NeroAAC encoding support will be disabled!\n", MUTILS_UTF8(neroFileInfo[i].fileName()));
			return;
		}
	}

	qDebug("Found Nero AAC encoder binary:\n%s\n", MUTILS_UTF8(neroFileInfo[0].canonicalFilePath()));

	//Lock the Nero binaries
	QScopedPointer<LockedFile> neroBin[3];
	try
	{
		for(int i = 0; i < 3; i++)
		{
			neroBin[i].reset(new LockedFile(neroFileInfo[i].canonicalFilePath()));
		}
	}
	catch(...)
	{
		qWarning("Failed to get excluive lock to Nero encoder binary -> NeroAAC encoding support will be disabled!");
		return;
	}

	QProcess process;
	MUtils::init_process(process, neroFileInfo[0].absolutePath());

	process.start(neroFileInfo[0].canonicalFilePath(), QStringList() << "-help");

	if(!process.waitForStarted())
	{
		qWarning("Nero process failed to create!");
		qWarning("Error message: \"%s\"\n", process.errorString().toLatin1().constData());
		process.kill();
		process.waitForFinished(-1);
		return;
	}

	bool neroSigFound = false;
	quint32 neroVersion = 0;

	QRegExp neroAacEncSig("Nero\\s+AAC\\s+Encoder", Qt::CaseInsensitive);
	QRegExp neroAacEncVer("Package\\s+version:\\s+(\\d)\\.(\\d)\\.(\\d)\\.(\\d)", Qt::CaseInsensitive);

	while(process.state() != QProcess::NotRunning)
	{
		if(!process.waitForReadyRead())
		{
			if(process.state() == QProcess::Running)
			{
				qWarning("NeroAAC process time out -> killing!");
				process.kill();
				process.waitForFinished(-1);
				return;
			}
		}
		while(process.canReadLine())
		{
			QString line = QString::fromUtf8(process.readLine().constData()).simplified();
			if(neroAacEncSig.lastIndexIn(line) >= 0)
			{
				neroSigFound = true;
				continue;
			}
			if(neroSigFound && (neroAacEncVer.lastIndexIn(line) >= 0))
			{
				quint32 tmp[4];
				if(MUtils::regexp_parse_uint32(neroAacEncVer, tmp, 4))
				{
					neroVersion = (qBound(0U, tmp[0], 9U) * 1000U) + (qBound(0U, tmp[1], 9U) * 100U) + (qBound(0U, tmp[2], 9U) * 10U) + qBound(0U, tmp[3], 9U);
				}
			}
		}
	}

	if(neroVersion <= 0)
	{
		qWarning("NeroAAC version could not be determined -> NeroAAC encoding support will be disabled!");
		return;
	}
	else if(neroVersion < lamexp_toolver_neroaac())
	{
		qWarning("NeroAAC version is too much outdated (%s) -> NeroAAC support will be disabled!", MUTILS_UTF8(lamexp_version2string("v?.?.?.?", neroVersion,              "N/A")));
		qWarning("Minimum required NeroAAC version currently is: %s\n",                            MUTILS_UTF8(lamexp_version2string("v?.?.?.?", lamexp_toolver_neroaac(), "N/A")));
		return;
	}

	qDebug("Enabled NeroAAC encoder %s.\n", MUTILS_UTF8(lamexp_version2string("v?.?.?.?", neroVersion, "N/A")));

	for(int i = 0; i < 3; i++)
	{
		lamexp_tools_register(neroFileInfo[i].fileName(), neroBin[i].take(), neroVersion);
	}
}

void InitializationThread::initAacEnc_FHG(void)
{
	const QString appPath = QDir(QCoreApplication::applicationDirPath()).canonicalPath();
	
	const QFileInfo fhgFileInfo[5] =
	{
		QFileInfo(QString("%1/fhgaacenc.exe")   .arg(appPath)),
		QFileInfo(QString("%1/enc_fhgaac.dll")  .arg(appPath)),
		QFileInfo(QString("%1/nsutil.dll")      .arg(appPath)),
		QFileInfo(QString("%1/libmp4v2.dll")    .arg(appPath)),
		QFileInfo(QString("%1/libsndfile-1.dll").arg(appPath))
	};
	
	bool fhgFilesFound = true;
	for(int i = 0; i < 5; i++)
	{
		if(!(fhgFileInfo[i].exists() && fhgFileInfo[i].isFile()))
		{
			fhgFilesFound = false;
		}
	}

	if(!fhgFilesFound)
	{
		qDebug("FhgAacEnc binaries not found -> FhgAacEnc support will be disabled!\n");
		return;
	}

	if(!MUtils::OS::is_executable_file(fhgFileInfo[0].canonicalFilePath()))
	{
		qDebug("FhgAacEnc executbale is invalid -> FhgAacEnc support will be disabled!\n");
		return;
	}

	qDebug("Found FhgAacEnc cli_exe:\n%s\n", MUTILS_UTF8(fhgFileInfo[0].canonicalFilePath()));
	qDebug("Found FhgAacEnc enc_dll:\n%s\n", MUTILS_UTF8(fhgFileInfo[1].canonicalFilePath()));

	//Lock the FhgAacEnc binaries
	QScopedPointer<LockedFile> fhgBin[5];
	try
	{
		for(int i = 0; i < 5; i++)
		{
			fhgBin[i].reset(new LockedFile(fhgFileInfo[i].canonicalFilePath()));
		}
	}
	catch(...)
	{
		qWarning("Failed to get excluive lock to FhgAacEnc binary -> FhgAacEnc support will be disabled!");
		return;
	}

	QProcess process;
	MUtils::init_process(process, fhgFileInfo[0].absolutePath());

	process.start(fhgFileInfo[0].canonicalFilePath(), QStringList() << "--version");

	if(!process.waitForStarted())
	{
		qWarning("FhgAacEnc process failed to create!");
		qWarning("Error message: \"%s\"\n", process.errorString().toLatin1().constData());
		process.kill();
		process.waitForFinished(-1);
		return;
	}

	quint32 fhgVersion = 0;
	QRegExp fhgAacEncSig("fhgaacenc version (\\d+) by tmkk", Qt::CaseInsensitive);

	while(process.state() != QProcess::NotRunning)
	{
		process.waitForReadyRead();
		if(!process.bytesAvailable() && process.state() == QProcess::Running)
		{
			qWarning("FhgAacEnc process time out -> killing!");
			process.kill();
			process.waitForFinished(-1);
			return;
		}
		while(process.bytesAvailable() > 0)
		{
			QString line = QString::fromUtf8(process.readLine().constData()).simplified();
			if(fhgAacEncSig.lastIndexIn(line) >= 0)
			{
				quint32 tmp;
				if(MUtils::regexp_parse_uint32(fhgAacEncSig, tmp))
				{
					fhgVersion = tmp;
				}
			}
		}
	}

	if(fhgVersion <= 0)
	{
		qWarning("FhgAacEnc version couldn't be determined -> FhgAacEnc support will be disabled!");
		return;
	}
	else if(fhgVersion < lamexp_toolver_fhgaacenc())
	{
		qWarning("FhgAacEnc version is too much outdated (%s) -> FhgAacEnc support will be disabled!", MUTILS_UTF8(lamexp_version2string("????-??-??", fhgVersion,                 "N/A")));
		qWarning("Minimum required FhgAacEnc version currently is: %s\n",                              MUTILS_UTF8(lamexp_version2string("????-??-??", lamexp_toolver_fhgaacenc(), "N/A")));
		return;
	}
	
	qDebug("Enabled FhgAacEnc %s.\n", MUTILS_UTF8(lamexp_version2string("????-??-??", fhgVersion, "N/A")));

	for(int i = 0; i < 5; i++)
	{
		lamexp_tools_register(fhgFileInfo[i].fileName(), fhgBin[i].take(), fhgVersion);
	}
}

void InitializationThread::initAacEnc_QAAC(void)
{
	const QString appPath = QDir(QCoreApplication::applicationDirPath()).canonicalPath();

	const QFileInfo qaacFileInfo[3] =
	{
		QFileInfo(QString("%1/qaac.exe")           .arg(appPath)),
		QFileInfo(QString("%1/libsoxr.dll")        .arg(appPath)),
		QFileInfo(QString("%1/libsoxconvolver.dll").arg(appPath))
	};
	
	bool qaacFilesFound = true;
	for(int i = 0; i < 3; i++)
	{
		if(!(qaacFileInfo[i].exists() && qaacFileInfo[i].isFile()))
		{
			qaacFilesFound = false;
		}
	}

	if(!qaacFilesFound)
	{
		qDebug("QAAC binary or companion DLL's not found -> QAAC support will be disabled!\n");
		return;
	}

	if(!MUtils::OS::is_executable_file(qaacFileInfo[0].canonicalFilePath()))
	{
		qDebug("QAAC executbale is invalid -> QAAC support will be disabled!\n");
		return;
	}

	qDebug("Found QAAC encoder:\n%s\n", MUTILS_UTF8(qaacFileInfo[0].canonicalFilePath()));

	//Lock the required QAAC binaries
	QScopedPointer<LockedFile> qaacBin[3];
	try
	{
		for(int i = 0; i < 3; i++)
		{
			qaacBin[i].reset(new LockedFile(qaacFileInfo[i].canonicalFilePath()));
		}
	}
	catch(...)
	{
		qWarning("Failed to get excluive lock to QAAC binary -> QAAC support will be disabled!");
		return;
	}

	QProcess process;
	MUtils::init_process(process, qaacFileInfo[0].absolutePath());
	process.start(qaacFileInfo[0].canonicalFilePath(), QStringList() << "--check");

	if(!process.waitForStarted())
	{
		qWarning("QAAC process failed to create!");
		qWarning("Error message: \"%s\"\n", process.errorString().toLatin1().constData());
		process.kill();
		process.waitForFinished(-1);
		return;
	}

	QRegExp qaacEncSig("qaac (\\d)\\.(\\d+)", Qt::CaseInsensitive);
	QRegExp coreEncSig("CoreAudioToolbox (\\d)\\.(\\d)\\.(\\d)\\.(\\d)", Qt::CaseInsensitive);
	QRegExp soxrEncSig("libsoxr-\\d\\.\\d\\.\\d", Qt::CaseInsensitive);
	QRegExp soxcEncSig("libsoxconvolver \\d\\.\\d\\.\\d", Qt::CaseInsensitive);

	quint32 qaacVersion = 0;
	quint32 coreVersion = 0;
	bool soxrFound = false;
	bool soxcFound = false;

	while(process.state() != QProcess::NotRunning)
	{
		process.waitForReadyRead();
		if(!process.bytesAvailable() && process.state() == QProcess::Running)
		{
			qWarning("QAAC process time out -> killing!");
			process.kill();
			process.waitForFinished(-1);
			return;
		}
		while(process.bytesAvailable() > 0)
		{
			QString line = QString::fromUtf8(process.readLine().constData()).simplified();
			if(qaacEncSig.lastIndexIn(line) >= 0)
			{
				quint32 tmp[2];
				if(MUtils::regexp_parse_uint32(qaacEncSig, tmp, 2))
				{
					qaacVersion = (qBound(0U, tmp[0], 9U) * 100) +  qBound(0U, tmp[1], 99U);
				}
			}
			if(coreEncSig.lastIndexIn(line) >= 0)
			{
				quint32 tmp[4];
				if(MUtils::regexp_parse_uint32(coreEncSig, tmp, 4))
				{
					coreVersion = (qBound(0U, tmp[0], 9U) * 1000U) + (qBound(0U, tmp[1], 9U) * 100U) + (qBound(0U, tmp[2], 9U) * 10U) + qBound(0U, tmp[3], 9U);
				}
			}
			if(soxcEncSig.lastIndexIn(line) >= 0)
			{
				soxcFound = true;
			}
			if(soxrEncSig.lastIndexIn(line) >= 0)
			{
				soxrFound = true;
			}
		}
	}

	if(qaacVersion <= 0)
	{
		qWarning("QAAC version couldn't be determined -> QAAC support will be disabled!");
		return;
	}
	else if(qaacVersion < lamexp_toolver_qaacenc())
	{
		qWarning("QAAC version is too much outdated (%s) -> QAAC support will be disabled!", MUTILS_UTF8(lamexp_version2string("v?.??", qaacVersion,              "N/A")));
		qWarning("Minimum required QAAC version currently is: %s.\n",                        MUTILS_UTF8(lamexp_version2string("v?.??", lamexp_toolver_qaacenc(), "N/A")));
		return;
	}

	if(coreVersion <= 0)
	{
		qWarning("CoreAudioToolbox version couldn't be determined -> QAAC support will be disabled!");
		return;
	}
	else if(coreVersion < lamexp_toolver_coreaudio())
	{
		qWarning("CoreAudioToolbox version is outdated (%s) -> QAAC support will be disabled!", MUTILS_UTF8(lamexp_version2string("v?.?.?.?", coreVersion,                "N/A")));
		qWarning("Minimum required CoreAudioToolbox version currently is: %s.\n",               MUTILS_UTF8(lamexp_version2string("v?.?.?.?", lamexp_toolver_coreaudio(), "N/A")));
		return;
	}

	if(!(soxrFound && soxcFound))
	{
		qWarning("libsoxr and/or libsoxconvolver not available -> QAAC support will be disabled!\n");
		return;
	}

	qDebug("Enabled qaac encoder %s (using CoreAudioToolbox %s).\n", MUTILS_UTF8(lamexp_version2string("v?.??", qaacVersion, "N/A")), MUTILS_UTF8(lamexp_version2string("v?.?.?.?", coreVersion, "N/A")));

	for(int i = 0; i < 3; i++)
	{
		lamexp_tools_register(qaacFileInfo[i].fileName(), qaacBin[i].take(), qaacVersion);
	}
}

////////////////////////////////////////////////////////////
// Self-Test Function
////////////////////////////////////////////////////////////

void InitializationThread::selfTest(void)
{
	const unsigned int cpu[4] = {CPU_TYPE_X86_GEN, CPU_TYPE_X86_SSE, CPU_TYPE_X64_GEN, CPU_TYPE_X64_SSE};

	LockedFile::selfTest();

	for(size_t k = 0; k < 4; k++)
	{
		qDebug("[TEST]");
		switch(cpu[k])
		{
			PRINT_CPU_TYPE(CPU_TYPE_X86_GEN); break;
			PRINT_CPU_TYPE(CPU_TYPE_X86_SSE); break;
			PRINT_CPU_TYPE(CPU_TYPE_X64_GEN); break;
			PRINT_CPU_TYPE(CPU_TYPE_X64_SSE); break;
			default: MUTILS_THROW("CPU support undefined!");
		}
		unsigned int n = 0;
		for(int i = 0; true; i++)
		{
			if(!(g_lamexp_tools[i].pcName || g_lamexp_tools[i].pcHash  || g_lamexp_tools[i].uiVersion))
			{
				break;
			}
			else if(g_lamexp_tools[i].pcName && g_lamexp_tools[i].pcHash && g_lamexp_tools[i].uiVersion)
			{
				const QString toolName = QString::fromLatin1(g_lamexp_tools[i].pcName);
				const QByteArray expectedHash = QByteArray(g_lamexp_tools[i].pcHash);
				if(g_lamexp_tools[i].uiCpuType & cpu[k])
				{
					qDebug("%02i -> %s", ++n, MUTILS_UTF8(toolName));
					QFile resource(QString(":/tools/%1").arg(toolName));
					if(!resource.open(QIODevice::ReadOnly))
					{
						qFatal("The resource for \"%s\" could not be opened!", MUTILS_UTF8(toolName));
						break;
					}
					QByteArray hash = LockedFile::fileHash(resource);
					if(hash.isNull() || _stricmp(hash.constData(), expectedHash.constData()))
					{
						qFatal("Hash check for tool \"%s\" has failed!", MUTILS_UTF8(toolName));
						break;
					}
					resource.close();
				}
			}
			else
			{
				qFatal("Inconsistent checksum data detected. Take care!");
			}
		}
		if(n != EXPECTED_TOOL_COUNT)
		{
			qFatal("Tool count mismatch for CPU type %u !!!", cpu[k]);
		}
		qDebug("Done.\n");
	}
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/
