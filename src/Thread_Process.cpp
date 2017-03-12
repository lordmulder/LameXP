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

#include "Thread_Process.h"

//Internal
#include "Global.h"
#include "Model_AudioFile.h"
#include "Model_Progress.h"
#include "Encoder_Abstract.h"
#include "Decoder_Abstract.h"
#include "Filter_Abstract.h"
#include "Filter_Downmix.h"
#include "Filter_Resample.h"
#include "Tool_WaveProperties.h"
#include "Registry_Decoder.h"
#include "Model_Settings.h"

//MUtils
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>
#include <MUtils/Version.h>

//Qt
#include <QUuid>
#include <QFileInfo>
#include <QDir>
#include <QMutex>
#include <QMutexLocker>
#include <QDate>
#include <QThreadPool>

//CRT
#include <limits.h>
#include <time.h>
#include <stdlib.h>

#define DIFF(X,Y) ((X > Y) ? (X-Y) : (Y-X))
#define IS_WAVE(X) ((X.containerType().compare("Wave", Qt::CaseInsensitive) == 0) && (X.audioType().compare("PCM", Qt::CaseInsensitive) == 0))
#define STRDEF(STR,DEF) ((!STR.isEmpty()) ? STR : DEF)

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

ProcessThread::ProcessThread(const AudioFileModel &audioFile, const QString &outputDirectory, const QString &tempDirectory, AbstractEncoder *encoder, const bool prependRelativeSourcePath)
:
	m_audioFile(audioFile),
	m_outputDirectory(outputDirectory),
	m_tempDirectory(tempDirectory),
	m_encoder(encoder),
	m_jobId(QUuid::createUuid()),
	m_prependRelativeSourcePath(prependRelativeSourcePath),
	m_renamePattern("<BaseName>"),
	m_overwriteMode(OverwriteMode_KeepBoth),
	m_keepDateTime(false),
	m_initialized(-1),
	m_aborted(false),
	m_propDetect(new WaveProperties())
{
	connect(m_encoder, SIGNAL(statusUpdated(int)), this, SLOT(handleUpdate(int)), Qt::DirectConnection);
	connect(m_encoder, SIGNAL(messageLogged(QString)), this, SLOT(handleMessage(QString)), Qt::DirectConnection);

	connect(m_propDetect, SIGNAL(statusUpdated(int)), this, SLOT(handleUpdate(int)), Qt::DirectConnection);
	connect(m_propDetect, SIGNAL(messageLogged(QString)), this, SLOT(handleMessage(QString)), Qt::DirectConnection);

	m_currentStep = UnknownStep;
}

ProcessThread::~ProcessThread(void)
{
	while(!m_tempFiles.isEmpty())
	{
		MUtils::remove_file(m_tempFiles.takeFirst());
	}

	while(!m_filters.isEmpty())
	{
		delete m_filters.takeFirst();
	}

	MUTILS_DELETE(m_encoder);
	MUTILS_DELETE(m_propDetect);

	emit processFinished();
}

////////////////////////////////////////////////////////////
// Init Function
////////////////////////////////////////////////////////////

bool ProcessThread::init(void)
{
	if(m_initialized < 0)
	{
		m_initialized = 0;

		//Initialize job status
		qDebug("Process thread %s has started.", m_jobId.toString().toLatin1().constData());
		emit processStateInitialized(m_jobId, QFileInfo(m_audioFile.filePath()).fileName(), tr("Starting..."), ProgressModel::JobRunning);

		//Initialize log
		handleMessage(QString().sprintf("LameXP v%u.%02u (Build #%u), compiled on %s at %s", lamexp_version_major(), lamexp_version_minor(), lamexp_version_build(), MUTILS_UTF8(MUtils::Version::app_build_date().toString(Qt::ISODate)), MUTILS_UTF8(MUtils::Version::app_build_time().toString(Qt::ISODate))));
		handleMessage("\n-------------------------------\n");

		return true;
	}

	qWarning("[ProcessThread::init] Job %s already initialialized, skipping!", m_jobId.toString().toLatin1().constData());
	return false;
}

