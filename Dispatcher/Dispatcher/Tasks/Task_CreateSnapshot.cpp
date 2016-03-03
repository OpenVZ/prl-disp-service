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
///	Task_CreateSnapshot.cpp
///
/// @brief
///	Definition of the class CDspTaskHelper
///
/// @brief
///	This class implements long running tasks helper class
///
/// @author sergeyt
///	Ilya@
///
////////////////////////////////////////////////////////////////////////////////

#include "Task_CreateSnapshot.h"
#include "Task_CommonHeaders.h"

#include "CProtoSerializer.h"
#include "CDspClientManager.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "CDspAccessManager.h"
#include "CDspVmSnapshotInfrastructure.h"
#include <prlcommon/PrlCommonUtilsBase/SysError.h>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include "Libraries/StatesUtils/StatesHelper.h"

using namespace Parallels;

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

namespace {
	PRL_BOOL DiskCallBack(PRL_STATES_CALLBACK_TYPE iCallbackType, PRL_INT32 iProgress, PRL_VOID_PTR pParameter)
	{
		UNUSED_PARAM(iCallbackType);
		LOG_MESSAGE(DBG_DEBUG, "Task_CreateSnapshot::DiskCallBack() iCallbackType [%d] iProgress [%d]", iCallbackType, iProgress);

		Task_CreateSnapshot *pTask = (Task_CreateSnapshot *)pParameter;

		if( pTask->operationIsCancelled() )
		{
			WRITE_TRACE(DBG_FATAL, "Task_CreateSnapshot::DiskCallBack(): operation was canceled!" );
			pTask->setHddErrorCode( PRL_ERR_OPERATION_WAS_CANCELED );
			pTask->wakeTask();
			return PRL_FALSE;
		}

		// Check progress
		if (iProgress < 0)
		{
			WRITE_TRACE(DBG_FATAL, "Error occurred while taking snapshot with code [%#x][%s]"
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

Task_CreateSnapshot::Task_CreateSnapshot (const SmartPtr<CDspClient>& user,
                             const SmartPtr<IOPackage>& p,
							 VIRTUAL_MACHINE_STATE initialVmState) :

	CDspTaskHelper( user, p ),
	m_initialVmState(initialVmState),
	m_uiProgress((uint )-1)
{
	setLastErrorCode(PRL_ERR_SUCCESS);
	setHddErrorCode(PRL_ERR_SUCCESS);

	m_pVmConfig = SmartPtr<CVmConfiguration>(new CVmConfiguration());
}

Task_CreateSnapshot::~Task_CreateSnapshot()
{
}

QString Task_CreateSnapshot::getVmUuid()
{
	return m_pVmConfig ? m_pVmConfig->getVmIdentification()->getVmUuid() : QString();
}

PRL_RESULT Task_CreateSnapshot::prepareTask()
{
	// check params in existing VM
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		////////////////////////////////////////////////////////////////////////
		// retrieve user parameters from request data
		////////////////////////////////////////////////////////////////////////

		CProtoCommandPtr cmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmCreateSnapshot, UTF8_2QSTR(getRequestPackage()->buffers[0].getImpl()));
		if ( ! cmd->IsValid() )
		{
			throw (PRL_ERR_FAILURE);
		}

		CProtoCreateSnapshotCommand *pVmCmd = CProtoSerializer::CastToProtoCommand<CProtoCreateSnapshotCommand>(cmd);
		QString strVmUuid = pVmCmd->GetVmUuid();
		m_SnapshotUuid = pVmCmd->GetSnapshotUuid();
		m_nFlags = pVmCmd->GetCommandFlags();
		m_sPath = pVmCmd->GetSnapshotPath();

		ret = PRL_ERR_SUCCESS;
		SmartPtr<CVmConfiguration>
			pVmConfig = CDspService::instance()->getVmDirHelper().getVmConfigByUuid(getClient(), strVmUuid, ret);
		if( !pVmConfig )
		{
			PRL_ASSERT( PRL_FAILED(ret) );
			if( !PRL_FAILED( ret ) )
				ret = PRL_ERR_FAILURE;

			WRITE_TRACE(DBG_FATAL, "Couldn't to extract VM config for UUID '%s'", QSTR2UTF8(strVmUuid));
			throw (ret);
		}
		m_pVmConfig = pVmConfig; // to prevent dereference NULL pointer in finalizeTask()

		if (!IS_OPERATION_SUCCEEDED(getLastErrorCode()))
			throw getLastErrorCode();

		if( !IS_OPERATION_SUCCEEDED( m_pVmConfig->m_uiRcInit ) )
			throw PRL_ERR_PARSE_VM_CONFIG;

		PRL_RESULT error = IsHasRightsForCreateSnapshot( m_pVmConfig, *getLastError() );
		if( PRL_FAILED( error ) )
		{
			WRITE_TRACE(DBG_FATAL, ">>> User hasn't rights to access this VM %#x,%s", error, PRL_RESULT_TO_STRING( error ) );
			throw error;
		}

		if (isBootCampUsed())
			throw PRL_ERR_VMCONF_BOOTCAMP_HARD_SNAPSHOTS_NOT_ALLOW;

		ret = PRL_ERR_SUCCESS;
	}
	catch(PRL_RESULT code)
	{
		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while registering configuration with code [%#x][%s]"
			, code, PRL_RESULT_TO_STRING( code ) );
	}

	setLastErrorCode(ret);

	return ret;
}

void Task_CreateSnapshot::notifyVmUsers( PRL_EVENT_TYPE evt )
{
	CVmEvent event( evt,
		getVmUuid(),
		PIE_DISPATCHER );

	SmartPtr<IOPackage> p = DispatcherPackage::createInstance( PVE::DspVmEvent, event, getRequestPackage());

	CDspService::instance()->getClientManager().sendPackageToVmClients( p,
			getClient()->getVmDirectoryUuid(), getVmUuid() );
}

void Task_CreateSnapshot::finalizeTask()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;
	QString sVmConfigPath;
	try
	{
		if (!IS_OPERATION_SUCCEEDED(getLastErrorCode()))
			throw getLastErrorCode();

		QString userName;
		{
			CDspLockedPointer< CDispUser > pLockedDispUser = CDspService::instance()->getDispConfigGuard()
					.getDispUserByUuid( getClient()->getUserSettingsUuid() );
			if( pLockedDispUser )
				userName = pLockedDispUser->getUserName();
		}

		/**
		* Set access rights to snapshot files
		*/
		{
			CDspLockedPointer<CVmDirectoryItem> pVmDirItem = \
				CDspService::instance()->getVmDirManager().getVmDirItemByUuid(
				getClient()->getVmDirectoryUuid(), getVmUuid() );

			sVmConfigPath = pVmDirItem.getPtr()->getVmHome();

			/**
			* Set access rights to "Snapshots" folder
			*/
			ret = CDspService::instance()->getVmSnapshotStoreHelper().SetDefaultAccessRights(
				m_SnapshotsDir
				, getClient()
				, pVmDirItem.getPtr() );

			if (PRL_FAILED(ret))
				throw (ret);


			/**
			* Set access rights to hard disks folders
			*/
			ret = CDspVmSnapshotStoreHelper::setDefaultAccessRightsToAllHdd( m_pVmConfig
				, getClient()
				, pVmDirItem.getPtr()
				, true /* break by error */
				);
			if( PRL_FAILED(ret) )
				throw ret;
		}

		// Generate "snapshots tree changed" event
		notifyVmUsers( PET_DSP_EVT_VM_SNAPSHOTS_TREE_CHANGED );

		// Notify all user: snapshot created
		notifyVmUsers( PET_DSP_EVT_VM_SNAPSHOTED );

		ret = PRL_ERR_SUCCESS;

	} catch (PRL_RESULT code) {
		ret = code; //PRL_ERR_OPERATION_FAILED;

		WRITE_TRACE(DBG_FATAL, "Error occurred while registering configuration with code [%#x][%s]"
			, code, PRL_RESULT_TO_STRING( code ) );
	}
	setLastErrorCode(ret);

	{
		{//need to destroy pVm before send reply
		 //destroy is long operation #PSBM-9509
			// Find VM
			SmartPtr<CDspVm> pVm =
				CDspVm::GetVmInstanceByUuid( getVmUuid(), getClient()->getVmDirectoryUuid() );
			if (pVm)
				// Restore initial VM status
				UpdateVmState(pVm);
		}

		if ( PRL_FAILED( getLastErrorCode() ) )
		{
			getClient()->sendResponseError( getLastError(), getRequestPackage() );
		}
		else
		{
			// Send response
			CProtoCommandPtr pCmd
				= CProtoSerializer::CreateDspWsResponseCommand( getRequestPackage(), getLastErrorCode() );
			CProtoCommandDspWsResponse *pResponseCmd = \
				CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pCmd );
			pResponseCmd->AddStandardParam( m_SnapshotUuid );
			getClient()->sendResponse( pCmd, getRequestPackage() );
			getLastError()->setEventCode( PRL_ERR_SUCCESS );
		}
	}
}


