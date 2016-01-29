///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspDispConnectionsManager.cpp
///
/// Manager of connections from another dispatchers to this one
///
/// @author sandro
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
///////////////////////////////////////////////////////////////////////////////

#include "CDspDispConnectionsManager.h"
#include "CDspService.h"
#include "CDspClientManager.h"
#include "CDspRouter.h"
#include "CDspVm.h"

#include <prlcommon/Std/PrlAssert.h>
#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"

#include "CDspVzHelper.h"

using namespace Parallels;

namespace {

bool is_migrate_protocol_package(int header_type)
{
	return header_type > VmMigrateRangeStart && header_type < VmMigrateRangeEnd;
}

bool is_migrate_package(int header_type)
{
	return is_migrate_protocol_package(header_type) || IS_FILE_COPY_PACKAGE(header_type);
}

}; // namespace

REGISTER_HANDLER( IOService::IOSender::Dispatcher,
				  "DispToDispHandler",
				  CDspDispConnectionsManager);

/*****************************************************************************/

CDspDispConnectionsManager::CDspDispConnectionsManager ( IOSender::Type type, const char* name ) :
	CDspHandler(type, name)
{
}

CDspDispConnectionsManager::~CDspDispConnectionsManager ()
{
}

void CDspDispConnectionsManager::handleClientDisconnected ( const IOSender::Handle& h )
{
	DeleteDispConnection(h);
}

void CDspDispConnectionsManager::DeleteDispConnection( const IOSender::Handle& h )
{
	QWriteLocker locker( &m_rwLock );
	m_dispconns.take(h);
}

void CDspDispConnectionsManager::handleToDispatcherPackage ( const IOSender::Handle& h,
												const SmartPtr<IOPackage> &p )
{
	unsigned int loglevel;
 	LOG_MESSAGE(DBG_DEBUG, "%s received package [%s]", __FUNCTION__, p->buffers[0].getImpl());

	if (IS_FILE_COPY_PACKAGE(p->header.type) || IS_ABACKUP_PROXY_PACKAGE(p->header.type) ||
			(p->header.type == CtMigrateCmd))
		/* trace backup command on debug mode only */
		loglevel = DBG_DEBUG;
	else
		loglevel = DBG_FATAL;

	WRITE_TRACE(loglevel, "Processing command %d", p->header.type);

	switch( p->header.type )
	{
		case DispToDispAuthorizeCmd:
		{
			PRL_RESULT ret;
			do {

				if (CDspService::instance()->isServerStopping())
				{
					WRITE_TRACE(DBG_FATAL, "Dispatcher shutdown is in progress!");
					ret = PRL_ERR_DISP_SHUTDOWN_IN_PROCESS;
					break;
				}

				m_rwLock.lockForRead();
				bool cliExists = m_dispconns.contains(h);
				m_rwLock.unlock();

				ret = PRL_ERR_DISP2DISP_SESSION_ALREADY_AUTHORIZED;
				if ( cliExists )
					break;

				SmartPtr<CDspDispConnection> pDispConnection =
					AuthorizeDispatcherConnection( h, p );
				if (!pDispConnection)
					// Error response should be sent by AuthorizeDispatcherConnection()
					//" method, so do nothing, just return!
					return;

				m_rwLock.lockForWrite();
				/* We have to recheck connection because lock was dropped. */
				if (m_dispconns.contains(h)) {
					m_rwLock.unlock();
					ret = PRL_ERR_DISP2DISP_SESSION_ALREADY_AUTHORIZED;
					break;
				}

				m_dispconns[h] = pDispConnection;
				m_rwLock.unlock();
				ret = PRL_ERR_SUCCESS;
			} while(0);
			CDspService::instance()->sendSimpleResponseToDispClient( h, p, ret);
			return;
		}
	}


	// Package should be authorized!
	m_rwLock.lockForRead();
	bool cliExists = m_dispconns.contains(h);

	// Check authorization
	if ( ! cliExists ) {
		m_rwLock.unlock();

		// Send error
		CDspService::instance()->sendSimpleResponseToDispClient( h, p, PRL_ERR_USER_OPERATION_NOT_AUTHORISED );
		return;
	}

	// #455781 under read access (QReadLocker) we should call only const methods
	// to prevent app crash after simultaneously call QT_CONT::detach_helper().
	SmartPtr<CDspDispConnection> pDispConnection = m_dispconns.value(h);

	m_rwLock.unlock();

#ifdef _CT_
	if (CDspService::instance()->getVzHelper()->handleToDispatcherPackage(pDispConnection, p))
		return;
#endif

	switch (p->header.type)
	{
		case DispToDispLogoffCmd:
		{
			ProcessDispConnectionLogoff(pDispConnection, p);
		}
		break;

		case VmMigrateCheckPreconditionsCmd:
		{
			CDspService::instance()->getVmMigrateHelper().checkPreconditions(pDispConnection, p);
		}
		break;

		case VmBackupCreateCmd:
		case VmBackupCreateLocalCmd:
		case VmBackupRestoreCmd:
		case VmBackupGetTreeCmd:
		case VmBackupRemoveCmd:
			CDspService::instance()->sendSimpleResponseToDispClient( h, p, PRL_ERR_UNIMPLEMENTED);
			break;

		default:
		{
			if (IS_FILE_COPY_PACKAGE(p->header.type) ||
				IS_ABACKUP_PROXY_PACKAGE(p->header.type) ||
				is_migrate_package(p->header.type) ||
				(p->header.type == CtMigrateCmd))
			{
				/* retransmit package to task */
				pDispConnection->handlePackage(p);
				break;
			}
			CDspService::instance()->sendSimpleResponseToDispClient( h, p, PRL_ERR_UNRECOGNIZED_REQUEST);
			break;
		}
	}
}

