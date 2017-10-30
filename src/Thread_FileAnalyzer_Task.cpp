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

#include "Thread_FileAnalyzer_Task.h"

//Internal
#include "Global.h"
#include "LockedFile.h"
#include "Model_AudioFile.h"
#include "MimeTypes.h"

//MUtils
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>
#include <MUtils/Exception.h>

//Qt
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QDate>
#include <QTime>
#include <QDebug>
#include <QImage>
#include <QReadLocker>
#include <QWriteLocker>
#include <QThread>
#include <QXmlSimpleReader>
#include <QXmlInputSource>
#include <QStack>

//CRT
#include <math.h>
#include <time.h>
#include <assert.h>

#define IS_KEY(KEY) (key.compare(KEY, Qt::CaseInsensitive) == 0)
#define IS_SEC(SEC) (key.startsWith((SEC "_"), Qt::CaseInsensitive))
#define FIRST_TOK(STR) (STR.split(" ", QString::SkipEmptyParts).first())

#define STR_EQ(A,B) ((A).compare((B), Qt::CaseInsensitive) == 0)

////////////////////////////////////////////////////////////
// XML Content Handler
////////////////////////////////////////////////////////////

class AnalyzeTask_XmlHandler : public QXmlDefaultHandler
{
public:
	AnalyzeTask_XmlHandler(AudioFileModel &audioFile, const quint32 &version) :
		m_audioFile(audioFile), m_version(version), m_trackType(trackType_non), m_trackIdx(0), m_properties(initializeProperties()) {}

protected:
	virtual bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts);
	virtual bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName);
	virtual bool characters(const QString& ch);

private:
	typedef enum
	{
		trackType_non = 0,
		trackType_gen = 1,
		trackType_aud = 2,
	}
	trackType_t;

	typedef enum
	{
		propertyId_gen_format,
		propertyId_gen_format_profile,
		propertyId_gen_duration,
		propertyId_aud_format,
		propertyId_aud_format_version,
		propertyId_aud_format_profile,
		propertyId_aud_channel_s_,
		propertyId_aud_samplingrate
	}
	propertyId_t;

	const quint32 m_version;
	const QMap<QPair<trackType_t, QString>, propertyId_t> &m_properties;

	QStack<QString> m_stack;
	AudioFileModel &m_audioFile;
	trackType_t m_trackType;
	quint32 m_trackIdx;
	propertyId_t m_currentProperty;

	static QReadWriteLock s_propertiesMutex;
	static QScopedPointer<const QMap<QPair<trackType_t, QString>, propertyId_t>> s_propertiesMap;

	bool updatePropertry(const propertyId_t &idx, const QString &value);

	static const QMap<QPair<trackType_t, QString>, propertyId_t> &initializeProperties();
	static bool parseUnsigned(const QString &str, quint32 &value);
	static quint32 decodeTime(quint32 &value);
};

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

AnalyzeTask::AnalyzeTask(const int taskId, const QString &inputFile, QAtomicInt &abortFlag)
:
	m_taskId(taskId),
	m_inputFile(inputFile),
	m_mediaInfoBin(lamexp_tools_lookup("mediainfo.exe")),
	m_mediaInfoVer(lamexp_tools_version("mediainfo.exe")),
	m_avs2wavBin(lamexp_tools_lookup("avs2wav.exe")),
	m_abortFlag(abortFlag)
{
	if(m_mediaInfoBin.isEmpty() || m_avs2wavBin.isEmpty())
	{
		qFatal("Invalid path to MediaInfo binary. Tool not initialized properly.");
	}
}

AnalyzeTask::~AnalyzeTask(void)
{
	emit taskCompleted(m_taskId);
}

////////////////////////////////////////////////////////////
// Thread Main
////////////////////////////////////////////////////////////

