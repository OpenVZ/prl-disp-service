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
/// @file netconfig.cpp
///
/// Helpers to do several networking configuration access tasks.
///
/// @author sdmitry
///
/////////////////////////////////////////////////////////////////////////////

#include "netconfig.h"

#include <QMap>

#include <Interfaces/ParallelsNamespace.h>
#include <prlcommon/PrlCommonUtilsBase/ParallelsDirs.h>
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/PrlCommonUtilsBase/PrlStringifyConsts.h>
#include <prlcommon/PrlCommonUtilsBase/NetworkUtils.h>

#include <prlsdk/PrlEnums.h>

#define DEFAULT_DHCP_SUBNET_IP	 "10.37.130.0"
#define DEFAULT_DHCP_SUBNET_MASK "255.255.255.0"

/* Use Local IPv6 Unicast Addresses (http://www.ietf.org/rfc/rfc4193.txt)
   The format is:
      | 7 bits |1|  40 bits   |  16 bits  |          64 bits           |
      +--------+-+------------+-----------+----------------------------+
      | Prefix |L| Global ID  | Subnet ID |        Interface ID        |
      +--------+-+------------+-----------+----------------------------+
   where prefix is fc00::/7, L=1,
   Global ID is some random number (we use constant = 0xb2:2c26:f4e4)
   SubnetID is equal to HostOnly Network Number
*/
#define DEFAULT_DHCP6_SUBNET_IP	  "fdb2:2c26:f4e4::"
#define DEFAULT_DHCP6_SUBNET_MASK "ffff:ffff:ffff:ffff:0:0:0:0"


#define PRLNET_DEFAULT_HOST_ONLY_ADAPTER_NAME		"Parallels Host-Only"
#define PRLNET_DEFAULT_SHARED_ADAPTER_NAME		"Parallels Shared"

#define PRLNET_DEFAULT_NETWORK_ID			"Host-Only"
#define PRLNET_DEFAULT_NETWORK_DESCRIPTION	"Host Only Network"
// This default net is created at the moment of creation of config
// and is changed only either explicitly, or at restore-defaults.
// It is bound to the adapter that was default at the moment
// of creation
#define PRLNET_SRV_DEFAULT_BRIDGED_ID		"Bridged"
#define PRLNET_SRV_DEFAULT_BRIDGED_DESCRIPTION	"Bridged Network"

static const char* DefaultVirtualNetworkIDs[2] =
{
	"Shared",
	"Host-Only"
};


static const char* DefaultVirtualNetworkDescriptions[2] =
{
	"Shared Networking",
	"Host Only Networking"
};

QString PrlNet::GetDefaultAdapterName(unsigned int nAdapterIndex, bool bSharedEnabled)
{
	QString rv;
	nAdapterIndex = nAdapterIndex & (PRL_ADAPTER_START_INDEX - 1);
	if (nAdapterIndex == 0 && bSharedEnabled) // Shared networking
		rv.sprintf("%s #%d", PRLNET_DEFAULT_SHARED_ADAPTER_NAME, nAdapterIndex);
	else
		rv.sprintf("%s #%d", PRLNET_DEFAULT_HOST_ONLY_ADAPTER_NAME, nAdapterIndex);
	return rv;
}


QString PrlNet::GetDefaultVirtualNetworkID(unsigned int nAdapterIndex, bool bSharedEnabled)
{
	if (bSharedEnabled && nAdapterIndex < 2)
		return DefaultVirtualNetworkIDs[nAdapterIndex];
	else if (!nAdapterIndex)
		return DefaultVirtualNetworkIDs[1];

	QString rv;
	rv.sprintf("%s #%d", PRLNET_DEFAULT_NETWORK_ID, nAdapterIndex);
	return rv;
}


QString PrlNet::GetDefaultVirtualNetworkDescription(unsigned int nAdapterIndex, bool bSharedEnabled)
{
	if (bSharedEnabled && nAdapterIndex < 2)
		return DefaultVirtualNetworkDescriptions[nAdapterIndex];
	else if (!nAdapterIndex)
		return DefaultVirtualNetworkDescriptions[1];

	QString rv;
	rv.sprintf("%s #%d", PRLNET_DEFAULT_NETWORK_DESCRIPTION, nAdapterIndex);
	return rv;
}


void	PrlNet::GetDefaultDhcpParams(
	unsigned int nAdapterIndex,
	quint32 &dhcpScopeStartIp,
	quint32 &dhcpScopeEndIp,
	quint32 &dhcpScopeMask )
{
	quint32 scopeMask = QHostAddress(DEFAULT_DHCP_SUBNET_MASK).toIPv4Address();

	quint32 scopeSubnet;

	scopeSubnet = QHostAddress(DEFAULT_DHCP_SUBNET_IP).toIPv4Address();
	scopeSubnet += nAdapterIndex * 256;

	quint32 scopeStart = scopeSubnet + 1;
	quint32 scopeEnd = (scopeSubnet | ~scopeMask) - 1;

	dhcpScopeStartIp = scopeStart;
	dhcpScopeEndIp = scopeEnd;
	dhcpScopeMask = scopeMask;
}

static void GetDefaultDhcpV6Params(
	unsigned int nAdapterIndex,
	Q_IPV6ADDR &scopeStartIp,
	Q_IPV6ADDR &scopeEndIp,
	Q_IPV6ADDR &scopeMask )
{
	scopeMask   = QHostAddress(DEFAULT_DHCP6_SUBNET_MASK).toIPv6Address();
	Q_IPV6ADDR scopeSubnet = QHostAddress(DEFAULT_DHCP6_SUBNET_IP).toIPv6Address();
	IPV6_ADD_VALUE_AT_POS( scopeSubnet, 4, nAdapterIndex );
	scopeStartIp = scopeEndIp = scopeSubnet;

	IPV6_ADD_VALUE_AT_POS( scopeStartIp, 0, 0 );
	IPV6_ADD_VALUE_AT_POS( scopeEndIp, 0, 0xffff );
	IPV6_ADD_VALUE_AT_POS( scopeEndIp, 1, 0xffff );
	IPV6_ADD_VALUE_AT_POS( scopeEndIp, 2, 0xffff );
	IPV6_ADD_VALUE_AT_POS( scopeEndIp, 3, 0xffff );
}


/************************************************************************/

