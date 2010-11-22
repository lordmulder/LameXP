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

#include "Global.h"

//Qt includes
#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QUuid>
#include <QMap>
#include <QDate>
#include <QIcon>
#include <QPlastiqueStyle>
#include <QImageReader>
#include <QSharedMemory>
#include <QSysInfo>
#include <QStringList>
#include <QSystemSemaphore>
#include <QMutex>
#include <QTextCodec>

//LameXP includes
#include "Resource.h"
#include "LockedFile.h"

//CRT includes
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <intrin.h>

//Debug only includes
#ifdef _DEBUG
#include <Psapi.h>
#endif //_DEBUG

//Static build only macros
#ifdef QT_NODLL
#pragma warning(disable:4101)
#define LAMEXP_INIT_QT_STATIC_PLUGIN(X) Q_IMPORT_PLUGIN(X)
#else
#define LAMEXP_INIT_QT_STATIC_PLUGIN(X)
#endif

///////////////////////////////////////////////////////////////////////////////
// TYPES
///////////////////////////////////////////////////////////////////////////////

typedef struct
{
	unsigned int command;
	unsigned int reserved_1;
	unsigned int reserved_2;
	char parameter[4096];
} lamexp_ipc_t;

///////////////////////////////////////////////////////////////////////////////
// GLOBAL VARS
///////////////////////////////////////////////////////////////////////////////

//Build version
static const unsigned int g_lamexp_version_major = VER_LAMEXP_MAJOR;
static const unsigned int g_lamexp_version_minor = VER_LAMEXP_MINOR;
static const unsigned int g_lamexp_version_build = VER_LAMEXP_BUILD;
static const char *g_lamexp_version_release = VER_LAMEXP_SUFFIX_STR;

//Build date
static QDate g_lamexp_version_date;
static const char *g_lamexp_months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static const char *g_lamexp_version_raw_date = __DATE__;

//Compiler version
#if _MSC_VER == 1400
	static const char *g_lamexp_version_compiler = "MSVC 8.0";
#else
	#if _MSC_VER == 1500
		static const char *g_lamexp_version_compiler = "MSVC 9.0";
	#else
		#if _MSC_VER == 1600
			static const char *g_lamexp_version_compiler = "MSVC 10.0";
		#else
			static const char *g_lamexp_version_compiler = "UNKNOWN";
		#endif
	#endif
#endif

//Tool versions (expected)
static const unsigned int g_lamexp_toolver_neroaac = VER_LAMEXP_TOOL_NEROAAC;

//Special folders
static QString g_lamexp_temp_folder;

//Tools
static QMap<QString, LockedFile*> g_lamexp_tool_registry;
static QMap<QString, unsigned int> g_lamexp_tool_versions;

//Shared memory
static const char *g_lamexp_sharedmem_uuid = "{21A68A42-6923-43bb-9CF6-64BF151942EE}";
static QSharedMemory *g_lamexp_sharedmem_ptr = NULL;
static const char *g_lamexp_semaphore_read_uuid = "{7A605549-F58C-4d78-B4E5-06EFC34F405B}";
static QSystemSemaphore *g_lamexp_semaphore_read_ptr = NULL;
static const char *g_lamexp_semaphore_write_uuid = "{60AA8D04-F6B8-497d-81EB-0F600F4A65B5}";
static QSystemSemaphore *g_lamexp_semaphore_write_ptr = NULL;

//Image formats
static const char *g_lamexp_imageformats[] = {"png", "gif", "ico", "svg", NULL};

//Global locks
static QMutex g_lamexp_message_mutex;

///////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

/*
 * Version getters
 */
unsigned int lamexp_version_major(void) { return g_lamexp_version_major; }
unsigned int lamexp_version_minor(void) { return g_lamexp_version_minor; }
unsigned int lamexp_version_build(void) { return g_lamexp_version_build; }
const char *lamexp_version_release(void) { return g_lamexp_version_release; }
const char *lamexp_version_compiler(void) {return g_lamexp_version_compiler; }
unsigned int lamexp_toolver_neroaac(void) { return g_lamexp_toolver_neroaac; }

