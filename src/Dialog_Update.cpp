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

#include "Dialog_Update.h"

//UIC includes
#include "UIC_UpdateDialog.h"

//LameXP includes
#include "Global.h"
#include "Dialog_LogView.h"
#include "Model_Settings.h"

//MUtils
#include <MUtils/UpdateChecker.h>
#include <MUtils/Version.h>
#include <MUtils/Exception.h>
#include <MUtils/Sound.h>
#include <MUtils/GUI.h>
#include <MUtils/OSSupport.h>
#include <MUtils/Taskbar7.h>

//Qt includes
#include <QClipboard>
#include <QFileDialog>
#include <QTimer>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QCloseEvent>
#include <QMovie>
#include <QMessageBox>

///////////////////////////////////////////////////////////////////////////////

#define SHOW_HINT(TEXT, ICON) do \
{ \
	ui->hintLabel->setText((TEXT)); \
	ui->hintIcon->setPixmap(QIcon((ICON)).pixmap(16,16)); \
	ui->hintIcon->show(); \
	ui->hintLabel->show(); \
} \
while(0)

#define UPDATE_TASKBAR(STATE, ICON) do \
{ \
	m_taskbar->setTaskbarState((STATE)); \
	m_taskbar->setOverlayIcon(&QIcon((ICON))); \
} \
while(0)

///////////////////////////////////////////////////////////////////////////////

UpdateDialog::UpdateDialog(const SettingsModel *const settings, QWidget *parent)
:
	QDialog(parent),
	ui(new Ui::UpdateDialog),
	m_taskbar(new MUtils::Taskbar7(parent)),
	m_settings(settings),
	m_logFile(new QStringList()),
	m_betaUpdates(settings ? (settings->autoUpdateCheckBeta() || lamexp_version_demo()) : lamexp_version_demo()),
	m_success(false),
	m_firstShow(true),
	m_updateReadyToInstall(false),
	m_updaterProcess(NULL),
	m_binaryUpdater(lamexp_tools_lookup("wupdate.exe")),
	m_binaryWGet(lamexp_tools_lookup("wget.exe")),
	m_binaryNC(lamexp_tools_lookup("nc.exe")),
	m_binaryGnuPG(lamexp_tools_lookup("gpgv.exe")),
	m_binaryKeys(lamexp_tools_lookup("keyring.gpg"))
{
	if(m_binaryUpdater.isEmpty() || m_binaryWGet.isEmpty() || m_binaryNC.isEmpty() || m_binaryGnuPG.isEmpty() || m_binaryKeys.isEmpty())
	{
		MUTILS_THROW("Tools not initialized correctly!");
	}

	//Init the dialog, from the .ui file
	ui->setupUi(this);
	setWindowFlags(windowFlags() ^ Qt::WindowContextHelpButtonHint);

	//Disable "X" button
	MUtils::GUI::enable_close_button(this, false);

	//Init animation
	m_animator.reset(new QMovie(":/images/Loading3.gif"));
	ui->labelAnimationCenter->setMovie(m_animator.data());
	m_animator->start();

	//Indicate beta updates
	if(m_betaUpdates)
	{
		setWindowTitle(windowTitle().append(" [Beta]"));
	}
	
	//Enable button
	connect(ui->retryButton, SIGNAL(clicked()), this, SLOT(checkForUpdates()));
	connect(ui->installButton, SIGNAL(clicked()), this, SLOT(applyUpdate()));
	connect(ui->infoLabel, SIGNAL(linkActivated(QString)), this, SLOT(linkActivated(QString)));
	connect(ui->logButton, SIGNAL(clicked()), this, SLOT(logButtonClicked()));

	//Enable progress bar
	connect(ui->progressBar, SIGNAL(valueChanged(int)), this, SLOT(progressBarValueChanged(int)));
}

UpdateDialog::~UpdateDialog(void)
{
	if(m_animator)
	{
		m_animator->stop();
	}

	if(!m_thread.isNull())
	{
		if(!m_thread->wait(1000))
		{
			m_thread->terminate();
			m_thread->wait();
		}
	}

	m_taskbar->setTaskbarState(MUtils::Taskbar7::TASKBAR_STATE_NONE);
	m_taskbar->setOverlayIcon(NULL);

	delete ui;
}

