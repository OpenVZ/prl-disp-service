///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspStatisticsGuard.cpp
///
/// Statistics guardian implementation
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

#include "CDspStatisticsGuard.h"
#include <prlcommon/Logging/Logging.h>
#include "CDspSystemInfo.h"
#include <prlcommon/Std/PrlAssert.h>

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

#include  <math.h>
#include  <stdlib.h>

#ifdef _WIN_
#	include  <windows.h>
#endif

#ifdef _LIN_
#	include <sys/time.h>
#	include <errno.h>
#endif

CSystemStatistics CDspStatisticsGuard::g_HostStatistics;
QMutex CDspStatisticsGuard::g_HostStatisticsMutex(QMutex::Recursive);

QString CDspStatisticsGuard::GetStatisticsStringRepresentation()
{
	QMutexLocker _lock(&g_HostStatisticsMutex);
	return (g_HostStatistics.toString());
}

QMutex *CDspStatisticsGuard::GetSynchroObject()
{
	return (&g_HostStatisticsMutex);
}

CSystemStatistics &CDspStatisticsGuard::GetHostStatistics()
{
	return (g_HostStatistics);
}

namespace {
/** Calculatess CPU total time */
quint64 CalcCpuTotalTime(SmartPtr<CCpuStatInfo> pCpuStatInfo)
{
	return (pCpuStatInfo->m_nUser + pCpuStatInfo->m_nNice + pCpuStatInfo->m_nSystem +	pCpuStatInfo->m_nIdle +
					pCpuStatInfo->m_nWait);
}

}

void CDspStatisticsGuard::StoreCpusStatInfo(SmartPtr<CCpusStatInfo> pPrevStat, SmartPtr<CCpusStatInfo> pCurrStat)
{
	//PRL_ASSERT( pPrevStat );
	PRL_ASSERT( pCurrStat );

	QMutexLocker _lock(&g_HostStatisticsMutex);
	for (int i = 0; i < pCurrStat->m_lstCpusStatInfo.size(); ++i)
	{
		SmartPtr<CCpuStatInfo> pCpuStatInfo = pCurrStat->m_lstCpusStatInfo.value(i);
		if (g_HostStatistics.m_lstCpusStatistics.size() == i)//expand CPUs statistics list if necessary
			g_HostStatistics.m_lstCpusStatistics.append(new CCpuStatistics);
		CCpuStatistics *pCpuStatistics = g_HostStatistics.m_lstCpusStatistics.value(i);
		quint64 nPercentsUsage = 50;
		pCpuStatistics->setUserTime(pCpuStatInfo->m_nUser/1000);
		pCpuStatistics->setSystemTime(pCpuStatInfo->m_nSystem/1000);
		pCpuStatistics->setTotalTime(CalcCpuTotalTime(pCpuStatInfo)/1000);
		if(pPrevStat)
		{
			SmartPtr<CCpuStatInfo> pPrevCpuStatInfo = pPrevStat->m_lstCpusStatInfo.value(i);
			// #424351
			if(!pPrevCpuStatInfo)
				continue;
			quint64 nTotalTimeDelta = CalcCpuTotalTime(pCpuStatInfo) - CalcCpuTotalTime(pPrevCpuStatInfo);
			quint64 nUserTimeDelta = pCpuStatInfo->m_nUser - pPrevCpuStatInfo->m_nUser;
			quint64 nNiceTimeDelta = pCpuStatInfo->m_nNice - pPrevCpuStatInfo->m_nNice;
			quint64 nSystemTimeDelta = pCpuStatInfo->m_nSystem - pPrevCpuStatInfo->m_nSystem;
			quint64 nWaitTimeDelta = pCpuStatInfo->m_nWait - pPrevCpuStatInfo->m_nWait;
			nPercentsUsage = quint32(double(nUserTimeDelta + nNiceTimeDelta + nSystemTimeDelta + nWaitTimeDelta)/double(nTotalTimeDelta)*100);
		}
		pCpuStatistics->setPercentsUsage(nPercentsUsage);
	}
}

#define GET_STATISTICS_ELEMENT(elem_type, elem_var, elems_container, compare_method, compare_id_value, set_id_method)\
	elem_type *elem_var = NULL;\
	foreach(elem_type *pStatElem, elems_container)\
	{\
		if (pStatElem->compare_method() == compare_id_value)\
		{\
			elem_var = pStatElem;\
			break;\
		}\
	}\
	if (!elem_var)\
	{\
		elem_var = new elem_type;\
		elem_var->set_id_method(compare_id_value);\
		elems_container.append(elem_var);\
	}

