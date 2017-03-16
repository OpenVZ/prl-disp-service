///////////////////////////////////////////////////////////////////////////////
///
/// @file IpStatistics.cpp
/// @author sdmitry
///
/// Functions for collecting network interfaces statistic (Unix)
///
/// Copyright (c) 2007-2017, Parallels International GmbH
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
#include "../IpStatistics.h"

#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include <ifaddrs.h>
#include <netdb.h>

#include <arpa/inet.h>

#include <prlcommon/Logging/Logging.h>

PRL_RESULT PrlNet::getIfaceStatistics( PrlNet::IfStatList& ifStatList )
{
	ifStatList.clear();

	return PRL_ERR_SUCCESS;
}



PRL_RESULT PrlNet::getIfaceIpList( PrlNet::IfIpList &ipList, bool includeIpv6 )
{
	ipList.clear();

    char host[NI_MAXHOST];
    struct ifaddrs *ifap = NULL;
    int res = getifaddrs(&ifap);
    if( res < 0 )
    {
        WRITE_TRACE(DBG_FATAL, "getifaddrs() failed." );
        return PRL_NET_SYSTEM_ERROR;
    }

    for( struct ifaddrs *ifa = ifap; NULL != ifa; ifa = ifa->ifa_next )
    {
        if( !ifa->ifa_addr )
            continue;
        if ( includeIpv6 &&
             ifa->ifa_addr->sa_family != AF_INET &&
             ifa->ifa_addr->sa_family != AF_INET6 )
            continue;
        else if ( ! includeIpv6 && ifa->ifa_addr->sa_family != AF_INET )
            continue;

        PrlNet::AddressInfo ai;
        ai.ifaceName = ifa->ifa_name;
        ai.ipv6 = (ifa->ifa_addr->sa_family == AF_INET6);

        // Get address name
        int family = ifa->ifa_addr->sa_family;
        res = getnameinfo(ifa->ifa_addr,
                          (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                                sizeof(struct sockaddr_in6),
                          host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if ( res == 0 ) {
            ai.address = host;
        }
        else {
            WRITE_TRACE(DBG_FATAL, "::getnameinfo failed: ret_code=%d, errno=%d",
                        res, errno);
            continue;
        }

        // Get netmask name
        res = getnameinfo(ifa->ifa_netmask,
                          (family == AF_INET) ? sizeof(struct sockaddr_in) :
                                                sizeof(struct sockaddr_in6),
                          host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
        if ( res == 0 ) {
            ai.netmask = host;
        }

        ipList.append( ai );
    }

    freeifaddrs(ifap);
    ifap = NULL;

    return PRL_ERR_SUCCESS;
}