void PrlNet::FillDefaultVirtualNetworkParams(
	CVirtualNetwork* pVirtualNetwork,
	unsigned int nAdapterIndex,
	bool bSharedEnabled)
{
	pVirtualNetwork->setNetworkType(PVN_HOST_ONLY);

	if (pVirtualNetwork->getNetworkID().isEmpty())
	{
		QString NetworkID = PrlNet::GetDefaultVirtualNetworkID(nAdapterIndex, bSharedEnabled);
		pVirtualNetwork->setNetworkID(NetworkID);
	}

	if (pVirtualNetwork->getDescription().isEmpty())
	{
		QString NetworkDescription =
			PrlNet::GetDefaultVirtualNetworkDescription(nAdapterIndex, bSharedEnabled);
		pVirtualNetwork->setDescription(NetworkDescription);
	}

	CHostOnlyNetwork *pHostOnlyNetworkParams = pVirtualNetwork->getHostOnlyNetwork();
	if (!pHostOnlyNetworkParams)
	{
		pHostOnlyNetworkParams = new CHostOnlyNetwork;
		pVirtualNetwork->setHostOnlyNetwork(pHostOnlyNetworkParams);
	}

	// IPv4
	quint32 dhcpScopeStartIp = 0;
	quint32 dhcpScopeEndIp = 0;
	quint32 dhcpScopeMask = 0;
	PrlNet::GetDefaultDhcpParams( nAdapterIndex, dhcpScopeStartIp, dhcpScopeEndIp, dhcpScopeMask );

	pHostOnlyNetworkParams->setDhcpIPAddress( QHostAddress(dhcpScopeStartIp) );
	pHostOnlyNetworkParams->setHostIPAddress( QHostAddress(dhcpScopeStartIp + 1) );
	pHostOnlyNetworkParams->setIPNetMask( QHostAddress(dhcpScopeMask) );

	// IPv6
	Q_IPV6ADDR dhcp6ScopeStartIp, dhcp6ScopeEndIp, dhcp6ScopeMask;
	dhcp6ScopeStartIp = dhcp6ScopeEndIp = dhcp6ScopeMask
		= QHostAddress( QHostAddress::AnyIPv6 ).toIPv6Address(); // ::
	GetDefaultDhcpV6Params( nAdapterIndex, dhcp6ScopeStartIp, dhcp6ScopeEndIp, dhcp6ScopeMask );

	Q_IPV6ADDR hostIp6 =  dhcp6ScopeStartIp;
	IPV6_ADD_VALUE_AT_POS( hostIp6, 0, 1); // next address after dhcp ip (like ip4)
	pHostOnlyNetworkParams->setDhcpIP6Address( QHostAddress(dhcp6ScopeStartIp) );
	pHostOnlyNetworkParams->setHostIP6Address( QHostAddress(hostIp6 ) );
	pHostOnlyNetworkParams->setIP6NetMask( QHostAddress(dhcp6ScopeMask) );

	CParallelsAdapter *pAdapter = pHostOnlyNetworkParams->getParallelsAdapter();
	if (!pAdapter)
	{
		pAdapter = new CParallelsAdapter;
		pHostOnlyNetworkParams->setParallelsAdapter(pAdapter);
	}

	QString adapterName = PrlNet::GetDefaultAdapterName(nAdapterIndex, bSharedEnabled);

	// Setup default value for whether adapter is hidden
	pAdapter->setHiddenAdapter();

	pAdapter->setName(adapterName);
	pAdapter->setPrlAdapterIndex(nAdapterIndex);

	// IPv4
	CDHCPServer *pDHCPServer = pHostOnlyNetworkParams->getDHCPServer();
	if (!pDHCPServer)
	{
		pDHCPServer = new CDHCPServer;
		pHostOnlyNetworkParams->setDHCPServer(pDHCPServer);
	}
	pDHCPServer->setEnabled(true);
	pDHCPServer->setIPScopeStart(QHostAddress(dhcpScopeStartIp));
	pDHCPServer->setIPScopeEnd(QHostAddress(dhcpScopeEndIp));

	// IPv6
	CDHCPServer *pDHCPv6Server = pHostOnlyNetworkParams->getDHCPv6Server();
	if (!pDHCPv6Server)
	{
		pDHCPv6Server = new CDHCPServer;
		pHostOnlyNetworkParams->setDHCPv6Server(pDHCPv6Server);
	}
	pDHCPv6Server->setEnabled(true);
	pDHCPv6Server->setIPScopeStart(QHostAddress(dhcp6ScopeStartIp));
	pDHCPv6Server->setIPScopeEnd(QHostAddress(dhcp6ScopeEndIp));

	CNATServer *pNatServer = pHostOnlyNetworkParams->getNATServer();
	if (!pNatServer)
	{
		pNatServer = new CNATServer;
		pHostOnlyNetworkParams->setNATServer(pNatServer);
	}

	if (nAdapterIndex == 0 && bSharedEnabled)
	{
		pNatServer->setEnabled(true);
	}
	else
	{
		pNatServer->setEnabled(false);
	}
}


void PrlNet::FillDefaultNetworks(CVirtualNetworks *pNetworks)
{
	int index = 0;

	if (isSharedEnabled())
	{
		CVirtualNetwork *pSharedNetwork = PrlNet::GetNetworkByParallelsAdapter(pNetworks, index);
		if (!pSharedNetwork)
		{
			pSharedNetwork = new CVirtualNetwork;
			pNetworks->m_lstVirtualNetwork.append(pSharedNetwork);
		}

		PrlNet::FillDefaultVirtualNetworkParams(pSharedNetwork, index, true);
		index++;
	}

	CVirtualNetwork *pHostOnlyNetwork = PrlNet::GetNetworkByParallelsAdapter(pNetworks, index);
	if (!pHostOnlyNetwork)
	{
		pHostOnlyNetwork = new CVirtualNetwork;
		pNetworks->m_lstVirtualNetwork.append(pHostOnlyNetwork);
	}
	PrlNet::FillDefaultVirtualNetworkParams(pHostOnlyNetwork, index, isSharedEnabled());

	PRL_APPLICATION_MODE appMode = ParallelsDirs::getAppExecuteMode();

	// Nothing to do for now for non-server mode since
	// vnets are supported only in server
	if (appMode != PAM_SERVER)
		return;

	// Add default-bridged for server-mode.
	// Note: default-bridged is calculated only once.
	// This is not a "default-adapter", but an adapter that is default
	// at the moment of calling of this function.

	// The things are too complicated if user have already done something with
	// virtual networks and adapter which is default now is already
	// present in some virtual network. It is too dificult and error-prone
	// to delete the network and other stuff.
	// Just don't do anything if default adapter is already present
	// in some vnet.
	EthAdaptersList ethList;
	PRL_RESULT prlResult = PrlNet::makeBindableAdapterList(ethList, true);
	if (PRL_FAILED(prlResult)) {
		WRITE_TRACE(DBG_FATAL, "FillDefaultNet: Failed to create list of host network adapters: 0x%08x",
					prlResult);
		return;
	}

	EthAdaptersList::iterator itAdapter;
	prlResult = PrlNet::getDefaultBridgedAdapter(ethList, itAdapter);
	if (PRL_FAILED(prlResult))
		prlResult = PrlNet::getFirstAdapter(ethList, itAdapter);
	if (PRL_FAILED(prlResult)) {
		WRITE_TRACE(DBG_FATAL, "FillDefaultNet: Failed to determine default adapter: 0x%08x",
					prlResult);
		return;
	}

	QString mac = ethAddressToString(itAdapter->_macAddr);
	CVirtualNetwork* pNet;
	pNet = GetNetworkByBoundCardMacAndVLANTag(pNetworks, mac, itAdapter->_vlanTag);
	if (pNet && pNet->getNetworkID() == PRLNET_SRV_DEFAULT_BRIDGED_ID)
		return; // it's already ok

	// it is not an error (see comment above about dificulties),
	// but we should log this event
	if (pNet) {
		WRITE_TRACE(DBG_FATAL,
					"FillDefaultNet: Failed to create %s "
					"because net '%s' is already using interface %s",
					PRLNET_SRV_DEFAULT_BRIDGED_ID,
					QSTR2UTF8(pNet->getNetworkID()),
					QSTR2UTF8(itAdapter->_systemName));
		return;
	}

	// At this point we know that thart default adapter is not used by anyone
	// Just remove existing default-server network and add it again.
	pNet = PrlNet::GetVirtualNetworkByID(pNetworks, PRLNET_SRV_DEFAULT_BRIDGED_ID);
	if (!pNet)
		pNet = new CVirtualNetwork;

	pNet->setEnabled(true);
	pNet->setNetworkID(PRLNET_SRV_DEFAULT_BRIDGED_ID);
	pNet->setNetworkType(PVN_BRIDGED_ETHERNET);
	pNet->setDescription(PRLNET_SRV_DEFAULT_BRIDGED_DESCRIPTION);
	pNet->setBoundCardMac(mac);
	pNet->setVLANTag(itAdapter->_vlanTag);
	pNetworks->m_lstVirtualNetwork.append(pNet);
}