bool lamexp_version_demo(void)
{ 

	return LAMEXP_DEBUG || !(strstr(g_lamexp_version_release, "Final") || strstr(g_lamexp_version_release, "Hotfix"));
}

/*
 * Get build date date
 */
const QDate &lamexp_version_date(void)
{
	if(!g_lamexp_version_date.isValid())
	{
		char temp[32];
		int date[3];

		char *this_token = NULL;
		char *next_token = NULL;

		strcpy_s(temp, 32, g_lamexp_version_raw_date);
		this_token = strtok_s(temp, " ", &next_token);

		for(int i = 0; i < 3; i++)
		{
			date[i] = -1;
			if(this_token)
			{
				for(int j = 0; j < 12; j++)
				{
					if(!_strcmpi(this_token, g_lamexp_months[j]))
					{
						date[i] = j+1;
						break;
					}
				}
				if(date[i] < 0)
				{
					date[i] = atoi(this_token);
				}
				this_token = strtok_s(NULL, " ", &next_token);
			}
		}

		if(date[0] >= 0 && date[1] >= 0 && date[2] >= 0)
		{
			g_lamexp_version_date = QDate(date[2], date[0], date[1]);
		}
	}

	return g_lamexp_version_date;
}

/*
 * Qt message handler
 */
void lamexp_message_handler(QtMsgType type, const char *msg)
{
	static HANDLE hConsole = NULL;
	QMutexLocker lock(&g_lamexp_message_mutex);

	if(!hConsole)
	{
		hConsole = CreateFile(L"CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
		if(hConsole == INVALID_HANDLE_VALUE) hConsole = NULL;
	}

	CONSOLE_SCREEN_BUFFER_INFO bufferInfo;
	GetConsoleScreenBufferInfo(hConsole, &bufferInfo);
	SetConsoleOutputCP(CP_UTF8);

	switch(type)
	{
	case QtCriticalMsg:
	case QtFatalMsg:
		fflush(stdout);
		fflush(stderr);
		SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
		fwprintf(stderr, L"\nCRITICAL ERROR !!!\n%S\n\n", msg);
		MessageBoxW(NULL, (wchar_t*) QString::fromUtf8(msg).utf16(), L"LameXP - CRITICAL ERROR", MB_ICONERROR | MB_TOPMOST | MB_TASKMODAL);
		break;
	case QtWarningMsg:
		SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
		//MessageBoxW(NULL, (wchar_t*) QString::fromUtf8(msg).utf16(), L"LameXP - CRITICAL ERROR", MB_ICONWARNING | MB_TOPMOST | MB_TASKMODAL);
		fwprintf(stderr, L"%S\n", msg);
		fflush(stderr);
		break;
	default:
		SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
		fwprintf(stderr, L"%S\n", msg);
		fflush(stderr);
		break;
	}

	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);

	if(type == QtCriticalMsg || type == QtFatalMsg)
	{
		lock.unlock();
		FatalAppExit(0, L"The application has encountered a critical error and will exit now!");
		TerminateProcess(GetCurrentProcess(), -1);
	}
 }

/*
 * Initialize the console
 */
void lamexp_init_console(int argc, char* argv[])
{
	for(int i = 0; i < argc; i++)
	{
		if(lamexp_version_demo() || !_stricmp(argv[i], "--console"))
		{
			if(AllocConsole())
			{
				//See: http://support.microsoft.com/default.aspx?scid=kb;en-us;105305
				int hCrtStdOut = _open_osfhandle((long) GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
				int hCrtStdErr = _open_osfhandle((long) GetStdHandle(STD_ERROR_HANDLE), _O_TEXT);
				FILE *hfStdOut = _fdopen(hCrtStdOut, "w");
				FILE *hfStderr = _fdopen(hCrtStdErr, "w");
				*stdout = *hfStdOut;
				*stderr = *hfStderr;
				setvbuf(stdout, NULL, _IONBF, 0);
				setvbuf(stderr, NULL, _IONBF, 0);
			}

			HMENU hMenu = GetSystemMenu(GetConsoleWindow(), 0);
			EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
			RemoveMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);

			SetConsoleCtrlHandler(NULL, TRUE);
			SetConsoleTitle(L"LameXP - Audio Encoder Front-End | Debug Console");
			SetConsoleOutputCP(CP_UTF8);
			
			break;
		}
	}
}

