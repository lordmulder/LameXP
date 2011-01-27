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

#include "ShellIntegration.h"

#include <QString>
#include <QStringList>
#include <QRegExp>
#include <QApplication>
#include <QFileInfo>
#include <QDir>

#include <Windows.h>
#include <Shlobj.h>
#include <Shlwapi.h>

#include "Global.h"
#include "Registry_Decoder.h"

//Const
static const char *g_lamexpShellAction = "ConvertWithLameXP";
static const char *g_lamexpFileType = "LameXP.SupportedAudioFile";

////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////

void ShellIntegration::install(void)
{
	HKEY key = NULL;

	const QString lamexpFileType(g_lamexpFileType);
	const QString lamexpFileInfo(tr("Audio File supported by LameXP"));
	const QString lamexpShellText(tr("Convert this file with LameXP v4"));
	const QString lamexpShellCommand = QString("\"%1\" --add \"%2\"").arg(QDir::toNativeSeparators(QFileInfo(QApplication::applicationFilePath()).canonicalFilePath()), "%1");
	const QString lamexpShellAction(g_lamexpShellAction);

	//Register the LameXP file type
	if(RegCreateKeyEx(HKEY_CURRENT_USER, QWCHAR(QString("Software\\Classes\\%1").arg(lamexpFileType)), NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL) == ERROR_SUCCESS)
	{
		RegSetKeyValue(key, NULL, NULL, REG_SZ, QWCHAR(lamexpFileInfo), (lamexpFileInfo.size() + 1) * sizeof(wchar_t));
		RegCloseKey(key);
	}
	if(RegCreateKeyEx(HKEY_CURRENT_USER, QWCHAR(QString("Software\\Classes\\%1\\shell").arg(lamexpFileType)), NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL) == ERROR_SUCCESS)
	{
		RegSetKeyValue(key, NULL, NULL, REG_SZ, QWCHAR(lamexpShellAction), (lamexpShellAction.size() + 1) * sizeof(wchar_t));
		RegCloseKey(key);
	}

	//Detect supported file types
	QStringList *types = detectTypes(lamexpFileType, lamexpShellAction);

	//Add LameXP shell action to all supported file types
	while(types && (!types->isEmpty()))
	{
		QString currentType = types->takeFirst();

		if(RegCreateKeyEx(HKEY_CURRENT_USER, QWCHAR(QString("Software\\Classes\\%1\\shell\\%2").arg(currentType, lamexpShellAction)), NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL) == ERROR_SUCCESS)
		{
			RegSetKeyValue(key, NULL, NULL, REG_SZ, QWCHAR(lamexpShellText), (lamexpShellText.size() + 1) * sizeof(wchar_t));
			RegCloseKey(key);
		}

		if(RegCreateKeyEx(HKEY_CURRENT_USER, QWCHAR(QString("Software\\Classes\\%1\\shell\\%2\\command").arg(currentType, lamexpShellAction)), NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL) == ERROR_SUCCESS)
		{
			RegSetKeyValue(key, NULL, NULL, REG_SZ, QWCHAR(lamexpShellCommand), (lamexpShellCommand.size() + 1) * sizeof(wchar_t));
			RegCloseKey(key);
		}
	}
	
	//Shell notification
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

	//Free
	delete types;
}

void ShellIntegration::remove(void)
{
	qDebug("Sorry, not implemented yet :-[");
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}

////////////////////////////////////////////////////////////
// Private Functions
////////////////////////////////////////////////////////////

QStringList *ShellIntegration::detectTypes(const QString &lamexpFileType, const QString &lamexpShellAction)
{
	HKEY key = NULL;

	QStringList *nativeTypes = new QStringList();
	QStringList supportedTypes = DecoderRegistry::getSupportedTypes();
	QStringList extensions;

	QRegExp regExp1("\\((.+)\\)");
	QRegExp regExp2("(\\.\\w+)");

	//Find all supported file extensions
	while(!supportedTypes.isEmpty())
	{
		if(regExp1.lastIndexIn(supportedTypes.takeFirst()) > 0)
		{
			int lastIndex = 0;
			while((lastIndex = regExp2.indexIn(regExp1.cap(1), lastIndex) + 1) >= 1)
			{
				extensions.append(regExp2.cap(1));
			}
		}
	}

	//Map supported extensions to native types
	while(!extensions.isEmpty())
	{
		QString currentExt = extensions.takeFirst();
		SHDeleteKey(HKEY_CURRENT_USER, QWCHAR(QString("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%1").arg(currentExt)));

		if(RegOpenKeyEx(HKEY_CLASSES_ROOT, QWCHAR(currentExt), NULL, KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
		{
			wchar_t data[256];
			DWORD dataLen = 256 * sizeof(wchar_t);
			DWORD type = NULL;
				
			if(RegQueryValueEx(key, NULL, NULL, &type, reinterpret_cast<BYTE*>(data), &dataLen) == ERROR_SUCCESS)
			{
				if((type == REG_SZ) || (type == REG_EXPAND_SZ))
				{
					QString currentType = QString::fromUtf16(reinterpret_cast<unsigned short*>(data));
					if((currentType.compare(lamexpFileType, Qt::CaseInsensitive) != 0) && !nativeTypes->contains(currentType, Qt::CaseInsensitive))
					{
						nativeTypes->append(currentType);
					}
				}
			}
			RegCloseKey(key);
		}
		else
		{
			if(RegCreateKeyEx(HKEY_CURRENT_USER, QWCHAR(QString("Software\\Classes\\%1").arg(currentExt)), NULL, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL) == ERROR_SUCCESS)
			{
				RegSetKeyValue(key, NULL, NULL, REG_SZ, QWCHAR(lamexpFileType), (lamexpFileType.size() + 1) * sizeof(wchar_t));
				RegCloseKey(key);
			}
		}
	}

	return nativeTypes;
}
