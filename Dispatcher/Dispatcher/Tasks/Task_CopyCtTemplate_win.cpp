///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_MigrateVmSource.cpp
///
/// Dispatcher task for VZWIN CT template migration
///
/// @author yur@
///
/// Copyright (c) 2011-2015 Parallels IP Holdings GmbH
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
#include <prlcommon/Std/PrlTime.h>

#include "CDspService.h"
#include <prlcommon/Std/PrlAssert.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/StatesUtils/StatesHelper.h"
#include "Libraries/DispToDispProtocols/CCtTemplateProto.h"
#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"

#include "Libraries/Virtuozzo/CVzHelper.h"

#include "Task_CopyCtTemplate_win.h"

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

Task_CopyCtTemplateSource::Task_CopyCtTemplateSource(
		const SmartPtr<CDspClient>& client,
		const CProtoCommandPtr cmd,
		const SmartPtr<IOPackage>& p)
	: CDspTaskHelper(client, p),
	  Task_DispToDispConnHelper(getLastError())
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
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );
	QString sPath;
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	SmartPtr<CtTemplate> ctTemplate;

	if (CDspService::instance()->isServerStopping()) {
		WRITE_TRACE(DBG_FATAL, "Dispatcher shutdown is in progress - operation rejected!");
		nRetCode = PRL_ERR_DISP_SHUTDOWN_IN_PROCESS;
		goto exit;
	}

	nRetCode = CVzHelper::getTemplateInfo(m_sTmplName, &ctTemplate);
	if (PRL_FAILED(nRetCode)) {
		setLastErrorCode(nRetCode);
		goto exit;
	}
	m_sTmplName = ctTemplate->getName();

	if (CDspService::instance()->getShellServiceHelper().isLocalAddress(m_sServerHostname)) {
		WRITE_TRACE(DBG_FATAL,
			    "Host %s is a local host, migration is impossible", QSTR2UTF8(m_sServerHostname));
		nRetCode = PRL_ERR_VM_MIGRATE_TO_THE_SAME_NODE;
		goto exit;
	}

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
	SmartPtr<IOPackage> pReply;
	CDispToDispCommandPtr pCmd;
	CDispToDispResponseCommand *pResponseCmd;
	CCopyCtTemplateReply *pCCReplyCmd;

	QString sEfdPath;
	bool isArchived = false;
	nRetCode = CVzHelper::getTemplateInfo(m_sTmplName, NULL, NULL, &sEfdPath, NULL, &isArchived);
	if (PRL_FAILED(nRetCode)) {
		setLastErrorCode(nRetCode);
		return nRetCode;
	}
	if (!isArchived || sEfdPath.isEmpty()) {
		WRITE_TRACE(DBG_FATAL, "Cannot migrate CT template %s without efd archive", QSTR2UTF8(m_sTmplName));
		nRetCode = PRL_ERR_TEMPLATE_NOT_FOUND;
		setLastErrorCode(nRetCode);
		return nRetCode;
	}

	QFileInfo fInfo(sEfdPath);
	QList<QPair<QFileInfo, QString> > fList;
	fList.append(qMakePair(fInfo, fInfo.fileName()));
	QList<QPair<QFileInfo, QString> > dList;

	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

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
		WRITE_TRACE(DBG_FATAL, "Unexpected response type on CopyCtTemplate command: %d", pReply->header.type);
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


	m_pSender = SmartPtr<CVmFileListCopySender>(new CVmFileListCopySenderClient(m_pIoClient));

	// Should be NULL before atomic assign for race-free check in cancelOperation()
	PRL_ASSERT(!m_pCopySource);

	m_pCopySource = SmartPtr<CVmFileListCopySource>(
		new CVmFileListCopySource(
			m_pSender.getImpl(),
			getClient()->getSessionUuid(),
			/* home */ NULL,
			1,
			getLastError(),
			m_nTimeout));
	m_pCopySource->SetRequest(getRequestPackage());
	//m_pCopySource->SetVmDirectoryUuid(m_sVmDirUuid);
	m_pCopySource->SetProgressNotifySender(NotifyClientsWithProgress);

	nRetCode = m_pCopySource->Copy(dList, fList);

	if (nRetCode == PRL_ERR_FILECOPY_FILE_EXIST)
		nRetCode = PRL_ERR_SUCCESS;

	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "Error occurred while migration with code [%#x][%s]",
			    nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		goto exit;
	}


	{
		SmartPtr<IOPackage> respPkg;
		IOSendJob::Response pResponse;
		bool bExited = false;

		// Handle reply from target
		while (!bExited) {
			if (getIoClient()->waitForResponse(hJob) != IOSendJob::Success) {
				WRITE_TRACE(DBG_FATAL, "Responce receiving failure");
				nRetCode = PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
				break;
			}
			pResponse = getIoClient()->takeResponse(hJob);
			if (pResponse.responseResult != IOSendJob::Success) {
				WRITE_TRACE(DBG_FATAL, "Copy CT template failed to take response: %x",
					    pResponse.responseResult);
				nRetCode = PRL_ERR_CT_MIGRATE_INTERNAL_ERROR;
				break;
			}

			foreach(respPkg, pResponse.responsePackages) {
				if (respPkg->header.type == PVE::DspVmEvent) {
					// FIXME: handle progress
					continue;
				} else if (respPkg->header.type == DispToDispResponseCmd) {
					// Task finished
					CDispToDispCommandPtr pCmd  = CDispToDispProtoSerializer::ParseCommand(DispToDispResponseCmd,
							UTF8_2QSTR(respPkg->buffers[0].getImpl()));
					CDispToDispResponseCommand *pResponseCmd =
						CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pCmd);
					getLastError()->fromString(pResponseCmd->GetErrorInfo()->toString());
					nRetCode = pResponseCmd->GetRetCode();
					bExited = true;
					break;
				} else {
					WRITE_TRACE(DBG_FATAL, "Unexpected package with type %d",
						respPkg->header.type);
				}
			}
		}
	}

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

