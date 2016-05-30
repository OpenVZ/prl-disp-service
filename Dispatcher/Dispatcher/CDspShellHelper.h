////////////////////////////////////////////////////////////////////////////////
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
/// @file
///	CDspShellHelper.h
///
/// @brief
///	Definition of the class CDspShellHelper
///
/// @brief
///	This class implements various environment-related logic
///
/// @author sergeyt
///	SergeyM
///
/// @date
///	2006-04-04
///
/// @history
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __CDspShellHelper_H_
#define __CDspShellHelper_H_

#include <prlcommon/Interfaces/ParallelsDomModel.h>
#include <prlcommon/Interfaces/ParallelsNamespace.h>
#include <prlcommon/IOService/IOCommunication/IOProtocol.h>
#include "CDspClient.h"
#include "CDspTaskHelper.h"
#include <prlcommon/PrlCommonUtilsBase/StringUtils.h>


/**
 * @brief This class implements various environment-related logic
 * @author SergeyM
 */
class CDspShellHelper : public QObject
{
public:

	// constructor
	CDspShellHelper();

	// destructor
	~CDspShellHelper();

	// cancel command
	void cancelOperation ( SmartPtr<CDspClient>&,
		const SmartPtr<IOPackage>& );

	// Sends host hardware info
	void sendHostHardwareInfo ( const SmartPtr<CDspClient>&,
								const SmartPtr<IOPackage>&);

	// Sends directory entries
	void sendDirectoryEntries ( SmartPtr<CDspClient>&,
								const SmartPtr<IOPackage>& );

	// Sends disk list
	void sendDiskEntries ( SmartPtr<CDspClient>&,
						   const SmartPtr<IOPackage>& );

	// Creates directory
	void createDirectoryEntry ( SmartPtr<CDspClient>&,
								const SmartPtr<IOPackage>& );

	// Renames file or directory
	void renameFsEntry  ( SmartPtr<CDspClient>&,
						  const SmartPtr<IOPackage>& );

	// Removes file or directory
	void removeFsEntry ( SmartPtr<CDspClient>&,
						 const SmartPtr<IOPackage>& );

	// Check if an user can create a file
	void canCreateFile ( SmartPtr<CDspClient>&,
						 const SmartPtr<IOPackage>& );

	// Generates entry name for specified directory
	void generateFsEntryName  ( SmartPtr<CDspClient>&,
						  const SmartPtr<IOPackage>& );

	// Sends host common info
	void sendHostCommonInfo ( SmartPtr<CDspClient>&,
							  const SmartPtr<IOPackage>& );

	// Sends host statistics
	void sendHostStatistics ( SmartPtr<CDspClient>&,
							  const SmartPtr<IOPackage>& );

	// Sends guest statistics
	void sendGuestStatistics ( SmartPtr<CDspClient>&,
							   const SmartPtr<IOPackage>& );

	/** Begins edit transaction of common server preferences */
	void hostCommonInfoBeginEdit ( SmartPtr<CDspClient>&,
								   const SmartPtr<IOPackage>& );


	/** Commits edit transaction of common server preferences */
	void hostCommonInfoCommit ( SmartPtr<CDspClient>&,
								const SmartPtr<IOPackage>& );

	/** Manages Parallels Network Service */
	void managePrlNetService ( SmartPtr<CDspClient>&,
							   const SmartPtr<IOPackage>& );

	/** Sends Parallels Network Service tatus */
	static void sendNetServiceStatus ( SmartPtr<CDspClient>&,
						   const SmartPtr<IOPackage>& );

	/** Attach session to executed task in closed session [ task #6009 ]**/
	void attachToLostTask( SmartPtr<CDspClient>& pUser,
		const SmartPtr<IOPackage>& p );

	/** Sends list information of all virtual networks */
	void sendVirtualNetworkList(SmartPtr<CDspClient>&,
								const SmartPtr<IOPackage>&);

	void sendNetworkClassesConfig(SmartPtr<CDspClient>& pUser, const SmartPtr<IOPackage>& p);

	void sendNetworkShapingConfig(SmartPtr<CDspClient>& pUser, const SmartPtr<IOPackage>& p);

	/** Sends list information of all private networks */
	void sendIPPrivateNetworksList(SmartPtr<CDspClient>& pUser, const SmartPtr<IOPackage>& p);

	/** Configure generic PCI devices */
	void configureGenericPci(SmartPtr<CDspClient>&,
							 const SmartPtr<IOPackage>&);

	/** beforeHostSuspend operation for all guests with generic PCI devices */
	void beforeHostSuspend(SmartPtr<CDspClient>&,
						  const SmartPtr<IOPackage>&);

	/** updates usb associations list from gui */
	void updateUsbAssociationsList(SmartPtr<CDspClient>&,
		const SmartPtr<IOPackage>&);

	/** afterHostResume operation for all guests with generic PCI devices */
	PRL_RESULT afterHostResume();
	void afterHostResume(SmartPtr<CDspClient>&,
		const SmartPtr<IOPackage>&);

	void changeServerInternalValue(SmartPtr<CDspClient>&, const SmartPtr<IOPackage>&);
	void changeVmInternalValue(SmartPtr<CDspClient>&, const SmartPtr<IOPackage>&);

	/* get available disk space for path */
	void sendDiskFreeSpace(SmartPtr<CDspClient>& pUser, const SmartPtr<IOPackage>& p);

	/* is sHost a local address */
	bool isLocalAddress(const QString &sHost);

	static bool isHeadlessModeEnabled();
	static PRL_RESULT enableHeadlessMode( bool bEnable );

	void sendCPUPoolsList(SmartPtr<CDspClient>&, const SmartPtr<IOPackage>&);
	void joinCPUPool(SmartPtr<CDspClient>& pUser, const SmartPtr<IOPackage>& p);
	void leaveCPUPool(SmartPtr<CDspClient>& pUser, const SmartPtr<IOPackage>& p);
	void moveToCPUPool(SmartPtr<CDspClient>& pUser, const SmartPtr<IOPackage>& p);
	void recalculateCPUPool(SmartPtr<CDspClient>& pUser, const SmartPtr<IOPackage>& p);

	void sendLicenseInfo(SmartPtr<CDspClient>& pUser, const SmartPtr<IOPackage>& p);
	void updateLicense(SmartPtr<CDspClient>& pUser, const SmartPtr<IOPackage>& p);

private:
	PRL_RESULT refreshVmsCPUFeatures(SmartPtr<CDspClient>& user_) const;
	PRL_RESULT checkAccessForHostCommonInfoEdit(SmartPtr<CDspClient>& pUser,
		const SmartPtr<IOPackage>& p);

	void sendSuccessResponseWithStandardParam(SmartPtr<CDspClient>& pUser,
											  const SmartPtr<IOPackage>& p,
											  const QString & param);

	void fixPrlServicesInEtcHosts(const QStringList& keys);
}; // class CDspShellHelper


#endif // __CDspShellHelper_H_
