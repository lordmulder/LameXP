///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2022 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU GENERAL PUBLIC LICENSE as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version; always including the non-optional
// LAMEXP GNU GENERAL PUBLIC LICENSE ADDENDUM. See "License.txt" file!
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

//Internal
#include "Global.h"
#include "Registry_Decoder.h"

//MUtils
#include <MUtils/Global.h>
#include <MUtils/Exception.h>
#include <MUtils/OSSupport.h>
#include <MUtils/Registry.h>

//Qt
#include <QString>
#include <QStringList>
#include <QRegExp>
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QMutexLocker>
#include <QtConcurrentRun>

//Const
static const char *g_lamexpShellAction = "ConvertWithLameXP";
static const char *g_lamexpFileType    = "LameXP.SupportedAudioFile";

//State values
static const int STATE_ENABLED =  1;
static const int STATE_UNKNOWN =  0;
static const int STATE_DISABLD = -1;

//State
QAtomicInt ShellIntegration::m_state(STATE_UNKNOWN);
QMutex ShellIntegration::m_mutex;

//Macros
#define REG_WRITE_STRING(KEY, STR) RegSetValueEx(key, NULL, NULL, REG_SZ, reinterpret_cast<const BYTE*>(STR.utf16()), (STR.size() + 1) * sizeof(wchar_t))

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

ShellIntegration::ShellIntegration(void)
{
	MUTILS_THROW("Cannot create instance of this class, sorry!");
}

////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////

void ShellIntegration::install(bool async)
{
	//Install asynchronously
	if(async)
	{
		QFuture<void>(QtConcurrent::run(install, false));
		return;
	}
		
	//Serialize
	QMutexLocker lock(&m_mutex);

	//Checking
	if(m_state.fetchAndStoreOrdered(STATE_ENABLED) == STATE_ENABLED)
	{
		return; /*already enabled, don't enable again!*/
	}

	//Init some consts
	const QString lamexpFileType(g_lamexpFileType);
	const QString lamexpFileInfo(tr("Audio File supported by LameXP"));
	const QString lamexpShellText(tr("Convert this file with LameXP v%1").arg(QString().sprintf("%d.%02d", lamexp_version_major(), lamexp_version_minor())));
	const QString lamexpShellCommand = QString("\"%1\" \"--add=%2\"").arg(QDir::toNativeSeparators(QFileInfo(QApplication::applicationFilePath()).canonicalFilePath()), "%1");
	const QString lamexpShellAction(g_lamexpShellAction);

	//Register the LameXP file type
	bool ok[4] = {false, false, false, false};
	ok[0] = MUtils::Registry::reg_value_write(MUtils::Registry::root_user, QString("Software\\Classes\\%1")                    .arg(lamexpFileType),                    QString(), lamexpFileInfo);
	ok[1] = MUtils::Registry::reg_value_write(MUtils::Registry::root_user, QString("Software\\Classes\\%1\\shell")             .arg(lamexpFileType),                    QString(), lamexpShellAction);
	ok[2] = MUtils::Registry::reg_value_write(MUtils::Registry::root_user, QString("Software\\Classes\\%1\\shell\\%2")         .arg(lamexpFileType, lamexpShellAction), QString(), lamexpShellText);
	ok[3] = MUtils::Registry::reg_value_write(MUtils::Registry::root_user, QString("Software\\Classes\\%1\\shell\\%2\\command").arg(lamexpFileType, lamexpShellAction), QString(), lamexpShellCommand);
	if(!(ok[0] && ok[1] && ok[2] && ok[3]))
	{
		m_state.fetchAndStoreOrdered(STATE_UNKNOWN);
		qWarning("Failed to register the LameXP file type!");
		return;
	}

	//Detect supported file types
	QStringList types;
	initializeTypes(lamexpFileType, lamexpShellAction, types);

	//Add LameXP shell action to all supported file types
	for(QStringList::ConstIterator iter = types.constBegin(); iter != types.constEnd(); iter++)
	{
		MUtils::Registry::reg_value_write(MUtils::Registry::root_user, QString("Software\\Classes\\%1\\shell\\%2")         .arg((*iter), lamexpShellAction), QString(), lamexpShellText);
		MUtils::Registry::reg_value_write(MUtils::Registry::root_user, QString("Software\\Classes\\%1\\shell\\%2\\command").arg((*iter), lamexpShellAction), QString(), lamexpShellCommand);
	}
	
	//Shell notification
	MUtils::OS::shell_change_notification();
}

