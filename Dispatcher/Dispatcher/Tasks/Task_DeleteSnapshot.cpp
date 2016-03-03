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
///	Task_DeleteSnapshot.cpp
///
/// @brief
///	Definition of the class Task_DeleteSnapshot
///
/// @brief
///	This class implements long running tasks helper class
///
////////////////////////////////////////////////////////////////////////////////

#include "Task_DeleteSnapshot.h"
#include "Task_CommonHeaders.h"

#include "CProtoSerializer.h"
#include "CDspVm.h"
#include "CDspClientManager.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include "Libraries/StatesStore/SavedStateStore.h"
#include "Libraries/StatesUtils/StatesHelper.h"
#include "CDspVmSnapshotInfrastructure.h"

using namespace Parallels;

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

namespace {
	PRL_BOOL DiskCallBack(PRL_STATES_CALLBACK_TYPE iCallbackType, PRL_INT32 iProgress, PRL_VOID_PTR pParameter)
	{
		UNUSED_PARAM(iCallbackType);
		LOG_MESSAGE(DBG_DEBUG, "CSSManager::DiskCallBack() iCallbackType [%d] iProgress [%d]", iCallbackType, iProgress);

		Task_DeleteSnapshot *pTask = (Task_DeleteSnapshot *)pParameter;

		if( pTask->operationIsCancelled() )
		{
			WRITE_TRACE(DBG_FATAL, "Task_DeleteSnapshot::DiskCallBack(): operation was canceled!" );
			pTask->setHddErrorCode( PRL_ERR_OPERATION_WAS_CANCELED );
			pTask->wakeTask();
			return PRL_FALSE;
		}


		// Check progress
		if (iProgress < 0)
		{
			WRITE_TRACE(DBG_FATAL, "Error occurred while deleting snapshot with code [%#x][%s]"
				, iProgress, PRL_RESULT_TO_STRING( iProgress ) );

			if (iProgress == PRL_ERR_STATE_NO_STATE)
			{
				pTask->wakeTask();
				return PRL_TRUE;
			}

			pTask->setHddErrorCode( iProgress );
			pTask->wakeTask();
			return PRL_FALSE;
		}

		pTask->PostProgressEvent((uint )(iProgress * 100 / PRL_STATE_PROGRESS_MAX));

		if (iProgress == PRL_STATE_PROGRESS_COMPLETED)
		{
			pTask->wakeTask();
		}
		return PRL_TRUE;
	}
}

Task_DeleteSnapshot::Task_DeleteSnapshot (SmartPtr<CDspClient>& user,
										  const SmartPtr<IOPackage>& p,
										  VIRTUAL_MACHINE_STATE initialVmState) :
	CDspTaskHelper( user, p ),
	m_initialVmState(initialVmState),
	m_flgChild(false),
	m_bVmRunning(false),
	m_bSnapLock(false),
	m_uiProgress((uint )-1),
	m_uiStepsCount(1),
	m_uiStep(0),
	m_bChildrenExist(false)
{
	setLastErrorCode(PRL_ERR_SUCCESS);
	setHddErrorCode(PRL_ERR_SUCCESS);
}

Task_DeleteSnapshot::~Task_DeleteSnapshot()
{
}