/*
 * Detect CPU features
 */
lamexp_cpu_t lamexp_detect_cpu_features(void)
{
	lamexp_cpu_t features;
	memset(&features, 0, sizeof(lamexp_cpu_t));

	int CPUInfo[4] = {-1};
	
	__cpuid(CPUInfo, 0);
	if(CPUInfo[0] >= 1)
	{
		__cpuid(CPUInfo, 1);
		features.mmx = (CPUInfo[3] & 0x800000) || false;
		features.sse = (CPUInfo[3] & 0x2000000) || false;
		features.sse2 = (CPUInfo[3] & 0x4000000) || false;
		features.ssse3 = (CPUInfo[2] & 0x200) || false;
		features.sse3 = (CPUInfo[2] & 0x1) || false;
		features.ssse3 = (CPUInfo[2] & 0x200) || false;
		features.stepping = CPUInfo[0] & 0xf;
		features.model = ((CPUInfo[0] >> 4) & 0xf) + (((CPUInfo[0] >> 16) & 0xf) << 4);
		features.family = ((CPUInfo[0] >> 8) & 0xf) + ((CPUInfo[0] >> 20) & 0xff);
	}

	char CPUBrandString[0x40];
	memset(CPUBrandString, 0, sizeof(CPUBrandString));
	__cpuid(CPUInfo, 0x80000000);
	int nExIds = CPUInfo[0];

	for(int i = 0x80000000; i <= nExIds; ++i)
	{
		__cpuid(CPUInfo, i);
		if(i == 0x80000002) memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
		else if(i == 0x80000003) memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
		else if(i == 0x80000004) memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
	}

	strcpy_s(features.brand, 0x40, CPUBrandString);

#if defined(_M_X64 ) || defined(_M_IA64)
	features.x64 = true;
#else
	BOOL x64 = FALSE;
	if(IsWow64Process(GetCurrentProcess(), &x64))
	{
		features.x64 = x64;
	}
#endif

	SYSTEM_INFO systemInfo;
	memset(&systemInfo, 0, sizeof(SYSTEM_INFO));
	GetNativeSystemInfo(&systemInfo);
	features.count = systemInfo.dwNumberOfProcessors;

	return features;
}

/*
 * Check for debugger
 */
void WINAPI debugThreadProc(__in  LPVOID lpParameter)
{
	BOOL remoteDebuggerPresent = FALSE;
	CheckRemoteDebuggerPresent(GetCurrentProcess, &remoteDebuggerPresent);

	while(!IsDebuggerPresent() && !remoteDebuggerPresent)
	{
		Sleep(333);
		CheckRemoteDebuggerPresent(GetCurrentProcess, &remoteDebuggerPresent);
	}
	
	TerminateProcess(GetCurrentProcess(), -1);
}


/*
 * Initialize Qt framework
 */
