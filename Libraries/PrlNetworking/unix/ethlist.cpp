///////////////////////////////////////////////////////////////////////////////
///
/// @file ethlist.cpp
///
/// Ethernet interfaces enumerating functions for Unix systems implementations
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
#include <prlsdk/PrlEnums.h>

#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/time.h>

#include <netinet/in.h>

#include "ethlist.h"

#include <net/if.h>
#include <sys/ioctl.h>

#include <string>
#include <limits.h>

#include <net/if_arp.h>

#if defined(_LIN_)
#include <fstream>
#include <algorithm>
#include <linux/if_vlan.h>
#include <linux/sockios.h>
#include <sys/stat.h>
#endif

#include "../PrlNetLibrary.h"

EthIface::EthIface()
	: _name()
	, _ifaceFlags()
	, _bcastIpAddr()
	, _vlanTag(PRL_INVALID_VLAN_TAG)
	, _nAdapter(-1)
{
	memset( _macAddr, 0, 6 );
}


EthIface::EthIface( const EthIface& eth )
	: _name(eth._name)
	, _ifaceFlags(eth._ifaceFlags)
	, _bcastIpAddr(eth._bcastIpAddr)
	, _vlanTag(eth._vlanTag)
	, _nAdapter(eth._nAdapter)
{
	memcpy( _macAddr, eth._macAddr, 6 );
}


EthIface::EthIface(const QString &name, int ifaceFlags, unsigned bcastIpAddr,
		const unsigned char macAddr[6], unsigned short vlanTag, int nAdapter)
	: _name(name)
	, _ifaceFlags(ifaceFlags)
	, _bcastIpAddr(bcastIpAddr)
	, _vlanTag(vlanTag)
	, _nAdapter(nAdapter)
{
	memcpy( _macAddr, macAddr, 6 );
}


namespace
{

class CEthIfaceListReader
{
	int 	 m_nAdapterCounter; /// counter for the real adapter index

