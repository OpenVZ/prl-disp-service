///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateVmSource.cpp
///
/// Source task for Vm migration
///
/// @author krasnov@
///
/// Copyright (c) 2010-2015 Parallels IP Holdings GmbH
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
/////////////////////////////////////////////////////////////////////////////////

//#define LOGGING_ON
//#define FORCE_LOGGING_LEVEL DBG_DEBUG

#include "Interfaces/Debug.h"
#include "Interfaces/ParallelsQt.h"
#include "Interfaces/ParallelsNamespace.h"

#include <prlcommon/Logging/Logging.h>
#include "Libraries/StatesStore/SavedStateTree.h"
#include <prlcommon/Std/PrlTime.h>

#include "CDspVmDirHelper.h"
#include "Task_CreateSnapshot.h"
#include "Task_MigrateVm.h"
#include "Task_CloneVm.h"
#include "Task_ChangeSID.h"
#include "CDspService.h"
#include "CDspBugPatcherLogic.h"
#include <prlcommon/Std/PrlAssert.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/DispToDispProtocols/CVmMigrationProto.h"
#ifdef _LIN_
#include "Libraries/Virtuozzo/CVzHelper.h"
#endif
#include "Libraries/StatesUtils/StatesHelper.h"
#include "Libraries/PrlCommonUtils/CVmMigrateHelper.h"
#include "XmlModel/Messaging/CVmEventParameterList.cpp"


/*
begin of shared code from Vm/CVmMigrateTask.cpp
*/

static PRL_RESULT CreateSharedFile(const QString &dir, QString &tmpFile)
{
	QTemporaryFile f(
		QString("%1/%2.XXXXXX")
			.arg(QDir::fromNativeSeparators(dir))
			.arg(Uuid::createUuid().toString())
		);

	if (!f.open())
	{
		WRITE_TRACE(DBG_FATAL,
			    "Failed to create/open temparary file %s for shared storage check",
			    QSTR2UTF8(f.fileName()));
		return PRL_ERR_OPERATION_FAILED;
	}

	f.setAutoRemove(false);
	tmpFile = f.fileName();

	return PRL_ERR_SUCCESS;
}

/*
end of shared code from Vm/CVmMigrateTask.cpp
*/

static void NotifyClientsWithProgress(
		const SmartPtr<IOPackage> &p,
		const QString &sVmDirectoryUuid,
		const QString &sVmUuid,
		int nPercents)
{
	CVmEvent event(PET_DSP_EVT_VM_MIGRATE_PROGRESS_CHANGED, sVmUuid, PIE_DISPATCHER);

	event.addEventParameter(new CVmEventParameter(
		PVE::UnsignedInt,
		QString::number(nPercents),
		EVT_PARAM_PROGRESS_CHANGED));

	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, event, p);
	CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage, sVmDirectoryUuid, sVmUuid);
}


Task_MigrateVmSource::Task_MigrateVmSource(
		const SmartPtr<CDspClient>& client,
		const CProtoCommandPtr cmd,
		const SmartPtr<IOPackage>& p)
:CDspTaskHelper(client, p),
Task_DispToDispConnHelper(getLastError()),
m_nRemoteVersion(MIGRATE_DISP_PROTO_V1),
m_pVmConfig(new CVmConfiguration()),
m_bNewVmInstance(false),
m_nSteps(0),
m_nTotalSize(0)
{
	CProtoVmMigrateCommand *pCmd = CProtoSerializer::CastToProtoCommand<CProtoVmMigrateCommand>(cmd);
	PRL_ASSERT(pCmd->IsValid());
	m_sVmUuid = pCmd->GetVmUuid();
	m_sVmDirUuid = getClient()->getVmDirectoryUuid();
	m_sServerHostname = pCmd->GetTargetServerHostname();
	m_nServerPort = pCmd->GetTargetServerPort();
	if (m_nServerPort == 0)
		m_nServerPort = CDspService::getDefaultListenPort();
	m_sServerSessionUuid = pCmd->GetTargetServerSessionUuid();
	m_sTargetServerVmName = pCmd->GetTargetServerVmName();
	m_sTargetServerVmHomePath = pCmd->GetTargetServerVmHomePath();
	m_nMigrationFlags = pCmd->GetMigrationFlags();
	/* clear migration type bits */
	m_nMigrationFlags &= ~PVMT_HOT_MIGRATION & ~PVMT_WARM_MIGRATION & ~PVMT_COLD_MIGRATION;
	m_nReservedFlags = pCmd->GetReservedFlags();
}

Task_MigrateVmSource::~Task_MigrateVmSource()
{
}

