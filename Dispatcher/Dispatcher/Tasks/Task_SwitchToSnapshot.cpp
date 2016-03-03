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
///	Task_SwitchToSnapshot.cpp
///
/// @brief
///	Definition of the class Task_SwitchToSnapshot
///
/// @brief
///	This class implements long running tasks helper class
///
/// @author sergeyt
///	Ilya@
///
////////////////////////////////////////////////////////////////////////////////

#include "Task_SwitchToSnapshot.h"
#include "Task_CommonHeaders.h"

#include "CProtoSerializer.h"
#include "CDspClientManager.h"
#include "CDspVmInfoDatabase.h"
#include "CVmValidateConfig.h"
#include "CDspBugPatcherLogic.h"
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include "Libraries/StatesUtils/StatesHelper.h"
#include "Libraries/StatesStore/SavedStateStore.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/PrlCommonUtils/CFirewallHelper.h"


#ifdef _LIN_
#include "Libraries/Virtuozzo/CCpuHelper.h"
#endif //_LIN_

using namespace Parallels;

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

namespace {
	PRL_BOOL DiskCallBack(PRL_STATES_CALLBACK_TYPE iCallbackType, PRL_INT32 iProgress, PRL_VOID_PTR pParameter)
	{
		UNUSED_PARAM(iCallbackType);
		LOG_MESSAGE(DBG_DEBUG, "Task_SwitchToSnapshot::DiskCallBack() iCallbackType [%d] iProgress [%d]", iCallbackType, iProgress);

		Task_SwitchToSnapshot *pTask = (Task_SwitchToSnapshot *)pParameter;

		if( pTask->operationIsCancelled() )
		{
			WRITE_TRACE(DBG_FATAL, "Task_SwitchToSnapshot::DiskCallBack(): operation was canceled!" );
			pTask->setHddErrorCode( PRL_ERR_OPERATION_WAS_CANCELED );
			pTask->wakeTask();
			return PRL_FALSE;
		}

		// Check progress
		if (iProgress < 0)
		{
			WRITE_TRACE(DBG_FATAL, "Error occurred while revert to snapshot with code [%#x][%s]"
				, iProgress, PRL_RESULT_TO_STRING( iProgress ) );

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

bool Task_SwitchToSnapshot::isSnapshotedVmRunning(void)
{
	switch (m_snapshotedVmState) {
	case PVE::SnapshotedVmRunning:
	case PVE::SnapshotedVmPaused:
		return true;
	case PVE::SnapshotedVmPoweredOff:
	case PVE::SnapshotedVmSuspended:
		return false;
	/* default:
	 * all states should be handled by switch
	 */
	}
	return false;
}

Task_SwitchToSnapshot::Task_SwitchToSnapshot (SmartPtr<CDspClient>& user,
											  const SmartPtr<IOPackage>& p,
											  VIRTUAL_MACHINE_STATE initialVmState) :

	CDspTaskHelper( user, p ),
	m_pSavedVmConfig(new CVmConfiguration()),
	m_initialVmState(initialVmState),
// VirtualDisk commented out by request from CP team
//	m_pStatesManager( new CDSManager() ),
	m_bVmStartedByTask(false),
	m_bVmConnectionWasClosed(false),
	m_bTaskWasFinished(false),
	m_uiProgress((uint )-1)
{
	setLastErrorCode(PRL_ERR_SUCCESS);
	setHddErrorCode(PRL_ERR_SUCCESS);
}

Task_SwitchToSnapshot::~Task_SwitchToSnapshot()
{
	releaseDiskManager();
}

QString Task_SwitchToSnapshot::getVmUuid()
{
	return m_pVmConfig ? m_pVmConfig->getVmIdentification()->getVmUuid() : QString();
}

PRL_RESULT Task_SwitchToSnapshot::prepareTask()
{
	// check params in existing VM
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		////////////////////////////////////////////////////////////////////////
		// retrieve user parameters from request data
		////////////////////////////////////////////////////////////////////////

		CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmSwitchToSnapshot, UTF8_2QSTR(getRequestPackage()->buffers[0].getImpl()));
		if ( ! cmd->IsValid() )
		{
			throw (PRL_ERR_FAILURE);
		}

		CProtoSwitchToSnapshotCommand *pVmCmd = CProtoSerializer::CastToProtoCommand<CProtoSwitchToSnapshotCommand>(cmd);
		QString strVmUuid = pVmCmd->GetVmUuid();
		m_SnapshotUuid = pVmCmd->GetSnapshotUuid();

		m_snapshotedVmState = CDspService::instance()->getVmSnapshotStoreHelper().
			getSnapshotedVmState(getClient(), getRequestPackage());

		/* leave VM in stopped state if PSSF_SKIP_RESUME flag is specified */
		if ((pVmCmd->GetCommandFlags() & PSSF_SKIP_RESUME) && isSnapshotedVmRunning())
			m_snapshotedVmState = PVE::SnapshotedVmPoweredOff;

		// LOCK inside brackets
		CDspLockedPointer<CVmDirectoryItem> pVmDirItem =  CDspService::instance()->getVmDirManager().getVmDirItemByUuid(
			getClient()->getVmDirectoryUuid(), strVmUuid );
		m_VmConfigPath = pVmDirItem->getVmHome();
		pVmDirItem.unlock();

		PRL_RESULT ret = PRL_ERR_SUCCESS;
		m_pVmConfig = CDspService::instance()->getVmDirHelper().getVmConfigByUuid(getClient(), strVmUuid, ret);
		if( !m_pVmConfig )
		{
			PRL_ASSERT( PRL_FAILED(ret) );
			if( !PRL_FAILED( ret ) )
				ret = PRL_ERR_FAILURE;

			WRITE_TRACE(DBG_FATAL, "Couldn't to extract VM config for UUID '%s'", QSTR2UTF8(strVmUuid));
			throw (ret);
		}

		if ( PRL_FAILED( getLastErrorCode() ) )
			throw getLastErrorCode();

		if( PRL_FAILED( m_pVmConfig->m_uiRcInit ) )
			throw PRL_ERR_PARSE_VM_CONFIG;

		m_pVmInfo = CDspVmInfoDatabase::readVmInfo( ParallelsDirs::getVmInfoPath(
													CFileHelper::GetFileRoot( m_VmConfigPath ) ) );

		PRL_RESULT error = IsHasRightsForSwitchToSnapshot( m_pVmConfig, *getLastError() );
		if( PRL_FAILED( error ) )
		{
			WRITE_TRACE(DBG_FATAL, ">>> User hasn't rights to access this VM %#x,%s", error, PRL_RESULT_TO_STRING( error ) );
			throw error;
		}

		ret = PRL_ERR_SUCCESS;
	}
	catch(PRL_RESULT code)
	{
		ret = code;
		getLastError()->setEventCode( code );
		WRITE_TRACE(DBG_FATAL, "Error occurred while prepare to revert to snapshot with code [%#x][%s]"
			, code, PRL_RESULT_TO_STRING( code ) );
	}

	return ret;
}

void Task_SwitchToSnapshot::finalizeTask()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		if (PRL_FAILED( getLastErrorCode() ))
			throw getLastErrorCode();

