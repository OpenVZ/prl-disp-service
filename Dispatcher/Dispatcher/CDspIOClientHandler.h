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

#ifndef CDSPIOCLIENTHANDLER_H
#define CDSPIOCLIENTHANDLER_H

#include "CDspHandlerRegistrator.h"
#include "CVmIdent.h"

class CDspIOClientHandler : public CDspHandler
{
public:
	/** Constructor */
	CDspIOClientHandler ( IOSender::Type, const char* );

	/** Destructor */
	virtual ~CDspIOClientHandler ();

	/**
	 * Do initialization after service starting.
	 */
	virtual void init ();

	/**
	 * Hanle new client connection.
	 * @param h client handle
	 */
	virtual void handleClientConnected ( const IOSender::Handle& h );

	/**
	 * Hanle client disconnection.
	 * @param h client handle
	 */
	virtual void handleClientDisconnected ( const IOSender::Handle& h );

	/**
	 * Hanle package from the remote client.
	 * @param h client handle
	 * @param p package from client
	 */
	virtual void handleToDispatcherPackage ( const IOSender::Handle&,
											 const SmartPtr<IOPackage>& p );


	/**
	 * Hanle package from other handler which should be sent by this handler.
	 * @param receiverHandler handler, which receives this package
	 * @param clientHandler transport client handler
	 * @param p package from handler to send
	 */
	virtual void handleFromDispatcherPackage (
								 const SmartPtr<CDspHandler>& receiverHandler,
								 const IOSender::Handle& clientHandler,
								 const SmartPtr<IOPackage>& p );

	/**
	 * Handle package from other handler which should be sent by this handler
	 * to direct receiver.
	 * @param receiverHandler handler, which receives this package
	 * @param clientHandler transport client handler
	 * @param directReceiverHandler transport client handler, to which
	 *                              this package should be directly send.
	 * @param p package from handler to send
	 */
	virtual void handleFromDispatcherPackage (
								 const SmartPtr<CDspHandler>& receiverHandler,
								 const IOSender::Handle& clientHandler,
								 const IOSender::Handle& directReceiverHandler,
								 const SmartPtr<IOPackage>& p );


	/**
	 * Hanle client state change.
	 * @param h client handle
	 * @param st new server state
	 */
	virtual void handleClientStateChanged ( const IOSender::Handle& h,
											IOSender::State st );

	/**
	 * Handle client detach.
	 * @param h client handle
	 * @param dc detached client state.
	 */
	virtual void handleDetachClient ( const IOSender::Handle& h,
									  const IOCommunication::DetachedClient& );

private:
	/** Sends vm attach response to IO client */
	void sendVmAttachResponse ( const IOSender::Handle& h,
								bool vmAttachRes, bool encodingRes );

private:
	struct ClientInfo
	{
		QString sessionId;
		IOSender::Handle vmHandle;
		CVmIdent vmIdent;
	};

	QMutex m_mutex;
	QHash< IOSender::Handle, ClientInfo > m_ioClients;

};

#endif //CDSPIOCLIENTHANDLER_H
