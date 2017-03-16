///////////////////////////////////////////////////////////////////////////////
///
/// @file IpStatistics.cpp
/// @author sdmitry
///
/// Functions for collecting network interfaces statistic (Windows)
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

#include <cassert>
#include <prlcommon/Std/scoped_array.h>
#include <prlcommon/Std/scoped_mem.h>

#include "WinNetworkHelpers.h"
#include "../IpStatistics.h"

#include <prlcommon/Logging/Logging.h>

#include <Winsock2.h>

PRL_RESULT PrlNet::getIfaceStatistics( PrlNet::IfStatList& ifStatList )
{
	ifStatList.clear();

	// Obtain interfaces table
	MIB_IFTABLE *pIfTable = PrlNetInternal::getIfTable();
	Prl::scoped_mem pIfTable_scm( pIfTable );
	if( NULL == pIfTable )
	{
		return PRL_NET_SYSTEM_ERROR;
	}

	MIB_IFROW *pIfRowArray = pIfTable->table;
	for( DWORD i = 0; i<pIfTable->dwNumEntries; ++i )
	{
		IfaceStat ifStat;

		ifStat.ifaceName = QString::fromLocal8Bit( (const char *)pIfRowArray[i].bDescr );

		ifStat.bOperational = (pIfRowArray[i].dwOperStatus == MIB_IF_OPER_STATUS_OPERATIONAL);

		ifStat.nBytesRcvd = pIfRowArray[i].dwInOctets;
		ifStat.nBytesSent = pIfRowArray[i].dwOutOctets;

		ifStat.nInErrors = pIfRowArray[i].dwInErrors;
		ifStat.nOutErrors = pIfRowArray[i].dwOutErrors;

		ifStat.nPktsRcvd = pIfRowArray[i].dwInNUcastPkts + pIfRowArray[i].dwInUcastPkts;
		ifStat.nPktsSent = pIfRowArray[i].dwOutNUcastPkts + pIfRowArray[i].dwOutUcastPkts;

		ifStatList.push_back(ifStat);
	}

	return PRL_ERR_SUCCESS;
}


PRL_RESULT PrlNet::getIfaceIpList( PrlNet::IfIpList &ipList, bool includeIpv6 )
{
    (void)includeIpv6;
	ipList.clear();

	IP_ADAPTER_INFO *pAdaptersList = PrlNetInternal::getAdaptersInfo();
	if( NULL == pAdaptersList )
		return PRL_NET_SYSTEM_ERROR;

	Prl::scoped_mem pAdaptersList_scm( pAdaptersList );

	for(IP_ADAPTER_INFO *pAdapter = pAdaptersList;
		NULL != pAdapter;
		pAdapter = pAdapter->Next )
	{
		for( IP_ADDR_STRING *pIpAddr = &pAdapter->IpAddressList;
			 NULL != pIpAddr;
			 pIpAddr = pIpAddr->Next )
		{
			PrlNet::AddressInfo ai;
			ai.ipv6 = false; //get adapters info returns only ipv4
			ai.ifaceName = pAdapter->Description;
			ai.address = pIpAddr->IpAddress.String;
			ipList.push_back(ai);
		}
	}

	return PRL_ERR_SUCCESS;
}