PRL_RESULT Task_CreateSnapshot::run_body()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	bool flgImpersonated = false;
	try
	{
		PRL_ASSERT( m_pVmConfig ); // it was checked in prepareTask();

		ret = prepareSnapshotDirectory( m_SnapshotsDir );
		if (PRL_FAILED(ret))
			throw (ret);

		// Let current thread impersonate the security context of a logged-on user
		if( ! getClient()->getAuthHelper().Impersonate() )
			throw PRL_ERR_IMPERSONATE_FAILED;
		flgImpersonated = true;

		/**
		* Check available size for new snapshot
		*/
		ret = CheckAvailableSize( m_initialVmState, m_SnapshotsDir, m_pVmConfig );

		if(PRL_FAILED(ret))
			throw ret;

		QString sVmConfigPath = CDspService::instance()
				->getVmDirManager().getVmDirItemByUuid( getClient()->getVmDirectoryUuid(), getVmUuid() )
					->getVmHome();

		if ( m_initialVmState == VMS_SUSPENDED )
		{
			/**
			* Backup suspended state files into "Snapshots" folder
			*/
			if ( !CopySuspendedVmFiles(sVmConfigPath) )
				throw PRL_ERR_VM_CREATE_SNAPSHOT_FAILED;
		}

		if ( m_initialVmState == VMS_STOPPED || m_initialVmState == VMS_SUSPENDED )
		{
			/**
			* Create disk snapshot
			*/
			if ( !CreateDiskSnapshot() )
				throw PRL_ERR_VM_CREATE_SNAPSHOT_FAILED;

			waitTask();

			if( !IS_OPERATION_SUCCEEDED( getHddErrorCode() ) )
				throw getHddErrorCode();

			CStatesHelper::SaveNVRAM(CFileHelper::GetFileRoot(sVmConfigPath), m_SnapshotUuid);
		}
		else if (m_initialVmState == VMS_RUNNING || m_initialVmState == VMS_PAUSED)
		{
			/**
			* Send "Create snapshot" command to VM and waiting for response
			*/
			PRL_RESULT ret = sendCreateSnapshotPackageToVm();

			if (PRL_FAILED(ret))
				throw (ret);
		}

		/**
		 * Save Snapshots.xml file
		 */
		CDspService::instance()->getVmSnapshotStoreHelper().createSnapshot(getClient(),
				getRequestPackage(),
				m_initialVmState,
				(m_nFlags & PCSF_BACKUP) ? SNAP_BACKUP_MODE : 0);

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
		// Terminates the impersonation of a user, return to Dispatcher access rights
		if( flgImpersonated && ! getActualClient()->getAuthHelper().RevertToSelf() )
			WRITE_TRACE(DBG_FATAL, "%s: error: %s", __FILE__, PRL_RESULT_TO_STRING( PRL_ERR_REVERT_IMPERSONATE_FAILED ) );

		WRITE_TRACE(DBG_FATAL, "Error while creating new snapshot with code [%#x][%s]"
			, code, PRL_RESULT_TO_STRING( code ) );

	}

	setLastErrorCode(ret);

	/**
	* finalize and cleanup
	*/

	return ret;
}