bool ProcessThread::start(QThreadPool *const pool)
{
	//Make sure object was initialized correctly
	if(m_initialized < 0)
	{
		MUTILS_THROW("Object not initialized yet!");
	}

	if(m_initialized < 1)
	{
		m_initialized = 1;

		m_outFileName.clear();
		bool bSuccess = false;

		//Generate output file name
		switch(generateOutFileName(m_outFileName))
		{
		case 1:
			//File name generated successfully :-)
			bSuccess = true;
			pool->start(this);
			break;
		case -1:
			//File name already exists -> skipping!
			emit processStateChanged(m_jobId, tr("Skipped."), ProgressModel::JobSkipped);
			emit processStateFinished(m_jobId, m_outFileName, -1);
			break;
		default:
			//File name could not be generated
			emit processStateChanged(m_jobId, tr("Not found!"), ProgressModel::JobFailed);
			emit processStateFinished(m_jobId, m_outFileName, 0);
			break;
		}

		if(!bSuccess)
		{
			emit processFinished();
		}

		return bSuccess;
	}

	qWarning("[ProcessThread::start] Job %s already started, skipping!", m_jobId.toString().toLatin1().constData());
	return false;
}

////////////////////////////////////////////////////////////
// Thread Entry Point
////////////////////////////////////////////////////////////