PRL_RESULT Task_MigrateVmSource::prepareTask()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;

	if (CDspService::instance()->isServerStopping())
	{
		WRITE_TRACE(DBG_FATAL, "Dispatcher shutdown is in progress - VM migrate rejected!");
		nRetCode = PRL_ERR_DISP_SHUTDOWN_IN_PROCESS;
		goto exit;
	}

	if (CDspService::instance()->getShellServiceHelper().isLocalAddress(m_sServerHostname)) {
		WRITE_TRACE(DBG_FATAL,
			"Host %s is a local host, migration is impossible", QSTR2UTF8(m_sServerHostname));
		nRetCode = PRL_ERR_VM_MIGRATE_TO_THE_SAME_NODE;
		goto exit;
	}

	/* check access for migration (for PVE::DspCmdDirVmMigrate, as for more strict) */
	nRetCode = CDspService::instance()->getAccessManager().checkAccess(
			getClient(), PVE::DspCmdDirVmMigrate, m_sVmUuid, NULL, getLastError());
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "[%s] Error occurred while CDspAccessManager::checkAccess() with code [%#x][%s]",
			__FUNCTION__, nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		goto exit;
	}

	{
		/* before CDspVm get instance, to select m_nRegisterCmd */
		/* LOCK inside brackets */
		CDspLockedPointer<CVmDirectoryItem> pVmDirItem =
			CDspService::instance()->getVmDirManager().getVmDirItemByUuid(m_sVmDirUuid, m_sVmUuid);
		if (!pVmDirItem) {
			nRetCode = PRL_ERR_VM_UUID_NOT_FOUND;
			WRITE_TRACE(DBG_FATAL, "Couldn't to find Vm with UUID '%s'", QSTR2UTF8(m_sVmUuid));
			goto exit;
		}
		m_sVmName = pVmDirItem->getVmName();
		m_sVmConfigPath = pVmDirItem->getVmHome();
		m_sVmHomePath = CFileHelper::GetFileRoot(m_sVmConfigPath);

		/* will load config with relative path */
		nRetCode = CDspService::instance()->getVmConfigManager().loadConfig(
					m_pVmConfig, pVmDirItem.getPtr()->getVmHome(), getClient(), false, true);
		if (PRL_FAILED(nRetCode)) {
			WRITE_TRACE(DBG_FATAL, "Vm %s config loading failed. Reason: %#x (%s)",
					QSTR2UTF8(m_sVmName), nRetCode, PRL_RESULT_TO_STRING(nRetCode));
			goto exit;
		}
	}
	/*
	 * Lets allow more then one simultaneous tasks for the same VM template migration in clone mode,
	 * as PACI want (https://jira.sw.ru/browse/PSBM-13328)
	 * Will use other command than PVE::DspCmdDirVmMigrate for registerExclusiveVmOperation() in this case.
	 */
	if ((PVMT_CLONE_MODE & getRequestFlags()))
		m_nRegisterCmd = PVE::DspCmdDirVmMigrateClone;
	else
		m_nRegisterCmd = PVE::DspCmdDirVmMigrate;

	m_pVm = CDspVm::CreateInstance(
		m_sVmUuid, m_sVmDirUuid, nRetCode, m_bNewVmInstance, getClient(), m_nRegisterCmd);
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "[%s] Error occurred while CDspVm::CreateInstance() with code [%#x][%s]",
			__FUNCTION__, nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		goto exit;
	}
	if (!m_pVm.getImpl()) {
		WRITE_TRACE(DBG_FATAL, "[%s] Unknown CDspVm::CreateInstance() error", __FUNCTION__);
		nRetCode = PRL_ERR_OPERATION_FAILED;
		goto exit;
	}
	if (!m_bNewVmInstance) {
		nRetCode = CDspService::instance()->getVmDirHelper().registerExclusiveVmOperation(
			m_sVmUuid, m_sVmDirUuid, m_nRegisterCmd, getClient());
		if ( PRL_FAILED(nRetCode) ) {
			WRITE_TRACE(DBG_FATAL, "[%s] registerExclusiveVmOperation failed. Reason: %#x (%s)",
				__FUNCTION__, nRetCode, PRL_RESULT_TO_STRING(nRetCode));
			goto exit;
		}
	}
	m_nSteps |= MIGRATE_VM_EXCL_PARAMS_LOCKED;

	m_nPrevVmState = m_pVm->getVmState();
	// template already in migrating stage, but original template state is stopped
	if (VMS_MIGRATING == m_nPrevVmState)
		m_nPrevVmState = VMS_STOPPED;

	//https://jira.sw.ru/browse/PSBM-4996
	//Check remote clone preconditions
	if ( PVMT_CLONE_MODE & getRequestFlags() )
	{
		//Check change SID preconditions
		if ( PVMT_CHANGE_SID & getRequestFlags() )
		{
			nRetCode = Task_CloneVm::CheckWhetherChangeSidOpPossible( m_pVmConfig, m_sVmDirUuid );
			if ( PRL_FAILED(nRetCode) )
				goto exit;
		}
	}

// VirtualDisk commented out by request from CP team
//	{
//		// to wait auto-copletion tasks (https://jira.sw.ru/browse/PSBM-15126)
//		CDSManager *pStatesManager = new CDSManager();
//		if (pStatesManager) {
//			nRetCode = CDspVmSnapshotStoreHelper::PrepareDiskStateManager(m_pVmConfig, pStatesManager);
//			if (PRL_FAILED(nRetCode))
//				WRITE_TRACE(DBG_FATAL,
//					"CDspVmSnapshotStoreHelper::PrepareDiskStateManager() failed : [%#x][%s]",
//					nRetCode, PRL_RESULT_TO_STRING(nRetCode));
//			else
//				pStatesManager->WaitForCompletion();
//			delete pStatesManager;
//		}
//	}

	{
		/* LOCK inside brackets */
		CDspLockedPointer<CDspHostInfo> lockedHostInfo = CDspService::instance()->getHostInfo();
		m_cHostInfo.fromString( lockedHostInfo->data()->toString() );
	}

	nRetCode = Connect(
		m_sServerHostname, m_nServerPort, m_sServerSessionUuid, QString(), QString(), m_nMigrationFlags);
	if (PRL_FAILED(nRetCode))
		goto exit;

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

PRL_RESULT Task_MigrateVmSource::run_body()
{
	SmartPtr<CVmEvent> pEvent;
	SmartPtr<IOPackage> pPackage;

	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	if (operationIsCancelled())
		setLastErrorCode(PRL_ERR_OPERATION_WAS_CANCELED);

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;

	if (PRL_FAILED(nRetCode = CheckVmMigrationPreconditions()))
		goto exit;

	// after CheckVmMigrationPreconditions() only
	if ((m_nReservedFlags & PVM_DONT_COPY_VM) && (PVMT_CLONE_MODE & getRequestFlags())) {
		// it's valid for templates only
		if (!m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate()) {
			WRITE_TRACE(DBG_FATAL, "Can't clone shared VM '%s'", QSTR2UTF8(m_sVmUuid));
			nRetCode = PRL_ERR_VM_MIGRATE_CANNOT_REMOTE_CLONE_SHARED_VM;
			goto exit;
		}
	}

	/* set migration mode */
	switch (m_nPrevVmState) {
	case VMS_RUNNING:
	case VMS_PAUSED:
		if (m_nRemoteVersion < MIGRATE_DISP_PROTO_V3) {
			WRITE_TRACE(DBG_FATAL, "[%s] Old Parallels Server on target (protocol version %d)."
				" Warm migration is not supported due to suspend/resume incompatibility,",
				__FUNCTION__, m_nRemoteVersion);
			nRetCode = PRL_ERR_VM_MIGRATE_WARM_MODE_NOT_SUPPORTED;
			goto exit;
		}
		m_nMigrationFlags |= PVMT_HOT_MIGRATION;
		break;
	default:
		m_nMigrationFlags |= PVMT_COLD_MIGRATION;
	}

	if (operationIsCancelled())
		setLastErrorCode(PRL_ERR_OPERATION_WAS_CANCELED);

	pEvent = SmartPtr<CVmEvent>(new CVmEvent(PET_DSP_EVT_VM_MIGRATE_STARTED, m_sVmUuid, PIE_DISPATCHER));
	pEvent->addEventParameter(new CVmEventParameter(PVE::Boolean, "true", EVT_PARAM_MIGRATE_IS_SOURCE));
	pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, pEvent->toString());
	/* change Vm state to VMS_MIGRATING */
	m_pVm->changeVmState(pPackage);
	m_nSteps |= MIGRATE_VM_STATE_CHANGED;
	/* and notify clients about VM migration start event */
	CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage, m_sVmDirUuid, m_sVmUuid);

	if ( !(PVMT_CLONE_MODE & getRequestFlags()) ) {
		/* remove target Vm config from watcher (#448235) */
		CDspService::instance()->getVmConfigWatcher().unregisterVmToWatch(m_sVmConfigPath);
		m_nSteps |= MIGRATE_UNREGISTER_VM_WATCH;
	}

	if ((m_nPrevVmState == VMS_RUNNING) || (m_nPrevVmState == VMS_PAUSED)) {
		nRetCode = migrateRunningVm();
	} else {
		nRetCode = migrateStoppedVm();
	}
	if (PRL_FAILED(nRetCode))
		goto exit;

	if (PRL_SUCCEEDED(nRetCode) && m_nRemoteVersion >= MIGRATE_DISP_PROTO_V3) {
		/* wait finish reply from target (https://jira.sw.ru/browse/PSBM-9596) */
		IOSendJob::Response pResponse;
		CDispToDispCommandPtr pCmd;
		CDispToDispResponseCommand *pRespCmd;
		quint32 nTimeout = m_nTimeout;

		/* wait target task finish */
		if (PVMT_CHANGE_SID & getRequestFlags())
			/* wait reply during changeSID task timeout (https://jira.sw.ru/browse/PSBM-9733) */
			nTimeout = CHANGESID_TIMEOUT;
		if (m_pIoClient->waitForResponse(m_hCheckReqJob, nTimeout) != IOSendJob::Success) {
			WRITE_TRACE(DBG_FATAL, "Finish acknowledgement receiving failure");
			nRetCode = PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
			goto exit;
		}
		pResponse = m_pIoClient->takeResponse(m_hCheckReqJob);
		if (pResponse.responseResult != IOSendJob::Success) {
			WRITE_TRACE(DBG_FATAL, "Finish acknowledgement receiving failure");
			nRetCode = PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
			goto exit;
		}
		pPackage  = pResponse.responsePackages[0];
		if (pPackage->header.type != DispToDispResponseCmd) {
			WRITE_TRACE(DBG_FATAL, "Invalid package type : %d", pPackage->header.type);
			nRetCode = PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
			goto exit;
		}

		pCmd = CDispToDispProtoSerializer::ParseCommand(
			DispToDispResponseCmd, UTF8_2QSTR(pPackage->buffers[0].getImpl()));
		pRespCmd = CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pCmd);
		nRetCode = pRespCmd->GetRetCode();
		if (PRL_FAILED(nRetCode))
			getLastError()->fromString(pRespCmd->GetErrorInfo()->toString());
	}

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