void AnalyzeTask::run()
{
	try
	{
		run_ex();
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

void AnalyzeTask::run_ex(void)
{
	int fileType = fileTypeNormal;
	QString currentFile = QDir::fromNativeSeparators(m_inputFile);
	qDebug("Analyzing: %s", MUTILS_UTF8(currentFile));
	
	AudioFileModel fileInfo(currentFile);
	analyzeFile(currentFile, fileInfo, &fileType);

	if(MUTILS_BOOLIFY(m_abortFlag))
	{
		qWarning("Operation cancelled by user!");
		return;
	}

	switch(fileType)
	{
	case fileTypeDenied:
		qWarning("Cannot access file for reading, skipping!");
		break;
	case fileTypeCDDA:
		qWarning("Dummy CDDA file detected, skipping!");
		break;
	default:
		if(fileInfo.metaInfo().title().isEmpty() || fileInfo.techInfo().containerType().isEmpty() || fileInfo.techInfo().audioType().isEmpty())
		{
			fileType = fileTypeUnknown;
			if(!QFileInfo(currentFile).suffix().compare("cue", Qt::CaseInsensitive))
			{
				qWarning("Cue Sheet file detected, skipping!");
				fileType = fileTypeCueSheet;
			}
			else if(!QFileInfo(currentFile).suffix().compare("avs", Qt::CaseInsensitive))
			{
				qDebug("Found a potential Avisynth script, investigating...");
				if(analyzeAvisynthFile(currentFile, fileInfo))
				{
					fileType = fileTypeNormal;
				}
				else
				{
					qDebug("Rejected Avisynth file: %s", MUTILS_UTF8(fileInfo.filePath()));
				}
			}
			else
			{
				qDebug("Rejected file of unknown type: %s", MUTILS_UTF8(fileInfo.filePath()));
			}
		}
		break;
	}

	//Emit the file now!
	emit fileAnalyzed(m_taskId, fileType, fileInfo);
}

////////////////////////////////////////////////////////////
// Privtae Functions
////////////////////////////////////////////////////////////

const AudioFileModel& AnalyzeTask::analyzeFile(const QString &filePath, AudioFileModel &audioFile, int *const type)
{
	*type = fileTypeNormal;
	QFile readTest(filePath);

	if (!readTest.open(QIODevice::ReadOnly))
	{
		*type = fileTypeDenied;
		return audioFile;
	}

	if (checkFile_CDDA(readTest))
	{
		*type = fileTypeCDDA;
		return audioFile;
	}

	readTest.close();
	return analyzeMediaFile(filePath, audioFile);
}

const AudioFileModel& AnalyzeTask::analyzeMediaFile(const QString &filePath, AudioFileModel &audioFile)
{
	//bool skipNext = false;
	QPair<quint32, quint32> id_val(UINT_MAX, UINT_MAX);
	quint32 coverType = UINT_MAX;
	QByteArray coverData;

	QStringList params;
	params << QString("--Language=raw");
	params << QString("-f");
	params << QString("--Output=XML");
	params << QDir::toNativeSeparators(filePath);

	QProcess process;
	MUtils::init_process(process, QFileInfo(m_mediaInfoBin).absolutePath());
	process.start(m_mediaInfoBin, params);

	QByteArray data;
	data.reserve(16384);

	if(!process.waitForStarted())
	{
		qWarning("MediaInfo process failed to create!");
		qWarning("Error message: \"%s\"\n", process.errorString().toLatin1().constData());
		process.kill();
		process.waitForFinished(-1);
		return audioFile;
	}

	while(process.state() != QProcess::NotRunning)
	{
		if(MUTILS_BOOLIFY(m_abortFlag))
		{
			process.kill();
			qWarning("Process was aborted on user request!");
			break;
		}
		
		if(!process.waitForReadyRead())
		{
			if(process.state() == QProcess::Running)
			{
				qWarning("MediaInfo time out. Killing the process now!");
				process.kill();
				process.waitForFinished(-1);
				break;
			}
		}

		forever
		{
			const QByteArray dataNext = process.readAll();
			if (dataNext.isEmpty()) {
				break; /*no more input data*/
			}
			data += dataNext;
		}
	}

	process.waitForFinished();
	if (process.state() != QProcess::NotRunning)
	{
		process.kill();
		process.waitForFinished(-1);
	}

	while (!process.atEnd())
	{
		const QByteArray dataNext = process.readAll();
		if (dataNext.isEmpty()) {
			break; /*no more input data*/
		}
		data += dataNext;
	}
	
	//qDebug("!!!--START-->>>\n%s\n<<<--END--!!!", data.constData());
	return parseMediaInfo(data, audioFile);

	/* if(audioFile.metaInfo().title().isEmpty())
	{
		QString baseName = QFileInfo(filePath).fileName();
		int index = baseName.lastIndexOf(".");

		if(index >= 0)
		{
			baseName = baseName.left(index);
		}

		baseName = baseName.replace("_", " ").simplified();
		index = baseName.lastIndexOf(" - ");

		if(index >= 0)
		{
			baseName = baseName.mid(index + 3).trimmed();
		}

		audioFile.metaInfo().setTitle(baseName);
	}

	if((coverType != UINT_MAX) && (!coverData.isEmpty()))
	{
		retrieveCover(audioFile, coverType, coverData);
	}

	if((audioFile.techInfo().audioType().compare("PCM", Qt::CaseInsensitive) == 0) && (audioFile.techInfo().audioProfile().compare("Float", Qt::CaseInsensitive) == 0))
	{
		if(audioFile.techInfo().audioBitdepth() == 32) audioFile.techInfo().setAudioBitdepth(AudioFileModel::BITDEPTH_IEEE_FLOAT32);
	}

	return audioFile;*/
}

const AudioFileModel& AnalyzeTask::parseMediaInfo(const QByteArray &data, AudioFileModel &audioFile)
{
	QXmlInputSource xmlSource;
	xmlSource.setData(data);

	QScopedPointer<QXmlDefaultHandler> xmlHandler(new AnalyzeTask_XmlHandler(audioFile, m_mediaInfoVer));

	QXmlSimpleReader xmlReader;
	xmlReader.setContentHandler(xmlHandler.data());
	xmlReader.setErrorHandler(xmlHandler.data());

	if (xmlReader.parse(xmlSource))
	{
		while (xmlReader.parseContinue()) {/*continue*/}
	}

	if(audioFile.metaInfo().title().isEmpty())
	{
		QString baseName = QFileInfo(audioFile.filePath()).fileName();
		int index;
		if((index = baseName.lastIndexOf("."))>= 0)
		{
			baseName = baseName.left(index);
		}
		baseName = baseName.replace("_", " ").simplified();
		if((index = baseName.lastIndexOf(" - ")) >= 0)
		{
			baseName = baseName.mid(index + 3).trimmed();
		}
		audioFile.metaInfo().setTitle(baseName);
	}

	if ((audioFile.techInfo().audioType().compare("PCM", Qt::CaseInsensitive) == 0) && (audioFile.techInfo().audioProfile().compare("Float", Qt::CaseInsensitive) == 0))
	{
		if (audioFile.techInfo().audioBitdepth() == 32) audioFile.techInfo().setAudioBitdepth(AudioFileModel::BITDEPTH_IEEE_FLOAT32);
	}

	return audioFile;
}

bool AnalyzeTask::checkFile_CDDA(QFile &file)
{
	file.reset();
	QByteArray data = file.read(128);
	
	int i = data.indexOf("RIFF");
	int j = data.indexOf("CDDA");
	int k = data.indexOf("fmt ");

	return ((i >= 0) && (j >= 0) && (k >= 0) && (k > j) && (j > i));
}

void AnalyzeTask::retrieveCover(AudioFileModel &audioFile, const quint32 coverType, const QByteArray &coverData)
{
	qDebug("Retrieving cover! (MIME_TYPES_MAX=%u)", MIME_TYPES_MAX);
	
	static const QString ext = QString::fromLatin1(MIME_TYPES[qBound(0U, coverType, MIME_TYPES_MAX)].ext[0]);
	if(!(QImage::fromData(coverData, ext.toUpper().toLatin1().constData()).isNull()))
	{
		QFile coverFile(QString("%1/%2.%3").arg(MUtils::temp_folder(), MUtils::next_rand_str(), ext));
		if(coverFile.open(QIODevice::WriteOnly))
		{
			coverFile.write(coverData);
			coverFile.close();
			audioFile.metaInfo().setCover(coverFile.fileName(), true);
		}
	}
	else
	{
		qWarning("Image data seems to be invalid :-(");
	}
}

bool AnalyzeTask::analyzeAvisynthFile(const QString &filePath, AudioFileModel &info)
{
	QProcess process;
	MUtils::init_process(process, QFileInfo(m_avs2wavBin).absolutePath());

	process.start(m_avs2wavBin, QStringList() << QDir::toNativeSeparators(filePath) << "?");

	if(!process.waitForStarted())
	{
		qWarning("AVS2WAV process failed to create!");
		qWarning("Error message: \"%s\"\n", process.errorString().toLatin1().constData());
		process.kill();
		process.waitForFinished(-1);
		return false;
	}

	bool bInfoHeaderFound = false;

	while(process.state() != QProcess::NotRunning)
	{
		if(MUTILS_BOOLIFY(m_abortFlag))
		{
			process.kill();
			qWarning("Process was aborted on user request!");
			break;
		}
		
		if(!process.waitForReadyRead())
		{
			if(process.state() == QProcess::Running)
			{
				qWarning("AVS2WAV time out. Killing process and skipping file!");
				process.kill();
				process.waitForFinished(-1);
				return false;
			}
		}

		QByteArray data;

		while(process.canReadLine())
		{
			QString line = QString::fromUtf8(process.readLine().constData()).simplified();
			if(!line.isEmpty())
			{
				int index = line.indexOf(':');
				if(index > 0)
				{
					QString key = line.left(index).trimmed();
					QString val = line.mid(index+1).trimmed();

					if(bInfoHeaderFound && !key.isEmpty() && !val.isEmpty())
					{
						if(key.compare("TotalSeconds", Qt::CaseInsensitive) == 0)
						{
							bool ok = false;
							unsigned int duration = val.toUInt(&ok);
							if(ok) info.techInfo().setDuration(duration);
						}
						if(key.compare("SamplesPerSec", Qt::CaseInsensitive) == 0)
						{
							bool ok = false;
							unsigned int samplerate = val.toUInt(&ok);
							if(ok) info.techInfo().setAudioSamplerate (samplerate);
						}
						if(key.compare("Channels", Qt::CaseInsensitive) == 0)
						{
							bool ok = false;
							unsigned int channels = val.toUInt(&ok);
							if(ok) info.techInfo().setAudioChannels(channels);
						}
						if(key.compare("BitsPerSample", Qt::CaseInsensitive) == 0)
						{
							bool ok = false;
							unsigned int bitdepth = val.toUInt(&ok);
							if(ok) info.techInfo().setAudioBitdepth(bitdepth);
						}
					}
				}
				else
				{
					if(line.contains("[Audio Info]", Qt::CaseInsensitive))
					{
						info.techInfo().setAudioType("Avisynth");
						info.techInfo().setContainerType("Avisynth");
						bInfoHeaderFound = true;
					}
				}
			}
		}
	}
	
	process.waitForFinished();
	if(process.state() != QProcess::NotRunning)
	{
		process.kill();
		process.waitForFinished(-1);
	}

	//Check exit code
	switch(process.exitCode())
	{
	case 0:
		qDebug("Avisynth script was analyzed successfully.");
		return true;
		break;
	case -5:
		qWarning("It appears that Avisynth is not installed on the system!");
		return false;
		break;
	default:
		qWarning("Failed to open the Avisynth script, bad AVS file?");
		return false;
		break;
	}
}

unsigned int AnalyzeTask::parseYear(const QString &str)
{
	if(str.startsWith("UTC", Qt::CaseInsensitive))
	{
		QDate date = QDate::fromString(str.mid(3).trimmed().left(10), "yyyy-MM-dd");
		if(date.isValid())
		{
			return date.year();
		}
		else
		{
			return 0;
		}
	}
	else
	{
		bool ok = false;
		int year = str.toInt(&ok);
		if(ok && year > 0)
		{
			return year;
		}
		else
		{
			return 0;
		}
	}
}

// ---------------------------------------------------------
// XML Content Handler Implementation
// ---------------------------------------------------------

#define DEFINE_PROPTERY_MAPPING(TYPE, NAME) do \
{ \
	builder->insert(qMakePair(trackType_##TYPE, QString::fromLatin1(#NAME)), propertyId_##TYPE##_##NAME); \
} \
while(0)

#define SET_OPTIONAL(TYPE, IF_CMD, THEN_CMD) do \
{ \
	TYPE _tmp;\
	if((IF_CMD)) { THEN_CMD; } \
} \
while(0)

QReadWriteLock AnalyzeTask_XmlHandler::s_propertiesMutex;
QScopedPointer<const QMap<QPair<AnalyzeTask_XmlHandler::trackType_t, QString>, AnalyzeTask_XmlHandler::propertyId_t>> AnalyzeTask_XmlHandler::s_propertiesMap;

const QMap<QPair<AnalyzeTask_XmlHandler::trackType_t, QString>, AnalyzeTask_XmlHandler::propertyId_t> &AnalyzeTask_XmlHandler::initializeProperties(void)
{
	QReadLocker rdLocker(&s_propertiesMutex);
	if (!s_propertiesMap.isNull())
	{
		return *s_propertiesMap.data();
	}

	rdLocker.unlock();
	QWriteLocker wrLocker(&s_propertiesMutex);

	if (s_propertiesMap.isNull())
	{
		QMap<QPair<trackType_t, QString>, propertyId_t> *const builder = new QMap<QPair<trackType_t, QString>, propertyId_t>();
		DEFINE_PROPTERY_MAPPING(gen, format);
		DEFINE_PROPTERY_MAPPING(gen, format_profile);
		DEFINE_PROPTERY_MAPPING(gen, duration);
		DEFINE_PROPTERY_MAPPING(aud, format);
		DEFINE_PROPTERY_MAPPING(aud, format_version);
		DEFINE_PROPTERY_MAPPING(aud, format_profile);
		DEFINE_PROPTERY_MAPPING(aud, channel_s_);
		DEFINE_PROPTERY_MAPPING(aud, samplingrate);
		s_propertiesMap.reset(builder);
	}

	return *s_propertiesMap.data();
}

bool AnalyzeTask_XmlHandler::startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts)
{
	m_stack.push(qName);
	switch (m_stack.size())
	{
	case 1:
		if (!STR_EQ(qName, "mediaInfo"))
		{
			qWarning("Invalid XML structure was detected! (1)");
			return false;
		}
		if (!STR_EQ(atts.value("version"), QString().sprintf("0.%u.%02u", m_version / 100U, m_version % 100)))
		{
			qWarning("Invalid version property was detected!");
			return false;
		}
		return true;
	case 2:
		if (!STR_EQ(qName, "file"))
		{
			qWarning("Invalid XML structure was detected! (2)");
			return false;
		}
		return true;
	case 3:
		if (!STR_EQ(qName, "track"))
		{
			qWarning("Invalid XML structure was detected! (3)");
			return false;
		}
		else
		{
			const QString value = atts.value("type").trimmed();
			if (STR_EQ(value, "general"))
			{
				m_trackType = trackType_gen;
			}
			else if (STR_EQ(value, "audio"))
			{
				if (m_trackIdx++)
				{
					qWarning("Skipping non-primary audio track!");
					m_trackType = trackType_non;
				}
				else
				{
					m_trackType = trackType_aud;
				}
			}
			else /*e.g. video*/
			{
				qWarning("Skipping a non-audio track!");
				m_trackType = trackType_non;
			}
			return true;
		}
	case 4:
		switch (m_trackType)
		{
		case trackType_gen:
			m_currentProperty = m_properties.value(qMakePair(trackType_gen, qName.simplified().toLower()), propertyId_t(-1));
			return true;
		case trackType_aud:
			m_currentProperty = m_properties.value(qMakePair(trackType_aud, qName.simplified().toLower()), propertyId_t(-1));
			return true;
		default:
			m_currentProperty = propertyId_t(-1);
			return true;
		}
	default:
		return true;
	}
}

bool AnalyzeTask_XmlHandler::endElement(const QString &namespaceURI, const QString &localName, const QString &qName)
{
	m_stack.pop();
	return true;
}

bool AnalyzeTask_XmlHandler::characters(const QString& ch)
{
	if ((m_currentProperty != propertyId_t(-1)) && (m_stack.size() == 4))
	{
		const QString value = ch.simplified();
		if (!value.isEmpty())
		{
			updatePropertry(m_currentProperty, value);
		}
	}
	return true;
}

bool AnalyzeTask_XmlHandler::updatePropertry(const propertyId_t &idx, const QString &value)
{
	switch (idx)
	{
		case propertyId_gen_format:         m_audioFile.techInfo().setContainerType(value);                                                          return true;
		case propertyId_gen_format_profile: m_audioFile.techInfo().setContainerProfile(value);                                                       return true;
		case propertyId_gen_duration:       SET_OPTIONAL(quint32, parseUnsigned(value, _tmp), m_audioFile.techInfo().setDuration(decodeTime(_tmp))); return true;
		case propertyId_aud_format:         m_audioFile.techInfo().setAudioType(value);                                                              return true;
		case propertyId_aud_format_version: m_audioFile.techInfo().setAudioVersion(value);                                                           return true;
		case propertyId_aud_format_profile: m_audioFile.techInfo().setAudioProfile(value);                                                           return true;
		case propertyId_aud_channel_s_:     SET_OPTIONAL(quint32, parseUnsigned(value, _tmp), m_audioFile.techInfo().setAudioChannels(_tmp));        return true;
		case propertyId_aud_samplingrate:   SET_OPTIONAL(quint32, parseUnsigned(value, _tmp), m_audioFile.techInfo().setAudioSamplerate(_tmp));      return true;
		default: MUTILS_THROW_FMT("Invalid property ID: %d", idx);
	}
}

bool AnalyzeTask_XmlHandler::parseUnsigned(const QString &str, quint32 &value)
{
	bool okay = false;
	value = str.toUInt(&okay);
	return okay;
}

quint32 AnalyzeTask_XmlHandler::decodeTime(quint32 &value)
{
	return (value + 500U) / 1000U;
}

////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////

/*NONE*/

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/
