/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2006-2017, Parallels International GmbH
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
/// @file
///		CDspStatisticsGuardTest.cpp
///
/// @author
///		sandro
///
/// @brief
///		Tests fixture class for testing dispatcher host statistics guardian class.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////

#include "CDspStatisticsGuardTest.h"
#include "Dispatcher/Dispatcher/Stat/CDspStatisticsGuard.h"

void CDspStatisticsGuardTest::testStoreCpusStatWithoutPrevValues()
{
	SmartPtr<CCpusStatInfo> pCpusStatInfo( new CCpusStatInfo );
	SmartPtr<CCpuStatInfo> pCpuStatInfo( new CCpuStatInfo(250020, 2034, 41045, 1492608, 26028) );
	pCpusStatInfo->m_lstCpusStatInfo.append(pCpuStatInfo);
	CDspStatisticsGuard::StoreCpusStatInfo(SmartPtr<CCpusStatInfo>(), pCpusStatInfo);
	QVERIFY(pCpusStatInfo->m_lstCpusStatInfo.size() == CDspStatisticsGuard::GetHostStatistics().m_lstCpusStatistics.size());
	for (int i = 0; i < pCpusStatInfo->m_lstCpusStatInfo.size(); ++i)
	{
		SmartPtr<CCpuStatInfo> pCpuStatInfo = pCpusStatInfo->m_lstCpusStatInfo.value(i);
		CCpuStatistics *pCpuStatistics = CDspStatisticsGuard::GetHostStatistics().m_lstCpusStatistics.value(i);
		QVERIFY(pCpuStatistics->getUserTime() == pCpuStatInfo->m_nUser/1000);
		QVERIFY(pCpuStatistics->getSystemTime() == pCpuStatInfo->m_nSystem/1000);
		quint64 nTotalCpuTime = pCpuStatInfo->m_nUser + pCpuStatInfo->m_nNice + pCpuStatInfo->m_nSystem +
															pCpuStatInfo->m_nIdle + pCpuStatInfo->m_nWait;
		QVERIFY(pCpuStatistics->getTotalTime() == nTotalCpuTime/1000);
		QVERIFY(pCpuStatistics->getPercentsUsage() == 50);
	}
}

void CDspStatisticsGuardTest::testStoreCpusStatWithPrevValues()
{
	SmartPtr<CCpusStatInfo> pPrevCpusStatInfo( new CCpusStatInfo );
	SmartPtr<CCpuStatInfo> pPrevCpuStatInfo( new CCpuStatInfo(250020, 2034, 41045, 1492608, 26028) );
	pPrevCpusStatInfo->m_lstCpusStatInfo.append(pPrevCpuStatInfo);
	SmartPtr<CCpusStatInfo> pCpusStatInfo( new CCpusStatInfo );
	SmartPtr<CCpuStatInfo> pCpuStatInfo( new CCpuStatInfo(259877, 2050, 43144, 2132676, 27121) );
	pCpusStatInfo->m_lstCpusStatInfo.append(pCpuStatInfo);

	CDspStatisticsGuard::StoreCpusStatInfo(pPrevCpusStatInfo, pCpusStatInfo);
	for (int i = 0; i < pCpusStatInfo->m_lstCpusStatInfo.size(); ++i)
	{
		SmartPtr<CCpuStatInfo> pCpuStatInfo = pCpusStatInfo->m_lstCpusStatInfo.value(i);
		CCpuStatistics *pCpuStatistics = CDspStatisticsGuard::GetHostStatistics().m_lstCpusStatistics.value(i);
		QVERIFY(pCpuStatistics->getUserTime() == pCpuStatInfo->m_nUser/1000);
		QVERIFY(pCpuStatistics->getSystemTime() == pCpuStatInfo->m_nSystem/1000);
		quint64 nTotalCpuTime = pCpuStatInfo->m_nUser + pCpuStatInfo->m_nNice + pCpuStatInfo->m_nSystem +
															pCpuStatInfo->m_nIdle + pCpuStatInfo->m_nWait;
		QVERIFY(pCpuStatistics->getTotalTime() == nTotalCpuTime/1000);
	}
}