void CDspStatisticsGuard::StoreDisksStatInfo(SmartPtr<CDisksStatInfo> pDisksStatInfo)
{
	QMutexLocker _lock(&g_HostStatisticsMutex);
	foreach(SmartPtr<CDiskStatInfo> pDiskStatInfo, pDisksStatInfo->m_lstDisksStatInfo)
	{
		GET_STATISTICS_ELEMENT(CDiskStatistics, pDiskStatistics, g_HostStatistics.m_lstDisksStatistics, getDeviceSystemName, pDiskStatInfo->m_sSystemName, setDeviceSystemName)
		pDiskStatistics->setUsageDiskSpace(pDiskStatInfo->m_nTotalBytes - pDiskStatInfo->m_nFreeBytes);
		pDiskStatistics->setFreeDiskSpace(pDiskStatInfo->m_nFreeBytes);
		foreach(SmartPtr<CDiskPartStatInfo> pDiskPartStatInfo, pDiskStatInfo->m_lstPartsStatInfo)
		{
			GET_STATISTICS_ELEMENT(CDiskPartStatistics, pDiskPartStatistics, pDiskStatistics->m_lstPartitionsStatistics, getDeviceSystemName, pDiskPartStatInfo->m_sSystemName, setDeviceSystemName)
			pDiskPartStatistics->setUsageDiskSpace(pDiskPartStatInfo->m_nTotalBytes - pDiskPartStatInfo->m_nFreeBytes);
			pDiskPartStatistics->setFreeDiskSpace(pDiskPartStatInfo->m_nFreeBytes);
		}
	}
}

void CDspStatisticsGuard::StoreRamStatInfo(SmartPtr<CRamStatInfo> pRamStatInfo)
{
	QMutexLocker _lock(&g_HostStatisticsMutex);
	g_HostStatistics.getMemoryStatistics()->setTotalSize(pRamStatInfo->m_nTotalSize);
	g_HostStatistics.getMemoryStatistics()->setUsageSize(pRamStatInfo->m_nTotalSize - pRamStatInfo->m_nFreeSize);
	g_HostStatistics.getMemoryStatistics()->setFreeSize(pRamStatInfo->m_nFreeSize);
}

void CDspStatisticsGuard::StoreSwapStatInfo(SmartPtr<CSwapStatInfo> pSwapStatInfo)
{
	QMutexLocker _lock(&g_HostStatisticsMutex);
	g_HostStatistics.getSwapStatistics()->setTotalSize(pSwapStatInfo->m_nTotalSize);
	g_HostStatistics.getSwapStatistics()->setUsageSize(pSwapStatInfo->m_nTotalSize - pSwapStatInfo->m_nFreeSize);
	g_HostStatistics.getSwapStatistics()->setFreeSize(pSwapStatInfo->m_nFreeSize);
}

void CDspStatisticsGuard::StoreUptimeStatInfo(SmartPtr<CUptimeStatInfo> pUptimeStatInfo)
{
	QMutexLocker _lock(&g_HostStatisticsMutex);
	g_HostStatistics.getUptimeStatistics()->setOsUptime(g_HostStatistics.getUptimeStatistics()->getOsUptime() + pUptimeStatInfo->m_nOsUptimeDelta);
	g_HostStatistics.getUptimeStatistics()->setDispatcherUptime(pUptimeStatInfo->m_nProcessUptime);
}