void PrlNet::FillDefaultConfiguration(CParallelsNetworkConfig *pNetworkConfig)
{
	CVirtualNetworks *pNetworks = new CVirtualNetworks;
	pNetworkConfig->setVirtualNetworks(pNetworks);
	PrlNet::FillDefaultNetworks(pNetworks);
}


void PrlNet::FillDefaultRuntimeNetworks(CParallelsNetworkConfig *pNetworkConfig,
	SmartPtr<CHostHardwareInfo> pHostHwInfo)
{
	NetworkAdapters *pNetworkAdapters = pHostHwInfo->getNetworkAdapters();
	PRL_ASSERT(pNetworkAdapters);

	//
	// Fill all available network cards as virtual networks.
	//
	CVirtualNetworks *pVirtualNetworks = pNetworkConfig->getVirtualNetworks();
	PRL_ASSERT(pVirtualNetworks);

	// Remove all previousle filled bridged networks from list.
	QMutableListIterator<CVirtualNetwork*> it(pVirtualNetworks->m_lstVirtualNetwork);
	while(it.hasNext())
	{
		CVirtualNetwork *pNetwork = it.next();
		if (pNetwork->getNetworkType() != PVN_HOST_ONLY)
		{
			delete pNetwork;//https://bugzilla.sw.ru/show_bug.cgi?id=444663
			it.remove();
		}
	}

	foreach(CHwNetAdapter* pAdapter, pNetworkAdapters->m_lstNetworkAdapter)
	{
		CVirtualNetwork *pNetwork = PrlNet::CreateVirtualNetworkForAdapter(
			pAdapter->getDeviceName(), pAdapter->getDeviceId(), pAdapter->getMacAddress(),
			pAdapter->getVLANTag());

		pVirtualNetworks->m_lstVirtualNetwork.append(pNetwork);
	}

	//
	// Add default adapter if at least one HwAdapter is present.
	//
	if (!pNetworkAdapters->m_lstNetworkAdapter.isEmpty())
	{
		pVirtualNetworks->m_lstVirtualNetwork.append(
			PrlNet::CreateVirtualNetworkForAdapter("","","", PRL_INVALID_VLAN_TAG) );
	}

	return;
}

bool PrlNet::PatchConfig(CParallelsNetworkConfig *pNetworkConfig)
{

	if (ParallelsDirs::getAppExecuteMode() != PAM_SERVER)
		return false;

	CVirtualNetworks *pVirtualNetworks = pNetworkConfig->getVirtualNetworks();
	if (!pVirtualNetworks)
		return false;

	bool bFixed = false;
	foreach(CVirtualNetwork *pNetwork, pVirtualNetworks->m_lstVirtualNetwork)
	{
		// fix default adapter
		if (pNetwork->getNetworkType() == PVN_BRIDGED_ETHERNET &&
				pNetwork->isBridgedToDefaultAdapter())
		{
			EthAdaptersList ethList;
			PRL_RESULT prlResult = PrlNet::makeBindableAdapterList(ethList, true);
			if (PRL_FAILED(prlResult)) {
				WRITE_TRACE(DBG_FATAL, "Failed to create list of host network adapters: 0x%08x",
						prlResult);
				return bFixed;
			}

			EthAdaptersList::iterator itAdapter;
			prlResult = PrlNet::getDefaultBridgedAdapter(ethList, itAdapter);
			if (PRL_FAILED(prlResult))
				prlResult = PrlNet::getFirstAdapter(ethList, itAdapter);
			if (PRL_FAILED(prlResult)) {
				WRITE_TRACE(DBG_FATAL, "Failed to determine default adapter: 0x%08x",
						prlResult);
				return bFixed;
			}

			QString mac = ethAddressToString(itAdapter->_macAddr);
			pNetwork->setBoundCardMac(mac);
			pNetwork->setVLANTag(itAdapter->_vlanTag);
			bFixed = true;

			WRITE_TRACE(DBG_INFO, "Virtual Network \"%s\" assignment to default adapter fixed.",
					QSTR2UTF8(pNetwork->getNetworkID()));
		}

		// no shared networking on server - ignore read Enabled value
		if (!isSharedEnabled() && pNetwork->getNetworkType() == PVN_HOST_ONLY)
		{
			CHostOnlyNetwork* pHostOnlyNetwork = pNetwork->getHostOnlyNetwork();
			CNATServer* pNatServer = pHostOnlyNetwork ? pHostOnlyNetwork->getNATServer() : NULL;
			if (pNatServer && pNatServer->isEnabled())
				pNatServer->setEnabled(false);
		}
	}
	return bFixed;
}

/************************************************************************/

static inline bool is_eth_separator(char c)
{
	return c =='-' || c == ':';
}

// converts hex-char character to number. -1 if c is  not a hex digit.
static inline int hex2num(char c)
{
	c = toupper(c);
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	else
		return -1;
}

bool PrlNet::isEqualEthAddress(const QString &addr1, const QString &addr2)
{
	unsigned char mac1[ETH_ADDRESS_LEN], mac2[ETH_ADDRESS_LEN];

	PrlNet::stringToEthAddress(addr1, mac1);
	PrlNet::stringToEthAddress(addr2, mac2);

	return (memcmp(mac1, mac2, ETH_ADDRESS_LEN) == 0);
}

/************************************************************************/

