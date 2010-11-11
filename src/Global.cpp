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

//LameXP includes
#include "Resource.h"
#include "LockedFile.h"

//CRT includes
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

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

//Special folders
static QString g_lamexp_temp_folder;

//Tools
static QMap<QString, LockedFile*> g_lamexp_tool_registry;

//Shared memory
static const char *g_lamexp_sharedmem_uuid = "{21A68A42-6923-43bb-9CF6-64BF151942EE}";
static QSharedMemory *g_lamexp_sharedmem_ptr = NULL;
static const char *g_lamexp_semaphore_read_uuid = "{7A605549-F58C-4d78-B4E5-06EFC34F405B}";
static QSystemSemaphore *g_lamexp_semaphore_read_ptr = NULL;
static const char *g_lamexp_semaphore_write_uuid = "{60AA8D04-F6B8-497d-81EB-0F600F4A65B5}";
static QSystemSemaphore *g_lamexp_semaphore_write_ptr = NULL;

//Image formats
static const char *g_lamexp_imageformats[] = {"png", "gif", "ico", "svg", NULL};

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

bool lamexp_version_demo(void)
{ 
	return !(strstr(g_lamexp_version_release, "Final") || strstr(g_lamexp_version_release, "Hotfix"));
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

	//Change application look
	QApplication::setStyle(new QPlastiqueStyle());

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
	
	g_lamexp_semaphore_read_ptr = new QSystemSemaphore(g_lamexp_semaphore_read_uuid, 0);
	g_lamexp_semaphore_write_ptr = new QSystemSemaphore(g_lamexp_semaphore_write_uuid, 0);

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

	g_lamexp_sharedmem_ptr = new QSharedMemory(g_lamexp_sharedmem_uuid, NULL);
	
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
		QDir tempFolder(QDir::tempPath());
		QString uuid = QUuid::createUuid().toString();
		tempFolder.mkdir(uuid);
		
		if(tempFolder.cd(uuid))
		{
			g_lamexp_temp_folder = tempFolder.absolutePath();
		}
		else
		{
			g_lamexp_temp_folder = QDir::tempPath();
		}
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
			lamexp_clean_folder(entryList.at(i).absoluteFilePath());
		}
		else
		{
			QFile::remove(entryList.at(i).absoluteFilePath());
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
void lamexp_register_tool(const QString &toolName, LockedFile *file)
{
	if(g_lamexp_tool_registry.contains(toolName))
	{
		throw "lamexp_register_tool: Tool is already registered!";
	}

	g_lamexp_tool_registry.insert(toolName, file);
}

/*
 * Register tool
 */
const QString lamexp_lookup_tool(const QString &toolName)
{
	if(g_lamexp_tool_registry.contains(toolName))
	{
		return g_lamexp_tool_registry.value(toolName)->filePath();
	}
	else
	{
		return QString();
	}
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
