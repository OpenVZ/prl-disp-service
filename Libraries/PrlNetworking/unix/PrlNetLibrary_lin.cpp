///////////////////////////////////////////////////////////////////////////////
///
/// @file PrlNetLibrary.cpp
/// @author sdmitry
///
/// Functions implementations for Unix OS
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
///////////////////////////////////////////////////////////////////////////////

#include "prlnet_common.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <net/route.h>
#include "unix/libarp.h"
#include <Libraries/PrlCommonUtilsBase/NetworkUtils.h>
#include <Libraries/Std/BitOps.h>

#include <cassert>
#include <iostream>
#include <fstream>

#ifndef in6_ifreq
struct in6_ifreq {
	struct in6_addr ifr6_addr;
	int           ifr6_prefixlen;
	int           ifr6_ifindex;
};
#endif

using namespace PrlNet_Private;

#define PARALLELS_MOD_UNLOAD_CMD		"/sbin/rmmod prl_vnic"
#define PARALLELS_MOD_PVSNET_UNLOAD_CMD "/sbin/rmmod prl_netbridge"
#define PARALLELS_MOD_IS_PRLVNIC_LOADED_CMD	"/sbin/lsmod 2>&1 | grep prl_vnic >/dev/null"
#define PARALLELS_MOD_IS_PVSNET_LOADED_CMD "/sbin/lsmod 2>&1 | grep prl_netbridge >/dev/null"
#define PARALLELS_MOD_LOAD_CMD			"/sbin/insmod"
#define PARALLELS_MOD_KEXT_NAME			"prl_vnic.ko"
#define PARALLELS_DHCP30_STARTED		"ps -A 2>&1 | grep prl_dhcpd >/dev/null"
#define PARALLELS_KILL_DHCP30			"killall -9 prl_dhcpd"

#define PARALLELS_MOD_KEXT_PVSNET_NAME	"prl_netbridge.*"

#define PARALLELS_NAT_DAEMON				"prl_naptd"

// Don't accept ipv6 router advertisements on vnic
PRL_RESULT PrlNet::DisablePrlIPv6RouterDiscovery(int adapterIndex)
{
	char name[64];
	int n = snprintf(name, sizeof(name),
				"/proc/sys/net/ipv6/conf/vnic%d/accept_ra",
				adapterIndex);
	if (n >= (int)sizeof(name) || n < 0) {
		assert(0);
		return PRL_ERR_FAILURE;
	}

	int fd = open(name, O_WRONLY);
	if (fd < 0) {
		MODULE_STORE_SYSTEM_ERROR();

		WRITE_TRACE(DBG_FATAL, "vnic_disable_ra: failed to open %s: %ld", name,
			PrlNet::getSysError());
		return PRL_NET_SYSTEM_ERROR;
	}

	PRL_RESULT prlResult = PRL_ERR_SUCCESS;
	ssize_t bw = write(fd, "0\n", 2);
	if (bw != 2) {
		MODULE_STORE_SYSTEM_ERROR();
		WRITE_TRACE(DBG_FATAL, "vnic_disable_ra: failed to write to %s: %ld", name,
			PrlNet::getSysError());
		prlResult = PRL_NET_SYSTEM_ERROR;
	}

	close(fd);
	return prlResult;
}


bool PrlNet_Private::configurePrlAdapter(
	const QString &prlDriversDir,
	int adapterIndex, bool bHiddenAdapter, const QString &adapterName)
{
	UNUSED_PARAM(prlDriversDir);
	UNUSED_PARAM(adapterName);
	UNUSED_PARAM(bHiddenAdapter);

	PrlNet::enablePrlAdapter(adapterIndex, true);

	return true;
}


void PrlNet_Private::unconfigurePrlAdapter(int adapterIndex)
{
	UNUSED_PARAM(adapterIndex);
}


//
// Install/Uninstall adapters section
//

// Loads prl_netbridge module
int PrlNet::loadPrlNetbridgeKext( const QString &qPrlDriversDir )
{
	std::string prlDriversDir = qPrlDriversDir.toStdString();

	// stop prl_dhcpd
	if( Prl::ExecuteProcess(PARALLELS_DHCP30_STARTED) == 0 )
	{
	    Prl::ExecuteProcess(PARALLELS_KILL_DHCP30);
	    ::sleep(1);
	}
	// unload 4.0 driver
	Prl::ExecuteProcess("/sbin/rmmod prl_netbridge > /dev/null 2>&1");
	// unload 3.0 driver
	// Prl::ExecuteProcess("/sbin/rmmod vm-bridge > /dev/null 2>&1");
	// create a /dev entry
	Prl::ExecuteProcess("mknod /dev/prl_netbridge c 4 0");

	// load driver and return result
	std::string cmd = PARALLELS_MOD_LOAD_CMD;
	cmd += " ";
	cmd += prlDriversDir;
	cmd += "/";
	cmd += PARALLELS_MOD_KEXT_PVSNET_NAME;

	return Prl::ExecuteProcess( cmd.c_str() );
}

