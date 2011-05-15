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

#include <QThread>
#include <QStringList>
#include <QMap>

class AudioFileModel;
class QFile;
class QDir;
class QFileInfo;

////////////////////////////////////////////////////////////
// Splash Thread
////////////////////////////////////////////////////////////

class CueSplitter: public QThread
{
	Q_OBJECT

public:
	CueSplitter(const QString &outputDir, const QString &baseName, const QList<AudioFileModel> &inputFiles);
	~CueSplitter(void);
	void run();
	bool getSuccess(void) { return !isRunning() && m_bSuccess; }
	unsigned int getTracksSuccess(void) { return m_nTracksSuccess; }
	unsigned int getTracksSkipped(void) { return m_nTracksSkipped; }
	void addTrack(const int trackNo, const QString &file, const double offset, const double length, const AudioFileModel &metaInfo);
	void setAlbumInfo(const QString &performer, const QString &title);

signals:
	void fileSelected(const QString &fileName);
	void fileSplit(const AudioFileModel &file);

private slots:
	void handleUpdate(int progress);

private:
	void splitFile(const QString &output, const int trackNo, const QString &file, const double offset, const double length, const AudioFileModel &metaInfo);
	QString indexToString(const double index) const;
	
	const QString m_soxBin;
	const QString m_outputDir;
	const QString m_baseName;
	unsigned int m_nTracksSuccess;
	unsigned int m_nTracksSkipped;
	bool m_bSuccess;
	volatile bool m_abortFlag;

	QString m_albumTitle;
	QString m_albumPerformer;
	QString m_activeFile;
	QMap<QString,AudioFileModel> m_inputFiles;
	QMap<QString,QString> m_decompressedFiles;
	QStringList m_tempFiles;
	QList<QString> m_trackFile;
	QList<int> m_trackNo;
	QList<double> m_trackOffset;
	QList<double> m_trackLength;
	QList<AudioFileModel> m_trackMetaInfo;
};