namespace {
/** Checks whether statistics element presents in stat info set */
bool ProcStatPresents(SmartPtr<CProcsStatInfo> pProcsStatInfo, CProcInfoStatistics *pProcInfoStatistics)
{
	foreach(SmartPtr<CProcStatInfo> pProcStatInfo, pProcsStatInfo->m_lstProcsStatInfo)
	{
		if (pProcInfoStatistics->getProcId() == pProcStatInfo->m_nId)
			return (true);
	}
	return (false);
}
/** Returns process statistics element by specified process id (creates it if necessary) */
CProcInfoStatistics *GetStatisticsElem(quint32 nProcId, QList<CProcInfoStatistics *> &lstProcessesStatistics)
{
	foreach(CProcInfoStatistics *pProcInfoStatistics, lstProcessesStatistics)
	{
		if (pProcInfoStatistics->getProcId() == nProcId)
			return (pProcInfoStatistics);
	}
	CProcInfoStatistics *pProcInfoStatistics = new CProcInfoStatistics;
	pProcInfoStatistics->setProcId(nProcId);
	lstProcessesStatistics.append(pProcInfoStatistics);
	return (pProcInfoStatistics);
}

/** Returns process statistics element by specified process id (creates it if necessary) */
quint32 CalcPercentsUsage(quint64 nTotalTime, quint64 nTotalTimePrev, quint32 nCpus, quint32 nDeltaMs)
{
	if ( !nCpus || !nDeltaMs )//Prevent possibilities of devision by zero
		return 0;
	quint64 nTotalTimeDelta = nTotalTime - nTotalTimePrev;
	quint32 nPercentsUsage = quint32( (100 * nTotalTimeDelta)/(nDeltaMs*nCpus) );
	return nPercentsUsage;
}


}
void CDspStatisticsGuard::StoreProcsStatInfo(SmartPtr<CProcsStatInfo> pProcsStatInfo, quint32 nDeltaMs)
{
	PRL_ASSERT(pProcsStatInfo);

	QMutexLocker _lock(&g_HostStatisticsMutex);
	QList<CProcInfoStatistics *>::iterator _proc_it = g_HostStatistics.m_lstProcessesStatistics.begin();
	while(_proc_it != g_HostStatistics.m_lstProcessesStatistics.end())
	{
		if (!ProcStatPresents(pProcsStatInfo, *_proc_it))
		{
			delete *_proc_it;
			_proc_it = g_HostStatistics.m_lstProcessesStatistics.erase(_proc_it);
		}
		else
			++_proc_it;
	}
	quint32 nCpus = CDspSystemInfo::GetNumberOfProcessors();
	foreach(SmartPtr<CProcStatInfo> pProcStatInfo, pProcsStatInfo->m_lstProcsStatInfo)
	{
		CProcInfoStatistics *pProcInfoStatistics = GetStatisticsElem(pProcStatInfo->m_nId, g_HostStatistics.m_lstProcessesStatistics);
		quint32 nPercentsUsage = 0;
		quint64 nTotalTimePrev = pProcInfoStatistics->getTotalTime();
		nPercentsUsage = (nTotalTimePrev != 0) ? CalcPercentsUsage(pProcStatInfo->m_nTotalTime, pProcInfoStatistics->getTotalTime(), nCpus, nDeltaMs) : 0;
		pProcInfoStatistics->setPercentsUsage(nPercentsUsage);
		pProcInfoStatistics->setCommandName(pProcStatInfo->m_sCommandName);
		pProcInfoStatistics->setOwnerUser(pProcStatInfo->m_sOwnerUser);
		pProcInfoStatistics->setTotalMemUsage(pProcStatInfo->m_nTotalMemUsage);
		pProcInfoStatistics->setRealMemUsage(pProcStatInfo->m_nRealMemUsage);
		pProcInfoStatistics->setVirtualMemUsage(pProcStatInfo->m_nVirtMemUsage);
		pProcInfoStatistics->setStartTime(pProcStatInfo->m_nStartTime);
		pProcInfoStatistics->setTotalTime(pProcStatInfo->m_nTotalTime);
		pProcInfoStatistics->setUserTime(pProcStatInfo->m_nUserTime);
		pProcInfoStatistics->setSystemTime(pProcStatInfo->m_nSystemTime);
		pProcInfoStatistics->setState(pProcStatInfo->m_nState);
	}
}

void CDspStatisticsGuard::StoreUsersStatInfo(SmartPtr<CUsersStatInfo> pUsersStatInfo)
{
	QMutexLocker _lock(&g_HostStatisticsMutex);
	foreach(CUserStatistics *pUserStatistics, g_HostStatistics.m_lstUsersStatistics)
		delete pUserStatistics;
	g_HostStatistics.m_lstUsersStatistics.clear();
	foreach(SmartPtr<CUserStatInfo> pUserStatInfo, pUsersStatInfo->m_lstUsersStatInfo)
	{
		CUserStatistics *pUserStatistics = new CUserStatistics;
		pUserStatistics->setUserName(pUserStatInfo->m_sUserName);
		pUserStatistics->setServiceName(pUserStatInfo->m_sServiceName);
		pUserStatistics->setHostName(pUserStatInfo->m_sHostName);
		pUserStatistics->setSessionTime(pUserStatInfo->m_nDurationTime);
		g_HostStatistics.m_lstUsersStatistics.append(pUserStatistics);
	}
}

void CDspStatisticsGuard::StoreIfacesStatInfo(SmartPtr<CIfacesStatInfo> pIfacesStatInfo)
{
	QMutexLocker _lock(&g_HostStatisticsMutex);
	foreach(CNetIfaceStatistics *pIfaceStatistics, g_HostStatistics.m_lstNetIfacesStatistics)
		delete pIfaceStatistics;
	g_HostStatistics.m_lstNetIfacesStatistics.clear();
	foreach(SmartPtr<CIfaceStatInfo> pIfaceStatInfo, pIfacesStatInfo->m_lstIfacesStatInfo)
	{
		CNetIfaceStatistics *pIfaceStatistics = new CNetIfaceStatistics;
		pIfaceStatistics->setIfaceSystemName(pIfaceStatInfo->m_sIfaceSystemName);
		pIfaceStatistics->setInDataSize(pIfaceStatInfo->m_nInDataSize);
		pIfaceStatistics->setInPkgsCount(pIfaceStatInfo->m_nInPkgsCount);
		pIfaceStatistics->setOutDataSize(pIfaceStatInfo->m_nOutDataSize);
		pIfaceStatistics->setOutPkgsCount(pIfaceStatInfo->m_nOutPkgsCount);
		g_HostStatistics.m_lstNetIfacesStatistics.append(pIfaceStatistics);
	}
}