void UpdateDialog::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);
	
	if(m_firstShow)
	{
		if(m_thread.isNull())
		{
			m_thread.reset(new MUtils::UpdateChecker(m_binaryWGet, m_binaryNC, m_binaryGnuPG, m_binaryKeys, QLatin1String("LameXP"), lamexp_version_build(), m_betaUpdates));
			connect(m_thread.data(), SIGNAL(statusChanged(int)), this, SLOT(threadStatusChanged(int)));
			connect(m_thread.data(), SIGNAL(progressChanged(int)), this, SLOT(threadProgressChanged(int)));
			connect(m_thread.data(), SIGNAL(messageLogged(QString)), this, SLOT(threadMessageLogged(QString)));
			connect(m_thread.data(), SIGNAL(finished()), this, SLOT(threadFinished()));
			connect(m_thread.data(), SIGNAL(terminated()), this, SLOT(threadFinished()));
		}

		threadStatusChanged(m_thread->getUpdateStatus());
		ui->labelVersionInstalled->setText(QString("%1 %2 (%3)").arg(tr("Build"), QString::number(lamexp_version_build()), MUtils::Version::app_build_date().toString(Qt::ISODate)));
		ui->labelVersionLatest->setText(QString("(%1)").arg(tr("Unknown")));

		ui->installButton->setEnabled(false);
		ui->closeButton->setEnabled(false);
		ui->retryButton->setEnabled(false);
		ui->logButton->setEnabled(false);
		ui->retryButton->hide();
		ui->logButton->hide();
		ui->infoLabel->hide();
		ui->hintLabel->hide();
		ui->hintIcon->hide();
		ui->frameAnimation->hide();
		ui->cancelLabel->hide();
	
		ui->progressBar->setMaximum(m_thread->getMaximumProgress());
		ui->progressBar->setValue(0);

		m_updaterProcess = NULL;

		QTimer::singleShot(0, this, SLOT(updateInit()));
		m_firstShow = false;
	}
}

void UpdateDialog::closeEvent(QCloseEvent *event)
{
	if(!ui->closeButton->isEnabled())
	{
		event->ignore();
	}
	else
	{
		m_taskbar->setTaskbarState(MUtils::Taskbar7::TASKBAR_STATE_NONE);
		m_taskbar->setOverlayIcon(NULL);
	}
}

void UpdateDialog::keyPressEvent(QKeyEvent *e)
{
	if (e->key() == Qt::Key_Escape)
	{
		if (!m_thread.isNull() && m_thread->isRunning())
		{
			ui->cancelLabel->hide();
			ui->statusLabel->setText(tr("Stopping update check, please wait..."));
			m_thread->cancel();
		}
	}
	else if(e->key() == Qt::Key_F11)
	{
		if(ui->closeButton->isEnabled()) logButtonClicked();
	}
	else if((e->key() == Qt::Key_F12) && e->modifiers().testFlag(Qt::ControlModifier))
	{
		if(ui->closeButton->isEnabled()) testKnownHosts();
	}
	else
	{
		QDialog::keyPressEvent(e);
	}
}

bool UpdateDialog::event(QEvent *e)
{
	if((e->type() == QEvent::ActivationChange) && (m_updaterProcess != NULL))
	{
		MUtils::GUI::bring_to_front(m_updaterProcess);
	}
	return QDialog::event(e);
}

void UpdateDialog::updateInit(void)
{
	setMinimumSize(size());
	setMaximumHeight(height());
	QTimer::singleShot(0, this, SLOT(checkForUpdates()));
}

