///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2017 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include <QAbstractTableModel>

#include <QString>
#include <QStringList>
#include <QHash>
#include <QSet>
#include <QUuid>
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
		JobWarning = 5,
		JobPerformance = 6,
		JobSkipped = 7
	};
	enum SysMsgType
	{
		SysMsg_Info = 0,
		SysMsg_Performance = 1,
		SysMsg_Warning = 2
	};

	//Model functions
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	//Public functions
	const QStringList &getLogFile(const QModelIndex &index) const;
	const QUuid &getJobId(const QModelIndex &index) const;
	const JobState getJobState(const QModelIndex &index) const;
	const QIcon &getIcon(ProgressModel::JobState state) const;
	void restoreHiddenItems(void);

public slots:
	void addJob(const QUuid &jobId, const QString &jobName, const QString &jobInitialStatus = QString("Initializing..."), int jobInitialState = JobRunning);
	void updateJob(const QUuid &jobId, const QString &newStatus, int newState);
	void appendToLog(const QUuid &jobId, const QString &line);
	void addSystemMessage(const QString &text, int type = SysMsg_Info);

private:
	QList<QUuid> m_jobList;
	QList<QUuid> m_jobListHidden;
	QHash<QUuid, QString> m_jobName;
	QHash<QUuid, QString> m_jobStatus;
	QHash<QUuid, int> m_jobState;
	QHash<QUuid, QStringList> m_jobLogFile;
	QHash<QUuid, int> m_jobIndexCache;
	QSet<QUuid> m_jobIdentifiers;

	const QIcon m_iconRunning;
	const QIcon m_iconPaused;
	const QIcon m_iconComplete;
	const QIcon m_iconFailed;
	const QIcon m_iconSystem;
	const QIcon m_iconWarning;
	const QIcon m_iconPerformance;
	const QIcon m_iconSkipped;
	const QIcon m_iconUndefined;

	const QStringList m_emptyList;
	const QUuid m_emptyUuid;
};