		QString userName;
		{
			CDspLockedPointer< CDispUser > pLockedDispUser = CDspService::instance()->getDispConfigGuard()
					.getDispUserByUuid( getClient()->getUserSettingsUuid() );
			if( pLockedDispUser )
				userName = pLockedDispUser->getUserName();
		}

		/**
		* VM restoring from stopped to running state, to continue switching to snapshot,
		* we need start VM
		*/
		if ( (m_initialVmState == VMS_STOPPED || m_initialVmState == VMS_SUSPENDED) &&
				isSnapshotedVmRunning())
		{
			PRL_RESULT outCreateError = PRL_ERR_SUCCESS;
			bool bNew = false;
			SmartPtr<CDspVm> pVm =
				CDspVm::CreateInstance(getVmUuid(), getClient()->getVmDirectoryUuid(), outCreateError, bNew, getClient());

			if (PRL_FAILED( outCreateError ))
				throw outCreateError;

			if ( pVm )
			{
				/**
				* Add Job Uuid parameter into VM start package
				*/
				std::auto_ptr<Snapshot::Revert::Command> c(makeRevertPackage());
				if (!c.get())
					throw PRL_ERR_FAILURE;
				SmartPtr<IOPackage> p = c->do_();
				if (NULL == p.getImpl())
					throw PRL_ERR_FAILURE;

				// there is no need to commit unfinished ops here
				// because the prepareTask has already done it.
//				if( !pVm->startSnapshotedVm( getClient(), p ) )
//				{
					// TODO: to investigate:
					// Why we unregister in anycase  notwithstanding to bNew flag ?
					// It may be because nNew is always false
					// ( when we start this task from VmManager by DspCmdVmSwitchToSnapshot command )

//					CDspVm::UnregisterVmObject(pVm);
//					throw PRL_ERR_VM_RESTORE_STATE_FAILED;
//				}
				m_bVmStartedByTask = true;

				// Wait while VM starting and reverting to snapshot
				waitTask();

				if ( PRL_FAILED( getLastErrorCode() ) )
					throw getLastErrorCode();
			}
		}

		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

		/**
		* save Snapshots.xml file
		*/
		CDspService::instance()->getVmSnapshotStoreHelper().switchToSnapshot( getClient(), getRequestPackage() );

		/**
		* Notify all user: snapshot tree changed
		*/

		// Generate "snapshots tree changed" event
		CDspVmSnapshotStoreHelper::notifyVmClientsTreeChanged(
				getRequestPackage(), getClient()->getVmIdent(
				m_pVmConfig->getVmIdentification()->getVmUuid() ));

		/**
		* Notify all user: state restored
		*/
		CVmEvent restored_event( PET_DSP_EVT_VM_RESTORED,
			m_pVmConfig->getVmIdentification()->getVmUuid(),
			PIE_DISPATCHER );

		SmartPtr<IOPackage> restoredPkg = DispatcherPackage::createInstance( PVE::DspVmEvent,
			restored_event,
			getRequestPackage());

		CDspService::instance()->getClientManager().sendPackageToVmClients( restoredPkg,
			getClient()->getVmDirectoryUuid(), m_pVmConfig->getVmIdentification()->getVmUuid() );

