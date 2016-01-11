///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_CopyCtTemplateSource.cpp
///
/// Source task for copy of CT template
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
#include <prlcommon/Interfaces/ParallelsQt.h>
#include <prlcommon/Interfaces/ParallelsNamespace.h>

#include <prlcommon/Logging/Logging.h>

#include "Task_CopyCtTemplate.h"
#include "CDspService.h"
#include <prlcommon/Std/PrlAssert.h>
//#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/DispToDispProtocols/CCtTemplateProto.h"
#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"
//#include "CDspVzHelper.h"

Task_CopyCtTemplateSource::Task_CopyCtTemplateSource(
		const SmartPtr<CDspClient>& client,
		const CProtoCommandPtr cmd,
		const SmartPtr<IOPackage>& p)
:Task_VzMigrate(client, p)
{
	CProtoCopyCtTemplateCommand *pCmd = CProtoSerializer::CastToProtoCommand<CProtoCopyCtTemplateCommand>(cmd);
	PRL_ASSERT(pCmd->IsValid());
	m_nVersion = pCmd->GetVersion();
	m_sTmplName = pCmd->GetTmplName();
	m_sOsTmplName = pCmd->GetOsTmplName();
	m_sServerHostname = pCmd->GetTargetServerHostname();
	m_nServerPort = pCmd->GetTargetServerPort();
	if (m_nServerPort == 0)
		m_nServerPort = CDspService::getDefaultListenPort();
	m_sServerSessionUuid = pCmd->GetTargetServerSessionUuid();
	m_nFlags = pCmd->GetCommandFlags();
	m_nReservedFlags = pCmd->GetReservedFlags();
}

PRL_RESULT Task_CopyCtTemplateSource::prepareTask()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;

	nRetCode = Connect(
		m_sServerHostname, m_nServerPort, m_sServerSessionUuid, QString(), QString(), m_nFlags);
	if (PRL_FAILED(nRetCode))
		goto exit;

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

PRL_RESULT Task_CopyCtTemplateSource::run_body()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	SmartPtr<IOPackage> pPackage;
	IOSendJob::Handle hJob;
	IOSendJob::Response pResponse;
	CDispToDispCommandPtr pCmd;
	CDispToDispResponseCommand *pResponseCmd;
	CCopyCtTemplateReply *pCCReplyCmd;

	QStringList lstArgs;
	SmartPtr<IOPackage> pReply;

	if (operationIsCancelled())
		setLastErrorCode(PRL_ERR_OPERATION_WAS_CANCELED);

	CDispToDispCommandPtr pRequest = CDispToDispProtoSerializer::CreateCopyCtTemplateCommand(
		m_sTmplName, m_sOsTmplName, m_nFlags, m_nReservedFlags);

	pPackage = DispatcherPackage::createInstance(pRequest->GetCommandId(), pRequest->GetCommand()->toString());
	hJob = m_pIoClient->sendPackage(pPackage);
	if (m_pIoClient->waitForSend(hJob, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Package sending failure");
		nRetCode = PRL_ERR_COPY_CT_TMPL_INTERNAL_ERROR;
		goto exit;
	}
	if (m_pIoClient->waitForResponse(hJob, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Responce receiving failure");
		nRetCode = PRL_ERR_COPY_CT_TMPL_INTERNAL_ERROR;
		goto exit;
	}
	pResponse = m_pIoClient->takeResponse(hJob);
	if (pResponse.responseResult != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Job failure: responseResult:%x", pResponse.responseResult);
		nRetCode = PRL_ERR_COPY_CT_TMPL_INTERNAL_ERROR;
		goto exit;
	}

	pReply = pResponse.responsePackages[0];
	if (!pReply.isValid()) {
		WRITE_TRACE(DBG_FATAL, "Invalid reply");
		nRetCode = PRL_ERR_COPY_CT_TMPL_INTERNAL_ERROR;
		goto exit;
	}
	if ((pReply->header.type != DispToDispResponseCmd) && (pReply->header.type != CopyCtTemplateReply)) {
		WRITE_TRACE(DBG_FATAL, "Unexpected response type on start command: %d", pReply->header.type);
		nRetCode = PRL_ERR_COPY_CT_TMPL_INTERNAL_ERROR;
		goto exit;
	}

	pCmd = CDispToDispProtoSerializer::ParseCommand(
			DispToDispResponseCmd, UTF8_2QSTR(pReply->buffers[0].getImpl()));
	if (pReply->header.type == DispToDispResponseCmd) {
		pResponseCmd =
			CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pCmd);
		nRetCode = pResponseCmd->GetRetCode();
		getLastError()->fromString(pResponseCmd->GetErrorInfo()->toString());
		goto exit;
	}
	pCCReplyCmd = CDispToDispProtoSerializer::CastToDispToDispCommand<CCopyCtTemplateReply>(pCmd);
	m_nVersion = pCCReplyCmd->GetVersion();
	// pCCReplyCmd->GetCommandFlags();

	/*lstArgs.append("-v");*/
	lstArgs.append("-z");
	lstArgs.append(m_sServerHostname);
	if (m_sOsTmplName.isEmpty())
		lstArgs.append(m_sTmplName);
	else
		lstArgs.append(QString("%1@%2").arg(m_sTmplName).arg(m_sOsTmplName));

	nRetCode = startVzMigrate(PRL_COPT_CT_TEMPLATE_CLIENT, lstArgs);
	if (PRL_FAILED(nRetCode))
		goto exit;

	nRetCode = execVzMigrate(pReply, m_pIoClient.getImpl(), hJob);
	if (PRL_FAILED(nRetCode))
		goto exit;

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

void Task_CopyCtTemplateSource::finalizeTask()
{
	SmartPtr<IOPackage> pPackage;

	Disconnect();

	if (PRL_SUCCEEDED(getLastErrorCode())) {
		CProtoCommandPtr pResponse =
			CProtoSerializer::CreateDspWsResponseCommand(getRequestPackage(), PRL_ERR_SUCCESS);
		getClient()->sendResponse(pResponse, getRequestPackage());
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
}

// cancel command
void Task_CopyCtTemplateSource::cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p)
{
	WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);

	terminateVzMigrate();
	terminateHandleDispPackageTask();

	CancelOperationSupport::cancelOperation(pUser, p);
}

