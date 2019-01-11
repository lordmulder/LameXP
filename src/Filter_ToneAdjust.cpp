///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2019 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Filter_ToneAdjust.h"

//Internal
#include "Global.h"

//MUtils
#include <MUtils/Exception.h>

//Qt
#include <QDir>
#include <QProcess>
#include <QRegExp>
#include <QFileInfo>

ToneAdjustFilter::ToneAdjustFilter(int bass, int treble)
:
	m_binary(lamexp_tools_lookup("sox.exe"))
{
	if(m_binary.isEmpty())
	{
		MUTILS_THROW("Error initializing SoX filter. Tool 'sox.exe' is not registred!");
	}

	m_bass = qMax(-2000, qMin(2000, bass));
	m_treble = qMax(-2000, qMin(2000, treble));
}

ToneAdjustFilter::~ToneAdjustFilter(void)
{
}

AbstractFilter::FilterResult ToneAdjustFilter::apply(const QString &sourceFile, const QString &outputFile, AudioFileModel_TechInfo *const formatInfo, QAtomicInt &abortFlag)
{
	QProcess process;
	QStringList args;

	args << "-V3" << "-S";
	args << "--guard" << "--temp" << ".";
	args << QDir::toNativeSeparators(sourceFile);
	args << QDir::toNativeSeparators(outputFile);

	if(m_bass != 0)
	{
		args << "bass" << QString().sprintf("%s%.2f", ((m_bass < 0) ? "-" : "+"), static_cast<double>(abs(m_bass)) / 100.0);
	}
	if(m_treble != 0)
	{
		args << "treble" << QString().sprintf("%s%.2f", ((m_treble < 0) ? "-" : "+"), static_cast<double>(abs(m_treble)) / 100.0);
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
	
	return AbstractFilter::FILTER_SUCCESS;
}