void ProcessThread::run()
{
	try
	{
		processFile();
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

void ProcessThread::processFile()
{
	m_aborted = false;
	bool bSuccess = true;

	//Make sure object was initialized correctly
	if(m_initialized < 1)
	{
		MUTILS_THROW("Object not initialized yet!");
	}

	QString sourceFile = m_audioFile.filePath();

	//-----------------------------------------------------
	// Decode source file
	//-----------------------------------------------------

	const AudioFileModel_TechInfo &formatInfo = m_audioFile.techInfo();
	if(!m_filters.isEmpty() || !m_encoder->isFormatSupported(formatInfo.containerType(), formatInfo.containerProfile(), formatInfo.audioType(), formatInfo.audioProfile(), formatInfo.audioVersion()))
	{
		m_currentStep = DecodingStep;
		AbstractDecoder *decoder = DecoderRegistry::lookup(formatInfo.containerType(), formatInfo.containerProfile(), formatInfo.audioType(), formatInfo.audioProfile(), formatInfo.audioVersion());
		
		if(decoder)
		{
			QString tempFile = generateTempFileName();

			connect(decoder, SIGNAL(statusUpdated(int)), this, SLOT(handleUpdate(int)), Qt::DirectConnection);
			connect(decoder, SIGNAL(messageLogged(QString)), this, SLOT(handleMessage(QString)), Qt::DirectConnection);

			bSuccess = decoder->decode(sourceFile, tempFile, &m_aborted);
			MUTILS_DELETE(decoder);

			if(bSuccess)
			{
				sourceFile = tempFile;
				m_audioFile.techInfo().setContainerType(QString::fromLatin1("Wave"));
				m_audioFile.techInfo().setAudioType(QString::fromLatin1("PCM"));

				if(QFileInfo(sourceFile).size() >= 4294967296i64)
				{
					handleMessage(tr("WARNING: Decoded file size exceeds 4 GB, problems might occur!\n"));
				}

				handleMessage("\n-------------------------------\n");
			}
		}
		else
		{
			if(QFileInfo(m_outFileName).exists() && (QFileInfo(m_outFileName).size() < 512)) QFile::remove(m_outFileName);
			handleMessage(QString("%1\n%2\n\n%3\t%4\n%5\t%6").arg(tr("The format of this file is NOT supported:"), m_audioFile.filePath(), tr("Container Format:"), m_audioFile.containerInfo(), tr("Audio Format:"), m_audioFile.audioCompressInfo()));
			emit processStateChanged(m_jobId, tr("Unsupported!"), ProgressModel::JobFailed);
			emit processStateFinished(m_jobId, m_outFileName, 0);
			return;
		}
	}

	//-----------------------------------------------------
	// Update audio properties after decode
	//-----------------------------------------------------

	if(bSuccess && !m_aborted && IS_WAVE(m_audioFile.techInfo()))
	{
		if(m_encoder->supportedSamplerates() || m_encoder->supportedBitdepths() || m_encoder->supportedChannelCount() || m_encoder->needsTimingInfo() || !m_filters.isEmpty())
		{
			m_currentStep = AnalyzeStep;
			bSuccess = m_propDetect->detect(sourceFile, &m_audioFile.techInfo(), &m_aborted);

			if(bSuccess)
			{
				handleMessage("\n-------------------------------\n");

				//Do we need to take care if Stereo downmix?
				const unsigned int *const supportedChannelCount = m_encoder->supportedChannelCount();
				if(supportedChannelCount && supportedChannelCount[0])
				{
					insertDownmixFilter(supportedChannelCount);
				}

				//Do we need to take care of downsampling the input?
				const unsigned int *const supportedSamplerates = m_encoder->supportedSamplerates();
				const unsigned int *const supportedBitdepths = m_encoder->supportedBitdepths();
				if((supportedSamplerates && supportedSamplerates[0]) || (supportedBitdepths && supportedBitdepths[0]))
				{
					insertDownsampleFilter(supportedSamplerates, supportedBitdepths);
				}
			}
		}
	}

	//-----------------------------------------------------
	// Apply all audio filters
	//-----------------------------------------------------

	while(bSuccess && (!m_filters.isEmpty()) && (!m_aborted))
	{
		QString tempFile = generateTempFileName();
		AbstractFilter *poFilter = m_filters.takeFirst();
		m_currentStep = FilteringStep;

		connect(poFilter, SIGNAL(statusUpdated(int)), this, SLOT(handleUpdate(int)), Qt::DirectConnection);
		connect(poFilter, SIGNAL(messageLogged(QString)), this, SLOT(handleMessage(QString)), Qt::DirectConnection);

		const AbstractFilter::FilterResult filterResult = poFilter->apply(sourceFile, tempFile, &m_audioFile.techInfo(), &m_aborted);
		switch (filterResult)
		{
		case AbstractFilter::FILTER_SUCCESS:
			sourceFile = tempFile;
			break;
		case AbstractFilter::FILTER_FAILURE:
			bSuccess = false;
			break;
		}

		handleMessage("\n-------------------------------\n");
		delete poFilter;
	}

	//-----------------------------------------------------
	// Encode audio file
	//-----------------------------------------------------

	if(bSuccess && !m_aborted)
	{
		m_currentStep = EncodingStep;
		bSuccess = m_encoder->encode(sourceFile, m_audioFile.metaInfo(), m_audioFile.techInfo().duration(), m_audioFile.techInfo().audioChannels(), m_outFileName, &m_aborted);
	}

	//Clean-up
	if((!bSuccess) || m_aborted)
	{
		QFileInfo fileInfo(m_outFileName);
		if(fileInfo.exists() && (fileInfo.size() < 1024))
		{
			QFile::remove(m_outFileName);
		}
	}

	//Make sure output file exists
	if(bSuccess && (!m_aborted))
	{
		const QFileInfo fileInfo(m_outFileName);
		bSuccess = fileInfo.exists() && fileInfo.isFile() && (fileInfo.size() >= 1024);
	}

	//-----------------------------------------------------
	// Finalize
	//-----------------------------------------------------

	if (bSuccess && (!m_aborted) && m_keepDateTime)
	{
		updateFileTime(m_audioFile.filePath(), m_outFileName);
	}

	MUtils::OS::sleep_ms(12);

	//Report result
	emit processStateChanged(m_jobId, (m_aborted ? tr("Aborted!") : (bSuccess ? tr("Done.") : tr("Failed!"))), ((bSuccess && !m_aborted) ? ProgressModel::JobComplete : ProgressModel::JobFailed));
	emit processStateFinished(m_jobId, m_outFileName, (bSuccess ? 1 : 0));

	qDebug("Process thread is done.");
}

////////////////////////////////////////////////////////////
// SLOTS
////////////////////////////////////////////////////////////

void ProcessThread::handleUpdate(int progress)
{
	//qDebug("Progress: %d\n", progress);
	
	switch(m_currentStep)
	{
	case EncodingStep:
		emit processStateChanged(m_jobId, QString("%1 (%2%)").arg(tr("Encoding"), QString::number(progress)), ProgressModel::JobRunning);
		break;
	case AnalyzeStep:
		emit processStateChanged(m_jobId, QString("%1 (%2%)").arg(tr("Analyzing"), QString::number(progress)), ProgressModel::JobRunning);
		break;
	case FilteringStep:
		emit processStateChanged(m_jobId, QString("%1 (%2%)").arg(tr("Filtering"), QString::number(progress)), ProgressModel::JobRunning);
		break;
	case DecodingStep:
		emit processStateChanged(m_jobId, QString("%1 (%2%)").arg(tr("Decoding"), QString::number(progress)), ProgressModel::JobRunning);
		break;
	}
}

void ProcessThread::handleMessage(const QString &line)
{
	emit processMessageLogged(m_jobId, line);
}

////////////////////////////////////////////////////////////
// PRIVAE FUNCTIONS
////////////////////////////////////////////////////////////

int ProcessThread::generateOutFileName(QString &outFileName)
{
	outFileName.clear();

	//Make sure the source file exists
	const QFileInfo sourceFile(m_audioFile.filePath());
	if(!(sourceFile.exists() && sourceFile.isFile()))
	{
		handleMessage(QString("%1\n%2").arg(tr("The source audio file could not be found:"), sourceFile.absoluteFilePath()));
		return 0;
	}

	//Make sure the source file readable
	QFile readTest(sourceFile.canonicalFilePath());
	if(!readTest.open(QIODevice::ReadOnly))
	{
		handleMessage(QString("%1\n%2").arg(tr("The source audio file could not be opened for reading:"), QDir::toNativeSeparators(readTest.fileName())));
		return 0;
	}
	else
	{
		readTest.close();
	}

	const QString baseName = sourceFile.completeBaseName();
	QDir targetDir(MUtils::clean_file_path(m_outputDirectory.isEmpty() ? sourceFile.canonicalPath() : m_outputDirectory));

	//Prepend relative source file path?
	if(m_prependRelativeSourcePath && !m_outputDirectory.isEmpty())
	{
		QDir sourceDir = sourceFile.dir();
		if (!sourceDir.isRoot())
		{
			quint32 depth = 0;
			while ((!sourceDir.isRoot()) && (++depth <= 0xFF))
			{
				if (!sourceDir.cdUp()) break;
			}
			const QString postfix = QFileInfo(sourceDir.relativeFilePath(sourceFile.canonicalFilePath())).path();
			targetDir.setPath(MUtils::clean_file_path(QString("%1/%2").arg(targetDir.absolutePath(), postfix)));
		}
	}
	
	//Make sure output directory does exist
	if(!targetDir.exists())
	{
		targetDir.mkpath(".");
		if(!targetDir.exists())
		{
			handleMessage(QString("%1\n%2").arg(tr("The target output directory doesn't exist and could NOT be created:"), QDir::toNativeSeparators(targetDir.absolutePath())));
			return 0;
		}
	}
	
	//Make sure that the output dir is writable
	QFile writeTest(QString("%1/.%2").arg(targetDir.canonicalPath(), MUtils::next_rand_str()));
	if(!writeTest.open(QIODevice::ReadWrite))
	{
		handleMessage(QString("%1\n%2").arg(tr("The target output directory is NOT writable:"), QDir::toNativeSeparators(targetDir.absolutePath())));
		return 0;
	}
	else
	{
		writeTest.remove();
	}

	//Apply rename pattern
	const QString fileName = MUtils::clean_file_name(applyRegularExpression(applyRenamePattern(baseName, m_audioFile.metaInfo())));

	//Generate full output path
	const QString fileExt = m_renameFileExt.isEmpty() ? QString::fromUtf8(m_encoder->toEncoderInfo()->extension()) : m_renameFileExt;
	outFileName = QString("%1/%2.%3").arg(targetDir.canonicalPath(), fileName, fileExt);

	//Skip file, if target file exists (optional!)
	if((m_overwriteMode == OverwriteMode_SkipExisting) && QFileInfo(outFileName).exists())
	{
		handleMessage(QString("%1\n%2\n").arg(tr("Target output file already exists, going to skip this file:"), QDir::toNativeSeparators(outFileName)));
		handleMessage(tr("If you don't want existing files to be skipped, please change the overwrite mode!"));
		return -1;
	}

	//Delete file, if target file exists (optional!)
	if((m_overwriteMode == OverwriteMode_Overwrite) && QFileInfo(outFileName).exists() && QFileInfo(outFileName).isFile())
	{
		handleMessage(QString("%1\n%2\n").arg(tr("Target output file already exists, going to delete existing file:"), QDir::toNativeSeparators(outFileName)));
		if(sourceFile.canonicalFilePath().compare(QFileInfo(outFileName).absoluteFilePath(), Qt::CaseInsensitive) != 0)
		{
			for(int i = 0; i < 16; i++)
			{
				if(QFile::remove(outFileName))
				{
					break;
				}
				MUtils::OS::sleep_ms(1);
			}
		}
		if(QFileInfo(outFileName).exists())
		{
			handleMessage(QString("%1\n").arg(tr("Failed to delete existing target file, will save to another file name!")));
		}
	}

	int n = 1;

	//Generate final name
	while(QFileInfo(outFileName).exists() && (n < (INT_MAX/2)))
	{
		outFileName = QString("%1/%2 (%3).%4").arg(targetDir.canonicalPath(), fileName, QString::number(++n), fileExt);
	}

	//Create placeholder
	QFile placeholder(outFileName);
	if(placeholder.open(QIODevice::WriteOnly))
	{
		placeholder.close();
	}

	return 1;
}

QString ProcessThread::applyRenamePattern(const QString &baseName, const AudioFileModel_MetaInfo &metaInfo)
{
	QString fileName = m_renamePattern;
	
	fileName.replace("<BaseName>", STRDEF(baseName, tr("Unknown File Name")),         Qt::CaseInsensitive);
	fileName.replace("<TrackNo>",  QString().sprintf("%02d", metaInfo.position()),    Qt::CaseInsensitive);
	fileName.replace("<Title>",    STRDEF(metaInfo.title(), tr("Unknown Title")) ,    Qt::CaseInsensitive);
	fileName.replace("<Artist>",   STRDEF(metaInfo.artist(), tr("Unknown Artist")),   Qt::CaseInsensitive);
	fileName.replace("<Album>",    STRDEF(metaInfo.album(), tr("Unknown Album")),     Qt::CaseInsensitive);
	fileName.replace("<Year>",     QString().sprintf("%04d", metaInfo.year()),        Qt::CaseInsensitive);
	fileName.replace("<Comment>",  STRDEF(metaInfo.comment(), tr("Unknown Comment")), Qt::CaseInsensitive);

	return fileName.trimmed().isEmpty() ? baseName : fileName;
}

QString ProcessThread::applyRegularExpression(const QString &baseName)
{
	if(m_renameRegExp_Search.isEmpty() || m_renameRegExp_Replace.isEmpty())
	{
		return baseName;
	}

	QRegExp regExp(m_renameRegExp_Search);
	if(!regExp.isValid())
	{
		qWarning("Invalid regular expression detected -> cannot rename!");
		return baseName;
	}
	
	const QString fileName = QString(baseName).replace(regExp, m_renameRegExp_Replace);
	return fileName.trimmed().isEmpty() ? baseName : fileName;
}

QString ProcessThread::generateTempFileName(void)
{
	const QString tempFileName = MUtils::make_temp_file(m_tempDirectory, "wav", true);
	if(tempFileName.isEmpty())
	{
		return QString("%1/~whoops%2.wav").arg(m_tempDirectory, QString::number(MUtils::next_rand_u32()));
	}

	m_tempFiles << tempFileName;
	return tempFileName;
}

bool ProcessThread::insertDownsampleFilter(const unsigned int *const supportedSamplerates, const unsigned int *const supportedBitdepths)
{
	int targetSampleRate = 0, targetBitDepth = 0;
	
	/* Adjust sample rate */
	if(supportedSamplerates && m_audioFile.techInfo().audioSamplerate())
	{
		const unsigned int inputRate = m_audioFile.techInfo().audioSamplerate();
		unsigned int currentDiff = UINT_MAX, minimumDiff = UINT_MAX, bestRate = UINT_MAX;

		//Find the most suitable supported sampling rate
		for(int i = 0; supportedSamplerates[i]; i++)
		{
			currentDiff = DIFF(inputRate, supportedSamplerates[i]);
			if((currentDiff < minimumDiff) || ((currentDiff == minimumDiff) && (bestRate < supportedSamplerates[i])))
			{
				bestRate = supportedSamplerates[i];
				minimumDiff = currentDiff;
				if(!(minimumDiff > 0)) break;
			}
		}
		
		if(bestRate != inputRate)
		{
			targetSampleRate = (bestRate != UINT_MAX) ? bestRate : supportedSamplerates[0];
		}
	}

	/* Adjust bit depth (word size) */
	if(supportedBitdepths && m_audioFile.techInfo().audioBitdepth())
	{
		const unsigned int inputBPS = m_audioFile.techInfo().audioBitdepth();
		bool bAdjustBitdepth = true;

		//Is the input bit depth supported exactly? (including IEEE Float)
		for(int i = 0; supportedBitdepths[i]; i++)
		{
			if(supportedBitdepths[i] == inputBPS) bAdjustBitdepth = false;
		}
		
		if(bAdjustBitdepth)
		{
			unsigned int currentDiff = UINT_MAX, minimumDiff = UINT_MAX, bestBPS = UINT_MAX;
			const unsigned int originalBPS = (inputBPS == AudioFileModel::BITDEPTH_IEEE_FLOAT32) ? 32 : inputBPS;

			//Find the most suitable supported bit depth
			for(int i = 0; supportedBitdepths[i]; i++)
			{
				if(supportedBitdepths[i] == AudioFileModel::BITDEPTH_IEEE_FLOAT32) continue;
				
				currentDiff = DIFF(originalBPS, supportedBitdepths[i]);
				if((currentDiff < minimumDiff) || ((currentDiff == minimumDiff) && (bestBPS < supportedBitdepths[i])))
				{
					bestBPS = supportedBitdepths[i];
					minimumDiff = currentDiff;
					if(!(minimumDiff > 0)) break;
				}
			}

			if(bestBPS != originalBPS)
			{
				targetBitDepth = (bestBPS != UINT_MAX) ? bestBPS : supportedBitdepths[0];
			}
		}
	}

	//Check if downsampling filter is already in the chain
	if (targetSampleRate || targetBitDepth)
	{
		for (int i = 0; i < m_filters.count(); i++)
		{
			if (dynamic_cast<ResampleFilter*>(m_filters.at(i)))
			{
				qWarning("Encoder requires downsampling, but user has already set resamling filter!");
				handleMessage("WARNING: Encoder may need resampling, but already using resample filter. Encoding *may* fail!\n");
				targetSampleRate = targetBitDepth = 0;
			}
		}
	}

	/* Insert the filter */
	if(targetSampleRate || targetBitDepth)
	{
		m_filters.append(new ResampleFilter(targetSampleRate, targetBitDepth));
		return true;
	}

	return false; /*did not insert the resample filter */
}

bool ProcessThread::insertDownmixFilter(const unsigned int *const supportedChannels)
{
	//Determine number of channels in source
	const unsigned int channels = m_audioFile.techInfo().audioChannels();
	bool requiresDownmix = (channels > 0);

	//Check whether encoder requires downmixing
	if(requiresDownmix)
	{
		for (int i = 0; supportedChannels[i]; i++)
		{
			if (supportedChannels[i] == channels)
			{
				requiresDownmix = false;
				break;
			}
		}
	}

	//Check if downmixing filter is already in the chain
	if (requiresDownmix)
	{
		for (int i = 0; i < m_filters.count(); i++)
		{
			if (dynamic_cast<DownmixFilter*>(m_filters.at(i)))
			{
				qWarning("Encoder requires Stereo downmix, but user has already forced downmix!");
				handleMessage("WARNING: Encoder may need downmixning, but already using downmixning filter. Encoding *may* fail!\n");
				requiresDownmix = false;
				break;
			}
		}
	}

	//Now add the downmixing filter, if needed
	if(requiresDownmix)
	{
		m_filters.append(new DownmixFilter());
		return true;
	}

	return false; /*did not insert the downmix filter*/
}

bool ProcessThread::updateFileTime(const QString &originalFile, const QString &modifiedFile)
{
	bool success = false;

	QFileInfo originalFileInfo(originalFile);
	const QDateTime timeCreated = originalFileInfo.created(), timeLastMod = originalFileInfo.lastModified();
	if (timeCreated.isValid() && timeLastMod.isValid())
	{
		if (!MUtils::OS::set_file_time(modifiedFile, timeCreated, timeLastMod))
		{
			qWarning("Failed to update creation/modified time of output file: \"%s\"", MUTILS_UTF8(modifiedFile));
		}
	}
	else
	{
		qWarning("Failed to read creation/modified time of source file: \"%s\"", MUTILS_UTF8(originalFile));
	}

	return success;
}

////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
////////////////////////////////////////////////////////////

void ProcessThread::addFilter(AbstractFilter *filter)
{
	m_filters.append(filter);
}

void ProcessThread::setRenamePattern(const QString &pattern)
{
	const QString newPattern = pattern.simplified();
	if(!newPattern.isEmpty()) m_renamePattern = newPattern;
}

void ProcessThread::setRenameRegExp(const QString &search, const QString &replace)
{
	const QString newSearch = search.trimmed(), newReplace = replace.simplified();
	if((!newSearch.isEmpty()) && (!newReplace.isEmpty()))
	{
		m_renameRegExp_Search  = newSearch;
		m_renameRegExp_Replace = newReplace;
	}
}

void ProcessThread::setRenameFileExt(const QString &fileExtension)
{
	m_renameFileExt = MUtils::clean_file_name(fileExtension).simplified();
	while(m_renameFileExt.startsWith('.'))
	{
		m_renameFileExt = m_renameFileExt.mid(1).trimmed();
	}
}

void ProcessThread::setOverwriteMode(const bool &bSkipExistingFile, const bool &bReplacesExisting)
{
	if(bSkipExistingFile && bReplacesExisting)
	{
		qWarning("Inconsistent overwrite flags -> reverting to default!");
		m_overwriteMode = OverwriteMode_KeepBoth;
	}
	else
	{
		m_overwriteMode = OverwriteMode_KeepBoth;
		if(bSkipExistingFile) m_overwriteMode = OverwriteMode_SkipExisting;
		if(bReplacesExisting) m_overwriteMode = OverwriteMode_Overwrite;
	}
}

void ProcessThread::setKeepDateTime(const bool &keepDateTime)
{
	m_keepDateTime = keepDateTime;
}

////////////////////////////////////////////////////////////
// EVENTS
////////////////////////////////////////////////////////////

/*NONE*/