void CDspStatisticsGuardTest::testStoreCpusStatWithSeveralCpus()
{
	SmartPtr<CCpusStatInfo> pCpusStatInfo( new CCpusStatInfo );
	pCpusStatInfo->m_lstCpusStatInfo.append(SmartPtr<CCpuStatInfo>(new CCpuStatInfo));
	pCpusStatInfo->m_lstCpusStatInfo.append(SmartPtr<CCpuStatInfo>(new CCpuStatInfo));
	CDspStatisticsGuard::StoreCpusStatInfo(SmartPtr<CCpusStatInfo>(), pCpusStatInfo);
	QVERIFY(pCpusStatInfo->m_lstCpusStatInfo.size() == CDspStatisticsGuard::GetHostStatistics().m_lstCpusStatistics.size());
}

#define CHECK_DISKS_STAT_INFO\
	QVERIFY(pDisksStatInfo->m_lstDisksStatInfo.size() == CDspStatisticsGuard::GetHostStatistics().m_lstDisksStatistics.size());\
	for (int i = 0; i < pDisksStatInfo->m_lstDisksStatInfo.size(); ++i)\
	{\
		SmartPtr<CDiskStatInfo> pDiskStatInfo = pDisksStatInfo->m_lstDisksStatInfo.value(i);\
		CDiskStatistics *pDiskStatistics = CDspStatisticsGuard::GetHostStatistics().m_lstDisksStatistics.value(i);\
		QVERIFY(pDiskStatistics->getDeviceSystemName() == pDiskStatInfo->m_sSystemName);\
		QVERIFY(pDiskStatistics->getUsageDiskSpace() == pDiskStatInfo->m_nTotalBytes-pDiskStatInfo->m_nFreeBytes);\
		QVERIFY(pDiskStatistics->getFreeDiskSpace() == pDiskStatInfo->m_nFreeBytes);\
		QVERIFY(pDiskStatistics->m_lstPartitionsStatistics.size() == pDiskStatInfo->m_lstPartsStatInfo.size());\
		for (int j = 0; j < pDiskStatInfo->m_lstPartsStatInfo.size(); ++j)\
		{\
			SmartPtr<CDiskPartStatInfo> pDiskPartStatInfo = pDiskStatInfo->m_lstPartsStatInfo.value(i);\
			CDiskPartStatistics *pDiskPartStatistics = pDiskStatistics->m_lstPartitionsStatistics.value(i);\
			QVERIFY(pDiskPartStatistics->getDeviceSystemName() == pDiskPartStatInfo->m_sSystemName);\
			QVERIFY(pDiskPartStatistics->getUsageDiskSpace() == pDiskPartStatInfo->m_nTotalBytes-pDiskPartStatInfo->m_nFreeBytes);\
			QVERIFY(pDiskPartStatistics->getFreeDiskSpace() == pDiskPartStatInfo->m_nFreeBytes);\
		}\
	}

void CDspStatisticsGuardTest::testStoreDisksStat()
{
	SmartPtr<CDisksStatInfo> pDisksStatInfo( new CDisksStatInfo );
	SmartPtr<CDiskStatInfo> pDiskStatInfo( new CDiskStatInfo("/dev/hda", 80*1024*1024*1024LL, 40*1024*1024*1024LL) );
	pDisksStatInfo->m_lstDisksStatInfo.append(pDiskStatInfo);
	SmartPtr<CDiskPartStatInfo> pDiskPartStatInfo1( new CDiskPartStatInfo("/dev/hda1",
                                                          40*1024*1024*1024LL, 20*1024*1024*1024LL) );
	pDiskStatInfo->m_lstPartsStatInfo.append(pDiskPartStatInfo1);
	SmartPtr<CDiskPartStatInfo> pDiskPartStatInfo2( new CDiskPartStatInfo("/dev/hda2",
                                                          40*1024*1024*1024LL, 20*1024*1024*1024LL) );
	pDiskStatInfo->m_lstPartsStatInfo.append(pDiskPartStatInfo2);
	CDspStatisticsGuard::StoreDisksStatInfo(pDisksStatInfo);
	CHECK_DISKS_STAT_INFO
	pDiskStatInfo->m_nFreeBytes = 30*1024*1024*1024LL;
	pDiskPartStatInfo1->m_nFreeBytes = 15*1024*1024*1024LL;
	pDiskPartStatInfo2->m_nFreeBytes = 15*1024*1024*1024LL;
	CDspStatisticsGuard::StoreDisksStatInfo(pDisksStatInfo);
	CHECK_DISKS_STAT_INFO
}

