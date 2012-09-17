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

#include "Dialog_About.h"

#include "Global.h"
#include "Resource.h"
#include "Model_Settings.h"

#include <QDate>
#include <QApplication>
#include <QIcon>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>
#include <QFileInfo>
#include <QDir>
#include <QDesktopWidget>
#include <QLabel>
#include <QMessageBox>
#include <QTextStream>
#include <QScrollBar>
#include <QCloseEvent>
#include <QWindowsVistaStyle>
#include <QWindowsXPStyle>

#include <ShellAPI.h>
#include <MMSystem.h>
#include <math.h>

//Helper macros
#define LINK(URL) QString("<a href=\"%1\">%2</a>").arg(URL).arg(QString(URL).replace("-", "&minus;"))
#define TRIM_RIGHT(STR) do { while(STR.endsWith(QChar(' ')) || STR.endsWith(QChar('\t')) || STR.endsWith(QChar('\r')) || STR.endsWith(QChar('\n'))) STR.chop(1); } while(0)
#define MAKE_TRANSPARENT(WIDGET) do { QPalette _p = (WIDGET)->palette(); _p.setColor(QPalette::Background, Qt::transparent); (WIDGET)->setPalette(_p); } while(0)


//Constants
const char *AboutDialog::neroAacUrl = "http://www.nero.com/eng/technologies-aac-codec.html";
const char *AboutDialog::disqueUrl =  "http://mulder.brhack.net/?player_url=38X-MXOB014"; //http://mulder.brhack.net/?player_url=yF6W-w0iAMM; http://www.youtube.com/watch_popup?v=yF6W-w0iAMM&vq=large

//Contributors
static const struct 
{
	char *pcFlag;
	wchar_t *pcLanguage;
	wchar_t *pcName;
	char *pcMail;
}
g_lamexp_translators[] =
{
	{"en", L"Englisch",   L"LoRd_MuldeR",         "MuldeR2@GMX.de"        },
	{"de", L"Deutsch",    L"LoRd_MuldeR",         "MuldeR2@GMX.de"        },
	{"",   L"",           L"Bodo Thevissen",      "Bodo@thevissen.de"     },
	{"es", L"Español",    L"Rub3nCT",             "Rub3nCT@gmail.com"     },
	{"fr", L"Française",  L"Dodich Informatique", "Dodich@live.fr"        },
	{"it", L"Italiano",   L"Roberto",             "Gulliver_69@libero.it" },
	{"kr", L"한국어",        L"JaeHyung Lee",        "Kolanp@gmail.com"      },
	{"pl", L"Polski",     L"Sir Daniel K",        "Sir.Daniel.K@gmail.com"},
	{"ru", L"Русский",    L"Neonailol",           "Neonailol@gmail.com"   },
	{"",   L"",           L"Иван Митин",          "bardak@inbox.ru"       },
	{"sv", L"Svenska",    L"Åke Engelbrektson",   "eson57@gmail.com"      },
	{"tw", L"繁体中文",       L"456Vv",               "123@456vv.com"         },
	{"uk", L"Українська", L"Arestarh",            "Arestarh@ukr.net"      },
	{"zh", L"简体中文",       L"456Vv",               "123@456vv.com"         },
	{NULL, NULL, NULL, NULL}
};

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