PRL_RESULT Task_DeleteSnapshot::prepareTask()
{
	// check params in existing VM
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		if (PRL_FAILED( getLastErrorCode() ))
			throw getLastErrorCode();

		CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmDeleteSnapshot, UTF8_2QSTR(getRequestPackage()->buffers[0].getImpl()));
		if ( ! cmd->IsValid() )
			throw PRL_ERR_FAILURE;

		CProtoDeleteSnapshotCommand *pVmCmd = CProtoSerializer::CastToProtoCommand<CProtoDeleteSnapshotCommand>(cmd);

		QString m_sVmUuid = pVmCmd->GetVmUuid();
		m_SnapshotUuid = pVmCmd->GetSnapshotUuid();
		m_flgChild = pVmCmd->GetChild();
		m_nFlags = pVmCmd->GetCommandFlags();

		m_pVmConfig = CDspService::instance()->getVmDirHelper().getVmConfigByUuid(getClient(), m_sVmUuid, ret);
		if( !m_pVmConfig )
		{
			PRL_ASSERT( PRL_FAILED(ret) );
			if( !PRL_FAILED( ret ) )
				ret = PRL_ERR_FAILURE;
			WRITE_TRACE(DBG_FATAL, "Couldn't to extract VM config for UUID '%s'", QSTR2UTF8(cmd->GetVmUuid()));
			throw ret;
		}

		if( PRL_FAILED( m_pVmConfig->m_uiRcInit ) )
			throw PRL_ERR_PARSE_VM_CONFIG;

		// Find VM
		SmartPtr<CDspVm> pVm = CDspVm::GetVmInstanceByUuid( getVmUuid(), getClient()->getVmDirectoryUuid() );
		if (pVm)
		{
			// TODO: possible need to use other func
			SmartPtr<CDspClient> pUser = pVm->getVmRunner();
			if( pUser )
				m_bVmRunning = true;
			else
				m_bVmRunning = false;
		}

		/* Serialize against locked snapshots */
		if (!(m_nFlags & PDSF_BACKUP)) {
			ret = CDspService::instance()->getVmSnapshotStoreHelper().lockSnapshotList(
				getVmIdent()
				, QStringList(m_SnapshotUuid)
				, PRL_ERR_VM_LOCKED_FOR_DELETE_TO_SNAPSHOT);

			if (PRL_FAILED(ret))
				throw ret;
			m_bSnapLock = true;
		}
		// NOTE: m_SnapshotUuid is exists because lockSnapshotList() returns success;

		m_bChildrenExist = CDspVmSnapshotStoreHelper::hasSnapshotChildren( getVmIdent(), m_SnapshotUuid );
		// LOCK inside brackets
		{
			CDspLockedPointer<CVmDirectoryItem> pVmDirItem
				= CDspService::instance()->getVmDirManager().getVmDirItemByUuid(
				getClient()->getVmDirectoryUuid(),
				getVmUuid());
			m_sVmConfigPath = pVmDirItem->getVmHome();
		}

		ret = PRL_ERR_SUCCESS;
	}
	catch(PRL_RESULT code)
	{
		ret=code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while prepare to delete snapshot with code [%#x][%s]"
			, code, PRL_RESULT_TO_STRING( code ) );
	}

	setLastErrorCode(ret);

	return ret;
}

void Task_DeleteSnapshot::finalizeTask()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;
	try
	{
		if (PRL_FAILED( getLastErrorCode() ))
			throw getLastErrorCode();

		/**
		 * Notify all user: state restored
		 */
		CVmEvent deleted_event( PET_DSP_EVT_VM_STATE_DELETED,
			getVmUuid(),
			PIE_DISPATCHER );

		SmartPtr<IOPackage> deletedPkg = DispatcherPackage::createInstance( PVE::DspVmEvent,
			deleted_event,
			getRequestPackage());

		CDspService::instance()->getClientManager().sendPackageToVmClients( deletedPkg,
			getClient()->getVmDirectoryUuid(), getVmUuid() );

		if (m_pVmConfig->getVmSettings()->getVmRuntimeOptions()->getUndoDisksModeEx() == PUD_DISABLE_UNDO_DISKS)
		{
			CDspVmSnapshotStoreHelper::notifyVmClientsTreeChanged(
				getRequestPackage(), getClient()->getVmIdent( getVmUuid() ) );
		}
		ret = PRL_ERR_SUCCESS;

	} catch (PRL_RESULT code) {
		ret = code; //PRL_ERR_OPERATION_FAILED;

		WRITE_TRACE(DBG_FATAL, "Error occurred while registering configuration with code [%#x][%s]"
			, code, PRL_RESULT_TO_STRING( code ) );
	}

	setLastErrorCode(ret);

	// Find VM
	SmartPtr<CDspVm> pVm = CDspVm::GetVmInstanceByUuid( getVmUuid(), getClient()->getVmDirectoryUuid() );

	/**
	* Restore initial VM status
	*/
	UpdateVmState(pVm);

	if (m_bSnapLock) {
		m_bSnapLock = false;
		CDspService::instance()->getVmSnapshotStoreHelper().unlockSnapshotList(
			getVmIdent()
			, QStringList(m_SnapshotUuid));
	}

	/* Unregister fake DspVM instance */
	if (pVm && ( m_initialVmState == VMS_STOPPED || m_initialVmState == VMS_SUSPENDED))
	{
		CDspVm::UnregisterVmObject(pVm);
		// NUllify pointer to call ~CDspVm.
		// in ~CDspVm we clear safe mode flag end post event about it #426355
		pVm = SmartPtr<CDspVm>();

		// check that ~CDspVm was called!
		if(pVm)
		{
			// This case may be occurred only when Task_DeleteSnapshot was called
			//		( from SmartGuard Task (Task_AutoProtect) )
			// But it doesn't wrong for SmartGuard task.
			WRITE_TRACE( DBG_WARNING, "Destructor of CDspVm didn't called! Case #426355 may be reproduced." );
		}
	}

	// send response
	if ( ! PRL_SUCCEEDED( getLastErrorCode() ) )
		getClient()->sendResponseError( getLastError(), getRequestPackage() );
	else
	{
		CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( getRequestPackage(), getLastErrorCode() );
		getClient()->sendResponse( pCmd, getRequestPackage() );
		getLastError()->setEventCode( PRL_ERR_SUCCESS );
	}
}

