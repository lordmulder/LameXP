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

#include <MMSystem.h>

//Helper macros
#define LINK(URL) QString("<a href=\"%1\">%2</a>").arg(URL).arg(URL)

//Constants
const char *AboutDialog::neroAacUrl = "http://www.nero.com/eng/technologies-aac-codec.html";

//Contributors
static const struct 
{
	char *pcFlag;
	wchar_t *pcLanguage;
	wchar_t *pcName;
	char *pcMail;
}
g_lamexp_contributors[] =
{
	{"en", L"Englisch",   L"LoRd_MuldeR",         "MuldeR2@GMX.de"       },
	{"de", L"Deutsch",    L"LoRd_MuldeR",         "MuldeR2@GMX.de"       },
	{"",   L"",           L"Bodo Thevissen",      "Bodo@thevissen.de"    },
	{"es", L"Español",    L"Rub3nCT",             "Rub3nCT@gmail.com"    },
	{"fr", L"Française",  L"Dodich Informatique", "Dodich@live.fr"       },
	{"it", L"Italiano",   L"Roberto",             "Gulliver_69@libero.it"},
	{"kr", L"한국어",    L"JaeHyung Lee",        "Kolanp@gmail.com"     },
	{"ru", L"Русский",    L"Neonailol",           "Neonailol@gmail.com"  },
	{"uk", L"Українська", L"Arestarh",            "Arestarh@ukr.net"     },
	{NULL, NULL, NULL, NULL}
};

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

AboutDialog::AboutDialog(SettingsModel *settings, QWidget *parent, bool firstStart)
:
	QMessageBox(parent),
	m_settings(settings)
{
	const QString versionStr = QString().sprintf
	(
		"Version %d.%02d %s, Build %d [%s], %s, Qt v%s",
		lamexp_version_major(),
		lamexp_version_minor(),
		lamexp_version_release(),
		lamexp_version_build(),
		lamexp_version_date().toString(Qt::ISODate).toLatin1().constData(),
		lamexp_version_compiler(),
		qVersion()
	);

	QString aboutText;

	aboutText += QString("<h2>%1</h2>").arg(tr("LameXP &minus; Audio Encoder Front-end"));
	aboutText += QString("<b>Copyright (C) 2004-%1 LoRd_MuldeR &lt;MuldeR2@GMX.de&gt;. Some rights reserved.</b><br>").arg(max(lamexp_version_date().year(), QDate::currentDate().year()));
	aboutText += QString("<b>%1</b><br><br>").arg(versionStr);
	aboutText += QString("<nobr>%1</nobr><br>").arg(tr("Please visit %1 for news and updates!").arg(LINK(lamexp_website_url())));
	
	if(LAMEXP_DEBUG)
	{
		int daysLeft = max(QDate::currentDate().daysTo(lamexp_version_expires()), 0);
		aboutText += QString("<hr><nobr><font color=\"crimson\">!!! %3 DEBUG BUILD %3 Expires at: %1 %3 Days left: %2 %3 DEBUG BUILD %3 !!!</font></nobr>").arg(lamexp_version_expires().toString(Qt::ISODate), QString::number(daysLeft), "&minus;&minus;&minus;");
	}
	else if(lamexp_version_demo())
	{
		int daysLeft = max(QDate::currentDate().daysTo(lamexp_version_expires()), 0);
		aboutText += QString("<hr><nobr><font color=\"crimson\">%1</font></nobr>").arg(tr("Note: This demo (pre-release) version of LameXP will expire at %1. Still %2 days left.").arg(lamexp_version_expires().toString(Qt::ISODate), QString::number(daysLeft)));
	}
	
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
	aboutText += "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.</tt></nobr><br>";
	aboutText += "<hr><table><tr>";
	aboutText += "<td valign=\"middle\"><img src=\":/icons/error_big.png\"</td><td>&nbsp;</td>";
	aboutText += QString("<td><font color=\"darkred\">%1</font></td>").arg(tr("Note: LameXP is free software. Do <b>not</b> pay money to obtain or use LameXP! If some third-party website tries to make you pay for downloading LameXP, you should <b>not</b> respond to the offer !!!"));
	aboutText += "</tr></table><hr><br>";
	aboutText += QString("%1<br>").arg(tr("Special thanks go out to \"John33\" from %1 for his continuous support.").arg(LINK("http://www.rarewares.org/")));

	setText(aboutText);
	setIconPixmap(dynamic_cast<QApplication*>(QApplication::instance())->windowIcon().pixmap(QSize(64,64)));
	setWindowTitle(tr("About LameXP"));

	if(firstStart)
	{
		QPushButton *firstButton = addButton(tr("Show License Text"), QMessageBox::AcceptRole);
		firstButton->setIcon(QIcon(":/icons/script.png"));
		firstButton->setIconSize(QSize(16, 16));
		firstButton->setMinimumWidth(135);
		firstButton->disconnect();
		connect(firstButton, SIGNAL(clicked()), this, SLOT(openLicenseText()));

		QPushButton *secondButton = addButton(tr("Accept License"), QMessageBox::AcceptRole);
		secondButton->setIcon(QIcon(":/icons/accept.png"));
		secondButton->setIconSize(QSize(16, 16));
		secondButton->setMinimumWidth(120);

		QPushButton *thirdButton = addButton(tr("Decline License"), QMessageBox::AcceptRole);
		thirdButton->setIcon(QIcon(":/icons/delete.png"));
		thirdButton->setIconSize(QSize(16, 16));
		thirdButton->setMinimumWidth(120);
		thirdButton->setEnabled(false);
	}
	else
	{
		QPushButton *firstButton = addButton(tr("3rd Party S/W"), QMessageBox::AcceptRole);
		firstButton->setIcon(QIcon(":/icons/page_white_cplusplus.png"));
		firstButton->setIconSize(QSize(16, 16));
		firstButton->setMinimumWidth(120);
		firstButton->disconnect();
		connect(firstButton, SIGNAL(clicked()), this, SLOT(showMoreAbout()));

		QPushButton *secondButton = addButton(tr("Contributors"), QMessageBox::AcceptRole);
		secondButton->setIcon(QIcon(":icons/user_suit.png"));
		secondButton->setIconSize(QSize(16, 16));
		secondButton->setMinimumWidth(120);
		secondButton->disconnect();
		connect(secondButton, SIGNAL(clicked()), this, SLOT(showAboutContributors()));

		QPushButton *thirdButton = addButton(tr("About Qt4"), QMessageBox::AcceptRole);
		thirdButton->setIcon(QIcon(":/images/Qt.svg"));
		thirdButton->setIconSize(QSize(16, 16));
		thirdButton->setMinimumWidth(120);
		thirdButton->disconnect();
		connect(thirdButton, SIGNAL(clicked()), this, SLOT(showAboutQt()));

		QPushButton *fourthButton = addButton(tr("Discard"), QMessageBox::AcceptRole);
		fourthButton->setIcon(QIcon(":/icons/cross.png"));
		fourthButton->setIconSize(QSize(16, 16));
		fourthButton->setMinimumWidth(90);
	}

	m_firstShow = firstStart;
}

