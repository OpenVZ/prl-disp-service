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

#include <QCoreApplication>
#include <QTimer>
#include "CDspTaskTrace.h"
#include "CDspTaskManager.h"
#include <prlcommon/HostUtils/HostUtils.h>

#include <prlcommon/Logging/Logging.h>

#include <prlcommon/Std/PrlAssert.h>

/*****************************************************************************/

CDspTaskManager::CDspTaskManager ()
:m_bDeinited(false)
{
	Task::Trace::setup();
}

CDspTaskManager::~CDspTaskManager ()
{
	deinit();
	Task::Trace::raze();
	WRITE_TRACE(DBG_FATAL, "CDspTaskManager was destructed." );
}

void CDspTaskManager::deinit()
{
	QMutexLocker lock( &m_mutex );
	if (m_bDeinited)
		return;

	m_bDeinited = true;

	QList< SmartPtr<CDspTaskHelper> >  taskList = m_tasks.values();

	lock.unlock();

	//////////////////////////////////////////////////////////////////////////
	// Cancel Tasks
	//////////////////////////////////////////////////////////////////////////
	int cancelCount = 0;
	foreach( SmartPtr<CDspTaskHelper> pTask, taskList )
	{
		if( pTask->isRunning() )
		{
			if( !pTask->operationIsCancelled() )
			{
				// TODO: need rework after fix task #5216 [ Need add "CancelOperation" support to dispatcher Tasks. ]
				pTask->cancelOperation(SmartPtr<CDspClient>(), SmartPtr<IOPackage>());
			}
			cancelCount++;
		}
	}

	if( ! cancelCount )
		return;


	//////////////////////////////////////////////////////////////////////////
	// sleep()
	//////////////////////////////////////////////////////////////////////////
	int waitToTerminateInMSec = TASK_TERMINATE_TIMEOUT;
	WRITE_TRACE(DBG_FATAL, "CDspTaskManager::deinit(): %d tasks were canceled. Wait for %d secs"
		, cancelCount
		, waitToTerminateInMSec/1000 );

	int checkInMSec = 200;
	for( int t = waitToTerminateInMSec; t > 0; t-= checkInMSec )
	{
		HostUtils::Sleep( checkInMSec );

		int runningCount = 0;
		foreach( SmartPtr<CDspTaskHelper> pTask, taskList )
		{
			if( !pTask->isRunning() )
				continue;
			runningCount++;
		}

		if( runningCount )
			continue;

		WRITE_TRACE(DBG_FATAL, "All canceled tasks were finished." );
		break;
	}

	// relock before terminate
	lock.relock();

	// update tasks list ( may be added on sleep time )
	taskList = m_tasks.values();
	m_tasks.clear();
	m_tasksToDelete.clear();
	lock.unlock();

	//////////////////////////////////////////////////////////////////////////
	// Terminate tasks
	//////////////////////////////////////////////////////////////////////////

	foreach( SmartPtr<CDspTaskHelper> pTask, taskList )
	{
		if( !pTask->isRunning() )
			continue;

		WRITE_TRACE(DBG_FATAL, "CDspTaskManager::deinit(): BEGIN TERMINATING task with uuid( %s )",
			QSTR2UTF8( pTask->getJobUuid().toString() ) );

		pTask->terminate();
		pTask->wait();

		WRITE_TRACE(DBG_FATAL, "CDspTaskManager::deinit(): END TERMINATING task with uuid( %s )",
			QSTR2UTF8( pTask->getJobUuid().toString() ) );

	}//for
	WRITE_TRACE(DBG_FATAL, "CDspTaskManager was deinited." );

}

SmartPtr<CDspTaskHelper> CDspTaskManager::registerTask ( CDspTaskHelper* task )
{
	PRL_ASSERT(task);
	LOG_MESSAGE(DBG_DEBUG, "Registering task %p", task);

	QMutexLocker locker( &m_mutex );

	if( m_bDeinited )
	{
		WRITE_TRACE(DBG_FATAL, "CDspTaskManager::registerTask: can't register task."
			" Reason: TasksManager is deinited.  Task will be running separately. Task_uuid = %s"
			, QSTR2UTF8( task->getJobUuid().toString() )  );

		// It is wrong case and should be eliminated !
		task->cancelOperation(task->getClient(), task->getRequestPackage());
		return SmartPtr<CDspTaskHelper>(task);
	}

    bool exists = m_tasks.contains( task->getJobUuid() );
	PRL_ASSERT(!exists);
	if( exists )
	{
		WRITE_TRACE(DBG_FATAL, "CDspTaskManager::registerTask: can't register task. Reason: task_uuid = %s already exists."
			, QSTR2UTF8( task->getJobUuid().toString() )  );
		return SmartPtr<CDspTaskHelper>();
	}

	// Handle thread finish
	bool bConnected = QObject::connect( task,
					  SIGNAL(finished()),
					  SLOT(cleanFinishedTasks()),
					  Qt::QueuedConnection );
	PRL_ASSERT(bConnected);
	Q_UNUSED(bConnected);

	m_tasks[ task->getJobUuid() ] = SmartPtr<CDspTaskHelper>(task);

	return m_tasks[ task->getJobUuid() ];
}