PRL_RESULT Task_DeleteSnapshot::deleteSnapshot(const QString& strSnapshotUuid, bool bMerge)
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	if (m_bVmRunning)
		ret = sendPackageToVm(strSnapshotUuid, bMerge);
	else
		ret = switchAndDeleteState(strSnapshotUuid, bMerge, m_nFlags & PDSF_BACKUP);

	m_uiStep++;

	// Remove additional snapshot files and "Snapshots" folder
	if ( PRL_SUCCEEDED(ret) )
	{
		CDspVmSnapshotStoreHelper &snapHelper =
				CDspService::instance()->getVmSnapshotStoreHelper();
		QString strVmFolder = m_pVmConfig->getConfigDirectory();

		snapHelper.deleteSnapshot( m_pVmConfig, strSnapshotUuid, ! bMerge );
		snapHelper.removeStateFiles(strVmFolder, strSnapshotUuid);
	}

    return ret;
}

PRL_RESULT Task_DeleteSnapshot::switchAndDeleteState(const QString& /*strSnapshotUuid*/, bool /*bMerge*/, bool /*bUseOnlyDiskSnapshot*/)
{
	return PRL_ERR_FAILURE;
}

PRL_RESULT Task_DeleteSnapshot::sendPackageToVm(const QString& strSnapshotUuid, bool bMerge)
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;
	SmartPtr<CVmConfiguration> pSavedVmConfig = getSavedVmConfigBySnapshotUuid(strSnapshotUuid, ret);
	if (PRL_FAILED(ret))
		return ret;

	Snapshot::Command::Destroy c(m_sVmUuid, strSnapshotUuid, m_nFlags, m_flgChild);
	c.step(m_uiStep);
	c.merge(bMerge);
	c.steps(m_uiStepsCount);
	c.config(pSavedVmConfig);
	Snapshot::Unit u(getVmUuid(), *this, m_initialVmState);
	return u.destroy(c);
}