AboutDialog::AboutDialog(SettingsModel *settings, QWidget *parent, bool firstStart)
:
	QDialog(parent),
	m_settings(settings),
	m_initFlags(new QMap<QWidget*,bool>),
	m_disque(NULL),
	m_disqueTimer(NULL),
	m_rotateNext(false),
	m_disqueDelay(_I64_MAX),
	m_lastTab(0)
{
	//Init the dialog, from the .ui file
	setupUi(this);
	setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
	resize(this->minimumSize());
	
	//Disable "X" button
	if(firstStart)
	{
		if(HMENU hMenu = GetSystemMenu((HWND) winId(), FALSE))
		{
			EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
		}
	}

	//Init images
	for(int i = 0; i < 4; i++)
	{
		m_cartoon[i] = NULL;
	}

	//Init tab widget
	connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

	//Make transparent
	QStyle *style = qApp->style();
	if((dynamic_cast<QWindowsVistaStyle*>(style)) || (dynamic_cast<QWindowsXPStyle*>(style)))
	{
		MAKE_TRANSPARENT(infoScrollArea);
		MAKE_TRANSPARENT(contributorsScrollArea);
		MAKE_TRANSPARENT(softwareScrollArea);
		MAKE_TRANSPARENT(licenseScrollArea);
	}

	//Show about dialog for the first time?
	if(!firstStart)
	{
		acceptButton->hide();
		declineButton->hide();
		aboutQtButton->show();
		closeButton->show();
		
		QPixmap disque(":/images/Disque.png");
		QRect screenGeometry = QApplication::desktop()->availableGeometry();
		m_disque = new QLabel(this, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
		m_disque->installEventFilter(this);
		m_disque->setStyleSheet("background:transparent;");
		m_disque->setAttribute(Qt::WA_TranslucentBackground);
		m_disque->setGeometry(qrand() % (screenGeometry.width() - disque.width()), qrand() % (screenGeometry.height() - disque.height()), disque.width(), disque.height());
		m_disque->setPixmap(disque);
		m_disque->setWindowOpacity(0.01);
		m_disque->show();
		m_disqueFlags[0] = (qrand() > (RAND_MAX/2));
		m_disqueFlags[1] = (qrand() > (RAND_MAX/2));
		m_disqueTimer = new QTimer;
		connect(m_disqueTimer, SIGNAL(timeout()), this, SLOT(moveDisque()));
		m_disqueTimer->setInterval(10);
		m_disqueTimer->start();

		connect(aboutQtButton, SIGNAL(clicked()), this, SLOT(showAboutQt()));
	}
	else
	{
		acceptButton->show();
		declineButton->show();
		aboutQtButton->hide();
		closeButton->hide();
	}

	//Activate "show license" button
	showLicenseButton->show();
	connect(showLicenseButton, SIGNAL(clicked()), this, SLOT(gotoLicenseTab()));

	m_firstShow = firstStart;
}

AboutDialog::~AboutDialog(void)
{
	if(m_disque)
	{
		m_disque->close();
		LAMEXP_DELETE(m_disque);
	}
	if(m_disqueTimer)
	{
		m_disqueTimer->stop();
		LAMEXP_DELETE(m_disqueTimer);
	}
	for(int i = 0; i < 4; i++)
	{
		LAMEXP_DELETE(m_cartoon[i]);
	}
	LAMEXP_DELETE(m_initFlags);
}

////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////

int AboutDialog::exec()
{
	if(m_settings->soundsEnabled())
	{
		if(m_firstShow)
		{
			if(!playResoureSound("imageres.dll", 5080, true))
			{
				PlaySound(TEXT("SystemStart"), NULL, SND_ALIAS | SND_ASYNC);
			}
		}
		else
		{
			PlaySound(MAKEINTRESOURCE(IDR_WAVE_ABOUT), GetModuleHandle(NULL), SND_RESOURCE | SND_ASYNC);
		}
	}
	
	switch(QDialog::exec())
	{
	case 1:
		return 1;
		break;
	case 2:
		return -1;
		break;
	default:
		return 0;
		break;
	}
}

////////////////////////////////////////////////////////////
// Slots
////////////////////////////////////////////////////////////

#define TEMP_HIDE_DISQUE(CMD) \
if(m_disque) { bool _tmp = m_disque->isVisible(); if(_tmp) m_disque->hide(); {CMD}; if(_tmp) { m_disque->show(); m_disque->setWindowOpacity(0.01); } } else {CMD}

void AboutDialog::tabChanged(int index)
{
	bool bInitialized = m_initFlags->value(tabWidget->widget(index), false);

	if(!bInitialized)
	{
		qApp->setOverrideCursor(QCursor(Qt::WaitCursor));

		if(QWidget *tab = tabWidget->widget(index))
		{
			bool ok = false;

			if(tab == infoTab) { initInformationTab(); ok = true; }
			if(tab == contributorsTab) { initContributorsTab(); ok = true; }
			if(tab == softwareTab) { initSoftwareTab(); ok = true; }
			if(tab == licenseTab) { initLicenseTab(); ok = true; }

			if(ok)
			{
				m_initFlags->insert(tab, true);
			}
			else
			{
				qWarning("Unknown tab %p encountered, cannot initialize !!!", tab);
			}
			
		}

		tabWidget->widget(index)->update();
		qApp->processEvents();
		qApp->restoreOverrideCursor();
	}

	//Scroll to the top
	if(QWidget *tab = tabWidget->widget(index))
	{
		if(tab == infoTab) infoScrollArea->verticalScrollBar()->setSliderPosition(0);
		if(tab == contributorsTab) contributorsScrollArea->verticalScrollBar()->setSliderPosition(0);
		if(tab == softwareTab) softwareScrollArea->verticalScrollBar()->setSliderPosition(0);
		if(tab == licenseTab) licenseScrollArea->verticalScrollBar()->setSliderPosition(0);
	}

	//Update license button
	showLicenseButton->setChecked(tabWidget->widget(index) == licenseTab);
	if(tabWidget->widget(index) != licenseTab) m_lastTab = index;
}

void AboutDialog::enableButtons(void)
{
	acceptButton->setEnabled(true);
	declineButton->setEnabled(true);
	setCursor(QCursor(Qt::ArrowCursor));
}

void AboutDialog::openURL(const QString &url)
{
	if(!QDesktopServices::openUrl(QUrl(url)))
	{
		ShellExecuteW(this->winId(), L"open", QWCHAR(url), NULL, NULL, SW_SHOW);
	}
}

void AboutDialog::showAboutQt(void)
{
	TEMP_HIDE_DISQUE
	(
		QMessageBox::aboutQt(this);
	);
}

void AboutDialog::gotoLicenseTab(void)
{
	tabWidget->setCurrentIndex(tabWidget->indexOf(showLicenseButton->isChecked() ? licenseTab : tabWidget->widget(m_lastTab)));
}

void AboutDialog::moveDisque(void)
{
	int delta = 2;
	LARGE_INTEGER perfCount, perfFrequ;

	if(QueryPerformanceFrequency(&perfFrequ) &&	QueryPerformanceCounter(&perfCount))
	{
		if(m_disqueDelay != _I64_MAX)
		{
			const double delay = static_cast<double>(perfCount.QuadPart) - static_cast<double>(m_disqueDelay);
			delta = qMax(1, qMin(128, static_cast<int>(ceil(delay / static_cast<double>(perfFrequ.QuadPart) / 0.00512))));
		}
		m_disqueDelay = perfCount.QuadPart;
	}

	if(m_disque)
	{
		QRect screenGeometry = QApplication::desktop()->availableGeometry();
		const int minX = screenGeometry.left();
		const int maxX = screenGeometry.width() - m_disque->width() + screenGeometry.left();
		const int minY = screenGeometry.top();
		const int maxY = screenGeometry.height() - m_disque->height() + screenGeometry.top();
		
		QPoint pos = m_disque->pos();
		pos.setX(m_disqueFlags[0] ? pos.x() + delta : pos.x() - delta);
		pos.setY(m_disqueFlags[1] ? pos.y() + delta : pos.y() - delta);

		if(pos.x() <= minX)
		{
			m_disqueFlags[0] = true;
			pos.setX(minX);
			m_rotateNext = true;
		}
		else if(pos.x() >= maxX)
		{
			m_disqueFlags[0] = false;
			pos.setX(maxX);
			m_rotateNext = true;
		}
		if(pos.y() <= minY)
		{
			m_disqueFlags[1] = true;
			pos.setY(minY);
			m_rotateNext = true;
		}
		else if(pos.y() >= maxY)
		{
			m_disqueFlags[1] = false;
			pos.setY(maxY);
			m_rotateNext = true;
		}

		m_disque->move(pos);

		if(m_rotateNext)
		{
			QPixmap *cartoon = NULL;
			if(m_disqueFlags[0] == true && m_disqueFlags[1] != true) cartoon = m_cartoon[0];
			if(m_disqueFlags[0] == true && m_disqueFlags[1] == true) cartoon = m_cartoon[1];
			if(m_disqueFlags[0] != true && m_disqueFlags[1] == true) cartoon = m_cartoon[2];
			if(m_disqueFlags[0] != true && m_disqueFlags[1] != true) cartoon = m_cartoon[3];
			if(cartoon)
			{
				m_disque->setPixmap(*cartoon);
				m_disque->resize(cartoon->size());
			}
			m_rotateNext = false;
		}

		if(m_disque->windowOpacity() < 0.9)
		{
			m_disque->setWindowOpacity(m_disque->windowOpacity() + 0.01);
		}
	}
}
////////////////////////////////////////////////////////////
// Protected Functions
////////////////////////////////////////////////////////////

void AboutDialog::showEvent(QShowEvent *e)
{
	QDialog::showEvent(e);
	
	tabWidget->setCurrentIndex(tabWidget->indexOf(infoTab));
	tabChanged(m_lastTab = tabWidget->currentIndex());
	
	if(m_firstShow)
	{
		acceptButton->setEnabled(false);
		declineButton->setEnabled(false);
		QTimer::singleShot(5000, this, SLOT(enableButtons()));
		setCursor(QCursor(Qt::WaitCursor));
	}
}

void AboutDialog::closeEvent(QCloseEvent *e)
{
	if(m_firstShow) e->ignore();
}

bool AboutDialog::eventFilter(QObject *obj, QEvent *event)
{
	if((obj == m_disque) && (event->type() == QEvent::MouseButtonPress))
	{
		QPixmap cartoon(":/images/Cartoon.png");
		for(int i = 0; i < 4; i++)
		{
			if(!m_cartoon[i])
			{
				m_cartoon[i] = new QPixmap(cartoon.transformed(QMatrix().rotate(static_cast<double>(i*90) + 45.0), Qt::SmoothTransformation));
				m_rotateNext = true;
			}
		}
		QDesktopServices::openUrl(QUrl(disqueUrl));
	}

	return false;
}

////////////////////////////////////////////////////////////
// Private Functions
////////////////////////////////////////////////////////////

void AboutDialog::initInformationTab(void)
{
	const QString versionStr = QString().sprintf
	(
		"Version %d.%02d %s, Build %d [%s], %s %s, Qt v%s",
		lamexp_version_major(),
		lamexp_version_minor(),
		lamexp_version_release(),
		lamexp_version_build(),
		lamexp_version_date().toString(Qt::ISODate).toLatin1().constData(),
		lamexp_version_compiler(),
		lamexp_version_arch(),
		qVersion()
	);

	const QString copyrightStr = QString().sprintf
	(
		"Copyright (C) 2004-%04d LoRd_MuldeR &lt;MuldeR2@GMX.de&gt;. Some rights reserved.",
		qMax(lamexp_version_date().year(), QDate::currentDate().year())
	);

	QString aboutText;

	aboutText += QString("<h2>%1</h2>").arg(NOBR(tr("LameXP - Audio Encoder Front-end")));
	aboutText += QString("<b>%1</b><br>").arg(NOBR(copyrightStr));
	aboutText += QString("<b>%1</b><br><br>").arg(NOBR(versionStr));
	aboutText += QString("%1<br>").arg(NOBR(tr("Please visit %1 for news and updates!").arg(LINK(lamexp_website_url()))));

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	if(LAMEXP_DEBUG)
	{
		int daysLeft = qMax(QDate::currentDate().daysTo(lamexp_version_expires()), 0);
		aboutText += QString("<hr><font color=\"crimson\">%1</font>").arg(NOBR(QString("!!! --- DEBUG BUILD --- Expires at: %1 &middot; Days left: %2 --- DEBUG BUILD --- !!!").arg(lamexp_version_expires().toString(Qt::ISODate), QString::number(daysLeft))));
	}
	else if(lamexp_version_demo())
	{
		int daysLeft = qMax(QDate::currentDate().daysTo(lamexp_version_expires()), 0);
		aboutText += QString("<hr><font color=\"crimson\">%1</font>").arg(NOBR(tr("Note: This demo (pre-release) version of LameXP will expire at %1. Still %2 days left.").arg(lamexp_version_expires().toString(Qt::ISODate), QString::number(daysLeft))));
	}
#else
	if(LAMEXP_DEBUG)
	{
		int daysLeft = qMax(QDate::currentDate().daysTo(lamexp_version_expires()), 0i64);
		aboutText += QString("<hr><font color=\"crimson\">%1</font>").arg(NOBR(QString("!!! --- DEBUG BUILD --- Expires at: %1 &middot; Days left: %2 --- DEBUG BUILD --- !!!").arg(lamexp_version_expires().toString(Qt::ISODate), QString::number(daysLeft))));
	}
	else if(lamexp_version_demo())
	{
		int daysLeft = qMax(QDate::currentDate().daysTo(lamexp_version_expires()), 0i64);
		aboutText += QString("<hr><font color=\"crimson\">%1</font>").arg(NOBR(tr("Note: This demo (pre-release) version of LameXP will expire at %1. Still %2 days left.").arg(lamexp_version_expires().toString(Qt::ISODate), QString::number(daysLeft))));
	}
#endif

	aboutText += "<hr><br>";
	aboutText += "<nobr><tt>This program is free software; you can redistribute it and/or<br>";
	aboutText += "modify it under the terms of the GNU General Public License<br>";
	aboutText += "as published by the Free Software Foundation; either version 2<br>";
	aboutText += "of the License, or (at your option) any later version.<br><br>";
	aboutText += "This program is distributed in the hope that it will be useful,<br>";
	aboutText += "but WITHOUT ANY WARRANTY; without even the implied warranty of<br>";
	aboutText += "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the<br>";
	aboutText += "GNU General Public License for more details.<br><br>";
	aboutText += "You should have received a copy of the GNU General Public License<br>";
	aboutText += "along with this program; if not, write to the Free Software<br>";
	aboutText += "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110&minus;1301, USA.</tt></nobr><br>";
	aboutText += "<hr><table style=\"margin-top:4px\"><tr>";
	aboutText += "<td valign=\"middle\"><img src=\":/icons/error_big.png\"</td><td>&nbsp;</td>";
	aboutText += QString("<td><font color=\"darkred\">%1</font></td>").arg(tr("Note: LameXP is free software. Do <b>not</b> pay money to obtain or use LameXP! If some third-party website tries to make you pay for downloading LameXP, you should <b>not</b> respond to the offer !!!"));
	aboutText += "</tr></table>";
	//aboutText += QString("%1<br>").arg(NOBR(tr("Special thanks go out to \"John33\" from %1 for his continuous support.")).arg(LINK("http://www.rarewares.org/")));

	infoLabel->setText(aboutText);
	infoIcon->setPixmap(lamexp_app_icon().pixmap(QSize(72,72)));
	connect(infoLabel, SIGNAL(linkActivated(QString)), this, SLOT(openURL(QString)));
}

void AboutDialog::initContributorsTab(void)
{
	const QString spaces("&nbsp;&nbsp;&nbsp;&nbsp;");
	const QString extraVSpace("<font style=\"font-size:7px\"><br>&nbsp;</font>");
	
	QString contributorsAboutText;
	contributorsAboutText += QString("<h3>%1</h3>").arg(NOBR(tr("The following people have contributed to LameXP:")));
	contributorsAboutText += "<table style=\"margin-top:12px;white-space:nowrap\">";
	
	contributorsAboutText += QString("<tr><td colspan=\"7\"><b>%1</b>%2</td></tr>").arg(tr("Programmers:"), extraVSpace);
	QString icon = QString("<img src=\":/icons/%1.png\">").arg("user_gray");
	contributorsAboutText += QString("<tr><td valign=\"middle\">%1</td><td>%2</td>").arg(icon, spaces);
	contributorsAboutText += QString("<td valign=\"middle\">%1</td><td>%2</td>").arg(tr("Project Leader"), spaces);
	contributorsAboutText += QString("<td valign=\"middle\">%1</td><td>%2</td><td><a href=\"mailto:%2\">&lt;%3&gt;</a></td></tr>").arg("LoRd_MuldeR", spaces, "MuldeR2@GMX.de");
	contributorsAboutText += QString("<tr><td colspan=\"7\"><b>&nbsp;</b></td></tr>");

	contributorsAboutText += QString("<tr><td colspan=\"7\"><b>%1</b>%2</td></tr>").arg(tr("Translators:"), extraVSpace);
	for(int i = 0; g_lamexp_translators[i].pcName; i++)
	{
		QString flagIcon = (strlen(g_lamexp_translators[i].pcFlag) > 0) ? QString("<img src=\":/flags/%1.png\">").arg(g_lamexp_translators[i].pcFlag) : QString();
		contributorsAboutText += QString("<tr><td valign=\"middle\">%1</td><td>%2</td>").arg(flagIcon, spaces);
		contributorsAboutText += QString("<td valign=\"middle\">%1</td><td>%2</td>").arg(WCHAR2QSTR(g_lamexp_translators[i].pcLanguage), spaces);
		contributorsAboutText += QString("<td valign=\"middle\">%1</td><td>%2</td><td><a href=\"mailto:%2\">&lt;%3&gt;</a></td></tr>").arg(WCHAR2QSTR(g_lamexp_translators[i].pcName), spaces, g_lamexp_translators[i].pcMail);
	}

	contributorsAboutText += QString("<tr><td colspan=\"7\"><b>&nbsp;</b></td></tr>");
	contributorsAboutText += QString("<tr><td colspan=\"7\"><b>%1</b>%2</td></tr>").arg(tr("Special thanks to:"), extraVSpace);

	QString webIcon = QString("<img src=\":/icons/%1.png\">").arg("world");
	contributorsAboutText += QString("<tr><td valign=\"middle\">%1</td><td>%2</td>").arg(webIcon, spaces);
	contributorsAboutText += QString("<td valign=\"middle\">%1</td><td>%2</td><td valign=\"middle\" colspan=\"3\"><a href=\"%3\">%3</td></tr>").arg(tr("Doom9's Forum"), spaces, "http://forum.doom9.org/");
	contributorsAboutText += QString("<tr><td valign=\"middle\">%1</td><td>%2</td>").arg(webIcon, spaces);
	contributorsAboutText += QString("<td valign=\"middle\">%1</td><td>%2</td><td valign=\"middle\" colspan=\"3\"><a href=\"%3\">%3</td></tr>").arg(tr("Gleitz | German Doom9"), spaces, "http://forum.gleitz.info/");
	contributorsAboutText += QString("<tr><td valign=\"middle\">%1</td><td>%2</td>").arg(webIcon, spaces);
	contributorsAboutText += QString("<td valign=\"middle\">%1</td><td>%2</td><td valign=\"middle\" colspan=\"3\"><a href=\"%3\">%3</td></tr>").arg(tr("Hydrogenaudio Forums"), spaces, "http://www.hydrogenaudio.org/");
	contributorsAboutText += QString("<tr><td valign=\"middle\">%1</td><td>%2</td>").arg(webIcon, spaces);
	contributorsAboutText += QString("<td valign=\"middle\">%1</td><td>%2</td><td valign=\"middle\" colspan=\"3\"><a href=\"%3\">%3</td></tr>").arg(tr("RareWares"), spaces, "http://www.rarewares.org/");
	contributorsAboutText += QString("<tr><td valign=\"middle\">%1</td><td>%2</td>").arg(webIcon, spaces);
	contributorsAboutText += QString("<td valign=\"middle\">%1</td><td>%2</td><td valign=\"middle\" colspan=\"3\"><a href=\"%3\">%3</td></tr>").arg(tr("GitHub"), spaces, "http://github.com/");
	contributorsAboutText += QString("<tr><td valign=\"middle\">%1</td><td>%2</td>").arg(webIcon, spaces);
	contributorsAboutText += QString("<td valign=\"middle\">%1</td><td>%2</td><td valign=\"middle\" colspan=\"3\"><a href=\"%3\">%3</td></tr>").arg(tr("SourceForge"), spaces, "http://sourceforge.net/");
	contributorsAboutText += QString("<tr><td valign=\"middle\">%1</td><td>%2</td>").arg(webIcon, spaces);
	contributorsAboutText += QString("<td valign=\"middle\">%1</td><td>%2</td><td valign=\"middle\" colspan=\"3\"><a href=\"%3\">%3</td></tr>").arg(tr("Qt Developer Network"), spaces, "http://qt-project.org/");
	contributorsAboutText += QString("<tr><td valign=\"middle\">%1</td><td>%2</td>").arg(webIcon, spaces);
	contributorsAboutText += QString("<td valign=\"middle\">%1</td><td>%2</td><td valign=\"middle\" colspan=\"3\"><a href=\"%3\">%3</td></tr>").arg(tr("Marius Hudea"), spaces, "http://savedonthe.net/");

	contributorsAboutText += "</table><br><br><br>";
	contributorsAboutText += QString("<i>%1</i><br>").arg(NOBR(tr("If you are willing to contribute a LameXP translation, feel free to contact us!")));

	contributorsLabel->setText(contributorsAboutText);
	contributorsIcon->setPixmap(QIcon(":/images/Logo_Contributors.png").pixmap(QSize(72,84)));
	connect(contributorsLabel, SIGNAL(linkActivated(QString)), this, SLOT(openURL(QString)));
}

void AboutDialog::initSoftwareTab(void)
{
	QString moreAboutText;

	moreAboutText += QString("<h3>%1</h3>").arg(tr("The following third-party software is used in LameXP:"));
	moreAboutText += "<div style=\"margin-left:-25px;white-space:nowrap\"><table><tr><td><ul>"; //;font-size:7pt
	
	moreAboutText += makeToolText
	(
		tr("LAME - OpenSource mp3 Encoder"),
		"lame.exe", "v?.??, Final-?",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://lame.sourceforge.net/"
	);
	moreAboutText += makeToolText
	(
		tr("OggEnc - Ogg Vorbis Encoder"),
		"oggenc2.exe", "v?.??, aoTuV Beta-?.??",
		tr("Completely open and patent-free audio encoding technology."),
		"http://www.vorbis.com/"
	);
	moreAboutText += makeToolText
	(
		tr("Nero AAC Reference MPEG-4 Encoder"),
		"neroAacEnc.exe", "v?.?.?.?",
		tr("Freeware state-of-the-art HE-AAC encoder with 2-Pass support."),
		neroAacUrl,
		tr("Available from vendor web-site as free download:")
	);
	moreAboutText += makeToolText
	(
		tr("Aften - A/52 audio encoder"),
		"aften.exe", "v?.?.?",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://aften.sourceforge.net/"
	);
	moreAboutText += makeToolText
	(
		tr("FLAC - Free Lossless Audio Codec"),
		"flac.exe", "v?.?.?",
		tr("Open and patent-free lossless audio compression technology."),
		"http://flac.sourceforge.net/"
	);
	moreAboutText += makeToolText
	(
		tr("Opus Audio Codec"),
		"opusenc_std.exe", "????-??-??",
		tr("Totally open, royalty-free, highly versatile audio codec."),
		"http://www.opus-codec.org/"
	);
	moreAboutText += makeToolText
	(
		tr("mpg123 - Fast Console MPEG Audio Player/Decoder"),
		"mpg123.exe", "v?.??.?",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://www.mpg123.de/"
	);
	moreAboutText += makeToolText
	(
		tr("FAAD - OpenSource MPEG-4 and MPEG-2 AAC Decoder"),
		"faad.exe", "v?.?",
		tr("Released under the terms of the GNU General Public License."),
		"http://www.audiocoding.com/"
	);
	moreAboutText += makeToolText
	(
		tr("AC3Filter Tools - AC3/DTS Decoder"),
		"valdec.exe", "v?.??",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://www.ac3filter.net/projects/tools"
	);
	moreAboutText += makeToolText
		(
		tr("WavPack - Hybrid Lossless Compression"),
		"wvunpack.exe", "v?.??.?",
		tr("Completely open audio compression format."),
		"http://www.wavpack.com/"
	);
	moreAboutText += makeToolText
		(
		tr("Musepack - Living Audio Compression"),
		"mpcdec.exe", "r???",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://www.musepack.net/"
	);
	moreAboutText += makeToolText
	(
		tr("Monkey's Audio - Lossless Audio Compressor"),
		"mac.exe", "v?.??",
		tr("Freely available source code, simple SDK and non-restrictive licensing."),
		"http://www.monkeysaudio.com/"
	);
	moreAboutText += makeToolText
	(
		tr("Shorten - Lossless Audio Compressor"),
		"shorten.exe", "v?.?.?",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://etree.org/shnutils/shorten/"
	);
	moreAboutText += makeToolText
	(
		tr("Speex - Free Codec For Free Speech"),
		"speexdec.exe", "v?.?",
		tr("Open Source patent-free audio format designed for speech."),
		"http://www.speex.org/"
	);
	moreAboutText += makeToolText
	(
		tr("The True Audio - Lossless Audio Codec"),
		"tta.exe", "v?.?",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://tta.sourceforge.net/"
	);
	moreAboutText += makeToolText
	(
		tr("ALAC Decoder"),
		"alac.exe", "v?.?.?",
		tr("Copyright (c) 2004 David Hammerton. Contributions by Cody Brocious."),
		"http://craz.net/programs/itunes/alac.html"
	);
	moreAboutText += makeToolText
	(
		tr("wma2wav - Dump WMA files to Wave Audio"),
		"wma2wav.exe", "????-??-??",
		tr("Copyright (c) 2011 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved."),
		"http://forum.doom9.org/showthread.php?t=140273"
	);
	moreAboutText += makeToolText
	(
		tr("avs2wav - Avisynth to Wave Audio converter"),
		"avs2wav.exe", "v?.?",
		tr("By Jory Stone <jcsston@toughguy.net> and LoRd_MuldeR <mulder2@gmx.de>."),
		"http://forum.doom9.org/showthread.php?t=70882"
	);
	moreAboutText += makeToolText
	(
		tr("dcaenc"),
		"dcaenc.exe", "????-??-??",
		tr("Copyright (c) 2008-2011 Alexander E. Patrakov. Distributed under the LGPL."),
		"http://gitorious.org/dtsenc/dtsenc/trees/master"
	);
	moreAboutText += makeToolText
	(
		tr("MediaInfo - Media File Analysis Tool"),
		"mediainfo.exe", "v?.?.??",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://mediainfo.sourceforge.net/"
	);
	moreAboutText += makeToolText
	(
		tr("SoX - Sound eXchange"),
		"sox.exe", "v??.?.?",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://sox.sourceforge.net/"
	);
	moreAboutText += makeToolText
	(
		tr("GnuPG - The GNU Privacy Guard"),
		"gpgv.exe", "v?.?.??",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://www.gnupg.org/"
	);
	moreAboutText += makeToolText
	(
		tr("GNU Wget - Software for retrieving files using HTTP"),
		"wget.exe", "v?.??.?",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://www.gnu.org/software/wget/"
	);
	moreAboutText += makeToolText
	(
		tr("Silk Icons - Over 700  icons in PNG format"),
		QString(), "v1.3",
		tr("By Mark James, released under the Creative Commons 'by' License."),
		"http://www.famfamfam.com/lab/icons/silk/"
	);
	moreAboutText += QString("</ul></td><td>&nbsp;</td></tr></table></div><br><i>%1</i><br>").arg
	(
		tr("The copyright of LameXP as a whole belongs to LoRd_MuldeR. The copyright of third-party software used in LameXP belongs to the individual authors.")
	);

	softwareLabel->setText(moreAboutText);
	softwareIcon->setPixmap(QIcon(":/images/Logo_Software.png").pixmap(QSize(72,65)));
	connect(softwareLabel, SIGNAL(linkActivated(QString)), this, SLOT(openURL(QString)));
}

void AboutDialog::initLicenseTab(void)
{
	QString licenseText;
	licenseText += ("<tt>");

	QFile file(":/License.txt");
	if(file.open(QIODevice::ReadOnly))
	{
		QTextStream stream(&file);
		unsigned int counter = 0;
		while((!stream.atEnd()) && (stream.status() == QTextStream::Ok))
		{
			QString line = stream.readLine();
			const bool bIsBlank = line.trimmed().isEmpty();
			line.replace('<', "&lt;").replace('>', "&gt;");

			switch(counter)
			{
			case 0:
				if(!bIsBlank) licenseText += QString("<font size=\"+2\">%1</font><br>").arg(line.simplified());
				break;
			case 1:
				if(!bIsBlank) licenseText += QString("<font size=\"+1\">%1</font><br>").arg(line.simplified());
				break;
			default:
				TRIM_RIGHT(line);
				licenseText += QString("<nobr>%1</nobr><br>").arg(line.replace(' ', "&nbsp;"));
				break;
			}

			if(!bIsBlank) counter++;
		}
		licenseText += QString("<br><br>%1").arg(LINK("http://www.gnu.org/licenses/gpl-2.0.html"));
		stream.device()->close();
	}
	else
	{
		licenseText += QString("<font color=\"darkred\">Oups, failed to load license text. Please refer to:</font><br>");
		licenseText += LINK("http://www.gnu.org/licenses/gpl-2.0.html");
	}

	licenseText += ("</tt>");

	licenseLabel->setText(licenseText);
	licenseIcon->setPixmap(QIcon(":/images/Logo_GNU.png").pixmap(QSize(72,65)));
	connect(licenseLabel, SIGNAL(linkActivated(QString)), this, SLOT(openURL(QString)));
}


QString AboutDialog::makeToolText(const QString &toolName, const QString &toolBin, const QString &toolVerFmt, const QString &toolLicense, const QString &toolWebsite, const QString &extraInfo)
{
	QString toolText, verStr(toolVerFmt);

	if(!toolBin.isEmpty())
	{
		verStr = lamexp_version2string(toolVerFmt, lamexp_tool_version(toolBin), tr("n/a"));
	}

	toolText += QString("<li>%1<br>").arg(NOBR(QString("<b>%1 (%2)</b>").arg(toolName, verStr)));
	toolText += QString("%1<br>").arg(NOBR(toolLicense));
	if(!extraInfo.isEmpty()) toolText += QString("<i>%1</i><br>").arg(NOBR(extraInfo));
	toolText += QString("<nobr>%1</nobr>").arg(LINK(toolWebsite));
	toolText += QString("<font style=\"font-size:9px\"><br>&nbsp;</font>");

	return toolText;
}


bool AboutDialog::playResoureSound(const QString &library, const unsigned long soundId, const bool async)
{
	HMODULE module = 0;
	DWORD flags = SND_RESOURCE;
	bool result = false;

	QFileInfo libraryFile(library);
	if(!libraryFile.isAbsolute())
	{
		unsigned int buffSize = GetSystemDirectoryW(NULL, NULL) + 1;
		wchar_t *buffer = (wchar_t*) _malloca(buffSize * sizeof(wchar_t));
		unsigned int result = GetSystemDirectory(buffer, buffSize);
		if(result > 0 && result < buffSize)
		{
			libraryFile.setFile(QString("%1/%2").arg(QDir::fromNativeSeparators(QString::fromUtf16(reinterpret_cast<const unsigned short*>(buffer))), library));
		}
		_freea(buffer);
	}

	if(async)
	{
		flags |= SND_ASYNC;
	}
	else
	{
		flags |= SND_SYNC;
	}

	module = LoadLibraryW(reinterpret_cast<const wchar_t*>(QDir::toNativeSeparators(libraryFile.absoluteFilePath()).utf16()));

	if(module)
	{
		result = PlaySound((LPCTSTR) soundId, module, flags);
		FreeLibrary(module);
	}

	return result;
}