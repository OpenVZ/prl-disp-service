///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateCtSource.cpp
///
/// Source task for Vm migration
///
/// @author krasnov@
///
/// Copyright (c) 2010-2017, Parallels International GmbH
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
#include "CDspClientManager.h"
#include <prlcommon/Logging/Logging.h>

#include "Task_MigrateCtSource.h"
#include "CDspService.h"
#include <prlcommon/Std/PrlAssert.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/DispToDispProtocols/CVmMigrationProto.h"
#include "CDspVzHelper.h"
#include <prlcommon/PrlCommonUtilsBase/ErrorSimple.h>

Task_MigrateCtSource::Task_MigrateCtSource(
		const SmartPtr<CDspClient>& client,
		const CProtoCommandPtr cmd,
		const SmartPtr<IOPackage>& p)
:Task_VzMigrate(client, p)
{
	CProtoVmMigrateCommand *pCmd = CProtoSerializer::CastToProtoCommand<CProtoVmMigrateCommand>(cmd);
	PRL_ASSERT(pCmd->IsValid());
	m_sCtUuid = pCmd->GetVmUuid();
	m_sServerHostname = pCmd->GetTargetServerHostname();
	m_nServerPort = pCmd->GetTargetServerPort();
	if (m_nServerPort == 0)
		m_nServerPort = CDspService::getDefaultListenPort();
	m_sServerSessionUuid = pCmd->GetTargetServerSessionUuid();
	m_sTargetServerCtName = pCmd->GetTargetServerVmName();
	m_sTargetServerCtHomePath = pCmd->GetTargetServerVmHomePath();
	m_nMigrationFlags = pCmd->GetMigrationFlags();
	m_nReservedFlags = pCmd->GetReservedFlags();
	m_bExVmOperationRegistered = false;
}

Task_MigrateCtSource::~Task_MigrateCtSource()
{
}

