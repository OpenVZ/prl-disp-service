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
///  CDspTaskHelper.cpp
///
/// @brief
///  Implementation of the class CDspTaskHelper
///
/// @brief
///  This class implements long running tasks helper class
///
/// @author sergeyt
///  SergeyM
///
/// @date
///  2006-02-16
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////
#include "CDspTaskHelper.h"
#include <prlcommon/Logging/Logging.h>
#include "CProtoSerializer.h"
#include "CProtoCommands.h"

#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/Interfaces/ParallelsQt.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlcommon/Std/SmartPtr.h>

#include "CDspService.h"
#include "CDspVm.h"
#include "CDspVmStateSender.h"
// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

#include <typeinfo>
#ifdef _LIN_
#include <cxxabi.h>
#endif // _LIN_
using namespace Parallels;

CancelOperationSupport::CancelOperationSupport()
:m_bOperationCancelled( false ),
m_bTaskCompleted ( false )
{

}

CancelOperationSupport::~CancelOperationSupport()
{

}

/**
* @brief Cancels operation
*/
void CancelOperationSupport::cancelOperation (SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p)
{
	m_bOperationCancelled = true;
	m_pUserCaller = pUserSession;
	if ( pUserSession )
	{
		if ( isTaskCompleted() )
			pUserSession->sendSimpleResponse(p, PRL_ERR_TASK_NOT_FOUND );
		else
			pUserSession->sendSimpleResponse(p, PRL_ERR_SUCCESS );
	}
}

/**
* @brief Checks if operation os canceled
*/
bool CancelOperationSupport::operationIsCancelled ( )
{
	return m_bOperationCancelled;
}

/**
* @brief Checks if operation os canceled
*/
void CancelOperationSupport::undoCancelled()
{
	m_bOperationCancelled = false;
	m_pUserCaller = SmartPtr<CDspClient>();
}

ProgressHelper::ProgressHelper()
:m_nLastProgressPercent(0),m_uLastProgressTimeStamp(0)
{
}

void ProgressHelper::getLastProgressInfo( PRL_UINT32& percent, PRL_UINT64& sendTimeStamp )
{
	percent = m_nLastProgressPercent;
	sendTimeStamp = m_uLastProgressTimeStamp;
}

void ProgressHelper::setLastProgressInfo( PRL_UINT32 percent, PRL_UINT64 sendTimeStamp )
{
	m_nLastProgressPercent = percent;
	m_uLastProgressTimeStamp = sendTimeStamp;
}


QMutex CDspTaskHelper::s_mtxSetTaskFlag;

CDspTaskHelper::CDspTaskHelper (
	const SmartPtr<CDspClient>& user,
	const SmartPtr<IOPackage>& p,
	bool bForceQuestionsSign,
	bool* pbIsRunning )
:
	m_assignMutex( QMutex::Recursive ),
	m_pUser(user),
	m_requestPkg(p),
	m_uiRequestFlags(0),
	m_JobUuid( Uuid::createUuid() ),
	m_pLastError( new CVmEvent() ),
	m_mtxTaskParams( QMutex::Recursive ),
	m_bForceQuestionsSign(bForceQuestionsSign),
	m_bExlusiveWasLocked(false),
	m_pbIsRunning( pbIsRunning ),
	m_mtxWaitExternalTask(QMutex::Recursive),
	m_externalTask(NULL)
{
	LOG_MESSAGE(DBG_DEBUG, "Task %p instantiated", this);
	PRL_ASSERT( m_pUser );
	PRL_ASSERT( m_requestPkg );
	m_bTaskLostByClient = false;
	setLastErrorCode(PRL_ERR_SUCCESS);

	parseFlags();
}

CDspTaskHelper::~CDspTaskHelper()
{
	LOG_MESSAGE(DBG_DEBUG, "Task %p destroyed", this);
	 delete m_pLastError;
}

/**
 * @brief method executed before thread started
 */
PRL_RESULT CDspTaskHelper::prepareTask()
{
	return PRL_ERR_SUCCESS;
}

/**
 * @brief finalizeTask method implementation
 */
