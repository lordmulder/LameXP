///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2016 LoRd_MuldeR <MuldeR2@GMX.de>
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
#include "ShellIntegration.h"

//MUitls
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>
#include <MUtils/Version.h>
#include <MUtils/CPUFeatures.h>
#include <MUtils/Terminal.h>
#include <MUtils/Startup.h>
#include <MUtils/IPCChannel.h>

//Qt includes
#include <QMutex>
#include <QApplication>
#include <QMessageBox>
#include <QDate>
#include <QDir>

//VLD
#ifdef _MSC_VER
#include <vld.h>
#endif

///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////

static void lamexp_print_logo(void)
{
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
}

static int lamexp_initialize_ipc(MUtils::IPCChannel *const ipcChannel)
{
	int iResult = 0;

	if((iResult = ipcChannel->initialize()) != MUtils::IPCChannel::RET_SUCCESS_MASTER)
	{
		if(iResult == MUtils::IPCChannel::RET_SUCCESS_SLAVE)
		{
			qDebug("LameXP is already running, connecting to running instance...");
			QScopedPointer<MessageProducerThread> messageProducerThread(new MessageProducerThread(ipcChannel));
			messageProducerThread->start();
			if(!messageProducerThread->wait(30000))
			{
				qWarning("MessageProducer thread has encountered timeout -> going to kill!");
				messageProducerThread->terminate();
				messageProducerThread->wait();
				MUtils::OS::system_message_err(L"LameXP", L"LameXP is already running, but the running instance doesn't respond!");
				return -1;
			}
			return 0;
		}
		else
		{
			qFatal("The IPC initialization has failed!");
			return -1;
		}
	}

	return 1;
}

static void lamexp_show_splash(const MUtils::CPUFetaures::cpu_info_t &cpuFeatures, SettingsModel *const settingsModel)
{
	QScopedPointer<InitializationThread> poInitializationThread(new InitializationThread(cpuFeatures));
	SplashScreen::showSplash(poInitializationThread.data());
	settingsModel->slowStartup(poInitializationThread->getSlowIndicator());
}

static int lamexp_main_loop(const MUtils::CPUFetaures::cpu_info_t &cpuFeatures, MUtils::IPCChannel *const ipcChannel, int &iShutdown)
{
	int iResult = -1;
	bool bAccepted = true;

	//Create models
	QScopedPointer<FileListModel>           fileListModel(new FileListModel()          );
	QScopedPointer<AudioFileModel_MetaInfo> metaInfo     (new AudioFileModel_MetaInfo());
	QScopedPointer<SettingsModel>           settingsModel(new SettingsModel()          );

	//Show splash screen
	lamexp_show_splash(cpuFeatures, settingsModel.data());

	//Validate settings
	settingsModel->validate();

	//Create main window
	QScopedPointer<MainWindow> poMainWindow(new MainWindow(ipcChannel, fileListModel.data(), metaInfo.data(), settingsModel.data()));

	//Main application loop
	while(bAccepted && (iShutdown <= SHUTDOWN_FLAG_NONE))
	{
		//Show main window
		poMainWindow->show();
		iResult = qApp->exec();
		bAccepted = poMainWindow->isAccepted();

		//Sync settings
		settingsModel->syncNow();

		//Show processing dialog
		if(bAccepted && (fileListModel->rowCount() > 0))
		{
			ProcessingDialog *processingDialog = new ProcessingDialog(fileListModel.data(), metaInfo.data(), settingsModel.data());
			processingDialog->exec();
			iShutdown = processingDialog->getShutdownFlag();
			MUTILS_DELETE(processingDialog);
		}
	}

	return iResult;
}

///////////////////////////////////////////////////////////////////////////////
// Main function
///////////////////////////////////////////////////////////////////////////////