PRL_RESULT Task_MigrateCtSource::prepareTask()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	SmartPtr<CDspClient> pUserSession = getClient();

	if (CDspService::instance()->getShellServiceHelper().isLocalAddress(m_sServerHostname)) {
		WRITE_TRACE(DBG_FATAL,
			"Host %s is a local host, migration is impossible", QSTR2UTF8(m_sServerHostname));
		nRetCode = PRL_ERR_VM_MIGRATE_TO_THE_SAME_NODE;
		goto exit;
	}

	{
		CDspLockedPointer<CVmDirectory> pDir = CDspService::instance()->getVmDirManager().getVzDirectory();
		if (!pDir) {
			nRetCode = PRL_ERR_VM_UUID_NOT_FOUND;
			WRITE_TRACE(DBG_FATAL, "Couldn't to find VZ directiory UUID");
			goto exit;
		}
		m_sVzDirUuid = pDir->getUuid();
	}

	/* will use dir uuid for Vm */
	nRetCode = CDspService::instance()->getVmDirHelper().registerExclusiveVmOperation(
		m_sCtUuid, m_sVzDirUuid, PVE::DspCmdDirVmMigrate, pUserSession);
	if ( PRL_FAILED(nRetCode) ) {
		WRITE_TRACE(DBG_FATAL, "[%s] registerExclusiveVmOperation failed. Reason: %#x (%s)",
			__FUNCTION__, nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		goto exit;
	}
	m_bExVmOperationRegistered = true;

	if ((m_nMigrationFlags & PVMT_HOT_MIGRATION) && (m_nPrevVmState != VMS_RUNNING)) {
		WRITE_TRACE(DBG_FATAL, "VM must be running for migration in 'hot' mode");
		nRetCode = PRL_ERR_VM_MIGRATE_UNSUITABLE_VM_STATE;
		goto exit;
	}

	nRetCode = CDspService::instance()->getVzHelper()->getVzlibHelper().get_env_status(m_sCtUuid, m_nPrevVmState);
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "Can not get status for uuid %s", QSTR2UTF8(m_sCtUuid));
		goto exit;
	}

	m_pVmConfig = CDspService::instance()->getVzHelper()->getCtConfig(pUserSession, m_sCtUuid);
	if (!m_pVmConfig.isValid()) {
		WRITE_TRACE(DBG_FATAL, "Can not load config for uuid %s", QSTR2UTF8(m_sCtUuid));
		goto exit;
	}
	{
		/* LOCK inside brackets */
		CDspLockedPointer<CDspHostInfo> lockedHostInfo = CDspService::instance()->getHostInfo();
		m_cHostInfo.fromString( lockedHostInfo->data()->toString() );
	}

	if (m_nMigrationFlags & PVMT_DIRECT_DATA_CONNECTION)
	{
		nRetCode = PRL_ERR_UNIMPLEMENTED;
		CDspTaskFailure(*this)
			(Error::Simple(nRetCode, "Tunnel-less container migration is not supported").convertToEvent());
		goto exit;
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
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	SmartPtr<IOPackage> pPackage;
	SmartPtr<CVmEvent> pEvent;
	IOSendJob::Response pResponse;
	CDispToDispCommandPtr pCmd;
	CDispToDispResponseCommand *pRespCmd;
	QStringList lstArgs;
	QString sVmHomePath = m_pVmConfig->getVmIdentification()->getHomePath();
	QString ctid;

	if (operationIsCancelled())
		setLastErrorCode(PRL_ERR_OPERATION_WAS_CANCELED);

	ctid = CVzHelper::get_ctid_by_uuid(m_sCtUuid);
	if (ctid.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "Unable to find Container with uuid %s",
			QSTR2UTF8(m_sCtUuid));
		return PRL_ERR_CT_NOT_FOUND;
	}

	if (m_sTargetServerCtHomePath.size())
	{
		// add CTID to target path, as for VM (https://jira.sw.ru/browse/PSBM-14488)
		QString x = m_nMigrationFlags & PVMT_CLONE_MODE ?
			CVzHelper::build_ctid_from_uuid(Uuid::createUuid().toString()) :
			QFileInfo(sVmHomePath).fileName();

		m_sTargetServerCtHomePath = QFileInfo(
			m_sTargetServerCtHomePath, x).absoluteFilePath();
	}

	if (PRL_FAILED(nRetCode = CheckVmMigrationPreconditions()))
		goto exit;

	nRetCode = SendStartRequest();
	if ( PRL_FAILED( nRetCode ) )
		goto exit;

	/* and notify clients about VM migration start event */
	pEvent = SmartPtr<CVmEvent>(new CVmEvent(PET_DSP_EVT_VM_MIGRATE_STARTED, m_sCtUuid, PIE_DISPATCHER));
	pEvent->addEventParameter(new CVmEventParameter(PVE::Boolean, "true", EVT_PARAM_MIGRATE_IS_SOURCE));
	pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, pEvent->toString());
	CDspService::instance()->getClientManager().sendPackageToAllClients(pPackage);

	if (GetLogLevel() > DBG_WARNING)
		lstArgs.append("-v");
	if (m_nMigrationFlags & PVMT_CLONE_MODE)
		lstArgs.append("--keep-src");

	if (m_nMigrationFlags & PVMT_REMOVE_SOURCE_BUNDLE) {
		lstArgs.append("--remove-area");
		lstArgs.append("yes");
	}

	if (m_nMigrationFlags & PVM_FORCE)
		lstArgs.append("-f");

	if (m_nMigrationFlags & PVMT_UNCOMPRESSED)
		lstArgs.append("--nocompress");

	if (!m_sTargetServerCtName.isEmpty())
		lstArgs.append(QString("--new-name=%1").arg(m_sTargetServerCtName));

	/*lstArgs.append("-v");*/
	if (m_nPrevVmState == VMS_RUNNING)
		lstArgs.append("--online");
	/* to force nfs -> non-nfs migration */
	lstArgs.append("--nonsharedfs");

	if (m_nBandwidth > 0)
		lstArgs.append(QString("--limit-speed=%1").arg(m_nBandwidth));

	lstArgs.append(m_sServerHostname);
	lstArgs.append(ctid);

	QObject::connect(m_pIoClient.getImpl(),
		SIGNAL(onResponsePackageReceived(IOSendJob::Handle, const SmartPtr<IOPackage>)),
		SLOT(onCheckResponse(IOSendJob::Handle, const SmartPtr<IOPackage>)),
		Qt::DirectConnection);

	if (PRL_FAILED(nRetCode = startVzMigrate(PRL_CT_MIGRATE_CLIENT, lstArgs,
			boost::bind(&Task_MigrateCtSource::reportProgress, this, _1, _2))))
		goto exit;

	pResponse = m_pIoClient->takeResponse(m_hCheckReqJob);
	if (IOSendJob::NoResponse == pResponse.responseResult)
	{
		nRetCode = execVzMigrate(m_pReply, m_pIoClient.getImpl(), m_hStartReqJob);
		// wait the target task finish.
		if (m_pIoClient->waitForResponse(m_hCheckReqJob, m_nTimeout) != IOSendJob::Success) {
			WRITE_TRACE(DBG_FATAL, "Finish acknowledgement receiving failure");
			nRetCode = PRL_FAILED(nRetCode) ? nRetCode :
						PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
		}
		if (PRL_FAILED(nRetCode))
			goto exit;

		pResponse = m_pIoClient->takeResponse(m_hCheckReqJob);
		if (pResponse.responseResult != IOSendJob::Success) {
			WRITE_TRACE(DBG_FATAL, "Finish acknowledgement receiving failure");
			nRetCode = PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
			goto exit;
		}
	}
	else
		terminate();

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
	m_pIoClient->disconnect(this, SLOT(onCheckResponse(IOSendJob::Handle, const SmartPtr<IOPackage>)));
	setLastErrorCode(nRetCode);
	return nRetCode;
}

