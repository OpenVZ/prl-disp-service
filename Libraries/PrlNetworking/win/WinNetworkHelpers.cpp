/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
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
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include <winsock2.h>
#include <ws2tcpip.h>
#include <IPHlpApi.h>

#include "PrlNetLibrary.h"
#include "ethlist.h"

#include <prlcommon/Std/scoped_array.h>
#include <prlcommon/Std/scoped_mem.h>

#include <cassert>
#include <memory>
#include <iostream>

#include <prlcommon/Logging/Logging.h>

#include <QHostAddress>

#include "WinNetworkHelpers.h"

using namespace PrlNetInternal;

PrlNetInternal::IpHlpApi IpHlpApi::_instance;

IpHlpApi::LPFN_GetIpForwardTable2 IpHlpApi::pfnGetIpForwardTable2 = NULL;
IpHlpApi::LPFN_FreeMibTable IpHlpApi::pfnFreeMibTable = NULL;
IpHlpApi::LPFN_NotifyMibChange IpHlpApi::pfnNotifyRouteChange2 = NULL;
IpHlpApi::LPFN_CancelMibChangeNotify2 IpHlpApi::pfnCancelMibChangeNotify2 = NULL;


PrlNetInternal::IpHlpApi::IpHlpApi()
{
	_netio_available = false;
	_hIpHlpApi = LoadLibrary( L"iphlpapi.dll" );
	if (NULL == _hIpHlpApi) {
		WRITE_TRACE(DBG_FATAL, "Failed to load iphlpapi.dll");
		return;
	}

	pfnGetIpForwardTable2 = (LPFN_GetIpForwardTable2)GetProcAddress(
		_hIpHlpApi, "GetIpForwardTable2");
	pfnFreeMibTable = (LPFN_FreeMibTable)GetProcAddress(
		_hIpHlpApi, "FreeMibTable");
	pfnCancelMibChangeNotify2 = (LPFN_CancelMibChangeNotify2)GetProcAddress(
		_hIpHlpApi, "CancelMibChangeNotify2");
	pfnNotifyRouteChange2 = (LPFN_NotifyMibChange)GetProcAddress(
		_hIpHlpApi, "NotifyRouteChange2");

	if (pfnFreeMibTable && pfnGetIpForwardTable2
			&& pfnCancelMibChangeNotify2 && pfnNotifyRouteChange2) {
		_netio_available = true;
	}
}


PrlNetInternal::IpHlpApi::~IpHlpApi()
{
	if (!_hIpHlpApi)
		return;
	FreeLibrary(_hIpHlpApi);
	_hIpHlpApi = NULL;
}

// splits string in format someting{GUID} to GUID
static QString
guidFromName(const QString &name)
{
	QRegExp rx("(\\{\\S+\\})");
	int pos = rx.indexIn(name);
	return pos < 0 ? "" : rx.cap(1);
}


// returns entry in the ethList corresponding to pIfRow ( end() if not found )
static EthIfaceList::const_iterator
findAdapterInList(const EthIfaceList &ethList, PIP_ADAPTER_INFO pAdapter)
{
	try {
		// Search using GUIDs
		QString search_guid = guidFromName(pAdapter->AdapterName);
		if (search_guid.isEmpty()) {
			WRITE_TRACE(DBG_FATAL, "iface_name %s doesn't contain guid",
				pAdapter->AdapterName);
			return ethList.end();
		}

		for (EthIfaceList::const_iterator it = ethList.begin(); it != ethList.end(); ++it) {
			// consider only real adapters
			if (IS_PRL_ADAPTER_INDEX(it->_nAdapter))
				continue;

			QString guid = guidFromName(it->_adapterGuid);
			if (0 == guid.compare(search_guid, Qt::CaseInsensitive))
				return it;
		}
	} catch (std::exception&) {
		WRITE_TRACE(DBG_FATAL, "std::exception caught");
	}

	return ethList.end();
}


