///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2025 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Filter_Resample.h"

//Internal
#include "Global.h"
#include "Model_AudioFile.h"

//MUtils
#include <MUtils/Exception.h>

//Qt
#include <QDir>
#include <QProcess>
#include <QRegExp>

static __inline int multipleOf(int value, int base)
{
	return qRound(static_cast<double>(value) / static_cast<double>(base)) * base;
}

ResampleFilter::ResampleFilter(int samplingRate, int bitDepth)
:
	m_binary(lamexp_tools_lookup("sox.exe"))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing SoX filter. Tool 'sox.exe' is not registred!");
	}

	m_samplingRate = (samplingRate > 0) ? qBound(8000, samplingRate, 192000) : 0;
	m_bitDepth = (bitDepth > 0) ? qBound(8, multipleOf(bitDepth, 8), 32) : 0;

	if((m_samplingRate == 0) && (m_bitDepth == 0))
	{
		qWarning("ResampleFilter: Nothing to do, filter will be NOP!");
	}
}

ResampleFilter::~ResampleFilter(void)
{
}

AbstractFilter::FilterResult ResampleFilter::apply(const QString &sourceFile, const QString &outputFile, AudioFileModel_TechInfo *const formatInfo, QAtomicInt &abortFlag)
{
	QProcess process;
	QStringList args;

	if((m_samplingRate == static_cast<int>(formatInfo->audioSamplerate())) && (m_bitDepth == static_cast<int>(formatInfo->audioBitdepth())))
	{
		messageLogged("Skipping resample filter!");
		qDebug("Resampling filter target samplerate/bitdepth is equals to the format of the input file, skipping!");
		return AbstractFilter::FILTER_SKIPPED;
	}

	args << "-V3" << "-S";
	args << "--guard" << "--temp" << ".";
	args << QDir::toNativeSeparators(sourceFile);

	if(m_bitDepth)
	{
		args << "-b" << QString::number(m_bitDepth);
	}

	args << QDir::toNativeSeparators(outputFile);

	if(m_samplingRate)
	{
		args << "rate";
		args << ((m_bitDepth > 16) ? "-v" : "-h");			//if resampling at/to > 16 bit depth (i.e. most commonly 24-bit), use VHQ (-v), otherwise, use HQ (-h)
		args << ((m_samplingRate > 40000) ? "-L" : "-I");	//if resampling to < 40k, use intermediate phase (-I), otherwise use linear phase (-L)
		args << QString::number(m_samplingRate);
	}

	if((m_bitDepth || m_samplingRate) && (m_bitDepth <= 16))
	{
		args << "dither" << "-s";					//if you're mastering to 16-bit, you also need to add 'dither' (and in most cases noise-shaping) after the rate
	}

	if(!startProcess(process, m_binary, args, QFileInfo(outputFile).canonicalPath()))
	{
		return AbstractFilter::FILTER_FAILURE;
	}

	int prevProgress = -1;
	QRegExp regExp("In:(\\d+)(\\.\\d+)*%");

	const result_t result = awaitProcess(process, abortFlag, [this, &prevProgress, &regExp](const QString &text)
	{
		if (regExp.lastIndexIn(text) >= 0)
		{
			qint32 newProgress;
			if (MUtils::regexp_parse_int32(regExp, newProgress))
			{
				if (newProgress > prevProgress)
				{
					emit statusUpdated(newProgress);
					prevProgress = NEXT_PROGRESS(newProgress);
				}
			}
			return true;
		}
		return false;
	});

	if (result != RESULT_SUCCESS)
	{
		return AbstractFilter::FILTER_FAILURE;
	}
	
	if (m_samplingRate)
	{
		formatInfo->setAudioSamplerate(m_samplingRate);
	}
	if (m_bitDepth)
	{
		formatInfo->setAudioBitdepth(m_bitDepth);
	}

	return AbstractFilter::FILTER_SUCCESS;
}