		/**
		 * In case of switch to stopped state emulate swapping phase finished situation
		 * see https://bugzilla.sw.ru/show_bug.cgi?id=471867 for more details
		 */
		if ( !isSnapshotedVmRunning() )
		{
			CVmEvent _swap_phase_finished_event( PET_DSP_EVT_VM_MEMORY_SWAPPING_FINISHED,
				m_pVmConfig->getVmIdentification()->getVmUuid(),
				PIE_VIRTUAL_MACHINE );

			SmartPtr<IOPackage> swapFinishedPkg = DispatcherPackage::createInstance( PVE::DspVmEvent,
				_swap_phase_finished_event,
				getRequestPackage() );

			CDspService::instance()->getClientManager().sendPackageToVmClients( swapFinishedPkg,
				getClient()->getVmDirectoryUuid(), m_pVmConfig->getVmIdentification()->getVmUuid() );
		}

		ret = changeFirewallSettings();
		if (PRL_FAILED(ret))
			throw ret;

		ret = PRL_ERR_SUCCESS;

	} catch (PRL_RESULT code) {
		ret = code; //PRL_ERR_OPERATION_FAILED;
		WRITE_TRACE(DBG_FATAL, "Error occurred while registering configuration with code [%#x][%s]"
			, code, PRL_RESULT_TO_STRING( code ) );
	}

	setLastErrorCode(ret);

	/**
	* Update VM status
	*/
	SmartPtr<CDspVm> pVm = CDspVm::GetVmInstanceByUuid( getVmUuid(), getClient()->getVmDirectoryUuid() );
	if (pVm)
	{
		UpdateVmState(pVm);
	}
	else
	if (m_note.isVmStopped())
	{
		// If VM object destroyed via VM_STOP request, just recreate VM instance and update state
		PRL_RESULT outCreateError = PRL_ERR_SUCCESS;
		// FIXME!!! It needs to check bNew value (#123497)
		bool bNew = false;
		pVm = CDspVm::CreateInstance(getVmUuid(), getClient()->getVmDirectoryUuid(), outCreateError, bNew, getClient() );
		if(pVm)
		{
			UpdateVmState(pVm);
		}
	}
	pVm.reset();

	// send response
	if ( PRL_FAILED( getLastErrorCode() ) )
	{
		getClient()->sendResponseError( getLastError(), getRequestPackage() );
		/**
		* Rollback VM config
		*/
		if( m_pVmConfig && PRL_SUCCEEDED( m_pVmConfig->m_uiRcInit ) )
		{
			CDspService::instance()->getVmConfigManager().saveConfig(m_pVmConfig,
				m_VmConfigPath,
				getClient(),
				true,
				true);
			NotifyConfigChanged();
		}

		if ( m_pVmInfo )
		{
			// Don't look at result because it's rollback (we can't rollback rollback now)
			CDspVmInfoDatabase::writeVmInfo( ParallelsDirs::getVmInfoPath(
											 CFileHelper::GetFileRoot( m_VmConfigPath ) ), m_pVmInfo );
		}
	}
	else
	{
		CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand( getRequestPackage(),
			getLastErrorCode() );
		getClient()->sendResponse( pCmd, getRequestPackage() );
	}
}