CVirtualNetwork *PrlNet::GetVirtualNetworkByID(const CVirtualNetworks *pNetworks, const QString &networkID)
{
	if (NULL == pNetworks)
		return NULL;

	foreach(CVirtualNetwork *pNetwork, pNetworks->m_lstVirtualNetwork)
	{
		if (pNetwork->getNetworkID() == networkID)
			return pNetwork;
	}

	return NULL;
}

CVirtualNetwork *PrlNet::GetVirtualNetworkByUuid(CParallelsNetworkConfig *pNetworkConfig, const QString &sUuid)
{
	CVirtualNetworks *pNetworks = pNetworkConfig->getVirtualNetworks();
	if (NULL == pNetworks)
		return NULL;

	foreach(CVirtualNetwork *pNetwork, pNetworks->m_lstVirtualNetwork)
	{
		if (pNetwork->getUuid() == sUuid)
			return pNetwork;
	}

	return NULL;
}

void PrlNet::DeleteVirtualNetwork(CParallelsNetworkConfig *pNetworkConfig, CVirtualNetwork* pVirtualNetwork)
{
	CVirtualNetworks *pNetworks = pNetworkConfig->getVirtualNetworks();
	if (NULL == pNetworks || ! pVirtualNetwork)
		return;

	pNetworks->m_lstVirtualNetwork.removeAll( pVirtualNetwork );
	delete pVirtualNetwork;
}

// Create list of host only networks sorted by Parallels Adapter number (both enabled and disabled)
QList<CVirtualNetwork *>  PrlNet::MakeHostOnlyNetworksList(const CParallelsNetworkConfig *pNetworkConfig)
{
	CVirtualNetworks *pNetworks = pNetworkConfig->getVirtualNetworks();
	if (NULL == pNetworks)
		return QList<CVirtualNetwork *>();

	QMap<unsigned int, CVirtualNetwork*> mapHostOnlyNetworks;

	foreach(CVirtualNetwork *pNetwork, pNetworks->m_lstVirtualNetwork)
	{
		if ( pNetwork->getNetworkType() != PVN_HOST_ONLY)
			continue;

		CHostOnlyNetwork *pHostOnlyNetwork = pNetwork->getHostOnlyNetwork();
		if ( !pHostOnlyNetwork )
			continue;

		CParallelsAdapter *pParallelsAdapter = pHostOnlyNetwork->getParallelsAdapter();
		if (!pParallelsAdapter)
			continue;

		mapHostOnlyNetworks.insert(pParallelsAdapter->getPrlAdapterIndex(), pNetwork);
	}

	return mapHostOnlyNetworks.values();
}


QList<CParallelsAdapter *> PrlNet::MakeParallelsAdaptersList(const CParallelsNetworkConfig *pNetworkConfig)
{
	QList<CParallelsAdapter *> adaptersList;

	QList<CVirtualNetwork *> pNetworks = PrlNet::MakeHostOnlyNetworksList(pNetworkConfig);
	foreach( CVirtualNetwork *pNetwork, pNetworks )
	{
		if (!pNetwork->isEnabled())
			continue;

		CHostOnlyNetwork *pHostOnlyNetwork = pNetwork->getHostOnlyNetwork();
		if (NULL == pHostOnlyNetwork)
		{
			Q_ASSERT(0);
			continue;
		}

		CParallelsAdapter *pAdapter = pHostOnlyNetwork->getParallelsAdapter();
		if (NULL == pAdapter)
		{
			continue;
		}
		adaptersList.append(pAdapter);
	}

	return adaptersList;
}


/************************************************************************/

CVirtualNetwork *PrlNet::GetHostOnlyNetwork(const CParallelsNetworkConfig *pNetworkConfig, UINT adapter_index)
{
	// In desktop mode adapter 0 is SharedNetworking. GetHostOnlyNetwork() never returns shared-networking.
	// For server there is not shared networking at all.
	if (ParallelsDirs::getAppExecuteMode() == PAM_SERVER) {
		if (PRL_DEFAULT_HOSTONLY_INDEX == adapter_index)
			adapter_index = 0;
	} else {
		if (PRL_DESKTOP_SHAREDNET_INDEX == adapter_index || PRL_DEFAULT_HOSTONLY_INDEX == adapter_index)
			adapter_index = 1;
	}

	CVirtualNetworks *pVirtualNetworks = pNetworkConfig->getVirtualNetworks();
	if (NULL == pVirtualNetworks) {
		PRL_ASSERT(pVirtualNetworks);
		return NULL;
	}

	adapter_index = GET_PRL_ADAPTER_NUMBER(adapter_index);
	return PrlNet::GetNetworkByParallelsAdapter(pVirtualNetworks, (int)adapter_index);
}


CVirtualNetwork *PrlNet::GetSharedNetwork(const CParallelsNetworkConfig *pNetworkConfig)
{
	QList<CVirtualNetwork *> lstNet = MakeHostOnlyNetworksList(pNetworkConfig);
	foreach(CVirtualNetwork *pNetwork, lstNet)
	{
		if (!pNetwork->isEnabled())
			continue;

		CHostOnlyNetwork* pHostOnlyParams = pNetwork->getHostOnlyNetwork();
		if (NULL == pHostOnlyParams)
			continue;

		CNATServer *pNatServer = pHostOnlyParams->getNATServer();
		if ( pNatServer && pNatServer->isEnabled() )
			return pNetwork;
	}
	return NULL;
}

CVirtualNetwork *PrlNet::GetBridgedNetwork(const CParallelsNetworkConfig *pNetworkConfig)
{
	CVirtualNetworks *pNetworks = pNetworkConfig->getVirtualNetworks();
	if (NULL == pNetworks)
		return NULL;

	foreach(CVirtualNetwork *pNetwork, pNetworks->m_lstVirtualNetwork)
	{
		if (pNetwork->isEnabled() && pNetwork->getNetworkType() == PVN_BRIDGED_ETHERNET)
			return pNetwork;
	}

	return NULL;
}


CIPReservations *PrlNet::GetSafeDHCPIPReservations(CVirtualNetwork *pNetwork)
{
	if (!pNetwork)
		return NULL;

	CHostOnlyNetwork *pHostOnly = pNetwork->getHostOnlyNetwork();
	if (!pHostOnly) {
		pHostOnly = new CHostOnlyNetwork();
		pNetwork->setHostOnlyNetwork( pHostOnly );
	}

	CDHCPServer* pCDHCPServer = pHostOnly->getDHCPServer();
	if (!pCDHCPServer) {
		pCDHCPServer = new CDHCPServer();
		pHostOnly->setDHCPServer(pCDHCPServer);
	}

	CIPReservations* pIPReservations = pCDHCPServer->getIPReservations();
	if (!pIPReservations) {
		pIPReservations = new CIPReservations();
		pCDHCPServer->setIPReservations(pIPReservations);
	}

	return pIPReservations;
}

CIPReservations *PrlNet::GetDHCPIPReservations(CVirtualNetwork *pNetwork)
{
	if (!pNetwork)
		return NULL;
	CHostOnlyNetwork *pHostOnly = pNetwork->getHostOnlyNetwork();
	if (!pHostOnly)
		return NULL;

	CDHCPServer* pCDHCPServer = pHostOnly->getDHCPServer();
	if (!pCDHCPServer)
		return NULL;

	CIPReservations* pIPReservations = pCDHCPServer->getIPReservations();

	return pIPReservations;
}