/// allocates memory for the adapters info and returns system adapters info list
/// THIS FUNCTION HAVE LIMITATIONS
/// IT USED WINAPI GetAdaptersInfo FOR COMPATIBILITY WITH W2K
/// DO NOT FORGET NEXT: This function do not return information about the IPv4 loopback interface.
/// SEE DESCRIPTION: http://msdn2.microsoft.com/en-us/library/aa365917.aspx
/// DUE TO THIS LIMITATIONS WAS ADDED WORKAROUND IN SDK\Handles\PrlServerScaner.cpp: constructor PrlServerScaner
/// BUGFIX 5972
/// AFTER REMOVING LIMITATIONS WORKAROUND MUST BE REMOVED.
PIP_ADAPTER_INFO PrlNetInternal::getAdaptersInfo()
{
	// Obtain adapters list
	DWORD dwAdaptersListSize = 0;
	DWORD dwErr = ::GetAdaptersInfo(NULL, &dwAdaptersListSize);
	if (ERROR_BUFFER_OVERFLOW != dwErr) {
		::SetLastError(dwErr);
		return NULL;
	}

	PIP_ADAPTER_INFO pAdaptersList = (PIP_ADAPTER_INFO)::malloc(dwAdaptersListSize);
	dwErr = ::GetAdaptersInfo( pAdaptersList, &dwAdaptersListSize );
	if (NO_ERROR != dwErr) {
		::SetLastError(dwErr);
		::free( pAdaptersList );
		return NULL;
	}

	return pAdaptersList;
}


PMIB_IFTABLE PrlNetInternal::getIfTable()
{
	DWORD dwIfTabSize = 0;
	DWORD dwErr = ::GetIfTable(NULL, &dwIfTabSize, FALSE);
	if (ERROR_INSUFFICIENT_BUFFER != dwErr) {
		::SetLastError(dwErr);
		return NULL;
	}

	PMIB_IFTABLE pIfTable = (PMIB_IFTABLE)::malloc(dwIfTabSize);

	dwErr = ::GetIfTable(pIfTable, &dwIfTabSize, FALSE);
	if (NO_ERROR != dwErr) {
		::free(pIfTable);
		::SetLastError(dwErr);
		return NULL;
	}

	return pIfTable;
}


/// searches Parallels adapter in the adapters list of the whole system
PRL_RESULT PrlNetInternal::findPrlAdapter(
	int prlAdapterIndex,
	PIP_ADAPTER_INFO pAdaptersList,
	PIP_ADAPTER_INFO *ppAdapter)
{
	prlAdapterIndex |= PRL_ADAPTER_START_INDEX;
	*ppAdapter = NULL;

	EthIfaceList ethList;
	if (!::makeEthIfacesList(ethList, true)) // only UP adapters
		return PRL_NET_ETHLIST_CREATE_ERROR;

	// find a Parallels adapter with specified index
	EthIfaceList::const_iterator itPrlAdapter = ethList.begin();
	for (; itPrlAdapter != ethList.end(); ++itPrlAdapter)
		if( itPrlAdapter->_nAdapter == prlAdapterIndex )
			break;

	if (itPrlAdapter == ethList.end()) {
		WRITE_TRACE(DBG_FATAL, "[PrlNetInternal::findPrlAdapter] Parallels Adapter %d doesn't exist",
			prlAdapterIndex );
		return PRL_ERR_FAILURE;
	}

	try {
		QString search_guid = guidFromName(itPrlAdapter->_adapterGuid);
		for (PIP_ADAPTER_INFO pAdapter = pAdaptersList;
				NULL != pAdapter; pAdapter = pAdapter->Next) {
			const QString guid = guidFromName(pAdapter->AdapterName);
			if (0 == guid.compare(search_guid, Qt::CaseInsensitive)) {
				*ppAdapter = pAdapter;
				return PRL_ERR_SUCCESS;
			}
		}
	} catch (std::exception&) {
		WRITE_TRACE(DBG_FATAL, "std::exception caught");
	}

	return PRL_ERR_FAILURE;
}


PIP_INTERFACE_INFO PrlNetInternal::getInterfacesInfo()
{
	Prl::scoped_mem scm;

	scm.alloc_mem( sizeof(IP_INTERFACE_INFO) );
	IP_INTERFACE_INFO *pInterfacesInfo = scm.get<IP_INTERFACE_INFO>();

	ULONG ulOutBufLen = 0;

	DWORD dwErr = GetInterfaceInfo(pInterfacesInfo, &ulOutBufLen);
	if (dwErr && dwErr != ERROR_INSUFFICIENT_BUFFER)
		return NULL;

	scm.alloc_mem( ulOutBufLen );
	pInterfacesInfo = scm.get<IP_INTERFACE_INFO>();

	dwErr = GetInterfaceInfo( pInterfacesInfo, &ulOutBufLen );
	if (dwErr != NO_ERROR)
		return NULL;

	scm.release();

	return pInterfacesInfo;
}


