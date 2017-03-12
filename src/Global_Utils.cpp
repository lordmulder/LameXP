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

//MUtils
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>

//CRT includes
#include <time.h>
#include <process.h>

///////////////////////////////////////////////////////////////////////////////
// GLOBAL VARS
///////////////////////////////////////////////////////////////////////////////

static QScopedPointer<QIcon> g_lamexp_icon_data;
static QReadWriteLock        g_lamexp_icon_lock;

///////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

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
	QReadLocker readLock(&g_lamexp_icon_lock);

	//Already initialized?
	if(!g_lamexp_icon_data.isNull())
	{
		return *g_lamexp_icon_data;
	}

	readLock.unlock();
	QWriteLocker writeLock(&g_lamexp_icon_lock);

	while(g_lamexp_icon_data.isNull())
	{
		QDate currentDate = QDate::currentDate();
		QTime currentTime = QTime::currentTime();
	
		if(lamexp_thanksgiving(currentDate))
		{
			g_lamexp_icon_data.reset(new QIcon(":/MainIcon6.png"));
		}
		else if(((currentDate.month() == 12) && (currentDate.day() == 31) && (currentTime.hour() >= 20)) || ((currentDate.month() == 1) && (currentDate.day() == 1)  && (currentTime.hour() <= 19)))
		{
			g_lamexp_icon_data.reset(new QIcon(":/MainIcon5.png"));
		}
		else if(((currentDate.month() == 10) && (currentDate.day() == 31) && (currentTime.hour() >= 12)) || ((currentDate.month() == 11) && (currentDate.day() == 1)  && (currentTime.hour() <= 11)))
		{
			g_lamexp_icon_data.reset(new QIcon(":/MainIcon4.png"));
		}
		else if((currentDate.month() == 12) && (currentDate.day() >= 24) && (currentDate.day() <= 26))
		{
			g_lamexp_icon_data.reset(new QIcon(":/MainIcon3.png"));
		}
		else if(lamexp_computus(currentDate))
		{
			g_lamexp_icon_data.reset(new QIcon(":/MainIcon2.png"));
		}
		else
		{
			g_lamexp_icon_data.reset(new QIcon(":/MainIcon1.png"));
		}
	}

	return *g_lamexp_icon_data;
}

/*
 * Version number to human-readable string
 */
const QString lamexp_version2string(const QString &pattern, unsigned int version, const QString &defaultText, const QString &tag)
{
	if(version == UINT_MAX)
	{
		return defaultText;
	}
	
	QString result = pattern;
	const int digits = result.count(QChar(L'?'), Qt::CaseInsensitive);
	
	if(digits < 1)
	{
		return result;
	}
	
	int pos = 0, index = -1;
	const QString versionStr = QString().sprintf("%0*u", digits, version);
	Q_ASSERT(versionStr.length() == digits);
	
	while((index = result.indexOf(QChar(L'?'), Qt::CaseInsensitive)) >= 0)
	{
		result[index] = versionStr[pos++];
	}

	if(!tag.isEmpty())
	{
		if((index = result.indexOf(QChar(L'#'), Qt::CaseInsensitive)) >= 0)
		{
			result.remove(index, 1).insert(index, tag);
		}
	}

	return result;
}
