///////////////////////////////////////////////////////////////////////////////
// LameXP - Audio Encoder Front-End
// Copyright (C) 2004-2016 LoRd_MuldeR <MuldeR2@GMX.de>
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

#include "Model_Progress.h"

#include <QUuid>

#define MAX_DISPLAY_ITEMS 48

ProgressModel::ProgressModel(void)
:
	m_iconRunning(":/icons/media_play.png"),
	m_iconPaused(":/icons/control_pause_blue.png"),
	m_iconComplete(":/icons/tick.png"),
	m_iconFailed(":/icons/exclamation.png"),
	m_iconSystem(":/icons/computer.png"),
	m_iconWarning(":/icons/error.png"),
	m_iconPerformance(":/icons/clock.png"),
	m_iconSkipped(":/icons/step_over.png"),
	m_iconUndefined(":/icons/report.png"),
	m_emptyUuid(0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),
	m_emptyList("Oups, no data available!")
{
}

ProgressModel::~ProgressModel(void)
{
}

int ProgressModel::columnCount(const QModelIndex &parent) const
{
	return 2;
}

int ProgressModel::rowCount(const QModelIndex &parent) const
{
	return m_jobList.count();
}

QVariant ProgressModel::data(const QModelIndex &index, int role) const
{
	if(index.row() >= 0 && index.row() < m_jobList.count())
	{
		if(role == Qt::DisplayRole)
		{
			switch(index.column())
			{
			case 0:
				return m_jobName.value(m_jobList.at(index.row()));
				break;
			case 1:
				return m_jobStatus.value(m_jobList.at(index.row()));
				break;
			default:
				return QVariant();
				break;
			}
		}
		else if(role == Qt::DecorationRole && index.column() == 0)
		{
			const int currentState = m_jobState.value(m_jobList.at(index.row()));
			return getIcon(static_cast<const JobState>(currentState));
		}
		else if(role == Qt::TextAlignmentRole)
		{
			return (index.column() == 1) ? QVariant(Qt::AlignHCenter | Qt::AlignVCenter) : QVariant();
		}
	}
	
	return QVariant();
}

QVariant ProgressModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role == Qt::DisplayRole)
	{
		if(orientation == Qt::Horizontal)
		{
			switch(section)
			{
			case 0:
				return tr("Job");
				break;
			case 1:
				return tr("Status");
				break;
			default:
				return QVariant();
				break;
			}
		}
		if(orientation == Qt::Vertical)
		{
			return QString::number(section + 1);
		}
	}

	return QVariant();
}

void ProgressModel::addJob(const QUuid &jobId, const QString &jobName, const QString &jobInitialStatus, int jobInitialState)
{
	if(m_jobIdentifiers.contains(jobId))
	{
		return;
	}

	while(m_jobList.count() >= MAX_DISPLAY_ITEMS)
	{
		beginRemoveRows(QModelIndex(), 0, 0);
		m_jobListHidden.append(m_jobList.takeFirst());
		m_jobIndexCache.clear();
		endRemoveRows();
	}

	int newIndex = m_jobList.count();
	beginInsertRows(QModelIndex(), newIndex, newIndex);

	m_jobList.append(jobId);
	m_jobName.insert(jobId, jobName);
	m_jobStatus.insert(jobId, jobInitialStatus);
	m_jobState.insert(jobId, jobInitialState);
	m_jobLogFile.insert(jobId, QStringList());
	m_jobIdentifiers.insert(jobId);
	
	endInsertRows();
}

void ProgressModel::updateJob(const QUuid &jobId, const QString &newStatus, int newState)
{
	if(!m_jobIdentifiers.contains(jobId))
	{
		return;
	}
	
	if(!newStatus.isEmpty()) m_jobStatus.insert(jobId, newStatus);
	if(newState >= 0) m_jobState.insert(jobId, newState);

	const int row = m_jobIndexCache.value(jobId, -1);

	if(row >= 0)
	{
		emit dataChanged(index(row, 0), index(row, 1));
	}
	else
	{
		const int tmp = m_jobList.indexOf(jobId);
		if(tmp >= 0)
		{
			m_jobIndexCache.insert(jobId, tmp);
			emit dataChanged(index(tmp, 0), index(tmp, 1));
		}
	}
}

void ProgressModel::appendToLog(const QUuid &jobId, const QString &line)
{
	if(m_jobIdentifiers.contains(jobId))
	{
		m_jobLogFile[jobId].append(line.split('\n'));
	}
}

const QStringList &ProgressModel::getLogFile(const QModelIndex &index) const
{
	if(index.row() < m_jobList.count())
	{
		QUuid id = m_jobList.at(index.row());
		QHash<QUuid,QStringList>::const_iterator iter = m_jobLogFile.constFind(id);
		if(iter != m_jobLogFile.constEnd()) { return iter.value(); }
	}

	return m_emptyList;
}

const QUuid &ProgressModel::getJobId(const QModelIndex &index) const
{
	if(index.row() < m_jobList.count())
	{
		return m_jobList.at(index.row());
	}

	return m_emptyUuid;
}

const ProgressModel::JobState ProgressModel::getJobState(const QModelIndex &index) const
{
	if(index.row() < m_jobList.count())
	{
		return static_cast<JobState>(m_jobState.value(m_jobList.at(index.row()), -1));
	}

	return static_cast<JobState>(-1);
}

void ProgressModel::addSystemMessage(const QString &text, int type)
{
	const QUuid &jobId = QUuid::createUuid();

	if(m_jobIdentifiers.contains(jobId))
	{
		return;
	}

	while(m_jobList.count() >= MAX_DISPLAY_ITEMS)
	{
		beginRemoveRows(QModelIndex(), 0, 0);
		m_jobListHidden.append(m_jobList.takeFirst());
		m_jobIndexCache.clear();
		endRemoveRows();
	}

	int newIndex = m_jobList.count();
	JobState jobState = JobState(-1);

	switch(type)
	{
	case SysMsg_Warning:
		jobState = JobWarning;
		break;
	case SysMsg_Performance:
		jobState = JobPerformance;
		break;
	default:
		jobState = JobSystem;
		break;
	}

	beginInsertRows(QModelIndex(), newIndex, newIndex);

	m_jobList.append(jobId);
	m_jobName.insert(jobId, text);
	m_jobStatus.insert(jobId, QString());
	m_jobState.insert(jobId, jobState);
	m_jobLogFile.insert(jobId, QStringList());
	m_jobIdentifiers.insert(jobId);
	
	endInsertRows();
}

void ProgressModel::restoreHiddenItems(void)
{
	if(!m_jobListHidden.isEmpty())
	{
		beginResetModel();
		while(!m_jobListHidden.isEmpty())
		{
			m_jobList.prepend(m_jobListHidden.takeLast());
		}
		m_jobIndexCache.clear();
		endResetModel();
	}
}

const QIcon &ProgressModel::getIcon(ProgressModel::JobState state) const
{
	switch(state)
	{
	case JobRunning:
		return m_iconRunning;
		break;
	case JobPaused:
		return m_iconPaused;
		break;
	case JobComplete:
		return m_iconComplete;
		break;
	case JobSystem:
		return m_iconSystem;
		break;
	case JobWarning:
		return m_iconWarning;
		break;
	case JobPerformance:
		return m_iconPerformance;
		break;
	case JobSkipped:
		return m_iconSkipped;
		break;
	default:
		return (state < 0) ? m_iconUndefined : m_iconFailed;
		break;
	}
}
