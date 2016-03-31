/////////////////////////////////////////////////////////////////////////////
/// Copyright (C) 2008-2015 Parallels IP Holdings GmbH
///
/// This file is part of Virtuozzo Core Libraries. Virtuozzo Core
/// Libraries is free software; you can redistribute it and/or modify it
/// under the terms of the GNU Lesser General Public License as published
/// by the Free Software Foundation; either version 2.1 of the License, or
/// (at your option) any later version.
///
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
///
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library.  If not, see
/// <http://www.gnu.org/licenses/> or write to Free Software Foundation,
/// 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
///
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/// @file netconfig.h
///
/// Helpers to do several networking configuration access tasks.
///
/// @author sdmitry
///
/////////////////////////////////////////////////////////////////////////////
#ifndef prl_netconfig_h__
#define prl_netconfig_h__

#include <prlcommon/Interfaces/ParallelsTypes.h>
#include <prlxmlmodel/NetworkConfig/CParallelsNetworkConfig.h>
#include <prlxmlmodel/VmConfig/CVmGenericNetworkAdapter.h>
#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include <Libraries/PrlNetworking/PrlNetLibrary.h>
#include <prlcommon/Std/SmartPtr.h>
#include <prlcommon/PrlCommonUtilsBase/netutils.h>
#include <QtAlgorithms>

#ifndef ETH_ADDRESS_LEN
# define ETH_ADDRESS_LEN 6
#endif

// _ipv6 is class Q_IPV6ADDR
// _pos is position in ipv6 addr (0..7)
// _val is 16bit value;
// Note:16bit value address part value stored in network byte order,
//		so we do swap to x86 format(little endian) to operate.
//		To get more info look at the test, commited in rev 568707
#define IPV6_ADD_VALUE_AT_POS( _ipv6, _pos, _val ) \
do { \
	Q_ASSERT( _pos < 8 ); \
	quint8* _p = ( _ipv6.c +( 15 - _pos*2 -1) ); \
	/* swap from network byte order	to little indian format (x86)*/ \
	qSwap( *_p, *(_p+1) ); \
	*( (quint16*)_p ) += (quint16)_val; \
	/* swap back to network byte order	*/ \
	qSwap( *_p, *(_p+1) ); \
}while(0);