PRL_RESULT Task_DeleteSnapshot::run_body()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;
	QString sVmUuid;

	bool flgImpersonated = false;
	try
	{
		PRL_UNDO_DISKS_MODE nUndoDisksMode
					= m_pVmConfig->getVmSettings()->getVmRuntimeOptions()->getUndoDisksModeEx();

		SmartPtr<CDspVm> pVm = CDspVm::GetVmInstanceByUuid( getVmUuid(), getClient()->getVmDirectoryUuid() );

		if (pVm)
		{
			sVmUuid = pVm->getVmUuid();

			if (nUndoDisksMode == PUD_DISABLE_UNDO_DISKS && pVm->isUndoDisksMode())
			{
				nUndoDisksMode = PUD_PROMPT_BEHAVIOUR;
			}

			if (pVm->isNoUndoDisksQuestion() && nUndoDisksMode == PUD_PROMPT_BEHAVIOUR)
			{
				WRITE_TRACE(DBG_FATAL,
					"Undo disks prompt behevior switched to discard because"
					" of VM was not started actually");

				nUndoDisksMode = PUD_REVERSE_CHANGES;
				// Reset undo disks question flag
				pVm->disableNoUndoDisksQuestion();
			}
		}

		if (nUndoDisksMode == PUD_PROMPT_BEHAVIOUR)
		{
			if ( !getForceQuestionsSign() )
			{
				QList<PRL_RESULT> lstChoices;
				lstChoices << PET_ANSWER_COMMIT << PET_ANSWER_REVERT;

				QList<CVmEventParameter*> lstParams;
				lstParams.append(new CVmEventParameter(PVE::String,
					sVmUuid,
					EVT_PARAM_VM_UUID )
					);

				PRL_RESULT nAnswer = getClient()
					->sendQuestionToUser(PET_QUESTION_UNDO_DISKS_MODE, lstChoices, lstParams,getRequestPackage());
				nUndoDisksMode = (nAnswer == PET_ANSWER_COMMIT ? PUD_COMMIT_CHANGES : PUD_REVERSE_CHANGES);
			}
			else
			{
				WRITE_TRACE(DBG_FATAL, "Changes to the disk will be reverted due non interactive client session");
				nUndoDisksMode = PUD_REVERSE_CHANGES;
			}
		}

		// Let current thread impersonate the security context of a logged-on user
		if( ! getClient()->getAuthHelper().Impersonate() )
			throw PRL_ERR_IMPERSONATE_FAILED;
		flgImpersonated = true;

		if (nUndoDisksMode != PUD_DISABLE_UNDO_DISKS)
		{
			ret = switchAndDeleteState(m_SnapshotUuid, nUndoDisksMode == PUD_COMMIT_CHANGES, true);

			/**
			* Set access rights to hard disks folders
			*/
			if (nUndoDisksMode != PUD_COMMIT_CHANGES)
			{
				CDspLockedPointer<CVmDirectoryItem> pVmDirItem = \
					CDspService::instance()->getVmDirManager().getVmDirItemByUuid(
					getClient()->getVmDirectoryUuid(), getVmUuid() );

				PRL_ASSERT( pVmDirItem );
				if( !pVmDirItem )
					throw PRL_ERR_UNEXPECTED;

				// #462940 move from impersonate to correct set permissions
				if( flgImpersonated && !getActualClient()->getAuthHelper().RevertToSelf() )
					WRITE_TRACE( DBG_FATAL, " near setHdd permissions %s", PRL_RESULT_TO_STRING(PRL_ERR_REVERT_IMPERSONATE_FAILED) );
				else
					flgImpersonated = false;

				PRL_RESULT error = CDspVmSnapshotStoreHelper::setDefaultAccessRightsToAllHdd( m_pVmConfig
					, getClient()
					, pVmDirItem.getPtr()
					, false /* NOT break by error */
					);
				if( PRL_FAILED(error) )
				{
					WRITE_TRACE( DBG_WARNING, "Error %s occured on delete snapshot. But continue."
						, PRL_RESULT_TO_STRING(error) );
				}
				// #462940 back to impersonate again
				if( !flgImpersonated && !getClient()->getAuthHelper().Impersonate() )
					WRITE_TRACE( DBG_FATAL, " near setHdd permissions %s", PRL_RESULT_TO_STRING(PRL_ERR_IMPERSONATE_FAILED) );
				else
					flgImpersonated = true;

			}

			if( PRL_FAILED( ret ) )
				throw ret;
		}
		else if (!m_flgChild)
		{
			ret = deleteSnapshot(m_SnapshotUuid, true);

			if( PRL_FAILED( ret ) )
				throw ret;
		}
		else // with Child
		{
			CSavedStateStore ssTree;
			if (! CDspService::instance()->getVmSnapshotStoreHelper().getSnapshotsTree(
				getClient()->getVmIdent(getVmUuid()), &ssTree))
			{
				throw PRL_ERR_VM_DELETE_STATE_FAILED;
			}

			CSavedStateTree *pTree = ssTree.GetSavedStateTree();
			if (!pTree)
				throw PRL_ERR_VM_DELETE_STATE_FAILED;

			CSavedStateTree * pState = pTree->FindByUuid(m_SnapshotUuid);
			if (!pState)
				throw PRL_ERR_VM_DELETE_STATE_FAILED;

			// it node have not you are here child - delete this without merge
			pState = pState->FindYouAreHereNode(pState);
			if( !pState )
			{
				LOG_MESSAGE(DBG_DEBUG, "it node have not you are here child");
				ret = deleteSnapshot(m_SnapshotUuid, false); //DeleteState w/o merge
				if( PRL_FAILED( ret ) )
					throw ret;
			}
			else
			{
				m_uiStepsCount = 0;
				CSavedStateTree* pTmpState = pState;
				do
				{
					// if node has you are here child
					if ( ! pTmpState->GetParent() )
						throw PRL_ERR_VM_DELETE_STATE_FAILED;

					QString strNodeUuid = pTmpState->GetGuid();
					pTmpState = pTmpState->GetParent();

					if ( pTmpState->GetChildCount() > 1 )
					{
						for( int i = 0 ; i < pTmpState->GetChildCount() ; i++ )
						{
							if ( (pTmpState->GetChild(i)->GetGuid() != strNodeUuid) )
								m_uiStepsCount++;
						}
					}

					m_uiStepsCount++;

				} while( pTmpState->GetGuid() != m_SnapshotUuid );

				do
				{
					QString strNodeUuid = pState->GetGuid();
					pState = pState->GetParent();

					//delete without merge
					if( pState->GetChildCount() > 1 )
					{
						for( int i = 0 ; i < pState->GetChildCount() ; i++ )
						{
							QString strCurUuid = pState->GetChild(i)->GetGuid();

							WRITE_TRACE(DBG_DEBUG, "child i == %i!! strNodeUuid == %s state uuid = %s",
								i, QSTR2UTF8(strNodeUuid), QSTR2UTF8(strCurUuid));

							if ( strCurUuid != strNodeUuid )
							{
								ret = deleteSnapshot(strCurUuid, false);
								if( PRL_FAILED( ret ) )
									throw ret;
							}
						}
					}
					//delete with merge
					ret = deleteSnapshot(pState->GetGuid(), true);
					if( PRL_FAILED( ret ) )
						throw ret;

				} while( pState->GetGuid() != m_SnapshotUuid );
			}
		}

		// Terminates the impersonation of a user, return to Dispatcher access rights
		if( ! getActualClient()->getAuthHelper().RevertToSelf() ) // Don't throw because this thread already finished.
			WRITE_TRACE(DBG_FATAL, "%s: error: %s", __FILE__, PRL_RESULT_TO_STRING( PRL_ERR_REVERT_IMPERSONATE_FAILED ) );
		else
			flgImpersonated = false;

		ret = PRL_ERR_SUCCESS;
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		getLastError()->setEventCode( code );
		// Terminates the impersonation of a user, return to Dispatcher access rights
		if( flgImpersonated && ! getActualClient()->getAuthHelper().RevertToSelf() )
			WRITE_TRACE(DBG_FATAL, "%s: error: %s", __FILE__, PRL_RESULT_TO_STRING( PRL_ERR_REVERT_IMPERSONATE_FAILED ) );

		WRITE_TRACE(DBG_FATAL, "Error while delete snapshot with code [%#x][%s]"
			, code, PRL_RESULT_TO_STRING( code ) );
	}
	return ret;
}

