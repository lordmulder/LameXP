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

#pragma once

#include <QDialog>

class CueSheetModel;
class LockedFile;
class AudioFileModel;
class FileListModel;
class SettingsModel;

//UIC forward declartion
namespace Ui {
	class CueSheetImport;
}

//CueImportDialog class
class CueImportDialog : public QDialog
{
	Q_OBJECT

public:
	CueImportDialog(QWidget *parent, FileListModel *fileList, const QString &cueFile, const SettingsModel *settings);
	~CueImportDialog(void);

	int exec(void);

protected:
	void CueImportDialog::showEvent(QShowEvent *event);

private slots:
	void browseButtonClicked(void);
	void importButtonClicked(void);
	void loadOtherButtonClicked(void);
	void modelChanged(void);
	void analyzedFile(const AudioFileModel &file);

private:
	Ui::CueSheetImport *ui; //for Qt UIC
	
	void importCueSheet(void);
	bool analyzeFiles(QStringList &files);
	void splitFiles(void);

	CueSheetModel *m_model;
	FileListModel *m_fileList;

	const SettingsModel *m_settings;

	QList<LockedFile*> m_locks;
	QList<AudioFileModel> m_fileInfo;
	QString m_cueFileName;
	QString m_outputDir;
};
