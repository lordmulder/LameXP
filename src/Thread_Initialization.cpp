///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2017 LoRd_MuldeR <MuldeR2@GMX.de>
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
#include "FileHash.h"
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
#include <QVector>

/* enable custom tools? */
static const bool ENABLE_CUSTOM_TOOLS = true;

/* helper macros */
#define PRINT_CPU_TYPE(X) case X: qDebug("Selected CPU is: " #X)
#define MAKE_REGEXP(STR) (((STR) && ((STR)[0])) ? QRegExp((STR)) : QRegExp())

/* constants */
static const double g_allowedExtractDelay = 12.0;
static const size_t BUFF_SIZE = 512;

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
// BaseTask class
////////////////////////////////////////////////////////////

class BaseTask : public QRunnable
{
public:
	BaseTask(void)
	{
	}

	~BaseTask(void)
	{
	}

	static void clearFlags(void)
	{
		s_exception.fetchAndStoreOrdered(0);
		s_errMsg[0] = char(0);
	}

	static bool getExcept(void)
	{
		return MUTILS_BOOLIFY(s_exception);
	}

	static bool getErrMsg(char *buffer, const size_t buffSize)
	{
		if(s_errMsg[0])
		{
			strncpy_s(buffer, BUFF_SIZE, s_errMsg, _TRUNCATE);
			return true;
		}
		return false;
	}

protected:
	virtual void taskMain(void) = 0;

	void run(void)
	{
		try
		{
			if(!getExcept()) taskMain();
		}
		catch(const std::exception &e)
		{
			if(!s_exception.fetchAndAddOrdered(1))
			{
				strncpy_s(s_errMsg, BUFF_SIZE, e.what(), _TRUNCATE);
			}
			qWarning("OptionalInitTask exception error:\n%s\n\n", e.what());
		}
		catch(...)
		{
			if (!s_exception.fetchAndAddOrdered(1))
			{
				strncpy_s(s_errMsg, BUFF_SIZE, "Unknown exception error!", _TRUNCATE);
			}
			qWarning("OptionalInitTask encountered an unknown exception!");
		}
	}

	static QAtomicInt s_exception;
	static char s_errMsg[BUFF_SIZE];
};

char BaseTask::s_errMsg[BUFF_SIZE] = {'\0'};
QAtomicInt BaseTask::s_exception(0);

////////////////////////////////////////////////////////////
// ExtractorTask class
////////////////////////////////////////////////////////////

class ExtractorTask : public BaseTask
{
public:
	ExtractorTask(QResource *const toolResource, const QDir &appDir, const QString &toolName, const QByteArray &toolHash, const unsigned int toolVersion, const QString &toolTag)
	:
		m_appDir(appDir),
		m_tempPath(MUtils::temp_folder()),
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
	}

	static bool getCustom(void)
	{
		return MUTILS_BOOLIFY(s_custom);
	}

	static void clearFlags(void)
	{
		BaseTask::clearFlags();
		s_custom.fetchAndStoreOrdered(0);
	}