static int lamexp_main(int &argc, char **argv)
{
	int iResult = -1;
	int iShutdown = SHUTDOWN_FLAG_NONE;

	//Print logo
	lamexp_print_logo();

	//Get CLI arguments
	const MUtils::OS::ArgumentMap &arguments = MUtils::OS::arguments();

	//Enumerate CLI arguments
	if(!arguments.isEmpty())
	{
		qDebug("Command-Line Arguments:");
		foreach(const QString &key, arguments.uniqueKeys())
		{
			foreach(const QString &val, arguments.values(key))
			{
				if(!val.isEmpty())
				{
					qDebug("--%s = \"%s\"", MUTILS_UTF8(key), MUTILS_UTF8(val));
					continue;
				}
				qDebug("--%s", MUTILS_UTF8(key));
			}
		}
		qDebug(" ");
	}

	//Uninstall?
	if(arguments.contains("uninstall"))
	{
		qWarning("Un-install: Removing LameXP shell integration...");
		ShellIntegration::remove(false);
		return EXIT_SUCCESS;
	}

	//Detect CPU capabilities
	const MUtils::CPUFetaures::cpu_info_t cpuFeatures = MUtils::CPUFetaures::detect();
	qDebug("   CPU vendor id  :  %s (Intel=%s)", cpuFeatures.vendor, MUTILS_BOOL2STR(cpuFeatures.intel));
	qDebug("CPU brand string  :  %s", cpuFeatures.brand);
	qDebug("   CPU signature  :  Family=%d Model=%d Stepping=%d", cpuFeatures.family, cpuFeatures.model, cpuFeatures.stepping);
	qDebug("CPU capabilities  :  MMX=%s SSE=%s SSE2=%s SSE3=%s SSSE3=%s SSE4=%s SSE4.2=%s x64=%s", MUTILS_BOOL2STR(cpuFeatures.features & MUtils::CPUFetaures::FLAG_MMX), MUTILS_BOOL2STR(cpuFeatures.features & MUtils::CPUFetaures::FLAG_SSE), MUTILS_BOOL2STR(cpuFeatures.features & MUtils::CPUFetaures::FLAG_SSE2), MUTILS_BOOL2STR(cpuFeatures.features & MUtils::CPUFetaures::FLAG_SSE3), MUTILS_BOOL2STR(cpuFeatures.features & MUtils::CPUFetaures::FLAG_SSSE3), MUTILS_BOOL2STR(cpuFeatures.features & MUtils::CPUFetaures::FLAG_SSE4), MUTILS_BOOL2STR(cpuFeatures.features & MUtils::CPUFetaures::FLAG_SSE42), MUTILS_BOOL2STR(cpuFeatures.x64));
	qDebug(" Number of CPU's  :  %d\n", cpuFeatures.count);

	//Initialize Qt
	QScopedPointer<QApplication> application(MUtils::Startup::create_qt(argc, argv, QLatin1String("LameXP - Audio Encoder Front-End")));
	if(application.isNull())
	{
		return EXIT_FAILURE;
	}

	//Initialize application
	application->setWindowIcon(lamexp_app_icon());
	application->setApplicationVersion(QString().sprintf("%d.%02d.%04d", lamexp_version_major(), lamexp_version_minor(), lamexp_version_build())); 

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

	//Initialize IPC
	QScopedPointer<MUtils::IPCChannel> ipcChannel(new MUtils::IPCChannel("lamexp-v4", lamexp_version_build(), "instance"));
	if((iResult = lamexp_initialize_ipc(ipcChannel.data())) < 1)
	{
		return (iResult == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	//Kill application?
	if(arguments.contains("kill") || arguments.contains("force-kill"))
	{
		return EXIT_SUCCESS;
	}
	
	//Self-test
	if(MUTILS_DEBUG)
	{
		InitializationThread::selfTest();
	}

	//Main application loop
	iResult = lamexp_main_loop(cpuFeatures, ipcChannel.data(), iShutdown);

	//Final clean-up
	qDebug("Shutting down, please wait...\n");

	//Shotdown computer
	if(iShutdown > SHUTDOWN_FLAG_NONE)
	{
		if(!MUtils::OS::shutdown_computer(QApplication::applicationFilePath(), 12, true, (iShutdown == SHUTDOWN_FLAG_HIBERNATE)))
		{
			QMessageBox messageBox(QMessageBox::Critical, "LameXP", "Sorry, LameXP was unable to shutdown your computer!", QMessageBox::NoButton, NULL, Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint);
		}
	}

	//Terminate
	return iResult;
}

///////////////////////////////////////////////////////////////////////////////
// Applicaton entry point
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	return MUtils::Startup::startup(argc, argv, lamexp_main, "LameXP", lamexp_version_demo());
}