PRL_RESULT Task_SwitchToSnapshot::run_body()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	bool flgImpersonated = false;
	try
	{
		// Let current thread impersonate the security context of a logged-on user
		if( ! getClient()->getAuthHelper().Impersonate() )
		{
			getLastError()->setEventCode(PRL_ERR_IMPERSONATE_FAILED);
			throw PRL_ERR_IMPERSONATE_FAILED;
		}
		flgImpersonated = true;

		/**
		* Check available size for revert operation
		*/
		ret = CheckAvailableSize(m_VmConfigPath);

		if(PRL_FAILED(ret))
			throw ret;

		/**
		* Restore VM configuration
		*/
		ret = RestoreVmConfig(m_VmConfigPath);

		if (PRL_FAILED(ret))
			throw (ret);

		bool bResVmInfo = CDspVmInfoDatabase::restoreVmInfoFromSnapshot(
										CFileHelper::GetFileRoot( m_VmConfigPath ), m_SnapshotUuid );

		if ( !bResVmInfo )
			throw (PRL_ERR_VM_RESTORE_STATE_FAILED);

		{
		CDspLockedPointer<CVmDirectoryItem> pVmDirItem =
				CDspService::instance()->getVmDirManager().getVmDirItemByUuid(
					getClient()->getVmDirectoryUuid(), getVmUuid() );
		CDspService::instance()->getVmSnapshotStoreHelper().SetDefaultAccessRights(
			    ParallelsDirs::getVmInfoPath( CFileHelper::GetFileRoot( m_VmConfigPath ) )
				, getClient()
				, pVmDirItem.getPtr() );
		pVmDirItem->setTemplate( m_pSavedVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate() );
		}

		// Notify users that VM configuration was changed
		NotifyConfigChanged();

		/**
		* Begin offline revert
		*/
		if ( (m_initialVmState != VMS_RUNNING) && (m_initialVmState != VMS_PAUSED) )
		{
			PRL_RESULT ret = offlineRevertToSnapshot( flgImpersonated );

			if (PRL_FAILED(ret))
				throw (ret);
		}

		if ( m_initialVmState == VMS_RUNNING || m_initialVmState == VMS_PAUSED )
		{
			if ( !isSnapshotedVmRunning() )
			{
				/**
				* Stop VM and offline revert
				*/
				PRL_RESULT ret = sendPackageToVm( makeStopPackage() );
				if (PRL_FAILED(ret))
					throw (ret);

				m_note.setVmStopped();

				ret = offlineRevertToSnapshot( flgImpersonated );
				if (PRL_FAILED(ret))
					throw (ret);

			}
			else
			{
				/**
				 * Validate saved VM config
				 */
				CVmValidateConfig vc(m_pSavedVmConfig);
				vc.CheckVmConfig(PVC_ALL, getClient());

				if (vc.HasError(PRL_ERR_VMCONF_GENERIC_PCI_DEVICE_DUPLICATE_IN_ANOTHER_VM)
					&& vc.GetParameter(PRL_ERR_VMCONF_GENERIC_PCI_DEVICE_DUPLICATE_IN_ANOTHER_VM, 2).toInt() != 0)
				{
					WRITE_TRACE(DBG_FATAL, "Vm config contains Vt-d devices are used by others VM's !" );

					getLastError()->addEventParameter(
						new CVmEventParameter(
									PVE::String,
									vc.GetParameter(PRL_ERR_VMCONF_GENERIC_PCI_DEVICE_DUPLICATE_IN_ANOTHER_VM, 1),
									EVT_PARAM_MESSAGE_PARAM_0 ));
					throw PRL_ERR_CANT_REVERT_VM_SINCE_VTD_DEVICE_ALREADY_USED;
				}

				/**
				* Send "Switch to snapshot" command to VM and waiting for response
				*/
				std::auto_ptr<Snapshot::Revert::Command> c(makeRevertPackage());
				Snapshot::Unit u(getVmUuid(), *this, m_initialVmState);
				PRL_RESULT ret = u.revert(m_note, *c);
				if (PRL_FAILED(ret))
					throw (ret);
			}
		}

		if( PRL_FAILED( getHddErrorCode() ) )
			throw getHddErrorCode();
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

		WRITE_TRACE(DBG_FATAL, "Error while reverting to snapshot with code [%#x][%s]"
			, code, PRL_RESULT_TO_STRING( code ) );
	}

	return ret;
}

PRL_RESULT Task_SwitchToSnapshot::IsHasRightsForSwitchToSnapshot(
	SmartPtr<CVmConfiguration> pVmConfig,
	CVmEvent& evtOutError)
{
	CAuthHelper rootAuth; // used for check file exists.

	/**
	* check if user is authorized to access this VM
	*/

	QString vm_uuid = pVmConfig->getVmIdentification()->getVmUuid();

	CDspAccessManager::VmAccessRights
		permissionToVm = CDspService::instance()->getAccessManager().getAccessRightsToVm( getClient() , vm_uuid );
	if ( ! permissionToVm.canRead() )
	{
		PRL_RESULT err = permissionToVm.isExists()
			? PRL_ERR_ACCESS_TO_VM_DENIED
			: PRL_ERR_VM_CONFIG_DOESNT_EXIST;

		CDspLockedPointer<CVmDirectoryItem> item =
			CDspService::instance()->getVmDirHelper().getVmDirectoryItemByUuid(getClient(), vm_uuid);

		getLastError()->setEventCode( err );
		if (item)
		{
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, item->getVmName(), EVT_PARAM_MESSAGE_PARAM_0 ) );
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, item->getVmHome(), EVT_PARAM_MESSAGE_PARAM_1 ) );
		}

		WRITE_TRACE(DBG_FATAL, "Client hasn't rights to read vm to switch snapshot. err: %#x, %s", err, PRL_RESULT_TO_STRING( err ) );
		return err;
	}

	/**
	 * get VM hardware list pointer
	 */

	CVmHardware* p_VmHardware = pVmConfig->getVmHardwareList();
	PRL_ASSERT( p_VmHardware );
	if( !p_VmHardware )
		return false;

	/**
	 * process hard disk images
	 */

	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	for( int iNdex = 0; iNdex < p_VmHardware->m_lstHardDisks.size(); iNdex++ )
	{
		CVmHardDisk* pDevice = p_VmHardware->m_lstHardDisks.at( iNdex );
		if( ( pDevice->getEmulatedType() == PVE::HardDiskImage ) )
		{
			if (!CFileHelper::FileCanRead(pDevice->getSystemName(), &getClient()->getAuthHelper()))
			{
				if ( CFileHelper::DirectoryExists(pDevice->getSystemName(), &rootAuth ) )
				{
					evtOutError.setEventCode( PRL_ERR_ACCESS_TO_VM_HDD_DENIED );
					evtOutError.addEventParameter(
						new CVmEventParameter( PVE::String,
							pVmConfig->getVmIdentification()->getVmName(), EVT_PARAM_MESSAGE_PARAM_0));
					evtOutError.addEventParameter(
						new CVmEventParameter( PVE::String,
							pDevice->getSystemName(), EVT_PARAM_MESSAGE_PARAM_1));
					return evtOutError.getEventCode();
				}
			}
		}
	}


	return PRL_ERR_SUCCESS;
}