/*
	return true - found, false - not found
*/
bool PrlNet::ContainsIPReservation(const CIPReservations* pIPReservations,
						const QHostAddress &IPAddress, const QString &MacAddress)
{
	if (!pIPReservations)
		return false;

	foreach(CIPReservation *pLease, pIPReservations->m_lstIPReservation)
	{
		if (!pLease)
			continue;

		if (pLease->getIPAddress() == IPAddress ||
			PrlNet::isEqualEthAddress(pLease->getMacAddress(), MacAddress))
				return true;
	}

	return false;
}

CVirtualNetwork *PrlNet::GetVirtualNetworkForAdapter(CParallelsNetworkConfig *networkConfig,
				PRL_NET_ADAPTER_EMULATED_TYPE emulatedType, QString virtualNetworkID )
{
	CVirtualNetwork *pVirtNetworking = NULL;
	if (!networkConfig)
		return NULL;

	if (!virtualNetworkID.isEmpty())
	{
		pVirtNetworking = PrlNet::GetVirtualNetworkByID(networkConfig->getVirtualNetworks(),
						virtualNetworkID);
	}else if(emulatedType == PNA_HOST_ONLY)
	{
		pVirtNetworking = PrlNet::GetHostOnlyNetwork(networkConfig, PRL_DEFAULT_HOSTONLY_INDEX);
	}else if(emulatedType == PNA_SHARED)
	{
		pVirtNetworking = PrlNet::GetSharedNetwork(networkConfig);
	}else if(emulatedType == PNA_BRIDGED_ETHERNET)
	{
		pVirtNetworking = PrlNet::GetBridgedNetwork(networkConfig);
	}

	return pVirtNetworking;
}


PRL_RESULT PrlNet::GetNetworkTypeForAdapter(CParallelsNetworkConfig *networkConfig, const CVmGenericNetworkAdapter *adapter,
				PRL_NET_VIRTUAL_NETWORK_TYPE *nVNetType)
{
	CVirtualNetwork *pVirtNetworking;

	if (!adapter)
		return PRL_ERR_FAILURE;

	pVirtNetworking = GetVirtualNetworkForAdapter(networkConfig,
												  adapter->getEmulatedType(),
												  adapter->getVirtualNetworkID());
	if (pVirtNetworking == NULL)
		return PRL_ERR_FAILURE;

	*nVNetType = pVirtNetworking->getNetworkType();

	return PRL_ERR_SUCCESS;
}


CVmGenericNetworkAdapter *PrlNet::GetAdapterByIndex(const QList<CVmGenericNetworkAdapter* > &m_lstNetworkAdapters,
							unsigned int adapterIndex)
{
	foreach(CVmGenericNetworkAdapter *adapter, m_lstNetworkAdapters)
	{
		if (!adapter)
			continue;
		if (adapterIndex == adapter->getIndex())
			return adapter;
	}

	return NULL;
}


CVirtualNetwork *PrlNet::GetNetworkByParallelsAdapter(
	CVirtualNetworks *pNetworks,
	int nAdapterIndex)
{
	if (pNetworks == NULL)
		return NULL;

	foreach(CVirtualNetwork *pNetwork, pNetworks->m_lstVirtualNetwork)
	{
		if ( pNetwork->getNetworkType() != PVN_HOST_ONLY)
			continue;

		CHostOnlyNetwork *pHostOnlyNetwork = pNetwork->getHostOnlyNetwork();
		if ( !pHostOnlyNetwork )
			continue;

		CParallelsAdapter *pParallelsAdapter = pHostOnlyNetwork->getParallelsAdapter();
		if (!pParallelsAdapter)
			continue;

		if (pParallelsAdapter->getPrlAdapterIndex() == nAdapterIndex)
			return pNetwork;
	}

	return NULL;
}

CVirtualNetwork *PrlNet::GetNetworkByBoundCardMacAndVLANTag(
	CVirtualNetworks *pNetworks, QString &qsMac, PRL_UINT16 nVLANTag)
{
	if (pNetworks == NULL)
		return NULL;

	foreach(CVirtualNetwork *pNetwork, pNetworks->m_lstVirtualNetwork)
	{
		if (pNetwork->getNetworkType() != PVN_BRIDGED_ETHERNET)
			continue;

		if (pNetwork->getBoundCardMac() == qsMac &&
			pNetwork->getVLANTag() == nVLANTag)
			return pNetwork;
	}

	return NULL;
}


// returns adapter for virtual network.
// This function should only be used from GetAdapterForVM() method.
PRL_RESULT PrlNet::GetAdapterForNetwork(
								PrlNet::EthAdaptersList &adaptersList,
								const CVirtualNetwork *pNetwork,
								PrlNet::EthAdaptersList::Iterator &itAdapter)
{
	itAdapter = adaptersList.end();
	PRL_ASSERT( pNetwork );
	if( !pNetwork )
		return PRL_NET_VMDEVICE_VIRTUAL_NETWORK_NO_ADAPTER;

	if (pNetwork->getNetworkType() == PVN_BRIDGED_ETHERNET)
	{
		if (pNetwork->isBridgedToDefaultAdapter())
		{
			PRL_RESULT prlResult = PrlNet::getDefaultBridgedAdapter(
					adaptersList, itAdapter);
			if (PRL_SUCCEEDED(prlResult))
				return prlResult;
			return PrlNet::getFirstAdapter(adaptersList, itAdapter);
		}

		unsigned char vmBoundCardMac[6];
		PrlNet::stringToEthAddress(pNetwork->getBoundCardMac(), vmBoundCardMac);

		for (PrlNet::EthAdaptersList::Iterator it = adaptersList.begin();
			it != adaptersList.end();
			++it )
		{
			if (0 == memcmp(it->_macAddr, vmBoundCardMac, sizeof(vmBoundCardMac))
				&& it->_vlanTag == pNetwork->getVLANTag())
			{
				itAdapter = it;
				return PRL_ERR_SUCCESS;
			}
		}

		WRITE_TRACE(DBG_FATAL,
			"Adapter for virtual network %s with MacAddress %s(VLAN=0x%04x) doesn't exist.",
			pNetwork->getNetworkID().toAscii().constData(),
			pNetwork->getBoundCardMac().toAscii().constData(),
			(unsigned int)pNetwork->getVLANTag() );

		return 	PRL_NET_VMDEVICE_VIRTUAL_NETWORK_NO_ADAPTER;
	}
	else
	{
		// Should search for parallels adapter with specified index.
		CHostOnlyNetwork *pHostOnlyParams = pNetwork->getHostOnlyNetwork();
		if (NULL == pHostOnlyParams)
		{
			WRITE_TRACE(DBG_FATAL, "Host only network params for virtual network %s are not configured.",
				pNetwork->getNetworkID().toAscii().constData());
			return PRL_NET_VMDEVICE_VIRTUAL_NETWORK_CONFIG_ERROR;
		}

		CParallelsAdapter *pAdapter = pHostOnlyParams->getParallelsAdapter();
		if (pAdapter == NULL)
		{
			WRITE_TRACE(DBG_FATAL, "Parallels Adapter for network %s are not configured.",
				pNetwork->getNetworkID().toAscii().constData());
			return PRL_NET_VMDEVICE_VIRTUAL_NETWORK_CONFIG_ERROR;
		}

		unsigned int nAdapterIndex = pAdapter->getPrlAdapterIndex();
		for (PrlNet::EthAdaptersList::Iterator it = adaptersList.begin();
			it != adaptersList.end();
			++it )
		{
			if (it->_bParallelsAdapter
				&& GET_PRL_ADAPTER_NUMBER(it->_adapterIndex) == GET_PRL_ADAPTER_NUMBER(nAdapterIndex) )
			{
				itAdapter = it;
				return PRL_ERR_SUCCESS;
			}
		}

		WRITE_TRACE(DBG_FATAL, "Parallels Adapter (%d) for virtual network %s doesn't exist or it is disabled.",
			(int)GET_PRL_ADAPTER_NUMBER(nAdapterIndex),
			pNetwork->getNetworkID().toAscii().constData() );

		return 	PRL_NET_VMDEVICE_VIRTUAL_NETWORK_NO_ADAPTER;
	}
}