SmartPtr<CDspDispConnection> CDspDispConnectionsManager::AuthorizeDispatcherConnection(
	const IOSender::Handle& h,
	const SmartPtr<IOPackage>& p
)
{
	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(p);
	if ( ! pCmd->IsValid() )
	{
		CDspService::instance()->sendSimpleResponseToDispClient( h, p, PRL_ERR_FAILURE );
		WRITE_TRACE(DBG_FATAL, "Wrong authorization package was received: [%s]",\
			p->buffers[0].getImpl());
		return SmartPtr<CDspDispConnection>(NULL);
	}
	CDispToDispAuthorizeCommand *pAuthorizeCommand =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispAuthorizeCommand>(pCmd);
	SmartPtr<CDspClient> pUserSession;
	if (pAuthorizeCommand->NeedAuthBySessionUuid())
	{
		 pUserSession =
			CDspService::instance()->getClientManager().getUserSession(pAuthorizeCommand->GetUserSessionUuid());
		if (! pUserSession.isValid())
			WRITE_TRACE(DBG_FATAL, "Dispatcher session wasn't authorized with '%s' session UUID",\
				qPrintable(pAuthorizeCommand->GetUserSessionUuid()));

	}else {
		//if session uuid is not defined, athorize via login & password (without session uuid)
		pUserSession = CDspService::instance()->getUserHelper().processDispacherLogin( h, p );

		if (!pUserSession.isValid())
			WRITE_TRACE(DBG_FATAL, "Dispatcher session wasn't authorized for user '%s'",\
				qPrintable(pAuthorizeCommand->GetUserName()));
	}

	if (! pUserSession.isValid() )
	{
		CDspService::instance()->sendSimpleResponseToDispClient( h, p, PRL_ERR_DISP2DISP_WRONG_USER_SESSION_UUID );
		return SmartPtr<CDspDispConnection>(NULL);
	}


	if (pAuthorizeCommand->NeedAuthBySessionUuid())
		WRITE_TRACE(DBG_FATAL, "Dispatcher session was successfully authorized with '%s' session UUID",\
			qPrintable(pAuthorizeCommand->GetUserSessionUuid()));
	else
		WRITE_TRACE(DBG_FATAL, "Dispatcher session was successfully authorized for '%s' user",\
			qPrintable(pAuthorizeCommand->GetUserName()));

	return SmartPtr<CDspDispConnection>(new CDspDispConnection(h, pUserSession));
}

void CDspDispConnectionsManager::ProcessDispConnectionLogoff(
	const SmartPtr<CDspDispConnection> &pDispConnection,
	const SmartPtr<IOPackage>& p
)
{
	IOSendJob::Handle hJob = pDispConnection->sendSimpleResponse( p, PRL_ERR_SUCCESS );
	CDspService::instance()->getIOServer().waitForSend(hJob);
	DeleteDispConnection(pDispConnection->GetConnectionHandle());
}

void CDspDispConnectionsManager::handleDetachClient (
    const IOSender::Handle& h,
	const IOCommunication::DetachedClient& detachedClient )
{
	// Lock
	QReadLocker locker( &m_rwLock );

	if ( ! m_dispconns.contains(h) ) {
		WRITE_TRACE(DBG_FATAL, "Client '%s' is not authed! Close connection!", qPrintable(h));
		CDspService::instance()->getIOServer().disconnectClient( h );
		return;
	}
	// #455781 under read access (QReadLocker) we should call only const methods
	// to prevent app crash after simultaneously call QT_CONT::detach_helper().
	SmartPtr<CDspDispConnection> pDispConnection = m_dispconns.value(h);
	locker.unlock();

	SmartPtr<CDspVm> pVm =
		CDspVm::GetVmInstanceByDispConnectionHandle(pDispConnection->GetConnectionHandle());

	if (!pVm.getImpl())
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't to map dispatcher connection object '%s' with VM process maintainer",\
			qPrintable(h));
		CDspService::instance()->getIOServer().disconnectClient( h );
		return;
	}

	IOSender::Handle vmHandle = pVm->getVmConnectionHandle();

	// Send detached client
	IOSendJob::Handle job =
		CDspService::instance()->getIOServer().sendDetachedClient( vmHandle,
														   detachedClient );
	CDspService::instance()->getIOServer().waitForSend( job );
	IOSendJob::Result res =
		CDspService::instance()->getIOServer().getSendResult( job );
	if ( res != IOSendJob::Success ) {
		WRITE_TRACE(DBG_FATAL, "Detach client send to Vm failed!");
		pDispConnection->sendSimpleResponse(
			pVm->getMigrateVmRequestPackage(),
			PRL_ERR_VM_MIGRATE_COULDNT_DETACH_TARGET_CONNECTION
		);
	}
}

/*****************************************************************************/
