///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateCt_win.cpp
///
/// Dispatcher task for VZWIN CT migration
///
/// @author yur@
///
/// Copyright (c) 2011-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////////

//#define LOGGING_ON
//#define FORCE_LOGGING_LEVEL DBG_DEBUG

#include "Interfaces/Debug.h"
#include <prlcommon/Interfaces/ParallelsQt.h>
#include <prlcommon/Interfaces/ParallelsNamespace.h>

#include <prlcommon/Logging/Logging.h>
#include <prlcommon/Std/PrlTime.h>
#include <prlcommon/Std/PrlAssert.h>

#include "CDspService.h"
#include "CDspVzHelper.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/StatesUtils/StatesHelper.h"
#include "Libraries/DispToDispProtocols/CVmMigrationProto.h"
#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"
#include "Libraries/Virtuozzo/CVzHelper.h"
#include "Task_CloneVm.h"

#include "Task_MigrateCt_win.h"

#define VM_MIGRATE_START_CMD_WAIT_TIMEOUT 600 * 1000
#define CHANGESID_TIMEOUT	(60 * 60 * 1000)

enum _PRL_VM_MIGRATE_STEP {
	MIGRATE_UNREGISTER_VM_WATCH	= (1 << 0),
	MIGRATE_SUSPENDED_VM		= (1 << 1),
	MIGRATE_VM_EXCL_PARAMS_LOCKED	= (1 << 2),
	MIGRATE_CONFIGSAV_BACKUPED	= (1 << 3),
	MIGRATE_VM_APP_STARTED		= (1 << 4),
	MIGRATE_VM_STATE_CHANGED	= (1 << 5),
	MIGRATE_STARTED			= (1 << 6),
};


/*
begin of shared code from Vm/CVmMigrateTask.cpp
*/

static PRL_RESULT GetEntryLists(
		const QString & aVmHomeDir,
		QList<QPair<QFileInfo, QString> > & dirList,
		QList<QPair<QFileInfo, QString> > & fileList)
{
	QString VmHomeDir = QFileInfo(aVmHomeDir).absoluteFilePath();
	QFileInfo dirInfo;
	QFileInfoList entryList;
	QDir dir;
	QDir startDir(VmHomeDir);
	int i, j;
	QFileInfo config, config_backup, log, statlog;

	config.setFile(VmHomeDir, VMDIR_DEFAULT_VM_CONFIG_FILE);
	config_backup.setFile(VmHomeDir, VMDIR_DEFAULT_VM_CONFIG_FILE VMDIR_DEFAULT_VM_BACKUP_SUFFIX);
	log.setFile(VmHomeDir, "parallels.log");
	statlog.setFile(VmHomeDir, PRL_VMTIMING_LOGFILENAME);

	dirInfo.setFile(VmHomeDir);
	if (!dirInfo.exists()) {
		WRITE_TRACE(DBG_FATAL, "Directory %s does not exist", QSTR2UTF8(VmHomeDir));
		return (PRL_ERR_VMDIR_INVALID_PATH);
	}
	dirList.append(qMakePair(dirInfo, QString(".")));
	for (i = 0; i < dirList.size(); ++i) {
		/* CDir::absoluteDir() is equal CDir::dir() : return parent directory */
		dir.setPath(dirList.at(i).first.absoluteFilePath());

		entryList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs | QDir::Hidden);

		WRITE_TRACE(DBG_DEBUG, "Directory %s", QSTR2UTF8(dirList.at(i).first.absoluteFilePath()));

		for (j = 0; j < entryList.size(); ++j) {
			const QFileInfo& fileInfo = entryList.at(j);

			if (dirInfo == fileInfo) {
				WRITE_TRACE(DBG_FATAL, "Infinite recursion in : %s", QSTR2UTF8(dirInfo.absoluteFilePath()));
				return (PRL_ERR_FAILURE);
			}
			if (!fileInfo.absoluteFilePath().startsWith(VmHomeDir)) {
				WRITE_TRACE(DBG_FATAL, "Path %s does not starts from VM home dir (%s)",
					QSTR2UTF8(fileInfo.absoluteFilePath()),
					QSTR2UTF8(VmHomeDir));
				return PRL_ERR_FAILURE;
			}
			;
			if (fileInfo.isDir()) {
				dirList.append(qMakePair(
					fileInfo, startDir.relativeFilePath(fileInfo.absoluteFilePath())));
			} else {
				/* skip config & config backup */
				if (fileInfo.absoluteFilePath() == config.absoluteFilePath())
					continue;
				if (fileInfo.absoluteFilePath() == config_backup.absoluteFilePath())
					continue;
				/* skip parallels.log */
				if (fileInfo.absoluteFilePath() == log.absoluteFilePath())
					continue;
				if (fileInfo.absoluteFilePath() == statlog.absoluteFilePath())
					/* will save statistic.log to temporary file */
					fileList.append(qMakePair(statlog,
						QString(PRL_VMTIMING_LOGFILENAME VMDIR_DEFAULT_VM_MIGRATE_SUFFIX)));
				else
					fileList.append(qMakePair(
						fileInfo, startDir.relativeFilePath(fileInfo.absoluteFilePath())));
			}
			WRITE_TRACE(DBG_DEBUG, "%x\t%s.%s\t%s",
				int(fileInfo.permissions()),
				QSTR2UTF8(fileInfo.owner()),
				QSTR2UTF8(fileInfo.group()),
				QSTR2UTF8(fileInfo.absoluteFilePath()));
		}
		entryList.clear();
	}
	/* remove VM home directory */
	dirList.removeFirst();

	return (PRL_ERR_SUCCESS);
}

/*
end of shared code from Vm/CVmMigrateTask.cpp
*/

/******************************** source ********************************/

