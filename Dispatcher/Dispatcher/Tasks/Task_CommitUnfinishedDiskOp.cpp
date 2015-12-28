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
///	Task_CommitUnfinishedDiskOp.cpp
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

#include "Task_CommitUnfinishedDiskOp.h"
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

///////////////////////////////////////////////////////////////////////////////
// struct External

struct External: CDspTaskHelper
{
	External(SmartPtr<CDspClient> user_, const SmartPtr<IOPackage>& request_,
		const QString& vm_);
protected:
	virtual PRL_RESULT prepareTask();

	virtual PRL_RESULT run_body();

	virtual void finalizeTask();
private:
	enum
	{
		TIMEOUT = 100
	};
	static CDspVmDirHelper& vm()
	{
		return CDspService::instance()->getVmDirHelper();
	}

	SmartPtr<CDspVm> m_vm;
};

External::External(SmartPtr<CDspClient> user_, const SmartPtr<IOPackage>& request_, const QString& vm_):
	CDspTaskHelper(user_, request_)
{
	m_vm = CDspVm::GetVmInstanceByUuid(vm_, user_->getVmDirectoryUuid());
}

PRL_RESULT External::prepareTask()
{
	if (NULL == m_vm.getImpl())
	{
		setLastErrorCode(PRL_ERR_VM_START_FAILED);
		return PRL_ERR_VM_START_FAILED;
	}
	while (true)
	{
		if (operationIsCancelled())
		{
			m_vm.reset();
			setLastErrorCode(PRL_ERR_OPERATION_WAS_CANCELED);
			return PRL_ERR_OPERATION_WAS_CANCELED;
		}
		// NB. registerExclusiveVmOperation doesn't block.
		// have to poll to emulate.
		PRL_RESULT e = vm().registerExclusiveVmOperation(m_vm->getVmUuid(), m_vm->getVmDirUuid(),
					Snapshot::CommitUnfinished::Command::name(), getClient());
		if (PRL_SUCCEEDED(e))
			break;

		WRITE_TRACE_RL(10, DBG_FATAL, "[%s] registerExclusiveVmOperation failed. Reason: %#x (%s)",
			__FUNCTION__, e, PRL_RESULT_TO_STRING(e));
		HostUtils::Sleep(TIMEOUT);
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT External::run_body()
{
	Snapshot::Unit u(m_vm, *this, VMS_UNKNOWN);
	setLastErrorCode(u.commitUnfinished());
	return getLastErrorCode();
}

void External::finalizeTask()
{
	if (NULL != m_vm.getImpl())
	{
		vm().unregisterExclusiveVmOperation(m_vm->getVmUuid(), m_vm->getVmDirUuid(),
			Snapshot::CommitUnfinished::Command::name(), getClient());
	}
}

namespace {
	PRL_BOOL DiskCallBack(PRL_STATES_CALLBACK_TYPE iCallbackType, PRL_INT32 iProgress, PRL_VOID_PTR pParameter)
	{
		UNUSED_PARAM(iCallbackType);
		LOG_MESSAGE(DBG_DEBUG, "Task_CommitUnfinishedDiskOp::DiskCallBack() iCallbackType [%d] iProgress [%d]",
			iCallbackType, iProgress);

		Task_CommitUnfinishedDiskOp *pTask = (Task_CommitUnfinishedDiskOp *)pParameter;

		if( pTask->operationIsCancelled() )
		{
			WRITE_TRACE(DBG_FATAL, "Task_CommitUnfinishedDiskOp::DiskCallBack(): operation was canceled!" );
			pTask->setHddErrorCode( PRL_ERR_OPERATION_WAS_CANCELED );
			pTask->wakeTask();
			return PRL_FALSE;
		}

		// Check progress
		if (iProgress < 0)
		{
			WRITE_TRACE(DBG_FATAL, "Error occurred while completing unfinished disk operation with code [%#x][%s]"
				, iProgress, PRL_RESULT_TO_STRING( iProgress ) );

			pTask->setHddErrorCode( iProgress );
			pTask->wakeTask();
			return PRL_FALSE;
		}

		if (iProgress == PRL_STATE_PROGRESS_COMPLETED)
		{
			pTask->wakeTask();
		}
		return PRL_TRUE;
	}
}

Task_CommitUnfinishedDiskOp::Task_CommitUnfinishedDiskOp (SmartPtr<CDspClient>& user,
														  const SmartPtr<IOPackage>& p,
														  bool bNeedStartVm) :

CDspTaskHelper( user, p )
{
	setLastErrorCode(PRL_ERR_SUCCESS);
	setHddErrorCode(PRL_ERR_SUCCESS);
	m_bNeedStartVm = bNeedStartVm;
// VirtualDisk commented out by request from CP team
//	m_pStatesManager = new CDSManager();
	m_pVmConfig = SmartPtr<CVmConfiguration>(new CVmConfiguration());
}

Task_CommitUnfinishedDiskOp::~Task_CommitUnfinishedDiskOp()
{
// VirtualDisk commented out by request from CP team
//	delete m_pStatesManager;
//	m_pStatesManager = NULL;
}

QString Task_CommitUnfinishedDiskOp::getVmUuid()
{
	return m_pVmConfig ? m_pVmConfig->getVmIdentification()->getVmUuid() : QString();
}

PRL_RESULT Task_CommitUnfinishedDiskOp::prepareTask()
{
	// check params in existing VM
	PRL_RESULT ret = PRL_ERR_SUCCESS;

	try
	{
		////////////////////////////////////////////////////////////////////////
		// retrieve user parameters from request data
		////////////////////////////////////////////////////////////////////////

		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

		CProtoCommandPtr pCmd = \
			CProtoSerializer::ParseCommand((PVE::IDispatcherCommands)getRequestPackage()->header.type,
			UTF8_2QSTR(getRequestPackage()->buffers[0].getImpl()));
		if (!pCmd->IsValid())
		{
			WRITE_TRACE(DBG_FATAL, "Wrong package received with type %d '%s'",
				getRequestPackage()->header.type, PVE::DispatcherCommandToString(getRequestPackage()->header.type));
			throw PRL_ERR_FAILURE;
		}

		QString strVmUuid = pCmd->GetVmUuid();

		ret = PRL_ERR_SUCCESS;

		m_pVmConfig = CDspService::instance()->getVmDirHelper().getVmConfigByUuid(getClient(), strVmUuid, ret);
		if( !m_pVmConfig )
		{
			PRL_ASSERT( PRL_FAILED(ret) );
			if( !PRL_FAILED( ret ) )
				ret = PRL_ERR_FAILURE;

			WRITE_TRACE(DBG_FATAL, "Couldn't to extract VM config for UUID '%s'", QSTR2UTF8(strVmUuid));
			throw (ret);
		}

		if (PRL_FAILED(ret))
			throw ret;

		PRL_RESULT error = getDevices();
		if( PRL_FAILED( error ) )
		{
			WRITE_TRACE(DBG_FATAL, ">>> User hasn't rights to access this VM %#x,%s",
				error, PRL_RESULT_TO_STRING( error ) );
			throw error;
		}
		ret = PRL_ERR_SUCCESS;
	}
	catch(PRL_RESULT code)
	{
		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while commit unfinished disk operation with code [%#x][%s]"
			, code, PRL_RESULT_TO_STRING( code ) );
	}

	setLastErrorCode(ret);

	return ret;
}

void Task_CommitUnfinishedDiskOp::finalizeTask()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;
	try
	{
		m_devices.clear();
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
			/**
			* Set access rights to "Snapshots" folder
			*/
			ret = CDspService::instance()->getVmSnapshotStoreHelper().SetDefaultAccessRights(
				m_SnapshotsDir
				, getClient()
				, pVmDirItem.getPtr() );

			if (PRL_FAILED(ret))
			{
				WRITE_TRACE(DBG_FATAL, "Failed to set access rights to folder [%s] with error [%s]"
					, QSTR2UTF8(m_SnapshotsDir), PRL_RESULT_TO_STRING(ret));
				throw (ret);
			}

// VirtualDisk commented out by request from CP team
//			{
//				//https://bugzilla.sw.ru/show_bug.cgi?id=267152
//				CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );
//
//				// Release disk handles
//				m_pStatesManager->Clear();
//			}

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

		/**
		* Start VM process if need
		*/
		if (m_bNeedStartVm)
		{
			SmartPtr<CDspVm> pVm = CDspVm::GetVmInstanceByUuid( getVmUuid(), getClient()->getVmDirectoryUuid() );
			if (pVm)
			{
				CProtoCommandPtr pRequest
					= CProtoSerializer::CreateProtoBasicVmCommand(PVE::DspCmdVmStart, pVm->getVmUuid());

				SmartPtr<IOPackage> pPackage
					= DispatcherPackage::duplicateInstance(getRequestPackage(), pRequest->GetCommand()->toString());
				pPackage->header.type = pRequest->GetCommandId();

				if( ! pVm->startVmAfterCommitUnfunishedDiskOp(getClient(), pPackage) )
				{
					WRITE_TRACE(DBG_FATAL, "Unable to start VM after commit unfinished disk operation");
					throw (PRL_ERR_VM_START_FAILED);
				}
			}
		}

		ret = PRL_ERR_SUCCESS;

	} catch (PRL_RESULT code) {
		ret = code;

		WRITE_TRACE(DBG_FATAL, "Error occurred while commit unfinished disk operation with code [%#x][%s]"
			, code, PRL_RESULT_TO_STRING( code ) );
	}

	setLastErrorCode(ret);
	postEventToClient(PET_DSP_EVT_COMMIT_UNFINISHED_DISK_OP_FINISHED);

	/**
	* Send response if need
	*/
	if ( m_bNeedStartVm && PRL_FAILED(getLastErrorCode()) )
	{
		{//need to destroy pVm before send reply
		 //destroy is long operation #PSBM-9509
			SmartPtr<CDspVm> pVm =
				CDspVm::GetVmInstanceByUuid( getVmUuid(), getClient()->getVmDirectoryUuid() );
			if (pVm)
			{
				CDspVm::UnregisterVmObject(pVm);
			}
		}
		getClient()->sendResponseError( getLastError(), getRequestPackage() );
	}
}


PRL_RESULT Task_CommitUnfinishedDiskOp::run_body()
{
	PRL_RESULT ret = PRL_ERR_SUCCESS;
	try
	{
		// do before impersonate to correct set permissions
		ret = prepareSnapshotDirectory( m_SnapshotsDir );
		if (PRL_FAILED(ret))
			throw (ret);

		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

		postEventToClient(PET_DSP_EVT_COMMIT_UNFINISHED_DISK_OP_STARTED);
		postProgressEvent(50);

		if (!m_devices.isEmpty())
		{
// VirtualDisk commented out by request from CP team
//			m_pStatesManager->Clear();
//			foreach (CVmHardDisk* d, m_devices)
//				m_pStatesManager->AddDisk(d->getSystemName());

			/**
			* Commit unfinished disk operation
			*/
			ret = commitUnfinishedDiskOp();

			if (PRL_SUCCEEDED(ret))
				waitTask();

// VirtualDisk commented out by request from CP team
//			// Release disk handles
//			m_pStatesManager->Clear();

			if (PRL_FAILED(ret))
				throw (ret);
		}

		if (PRL_FAILED(getHddErrorCode()))
			throw getHddErrorCode();

		removeHalfDeletedSnapshots( m_pVmConfig, m_deletedSnapshots );
		ret = PRL_ERR_SUCCESS;
	}
	catch (PRL_RESULT code)
	{
		ret = code;
		WRITE_TRACE(DBG_FATAL, "Error occurred while commit unfinished disk operation with code [%#x][%s]"
			, code, PRL_RESULT_TO_STRING( code ) );

	}

	setLastErrorCode(ret);

	/**
	* finalize and cleanup
	*/

	return ret;
}

PRL_RESULT Task_CommitUnfinishedDiskOp::getDevices()
{
	/**
	* check if user is authorized to access this VM
	*/

	QString vm_uuid = m_pVmConfig->getVmIdentification()->getVmUuid();

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

		WRITE_TRACE(DBG_FATAL, "Client hasn't rights to read vm to create new snapshot. err: %#x, %s",
			err, PRL_RESULT_TO_STRING( err ) );
		return err;
	}

	Snapshot::CommitUnfinished::Diagnostic u(m_pVmConfig);
	foreach (CVmHardDisk* d, u.getDevices())
	{
		QString n = d->getSystemName();
		if (!CFileHelper::FileCanRead(n, &getClient()->getAuthHelper()))
		{
			getLastError()->setEventCode(PRL_ERR_ACCESS_TO_VM_HDD_DENIED);
			getLastError()->addEventParameter(new CVmEventParameter(PVE::String,
				m_pVmConfig->getVmIdentification()->getVmName(),
				EVT_PARAM_MESSAGE_PARAM_0));
			getLastError()->addEventParameter(new CVmEventParameter(PVE::String,
				n, EVT_PARAM_MESSAGE_PARAM_1));
			return PRL_ERR_ACCESS_TO_VM_HDD_DENIED;
		}
	}
	m_devices = u.getDevices();
	m_deletedSnapshots = u.getSnapshots();
	return PRL_ERR_SUCCESS;
}

