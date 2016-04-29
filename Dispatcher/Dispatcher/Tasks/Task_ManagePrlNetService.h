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
///	Task_ManagePrlNetService.h
///
/// @brief
///	Definition of the class CDspTaskHelper
///
/// @brief
///	This class implements long running tasks helper class
///
/// @author sergeyt
///	SergeyT@
///
////////////////////////////////////////////////////////////////////////////////

#ifndef __Task_ManagePrlNetService_H_
#define __Task_ManagePrlNetService_H_

#include <QMutex>
#include "CDspTaskHelper.h"

#include <prlxmlmodel/NetworkConfig/CParallelsNetworkConfig.h>
#include <prlxmlmodel/VmConfig/CVmGenericNetworkAdapter.h>
#include "Libraries/PrlNetworking/PrlNetLibrary.h"
#include <prlcommon/Interfaces/ParallelsNamespace.h>
#include "Dispatcher/Dispatcher/CDspVzHelper.h"

class CDispNetworkPreferences;
class CDispNetAdapter;

class Task_ManagePrlNetService : public  CDspTaskHelper
{
   Q_OBJECT
public:
	Task_ManagePrlNetService ( SmartPtr<CDspClient>&,
			   const SmartPtr<IOPackage>& );

	Task_ManagePrlNetService ( SmartPtr<CDspClient>&,
				const SmartPtr<IOPackage>&,
				PVE::IDispatcherCommands nCmd);

	PVE::IDispatcherCommands	getCmdNumber();

	/// convert CParallelsNetworkConfig to CDispNetworkPreferences
	static SmartPtr<CDispNetworkPreferences> convertNetworkConfig( SmartPtr<CParallelsNetworkConfig> pNetworkConfig );

#ifdef _LIN_
	/// set nessary settings on host for particular vm network adapter
	/// used in psbm only for now
	static void updateAdapter(const CVmGenericNetworkAdapter& pAdapter, bool bEnable);
#endif

	/// set nessary settings on host for vm networking
	static void updateVmNetworking(SmartPtr<CVmConfiguration> pVmConfig, bool bEnable);

	/// add IPs to from vm config to network.xml
	static void addVmIPAddress(SmartPtr<CVmConfiguration> pVmConfig);

	/// remove IPs from network xml
	static void removeVmIPAddress(SmartPtr<CVmConfiguration> pVmConfig);

	/// get duplicates IP Addresses with other VMs
	static QSet<QHostAddress> checkIPAddressDuplicates(SmartPtr<CVmConfiguration> pVmConfig,
                                        const QList< SmartPtr<CVmConfiguration> > &vmList);

	/// extract all IP addresses from VM config
	static void extractIPAddressesFromVMConfiguration(SmartPtr<CVmConfiguration> config, QSet<QHostAddress>& IPs);

	/// extract all IP addresses from VM config for interface with specified type
	static QSet<QString> extractIPAddressesFromVMConfiguration( SmartPtr<CVmConfiguration> config,
										PRL_NET_VIRTUAL_NETWORK_TYPE type );

	/// extract all host only IP addresses from VM config
	static QSet<QString> extractHostOnlyIPAddressesFromVMConfiguration( SmartPtr<CVmConfiguration> config );

	/// extract all IP addresses from routed network adapters from VM config
	static QSet<QString> extractRoutedIPAddressesFromVMConfiguration(SmartPtr<CVmConfiguration> config);

	/// get mac addresses of physical/virtual host adapters
	static bool getHostMacAddresses(QSet< QString >& hostMacs);

protected:
	virtual PRL_RESULT run_body();

	/// restore default network settings
	PRL_RESULT cmdNetPrlNetworkServiceRestoreDefaults();
	/// add new parallels network adapter
	PRL_RESULT cmdAddNetAdapter();
	/// delete parallels network adapter
	PRL_RESULT cmdDeleteNetAdapter();
	/// update configuration of the parallels network adapter
	PRL_RESULT cmdUpdateNetAdapter();

	/// add a new virtual network
	PRL_RESULT cmdAddVirtualNetwork();
	/// update virtual network
	PRL_RESULT cmdUpdateVirtualNetwork();
	/// delete virtual network
	PRL_RESULT cmdDeleteVirtualNetwork();
	PRL_RESULT cmdUpdateNetworkClassesConfig();
	PRL_RESULT cmdRestartNetworkShaping();
	PRL_RESULT cmdUpdateNetworkShapingConfig();

	PRL_RESULT cmdAddIPPrivateNetwork();
	PRL_RESULT cmdUpdateIPPrivateNetwork();
	PRL_RESULT cmdRemoveIPPrivateNetwork();

private:
	/// Starts PrlNetService if it is not running or restarts it.
	PRL_RESULT restartPrlNetService( );

	/// Helper function.
	/// Restart and reconfigure Parallels Adapter if it doesn't started yet.
	PRL_RESULT restartParallelsAdapter(
	    CVirtualNetwork *pOldConfig,
	    CVirtualNetwork *pNewConfig,
	    PrlNet::EthAdaptersList &prlAdaptersList);

	PRL_RESULT addHostOnlyVirtualNetwork(CVirtualNetwork *pVirtualNetwork,
			CDspLockedPointer<CParallelsNetworkConfig>& pNetworkConfig);
	PRL_RESULT updateVmNetwork(
			const QString sVmUuid,
			const QString svmDirUuid,
			const QString &sNewVirtualNetworkID,
			const QString &sOldVirtualNetworkID);

	PRL_RESULT updateVmNetworkSettings(
			const QString &sNewVirtualNetworkID,
			const QString &sOldVirtualNetworkID);
	PRL_RESULT cmdUpdateVirtualNetwork( CVirtualNetwork* pNewVirtualNetwork,
					CVirtualNetwork* pOldVirtualNetwork,
					CDspLockedPointer<CParallelsNetworkConfig>& pNetworkConfig);
	void convertDispAdapterToVirtualNetwork( CDispNetAdapter* pOldNetAdapter, CVirtualNetwork* pVirtualNetwork );
	PRL_RESULT validateIPPrivateNetwork(CPrivateNetwork *pNetwork);
private:
	PVE::IDispatcherCommands m_nCmd;
	static QMutex *g_pRestartShapingMtx;
	static bool g_restartShaping;
};

#endif //__Task_ManagePrlNetService_H_
