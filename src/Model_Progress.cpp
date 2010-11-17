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

#include "Model_Progress.h"

ProgressModel::ProgressModel(void) :
	m_iconRunning(":/icons/media_play.png"),
	m_iconPaused(":/icons/control_pause_blue.png"),
	m_iconComplete(":/icons/tick.png"),
	m_iconFailed(":/icons/exclamation.png")
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
			switch(m_jobState.value(m_jobList.at(index.row())))
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
			default:
				return m_iconFailed;
				break;
			}
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
				return "Job";
				break;
			case 1:
				return "Status";
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

void ProgressModel::addJob(const QString &jobId, const QString &jobName, const QString &jobInitialStatus, int jobInitialState)
{
	if(m_jobList.contains(jobId, Qt::CaseInsensitive))
	{
		return;
	}

	QString id = jobId.toLower();

	beginResetModel();
	m_jobList.append(id);
	m_jobName.insert(id, jobName);
	m_jobStatus.insert(id, jobInitialStatus);
	m_jobState.insert(id, jobInitialState);
	endResetModel();
}

void ProgressModel::updateJob(const QString &jobId, const QString &newStatus, int newState)
{
	if(!m_jobList.contains(jobId, Qt::CaseInsensitive))
	{
		return;
	}
	
	QString id = jobId.toLower();
	
	beginResetModel();
	if(!newStatus.isEmpty()) m_jobStatus.insert(id, newStatus);
	if(newState >= 0) m_jobState.insert(id, newState);
	endResetModel();
}
