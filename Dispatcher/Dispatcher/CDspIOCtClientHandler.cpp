///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspIOCtClientHandler
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

#include "CDspIOCtClientHandler.h"
#include "CDspService.h"
#include "CDspVm.h"
#include "CDspVmManager.h"
#include "CDspClientManager.h"

#include "Libraries/Std/PrlAssert.h"

#include <prlsdk/PrlIOStructs.h>

REGISTER_HANDLER( IOService::IOSender::IOCtClient,
				  "IOCtClientHandler",
				  CDspIOCtClientHandler );

/*****************************************************************************/
void CDspCtResponseHandler::process(const SmartPtr<IOPackage>& p)
{
	switch(p->header.type)
	{
		case PET_IO_STDIN_PORTION:
		case PET_IO_STDIN_WAS_CLOSED:
			emit onStdinPackageReceived(p);
			break;

		case PET_IO_CLIENT_PROCESSED_ALL_DESCS_DATA:
			emit onFinPackageReceived();
			break;

		default:
			LOG_MESSAGE(DBG_WARNING, "Unknown package type %d.", p->header.type);
	}
}

/*****************************************************************************/

CDspIOCtClientHandler::CDspIOCtClientHandler ( IOSender::Type type,
										   const char* name ) :
	CDspHandler( type, name )
{
}

CDspIOCtClientHandler::~CDspIOCtClientHandler ()
{
}

void CDspIOCtClientHandler::init ()
{
}

void CDspIOCtClientHandler::handleClientConnected ( const IOSender::Handle& h )
{
	WRITE_TRACE( DBG_WARNING, "CDspIOCtClientHandler::handleClientConnected %s",
			QSTR2UTF8(h));
	QMutexLocker locker( &m_mutex );
	m_ioClients[h] = ClientInfo();
}

void CDspIOCtClientHandler::handleClientDisconnected ( const IOSender::Handle& h )
{
	WRITE_TRACE( DBG_WARNING, "CDspIOCtClientHandler::handleClientDisconnected %s",
			QSTR2UTF8(h));
	QMutexLocker locker( &m_mutex );
	m_ioClients.remove( h );
}

void CDspIOCtClientHandler::handleToDispatcherPackage (
		const IOSender::Handle& h,
		const SmartPtr<IOPackage>& p )
{

	if ( p->header.type == PET_IO_CLI_AUTHENTICATE_EXEC_SESSION ) {
		const PRL_IO_AUTH_EXEC_REQUEST *authRequest =
			reinterpret_cast<const PRL_IO_AUTH_EXEC_REQUEST*>( p->buffers[0].getImpl() );

		if ( p->data[0].bufferSize < sizeof(*authRequest) ) {
			WRITE_TRACE(DBG_FATAL, "Wrong auth session package!");
			CDspService::instance()->getIOServer().disconnectClient( h );
			return;
		}

		Uuid sessionId = Uuid::toUuid( authRequest->sessionUuid );
		if ( sessionId.isNull() ) {
			WRITE_TRACE(DBG_FATAL, "Wrong auth session package!");
			CDspService::instance()->getIOServer().disconnectClient( h );
			return;
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
		m_ioClients[h] = cliInfo;

		locker.unlock();

		// Send package
		IOSendJob::Handle job =
			CDspService::instance()->getIOServer().sendPackage( h, pkg );
		IOSendJob::Result res =
			CDspService::instance()->getIOServer().waitForSend( job );

		// Close connection if fail
		if ( ! auth || res != IOSendJob::Success ) {
			WRITE_TRACE(DBG_FATAL, "Error: %s", (! auth ? "authentication failed!" :
						"send of authentication pkg failed!"));
			CDspService::instance()->getIOServer().disconnectClient( h );
		}

		return;
	} else if (p->header.type == PET_IO_STDIN_PORTION ||
			p->header.type == PET_IO_STDIN_WAS_CLOSED ||
			p->header.type == PET_IO_CLIENT_PROCESSED_ALL_DESCS_DATA)
	{
		if (!getIOUserSession(h)) {
			WRITE_TRACE(DBG_FATAL, "Client '%s' is not authed! Close connection!",
					QSTR2UTF8(h));
			CDspService::instance()->getIOServer().disconnectClient( h );
			return;
		}

		SmartPtr<CDspCtResponseHandler> hResponse = getResponseHandler(h,
				Uuid::toString(p->header.parentUuid));
		if (!hResponse) {
			WRITE_TRACE(DBG_FATAL, "Client %s guest session %s is not created pkt.type=%d",
					QSTR2UTF8(h),
					QSTR2UTF8((Uuid::toString(p->header.parentUuid))),
					p->header.type);
			CDspService::instance()->getIOServer().disconnectClient( h );
			return;
		}
		hResponse->process(p);
	}
}

void CDspIOCtClientHandler::handleFromDispatcherPackage (
	const SmartPtr<CDspHandler>&,
	const IOSender::Handle&,
	const SmartPtr<IOPackage>& )
{
	// Never should be called for this handler
	PRL_ASSERT(0);
}

void CDspIOCtClientHandler::handleFromDispatcherPackage (
	const SmartPtr<CDspHandler>&,
	const IOSender::Handle&,
	const IOSender::Handle&,
	const SmartPtr<IOPackage>& )
{
	// Never should be called for this handler
	PRL_ASSERT(0);
}

void CDspIOCtClientHandler::handleClientStateChanged ( const IOSender::Handle&,
													 IOSender::State )
{
}

SmartPtr<CDspCtResponseHandler> CDspIOCtClientHandler::registerResponseHandler(
		const IOSender::Handle& h,
		const QString &sGuestSessionUuid)
{
	QMutexLocker locker( &m_mutex );

	if (!m_ioClients.contains(h))
		return SmartPtr<CDspCtResponseHandler>();

	struct ClientInfo &ci = m_ioClients[h];

	if (ci.sessions.contains(sGuestSessionUuid)) {
		WRITE_TRACE(DBG_FATAL, "Guest session %s handler already registersd",
				QSTR2UTF8(sGuestSessionUuid));
		return SmartPtr<CDspCtResponseHandler>();
	}

	WRITE_TRACE(DBG_WARNING, "Register client %s guest session %s response handler",
			QSTR2UTF8(h),
			QSTR2UTF8(sGuestSessionUuid));
	ci.sessions[sGuestSessionUuid] = SmartPtr<CDspCtResponseHandler> (new CDspCtResponseHandler());

	return ci.sessions[sGuestSessionUuid];
}

void CDspIOCtClientHandler::unregisterResponseHandler(const IOSender::Handle &h,
		const QString &sGuestSessionUuid)
{
	QMutexLocker locker( &m_mutex );
	if (m_ioClients.contains(h))
		m_ioClients[h].sessions.remove(sGuestSessionUuid);
}

SmartPtr<CDspClient> CDspIOCtClientHandler::getIOUserSession (const IOSender::Handle& h)
{
	QMutexLocker locker( &m_mutex );

	if (m_ioClients.contains(h))
		return SmartPtr<CDspClient>( new CDspClient(h));

	return SmartPtr<CDspClient>();
}

SmartPtr<CDspCtResponseHandler> CDspIOCtClientHandler::getResponseHandler(
		const IOSender::Handle &h,
		const QString &sGuestSessionUuid)
{
	QMutexLocker locker( &m_mutex );

	if (!m_ioClients.contains(h) || !m_ioClients[h].sessions.contains(sGuestSessionUuid))
		return SmartPtr<CDspCtResponseHandler>();

	return m_ioClients[h].sessions[sGuestSessionUuid];
}

/*****************************************************************************/