static void NotifyClientsWithProgress(
		const SmartPtr<IOPackage> &p,
		const QString &sDirUuid,
		const QString &sClientUuid,
		int nPercents)
{
	Q_UNUSED(sDirUuid);
	const IOSender::Handle &hIoSender = sClientUuid;
	CVmEvent event(PET_DSP_EVT_VM_MIGRATE_PROGRESS_CHANGED, NULL, PIE_DISPATCHER);
	event.addEventParameter(new CVmEventParameter(
		PVE::UnsignedInt,
		QString::number(nPercents),
		EVT_PARAM_PROGRESS_CHANGED));

	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, event, p);
	SmartPtr<CDspClient> pClient = CDspService::instance()->getClientManager().getUserSession(hIoSender);
	if (pClient != NULL)
		pClient->sendPackage(pPackage);
}

Task_MigrateCtSource::Task_MigrateCtSource(
		const SmartPtr<CDspClient>& client,
		const CProtoCommandPtr cmd,
		const SmartPtr<IOPackage>& p)
    :
	CDspTaskHelper(client, p),
	Task_DispToDispConnHelper(getLastError()),
	m_pVmConfig(new CVmConfiguration()),
	m_bNewVmInstance(false),
	m_nSteps(0),
	m_nTotalSize(0),
	m_pOpaqueLock(NULL)
{
	CProtoVmMigrateCommand *pCmd = CProtoSerializer::CastToProtoCommand<CProtoVmMigrateCommand>(cmd);
	PRL_ASSERT(pCmd->IsValid());
	m_sVmUuid = pCmd->GetVmUuid();
	m_sServerHostname = pCmd->GetTargetServerHostname();
	m_nServerPort = pCmd->GetTargetServerPort();
	if (m_nServerPort == 0)
		m_nServerPort = CDspService::getDefaultListenPort();
	m_sServerSessionUuid = pCmd->GetTargetServerSessionUuid();
	m_sTargetServerCtHomePath = pCmd->GetTargetServerVmHomePath();
	m_nMigrationFlags = pCmd->GetMigrationFlags();
	m_nReservedFlags = pCmd->GetReservedFlags();
	m_bExVmOperationRegistered = false;
}

PRL_RESULT Task_MigrateCtSource::prepareTask()
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

	/* will use Vz dir uuid for Vm */
	{
		CDspLockedPointer<CVmDirectory> pDir = CDspService::instance()->getVmDirManager().getVzDirectory();
		if (!pDir) {
			nRetCode = PRL_ERR_VM_UUID_NOT_FOUND;
			WRITE_TRACE(DBG_FATAL, "Couldn't to find VZ directiory UUID");
			goto exit;
		}
		m_sVzDirUuid = pDir->getUuid();

		nRetCode = CDspService::instance()->getVmDirHelper().registerExclusiveVmOperation(
			m_sVmUuid, m_sVzDirUuid, PVE::DspCmdDirVmMigrate, getClient());
		if ( PRL_FAILED(nRetCode) ) {
			WRITE_TRACE(DBG_FATAL, "[%s] registerExclusiveVmOperation failed. Reason: %#x (%s)",
				    __FUNCTION__, nRetCode, PRL_RESULT_TO_STRING(nRetCode));
			goto exit;
		}
		m_bExVmOperationRegistered = true;
	}

	m_pVmConfig = CDspService::instance()->getVzHelper()->getCtConfig(getClient(), m_sVmUuid);
	if (!m_pVmConfig.isValid()) {
		WRITE_TRACE(DBG_FATAL, "Can not load config for uuid %s", QSTR2UTF8(m_sVmUuid));
		goto exit;
	}
	m_sVmHomePath = m_pVmConfig->getVmIdentification()->getHomePath();
	m_nCtid = m_pVmConfig->getVmIdentification()->getEnvId();

	nRetCode = CDspService::instance()->getVzHelper()->getVzlibHelper().get_env_status(m_sVmUuid, m_nPrevVmState);
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "Can not get status for uuid %s", QSTR2UTF8(m_sVmUuid));
		goto exit;
	}

	if (m_nPrevVmState != VMS_STOPPED) {
		WRITE_TRACE(DBG_FATAL, "CT must be stopped for migration");
		nRetCode = PRL_ERR_VM_MIGRATE_UNSUITABLE_VM_STATE;
		goto exit;
	}

	nRetCode = CVzHelper::src_start_migrate_env(m_nCtid, &m_pOpaqueLock);
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "Can not start source CT migration for uuid %s", QSTR2UTF8(m_sVmUuid));
		goto exit;
	}

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

PRL_RESULT Task_MigrateCtSource::run_body()
{
	SmartPtr<CVmEvent> pEvent;
	SmartPtr<IOPackage> pPackage;
	IOSendJob::Response pResponse;
	CDispToDispCommandPtr pCmd;
	CDispToDispResponseCommand *pRespCmd;

	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	if (operationIsCancelled())
		setLastErrorCode(PRL_ERR_OPERATION_WAS_CANCELED);

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;

	if (PRL_FAILED(nRetCode = CheckVmMigrationPreconditions()))
		goto exit;

	/* set migration mode */
	switch (m_nPrevVmState) {
	case VMS_RUNNING:
	case VMS_PAUSED:
		nRetCode = PRL_ERR_VM_MIGRATE_WARM_MODE_NOT_SUPPORTED;
		goto exit;
	default:
		m_nMigrationFlags |= PVMT_COLD_MIGRATION;
	}

	if (operationIsCancelled())
		setLastErrorCode(PRL_ERR_OPERATION_WAS_CANCELED);

	pEvent = SmartPtr<CVmEvent>(new CVmEvent(PET_DSP_EVT_VM_MIGRATE_STARTED, m_sVmUuid, PIE_DISPATCHER));
	pEvent->addEventParameter(new CVmEventParameter(PVE::Boolean, "true", EVT_PARAM_MIGRATE_IS_SOURCE));
	pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, pEvent->toString());

	m_nSteps |= MIGRATE_VM_STATE_CHANGED;
	/* and notify clients about VM migration start event */
	CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage, m_sVzDirUuid, m_sVmUuid);

	/* remove target Vm config from watcher (#448235) */
	CDspService::instance()->getVmConfigWatcher().unregisterVmToWatch(m_sVmConfigPath);
	m_nSteps |= MIGRATE_UNREGISTER_VM_WATCH;

	nRetCode = migrateStoppedCt();
	if (PRL_FAILED(nRetCode))
		goto exit;

	/* wait finish reply from target (https://jira.sw.ru/browse/PSBM-9596) */
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

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

