///////////////////////////////////////////////////////////////////////////////
///
/// @file PrlNetLibrary.h
/// @author sdmitry
///
/// This library solves next tasks:
/// - Enumerating existed Parallels adapters
/// - Enumerating existing Ethernet adapters
/// - Starting/Stopping/Pausing network daemons
/// - Obtaining network daemons status
/// - Notifying network daemons about changing configuration
/// - Installing/Uninstalling/Enabling/Disabling Parallels adapters
///
/// After all of these function system specific last error is correctly set
/// and in case of error system-specific text of the error could be obtained
/// using PrlNet::getSysErrorText();
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#ifndef PrlNetLibrary_h__
#define PrlNetLibrary_h__

#include <QString>
#include <QList>
#include <QHostAddress>

#include <prlcommon/Interfaces/ParallelsTypes.h>
#include <prlsdk/PrlErrors.h>
#include <prlxmlmodel/NetworkConfig/CHostOnlyNetwork.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlxmlmodel/VmConfig/CVmGenericNetworkAdapter.h>
#include <prlcommon/Std/SmartPtr.h>
#include "PrlNetworkingConstants.h"

class CParallelsNetworkConfig;

namespace PrlNet
{

/// @class ParallelsAdapter
/// Describes Ethernet Adapters (both Parallels and Ethernet)
struct EthernetAdapter
{
	QString 	_name;			///< User-friendly name of the adapter

	QString 	_systemName;    ///< System name of the adapter

	int			_adapterIndex;	///< Index of the adapter.
								///< Note: Prl adapters starts from index 0x80000000

	bool		_bParallelsAdapter; /// true if is a parallels adapter.

	QString 	_adapterGuid;   ///< Windows GUID of the adapter (Win32 only)

	unsigned short _vlanTag;	///< VLAN tag; PRL_INVALID_VLAN_TAG if not a vlan

	unsigned char _macAddr[6];  ///< Macaddress of the adapter

	bool		_bEnabled;		///< Is adapter enabled