// loads prl_vnic
int PrlNet::loadPrlAdaptersKext( const QString &qPrlDriversDir )
{
	std::string prlDriversDir = qPrlDriversDir.toStdString();

	// form load string
	std::string cmd = PARALLELS_MOD_LOAD_CMD;
	cmd += " ";
	cmd += prlDriversDir;
	cmd += "/";
	cmd += PARALLELS_MOD_KEXT_NAME;

	return Prl::ExecuteProcess(cmd.c_str());
}


bool PrlNet::isWIFIAdapter(const EthernetAdapter& ethAdapter)
{
	// always false for Linux.
	UNUSED_PARAM(ethAdapter);

	return false;
}


bool PrlNet::isVirtualAdapter(const EthernetAdapter& ethAdapter)
{
	if (IS_VME_ADAPTER_INDEX(ethAdapter._adapterIndex))
		return true;

	if (ethAdapter._systemName.startsWith("vme") ||
		ethAdapter._systemName.startsWith("veth"))
		return true;
	return false;
}


PRL_RESULT PrlNet::renamePrlAdapter(
	const QString &prlDriversDir,
	int adapterIndex,
	bool bHiddenAdapter,
	const QString &newName )
{
	UNUSED_PARAM(prlDriversDir);
	UNUSED_PARAM(adapterIndex);
	UNUSED_PARAM(bHiddenAdapter);
	UNUSED_PARAM(newName);
	return PRL_ERR_SUCCESS;
}


PRL_RESULT PrlNet::startPrlNetService( const QString &parallelsDir, PrlNet::SrvAction::Action action )
{
	QString cmd;
	QString arg0;
	PrlNet_Private::getPrlNatdNames(parallelsDir, cmd, arg0);

	if( action == PrlNet::SrvAction::Start )
	{
		//always start with readconf to reread configuration if daemon is already started
		return PrlNet_Private::execDaemon( cmd, arg0, "readconf" );
	}
	else if( action == PrlNet::SrvAction::Stop )
	{
		PRL_RESULT prlResult = execDaemon( cmd, arg0, "stop" );
		if( PRL_SUCCEEDED(prlResult) )
		{
			int i = 0;
			for( i = 0; i<40; i++ )
			{
				if( !PrlNet_Private::isNatdRunning() )
					break;
				::usleep( 100*1000 ); // sleep for 100 msecs
			}

			// it is strange, if no iteration was done..
			if( i == 0 )
				::usleep( 500*1000 ); // sleep for 500 msecs
		}
		return prlResult;
	}
	else
	{
		WRITE_TRACE(DBG_FATAL, "[startPrlNetService] Unknown control code %d", (int)action );
		return PRL_ERR_FAILURE;
	}
}


static int open_ctrl_sock(int af)
{
	int skfd = socket(af, SOCK_DGRAM, 0);
	if (skfd < 0)
	{
		MODULE_STORE_SYSTEM_ERROR();
		WRITE_TRACE(DBG_FATAL, "[PrlNet] Failed to open control socket: err %d", errno);
		return -1;
	}
	return skfd;
}


PRL_RESULT PrlNet::notifyPrlNetService(const QString &parallelsDir)
{
	QString cmd;
	QString arg0;
	getPrlNatdNames( parallelsDir, cmd, arg0 );
	return execDaemon( cmd, arg0, "readconf" );
}


