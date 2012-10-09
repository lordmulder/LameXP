///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2012 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Dialog_Update.h"

#include "Global.h"
#include "Resource.h"
#include "Dialog_LogView.h"
#include "Model_Settings.h"
#include "WinSevenTaskbar.h"

#include <QClipboard>
#include <QFileDialog>
#include <QTimer>
#include <QProcess>
#include <QDate>
#include <QRegExp>
#include <QDesktopServices>
#include <QUrl>
#include <QCloseEvent>
#include <QMovie>
#include <QtConcurrentRun>

#include <time.h>
#include <MMSystem.h>
#include <WinInet.h>
#include <process.h>

///////////////////////////////////////////////////////////////////////////////

static const char *header_id = "!Update";
static const char *section_id = "LameXP";

static const char *mirror_url_postfix[] = 
{
	"update.ver",
	"update_beta.ver",
	NULL
};

static const char *update_mirrors_prim[] =
{
	"http://mulder.brhack.net/",
	"http://mulder.bplaced.net/",
	"http://lamexp.sourceforge.net/",
	"http://free.pages.at/borschdfresser/",
	"http://lordmulder.github.com/LameXP/",
	"http://lord_mulder.bitbucket.org/",
	"http://www.tricksoft.de/",
	NULL
};

static const char *update_mirrors_back[] =
{
	"http://mplayer.savedonthe.net/",
	"http://www.rarewares.org/",
	"http://lord_mulder.doom9.net/",
	NULL
};

static const char *known_hosts[] =		//Taken form: http://www.alexa.com/topsites
{
	"http://www.163.com/",
	"http://www.360buy.com/",
	"http://www.amazon.com/",
	"http://www.aol.com/",
	"http://www.apache.org/",
	"http://www.apple.com/",
	"http://www.adobe.com/",
	"http://www.avidemux.org/",
	"http://www.babylon.com/",
	"http://www.baidu.com/",
	"http://www.bbc.co.uk/",
	"http://www.bing.com/",
	"http://www.cnet.com/",
	"http://cnzz.com/",
	"http://qt.digia.com/",
	"http://www.ebay.com/",
	"http://www.equation.com/",
	"http://fc2.com/",
	"http://www.ffmpeg.org/",
	"http://www.flickr.com/",
	"http://www.gitorious.org/",
	"http://git-scm.com/",
	"http://www.gnome.org/",
	"http://www.gnu.org/",
	"http://go.com/",
	"http://code.google.com/",
	"http://www.heise.de/",
	"http://www.huffingtonpost.co.uk/",
	"http://www.iana.org/",
	"http://www.imdb.com/",
	"http://www.imgburn.com/",
	"http://imgur.com/",
	"http://www.kernel.org/",
	"http://www.libav.org/",
	"http://www.linkedin.com/",
	"http://www.livedoor.com/",
	"http://www.livejournal.com/",
	"http://mail.ru/",
	"http://www.mediafire.com/",
	"http://www.mozilla.org/en-US/",
	"http://mplayerhq.hu/",
	"http://www.msn.com/?st=1",
	"http://oss.netfarm.it/",
	"http://www.nytimes.com/",
	"http://www.opera.com/",
	"http://www.quakelive.com/",
	"http://qt-project.org/",
	"http://www.seamonkey-project.org/",
	"http://www.sina.com.cn/",
	"http://www.sohu.com/",
	"http://www.soso.com/",
	"http://sourceforge.net/",
	"http://www.spiegel.de/",
	"http://tdm-gcc.tdragon.net/",
	"http://www.tdrsmusic.com/",
	"http://www.ubuntu.com/",
	"http://twitter.com/",
	"http://www.uol.com.br/",
	"http://www.videohelp.com/",
	"http://www.videolan.org/",
	"http://www.weibo.com/",
	"http://www.wikipedia.org/",
	"http://wordpress.com/",
	"http://us.yahoo.com/",
	"http://www.yandex.ru/",
	"http://www.youtube.com/",
	"http://www.zedo.com/",
	"http://ffmpeg.zeranoe.com/",
	NULL
};

static const int MIN_CONNSCORE = 8;
static const int VERSION_INFO_EXPIRES_MONTHS = 6;
static char *USER_AGENT_STR = "Mozilla/5.0 (X11; Linux i686; rv:7.0.1) Gecko/20111106 IceCat/7.0.1";

static BOOL getInternetConnectedState(void)
{
	DWORD lpdwFlags = NULL;
	BOOL result = InternetGetConnectedState(&lpdwFlags, NULL);
	return result;
}