AboutDialog::~AboutDialog(void)
{
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
	
	switch(QMessageBox::exec())
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

void AboutDialog::enableButtons(void)
{
	const QList<QAbstractButton*> buttonList = buttons();
	
	for(int i = 0; i < buttonList.count(); i++)
	{
		buttonList.at(i)->setEnabled(true);
	}

	setCursor(QCursor(Qt::ArrowCursor));
}

void AboutDialog::openLicenseText(void)
{
	QDesktopServices::openUrl(QUrl("http://www.gnu.org/licenses/gpl-2.0.txt"));
}

void AboutDialog::showAboutQt(void)
{
	QMessageBox::aboutQt(this);
}

void AboutDialog::showAboutContributors(void)
{
	QString contributorsAboutText;

	contributorsAboutText += QString("<h3><nobr>%1</nobr></h3>").arg(tr("The following people have contributed to LameXP:"));
	contributorsAboutText += QString("<b>%1</b>").arg(tr("Translators:"));
	contributorsAboutText += "<table style=\"margin-top:5px\">";
	for(int i = 0; g_lamexp_contributors[i].pcName; i++)
	{
		QString flagIcon = (strlen(g_lamexp_contributors[i].pcFlag) > 0) ? QString("<img src=\":/flags/%1.png\">").arg(g_lamexp_contributors[i].pcFlag) : QString();
		contributorsAboutText += QString("<tr><td valign=\"middle\">%1</td><td>&nbsp;&nbsp;</td>").arg(flagIcon);
		contributorsAboutText += QString("<td valign=\"middle\">%2</td><td>&nbsp;&nbsp;</td>").arg(WCHAR2QSTR(g_lamexp_contributors[i].pcLanguage));
		contributorsAboutText += QString("<td valign=\"middle\">%3</td><td>&nbsp;&nbsp;</td><td>&lt;%4&gt;</td></tr>").arg(WCHAR2QSTR(g_lamexp_contributors[i].pcName), g_lamexp_contributors[i].pcMail);
	}
	contributorsAboutText += "</table>";
	contributorsAboutText += "<br><br>";
	contributorsAboutText += QString("<nobr><i>%1</i></nobr><br>").arg(tr("If you are willing to contribute a LameXP translation, feel free to contact us!"));

	QMessageBox *contributorsAboutBox = new QMessageBox(this);
	contributorsAboutBox->setText(contributorsAboutText);
	contributorsAboutBox->setIconPixmap(dynamic_cast<QApplication*>(QApplication::instance())->windowIcon().pixmap(QSize(64,64)));

	QPushButton *closeButton = contributorsAboutBox->addButton(tr("Discard"), QMessageBox::AcceptRole);
	closeButton->setIcon(QIcon(":/icons/cross.png"));
	closeButton->setMinimumWidth(90);

	contributorsAboutBox->setWindowTitle(tr("About Contributors"));
	contributorsAboutBox->setIconPixmap(QIcon(":/images/Logo_Contributors.png").pixmap(QSize(64,74)));
	contributorsAboutBox->setWindowIcon(QIcon(":/icons/user_suit.png"));
	contributorsAboutBox->exec();
				
	LAMEXP_DELETE(contributorsAboutBox);
}

void AboutDialog::showMoreAbout(void)
{
	QString moreAboutText;

	moreAboutText += QString("<h3>%1</h3>").arg(tr("The following third-party software is used in LameXP:"));
	moreAboutText += "<div style=\"margin-left:-25px;font-size:8pt;white-space:nowrap\"><table><tr><td><ul>";
	
	moreAboutText += makeToolText
	(
		tr("LAME &minus; OpenSource mp3 Encoder"),
		"lame.exe", "v?.??, Beta-?",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://lame.sourceforge.net/"
	);
	moreAboutText += makeToolText
	(
		tr("OggEnc &minus; Ogg Vorbis Encoder"),
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
		tr("FLAC &minus; Free Lossless Audio Codec"),
		"flac.exe", "v?.?.?",
		tr("Open and patent-free lossless audio compression technology."),
		"http://flac.sourceforge.net/"
	);
	moreAboutText += makeToolText
	(
		tr("mpg123 &minus; Fast Console MPEG Audio Player/Decoder"),
		"mpg123.exe", "v?.??.?",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://www.mpg123.de/"
	);
	moreAboutText += makeToolText
	(
		tr("FAAD &minus; OpenSource MPEG-4 and MPEG-2 AAC Decoder"),
		"faad.exe", "v?.?",
		tr("Released under the terms of the GNU General Public License."),
		"http://www.audiocoding.com/"
	);
	moreAboutText += makeToolText
	(
		tr("AC3Filter Tools &minus; AC3/DTS Decoder"),
		"valdec.exe", "v?.??",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://www.ac3filter.net/projects/tools"
	);
	moreAboutText += makeToolText
		(
		tr("WavPack &minus; Hybrid Lossless Compression"),
		"wvunpack.exe", "v?.??.?",
		tr("Completely open audio compression format."),
		"http://www.wavpack.com/"
	);
	moreAboutText += makeToolText
		(
		tr("Musepack &minus; Living Audio Compression"),
		"mpcdec.exe", "r???",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://www.musepack.net/"
	);
	moreAboutText += QString
	(
		"</ul></td><td><ul>"
	);
	moreAboutText += makeToolText
	(
		tr("Monkey's Audio &minus; Lossless Audio Compressor"),
		"mac.exe", "v?.??",
		tr("Freely available source code, simple SDK and non-restrictive licensing."),
		"http://www.monkeysaudio.com/"
	);
	moreAboutText += makeToolText
	(
		tr("Shorten &minus; Lossless Audio Compressor"),
		"shorten.exe", "v?.?.?",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://etree.org/shnutils/shorten/"
	);
	moreAboutText += makeToolText
	(
		tr("Speex &minus; Free Codec For Free Speech"),
		"speexdec.exe", "v?.?",
		tr("Open Source patent-free audio format designed for speech."),
		"http://www.speex.org/"
	);
	moreAboutText += makeToolText
	(
		tr("The True Audio &minus; Lossless Audio Codec"),
		"tta.exe", "v?.?",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://tta.sourceforge.net/"
	);
	moreAboutText += makeToolText
	(
		tr("MediaInfo &minus; Media File Analysis Tool"),
		"mediainfo.exe", "v?.?.??",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://mediainfo.sourceforge.net/"
	);
	moreAboutText += makeToolText
	(
		tr("SoX &minus; Sound eXchange"),
		"sox.exe", "v??.?.?",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://sox.sourceforge.net/"
	);
	moreAboutText += makeToolText
	(
		tr("GnuPG &minus; The GNU Privacy Guard"),
		"gpgv.exe", "v?.?.??",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://www.gnupg.org/"
	);


	moreAboutText += makeToolText
	(
		tr("GNU Wget &minus; Software for retrieving files using HTTP"),
		"wget.exe", "v?.??.?",
		tr("Released under the terms of the GNU Lesser General Public License."),
		"http://www.gnu.org/software/wget/"
	);


	moreAboutText += makeToolText
	(
		tr("Silk Icons &minus; Over 700  icons in PNG format"),
		QString(), "v1.3",
		tr("By Mark James, released under the Creative Commons 'by' License."),
		"http://www.famfamfam.com/lab/icons/silk/"
	);
	moreAboutText += QString("</ul></td><td>&nbsp;</td></tr></table></div><i>%1</i><br>").arg
	(
		tr("LameXP as a whole is copyrighted by LoRd_MuldeR. The copyright of thrird-party software used in LameXP belongs to the individual authors.")
	);

	QMessageBox *moreAboutBox = new QMessageBox(this);
	moreAboutBox->setText(moreAboutText);
	moreAboutBox->setIconPixmap(dynamic_cast<QApplication*>(QApplication::instance())->windowIcon().pixmap(QSize(64,64)));

	QPushButton *closeButton = moreAboutBox->addButton(tr("Discard"), QMessageBox::AcceptRole);
	closeButton->setIcon(QIcon(":/icons/cross.png"));
	closeButton->setMinimumWidth(90);

	moreAboutBox->setWindowTitle(tr("About Third-party Software"));
	moreAboutBox->setIconPixmap(QIcon(":/images/Logo_Software.png").pixmap(QSize(64,71)));
	moreAboutBox->setWindowIcon(QIcon(":/icons/page_white_cplusplus.png"));
	moreAboutBox->exec();
				
	LAMEXP_DELETE(moreAboutBox);
}

////////////////////////////////////////////////////////////
// Protected Functions
////////////////////////////////////////////////////////////

void AboutDialog::showEvent(QShowEvent *e)
{
	QDialog::showEvent(e);
	if(m_firstShow)
	{
		const QList<QAbstractButton*> buttonList = buttons();

		for(int i = 1; i < buttonList.count(); i++)
		{
			buttonList.at(i)->setEnabled(false);
		}

		QTimer::singleShot(5000, this, SLOT(enableButtons()));
		setCursor(QCursor(Qt::WaitCursor));
	}
}

////////////////////////////////////////////////////////////
// Private Functions
////////////////////////////////////////////////////////////

QString AboutDialog::makeToolText(const QString &toolName, const QString &toolBin, const QString &toolVerFmt, const QString &toolLicense, const QString &toolWebsite, const QString &extraInfo)
{
	QString toolText, verStr(toolVerFmt);

	if(!toolBin.isEmpty())
	{
		verStr = lamexp_version2string(toolVerFmt, lamexp_tool_version(toolBin), tr("n/a"));
	}

	toolText += QString("<li><nobr><b>%1 (%2)</b></nobr><br>").arg(toolName, verStr);
	toolText += QString("<nobr>%1</nobr><br>").arg(toolLicense);
	if(!extraInfo.isEmpty()) toolText += QString("<nobr><i>%1</i></nobr><br>").arg(extraInfo);
	toolText += QString("<a href=\"%1\">%1</a>").arg(toolWebsite);
	toolText += QString("<div style=\"font-size:1pt\"><br></div>");

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