void Task_DeleteSnapshot::waitTask()
{
	m_semaphore.acquire();
}

void Task_DeleteSnapshot::wakeTask()
{
	m_semaphore.release();
}

void Task_DeleteSnapshot::setHddErrorCode(PRL_RESULT errCode)
{
	m_hddErrCode = errCode;
}

PRL_RESULT Task_DeleteSnapshot::getHddErrorCode()
{
	return m_hddErrCode;
}

void Task_DeleteSnapshot::UpdateVmState(SmartPtr<CDspVm> &pVm)
{
	PRL_EVENT_TYPE evtType = PET_DSP_EVT_VM_STOPPED;

	switch (m_initialVmState)
	{
	case VMS_RUNNING:	evtType = PET_DSP_EVT_VM_CONTINUED; break;
	case VMS_PAUSED:	evtType = PET_DSP_EVT_VM_PAUSED; break;
	case VMS_STOPPED:	evtType = PET_DSP_EVT_VM_STOPPED; break;
	case VMS_SUSPENDED: evtType = PET_DSP_EVT_VM_SUSPENDED; break;
	default:
		WRITE_TRACE(DBG_FATAL, "m_initialVmState: %d", m_initialVmState );
		break;
	}

	/* Send to all VM client new VM state */
	CVmEvent event( evtType, getVmUuid(), PIE_DISPATCHER );
	SmartPtr<IOPackage> pUpdateVmStatePkg = DispatcherPackage::createInstance( PVE::DspVmEvent,
		event,
		getRequestPackage() );

	if (pVm)
	{
		pVm->changeVmState(pUpdateVmStatePkg);
	}
}