// returns ethernet adapter for VMConfiguration.
PRL_RESULT PrlNet::GetAdapterForVM(
						   PrlNet::EthAdaptersList &adaptersList,
						   CParallelsNetworkConfig *pNetworkConfig,
						   const CVmGenericNetworkAdapter& vmAdapter,
						   PrlNet::EthAdaptersList::Iterator &itAdapter)
{
	UNUSED_PARAM(pNetworkConfig);
	UNUSED_PARAM(vmAdapter);
	itAdapter = adaptersList.end();

	QString networkID = vmAdapter.getVirtualNetworkID();
	if (!networkID.isEmpty() &&
			ParallelsDirs::getAppExecuteMode() == PAM_SERVER) {
		CVirtualNetwork *pNetwork = PrlNet::GetVirtualNetworkByID(
			pNetworkConfig->getVirtualNetworks(),
			networkID);
		if ( !pNetwork )
		{
			WRITE_TRACE(DBG_FATAL, "Virtual network %s is not configured.", networkID.toAscii().constData());
			return PRL_NET_VMDEVICE_VIRTUAL_NETWORK_NOT_EXIST;
		}

		if (!pNetwork->isEnabled())
		{
			WRITE_TRACE(DBG_FATAL, "Virtual network %s is disabled.", networkID.toAscii().constData());
			return PRL_NET_VMDEVICE_VIRTUAL_NETWORK_DISABLED;
		}

		return PrlNet::GetAdapterForNetwork(adaptersList, pNetwork, itAdapter);
	}

	// Old VM conviguration. Should find adapter using old way.
	PRL_NET_ADAPTER_EMULATED_TYPE networkingType = vmAdapter.getEmulatedType();
	int adapterIndex = (int)vmAdapter.getBoundAdapterIndex();
	QString adapterName = vmAdapter.getBoundAdapterName();

	if (networkingType == 3 )// custom
	{
		networkingType = PNA_BRIDGED_ETHERNET;
		adapterIndex |= PRL_ADAPTER_START_INDEX;
	}

	if( PNA_BRIDGED_ETHERNET == networkingType )
	{
		if (adapterIndex < 0) {
			PRL_RESULT res = PrlNet::getDefaultBridgedAdapter(
						adaptersList, itAdapter);
			if (PRL_SUCCEEDED(res))
				return res;
			return PrlNet::getFirstAdapter(adaptersList, itAdapter);
		}

		for(PrlNet::EthAdaptersList::Iterator it = adaptersList.begin();
			it != adaptersList.end();
			++it )
		{
			if( IS_PRL_ADAPTER_INDEX(adapterIndex) && it->_bParallelsAdapter)
			{
				if (GET_PRL_ADAPTER_NUMBER(adapterIndex)
					== GET_PRL_ADAPTER_NUMBER(it->_adapterIndex))
				{
					itAdapter = it;
					return PRL_ERR_SUCCESS;
				}
				continue;
			}

			if ( it->_name == adapterName )
			{
				itAdapter = it;
				return PRL_ERR_SUCCESS;
			}
		}

		return PRL_NET_ADAPTER_NOT_EXIST;
	}

	if( PNA_HOST_ONLY != networkingType && PNA_SHARED != networkingType )
	{
		WRITE_TRACE(DBG_FATAL, "[PrlNet::getAdapter] Error in parameters: wrong networking type specified: %d",
			(int)networkingType );
		return PRL_ERR_INCONSISTENCY_VM_CONFIG;
	}

	CVirtualNetwork *pNetwork = NULL;
	if (PNA_SHARED == networkingType)
	{
		pNetwork = PrlNet::GetSharedNetwork(pNetworkConfig);
		if (NULL == pNetwork)
		{
			WRITE_TRACE(DBG_FATAL, "Shared networking is not configured.");
		}
	}
	else
	{
		pNetwork = PrlNet::GetHostOnlyNetwork(pNetworkConfig, (UINT)adapterIndex);
		if (NULL == pNetwork)
		{
			WRITE_TRACE(DBG_FATAL, "Host only networking %d is not configured.", adapterIndex);
		}
	}

	if (NULL == pNetwork)
	{
		return PRL_NET_ERR_PRL_NO_BINDABLE_ADAPTER;
	}

	return PrlNet::GetAdapterForNetwork(adaptersList, pNetwork, itAdapter);
}



// Read networking configuration from config file.
static PRL_RESULT readNetworkConfig(CParallelsNetworkConfig &networkConfig, PRL_APPLICATION_MODE appMode )
{
	QString strConfigFile = (PAM_UNKNOWN == appMode)
		? ParallelsDirs::getNetworkConfigFilePath()
		: ParallelsDirs::getNetworkConfigFilePath( appMode );

	QFile f_in_cfg( strConfigFile );

	if( !f_in_cfg.exists() )
	{
		WRITE_TRACE(DBG_FATAL, "ReadNetworkConfig: failed to read networking configuration: file %s doesn't exist",
			QSTR2UTF8( strConfigFile ) );

		return PRL_ERR_FILE_NOT_FOUND;
	}

	PRL_RESULT prlResult = networkConfig.loadFromFile(strConfigFile);
	if (PRL_FAILED(prlResult))
	{
		QString strErrorMessage = QString("Failed to load network configuration file %1.").arg(strConfigFile);
		if (prlResult == PRL_ERR_PARSE_VM_CONFIG)
		{
			strErrorMessage += " ";
			strErrorMessage += networkConfig.GetErrorMessage();
		}

		WRITE_TRACE(DBG_FATAL, "(%#x : %s  ): %s",
			prlResult,
			PRL_RESULT_TO_STRING( prlResult ),
			QSTR2UTF8(strErrorMessage));
	}

	return prlResult;
}