void CDspTaskHelper::finalizeTask()
{
	// send simple response
	if ( PRL_FAILED( getLastErrorCode() ) )
	{
		//https://jira.sw.ru/browse/PDFM-22016
		if ( operationIsCancelled() )
			getClient()->sendSimpleResponse( getRequestPackage(), PRL_ERR_OPERATION_WAS_CANCELED );
		else
			getClient()->sendResponseError( getLastError(), getRequestPackage() );
	}
	else
		getClient()->sendSimpleResponse( getRequestPackage(), getLastErrorCode() );
}

/**
 * @brief override start method
 */
void CDspTaskHelper::start ( QThread::Priority priority)
{
	if (operationIsCancelled())
	{
		WRITE_TRACE(DBG_FATAL, "cannot start already cancelled task of type %s.",
			typeid(*this).name());
		return;
	}
	QThread::setStackSize( 2* 1024 * 1024 );
	QThread::start(priority);
}

// thread runner
void CDspTaskHelper::run()
{
	WRITE_TRACE(DBG_FATAL, "Task '%s' with uuid = %s was started. Flags = %#x"
		, typeid( *this ).name()
		, QSTR2UTF8( getJobUuid().toString() )
		, getRequestFlags()
		);

	bool bExceptionWasCaught = false;
	try
	{
		PRL_RESULT ret = prepareTask();

		if( getLastErrorCode() != ret )
		{
			WRITE_TRACE(DBG_FATAL, "run_body() returns %s (%#x), but getLastError() was inited for error %s (%#x)."
				" taskInfo: [%s, %s]"
				, PRL_RESULT_TO_STRING(ret)
				, ret
				, PRL_RESULT_TO_STRING( getLastErrorCode() )
				, getLastErrorCode()
				, typeid( *this ).name()
				, QSTR2UTF8( getJobUuid().toString() )
				);

			// assert to prevent show messages with %1 %2 instead parameters.
			PRL_ASSERT( getLastErrorCode() == ret );

			setLastErrorCode( ret );
		}

		LOG_MESSAGE( DBG_DEBUG, "prepareTask() was complete [error: %#x, %s]"
			, ret
			, PRL_RESULT_TO_STRING( ret )
			);

		if ( PRL_SUCCEEDED( getLastErrorCode() ) )
		{
			checkVmAdditionState();
			ret = run_body();

			if( getLastErrorCode() != ret )
			{
				WRITE_TRACE(DBG_FATAL, "run_body() returns %s (%#x), but getLastError() was inited for error %s (%#x)."
					" taskInfo: [%s, %s]"
					, PRL_RESULT_TO_STRING(ret)
					, ret
					, PRL_RESULT_TO_STRING( getLastErrorCode() )
					, getLastErrorCode()
					, typeid( *this ).name()
					, QSTR2UTF8( getJobUuid().toString() )
					);

				// second assert to prevent show messages with %1 %2 instead parameters.
				PRL_ASSERT( getLastErrorCode() == ret );

				setLastErrorCode( ret );
			}

			LOG_MESSAGE( DBG_DEBUG, "run_body() was complete [error: %#x, %s]"
				, ret
				, PRL_RESULT_TO_STRING( ret )
				);
		}


		finalizeTask();
		setTaskCompleted();

		LOG_MESSAGE( DBG_DEBUG, "finalizeTask() was complete [error: %#x, %s]"
			, getLastErrorCode()
			, PRL_RESULT_TO_STRING( getLastErrorCode() )
			);
	}
	catch ( std::exception& e )
	{
		bExceptionWasCaught = true;
		WRITE_TRACE(DBG_FATAL, "%s:%d std error was catched: [ %s ] ", __FILE__, __LINE__
			, e.what() );
	}
	catch ( PRL_RESULT err )
	{
		bExceptionWasCaught = true;
		WRITE_TRACE(DBG_FATAL, "%s:%d PRL_RESULT error was catched: [ %#x, %s ] ",  __FILE__, __LINE__,
			err, PRL_RESULT_TO_STRING( err ) );
	}
#ifdef _LIN_
	catch (const abi::__forced_unwind&)
	{
		throw;
	}
#endif // _LIN_
	catch ( ... )
	{
		bExceptionWasCaught = true;
		WRITE_TRACE(DBG_FATAL, "%s:%d UNEXPECTED error was catched",  __FILE__, __LINE__ );
	}

	if( bExceptionWasCaught )
	{
		// #114031
		WRITE_TRACE(DBG_FATAL, "Some exception was uncatched inside task. "
			"Send default response %s to client prevent client app hangs."
			, PRL_RESULT_TO_STRING(PRL_ERR_UNEXPECTED) );

		getClient()->sendSimpleResponse( getRequestPackage(), PRL_ERR_UNEXPECTED );
	}

	WRITE_TRACE(DBG_FATAL, "Task '%s' with uuid = %s was finished with result %s (%#x) ) "
		, typeid( *this ).name()
		, QSTR2UTF8( getJobUuid().toString() )
		, PRL_RESULT_TO_STRING( getLastErrorCode() )
		, getLastErrorCode()
		);

	checkVmAdditionState( true );
}

