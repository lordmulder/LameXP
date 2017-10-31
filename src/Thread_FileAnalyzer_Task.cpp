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
#include <QXmlStreamReader>
#include <QStack>

//CRT
#include <math.h>
#include <time.h>
#include <assert.h>

//Debug
#undef DUMP_MI_OUTPUT

////////////////////////////////////////////////////////////
// Helper Macros
////////////////////////////////////////////////////////////

#define ADD_PROPTERY_MAPPING_1(TYPE, NAME) do \
{ \
	ADD_PROPTERY_MAPPING_2(TYPE, NAME, NAME); \
} \
while(0)

#define ADD_PROPTERY_MAPPING_2(TYPE, MI_NAME, LX_NAME) do \
{ \
	builder->insert(qMakePair(trackType_##TYPE, QString::fromLatin1(#MI_NAME)), propertyId_##LX_NAME); \
} \
while(0)


#define SET_OPTIONAL(TYPE, IF_CMD, THEN_CMD) do \
{ \
	TYPE _tmp;\
	if((IF_CMD)) { THEN_CMD; } \
} \
while(0)

#define STRICMP(A,B) ((A).compare((B), Qt::CaseInsensitive) == 0)

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
	m_abortFlag(abortFlag),
	m_propertiesIdx(initPropertiesIdx()),
	m_trackTypes(initTrackTypes())
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
// Static initialization
////////////////////////////////////////////////////////////

QReadWriteLock AnalyzeTask::s_lock;
QScopedPointer<const QMap<QPair<AnalyzeTask::MI_trackType_t, QString>, AnalyzeTask::MI_propertyId_t>> AnalyzeTask::s_pPropertiesIdx;
QScopedPointer<const QMap<QString, AnalyzeTask::MI_trackType_t>> AnalyzeTask::s_pTrackTypes;

const QMap<QPair<AnalyzeTask::MI_trackType_t, QString>, AnalyzeTask::MI_propertyId_t> &AnalyzeTask::initPropertiesIdx(void)
{
	QReadLocker rdLocker(&s_lock);
	if (s_pPropertiesIdx.isNull())
	{
		rdLocker.unlock();
		QWriteLocker wrLocker(&s_lock);
		if (s_pPropertiesIdx.isNull())
		{
			QMap<QPair<MI_trackType_t, QString>, MI_propertyId_t> *const builder = new QMap<QPair<MI_trackType_t, QString>, MI_propertyId_t>();
			ADD_PROPTERY_MAPPING_2(gen, format, container);
			ADD_PROPTERY_MAPPING_2(gen, format_profile, container_profile);
			ADD_PROPTERY_MAPPING_1(gen, duration);
			ADD_PROPTERY_MAPPING_1(gen, title);
			ADD_PROPTERY_MAPPING_2(gen, track, title);
			ADD_PROPTERY_MAPPING_1(gen, artist);
			ADD_PROPTERY_MAPPING_2(gen, performer, artist);
			ADD_PROPTERY_MAPPING_1(gen, album);
			ADD_PROPTERY_MAPPING_1(gen, genre);
			ADD_PROPTERY_MAPPING_1(gen, released_date);
			ADD_PROPTERY_MAPPING_2(gen, recorded_date, released_date);
			ADD_PROPTERY_MAPPING_1(gen, track_position);
			ADD_PROPTERY_MAPPING_1(gen, comment);
			ADD_PROPTERY_MAPPING_1(aud, format);
			ADD_PROPTERY_MAPPING_1(aud, format_version);
			ADD_PROPTERY_MAPPING_1(aud, format_profile);
			ADD_PROPTERY_MAPPING_1(aud, duration);
			ADD_PROPTERY_MAPPING_1(aud, channel_s_);
			ADD_PROPTERY_MAPPING_1(aud, samplingrate);
			ADD_PROPTERY_MAPPING_1(aud, bitdepth);
			ADD_PROPTERY_MAPPING_1(aud, bitrate);
			ADD_PROPTERY_MAPPING_1(aud, bitrate_mode);
			ADD_PROPTERY_MAPPING_1(aud, encoded_library);
			s_pPropertiesIdx.reset(builder);
		}
		wrLocker.unlock();
		rdLocker.relock();
	}
	return (*s_pPropertiesIdx);
}

const QMap<QString, AnalyzeTask::MI_trackType_t> &AnalyzeTask::initTrackTypes(void)
{
	QReadLocker rdLocker(&s_lock);
	if (s_pTrackTypes.isNull())
	{
		rdLocker.unlock();
		QWriteLocker wrLocker(&s_lock);
		if (s_pTrackTypes.isNull())
		{
			QMap<QString, AnalyzeTask::MI_trackType_t> *const builder = new QMap<QString, AnalyzeTask::MI_trackType_t>();
			builder->insert("general", trackType_gen);
			builder->insert("audio",   trackType_aud);
			s_pTrackTypes.reset(builder);
		}
		wrLocker.unlock();
		rdLocker.relock();
	}
	return (*s_pTrackTypes);
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
	
#if defined(DUMP_MI_OUTPUT)
	qDebug("!!!--START-->>>\n%s\n<<<--END--!!!", data.constData());
#endif //DUMP_MI_OUTPUT

	return parseMediaInfo(data, audioFile);
}

const AudioFileModel& AnalyzeTask::parseMediaInfo(const QByteArray &data, AudioFileModel &audioFile)
{
	QXmlStreamReader xmlStream(data);
	bool firstFile = true;

	if (findNextElement(QLatin1String("MediaInfo"), xmlStream))
	{
		const QString version = findAttribute(QLatin1String("Version"),  xmlStream.attributes());
		if (version.isEmpty() || (!STRICMP(version, QString().sprintf("0.%u.%02u", m_mediaInfoVer / 100U, m_mediaInfoVer % 100))))
		{
			qWarning("Invalid version property \"%s\" was detected!", MUTILS_UTF8(version));
			return audioFile;
		}
		while (findNextElement(QLatin1String("File"), xmlStream))
		{
			if (firstFile)
			{
				firstFile = false;
				parseFileInfo(xmlStream, audioFile);
			}
			else
			{
				qWarning("Skipping non-primary file!");
				xmlStream.skipCurrentElement();
			}
		}
	}

	if (!(audioFile.techInfo().containerType().isEmpty() || audioFile.techInfo().audioType().isEmpty()))
	{
		if (audioFile.metaInfo().title().isEmpty())
		{
			QString baseName = QFileInfo(audioFile.filePath()).fileName();
			int index;
			if ((index = baseName.lastIndexOf(".")) >= 0)
			{
				baseName = baseName.left(index);
			}
			baseName = baseName.replace("_", " ").simplified();
			if ((index = baseName.lastIndexOf(" - ")) >= 0)
			{
				baseName = baseName.mid(index + 3).trimmed();
			}
			audioFile.metaInfo().setTitle(baseName);
		}
		if ((audioFile.techInfo().audioType().compare("PCM", Qt::CaseInsensitive) == 0) && (audioFile.techInfo().audioProfile().compare("Float", Qt::CaseInsensitive) == 0))
		{
			if (audioFile.techInfo().audioBitdepth() == 32) audioFile.techInfo().setAudioBitdepth(AudioFileModel::BITDEPTH_IEEE_FLOAT32);
		}
	}
	else
	{
		qWarning("Audio file format could *not* be recognized!");
	}

	return audioFile;
}

void AnalyzeTask::parseFileInfo(QXmlStreamReader &xmlStream, AudioFileModel &audioFile)
{
	MI_trackType_t trackType;
	QSet<MI_trackType_t> tracksProcessed;

	while (findNextElement(QLatin1String("Track"), xmlStream))
	{
		const QString typeString = findAttribute(QLatin1String("Type"), xmlStream.attributes());
		if ((trackType = m_trackTypes.value(typeString.toLower(), MI_trackType_t(-1))) != MI_trackType_t(-1))
		{
			if (!tracksProcessed.contains(trackType))
			{
				tracksProcessed << trackType;
				parseTrackInfo(xmlStream, trackType, audioFile);
			}
			else
			{
				qWarning("Skipping non-primary '%s' track!", MUTILS_UTF8(typeString));
				xmlStream.skipCurrentElement();
			}
		}
		else
		{
			qWarning("Skipping unsupported '%s' track!", MUTILS_UTF8(typeString));
			xmlStream.skipCurrentElement();
		}
	}
}

void AnalyzeTask::parseTrackInfo(QXmlStreamReader &xmlStream, const MI_trackType_t trackType, AudioFileModel &audioFile)
{
	while (xmlStream.readNextStartElement())
	{
		qDebug("%d::%s", trackType, MUTILS_UTF8(xmlStream.name()));
		const MI_propertyId_t idx = s_pPropertiesIdx->value(qMakePair(trackType, xmlStream.name().toString().simplified().toLower()), MI_propertyId_t(-1));
		if (idx != MI_propertyId_t(-1))
		{
			const QString encoding = findAttribute(QLatin1String("dt"), xmlStream.attributes());
			const QString value = xmlStream.readElementText(QXmlStreamReader::SkipChildElements).simplified();
			if (!value.isEmpty())
			{
				parseProperty(encoding.isEmpty() ? value : decodeStr(value, encoding), idx, audioFile);
			}
		}
		else
		{
			xmlStream.skipCurrentElement();
		}
	}
}

void AnalyzeTask::parseProperty(const QString &value, const MI_propertyId_t propertyIdx, AudioFileModel &audioFile)
{
	qWarning("--> %d: \"%s\"", propertyIdx, MUTILS_UTF8(value));
	switch (propertyIdx)
	{
		case propertyId_container:         audioFile.techInfo().setContainerType(value);                                                     return;
		case propertyId_container_profile: audioFile.techInfo().setContainerProfile(value);                                                  return;
		case propertyId_duration:          SET_OPTIONAL(quint32, parseDuration(value, _tmp), audioFile.techInfo().setDuration(_tmp));        return;
		case propertyId_title:             audioFile.metaInfo().setTitle(value);                                                             return;
		case propertyId_artist:            audioFile.metaInfo().setArtist(value);                                                            return;
		case propertyId_album:             audioFile.metaInfo().setAlbum(value);                                                             return;
		case propertyId_genre:             audioFile.metaInfo().setGenre(value);                                                             return;
		case propertyId_released_date:     SET_OPTIONAL(quint32, parseYear(value, _tmp), audioFile.metaInfo().setYear(_tmp));                return;
		case propertyId_track_position:    SET_OPTIONAL(quint32, parseUnsigned(value, _tmp), audioFile.metaInfo().setPosition(_tmp));        return;
		case propertyId_comment:           audioFile.metaInfo().setComment(value);                                                           return;
		case propertyId_format:            audioFile.techInfo().setAudioType(value);                                                         return;
		case propertyId_format_version:    audioFile.techInfo().setAudioVersion(value);                                                      return;
		case propertyId_format_profile:    audioFile.techInfo().setAudioProfile(value);                                                      return;
		case propertyId_channel_s_:        SET_OPTIONAL(quint32, parseUnsigned(value, _tmp), audioFile.techInfo().setAudioChannels(_tmp));   return;
		case propertyId_samplingrate:      SET_OPTIONAL(quint32, parseUnsigned(value, _tmp), audioFile.techInfo().setAudioSamplerate(_tmp)); return;
		case propertyId_bitdepth:          SET_OPTIONAL(quint32, parseUnsigned(value, _tmp), audioFile.techInfo().setAudioBitdepth(_tmp));   return;
		case propertyId_bitrate:           SET_OPTIONAL(quint32, parseDuration(value, _tmp), audioFile.techInfo().setAudioBitrate(_tmp));    return;
		case propertyId_bitrate_mode:      SET_OPTIONAL(quint32, parseRCMode(value, _tmp), audioFile.techInfo().setAudioBitrateMode(_tmp));  return;
		case propertyId_encoded_library:   audioFile.techInfo().setAudioEncodeLib(cleanAsciiStr(value));                                     return;
		default: MUTILS_THROW_FMT("Invalid property ID: %d", propertyIdx);
	}
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

/*
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
*/

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

// ---------------------------------------------------------
// Utility Functions
// ---------------------------------------------------------

QString AnalyzeTask::decodeStr(const QString &str, const QString &encoding)
{
	if (STRICMP(encoding, QLatin1String("binary.base64")))
	{
		const QString decoded = QString::fromUtf8(QByteArray::fromBase64(str.toLatin1()));
		return decoded;
	}
	return QString();
}

bool AnalyzeTask::parseUnsigned(const QString &str, quint32 &value)
{
	bool okay = false;
	value = str.toUInt(&okay);
	return okay;
}
bool AnalyzeTask::parseDuration(const QString &str, quint32 &value)
{
	if (parseUnsigned(str, value))
	{
		value = (value + 500U) / 1000U;
		return true;
	}
	return false;
}

bool AnalyzeTask::parseYear(const QString &str, quint32 &value)
{
	if (str.startsWith(QLatin1String("UTC"), Qt::CaseInsensitive))
	{
		const QDate date = QDate::fromString(str.mid(3).trimmed().left(10), QLatin1String("yyyy-MM-dd"));
		if (date.isValid())
		{
			value = date.year();
			return true;
		}
		return false;
	}
	else
	{
		return parseUnsigned(str, value);
	}
}

bool AnalyzeTask::parseRCMode(const QString &str, quint32 &value)
{
	if (STRICMP(str, QLatin1String("CBR")))
	{
		value = AudioFileModel::BitrateModeConstant;
		return true;
	}
	if (STRICMP(str, QLatin1String("VBR")))
	{
		value = AudioFileModel::BitrateModeVariable;
		return true;
	}
	return false;
}

QString AnalyzeTask::cleanAsciiStr(const QString &str)
{
	QByteArray ascii = str.toLatin1();
	for (QByteArray::Iterator iter = ascii.begin(); iter != ascii.end(); ++iter)
	{
		if ((*iter < 0x20) || (*iter >= 0x7F)) *iter = 0x3F;
	}
	return QString::fromLatin1(ascii).remove(QLatin1Char('?')).simplified();
}

bool AnalyzeTask::findNextElement(const QString &name, QXmlStreamReader &xmlStream)
{
	while (xmlStream.readNextStartElement())
	{
		if (STRICMP(xmlStream.name(), name))
		{
			return true;
		}
		xmlStream.skipCurrentElement();
	}
	return false;
}

QString AnalyzeTask::findAttribute(const QString &name, const QXmlStreamAttributes &xmlAttributes)
{
	for (QXmlStreamAttributes::ConstIterator iter = xmlAttributes.constBegin(); iter != xmlAttributes.constEnd(); ++iter)
	{
		if (STRICMP(iter->name(), name))
		{
			const QString value = iter->value().toString().simplified();
			if (!value.isEmpty())
			{
				return value; /*found*/
			}
		}
	}
	return QString();
}

////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////

/*NONE*/

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/