void Task_MigrateCtSource::finalizeTask()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );
	SmartPtr<IOPackage> pPackage;

	Disconnect();

	if (PRL_SUCCEEDED(getLastErrorCode())) {
		CProtoCommandPtr pResponse =
			CProtoSerializer::CreateDspWsResponseCommand(getRequestPackage(), PRL_ERR_SUCCESS);
		getClient()->sendResponse(pResponse, getRequestPackage());
		/* We may not report error here since CT already live on destination */
		bool bDeleteSource = !(m_nMigrationFlags & PVMT_CLONE_MODE);
		PRL_RESULT nRetCode = CVzHelper::src_complete_migrate_env(m_nCtid, &m_pOpaqueLock, bDeleteSource);
		if (PRL_FAILED(nRetCode))
			WRITE_TRACE(DBG_FATAL, "Can not complete source CT migration for uuid %s", QSTR2UTF8(m_sVmUuid));
	} else {
		/* send response */
		CProtoCommandPtr pResponse =
			CProtoSerializer::CreateDspWsResponseCommand(getRequestPackage(), getLastErrorCode());
		CProtoCommandDspWsResponse *wsResponse =
			CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pResponse );
		getLastError()->setEventCode(getLastErrorCode());
		wsResponse->SetError(getLastError()->toString());
		getClient()->sendResponse( pResponse, getRequestPackage() );

		getClient()->sendResponseError( getLastError(), getRequestPackage() );
	}

	if (m_bExVmOperationRegistered)
		CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation(
				m_sVmUuid, m_sVzDirUuid, PVE::DspCmdDirVmMigrate, getClient());
}

// cancel command
void Task_MigrateCtSource::cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p)
{
	WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);

	CancelOperationSupport::cancelOperation(pUser, p);

	if (m_pVmCopySource)
		m_pVmCopySource->cancelOperation();

}

/*
* Check preconditions
* return PRL_ERR_SUCCESS - no errors
* 	 PRL_ERR_VM_MIGRATE_CHECKING_PRECONDITIONS_FAILED - precondition error
*	 other - internal error
*/
PRL_RESULT Task_MigrateCtSource::CheckVmMigrationPreconditions()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;

	CDispToDispCommandPtr pRequest =
		CDispToDispProtoSerializer::CreateVmMigrateCheckPreconditionsCommand(
			m_pVmConfig->toString(),
			m_cHostInfo.toString(),
			m_sTargetServerCtHomePath,
			QString(),
			QStringList(),
			QString(),
			0,
			m_nMigrationFlags,
			m_nReservedFlags | PVM_CT_MIGRATE | PVM_FULL_DISP_TASK,
			m_nPrevVmState
		);

	SmartPtr<IOPackage> pPackage =
		DispatcherPackage::createInstance(pRequest->GetCommandId(), pRequest->GetCommand()->toString());
	SmartPtr<IOPackage> pReply;
	IOSendJob::Response pResponse;

	m_hCheckReqJob = m_pIoClient->sendPackage(pPackage);
	if (m_pIoClient->waitForSend(m_hCheckReqJob, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Package sending failure");
		return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
	}
	if (m_pIoClient->waitForResponse(m_hCheckReqJob, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Responce receiving failure");
		return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
	}
	pResponse = m_pIoClient->takeResponse(m_hCheckReqJob);
	if (pResponse.responseResult != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Job failure: responseResult:%x", pResponse.responseResult);
		return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
	}

	pReply = pResponse.responsePackages[0];
	if (!pReply.isValid()) {
		WRITE_TRACE(DBG_FATAL, "Invalid reply");
		return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
	}
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

	if (pReply->header.type == DispToDispResponseCmd) {
		CDispToDispResponseCommand *pResponseCmd =
			CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pCmd);
		nRetCode = pResponseCmd->GetRetCode();
		getLastError()->fromString(pResponseCmd->GetErrorInfo()->toString());
		return nRetCode;
	} else {
		CVmMigrateCheckPreconditionsReply *pResponseCmd =
			CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateCheckPreconditionsReply>(pCmd);
		m_lstCheckPrecondsErrors = pResponseCmd->GetCheckPreconditionsResult();
		m_nReservedFlags = pResponseCmd->GetCommandFlags();
		if (!(m_nReservedFlags & PVM_CT_MIGRATE)) {
			/* migration of the containers does not supports */
			WRITE_TRACE(DBG_FATAL, "Remote server does not support migration of the containers");
			return PRL_ERR_UNIMPLEMENTED;
		}
	}
	if (!m_lstCheckPrecondsErrors.isEmpty())
		nRetCode = PRL_ERR_VM_MIGRATE_CHECKING_PRECONDITIONS_FAILED;

	return nRetCode;
}