void Task_MigrateVmSource::releaseLocks()
{
	if (m_bNewVmInstance) {
		CDspVm::UnregisterVmObject(m_pVm);
		m_bNewVmInstance = false;
	} else if (m_nSteps & MIGRATE_VM_EXCL_PARAMS_LOCKED) {
		CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation(
				m_sVmUuid, m_sVmDirUuid, m_nRegisterCmd, getClient());
		m_nSteps &= ~MIGRATE_VM_EXCL_PARAMS_LOCKED;
	}

	/* Do not touch Vm config */
	if (m_pVm)
		m_pVm->disableStoreRunningState(true);

	// call destructor to unregister exclusive operaion (https://jira.sw.ru/browse/PSBM-13445)
	m_pVm = SmartPtr<CDspVm>(0);
}

void Task_MigrateVmSource::finalizeTask()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );
	SmartPtr<IOPackage> pPackage;
	CProtoCommandPtr pResponse =
		CProtoSerializer::CreateDspWsResponseCommand(getRequestPackage(), getLastErrorCode());

	Disconnect();

	if (PRL_SUCCEEDED(getLastErrorCode()))
	{
		QList< SmartPtr< CDspClient > > lstClient = CDspService::instance()->getClientManager().
				getSessionListByVm(m_sVmDirUuid, m_sVmUuid).values();

		if (CFileHelper::isSharedFS(m_sVmHomePath) && !(m_nReservedFlags & PVM_DONT_COPY_VM))
		{
			// handle only VM on shared FS - nfs, gfs, gfs2, pcs
			// for shared migration case target task already moved resource
			CDspService::instance()->getHaClusterHelper()->removeClusterResource(m_sVmName);
		}

		if (m_cSavFileCopy.exists())
			QFile::remove(m_cSavFileCopy.absoluteFilePath());
		if ((m_nSteps & MIGRATE_CONFIGSAV_BACKUPED) && m_sConfigSavBackup.size())
			QFile::remove(m_sConfigSavBackup);
		if (m_cLocalMemFile.exists()) {
			/* to remove origin mem file on success */
			QDir cMemFileDir = m_cLocalMemFile.absoluteDir();
			if (m_sVmUuid == cMemFileDir.dirName())
				CFileHelper::ClearAndDeleteDir(cMemFileDir.absolutePath());
			else
				QFile::remove(m_cLocalMemFile.absoluteFilePath());
		}

		/* prepare event for client before Vm removing */
		CVmEvent cEvent(PET_DSP_EVT_VM_MIGRATE_FINISHED, m_sVmUuid, PIE_DISPATCHER);
		pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, cEvent.toString());
		CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage, m_sVmDirUuid, m_sVmUuid);

		/* Lock VM to prevent race after delete CDspVm and before remove VM. */
		if ( !(PVMT_CLONE_MODE & getRequestFlags()) )
			CDspService::instance()->getVmDirHelper().
						lockVm(m_sVmUuid, getClient(), getRequestPackage());

		/* cleanup CDspVm object before remove VM from disk #PSBM-9753
		 * The CDspVm::cleanupObject use Vm config that will be removed.
		 */
		releaseLocks();

		if ( !(PVMT_CLONE_MODE & getRequestFlags()) )
		{
			CProtoCommandPtr pRequest = CProtoSerializer::CreateVmDeleteProtoCommand(m_sVmUuid, QStringList());
			SmartPtr<IOPackage> pDelPackage
				= DispatcherPackage::createInstance(PVE::DspCmdDirVmDelete, pRequest);

			CDspTaskFuture<Task_DeleteVm> pTask;
			SmartPtr<CDspClient> fakeClient = CDspClient::makeServiceUser( m_sVmDirUuid );
			if ( !(m_nReservedFlags & PVM_DONT_COPY_VM) )
			{
				WRITE_TRACE(DBG_INFO, "Delete VM '%s'", QSTR2UTF8(m_sVmName));
				pDelPackage->header.type = PVE::DspCmdDirVmDelete;

				pTask = CDspService::instance()->getVmDirHelper().
					unregOrDeleteVm( fakeClient, pDelPackage, m_pVmConfig->toString(),
					PVD_SKIP_VM_OPERATION_LOCK | PVD_SKIP_HA_CLUSTER);
			}
			else
			{
				WRITE_TRACE(DBG_INFO, "Unreg VM '%s'", QSTR2UTF8(m_sVmName));
				pDelPackage->header.type = PVE::DspCmdDirUnregVm;

				pTask = CDspService::instance()->getVmDirHelper().
					unregOrDeleteVm( fakeClient, pDelPackage, m_pVmConfig->toString(),
					PVD_UNREGISTER_ONLY | PVD_SKIP_VM_OPERATION_LOCK |
					PVD_NOT_MODIFY_VM_CONFIG | PVD_SKIP_HA_CLUSTER);
			}
			CVmEvent evt;
			pTask.wait().getResult(&evt);
			if (PRL_FAILED(evt.getEventCode()))
			{
				PRL_RESULT wcode = PRL_ERR_VM_MIGRATE_ERROR_UNREGISTER_VM;
				if ( !(m_nReservedFlags & PVM_DONT_COPY_VM) )
					wcode = PRL_ERR_VM_MIGRATE_ERROR_DELETE_VM;

				CVmEvent evt( PET_DSP_EVT_VM_MESSAGE, m_sVmName,
								PIE_DISPATCHER, wcode );

				SmartPtr<IOPackage> pWarn = DispatcherPackage::createInstance(
						PVE::DspVmEvent, evt );

				getClient()->sendPackage( pWarn );
			}

			// delete non-shared external disks
			PRL_RESULT wcode = PRL_ERR_SUCCESS;
			foreach (const QString &disk, m_lstNonSharedDisks) {
				WRITE_TRACE(DBG_INFO, "Deleting external disk '%s'", QSTR2UTF8(disk));
				if (!CFileHelper::ClearAndDeleteDir(disk))
					wcode = PRL_ERR_NOT_ALL_FILES_WAS_DELETED;
			}
			if (wcode != PRL_ERR_SUCCESS) {
				CVmEvent evt( PET_DSP_EVT_VM_MESSAGE, m_sVmName,
								PIE_DISPATCHER, wcode );

				SmartPtr<IOPackage> pWarn = DispatcherPackage::createInstance(
						PVE::DspVmEvent, evt );

				getClient()->sendPackage( pWarn );
			}

			CDspService::instance()->getVmDirHelper().
						unlockVm(m_sVmUuid, getClient(), getRequestPackage());
		}
	}
	else
	{
		if (operationIsCancelled())
			setLastErrorCode(PRL_ERR_OPERATION_WAS_CANCELED);

		PRL_EVENT_TYPE evtType;

		if (m_nSteps & MIGRATE_UNREGISTER_VM_WATCH)
			CDspService::instance()->getVmConfigWatcher().registerVmToWatch(
					m_sVmConfigPath, m_sVmDirUuid, m_sVmUuid);

		if (m_cSavFileCopy.exists())
			QFile::remove(m_cSavFileCopy.absoluteFilePath());
		if (m_nSteps & MIGRATE_CONFIGSAV_BACKUPED) {
			/* restore backuped config.sav on failure */
			if (m_cSavFile.exists())
				QFile::remove(m_cSavFile.absoluteFilePath());
			if (m_sConfigSavBackup.size())
				QFile::rename(m_sConfigSavBackup, m_cSavFile.absoluteFilePath());
		}
		CVmEvent cEvent(PET_DSP_EVT_VM_MIGRATE_CANCELLED, m_sVmUuid, PIE_DISPATCHER);
		pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, cEvent.toString());
		CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage, m_sVmDirUuid, m_sVmUuid);
		if (m_nSteps & MIGRATE_VM_STATE_CHANGED)
		{
			/* restore Vm previous state */
			switch (m_nPrevVmState)
			{
			case VMS_RUNNING:
				evtType = PET_DSP_EVT_VM_STARTED;
				break;
			case VMS_PAUSED:
				evtType = PET_DSP_EVT_VM_PAUSED;
				break;
			case VMS_SUSPENDED:
				evtType = PET_DSP_EVT_VM_SUSPENDED;
				break;
			case VMS_STOPPED:
			default:
				evtType = PET_DSP_EVT_VM_STOPPED;
				break;
			}
			CVmEvent cStateEvent(evtType, m_sVmUuid, PIE_DISPATCHER);
			SmartPtr<IOPackage> pUpdateVmStatePkg =
				DispatcherPackage::createInstance( PVE::DspVmEvent, cStateEvent.toString());
			if (m_pVm.getImpl())
				m_pVm->changeVmState(pUpdateVmStatePkg);
			CDspService::instance()->getClientManager().
					sendPackageToVmClients(pUpdateVmStatePkg, m_sVmDirUuid, m_sVmUuid);
		}

		releaseLocks();

		// fill response
		CProtoCommandDspWsResponse *wsResponse =
			CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pResponse );
		if (getLastErrorCode() == PRL_ERR_VM_MIGRATE_CHECKING_PRECONDITIONS_FAILED)
			wsResponse->SetParamsList( m_lstCheckPrecondsErrors );
		getLastError()->setEventCode(getLastErrorCode());
		wsResponse->SetError(getLastError()->toString());
	}

	foreach (const QString &file, m_lstAllCheckFiles)
		CFileHelper::RemoveEntry(file, &getClient()->getAuthHelper());

	getClient()->sendResponse(pResponse, getRequestPackage());
}

