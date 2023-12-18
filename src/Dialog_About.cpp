///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2023 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU GENERAL PUBLIC LICENSE as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version; always including the non-optional
// LAMEXP GNU GENERAL PUBLIC LICENSE ADDENDUM. See "License.txt" file!
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

#include "UIC_AboutDialog.h"

//Internal
#include "Global.h"
#include "Model_Settings.h"

//MUtils
#include <MUtils/Global.h>
#include <MUtils/OSSupport.h>
#include <MUtils/Sound.h>
#include <MUtils/GUI.h>
#include <MUtils/Version.h>

//Qt
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
#include <QElapsedTimer>
#include <QLibraryInfo>
#include <QWindowsVistaStyle>
#include <QWindowsXPStyle>

//CRT
#include <math.h>
#include <typeinfo>

//Helper macros
#define LINK(URL) QString("<a href=\"%1\">%2</a>").arg(URL).arg(QString(URL).replace("-", "&minus;"))
#define TRIM_RIGHT(STR) do { while(STR.endsWith(QChar(' ')) || STR.endsWith(QChar('\t')) || STR.endsWith(QChar('\r')) || STR.endsWith(QChar('\n'))) STR.chop(1); } while(0)
#define PLAY_SOUND_OPTIONAL(NAME, ASYNC) do { if(m_settings->soundsEnabled()) MUtils::Sound::play_sound((NAME), (ASYNC)); } while(0)
#define MAKE_TRANSPARENT(WIDGET) do { QPalette _p = (WIDGET)->palette(); _p.setColor(QPalette::Background, Qt::transparent); (WIDGET)->setPalette(_p); } while(0)

//Constants
const char *AboutDialog::neroAacUrl = "http://www.videohelp.com/software/Nero-AAC-Codec"; //"http://www.nero.com/eng/company/about-nero/nero-aac-codec.php"
const char *AboutDialog::disqueUrl =  "https://www.youtube.com/watch?v=P5D6NtIFULA?autoplay=1"; //n4bply6Ibqw

