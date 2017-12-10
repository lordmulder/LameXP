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

#include "Filter_Normalize.h"

//Internal
#include "Global.h"

//MUtils
#include <MUtils/Global.h>
#include <MUtils/Exception.h>

//Qt
#include <QDir>
#include <QProcess>
#include <QRegExp>

static double dbToLinear(const double &value)
{
	return pow(10.0, value / 20.0);
}

NormalizeFilter::NormalizeFilter(const int &peakVolume, const bool &dnyAudNorm, const bool &channelsCoupled, const int &filterSize)
:
	m_binary(lamexp_tools_lookup("sox.exe")),
	m_useDynAudNorm(dnyAudNorm),
	m_peakVolume(qMin(-50, qMax(-3200, peakVolume))),
	m_channelsCoupled(channelsCoupled),
	m_filterLength(qBound(3, filterSize + (1 - (filterSize % 2)), 301))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing SoX filter. Tool 'sox.exe' is not registred!");
	}
}

NormalizeFilter::~NormalizeFilter(void)
{
}

AbstractFilter::FilterResult NormalizeFilter::apply(const QString &sourceFile, const QString &outputFile, AudioFileModel_TechInfo *const formatInfo, QAtomicInt &abortFlag)
{
	QProcess process;
	QStringList args;

	args << "-V3" << "-S";
	args << "--temp" << ".";
	args << QDir::toNativeSeparators(sourceFile);
	args << QDir::toNativeSeparators(outputFile);

	if(!m_useDynAudNorm)
	{
		args << "gain";
		args << (m_channelsCoupled ? "-n" : "-nb");
		args << QString().sprintf("%.2f", static_cast<double>(m_peakVolume) / 100.0);
	}
	else
	{
		args << "dynaudnorm";
		args << "-p" << QString().sprintf("%.2f", qBound(0.1, dbToLinear(static_cast<double>(m_peakVolume) / 100.0), 1.0));
		args << "-g" << QString().sprintf("%d", m_filterLength);
		if(!m_channelsCoupled)
		{
			args << "-n";
		}
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
					prevProgress = (newProgress < 99) ? (newProgress + 1) : newProgress;
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
	
	return AbstractFilter::FILTER_SUCCESS;
}