void CDspTaskHelper::parseFlags()
{
	if ( m_requestPkg.getImpl() )
	{
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(m_requestPkg);

		m_uiRequestFlags = pCmd->GetCommandFlags();

		//Process common non interactive mode flag
		if ( m_uiRequestFlags & PACF_NON_INTERACTIVE_MODE )
			m_bForceQuestionsSign = true;
	}
}

/**
 * @brief Returns task job uuid
 */
const Uuid& CDspTaskHelper::getJobUuid () const
{
	return m_JobUuid;
}

/**
 * @brief Returns task request uuid
 */
SmartPtr<IOPackage> CDspTaskHelper::getRequestPackage ()
{
	QMutexLocker lock( &m_assignMutex );
	return m_requestPkg;
}

PRL_UINT32 CDspTaskHelper::getRequestFlags ()
{
	QMutexLocker lock( &m_assignMutex );
	return m_uiRequestFlags;
}

/**
 * @brief Returns task user
 */
SmartPtr<CDspClient> CDspTaskHelper::getClient ()
{
	QMutexLocker lock( &m_assignMutex );
	return m_pUser;
}

CVmIdent CDspTaskHelper::getVmIdent()
{
	SmartPtr<CDspClient> pUser = getClient();
	PRL_ASSERT(pUser);
	if( !pUser )
		return CVmIdent();
	return pUser->getVmIdent(getVmUuid());
}

void	CDspTaskHelper::reassignTask( SmartPtr<CDspClient>& pNewClient, const SmartPtr<IOPackage>& pNewPackage )
{
	PRL_ASSERT( pNewClient );
	PRL_ASSERT( pNewPackage );
	if ( ! pNewClient || ! pNewPackage )
		return;

	QMutexLocker lock( &m_assignMutex );

	IOPackage::Type old_cmd = getRequestPackage()->header.type;

	WRITE_TRACE(DBG_FATAL, "Task with uuid = %s would be reassigned: "
		"session[ %s => %s ]"
		", packet [%#x => %#x]  "
		, QSTR2UTF8( getJobUuid().toString() )
		, QSTR2UTF8( getClient()->getClientHandle() )
		, QSTR2UTF8( pNewClient->getClientHandle() )
		, getRequestPackage()->header.type
		, pNewPackage->header.type
		);

	if(!m_bTaskLostByClient)
	{
		m_bTaskLostByClient = true;
		m_pLostTaskUser = m_pUser;
	}
	m_pUser = pNewClient;
	m_requestPkg = IOPackage::duplicateInstance( pNewPackage );
	m_requestPkg->header.type = old_cmd;

	parseFlags();

}


/**
 * @brief Return last error
 */
CVmEvent *CDspTaskHelper::getLastError()
{
	PRL_ASSERT( m_pLastError );
	return m_pLastError;
}

CDspLockedPointer<CVmEvent> CDspTaskHelper::getTaskParameters()
{
	return CDspLockedPointer<CVmEvent> ( &m_mtxTaskParams, &m_evtTaskParams );
}

PRL_RESULT	CDspTaskHelper::getLastErrorCode()
{
	return getLastError()->getEventCode();
}

void CDspTaskHelper::setLastErrorCode(PRL_RESULT rc)
{
	getLastError()->setEventCode( rc );
}

SmartPtr<CDspClient> CDspTaskHelper::getActualClient()
{
	if (m_bTaskLostByClient)
		return m_pLostTaskUser;
	else
		return m_pUser;

}

