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

#include "Global.h"

//Qt includes
#include <QApplication>
#include <QDate>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QMutex>
#include <QProcessEnvironment>
#include <QReadWriteLock>
#include <QTextCodec>
#include <QUuid>
#include <QWidget>

//Natural sort
#include "strnatcmp.h"

//CRT includes
#include <time.h>
#include <process.h>

///////////////////////////////////////////////////////////////////////////////
// GLOBAL VARS
///////////////////////////////////////////////////////////////////////////////

//%TEMP% folder
static struct
{
	QString *path;
	QReadWriteLock lock;
}
g_lamexp_temp_folder;

static struct
{
	QIcon *appIcon;
	QReadWriteLock lock;
}
g_lamexp_app_icon;

///////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

/*
 * Get a random string
 */
QString lamexp_rand_str(const bool bLong)
{
	const QUuid uuid = QUuid::createUuid().toString();

	const unsigned int u1 = uuid.data1;
	const unsigned int u2 = (((unsigned int)(uuid.data2)) << 16) | ((unsigned int)(uuid.data3));
	const unsigned int u3 = (((unsigned int)(uuid.data4[0])) << 24) | (((unsigned int)(uuid.data4[1])) << 16) | (((unsigned int)(uuid.data4[2])) << 8) | ((unsigned int)(uuid.data4[3]));
	const unsigned int u4 = (((unsigned int)(uuid.data4[4])) << 24) | (((unsigned int)(uuid.data4[5])) << 16) | (((unsigned int)(uuid.data4[6])) << 8) | ((unsigned int)(uuid.data4[7]));

	return bLong ? QString().sprintf("%08x%08x%08x%08x", u1, u2, u3, u4) : QString().sprintf("%08x%08x", (u1 ^ u2), (u3 ^ u4));
}

/*
 * Try to initialize the folder (with *write* access)
 */
static QString lamexp_try_init_folder(const QString &folderPath)
{
	static const char *TEST_DATA = "Lorem ipsum dolor sit amet, consectetur, adipisci velit!";
	
	bool success = false;

	const QFileInfo folderInfo(folderPath);
	const QDir folderDir(folderInfo.absoluteFilePath());

	//Create folder, if it does *not* exist yet
	for(int i = 0; (i < 16) && (!folderDir.exists()); i++)
	{
		folderDir.mkpath(".");
	}

	//Make sure folder exists now *and* is writable
	if(folderDir.exists())
	{
		const QByteArray testData = QByteArray(TEST_DATA);
		for(int i = 0; (i < 32) && (!success); i++)
		{
			QFile testFile(folderDir.absoluteFilePath(QString("~%1.tmp").arg(lamexp_rand_str())));
			if(testFile.open(QIODevice::ReadWrite | QIODevice::Truncate))
			{
				if(testFile.write(testData) >= testData.size())
				{
					success = true;
				}
				testFile.remove();
			}
		}
	}

	return (success ? folderDir.canonicalPath() : QString());
}

/*
 * Initialize LameXP temp folder
 */
#define INIT_TEMP_FOLDER_RAND(OUT_PTR, BASE_DIR) do \
{ \
	for(int _i = 0; _i < 128; _i++) \
	{ \
		const QString _randDir =  QString("%1/%2").arg((BASE_DIR), lamexp_rand_str()); \
		if(!QDir(_randDir).exists()) \
		{ \
			*(OUT_PTR) = lamexp_try_init_folder(_randDir); \
			if(!(OUT_PTR)->isEmpty()) break; \
		} \
	} \
} \
while(0)

/*
 * Get LameXP temp folder
 */