void Task_SwitchToSnapshot::waitTask()
{
	m_semaphore.acquire();
}

void Task_SwitchToSnapshot::wakeTask()
{
	m_semaphore.release();
}

void Task_SwitchToSnapshot::setHddErrorCode(PRL_RESULT errCode)
{
	m_hddErrCode = errCode;
}

PRL_RESULT Task_SwitchToSnapshot::getHddErrorCode()
{
	return m_hddErrCode;
}

/**
* Revert suspended state files
*/
bool Task_SwitchToSnapshot::RestoreSuspendedVmFiles(const QString &strVmConfigPath)
{
	// Get directory path where store configuration file
	QString dir_path = CFileHelper::GetFileRoot(strVmConfigPath);
	QString dest_mem_file;

	QString src_mem_file = dir_path + "/Snapshots/" + m_SnapshotUuid + ".mem";
	QString src_sav_file = dir_path + "/Snapshots/" + m_SnapshotUuid + ".sav";
	QString src_png_file = dir_path + "/Snapshots/" + m_SnapshotUuid + ".png";

	// Get configuration file name
	QString base_name = QFileInfo(strVmConfigPath).completeBaseName();
	// Generate full file names for .mem and .sav files
	CStatesHelper sh(src_sav_file);
	// Generate new memory file name (see bug 266903)
	QString mem_file = Uuid::createUuid().toString() + ".mem";

	if (sh.extractMemFilePath( src_sav_file, dest_mem_file))
		dest_mem_file = dest_mem_file + "/" + mem_file;
	else
		dest_mem_file = dir_path + "/" + mem_file;

	QString dest_sav_file = dir_path + "/" + base_name + ".sav";
	QString dest_png_file = dir_path + "/" + PRL_VM_SUSPENDED_SCREEN_FILE_NAME;

	CStatesHelper::CopyStateFile(src_png_file, dest_png_file);
	bool bRet = (CStatesHelper::CopyStateFile(src_sav_file, dest_sav_file) &&
				 CStatesHelper::CopyStateFile(src_mem_file, dest_mem_file) );

	if (bRet)
		// Rewrite memory file name to avoid https://bugzilla.sw.ru/show_bug.cgi?id=266903
		CStatesHelper::writeMemFileName(dest_sav_file, mem_file);

	/**
	* Set access rights to snapshot files
	*/
	{
		CDspLockedPointer<CVmDirectoryItem> pVmDirItem = \
			CDspService::instance()->getVmDirManager().getVmDirItemByUuid(
			getClient()->getVmDirectoryUuid(), getVmUuid() );

		PRL_ASSERT( pVmDirItem );
		if( !pVmDirItem )
			return false;

		CDspService::instance()->getVmSnapshotStoreHelper().SetDefaultAccessRights(
			dest_mem_file
			, getClient()
			, pVmDirItem.getPtr() );
		CDspService::instance()->getVmSnapshotStoreHelper().SetDefaultAccessRights(
			dest_sav_file
			, getClient()
			, pVmDirItem.getPtr() );
		CDspService::instance()->getVmSnapshotStoreHelper().SetDefaultAccessRights(
			dest_png_file
			, getClient()
			, pVmDirItem.getPtr() );
	}

	return bRet;
}

/**
* Drop suspended state files
*/
void Task_SwitchToSnapshot::DropSuspendedVmFiles(const QString &strVmConfigPath)
{
	CStatesHelper sh(strVmConfigPath);
	QFile::remove(sh.getSavFileName());
}

bool Task_SwitchToSnapshot::RevertDiskToSnapshot(const QString& strVmConfigPath)
{
	// Remove later
	(void)strVmConfigPath;

// VirtualDisk commented out by request from CP team
//	/**
//	* Restore disk state
//	*/
//
//	PRL_RESULT ret = CDspVmSnapshotStoreHelper::PrepareDiskStateManager(m_pSavedVmConfig, m_pStatesManager);
//	if (PRL_FAILED(ret))
//		return false;
//
//	ret = m_pStatesManager->SwitchToState(
//		Uuid(m_SnapshotUuid),
//		&DiskCallBack,
//		this);
//
//	return PRL_SUCCEEDED(ret);
	return false;
}

Snapshot::Revert::Command* Task_SwitchToSnapshot::makeRevertPackage()
{
	m_pSavedVmConfig->getVmIdentification()->setHomePath( m_VmConfigPath );
	if (CDspService::isServerModePSBM())
	{
#ifdef _LIN_
		// set CPU features mask (https://jira.sw.ru/browse/PSBM-11171)
		if (!CCpuHelper::update(*m_pSavedVmConfig))
			return NULL;
#endif
	}

	return new Snapshot::Revert::Command(m_SnapshotUuid, *this, m_pVmConfig, m_pSavedVmConfig);
}

