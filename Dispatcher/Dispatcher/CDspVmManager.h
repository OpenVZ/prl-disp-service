///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspVmManager.h
///
/// Vm manager and handler, which is responsible for all Vms and
/// packages for these vms or packages from these vms.
///
/// @author romanp, sandro
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

#ifndef CDSPVMMANAGER_H
#define CDSPVMMANAGER_H

#include "CDspHandlerRegistrator.h"
#include "CDspVm.h"
#include <prlsdk/PrlEnums.h>
#include "CDspAccessManager.h"


class CDspVmManager : public CDspHandler
{
public:
	CDspVmManager ( IOSender::Type, const char* );
	virtual ~CDspVmManager ();


	/**
	 * Do initialization after service starting.
	 */
	virtual void init ();

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
								 const SmartPtr<IOPackage>& pPackage );

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

	virtual void handleWsResponse( SmartPtr<CDspVm> pVm,
		const SmartPtr<IOPackage>& p,
		bool & bNeedToRoute);

	/**
	* Returns all running vms.
	*/
	QList< SmartPtr<CDspVm> > getAllRunningVms() const;

	/**
	 * Returns Vms by client.
	 */
	QList< SmartPtr<CDspVm> > getVmsByClient ( const SmartPtr<CDspClient>& ) const;

	/** Returns sign whether some running VMs present */
	bool hasAnyRunningVms() const;

	/**
	 * Sending to the active VMs processes command on change log level
	 * @param user session initiated call
	 * @param sign whether verbose log level enabled
	 */
	void changeLogLevelForActiveVMs( SmartPtr<CDspClient> pUser, bool bVerboseLogEnabled ) const;

	/**
	 * Synchronizes VMs uptime into their configurations files
	 */
	void syncVMsUptime();

	/**
	 * get CDspVmSuspendHelper
	 */
	CDspLockedPointer<CDspVmSuspendHelper> getSuspendHelper();

private:
	/**
	 * Handle command, extension for handleFromDispatcherPackage
	 * @param pUser
	 * @param clientHandler transport client handler
	 * @param pPackage package from handler to send
	 */
	void handleFromDispatcherPackageInternal ( const SmartPtr<CDspClient> pUser,
											   const SmartPtr<IOPackage> &pPackage);

	QString getVmIdByHandle(const IOSender::Handle& h) const;

	/** Check non-interactive sessions */
	bool haveNonInteractiveSessionsWithRequestToVm(	const SmartPtr<CDspVm>& pVm,
													const QList< SmartPtr<CDspClient> >& lstSessions) const;

	/** Check interactive sessions */
	bool haveInteractiveSessions( const QList< SmartPtr<CDspClient> >& lstSessions) const;

	/**
	 * Checks whether use defualt answers mechanism activated for specified VM
	 * @param pointer to the VM process maintainer oblect
	 * @return sign whether use default answers mechanism activated
	 */
	bool defaultAnswersMechActivated(const SmartPtr<CDspVm>& pVm) const;

	/**
	 * Change host time according to guest's request. Two-way timesync feature.
	 * @param time in seconds
	 * @param time zone index in compatibility table
	 */
	void changeHostTimeSettings( int seconds, int tzIndex );

	/**
	 * Change current host timezone to the one with spectified index
	 * @param timezone index in compatibility table
	 */
	void changeTimezone( int tzIndex );

	/**
	 * Check ability to use fast reboot
	 */
	bool checkFastReboot(void) const;
private:

	/** VMs clients hash access synchronization object */
	mutable QReadWriteLock m_rwLock;
	/** VMs connections hash */
	QHash< IOSender::Handle, SmartPtr<CDspVm> > m_vms;

	QMutex	m_suspendHelperMutex;
	CDspVmSuspendHelper m_suspendHelper;
};

#endif //CDSPVMMANAGER_H