static PRL_RESULT setIPv4Address(const char *ifname, quint32 ipAddress,
	quint32 netMask)
{
	int skfd = open_ctrl_sock(AF_INET);
	if (skfd < 0)
		return PRL_NET_SYSTEM_ERROR;

	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifreq));
	strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	ifr.ifr_name[IFNAMSIZ-1] = '\0';

	//
	// Set up IP address
	//
	sockaddr_in* pInet = (sockaddr_in*)&ifr.ifr_addr;
	pInet->sin_addr.s_addr = htonl(ipAddress);
	pInet->sin_family = AF_INET; // 2
	if( ioctl(skfd, SIOCSIFADDR, &ifr) < 0 )
	{
		MODULE_STORE_SYSTEM_ERROR();

		WRITE_TRACE(DBG_FATAL, "[PrlNet] Failed to set up adapter"
			" configuration: SIOCSIFADDR for %s failed.", ifname);
		::close(skfd);
		return PRL_NET_SYSTEM_ERROR;
	}

	//
	// Set ip netmask
	//
	pInet = (sockaddr_in*)&ifr.ifr_netmask;
	pInet->sin_addr.s_addr = htonl(netMask);
	pInet->sin_family = AF_INET; // 2
	if( ioctl(skfd, SIOCSIFNETMASK, &ifr) < 0 )
	{
		MODULE_STORE_SYSTEM_ERROR();
		WRITE_TRACE(DBG_FATAL, "[PrlNet] Failed to set up adapter"
			" configuration: SIOCSIFNETMASK for %s failed.",
			ifname);
		::close(skfd);
		return PRL_NET_SYSTEM_ERROR;
	}

	::close(skfd);

	return PRL_ERR_SUCCESS;
}

static PRL_RESULT setIPv6Address(const char *ifname,
	const Q_IPV6ADDR & ipAddress, const Q_IPV6ADDR & netMask)
{
	int skfd = open_ctrl_sock(AF_INET6);
	if (skfd < 0)
		return PRL_NET_SYSTEM_ERROR;

	struct in6_ifreq ifr6;
	memset(&ifr6, 0, sizeof(ifr6));
	ifr6.ifr6_ifindex = if_nametoindex(ifname);

	memcpy(&ifr6.ifr6_addr, &ipAddress, sizeof(Q_IPV6ADDR));
	ifr6.ifr6_prefixlen = PrlNet::getIPv6PrefixFromMask(&netMask);
	// ignore EEXIST cause it fails only when this addr set up on
	// this adapter. If the addr set up on another adpater, no error
	// returned.
	if ((ioctl(skfd, SIOCSIFADDR, &ifr6) < 0) &&
		(errno != EEXIST))
	{
		MODULE_STORE_SYSTEM_ERROR();
		WRITE_TRACE(DBG_FATAL, "[PrlNet] Failed to set up adapter"
			" configuration: SIOCSIFADDR for %s failed, error = %d",
			ifname, errno);
		::close(skfd);
		return PRL_NET_SYSTEM_ERROR;
	}

	::close(skfd);

	return PRL_ERR_SUCCESS;
}

static PRL_RESULT setAdapterIpAddress(const QString& adapterName,
	const QHostAddress & ipAddress, const QHostAddress & netMask )
{
	if (ipAddress.protocol() == QAbstractSocket::IPv6Protocol)
		return setIPv6Address(adapterName.toAscii(),
			ipAddress.toIPv6Address(), netMask.toIPv6Address());

	return setIPv4Address(adapterName.toAscii(), ipAddress.toIPv4Address(),
			netMask.toIPv4Address());
}

PRL_RESULT PrlNet::setAdapterIpAddress(const QString& adapterName,
								const QString & ipAddressMask)
{
	QHostAddress ip, mask;
	QString ipstr;
	if (!NetworkUtils::ParseIpMask(ipAddressMask, ipstr))
	{
		WRITE_TRACE(DBG_FATAL, "Failed to parse ipaddress/mask '%s'", QSTR2UTF8( ipAddressMask ) );
		return PRL_ERR_NO_DATA;
	}

	QPair<QHostAddress, int> pair = QHostAddress::parseSubnet(ipAddressMask);
	if (pair.second == -1)
	{
		WRITE_TRACE(DBG_FATAL, "Failed to parse ipaddress/mask '%s'", QSTR2UTF8( ipAddressMask ) );
		return PRL_ERR_NO_DATA;
	}

	//get ip
	ip = QHostAddress(ipstr);

	//calculate mask
	if (ip.protocol() == QAbstractSocket::IPv4Protocol)
	{
		quint32 u_mask = 0;
		for (int i = 0; i < pair.second; i++)
			BMAP_SET((PRL_UINT8_PTR) &u_mask, (sizeof(u_mask)*8)-1 - i);
			//u_mask = u_mask | (1 << (31-i));
		mask.setAddress(u_mask);
        }
	else
	{
		Q_IPV6ADDR u_mask;
		memset(&u_mask, 0, sizeof(u_mask));

		for (int i = 0; i < pair.second; i++)
		{
			//quint8 *seg = &(u_mask.c[i/8]);
			//*seg = *seg | (1 << (7-i%8));
			BMAP_SET((PRL_UINT8_PTR) u_mask.c, (sizeof(u_mask)*8)-1 - i);
		}
		mask.setAddress(u_mask);
	}

	PRL_RESULT res = setAdapterIpAddress(adapterName, ip, mask);

	WRITE_TRACE(DBG_DEBUG, "Add ip address '%s' mask='%s' to '%s' rc=%d",
						QSTR2UTF8( ipAddressMask), QSTR2UTF8(mask.toString()),
						QSTR2UTF8( adapterName ), res );

	return res;
}