static BOOL CALLBACK focusUpdaterWindow(HWND hwnd, LPARAM lParam)
{
	DWORD processId = *reinterpret_cast<WORD*>(lParam);
	DWORD windowProcessId = NULL;
	GetWindowThreadProcessId(hwnd, &windowProcessId);
	if(windowProcessId == processId)
	{
		SwitchToThisWindow(hwnd, TRUE);
		SetForegroundWindow(hwnd);
		return FALSE;
	}

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////

class UpdateInfo
{
public:
	UpdateInfo(void) { resetInfo(); }
	
	void resetInfo(void)
	{
		m_buildNo = 0;
		m_buildDate.setDate(1900, 1, 1);
		m_downloadSite.clear();
		m_downloadAddress.clear();
		m_downloadFilename.clear();
		m_downloadFilecode.clear();
	}

	unsigned int m_buildNo;
	QDate m_buildDate;
	QString m_downloadSite;
	QString m_downloadAddress;
	QString m_downloadFilename;
	QString m_downloadFilecode;
};

///////////////////////////////////////////////////////////////////////////////

UpdateDialog::UpdateDialog(SettingsModel *settings, QWidget *parent)
:
	QDialog(parent),
	m_binaryWGet(lamexp_lookup_tool("wget.exe")),
	m_binaryGnuPG(lamexp_lookup_tool("gpgv.exe")),
	m_binaryUpdater(lamexp_lookup_tool("wupdate.exe")),
	m_binaryKeys(lamexp_lookup_tool("gpgv.gpg")),
	m_updateInfo(NULL),
	m_settings(settings),
	m_logFile(new QStringList()),
	m_betaUpdates(settings ? (settings->autoUpdateCheckBeta() || lamexp_version_demo()) : lamexp_version_demo()),
	m_success(false),
	m_firstShow(true),
	m_updateReadyToInstall(false),
	m_updaterProcess(NULL)
{
	if(m_binaryWGet.isEmpty() || m_binaryGnuPG.isEmpty() || m_binaryUpdater.isEmpty() || m_binaryKeys.isEmpty())
	{
		throw "Tools not initialized correctly!";
	}
	
	//Init the dialog, from the .ui file
	setupUi(this);
	setWindowFlags(windowFlags() ^ Qt::WindowContextHelpButtonHint);

	//Disable "X" button
	HMENU hMenu = GetSystemMenu((HWND) winId(), FALSE);
	EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);

	//Init animation
	m_animator = new QMovie(":/images/Loading3.gif");
	labelAnimationCenter->setMovie(m_animator);
	m_animator->start();

	//Indicate beta updates
	if(m_betaUpdates)
	{
		setWindowTitle(windowTitle().append(" [Beta]"));
	}
	
	//Enable button
	connect(retryButton, SIGNAL(clicked()), this, SLOT(checkForUpdates()));
	connect(installButton, SIGNAL(clicked()), this, SLOT(applyUpdate()));
	connect(infoLabel, SIGNAL(linkActivated(QString)), this, SLOT(linkActivated(QString)));
	connect(logButton, SIGNAL(clicked()), this, SLOT(logButtonClicked()));

	//Enable progress bar
	connect(progressBar, SIGNAL(valueChanged(int)), this, SLOT(progressBarValueChanged(int)));
}

UpdateDialog::~UpdateDialog(void)
{
	if(m_animator) m_animator->stop();
	
	LAMEXP_DELETE(m_updateInfo);
	LAMEXP_DELETE(m_logFile);
	LAMEXP_DELETE(m_animator);

	WinSevenTaskbar::setTaskbarState(this->parentWidget(), WinSevenTaskbar::WinSevenTaskbarNoState);
	WinSevenTaskbar::setOverlayIcon(this->parentWidget(), NULL);
}

void UpdateDialog::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);
	
	if(m_firstShow)
	{
		labelVersionInstalled->setText(QString("%1 %2 (%3)").arg(tr("Build"), QString::number(lamexp_version_build()), lamexp_version_date().toString(Qt::ISODate)));
		labelVersionLatest->setText(QString("(%1)").arg(tr("Unknown")));

		installButton->setEnabled(false);
		closeButton->setEnabled(false);
		retryButton->setEnabled(false);
		logButton->setEnabled(false);
		retryButton->hide();
		logButton->hide();
		infoLabel->hide();
		hintLabel->hide();
		hintIcon->hide();
		frameAnimation->hide();
	
		int counter = MIN_CONNSCORE + 2;
		for(int i = 0; update_mirrors_prim[i]; i++) counter++;
		for(int i = 0; update_mirrors_back[i]; i++) counter++;

		progressBar->setMaximum(counter);
		progressBar->setValue(0);

		m_updaterProcess = NULL;

		QTimer::singleShot(0, this, SLOT(updateInit()));
		m_firstShow = false;
	}
}

void UpdateDialog::closeEvent(QCloseEvent *event)
{
	if(!closeButton->isEnabled())
	{
		event->ignore();
	}
	else
	{
		WinSevenTaskbar::setTaskbarState(this->parentWidget(), WinSevenTaskbar::WinSevenTaskbarNoState);
		WinSevenTaskbar::setOverlayIcon(this->parentWidget(), NULL);
	}
}

void UpdateDialog::keyPressEvent(QKeyEvent *e)
{
	if(e->key() == Qt::Key_F11)
	{
		if(closeButton->isEnabled()) logButtonClicked();
	}
	else if((e->key() == Qt::Key_F12) && e->modifiers().testFlag(Qt::ControlModifier))
	{
		if(closeButton->isEnabled())
		{
			testKnownWebSites();
			logButtonClicked();
		}
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
		EnumWindows(focusUpdaterWindow, reinterpret_cast<LPARAM>(&m_updaterProcess));
	}
	return QDialog::event(e);
}