void Task_CommitUnfinishedDiskOp::waitTask()
{
	m_semaphore.acquire();
}

void Task_CommitUnfinishedDiskOp::wakeTask()
{
	m_semaphore.release();
}

void Task_CommitUnfinishedDiskOp::setHddErrorCode(PRL_RESULT errCode)
{
	m_hddErrCode = errCode;
}

PRL_RESULT Task_CommitUnfinishedDiskOp::getHddErrorCode()
{
	return m_hddErrCode;
}

PRL_RESULT Task_CommitUnfinishedDiskOp::commitUnfinishedDiskOp()
{
// VirtualDisk commented out by request from CP team
//	PRL_RESULT ret = m_pStatesManager->CommitUnfinished(&DiskCallBack, this);
//
//	if( PRL_FAILED(ret) )
//		WRITE_TRACE(DBG_FATAL, "commitUnfinishedDiskOp() failed by error %#x (%s)", ret, PRL_RESULT_TO_STRING(ret) );
//
//	return ret;
	return PRL_ERR_FAILURE;
}


/**
* Notify job caller about progress
*/
void Task_CommitUnfinishedDiskOp::postProgressEvent(uint uiProgress)
{
	// Create event for client
	CVmEvent event(	PET_JOB_COMMIT_UNFINISHED_DISK_OP_PROGRESS_CHANGED,
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

/**
* Commit unfinished disk operation in synchronous mode
*/
PRL_RESULT Task_CommitUnfinishedDiskOp::commitDiskOpSync(SmartPtr<CDspClient> user, const SmartPtr<IOPackage> &pkg)
{
	CVmEvent e;
	CDspService::instance()->getTaskManager()
		.schedule(new Task_CommitUnfinishedDiskOp(user, pkg))
			.wait().getResult(&e);

	WRITE_TRACE(DBG_FATAL, "Check disk completed");
	return e.getEventCode();
}

/**
* Commit unfinished disk operation inside a running VM in asynchronous mode
*/
CDspTaskFuture<CDspTaskHelper> Task_CommitUnfinishedDiskOp::pushVmCommit(const CDspVm& vm_)
{
	// Make fake command
	Snapshot::DiskState st;
	SmartPtr<IOPackage> p = Snapshot::CommitUnfinished::Command::request(vm_.getVmUuid(), st);
	CDspTaskHelper* t = new External(vm_.getVmRunner(), p, vm_.getVmUuid());
	return CDspService::instance()->getTaskManager().schedule(t);
}

void Task_CommitUnfinishedDiskOp::postEventToClient(PRL_EVENT_TYPE evtType)
{
	// Generate event
	CVmEvent event( evtType,
		getVmUuid(),
		PIE_DISPATCHER );

	SmartPtr<IOPackage> p =
		DispatcherPackage::createInstance( PVE::DspVmEvent, event, getRequestPackage());

	CDspService::instance()->getClientManager().sendPackageToVmClients( p,
		getClient()->getVmDirectoryUuid(), getVmUuid() );
}

PRL_RESULT Task_CommitUnfinishedDiskOp::prepareSnapshotDirectory( QString& /* OUT */ sSnapshotsDir )
{
	PRL_RESULT ret = PRL_ERR_FAILURE;

	CDspLockedPointer<CVmDirectoryItem> pVmDirItem =
		CDspService::instance()->getVmDirManager().getVmDirItemByUuid(
			getClient()->getVmDirectoryUuid(), getVmUuid() );

	if (!pVmDirItem)
		return PRL_ERR_VM_UUID_NOT_FOUND;

	QString sVmConfigPath = pVmDirItem->getVmHome();

	/**
	* Prepare "Snapshots" folder
	*/
	{
		//https://bugzilla.sw.ru/show_bug.cgi?id=267152
		CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

		m_SnapshotsDir = CFileHelper::GetFileRoot( sVmConfigPath ) + "/" + VM_GENERATED_WINDOWS_SNAPSHOTS_DIR;

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

void Task_CommitUnfinishedDiskOp::removeHalfDeletedSnapshots(
	const SmartPtr<CVmConfiguration>& pVmConfig, const QSet<QString>& snapshotIds )
{
	if (snapshotIds.isEmpty())
		return;

	CDspVmSnapshotStoreHelper &snapHelper = CDspService::instance()->getVmSnapshotStoreHelper();
	QString strVmFolder = pVmConfig->getConfigDirectory();

	foreach(const QString& id, snapshotIds)
	{
		snapHelper.deleteSnapshot(pVmConfig, id, false);
		snapHelper.removeStateFiles(strVmFolder, id);
	}
}