namespace PrlNet
{

// Compare two MAC addresses
bool isEqualEthAddress(const QString &addr1, const QString &addr2);

/************************************************************************/

QString GetDefaultAdapterName(unsigned int adapterIndex, bool bSharedEnabled = true);
QString GetDefaultVirtualNetworkID(unsigned int adapterIndex, bool bSharedEnabled);
QString GetDefaultVirtualNetworkDescription(unsigned int adapterIndex, bool bSharedEnabled);

void	GetDefaultDhcpParams(
	unsigned int     adapterIndex,
	quint32 &dhcpScopeStartIp,
	quint32 &dhcpScopeEndIp,
	quint32 &dhcpScopeMask );

// Fills virtual network params with default values.
void FillDefaultVirtualNetworkParams( CVirtualNetwork* pVirtualNetwork,
	unsigned int nAdapterIndex, bool bSharedEnabled);

// Fills virtual network configuration with default values.
// Note: default values depend on current execute mode
void FillDefaultNetworks(CVirtualNetworks *pNetworks);

// Fills run-time virtual networks configuration.
void FillDefaultRuntimeNetworks(CParallelsNetworkConfig *pNetworkConfig,
		SmartPtr<CHostHardwareInfo> pHostHwInfo);

// Reset network configuration.
// This function removes all present virtual networks from configuration
// The function behaves differently depending on current execute-mode
// (see ParallelsDirs::getAppExecuteMode())
void FillDefaultConfiguration(CParallelsNetworkConfig *pNetworkConfig);

// Patches read config for product's specifics.
// * Changes network assignment from run-time DefaultAdapter to default adapter at
//   the moment of function call.
// * Disable NATServer for server product in run-time.
bool PatchConfig(CParallelsNetworkConfig *pNetworkConfig);

/************************************************************************/

// Get Virtual Network by networkID
CVirtualNetwork *GetVirtualNetworkByID(const CVirtualNetworks *pNetworks, const QString &networkID);
// Get Virtual Network by uuid
CVirtualNetwork *GetVirtualNetworkByUuid(CParallelsNetworkConfig *pNetworkConfig, const QString &sUuid);

// pNetworkConfig current network config
// pVirtualNetwork deleting virtual network object
// NOTE: After deleting the pointer pVirtualNetwork is not valid !!!
void DeleteVirtualNetwork(CParallelsNetworkConfig *pNetworkConfig, CVirtualNetwork* pVirtualNetwork);

// return VirtualNetwork that have specified Parallels adapter.
CVirtualNetwork *GetNetworkByParallelsAdapter(
	CVirtualNetworks *pNetworks,
	int nAdapterIndex);
// return VirtualNetwork that have specified MAC address and VLAN tag.
CVirtualNetwork *GetNetworkByBoundCardMacAndVLANTag(
	CVirtualNetworks *pNetworks, QString &qsMac, PRL_UINT16 nVLANTag);

// @param adapter_index index of HostOnly adapter. -1 for default host only.
//
#define PRL_DEFAULT_HOSTONLY_INDEX ((UINT)(-1))
#define PRL_DESKTOP_SHAREDNET_INDEX (0)
// Note: network 0 is always shared-networking for desktop.
// for desktop-mode GetHostOnlyNetwork() treats 0
// also as default host-only.
CVirtualNetwork *GetHostOnlyNetwork(const CParallelsNetworkConfig *pNetworkConfig, UINT adapter_index);
CVirtualNetwork *GetSharedNetwork(const CParallelsNetworkConfig *pNetworkConfig);
CVirtualNetwork *GetBridgedNetwork(const CParallelsNetworkConfig *pNetworkConfig);
CIPReservations *GetSafeDHCPIPReservations(CVirtualNetwork *pNetwork);
CIPReservations *GetDHCPIPReservations(CVirtualNetwork *pNetwork);

// Get Network adapter by Index
CVmGenericNetworkAdapter *GetAdapterByIndex(
	const QList<CVmGenericNetworkAdapter* > &m_lstNetworkAdapters,
	unsigned int adapterIndex);

CVmGenericNetworkAdapter fixMacFilter(
		const CVmGenericNetworkAdapter &adapter,
		const QList<CVmGenericNetworkAdapter*> &adapters);

// Get Virtual Network by adapter
// Warning! This function is rather helper in non-standard situations.
// Use with care.
CVirtualNetwork *GetVirtualNetworkForAdapter(
				CParallelsNetworkConfig *networkConfig,
				PRL_NET_ADAPTER_EMULATED_TYPE emulatedType,
				QString virtualNetworkID);

// Get Virtual Network type by adapter
PRL_RESULT GetNetworkTypeForAdapter(
				CParallelsNetworkConfig *networkConfig,
				const CVmGenericNetworkAdapter *adapter,
				PRL_NET_VIRTUAL_NETWORK_TYPE *nVNetType);

/*
	search lease with same IP or with same MAC
	return true - found, false - not found
*/
bool ContainsIPReservation(const CIPReservations* pIPReservations,
				const QHostAddress &IPAddress, const QString &MacAddress);


// Create list of host only networks sorted by Parallels Adapter number.
QList<CVirtualNetwork *> MakeHostOnlyNetworksList(const CParallelsNetworkConfig *pNetworkConfig);

// Create list of Parallels Adapters
QList<CParallelsAdapter *> MakeParallelsAdaptersList(const CParallelsNetworkConfig *pNetworkConfig);

// Return port for offmgmt-service with name srvName
UINT16 GetOffmgmtPortByService(const CParallelsNetworkConfig *pNetworkConfig, const QString &srvName);
// Return name for the offmgmt-service on port port
QString GetOffmgmtServiceByPort(const CParallelsNetworkConfig *pNetworkConfig, UINT16 port);


// returns adapter for virtual network
PRL_RESULT GetAdapterForNetwork(
	EthAdaptersList &adaptersList,
	const CVirtualNetwork *pNetwork,
	EthAdaptersList::Iterator &itAdapter);


// returns ethernet adapter for CVmGenericNetworkAdapter.
// Note: this function will check for presence of NetworkID in vmAdapter and will use it if it is filled.
// Otherwise, it will use PS30 params.
//
PRL_RESULT GetAdapterForVM(
	EthAdaptersList &adaptersList,
	CParallelsNetworkConfig *pNetworkConfig,
	const CVmGenericNetworkAdapter& vmAdapter,
	EthAdaptersList::Iterator &itAdapter);

/************************************************************************/

// Read networking configuration from default config file.
PRL_RESULT ReadNetworkConfig( CParallelsNetworkConfig &networkConfig );

// Write network configuration to the default config file location.
PRL_RESULT WriteNetworkConfig(CParallelsNetworkConfig &networkConfig);

// Read network configuration from default location.
// Create default network configuration if file don't exist.
// This is initial task that either done by dispatcher, or by unix
// Parallels network startup program.
// Returns an error if failed to parse file.
//
// @param bConfigurationRestored Will setup bConfigurationRestored if default configuration was created or
// existing configuration was restored.
PRL_RESULT InitNetworkConfig(CParallelsNetworkConfig &networkConfig, bool &bConfigurationRestored);

/************************************************************************/

// Validate parameters of host only network.
PRL_RESULT ValidateHostOnlyNetworkParams(CVirtualNetwork *pNetwork);

// Make VirtualNetworkID for adapter.
// if adapterName is empty, virtual network for default adapter will be created.
QString MakeVirtualNetworkID(const QString &adapterName, const QString &adapterSystemName );

// Make VirtualNetworkDescription for adapter.
// if adapterName is emptry, description for default adapter will be created.
QString MakeVirtualNetworkDescription(const QString &adapterName, const QString &adapterSystemName );

// Create a virtual network that is bound to the adapter.
// Virtual network on default adapter will be created if adapterName is empty.
CVirtualNetwork *CreateVirtualNetworkForAdapter(
	const QString &adapterName,
	const QString &adapterSystemName,
	const QString &adapterMacAddress,
	unsigned short vlanTag);

/************************************************************************/

// Allocates buffer with IP-addresses
// Returned ips in buffer are in host byte-order
// Returned buffer should be de deallocated with ::free
PRL_RESULT CreateIpsBuffer(const QList<QString> &ipList, UINT32 **ppBuffer, unsigned *pcnt);

// Allocates buffer with IP6-addresses
// Returned ips in buffer are in host byte-order
// Returned buffer should be de deallocated with ::free
PRL_RESULT CreateIp6sBuffer(const QList<QString> &ipList, UINT8 **ppBuffer, unsigned *pcnt);

// Initialize library global variables
// like system-flags, whether IPv6 enabled, whether need to use tap-networking etc.
void InitConfigLibrary(const CParallelsNetworkConfig *pNetworkConfig);

// Setup whether IPv6 is enabled. This value will be remembered
// and will be returned by isIPv6Enabled()
void setIPv6Enabled(bool enabled);

// IPv6 is enabled for Parallels desktop
// The value for this flag has to be calculated separately using
// CParallelsNetworkConfig and some host settings
bool isIPv6Enabled();

// Disable/Enabled use_tap_networking global var.
// This func allows to overwrite value initialized in InitConfigLibrary().
void setTapEnabled(bool enabled);

// True if TAP-mode is on.
bool isTapEnabled();

// Initialize system-flags for Networking.
// This func is called from InitLibrary(), but it is possible to override flags
// with this call.
void initSystemFlags(const QString &systemFlags);

// Obtain system-flag from network-configuration
UINT32 SfGetUINT32(const char *flag, UINT32 defaultValue);
UINT64 SfGetUINT64(const char *flag, UINT64 defaultValue);

// Shared Networking is disabled for PSBM
bool isSharedEnabled();

}

#endif //prl_netconfig_h__