// cancel command
void Task_MigrateVmSource::cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& pkg)
{
	SmartPtr<IOPackage> p;
	WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);

	if (!p.isValid()) {
		// CDspTaskManager destructor call cancelOperation() for task with dummy package
		CProtoCommandPtr pCmd = CProtoSerializer::CreateProtoBasicVmCommand(
					PVE::DspCmdVmMigrateCancel, m_sVmUuid);
		p = DispatcherPackage::createInstance(PVE::DspCmdVmMigrateCancel, pCmd);
	} else {
		p = pkg;
	}
	/* cancel VmMigrationTask in vm_app */
	if (m_nSteps & MIGRATE_VM_APP_STARTED)
		m_pVm->sendPackageToVm( p );

	if (m_pVmCopySource)
		m_pVmCopySource->cancelOperation();

	CancelOperationSupport::cancelOperation(pUser, p);
}

PRL_RESULT Task_MigrateVmSource::migrateRunningVm()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	CVmEventParameter *pEventParam;
	SmartPtr<IOPackage> p;
	SmartPtr<IOPackage> respPkg;
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(getRequestPackage());
	IOSendJob::Handle hJob;
	IOSendJob::Result nResult;
	IOSendJob::Response pResponse;

	if (m_sSnapshotUuid.size())
		// Insert to migrate VM package additional snapshot uuid field
		pCmd->GetCommand()->addEventParameter(
			new CVmEventParameter(PVE::String, m_sSnapshotUuid, EVT_PARAM_SNAPSHOT_UUID));
	/* reset migration flags (migration mode) */
	pEventParam = pCmd->GetCommand()->getEventParameter(EVT_PARAM_PROTO_CMD_FLAGS);
	PRL_ASSERT(pEventParam);
	if (pEventParam)
		pEventParam->setParamValue(QString("%1").arg(m_nMigrationFlags));
	/* reset reserved flag (PVM_DONT_COPY_VM) */
	pEventParam = pCmd->GetCommand()->getEventParameter(EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS);
	PRL_ASSERT(pEventParam);
	if (pEventParam)
		pEventParam->setParamValue(QString("%1").arg(m_nReservedFlags));

	pCmd->GetCommand()->addEventParameter(
		new CVmEventParameter(PVE::String, QString("%1").arg(m_nTimeout), EVT_PARAM_SEND_RECEIVE_TIMEOUT));
	PRL_UINT32 nConnTimeout = (PRL_UINT32)
		CDspService::instance()->getDispConfigGuard().getDispToDispPrefs()->getConnectionTimeout() * 1000;
	pCmd->GetCommand()->addEventParameter(
		new CVmEventParameter(PVE::String, QString("%1").arg(nConnTimeout), EVT_PARAM_CONNECTION_TIMEOUT));

	pCmd->GetCommand()->addEventParameter( new CVmEventParameterList(PVE::String, m_lstNonSharedDisks,
			EVT_PARAM_MIGRATE_CMD_SHARED_FILE_NAMES_EXTRA));

	p = DispatcherPackage::duplicateInstance(getRequestPackage(), pCmd->GetCommand()->toString());

	hJob = m_pVm->sendPackageToVmEx(
		m_pVm->CreateVmMigrationPackageWithAdditionalInfo(pCmd, p, m_nPrevVmState), m_nPrevVmState);
	if (!hJob.isValid()) {
		WRITE_TRACE(DBG_FATAL, "Unable to send start migration command to VM %s", QSTR2UTF8(m_sVmUuid));
		return PRL_ERR_OPERATION_FAILED;
	}
	m_nSteps |= MIGRATE_VM_APP_STARTED;
	while (true) {
		if (operationIsCancelled()) {
			setLastErrorCode(PRL_ERR_OPERATION_WAS_CANCELED);
			break;
		}

		nResult = CDspService::instance()->getIOServer().waitForResponse( hJob );
		if (nResult != IOSendJob::Success)
		{
			WRITE_TRACE(DBG_FATAL, "Unable to wait responce from Vm %s, waitForResponse() retcode %#x",
				QSTR2UTF8(m_sVmUuid), nResult);
			return PRL_ERR_OPERATION_FAILED;
		}

		pResponse = CDspService::instance()->getIOServer().takeResponse( hJob );
		if ( pResponse.responseResult != IOSendJob::Success )
		{
			WRITE_TRACE(DBG_FATAL, "Unable to take response for VM %s, takeResponse() result is %#x",
				QSTR2UTF8(m_sVmUuid), pResponse.responseResult);
			return PRL_ERR_OPERATION_FAILED;
		}

		foreach(respPkg, pResponse.responsePackages) {
			if (respPkg->header.type == PVE::DspVmEvent) {
				// ignore events (progress, etc)
				continue;
			} else if (respPkg->header.type == PVE::DspWsResponse) {
				goto finish;
			} else {
				WRITE_TRACE(DBG_FATAL, "Unexpected package with type %d", respPkg->header.type);
			}
		}
	}
