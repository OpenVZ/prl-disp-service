///////////////////////////////////////////////////////////////////////////////
///
/// @file PrlNetLibrary.cpp
/// @author sdmitry
///
/// Functions implementations for Unix OS
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

#include "prlnet_common.h"

#if defined(_LIN_)
#include "Libraries/Virtuozzo/CVzHelper.h"
#endif

#include <linux/sockios.h>

using namespace PrlNet_Private;

#define PARALLELS_NAT_IS_RUNNING "ps -A 2>&1 | grep prl_naptd >/dev/null"

static unsigned int s_LastSystemError;
void PrlNet_Private::MODULE_STORE_SYSTEM_ERROR()
{
	s_LastSystemError = (unsigned int)errno;
}


int	PrlNet::getMaximumAdapterIndex()
{
	return PARALLELS_MAXIMUM_ADAPTER_INDEX;
}


PRL_RESULT PrlNet_Private::findPrlAdapter( int adapterIndex, PrlNet::EthernetAdapter &adapter )
{
	bool bSuccess = false;
	PrlNet::EthAdaptersList ethList;
	makePrlAdaptersList(ethList);
	for( PrlNet::EthAdaptersList::iterator it = ethList.begin(); it != ethList.end(); ++it )
	{
		if( it->_adapterIndex == (adapterIndex|PRL_ADAPTER_START_INDEX) )
		{
			adapter = *it;
			bSuccess = true;
			break;
		}
	}

	return bSuccess ? PRL_ERR_SUCCESS : PRL_ERR_OPERATION_FAILED;
}


//
// Other functions
//

QString PrlNet::findAdapterName(const QString& mac_, unsigned short vlan_)
{
	EthIfaceList l;
	if(!::makeBindableEthIfacesList(l, false))
		return QString();

	foreach(const EthIface& e, l)
	{
		if (ethAddressToString(e._macAddr) == mac_ && e._vlanTag == vlan_)
			return QString(e._name);
	}

	return QString();
}

PRL_RESULT PrlNet::makePrlAdaptersList( PrlNet::EthAdaptersList &adaptersList )
{
	adaptersList.clear();

	// create a list of all Parallels adapter. No matter up or down
	EthIfaceList ethList;
	if( !::makeBindableEthIfacesList( ethList, false ) )
	{
		MODULE_STORE_SYSTEM_ERROR();

		WRITE_TRACE(DBG_FATAL, "[PrlNet]  makeEthIfacesList returned error: %d",
			s_LastSystemError );
		return PRL_NET_ETHLIST_CREATE_ERROR;
	}

	for( EthIfaceList::iterator it = ethList.begin();
		it != ethList.end();
		++it )
	{
		// consider only PVSNIC adapters
		if( it->_nAdapter < 0 || !IS_PRL_ADAPTER_INDEX(it->_nAdapter) )
			continue;

		PrlNet::EthernetAdapter ethAdapter;
		ethAdapter._name = it->_name;
		ethAdapter._systemName = it->_name;
		ethAdapter._adapterIndex = it->_nAdapter;
		ethAdapter._bEnabled = (it->_ifaceFlags&IFF_UP) ? true : false;
		ethAdapter._bParallelsAdapter = true;
		ethAdapter._vlanTag = PRL_INVALID_VLAN_TAG;
		memcpy( ethAdapter._macAddr, it->_macAddr, 6 );

#if defined(_MAC_)
		Mac_GetAdapterName( it->_name, ethAdapter._name );
#endif

		adaptersList.push_back(ethAdapter);
	}

	qSort(adaptersList); // sort adapters

	return PRL_ERR_SUCCESS;
}


// set set_flags and clear clear_flags for interface with name if_name
bool PrlNet::setIfFlags( const char *if_name, int set_flags, int clear_flags )
{
	int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		return false;
	}

	struct ifreq ifr;
	memset( &ifr, 0, sizeof(ifr) );
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);

	if( ::ioctl(sock, SIOCGIFFLAGS, &ifr) < 0 )
	{
		::close(sock);
		return false;
	}

	int flags = ifr.ifr_flags;

	flags |= set_flags;
	flags &= ~clear_flags;

	ifr.ifr_flags = flags;
	if (::ioctl(sock, SIOCSIFFLAGS, &ifr) < 0 )
	{
		::close(sock);
		return false;
	}

	::close(sock);

	return true;
}


