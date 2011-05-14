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

#pragma once

#include <QDialog>
#include "../tmp/UIC_CueSheetImport.h"

#include "Model_AudioFile.h"

class CueSheetModel;
class LockedFile;
class FileListModel;

class CueImportDialog : public QDialog, private Ui::CueSheetImport
{
	Q_OBJECT

public:
	CueImportDialog(QWidget *parent, FileListModel *fileList, const QString &cueFile);
	~CueImportDialog(void);

	int exec(void);

protected:
	void CueImportDialog::showEvent(QShowEvent *event);

private slots:
	void browseButtonClicked(void);
	void importButtonClicked(void);
	void modelChanged(void);
	void analyzedFile(const AudioFileModel &file);

private:
	void importCueSheet(void);
	void analyzeFiles(QStringList &files);
	void CueImportDialog::splitFiles(void);

	CueSheetModel *m_model;
	FileListModel *m_fileList;

	QList<LockedFile*> m_locks;
	QList<AudioFileModel> m_fileInfo;
	QString m_cueFileName;
	QString m_outputDir;
};