const QString &lamexp_temp_folder2(void)
{
	QReadLocker readLock(&g_lamexp_temp_folder.lock);

	//Already initialized?
	if(g_lamexp_temp_folder.path && (!g_lamexp_temp_folder.path->isEmpty()))
	{
		if(QDir(*g_lamexp_temp_folder.path).exists())
		{
			return *g_lamexp_temp_folder.path;
		}
	}

	//Obtain the write lock to initilaize
	readLock.unlock();
	QWriteLocker writeLock(&g_lamexp_temp_folder.lock);
	
	//Still uninitilaized?
	if(g_lamexp_temp_folder.path && (!g_lamexp_temp_folder.path->isEmpty()))
	{
		if(QDir(*g_lamexp_temp_folder.path).exists())
		{
			return *g_lamexp_temp_folder.path;
		}
	}

	//Create the string, if not done yet
	if(!g_lamexp_temp_folder.path)
	{
		g_lamexp_temp_folder.path = new QString();
	}
	
	g_lamexp_temp_folder.path->clear();

	//Try the %TMP% or %TEMP% directory first
	QString tempPath = lamexp_try_init_folder(QDir::temp().absolutePath());
	if(!tempPath.isEmpty())
	{
		INIT_TEMP_FOLDER_RAND(g_lamexp_temp_folder.path, tempPath);
	}

	//Otherwise create TEMP folder in %LOCALAPPDATA% or %SYSTEMROOT%
	if(g_lamexp_temp_folder.path->isEmpty())
	{
		qWarning("%%TEMP%% directory not found -> trying fallback mode now!");
		static const lamexp_known_folder_t folderId[2] = { lamexp_folder_localappdata, lamexp_folder_systroot_dir };
		for(size_t id = 0; (g_lamexp_temp_folder.path->isEmpty() && (id < 2)); id++)
		{
			const QString &knownFolder = lamexp_known_folder(folderId[id]);
			if(!knownFolder.isEmpty())
			{
				tempPath = lamexp_try_init_folder(QString("%1/Temp").arg(knownFolder));
				if(!tempPath.isEmpty())
				{
					INIT_TEMP_FOLDER_RAND(g_lamexp_temp_folder.path, tempPath);
				}
			}
		}
	}

	//Failed to create TEMP folder?
	if(g_lamexp_temp_folder.path->isEmpty())
	{
		qFatal("Temporary directory could not be initialized !!!");
	}
	
	return *g_lamexp_temp_folder.path;
}

/*
 * Setup QPorcess object
 */
void lamexp_init_process(QProcess &process, const QString &wokringDir, const bool bReplaceTempDir)
{
	//Environment variable names
	static const char *const s_envvar_names_temp[] =
	{
		"TEMP", "TMP", "TMPDIR", "HOME", "USERPROFILE", "HOMEPATH", NULL
	};
	static const char *const s_envvar_names_remove[] =
	{
		"WGETRC", "SYSTEM_WGETRC", "HTTP_PROXY", "FTP_PROXY", "NO_PROXY", "GNUPGHOME", "LC_ALL", "LC_COLLATE", "LC_CTYPE", "LC_MESSAGES", "LC_MONETARY", "LC_NUMERIC", "LC_TIME", "LANG", NULL
	};

	//Initialize environment
	QProcessEnvironment env = process.processEnvironment();
	if(env.isEmpty()) env = QProcessEnvironment::systemEnvironment();

	//Clean a number of enviroment variables that might affect our tools
	for(size_t i = 0; s_envvar_names_remove[i]; i++)
	{
		env.remove(QString::fromLatin1(s_envvar_names_remove[i]));
		env.remove(QString::fromLatin1(s_envvar_names_remove[i]).toLower());
	}

	const QString tempDir = QDir::toNativeSeparators(lamexp_temp_folder2());

	//Replace TEMP directory in environment
	if(bReplaceTempDir)
	{
		for(size_t i = 0; s_envvar_names_temp[i]; i++)
		{
			env.insert(s_envvar_names_temp[i], tempDir);
		}
	}

	//Setup PATH variable
	const QString path = env.value("PATH", QString()).trimmed();
	env.insert("PATH", path.isEmpty() ? tempDir : QString("%1;%2").arg(tempDir, path));
	
	//Setup QPorcess object
	process.setWorkingDirectory(wokringDir);
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.setReadChannel(QProcess::StandardOutput);
	process.setProcessEnvironment(env);
}