/* To send start request to remote dispatcher and wait reply */
PRL_RESULT Task_MigrateCtSource::SendStartRequest()
{
	CDispToDispCommandPtr pCmd;
	IOSendJob::Response pResponse;

	CDispToDispCommandPtr pMigrateStartCmd = CDispToDispProtoSerializer::CreateVmMigrateStartCommand(
				m_pVmConfig->toString(),
				QString(),
				m_sTargetServerCtHomePath,
				QString(),
				0,
				0,
				m_nMigrationFlags,
				m_nReservedFlags | PVM_CT_MIGRATE | PVM_FULL_DISP_TASK,
				m_nPrevVmState);

	SmartPtr<IOPackage> pPackage = DispatcherPackage::createInstance(
			pMigrateStartCmd->GetCommandId(),
			pMigrateStartCmd->GetCommand()->toString());

	m_hStartReqJob = m_pIoClient->sendPackage(pPackage);
	if (m_pIoClient->waitForSend(m_hStartReqJob, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Package sending failure");
		return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
	}
	if (m_pIoClient->waitForResponse(m_hStartReqJob, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Responce receiving failure");
		return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
	}
	pResponse = m_pIoClient->takeResponse(m_hStartReqJob);
	if (pResponse.responseResult != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Job failure: responseResult:%x", pResponse.responseResult);
		return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
	}

	m_pReply = pResponse.responsePackages[0];
	if (!m_pReply.isValid()) {
		WRITE_TRACE(DBG_FATAL, "Invalid reply");
		return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
	}

	if ((m_pReply->header.type != DispToDispResponseCmd) && (m_pReply->header.type != VmMigrateReply)) {
		WRITE_TRACE(DBG_FATAL, "Unexpected response type on migration start command: %d", m_pReply->header.type);
		return PRL_ERR_OPERATION_FAILED;
	}

	pCmd = CDispToDispProtoSerializer::ParseCommand(
			(Parallels::IDispToDispCommands)m_pReply->header.type,
			UTF8_2QSTR(m_pReply->buffers[0].getImpl()));

	if (m_pReply->header.type == DispToDispResponseCmd) {
		CDispToDispResponseCommand *pResponseCmd =
			CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pCmd);
		getLastError()->fromString(pResponseCmd->GetErrorInfo()->toString());
		return pResponseCmd->GetRetCode();
	}

	return PRL_ERR_SUCCESS;
}

/* send package with migration data to target dispatcher */
PRL_RESULT Task_MigrateCtSource::sendDispPackage(SmartPtr<IOPackage> &pPackage)
{
	IOSendJob::Handle hJob;

	hJob = m_pIoClient->sendPackage(pPackage);
	if (m_pIoClient->waitForSend(hJob, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Package sending failure");
		return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
	}
	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_MigrateCtSource::migrateStoppedCt()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	int i;
	QList<QPair<QFileInfo, QString> > dList;
	QList<QPair<QFileInfo, QString> > fList;

	if ( !(m_nReservedFlags & PVM_DONT_COPY_VM) )
		/* get full directories and files lists for migration */
		if ((nRetCode = GetEntryLists(m_sVmHomePath, dList, fList)) != PRL_ERR_SUCCESS)
			return nRetCode;

	if (PRL_FAILED(nRetCode = SendStartRequest()))
		return nRetCode;

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
	m_pVmCopySource->SetVmDirectoryUuid(m_sVzDirUuid);
	m_pVmCopySource->SetProgressNotifySender(NotifyClientsWithProgress);

	nRetCode = m_pVmCopySource->Copy(dList, fList);
	if (PRL_FAILED(nRetCode))
		WRITE_TRACE(DBG_FATAL, "Error occurred while migration with code [%#x][%s]",
			nRetCode, PRL_RESULT_TO_STRING(nRetCode));

	return nRetCode;
}

Task_MigrateCtSource::~Task_MigrateCtSource()
{
	// #439777 protect handler from destroying object
	if (m_pOpaqueLock)
		CVzHelper::unlock_env(m_nCtid, &m_pOpaqueLock);
}


/******************************** target ********************************/

#define VM_MIGRATE_START_CMD_WAIT_TIMEOUT 600 * 1000

Task_MigrateCtTarget::Task_MigrateCtTarget(
	const QObject *parent,
	const SmartPtr<CDspDispConnection> &pDispConnection,
	CDispToDispCommandPtr pCmd,
	const SmartPtr<IOPackage> &p)
   :
	CDspTaskHelper(pDispConnection->getUserSession(), p),
	Task_DispToDispConnHelper(getLastError()),
	m_pParent(parent),
	m_pCheckDispConnection(pDispConnection),
	m_pCheckPackage(p),
	m_cDstHostInfo(CDspService::instance()->getHostInfo()->data()),
	m_pOpaqueLock(NULL),
	m_nSteps(0),
	m_nBundlePermissions(0),
	m_nConfigPermissions(0)
{
	CVmMigrateCheckPreconditionsCommand * pCheckCmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateCheckPreconditionsCommand>(pCmd);

	m_hConnHandle = pDispConnection->GetConnectionHandle();
	m_nMigrationFlags = pCheckCmd->GetMigrationFlags();
	m_nReservedFlags = pCheckCmd->GetReservedFlags();
	m_nVersion = pCheckCmd->GetVersion();
	m_sVmConfig = pCheckCmd->GetVmConfig();
	m_nPrevVmState = pCheckCmd->GetVmPrevState();
	m_sVmDirPath = pCheckCmd->GetTargetVmHomePath();
	m_sSrcHostInfo = pCheckCmd->GetSourceHostHardwareInfo();

	/* initialize all vars from pCheckCmd - after exit from constructor package buffer will invalid */

	bool bConnected = QObject::connect(
		&CDspService::instance()->getIOServer(),
		SIGNAL(onClientDisconnected(IOSender::Handle)),
		SLOT(clientDisconnected(IOSender::Handle)),
		Qt::DirectConnection);
	PRL_ASSERT(bConnected);
}

Task_MigrateCtTarget::~Task_MigrateCtTarget()
{
	// #439777 protect handler from destroying object
	m_waiter.waitUnlockAndFinalize();
	if (m_pOpaqueLock)
		CVzHelper::unlock_env(m_nCtid, &m_pOpaqueLock);
}

/* process VmMigrateCheckPreconditionsCommand */
PRL_RESULT Task_MigrateCtTarget::prepareTask()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QString sVmDirPath;

	if (CDspService::instance()->isServerStopping()) {
		WRITE_TRACE(DBG_FATAL, "Dispatcher shutdown is in progress - CT migrate rejected!");
		nRetCode = PRL_ERR_DISP_SHUTDOWN_IN_PROCESS;
		goto exit;
	}

	m_pVmConfig = SmartPtr<CVmConfiguration>(new CVmConfiguration(m_sVmConfig));
	if (PRL_FAILED(m_pVmConfig->m_uiRcInit)) {
		nRetCode = PRL_ERR_PARSE_VM_CONFIG;
		WRITE_TRACE(DBG_FATAL, "Wrong VM condiguration was received: [%s]", QSTR2UTF8(m_sVmConfig));
		goto exit;
	}
	m_sOriginVmUuid = m_pVmConfig->getVmIdentification()->getVmUuid();
	m_nCtid = m_nOriginCtid = m_pVmConfig->getVmIdentification()->getEnvId();
	m_sVmName = m_pVmConfig->getVmIdentification()->getVmName();

	nRetCode = CVzHelper::dst_start_migrate_env(&m_nCtid, &m_pOpaqueLock);
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "Can not start destination CT migration for uuid %s", QSTR2UTF8(m_sVmUuid));
		goto exit;
	}

	if (m_nCtid != m_nOriginCtid)
		m_pVmConfig->getVmIdentification()->setEnvId(m_nCtid);

	{
		CDspLockedPointer<CVmDirectory> pDir = CDspService::instance()->getVmDirManager().getVzDirectory();
		if (!pDir) {
			nRetCode = PRL_ERR_VM_UUID_NOT_FOUND;
			WRITE_TRACE(DBG_FATAL, "Couldn't to find VZ directiory UUID");
			goto exit;
		}
		m_sVzDirUuid = pDir->getUuid();
	}
	m_sVmUuid = (PVMT_CLONE_MODE & getRequestFlags()) ? Uuid::createUuid().toString() : m_sOriginVmUuid;

	WRITE_TRACE(DBG_FATAL, "Start migrate origin CTID=%u to CTID=%u for uuid %s",
		    m_nOriginCtid, m_nCtid, QSTR2UTF8(m_sVmUuid));

	/*  to get target VM home directory path */
	if (!m_sVmDirPath.isEmpty())
		m_sTargetVmHomePath = m_sVmDirPath;
	else {
		m_sTargetVmHomePath = CVzHelper::getCtPrivatePath(m_nCtid);
	}
	m_sTargetVmHomePath = QFileInfo(m_sTargetVmHomePath).absoluteFilePath();
	m_sVmConfigPath = QString("%1/" VMDIR_DEFAULT_VM_CONFIG_FILE).arg(m_sTargetVmHomePath);

	/* lock Vm exclusive parameters */
	m_pVmInfo = SmartPtr<CVmDirectory::TemporaryCatalogueItem>(new CVmDirectory::TemporaryCatalogueItem(
			m_sVmUuid, m_sVmConfigPath, m_sVmName));

	nRetCode = CDspService::instance()->getVmDirManager()
			.checkAndLockNotExistsExclusiveVmParameters(QStringList(), m_pVmInfo.getImpl());
	if (PRL_FAILED(nRetCode))
	{
		switch (nRetCode)
		{
		case PRL_ERR_VM_ALREADY_REGISTERED_VM_UUID:
			WRITE_TRACE(DBG_FATAL, "UUID '%s' already registered", QSTR2UTF8(m_pVmInfo->vmUuid));
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, m_pVmInfo->vmUuid, EVT_PARAM_RETURN_PARAM_TOKEN));
			break;

		case PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH:
			WRITE_TRACE(DBG_FATAL, "path '%s' already registered", QSTR2UTF8(m_pVmInfo->vmXmlPath));
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, m_pVmInfo->vmXmlPath, EVT_PARAM_MESSAGE_PARAM_1));
			break;

		case PRL_ERR_VM_ALREADY_REGISTERED_VM_NAME:
			WRITE_TRACE(DBG_FATAL, "name '%s' already registered", QSTR2UTF8(m_pVmInfo->vmName));
			getLastError()->addEventParameter(
				new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
			break;

		case PRL_ERR_VM_ALREADY_REGISTERED:
			WRITE_TRACE(DBG_FATAL, "container '%s' already registered", QSTR2UTF8(m_pVmInfo->vmName));
			getLastError()->addEventParameter(
					new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_MESSAGE_PARAM_0));
			break;

		case PRL_ERR_VM_ALREADY_REGISTERED_UNIQUE_PARAMS:; // use default

		default:
			WRITE_TRACE(DBG_FATAL, "can't register container with UUID '%s', name '%s', path '%s",
				QSTR2UTF8(m_pVmInfo->vmUuid), QSTR2UTF8(m_pVmInfo->vmName), QSTR2UTF8(m_pVmInfo->vmXmlPath));
			getLastError()->addEventParameter(
				 new CVmEventParameter( PVE::String, m_pVmInfo->vmUuid, EVT_PARAM_RETURN_PARAM_TOKEN));
			getLastError()->addEventParameter(
				 new CVmEventParameter( PVE::String, m_pVmInfo->vmXmlPath, EVT_PARAM_RETURN_PARAM_TOKEN));
			getLastError()->addEventParameter(
				 new CVmEventParameter( PVE::String, m_pVmInfo->vmName, EVT_PARAM_RETURN_PARAM_TOKEN));
		}
		goto exit;
	}
	m_nSteps |= MIGRATE_VM_EXCL_PARAMS_LOCKED;

	if (!(PVMT_SWITCH_TEMPLATE & getRequestFlags())) {
		/* skip checking for copy to template case (https://jira.sw.ru/browse/PSBM-9597) */
		//checkTargetCpusNumber();
		//checkTargetCpuCompatibility();
	}

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

