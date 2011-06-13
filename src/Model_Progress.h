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

#include <QAbstractTableModel>

#include <QString>
#include <QStringList>
#include <QMap>
#include <QIcon>
#include <QUuid>

class ProgressModel : public QAbstractTableModel
{
	Q_OBJECT

public:
	ProgressModel(void);
	~ProgressModel(void);

	//Enums
	enum JobState
	{
		JobRunning = 0,
		JobPaused = 1,
		JobComplete = 2,
		JobFailed = 3,
		JobSystem = 4,
		JobWarning = 5
	};

	//Model functions
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	//Public functions
	const QStringList &getLogFile(const QModelIndex &index);
	const QUuid &getJobId(const QModelIndex &index);
	void restoreHiddenItems(void);

public slots:
	void addJob(const QUuid &jobId, const QString &jobName, const QString &jobInitialStatus = QString("Initializing..."), int jobInitialState = JobRunning);
	void updateJob(const QUuid &jobId, const QString &newStatus, int newState);
	void appendToLog(const QUuid &jobId, const QString &line);
	void addSystemMessage(const QString &text, bool isWarning = false);

private:
	QList<QUuid> m_jobList;
	QList<QUuid> m_jobListHidden;
	QMap<QUuid, QString> m_jobName;
	QMap<QUuid, QString> m_jobStatus;
	QMap<QUuid, int> m_jobState;
	QMap<QUuid, QStringList> m_jobLogFile;

	const QIcon m_iconRunning;
	const QIcon m_iconPaused;
	const QIcon m_iconComplete;
	const QIcon m_iconFailed;
	const QIcon m_iconSystem;
	const QIcon m_iconWarning;
};