void ShellIntegration::remove(bool async)
{
	//Remove asynchronously
	if(async)
	{
		QFuture<void>(QtConcurrent::run(remove, false));
		return;
	}
	
	//Serialize
	QMutexLocker lock(&m_mutex);

	//Checking
	if(m_state.fetchAndStoreOrdered(STATE_DISABLD) == STATE_DISABLD)
	{
		return; /*already disabled, don't disable again!*/
	}

	//Init some consts
	const QString lamexpFileType(g_lamexpFileType);
	const QString lamexpShellAction(g_lamexpShellAction);

	//Initialization
	QStringList fileTypes;

	//Find all registered file types
	if(!MUtils::Registry::reg_enum_subkeys(MUtils::Registry::root_user, "Software\\Classes", fileTypes))
	{
		m_state.fetchAndStoreOrdered(STATE_UNKNOWN);
		qWarning("Failed to enumerate file types!");
		return;
	}

	//Remove shell action from all file types
	for(QStringList::ConstIterator iter = fileTypes.constBegin(); iter != fileTypes.constEnd(); iter++)
	{
		//Remove LameXP-specific types altogether
		if(iter->startsWith('.'))
		{
			QString currentFileType;
			if(MUtils::Registry::reg_value_read(MUtils::Registry::root_user, QString("Software\\Classes\\%1").arg(*iter), QString(), currentFileType))
			{
				if(currentFileType.compare(lamexpFileType, Qt::CaseInsensitive) == 0)
				{
					MUtils::Registry::reg_key_delete(MUtils::Registry::root_user, QString("Software\\Classes\\%1").arg(*iter));
					continue;
				}
			}
		}

		//Remove shell action for non-LameXP types
		MUtils::Registry::reg_key_delete(MUtils::Registry::root_user, QString("Software\\Classes\\%1\\shell\\%2").arg((*iter), lamexpShellAction));

		//Remove from sub-tree too
		QStringList subTypes;
		if(MUtils::Registry::reg_enum_subkeys(MUtils::Registry::root_user, QString("Software\\Classes\\%1").arg(*iter), subTypes))
		{
			for(QStringList::ConstIterator iter2 = subTypes.constBegin(); iter2 != subTypes.constEnd(); iter2++)
			{
				MUtils::Registry::reg_key_delete(MUtils::Registry::root_user, QString("Software\\Classes\\%1\\%2\\shell\\%3").arg((*iter), (*iter2), lamexpShellAction));
			}
		}
	}


	//Unregister LameXP file type
	MUtils::Registry::reg_key_delete(MUtils::Registry::root_user, QString("Software\\Classes\\%1").arg(lamexpFileType));

	//Shell notification
	MUtils::OS::shell_change_notification();
}

////////////////////////////////////////////////////////////
// Private Functions
////////////////////////////////////////////////////////////

void ShellIntegration::initializeTypes(const QString &lamexpFileType, const QString& /*lamexpShellAction*/, QStringList &nativeTypes)
{
	nativeTypes.clear();
	const QString progId = "Progid";

	//Map supported extensions to native types
	const QStringList &supportedExts = DecoderRegistry::getSupportedExts();
	for(QStringList::ConstIterator iter = supportedExts.constBegin(); iter != supportedExts.constEnd(); iter++)
	{
		const QString currentExt = (*iter).mid(1).trimmed(); /*remove leading asterisk*/
		if(currentExt.isEmpty() || (!currentExt.startsWith('.')))
		{
			qWarning("Invalid file extension '%s' encountered -> skipping!", MUTILS_UTF8(currentExt));
			continue;
		}

		bool hasExistingType = false;
		QString currentType;
		if(MUtils::Registry::reg_key_exists(MUtils::Registry::root_classes, currentExt))
		{
			if(MUtils::Registry::reg_value_read(MUtils::Registry::root_classes, currentExt, QString(), currentType))
			{
				currentType = QDir::toNativeSeparators(currentType);
				if((currentType.compare(lamexpFileType, Qt::CaseInsensitive) != 0) && (!nativeTypes.contains(currentType, Qt::CaseInsensitive)))
				{
					nativeTypes.append(currentType);
					hasExistingType = true;
				}
			}
		}
		if(!hasExistingType)
		{
			currentType = lamexpFileType;
			MUtils::Registry::reg_value_write(MUtils::Registry::root_user, QString("Software\\Classes\\%1").arg(currentExt), QString(), lamexpFileType);
		}

		const QString userChoiceKey = QString("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%1\\UserChoice").arg(currentExt);
		if(MUtils::Registry::reg_key_exists(MUtils::Registry::root_user, userChoiceKey))
		{
			if(MUtils::Registry::reg_value_read(MUtils::Registry::root_user, userChoiceKey, progId, currentType))
			{
				currentType = QDir::toNativeSeparators(currentType);
				if((currentType.compare(lamexpFileType, Qt::CaseInsensitive) != 0) && (!nativeTypes.contains(currentType, Qt::CaseInsensitive)))
				{
					nativeTypes.append(currentType);
				}
			}
		}

		QStringList progids;
		if(MUtils::Registry::reg_enum_values(MUtils::Registry::root_user, QString("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%1\\OpenWithProgids").arg(currentExt), progids))
		{
			for(QStringList::ConstIterator iter2 = progids.constBegin(); iter2 != progids.constEnd(); iter2++)
			{
				currentType = QDir::toNativeSeparators(currentType);
				if((iter2->compare(lamexpFileType, Qt::CaseInsensitive) != 0) && (!nativeTypes.contains((*iter2), Qt::CaseInsensitive)))
				{
					nativeTypes.append(*iter2);
				}
			}
		}
	}
}