PRL_RESULT Task_MigrateCtTarget::run_body()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	bool bConnected;
	QTimer *pTimer;
	CDispToDispCommandPtr pReply;
	SmartPtr<IOPackage> pPackage;
	IOSendJob::Handle hJob;

	if (operationIsCancelled()) {
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit;
	}

	bConnected = QObject::connect(m_pParent,
		SIGNAL(onPackageReceived(
			const SmartPtr<CDspDispConnection> &,
			const QString &,
			const SmartPtr<IOPackage> &)),
		SLOT(handleStartPackage(
			const SmartPtr<CDspDispConnection> &,
			const QString &,
			const SmartPtr<IOPackage> &)),
		Qt::DirectConnection);
	PRL_ASSERT(bConnected);

	m_nReservedFlags |= PVM_CT_MIGRATE;
	pReply = CDispToDispProtoSerializer::CreateVmMigrateCheckPreconditionsReply(
		m_lstCheckPrecondsErrors, QStringList(), m_nReservedFlags);
	pPackage =
		DispatcherPackage::createInstance(
			pReply->GetCommandId(), pReply->GetCommand()->toString(), m_pCheckPackage);
	hJob = m_pCheckDispConnection->sendPackage(pPackage);
	if (!hJob.isValid()) {
		nRetCode = PRL_ERR_OPERATION_FAILED;
		goto exit;
	}

	/* set timer */
	pTimer = new QTimer();
	pTimer->setSingleShot(true);
	bConnected = QObject::connect(pTimer, SIGNAL(timeout()), SLOT(handleStartCommandTimeout()), Qt::DirectConnection);
	pTimer->start(VM_MIGRATE_START_CMD_WAIT_TIMEOUT);

	/* will wait StartMigration command */
	nRetCode = exec();
	pTimer->stop();
	if (bConnected)
		QObject::disconnect(pTimer, SIGNAL(timeout()), this, SLOT(handleStartCommandTimeout()));
	delete pTimer;

	QObject::disconnect(m_pParent,
		SIGNAL(onPackageReceived(
			const SmartPtr<CDspDispConnection> &,
			const QString &,
			const SmartPtr<IOPackage> &)),
		this,
		SLOT(handleStartPackage(
			const SmartPtr<CDspDispConnection> &,
			const QString &,
			const SmartPtr<IOPackage> &)));

	if (PRL_FAILED(nRetCode))
		goto exit;

	if (operationIsCancelled()) {
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit;
	}

	if ( !(m_nReservedFlags & PVM_DONT_COPY_VM) ) {
		/* to create Vm bundle */
		if (!CFileHelper::WriteDirectory(m_sTargetVmHomePath, &getClient()->getAuthHelper())) {
			nRetCode = PRL_ERR_BACKUP_CANNOT_CREATE_DIRECTORY;
			CVmEvent *pEvent = getLastError();
			pEvent->setEventCode(nRetCode);
			pEvent->addEventParameter(
				new CVmEventParameter(PVE::String, m_sTargetVmHomePath, EVT_PARAM_MESSAGE_PARAM_0));
			WRITE_TRACE(DBG_FATAL, "[%s] Cannot create \"%s\" directory",
					__FUNCTION__, QSTR2UTF8(m_sTargetVmHomePath));
			goto exit;
		}
		/* set original permissions to Vm bundle (https://jira.sw.ru/browse/PSBM-8269) */
		if (m_nBundlePermissions) {
			QFile vmBundle(m_sTargetVmHomePath);
			if (!vmBundle.setPermissions((QFile::Permissions)m_nBundlePermissions)) {
				WRITE_TRACE(DBG_FATAL,
					"[%s] Cannot set permissions for Vm bundle \"%s\", will use default",
					__FUNCTION__, QSTR2UTF8(m_sTargetVmHomePath));
			}
		}
	}
	m_nSteps |= MIGRATE_STARTED;
	if ((m_nPrevVmState == VMS_RUNNING) || (m_nPrevVmState == VMS_PAUSED)) {
		nRetCode = PRL_ERR_VM_MIGRATE_UNSUITABLE_VM_STATE;
	} else {
		nRetCode = migrateStoppedVm();

		/* migration completed and connection state is indifferent for us
		   https://jira.sw.ru/browse/PSBM-8925 */
		QObject::disconnect(
			&CDspService::instance()->getIOServer(),
			SIGNAL(onClientDisconnected(IOSender::Handle)),
			this,
			SLOT(clientDisconnected(IOSender::Handle)));
	}

	if (PRL_FAILED(nRetCode))
		goto exit;

	/* rename conf file since source sent it with his origin ctid */
	if (m_nOriginCtid != m_nCtid) {
		QString srcPath = m_sTargetVmHomePath + QString("/%1.conf").arg(m_nOriginCtid);
		QString dstPath = m_sTargetVmHomePath + QString("/%1.conf").arg(m_nCtid);
		QFile::rename(srcPath, dstPath);
	}

	nRetCode = CVzHelper::dst_complete_migrate_env(m_sVmUuid, m_nCtid, m_nOriginCtid, &m_pOpaqueLock);
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "Can not compelete destination CT migration for ctid %u uuid %s",
			    m_nCtid,  QSTR2UTF8(m_sVmUuid));
		goto exit;
	}

	if (PVMT_CLONE_MODE & getRequestFlags())
		m_pVmConfig->getVmIdentification()->setSourceVmUuid(m_sOriginVmUuid);

	if (PVMT_CLONE_MODE & getRequestFlags()) {
		if ( PVMT_SWITCH_TEMPLATE & getRequestFlags() )
			m_pVmConfig->getVmSettings()->getVmCommonOptions()->setTemplate(
				!m_pVmConfig->getVmSettings()->getVmCommonOptions()->isTemplate() );

		Task_CloneVm::ResetNetSettings(m_pVmConfig);
	}

	/* do not fail since here - Ct already migrated */
	{
		QString sServerUuid = CDspService::instance()->getDispConfigGuard().getDispConfig()
			->getVmServerIdentification()->getServerUuid();
		QString sLastServerUuid = m_pVmConfig->getVmIdentification()->getServerUuid();
		m_pVmConfig->getVmIdentification()->setServerUuid(sServerUuid);
		m_pVmConfig->getVmIdentification()->setLastServerUuid(sLastServerUuid);
		SmartPtr<CVmConfiguration> pOldConfig = CDspService::instance()->getVzHelper()
			->getVzlibHelper().get_env_config(m_nCtid);
		if (pOldConfig == NULL) {
			WRITE_TRACE(DBG_FATAL, "Can't get config for CT %s [%d]", QSTR2UTF8(m_sVmUuid), m_nCtid);
		} else {
			get_op_helper().apply_env_config(m_pVmConfig, pOldConfig);
		}
	}

	// insert new item in user's VM Directory
	m_pVmConfig = CDspService::instance()->getVzHelper()->getVzlibHelper().get_env_config(m_nCtid);
	if (PRL_FAILED(CDspService::instance()->getVzHelper()->insertVmDirectoryItem(m_pVmConfig))) {
		WRITE_TRACE(DBG_FATAL, "Can't insert CT to VmDirectory by error %#x, %s",
			    nRetCode, PRL_RESULT_TO_STRING(nRetCode) );
	}

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