void Task_MigrateCtSource::finalizeTask()
{
	SmartPtr<IOPackage> pPackage;

	Disconnect();

	if (PRL_SUCCEEDED(getLastErrorCode())) {

		if ( ! (m_nMigrationFlags & PVMT_CLONE_MODE))
		{
			CDspService::instance()->getVmDirHelper().deleteVmDirectoryItem(m_sVzDirUuid, m_sCtUuid);
			CVmEvent cDelEvent(PET_DSP_EVT_VM_DELETED, m_sCtUuid, PIE_DISPATCHER);
			SmartPtr<IOPackage> pDelPackage =
					DispatcherPackage::createInstance(PVE::DspVmEvent, cDelEvent.toString());
			CDspService::instance()->getClientManager().sendPackageToAllClients(pDelPackage);
		}

		/* prepare event for client before Vm removing */
		CVmEvent cEvent(PET_DSP_EVT_VM_MIGRATE_FINISHED, m_sCtUuid, PIE_DISPATCHER);
		pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, cEvent.toString());
		CDspService::instance()->getClientManager().sendPackageToAllClients(pPackage);

		CProtoCommandPtr pResponse =
			CProtoSerializer::CreateDspWsResponseCommand(getRequestPackage(), PRL_ERR_SUCCESS);
		getClient()->sendResponse(pResponse, getRequestPackage());
	} else {
		if (operationIsCancelled())
			setLastErrorCode(PRL_ERR_OPERATION_WAS_CANCELED);

		/* send response */
		CProtoCommandPtr pResponse =
			CProtoSerializer::CreateDspWsResponseCommand(getRequestPackage(), getLastErrorCode());
		CProtoCommandDspWsResponse *wsResponse =
			CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>( pResponse );
		if (getLastErrorCode() == PRL_ERR_VM_MIGRATE_CHECKING_PRECONDITIONS_FAILED)
			wsResponse->SetParamsList( m_lstCheckPrecondsErrors );
		getLastError()->setEventCode(getLastErrorCode());
		wsResponse->SetError(getLastError()->toString());
		getClient()->sendResponse( pResponse, getRequestPackage() );

		CVmEvent cEvent(PET_DSP_EVT_VM_MIGRATE_CANCELLED, m_sCtUuid, PIE_DISPATCHER);
		pPackage = DispatcherPackage::createInstance(PVE::DspVmEvent, cEvent.toString());
		CDspService::instance()->getClientManager().sendPackageToAllClients(pPackage);

		getClient()->sendResponseError( getLastError(), getRequestPackage() );
	}
	if (m_bExVmOperationRegistered)
		CDspService::instance()->getVmDirHelper().unregisterExclusiveVmOperation(
				m_sCtUuid, m_sVzDirUuid, PVE::DspCmdDirVmMigrate, getClient());
}

