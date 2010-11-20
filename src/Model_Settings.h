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

#pragma once

class QSettings;
class QString;

#define MAKE_GETTER_DEC1(OPT) int OPT(void)
#define MAKE_SETTER_DEC1(OPT) void OPT(int value)
#define MAKE_GETTER_DEC2(OPT) QString OPT(void)
#define MAKE_SETTER_DEC2(OPT) void OPT(const QString &value)
#define MAKE_GETTER_DEC3(OPT) bool OPT(void)
#define MAKE_SETTER_DEC3(OPT) void OPT(bool value)

class SettingsModel
{
public:
	SettingsModel(void);
	~SettingsModel(void);

	//Enums
	enum Encoder
	{
		MP3Encoder = 0,
		VorbisEncoder = 1,
		AACEncoder = 2,
		FLACEncoder = 3,
		PCMEncoder = 4
	};
	enum RCMode
	{
		VBRMode = 0,
		ABRMode = 1,
		CBRMode = 2
	};
	
	//Consts
	static const int mp3Bitrates[15];

	//Getters
	MAKE_GETTER_DEC1(licenseAccepted);
	MAKE_GETTER_DEC1(interfaceStyle);
	MAKE_GETTER_DEC1(compressionEncoder);
	MAKE_GETTER_DEC1(compressionRCMode);
	MAKE_GETTER_DEC1(compressionBitrate);
	MAKE_GETTER_DEC2(outputDir);
	MAKE_GETTER_DEC3(writeMetaTags);

	//Setters
	MAKE_SETTER_DEC1(licenseAccepted);
	MAKE_SETTER_DEC1(interfaceStyle);
	MAKE_SETTER_DEC1(compressionBitrate);
	MAKE_SETTER_DEC1(compressionRCMode);
	MAKE_SETTER_DEC1(compressionEncoder);
	MAKE_SETTER_DEC2(outputDir);
	MAKE_SETTER_DEC3(writeMetaTags);

	void validate(void);

private:
	QSettings *m_settings;
};

#undef MAKE_GETTER_DEC1
#undef MAKE_SETTER_DEC1
#undef MAKE_GETTER_DEC2
#undef MAKE_SETTER_DEC2
#undef MAKE_GETTER_DEC3
#undef MAKE_SETTER_DEC3
