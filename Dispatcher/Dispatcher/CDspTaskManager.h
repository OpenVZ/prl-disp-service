///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspTaskManager
///
/// Dispatcher task manager
///
/// @author romanp
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
///
/// This file is part of Virtuozzo Core. Virtuozzo Core is free
/// software; you can redistribute it and/or modify it under the terms
/// of the GNU General Public License as published by the Free Software
/// Foundation; either version 2 of the License, or (at your option) any
/// later version.
/// 
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
/// 
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
/// 02110-1301, USA.
///
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#ifndef CDSPTASKMANAGER_H
#define CDSPTASKMANAGER_H

#include <QHash>
#include <QList>
#include <QMutex>

#include "CDspTaskHelper.h"
#include <prlxmlmodel/Messaging/CVmEvent.h>

class CDspTaskManager : public QObject
{
	Q_OBJECT
public:

	CDspTaskManager ();
	~CDspTaskManager ();

	void deinit();

	/** Registers new task */
	SmartPtr<CDspTaskHelper> registerTask ( CDspTaskHelper* );

	/**
	 * Cancels task
	 * @return true if task is found and canceled,
	 *         false otherwise
	 */
	bool cancelTask ( const QString& taskUuid);

	/**
	 * Unregisteries task by specified task UUID
	 */
	void unregisterTask ( const Uuid& taskUuid);

	/** Finds task by task_uuid */
	SmartPtr<CDspTaskHelper> findTaskByUuid ( const Uuid& ) const;

	/** Find tasks by packet_uuid */
	QList< SmartPtr<CDspTaskHelper> > getTaskListByPacketUuid( const QString& requestUuid );

	/** Find tasks by session uuid */
	QList< SmartPtr< CDspTaskHelper > > getTaskListBySession( const QString& sessionUuid );

	/** Returns sign whether any active tasks present at tasks list */
	bool hasAnyTasks() const;

	/** get all working tasks*/
	QList< SmartPtr< CDspTaskHelper > > getAllTasks();

	template<class T>
	CDspTaskFuture<T> schedule(T* task_)
	{
		SmartPtr<CDspTaskHelper> p = registerTask(task_);
		if (!p.isValid())
			return CDspTaskFuture<T>();

		p->start();
		return CDspTaskFuture<T>(p);
	}

public slots:
	void cleanFinishedTasks();

private:
	bool m_bDeinited;
	QHash< Uuid, SmartPtr<CDspTaskHelper> > m_tasks;
	QList< SmartPtr<CDspTaskHelper> > m_tasksToDelete;
	mutable QMutex m_mutex;
};

#endif //CDSPTASKMANAGER_H