/*
 * Natural Order String Comparison - the 'lessThan' helper function
 */
static bool lamexp_natural_string_sort_helper(const QString &str1, const QString &str2)
{
	return (strnatcmp(QWCHAR(str1), QWCHAR(str2)) < 0);
}

/*
 * Natural Order String Comparison - the 'lessThan' helper function *with* case folding
 */
static bool lamexp_natural_string_sort_helper_fold_case(const QString &str1, const QString &str2)
{
	return (strnatcasecmp(QWCHAR(str1), QWCHAR(str2)) < 0);
}

/*
 * Natural Order String Comparison - the main sorting function
 */
void lamexp_natural_string_sort(QStringList &list, const bool bIgnoreCase)
{
	qSort(list.begin(), list.end(), bIgnoreCase ? lamexp_natural_string_sort_helper_fold_case : lamexp_natural_string_sort_helper);
}

/*
 * Remove forbidden characters from a filename
 */
const QString lamexp_clean_filename(const QString &str)
{
	QString newStr(str);
	QRegExp rx("\"(.+)\"");
	rx.setMinimal(true);

	newStr.replace("\\", "-");
	newStr.replace(" / ", ", ");
	newStr.replace("/", ",");
	newStr.replace(":", "-");
	newStr.replace("*", "x");
	newStr.replace("?", "");
	newStr.replace("<", "[");
	newStr.replace(">", "]");
	newStr.replace("|", "!");
	newStr.replace(rx, "`\\1´");
	newStr.replace("\"", "'");
	
	return newStr.simplified();
}

/*
 * Remove forbidden characters from a file path
 */
const QString lamexp_clean_filepath(const QString &str)
{
	QStringList parts = QString(str).replace("\\", "/").split("/");

	for(int i = 0; i < parts.count(); i++)
	{
		parts[i] = lamexp_clean_filename(parts[i]);
	}

	return parts.join("/");
}

/*
 * Get a list of all available Qt Text Codecs
 */
QStringList lamexp_available_codepages(bool noAliases)
{
	QStringList codecList;
	
	QList<QByteArray> availableCodecs = QTextCodec::availableCodecs();
	while(!availableCodecs.isEmpty())
	{
		QByteArray current = availableCodecs.takeFirst();
		if(!(current.startsWith("system") || current.startsWith("System")))
		{
			codecList << QString::fromLatin1(current.constData(), current.size());
			if(noAliases)
			{
				if(QTextCodec *currentCodec = QTextCodec::codecForName(current.constData()))
				{
					
					QList<QByteArray> aliases = currentCodec->aliases();
					while(!aliases.isEmpty()) availableCodecs.removeAll(aliases.takeFirst());
				}
			}
		}
	}

	return codecList;
}

/*
 * Robert Jenkins' 96 bit Mix Function
 * Source: http://www.concentric.net/~Ttwang/tech/inthash.htm
 */
static unsigned int lamexp_mix(const unsigned int x, const unsigned int y, const unsigned int z)
{
	unsigned int a = x;
	unsigned int b = y;
	unsigned int c = z;
	
	a=a-b;  a=a-c;  a=a^(c >> 13);
	b=b-c;  b=b-a;  b=b^(a << 8); 
	c=c-a;  c=c-b;  c=c^(b >> 13);
	a=a-b;  a=a-c;  a=a^(c >> 12);
	b=b-c;  b=b-a;  b=b^(a << 16);
	c=c-a;  c=c-b;  c=c^(b >> 5);
	a=a-b;  a=a-c;  a=a^(c >> 3);
	b=b-c;  b=b-a;  b=b^(a << 10);
	c=c-a;  c=c-b;  c=c^(b >> 15);

	return c;
}

/*
 * Seeds the random number generator
 * Note: Altough rand_s() doesn't need a seed, this must be called pripr to lamexp_rand(), just to to be sure!
 */
void lamexp_seed_rand(void)
{
	qsrand(lamexp_mix(clock(), time(NULL), _getpid()));
}