	int		_nType;			///< type of adapter (PRL_NET_ADAPTER_TYPE)
};

// PrlAdapter is always greater then eth adapter.
// Or compare using index.
inline bool operator < (const EthernetAdapter& a, const EthernetAdapter& b)
{
	return a._adapterIndex < b._adapterIndex;
}


/// list of adapters
typedef QList<EthernetAdapter> EthAdaptersList;

/// returns maximum adapter index in the system.
/// e.g if 6 is returned, then adapter can have maximum index 6.
int		   getMaximumAdapterIndex();

/// Creates a list of Parallels adapters, both enabled and disabled
/// @param adaptersList [out] Resulting list of adapters
PRL_RESULT makePrlAdaptersList( EthAdaptersList &adaptersList );


/// Creates a list of all ethernet adapters.
/// @param adaptersList [out] Resulting list of adapters
/// @param bUpAdapters [in] Enumerate only UP adapters
/// @param bConfigured [in] Enumerate only configured adapters (with ip addrsses)
PRL_RESULT makeAdapterList( EthAdaptersList &adaptersList,
		bool bUpAdapters, bool bConfigured = false );

/// Creates a list of ethernet adapters to which VM and NAT is able to bind.
/// @param adaptersList [out] Resulting list of adapters
/// @param bUpAdapters [in] Enumerate only UP adapters
/// @param bConfigured [in] Enumerate only configured adapters (with ip addrsses)
PRL_RESULT makeBindableAdapterList( EthAdaptersList &adaptersList,
		bool bUpAdapters, bool bConfigured = false );


/// Get first non-parallels adapter on system.
/// Usually used in sequence
/// if (PRL_FAILED(getDefaultBridgedAdapter(...)
///     return getFirstAdapter(...);
PRL_RESULT getFirstAdapter(EthAdaptersList &adaptersList,
		EthAdaptersList::Iterator &itFirstAdapter);

/// Returns default ethernet adapter.
/// Note: when PRL_ERR_FAILURE is returned, it is recommended to
/// call getFirstAdapter().
PRL_RESULT getDefaultBridgedAdapter(EthAdaptersList &adaptersList,
	EthAdaptersList::Iterator &defaultAdapter);

/// Returns true if ethernet adapter is a WiFi adapter.
bool isWIFIAdapter(const EthernetAdapter& ethAdapter);

/// Returns true if ethernet adapter is the Parallels virtual adapter.
bool isVirtualAdapter(const EthernetAdapter& ethAdapter);

/// Enable/Disable Parallels adapter
/// @param adapterIndex [in] Parallels adapter to enable/disable
/// @param bEnable [in] True to enable operation, false - disable
PRL_RESULT enablePrlAdapter( int adapterIndex, bool bEnable );

/// Renames Parallels adapter
/// @param adapterIndex [in] Parallels adapter to rename
/// @param newName [in] new name of the adapter
PRL_RESULT renamePrlAdapter(
    const QString &prlDriversDir,
    int adapterIndex,
    bool bHiddenAdapter,
    const QString &newName );

/// Installs and starts Parallels network service
/// Note: in unix-systems installation of the service is not required
/// This function will simple start service
/// @param parallelsDir Directory where Parallels is installed
PRL_RESULT installPrlService( const QString &parallelsDir );

/// For debugging purposes
/// Removes Parallels network service
PRL_RESULT uninstallPrlService( );

struct SrvAction
{
	enum Action
	{
		Start,	///< Start service
		Stop,	///< Stop service
	};
};

/// Starts/stops Parallels network service
PRL_RESULT startPrlNetService( const QString &parallelsDir, SrvAction::Action action );

struct SrvStatus
{
	enum Status
	{
		Started,	 ///< Started
		Stopped,	 ///< Stopped
		NotInstalled, ///< NotInstalled, Win32 only
	};
};

/// Get current state of the Parallels network service
PRL_RESULT getPrlNetServiceStatus(SrvStatus::Status *pStatus);

/// Notifies Parallels network service about changes in configuration
/// (NIC was installed/uninstalled, DHCP parameters where changed etc)
PRL_RESULT notifyPrlNetService(const QString &parallelsDir);

/// Returns system last error
unsigned long getSysError();

/// Returns system-specific text for system last error
QString getSysErrorText();

/// Set PrlNet system error.
void setSysError(unsigned long err);


/// Returns default dhcp params for adapter
/// ToDo: remove this function and use one from netconfig.h
/// @param adapterIndex [in] index of the adapter
/// @param dhcpScopeStartIp [out] Start address of the DHCP Range
/// @param dhcpScopeEndIp 	[out] End address of the DHCP Range
/// @param dhcpScopeMask 	[out] Mask of the DHCP Range
void	getDefaultDhcpParams(
	int 		  adapterIndex,
	quint32 &dhcpScopeStartIp,
	quint32 &dhcpScopeEndIp,
	quint32 &dhcpScopeMask );

/// Starts networking adapters configured in Dispatcher.xml
PRL_RESULT startNetworking( const QString &parallelsDir, const QString &prlDriversDir );

/// Stops parallels networking adapters (Unix only)
PRL_RESULT stopNetworking(const QString &parallelsDir);

#if defined(_MAC_)
/// Removes all parallels interfaces from the SCPreferences
PRL_RESULT uninstallNetworkAdapters();

/// returns true if Mac Connection Sharing is enabled for the adapter
bool isConnectionSharingEnabled(const char *bsdIfaceName);
#endif

#ifdef _LIN_
/// setup IP-addresses for adapter.
PRL_RESULT setAdapterIpAddresses(const QString& sAdapter,
		const CHostOnlyNetwork *pNetwork);

PRL_RESULT setAdapterIpAddress(const QString& adapterName,
                const QString & ipAddressMask);
#endif


/// setup IP-addresses for parallels adapter.
/// @param bHiddenAdapter Ignored for win/lin and used for Mac only.
///	true  if change should be done using IOCTL to interface,
///	false if using Mac OS Network Preferences.
PRL_RESULT setPrlAdapterIpAddresses(int adapterIndex, bool bHiddenAdapter,
		const CHostOnlyNetwork *pNetwork);

/// Delete IP-addresses from parallels adapter.
/// So far do nothing for win/lin
/// @param bHiddenAdapter see setPrlAdapterIpAddresses func.
PRL_RESULT delPrlAdapterIpAddresses(int adapterIndex, bool bHiddenAdapter,
		const CHostOnlyNetwork *pNetwork);

#if !defined(_WIN_)
/// load Parallels Netbridge driver.
int loadPrlNetbridgeKext( const QString &qPrlDriversDir );
/// load Parallels Adapters driver.
int loadPrlAdaptersKext( const QString &qPrlDriversDir );
/// Set unix-interface flags
bool setIfFlags( const char *if_name, int set_flags, int clear_flags );
#endif

#if defined(_WIN_)

/// Obtains parameters of Parallels adapter as it is configured in the
/// host operating system. This is required for the adapter
/// for which DHCP is disabled but NAT is enabled
/// @param prlAdapterIndex [in] index of the Parallels adapter
/// @param ipaddr [out] ip address of the adapter
/// @param netmask [out] netmask of the adapter
PRL_RESULT  getPrlSystemAdapterParams(
	int prlAdapterIndex,
	unsigned int &ipaddr,
	unsigned int &netmask );
#endif

#if defined(_WIN_) || defined(_LIN_)
/// Disable IPv6 router discovery on Parallels adapter
PRL_RESULT DisablePrlIPv6RouterDiscovery(int prlAdapterIndex);
#endif

// #133154  fake call to link prl_net_start  with PrlNetworking library without errors for linux / MacOS X Tiger
// more info: https://bugzilla.sw.ru/show_bug.cgi?id=133154#c13
#if defined(_LIN_) || defined(_MAC_)
void FakeInitCall();
#endif


#if !defined(_WIN_)
/// Create an ARP address mapping entry for ip address with hardware address set to hwaddr
///
/// @param ip [in] ip address
/// @param devName [in] network interface
/// @param hwaddr [in] hardware address
PRL_RESULT addArp(const QString &ip, const QString &devName, const QString &hwaddr);

/// Remove any ARP entry for the specified hardware address
PRL_RESULT delArp(const QString &ip, const QString &devName, const QString &hwaddr);

/// Add or delete routes for IP via device
/// function is analog of 'ip route add <ipAddress> dev <devName>'
/// @param ipAddress [in] - IPv4 or IPv6 address and word 'default'
/// @param devName [in] - name of interface
/// @param add [in] - true = add route, false = delete route
/// @param metric [in] - route priority, -1 not specified
/// @return true  - settings successfully applied
///         false - some error
bool SetRouteToDevice(const QString &ipAddress, const QString &devName, bool add = true, int metric = -1);
#endif

#if defined(_LIN_)

/// Add or delete proxy ARP for device.
/// function is analog of 'ip neigh add proxy <ip> dev <devName>'
/// @param ipAddress [in] - IPv4 or IPv6 address
/// @param devName [in] - name of interface
/// @param add [in] - true = add arp, false = delete
/// @return true  - settings successfully applied
///         false - some error
bool SetArpToDevice(const QString &ip, const QString &devName, bool add = true);

/// Add or delete proxy arp to all physical devices on hardware node
/// command line analog
/// for d in eth* ; do
///	ip neigh add proxy <ip> dev $d
/// done
/// @param ipAddress [in] - IPv4 or IPv6 address
//. @param srcMmac [in] - source mac asddress used for update neighbours ARP caches
/// @param add [in] - true = add arp, false = remove arp
/// @param annonce [in] - update neighbours ARP caches
/// @return true  - settings successfully applied
///         false - some error
bool SetArpToNodeDevices(const QString &ipAddress, const QString &srcMac, bool add, bool annonce);

#endif

#if defined(_MAC_)
/// Obtains parent interface name and vlan-tag of adapter
/// @return false if not a vlan adapter
bool GetVLANInfo(const char *vlan_if_name, QString &parent_name, unsigned short &vlan_tag);
#endif

/// Update neighbours ARP caches
bool updateArp(const QString &ip, const QString &src_hwaddr, const QString &iface);

/// Calculates IPv6 prefix in bits from provided IPv6 network mask
unsigned char getIPv6PrefixFromMask(const Q_IPV6ADDR *mask);

/// Gets current networking mode (vme* or vnic*)
PRL_NET_MODE getMode();

/// returns true if IPv6 default route is present on host
bool IsIPv6DefaultRoutePresent();

QHostAddress getIPv4MaskFromPrefix(quint32 prefix4_);
QHostAddress getIPv6MaskFromPrefix(quint32 prefix6_);

QString findAdapterName(const QString& mac_, unsigned short vlan_);

// return bridge name that has given interface
QString getBridgeName(const QString& iface);

// remove interface from its bridge
bool releaseInterface(const QString& iface);
bool connectInterface(const QString& iface, const QString& bridge);

QStringList makePhysicalAdapterList();
}

#endif // PrlNetLibrary_h__

