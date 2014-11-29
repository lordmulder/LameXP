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

//MUtils
#include <MUtils/Global.h>

//CRT includes
#include <time.h>
#include <process.h>

///////////////////////////////////////////////////////////////////////////////
// GLOBAL VARS
///////////////////////////////////////////////////////////////////////////////

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
 * Natural Order String Comparison - the 'lessThan' helper function
 */
static bool lamexp_natural_string_sort_helper(const QString &str1, const QString &str2)
{
	return (strnatcmp(MUTILS_WCHR(str1), MUTILS_WCHR(str2)) < 0);
}

/*
 * Natural Order String Comparison - the 'lessThan' helper function *with* case folding
 */
static bool lamexp_natural_string_sort_helper_fold_case(const QString &str1, const QString &str2)
{
	return (strnatcasecmp(MUTILS_WCHR(str1), MUTILS_WCHR(str2)) < 0);
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

///////////////////////////////////////////////////////////////////////////////
// INITIALIZATION
///////////////////////////////////////////////////////////////////////////////

extern "C" void _lamexp_global_init_utils(void)
{
	LAMEXP_ZERO_MEMORY(g_lamexp_app_icon);
}

///////////////////////////////////////////////////////////////////////////////
// FINALIZATION
///////////////////////////////////////////////////////////////////////////////

extern "C" void _lamexp_global_free_utils(void)
{
	//Free memory
	MUTILS_DELETE(g_lamexp_app_icon.appIcon);
}