/*
 * Returns a randum number
 * Note: This function uses rand_s() if available, but falls back to qrand() otherwise
 */
unsigned int lamexp_rand(void)
{
	quint32 rnd = 0;

	if(rand_s(&rnd) == 0)
	{
		return rnd;
	}

	for(size_t i = 0; i < sizeof(unsigned int); i++)
	{
		rnd = (rnd << 8) ^ qrand();
	}

	return rnd;
}

/*
 * Make a window blink (to draw user's attention)
 */
void lamexp_blink_window(QWidget *poWindow, unsigned int count, unsigned int delay)
{
	static QMutex blinkMutex;

	const double maxOpac = 1.0;
	const double minOpac = 0.3;
	const double delOpac = 0.1;

	if(!blinkMutex.tryLock())
	{
		qWarning("Blinking is already in progress, skipping!");
		return;
	}
	
	try
	{
		const int steps = static_cast<int>(ceil(maxOpac - minOpac) / delOpac);
		const int sleep = static_cast<int>(floor(static_cast<double>(delay) / static_cast<double>(steps)));
		const double opacity = poWindow->windowOpacity();
	
		for(unsigned int i = 0; i < count; i++)
		{
			for(double x = maxOpac; x >= minOpac; x -= delOpac)
			{
				poWindow->setWindowOpacity(x);
				QApplication::processEvents();
				lamexp_sleep(sleep);
			}

			for(double x = minOpac; x <= maxOpac; x += delOpac)
			{
				poWindow->setWindowOpacity(x);
				QApplication::processEvents();
				lamexp_sleep(sleep);
			}
		}

		poWindow->setWindowOpacity(opacity);
		QApplication::processEvents();
		blinkMutex.unlock();
	}
	catch(...)
	{
		blinkMutex.unlock();
		qWarning("Exception error while blinking!");
	}
}

/*
 * Clean folder
 */
bool lamexp_clean_folder(const QString &folderPath)
{
	QDir tempFolder(folderPath);
	if(tempFolder.exists())
	{
		QFileInfoList entryList = tempFolder.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);

		for(int i = 0; i < entryList.count(); i++)
		{
			if(entryList.at(i).isDir())
			{
				lamexp_clean_folder(entryList.at(i).canonicalFilePath());
			}
			else
			{
				for(int j = 0; j < 3; j++)
				{
					if(lamexp_remove_file(entryList.at(i).canonicalFilePath()))
					{
						break;
					}
				}
			}
		}
		return tempFolder.rmdir(".");
	}
	return true;
}

/*
 * Computus according to H. Lichtenberg
 */
static bool lamexp_computus(const QDate &date)
{
	int X = date.year();
	int A = X % 19;
	int K = X / 100;
	int M = 15 + (3*K + 3) / 4 - (8*K + 13) / 25;
	int D = (19*A + M) % 30;
	int S = 2 - (3*K + 3) / 4;
	int R = D / 29 + (D / 28 - D / 29) * (A / 11);
	int OG = 21 + D - R;
	int SZ = 7 - (X + X / 4 + S) % 7;
	int OE = 7 - (OG - SZ) % 7;
	int OS = (OG + OE);

	if(OS > 31)
	{
		return (date.month() == 4) && (date.day() == (OS - 31));
	}
	else
	{
		return (date.month() == 3) && (date.day() == OS);
	}
}

/*
 * Check for Thanksgiving
 */
static bool lamexp_thanksgiving(const QDate &date)
{
	int day = 0;

	switch(QDate(date.year(), 11, 1).dayOfWeek())
	{
		case 1: day = 25; break; 
		case 2: day = 24; break; 
		case 3: day = 23; break; 
		case 4: day = 22; break; 
		case 5: day = 28; break; 
		case 6: day = 27; break; 
		case 7: day = 26; break;
	}

	return (date.month() == 11) && (date.day() == day);
}

/*
 * Initialize app icon
 */
