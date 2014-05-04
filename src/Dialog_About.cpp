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

#include "Dialog_About.h"

#include "../tmp/UIC_AboutDialog.h"

#include "Global.h"
#include "Model_Settings.h"

#include <math.h>

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

//Helper macros
#define LINK(URL) QString("<a href=\"%1\">%2</a>").arg(URL).arg(QString(URL).replace("-", "&minus;"))
#define TRIM_RIGHT(STR) do { while(STR.endsWith(QChar(' ')) || STR.endsWith(QChar('\t')) || STR.endsWith(QChar('\r')) || STR.endsWith(QChar('\n'))) STR.chop(1); } while(0)
#define MAKE_TRANSPARENT(WIDGET) do { QPalette _p = (WIDGET)->palette(); _p.setColor(QPalette::Background, Qt::transparent); (WIDGET)->setPalette(_p); } while(0)

//Constants
const char *AboutDialog::neroAacUrl = "http://www.nero.com/eng/company/about-nero/nero-aac-codec.php";
const char *AboutDialog::disqueUrl =  "http://muldersoft.com/?player_url=38X-MXOB014";

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
	{"",   L"",           L"庄泓川",                 "kidneybean@sohu.com"   },
	{NULL, NULL, NULL, NULL}
};

//Special Thanks
static const struct
{
	char* pcName;
	char *pcAddress;
}
g_lamexp_specialThanks[] =
{
	{ "Doom9's Forum",         "http://forum.doom9.org/"       },
	{ "Gleitz | German Doom9", "http://forum.gleitz.info/"     },
	{ "Hydrogenaudio Forums",  "http://www.hydrogenaudio.org/" },
	{ "RareWares",             "http://www.rarewares.org/"     },
	{ "GitHub",                "http://github.com/"            },
	{ "SourceForge",           "http://sourceforge.net/"       },
	{ "Qt Developer Network",  "http://qt-project.org/"        },
	{ "BerliOS Developer",     "http://developer.berlios.de/"  },
	{ "CodePlex",              "http://www.codeplex.com/"      },
	{ "Marius Hudea",          "http://savedonthe.net/"        },
	{ "Codecs.com",            "http://www.codecs.com/"        },
	{ NULL, NULL }
};

//Mirrors
static const struct
{
	char* pcName;
	char *pcAddress;
}
g_lamexp_mirrors[] =
{
	{ "GitHub.com",      "https://github.com/lordmulder/LameXP"              },
	{ "SourceForge.net", "http://sourceforge.net/p/lamexp/code/"             },
	{ "Bitbucket.org",   "https://bitbucket.org/lord_mulder/lamexp"          },
	{ "Gitorious.org",   "https://gitorious.org/lamexp"                      },
	{ "Codeplex.com",    "https://lamexp.codeplex.com/SourceControl/latest"  },
	{ "Berlios.de",      "http://git.berlios.de/cgi-bin/gitweb.cgi?p=lamexp" },
	{ "Assembla.com",    "https://www.assembla.com/spaces/lamexp/"           },
	{ NULL, NULL }
};

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

