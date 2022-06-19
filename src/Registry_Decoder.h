///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2022 LoRd_MuldeR <MuldeR2@GMX.de>
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

#pragma once

#include <QObject>
#include "Decoder_AAC.h"

class QString;
class QStringList;
class QMutex;
class SettingsModel;

class DecoderRegistry : public QObject
{
	Q_OBJECT

public:
	static void configureDecoders(const SettingsModel *settings);
	static AbstractDecoder *lookup(const QString &containerType, const QString &containerProfile, const QString &formatType, const QString &formatProfile, const QString &formatVersion);
	static const QStringList &getSupportedExts(void);
	static const QStringList &getSupportedTypes(void);

private:
	typedef QList<const AbstractDecoder::supportedType_t*> typeList_t;

	static QMutex m_lock;
	static QScopedPointer<QStringList> m_supportedExts;
	static QScopedPointer<QStringList> m_supportedTypes;
	static QScopedPointer<typeList_t> m_availableTypes;

	static const typeList_t &getAvailableDecoderTypes(void);
};