const QIcon &lamexp_app_icon(void)
{
	QReadLocker readLock(&g_lamexp_app_icon.lock);

	//Already initialized?
	if(g_lamexp_app_icon.appIcon)
	{
		return *g_lamexp_app_icon.appIcon;
	}

	readLock.unlock();
	QWriteLocker writeLock(&g_lamexp_app_icon.lock);

	while(!g_lamexp_app_icon.appIcon)
	{
		QDate currentDate = QDate::currentDate();
		QTime currentTime = QTime::currentTime();
	
		if(lamexp_thanksgiving(currentDate))
		{
			g_lamexp_app_icon.appIcon = new QIcon(":/MainIcon6.png");
		}
		else if(((currentDate.month() == 12) && (currentDate.day() == 31) && (currentTime.hour() >= 20)) || ((currentDate.month() == 1) && (currentDate.day() == 1)  && (currentTime.hour() <= 19)))
		{
			g_lamexp_app_icon.appIcon = new QIcon(":/MainIcon5.png");
		}
		else if(((currentDate.month() == 10) && (currentDate.day() == 31) && (currentTime.hour() >= 12)) || ((currentDate.month() == 11) && (currentDate.day() == 1)  && (currentTime.hour() <= 11)))
		{
			g_lamexp_app_icon.appIcon = new QIcon(":/MainIcon4.png");
		}
		else if((currentDate.month() == 12) && (currentDate.day() >= 24) && (currentDate.day() <= 26))
		{
			g_lamexp_app_icon.appIcon = new QIcon(":/MainIcon3.png");
		}
		else if(lamexp_computus(currentDate))
		{
			g_lamexp_app_icon.appIcon = new QIcon(":/MainIcon2.png");
		}
		else
		{
			g_lamexp_app_icon.appIcon = new QIcon(":/MainIcon1.png");
		}
	}

	return *g_lamexp_app_icon.appIcon;
}

/*
 * Broadcast event to all windows
 */
bool lamexp_broadcast(int eventType, bool onlyToVisible)
{
	if(QApplication *app = dynamic_cast<QApplication*>(QApplication::instance()))
	{
		qDebug("Broadcasting %d", eventType);
		
		bool allOk = true;
		QEvent poEvent(static_cast<QEvent::Type>(eventType));
		QWidgetList list = app->topLevelWidgets();

		while(!list.isEmpty())
		{
			QWidget *widget = list.takeFirst();
			if(!onlyToVisible || widget->isVisible())
			{
				if(!app->sendEvent(widget, &poEvent))
				{
					allOk = false;
				}
			}
		}

		qDebug("Broadcast %d done (%s)", eventType, (allOk ? "OK" : "Stopped"));
		return allOk;
	}
	else
	{
		qWarning("Broadcast failed, could not get QApplication instance!");
		return false;
	}
}

///////////////////////////////////////////////////////////////////////////////
// INITIALIZATION
///////////////////////////////////////////////////////////////////////////////

extern "C" void _lamexp_global_init_utils(void)
{
	LAMEXP_ZERO_MEMORY(g_lamexp_temp_folder);
	LAMEXP_ZERO_MEMORY(g_lamexp_app_icon);
}

///////////////////////////////////////////////////////////////////////////////
// FINALIZATION
///////////////////////////////////////////////////////////////////////////////

extern "C" void _lamexp_global_free_utils(void)
{
	//Delete temporary files
	const QString &tempFolder = lamexp_temp_folder2();
	if(!tempFolder.isEmpty())
	{
		bool success = false;
		for(int i = 0; i < 100; i++)
		{
			if(lamexp_clean_folder(tempFolder))
			{
				success = true;
				break;
			}
			lamexp_sleep(100);
		}
		if(!success)
		{
			lamexp_system_message(L"Sorry, LameXP was unable to clean up all temporary files. Some residual files in your TEMP directory may require manual deletion!", lamexp_beep_warning);
			lamexp_exec_shell(NULL, tempFolder, QString(), QString(), true);
		}
	}

	//Free memory
	LAMEXP_DELETE(g_lamexp_temp_folder.path);
	LAMEXP_DELETE(g_lamexp_app_icon.appIcon);
}