void UpdateDialog::checkForUpdates(void)
{
	if(m_thread->isRunning())
	{
		qWarning("Update in progress, cannot check for updates now!");
	}

	if(!MUtils::OS::user_is_admin())
	{
		qWarning("User is not in the \"admin\" group, cannot update!");
		QString message;
		message += QString("<nobr>%1</nobr><br>").arg(tr("Sorry, but only users in the \"Administrators\" group can install updates."));
		message += QString("<nobr>%1</nobr>").arg(tr("Please start application from an administrator account and try again!"));
		if(QMessageBox::critical(this, this->windowTitle(), message, tr("Discard"), tr("Ignore")) != 1)
		{
			ui->closeButton->setEnabled(true);
			close(); return;
		}
	}

	m_taskbar->setTaskbarState(MUtils::Taskbar7::TASKBAR_STATE_NORMAL);
	m_taskbar->setOverlayIcon(&QIcon(":/icons/transmit_blue.png"));

	ui->progressBar->setValue(0);
	ui->installButton->setEnabled(false);
	ui->closeButton->setEnabled(false);
	ui->retryButton->setEnabled(false);
	ui->logButton->setEnabled(false);
	if(ui->infoLabel->isVisible()) ui->infoLabel->hide();
	if(ui->hintLabel->isVisible()) ui->hintLabel->hide();
	if(ui->hintIcon->isVisible()) ui->hintIcon->hide();
	ui->cancelLabel->show();
	ui->frameAnimation->show();

	QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
	QApplication::setOverrideCursor(Qt::WaitCursor);

	m_logFile->clear();
	m_thread->start();
}

void UpdateDialog::threadStatusChanged(const int status)
{
	switch(status)
	{
	case MUtils::UpdateChecker::UpdateStatus_NotStartedYet:
		ui->statusLabel->setText(tr("Initializing, please wait..."));
		break;
	case MUtils::UpdateChecker::UpdateStatus_CheckingConnection:
		ui->statusLabel->setText(tr("Testing your internet connection, please wait..."));
		break;
	case MUtils::UpdateChecker::UpdateStatus_FetchingUpdates:
		ui->statusLabel->setText(tr("Checking for new updates online, please wait..."));
		break;
	case MUtils::UpdateChecker::UpdateStatus_CompletedUpdateAvailable:
		ui->statusLabel->setText(tr("A new version of LameXP is available!"));
		SHOW_HINT(tr("We highly recommend all users to install this update as soon as possible."), ":/icons/shield_exclamation.png");
		UPDATE_TASKBAR(MUtils::Taskbar7::TASKBAR_STATE_NORMAL, ":/icons/shield_exclamation.png");
		break;
	case MUtils::UpdateChecker::UpdateStatus_CompletedNoUpdates:
		ui->statusLabel->setText(tr("No new updates available at this time."));
		SHOW_HINT(tr("Your version of LameXP is still up-to-date. Please check for updates regularly!"), ":/icons/shield_green.png");
		UPDATE_TASKBAR(MUtils::Taskbar7::TASKBAR_STATE_NORMAL, ":/icons/shield_green.png");
		break;
	case MUtils::UpdateChecker::UpdateStatus_CompletedNewVersionOlder:
		ui->statusLabel->setText(tr("Your version appears to be newer than the latest release."));
		SHOW_HINT(tr("This usually indicates your are currently using a pre-release version of LameXP."), ":/icons/shield_blue.png");
		UPDATE_TASKBAR(MUtils::Taskbar7::TASKBAR_STATE_NORMAL, ":/icons/shield_error.png");
		break;
	case MUtils::UpdateChecker::UpdateStatus_ErrorNoConnection:
		ui->statusLabel->setText(tr("It appears that the computer currently is offline!"));
		SHOW_HINT(tr("Please make sure your computer is connected to the internet and try again."), ":/icons/network_error.png");
		UPDATE_TASKBAR(MUtils::Taskbar7::TASKBAR_STATE_NORMAL, ":/icons/exclamation.png");
		break;
	case MUtils::UpdateChecker::UpdateStatus_ErrorConnectionTestFailed:
		ui->statusLabel->setText(tr("Network connectivity test has failed!"));
		SHOW_HINT(tr("Please make sure your computer is connected to the internet and try again."), ":/icons/network_error.png");
		UPDATE_TASKBAR(MUtils::Taskbar7::TASKBAR_STATE_NORMAL, ":/icons/exclamation.png");
		break;
	case MUtils::UpdateChecker::UpdateStatus_ErrorFetchUpdateInfo:
		ui->statusLabel->setText(tr("Failed to fetch update information from server!"));
		SHOW_HINT(tr("Sorry, the update server might be busy at this time. Plase try again later."), ":/icons/server_error.png");
		UPDATE_TASKBAR(MUtils::Taskbar7::TASKBAR_STATE_NORMAL, ":/icons/exclamation.png");
		break;
	case MUtils::UpdateChecker::UpdateStatus_CancelledByUser:
		ui->statusLabel->setText(tr("Update check has been cancelled!"));
		SHOW_HINT(tr("The update check has been cancelled by the user. Please try again later."), ":/icons/server_error.png");
		UPDATE_TASKBAR(MUtils::Taskbar7::TASKBAR_STATE_NORMAL, ":/icons/exclamation.png");
		break;
	default:
		qWarning("Unknown status %d !!!", int(status));
	}
}

