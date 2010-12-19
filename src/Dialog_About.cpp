///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2010 LoRd_MuldeR <MuldeR2@GMX.de>
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

//Qt includes
#include <QDate>
#include <QApplication>
#include <QIcon>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>
#include <QFileInfo>
#include <QDir>

//Win32 includes
#include <Windows.h>

//Helper macros
#define LINK(URL) QString("<a href=\"%1\">%2</a>").arg(URL).arg(URL)
#define CONTRIBUTOR(LANG, CNTR, ICON) QString("<tr><td valign=\"middle\"><img src=\"%1\"></td><td>&nbsp;&nbsp;</td><td valign=\"middle\">%2</td><td>&nbsp;&nbsp;</td><td valign=\"middle\">%3</td></tr>").arg(ICON, LANG, CNTR);
#define VSTR(BASE,TOOL,FORMAT) QString(BASE).arg(lamexp_version2string(FORMAT, lamexp_tool_version(TOOL)))

//Constants
const char *AboutDialog::neroAacUrl = "http://www.nero.com/eng/technologies-aac-codec.html";

////////////////////////////////////////////////////////////
// Constructor
////////////////////////////////////////////////////////////

AboutDialog::AboutDialog(SettingsModel *settings, QWidget *parent, bool firstStart)
:
	QMessageBox(parent),
	m_settings(settings)
{
	QString aboutText;

	aboutText += "<h2>LameXP - Audio Encoder Front-end</h2>";
	aboutText += QString("<b>Copyright (C) 2004-%1 LoRd_MuldeR &lt;MuldeR2@GMX.de&gt;. Some rights reserved.</b><br>").arg(max(lamexp_version_date().year(),QDate::currentDate().year()));
	aboutText += QString().sprintf("<b>Version %d.%02d %s, Build %d [%s]</b><br><br>", lamexp_version_major(), lamexp_version_minor(), lamexp_version_release(), lamexp_version_build(), lamexp_version_date().toString(Qt::ISODate).toLatin1().constData());
	aboutText += "<nobr>Please visit "; //the official web-site at
	aboutText += LINK("http://forum.doom9.org/showthread.php?t=157726") += " for news and updates!</nobr><br>"; //LINK("http://mulder.dummwiedeutsch.de/")
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
	if(!firstStart)
	{
		aboutText += QString("<br>Please see %1 for details!</br><br>").arg(LINK("http://www.gnu.org/licenses/gpl-2.0.txt"));
	}
	aboutText += "<hr><br>";
	aboutText += QString("Special thanks go out to \"John33\" from %1 for his continuous support.<br>").arg(LINK("http://www.rarewares.org/"));
	
	setText(aboutText);
	setIconPixmap(dynamic_cast<QApplication*>(QApplication::instance())->windowIcon().pixmap(QSize(64,64)));
	setWindowTitle("About LameXP");
	
	if(firstStart)
	{
		QPushButton *firstButton = addButton("Show License Text", QMessageBox::AcceptRole);
		firstButton->setIcon(QIcon(":/icons/script.png"));
		firstButton->setMinimumWidth(135);
		firstButton->disconnect();
		connect(firstButton, SIGNAL(clicked()), this, SLOT(openLicenseText()));

		QPushButton *secondButton = addButton("Accept License", QMessageBox::AcceptRole);
		secondButton->setIcon(QIcon(":/icons/accept.png"));
		secondButton->setMinimumWidth(120);

		QPushButton *thirdButton = addButton("Decline License", QMessageBox::AcceptRole);
		thirdButton->setIcon(QIcon(":/icons/delete.png"));
		thirdButton->setMinimumWidth(120);
		thirdButton->setEnabled(false);
	}
	else
	{
		QPushButton *firstButton = addButton("3rd Party S/W", QMessageBox::AcceptRole);
		firstButton->setIcon(QIcon(":/icons/page_white_cplusplus.png"));
		firstButton->setMinimumWidth(120);
		firstButton->disconnect();
		connect(firstButton, SIGNAL(clicked()), this, SLOT(showMoreAbout()));

		QPushButton *secondButton = addButton("Contributors", QMessageBox::AcceptRole);
		secondButton->setIcon(QIcon(":icons/user_suit.png"));
		secondButton->setMinimumWidth(120);
		secondButton->disconnect();
		connect(secondButton, SIGNAL(clicked()), this, SLOT(showAboutContributors()));

		QPushButton *thirdButton = addButton("About Qt4", QMessageBox::AcceptRole);
		thirdButton->setIcon(QIcon(":/images/Qt.svg"));
		thirdButton->setMinimumWidth(120);
		thirdButton->disconnect();
		connect(thirdButton, SIGNAL(clicked()), this, SLOT(showAboutQt()));

		QPushButton *fourthButton = addButton("Discard", QMessageBox::AcceptRole);
		fourthButton->setIcon(QIcon(":/icons/cross.png"));
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
		if(!m_firstShow || !playResoureSound("imageres.dll", 5080, true))
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
	contributorsAboutText += "<h3>The following people have contributed to LameXP:</h3>";
	contributorsAboutText += "<b>Translators:</b>";
	contributorsAboutText += "<table style=\"margin-top:5px\">";
	contributorsAboutText += CONTRIBUTOR("Englisch", "LoRd_MuldeR &lt;MuldeR2@GMX.de&gt;", ":/flags/gb.png");
	contributorsAboutText += CONTRIBUTOR("Deutsch", "LoRd_MuldeR &lt;MuldeR2@GMX.de&gt;", ":/flags/de.png");
	contributorsAboutText += "</table>";
	contributorsAboutText += "<br><br>";
	contributorsAboutText += "<i>If you are willing to contribute a LameXP translation, feel free to contact us!</i><br>";

	QMessageBox *contributorsAboutBox = new QMessageBox(this);
	contributorsAboutBox->setText(contributorsAboutText);
	contributorsAboutBox->setIconPixmap(dynamic_cast<QApplication*>(QApplication::instance())->windowIcon().pixmap(QSize(64,64)));

	QPushButton *closeButton = contributorsAboutBox->addButton("Discard", QMessageBox::AcceptRole);
	closeButton->setIcon(QIcon(":/icons/cross.png"));
	closeButton->setMinimumWidth(90);

	contributorsAboutBox->setWindowTitle("About Contributors");
	contributorsAboutBox->setIconPixmap(QIcon(":/images/Logo_Contributors.png").pixmap(QSize(64,74)));
	contributorsAboutBox->setWindowIcon(QIcon(":/icons/user_suit.png"));
	contributorsAboutBox->exec();
				
	LAMEXP_DELETE(contributorsAboutBox);
}

void AboutDialog::showMoreAbout(void)
{
	QString moreAboutText;
	moreAboutText += "<h3>The following third-party software is used in LameXP:</h3>";
	moreAboutText += "<div  style=\"margin-left:-25px;font-size:8pt\"><ul>";
	
	moreAboutText += VSTR( "<li><b>LAME - OpenSource mp3 Encoder (%1)</b><br>", "lame.exe", "v?.?? a??");
	moreAboutText += "Released under the terms of the GNU Leser General Public License.<br>";
	moreAboutText += LINK("http://lame.sourceforge.net/");
	moreAboutText += "<br>";
	
	moreAboutText += VSTR("<li><b>OggEnc - Ogg Vorbis Encoder (%1)</b>", "oggenc2_i386.exe", "v?.??");
	moreAboutText += "<br>Completely open and patent-free audio encoding technology.<br>";
	moreAboutText += LINK("http://www.vorbis.com/");
	moreAboutText += "<br>";
	
	moreAboutText += VSTR("<li><b>Nero AAC reference MPEG-4 Encoder (%1)</b><br>", "neroAacEnc.exe", "v?.?.?.?");
	moreAboutText += "Freeware state-of-the-art HE-AAC encoder with 2-Pass support.<br>";
	moreAboutText += "<i>Available from vendor web-site as free download:</i><br>";
	moreAboutText += LINK(neroAacUrl);
	moreAboutText += "<br>";
	
	moreAboutText += VSTR("<li><b>FLAC - Free Lossless Audio Codec (%1)</b><br>", "flac.exe", "v?.?.?");
	moreAboutText += "Open and patent-free lossless audio compression technology.<br>";
	moreAboutText += LINK("http://flac.sourceforge.net/");
	moreAboutText += "<br>";

	moreAboutText += VSTR("<li><b>AC3Filter Tools - AC3/DTS Decoder (%1)</b><br>", "valdec.exe", "v?.??");
	moreAboutText += "Released under the terms of the GNU Leser General Public License.<br>";
	moreAboutText += LINK("http://www.ac3filter.net/projects/tools");
	moreAboutText += "<br>";

	moreAboutText += VSTR("<li><b>MediaInfo - Media File Analysis Tool (%1)</b><br>", "mediainfo_i386.exe", "v?.?.?");
	moreAboutText += "Released under the terms of the GNU Leser General Public License.<br>";
	moreAboutText += LINK("http://mediainfo.sourceforge.net/");
	moreAboutText += "<br>";

	moreAboutText += VSTR("<li><b>SoX - Sound eXchange (%1)</b><br>", "sox.exe", "v??.?.?");
	moreAboutText += "Released under the terms of the GNU Leser General Public License.<br>";
	moreAboutText += LINK("http://sox.sourceforge.net/");
	moreAboutText += "<br>";
	
	moreAboutText += VSTR("<li><b>GnuPG - The GNU Privacy Guard (%1)</b><br>", "gpgv.exe", "v?.?.??");
	moreAboutText += "Released under the terms of the GNU Leser General Public License.<br>";
	moreAboutText += LINK("http://www.gnupg.org/");
	moreAboutText += "<br>";

	moreAboutText += "<li><b>Silk Icons - Over 700  icons in PNG format (v1.3)</b><br>";
	moreAboutText += "<nobr>By Mark James, released under the Creative Commons 'by' License.</nobr><br>";
	moreAboutText += LINK("http://www.famfamfam.com/lab/icons/silk/");
	moreAboutText += "<br></ul></div>";

	QMessageBox *moreAboutBox = new QMessageBox(this);
	moreAboutBox->setText(moreAboutText);
	moreAboutBox->setIconPixmap(dynamic_cast<QApplication*>(QApplication::instance())->windowIcon().pixmap(QSize(64,64)));

	QPushButton *closeButton = moreAboutBox->addButton("Discard", QMessageBox::AcceptRole);
	closeButton->setIcon(QIcon(":/icons/cross.png"));
	closeButton->setMinimumWidth(90);

	moreAboutBox->setWindowTitle("About Third-party Software");
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