void Task_MigrateCtTarget::finalizeTask()
{
	IOSendJob::Handle hJob;
	SmartPtr<IOPackage> pPackage;

	// delete temporary registration
	if (m_nSteps & MIGRATE_VM_EXCL_PARAMS_LOCKED)
		CDspService::instance()->getVmDirManager().unlockExclusiveVmParameters(m_pVmInfo.getImpl());

	int nRetCode = getLastErrorCode();
	if (PRL_SUCCEEDED(nRetCode)) {
		if (m_pVm.getImpl()) {
			PRL_EVENT_TYPE evtType;
			/* restore Vm previous state */
			switch (m_nPrevVmState)
			{
			case VMS_RUNNING:
				evtType = PET_DSP_EVT_VM_STARTED;
				break;
			case VMS_STOPPED:
			default:
				evtType = PET_DSP_EVT_VM_STOPPED;
				break;
			}
			CVmEvent cStateEvent(evtType, m_sVmUuid, PIE_DISPATCHER);
			SmartPtr<IOPackage> pUpdateVmStatePkg =
				DispatcherPackage::createInstance( PVE::DspVmEvent, cStateEvent.toString());
			m_pVm->changeVmState(pUpdateVmStatePkg);
			CDspService::instance()->getClientManager().
				sendPackageToVmClients(pUpdateVmStatePkg, m_sVzDirUuid, m_sVmUuid);

			if (m_nPrevVmState == VMS_STOPPED) {
				CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation(
					m_sVmUuid, m_sVzDirUuid, PVE::DspCmdDirVmMigrate, getClient());
				CDspVm::UnregisterVmObject(m_pVm);

			} else {
				/* lock running Vm (https://jira.sw.ru/browse/PSBM-7682) */
				/* will do it via initial Vm creation command
				   (https://jira.sw.ru/browse/PSBM-8222) */
				m_pVm->replaceInitDspCmd(PVE::DspCmdVmStart, getClient());
			}
		}

		/* notify source task about target task finish,
		   will send reply to check precondition request. https://jira.sw.ru/browse/PSBM-9596 */
		m_pCheckDispConnection->sendSimpleResponse(m_pCheckPackage, PRL_ERR_SUCCESS);

		/* migration initiator wait event from original Vm uuid */
		CVmEvent cEvent(PET_DSP_EVT_VM_MIGRATE_FINISHED, m_sOriginVmUuid, PIE_DISPATCHER);
		pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, cEvent.toString());
		CDspService::instance()->getClientManager().sendPackageToVmClients(pPackage, m_sVzDirUuid, m_sOriginVmUuid);
	}
	else
	{
		/* It is not possible to get Vm client list after deleteVmDirectoryItem(), and
		   sendPackageToVmClients() does not work. Get list right now and will use
		   sendPackageToClientList() call (https://jira.sw.ru/browse/PSBM-9159) */
		QList< SmartPtr<CDspClient> > clientList =
			CDspService::instance()->getClientManager().
				getSessionListByVm(m_sVzDirUuid, m_sVmUuid).values();

		if (operationIsCancelled())
			setLastErrorCode(PRL_ERR_OPERATION_WAS_CANCELED);

		/* if migration was not started, do not cleanup anything -
		   we can remove 'already existed Vm' */
		if (m_nSteps & MIGRATE_STARTED)
		{

			if (m_nSteps & MIGRATE_VM_APP_STARTED && m_pVm.getImpl())
			{
				/* stop Vm */
				CProtoCommandPtr pCmd =
					CProtoSerializer::CreateProtoVmCommandStop(m_sVmUuid, PSM_KILL, 0);
				pPackage = DispatcherPackage::createInstance( PVE::DspCmdVmStop, pCmd );
				m_pVm->stop(getClient(), pPackage, PSM_KILL, true);
				// Wait until VM stopped
				while (m_pVm->isConnected()) {
					if (operationIsCancelled() && CDspService::instance()->isServerStopping())
						break;
					QThread::msleep(1000);
				}
			}
			if (m_pVm.getImpl()) {
				CDspVm::UnregisterVmObject(m_pVm);
				m_pVm = SmartPtr<CDspVm>(0);
			}

			if (!CDspService::instance()->isServerStopping())
				CDspService::instance()->getVmConfigWatcher().unregisterVmToWatch(m_sTargetVmHomePath);
			if ( !(m_nReservedFlags & PVM_DONT_COPY_VM) ) {
				CFileHelper::ClearAndDeleteDir(m_sTargetVmHomePath);
			}

			// Unregister VM dir item
			CDspService::instance()->getVmDirHelper().deleteVmDirectoryItem(m_sVzDirUuid, m_sVmUuid);

			CVmEvent cDelEvent(PET_DSP_EVT_VM_DELETED, m_sVmUuid, PIE_DISPATCHER);
			SmartPtr<IOPackage> pDelPackage =
					DispatcherPackage::createInstance(PVE::DspVmEvent, cDelEvent.toString());
			CDspService::instance()->getClientManager().sendPackageToClientList(
					pDelPackage, clientList);
		}

		/* migration initiator wait event from original Vm uuid */
		CVmEvent cEvent(PET_DSP_EVT_VM_MIGRATE_CANCELLED, m_sOriginVmUuid, PIE_DISPATCHER);
		pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, cEvent.toString());
		CDspService::instance()->getClientManager().sendPackageToClientList(pPackage, clientList);

		if (m_pStartDispConnection.isValid())
			hJob = m_pStartDispConnection->sendResponseError(getLastError(), getRequestPackage());
		else
			hJob = m_pCheckDispConnection->sendResponseError(getLastError(), getRequestPackage());
		CDspService::instance()->getIOServer().waitForSend(hJob, m_nTimeout);
	}
}