PRL_RESULT PrlNet::enablePrlAdapter( int adapterIndex, bool bEnable )
{
	assert( adapterIndex>=0 );

	adapterIndex |= PRL_ADAPTER_START_INDEX;

	// find apdater with required index
	EthIfaceList ethList;
	if( !::makeEthIfacesList( ethList, false ) ) //all interfaces, not only UP
	{
		MODULE_STORE_SYSTEM_ERROR();

		WRITE_TRACE(DBG_FATAL, "[PrlNet]  makeEthIfacesList returned error: %d",
			s_LastSystemError );
		return PRL_NET_ETHLIST_CREATE_ERROR;
	}

	EthIfaceList::iterator adapter = ethList.end();
	for( EthIfaceList::iterator it = ethList.begin();
		it != ethList.end();
		++it )
	{
		if( it->_nAdapter == adapterIndex )
		{
			adapter = it;
			break;
		}
	}

	if( adapter == ethList.end() )
	{
		WRITE_TRACE(DBG_FATAL, "[PrlNet] Error enabling adapter: Adapter %d is not installed",
			GET_PRL_ADAPTER_NUMBER(adapterIndex) );
		return PRL_NET_ADAPTER_NOT_EXIST;
	}

	int set_flags = bEnable ? IFF_UP : 0;
	int clear_flags = bEnable ? 0 : IFF_UP;

	if( !PrlNet::setIfFlags(adapter->_name.toAscii(), set_flags, clear_flags) )
	{
		MODULE_STORE_SYSTEM_ERROR();

		WRITE_TRACE(DBG_FATAL, "[PrlNet] Error enabling adapter: setIfFlags(): %d",
			s_LastSystemError );
		return PRL_NET_SYSTEM_ERROR;
	}

	return PRL_ERR_SUCCESS;
}


unsigned long PrlNet::getSysError()
{
	return (unsigned long)s_LastSystemError;
}


void PrlNet::setSysError(unsigned long err)
{
	s_LastSystemError = err;
}


QString PrlNet::getSysErrorText()
{
	char s[256];
#ifdef _LIN_
	//Use GNU specific strerror_r system call
	char *sErrString = strerror_r(s_LastSystemError, s, 256);
	return UTF8_2QSTR(sErrString);
#else
	strerror_r(s_LastSystemError, s, 256);
	return UTF8_2QSTR(s);
#endif
}


PRL_RESULT PrlNet::installPrlService( const QString &parallelsDir )
{
	UNUSED_PARAM(parallelsDir);
	return PRL_ERR_SUCCESS; // do nothing for MAC and linux
}


PRL_RESULT PrlNet::uninstallPrlService( )
{
	return PRL_ERR_SUCCESS; // do nothing for MAC and linux
}


void	PrlNet::getDefaultDhcpParams(
	int 	adapterIndex,
	quint32 &dhcpScopeStartIp,
	quint32 &dhcpScopeEndIp,
	quint32 &dhcpScopeMask )
{
	PrlNet::GetDefaultDhcpParams(
		GET_PRL_ADAPTER_NUMBER(adapterIndex),
		dhcpScopeStartIp,
		dhcpScopeEndIp,
		dhcpScopeMask );
}


PRL_RESULT PrlNet::stopNetworking(const QString &parallelsDir)
{
	Q_UNUSED(parallelsDir);
#if !defined(EXTERNALLY_AVAILABLE_BUILD)
	// read system-flags
	CParallelsNetworkConfig networkConfig;
	PRL_RESULT prlResult = PrlNet::ReadNetworkConfig( networkConfig );
	if (PRL_FAILED(prlResult))
		return PRL_ERR_SUCCESS;

	PrlNet::InitConfigLibrary(&networkConfig);

	if (PrlNet::isTapEnabled()) {
		int r = ::system("/vz/tapnet_stop_net.sh");
		WRITE_TRACE(DBG_FATAL, "/vz/tapnet_start_net.sh finished with %d", r);
	}
#endif
	return PRL_ERR_SUCCESS;
}

#if 0
PRL_RESULT PrlNet::VZSyncConfig(CParallelsNetworkConfig *pConfig, bool *pbConfigChanged)
{
	*pbConfigChanged = false;

#if !defined(_LIN_)
	UNUSED_PARAM(pConfig);
	return PRL_ERR_SUCCESS;
#else
	if (!CVzHelper::is_vz_running())
		return PRL_ERR_SUCCESS;
	/* need to restore default networks for Containers */
	CVzVirtualNetwork VzVNet;
	CVirtualNetworks *pNetworks = pConfig->getVirtualNetworks();
	foreach (CVirtualNetwork *pNetwork, pNetworks->m_lstVirtualNetwork)
	{
		if (!pNetwork->isEnabled())
			continue;
		// ToDo: we this network may already exist in VZ with different adapter.
		// we should check this condition and reconfigure this net.
		PRL_RESULT prlResult = VzVNet.AddVirtualNetwork(pNetwork);
		if (PRL_FAILED(prlResult))
		{
			/* already exists - ignore error */
			if (prlResult == PRL_NET_DUPLICATE_VIRTUAL_NETWORK_ID)
				continue;

			WRITE_TRACE(DBG_FATAL, "Failed to create"
						" Container's network %s, error 0x%08x."
						" Network will be deleted.",
						pNetwork->getNetworkID().toAscii().constData(),
						prlResult);
			/* VMs and CTs networks should be synced */
			PrlNet::DeleteVirtualNetwork(pConfig, pNetwork);
			*pbConfigChanged = true;
		}
	}
	return PRL_ERR_SUCCESS;
#endif
}
#endif