bool PrlNetInternal::GetPrlAdapterSysIndex(int adapterIndex, int &iface_idx)
{
	IP_ADAPTER_INFO *pAdaptersList = getAdaptersInfo();
	Prl::scoped_mem pAdaptersList_scm(pAdaptersList);
	if (NULL == pAdaptersList)
		return false;

	IP_ADAPTER_INFO *pAdapter = NULL;
	PRL_RESULT prlResult = findPrlAdapter(adapterIndex, pAdaptersList, &pAdapter);
	if (!PRL_SUCCEEDED(prlResult))
		return false;
	iface_idx = pAdapter->Index;
	return true;
}


static PIP_ADAPTER_ADDRESSES getAdaptersAddresses(ULONG family)
{
	PIP_ADAPTER_ADDRESSES pAddresses = NULL;

	int Iterations = 0;
	DWORD rv;
	DWORD outBufLen = 16384;
	do {
		pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen);
		if (pAddresses == NULL) {
			WRITE_TRACE(DBG_FATAL, "Failed to malloc IP_ADAPTER_ADDRESSES");
			return NULL;
		}

		rv = GetAdaptersAddresses(
			family,
			GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_ANYCAST
				| GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_FRIENDLY_NAME
				| GAA_FLAG_SKIP_DNS_SERVER,
			NULL, pAddresses, &outBufLen);
		if (rv != ERROR_BUFFER_OVERFLOW)
			break;

		free(pAddresses);
		pAddresses = NULL;
		Iterations++;
	} while (Iterations < 3);

	if (rv != 0) {
		free(pAddresses);
		WRITE_TRACE(DBG_FATAL, "Failed to GetAdaptersAddresses(): error %u", rv);
		return NULL;
	}

	return pAddresses;
}


PRL_RESULT  PrlNetInternal::getAdapterIPv6List(
	int iface_sysidx,
	QList<IP6AddrMask> &ipv6List)
{
	ipv6List.clear();

	PIP_ADAPTER_ADDRESSES pAdaptersAddresses = getAdaptersAddresses(AF_INET6);
	Prl::scoped_mem pAdaptersAddresses_scm(pAdaptersAddresses);

	if (NULL == pAdaptersAddresses)
		return PRL_NET_SYSTEM_ERROR;

	for (PIP_ADAPTER_ADDRESSES curr = pAdaptersAddresses; curr != NULL; curr = curr->Next) {
		if (curr->Ipv6IfIndex != iface_sysidx)
			continue;
		for (IP_ADAPTER_UNICAST_ADDRESS *addr = curr->FirstUnicastAddress;
			addr != NULL;
			addr = addr->Next)
		{
			int prefix_len = 0; // unknown
			// Note: it is possible to iterate through curr->FirstPrefix list
			// and determine exact prefix, but it is too much for this code.
#if (_WIN32_WINNT >= 0x0600)
			prefix_len = addr->OnLinkPrefixLength;
#endif
			sockaddr_in6 *s6 = (sockaddr_in6 *)addr->Address.lpSockaddr;
			QHostAddress ip6((UINT8 *)&s6->sin6_addr);
			IP6AddrMask a(ip6, prefix_len);
			ipv6List.push_back(a);
		}
	}
	return PRL_ERR_SUCCESS;
}


static MIB_IPFORWARDTABLE *getIpForwardTable()
{
	// Obtain routing table
	DWORD dwRouteTableSize = 0;
	DWORD dwErr = GetIpForwardTable(NULL, &dwRouteTableSize, FALSE);
	if (ERROR_INSUFFICIENT_BUFFER != dwErr) {
		::SetLastError(dwErr);
		return NULL;
	}

	PMIB_IPFORWARDTABLE route = (PMIB_IPFORWARDTABLE)::malloc(dwRouteTableSize);
	dwErr = GetIpForwardTable(route, &dwRouteTableSize, FALSE);
	if (NO_ERROR != dwErr) {
		free(route);
		::SetLastError(dwErr);
		return NULL;
	}
	return route;
}


static void mib_free_helper(VOID *p)
{
	IpHlpApi::pfnFreeMibTable(p);
}


SmartPtr<MIB_IPFORWARD_TABLE2> PrlNetInternal::getIpForwardTable2()
{
	if (!IpHlpApi::IsNetIoAvail())
		return SmartPtr<MIB_IPFORWARD_TABLE2>(NULL);

	// Obtain routing table
	PMIB_IPFORWARD_TABLE2 table = NULL;

	DWORD dwErr = IpHlpApi::pfnGetIpForwardTable2(0 /*any*/, &table);
	if (0 != dwErr) {
		::SetLastError(dwErr);
		return SmartPtr<MIB_IPFORWARD_TABLE2>(NULL);
	}
	return SmartPtr<MIB_IPFORWARD_TABLE2>(table, mib_free_helper);
}


