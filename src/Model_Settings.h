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

#define MAKE_GETTER_DEC(OPT) int OPT(void)
#define MAKE_SETTER_DEC(OPT) void OPT(int value)

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
	MAKE_GETTER_DEC(licenseAccepted);
	MAKE_GETTER_DEC(interfaceStyle);
	MAKE_GETTER_DEC(compressionEncoder);
	MAKE_GETTER_DEC(compressionRCMode);
	MAKE_GETTER_DEC(compressionBitrate);


	//Setters
	MAKE_SETTER_DEC(licenseAccepted);
	MAKE_SETTER_DEC(interfaceStyle);
	MAKE_SETTER_DEC(compressionBitrate);
	MAKE_SETTER_DEC(compressionRCMode);
	MAKE_SETTER_DEC(compressionEncoder);

	void validate(void);

private:
	QSettings *m_settings;
};

#undef MAKE_GETTER_DEC
#undef MAKE_SETTER_DEC
