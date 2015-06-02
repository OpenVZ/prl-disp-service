////////////////////////////////////////////////////////////////////////////////
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
/// @file
///	CDspTaskHelper.h
///
/// @brief
///	Definition of the class CDspTaskHelper
///
/// @brief
///	This class implements long running tasks helper class
///
/// @author sergeyt
///	SergeyM
///
/// @date
///	2006-02-16
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __CDspTaskHelper_H_
#define __CDspTaskHelper_H_

#include <QObject>
#include <QDateTime>
#include <QThread>

#include "CDspClient.h"
#include  "CDspSync.h"

#include "ParallelsDomModel.h"
#include "ParallelsNamespace.h"
#include "XmlModel/VmDirectory/CVmDirectory.h"
#include "XmlModel/Messaging/CVmEvent.h"
#include "Libraries/PrlUuid/Uuid.h"
#include "Libraries/PrlCommonUtilsBase/PrlStringifyConsts.h"

class CVmConfiguration;

#define TASK_TERMINATE_TIMEOUT		7*1000	// 7 seconds

class CancelOperationSupport
{
public:
	/**
		* Cancels running operation
		* @param pointer to the user session object requesting operation cancel
		* @param pointer to the cancel operation request package
		*/
	virtual void cancelOperation(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p);
	virtual bool operationIsCancelled();
	virtual PRL_RESULT getCancelResult() { return PRL_ERR_OPERATION_WAS_CANCELED; }

	CancelOperationSupport();
	virtual ~CancelOperationSupport();

protected:
	/**
	* set to default internal canceled parameters
	*/
	void undoCancelled();

	/**
	* return Canceler User - it pointer may be null in specific case
	*/
	SmartPtr<CDspClient> getCancelerUser(){ return m_pUserCaller; }

	/**
	* complete task setter/getter
	*/
	void setTaskCompleted(){ m_bTaskCompleted = true; }

protected:
	SmartPtr<CDspClient>	m_pUserCaller;

private:
	bool isTaskCompleted(){ return m_bTaskCompleted; }

private:
	bool volatile			m_bOperationCancelled;
	bool volatile			m_bTaskCompleted;
};

class ProgressHelper
{
protected:
	ProgressHelper();
	virtual ~ProgressHelper(){}
	// progress helpers
	void getLastProgressInfo( PRL_UINT32& percent, PRL_UINT64& sendTimeStamp );
	void setLastProgressInfo( PRL_UINT32 percent, PRL_UINT64 sendTimeStamp );

private:
	PRL_UINT32 m_nLastProgressPercent;
	PRL_UINT64 m_uLastProgressTimeStamp;
};