PRL_RESULT PrlNet::setAdapterIpAddresses(const QString& sAdapter,
		const CHostOnlyNetwork *pNetwork)
{
	PRL_RESULT prlResult = setAdapterIpAddress(sAdapter,
			pNetwork->getHostIPAddress(), pNetwork->getIPNetMask());
	if (PRL_FAILED(prlResult))
		WRITE_TRACE(DBG_FATAL, "[PrlNet] Failed to set up IPv4 address"
			" for Adapter '%s': error 0x%08x",
			QSTR2UTF8(sAdapter), prlResult);

	if (PrlNet::isIPv6Enabled())
	{
		prlResult |= setAdapterIpAddress(sAdapter,
			pNetwork->getHostIP6Address(), pNetwork->getIP6NetMask());
		if (PRL_FAILED(prlResult))
			WRITE_TRACE(DBG_FATAL, "[PrlNet] Failed to set up IPv6"
				" address for Adapter '%s': error 0x%08x",
			QSTR2UTF8(sAdapter), prlResult);
	}

	return prlResult;
}

PRL_RESULT PrlNet::setPrlAdapterIpAddresses(int adapterIndex, bool bHiddenAdapter,
	const CHostOnlyNetwork *pNetwork)
{
	UNUSED_PARAM(bHiddenAdapter);
	// Set address for our adapter (via SIOCSIFADDR)
	assert( adapterIndex>=0 );

	if (!pNetwork)
		return PRL_ERR_INVALID_ARG;

	adapterIndex |= PRL_ADAPTER_START_INDEX;

	// find apdater with required index
	EthIfaceList ethList;
	if( !::makeEthIfacesList( ethList, false ) ) //all interfaces, not only UP
	{
		MODULE_STORE_SYSTEM_ERROR();

		WRITE_TRACE(DBG_FATAL, "[PrlNet]  makeEthIfacesList returned error: %ld",
			PrlNet::getSysError() );
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
			adapterIndex );
		return PRL_NET_ADAPTER_NOT_EXIST;
	}

	return setAdapterIpAddresses(adapter->_name, pNetwork);
}


PRL_RESULT PrlNet::delPrlAdapterIpAddresses(int adapterIndex, bool bHiddenAdapter,
	const CHostOnlyNetwork *pNetwork)
{
	UNUSED_PARAM(adapterIndex);
	UNUSED_PARAM(bHiddenAdapter);
	UNUSED_PARAM(pNetwork);
	return PRL_ERR_SUCCESS;
}


PRL_RESULT PrlNet::addArp(const QString &ip, const QString &devName, const QString &hwaddr)
{
	unsigned char ethmac[ETH_ADDRESS_LEN];

	if (!PrlNet::stringToEthAddress(hwaddr, ethmac))
		return PRL_ERR_FAILURE;

	WRITE_TRACE(DBG_DEBUG, "addArp ip=%s devName=%s hwaddr=%s",
			ip.toUtf8().constData(),
			devName.toUtf8().constData(),
			hwaddr.toUtf8().constData());
	if (arp_add(ip.toUtf8().constData(), devName.toUtf8().constData(), ethmac))
		return PRL_ERR_FAILURE;

	return PRL_ERR_SUCCESS;
}


PRL_RESULT PrlNet::delArp(const QString &ip, const QString &devName, const QString &hwaddr)
{
	(void) ip;
	unsigned char ethmac[ETH_ADDRESS_LEN];

	if (!PrlNet::stringToEthAddress(hwaddr, ethmac))
		return PRL_ERR_FAILURE;

	WRITE_TRACE(DBG_DEBUG, "delArp devName=%s hwaddr=%s",
			devName.toUtf8().constData(),
			hwaddr.toUtf8().constData());
	if (arp_del_by_hwaddr(devName.toUtf8().constData(), ethmac))
		return PRL_ERR_FAILURE;

	return PRL_ERR_SUCCESS;
}

