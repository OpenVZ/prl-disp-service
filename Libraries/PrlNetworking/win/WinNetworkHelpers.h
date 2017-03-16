/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 *
 * This file is part of Virtuozzo Core Libraries. Virtuozzo Core
 * Libraries is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef WinNetworkHelpers_h__
#define WinNetworkHelpers_h__

#include "../PrlNetInternal.h"
#include "ethlist.h"
#include <winsock2.h>
#include <WS2tcpip.h>
#include <IpHlpApi.h>
#include <prlcommon/Std/SmartPtr.h>

namespace PrlNetInternal
{

typedef union _SOCKADDR_INET {
    SOCKADDR_IN Ipv4;
    SOCKADDR_IN6 Ipv6;
    USHORT si_family;
} SOCKADDR_INET, *PSOCKADDR_INET;


typedef struct _IP_ADDRESS_PREFIX {
    SOCKADDR_INET Prefix;
    UINT8 PrefixLength;
} IP_ADDRESS_PREFIX, *PIP_ADDRESS_PREFIX;


typedef struct _MIB_IPFORWARD_ROW2 {
	ULONG64           InterfaceLuid;
	ULONG			  InterfaceIndex;
	IP_ADDRESS_PREFIX DestinationPrefix;
	SOCKADDR_INET     NextHop;
	UCHAR             SitePrefixLength;
	ULONG             ValidLifetime;
	ULONG             PreferredLifetime;
	ULONG             Metric;
	ULONG			  Protocol;
	BOOLEAN           Loopback;
	BOOLEAN           AutoconfigureAddress;
	BOOLEAN           Publish;
	BOOLEAN           Immortal;
	ULONG             Age;
	ULONG			  Origin;
} MIB_IPFORWARD_ROW2, *PMIB_IPFORWARD_ROW2;


typedef struct _MIB_IPFORWARD_TABLE2 {
	ULONG              NumEntries;
	MIB_IPFORWARD_ROW2 Table[ANY_SIZE];
} MIB_IPFORWARD_TABLE2, *PMIB_IPFORWARD_TABLE2;


static inline bool
isDefaultIpv6Route(MIB_IPFORWARD_ROW2 *Row)
{
	return (Row->DestinationPrefix.Prefix.si_family == AF_INET6
			&& Row->DestinationPrefix.PrefixLength == 0
			&& IN6_IS_ADDR_UNSPECIFIED(&Row->DestinationPrefix.Prefix.Ipv6.sin6_addr));
}


// helper to get access to IPHlpApi.dll
class IpHlpApi {
	HMODULE _hIpHlpApi;
	bool _netio_available;

	IpHlpApi();
	~IpHlpApi();

	static IpHlpApi _instance;
public:

	typedef DWORD (WINAPI *LPFN_GetIpForwardTable2)(USHORT Family, PMIB_IPFORWARD_TABLE2 *Table);
	typedef VOID (WINAPI *LPFN_FreeMibTable)(PVOID mem);
	typedef VOID (WINAPI *PMIB_CHANGE_CALLBACK)(
		PVOID CallerContext,
		PVOID Row,
		UINT NotificationType);
	typedef DWORD	(WINAPI *LPFN_NotifyMibChange)(
		USHORT Family,
		PMIB_CHANGE_CALLBACK Callback,
		PVOID CallerContext,
		BOOLEAN InitialNotification,
		HANDLE *NotificationHandle);

	typedef DWORD	(WINAPI *LPFN_CancelMibChangeNotify2)(
		HANDLE NotificationHandle);

	// TRUE if Vista-functions are available
	static bool IsNetIoAvail() {
		return _instance._netio_available;
	}

	static LPFN_GetIpForwardTable2 pfnGetIpForwardTable2;
	static LPFN_FreeMibTable pfnFreeMibTable;
	static LPFN_NotifyMibChange pfnNotifyRouteChange2;
	static LPFN_CancelMibChangeNotify2 pfnCancelMibChangeNotify2;
};


/// Allocates memory for the adapters info and returns system adapters info list.
/// Caller is responsible to call ::free on returned memory
/// THIS FUNCTION HAVE LIMITATIONS
/// IT USED WINAPI GetAdaptersInfo FOR COMPATIBILITY WITH W2K
/// DO NOT FORGET NEXT: This function do not return information about the IPv4 loopback interface.
/// SEE DESCRIPTION: http://msdn2.microsoft.com/en-us/library/aa365917.aspx
/// DUE TO THIS LIMITATIONS WAS ADDED WORKAROUND IN SDK\Handles\PrlServerScaner.cpp: constructor PrlServerScaner
/// BUGFIX 5972
/// AFTER REMOVING LIMITATIONS WORKAROUND MUST BE REMOVED.
PIP_ADAPTER_INFO getAdaptersInfo();

/// Allocates memory for the interfaces info and returns info list.
/// Caller is responsible to call ::free on returned memory
PIP_INTERFACE_INFO getInterfacesInfo();

/// Allocates memory for the interfaces info and returns system adapters info list
/// Caller is responsible to call ::free on returned memory
PMIB_IFTABLE getIfTable();

/// Get routing table.
SmartPtr<MIB_IPFORWARD_TABLE2> getIpForwardTable2();

/// Searches Parallels adapter in the adapters list of the whole system
PRL_RESULT findPrlAdapter(int prlAdapterIndex, PIP_ADAPTER_INFO adaptersList, PIP_ADAPTER_INFO *ppAdapter);

/// returns windows-index of parallels adapter
bool GetPrlAdapterSysIndex(int prlAdapterIndex, int &iface_idx);

/// IP6Address/prefixLen
/// zero prefixLen means unknown
typedef QPair<QHostAddress, int> IP6AddrMask;

/// Obtain list of IPv6 addresses on windows-adapter
/// @param iface_sysidx Windows-index of adapter
PRL_RESULT getAdapterIPv6List(
	int iface_sysidx,
	QList<IP6AddrMask> &ipv6List);

};

#endif // WinNetworkHelpers_h__