bool CDspTaskHelper::isFileInsideVmHome( const QString& filePath, const QString & strVmHome )
{
	if ( strVmHome.isEmpty() || filePath.isEmpty() )
		return false;

	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	QString strVmHomeDir = QFileInfo(strVmHome).dir().path();

	strVmHomeDir= QDir::fromNativeSeparators( strVmHomeDir);


	Qt::CaseSensitivity howCompare = Qt::CaseSensitive;
#           ifdef _WIN_
	howCompare = Qt::CaseInsensitive;
#           endif

	if( filePath.startsWith ( strVmHomeDir, howCompare ) )
		return true;

	return false;
}

bool CDspTaskHelper::lockToExecute()
{
	if ( ! m_pbIsRunning )
		return false;

	QMutexLocker locker( &s_mtxSetTaskFlag );

	bool bRes = false;

	if ( ! *m_pbIsRunning )
	{
		*m_pbIsRunning = true;
		m_bExlusiveWasLocked = true;
		bRes = true;
	}

	return bRes;
}

void CDspTaskHelper::unlockToExecute()
{
	if ( ! m_pbIsRunning )
		return;

	QMutexLocker locker( &s_mtxSetTaskFlag );

	if( m_bExlusiveWasLocked )
		*m_pbIsRunning = false;
}

// check if addition vm state was changed and post event if changed
void CDspTaskHelper::checkVmAdditionState( bool bExcludeThisForSearch )
{
	if( !providedAdditionState() || !getClient() )
		return;

	if( getVmUuid().isEmpty() )
		return;

	VIRTUAL_MACHINE_ADDITION_STATE state =
				CDspVm::getVmAdditionState( getVmUuid(), getClient()->getVmDirectoryUuid(),
											bExcludeThisForSearch ? this : NULL );

	CDspLockedPointer<CDspVmStateSender>
		pLockedVmStateSender = CDspService::instance()->getVmStateSender();

	if( pLockedVmStateSender )
		pLockedVmStateSender->onVmAdditionStateChanged( state,
														getVmUuid(),
														getClient()->getVmDirectoryUuid() );
}

void CDspTaskHelper::setExternalTask(CDspTaskHelper *pTask)
{
	QMutexLocker locker(&m_mtxWaitExternalTask);
	if (pTask)
		PRL_ASSERT(!m_externalTask);
	m_externalTask = pTask;
}

void CDspTaskHelper::cancelOperation(SmartPtr<CDspClient> pUserSession, const SmartPtr<IOPackage> &p)
{
	CancelOperationSupport::cancelOperation(pUserSession, p);
	QMutexLocker locker(&m_mtxWaitExternalTask);
	if (m_externalTask)
		m_externalTask->cancelOperation(pUserSession, p);
}

PRL_RESULT CDspTaskHelper::runExternalTask(CDspTaskHelper *pTask)
{
	setExternalTask(pTask);
	pTask->start();
	pTask->wait();
	setExternalTask();
	return pTask->getLastError()->getEventCode();
}

///////////////////////////////////////////////////////////////////////////////
// struct CDspTaskFailure

CDspTaskFailure& CDspTaskFailure::setCode(PRL_RESULT code_)
{
	m_code = code_;
	return *this;
}

CDspTaskFailure& CDspTaskFailure::setToken(const QString& token_)
{
	getError().addEventParameter(
		new CVmEventParameter(PVE::String, token_, EVT_PARAM_RETURN_PARAM_TOKEN));
	return *this;
}

PRL_RESULT CDspTaskFailure::operator()()
{
	PRL_RESULT output = m_code;
	getError().setEventCode(output);
	m_code = PRL_ERR_SUCCESS;
	return output;
}

PRL_RESULT CDspTaskFailure::operator()(const QString& first_)
{
	getError().addEventParameter(
		new CVmEventParameter(PVE::String, first_, EVT_PARAM_MESSAGE_PARAM_0));
	return operator()();
}

PRL_RESULT CDspTaskFailure::operator()(const QString& first_, const QString& second_)
{
	getError().addEventParameter(
		new CVmEventParameter(PVE::String, first_, EVT_PARAM_MESSAGE_PARAM_0));
	getError().addEventParameter(
		new CVmEventParameter(PVE::String, second_, EVT_PARAM_MESSAGE_PARAM_1));
	return operator()();
}

PRL_RESULT CDspTaskFailure::operator()(const CVmEvent& src_)
{
	getError().fromString(src_.toString());
	return operator()(src_.getEventCode());
}

CVmEvent& CDspTaskFailure::getError() const
{
	return *(m_task->getLastError());
}