finish:

	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, UTF8_2QSTR(respPkg->buffers[0].getImpl()));
	CProtoCommandDspWsResponse *pResponseCmd =
		CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	if (PRL_FAILED(nRetCode = pResponseCmd->GetRetCode()))
		return nRetCode;

	if (m_nMigrationFlags & PVMT_CLONE_MODE)
		return nRetCode;

	/* stop Vm */
	pCmd = CProtoSerializer::CreateProtoVmCommandStop(m_sVmUuid, PSM_KILL, 0);
	p = DispatcherPackage::createInstance( PVE::DspCmdVmStop, pCmd );
	m_pVm->stop(getClient(), p, PSM_KILL, true);
	//Wait until VM stopped
	while (m_pVm->isConnected())
		QThread::msleep(1000);

	return nRetCode;
}

/*
   If local and remote memory file pathes are differ,
   create config.sav copy and rewrite memory file path in it.
   This copy will send to target node.
*/
PRL_RESULT Task_MigrateVmSource::fixConfigSav(const QString &sMemFilePath, QList<QPair<QFileInfo, QString> > &list)
{
	if (!m_cSavFile.exists()) {
		WRITE_TRACE(DBG_FATAL, "File \"%s\" is not found",
				QSTR2UTF8(m_cSavFile.absoluteFilePath()));
		return PRL_ERR_OPERATION_FAILED;
	}
	m_cSavFileCopy.setFile(m_cSavFile.absoluteFilePath() + ".migrate");
	m_sConfigSavBackup = m_cSavFileCopy.absoluteFilePath() + ".backup";
	QFile::remove(m_cSavFileCopy.absoluteFilePath());
	if (!QFile::copy(m_cSavFile.absoluteFilePath(), m_cSavFileCopy.absoluteFilePath())) {
		WRITE_TRACE(DBG_FATAL, "QFile::copy(\"%s\", \"%s\") failed",
			QSTR2UTF8(m_cSavFile.absoluteFilePath()), QSTR2UTF8(m_cSavFileCopy.absoluteFilePath()));
		return PRL_ERR_OPERATION_FAILED;
	}

	if ((QFileInfo(sMemFilePath) != QFileInfo(m_sTargetMemFilePath))) {
		/* rewrote config.sav with new memfile path */
		if (!CStatesHelper::writeMemFilePath(m_cSavFileCopy.absoluteFilePath(), m_sTargetMemFilePath)) {
			WRITE_TRACE(DBG_FATAL, "Can't save memory file path at \"%s\"",
					QSTR2UTF8(m_cSavFileCopy.absoluteFilePath()));
			QFile::remove(m_cSavFileCopy.absoluteFilePath());
			return PRL_ERR_OPERATION_FAILED;
		}
	}

	if (m_nReservedFlags & PVM_DONT_COPY_VM) {
		/* For shared Vm replace config.sav file by new with backup copy */
		QFile::rename(m_cSavFile.absoluteFilePath(), m_sConfigSavBackup);
		QFile::rename(m_cSavFileCopy.absoluteFilePath(), m_cSavFile.absoluteFilePath());
		m_nSteps |= MIGRATE_CONFIGSAV_BACKUPED;
	} else {
		/* and replace in list config.sav to config.sav.migrate */
		for (int i = 0; i < list.size(); ++i) {
			if (list.at(i).first.absoluteFilePath() == m_cSavFile.absoluteFilePath()) {
				list[i].first = m_cSavFileCopy.absoluteFilePath();
				return PRL_ERR_SUCCESS;
			}
		}
		QDir dir(m_sVmHomePath);
		list.append(
			qMakePair(m_cSavFileCopy, dir.relativeFilePath(m_cSavFile.absoluteFilePath())));
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_MigrateVmSource::migrateStoppedVm()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	int i;
	QList<QPair<QFileInfo, QString> > dList;
	QList<QPair<QFileInfo, QString> > fList;

#if _LIN_
	CVzHelper vz;
	if (vz.set_vziolimit("VZ_TOOLS"))
		WRITE_TRACE(DBG_FATAL, "Warning: Ignore IO limit parameters");
#endif

	if ( !(m_nReservedFlags & PVM_DONT_COPY_VM) ) {
		/* get full directories and files lists for migration */
		if (PRL_FAILED(nRetCode = CVmMigrateHelper::GetEntryListsVmHome(m_sVmHomePath, dList, fList)))
			return nRetCode;
	}

	if (PRL_FAILED(nRetCode = CVmMigrateHelper::GetEntryListsExternal(m_lstNonSharedDisks, dList, fList)))
		return nRetCode;

	if (PRL_FAILED(nRetCode = SendStartRequest()))
		return nRetCode;

	if (m_nPrevVmState == VMS_SUSPENDED) {
		QString sMemFileName, sMemFilePath;
		QDir dir(m_sVmHomePath);
		QFileInfo cConfig(dir, VMDIR_DEFAULT_VM_CONFIG_FILE);
		CStatesHelper cStatesHelper(cConfig.absoluteFilePath());
		m_cSavFile.setFile(dir, cStatesHelper.getSavFileName());

		if (!CStatesHelper::extractMemFileName(m_cSavFile.absoluteFilePath(), sMemFileName))
		{
			WRITE_TRACE(DBG_FATAL, "Can not get memory file name from \"%s\"",
					QSTR2UTF8(m_cSavFile.absoluteFilePath()));
			return PRL_ERR_OPERATION_FAILED;
		}
		/* if memfile path is absent or is empty, it's a local path */
		CStatesHelper::extractMemFilePath(m_cSavFile.absoluteFilePath(), sMemFilePath);

		/* memfile is place in Vm home and Vm home is shared == memfile is shared */
		if (!(m_nReservedFlags & PVM_DONT_COPY_VM) || !sMemFilePath.isEmpty())
		{
			/* rewrite memfile path in list */
			QString sRemoteMemFile = m_sTargetMemFilePath.isEmpty() ? sMemFileName :
				QFileInfo(m_sTargetMemFilePath, sMemFileName).absoluteFilePath();
			m_cLocalMemFile.setFile(
				QDir(sMemFilePath.isEmpty() ? m_sVmHomePath : sMemFilePath), sMemFileName);
			bool bFound = false;
			for (int i = 0; i < fList.size(); ++i) {
				if (fList.at(i).first.absoluteFilePath() == m_cLocalMemFile.absoluteFilePath()) {
					fList[i].second = sRemoteMemFile;
					bFound = true;
					break;
				}
			}
			if (!bFound)
				fList.append(qMakePair(m_cLocalMemFile, sRemoteMemFile));

			if ((QFileInfo(sMemFilePath) != QFileInfo(m_sTargetMemFilePath)))
				if (PRL_FAILED(nRetCode = fixConfigSav(sMemFilePath, fList)))
					return nRetCode;
		}
	}

	/* calculate total transaction size */
	for (i = 0; i < fList.size(); ++i)
		m_nTotalSize += fList.at(i).first.size();

	m_pSender = SmartPtr<CVmFileListCopySender>(new CVmFileListCopySenderClient(m_pIoClient));
	m_pVmCopySource = SmartPtr<CVmFileListCopySource>(
			new CVmFileListCopySource(
				m_pSender.getImpl(),
				m_pVmConfig->getVmIdentification()->getVmUuid(),
				m_sVmHomePath,
				m_nTotalSize,
				getLastError(),
				m_nTimeout));
	m_pVmCopySource->SetRequest(getRequestPackage());
	m_pVmCopySource->SetVmDirectoryUuid(m_sVmDirUuid);
	m_pVmCopySource->SetProgressNotifySender(NotifyClientsWithProgress);

	nRetCode = m_pVmCopySource->Copy(dList, fList);
	if (PRL_FAILED(nRetCode))
		WRITE_TRACE(DBG_FATAL, "Error occurred while migration with code [%#x][%s]",
			nRetCode, PRL_RESULT_TO_STRING(nRetCode));
	return nRetCode;
}

/* check that image file of devive is place into Vm bundle */
void Task_MigrateVmSource::DisconnectExternalImageDevice(CVmDevice* pDevice)
{
	QFileInfo fInfo(pDevice->getSystemName());

	/* skip disabled and disconnected devices */
	if (PVE::DeviceDisabled == pDevice->getEnabled())
		return;
	if (PVE::DeviceDisconnected == pDevice->getConnected())
		return;

	if (fInfo.isRelative())
		return;

	if (fInfo.absoluteFilePath().startsWith(m_sVmHomePath))
		return;

	WRITE_TRACE(DBG_FATAL, "[%s] Image file %s of device %s is place out of Vm bundle %s",
			__FUNCTION__,
			QSTR2UTF8(pDevice->getUserFriendlyName()),
			QSTR2UTF8(pDevice->getSystemName()),
			QSTR2UTF8(m_sVmHomePath));
	pDevice->setConnected(PVE::DeviceDisconnected);
}

/* return PRL_ERR_SUCCESS in case of success checking, result no matter
*	  error - if can't check
*/
PRL_RESULT Task_MigrateVmSource::CheckVmDevices()
{
	CVmHardware* pVmHardware = m_pVmConfig->getVmHardwareList();

	if (pVmHardware == NULL) {
		WRITE_TRACE(DBG_FATAL, "[%s] Can not get Vm hardware list", __FUNCTION__);
		return PRL_ERR_OPERATION_FAILED;
	}

	foreach(CVmOpticalDisk* disk, pVmHardware->m_lstOpticalDisks) {
		if (NULL == disk)
			continue;
		if (disk->isRemote() && (PVE::DeviceEnabled == disk->getEnabled()) &&
					(PVE::DeviceDisconnected != disk->getConnected()))
		{
			WRITE_TRACE(DBG_FATAL, "ERROR: Connected remote device");
			CVmEvent _error;
			_error.setEventCode(PRL_ERR_VM_MIGRATE_REMOTE_DEVICE_IS_ATTACHED);
			_error.addEventParameter(new CVmEventParameter(PVE::String,
					disk->getSystemName(),
					EVT_PARAM_MESSAGE_PARAM_0));
			m_lstCheckPrecondsErrors.append(_error.toString());
		}
	}

	QList<CVmDevice*> lstPciDevices = *((QList<CVmDevice*>* )&pVmHardware->m_lstGenericPciDevices);
	lstPciDevices += *((QList<CVmDevice*>* )&pVmHardware->m_lstNetworkAdapters);
	lstPciDevices += *((QList<CVmDevice*>* )&pVmHardware->m_lstPciVideoAdapters);

	for(int i = 0; i < lstPciDevices.size(); i++)
	{
		CVmDevice* pPciDev = lstPciDevices[i];
		if ( pPciDev->getEnabled() != PVE::DeviceEnabled )
			continue;
		if ( pPciDev->getDeviceType() == PDE_GENERIC_NETWORK_ADAPTER
			&& pPciDev->getEmulatedType() != PDT_USE_DIRECT_ASSIGN )
			continue;

		WRITE_TRACE(DBG_DEBUG, "ERROR: Connected VTd device");
		CVmEvent _error;
		_error.setEventCode(PRL_ERR_VM_MIGRATE_REMOTE_DEVICE_IS_ATTACHED);
		_error.addEventParameter(new CVmEventParameter(PVE::String,
				pPciDev->getUserFriendlyName(),
				EVT_PARAM_MESSAGE_PARAM_0));
		m_lstCheckPrecondsErrors.append(_error.toString());
	}

	/* check optical & hard disks: is it images? */
	QList< CVmMassStorageDevice* > disks;
	foreach(CVmHardDisk* disk, pVmHardware->m_lstHardDisks)
		disks.append(disk);
	foreach(CVmOpticalDisk* dvd, pVmHardware->m_lstOpticalDisks)
		disks.append(dvd);
	foreach(CVmMassStorageDevice* pDevice, disks) {
		if ((_PRL_VM_DEV_EMULATION_TYPE)pDevice->getEmulatedType() == PDT_USE_REAL_DEVICE) {
			/* skip disabled and disconnected devices */
			if (PVE::DeviceDisabled == pDevice->getEnabled())
				continue;
			if (PVE::DeviceDisconnected == pDevice->getConnected())
				continue;

			WRITE_TRACE(DBG_FATAL, "[%s] Device %s has inappropriate emulated type %d",
				__FUNCTION__, QSTR2UTF8(pDevice->getUserFriendlyName()), pDevice->getEmulatedType());
			CVmEvent event;
			event.setEventCode(PRL_ERR_VM_MIGRATE_INVALID_DISK_TYPE);
			event.addEventParameter(new CVmEventParameter(PVE::String,
					m_sVmName,
					EVT_PARAM_MESSAGE_PARAM_0));
			event.addEventParameter(new CVmEventParameter(PVE::String,
					pDevice->getSystemName(),
					EVT_PARAM_MESSAGE_PARAM_1));
		} else if (pDevice->getDeviceType() != PDE_HARD_DISK) {
			DisconnectExternalImageDevice(pDevice);
		}
	}

	/* process floppy disk images */
	foreach (CVmFloppyDisk* pDevice, pVmHardware->m_lstFloppyDisks) {
		if (pDevice->getEmulatedType() != PVE::FloppyDiskImage)
			continue;
		DisconnectExternalImageDevice(pDevice);
	}

	/* process serial ports output files */
	foreach (CVmSerialPort* pDevice, pVmHardware->m_lstSerialPorts) {
		if (pDevice->getEmulatedType() != PVE::SerialOutputFile)
			continue;
		DisconnectExternalImageDevice(pDevice);
	}

	/* process parallel ports output files */
	foreach (CVmParallelPort* pDevice, pVmHardware->m_lstParallelPorts) {
		if (pDevice->getEmulatedType() != PVE::ParallelOutputFile)
			continue;
		DisconnectExternalImageDevice(pDevice);
	}

	return PRL_ERR_SUCCESS;
}

/*
* Check preconditions
* return PRL_ERR_SUCCESS - no errors
* 	 PRL_ERR_VM_MIGRATE_CHECKING_PRECONDITIONS_FAILED - precondition error
*	 other - internal error
*/
PRL_RESULT Task_MigrateVmSource::CheckVmMigrationPreconditions()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;

	if (PRL_FAILED(nRetCode = CheckVmDevices()))
		return nRetCode;

	if (m_lstCheckPrecondsErrors.size())
		return PRL_ERR_VM_MIGRATE_CHECKING_PRECONDITIONS_FAILED;

	// Get size of VM directory
	PRL_UINT64 nRequiresDiskSpace;
	nRetCode = CSimpleFileHelper::GetDirSize(m_sVmHomePath, &nRequiresDiskSpace);
	if ( PRL_FAILED(nRetCode) ) {
		nRequiresDiskSpace = 0ULL;
		WRITE_TRACE(DBG_FATAL, "CSimpleFileHelper::GetDirSize(%s) return %#x[%s], disk space check will skipped",
				QSTR2UTF8(m_sVmHomePath), nRetCode, PRL_RESULT_TO_STRING(nRetCode));
	}


	// Create temporary file in home to check if its shared between source
	// and destination
	QStringList extCheckFiles;
	QString tmpFile, homeCheckFile;
	if (PRL_FAILED(nRetCode = CreateSharedFile(m_sVmHomePath, tmpFile)))
		return nRetCode;
	// relative path for home checkfile
	homeCheckFile = QFileInfo(tmpFile).fileName();
	m_lstAllCheckFiles.append(tmpFile);

	// Create temporary file in each external disk directory for the same
	// purpose
	foreach(const CVmHardDisk* disk, m_pVmConfig->getVmHardwareList()->m_lstHardDisks) {
		if (NULL == disk)
			continue;
		QFileInfo img(disk->getSystemName());
		if (img.isAbsolute() && !img.absoluteFilePath().startsWith(m_sVmHomePath)) {
			if (PRL_FAILED(nRetCode = CreateSharedFile(img.absoluteFilePath(), tmpFile)))
				return nRetCode;
			// absolute path is passed for external disks check file
			extCheckFiles.append(tmpFile);
			m_lstAllCheckFiles.append(tmpFile);
		}
	}
	QString sHaClusterId;
	// try to get HA cluster ID for shared migration
	nRetCode = CDspService::instance()->getHaClusterHelper()->getHaClusterID(sHaClusterId);
	if (PRL_FAILED(nRetCode))
		return nRetCode;

	//2. Send check preconditions request to the target dispatcher
	CDispToDispCommandPtr pRequest =
		CDispToDispProtoSerializer::CreateVmMigrateCheckPreconditionsCommand(
			m_pVmConfig->toString(),
			m_cHostInfo.toString(),
			m_sTargetServerVmName,
			m_sTargetServerVmHomePath,
			homeCheckFile,
			extCheckFiles,
			sHaClusterId,
			nRequiresDiskSpace,
			m_nMigrationFlags,
			m_nReservedFlags,
			m_nPrevVmState
		);

	SmartPtr<IOPackage> pPackage =
		DispatcherPackage::createInstance(pRequest->GetCommandId(), pRequest->GetCommand()->toString());
	SmartPtr<IOPackage> pReply;
	if (PRL_FAILED(nRetCode = SendReqAndWaitReply(pPackage, pReply, m_hCheckReqJob)))
		return nRetCode;

	if ((pReply->header.type != DispToDispResponseCmd) && (pReply->header.type != VmMigrateCheckPreconditionsReply))
	{
		WRITE_TRACE(DBG_FATAL, "Unexpected response type (%d)", pReply->header.type);
		return PRL_ERR_OPERATION_FAILED;
	}

	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(
			DispToDispResponseCmd,
			UTF8_2QSTR(pReply->buffers[0].getImpl())
		);

	QStringList lstErrors;
	/*
	for protocol v1 we can get DispToDispResponseCmd only
	for protocol v2 and later we can get VmMigrateCheckPreconditionsReply and DispToDispResponseCmd (on failure)
	*/
	if (pReply->header.type == DispToDispResponseCmd) {
		CDispToDispResponseCommand *pResponseCmd =
			CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pCmd);
		nRetCode = pResponseCmd->GetRetCode();
		if (nRetCode == PRL_ERR_VM_MIGRATE_CHECKING_PRECONDITIONS_FAILED) {
			lstErrors = pResponseCmd->GetParams();
		} else if (PRL_FAILED(nRetCode)) {
			getLastError()->fromString(pResponseCmd->GetErrorInfo()->toString());
			return nRetCode;
		}
	} else {
		CVmMigrateCheckPreconditionsReply *pResponseCmd =
			CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateCheckPreconditionsReply>(pCmd);
		lstErrors = pResponseCmd->GetCheckPreconditionsResult();
		m_nRemoteVersion = pResponseCmd->GetVersion();
		m_nReservedFlags = pResponseCmd->GetCommandFlags();
		if (m_nRemoteVersion >= MIGRATE_DISP_PROTO_V4)
			m_lstNonSharedDisks = pResponseCmd->GetNonSharedDisks();
	}

	if (!lstErrors.isEmpty()) {
		//Let analyse them
		m_lstCheckPrecondsErrors.clear();
		foreach(QString sError, lstErrors)
		{
			CVmEvent _error(sError);
			if (_error.getEventCode() == PRL_INFO_VM_MIGRATE_STORAGE_IS_SHARED)
			{
				m_nReservedFlags |= PVM_DONT_COPY_VM;

				if (!m_pVmConfig->getVmSettings()->getHighAvailability()->isEnabled())
					continue;

				QString ha;
				CDspService::instance()->getHaClusterHelper()->getHaClusterID(ha);
				m_nReservedFlags |= ((!ha.isEmpty()) * PVM_HA_MOVE_VM);
				nRetCode = PRL_ERR_SUCCESS;
				continue;
			}

			if (_error.getEventCode() == PRL_ERR_VM_MIGRATE_NON_COMPATIBLE_CPU_ON_TARGET)
			{
#if 0			//Currently clients don't support migration & asking question. Temprorarily disable it.
				//We can't apply warm or hot migration when source and target CPUs aren't compatible
				if (m_nMigrationType != PVMT_COLD_MIGRATION)
				{
					CVMController::VmQuestionChoices choices;
					choices.push_back(PET_ANSWER_OK);
					choices.push_back(PET_ANSWER_CANCEL);

					CVMController::VmNamedParams params;
					params.addParameter(
						m_pVmConfig->getVmIdentification()->getVmName(),
						CVmParamNames::Param0
					);
					if ( m_bForceQuestionSign ||
						GetVmController()->vmSendQuestionEvent(
							PET_QUESTION_CHANGE_VM_MIGRATION_TYPE,
							choices,
							params
						) == PET_ANSWER_OK)
					{
						m_nMigrationType = PVMT_COLD_MIGRATION;
						/*
						TODO : stop Vm
						m_nPrevVmState = VMS_STOPPED;
						and continue in cold mode
						*/
						if (lstErrors.size() == 1)//Just one error with CPUs incompatibility
							ret = PRL_ERR_SUCCESS;//so ignore it
						continue;
					}
				}
				else//Migration type is already valid - just ignore checking preconditions error
#endif
				if (	(VMS_STOPPED ==  m_nPrevVmState) ||
					(m_nMigrationFlags & PVM_DONT_RESUME_VM))
				{
					// error with CPUs incompatibility
					nRetCode = PRL_ERR_SUCCESS;//so ignore it
					// If another error will be encountered in this loop, it will be added
					// to m_lstCheckPrecondsErrors and checked below...
					continue;
				}
			}
			m_lstCheckPrecondsErrors.append(sError);
		} // foreach
		if (m_lstCheckPrecondsErrors.size())
			nRetCode = PRL_ERR_VM_MIGRATE_CHECKING_PRECONDITIONS_FAILED;
	}

	if (!extCheckFiles.empty() && (m_nRemoteVersion < MIGRATE_DISP_PROTO_V4)) {
		WRITE_TRACE(DBG_FATAL, "[%s] Old Parallels Server on target (protocol version %d)."
			"Migration with external disks is not supported on remote side.",
			__FUNCTION__, m_nRemoteVersion);
		return PRL_ERR_VM_MIGRATE_EXTERNAL_DISKS_NOT_SUPPORTED;
	}

	return nRetCode;
}