void UpdateDialog::threadProgressChanged(const int progress)
{
	ui->progressBar->setValue(progress);
}

void UpdateDialog::threadMessageLogged(const QString &message)
{
	(*m_logFile) << message;
}

void UpdateDialog::threadFinished(void)
{
	const bool bSuccess = m_thread->getSuccess();
	
	ui->closeButton->setEnabled(true);
	ui->cancelLabel->hide();
	if(ui->frameAnimation->isVisible()) ui->frameAnimation->hide();
	ui->progressBar->setValue(ui->progressBar->maximum());

	if(!bSuccess)
	{
		if(m_settings->soundsEnabled()) MUtils::Sound::play_sound("error", true);
	}
	else
	{
		const bool bHaveUpdate = (m_thread->getUpdateStatus() == MUtils::UpdateChecker::UpdateStatus_CompletedUpdateAvailable);
		ui->installButton->setEnabled(bHaveUpdate);
		MUtils::Sound::beep(bHaveUpdate ? MUtils::Sound::BEEP_NFO : MUtils::Sound::BEEP_WRN);

		if(const MUtils::UpdateCheckerInfo *const updateInfo = m_thread->getUpdateInfo())
		{
			ui->infoLabel->setText(QString("%1<br><a href=\"%2\">%2</a>").arg(tr("More information available at:"), updateInfo->getDownloadSite()));
			ui->labelVersionLatest->setText(QString("%1 %2 (%3)").arg(tr("Build"), QString::number(updateInfo->getBuildNo()), updateInfo->getBuildDate().toString(Qt::ISODate)));
			ui->infoLabel->show();
		}

		m_success = true;
	}

	ui->retryButton->setVisible(!bSuccess);
	ui->logButton->setVisible(!bSuccess);
	ui->retryButton->setEnabled(!bSuccess);
	ui->logButton->setEnabled(!bSuccess);

	QApplication::restoreOverrideCursor();
}

void UpdateDialog::linkActivated(const QString &link)
{
	QDesktopServices::openUrl(QUrl(link));
}