#if !defined(_WIN_) && !defined(EXTERNALLY_AVAILABLE_BUILD)
static void startTapNetworking(const CParallelsNetworkConfig &networkConfig)
{
	(void)networkConfig;
	int r = ::system("/vz/tapnet_start_net.sh");
	WRITE_TRACE(DBG_FATAL, "/vz/tapnet_start_net.sh finished with %d", r);
}
#endif

/// Starts networking adapters configured in Dispatcher.xml
PRL_RESULT PrlNet::startNetworking( const QString &parallelsDir, const QString &prlDriversDir )
{
	//
	// Load and process configuration.
	//
	Q_UNUSED(prlDriversDir);
	Q_UNUSED(parallelsDir);
	CParallelsNetworkConfig networkConfig;
	bool bConfigurationRestored = false;
	PRL_RESULT prlResult = InitNetworkConfig(networkConfig, bConfigurationRestored);
	if (!PRL_SUCCEEDED(prlResult))
	{
		//
		// Note: we should not interrupt networking installation process due to configuration error.
		// We should start drivers and napt anyway. So, just make a notice in syslog.
		//
		syslog(LOG_ERR, "Failed to read Parallels networking configuration."
			" See Parallels.log for description. Status %s(%x)\n",
			PRL_RESULT_TO_STRING ( prlResult ), (unsigned)prlResult );

		WRITE_TRACE(DBG_FATAL, "Failed to read Parallels networking configuration."
			" See Parallels.log for description. Status %s(%x)",
			PRL_RESULT_TO_STRING ( prlResult ), (unsigned)prlResult );

		networkConfig.setDefaults(); // Note: this will make empty config.
	}

	PrlNet::InitConfigLibrary(&networkConfig);

	bool bNetConfigChanged = false;
	if (PrlNet::getMode() == PRL_NET_MODE_VME)
	{
#if 0
		// vme uses vz networks info on setup
		if (bConfigurationRestored)
			PrlNet::VZSyncConfig(&networkConfig, &bNetConfigChanged);
		startVmeNetworking(networkConfig);
#endif
	}
#if !defined(_WIN_) && !defined(EXTERNALLY_AVAILABLE_BUILD)
	else if (PrlNet::isTapEnabled()) {
		startTapNetworking(networkConfig);
	}
#endif
	if (bNetConfigChanged)
	{
		prlResult = PrlNet::WriteNetworkConfig(networkConfig);
		if (PRL_FAILED(prlResult))
		{
			WRITE_TRACE(DBG_FATAL, "Failed to write updated network"
				" config, error 0x%08x.", prlResult);
			return prlResult;
		}
	}

	return PRL_ERR_SUCCESS;
}


// returns names of the Parallels NATD
void PrlNet_Private::getPrlNatdNames( const QString &parallelsDir, QString &cmd, QString &arg0 )
{
	arg0 = "prl_naptd";
	cmd = "\"" + parallelsDir + "/" + arg0 + "\"";
}


QString PrlNet::getBridgeName(const QString& iface)
{
	QString brPath = QFile::symLinkTarget
		(QString("/sys/class/net/%1/brport/bridge").arg(iface));

	return QFileInfo(brPath).fileName();
}

namespace
{

bool setupBridge(const QString& bridge, const QString& iface, int command)
{
	struct ifreq ifr;

	int ifindex = if_nametoindex(iface.toAscii().data());
	if (ifindex == 0)
		return false;

	int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		return false;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, bridge.toAscii().data(), sizeof(ifr.ifr_name) - 1);
	ifr.ifr_ifindex = ifindex;
	if (::ioctl(sock, command, &ifr) < 0)
	{
		::close(sock);
		return false;
	}

	::close(sock);
	return true;
}

} // namespace

// remove interface from its bridge
bool PrlNet::releaseInterface(const QString& iface)
{
	QString br = getBridgeName(iface);

	if (br.isEmpty())
		return true;

	return setupBridge(br, iface, SIOCBRDELIF);
}

bool PrlNet::connectInterface(const QString& iface, const QString& bridge)
{
	QString br = getBridgeName(iface);

	if (br == bridge)
		return true;

	return setupBridge(bridge, iface, SIOCBRADDIF);
}