/* To send start request to remote dispatcher and wait reply */
PRL_RESULT Task_MigrateVmSource::SendStartRequest()
{
	PRL_RESULT nRetCode;
	CDispToDispCommandPtr pCmd;
	QFileInfo vmBundle(m_sVmHomePath);
	QFileInfo vmConfig(m_sVmConfigPath);

	CDispToDispCommandPtr pMigrateStartCmd = CDispToDispProtoSerializer::CreateVmMigrateStartCommand(
				m_pVmConfig->toString(),
				QString(), // we can't get runtime config here - do it on VM level
				m_sTargetServerVmHomePath,
				m_sSnapshotUuid,
				vmBundle.permissions(),
				vmConfig.permissions(),
				m_nMigrationFlags,
				m_nReservedFlags,
				m_nPrevVmState);

	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(
			pMigrateStartCmd->GetCommandId(),
			pMigrateStartCmd->GetCommand()->toString());
	SmartPtr<IOPackage> pReply;

	if (PRL_FAILED(nRetCode = SendReqAndWaitReply(pPackage, pReply)))
		return nRetCode;

	if ((pReply->header.type != DispToDispResponseCmd) && (pReply->header.type != VmMigrateReply)) {
		WRITE_TRACE(DBG_FATAL, "Unexpected response type on migration start command: %d", pReply->header.type);
		return PRL_ERR_OPERATION_FAILED;
	}

	pCmd = CDispToDispProtoSerializer::ParseCommand(
			(Parallels::IDispToDispCommands)pReply->header.type,
			UTF8_2QSTR(pReply->buffers[0].getImpl()));

	if (pReply->header.type == DispToDispResponseCmd) {
		CDispToDispResponseCommand *pResponseCmd =
			CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pCmd);
		getLastError()->fromString(pResponseCmd->GetErrorInfo()->toString());
		return pResponseCmd->GetRetCode();
	}
	CVmMigrateReply *pMigrateReply = CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateReply>(pCmd);
	m_sTargetMemFilePath = pMigrateReply->GetMemFilePath();

	return PRL_ERR_SUCCESS;
}

