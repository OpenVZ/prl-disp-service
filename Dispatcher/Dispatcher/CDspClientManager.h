///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspClientManager
///
/// Client manager and handler, which is responsible for all clients and
/// packages for these clients or packages from these clients.
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

#ifndef CDSPCLIENTMANAGER_H
#define CDSPCLIENTMANAGER_H

#include <QHash>
#include <QReadWriteLock>

#include "CDspHandlerRegistrator.h"
#include "CDspClient.h"
#include "CDspUserHelper.h"
#include "DspLogicGuard.h"
#include "CDspVmDirManager.h"
#include "CDspAccessManager.h"

#include "Libraries/PerfCount/PerfLib/PerfCounter.h"

class CDspClientManager : public CDspHandler
{
public:
	CDspClientManager ( IOSender::Type, const char* );
	virtual ~CDspClientManager ();

	/**
	 * Do initialization after service starting.
	 */
	virtual void init ();

	/**
	 * Handle new client connection.
	 * @param h client handle
	 */
	virtual void handleClientConnected ( const IOSender::Handle& h );

	/**
	 * Handle client disconnection.
	 * @param h client handle
	 */
	virtual void handleClientDisconnected ( const IOSender::Handle& h );

	/**
	 * Handle package from the remote client.
	 * @param h client handle
	 * @param p package from client
	 */
	virtual void handleToDispatcherPackage ( const IOSender::Handle&,
											 const SmartPtr<IOPackage>& p );

	/**
	 * Handle package from other handler which should be sent by this handler.
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
	 * Handle client state change.
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


	/**
	 * Returns users sessions hash snapshot for external purposes
	 */
	QHash< IOSender::Handle, SmartPtr<CDspClient> >
		getSessionsListSnapshot( const QString& vmDirUuid = "",
								 CDspClient::CLIENT_TYPE nClientType = CDspClient::CT_ALL_CLIENTS ) const;

	/**
	* Returns users sessions hash snapshot for users have READ right to VM
	*/
	QHash< IOSender::Handle, SmartPtr<CDspClient> >
		getSessionListByVm( const CVmIdent& vmIdent,
			bool bSearchAsSharedVm = false, // to search the same vm which registered in another vm directory
			int accessRights = CDspAccessManager::VmAccessRights::arCanRead) const;

	QHash< IOSender::Handle, SmartPtr<CDspClient> >
		getSessionListByVm( const QString& vmDirUuid, const QString& vmUuid,
			int accessRights = CDspAccessManager::VmAccessRights::arCanRead) const;

	/**
	 * Returns pointer to client session object by specified connection handle
	 * @param connection handle
	 * @return pointer to client session object
	 */
	SmartPtr<CDspClient> getUserSession ( const IOSender::Handle& h ) const;

	/**
	* @param connection handle
	*/
	void deleteUserSession ( const IOSender::Handle& h );

	/**
	* Sends package to list of users sessions
	* @return list of send jobs.
	**/
	QList< IOSendJob::Handle > sendPackageToClientList( const SmartPtr<IOPackage>& p,
		QList< SmartPtr<CDspClient> >&  lst);

	/**
	* Sends package to current users sessions which client have permissions to this vm
	*
	* @return list of send jobs.
	*/
	QList< IOSendJob::Handle > sendPackageToVmClients(
		const SmartPtr<IOPackage>& p, const QString& vmDirUuid, const QString& vmUuid );
	QList< IOSendJob::Handle > sendPackageToVmClients(
		const SmartPtr<IOPackage>& p, const CVmIdent& vmIdent )
	{
		return sendPackageToVmClients( p, vmIdent.second, vmIdent.first );
	}


	/**
	* Sends package to all current users sessions
	* @return list of send jobs.
	*/
	QList< IOSendJob::Handle > sendPackageToAllClients( const SmartPtr<IOPackage>& p );

	/**
	* Send to the client VM question
	* @param pQuestionPacket Packet with question
	* @param pClient Client
	* @return boolean Question packet was sent
	*/
	void sendQuestionToClient(SmartPtr<CDspClient> pClient);

	/**
	 * Check if client is preauthorized. That means, the client is successfully
	 * connected (and established trusted channel in case of secure login), but
	 * not provided credentials for authorization. Only host users list request
	 * is allowed in this state. Client can send DspCmdLogin command to complete
	 * the authorization and log in.
	 */
	bool isPreAuthorized(const IOSender::Handle& h);
private:

	QHash< IOSender::Handle, SmartPtr<CDspClient> > m_clients;
	mutable QReadWriteLock m_rwLock;
	DspLogicGuard m_logicGuard;

	bool CaptureLogonClient(const IOSender::Handle& h);
	void ReleaseLogonClient(const IOSender::Handle& h);

	typedef QSet< IOSender::Handle >	handle_set;
	handle_set	m_setLogonClients;
	handle_set  m_preAuthorizedSessions;

    counter_t * dsp_commands ;
    counter_t * dsp_error_commands ;
};

#endif //CDSPCLIENTMANAGER_H