class CDspTaskHelper :
	public QThread,
	public CancelOperationSupport
{
	Q_OBJECT
public:
	virtual ~CDspTaskHelper();

	const Uuid&	getJobUuid() const;

	SmartPtr<CDspClient> getClient();
	SmartPtr<IOPackage> getRequestPackage ();
	PRL_UINT32 getRequestFlags ();

	CVmEvent *getLastError();

	// method to easy access to getLastError()->getEventCode()
	PRL_RESULT	getLastErrorCode();

	// Flag allows to cancel task on client disconnect.
	// NOTE: Some tasks ask questions on cancel and can hangs here ( convertBootcamp )!
	virtual bool shouldBeCanceled_OnClientDisconnect()
		{ return m_uiRequestFlags & PACF_CANCEL_TASK_ON_END_SESSION; }

	//Should be overrided in classes, that support it.
	virtual QString  getVmUuid() {return "";}

	CVmIdent getVmIdent();

	/* Returns task parameters [ task #6009 ]  */
	CDspLockedPointer<CVmEvent> getTaskParameters();

	/* Reassign task to new session [ task #6009 ]  */
	virtual void	reassignTask( SmartPtr<CDspClient>& pNewClient, const SmartPtr<IOPackage>& pNewPackage );

	/** Returns sign whether task questions shoud sending to client during task work */
	bool getForceQuestionsSign() const;

	/**
	* checks is file inside vm directory
	*/
	bool isFileInsideVmHome( const QString& filePath, const QString & strVmHome );

	virtual void cancelOperation(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p);
	virtual PRL_RESULT runExternalTask(CDspTaskHelper *pTask);

public slots:
	void start ( QThread::Priority priority = QThread::InheritPriority );

protected:
	CDspTaskHelper(
					const SmartPtr<CDspClient>&,
					const SmartPtr<IOPackage>&,
					bool bForceQuestionsSign = false,
					// pointer to static flag to provide running state for 'OnceInstanceTask'
					// NOTE: THIS pointer MUST POINT TO STATIC VARIABLE!
					bool* pbIsRunning = 0 );

	//should be overrided
	virtual PRL_RESULT run_body()=0;

protected:

	//method calls before thread started
	virtual PRL_RESULT prepareTask();
	//method calls after before thread started
	virtual void finalizeTask();

	// method to easy access to getLastError()->setEventCode()
	void setLastErrorCode(PRL_RESULT rc);

	// it need to reverse impersonation after lost task reattach
	SmartPtr<CDspClient> getActualClient();

	//Should be overrided in classes, that support it.
	virtual	bool providedAdditionState(){ return false; }

	// check if addition vm state was changed and post event if changed
	void checkVmAdditionState( bool bExcludeThisForSearch = false );

	void setExternalTask(CDspTaskHelper *pTask = NULL);

private:
	virtual void run();
	/**
	 * Processes common commands flags set
	 */
	void parseFlags();

private:

	// Mutex to get/set 'm_pUser'/'m_requestPkg' members.
	//		Note: should be defined before 'm_pUser' and 'm_requestPkg'
	QMutex	m_assignMutex;

	// parameters common for all kinds of job
	SmartPtr<CDspClient> m_pUser;
	SmartPtr<IOPackage> m_requestPkg;
	PRL_UINT32					m_uiRequestFlags;

	// thread uuid
	Uuid m_JobUuid;

	CVmEvent *m_pLastError;

	QMutex		m_mtxTaskParams;
	CVmEvent	m_evtTaskParams;
	/** Sign that specifying whether questions should sending on client side during task progress */
	bool m_bForceQuestionsSign;
	// saved lost task user
	SmartPtr<CDspClient> m_pLostTaskUser;
	bool	m_bTaskLostByClient;

	static QMutex	s_mtxSetTaskFlag;

	bool	m_bExlusiveWasLocked;
	bool*	m_pbIsRunning;

	QMutex m_mtxWaitExternalTask;
	// External task thread
	CDspTaskHelper *m_externalTask;

protected:

	bool lockToExecute();
	void unlockToExecute();
};

///////////////////////////////////////////////////////////////////////////////
// class CDspTaskFuture

template<class T>
struct CDspTaskFuture
{
	CDspTaskFuture()
	{
	}
	explicit CDspTaskFuture(const SmartPtr<CDspTaskHelper>& started_):
		m_task(started_)
	{
	}

	const T* getTask() const
	{
		return dynamic_cast<T* >(m_task.getImpl());
	}
	CDspTaskFuture& stop()
	{
		if (m_task.isValid())
			doStop(dynamic_cast<T& >(*m_task));

		return wait();
	}
	CDspTaskFuture& abort(const SmartPtr<CDspClient>& user_, const SmartPtr<IOPackage>& request_)
	{
		if (m_task.isValid() && !m_task->operationIsCancelled() && !m_task->isFinished())
			m_task->cancelOperation(user_, request_);

		return *this;
	}
	CDspTaskFuture& wait(bool do_ = true)
	{
		if (do_ && m_task.isValid() && m_task->isRunning())
			m_task->wait();

		return *this;
	}
	CDspTaskFuture& getResult(CVmEvent* result_)
	{
		if (NULL == result_)
		{}
		else if (!m_task.isValid())
			result_->setEventCode(PRL_ERR_OPERATION_WAS_CANCELED);
		else if (m_task->isFinished())
			result_->fromString(m_task->getLastError()->toString());
		else if (m_task->operationIsCancelled())
			result_->setEventCode(m_task->getCancelResult());

		return *this;
	}

private:
	static void doStop(T& task_)
	{
		task_.terminate();
	}

	SmartPtr<CDspTaskHelper> m_task;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


#endif // __CDspTaskHelper_H_