/**
* Notify job caller about progress
*/
void Task_DeleteSnapshot::PostProgressEvent(uint uiProgress)
{
	PRL_ASSERT(m_uiStepsCount);
	if ( ! m_uiStepsCount )
		return;

	uiProgress = (100 * m_uiStep + uiProgress) / m_uiStepsCount;
	if ( m_uiProgress == uiProgress )
		return;
	m_uiProgress = uiProgress;

	// Create event for client
	CVmEvent event(	PET_JOB_DELETE_SNAPSHOT_PROGRESS_CHANGED,
		getVmUuid(),
		PIE_DISPATCHER );

	/**
	* Add event parameters
	*/
	event.addEventParameter(new CVmEventParameter( PVE::UnsignedInt,
		QString::number(uiProgress),
		EVT_PARAM_PROGRESS_CHANGED));

	SmartPtr<IOPackage> p =
		DispatcherPackage::createInstance( PVE::DspVmEvent, event,
		getRequestPackage());

	getClient()->sendPackage( p );
}

QString Task_DeleteSnapshot::getVmUuid()
{
	return m_pVmConfig ? m_pVmConfig->getVmIdentification()->getVmUuid() : QString();
}

SmartPtr<CVmConfiguration> Task_DeleteSnapshot::getSavedVmConfigBySnapshotUuid(const QString& uuid, PRL_RESULT& error)
{
	error = PRL_ERR_SUCCESS;
	/**
	* Try to read and interpret config file for given snapshot
	*/
	QString sSavedVmConfig = CStatesHelper::MakeCfgFileName(CFileHelper::GetFileRoot(m_sVmConfigPath), uuid);
	SmartPtr<CVmConfiguration> pVmConfig( new CVmConfiguration() );

	PRL_RESULT ret = CDspService::instance()->getVmConfigManager().loadConfig(pVmConfig,
		sSavedVmConfig,
		getClient(),
		false );

	if( PRL_FAILED(ret) )
	{
		WRITE_TRACE(DBG_FATAL, "Can't parse VM config file %s. error = %#x( '%s' )"
			, QSTR2UTF8(sSavedVmConfig)
			, ret
			, PRL_RESULT_TO_STRING( ret ) );
		pVmConfig = SmartPtr<CVmConfiguration>();
		error = ret;
	}
	else
	{
		// Load absolutes pathes for device images from current config
		pVmConfig->getVmHardwareList()->RevertDevicesPathToAbsolute(m_pVmConfig->getConfigDirectory());
		// sync VmConfig representation with start command #PSBM-20611
		CDspService::instance()->getVmDirHelper().UpdateHardDiskInformation(pVmConfig);
	}
	return pVmConfig;
}