void Task_CopyCtTemplateSource::finalizeTask()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );
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

	CancelOperationSupport::cancelOperation(pUser, p);
	if (m_pCopySource.getImpl())
		m_pCopySource->cancelOperation();
}

/******************************** target ********************************/

Task_CopyCtTemplateTarget::Task_CopyCtTemplateTarget(
		const SmartPtr<CDspDispConnection> &pDispConnection,
		CDispToDispCommandPtr cmd,
		const SmartPtr<IOPackage> &p)
 :
	CDspTaskHelper(pDispConnection->getUserSession(), p),
	Task_DispToDispConnHelper(getLastError()),
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

/* process VmMigrateCheckPreconditionsCommand */
PRL_RESULT Task_CopyCtTemplateTarget::prepareTask()
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=267152
	CAuthHelperImpersonateWrapper _impersonate( &getClient()->getAuthHelper() );

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	QString sVmDirPath;

	if (CDspService::instance()->isServerStopping()) {
		WRITE_TRACE(DBG_FATAL, "Dispatcher shutdown is in progress - migrate rejected!");
		nRetCode = PRL_ERR_DISP_SHUTDOWN_IN_PROCESS;
		goto exit;
	}

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

PRL_RESULT Task_CopyCtTemplateTarget::run_body()
{
	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	CDispToDispCommandPtr pReply;
	SmartPtr<IOPackage> pPackage;
	IOSendJob::Handle hJob;
	bool bOverwriteMode = false;

	QString sEfdDir = CVzHelper::getVzTemplateDir() + QString("\\\\__cache");
	SmartPtr<CtTemplate> ctTemplate;
	QString sInstallPath;
	QString sEfdPath;
	QString sTmplName;
	bool isInstalled = false;
	bool isArchived = false;

	CVzHelper::getTemplateInfo(m_sTmplName,
				   &ctTemplate, &sInstallPath, &sEfdPath,
				   &isInstalled, &isArchived);
	if (ctTemplate.getImpl())
		sTmplName = ctTemplate->getName();

	WRITE_TRACE(DBG_DEBUG, "source '%s', dest '%s', efd '%s', installed %d, archived %d",
		    QSTR2UTF8(m_sTmplName), QSTR2UTF8(sTmplName), QSTR2UTF8(sEfdPath),
		    isInstalled, isArchived);

	if (operationIsCancelled()) {
		nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
		goto exit;
	}

	if (sTmplName.compare(m_sTmplName, Qt::CaseInsensitive) == 0) {
		// Template already exists on source, at least as efd archive.
		// CVmFileListCopySender may return PRL_ERR_FILECOPY_FILE_EXIST,
		// but we will report PRL_ERR_SUCCESS as task result.
	} else {
		// Efd file may have different name, which we will know only from source,
		// so need to set overrite mode for CVmFileListCopyTarget unconditionally.
		bOverwriteMode = true;
	}

	// Remove may be partial incomplete efd archive and re-get it from source.
	if (!sEfdPath.isEmpty() && !isArchived) {
		QFile(sEfdPath).remove();
	}

	pReply = CDispToDispProtoSerializer::CreateCopyCtTemplateReply();
	pPackage = DispatcherPackage::createInstance(
		pReply->GetCommandId(), pReply->GetCommand()->toString(), getRequestPackage());
	m_pDispConnection->sendPackage(pPackage);

	m_pSender = SmartPtr<CVmFileListCopySender>(
		new CVmFileListCopySenderServer(
			CDspService::instance()->getIOServer(),
			m_pDispConnection->GetConnectionHandle()));
	/* CVmFileListCopyTarget will use Vm uuid in progress messages for clients, so will use original Vm uuid */
	m_pTarget = SmartPtr<CVmFileListCopyTarget>(
		new CVmFileListCopyTarget(
			m_pSender.getImpl(), /*uuid*/ NULL, sEfdDir, NULL, m_nTimeout, bOverwriteMode));

	// Connect to handle traffic report package
	bool bConnected = QObject::connect(
		m_pDispConnection.getImpl(),
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		SLOT(handlePackage(IOSender::Handle, const SmartPtr<IOPackage>)),
		Qt::DirectConnection );
	//PRL_ASSERT(bConnected);
	WRITE_TRACE(DBG_DEBUG, "bConnected %d", bConnected);

	nRetCode = QThread::exec();

	// do not fail if dest file already exists
	if (nRetCode == PRL_ERR_FILECOPY_FILE_EXIST)
		nRetCode = PRL_ERR_SUCCESS;

	QObject::disconnect(
		m_pDispConnection.getImpl(),
		SIGNAL(onPackageReceived(IOSender::Handle, const SmartPtr<IOPackage>)),
		this,
		SLOT(handlePackage(IOSender::Handle, const SmartPtr<IOPackage>)));

	if (!isInstalled) {
		if (operationIsCancelled()) {
			nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
			goto exit;
		}
		WRITE_TRACE(DBG_DEBUG, "install template '%s'", QSTR2UTF8(m_sTmplName));
		nRetCode = CVzHelper::install_template(m_sTmplName, m_sOsTmplName);
	}

exit:
	setLastErrorCode(nRetCode);
	return nRetCode;
}