AboutDialog::AboutDialog(SettingsModel *settings, QWidget *parent, bool firstStart)
:
	QDialog(parent),
	ui(new Ui::AboutDialog),
	m_settings(settings),
	m_initFlags(new QMap<QWidget*,bool>),
	m_disque(NULL),
	m_disqueTimer(NULL),
	m_rotateNext(false),
	m_disqueDelay(_I64_MAX),
	m_lastTab(0)
{
	//Init the dialog, from the .ui file
	ui->setupUi(this);
	setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
	resize(this->minimumSize());
	
	//Disable "X" button
	if(firstStart)
	{
		lamexp_enable_close_button(this, false);
	}

	//Init images
	for(int i = 0; i < 4; i++)
	{
		m_cartoon[i] = NULL;
	}

	//Init tab widget
	connect(ui->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

	//Make transparent
	const type_info &styleType = typeid(*qApp->style());
	if((typeid(QWindowsVistaStyle) == styleType) || (typeid(QWindowsXPStyle) == styleType))
	{
		MAKE_TRANSPARENT(ui->infoScrollArea);
		MAKE_TRANSPARENT(ui->contributorsScrollArea);
		MAKE_TRANSPARENT(ui->softwareScrollArea);
		MAKE_TRANSPARENT(ui->licenseScrollArea);
	}

	//Show about dialog for the first time?
	if(!firstStart)
	{
		lamexp_seed_rand();

		ui->acceptButton->hide();
		ui->declineButton->hide();
		ui->aboutQtButton->show();
		ui->closeButton->show();

		QPixmap disque(":/images/Disque.png");
		m_disque = new QLabel(this, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
		m_disque->resize(disque.size());
		m_disque->setStyleSheet("background:transparent;");
		m_disque->setAttribute(Qt::WA_TranslucentBackground);
		m_disque->setPixmap(disque);
		m_disque->installEventFilter(this);

		connect(QApplication::desktop(), SIGNAL(workAreaResized(int)), this, SLOT(geometryUpdated()));
		geometryUpdated();

		m_discOpacity = 0.01;
		m_disquePos.setX(static_cast<int>(lamexp_rand() % static_cast<unsigned int>(m_disqueBound.right()  - disque.width()  - m_disqueBound.left())) + m_disqueBound.left());
		m_disquePos.setY(static_cast<int>(lamexp_rand() % static_cast<unsigned int>(m_disqueBound.bottom() - disque.height() - m_disqueBound.top()))  + m_disqueBound.top());
		m_disqueFlags[0] = (lamexp_rand() > (UINT_MAX/2));
		m_disqueFlags[1] = (lamexp_rand() > (UINT_MAX/2));
		m_disque->move(m_disquePos);
		m_disque->setWindowOpacity(m_discOpacity);
		m_disque->show();

		m_disqueTimer = new QTimer;
		connect(m_disqueTimer, SIGNAL(timeout()), this, SLOT(moveDisque()));
		m_disqueTimer->start(10);

		connect(ui->aboutQtButton, SIGNAL(clicked()), this, SLOT(showAboutQt()));
	}
	else
	{
		ui->acceptButton->show();
		ui->declineButton->show();
		ui->aboutQtButton->hide();
		ui->closeButton->hide();
	}

	//Activate "show license" button
	ui->showLicenseButton->show();
	connect(ui->showLicenseButton, SIGNAL(clicked()), this, SLOT(gotoLicenseTab()));

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
	LAMEXP_DELETE(ui);
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
			if(!lamexp_play_sound_file("imageres.dll", 5080, true))
			{
				lamexp_play_sound_alias("SystemStart", true);
			}
		}
		else
		{
			lamexp_play_sound("uuaarrgh", true);
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

#define TEMP_HIDE_DISQUE(CMD) do \
{ \
	bool _tmp = (m_disque) ? m_disque->isVisible() : false; \
	if(_tmp) m_disque->hide(); \
	{ CMD } \
	if(_tmp) \
	{ \
		m_discOpacity = 0.01; \
		m_disque->setWindowOpacity(m_discOpacity); \
		m_disque->show(); \
	} \
} \
while(0)

void AboutDialog::tabChanged(int index, const bool silent)
{
	bool bInitialized = m_initFlags->value(ui->tabWidget->widget(index), false);

	if(!bInitialized)
	{
		qApp->setOverrideCursor(QCursor(Qt::WaitCursor));

		if(QWidget *tab = ui->tabWidget->widget(index))
		{
			bool ok = false;

			if(tab == ui->infoTab) { initInformationTab(); ok = true; }
			if(tab == ui->contributorsTab) { initContributorsTab(); ok = true; }
			if(tab == ui->softwareTab) { initSoftwareTab(); ok = true; }
			if(tab == ui->licenseTab) { initLicenseTab(); ok = true; }

			if(ok)
			{
				m_initFlags->insert(tab, true);
			}
			else
			{
				qWarning("Unknown tab %p encountered, cannot initialize !!!", tab);
			}
			
		}

		ui->tabWidget->widget(index)->update();
		qApp->processEvents();
		qApp->restoreOverrideCursor();
	}

	//Play tick sound
	if(m_settings->soundsEnabled() && (!silent))
	{
		lamexp_play_sound("tick", true);
	}

	//Scroll to the top
	if(QWidget *tab = ui->tabWidget->widget(index))
	{
		if(tab == ui->infoTab) ui->infoScrollArea->verticalScrollBar()->setSliderPosition(0);
		if(tab == ui->contributorsTab) ui->contributorsScrollArea->verticalScrollBar()->setSliderPosition(0);
		if(tab == ui->softwareTab) ui->softwareScrollArea->verticalScrollBar()->setSliderPosition(0);
		if(tab == ui->licenseTab) ui->licenseScrollArea->verticalScrollBar()->setSliderPosition(0);
	}

	//Update license button
	ui->showLicenseButton->setChecked(ui->tabWidget->widget(index) == ui->licenseTab);
	if(ui->tabWidget->widget(index) != ui->licenseTab) m_lastTab = index;
}

void AboutDialog::enableButtons(void)
{
	ui->acceptButton->setEnabled(true);
	ui->declineButton->setEnabled(true);
	setCursor(QCursor(Qt::ArrowCursor));
}

void AboutDialog::openURL(const QString &url)
{
	if(!QDesktopServices::openUrl(QUrl(url)))
	{
		lamexp_exec_shell(this, url);
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
	ui->tabWidget->setCurrentIndex(ui->tabWidget->indexOf(ui->showLicenseButton->isChecked() ? ui->licenseTab : ui->tabWidget->widget(m_lastTab)));
}

void AboutDialog::moveDisque(void)
{
	int delta = 2;
	const __int64 perfFrequ = lamexp_perfcounter_frequ();
	const __int64 perfCount = lamexp_perfcounter_value();

	if((perfFrequ >= 0) && (perfCount >= 0))
	{
		if(m_disqueDelay != _I64_MAX)
		{
			const double delay = static_cast<double>(perfCount) - static_cast<double>(m_disqueDelay);
			delta = qMax(1, qMin(128, static_cast<int>(ceil(delay / static_cast<double>(perfFrequ) / 0.00512))));
		}
		m_disqueDelay = perfCount;
	}

	if(m_disque)
	{
		if(m_disquePos.x() <= m_disqueBound.left())   { m_disqueFlags[0] = true;  m_rotateNext = true; }
		if(m_disquePos.x() >= m_disqueBound.right())  { m_disqueFlags[0] = false; m_rotateNext = true; }
		if(m_disquePos.y() <= m_disqueBound.top())    { m_disqueFlags[1] = true;  m_rotateNext = true; }
		if(m_disquePos.y() >= m_disqueBound.bottom()) { m_disqueFlags[1] = false; m_rotateNext = true; }
		
		m_disquePos.setX(m_disqueFlags[0] ? (m_disquePos.x() + delta) : (m_disquePos.x() - delta));
		m_disquePos.setY(m_disqueFlags[1] ? (m_disquePos.y() + delta) : (m_disquePos.y() - delta));

		m_disque->move(m_disquePos);
		
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
				if(m_disque->size() != cartoon->size())
				{
					m_disque->resize(cartoon->size());
					geometryUpdated();
				}
			}
			m_rotateNext = false;
		}

		if(m_discOpacity != 1.0)
		{
			m_discOpacity = m_discOpacity + 0.01;
			if(qFuzzyCompare(m_discOpacity, 1.0) || (m_discOpacity > 1.0))
			{
				m_discOpacity = 1.0;
			}
			m_disque->setWindowOpacity(m_discOpacity);
			m_disque->update();
		}
	}
}

void AboutDialog::geometryUpdated(void)
{
	if(m_disque)
	{
		QRect screenGeometry = QApplication::desktop()->availableGeometry();
		m_disqueBound.setLeft(screenGeometry.left());
		m_disqueBound.setRight(screenGeometry.width() - m_disque->width() + screenGeometry.left());
		m_disqueBound.setTop(screenGeometry.top());
		m_disqueBound.setBottom(screenGeometry.height() - m_disque->height() + screenGeometry.top());
	}
	else
	{
		m_disqueBound = QApplication::desktop()->availableGeometry();
	}
}

void AboutDialog::adjustSize(void)
{
	const int maxH = QApplication::desktop()->availableGeometry().height();
	const int maxW = QApplication::desktop()->availableGeometry().width();

	const int deltaH = ui->infoScrollArea->widget()->height() - ui->infoScrollArea->viewport()->height();
	const int deltaW = ui->infoScrollArea->widget()->width()  - ui->infoScrollArea->viewport()->width();

	if(deltaH > 0)
	{
		this->resize(this->width(), qMin(this->height() + deltaH, maxH));
		this->move(this->x(), this->y() - (deltaH / 2));
		this->setMinimumHeight(qMax(this->minimumHeight(), this->height()));
	}

	if(deltaW > 0)
	{
		this->resize(qMin(this->width() + deltaW, maxW), this->height());
		this->move(this->x() - (deltaW / 2), this->y());
		this->setMinimumWidth(qMax(this->minimumWidth(), this->width()));
	}
}

////////////////////////////////////////////////////////////
// Protected Functions
////////////////////////////////////////////////////////////

void AboutDialog::showEvent(QShowEvent *e)
{
	QDialog::showEvent(e);
	
	ui->tabWidget->setCurrentIndex(ui->tabWidget->indexOf(ui->infoTab));
	tabChanged(m_lastTab = ui->tabWidget->currentIndex(), true);
	
	if(m_firstShow)
	{
		ui->acceptButton->setEnabled(false);
		ui->declineButton->setEnabled(false);
		QTimer::singleShot(5000, this, SLOT(enableButtons()));
		setCursor(QCursor(Qt::WaitCursor));
	}

	QTimer::singleShot(0, this, SLOT(adjustSize()));
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
		qMax(lamexp_version_date().year(), lamexp_current_date_safe().year())
	);

	QString aboutText;

	aboutText += QString("<h2>%1</h2>").arg(NOBR(tr("LameXP - Audio Encoder Front-end")));
	aboutText += QString("<b>%1</b><br>").arg(NOBR(copyrightStr));
	aboutText += QString("<b>%1</b><br><br>").arg(NOBR(versionStr));
	aboutText += QString("%1<br>").arg(NOBR(tr("Please visit %1 for news and updates!").arg(LINK(lamexp_website_url()))));

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	const QDate currentDate = lamexp_current_date_safe();
	if(LAMEXP_DEBUG)
	{
		int daysLeft = qMax(currentDate.daysTo(lamexp_version_expires()), 0);
		aboutText += QString("<hr><font color=\"crimson\">%1</font>").arg(NOBR(QString("!!! --- DEBUG BUILD --- Expires at: %1 &middot; Days left: %2 --- DEBUG BUILD --- !!!").arg(lamexp_version_expires().toString(Qt::ISODate), QString::number(daysLeft))));
	}
	else if(lamexp_version_demo())
	{
		int daysLeft = qMax(currentDate.daysTo(lamexp_version_expires()), 0);
		aboutText += QString("<hr><font color=\"crimson\">%1</font>").arg(NOBR(tr("Note: This demo (pre-release) version of LameXP will expire at %1. Still %2 days left.").arg(lamexp_version_expires().toString(Qt::ISODate), QString::number(daysLeft))));
	}
#else
	const QDate currentDate = lamexp_current_date_safe();
	if(LAMEXP_DEBUG)
	{
		int daysLeft = qMax(currentDate.daysTo(lamexp_version_expires()), 0i64);
		aboutText += QString("<hr><font color=\"crimson\">%1</font>").arg(NOBR(QString("!!! --- DEBUG BUILD --- Expires at: %1 &middot; Days left: %2 --- DEBUG BUILD --- !!!").arg(lamexp_version_expires().toString(Qt::ISODate), QString::number(daysLeft))));
	}
	else if(lamexp_version_demo())
	{
		int daysLeft = qMax(currentDate.daysTo(lamexp_version_expires()), 0i64);
		aboutText += QString("<hr><font color=\"crimson\">%1</font>").arg(NOBR(tr("Note: This demo (pre-release) version of LameXP will expire at %1. Still %2 days left.").arg(lamexp_version_expires().toString(Qt::ISODate), QString::number(daysLeft))));
	}
#endif

	aboutText += "<hr><br>";
	
	aboutText += "<nobr><tt>This program is free software; you can redistribute it and/or modify<br>";
	aboutText += "it under the terms of the GNU General Public License as published by<br>";
	aboutText += "the Free Software Foundation; either version 2 of the License, or<br>";
	aboutText += "(at your option) any later version, but always including the *additional*<br>";
	aboutText += "restrictions defined in the \"License.txt\" file (see \"License\" tab).<br><br>";
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

	ui->infoLabel->setText(aboutText);
	ui->infoIcon->setPixmap(lamexp_app_icon().pixmap(QSize(72,72)));
	connect(ui->infoLabel, SIGNAL(linkActivated(QString)), this, SLOT(openURL(QString)));
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
	for(int i = 0; g_lamexp_specialThanks[i].pcName; i++)
	{
		contributorsAboutText += QString("<tr><td valign=\"middle\">%1</td><td>%2</td>").arg(webIcon, spaces);
		contributorsAboutText += QString("<td valign=\"middle\">%1</td><td>%2</td><td valign=\"middle\" colspan=\"3\"><a href=\"%3\">%3</td></tr>").arg(QString::fromLatin1(g_lamexp_specialThanks[i].pcName), spaces, QString::fromLatin1(g_lamexp_specialThanks[i].pcAddress));
	}

	contributorsAboutText += QString("<tr><td colspan=\"7\"><b>&nbsp;</b></td></tr>");
	contributorsAboutText += QString("<tr><td colspan=\"7\"><b>%1</b>%2</td></tr>").arg(tr("Official Mirrors:"), extraVSpace);

	QString serverIcon = QString("<img src=\":/icons/%1.png\">").arg("server_database");
	for(int i = 0; g_lamexp_mirrors[i].pcName; i++)
	{
		contributorsAboutText += QString("<tr><td valign=\"middle\">%1</td><td>%2</td>").arg(serverIcon, spaces);
		contributorsAboutText += QString("<td valign=\"middle\">%1</td><td>%2</td><td valign=\"middle\" colspan=\"3\"><a href=\"%3\">%3</td></tr>").arg(QString::fromLatin1(g_lamexp_mirrors[i].pcName), spaces, QString::fromLatin1(g_lamexp_mirrors[i].pcAddress));
	}

	contributorsAboutText += "</table><br><br><br>";
	contributorsAboutText += QString("<i>%1</i><br>").arg(NOBR(tr("If you are willing to contribute a LameXP translation, feel free to contact us!")));

	ui->contributorsLabel->setText(contributorsAboutText);
	ui->contributorsIcon->setPixmap(QIcon(":/images/Logo_Contributors.png").pixmap(QSize(72,84)));
	connect(ui->contributorsLabel, SIGNAL(linkActivated(QString)), this, SLOT(openURL(QString)));
}

void AboutDialog::initSoftwareTab(void)
{
	QString moreAboutText;

	moreAboutText += QString("<h3>%1</h3>").arg(tr("The following third-party software is used in LameXP:"));
	moreAboutText += "<div style=\"margin-left:-25px;white-space:nowrap\"><table><tr><td><ul>"; //;font-size:7pt
	
	moreAboutText += makeToolText
	(
		tr("LAME - OpenSource mp3 Encoder"),
		"lame.exe", "v?.??, #-?",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://lame.sourceforge.net/"
	);
	moreAboutText += makeToolText
	(
		tr("OggEnc - Ogg Vorbis Encoder"),
		"oggenc2.exe", "v?.??, aoTuV #-?.??",
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
		"opusenc.exe", "#, ????-??-??",
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
		tr("Valdec from AC3Filter Tools - AC3/DTS Decoder"),
		"valdec.exe", "v?.??#",
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
		tr("refalac - Win32 command line ALAC encoder/decoder"),
		"refalac.exe", "v?.??",
		tr("The ALAC reference implementation by Apple is available under the Apache license."),
		"http://alac.macosforge.org/"
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
		tr("UPX - The Ultimate Packer for eXecutables"),
		QString(), "v3.09",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://upx.sourceforge.net/"
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

	ui->softwareLabel->setText(moreAboutText);
	ui->softwareIcon->setPixmap(QIcon(":/images/Logo_Software.png").pixmap(QSize(72,65)));
	connect(ui->softwareLabel, SIGNAL(linkActivated(QString)), this, SLOT(openURL(QString)));
}

void AboutDialog::initLicenseTab(void)
{
	bool bFoundHeader = false;
	QRegExp header("^(\\s*)(GNU GENERAL PUBLIC LICENSE)(\\s*)$");

	QString licenseText;
	licenseText += ("<tt>");

	QFile file(":/License.txt");
	if(file.open(QIODevice::ReadOnly))
	{
		QTextStream stream(&file);
		while((!stream.atEnd()) && (stream.status() == QTextStream::Ok))
		{
			QString line = stream.readLine();
			line.replace('<', "&lt;").replace('>', "&gt;");
			if((!bFoundHeader) && (header.indexIn(line) >= 0))
			{
				line.replace(header, "\\1<b>\\2</b>\\3");
				bFoundHeader = true;
			}
			TRIM_RIGHT(line);
			licenseText += QString("<nobr>%1</nobr><br>").arg(line.replace(' ', "&nbsp;"));
		}
		licenseText += QString("<br>");
		stream.device()->close();
	}
	else
	{
		licenseText += QString("<font color=\"darkred\">Oups, failed to load license text. Please refer to:</font><br>");
		licenseText += LINK("http://www.gnu.org/licenses/gpl-2.0.html");
	}

	licenseText += ("</tt>");

	ui->licenseLabel->setText(licenseText);
	ui->licenseIcon->setPixmap(QIcon(":/images/Logo_GNU.png").pixmap(QSize(72,65)));
	connect(ui->licenseLabel, SIGNAL(linkActivated(QString)), this, SLOT(openURL(QString)));
}


QString AboutDialog::makeToolText(const QString &toolName, const QString &toolBin, const QString &toolVerFmt, const QString &toolLicense, const QString &toolWebsite, const QString &extraInfo)
{
	QString toolText, toolTag, verStr(toolVerFmt);

	if(!toolBin.isEmpty())
	{
		const unsigned int version = lamexp_tool_version(toolBin, &toolTag);
		verStr = lamexp_version2string(toolVerFmt, version, tr("n/a"), &toolTag);
	}

	toolText += QString("<li>%1<br>").arg(NOBR(QString("<b>%1 (%2)</b>").arg(toolName, verStr)));
	toolText += QString("%1<br>").arg(NOBR(toolLicense));
	if(!extraInfo.isEmpty()) toolText += QString("<i>%1</i><br>").arg(NOBR(extraInfo));
	toolText += QString("<nobr>%1</nobr>").arg(LINK(toolWebsite));
	toolText += QString("<font style=\"font-size:9px\"><br>&nbsp;</font>");

	return toolText;
}