void CDspStatisticsGuardTest::testStoreRamStat()
{
	SmartPtr<CRamStatInfo> pRamStatInfo( new CRamStatInfo(1024*1024*1024LL, 512*1024*1024LL) );
	CDspStatisticsGuard::StoreRamStatInfo(pRamStatInfo);
	QVERIFY(CDspStatisticsGuard::GetHostStatistics().getMemoryStatistics()->getTotalSize() == pRamStatInfo->m_nTotalSize);
	QVERIFY(CDspStatisticsGuard::GetHostStatistics().getMemoryStatistics()->getUsageSize() == pRamStatInfo->m_nTotalSize - pRamStatInfo->m_nFreeSize);
	QVERIFY(CDspStatisticsGuard::GetHostStatistics().getMemoryStatistics()->getFreeSize() == pRamStatInfo->m_nFreeSize);
}

void CDspStatisticsGuardTest::testStoreSwapStat()
{
	SmartPtr<CSwapStatInfo> pSwapStatInfo( new CSwapStatInfo(512*1024*1024LL, 256*1024*1024LL) );
	CDspStatisticsGuard::StoreSwapStatInfo(pSwapStatInfo);
	QVERIFY(CDspStatisticsGuard::GetHostStatistics().getSwapStatistics()->getTotalSize() == pSwapStatInfo->m_nTotalSize);
	QVERIFY(CDspStatisticsGuard::GetHostStatistics().getSwapStatistics()->getUsageSize() == pSwapStatInfo->m_nTotalSize - pSwapStatInfo->m_nFreeSize);
	QVERIFY(CDspStatisticsGuard::GetHostStatistics().getSwapStatistics()->getFreeSize() == pSwapStatInfo->m_nFreeSize);
}

void CDspStatisticsGuardTest::testStoreUptimeStat()
{
	SmartPtr<CUptimeStatInfo> pUptimeStatInfo1( new CUptimeStatInfo(30000LL, 10000LL) );
	CDspStatisticsGuard::StoreUptimeStatInfo(pUptimeStatInfo1);
	QVERIFY(CDspStatisticsGuard::GetHostStatistics().getUptimeStatistics()->getOsUptime() == pUptimeStatInfo1->m_nOsUptimeDelta);
	QVERIFY(CDspStatisticsGuard::GetHostStatistics().getUptimeStatistics()->getDispatcherUptime() == pUptimeStatInfo1->m_nProcessUptime);

	SmartPtr<CUptimeStatInfo> pUptimeStatInfo2( new CUptimeStatInfo(30LL, 10030LL) );
	CDspStatisticsGuard::StoreUptimeStatInfo(pUptimeStatInfo2);
	QVERIFY(CDspStatisticsGuard::GetHostStatistics().getUptimeStatistics()->getOsUptime() == pUptimeStatInfo1->m_nOsUptimeDelta + pUptimeStatInfo2->m_nOsUptimeDelta);
	QVERIFY(CDspStatisticsGuard::GetHostStatistics().getUptimeStatistics()->getDispatcherUptime() == pUptimeStatInfo2->m_nProcessUptime);
}

namespace {
	CProcInfoStatistics *FindProcessStatistics(quint32 nProcId)
	{
		foreach(CProcInfoStatistics *pProcInfoStatistics, CDspStatisticsGuard::GetHostStatistics().m_lstProcessesStatistics)
		{
			if (pProcInfoStatistics->getProcId() == nProcId)
				return (pProcInfoStatistics);
		}
		return (NULL);
	}
}

#define CHECK_PROCS_STAT_INFO\
	QVERIFY(pProcsStatInfo->m_lstProcsStatInfo.size() == CDspStatisticsGuard::GetHostStatistics().m_lstProcessesStatistics.size());\
	foreach(SmartPtr<CProcStatInfo> pProcStatInfo, pProcsStatInfo->m_lstProcsStatInfo)\
	{\
		CProcInfoStatistics *pProcInfoStatistics = FindProcessStatistics(pProcStatInfo->m_nId);\
		QVERIFY(pProcInfoStatistics != NULL);\
		QVERIFY(pProcInfoStatistics->getCommandName() == pProcStatInfo->m_sCommandName);\
		QVERIFY(pProcInfoStatistics->getOwnerUser() == pProcStatInfo->m_sOwnerUser);\
		QVERIFY(pProcInfoStatistics->getTotalMemUsage() == pProcStatInfo->m_nTotalMemUsage);\
		QVERIFY(pProcInfoStatistics->getRealMemUsage() == pProcStatInfo->m_nRealMemUsage);\
		QVERIFY(pProcInfoStatistics->getVirtualMemUsage() == pProcStatInfo->m_nVirtMemUsage);\
		QVERIFY(pProcInfoStatistics->getStartTime() == pProcStatInfo->m_nStartTime);\
		QVERIFY(pProcInfoStatistics->getTotalTime() == pProcStatInfo->m_nTotalTime);\
		QVERIFY(pProcInfoStatistics->getUserTime() == pProcStatInfo->m_nUserTime);\
		QVERIFY(pProcInfoStatistics->getSystemTime() == pProcStatInfo->m_nSystemTime);\
		QVERIFY(pProcInfoStatistics->getState() == pProcStatInfo->m_nState);\
	}

