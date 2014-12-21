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

#pragma once

//Internal
#include "Global.h"

//MUtils
#include <MUtils/CPUFeatures.h>

//Qt
#include <QThread>

////////////////////////////////////////////////////////////
// Splash Thread
////////////////////////////////////////////////////////////

class InitializationThread: public QThread
{
	Q_OBJECT

public:
	InitializationThread(const MUtils::CPUFetaures::cpu_info_t &cpuFeatures);

	bool getSuccess(void) { return !isRunning() && m_bSuccess; }
	bool getSlowIndicator(void) { return m_slowIndicator; }

	static void selfTest(void);

protected:
	void run(void);
	double doInit(const size_t threadCount = 0);

private:
	void delay(void);
	void initTranslations(void);
	void initNeroAac(void);
	void initFhgAac(void);
	void initQAac(void);

	bool m_bSuccess;
	MUtils::CPUFetaures::cpu_info_t m_cpuFeatures;
	bool m_slowIndicator;
};