bool lamexp_init_qt(int argc, char* argv[])
{
	static bool qt_initialized = false;

	//Don't initialized again, if done already
	if(qt_initialized)
	{
		return true;
	}
	
	//Check Qt version
	qDebug("Using Qt Framework v%s, compiled with Qt v%s", qVersion(), QT_VERSION_STR);
	QT_REQUIRE_VERSION(argc, argv, QT_VERSION_STR);
	
	//Check the Windows version
	switch(QSysInfo::windowsVersion() & QSysInfo::WV_NT_based)
	{
	case QSysInfo::WV_2000:
		qDebug("Running on Windows 2000 (not offically supported!).\n");
		break;
	case QSysInfo::WV_XP:
		qDebug("Running on Windows XP.\n\n");
		break;
	case QSysInfo::WV_2003:
		qDebug("Running on Windows Server 2003 or Windows XP Professional x64 Edition.\n");
		break;
	case QSysInfo::WV_VISTA:
		qDebug("Running on Windows Vista or Windows Server 200.8\n");
		break;
	case QSysInfo::WV_WINDOWS7:
		qDebug("Running on Windows 7 or Windows Server 2008 R2.\n");
		break;
	default:
		qFatal("Unsupported OS, only Windows 2000 or later is supported!");
		break;
	}
	
	//Create Qt application instance and setup version info
	QApplication *application = new QApplication(argc, argv);
	application->setApplicationName("LameXP - Audio Encoder Front-End");
	application->setApplicationVersion(QString().sprintf("%d.%02d.%04d", lamexp_version_major(), lamexp_version_minor(), lamexp_version_build())); 
	application->setOrganizationName("LoRd_MuldeR");
	application->setOrganizationDomain("mulder.dummwiedeutsch.de");
	application->setWindowIcon(QIcon(":/MainIcon.png"));
	
	//Load plugins from application directory
	QCoreApplication::setLibraryPaths(QStringList() << QApplication::applicationDirPath());
	qDebug("Library Path:\n%s\n", QApplication::libraryPaths().first().toUtf8().constData());

	//Init static Qt plugins
	LAMEXP_INIT_QT_STATIC_PLUGIN(qico);
	LAMEXP_INIT_QT_STATIC_PLUGIN(qsvg);

	//Check for supported image formats
	QList<QByteArray> supportedFormats = QImageReader::supportedImageFormats();
	for(int i = 0; g_lamexp_imageformats[i]; i++)
	{
		if(!supportedFormats.contains(g_lamexp_imageformats[i]))
		{
			qFatal("Qt initialization error: At least one image format plugin is missing! (%s)", g_lamexp_imageformats[i]);
			return false;
		}
	}
	
	//Done
	qt_initialized = true;
	return true;
}

/*
 * Initialize IPC
 */
int lamexp_init_ipc(void)
{
	if(g_lamexp_sharedmem_ptr && g_lamexp_semaphore_read_ptr && g_lamexp_semaphore_write_ptr)
	{
		return 0;
	}

	g_lamexp_semaphore_read_ptr = new QSystemSemaphore(QString(g_lamexp_semaphore_read_uuid), 0);
	g_lamexp_semaphore_write_ptr = new QSystemSemaphore(QString(g_lamexp_semaphore_write_uuid), 0);

	if(g_lamexp_semaphore_read_ptr->error() != QSystemSemaphore::NoError)
	{
		QString errorMessage = g_lamexp_semaphore_read_ptr->errorString();
		LAMEXP_DELETE(g_lamexp_semaphore_read_ptr);
		LAMEXP_DELETE(g_lamexp_semaphore_write_ptr);
		qFatal("Failed to create system smaphore: %s", errorMessage.toUtf8().constData());
		return -1;
	}
	if(g_lamexp_semaphore_write_ptr->error() != QSystemSemaphore::NoError)
	{
		QString errorMessage = g_lamexp_semaphore_write_ptr->errorString();
		LAMEXP_DELETE(g_lamexp_semaphore_read_ptr);
		LAMEXP_DELETE(g_lamexp_semaphore_write_ptr);
		qFatal("Failed to create system smaphore: %s", errorMessage.toUtf8().constData());
		return -1;
	}

	g_lamexp_sharedmem_ptr = new QSharedMemory(QString(g_lamexp_sharedmem_uuid), NULL);
	
	if(!g_lamexp_sharedmem_ptr->create(sizeof(lamexp_ipc_t)))
	{
		if(g_lamexp_sharedmem_ptr->error() == QSharedMemory::AlreadyExists)
		{
			g_lamexp_sharedmem_ptr->attach();
			if(g_lamexp_sharedmem_ptr->error() == QSharedMemory::NoError)
			{
				return 1;
			}
			else
			{
				QString errorMessage = g_lamexp_sharedmem_ptr->errorString();
				qFatal("Failed to attach to shared memory: %s", errorMessage.toUtf8().constData());
				return -1;
			}
		}
		else
		{
			QString errorMessage = g_lamexp_sharedmem_ptr->errorString();
			qFatal("Failed to create shared memory: %s", errorMessage.toUtf8().constData());
			return -1;
		}
	}

	memset(g_lamexp_sharedmem_ptr->data(), 0, sizeof(lamexp_ipc_t));
	g_lamexp_semaphore_write_ptr->release();

	return 0;
}

/*
 * IPC send message
 */
