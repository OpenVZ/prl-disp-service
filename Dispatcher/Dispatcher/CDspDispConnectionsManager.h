///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspDispConnectionsManager.h
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

#ifndef CDspDispConnectionsManager_H
#define CDspDispConnectionsManager_H

#include "CDspHandlerRegistrator.h"
#include "CDspDispConnection.h"
#include "CDspBackupHelper.h"

class CDspService;

/**
 * Manager of connections from another dispatchers to this one
 */
class CDspDispConnectionsManager : public CDspHandler
{
public:
	/**
	 * Class constructor
	 * @param maintaining connections type
	 * @param name of handler that identifies it from others
	 */
	CDspDispConnectionsManager(CDspService& service_, const Backup::Task::Launcher& backup_);
	/**
	 * Class destructor
	 */
	virtual ~CDspDispConnectionsManager ();

	/**
	 * Hanle client disconnection.
	 * @param client handle
	 */
	virtual void handleClientDisconnected ( const IOSender::Handle& h );

	/**
	 * Hanle package from the remote client.
	 * @param client handle
	 * @param package from client
	 */
	virtual void handleToDispatcherPackage ( const IOSender::Handle&,
		 const SmartPtr<IOPackage>& p );

	/**
	 * * Handle detached dispatcher connection
	 *  * @param handle to dispatcher connection
	 * @param detached client info
	 */
	virtual void handleDetachClient(
		const IOSender::Handle& h,
		const IOCommunication::DetachedClient& detachedClient);

private:
	/**
	 * Authorizes dispatcher connection
	 * @param handle to dispatcher connection
	 * @param pointer to authorization package object
	 * @returns pointer to instantiated and authorized dispatcher connection object (null if authorization was failed)
	 */
	SmartPtr<CDspDispConnection> AuthorizeDispatcherConnection(
		const IOSender::Handle& h,
		const SmartPtr<IOPackage>& p
	);
	/**
	 * Processes dispatcher connection logoff
	 * @param pointer to dispatcher connection object
	 * @param pointer to logoff request package object
	 */
	void ProcessDispConnectionLogoff(
		const SmartPtr<CDspDispConnection> &pDispConnection,
		const SmartPtr<IOPackage>& p
	);
	/**
	 * Deletes dispatcher connection from registered connections hash
	 * @param handle to deleting dispatcher connection
	 */
	void DeleteDispConnection( const IOSender::Handle& h );

private:
	/** VMs clients hash access synchronization object */
	mutable QReadWriteLock m_rwLock;
	/** VMs connections hash */
	QHash< IOSender::Handle, SmartPtr<CDspDispConnection> > m_dispconns;

	CDspService* m_service;
	Backup::Task::Launcher m_backup;
};

#endif //CDspDispConnectionsManager_H
