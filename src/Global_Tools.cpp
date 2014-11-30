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

//LameXP includes
#include "LockedFile.h"

//Qt includes
#include <QApplication>
#include <QMap>
#include <QReadWriteLock>
#include <QReadLocker>
#include <QWriteLocker>
#include <QString>
#include <QStringList>
#include <QTranslator>
#include <QFileInfo>

//MUtils
#include <MUtils/Global.h>
#include <MUtils/Exception.h>

///////////////////////////////////////////////////////////////////////////////
// GLOBAL VARS
///////////////////////////////////////////////////////////////////////////////

//Tools
static struct
{
	QMap<QString, LockedFile*> *registry;
	QMap<QString, unsigned int> *versions;
	QMap<QString, QString> *tags;
	QReadWriteLock lock;
}
g_lamexp_tools;

//Supported languages
static struct
{
	QMap<QString, QString> *files;
	QMap<QString, QString> *names;
	QMap<QString, unsigned int> *sysid;
	QMap<QString, unsigned int> *cntry;
	QReadWriteLock lock;
}
g_lamexp_translation;

//Translator
static struct
{
	QTranslator *instance;
	QReadWriteLock lock;
}
g_lamexp_currentTranslator;

///////////////////////////////////////////////////////////////////////////////
// GLOBAL FUNCTIONS
///////////////////////////////////////////////////////////////////////////////

/*
 * Register tool
 */
void lamexp_register_tool(const QString &toolName, LockedFile *file, unsigned int version, const QString *tag)
{
	QWriteLocker writeLock(&g_lamexp_tools.lock);
	
	if(!g_lamexp_tools.registry) g_lamexp_tools.registry = new QMap<QString, LockedFile*>();
	if(!g_lamexp_tools.versions) g_lamexp_tools.versions = new QMap<QString, unsigned int>();
	if(!g_lamexp_tools.tags) g_lamexp_tools.tags = new QMap<QString, QString>();

	if(g_lamexp_tools.registry->contains(toolName.toLower()))
	{
		MUTILS_THROW("lamexp_register_tool: Tool is already registered!");
	}

	g_lamexp_tools.registry->insert(toolName.toLower(), file);
	g_lamexp_tools.versions->insert(toolName.toLower(), version);
	g_lamexp_tools.tags->insert(toolName.toLower(), (tag) ? (*tag) : QString());
}

/*
 * Check for tool
 */
bool lamexp_check_tool(const QString &toolName)
{
	QReadLocker readLock(&g_lamexp_tools.lock);
	return (g_lamexp_tools.registry) ? g_lamexp_tools.registry->contains(toolName.toLower()) : false;
}

/*
 * Lookup tool path
 */
const QString lamexp_lookup_tool(const QString &toolName)
{
	QReadLocker readLock(&g_lamexp_tools.lock);

	if(g_lamexp_tools.registry)
	{
		if(g_lamexp_tools.registry->contains(toolName.toLower()))
		{
			return g_lamexp_tools.registry->value(toolName.toLower())->filePath();
		}
		else
		{
			return QString();
		}
	}
	else
	{
		return QString();
	}
}

/*
 * Lookup tool version
 */
unsigned int lamexp_tool_version(const QString &toolName, QString *tag)
{
	QReadLocker readLock(&g_lamexp_tools.lock);
	if(tag) tag->clear();

	if(g_lamexp_tools.versions)
	{
		if(g_lamexp_tools.versions->contains(toolName.toLower()))
		{
			if(tag)
			{
				if(g_lamexp_tools.tags->contains(toolName.toLower())) *tag = g_lamexp_tools.tags->value(toolName.toLower());
			}
			return g_lamexp_tools.versions->value(toolName.toLower());
		}
		else
		{
			return UINT_MAX;
		}
	}
	else
	{
		return UINT_MAX;
	}
}

/*
 * Version number to human-readable string
 */
const QString lamexp_version2string(const QString &pattern, unsigned int version, const QString &defaultText, const QString *tag)
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

	if(tag)
	{
		if((index = result.indexOf(QChar(L'#'), Qt::CaseInsensitive)) >= 0)
		{
			result.remove(index, 1).insert(index, (*tag));
		}
	}

	return result;
}

/*
 * Initialize translations and add default language
 */
bool lamexp_translation_init(void)
{
	QWriteLocker writeLockTranslations(&g_lamexp_translation.lock);

	if((!g_lamexp_translation.files) && (!g_lamexp_translation.names))
	{
		g_lamexp_translation.files = new QMap<QString, QString>();
		g_lamexp_translation.names = new QMap<QString, QString>();
		g_lamexp_translation.files->insert(LAMEXP_DEFAULT_LANGID, "");
		g_lamexp_translation.names->insert(LAMEXP_DEFAULT_LANGID, "English");
		return true;
	}
	else
	{
		qWarning("[lamexp_translation_init] Error: Already initialized!");
		return false;
	}
}

/*
 * Register a new translation
 */