void lamexp_ipc_send(unsigned int command, const char* message)
{
	if(!g_lamexp_sharedmem_ptr || !g_lamexp_semaphore_read_ptr || !g_lamexp_semaphore_write_ptr)
	{
		throw "Shared memory for IPC not initialized yet.";
	}

	lamexp_ipc_t *lamexp_ipc = new lamexp_ipc_t;
	memset(lamexp_ipc, 0, sizeof(lamexp_ipc_t));
	lamexp_ipc->command = command;
	if(message)
	{
		strcpy_s(lamexp_ipc->parameter, 4096, message);
	}

	if(g_lamexp_semaphore_write_ptr->acquire())
	{
		memcpy(g_lamexp_sharedmem_ptr->data(), lamexp_ipc, sizeof(lamexp_ipc_t));
		g_lamexp_semaphore_read_ptr->release();
	}

	LAMEXP_DELETE(lamexp_ipc);
}

/*
 * IPC read message
 */
void lamexp_ipc_read(unsigned int *command, char* message, size_t buffSize)
{
	*command = 0;
	message[0] = '\0';
	
	if(!g_lamexp_sharedmem_ptr || !g_lamexp_semaphore_read_ptr || !g_lamexp_semaphore_write_ptr)
	{
		throw "Shared memory for IPC not initialized yet.";
	}

	lamexp_ipc_t *lamexp_ipc = new lamexp_ipc_t;
	memset(lamexp_ipc, 0, sizeof(lamexp_ipc_t));

	if(g_lamexp_semaphore_read_ptr->acquire())
	{
		memcpy(lamexp_ipc, g_lamexp_sharedmem_ptr->data(), sizeof(lamexp_ipc_t));
		g_lamexp_semaphore_write_ptr->release();

		if(!(lamexp_ipc->reserved_1 || lamexp_ipc->reserved_2))
		{
			*command = lamexp_ipc->command;
			strcpy_s(message, buffSize, lamexp_ipc->parameter);
		}
		else
		{
			qWarning("Malformed IPC message, will be ignored");
		}
	}

	LAMEXP_DELETE(lamexp_ipc);
}

/*
 * Get LameXP temp folder
 */
const QString &lamexp_temp_folder(void)
{
	if(g_lamexp_temp_folder.isEmpty())
	{
		QDir temp = QDir::temp();

		if(!temp.exists())
		{
			temp.mkpath(".");
			if(!temp.exists())
			{
				qFatal("The system's temporary directory does not exist:\n%s", temp.absolutePath().toUtf8().constData());
				return g_lamexp_temp_folder;
			}
		}

		QString uuid = QUuid::createUuid().toString();
		if(!temp.mkdir(uuid))
		{
			qFatal("Temporary directory could not be created:\n%s", QString("%1/%2").arg(temp.canonicalPath(), uuid).toUtf8().constData());
			return g_lamexp_temp_folder;
		}
		if(!temp.cd(uuid))
		{
			qFatal("Temporary directory could not be entered:\n%s", QString("%1/%2").arg(temp.canonicalPath(), uuid).toUtf8().constData());
			return g_lamexp_temp_folder;
		}
		
		QFile testFile(QString("%1/~test.txt").arg(temp.canonicalPath()));
		if(!testFile.open(QIODevice::ReadWrite) || testFile.write("LAMEXP_TEST\n") < 12)
		{
			qFatal("Write access to temporary directory has been denied:\n%s", temp.canonicalPath().toUtf8().constData());
			return g_lamexp_temp_folder;
		}

		testFile.close();
		QFile::remove(testFile.fileName());
		
		g_lamexp_temp_folder = temp.canonicalPath();
	}
	
	return g_lamexp_temp_folder;
}

/*
 * Clean folder
 */
bool lamexp_clean_folder(const QString folderPath)
{
	QDir tempFolder(folderPath);
	QFileInfoList entryList = tempFolder.entryInfoList();
	
	for(int i = 0; i < entryList.count(); i++)
	{
		if(entryList.at(i).fileName().compare(".") == 0 || entryList.at(i).fileName().compare("..") == 0)
		{
			continue;
		}
		
		if(entryList.at(i).isDir())
		{
			lamexp_clean_folder(entryList.at(i).canonicalFilePath());
		}
		else
		{
			QFile::remove(entryList.at(i).canonicalFilePath());
		}
	}
	
	tempFolder.rmdir(".");
	return !tempFolder.exists();
}