bool CDspTaskManager::hasAnyTasks() const
{
	QMutexLocker locker( &m_mutex );
	return (m_tasks.size() > 0);
}

bool CDspTaskManager::cancelTask ( const QString& taskUuid )
{
	QMutexLocker locker( &m_mutex );

    bool exists = m_tasks.contains( taskUuid );
	if ( ! exists )
		return false;

	SmartPtr<CDspTaskHelper> task = m_tasks[taskUuid];

	locker.unlock();

	task->cancelOperation(SmartPtr<CDspClient>(), SmartPtr<IOPackage>());

	return true;
}

SmartPtr<CDspTaskHelper> CDspTaskManager::findTaskByUuid (
    const Uuid& uuid ) const
{
	QMutexLocker locker( &m_mutex );

    if ( m_tasks.contains(uuid) )
		return m_tasks[uuid];

	return SmartPtr<CDspTaskHelper>();
}

QList< SmartPtr<CDspTaskHelper> > CDspTaskManager::getTaskListByPacketUuid( const QString& requestUuid )
{
	QMutexLocker locker( &m_mutex );

	QList< SmartPtr<CDspTaskHelper> > taskList;
	QHashIterator< Uuid, SmartPtr<CDspTaskHelper> >  it(m_tasks);
	while( it.hasNext() )
	{
		SmartPtr<CDspTaskHelper> pTask = it.next().value();
		SmartPtr<IOPackage> pkg = pTask->getRequestPackage();
		if( !pkg )
			continue;
		if( requestUuid == Uuid::toString( pkg->header.uuid ) )
			taskList << pTask;
	}

	return taskList;
}

QList< SmartPtr< CDspTaskHelper > > CDspTaskManager::getTaskListBySession( const QString& sessionUuid )
{
	QList< SmartPtr< CDspTaskHelper > > taskList;
	if( sessionUuid.isEmpty() )
		return taskList;

	QMutexLocker locker( &m_mutex );

	SmartPtr<CDspTaskHelper> pTask;
	QHashIterator< Uuid, SmartPtr<CDspTaskHelper> >  it(m_tasks);
	while( it.hasNext() )
	{
		SmartPtr<CDspTaskHelper> pTask = it.next().value();
		if( pTask->getClient()->getClientHandle() == sessionUuid )
			taskList.append( pTask );
	}

	return taskList;
}

void CDspTaskManager::unregisterTask ( const Uuid &taskUuid)
{
	QMutexLocker locker( &m_mutex );
	SmartPtr<CDspTaskHelper> pTask = m_tasks.take(taskUuid);
	if (pTask.isValid())
	{
		LOG_MESSAGE(DBG_DEBUG, "Unregistering task %p", pTask.getImpl());
		m_tasksToDelete.append(pTask);

		// to call cleanup for not-executed tasks too
		QTimer::singleShot( 500, this, SLOT(cleanFinishedTasks()) );
	}
}

void CDspTaskManager::cleanFinishedTasks()
{
    // Should be the main thread
    PRL_ASSERT(QCoreApplication::instance()->thread() == QThread::currentThread());

	// https://bugzilla.sw.ru/show_bug.cgi?id=484724
	// Tasks should not be deleted under mutex
	QList< SmartPtr<CDspTaskHelper> > tasksToDeleteNow;

	QMutexLocker locker( &m_mutex );

	CDspTaskHelper* taskPtr = qobject_cast<CDspTaskHelper*>( sender() );

	// If sender is not a task, try to remove all finished tasks
	if ( taskPtr ) {

		SmartPtr<CDspTaskHelper> task;
		foreach ( SmartPtr<CDspTaskHelper> smartTask, m_tasks.values() ) {
			if ( smartTask.getImpl() == taskPtr ) {
				task = smartTask;
				break;
			}
		}

		if ( task.isValid() ) {
			// Append task to deletion list
			if( task->isRunning() )
				m_tasksToDelete.append( task );
			else
			{
				tasksToDeleteNow.append( task );
				m_tasks.remove( task->getJobUuid() );
			}
		}
		else
			LOG_MESSAGE(DBG_DEBUG, "Task %p already was freed", taskPtr);

	}

	QList< SmartPtr<CDspTaskHelper> >::Iterator it =
		m_tasksToDelete.begin();
	while ( it != m_tasksToDelete.end() ) {
		SmartPtr<CDspTaskHelper>& task = *it;
		if ( ! task->isRunning() )
		{
			LOG_MESSAGE(DBG_DEBUG, "Destroying task %p", it->getImpl());

			tasksToDeleteNow.append(task);

			// check to remove finished task from m_tasks
			if( m_tasks.contains(task->getJobUuid()) )
				m_tasks.remove( task->getJobUuid() );

			it = m_tasksToDelete.erase(it);
		}
		else
			++it;
	}

	// If we have some tasks to delete, call clean one more time
	if ( m_tasksToDelete.size() )
		QTimer::singleShot( 500, this, SLOT(cleanFinishedTasks()) );

	locker.unlock();

	// Delete stopped finished tasks without mutex
	tasksToDeleteNow.clear();
}

/** get all working tasks*/
QList< SmartPtr< CDspTaskHelper > >
CDspTaskManager::getAllTasks()
{
	QMutexLocker locker( &m_mutex );
	return m_tasks.values();
}

/*****************************************************************************/