protected:
	void taskMain(void)
	{
		QScopedPointer<LockedFile> lockedFile;
		unsigned int version = m_toolVersion;

		const QFileInfo toolFileInfo(m_toolName);
		const QString   toolShrtName = QString("%1.%2").arg(toolFileInfo.baseName().toLower(), toolFileInfo.suffix().toLower());

		//Try to load a "custom" tool first
		if(ENABLE_CUSTOM_TOOLS)
		{
			const QFileInfo customTool(QString("%1/tools/%2/%3").arg(m_appDir.canonicalPath(), QString::number(lamexp_version_build()), toolShrtName));
			if(customTool.exists() && customTool.isFile())
			{
				qDebug("Setting up file: %s <- %s", toolShrtName.toLatin1().constData(), m_appDir.relativeFilePath(customTool.canonicalFilePath()).toLatin1().constData());
				try
				{
					lockedFile.reset(new LockedFile(customTool.canonicalFilePath()));
					version = UINT_MAX; s_custom.ref();
				}
				catch(std::runtime_error&)
				{
					lockedFile.reset();
				}
			}
		}

		//Try to load the tool from the "cache" next
		if(lockedFile.isNull())
		{
			const QFileInfo chachedTool(QString("%1/cache/%2").arg(m_appDir.canonicalPath(), toolFileInfo.fileName()));
			if(chachedTool.exists() && chachedTool.isFile())
			{
				qDebug("Validating file: %s <- %s", toolShrtName.toLatin1().constData(), m_appDir.relativeFilePath(chachedTool.canonicalFilePath()).toLatin1().constData());
				try
				{
					lockedFile.reset(new LockedFile(chachedTool.canonicalFilePath(), m_toolHash));
				}
				catch(std::runtime_error&)
				{
					lockedFile.reset();
				}
			}
		}

		//If still not initialized, extract tool now!
		if(lockedFile.isNull())
		{
			qDebug("Extracting file: %s -> %s", m_toolName.toLatin1().constData(), toolShrtName.toLatin1().constData());
			lockedFile.reset(new LockedFile(m_toolResource.data(), QString("%1/lxp_%2").arg(m_tempPath, toolShrtName), m_toolHash));
		}

		//Register tool
		lamexp_tools_register(toolShrtName, lockedFile.take(), version, m_toolTag);
	}

private:
	static QAtomicInt         s_custom;
	QScopedPointer<QResource> m_toolResource;
	const QDir                m_appDir;
	const QString             m_tempPath;
	const QString             m_toolName;
	const QByteArray          m_toolHash;
	const unsigned int        m_toolVersion;
	const QString             m_toolTag;
};

QAtomicInt ExtractorTask::s_custom = false;

////////////////////////////////////////////////////////////
// InitAacEncTask class
////////////////////////////////////////////////////////////

class InitAacEncTask : public BaseTask
{
public:
	InitAacEncTask(const aac_encoder_t *const encoder_info)
	:
		m_encoder_info(encoder_info)
	{
	}

	~InitAacEncTask(void)
	{
	}

protected:
	void taskMain(void)
	{
		initAacEncImpl(m_encoder_info->toolName, m_encoder_info->fileNames, m_encoder_info->checkArgs ? (QStringList() << QString::fromLatin1(m_encoder_info->checkArgs)) : QStringList(), m_encoder_info->toolMinVersion, m_encoder_info->verDigits, m_encoder_info->verShift, m_encoder_info->verStr, MAKE_REGEXP(m_encoder_info->regExpVer), MAKE_REGEXP(m_encoder_info->regExpSig));
	}

