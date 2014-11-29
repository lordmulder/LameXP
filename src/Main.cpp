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

//LameXP includes
#include "Global.h"
#include "Dialog_SplashScreen.h"
#include "Dialog_MainWindow.h"
#include "Dialog_Processing.h"
#include "Thread_Initialization.h"
#include "Thread_MessageProducer.h"
#include "Model_Settings.h"
#include "Model_FileList.h"
#include "Model_AudioFile.h"
#include "Encoder_Abstract.h"
#include "WinSevenTaskbar.h"

//MUitls
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>
#include <MUtils/Version.h>
#include <MUtils/CPUFeatures.h>
#include <MUtils/Terminal.h>
#include <MUtils/Startup.h>

//Qt includes
#include <QApplication>
#include <QMessageBox>
#include <QDate>
#include <QMutex>
#include <QDir>

///////////////////////////////////////////////////////////////////////////////
// Main function
///////////////////////////////////////////////////////////////////////////////

static int lamexp_main(int &argc, char **argv)
{
	int iResult = -1;
	int iShutdown = shutdownFlag_None;
	bool bAccepted = true;

	//Print version info
	qDebug("LameXP - Audio Encoder Front-End v%d.%02d %s (Build #%03d)", lamexp_version_major(), lamexp_version_minor(), lamexp_version_release(), lamexp_version_build());
	qDebug("Copyright (c) 2004-%04d LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.", qMax(MUtils::Version::app_build_date().year(), MUtils::OS::current_date().year()));
	qDebug("Built on %s at %s with %s for Win-%s.\n", MUTILS_UTF8(MUtils::Version::app_build_date().toString(Qt::ISODate)), MUTILS_UTF8(MUtils::Version::app_build_time().toString(Qt::ISODate)), MUtils::Version::compiler_version(), MUtils::Version::compiler_arch());
	
	//print license info
	qDebug("This program is free software: you can redistribute it and/or modify");
	qDebug("it under the terms of the GNU General Public License <http://www.gnu.org/>.");
	qDebug("Note that this program is distributed with ABSOLUTELY NO WARRANTY.\n");

	//Print library version
	qDebug("This application is powerd by MUtils library v%u.%02u (%s, %s).\n", MUtils::Version::lib_version_major(), MUtils::Version::lib_version_minor(), MUTILS_UTF8(MUtils::Version::lib_build_date().toString(Qt::ISODate)), MUTILS_UTF8(MUtils::Version::lib_build_time().toString(Qt::ISODate)));

	//Print warning, if this is a "debug" build
	if(MUTILS_DEBUG)
	{
		qWarning("---------------------------------------------------------");
		qWarning("DEBUG BUILD: DO NOT RELEASE THIS BINARY TO THE PUBLIC !!!");
		qWarning("---------------------------------------------------------\n");
	}
	
	//Get CLI arguments
	const QStringList &arguments = MUtils::OS::arguments();

	//Enumerate CLI arguments
	qDebug("Command-Line Arguments:");
	for(int i = 0; i < arguments.count(); i++)
	{
		qDebug("argv[%d]=%s", i, MUTILS_UTF8(arguments.at(i)));
	}
	qDebug(" ");

	//Detect CPU capabilities
	const MUtils::CPUFetaures::cpu_info_t cpuFeatures = MUtils::CPUFetaures::detect(MUtils::OS::arguments());
	qDebug("   CPU vendor id  :  %s (Intel=%s)", cpuFeatures.vendor, MUTILS_BOOL2STR(cpuFeatures.intel));
	qDebug("CPU brand string  :  %s", cpuFeatures.brand);
	qDebug("   CPU signature  :  Family=%d Model=%d Stepping=%d", cpuFeatures.family, cpuFeatures.model, cpuFeatures.stepping);
	qDebug("CPU capabilities  :  MMX=%s SSE=%s SSE2=%s SSE3=%s SSSE3=%s SSE4=%s SSE4.2=%s x64=%s", MUTILS_BOOL2STR(cpuFeatures.features & MUtils::CPUFetaures::FLAG_MMX), MUTILS_BOOL2STR(cpuFeatures.features & MUtils::CPUFetaures::FLAG_SSE), MUTILS_BOOL2STR(cpuFeatures.features & MUtils::CPUFetaures::FLAG_SSE2), MUTILS_BOOL2STR(cpuFeatures.features & MUtils::CPUFetaures::FLAG_SSE3), MUTILS_BOOL2STR(cpuFeatures.features & MUtils::CPUFetaures::FLAG_SSSE3), MUTILS_BOOL2STR(cpuFeatures.features & MUtils::CPUFetaures::FLAG_SSE4), MUTILS_BOOL2STR(cpuFeatures.features & MUtils::CPUFetaures::FLAG_SSE42), MUTILS_BOOL2STR(cpuFeatures.x64));
	qDebug(" Number of CPU's  :  %d\n", cpuFeatures.count);

	//Initialize Qt
	if(!MUtils::Startup::init_qt(argc, argv, QLatin1String("LameXP - Audio Encoder Front-End")))
	{
		lamexp_finalization();
		return -1;
	}

	//Initialize application
	MUtils::Terminal::set_icon(QIcon(":/icons/sound.png"));
	qApp->setWindowIcon(lamexp_app_icon());
	qApp->setApplicationVersion(QString().sprintf("%d.%02d.%04d", lamexp_version_major(), lamexp_version_minor(), lamexp_version_build())); 

	//Add the default translations
	lamexp_translation_init();

	//Check for expiration
	if(lamexp_version_demo())
	{
		const QDate currentDate = MUtils::OS::current_date();
		if(currentDate.addDays(1) < MUtils::Version::app_build_date())
		{
			qFatal("System's date (%s) is before LameXP build date (%s). Huh?", currentDate.toString(Qt::ISODate).toLatin1().constData(), MUtils::Version::app_build_date().toString(Qt::ISODate).toLatin1().constData());
		}
		qWarning(QString("Note: This demo (pre-release) version of LameXP will expire at %1.\n").arg(lamexp_version_expires().toString(Qt::ISODate)).toLatin1().constData());
	}

	//Check for multiple instances of LameXP
	if((iResult = lamexp_init_ipc()) != 0)
	{
		qDebug("LameXP is already running, connecting to running instance...");
		if(iResult == 1)
		{
			MessageProducerThread *messageProducerThread = new MessageProducerThread();
			messageProducerThread->start();
			if(!messageProducerThread->wait(30000))
			{
				messageProducerThread->terminate();
				QMessageBox messageBox(QMessageBox::Critical, "LameXP", "LameXP is already running, but the running instance doesn't respond!", QMessageBox::NoButton, NULL, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
				messageBox.exec();
				messageProducerThread->wait();
				MUTILS_DELETE(messageProducerThread);
				lamexp_finalization();
				return -1;
			}
			MUTILS_DELETE(messageProducerThread);
		}
		lamexp_finalization();
		return 0;
	}

	//Kill application?
	for(int i = 0; i < argc; i++)
	{
		if(!arguments[i].compare("--kill", Qt::CaseInsensitive) || !arguments[i].compare("--force-kill", Qt::CaseInsensitive))
		{
			lamexp_finalization();
			return 0;
		}
	}
	
	//Self-test
	if(MUTILS_DEBUG)
	{
		InitializationThread::selfTest();
	}

	//Taskbar init
	WinSevenTaskbar::init();

	//Create models
	FileListModel *fileListModel = new FileListModel();
	AudioFileModel_MetaInfo *metaInfo = new AudioFileModel_MetaInfo();
	SettingsModel *settingsModel = new SettingsModel();

	//Show splash screen
	InitializationThread *poInitializationThread = new InitializationThread(cpuFeatures);
	SplashScreen::showSplash(poInitializationThread);
	settingsModel->slowStartup(poInitializationThread->getSlowIndicator());
	MUTILS_DELETE(poInitializationThread);

	//Validate settings
	settingsModel->validate();

	//Create main window
	MainWindow *poMainWindow = new MainWindow(fileListModel, metaInfo, settingsModel);
	
	//Main application loop
	while(bAccepted && (iShutdown <= shutdownFlag_None))
	{
		//Show main window
		poMainWindow->show();
		iResult = QApplication::instance()->exec();
		bAccepted = poMainWindow->isAccepted();

		//Sync settings
		settingsModel->syncNow();

		//Show processing dialog
		if(bAccepted && (fileListModel->rowCount() > 0))
		{
			ProcessingDialog *processingDialog = new ProcessingDialog(fileListModel, metaInfo, settingsModel);
			processingDialog->exec();
			iShutdown = processingDialog->getShutdownFlag();
			MUTILS_DELETE(processingDialog);
		}
	}
	
	//Free models
	MUTILS_DELETE(poMainWindow);
	MUTILS_DELETE(fileListModel);
	MUTILS_DELETE(metaInfo);
	MUTILS_DELETE(settingsModel);

	//Taskbar un-init
	WinSevenTaskbar::uninit();

	//Final clean-up
	qDebug("Shutting down, please wait...\n");

	//Shotdown computer
	if(iShutdown > shutdownFlag_None)
	{
		if(!lamexp_shutdown_computer(QApplication::applicationFilePath(), 12, true, (iShutdown == shutdownFlag_Hibernate)))
		{
			QMessageBox messageBox(QMessageBox::Critical, "LameXP", "Sorry, LameXP was unable to shutdown your computer!", QMessageBox::NoButton, NULL, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
		}
	}

	//Terminate
	lamexp_finalization();
	return iResult;
}

///////////////////////////////////////////////////////////////////////////////
// Applicaton entry point
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	return MUtils::Startup::startup(argc, argv, lamexp_main);
}