/* send package with migration data to target dispatcher */
PRL_RESULT Task_CopyCtTemplateSource::sendDispPackage(SmartPtr<IOPackage> &pPackage)
{
	IOSendJob::Handle hJob;

	hJob = m_pIoClient->sendPackage(pPackage);
	if (m_pIoClient->waitForSend(hJob, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Package sending failure");
		return PRL_ERR_COPY_CT_TMPL_INTERNAL_ERROR;
	}
	return PRL_ERR_SUCCESS;
}



/*
 Target-side task
*/
Task_CopyCtTemplateTarget::Task_CopyCtTemplateTarget(
		const SmartPtr<CDspDispConnection> &pDispConnection,
		CDispToDispCommandPtr cmd,
		const SmartPtr<IOPackage> &p)
:Task_VzMigrate(pDispConnection->getUserSession(), p),
m_pDispConnection(pDispConnection)
{
	CCopyCtTemplateCommand * pCmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CCopyCtTemplateCommand>(cmd);
	PRL_ASSERT(pCmd->IsValid());

	m_hConnHandle = pDispConnection->GetConnectionHandle();

	m_nVersion = pCmd->GetVersion();
	m_sTmplName = pCmd->GetTmplName();
	m_sOsTmplName = pCmd->GetOsTmplName();
	m_nFlags = pCmd->GetCommandFlags();
	m_nReservedFlags = pCmd->GetReservedFlags();

	bool bConnected = QObject::connect(
		&CDspService::instance()->getIOServer(),
		SIGNAL(onClientDisconnected(IOSender::Handle)),
		SLOT(clientDisconnected(IOSender::Handle)),
		Qt::DirectConnection);
	PRL_ASSERT(bConnected);
}

Task_CopyCtTemplateTarget::~Task_CopyCtTemplateTarget()
{
	m_waiter.waitUnlockAndFinalize();
}

PRL_RESULT Task_CopyCtTemplateTarget::run_body()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	CDispToDispCommandPtr pReply;
	SmartPtr<IOPackage> pPackage;
	IOSendJob::Handle hJob;
	QStringList lstArgs;

	if (operationIsCancelled()) {
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit;
	}

	/*lstArgs.append("-v");*/
	lstArgs.append("-z");
	lstArgs.append("localhost");
	if (m_sOsTmplName.isEmpty())
		lstArgs.append(m_sTmplName);
	else
		lstArgs.append(QString("%1@%2").arg(m_sTmplName).arg(m_sOsTmplName));

	nRetCode = startVzMigrate(PRL_COPT_CT_TEMPLATE_SERVER, lstArgs);
	if (PRL_FAILED(nRetCode))
		goto exit;

	/* send reply */
	pReply = CDispToDispProtoSerializer::CreateCopyCtTemplateReply();
	pPackage = DispatcherPackage::createInstance(
		pReply->GetCommandId(), pReply->GetCommand()->toString(), getRequestPackage());
	hJob = m_pDispConnection->sendPackage(pPackage);

	nRetCode = execVzMigrate(getRequestPackage(), &CDspService::instance()->getIOServer(), hJob);
exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

void Task_CopyCtTemplateTarget::finalizeTask()
{
	IOSendJob::Handle hJob;

	QObject::disconnect(
			&CDspService::instance()->getIOServer(),
			SIGNAL(onClientDisconnected(IOSender::Handle)),
			this,
			SLOT(clientDisconnected(IOSender::Handle)));

	if (PRL_SUCCEEDED(getLastErrorCode())) {
		hJob = m_pDispConnection->sendSimpleResponse(getRequestPackage(), getLastErrorCode());
	} else {
		hJob = m_pDispConnection->sendResponseError(getLastError(), getRequestPackage());
	}
	CDspService::instance()->getIOServer().waitForSend(hJob, m_nTimeout);
}

void Task_CopyCtTemplateTarget::clientDisconnected(IOSender::Handle h)
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

// cancel command
void Task_CopyCtTemplateTarget::cancelOperation(SmartPtr<CDspClient> pUser, const SmartPtr<IOPackage>& p)
{
	WRITE_TRACE(DBG_FATAL, "%s", __FUNCTION__);

	terminateVzMigrate();
	terminateHandleDispPackageTask();

	CancelOperationSupport::cancelOperation(pUser, p);
}

/* send package with migration data to target dispatcher */
PRL_RESULT Task_CopyCtTemplateTarget::sendDispPackage(SmartPtr<IOPackage> &pPackage)
{
	IOSendJob::Handle hJob;

	hJob = m_pDispConnection->sendPackage(pPackage);
	if (CDspService::instance()->getIOServer().waitForSend(hJob, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Package sending failure");
		return PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
	}
	return PRL_ERR_SUCCESS;
}