PRL_RESULT Task_MigrateCtTarget::migrateStoppedVm()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	CDispToDispCommandPtr pReply;
	SmartPtr<IOPackage> pPackage;

	m_pSender = SmartPtr<CVmFileListCopySender>(new CVmFileListCopySenderServer(
			CDspService::instance()->getIOServer(),
			m_pStartDispConnection->GetConnectionHandle()));

	/* CVmFileListCopyTarget will use Vm uuid in progress messages for clients, so will use original Vm uuid */
	m_pVmMigrateTarget = SmartPtr<CVmFileListCopyTarget>(
		new CVmFileListCopyTarget(m_pSender.getImpl(), m_sOriginVmUuid, m_sTargetVmHomePath, NULL, m_nTimeout));


	// Connect to handle traffic report package
	bool bConnected = QObject::connect(
		m_pStartDispConnection.getImpl(),
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		SLOT(handlePackage(IOSender::Handle, const SmartPtr<IOPackage>)),
		Qt::DirectConnection );
	PRL_ASSERT(bConnected);

	// set empty string for default path
	pReply = CDispToDispProtoSerializer::CreateVmMigrateReply(QString());
	pPackage = DispatcherPackage::createInstance(
		pReply->GetCommandId(), pReply->GetCommand()->toString(), m_pStartPackage);
	m_pStartDispConnection->sendPackage(pPackage);

	nRetCode = QThread::exec();

	QObject::disconnect(
		m_pStartDispConnection.getImpl(),
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		this,
		SLOT(handlePackage(IOSender::Handle, const SmartPtr<IOPackage>)));

	if (PRL_FAILED(nRetCode))
		return nRetCode;

	return PRL_ERR_SUCCESS;
}

