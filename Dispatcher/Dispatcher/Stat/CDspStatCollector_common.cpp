///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspStatCollector_common.cpp
///
/// Statistics platform independent part of collector implementation
///
/// @author sandro
///
/// Copyright (c) 2005-2017, Parallels International GmbH
///
/// This file is part of Virtuozzo Core. Virtuozzo Core is free
/// software; you can redistribute it and/or modify it under the terms
/// of the GNU General Public License as published by the Free Software
/// Foundation; either version 2 of the License, or (at your option) any
/// later version.
/// 
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
/// 
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
/// 02110-1301, USA.
///
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#include "CDspStatCollector.h"
#include "Libraries/PrlNetworking/IpStatistics.h"
#include <prlcommon/Logging/Logging.h>
#include "CDspHandlerRegistrator.h"
#include "CDspClientManager.h"
#include "CDspService.h"

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"


CDspStatCollector::CDspStatCollector()
{}

void CDspStatCollector::GetUsersStatInfo(SmartPtr<CUsersStatInfo> pUsersStatInfo)
{
	GetDispatcherUsersSessions(pUsersStatInfo);
	GetSystemUsersSessions(pUsersStatInfo);
}

void CDspStatCollector::GetDispatcherUsersSessions(SmartPtr<CUsersStatInfo> pUsersStatInfo)
{
	QHash< IOSender::Handle, SmartPtr<CDspClient> >
		syncHash = CDspService::instance()->getClientManager()
			.getSessionsListSnapshot();

	QHashIterator< IOSender::Handle, SmartPtr<CDspClient> > itSessions( syncHash );
	while( itSessions.hasNext() )
	{
		SmartPtr<CDspClient> pUser = itSessions.next().value();
		SmartPtr<CUserStatInfo> pUserStatInfo( new CUserStatInfo );
		pUserStatInfo->m_sUserName = pUser->getAuthHelper().getUserName();
		pUserStatInfo->m_sServiceName = UTF8_2QSTR("Parallels dispatcher service");
		pUserStatInfo->m_sHostName = pUser->getUserHostAddress();
		pUserStatInfo->m_nDurationTime = pUser->getSessionUptimeInSec();
		pUsersStatInfo->m_lstUsersStatInfo.append(pUserStatInfo);
	}
}

void CDspStatCollector::GetIfacesStatInfo(SmartPtr<CIfacesStatInfo> pIfacesStatInfo)
{
	using namespace PrlNet;

	IfStatList _ifaces_list;
	PRL_RESULT nRetCode = getIfaceStatistics(_ifaces_list);
	if (PRL_SUCCEEDED(nRetCode))
	{
		foreach(IfaceStat _iface_stat, _ifaces_list)
		{
			SmartPtr<CIfaceStatInfo> pIfaceStatInfo( new CIfaceStatInfo );
			pIfaceStatInfo->m_sIfaceSystemName = _iface_stat.ifaceName;
			pIfaceStatInfo->m_nInDataSize = _iface_stat.nBytesRcvd;
			pIfaceStatInfo->m_nInPkgsCount = _iface_stat.nPktsRcvd;
			pIfaceStatInfo->m_nOutDataSize = _iface_stat.nBytesSent;
			pIfaceStatInfo->m_nOutPkgsCount = _iface_stat.nPktsSent;
			pIfacesStatInfo->m_lstIfacesStatInfo.append(pIfaceStatInfo);
		}
	}
	else
		WRITE_TRACE(DBG_FATAL, "Couldn't to get net ifaces statistics. Error code: %.8X", nRetCode);
}