	static void initAacEncImpl(const char *const toolName, const char *const fileNames[], const QStringList &checkArgs, const quint32 &toolMinVersion, const quint32 &verDigits, const quint32 &verShift, const char *const verStr, QRegExp &regExpVer, QRegExp &regExpSig = QRegExp());

private:
	const aac_encoder_t *const m_encoder_info;
};

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
	const bool haveSSE2 = (m_cpuFeatures.features & MUtils::CPUFetaures::FLAG_SSE) && (m_cpuFeatures.features & MUtils::CPUFetaures::FLAG_SSE2);
	if(haveSSE2 && (m_cpuFeatures.vendor & MUtils::CPUFetaures::VENDOR_INTEL))
	{
		if (m_cpuFeatures.features & MUtils::CPUFetaures::FLAG_AVX)
		{
			cpuSupport = m_cpuFeatures.x64 ? CPU_TYPE_X64_AVX : CPU_TYPE_X86_AVX;
		}
		else
		{
			cpuSupport = m_cpuFeatures.x64 ? CPU_TYPE_X64_SSE : CPU_TYPE_X86_SSE;
		}
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
		PRINT_CPU_TYPE(CPU_TYPE_X86_AVX); break;
		PRINT_CPU_TYPE(CPU_TYPE_X64_GEN); break;
		PRINT_CPU_TYPE(CPU_TYPE_X64_SSE); break;
		PRINT_CPU_TYPE(CPU_TYPE_X64_AVX); break;
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

	QScopedPointer<QThreadPool> pool(new QThreadPool());
	pool->setMaxThreadCount((threadCount > 0) ? threadCount : qBound(2U, cores2threads(m_cpuFeatures.count), 16U));
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
			
		QScopedPointer<QResource> resource(new QResource(QString(":/tools/%1").arg(toolName)));
		if(!(resource->isValid() && resource->data()))
		{
			qFatal("The resource for \"%s\" could not be found!", MUTILS_UTF8(toolName));
			return -1.0;
		}
			
		if(cpuType & cpuSupport)
		{
			pool->start(new ExtractorTask(resource.take(), appDir, toolName, toolHash, version, versInfo));
			continue;
		}
	}

	//Sanity Check
	if(!(queueToolName.isEmpty() && queueChecksum.isEmpty() && queueVersInfo.isEmpty() && queueCpuTypes.isEmpty() && queueVersions.isEmpty()))
	{
		qFatal("Checksum queues *not* empty fater verification completed. Take care!");
	}

	//Wait for extrator threads to finish
	pool->waitForDone();

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
	InitAacEncTask::clearFlags();
	for(size_t i = 0; g_lamexp_aacenc[i].toolName; i++)
	{
		pool->start(new InitAacEncTask(&(g_lamexp_aacenc[i])));
	}
	pool->waitForDone();

	//Make sure initialization finished correctly
	if(InitAacEncTask::getExcept())
	{
		char errorMsg[BUFF_SIZE];
		if(InitAacEncTask::getErrMsg(errorMsg, BUFF_SIZE))
		{
			qFatal("At least one optional component failed to initialize:\n%s", errorMsg);
			return -1.0;
		}
		qFatal("At least one optional component failed to initialize!");
		return -1.0;
	}

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
			QScopedPointer<QResource> langRes(new QResource(langResTemplate.arg(*iter)));
			if(langRes->isValid() && langRes->size() > 0)
			{
				QByteArray data = QByteArray::fromRawData(reinterpret_cast<const char*>(langRes->data()), langRes->size());
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

void InitAacEncTask::initAacEncImpl(const char *const toolName, const char *const fileNames[], const QStringList &checkArgs, const quint32 &toolMinVersion, const quint32 &verDigits, const quint32 &verShift, const char *const verStr, QRegExp &regExpVer, QRegExp &regExpSig)
{
	static const size_t MAX_FILES = 8;
	const QString appPath = QDir(QCoreApplication::applicationDirPath()).canonicalPath();
	
	QFileInfoList fileInfo;
	for(size_t i = 0; fileNames[i] && (fileInfo.count() < MAX_FILES); i++)
	{
		fileInfo.append(QFileInfo(QString("%1/%2").arg(appPath, QString::fromLatin1(fileNames[i]))));
	}
	
	for(QFileInfoList::ConstIterator iter = fileInfo.constBegin(); iter != fileInfo.constEnd(); iter++)
	{
		if(!(iter->exists() && iter->isFile()))
		{
			qDebug("%s encoder binaries not found -> Encoding support will be disabled!\n", toolName);
			return;
		}
		if((iter->suffix().compare("exe", Qt::CaseInsensitive) == 0) && (!MUtils::OS::is_executable_file(iter->canonicalFilePath())))
		{
			qDebug("%s executable is invalid -> %s support will be disabled!\n", MUTILS_UTF8(iter->fileName()), toolName);
			return;
		}
	}

	qDebug("Found %s encoder binary:\n%s\n", toolName, MUTILS_UTF8(fileInfo.first().canonicalFilePath()));

	//Lock the encoder binaries
	QScopedPointer<LockedFile> binaries[MAX_FILES];
	try
	{
		size_t index = 0;
		for(QFileInfoList::ConstIterator iter = fileInfo.constBegin(); iter != fileInfo.constEnd(); iter++)
		{
			binaries[index++].reset(new LockedFile(iter->canonicalFilePath()));
		}
	}
	catch(...)
	{
		qWarning("Failed to get excluive lock to encoder binary -> %s support will be disabled!", toolName);
		return;
	}

	QProcess process;
	MUtils::init_process(process, fileInfo.first().absolutePath());
	process.start(fileInfo.first().canonicalFilePath(), checkArgs);

	if(!process.waitForStarted())
	{
		qWarning("%s process failed to create!", toolName);
		qWarning("Error message: \"%s\"\n", process.errorString().toLatin1().constData());
		process.kill();
		process.waitForFinished(-1);
		return;
	}

	quint32 toolVersion = 0;
	bool sigFound = regExpSig.isEmpty() ? true : false;

	while(process.state() != QProcess::NotRunning)
	{
		if(!process.waitForReadyRead())
		{
			if(process.state() == QProcess::Running)
			{
				qWarning("%s process time out -> killing!", toolName);
				process.kill();
				process.waitForFinished(-1);
				return;
			}
		}
		while(process.canReadLine())
		{
			QString line = QString::fromUtf8(process.readLine().constData()).simplified();
			if((!sigFound) && regExpSig.lastIndexIn(line) >= 0)
			{
				sigFound = true;
				continue;
			}
			if(sigFound && (regExpVer.lastIndexIn(line) >= 0))
			{
				quint32 tmp[8];
				if(MUtils::regexp_parse_uint32(regExpVer, tmp, qMin(verDigits, 8U)))
				{
					toolVersion = 0;
					for(quint32 i = 0; i < verDigits; i++)
					{
						toolVersion = (verShift > 0) ? ((toolVersion * verShift) + qBound(0U, tmp[i], (verShift - 1))) : tmp[i];
					}
				}
			}
		}
	}

	if(toolVersion <= 0)
	{
		qWarning("%s version could not be determined -> Encoding support will be disabled!", toolName);
		return;
	}
	else if(toolVersion < toolMinVersion)
	{
		qWarning("%s version is too much outdated (%s) -> Encoding support will be disabled!", toolName, MUTILS_UTF8(lamexp_version2string(verStr, toolVersion,    "N/A")));
		qWarning("Minimum required %s version currently is: %s\n",                             toolName, MUTILS_UTF8(lamexp_version2string(verStr, toolMinVersion, "N/A")));
		return;
	}

	qDebug("Enabled %s encoder %s.\n", toolName, MUTILS_UTF8(lamexp_version2string(verStr, toolVersion, "N/A")));

	size_t index = 0;
	for(QFileInfoList::ConstIterator iter = fileInfo.constBegin(); iter != fileInfo.constEnd(); iter++)
	{
		lamexp_tools_register(iter->fileName(), binaries[index++].take(), toolVersion);
	}
}

////////////////////////////////////////////////////////////
// Self-Test Function
////////////////////////////////////////////////////////////

void InitializationThread::selfTest(void)
{
	const unsigned int cpu[7] = {CPU_TYPE_X86_GEN, CPU_TYPE_X86_SSE, CPU_TYPE_X86_AVX, CPU_TYPE_X64_GEN, CPU_TYPE_X64_SSE, CPU_TYPE_X64_AVX, 0 };

	unsigned int expectedCount = UINT_MAX;
	for(size_t k = 0; cpu[k]; k++)
	{
		qDebug("[TEST]");
		switch(cpu[k])
		{
			PRINT_CPU_TYPE(CPU_TYPE_X86_GEN); break;
			PRINT_CPU_TYPE(CPU_TYPE_X86_SSE); break;
			PRINT_CPU_TYPE(CPU_TYPE_X86_AVX); break;
			PRINT_CPU_TYPE(CPU_TYPE_X64_GEN); break;
			PRINT_CPU_TYPE(CPU_TYPE_X64_SSE); break;
			PRINT_CPU_TYPE(CPU_TYPE_X64_AVX); break;
		default:
			MUTILS_THROW("CPU support undefined!");
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
					QByteArray hash = FileHash::computeHash(resource);
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
		if (expectedCount != UINT_MAX)
		{
			if (n != expectedCount)
			{
				qFatal("Tool count mismatch for CPU type %u. Should be %u, but got %u !!!", cpu[k], expectedCount, n);
			}
		}
		else
		{
			expectedCount = n; /*remember count*/
		}
		qDebug("Done.\n");
	}
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/