SmartPtr<IOPackage> Task_SwitchToSnapshot::makeStopPackage()
{
	/**
	* Prepare VM Stop package
	*/
	CProtoCommandPtr pCmd = CProtoSerializer::CreateProtoVmCommandStop(getVmUuid(), PSM_KILL, 0);
	return DispatcherPackage::createInstance( PVE::DspCmdVmStop, pCmd );
}

PRL_RESULT Task_SwitchToSnapshot::sendPackageToVm(const SmartPtr<IOPackage> &pRequestPkg)
{

	/**
	* Send package to VM and waiting for response
	*/
	SmartPtr<CDspVm> pVm = CDspVm::GetVmInstanceByUuid( getVmUuid(), getClient()->getVmDirectoryUuid() );
	if (!pVm)
		return PRL_ERR_FAILURE;
	IOSendJob::Handle hJob = pVm->sendPackageToVmEx( pRequestPkg, m_initialVmState );
	IOSendJob::Result nResult = CDspService::instance()->getIOServer().waitForResponse( hJob );

	if (nResult != IOSendJob::Success)
		return PRL_ERR_VM_RESTORE_STATE_FAILED;

	Snapshot::Revert::Answer a(m_note);
	PRL_RESULT e = a(hJob);
	if (PRL_FAILED(e) || m_note.isVmStopped())
		return e;

	return a.command()->GetRetCode();
}

void Task_SwitchToSnapshot::UpdateVmState(SmartPtr<CDspVm> &pVm)
{
	VIRTUAL_MACHINE_STATE newState = ( (m_note.isUnexpected() || m_note.isVmStopped()) ?
		VMS_STOPPED : m_initialVmState );
	PRL_EVENT_TYPE evtType = PET_DSP_EVT_VM_STOPPED;

	// Handle Running -> Stopped case for PVA_POSTSTOP action script
	if (m_note.isVmStopped())
		pVm->runActionScript(PVA_POSTSTOP, pVm, true);

	if ( PRL_SUCCEEDED( getLastErrorCode() ) )
	{
		switch(m_snapshotedVmState)
		{
		case PVE::SnapshotedVmRunning: newState = VMS_RUNNING; break;
		case PVE::SnapshotedVmPaused: newState = VMS_PAUSED; break;
		case PVE::SnapshotedVmSuspended: newState = VMS_SUSPENDED; break;
		case PVE::SnapshotedVmPoweredOff: newState = VMS_STOPPED; break;
		default:
			WRITE_TRACE(DBG_FATAL, "stateAfterSwitch: %d", m_snapshotedVmState );
			break;
		}
		//https://bugzilla.sw.ru/show_bug.cgi?id=446383
		if ( isSnapshotedVmRunning() &&
			( VMS_STOPPED == m_initialVmState || VMS_SUSPENDED == m_initialVmState ) )
		{
			pVm->replaceInitDspCmd( PVE::DspCmdVmStart, getClient() );
		}
	}

	switch (newState)
	{
	case VMS_RUNNING:	evtType = PET_DSP_EVT_VM_STARTED; break;
	case VMS_PAUSED:	evtType = PET_DSP_EVT_VM_PAUSED; break;
	case VMS_STOPPED:	evtType = PET_DSP_EVT_VM_STOPPED; break;
	case VMS_SUSPENDED:	evtType = PET_DSP_EVT_VM_SUSPENDED; break;
	default:
		WRITE_TRACE(DBG_FATAL, "newState: %d", newState );
		break;
	}

	/* Send to all VM client new VM state */
	CVmEvent event( evtType, getVmUuid(), PIE_DISPATCHER );
	SmartPtr<IOPackage> pUpdateVmStatePkg = DispatcherPackage::createInstance( PVE::DspVmEvent,
		event,
		getRequestPackage() );

	pVm->changeVmState(pUpdateVmStatePkg);

	/* Unregister fake DspVM instance */
	if( newState == VMS_STOPPED || newState == VMS_SUSPENDED )
	{
		QMutexLocker lock( &m_mtxVmConnectionWasClosed);
		bool bVmStartedByTask_AndExecutedYet
			= m_bVmStartedByTask && !m_bVmConnectionWasClosed;
		m_bTaskWasFinished = true;
		lock.unlock();

		if( !bVmStartedByTask_AndExecutedYet )
			CDspVm::UnregisterVmObject(pVm);
	}
	else
	{
		QMutexLocker lock( &m_mtxVmConnectionWasClosed);
		bool bVmConnectionWasClosed = m_bVmConnectionWasClosed;
		/* Flags m_bTaskWasFinished used by handleClientDisconnected logic
		 * as bNeedUnregisterVmObject.
		 * To avoid races with CDspVmManager::handleClientDisconnected do release the owner.
		 */
		m_bTaskWasFinished = true;
		lock.unlock();

		if (bVmConnectionWasClosed)
			CDspVm::UnregisterVmObject(pVm);
	}
}

