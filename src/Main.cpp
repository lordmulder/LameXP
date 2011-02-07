///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2011 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
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

//Qt includes
#include <QApplication>
#include <QMessageBox>
#include <QDate>
#include <QMutex>
#include <QDir>
#include <QInputDialog>

///////////////////////////////////////////////////////////////////////////////
// Main function
///////////////////////////////////////////////////////////////////////////////

int lamexp_main(int argc, char* argv[])
{
	int iResult = -1;
	bool bAccepted = true;
	
	//Init console
	lamexp_init_console(argc, argv);

	//Print version info
	qDebug("LameXP - Audio Encoder Front-End");
	qDebug("Version %d.%02d %s, Build %d [%s], compiled with %s", lamexp_version_major(), lamexp_version_minor(), lamexp_version_release(), lamexp_version_build(), lamexp_version_date().toString(Qt::ISODate).toLatin1().constData(), lamexp_version_compiler());
	qDebug("Copyright (C) 2004-%04d LoRd_MuldeR <MuldeR2@GMX.de>\n", max(lamexp_version_date().year(),QDate::currentDate().year()));
	
	//print license info
	qDebug("This program is free software: you can redistribute it and/or modify");
	qDebug("it under the terms of the GNU General Public License <http://www.gnu.org/>.");
	qDebug("This program comes with ABSOLUTELY NO WARRANTY.\n");

	//Print warning, if this is a "debug" build
	LAMEXP_CHECK_DEBUG_BUILD;
	
	//Detect CPU capabilities
	lamexp_cpu_t cpuFeatures = lamexp_detect_cpu_features();
	qDebug("   CPU vendor id  :  %s (Intel: %d)", cpuFeatures.vendor, (cpuFeatures.intel ? 1 : 0));
	qDebug("CPU brand string  :  %s", cpuFeatures.brand);
	qDebug("   CPU signature  :  Family: %d, Model: %d, Stepping: %d", cpuFeatures.family, cpuFeatures.model, cpuFeatures.stepping);
	qDebug("CPU capabilities  :  MMX: %s, SSE: %s, SSE2: %s, SSE3: %s, SSSE3: %s, x64: %s", LAMEXP_BOOL(cpuFeatures.mmx), LAMEXP_BOOL(cpuFeatures.sse), LAMEXP_BOOL(cpuFeatures.sse2), LAMEXP_BOOL(cpuFeatures.sse3), LAMEXP_BOOL(cpuFeatures.ssse3), LAMEXP_BOOL(cpuFeatures.x64));
	qDebug(" Number of CPU's  :  %d\n", cpuFeatures.count);
	
	//Initialize Qt
	if(!lamexp_init_qt(argc, argv))
	{
		return -1;
	}

	//Check for expiration
	if(lamexp_version_demo())
	{
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
				LAMEXP_DELETE(messageProducerThread);
				return -1;
			}
			LAMEXP_DELETE(messageProducerThread);
		}
		return 0;
	}

	//Kill application?
	for(int i = 0; i < argc; i++)
	{
		if(!_stricmp("--kill", argv[i]) || !_stricmp("--force-kill", argv[i]))
		{
			return 0;
		}
	}
	
	//Create models
	FileListModel *fileListModel = new FileListModel();
	AudioFileModel *metaInfo = new AudioFileModel();
	SettingsModel *settingsModel = new SettingsModel();
	
	//Show splash screen
	InitializationThread *poInitializationThread = new InitializationThread();
	SplashScreen::showSplash(poInitializationThread);
	LAMEXP_DELETE(poInitializationThread);

	//Validate settings
	settingsModel->validate();

	//Create main window
	MainWindow *poMainWindow = new MainWindow(fileListModel, metaInfo, settingsModel);
	
	//Main application loop
	while(bAccepted)
	{
		//Show main window
		poMainWindow->show();
		iResult = QApplication::instance()->exec();
		bAccepted = poMainWindow->isAccepted();

		//Show processing dialog
		if(bAccepted && fileListModel->rowCount() > 0)
		{
			ProcessingDialog *processingDialog = new ProcessingDialog(fileListModel, metaInfo, settingsModel);
			processingDialog->exec();
			LAMEXP_DELETE(processingDialog);
		}
	}
	
	//Free models
	LAMEXP_DELETE(poMainWindow);
	LAMEXP_DELETE(fileListModel);
	LAMEXP_DELETE(metaInfo);
	LAMEXP_DELETE(settingsModel);

	//Final clean-up
	qDebug("Shutting down, please wait...\n");

	//Terminate
	return iResult;
}

///////////////////////////////////////////////////////////////////////////////
// Applicaton entry point
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
#ifdef _DEBUG
	int iResult;
	qInstallMsgHandler(lamexp_message_handler);
	LAMEXP_MEMORY_CHECK(iResult = lamexp_main(argc, argv));
	lamexp_finalization();
	return iResult;
#else
	try
	{
		int iResult;
		qInstallMsgHandler(lamexp_message_handler);
		iResult = lamexp_main(argc, argv);
		lamexp_finalization();
		return iResult;
	}
	catch(char *error)
	{
		fflush(stdout);
		fflush(stderr);
		fprintf(stderr, "\nGURU MEDITATION: %s\n", error);
		FatalAppExit(0, L"Unhandeled exception error, application will exit!");
		TerminateProcess(GetCurrentProcess(), -1);
	}
	catch(int error)
	{
		fflush(stdout);
		fflush(stderr);
		fprintf(stderr, "\nGURU MEDITATION: Error code 0x%X\n", error);
		FatalAppExit(0, L"Unhandeled exception error, application will exit!");
		TerminateProcess(GetCurrentProcess(), -1);
	}
	catch(...)
	{
		fflush(stdout);
		fflush(stderr);
		fprintf(stderr, "\nGURU MEDITATION !!!\n");
		FatalAppExit(0, L"Unhandeled exception error, application will exit!");
		TerminateProcess(GetCurrentProcess(), -1);
	}
#endif
}
