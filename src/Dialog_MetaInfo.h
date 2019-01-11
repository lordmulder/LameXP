///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2019 LoRd_MuldeR <MuldeR2@GMX.de>
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

#pragma once

#include <QDialog>
#include "UIC_MetaInfo.h"

#include "Model_AudioFile.h"

class MetaInfoDialog : public QDialog, private Ui::MetaInfo
{
	Q_OBJECT

public:
	MetaInfoDialog(QWidget *parent);
	~MetaInfoDialog(void);

	int exec(AudioFileModel &audioFile,  bool allowUp, bool allowDown);

private slots:
	void upButtonClicked(void);
	void downButtonClicked(void);
	void editButtonClicked(void);
	void infoContextMenuRequested(const QPoint &pos);
	void artworkContextMenuRequested(const QPoint &pos);
	void copyMetaInfoActionTriggered(void);
	void clearMetaInfoActionTriggered(void);
	void clearArtworkActionTriggered(void);

protected:
	bool eventFilter(QObject *obj, QEvent *event);

private:
	QMenu *m_contextMenuInfo;
	QMenu *m_contextMenuArtwork;
};