void Task_CopyCtTemplateTarget::finalizeTask()
{
	QObject::disconnect(
		&CDspService::instance()->getIOServer(),
		SIGNAL(onClientDisconnected(IOSender::Handle)),
		this,
		SLOT(clientDisconnected(IOSender::Handle)));
	IOSendJob::Handle hJob;
	SmartPtr<IOPackage> pPackage;
	if (PRL_SUCCEEDED(getLastErrorCode())) {
		hJob = m_pDispConnection->sendSimpleResponse(getRequestPackage(), PRL_ERR_SUCCESS);
	} else {
		// TODO: need to remove incomplete file.
		hJob = m_pDispConnection->sendResponseError(getLastError(), getRequestPackage());
	}
	CDspService::instance()->getIOServer().waitForSend(hJob, m_nTimeout);
}

void Task_CopyCtTemplateTarget::handlePackage(IOSender::Handle h, const SmartPtr<IOPackage> p)
{
	PRL_RESULT nRetCode;
	bool bExit;

	PRL_ASSERT(m_pTarget.getImpl());

	if (h != m_pDispConnection->GetConnectionHandle())
		return;

	if (IS_FILE_COPY_PACKAGE(p->header.type)) {
		nRetCode = m_pTarget->handlePackage(p, &bExit);
		if (bExit)
			QThread::exit(nRetCode);
	} else if (p->header.type == VmMigrateCancelCmd) {
		WRITE_TRACE(DBG_DEBUG, "Migration was cancelled");
		QThread::exit(PRL_ERR_OPERATION_WAS_CANCELED);
	} else {
		WRITE_TRACE(DBG_FATAL, "Invalid package type %d, ignored", p->header.type);
	}
}

void Task_CopyCtTemplateTarget::clientDisconnected(IOSender::Handle h)
{
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
	CancelOperationSupport::cancelOperation(pUser, p);

	if (m_pTarget.isValid())
		m_pTarget->cancelOperation();

	QThread::exit(PRL_ERR_OPERATION_WAS_CANCELED);
}