PRL_RESULT Task_SwitchToSnapshot::RestoreVmConfig(const QString &sVmConfigPath)
{
	/**
	* Try to read and interpret config file for given snapshot
	*/
	QString sSavedVmConfig = CStatesHelper::MakeCfgFileName(CFileHelper::GetFileRoot(sVmConfigPath), m_SnapshotUuid);

	PRL_RESULT ret = CDspService::instance()->getVmConfigManager().loadConfig(m_pSavedVmConfig,
		sSavedVmConfig,
		getClient(),
		false );

	if( PRL_FAILED(ret) )
	{
		WRITE_TRACE(DBG_FATAL, "Can't parse VM config file. error = %#x( '%s' )"
			, ret
			, PRL_RESULT_TO_STRING( ret ) );
		return ret;
	}

	/**
	* Replace current config file with loaded
	*/
	//Apply valid VM UUID and server UUID
	//https://bugzilla.sw.ru/show_bug.cgi?id=120738
	if ( m_pSavedVmConfig->getVmIdentification()->getSourceVmUuid().isEmpty() ||
					Uuid( m_pSavedVmConfig->getVmIdentification()->getSourceVmUuid() ).isNull() )
		m_pSavedVmConfig->getVmIdentification()->setSourceVmUuid(m_pSavedVmConfig->getVmIdentification()->getVmUuid());
	m_pSavedVmConfig->getVmIdentification()->setVmUuid(m_pVmConfig->getVmIdentification()->getVmUuid());
	m_pSavedVmConfig->getVmIdentification()->setVmName(m_pVmConfig->getVmIdentification()->getVmName());
	m_pSavedVmConfig->getVmIdentification()->setServerUuid(
				CDspService::instance()->getDispConfigGuard().getDispConfig()->
						getVmServerIdentification()->getServerUuid());
	//https://bugzilla.sw.ru/show_bug.cgi?id=464660
	//Store current VM uptime values
	m_pSavedVmConfig->getVmIdentification()->setVmUptimeInSeconds(
		m_pVmConfig->getVmIdentification()->getVmUptimeInSeconds()
	);
	m_pSavedVmConfig->getVmIdentification()->setVmUptimeStartDateTime(
		m_pVmConfig->getVmIdentification()->getVmUptimeStartDateTime()
	);

	{
		CDspBugPatcherLogic logic( *CDspService::instance()->getHostInfo()->data() );

		logic.patchOldConfig( getClient()->getVmDirectoryUuid(),
							  m_pSavedVmConfig,
							  CDspBugPatcherLogic::pkSwitchToSnapshot );
	}

	ret = CDspService::instance()->getVmConfigManager().saveConfig(m_pSavedVmConfig,
		sVmConfigPath,
		getClient(),
		true,
		true);

	if( PRL_FAILED( ret ) )
	{
		WRITE_TRACE(DBG_FATAL, "Can't save vm config" );
		return ret;
	}

	return ret;
}