#define PRL_CMD_WORK_TIMEOUT 5 * 1000
static PRL_RESULT run_prg(const char *name, const QStringList &lstArgs)
{
	QProcess proc;
	WRITE_TRACE(DBG_INFO, "%s %s", name, lstArgs.join(" ").toUtf8().constData());
	proc.start(name, lstArgs);
	if (!proc.waitForFinished(PRL_CMD_WORK_TIMEOUT))
	{
		WRITE_TRACE(DBG_FATAL, "%s tool not responding. Terminate it now.", name);
		proc.kill();
		return PRL_ERR_OPERATION_FAILED;
	}

	if (0 != proc.exitCode())
	{
		WRITE_TRACE(DBG_FATAL, "%s utility failed: %s %s [%d]\nout=%s\nerr=%s",
				name, name,
				lstArgs.join(" ").toUtf8().constData(),
				proc.exitCode(),
				proc.readAllStandardOutput().data(),
				proc.readAllStandardError().data());
		return PRL_ERR_OPERATION_FAILED;
	}
	return PRL_ERR_SUCCESS;
}

bool PrlNet::updateArp(const QString &ip, const QString &src_hwaddr, const QString &iface)
{
	QStringList lstArgs;

	lstArgs += "-U";

	lstArgs += "-w1";
	lstArgs += "-c1";

	if (!src_hwaddr.isEmpty()) {
		lstArgs += "-S";
		lstArgs += src_hwaddr;
		lstArgs += "-s";
		lstArgs += src_hwaddr;

	}
	lstArgs += "-e";
	lstArgs += ip;
	lstArgs += "-i";
	lstArgs += ip;

	lstArgs += iface;

	return run_prg("/usr/sbin/prl_arpsend", lstArgs);
}


#if 0
void PrlNet::startVmeNetworking(const CParallelsNetworkConfig &networkConfig)
{
	CVirtualNetworks *pNetworks = networkConfig.getVirtualNetworks();

	foreach(CVirtualNetwork *pNetwork, pNetworks->m_lstVirtualNetwork)
	{
		if (!pNetwork->isEnabled() ||
			pNetwork->getNetworkType() != PVN_HOST_ONLY)
			continue;

		CHostOnlyNetwork *pHostOnlyNetwork = pNetwork->getHostOnlyNetwork();
		if (!pHostOnlyNetwork)
			continue;

		QString sAdapter;
		CVzVirtualNetwork VzNet;
		PRL_RESULT prlResult = VzNet.GetVirtualNetworkBridgeAdapter(
					pNetwork->getNetworkID(), sAdapter);
		if (PRL_FAILED(prlResult) || sAdapter.isEmpty()) // not found
			continue;

		prlResult = setAdapterIpAddresses(sAdapter, pHostOnlyNetwork);
		if (!PRL_SUCCEEDED(prlResult))
		{
			WRITE_TRACE(DBG_FATAL, "Failed to configure Host Adapter"
				"'%s' for network '%s'", QSTR2UTF8(sAdapter),
				QSTR2UTF8(pNetwork->getNetworkID()));
			syslog(LOG_NOTICE, "Failed to configure Host Adapter"
				"'%s' for network '%s'\n", QSTR2UTF8(sAdapter),
				QSTR2UTF8(pNetwork->getNetworkID()));
		}
	}
}
#endif


bool PrlNet::IsIPv6DefaultRoutePresent()
{
	char buf[1024];

	FILE *f = fopen("/proc/net/ipv6_route", "r");
	if (NULL == f) {
		WRITE_TRACE(DBG_DEBUG, "IPv6 is not configured on the host");
		return FALSE;
	}

	bool defaultgw_found = false;
	while (fgets(buf, 1023, f)) {
		char iface[17];
		UINT32 addr[4], saddr[4], naddr[4];
		int iflags, metric, refcnt, use, prefix_len, slen;
		iflags = 0;
		(void)sscanf(buf, "%08x%08x%08x%08x %02x %08x%08x%08x%08x %02x %08x%08x%08x%08x"
							" %08x %08x %08x %08x %16s\n",
					&addr[0], &addr[1], &addr[2], &addr[3],
					&prefix_len,
					&saddr[0], &saddr[1], &saddr[2], &saddr[3],
					&slen,
					&naddr[0], &naddr[1], &naddr[2], &naddr[3],
					&metric, &use, &refcnt, &iflags, iface);
		if ((iflags & (RTF_UP | RTF_GATEWAY)) != (RTF_UP | RTF_GATEWAY))
			continue;
		if (prefix_len == 0) {
			defaultgw_found = true;
			break;
		}
	}
	fclose(f);

	return defaultgw_found;
}