bool lamexp_translation_register(const QString &langId, const QString &qmFile, const QString &langName, unsigned int &systemId, unsigned int &country)
{
	QWriteLocker writeLockTranslations(&g_lamexp_translation.lock);

	if(qmFile.isEmpty() || langName.isEmpty() || systemId < 1)
	{
		return false;
	}

	if(!g_lamexp_translation.files) g_lamexp_translation.files = new QMap<QString, QString>();
	if(!g_lamexp_translation.names) g_lamexp_translation.names = new QMap<QString, QString>();
	if(!g_lamexp_translation.sysid) g_lamexp_translation.sysid = new QMap<QString, unsigned int>();
	if(!g_lamexp_translation.cntry) g_lamexp_translation.cntry = new QMap<QString, unsigned int>();

	g_lamexp_translation.files->insert(langId, qmFile);
	g_lamexp_translation.names->insert(langId, langName);
	g_lamexp_translation.sysid->insert(langId, systemId);
	g_lamexp_translation.cntry->insert(langId, country);

	return true;
}

/*
 * Get list of all translations
 */
QStringList lamexp_query_translations(void)
{
	QReadLocker readLockTranslations(&g_lamexp_translation.lock);
	return (g_lamexp_translation.files) ? g_lamexp_translation.files->keys() : QStringList();
}

/*
 * Get translation name
 */
QString lamexp_translation_name(const QString &langId)
{
	QReadLocker readLockTranslations(&g_lamexp_translation.lock);
	return (g_lamexp_translation.names) ? g_lamexp_translation.names->value(langId.toLower(), QString()) : QString();
}

/*
 * Get translation system id
 */
unsigned int lamexp_translation_sysid(const QString &langId)
{
	QReadLocker readLockTranslations(&g_lamexp_translation.lock);
	return (g_lamexp_translation.sysid) ? g_lamexp_translation.sysid->value(langId.toLower(), 0) : 0;
}

/*
 * Get translation script id
 */
unsigned int lamexp_translation_country(const QString &langId)
{
	QReadLocker readLockTranslations(&g_lamexp_translation.lock);
	return (g_lamexp_translation.cntry) ? g_lamexp_translation.cntry->value(langId.toLower(), 0) : 0;
}

/*
 * Install a new translator
 */
bool lamexp_install_translator(const QString &langId)
{
	bool success = false;
	const QString qmFileToPath(":/localization/%1");

	if(langId.isEmpty() || langId.toLower().compare(LAMEXP_DEFAULT_LANGID) == 0)
	{
		success = lamexp_install_translator_from_file(qmFileToPath.arg(LAMEXP_DEFAULT_TRANSLATION));
	}
	else
	{
		QReadLocker readLock(&g_lamexp_translation.lock);
		QString qmFile = (g_lamexp_translation.files) ? g_lamexp_translation.files->value(langId.toLower(), QString()) : QString();
		readLock.unlock();

		if(!qmFile.isEmpty())
		{
			success = lamexp_install_translator_from_file(qmFileToPath.arg(qmFile));
		}
		else
		{
			qWarning("Translation '%s' not available!", langId.toLatin1().constData());
		}
	}

	return success;
}

/*
 * Install a new translator from file
 */
bool lamexp_install_translator_from_file(const QString &qmFile)
{
	QWriteLocker writeLock(&g_lamexp_currentTranslator.lock);
	bool success = false;

	if(!g_lamexp_currentTranslator.instance)
	{
		g_lamexp_currentTranslator.instance = new QTranslator();
	}

	if(!qmFile.isEmpty())
	{
		QString qmPath = QFileInfo(qmFile).canonicalFilePath();
		QApplication::removeTranslator(g_lamexp_currentTranslator.instance);
		if(success = g_lamexp_currentTranslator.instance->load(qmPath))
		{
			QApplication::installTranslator(g_lamexp_currentTranslator.instance);
		}
		else
		{
			qWarning("Failed to load translation:\n\"%s\"", qmPath.toLatin1().constData());
		}
	}
	else
	{
		QApplication::removeTranslator(g_lamexp_currentTranslator.instance);
		success = true;
	}

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// INITIALIZATION
///////////////////////////////////////////////////////////////////////////////

extern "C" void _lamexp_global_init_tools(void)
{
	MUTILS_ZERO_MEMORY(g_lamexp_tools);
	MUTILS_ZERO_MEMORY(g_lamexp_currentTranslator);
	MUTILS_ZERO_MEMORY(g_lamexp_translation);
}

///////////////////////////////////////////////////////////////////////////////
// FINALIZATION
///////////////////////////////////////////////////////////////////////////////

extern "C" void _lamexp_global_free_tools(void)
{
	//Free *all* registered translations
	if(g_lamexp_currentTranslator.instance)
	{
		QApplication::removeTranslator(g_lamexp_currentTranslator.instance);
		MUTILS_DELETE(g_lamexp_currentTranslator.instance);
	}
	MUTILS_DELETE(g_lamexp_translation.files);
	MUTILS_DELETE(g_lamexp_translation.names);
	MUTILS_DELETE(g_lamexp_translation.cntry);
	MUTILS_DELETE(g_lamexp_translation.sysid);

	//Free *all* registered tools
	if(g_lamexp_tools.registry)
	{
		QStringList keys = g_lamexp_tools.registry->keys();
		for(int i = 0; i < keys.count(); i++)
		{
			LockedFile *lf = g_lamexp_tools.registry->take(keys.at(i));
			MUTILS_DELETE(lf);
		}
		g_lamexp_tools.registry->clear();
		g_lamexp_tools.versions->clear();
		g_lamexp_tools.tags->clear();
	}
	MUTILS_DELETE(g_lamexp_tools.registry);
	MUTILS_DELETE(g_lamexp_tools.versions);
	MUTILS_DELETE(g_lamexp_tools.tags);

}
