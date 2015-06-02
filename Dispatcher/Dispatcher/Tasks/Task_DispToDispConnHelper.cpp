///////////////////////////////////////////////////////////////////////////////
///
/// @file Task_DispToDispConnHelper.cpp
///
/// Common functions for connection to remote dispatcher and send message
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

#include "Libraries/Logging/Logging.h"

#include "Task_DispToDispConnHelper.h"
#include "CDspService.h"
#include "Libraries/Std/PrlAssert.h"

Task_DispToDispConnHelper::Task_DispToDispConnHelper(CVmEvent *pEvent)
:m_pEvent(pEvent)
{
	m_nTimeout = (quint32)
		CDspService::instance()->getDispConfigGuard().getDispToDispPrefs()->getSendReceiveTimeout() * 1000;
}

Task_DispToDispConnHelper::~Task_DispToDispConnHelper()
{
}

bool Task_DispToDispConnHelper::isConnected()
{
	return (m_pIoClient.getImpl() && m_pIoClient->state() == IOSender::Connected);
}

PRL_RESULT Task_DispToDispConnHelper::Connect(
			QString sServerHostname,
			quint32 nServerPort,
			QString sServerSessionUuid,
			QString sUser,
			QString sPassword,
			quint32 nFlags)
{
	if (isCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	PRL_SECURITY_LEVEL connSec = PVM_GET_SECURITY_LEVEL(nFlags);

	WRITE_TRACE(DBG_FATAL, "connect to the target dispatcher: host '%s' port %u",\
		qPrintable(sServerHostname), nServerPort);

	// Create IO client
	m_pIoClient = SmartPtr<IOClient>(new IOClient(
		IORoutingTableHelper::GetClientRoutingTable(connSec),
		IOSender::Dispatcher, sServerHostname, nServerPort));

	m_pIoClient->connectClient(
		CDspService::instance()->getDispConfigGuard().getDispToDispPrefs()->getConnectionTimeout() * 1000);
	IOSender::State state = m_pIoClient->waitForConnection();
	if (state != IOSender::Connected) {
		WRITE_TRACE(DBG_FATAL, "Can't connect to the target dispatcher: host '%s' port %u",\
			qPrintable(sServerHostname), nServerPort);

		m_pEvent->addEventParameter(
			new CVmEventParameter(PVE::String,
				sServerHostname,
				EVT_PARAM_MESSAGE_PARAM_0 ) );

		if ( m_pIoClient->error() == IOSender::HandshakeError ) {
			nRetCode = PRL_ERR_HANDSHAKE_FAILED;
		} else if ( m_pIoClient->error() == IOSender::ProtocolVersionError ) {
			IOCommunication::ProtocolVersion ver;
			m_pIoClient->serverProtocolVersion(ver);
			m_pEvent->addEventParameter(
				new CVmEventParameter(PVE::String,
					QString(IOService::IOProtocolVersion.whatWeAre),
					EVT_PARAM_MESSAGE_PARAM_1 ) );
			m_pEvent->addEventParameter(
				new CVmEventParameter(PVE::String,
					QString(ver.whatWeAre),
					EVT_PARAM_MESSAGE_PARAM_2 ) );

			nRetCode = PRL_ERR_WRONG_PROTOCOL_VERSION;
		} else if ( m_pIoClient->error() == IOSender::SSLHandshakeError ) {
			nRetCode = PRL_ERR_SSL_HANDSHAKE_FAILED;
		} else if ( m_pIoClient->error() == IOSender::RoutingTableAcceptError ) {
			nRetCode = PRL_ERR_WRONG_CONNECTION_SECURITY_LEVEL;
		} else if ( m_pIoClient->error() == IOSender::HostnameResolveError ) {
			m_pEvent->addEventParameter(
				new CVmEventParameter(PVE::String,
					QHostInfo::localHostName(),
					EVT_PARAM_MESSAGE_PARAM_1));
			nRetCode = PRL_ERR_CANT_RESOLVE_HOSTNAME;
		} else {
			m_pEvent->addEventParameter(
				new CVmEventParameter(PVE::String,
					QHostInfo::localHostName(),
					EVT_PARAM_MESSAGE_PARAM_1));
			nRetCode = PRL_ERR_CANT_CONNECT_TO_DISPATCHER_ITERATIVELY;
		}
		return nRetCode;
	}


	CDispToDispCommandPtr pAuthorizeCmd;
	if (sServerSessionUuid.isEmpty())
		pAuthorizeCmd =	CDispToDispProtoSerializer::CreateDispToDispAuthorizeCommand(sUser, sPassword);
	else
		pAuthorizeCmd = CDispToDispProtoSerializer::CreateDispToDispAuthorizeCommand(sServerSessionUuid);

	SmartPtr<IOPackage> pPackage =
		DispatcherPackage::createInstance(pAuthorizeCmd->GetCommandId(), pAuthorizeCmd->GetCommand()->toString());

	nRetCode = SendReqAndWaitReply(pPackage);

	return nRetCode;
}

void Task_DispToDispConnHelper::Disconnect()
{
	if (!isConnected())
		return;
	CDispToDispCommandPtr pLogoffCmd =
		CDispToDispProtoSerializer::CreateDispToDispCommandWithoutParams(DispToDispLogoffCmd);
	SmartPtr<IOPackage> pPackage =
		DispatcherPackage::createInstance(pLogoffCmd->GetCommandId(), pLogoffCmd->GetCommand()->toString());
	SendPkg(pPackage);
}

PRL_RESULT Task_DispToDispConnHelper::SendPkg(const SmartPtr<IOPackage> &package)
{
	if (isCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	IOSendJob::Handle job;

	job = m_pIoClient->sendPackage(package);
	if (m_pIoClient->waitForSend(job, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Package sending failure");
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	}

	return PRL_ERR_SUCCESS;
}

PRL_RESULT Task_DispToDispConnHelper::SendReqAndWaitReply(const SmartPtr<IOPackage> &package)
{
	if (isCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	PRL_RESULT nRetCode = PRL_ERR_SUCCESS;
	IOSendJob::Handle hJob;
	SmartPtr<IOPackage> pReply;

	if (PRL_FAILED(nRetCode = SendReqAndWaitReply(package, pReply, hJob)))
		return nRetCode;

	if (pReply->header.type != DispToDispResponseCmd ) {
		WRITE_TRACE(DBG_FATAL, "Invalid package header:%x,  expected header:%x",
			pReply->header.type, DispToDispResponseCmd);
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	}

	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(
		DispToDispResponseCmd, UTF8_2QSTR(pReply->buffers[0].getImpl()));
	CDispToDispResponseCommand *pResponseCmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pCmd);
	if (PRL_FAILED(nRetCode = pResponseCmd->GetRetCode())) {
		m_pEvent->fromString(pResponseCmd->GetErrorInfo()->toString());
		return nRetCode;
	}
	return nRetCode;
}
PRL_RESULT Task_DispToDispConnHelper::SendReqAndWaitReply(
				const SmartPtr<IOPackage> &package,
				SmartPtr<IOPackage> &pReply)
{
	IOSendJob::Handle hJob;
	return SendReqAndWaitReply(package, pReply, hJob);
}


PRL_RESULT Task_DispToDispConnHelper::SendReqAndWaitReply(
				const SmartPtr<IOPackage> &package,
				SmartPtr<IOPackage> &pReply, IOSendJob::Handle &hJob)
{
	if (isCancelled())
		return PRL_ERR_OPERATION_WAS_CANCELED;

	IOSendJob::Response resp;

	hJob = m_pIoClient->sendPackage(package);
	if (m_pIoClient->waitForSend(hJob, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Package sending failure");
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	}
	if (m_pIoClient->waitForResponse(hJob, m_nTimeout) != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Responce receiving failure");
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	}
	resp = m_pIoClient->takeResponse(hJob);
	if (resp.responseResult != IOSendJob::Success) {
		WRITE_TRACE(DBG_FATAL, "Job failure: responseResult:%x", resp.responseResult);
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	}

	pReply = resp.responsePackages[0];
	if (!pReply.isValid()) {
		WRITE_TRACE(DBG_FATAL, "Invalid replay");
		return PRL_ERR_BACKUP_INTERNAL_PROTO_ERROR;
	}

	return PRL_ERR_SUCCESS;
}