//Contributors
static const struct 
{
	char *pcFlag;
	wchar_t *pcLanguage;
	wchar_t *pcName;
	char *pcContactAddr;
}
g_lamexp_translators[] =
{
	{"en", L"English",    L"LoRd_MuldeR",         "MuldeR2@GMX.de"           },
	{"de", L"Deutsch",    L"LoRd_MuldeR",         "MuldeR2@GMX.de"           },
	{"",   L"",           L"Bodo Thevissen",      "Bodo@thevissen.de"        },
	{"es", L"Español",    L"Rub3nCT",             "Rub3nCT@gmail.com"        },
	{"fr", L"Française",  L"Dodich Informatique", "Dodich@live.fr"           },
	{"",   L"Française",  L"Sami Ghoul-Duclos",   "samgd14@live.ca"          },
	{"hu", L"Magyarul",   L"ZityiSoft Team",      "zityisoft@gmail.com"      },
	{"it", L"Italiano",   L"Roberto",             "Gulliver_69@libero.it"    },
	{"",   L"",           L"Gianluca Papi",       "johnnyb.goode68@gmail.com"},
	{"ja", L"日本語",        L"Maboroshin",          "pc.genkaku.in"            },
	{"kr", L"한국어",        L"JaeHyung Lee",        "Kolanp@gmail.com"         },
	{"pl", L"Polski",     L"Sir Daniel K",        "Sir.Daniel.K@gmail.com"   },
	{"ru", L"Русский",    L"Neonailol",           "Neonailol@gmail.com"      },
	{"",   L"",           L"Иван Митин",          "bardak@inbox.ru"          },
	{"sv", L"Svenska",    L"Åke Engelbrektson",   "eson@svenskasprakfiler.se"},
	{"tw", L"繁体中文",       L"456Vv",               "123@456vv.com"            },
	{"uk", L"Українська", L"Arestarh",            "Arestarh@ukr.net"         },
	{"zh", L"简体中文",       L"456Vv",               "123@456vv.com"            },
	{"",   L"",           L"庄泓川",                 "kidneybean@pku.edu.cn"    },
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
	{ "Doom9's Forum",         "http://forum.doom9.org/"          },
	{ "Gleitz | German Doom9", "http://forum.gleitz.info/"        },
	{ "Portable Freeware",     "http://www.portablefreeware.com/" },
	{ "Hydrogenaudio Forums",  "http://www.hydrogenaudio.org/"    },
	{ "RareWares",             "http://www.rarewares.org/"        },
	{ "GitHub",                "http://github.com/"               },
	{ "SourceForge",           "http://sourceforge.net/"          },
	{ "OSDN.net",              "http://osdn.net/"                 },
	{ "Marius Hudea",          "http://savedonthe.net/"           },
	{ "Qt Developer Network",  "http://www.qt.io/developers/"     },
	{ "Codecs.com",            "http://www.codecs.com/"           },
	{ "VideoHelp",             "http://www.videohelp.com/"        },
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
	{ "GitHub.com",      "https://github.com/lordmulder/LameXP"             },
	{ "SourceForge.net", "http://sourceforge.net/p/lamexp/code/"            },
    { "OSDN.net",        "https://osdn.net/projects/lamexp/scm/git/LameXP/" },
	{ "Bitbucket.org",   "https://bitbucket.org/muldersoft/lamexp"          },
	{ "GitLab.com"   ,   "https://gitlab.com/lamexp/lamexp"                 },
	{ "Assembla.com",    "https://www.assembla.com/spaces/lamexp/"          },
	{ "repo.or.cz",      "https://repo.or.cz/w/LameXP.git"                  },
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
	m_lastTab(0),
	m_firstStart(firstStart)
{
	//Init the dialog, from the .ui file
	ui->setupUi(this);
	setWindowFlags(windowFlags() & (~Qt::WindowContextHelpButtonHint));
	setMinimumSize(this->size());

	//Adjust size to DPI settings and re-center
	MUtils::GUI::scale_widget(this);

	//Disable "X" button
	if(firstStart)
	{
		MUtils::GUI::enable_close_button(this, false);
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
		ui->acceptButton->hide();
		ui->declineButton->hide();
		ui->aboutQtButton->show();
		ui->closeButton->show();

		QPixmap disque(":/images/Disque.png");
		m_disque.reset(new QLabel(this, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint));
		m_disque->resize(disque.size());
		m_disque->setStyleSheet("background:transparent;");
		m_disque->setAttribute(Qt::WA_TranslucentBackground);
		m_disque->setPixmap(disque);
		m_disque->setCursor(QCursor(Qt::PointingHandCursor));
		m_disque->installEventFilter(this);

		connect(QApplication::desktop(), SIGNAL(workAreaResized(int)), this, SLOT(geometryUpdated()));
		geometryUpdated();

		m_discOpacity = 0.01;
		m_disquePos.setX(static_cast<int>(MUtils::next_rand_u32() % static_cast<unsigned int>(m_disqueBound.right()  - disque.width()  - m_disqueBound.left())) + m_disqueBound.left());
		m_disquePos.setY(static_cast<int>(MUtils::next_rand_u32() % static_cast<unsigned int>(m_disqueBound.bottom() - disque.height() - m_disqueBound.top()))  + m_disqueBound.top());
		m_disqueFlags[0] = (MUtils::next_rand_u32() > (UINT_MAX/2));
		m_disqueFlags[1] = (MUtils::next_rand_u32() > (UINT_MAX/2));
		m_disque->move(m_disquePos);
		m_disque->setWindowOpacity(m_discOpacity);

		m_disqueTimer.reset(new QTimer());

		connect(m_disqueTimer.data(), SIGNAL(timeout()), this, SLOT(moveDisque()));
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
}

AboutDialog::~AboutDialog(void)
{
	if(m_disque)
	{
		m_disque->close();
	}
	if(m_disqueTimer)
	{
		m_disqueTimer->stop();
	}
	MUTILS_DELETE(ui);
}

////////////////////////////////////////////////////////////
// Public Functions
////////////////////////////////////////////////////////////

int AboutDialog::exec()
{
	if(m_firstStart)
	{
		if (m_settings->soundsEnabled())
		{
			if (!MUtils::Sound::play_sound_file("imageres.dll", 5080, true))
			{
				MUtils::Sound::play_system_sound("WindowsLogon", true);
			}
		}
	}
	else
	{
		PLAY_SOUND_OPTIONAL("slunk", true);
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

	//Play tick sound, unless silent
	if (!silent)
	{
		PLAY_SOUND_OPTIONAL("tick", true);
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
		MUtils::OS::shell_open(this, url);
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
	QElapsedTimer elapsedTimer;
	elapsedTimer.start();

	if((!m_disqueDelay.isNull()) && m_disqueDelay->isValid())
	{
		const qint64 delay = m_disqueDelay->restart();
		delta = qBound(1, static_cast<int>(ceil(static_cast<double>(delay) / 5.12)), 128);
	}
	else
	{
		m_disqueDelay.reset(new QElapsedTimer());
		m_disqueDelay->start();
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
			const QPixmap *const cartoon = m_cartoon[m_disqueFlags[0] ? (m_disqueFlags[1] ? 1 : 0): (m_disqueFlags[1] ? 2 : 3)].data();
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
	
	if(m_firstStart)
	{
		ui->acceptButton->setEnabled(false);
		ui->declineButton->setEnabled(false);
		QTimer::singleShot(5000, this, SLOT(enableButtons()));
		setCursor(QCursor(Qt::WaitCursor));
	}

	if (!(m_disque.isNull() || m_disqueTimer.isNull()))
	{
		m_disque->show();
		m_disqueTimer->start(10);
	}

	QTimer::singleShot(0, this, SLOT(adjustSize()));
}

void AboutDialog::closeEvent(QCloseEvent *e)
{
	if (m_firstStart)
	{
		e->ignore();
	}
}

bool AboutDialog::eventFilter(QObject *obj, QEvent *event)
{
	if((!m_disque.isNull()) && (obj == m_disque.data()) && (event->type() == QEvent::MouseButtonPress))
	{
		PLAY_SOUND_OPTIONAL("chicken", true);
		if (!m_cartoon[0])
		{
			QPixmap cartoon(":/images/Cartoon.png");
			for(int i = 0; i < 4; i++)
			{
				m_cartoon[i].reset(new QPixmap(cartoon.transformed(QMatrix().rotate(static_cast<double>(i*90) + 45.0), Qt::SmoothTransformation)));
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
	const QDate versionDate = MUtils::Version::app_build_date();
	const QTime versionTime = MUtils::Version::app_build_time();

	const QString versionStr = QString("Version %1 %2, Build %3 [%4] [%5]").arg
	(
		QString().sprintf("%u.%02u", lamexp_version_major(), lamexp_version_minor()),
		QString::fromLatin1(lamexp_version_release()),
		QString::number(lamexp_version_build()),
		versionDate.toString(Qt::ISODate),
		versionTime.toString(Qt::ISODate)
	);

	const QString platformStr = QString("%1 [%2], MUtilities %3 [%4] [%5], Qt Framework v%6 [%7]").arg
	(
		QString::fromLatin1(MUtils::Version::compiler_version()),
		QString::fromLatin1(MUtils::Version::compiler_arch()),
		QString().sprintf("%u.%02u", MUtils::Version::lib_version_major(), MUtils::Version::lib_version_minor()),
		MUtils::Version::lib_build_date().toString(Qt::ISODate).toLatin1().constData(),
		MUtils::Version::lib_build_time().toString(Qt::ISODate).toLatin1().constData(),
		QString::fromLatin1(qVersion()),
		QLibraryInfo::buildDate().toString(Qt::ISODate)
	);

	const QString copyrightStr = QString().sprintf
	(
		"Copyright (C) 2004-%04d LoRd_MuldeR &lt;MuldeR2@GMX.de&gt;. Some rights reserved.",
		qMax(versionDate.year(), MUtils::OS::current_date().year())
	);

	QString aboutText;

	aboutText += QString("<h2>%1</h2>").arg(tr("LameXP - Audio Encoder Front-end"));
	aboutText += QString("<b>%1</b><br>").arg(copyrightStr);
	aboutText += QString("<b>%1</b><br>").arg(versionStr);
	aboutText += QString("<b>%1</b><br><br>").arg(platformStr);
	aboutText += QString("%1<br>").arg(tr("Please visit %1 for news and updates!").arg(LINK(lamexp_website_url())));

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	const QDate currentDate = MUtils::OS::current_date();
	if(MUTILS_DEBUG)
	{
		int daysLeft = qMax(currentDate.daysTo(lamexp_version_expires()), 0);
		aboutText += QString("<hr><font color=\"crimson\">%1</font>").arg(QString("!!! --- DEBUG BUILD --- Expires at: %1 &middot; Days left: %2 --- DEBUG BUILD --- !!!").arg(lamexp_version_expires().toString(Qt::ISODate), QString::number(daysLeft)));
	}
	else if(lamexp_version_test())
	{
		int daysLeft = qMax(currentDate.daysTo(lamexp_version_expires()), 0);
		aboutText += QString("<hr><font color=\"crimson\">%1</font>").arg(tr("Note: This demo (pre-release) version of LameXP will expire at %1. Still %2 days left.").arg(lamexp_version_expires().toString(Qt::ISODate), QString::number(daysLeft)));
	}
#else
	const QDate currentDate = lamexp_current_date_safe();
	if(LAMEXP_DEBUG)
	{
		int daysLeft = qMax(currentDate.daysTo(lamexp_version_expires()), 0i64);
		aboutText += QString("<hr><font color=\"crimson\">%1</font>").arg(QString("!!! --- DEBUG BUILD --- Expires at: %1 &middot; Days left: %2 --- DEBUG BUILD --- !!!").arg(lamexp_version_expires().toString(Qt::ISODate), QString::number(daysLeft)));
	}
	else if(lamexp_version_demo())
	{
		int daysLeft = qMax(currentDate.daysTo(lamexp_version_expires()), 0i64);
		aboutText += QString("<hr><font color=\"crimson\">%1</font>").arg(tr("Note: This demo (pre-release) version of LameXP will expire at %1. Still %2 days left.").arg(lamexp_version_expires().toString(Qt::ISODate), QString::number(daysLeft)));
	}
#endif

	aboutText += "<hr><br>";
	
	aboutText += "<tt>This program is free software; you can redistribute it and/or modify<br>";
	aboutText += "it under the terms of the GNU GENERAL PUBLIC LICENSE as published by<br>";
	aboutText += "the Free Software Foundation; either version 2 of the License, or<br>";
	aboutText += "(at your option) any later version; always including the non-optional<br>";
	aboutText += "refinements defined in the \"License.txt\" file (see \"License\" tab).<br><br>";
	aboutText += "This program is distributed in the hope that it will be useful,<br>";
	aboutText += "but WITHOUT ANY WARRANTY; without even the implied warranty of<br>";
	aboutText += "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the<br>";
	aboutText += "GNU General Public License for more details.<br><br>";
	aboutText += "You should have received a copy of the GNU General Public License<br>";
	aboutText += "along with this program; if not, write to the Free Software<br>";
	aboutText += "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110&minus;1301, USA.</tt><br>";
	aboutText += "<hr><table style=\"margin-top:4px\"><tr>";
	aboutText += "<td valign=\"middle\"><img src=\":/icons/error_big.png\"</td><td>&nbsp;</td>";
	aboutText += QString("<td style='white-space:normal'><font color=\"darkred\">%1</font></td>").arg(tr("Note: LameXP is free software. Do <b>not</b> pay money to obtain or use LameXP! If some third-party website tries to make you pay for downloading LameXP, you should <b>not</b> respond to the offer !!!"));
	aboutText += "</tr></table>";

	ui->infoLabel->setText(NOBREAK(aboutText));
	ui->infoIcon->setPixmap(lamexp_app_icon().pixmap(QSize(72,72)));
	connect(ui->infoLabel, SIGNAL(linkActivated(QString)), this, SLOT(openURL(QString)));
}

void AboutDialog::initContributorsTab(void)
{
	const QString spaces("&nbsp;&nbsp;&nbsp;&nbsp;");
	const QString extraVSpace("<font style=\"font-size:7px\"><br>&nbsp;</font>");
	
	QString contributorsAboutText;
	contributorsAboutText += QString("<h3>%1</h3>").arg(tr("The following people have contributed to LameXP:"));
	contributorsAboutText += "<table style=\"margin-top:12px;white-space:nowrap\">";
	
	contributorsAboutText += QString("<tr><td colspan=\"7\"><b>%1</b>%2</td></tr>").arg(tr("Programmers:"), extraVSpace);
	QString icon = QString("<img src=\":/icons/%1.png\">").arg("user_gray");
	contributorsAboutText += QString("<tr><td valign=\"middle\">%1</td><td>%2</td>").arg(icon, spaces);
	contributorsAboutText += QString("<td valign=\"middle\">%1</td><td>%2</td>").arg(tr("Project Leader"), spaces);
	contributorsAboutText += QString("<td valign=\"middle\">%1</td><td>%2</td><td><a href=\"mailto:%3?subject=LameXP\">&lt;%3&gt;</a></td></tr>").arg(L1S("LoRd_MuldeR"), spaces, L1S("MuldeR2@GMX.de"));
	contributorsAboutText += QString("<tr><td colspan=\"7\"><b>&nbsp;</b></td></tr>");

	contributorsAboutText += QString("<tr><td colspan=\"7\"><b>%1</b>%2</td></tr>").arg(tr("Translators:"), extraVSpace);
	for(int i = 0; g_lamexp_translators[i].pcName; i++)
	{
		const QString flagIcon = (strlen(g_lamexp_translators[i].pcFlag) > 0) ? QString("<img src=\":/flags/%1.png\">").arg(QString::fromLatin1(g_lamexp_translators[i].pcFlag)) : QString();
		const QString contactAddr = QString::fromLatin1(g_lamexp_translators[i].pcContactAddr);
		const QString linkUrl = QString(contactAddr.contains('@') ? "mailto:%1?subject=LameXP" : "http://%1").arg(contactAddr);
		contributorsAboutText += QString("<tr><td valign=\"middle\">%1</td><td>%2</td>").arg(flagIcon, spaces);
		contributorsAboutText += QString("<td valign=\"middle\">%1</td><td>%2</td>").arg(MUTILS_QSTR(g_lamexp_translators[i].pcLanguage), spaces);
		contributorsAboutText += QString("<td valign=\"middle\">%1</td><td>%2</td><td><a href=\"%3\">&lt;%4&gt;</a></td></tr>").arg(MUTILS_QSTR(g_lamexp_translators[i].pcName), spaces, linkUrl, contactAddr);
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
	contributorsAboutText += QString("<i>%1</i><br>").arg(tr("If you are willing to contribute a LameXP translation, feel free to contact us!"));

	ui->contributorsLabel->setText(NOBREAK(contributorsAboutText));
	ui->contributorsIcon->setPixmap(QIcon(":/images/Logo_Contributors.png").pixmap(QSize(72,84)));
	connect(ui->contributorsLabel, SIGNAL(linkActivated(QString)), this, SLOT(openURL(QString)));
}

void AboutDialog::initSoftwareTab(void)
{
	QString moreAboutText;

	moreAboutText += QString("<h3>%1</h3>").arg(tr("The following third-party software is used in LameXP:"));
	moreAboutText += "<ul style='margin-left:-25px'>"; //;font-size:7pt
	
	moreAboutText += makeToolText
	(
		tr("LAME - OpenSource mp3 Encoder"),
		"lame.exe", "v?.???, #-?",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://lame.sourceforge.net/"
	);
	moreAboutText += makeToolText
	(
		tr("OggEnc - Vorbis Encoder"),
		"oggenc2.exe", "v?.??, libvorbis v?.?? + aoTuV b?.??_#",
		tr("Completely open and patent-free audio encoding technology."),
		"http://www.rarewares.org/ogg-oggenc.php"
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
		"mpg123.exe", "v?.??.??",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://www.mpg123.de/"
	);
	moreAboutText += makeToolText
	(
		tr("FAAD - OpenSource MPEG-4 and MPEG-2 AAC Decoder"),
		"faad.exe", "v?.?.?",
		tr("Released under the terms of the GNU General Public License."),
		"https://sourceforge.net/projects/faac/"	//"http://www.audiocoding.com/"
	);
	moreAboutText += makeToolText
	(
		tr("OggDec - Vorbis Decoder"),
		"oggdec.exe", "v?.??.?",
		tr("Command line Ogg Vorbis decoder created by John33."),
		"http://www.rarewares.org/ogg-oggdec.php"
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
		tr("Copyright (c) 2011 LoRd_MuldeR &lt;mulder2@gmx.de&gt;. Some rights reserved."),
		"http://forum.doom9.org/showthread.php?t=140273"
	);
	moreAboutText += makeToolText
	(
		tr("avs2wav - Avisynth to Wave Audio converter"),
		"avs2wav.exe", "v?.?",
		tr("By Jory Stone &lt;jcsston@toughguy.net&gt; and LoRd_MuldeR &lt;mulder2@gmx.de&gt;."),
		"http://forum.doom9.org/showthread.php?t=70882"
	);
	moreAboutText += makeToolText
	(
		tr("dcaenc"),
		"dcaenc.exe", "????-??-??",
		tr("Copyright (c) 2008-2011 Alexander E. Patrakov. Distributed under the LGPL."),
		"https://gitlab.com/patrakov/dcaenc"
	);
	moreAboutText += makeToolText
	(
		tr("MediaInfo - Media File Analysis Tool"),
		"mediainfo.exe", "v??.??.?",
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
		tr("cURL - Curl URL Request Library"),
		"curl.exe", "v?.??.?",
		tr("By Daniel Stenberg, released under the terms of the MIT/X Derivate License."),
		"https://curl.haxx.se/"
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
		tr("Silk Icons - Over 700 icons in PNG format"),
		QString(), "v1.3",
		tr("By Mark James, released under the Creative Commons 'BY' License."),
		"http://www.famfamfam.com/lab/icons/silk/"
	);
	moreAboutText += makeToolText
	(
		tr("Angry Chicken and Ghost Scream sound"),
		QString(), "v1.0",
		tr("By Alexander, released under the Creative Commons 'BY' License."),
		"http://www.orangefreesounds.com/"
	);
	moreAboutText += QString("</ul><br><i>%1</i><br>").arg
	(
		tr("The copyright of LameXP as a whole belongs to LoRd_MuldeR. The copyright of third-party software used in LameXP belongs to the individual authors.")
	);

	ui->softwareLabel->setText(NOBREAK(moreAboutText));
	ui->softwareIcon->setPixmap(QIcon(":/images/Logo_Software.png").pixmap(QSize(72,65)));
	connect(ui->softwareLabel, SIGNAL(linkActivated(QString)), this, SLOT(openURL(QString)));
}

void AboutDialog::initLicenseTab(void)
{
	int headerCount = 0;
	QRegExp header("^(\\s*)((?:\\w+\\s+)?GNU GENERAL PUBLIC LICENSE(?:\\s+\\w+)?)(\\s*)$");

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
			if((headerCount < 2) && (header.indexIn(line) >= 0))
			{
				line.replace(header, "\\1<b>\\2</b>\\3");
				++headerCount;
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
		const unsigned int version = lamexp_tools_version(toolBin, &toolTag);
		verStr = lamexp_version2string(toolVerFmt, version, tr("n/a"), toolTag);
	}

	toolText += QString("<li>%1<br>").arg(QString("<b>%1 (%2)</b>").arg(toolName, verStr));
	toolText += QString("%1<br>").arg(toolLicense);
	if(!extraInfo.isEmpty()) toolText += QString("<i>%1</i><br>").arg(extraInfo);
	toolText += LINK(toolWebsite);
	toolText += QString("<font style=\"font-size:9px\"><br>&nbsp;</font>");

	return toolText;
}