PRL_RESULT Task_CreateSnapshot::IsHasRightsForCreateSnapshot(
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

		WRITE_TRACE(DBG_FATAL, "Client hasn't rights to read vm to create new snapshot. err: %#x, %s", err, PRL_RESULT_TO_STRING( err ) );
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

void Task_CreateSnapshot::waitTask()
{
	m_semaphore.acquire();
}

void Task_CreateSnapshot::wakeTask()
{
	m_semaphore.release();
}

void Task_CreateSnapshot::setHddErrorCode(PRL_RESULT errCode)
{
	m_hddErrCode = errCode;
}

PRL_RESULT Task_CreateSnapshot::getHddErrorCode()
{
	return m_hddErrCode;
}

/**
* Backup suspended state files into "Snapshots" folder
*/
bool Task_CreateSnapshot::CopySuspendedVmFiles(const QString& strVmConfigPath)
{
	// Get directory path where store configuration file
	QString dir_path = CFileHelper::GetFileRoot(strVmConfigPath);
	// Create "Snapshots "folder
	QString snapshots_dir = dir_path + "/Snapshots/";

	if (!QDir().exists(snapshots_dir))
	{
		QDir().mkpath(snapshots_dir);
	}
	// Generate full file names for .mem and .sav files
	CStatesHelper sh(strVmConfigPath);

	QString src_sav_file = sh.getSavFileName();
	QString src_mem_file;
	QString mem_file;

	if ( !sh.extractMemFileName(src_sav_file, mem_file) )
	{
		WRITE_TRACE(DBG_FATAL, "Failed to extract memory file name from [%s]", QSTR2UTF8(src_sav_file));
		return false;
	}

	if ( sh.extractMemFilePath(src_sav_file, src_mem_file) )
		src_mem_file  = src_mem_file + "/" + mem_file;
	else
		src_mem_file  = dir_path + "/" + mem_file;

	QString src_png_file = dir_path + "/" PRL_VM_SUSPENDED_SCREEN_FILE_NAME;

	QString dest_mem_file = dir_path + "/Snapshots/" + m_SnapshotUuid + ".mem";
	QString dest_sav_file = dir_path + "/Snapshots/" + m_SnapshotUuid + ".sav";
	QString dest_png_file = dir_path + "/Snapshots/" + m_SnapshotUuid + ".png";

	CStatesHelper::CopyStateFile(src_png_file, dest_png_file);
	CStatesHelper::SaveNVRAM(dir_path, m_SnapshotUuid);

	bool ret = ( CStatesHelper::CopyStateFile(src_sav_file, dest_sav_file) &&
		CStatesHelper::CopyStateFile(src_mem_file, dest_mem_file) );

	return  ret;
}

bool Task_CreateSnapshot::isBootCampUsed()
{
	CVmHardware *lpcHardware = m_pVmConfig->getVmHardwareList();
	for (int i = 0; i < lpcHardware->m_lstHardDisks.size() ; i++)
	{
		if( lpcHardware->m_lstHardDisks[i]->getEnabled())
		{
			if( lpcHardware->m_lstHardDisks[i]->getEmulatedType() == PVE::BootCampHardDisk)
			{
				return true;
			}
		}
	}
	return false;
}

bool Task_CreateSnapshot::CreateDiskSnapshot()
{
	return false;
}

PRL_RESULT Task_CreateSnapshot::DeleteDiskSnapshot()
{
	return PRL_ERR_FAILURE;
}

PRL_RESULT Task_CreateSnapshot::sendCreateSnapshotPackageToVm()
{
	Snapshot::Unit u(getVmUuid(), *this, m_initialVmState);
	return u.create();
}

void Task_CreateSnapshot::UpdateVmState(SmartPtr<CDspVm> &pVm)
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

	pVm->changeVmState(pUpdateVmStatePkg);

	/* Unregister fake DspVM instance */
	if (m_initialVmState == VMS_STOPPED || m_initialVmState == VMS_SUSPENDED)
	{
		CDspVm::UnregisterVmObject(pVm);
	}
}