void CDspStatisticsGuardTest::testStoreProcessesStat()
{
	SmartPtr<CProcsStatInfo> pProcsStatInfo( new CProcsStatInfo );

	SmartPtr<CProcStatInfo> pProcStatInfo1( new CProcStatInfo("cmd1", 1234, "root", 40*1024*1024LL,
                20*1024*1024LL, 20*1024*1024LL, 156765651765LL, 4000LL, 2800LL, 1200LL, PPS_PROC_RUN) );
	pProcsStatInfo->m_lstProcsStatInfo.append(pProcStatInfo1);

	SmartPtr<CProcStatInfo> pProcStatInfo2( new CProcStatInfo("cmd2", 1235, "root", 20*1024*1024LL,
                10*1024*1024LL, 10*1024*1024LL, 156765651965LL, 3800LL, 2800LL, 1000LL, PPS_PROC_SLEEP) );
	pProcsStatInfo->m_lstProcsStatInfo.append(pProcStatInfo2);

	SmartPtr<CProcStatInfo> pProcStatInfo3( new CProcStatInfo("cmd1", 1236, "root", 10*1024*1024LL,
                5*1024*1024LL, 5*1024*1024LL, 156765651985LL, 3780LL, 2780LL, 1000LL, PPS_PROC_IDLE) );
	pProcsStatInfo->m_lstProcsStatInfo.append(pProcStatInfo3);

	CDspStatisticsGuard::StoreProcsStatInfo(pProcsStatInfo, 1000);
	CHECK_PROCS_STAT_INFO

	pProcsStatInfo->m_lstProcsStatInfo.removeFirst();
	SmartPtr<CProcStatInfo> pProcStatInfo4( new CProcStatInfo("cmd3", 1237, "root", 10*1024*1024LL, 5*1024*1024LL,
              5*1024*1024LL, 156765651995LL, 3770LL, 2770LL, 1000LL, PPS_PROC_ZOMBIE) );
	pProcsStatInfo->m_lstProcsStatInfo.append(pProcStatInfo4);
	CDspStatisticsGuard::StoreProcsStatInfo(pProcsStatInfo, 1000);
	CHECK_PROCS_STAT_INFO

	pProcStatInfo2->m_sCommandName = "cmd5";
	pProcStatInfo2->m_sOwnerUser = "user1";
	pProcStatInfo2->m_nStartTime = 156765652500LL;
	pProcStatInfo3->m_nTotalMemUsage = 20*1024*1024LL;
	pProcStatInfo3->m_nRealMemUsage = 10*1024*1024LL;
	pProcStatInfo3->m_nVirtMemUsage = 10*1024*1024LL;
	pProcStatInfo4->m_nTotalTime = 4270LL;
	pProcStatInfo4->m_nUserTime = 3070LL;
	pProcStatInfo4->m_nSystemTime = 1200LL;
	pProcStatInfo4->m_nState = PPS_PROC_STOP;
	CDspStatisticsGuard::StoreProcsStatInfo(pProcsStatInfo, 1000);
	CHECK_PROCS_STAT_INFO
}

#define CHECK_USERS_STAT_INFO\
	QVERIFY(pUsersStatInfo->m_lstUsersStatInfo.size() == CDspStatisticsGuard::GetHostStatistics().m_lstUsersStatistics.size());\
	for(int i = 0; i < pUsersStatInfo->m_lstUsersStatInfo.size(); ++i)\
	{\
		SmartPtr<CUserStatInfo> pUserStatInfo = pUsersStatInfo->m_lstUsersStatInfo.value(i);\
		CUserStatistics *pUserStatistics = CDspStatisticsGuard::GetHostStatistics().m_lstUsersStatistics.value(i);\
		QVERIFY(pUserStatistics->getUserName() == pUserStatInfo->m_sUserName);\
		QVERIFY(pUserStatistics->getServiceName() == pUserStatInfo->m_sServiceName);\
		QVERIFY(pUserStatistics->getHostName() == pUserStatInfo->m_sHostName);\
		QVERIFY(pUserStatistics->getSessionTime() == pUserStatInfo->m_nDurationTime);\
	}