void Task_MigrateCtTarget::handleVmMigrateEvent(const QString &sVmUuid, const SmartPtr<IOPackage> &p)
{
	if (operationIsCancelled())
		return;

	if (sVmUuid != m_sVmUuid)
		return;

	if (p->header.type != PVE::DspVmEvent) {
		WRITE_TRACE(DBG_FATAL, "Unexpected package with type %d, ignored", p->header.type);
		return;
	}

	CVmEvent event(UTF8_2QSTR(p->buffers[0].getImpl()));
	switch (event.getEventType())
	{
	case PET_DSP_EVT_VM_MIGRATE_WAIT_REMOUNT:
		// TODO : send error package to Vm
		break;
	case PET_DSP_EVT_VM_MIGRATE_CANCELLED_DISP:
		exit(PRL_ERR_FAILURE);
		break;
	case PET_DSP_EVT_VM_MIGRATE_FINISHED_DISP:
		exit(PRL_ERR_SUCCESS);
		break;
	default:
		WRITE_TRACE(DBG_FATAL, "Unexpected event with type %d, ignored", event.getEventType());
		break;
	}
}

void Task_MigrateCtTarget::handlePackage(IOSender::Handle h, const SmartPtr<IOPackage> p)
{
	PRL_RESULT nRetCode;
	bool bExit;
	PRL_ASSERT(m_pVmMigrateTarget.getImpl());

	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	if (h != m_pStartDispConnection->GetConnectionHandle())
		return;

	if (IS_FILE_COPY_PACKAGE(p->header.type)) {
		nRetCode = m_pVmMigrateTarget->handlePackage(p, &bExit);
		if (bExit)
			QThread::exit(nRetCode);
	} else if (p->header.type == VmMigrateCancelCmd) {
		WRITE_TRACE(DBG_DEBUG, "Migration was cancelled");
		QThread::exit(PRL_ERR_OPERATION_WAS_CANCELED);
	} else {
		WRITE_TRACE(DBG_FATAL, "Invalid package type %d, ignored", p->header.type);
	}
}

void Task_MigrateCtTarget::clientDisconnected(IOSender::Handle h)
{
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	if (h != m_hConnHandle)
		return;

	WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);
	SmartPtr<CDspClient> nullClient;
	cancelOperation(nullClient, getRequestPackage());
}

void Task_MigrateCtTarget::handleStartPackage(
		const SmartPtr<CDspDispConnection> &pDispConnection,
		const QString &sVmUuid,
		const SmartPtr<IOPackage> &p)
{
	// ??? LOCK
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	if (operationIsCancelled())
		return;

	/* this function will handle StartMigrateCommand with _original_ Vm uuid */
	if (m_sOriginVmUuid != sVmUuid)
		return;

	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(p);
	if ( !pCmd->IsValid() )
	{
		WRITE_TRACE(DBG_FATAL, "Wrong start migration package was received: [%s]",
			p->buffers[0].getImpl());
		QThread::exit(PRL_ERR_FAILURE);
	}

	CVmMigrateStartCommand *pStartCmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CVmMigrateStartCommand>(pCmd);
	if ( NULL == pStartCmd )
	{
		WRITE_TRACE(DBG_FATAL, "Wrong start migration package was received: [%s]",
			p->buffers[0].getImpl());
		QThread::exit(PRL_ERR_FAILURE);
	}
	m_nMigrationFlags = pStartCmd->GetMigrationFlags();
	m_nReservedFlags = pStartCmd->GetReservedFlags();
	m_nBundlePermissions = pStartCmd->GetBundlePermissions();
	m_nConfigPermissions = pStartCmd->GetConfigPermissions();

	/* reassign task to new connection and package and return to main thread */
	SmartPtr<CDspClient> pClient = pDispConnection->getUserSession();
	reassignTask( pClient, p );
	m_pStartDispConnection = pDispConnection;
	/* attn: do not use getRequestPackage() after this - reassignTask() changed type of new package */
	m_pStartPackage = IOPackage::duplicateInstance(p);

	QThread::exit(0);
}

// cancel command
void Task_MigrateCtTarget::cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p)
{
	WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);
	CancelOperationSupport::cancelOperation(pUser, p);

	if (m_pVmMigrateTarget.isValid())
		m_pVmMigrateTarget->cancelOperation();

	QThread::exit(PRL_ERR_OPERATION_WAS_CANCELED);
}

void Task_MigrateCtTarget::handleStartCommandTimeout()
{
	// #439777 to protect call handler for destroying object
	WaiterTillHandlerUsingObject::AutoUnlock lock( m_waiter );
	if( !lock.isLocked() )
		return;

	WRITE_TRACE( DBG_DEBUG, "StartMigrateCommand timeout" );
	QThread::exit(PRL_ERR_TIMEOUT);
}