// Obtains bridged interface with specified index
PRL_RESULT PrlNetInternal::getDefaultBridgedAdapter(
	const EthIfaceList &ethList, EthIfaceList::const_iterator &itAdapter)
{
	//
	// Default Bridged adapter is the one which handles default route
	//
	itAdapter = ethList.end();

	IP_ADAPTER_INFO *pAdaptersList = getAdaptersInfo();
	Prl::scoped_mem pAdaptersList_scm(pAdaptersList);
	if (NULL == pAdaptersList)
		return PRL_NET_SYSTEM_ERROR;

	// Obtain routing table
	PMIB_IPFORWARDTABLE route = getIpForwardTable();
	Prl::scoped_mem route_scm(route);
	if (!route)
		return PRL_NET_SYSTEM_ERROR;

#ifdef LOGGING_ON
	LOG_MESSAGE(DBG_TRACE, "------------");
	for (EthIfaceList::const_iterator it = ethList.begin(); it != ethList.end(); ++it)
		LOG_MESSAGE(DBG_TRACE, "iface %s %s", QSTR2UTF8(it->_name), QSTR2UTF8(it->_adapterGuid));

	LOG_MESSAGE(DBG_TRACE, "-------------AdaptersInfo--------------");
	PIP_ADAPTER_INFO pAdapter;
	for (pAdapter = pAdaptersList;	NULL != pAdapter; pAdapter = pAdapter->Next) {
		LOG_MESSAGE(DBG_TRACE, "idx %d descr %s name %ws",
			pAdapter->Index, pAdapter->Description, pAdapter->AdapterName);
	}

	LOG_MESSAGE(DBG_TRACE, "---------------Routes---------------");
	for (uint i = 0; i<route->dwNumEntries; i++) {
		LOG_MESSAGE(DBG_TRACE, "%d: if %d, proto %d, %08x/%08x, M=%d",
			i, route->table[i].dwForwardIfIndex,
			route->table[i].dwForwardProto,
			htonl(route->table[i].dwForwardDest), htonl(route->table[i].dwForwardMask),
			route->table[i].dwForwardMetric1);
	}
#endif /* LOGGING_ON */

	// Find the route with lowest metric and corresponding to it adapter
	DWORD dwMinMetric = 0x7fffffff;
	for (UINT i = 0; i<route->dwNumEntries; ++i) {
		if (0 != route->table[i].dwForwardDest
				|| route->table[i].dwForwardMetric1 >= dwMinMetric)
			continue;

		IP_ADAPTER_INFO *pAdapter;
		// find the adapter in the adapterslist by its index
		for (pAdapter = pAdaptersList; NULL != pAdapter; pAdapter = pAdapter->Next)
			if (pAdapter->Index == route->table[i].dwForwardIfIndex)
				break;

		if (NULL == pAdapter) {
			WRITE_TRACE(DBG_FATAL, "AdapterIndex %d not found in IP_ADAPTER_INFO list",
				(int)route->table[i].dwForwardIfIndex);
			continue; // impossible, but...
		}

		EthIfaceList::const_iterator it =
			findAdapterInList(ethList, pAdapter);

		if (it != ethList.end()) {
			itAdapter = it;
			dwMinMetric = route->table[i].dwForwardMetric1;
		}
	}

	if (itAdapter != ethList.end())
		return PRL_ERR_SUCCESS;
	return PRL_ERR_FAILURE;
}


PRL_RESULT  PrlNetInternal::getPrlSystemAdapterParams(
	int prlAdapterIndex,
	unsigned int &ipaddr,
	unsigned int &netmask )
{
	IP_ADAPTER_INFO *pAdaptersList = getAdaptersInfo();
	Prl::scoped_mem pAdaptersList_scm(pAdaptersList);
	if (NULL == pAdaptersList)
		return PRL_NET_SYSTEM_ERROR;

	PIP_ADAPTER_INFO pAdapter = NULL;
	PRL_RESULT prlResult = findPrlAdapter(prlAdapterIndex, pAdaptersList, &pAdapter);
	if (!PRL_SUCCEEDED(prlResult))
		return prlResult;

	ipaddr = QHostAddress(pAdapter->IpAddressList.IpAddress.String).toIPv4Address();
	netmask = QHostAddress(pAdapter->IpAddressList.IpMask.String).toIPv4Address();
	return PRL_ERR_SUCCESS;
}