void CDspStatisticsGuardTest::testStoreUsersStat()
{
	SmartPtr<CUsersStatInfo> pUsersStatInfo( new CUsersStatInfo );

	pUsersStatInfo->m_lstUsersStatInfo.append(SmartPtr<CUserStatInfo>(
                            new CUserStatInfo("user1", "service1", "host1", 100LL)));
	pUsersStatInfo->m_lstUsersStatInfo.append(SmartPtr<CUserStatInfo>(
                            new CUserStatInfo("user1", "service1", "host1", 50LL)));
	pUsersStatInfo->m_lstUsersStatInfo.append(SmartPtr<CUserStatInfo>(
                            new CUserStatInfo("user2", "service2", "host2", 200LL)));

	CDspStatisticsGuard::StoreUsersStatInfo(pUsersStatInfo);
	CHECK_USERS_STAT_INFO

	pUsersStatInfo->m_lstUsersStatInfo.removeFirst();
	pUsersStatInfo->m_lstUsersStatInfo.append(SmartPtr<CUserStatInfo>(
                            new CUserStatInfo("user3", "service3", "host3", 20LL)));
	CDspStatisticsGuard::StoreUsersStatInfo(pUsersStatInfo);
	CHECK_USERS_STAT_INFO
}

#define CHECK_IFACES_STAT_INFO\
	QVERIFY(pIfacesStatInfo->m_lstIfacesStatInfo.size() == CDspStatisticsGuard::GetHostStatistics().m_lstNetIfacesStatistics.size());\
	for(int i = 0; i < pIfacesStatInfo->m_lstIfacesStatInfo.size(); ++i)\
	{\
		SmartPtr<CIfaceStatInfo> pIfaceStatInfo = pIfacesStatInfo->m_lstIfacesStatInfo.value(i);\
		CNetIfaceStatistics *pIfaceStatistics = CDspStatisticsGuard::GetHostStatistics().m_lstNetIfacesStatistics.value(i);\
		QVERIFY(pIfaceStatistics->getIfaceSystemName() == pIfaceStatInfo->m_sIfaceSystemName);\
		QVERIFY(pIfaceStatistics->getInDataSize() == pIfaceStatInfo->m_nInDataSize);\
		QVERIFY(pIfaceStatistics->getInPkgsCount() == pIfaceStatInfo->m_nInPkgsCount);\
		QVERIFY(pIfaceStatistics->getOutDataSize() == pIfaceStatInfo->m_nOutDataSize);\
		QVERIFY(pIfaceStatistics->getOutPkgsCount() == pIfaceStatInfo->m_nOutPkgsCount);\
	}

void CDspStatisticsGuardTest::testStoreIfacesStat()
{
	SmartPtr<CIfacesStatInfo> pIfacesStatInfo( new CIfacesStatInfo );

	pIfacesStatInfo->m_lstIfacesStatInfo.append(SmartPtr<CIfaceStatInfo>(
                          new CIfaceStatInfo("eth0", 10000000LL, 100LL, 200000000LL, 200LL)));
	pIfacesStatInfo->m_lstIfacesStatInfo.append(SmartPtr<CIfaceStatInfo>(
                          new CIfaceStatInfo("vnic0", 30000000LL, 300LL, 600000000LL, 600LL)));

	CDspStatisticsGuard::StoreIfacesStatInfo(pIfacesStatInfo);
	CHECK_IFACES_STAT_INFO

	pIfacesStatInfo->m_lstIfacesStatInfo.removeFirst();
	pIfacesStatInfo->m_lstIfacesStatInfo.append(SmartPtr<CIfaceStatInfo>(
                          new CIfaceStatInfo("loopback", 30000000LL, 300LL, 600000000LL, 600LL)));
	CDspStatisticsGuard::StoreIfacesStatInfo(pIfacesStatInfo);
	CHECK_IFACES_STAT_INFO
}
