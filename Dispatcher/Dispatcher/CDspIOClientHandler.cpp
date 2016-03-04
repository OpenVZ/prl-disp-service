///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspIOClientHandler
///
/// IO handler, which is responsible for all IO packages
///
/// @author romanp
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

#include "CDspIOClientHandler.h"
#include "CDspService.h"
#include "CDspVm.h"
#include "CDspVmManager.h"
#include "CDspClientManager.h"

#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/PrlCommonUtilsBase/PrlStringifyConsts.h>

#include <prlsdk/PrlIOStructs.h>

REGISTER_HANDLER( IOService::IOSender::IOClient,
				  "IOClientHandler",
				  CDspIOClientHandler );

/*****************************************************************************/

CDspIOClientHandler::CDspIOClientHandler ( IOSender::Type type,
										   const char* name ) :
	CDspHandler( type, name )
{
}

CDspIOClientHandler::~CDspIOClientHandler ()
{
}

void CDspIOClientHandler::init ()
{
}

void CDspIOClientHandler::handleClientConnected ( const IOSender::Handle& h )
{
	WRITE_TRACE( DBG_WARNING, "%s", __FUNCTION__ );
	QMutexLocker locker( &m_mutex );
	m_ioClients[h] = ClientInfo();
}

void CDspIOClientHandler::handleClientDisconnected ( const IOSender::Handle& h )
{
	WRITE_TRACE( DBG_WARNING, "%s", __FUNCTION__ );
	QMutexLocker locker( &m_mutex );
	m_ioClients.remove( h );
}

void CDspIOClientHandler::handleToDispatcherPackage (
    const IOSender::Handle& h,
	const SmartPtr<IOPackage>& p )
{
    switch ( p->header.type ) {

    case ( PET_IO_CLI_AUTHENTICATE_SESSION ) : {
		const PRL_IO_AUTH_REQUEST* authRequest =
			reinterpret_cast<const PRL_IO_AUTH_REQUEST*>( p->buffers[0].getImpl() );

		if ( p->data[0].bufferSize < sizeof(*authRequest) ) {
			WRITE_TRACE(DBG_FATAL, "Wrong auth session package!");
			CDspService::instance()->getIOServer().disconnectClient( h );
			break;
		}

		Uuid sessionId = Uuid::toUuid( authRequest->sessionUuid );
		if ( sessionId.isNull() ) {
			WRITE_TRACE(DBG_FATAL, "Wrong auth session package!");
			CDspService::instance()->getIOServer().disconnectClient( h );
			break;
		}

		// Try to find client by auth session
		SmartPtr<CDspClient> client = CDspService::instance()->
			getClientManager().getUserSession( sessionId.toString() );

		bool auth = client.isValid();

		PRL_IO_AUTH_RESPONSE authResp = { auth };
		SmartPtr<IOPackage> pkg =
			IOPackage::createInstance( PET_IO_AUTH_RESPONSE,
									   IOPackage::RawEncoding,
									   &authResp, sizeof(authResp) );

		// Save result
		QMutexLocker locker( &m_mutex );

		ClientInfo cliInfo;
		cliInfo.sessionId = sessionId.toString();
		m_ioClients[h] = cliInfo;

		locker.unlock();

		// Send package
		IOSendJob::Handle job =
			CDspService::instance()->getIOServer().sendPackage( h, pkg );
		IOSendJob::Result res =
			CDspService::instance()->getIOServer().waitForSend( job );

		// Close connection if fail
		if ( ! auth || res != IOSendJob::Success ) {
			WRITE_TRACE(DBG_FATAL,
						"Error: %s", (! auth ? "authentication failed!" :
									  "send of authentication pkg failed!"));
			CDspService::instance()->getIOServer().disconnectClient( h );
		}

		break;
    }
    default:
        LOG_MESSAGE(DBG_WARNING, "Unknown package type %d.", p->header.type);
        break;
    }
}

void CDspIOClientHandler::handleFromDispatcherPackage (
	const SmartPtr<CDspHandler>&,
	const IOSender::Handle&,
	const SmartPtr<IOPackage>& )
{
	// Never should be called for this handler
	PRL_ASSERT(0);
}

void CDspIOClientHandler::handleFromDispatcherPackage (
	const SmartPtr<CDspHandler>&,
	const IOSender::Handle&,
	const IOSender::Handle&,
	const SmartPtr<IOPackage>& )
{
	// Never should be called for this handler
	PRL_ASSERT(0);
}

void CDspIOClientHandler::handleClientStateChanged ( const IOSender::Handle&,
													 IOSender::State )
{
}

void CDspIOClientHandler::handleDetachClient (
    const IOSender::Handle& h,
	const IOCommunication::DetachedClient& detachedClient )
{
	//
	// BEWARE! You can't send any data to already detached client!
	//

	// Lock
	QMutexLocker locker( &m_mutex );

	if ( ! m_ioClients.contains(h) || m_ioClients[h].sessionId.isEmpty() ) {
		WRITE_TRACE(DBG_FATAL,
					"Client '%s' is not authed! Close connection!",
					QSTR2UTF8(h) );
		CDspService::instance()->getIOServer().disconnectClient( h );
		return;
	}
	else if ( m_ioClients[h].vmHandle.isEmpty() ) {
		WRITE_TRACE(DBG_FATAL,
					"Client '%s' is not attached to Vm! Close connection!",
					QSTR2UTF8(h) );
		CDspService::instance()->getIOServer().disconnectClient( h );
		return;
	}

	// copy to use without lock
	ClientInfo cliInfo = m_ioClients[h];

	SmartPtr<CDspVm> pVm =
		CDspVm::GetVmInstanceByUuid( m_ioClients[h].vmIdent );

	if( !pVm ){
		WRITE_TRACE(DBG_FATAL,
			"Client '%s' is not attached to Vm! Vm lost! Close connection!",
			QSTR2UTF8(h) );
		CDspService::instance()->getIOServer().disconnectClient( h );
		return;
	}

	// Unlock
	locker.unlock();

	// Send detached client
	IOSendJob::Handle job =
		CDspService::instance()->getIOServer().sendDetachedClient( cliInfo.vmHandle,
														   detachedClient );
	CDspService::instance()->getIOServer().waitForSend( job );
	IOSendJob::Result res =
		CDspService::instance()->getIOServer().getSendResult( job );
	if ( res != IOSendJob::Success ) {
		// We can't send attach response to already detached client,
		// so just log an error. Client will determine this error
		// by closed connection.
		WRITE_TRACE(DBG_FATAL,
					"Error: send of detached client state to Vm failed!");
	}

	SmartPtr<CDspClient>
		pClient = CDspService::instance()->getClientManager().getUserSession( cliInfo.sessionId );
	if( !pClient )
	{
		WRITE_TRACE(DBG_FATAL
			, "Can't add IOClient %s to statistic because session %s was disconnected!"
			, qPrintable(h), qPrintable(cliInfo.sessionId) );
	}
}

void CDspIOClientHandler::sendVmAttachResponse ( const IOSender::Handle& h,
												 bool vmAttachRes,
												 bool encodingRes )
{
    PRL_IO_ATTACH_RESPONSE resp = { !!vmAttachRes, !!encodingRes };

	// Create package
	SmartPtr<IOPackage> pkg = IOPackage::createInstance(
										 PET_IO_CLI_ATTACH_TO_VM,
										 IOPackage::RawEncoding,
										 &resp, sizeof(resp) );

	// Send
	IOSendJob::Handle job =
		CDspService::instance()->getIOServer().sendPackage( h, pkg );
	CDspService::instance()->getIOServer().waitForSend( job );
}

/*****************************************************************************/