bool UpdateDialog::winEvent(MSG *message, long *result)
{
	return WinSevenTaskbar::handleWinEvent(message, result);
}

void UpdateDialog::updateInit(void)
{
	setMinimumSize(size());
	setMaximumHeight(height());
	QApplication::processEvents();
	checkForUpdates();
}

void UpdateDialog::checkForUpdates(void)
{
	bool success = false;
	int connectionScore = 0;

	// ----- Initialization ----- //

	m_updateInfo = new UpdateInfo;

	progressBar->setValue(0);
	WinSevenTaskbar::setTaskbarState(this->parentWidget(), WinSevenTaskbar::WinSevenTaskbarNormalState);
	WinSevenTaskbar::setOverlayIcon(this->parentWidget(), &QIcon(":/icons/transmit_blue.png"));
	installButton->setEnabled(false);
	closeButton->setEnabled(false);
	retryButton->setEnabled(false);
	logButton->setEnabled(false);
	if(infoLabel->isVisible()) infoLabel->hide();
	if(hintLabel->isVisible()) hintLabel->hide();
	if(hintIcon->isVisible()) hintIcon->hide();
	frameAnimation->show();

	QApplication::processEvents();
	QApplication::setOverrideCursor(Qt::WaitCursor);

	// ----- Test Internet Connection ----- //

	statusLabel->setText(tr("Testing your internet connection, please wait..."));

	m_logFile->clear();
	m_logFile->append("Checking internet connection...");
	
	QFuture<BOOL> connectedState = QtConcurrent::run(getInternetConnectedState);
	while(!connectedState.isFinished())
	{
		QApplication::processEvents(QEventLoop::WaitForMoreEvents);
	}

	if(!connectedState.result())
	{
		m_logFile->append(QStringList() << "" << "Operating system reports that the computer is currently offline !!!");
		if(!retryButton->isVisible()) retryButton->show();
		if(!logButton->isVisible()) logButton->show();
		closeButton->setEnabled(true);
		retryButton->setEnabled(true);
		logButton->setEnabled(true);
		if(frameAnimation->isVisible()) frameAnimation->hide();
		statusLabel->setText(tr("It appears that the computer currently is offline!"));
		progressBar->setValue(progressBar->maximum());
		hintIcon->setPixmap(QIcon(":/icons/network_error.png").pixmap(16,16));
		hintLabel->setText(tr("Please make sure your computer is connected to the internet and try again."));
		hintIcon->show();
		hintLabel->show();
		LAMEXP_DELETE(m_updateInfo);
		if(m_settings->soundsEnabled()) PlaySound(MAKEINTRESOURCE(IDR_WAVE_ERROR), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
		QApplication::restoreOverrideCursor();
		progressBar->setValue(progressBar->maximum());
		WinSevenTaskbar::setTaskbarState(this->parentWidget(), WinSevenTaskbar::WinSevenTaskbarErrorState);
		WinSevenTaskbar::setOverlayIcon(this->parentWidget(), &QIcon(":/icons/exclamation.png"));
		return;
	}
	
	progressBar->setValue(1);
	QApplication::processEvents();

	// ----- Test Known Hosts Connectivity ----- //

	QStringList hostList;
	for(int i = 0; known_hosts[i]; i++)
	{
		hostList << QString::fromLatin1(known_hosts[i]);
	}

	qsrand(lamexp_mix(clock(), time(NULL), _getpid()));
	while(!hostList.isEmpty())
	{
		QString currentHost = hostList.takeAt(qrand() % hostList.count());
		if(connectionScore < MIN_CONNSCORE)
		{
			m_logFile->append(QStringList() << "" << "Testing host:" << currentHost << "");
			QString outFile = QString("%1/%2.htm").arg(lamexp_temp_folder2(), lamexp_rand_str());
			bool httpOk = false;
			if(getFile(currentHost, outFile, 0, &httpOk))
			{
				connectionScore++;
				progressBar->setValue(qBound(1, connectionScore + 1, MIN_CONNSCORE + 1));
				QApplication::processEvents();
				Sleep(125);
			}
			if(httpOk)
			{
				connectionScore++;
				progressBar->setValue(qBound(1, connectionScore + 1, MIN_CONNSCORE + 1));
				QApplication::processEvents();
				Sleep(125);
			}
			QFile::remove(outFile);
		}
	}

	if(connectionScore < MIN_CONNSCORE)
	{
		if(!retryButton->isVisible()) retryButton->show();
		if(!logButton->isVisible()) logButton->show();
		closeButton->setEnabled(true);
		retryButton->setEnabled(true);
		logButton->setEnabled(true);
		if(frameAnimation->isVisible()) frameAnimation->hide();
		statusLabel->setText(tr("Network connectivity test has failed!"));
		progressBar->setValue(progressBar->maximum());
		hintIcon->setPixmap(QIcon(":/icons/network_error.png").pixmap(16,16));
		hintLabel->setText(tr("Please make sure your internet connection is working properly and try again."));
		hintIcon->show();
		hintLabel->show();
		LAMEXP_DELETE(m_updateInfo);
		if(m_settings->soundsEnabled()) PlaySound(MAKEINTRESOURCE(IDR_WAVE_ERROR), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
		QApplication::restoreOverrideCursor();
		progressBar->setValue(progressBar->maximum());
		WinSevenTaskbar::setTaskbarState(this->parentWidget(), WinSevenTaskbar::WinSevenTaskbarErrorState);
		WinSevenTaskbar::setOverlayIcon(this->parentWidget(), &QIcon(":/icons/exclamation.png"));
		return;
	}

	// ----- Build Mirror List ----- //

	statusLabel->setText(tr("Checking for new updates online, please wait..."));
	m_logFile->append(QStringList() << "" << "----" << "" << "Checking for updates online...");

	QStringList mirrorList;
	for(int index = 0; update_mirrors_prim[index]; index++)
	{
		mirrorList << QString::fromLatin1(update_mirrors_prim[index]);
	}

	qsrand(lamexp_mix(clock(), time(NULL), _getpid()));
	if(const int len = mirrorList.count())
	{
		const int rounds = len * 1097;
		for(int i = 0; i < rounds; i++)
		{
			mirrorList.swap(i % len, qrand() % len);
		}
	}

	for(int index = 0; update_mirrors_back[index]; index++)
	{
		mirrorList << QString::fromLatin1(update_mirrors_back[index]);
	}
	
	// ----- Fetch Update Info From Server ----- //

	while(!mirrorList.isEmpty())
	{
		QString currentMirror = mirrorList.takeFirst();
		progressBar->setValue(progressBar->value() + 1);
		if(!success)
		{
			if(tryUpdateMirror(m_updateInfo, currentMirror))
			{
				success = true;
			}
		}
		else
		{
			QApplication::processEvents();
			Sleep(125);
		}
	}
	
	QApplication::restoreOverrideCursor();
	progressBar->setValue(progressBar->maximum());
	
	if(!success)
	{
		if(!retryButton->isVisible()) retryButton->show();
		if(!logButton->isVisible()) logButton->show();
		closeButton->setEnabled(true);
		retryButton->setEnabled(true);
		logButton->setEnabled(true);
		if(frameAnimation->isVisible()) frameAnimation->hide();
		statusLabel->setText(tr("Failed to fetch update information from server!"));
		progressBar->setValue(progressBar->maximum());
		WinSevenTaskbar::setTaskbarState(this->parentWidget(), WinSevenTaskbar::WinSevenTaskbarErrorState);
		WinSevenTaskbar::setOverlayIcon(this->parentWidget(), &QIcon(":/icons/exclamation.png"));
		hintIcon->setPixmap(QIcon(":/icons/server_error.png").pixmap(16,16));
		hintLabel->setText(tr("Sorry, the update server might be busy at this time. Plase try again later."));
		hintIcon->show();
		hintLabel->show();
		LAMEXP_DELETE(m_updateInfo);
		if(m_settings->soundsEnabled()) PlaySound(MAKEINTRESOURCE(IDR_WAVE_ERROR), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
		return;
	}

	// ----- Download New Program Version ----- //
	
	labelVersionLatest->setText(QString("%1 %2 (%3)").arg(tr("Build"), QString::number(m_updateInfo->m_buildNo), m_updateInfo->m_buildDate.toString(Qt::ISODate)));
	infoLabel->show();
	infoLabel->setText(QString("%1<br><a href=\"%2\">%2</a>").arg(tr("More information available at:"), m_updateInfo->m_downloadSite));
	QApplication::processEvents();
	
	if(m_updateInfo->m_buildNo > lamexp_version_build())
	{
		installButton->setEnabled(true);
		statusLabel->setText(tr("A new version of LameXP is available!"));
		hintIcon->setPixmap(QIcon(":/icons/shield_exclamation.png").pixmap(16,16));
		hintLabel->setText(tr("We highly recommend all users to install this update as soon as possible."));
		if(frameAnimation->isVisible()) frameAnimation->hide();
		hintIcon->show();
		hintLabel->show();
		WinSevenTaskbar::setOverlayIcon(this->parentWidget(), &QIcon(":/icons/shield_exclamation.png"));
		MessageBeep(MB_ICONINFORMATION);
	}
	else if(m_updateInfo->m_buildNo == lamexp_version_build())
	{
		statusLabel->setText(tr("No new updates available at this time."));
		hintIcon->setPixmap(QIcon(":/icons/shield_green.png").pixmap(16,16));
		hintLabel->setText(tr("Your version of LameXP is still up-to-date. Please check for updates regularly!"));
		if(frameAnimation->isVisible()) frameAnimation->hide();
		hintIcon->show();
		hintLabel->show();
		WinSevenTaskbar::setOverlayIcon(this->parentWidget(), &QIcon(":/icons/shield_green.png"));
		MessageBeep(MB_ICONINFORMATION);
	}
	else
	{
		statusLabel->setText(tr("Your version appears to be newer than the latest release."));
		hintIcon->setPixmap(QIcon(":/icons/shield_error.png").pixmap(16,16));
		hintLabel->setText(tr("This usually indicates your are currently using a pre-release version of LameXP."));
		if(frameAnimation->isVisible()) frameAnimation->hide();
		hintIcon->show();
		hintLabel->show();
		WinSevenTaskbar::setOverlayIcon(this->parentWidget(), &QIcon(":/icons/shield_error.png"));
		MessageBeep(MB_ICONEXCLAMATION);
	}

	closeButton->setEnabled(true);
	if(retryButton->isVisible()) retryButton->hide();
	if(logButton->isVisible()) logButton->hide();
	if(frameAnimation->isVisible()) frameAnimation->hide();

	m_success = true;
}

bool UpdateDialog::tryUpdateMirror(UpdateInfo *updateInfo, const QString &url)
{
	bool success = false;
	m_logFile->append(QStringList() << "" << "Trying mirror:" << url);

	QString randPart = lamexp_rand_str();
	QString outFileVersionInfo = QString("%1/%2.ver").arg(lamexp_temp_folder2(), randPart);
	QString outFileSignature = QString("%1/%2.sig").arg(lamexp_temp_folder2(), randPart);

	m_logFile->append(QStringList() << "" << "Downloading update info:");
	bool ok1 = getFile(QString("%1%2").arg(url, mirror_url_postfix[m_betaUpdates ? 1 : 0]), outFileVersionInfo);

	m_logFile->append(QStringList() << "" << "Downloading signature:");
	bool ok2 = getFile(QString("%1%2.sig").arg(url, mirror_url_postfix[m_betaUpdates ? 1 : 0]), outFileSignature);

	if(ok1 && ok2)
	{
		m_logFile->append(QStringList() << "" << "Download okay, checking signature:");
		if(checkSignature(outFileVersionInfo, outFileSignature))
		{
			m_logFile->append(QStringList() << "" << "Signature okay, parsing info:");
			success = parseVersionInfo(outFileVersionInfo, updateInfo);
		}
		else
		{
			m_logFile->append(QStringList() << "" << "Bad signature, take care!");
		}
	}
	else
	{
		m_logFile->append(QStringList() << "" << "Download has failed!");
	}

	QFile::remove(outFileVersionInfo);
	QFile::remove(outFileSignature);
	
	return success;
}

bool UpdateDialog::getFile(const QString &url, const QString &outFile, unsigned int maxRedir, bool *httpOk)
{
	QFileInfo output(outFile);
	output.setCaching(false);
	if(httpOk) *httpOk = false;

	if(output.exists())
	{
		QFile::remove(output.canonicalFilePath());
		if(output.exists())
		{
			return false;
		}
	}

	QProcess process;
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.setReadChannel(QProcess::StandardOutput);
	process.setWorkingDirectory(output.absolutePath());

	QStringList args;
	args << "--no-cache" << "--no-dns-cache" << QString().sprintf("--max-redirect=%u", maxRedir);
	args << QString("--referer=%1://%2/").arg(QUrl(url).scheme(), QUrl(url).host()) << "-U" << USER_AGENT_STR;
	args << "-O" << output.fileName() << url;

	QEventLoop loop;
	connect(&process, SIGNAL(error(QProcess::ProcessError)), &loop, SLOT(quit()));
	connect(&process, SIGNAL(finished(int,QProcess::ExitStatus)), &loop, SLOT(quit()));
	connect(&process, SIGNAL(readyRead()), &loop, SLOT(quit()));

	QTimer timer;
	timer.setSingleShot(true);
	timer.setInterval(25000);
	connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

	const QRegExp httpResponseOK("200 OK$");
	
	process.start(m_binaryWGet, args);
	
	if(!process.waitForStarted())
	{
		return false;
	}

	timer.start();

	while(process.state() == QProcess::Running)
	{
		loop.exec();
		bool bTimeOut = (!timer.isActive());
		while(process.canReadLine())
		{
			QString line = QString::fromLatin1(process.readLine()).simplified();
			if(line.contains(httpResponseOK))
			{
				line.append(" [OK]");
				if(httpOk) *httpOk = true;
			}
			m_logFile->append(line);
		}
		if(bTimeOut)
		{
			qWarning("WGet process timed out <-- killing!");
			process.kill();
			process.waitForFinished();
			m_logFile->append("!!! TIMEOUT !!!");
			return false;
		}
	}
	
	timer.stop();
	timer.disconnect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

	m_logFile->append(QString().sprintf("Exited with code %d", process.exitCode()));
	return (process.exitCode() == 0) && output.exists() && output.isFile();
}

bool UpdateDialog::checkSignature(const QString &file, const QString &signature)
{
	if(QFileInfo(file).absolutePath().compare(QFileInfo(signature).absolutePath(), Qt::CaseInsensitive) != 0)
	{
		qWarning("CheckSignature: File and signature should be in same folder!");
		return false;
	}
	if(QFileInfo(file).absolutePath().compare(QFileInfo(m_binaryKeys).absolutePath(), Qt::CaseInsensitive) != 0)
	{
		qWarning("CheckSignature: File and keyring should be in same folder!");
		return false;
	}

	QProcess process;
	process.setProcessChannelMode(QProcess::MergedChannels);
	process.setReadChannel(QProcess::StandardOutput);
	process.setWorkingDirectory(QFileInfo(file).absolutePath());

	QEventLoop loop;
	connect(&process, SIGNAL(error(QProcess::ProcessError)), &loop, SLOT(quit()));
	connect(&process, SIGNAL(finished(int,QProcess::ExitStatus)), &loop, SLOT(quit()));
	connect(&process, SIGNAL(readyRead()), &loop, SLOT(quit()));
	
	process.start(m_binaryGnuPG, QStringList() << "--homedir" << "." << "--keyring" << QFileInfo(m_binaryKeys).fileName() << QFileInfo(signature).fileName() << QFileInfo(file).fileName());

	if(!process.waitForStarted())
	{
		return false;
	}

	while(process.state() == QProcess::Running)
	{
		loop.exec();
		while(process.canReadLine())
		{
			m_logFile->append(QString::fromLatin1(process.readLine()).simplified());
		}
	}
	
	m_logFile->append(QString().sprintf("Exited with code %d", process.exitCode()));
	return (process.exitCode() == 0);
}

bool UpdateDialog::parseVersionInfo(const QString &file, UpdateInfo *updateInfo)
{
	QRegExp value("^(\\w+)=(.+)$");
	QRegExp section("^\\[(.+)\\]$");

	QDate updateInfoDate;
	updateInfo->resetInfo();

	QFile data(file);
	if(!data.open(QIODevice::ReadOnly))
	{
		qWarning("Cannot open update info file for reading!");
		return false;
	}
	
	bool inHeader = false;
	bool inSection = false;
	
	while(!data.atEnd())
	{
		QString line = QString::fromLatin1(data.readLine()).trimmed();
		if(section.indexIn(line) >= 0)
		{
			m_logFile->append(QString("Sec: [%1]").arg(section.cap(1)));
			inSection = (section.cap(1).compare(section_id, Qt::CaseInsensitive) == 0);
			inHeader = (section.cap(1).compare(header_id, Qt::CaseInsensitive) == 0);
			continue;
		}
		if(inSection && (value.indexIn(line) >= 0))
		{
			m_logFile->append(QString("Val: '%1' ==> '%2").arg(value.cap(1), value.cap(2)));
			if(value.cap(1).compare("BuildNo", Qt::CaseInsensitive) == 0)
			{
				bool ok = false;
				unsigned int temp = value.cap(2).toUInt(&ok);
				if(ok) updateInfo->m_buildNo = temp;
			}
			else if(value.cap(1).compare("BuildDate", Qt::CaseInsensitive) == 0)
			{
				QDate temp = QDate::fromString(value.cap(2).trimmed(), Qt::ISODate);
				if(temp.isValid()) updateInfo->m_buildDate = temp;
			}
			else if(value.cap(1).compare("DownloadSite", Qt::CaseInsensitive) == 0)
			{
				updateInfo->m_downloadSite = value.cap(2).trimmed();
			}
			else if(value.cap(1).compare("DownloadAddress", Qt::CaseInsensitive) == 0)
			{
				updateInfo->m_downloadAddress = value.cap(2).trimmed();
			}
			else if(value.cap(1).compare("DownloadFilename", Qt::CaseInsensitive) == 0)
			{
				updateInfo->m_downloadFilename = value.cap(2).trimmed();
			}
			else if(value.cap(1).compare("DownloadFilecode", Qt::CaseInsensitive) == 0)
			{
				updateInfo->m_downloadFilecode = value.cap(2).trimmed();
			}
		}
		if(inHeader && (value.indexIn(line) >= 0))
		{
			m_logFile->append(QString("Val: '%1' ==> '%2").arg(value.cap(1), value.cap(2)));
			if(value.cap(1).compare("TimestampCreated", Qt::CaseInsensitive) == 0)
			{
				QDate temp = QDate::fromString(value.cap(2).trimmed(), Qt::ISODate);
				if(temp.isValid()) updateInfoDate = temp;
			}
		}
	}
	
	if(!updateInfoDate.isValid())
	{
		updateInfo->resetInfo();
		m_logFile->append("WARNING: Version info timestamp is missing!");
		return false;
	}
	else if(updateInfoDate.addMonths(VERSION_INFO_EXPIRES_MONTHS) < QDate::currentDate())
	{
		updateInfo->resetInfo();
		m_logFile->append(QString::fromLatin1("WARNING: This version info has expired at %1!").arg(updateInfoDate.addMonths(VERSION_INFO_EXPIRES_MONTHS).toString(Qt::ISODate)));
		return false;
	}
	else if(QDate::currentDate() < updateInfoDate)
	{
		m_logFile->append("Version info is from the future, take care!");
		qWarning("Version info is from the future, take care!");
	}

	bool complete = true;

	if(!(updateInfo->m_buildNo > 0)) complete = false;
	if(!(updateInfo->m_buildDate.year() >= 2010)) complete = false;
	if(updateInfo->m_downloadSite.isEmpty()) complete = false;
	if(updateInfo->m_downloadAddress.isEmpty()) complete = false;
	if(updateInfo->m_downloadFilename.isEmpty()) complete = false;
	if(updateInfo->m_downloadFilecode.isEmpty()) complete = false;
	
	if(!complete)
	{
		m_logFile->append("WARNING: Version info is incomplete!");
	}

	return complete;
}

void UpdateDialog::linkActivated(const QString &link)
{
	QDesktopServices::openUrl(QUrl(link));
}

void UpdateDialog::applyUpdate(void)
{
	installButton->setEnabled(false);
	closeButton->setEnabled(false);
	retryButton->setEnabled(false);

	if(m_updateInfo)
	{
		statusLabel->setText(tr("Update is being downloaded, please be patient..."));
		frameAnimation->show();
		if(hintLabel->isVisible()) hintLabel->hide();
		if(hintIcon->isVisible()) hintIcon->hide();
		int oldMax = progressBar->maximum();
		int oldMin = progressBar->minimum();
		progressBar->setRange(0, 0);
		QApplication::processEvents();
		
		QProcess process;
		QStringList args;
		QEventLoop loop;

		connect(&process, SIGNAL(error(QProcess::ProcessError)), &loop, SLOT(quit()));
		connect(&process, SIGNAL(finished(int,QProcess::ExitStatus)), &loop, SLOT(quit()));

		args << QString("/Location=%1").arg(m_updateInfo->m_downloadAddress);
		args << QString("/Filename=%1").arg(m_updateInfo->m_downloadFilename);
		args << QString("/TicketID=%1").arg(m_updateInfo->m_downloadFilecode);
		args << QString("/ToFolder=%1").arg(QDir::toNativeSeparators(QDir(QApplication::applicationDirPath()).canonicalPath())); 
		args << QString("/ToExFile=%1.exe").arg(QFileInfo(QFileInfo(QApplication::applicationFilePath()).canonicalFilePath()).completeBaseName());
		args << QString("/AppTitle=LameXP (Build #%1)").arg(QString::number(m_updateInfo->m_buildNo));

		QApplication::setOverrideCursor(Qt::WaitCursor);
		WinSevenTaskbar::setTaskbarState(this->parentWidget(), WinSevenTaskbar::WinSevenTaskbarIndeterminateState);
		WinSevenTaskbar::setOverlayIcon(this->parentWidget(), &QIcon(":/icons/transmit_blue.png"));

		process.start(m_binaryUpdater, args);
		bool updateStarted = process.waitForStarted();
		if(updateStarted)
		{
			m_updaterProcess = process.pid()->dwProcessId;
			loop.exec();
		}
		m_updaterProcess = NULL;
		QApplication::restoreOverrideCursor();

		hintLabel->show();
		hintIcon->show();
		progressBar->setRange(oldMin, oldMax);
		progressBar->setValue(oldMax);
		frameAnimation->hide();

		if(updateStarted && (process.exitCode() == 0))
		{
			statusLabel->setText(tr("Update ready to install. Applicaion will quit..."));
			m_updateReadyToInstall = true;
			WinSevenTaskbar::setTaskbarState(this->parentWidget(), WinSevenTaskbar::WinSevenTaskbarNoState);
			WinSevenTaskbar::setOverlayIcon(this->parentWidget(), NULL);
			accept();
		}
		else
		{
			statusLabel->setText(tr("Update failed. Please try again or download manually!"));
			WinSevenTaskbar::setTaskbarState(this->parentWidget(), WinSevenTaskbar::WinSevenTaskbarErrorState);
			WinSevenTaskbar::setOverlayIcon(this->parentWidget(), &QIcon(":/icons/exclamation.png"));
			WinSevenTaskbar::setTaskbarProgress(this->parentWidget(), 100, 100);
		}
	}

	installButton->setEnabled(true);
	closeButton->setEnabled(true);
}

void UpdateDialog::logButtonClicked(void)
{
	LogViewDialog *logView = new LogViewDialog(this);
	logView->exec(*m_logFile);
	LAMEXP_DELETE(logView);
}

void UpdateDialog::progressBarValueChanged(int value)
{
	WinSevenTaskbar::setTaskbarProgress(this->parentWidget(), value, progressBar->maximum());
}

void UpdateDialog::testKnownWebSites(void)
{
	int connectionScore = 0;

	// ----- Initialization ----- //

	progressBar->setValue(0);
	WinSevenTaskbar::setTaskbarState(this->parentWidget(), WinSevenTaskbar::WinSevenTaskbarNormalState);
	WinSevenTaskbar::setOverlayIcon(this->parentWidget(), &QIcon(":/icons/transmit_blue.png"));
	installButton->setEnabled(false);
	closeButton->setEnabled(false);
	retryButton->setEnabled(false);
	logButton->setEnabled(false);
	if(infoLabel->isVisible()) infoLabel->hide();
	if(hintLabel->isVisible()) hintLabel->hide();
	if(hintIcon->isVisible()) hintIcon->hide();
	frameAnimation->show();

	QApplication::processEvents();
	QApplication::setOverrideCursor(Qt::WaitCursor);

	// ----- Test Internet Connection ----- //

	statusLabel->setText("Testing all known hosts, this may take a few minutes...");

	m_logFile->clear();
	m_logFile->append("Checking internet connection...");
	
	QFuture<BOOL> connectedState = QtConcurrent::run(getInternetConnectedState);
	while(!connectedState.isFinished())
	{
		QApplication::processEvents(QEventLoop::WaitForMoreEvents);
	}

	if(!connectedState.result())
	{
		m_logFile->append(QStringList() << "" << "Operating system reports that the computer is currently offline !!!");
		if(!retryButton->isVisible()) retryButton->show();
		if(!logButton->isVisible()) logButton->show();
		closeButton->setEnabled(true);
		retryButton->setEnabled(true);
		logButton->setEnabled(true);
		if(frameAnimation->isVisible()) frameAnimation->hide();
		statusLabel->setText(tr("It appears that the computer currently is offline!"));
		progressBar->setValue(progressBar->maximum());
		hintIcon->setPixmap(QIcon(":/icons/network_error.png").pixmap(16,16));
		hintLabel->setText(tr("Please make sure your computer is connected to the internet and try again."));
		hintIcon->show();
		hintLabel->show();
		LAMEXP_DELETE(m_updateInfo);
		if(m_settings->soundsEnabled()) PlaySound(MAKEINTRESOURCE(IDR_WAVE_ERROR), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
		QApplication::restoreOverrideCursor();
		progressBar->setValue(progressBar->maximum());
		WinSevenTaskbar::setTaskbarState(this->parentWidget(), WinSevenTaskbar::WinSevenTaskbarErrorState);
		WinSevenTaskbar::setOverlayIcon(this->parentWidget(), &QIcon(":/icons/exclamation.png"));
		return;
	}
	
	// ----- Test Known Hosts Connectivity ----- //

	QStringList hostList;
	for(int i = 0; known_hosts[i]; i++)
	{
		hostList << QString::fromLatin1(known_hosts[i]);
	}

	qDebug("[Known Hosts]");

	int hostCount = hostList.count();
	while(!hostList.isEmpty())
	{
		QString currentHost = hostList.takeFirst();
		progressBar->setValue(qRound((static_cast<double>(progressBar->maximum() - 1) / static_cast<double>(hostCount)) * static_cast<double>(connectionScore)) + 1);
		qDebug("Testing: %s", currentHost.toLatin1().constData());
		m_logFile->append(QStringList() << "" << "Testing host:" << currentHost << "");
		QString outFile = QString("%1/%2.htm").arg(lamexp_temp_folder2(), lamexp_rand_str());
		bool httpOk = false;
		if(getFile(currentHost, outFile, 0, &httpOk))
		{
			connectionScore++;
		}
		else
		{
			if(httpOk)
			{
				qWarning("\nConnectivity test was SLOW on the following site:\n%s\n", currentHost.toLatin1().constData());
				connectionScore++;
			}
			else
			{
				qWarning("\nConnectivity test FAILED on the following site:\n%s\n", currentHost.toLatin1().constData());
			}
		}
		QFile::remove(outFile);
	}

	if(connectionScore < hostCount)
	{
		if(!retryButton->isVisible()) retryButton->show();
		if(!logButton->isVisible()) logButton->show();
		closeButton->setEnabled(true);
		retryButton->setEnabled(true);
		logButton->setEnabled(true);
		if(frameAnimation->isVisible()) frameAnimation->hide();
		statusLabel->setText("At least one host could not be reached!");
		progressBar->setValue(progressBar->maximum());
		hintIcon->setPixmap(QIcon(":/icons/network_error.png").pixmap(16,16));
		hintLabel->setText("Please make sure your internet connection is working properly and try again.");
		hintIcon->show();
		hintLabel->show();
		LAMEXP_DELETE(m_updateInfo);
		if(m_settings->soundsEnabled()) PlaySound(MAKEINTRESOURCE(IDR_WAVE_ERROR), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
		QApplication::restoreOverrideCursor();
		progressBar->setValue(progressBar->maximum());
		WinSevenTaskbar::setTaskbarState(this->parentWidget(), WinSevenTaskbar::WinSevenTaskbarErrorState);
		WinSevenTaskbar::setOverlayIcon(this->parentWidget(), &QIcon(":/icons/exclamation.png"));
		return;
	}

	// ----- Done ----- //
	
	QApplication::restoreOverrideCursor();
	progressBar->setValue(progressBar->maximum());

	statusLabel->setText("Test completed.");
	hintIcon->setPixmap(QIcon(":/icons/shield_green.png").pixmap(16,16));
	hintLabel->setText("Congratulations, the test has completed.");
	if(frameAnimation->isVisible()) frameAnimation->hide();
	hintIcon->show();
	hintLabel->show();
	WinSevenTaskbar::setOverlayIcon(this->parentWidget(), &QIcon(":/icons/shield_green.png"));
	MessageBeep(MB_ICONINFORMATION);

	closeButton->setEnabled(true);
	if(retryButton->isVisible()) retryButton->hide();
	if(logButton->isVisible()) logButton->hide();
	if(frameAnimation->isVisible()) frameAnimation->hide();
}