	/// Reads configuration for the interface with parameters filled in ifr using control socket sock
	/// to the eth
	///@param sock  Control socket [in]
	///@param ifr  	Request structure with filled name and AddrFamily. In MAC it must be as it came from SIOCGIFCONF [in]
	///@param eth 	Resulting configuratuib [out]
	///@param bUpOnly If true, then only UP interface will be processed. For down interfaces false will be returned
	///@return true if configuration was successfully read
	bool readIfaceConf( int sock, const struct ifreq *ifr, EthIface &eth, bool bUpOnly, bool bConfigured );
public:
	CEthIfaceListReader();
	bool makeEthIfacesList( EthIfaceList &ethList, bool bUpOnly, bool bCOnfigured = false );
};


CEthIfaceListReader::CEthIfaceListReader()
{
	m_nAdapterCounter = 0;
}



// Obtain VLAN-info of the interface
// @param sock control-socket
// @param ifr request with filled interface name to ioctl to
// @param parent_name name of the parent interface (NULL if not required), at least IFNAMSIZ bytes
// @param vlan_tag tag of the vlan-interface
// @return false if interface is not a vlan
static bool getVLANInfo(int sock, const struct ifreq *ifr, char *parent_name, unsigned short &vlan_tag)
{
#if defined(_MAC_)
    // Headers on my system doesn't define this struct,
	// so define it manually and add some guards.
	struct  vlanreq {
		char    vlr_parent[IFNAMSIZ];
		u_short vlr_tag;

		int     my_tag;
		char    padding[256];
	} vlr;

	memset(&vlr, 0, sizeof(vlr));
	vlr.vlr_tag = PRL_INVALID_VLAN_TAG; // should be 1-4095 after completion
	vlr.my_tag = 'PRLV';  // should not be rewritten by ioctl

	struct ifreq ifreq;
	memset(&ifreq, 0, sizeof(ifreq));
	memcpy(&ifreq.ifr_name, ifr->ifr_name, IFNAMSIZ);
	ifreq.ifr_data = (char *)&vlr;

	if (ioctl(sock, SIOCGETVLAN, &ifreq) < 0)
		return false;

	if (vlr.my_tag != 'PRLV')
		return false;

	vlr.my_tag = 0; // in case that vlr_parent is invalid
	int parent_name_len = strlen(vlr.vlr_parent);
	if (parent_name_len == 0 || parent_name_len >= IFNAMSIZ)
		return false;
	if (NULL != parent_name)
		strcpy(parent_name, vlr.vlr_parent);
	vlan_tag = vlr.vlr_tag;

	return (vlr.vlr_tag <= PRL_MAX_VLAN_TAG);
#else
	(void)parent_name;
	struct vlan_ioctl_args vlan_req;
	memset(&vlan_req, 0, sizeof(vlan_req));
	strncpy(vlan_req.device1, ifr->ifr_name, sizeof(vlan_req.device1));
	// Note: this ioctl is only available at kernels >=2.6.10
	vlan_req.cmd = GET_VLAN_VID_CMD;
	if (::ioctl(sock, SIOCGIFVLAN, &vlan_req) != 0 )
		return PRL_INVALID_VLAN_TAG;
	vlan_tag = vlan_req.u.VID;
	return (vlan_req.u.VID <= PRL_MAX_VLAN_TAG);
#endif
}

static unsigned short readVLANTag(int sock, const struct ifreq *ifr)
{
	unsigned short vlan_tag = PRL_INVALID_VLAN_TAG;
	if (!getVLANInfo(sock, ifr, NULL, vlan_tag))
		return PRL_INVALID_VLAN_TAG;
	return vlan_tag;
}


/// Read mac-addr.
/// Note: For Mac OS MAC is already contained in ifr and should be just
///       copied to macAddr
/// @param macAddr array to store the macAddr to
static bool readMacAddr(int sock, const struct ifreq *ifr,
						 unsigned char *macAddr)
{
#if defined(_MAC_)
	(void)sock;

	struct sockaddr_dl *sdl = (struct sockaddr_dl *)&ifr->ifr_addr;
	if( sdl->sdl_alen != 6 )
		return false; // not an ethernet interface

	memcpy( macAddr, (unsigned char *)sdl->sdl_data + sdl->sdl_nlen, 6);

	return true;
#elif defined(_LIN_)
	struct ifreq ifreq; // create a copy for each request
	memcpy(&ifreq, ifr, sizeof(ifreq));
	if (ioctl(sock, SIOCGIFHWADDR, &ifreq) < 0)
		return false;

	if (ifreq.ifr_hwaddr.sa_family != ARPHRD_ETHER)
		return false; //not an ethernet NIC
	memcpy( macAddr, (unsigned char *)ifreq.ifr_hwaddr.sa_data, 6 );

	return true;
#endif
}


// Detects whether the interface is Parallels VNIC and returns its number
// @return -1 if interface is not a VNIC, its number otherwise
static int readVNICNumber(int sock, const struct ifreq *ifr)
{
#if defined(_MAC_)
	struct ifreq ifreq; // create a copy for each request
	memcpy(&ifreq, ifr, sizeof(ifreq));
	ifreq.ifr_intval = 0;
	if (ioctl(sock, SIOCGPRLVNICNO, &ifreq) != 0)
		return -1;

	int nAdapter = ifreq.ifr_intval - 'PVS0';
	if (nAdapter >=0 && nAdapter <= 16)
		return (nAdapter + PRL_ADAPTER_START_INDEX);
	else
		return -1; // strange. Incorrect value returned.
#else
	// should be name in format "vnicX"
	char c;
	(void)sock;

	int nAdapter = -1;
	if (1 == sscanf(ifr->ifr_name, "vnic%u%c", &nAdapter, &c))
		return (nAdapter + PRL_ADAPTER_START_INDEX);
	else
		return -1;
#endif
}

#if !defined(_LIN_)
static bool IsPrlVme(const char *ifname)
{
	(void)ifname;
	return false;
}

#else

static bool IsPrlVme(const char *ifname)
{
	char path[128];
	struct stat st;

	snprintf(path, sizeof(path),
			"/sys/class/net/%s/prl_vme", ifname);

	return (stat(path, &st) == 0);
}

#endif


bool CEthIfaceListReader::readIfaceConf( int sock, const struct ifreq *ifr, EthIface &eth,
		bool bUpOnly, bool bConfigured )
{
	struct ifreq ifreq; // create a copy for each request
	//
	// Obtain flags of the interface
	//
	memcpy(&ifreq, ifr, sizeof(ifreq));
	if ( ioctl(sock, SIOCGIFFLAGS, &ifreq) < 0 )
		return false;

	int ifaceFlags = ifreq.ifr_flags;

	if( bUpOnly && !(ifaceFlags&IFF_UP) )
		return false;

	if ( bConfigured && ioctl(sock, SIOCGIFADDR, &ifreq) )
		return false;

#ifdef _LIN_
	// filter out bonding slaves
	if (ifaceFlags & IFF_SLAVE)
		return false;
#endif
	unsigned char macAddr[6];
	if ( !readMacAddr( sock, ifr, macAddr) )
		return false;

	// obtain broadcast address of the interface
	unsigned int bcastIpAddr = 0;
	if(  ifaceFlags & IFF_BROADCAST && !(ifaceFlags & IFF_LOOPBACK) )
	{
		memcpy(&ifreq, ifr, sizeof(ifreq));
		if( ioctl(sock, SIOCGIFBRDADDR, &ifreq)  >= 0 )
		{
			struct sockaddr_in *sin = (struct sockaddr_in *)&ifreq.ifr_addr;
			if ( sin->sin_addr.s_addr != 0 && 0xffffffff != sin->sin_addr.s_addr )
			{
				bcastIpAddr = sin->sin_addr.s_addr;
			}
		}
	}

	unsigned short vlan_tag = readVLANTag(sock, ifr);

	// is it a PVSNIC ?
	int nAdapter = readVNICNumber(sock, ifr);
	if (nAdapter < 0) {
		nAdapter = m_nAdapterCounter++;
		bool is_vme = IsPrlVme(ifr->ifr_name);
		if (is_vme)
			nAdapter |= PRL_VME_ADAPTER_START_INDEX;
	}

#if defined(_LIN_)
	// filter out Linux bridges
	{
		char path[PATH_MAX];
		struct stat st;

		snprintf(path, PATH_MAX,
				"/sys/class/net/%s/bridge", ifr->ifr_name);
		if ((stat(path, &st) == 0) && S_ISDIR(st.st_mode))
			return false;
	}
#endif

	eth = EthIface(ifr->ifr_name, ifaceFlags, bcastIpAddr, macAddr, vlan_tag, nAdapter ) ;

	return true;
}

#if defined(_LIN_)

#define PROC_NET_DEV "/proc/net/dev"

// in linux SIOCGIFCONF returns only UP interfaces and only configured. So we must use some other method..
bool CEthIfaceListReader::makeEthIfacesList( EthIfaceList &ethList, bool bUpOnly,  bool bConfigured)
{
	std::ifstream f( PROC_NET_DEV );
	if( !f )
		return false;

	std::string sEntry; // line in the netdev file

	// skip first two lines
	for( int i = 0; i<2; i++ )
	{
		std::getline(f, sEntry);
		if( f.fail() )
			return false;
	}

	// open socket for obtaining ifaces configurations
	int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		return false;

	// for all entries in netdev
	while( !f.eof() )
	{
		std::getline(f, sEntry);
		if( f.fail() )
			break;

		std::size_t pos = sEntry.find_first_not_of(" ");
		if( pos == std::string::npos )
			continue; // invalid entry
		std::size_t pos2 = sEntry.find( ':', pos );
		if( pos2 == std::string::npos )
			continue; // invalid entry

		std::string ifName = sEntry.substr( pos, pos2-pos );

		struct ifreq ifr;
		memset( &ifr, 0, sizeof(ifr) );
		ifr.ifr_addr.sa_family = AF_INET;
		strncpy( ifr.ifr_name, ifName.c_str(), sizeof(ifr.ifr_name) );

		EthIface ethIface;
		if( !readIfaceConf(sock, &ifr, ethIface, bUpOnly, bConfigured ) )
			continue;

		ethList.push_back(ethIface);
	}

	::close(sock);

	return true;
}
#else
#error "Trouble with system macro"
#endif

} // end of anonymous namespace

bool makeEthIfacesList( EthIfaceList &ethList, bool bUpOnly, bool bConfigured )
{
	CEthIfaceListReader ethIfaceListReader;

	bool bRes = ethIfaceListReader.makeEthIfacesList( ethList, bUpOnly, bConfigured );
	if( bRes )
		ethList.sort(); // sort adapters
	return bRes;
}

#if defined(_MAC_)
bool PrlNet::GetVLANInfo(const char *vlan_if_name, QString &parent_name, unsigned short &vlan_tag)
{
	int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		return false;

	struct ifreq ifr;
	memset( &ifr, 0, sizeof(ifr) );
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy( ifr.ifr_name, vlan_if_name, sizeof(ifr.ifr_name) );
	char parent_iff[IFNAMSIZ];
	if (!getVLANInfo(sock, &ifr, parent_iff, vlan_tag)) {
		::close(sock);
		return false;
	}
	::close(sock);
	parent_name = parent_iff;
	return true;
}
#endif