static PRL_RESULT tryToRecoverNetworkConfig(CParallelsNetworkConfig &networkConfig)
{
	// #133154  compatibility issues:
	//		desktop: Load from network.xml and copy to network.desktop.xml
	//		workstation: Load from network.xml and copy to network.workstation.xml
	PRL_RESULT prlResult = PrlNet::ReadNetworkConfig(networkConfig);

	PRL_ASSERT( PRL_FAILED( prlResult ) );
	PRL_ASSERT( PRL_ERR_FILE_NOT_FOUND == prlResult );

	if( prlResult != PRL_ERR_FILE_NOT_FOUND )
		return prlResult;

	return PRL_ERR_FILE_NOT_FOUND;
}


PRL_RESULT PrlNet::ReadNetworkConfig( CParallelsNetworkConfig &networkConfig )
{
	return ::readNetworkConfig( networkConfig, PAM_UNKNOWN );
}


PRL_RESULT PrlNet::WriteNetworkConfig(CParallelsNetworkConfig &networkConfig)
{
	QString strConfigFile = ParallelsDirs::getNetworkConfigFilePath();
	QFile cfgFile(strConfigFile);

	PRL_RESULT rc = networkConfig.saveToFile( &cfgFile );
	if ( PRL_FAILED (rc) )
	{
		WRITE_TRACE(DBG_FATAL, "Can't write network configuration to file %s",
			QSTR2UTF8 ( strConfigFile ) );
		return rc;
	}

	return PRL_ERR_SUCCESS;
}


PRL_RESULT PrlNet::InitNetworkConfig(
	CParallelsNetworkConfig &networkConfig
	, bool &bConfigurationRestored)
{
	bConfigurationRestored = false;
	//////////////////////////////////////////////////////////////////////
	// load  config
	QString strConfigFile = ParallelsDirs::getNetworkConfigFilePath();

	PRL_RESULT prlResult = PrlNet::ReadNetworkConfig( networkConfig );
	if (prlResult == PRL_ERR_FILE_NOT_FOUND)
	{
		// copy existing network.xml to network.{exec_mode}.xml
		prlResult = tryToRecoverNetworkConfig(networkConfig);
		if( PRL_FAILED(prlResult) )
		{
			WRITE_TRACE(DBG_FATAL, "Unable to restore network config by error %s %#x"
				, PRL_RESULT_TO_STRING( prlResult )
				, prlResult);
		}
		else
		{
			bConfigurationRestored = true;
			return PRL_ERR_SUCCESS;
		}
	}

	if( PRL_FAILED(prlResult) )
	{
		if (prlResult != PRL_ERR_FILE_NOT_FOUND)
		{
			// Recover network configuration.
			QString strBackupFile = strConfigFile + QString( ".BACKUP." ) +
				QDateTime::currentDateTime().toString( "yyyy-MM-dd_hh-mm-ss" );

			QString strErrorMessage = QString("Current configuration will be stored to %1."
					" Network will start with default configuration.")
				.arg(strBackupFile);

			WRITE_TRACE(DBG_FATAL, "%s", QSTR2UTF8(strErrorMessage) );

			if( ! QFile::rename( strConfigFile, strBackupFile ) )
			{
				WRITE_TRACE(DBG_FATAL, "ERROR in NetworkConfig recovering:"
						" Can't rename '%s' ==> '%s'"
						, QSTR2UTF8( strConfigFile )
						, QSTR2UTF8( strBackupFile ) );

				return PRL_ERR_FAILURE;
			}
		} else
			WRITE_TRACE(DBG_FATAL, "Network config file does not exist."
					" Trying to create file ... [%s]", QSTR2UTF8( strConfigFile ) );

		PrlNet::FillDefaultConfiguration(&networkConfig);

		bConfigurationRestored = true;
	} else
		bConfigurationRestored = PrlNet::PatchConfig(&networkConfig);

	if (bConfigurationRestored)
	{
		prlResult = PrlNet::WriteNetworkConfig(networkConfig);
		if (PRL_FAILED(prlResult))
		{
			bConfigurationRestored = false;
			return PRL_ERR_FAILURE;
		}
	}

	return PRL_ERR_SUCCESS;
}

/************************************************************************/
// returns true if IP address is valid
static bool validateIp(UINT ipAddr)
{
	if( ipAddr == 0 )
		return false;

	while( ipAddr )
	{
		if( (ipAddr&0xff) == 0xff )
			return false;

		ipAddr >>= 8;
	}
	return true;
}

static bool validateNetMask(UINT netMask)
{
	// most left bit must be 1
	if( !(netMask&0x10000000) )
		return false;

	// mask must include at least 32 addresses
	if( 0 != (netMask&0x1f) )
		return false;

	// mask must be in form 1...10..0
	bool bNeedOne = false;
	while(netMask)
	{
		int bit = netMask & 1;
		if( bNeedOne && 0 == bit )
		{
			return false;
		}
		if( bit )
			bNeedOne = true;
		netMask >>= 1;
	}
	return true;
}


// Validate host only network params.
PRL_RESULT PrlNet::ValidateHostOnlyNetworkParams(CVirtualNetwork *pNetwork)
{
	CHostOnlyNetwork *pHostOnly = pNetwork->getHostOnlyNetwork();
	if ( !pHostOnly )
		return PRL_NET_VALID_FAILURE;

	quint32 scopeMask = pHostOnly->getIPNetMask().toIPv4Address();
	// 	Mask must be valid
	if( !validateNetMask(scopeMask) )
		return PRL_NET_VALID_WRONG_NET_MASK;

	quint32 hostIP = pHostOnly->getHostIPAddress().toIPv4Address();
	if (!validateIp(hostIP))
		return PRL_NET_VALID_WRONG_HOST_IP_ADDR;

	quint32 dhcpIP = pHostOnly->getDhcpIPAddress().toIPv4Address();
	if (!validateIp(dhcpIP))
		return PRL_NET_VALID_WRONG_DHCP_IP_ADDR;

	CDHCPServer *pDHCPServer = pHostOnly->getDHCPServer();
	if (pDHCPServer && pDHCPServer->isEnabled())
	{
		quint32 scopeStart = pDHCPServer->getIPScopeStart().toIPv4Address();
		quint32 scopeEnd = pDHCPServer->getIPScopeEnd().toIPv4Address();

		// IPAddress must not be zero and must not contain 0xff
		if( !validateIp(scopeStart) || !validateIp(scopeEnd) )
			return PRL_NET_VALID_DHCP_RANGE_WRONG_IP_ADDRS;

		// Mask must match
		if( (scopeEnd&scopeMask) != (scopeStart&scopeMask) )
			return PRL_NET_VALID_MISMATCH_DHCP_SCOPE_MASK;

		// Range must include at least 16 addresses
		int range = scopeEnd - scopeStart;
		if( range < 15 )
			return PRL_NET_VALID_DHCP_SCOPE_RANGE_LESS_MIN;
	}

	return PRL_ERR_SUCCESS;
}