void Task_MigrateCtSource::terminate()
{
	terminateVzMigrate();
	terminateHandleDispPackageTask();
}

void Task_MigrateCtSource::onCheckResponse(IOSendJob::Handle job_, const SmartPtr<IOPackage> package_)
{
	if (job_ != m_hCheckReqJob)
		return;

	CDispToDispCommandPtr x = CDispToDispProtoSerializer::ParseCommand(
			DispToDispResponseCmd, UTF8_2QSTR(package_->buffers[0].getImpl()));
	CDispToDispResponseCommand* y =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(x);
	if (NULL == y || PRL_FAILED(y->GetRetCode()))
	{
		WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);
		terminate();
	}
}

// cancel command
void Task_MigrateCtSource::cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p)
{
	WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);

	terminate();
	CancelOperationSupport::cancelOperation(pUser, p);
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
			m_sTargetServerCtName,
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
		m_nBandwidth = getDegree();
		quint64 targetBw = pResponseCmd->GetBandwidth();
		if (targetBw > 0 && targetBw < m_nBandwidth)
			m_nBandwidth = targetBw;
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
	IOSendJob::Handle j(m_pIoClient->sendPackage(pPackage));
	while (!j.isValid() || m_pIoClient->getSendResult(j) == IOSendJob::SendQueueIsFull)
	{
		if (m_sent.empty())
		{
			WRITE_TRACE(DBG_FATAL, "Package sending failure");
			return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
		}
		j = m_sent.takeFirst();
		if (IOSendJob::Success != m_pIoClient->waitForSend(j, m_nTimeout))
		{
			WRITE_TRACE(DBG_FATAL, "Package sending failure");
			return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
		}
		j = m_pIoClient->sendPackage(pPackage);
	}
	bool x = m_sent.isEmpty();
	m_sent.push_back(j);
	while (!x && !m_sent.empty())
	{
		switch (m_pIoClient->getSendResult(m_sent.front()))
		{
		case IOSendJob::Success:
			m_sent.pop_front();
			break;
		case IOSendJob::SendPended:
			return PRL_ERR_SUCCESS;
		default:
			WRITE_TRACE(DBG_FATAL, "Package sending failure");
			return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
		}
	}
	return PRL_ERR_SUCCESS;
}

void Task_MigrateCtSource::reportProgress(const QString& stage_, int progress_)
{
	CVmEvent event(PET_JOB_STAGE_PROGRESS_CHANGED, m_sCtUuid, PIE_DISPATCHER);

	event.addEventParameter(new CVmEventParameter(
				PVE::UnsignedInt,
				QString::number(progress_),
				EVT_PARAM_PROGRESS_CHANGED));

	event.addEventParameter(new CVmEventParameter(
				PVE::String,
				stage_,
				EVT_PARAM_PROGRESS_STAGE));

	SmartPtr<IOPackage> p =	DispatcherPackage::createInstance(PVE::DspVmEvent, event,
			getRequestPackage());

	getClient()->sendPackage(p);
}