/**
* Notify job caller about progress
*/
void Task_SwitchToSnapshot::PostProgressEvent(uint uiProgress)
{
	if ( m_uiProgress == uiProgress )
		return;
	m_uiProgress = uiProgress;

	// Create event for client
	CVmEvent event(	PET_JOB_SWITCH_TO_SNAPSHOT_PROGRESS_CHANGED,
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

void Task_SwitchToSnapshot::releaseDiskManager()
{
// VirtualDisk commented out by request from CP team
//	if (m_pStatesManager)
//	{
//		m_pStatesManager->Clear();
//		delete m_pStatesManager;
//		m_pStatesManager = NULL;
//	}
}

void Task_SwitchToSnapshot::handleVmEvents(const SmartPtr<IOPackage> &p)
{
	if ( (m_initialVmState == VMS_STOPPED || m_initialVmState == VMS_SUSPENDED) &&
			isSnapshotedVmRunning() )
	{
		CVmEvent vmEvent(UTF8_2QSTR(p->buffers[0].getImpl()));
		CVmEventParameter* pRetCode = vmEvent.getEventParameter( EVT_PARAM_OP_RC );

		if (!pRetCode)
		{
			WRITE_TRACE(DBG_FATAL, "(!)Error: can't get event parameter." );
			return;
		}

		getLastError()->fromString(vmEvent.toString());
		setLastErrorCode(pRetCode->getParamValue().toUInt());
		wakeTask();
	}
}

void Task_SwitchToSnapshot::handleClientDisconnected( /* OUT */ bool &bNeedUnregisterVmObject )
{
	if ( (m_initialVmState == VMS_STOPPED || m_initialVmState == VMS_SUSPENDED) &&
			isSnapshotedVmRunning() )
	{
		WRITE_TRACE(DBG_FATAL, "Stop reverting process...");
		setLastErrorCode(PRL_ERR_VM_RESTORE_STATE_FAILED);
		wakeTask();
	}
	m_note.setVmStopped();

	QMutexLocker lock( &m_mtxVmConnectionWasClosed);
	m_bVmConnectionWasClosed = true;
	bNeedUnregisterVmObject = m_bTaskWasFinished;
	lock.unlock();
}


PRL_RESULT Task_SwitchToSnapshot::offlineRevertToSnapshot( bool& /* in|out */ flgImpersonated )
{
	if ( !RevertDiskToSnapshot(m_VmConfigPath) )
		return PRL_ERR_VM_RESTORE_STATE_FAILED;

	waitTask();

	releaseDiskManager();

	if ( m_snapshotedVmState == PVE::SnapshotedVmSuspended )
	{
		/**
		* Restore suspended state files
		*/
		if (!RestoreSuspendedVmFiles(m_VmConfigPath))
			return PRL_ERR_VM_RESTORE_STATE_FAILED;
	}
	else
	{
		DropSuspendedVmFiles(m_VmConfigPath);
	}

	/**
	* Restore NVRAM
	*/
	QString strNvRamFile = CFileHelper::GetFileRoot(m_VmConfigPath) + "/" + PRL_VM_NVRAM_FILE_NAME;

	if (!CStatesHelper::RestoreNVRAM(CFileHelper::GetFileRoot(m_VmConfigPath), m_SnapshotUuid))
		QFile::remove(strNvRamFile);

	{
		/**
		* Set access rights to hard disks folders
		*/
		CDspLockedPointer<CVmDirectoryItem> pVmDirItem = \
			CDspService::instance()->getVmDirManager().getVmDirItemByUuid(
			getClient()->getVmDirectoryUuid(), getVmUuid() );

		PRL_ASSERT( pVmDirItem );
		if( !pVmDirItem )
			return PRL_ERR_UNEXPECTED;

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

		/**
		* Set access rights to NVRAM.dat
		*/
		if (QFileInfo(strNvRamFile).exists()) {
			CDspService::instance()->getVmSnapshotStoreHelper().SetDefaultAccessRights(
			strNvRamFile
			, getClient()
			, pVmDirItem.getPtr() );
		}

		// #462940 back to impersonate again
		if( !flgImpersonated && !getClient()->getAuthHelper().Impersonate() )
			WRITE_TRACE( DBG_FATAL, " near setHdd permissions %s", PRL_RESULT_TO_STRING(PRL_ERR_IMPERSONATE_FAILED) );
		else
			flgImpersonated = true;

	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_SwitchToSnapshot::CheckAvailableSize(const QString& strVmConfigPath)
{
	if (m_snapshotedVmState != PVE::SnapshotedVmSuspended)
		return PRL_ERR_SUCCESS;

	// Get disk free space
	quint64 nFreeSpace = 0;
	QString strDir = CFileHelper::GetFileRoot(strVmConfigPath);

	if (!QFile::exists(strDir))
		strDir = CFileHelper::GetFileRoot(strDir);

	PRL_RESULT ret = CFileHelper::GetDiskAvailableSpace( strDir, &nFreeSpace );
	if ( PRL_FAILED(ret) )
	{
		WRITE_TRACE(DBG_FATAL, "GetDiskAvailableSpace() failed with code [%#x][%s]"
			, ret, PRL_RESULT_TO_STRING( ret ) );
		return PRL_ERR_VM_CREATE_SNAPSHOT_FAILED;
	}
	else
	{
		// Calculate required disk size
		PRL_UINT64 nRequiredSize = (PRL_UINT64)(m_pSavedVmConfig->getVmHardwareList()->getMemory()->getRamSize() +
			m_pSavedVmConfig->getVmHardwareList()->getVideo()->getMemorySize() + 10);
		// Check if required disk size is more then disk free space
		PRL_UINT64 nFreeSpaceInMb = ( PRL_UINT64 )( ((nFreeSpace)/1024)/1024 );
		if ( nRequiredSize >= nFreeSpaceInMb )
		{
			WRITE_TRACE(DBG_FATAL, "Task_SwitchToSnapshot : There is not enough disk free space for operation!" );
			return PRL_ERR_FREE_DISK_SPACE_FOR_REVERT_TO_SNAPSHOT;
		}
	}
	return PRL_ERR_SUCCESS;
}

void Task_SwitchToSnapshot::NotifyConfigChanged()
{
	// Notify users that VM configuration was changed
	CVmEvent event( PET_DSP_EVT_VM_CONFIG_CHANGED, getVmUuid(), PIE_DISPATCHER );

	SmartPtr<IOPackage> pkgConfigChanged = DispatcherPackage::createInstance( PVE::DspVmEvent,
		event,
		getRequestPackage() );

	CDspService::instance()->getClientManager().sendPackageToVmClients( pkgConfigChanged,
		getClient()->getVmDirectoryUuid(),
		getVmUuid() );
}

PRL_RESULT Task_SwitchToSnapshot::changeFirewallSettings()
{
	if ( ! CDspService::isServerModePSBM() )
		return PRL_ERR_SUCCESS;

	if ( m_initialVmState != VMS_RUNNING && m_initialVmState != VMS_PAUSED )
		return PRL_ERR_SUCCESS;

	if ( !isSnapshotedVmRunning() )
		return PRL_ERR_SUCCESS;

	CFirewallHelper fw(m_pSavedVmConfig);

	PRL_RESULT ret = fw.Execute();
	if (PRL_FAILED(ret))
	{
		if ( ret == PRL_ERR_FIREWALL_TOOL_EXECUTED_WITH_ERROR )
		{
			getLastError()->setEventType( PET_DSP_EVT_ERROR_MESSAGE );
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String,
				fw.GetErrorMessage(),
				EVT_PARAM_DETAIL_DESCRIPTION )
				);
		}
		return ret;
	}

	return PRL_ERR_SUCCESS;
}