/*
 * Finalization function (final clean-up)
 */
void lamexp_finalization(void)
{
	//Free all tools
	if(!g_lamexp_tool_registry.isEmpty())
	{
		QStringList keys = g_lamexp_tool_registry.keys();
		for(int i = 0; i < keys.count(); i++)
		{
			LAMEXP_DELETE(g_lamexp_tool_registry[keys.at(i)]);
		}
		g_lamexp_tool_registry.clear();
		g_lamexp_tool_versions.clear();
	}
	
	//Delete temporary files
	if(!g_lamexp_temp_folder.isEmpty())
	{
		for(int i = 0; i < 100; i++)
		{
			if(lamexp_clean_folder(g_lamexp_temp_folder)) break;
			Sleep(125);
		}
		g_lamexp_temp_folder.clear();
	}

	//Destroy Qt application object
	QApplication *application = dynamic_cast<QApplication*>(QApplication::instance());
	LAMEXP_DELETE(application);

	//Detach from shared memory
	if(g_lamexp_sharedmem_ptr) g_lamexp_sharedmem_ptr->detach();
	LAMEXP_DELETE(g_lamexp_sharedmem_ptr);
	LAMEXP_DELETE(g_lamexp_semaphore_read_ptr);
	LAMEXP_DELETE(g_lamexp_semaphore_write_ptr);
}

/*
 * Register tool
 */
void lamexp_register_tool(const QString &toolName, LockedFile *file, unsigned int version)
{
	if(g_lamexp_tool_registry.contains(toolName.toLower()))
	{
		throw "lamexp_register_tool: Tool is already registered!";
	}

	g_lamexp_tool_registry.insert(toolName.toLower(), file);
	g_lamexp_tool_versions.insert(toolName.toLower(), version);
}

/*
 * Check for tool
 */
bool lamexp_check_tool(const QString &toolName)
{
	return g_lamexp_tool_registry.contains(toolName.toLower());
}

/*
 * Lookup tool
 */
const QString lamexp_lookup_tool(const QString &toolName)
{
	if(g_lamexp_tool_registry.contains(toolName.toLower()))
	{
		return g_lamexp_tool_registry.value(toolName.toLower())->filePath();
	}
	else
	{
		return QString();
	}
}

/*
 * Lookup tool
 */
unsigned int lamexp_tool_version(const QString &toolName)
{
	if(g_lamexp_tool_versions.contains(toolName.toLower()))
	{
		return g_lamexp_tool_versions.value(toolName.toLower());
	}
	else
	{
		return UINT_MAX;
	}
}

/*
 * Version number to human-readable string
 */
const QString lamexp_version2string(const QString &pattern, unsigned int version)
{
	QString result = pattern;
	int digits = result.count("?", Qt::CaseInsensitive);
	
	if(digits < 1)
	{
		return result;
	}
	
	int pos = 0;
	QString versionStr = QString().sprintf(QString().sprintf("%%0%du", digits).toLatin1().constData(), version);
	int index = result.indexOf("?", Qt::CaseInsensitive);
	
	while(index >= 0 && pos < versionStr.length())
	{
		result[index] = versionStr[pos++];
		index = result.indexOf("?", Qt::CaseInsensitive);
	}

	return result;
}


/*
 * Get number private bytes [debug only]
 */
SIZE_T lamexp_dbg_private_bytes(void)
{
#ifdef _DEBUG
	PROCESS_MEMORY_COUNTERS_EX memoryCounters;
	memoryCounters.cb = sizeof(PROCESS_MEMORY_COUNTERS_EX);
	GetProcessMemoryInfo(GetCurrentProcess(), (PPROCESS_MEMORY_COUNTERS) &memoryCounters, sizeof(PROCESS_MEMORY_COUNTERS_EX));
	return memoryCounters.PrivateUsage;
#else
	throw "Cannot call this function in a non-debug build!";
#endif //_DEBUG
}