QString PrlNet::MakeVirtualNetworkID(const QString &adapterName, const QString &adapterSystemName )
{
	UNUSED_PARAM(adapterSystemName);
	if (adapterName.isEmpty())
	{
		return "Default";
	}
	else
	{
#if defined(_LIN_)
		QString rv = adapterName;
#elif defined(_WIN_)
		QString rv = adapterName;
#else
		QString rv = QString("%1 (%2)").arg(adapterName).arg(adapterSystemName);
#endif
		return rv;
	}
}


QString PrlNet::MakeVirtualNetworkDescription(const QString &adapterName, const QString &adapterSystemName )
{
	return MakeVirtualNetworkID(adapterName, adapterSystemName);
}


CVirtualNetwork *PrlNet::CreateVirtualNetworkForAdapter(
	const QString &adapterName,
	const QString &adapterSystemName,
	const QString &adapterMacAddress,
	unsigned short vlanTag)
{
	CVirtualNetwork *pNetwork = new CVirtualNetwork;
	pNetwork->setNetworkID( MakeVirtualNetworkID(adapterName, adapterSystemName) );
	pNetwork->setDescription( MakeVirtualNetworkDescription(adapterName, adapterSystemName) );
	pNetwork->setNetworkType( PVN_BRIDGED_ETHERNET );
	pNetwork->setVLANTag(vlanTag);

	if (adapterName.isEmpty() || adapterMacAddress.isEmpty() )
	{
		pNetwork->setBridgedToDefaultAdapter();
	}
	else
	{
		pNetwork->setBoundCardMac(adapterMacAddress);
	}

	return pNetwork;
}


UINT16 PrlNet::GetOffmgmtPortByService(const CParallelsNetworkConfig *pNetworkConfig, const QString &srvName)
{
	COffmgmtServices *services = pNetworkConfig->getOffmgmtServices();
	if (!services) {
		PRL_ASSERT(services);
		return 0;
	}
	foreach (COffmgmtService *srv, services->m_lstOffmgmtService) {
		if (srv->getName() == srvName)
			return srv->getPort();
	}
	return 0;
}


QString PrlNet::GetOffmgmtServiceByPort(const CParallelsNetworkConfig *pNetworkConfig, UINT16 port)
{
	COffmgmtServices *services = pNetworkConfig->getOffmgmtServices();
	if (!services) {
		PRL_ASSERT(services);
		return "";
	}

	foreach (COffmgmtService *srv, services->m_lstOffmgmtService) {
		if (srv->getPort() == port)
			return srv->getName();
	}
	return "";
}

static QAbstractSocket::NetworkLayerProtocol
ipmask_to_ip(const QString &ip_mask, QHostAddress &addr)
{
	QString ip;
	if (!NetworkUtils::ParseIpMask(ip_mask, ip))
		return QAbstractSocket::UnknownNetworkLayerProtocol;

	addr = QHostAddress(ip);
	return addr.protocol();
}

PRL_RESULT PrlNet::CreateIpsBuffer(
	const QList<QString> &ipList, UINT32 **ppBuffer, unsigned *pcnt)
{
	*pcnt = 0;
	*ppBuffer = NULL;

	if (ipList.size() == 0)
		return 0;

	unsigned ip4_num = 0;
	unsigned *ips_buf = (unsigned*)::malloc(ipList.size()*sizeof(unsigned));
	if (NULL == ips_buf)
		return PRL_ERR_OUT_OF_MEMORY;

	foreach (QString ip_mask, ipList) {
		QHostAddress addr;
		switch (ipmask_to_ip(ip_mask, addr)) {
			case  QAbstractSocket::IPv4Protocol:
				ips_buf[ip4_num++] = addr.toIPv4Address();
				break;
			case QAbstractSocket::IPv6Protocol:
				break;
			default:
				WRITE_TRACE(DBG_FATAL,
						"[VMNET] Wrong-formatted address "
						"in the list of allowed Sources IPs: %s",
						QSTR2UTF8(ip_mask));
				continue;
		}
	}
	if (ip4_num == 0) {
		::free(ips_buf);
		ips_buf = NULL;
	}
	*ppBuffer = ips_buf;
	*pcnt = ip4_num;
	return 0;
}


PRL_RESULT PrlNet::CreateIp6sBuffer(
	const QList<QString> &ipList, UINT8 **ppBuffer, unsigned *pcnt)
{
	*pcnt = 0;
	*ppBuffer = NULL;

	if (ipList.size() == 0)
		return 0;

#define IP6_ADDR_SIZE	16

	unsigned ip_num = 0;
	UINT8 *ips_buf = (UINT8 *)::malloc(ipList.size() * IP6_ADDR_SIZE);
	if (NULL == ips_buf)
		return PRL_ERR_OUT_OF_MEMORY;

	foreach (QString ip_mask, ipList) {
		QHostAddress addr;
		switch (ipmask_to_ip(ip_mask, addr)) {
			case QAbstractSocket::IPv4Protocol:
				break;
			case QAbstractSocket::IPv6Protocol: {
				Q_IPV6ADDR ip6 = addr.toIPv6Address();
				memcpy(ips_buf + IP6_ADDR_SIZE*ip_num, ip6.c, IP6_ADDR_SIZE);
				ip_num++;
				break;
			}
			default:
				WRITE_TRACE(DBG_FATAL,
						"[VMNET] Wrong-formatted address "
						"in the list of allowed Sources IPs: %s",
						QSTR2UTF8(ip_mask));
				continue;
		}
	}
	if (ip_num == 0) {
		::free(ips_buf);
		ips_buf = NULL;
	}
	*ppBuffer = ips_buf;
	*pcnt = ip_num;
#undef IP6_ADDR_SIZE
	return 0;
}


static bool s_is_ipv6_enabled = true;


void PrlNet::setIPv6Enabled(bool enabled)
{
	s_is_ipv6_enabled = enabled;
}


bool PrlNet::isIPv6Enabled()
{
	return s_is_ipv6_enabled;
}


bool PrlNet::isSharedEnabled()
{
#if defined(_LIN_)
	if (ParallelsDirs::getAppExecuteMode() == PAM_SERVER)
		return false;
#endif
	return true;
}


static bool s_is_tap_enabled = false;


void PrlNet::setTapEnabled(bool enabled)
{
	s_is_tap_enabled = enabled;
}


bool PrlNet::isTapEnabled()
{
#if !defined(EXTERNALLY_AVAILABLE_BUILD)
	return s_is_tap_enabled;
#else
	return false;
#endif
}


void PrlNet::InitConfigLibrary(const CParallelsNetworkConfig *pNetworkConfig)
{
	PrlNet::setIPv6Enabled(pNetworkConfig->isIPv6Enabled());
}