/**
* Notify job caller about progress
*/
void Task_CreateSnapshot::PostProgressEvent(uint uiProgress)
{
	if ( m_uiProgress == uiProgress )
		return;
	m_uiProgress = uiProgress;

	// Create event for client
	CVmEvent event(	PET_JOB_CREATE_SNAPSHOT_PROGRESS_CHANGED,
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

PRL_RESULT Task_CreateSnapshot::CheckAvailableSize(
	VIRTUAL_MACHINE_STATE nVmState,
	const QString& qsSnapshotsDir,
	SmartPtr<CVmConfiguration> pVmConfig )
{
	if ( nVmState == VMS_STOPPED )
		return PRL_ERR_SUCCESS;

	// Get disk free space
	quint64 nFreeSpace = 0;
	QString strDir = qsSnapshotsDir;
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
		PRL_UINT64 nRequiredSize = (PRL_UINT64)(pVmConfig->getVmHardwareList()->getMemory()->getRamSize() +
			pVmConfig->getVmHardwareList()->getVideo()->getMemorySize() + 10);
		// Check if required disk size is more then disk free space
		PRL_UINT64 nFreeSpaceInMb = ( PRL_UINT64 )( ((nFreeSpace)/1024)/1024 );
		if ( nRequiredSize >= nFreeSpaceInMb )
		{
			WRITE_TRACE(DBG_FATAL, "Task_CreateSnapshot : There is not enough disk free space for operation!" );
			return PRL_ERR_FREE_DISK_SPACE_FOR_CREATE_SNAPSHOT;
		}
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_CreateSnapshot::prepareSnapshotDirectory( QString& /* OUT */ sSnapshotsDir )
{
	PRL_RESULT ret = PRL_ERR_FAILURE;

	CDspLockedPointer<CVmDirectoryItem> pVmDirItem
		= CDspService::instance()->getVmDirManager().getVmDirItemByUuid(
		getClient()->getVmDirectoryUuid(), getVmUuid() );
	QString sVmConfigPath = pVmDirItem->getVmHome();

	/**
	* Prepare "Snapshots" folder
	*/
	{
		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

		sSnapshotsDir = CFileHelper::GetFileRoot( sVmConfigPath ) + "/" + VM_GENERATED_WINDOWS_SNAPSHOTS_DIR;

		if( !CFileHelper::DirectoryExists(sSnapshotsDir, &getClient()->getAuthHelper())
			&&
			!CFileHelper::WriteDirectory(sSnapshotsDir, &getClient()->getAuthHelper())
			)
		{
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String,
				sSnapshotsDir,
				EVT_PARAM_MESSAGE_PARAM_0));

			return PRL_ERR_MAKE_DIRECTORY;
		}
	}

	ret = CDspService::instance()->getVmSnapshotStoreHelper().SetDefaultAccessRights(
		sSnapshotsDir
		, getClient()
		, pVmDirItem.getPtr() );

	return ret;
}