void UpdateDialog::applyUpdate(void)
{
	ui->installButton->setEnabled(false);
	ui->closeButton->setEnabled(false);
	ui->retryButton->setEnabled(false);

	if(const MUtils::UpdateCheckerInfo *updateInfo = m_thread->getUpdateInfo())
	{
		ui->statusLabel->setText(tr("Update is being downloaded, please be patient..."));
		ui->frameAnimation->show();
		if(ui->hintLabel->isVisible()) ui->hintLabel->hide();
		if(ui->hintIcon->isVisible()) ui->hintIcon->hide();
		int oldMax = ui->progressBar->maximum();
		int oldMin = ui->progressBar->minimum();
		ui->progressBar->setRange(0, 0);
		QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
		
		QProcess process;
		QStringList args;
		QEventLoop loop;

		MUtils::init_process(process, QFileInfo(m_binaryUpdater).absolutePath(), false);

		connect(&process, SIGNAL(error(QProcess::ProcessError)), &loop, SLOT(quit()));
		connect(&process, SIGNAL(finished(int,QProcess::ExitStatus)), &loop, SLOT(quit()));

		args << QString("/Location=%1").arg(updateInfo->getDownloadAddress());
		args << QString("/Filename=%1").arg(updateInfo->getDownloadFilename());
		args << QString("/TicketID=%1").arg(updateInfo->getDownloadFilecode());
		args << QString("/CheckSum=%1").arg(updateInfo->getDownloadChecksum());
		args << QString("/ToFolder=%1").arg(QDir::toNativeSeparators(QDir(QApplication::applicationDirPath()).canonicalPath()));
		args << QString("/ToExFile=%1.exe").arg(QFileInfo(QFileInfo(QApplication::applicationFilePath()).canonicalFilePath()).completeBaseName());
		args << QString("/AppTitle=LameXP (Build #%1)").arg(QString::number(updateInfo->getBuildNo()));

		QApplication::setOverrideCursor(Qt::WaitCursor);
		UPDATE_TASKBAR(MUtils::Taskbar7::TASKBAR_STATE_INTERMEDIATE, ":/icons/transmit_blue.png");

		process.start(m_binaryUpdater, args);
		bool updateStarted = process.waitForStarted();
		if(updateStarted)
		{
			m_updaterProcess = MUtils::OS::process_id(&process);
			loop.exec(QEventLoop::ExcludeUserInputEvents);
		}

		m_updaterProcess = NULL;
		QApplication::restoreOverrideCursor();

		ui->hintLabel->show();
		ui->hintIcon->show();
		ui->progressBar->setRange(oldMin, oldMax);
		ui->progressBar->setValue(oldMax);
		ui->frameAnimation->hide();

		if(updateStarted && (process.exitCode() == 0))
		{
			ui->statusLabel->setText(tr("Update ready to install. Applicaion will quit..."));
			m_updateReadyToInstall = true;
			m_taskbar->setTaskbarState(MUtils::Taskbar7::TASKBAR_STATE_NONE);
			m_taskbar->setOverlayIcon(NULL);
			accept();
		}
		else
		{
			ui->statusLabel->setText(tr("Update failed. Please try again or download manually!"));
			m_taskbar->setTaskbarState(MUtils::Taskbar7::TASKBAR_STATE_ERROR);
			m_taskbar->setOverlayIcon(&QIcon(":/icons/exclamation.png"));
			m_taskbar->setTaskbarProgress(100, 100);
		}
	}

	ui->installButton->setEnabled(true);
	ui->closeButton->setEnabled(true);
}

void UpdateDialog::logButtonClicked(void)
{
	LogViewDialog *logView = new LogViewDialog(this);
	logView->exec(*m_logFile);
	MUTILS_DELETE(logView);
}

void UpdateDialog::progressBarValueChanged(int value)
{
	m_taskbar->setTaskbarProgress(value, ui->progressBar->maximum());
}

void UpdateDialog::testKnownHosts(void)
{
	ui->statusLabel->setText("Testing all known hosts, this may take a few minutes...");
	
	if(MUtils::UpdateChecker *testThread = new MUtils::UpdateChecker(m_binaryWGet, m_binaryNC, m_binaryGnuPG, m_binaryKeys, QLatin1String("LameXP"), lamexp_version_build(), m_betaUpdates, true))
	{
		QEventLoop loop;
		m_logFile->clear();

		connect(testThread, SIGNAL(messageLogged(QString)), this, SLOT(threadMessageLogged(QString)));
		connect(testThread, SIGNAL(finished()), &loop, SLOT(quit()));
		connect(testThread, SIGNAL(terminated()), &loop, SLOT(quit()));

		testThread->start();
		while(testThread->isRunning())
		{
			QTimer::singleShot(5000, &loop, SLOT(quit()));
			loop.exec(QEventLoop::ExcludeUserInputEvents);
		}

		MUTILS_DELETE(testThread);
		logButtonClicked();
	}

	ui->statusLabel->setText("Test completed.");
	MUtils::Sound::beep(MUtils::Sound::BEEP_NFO);
}
