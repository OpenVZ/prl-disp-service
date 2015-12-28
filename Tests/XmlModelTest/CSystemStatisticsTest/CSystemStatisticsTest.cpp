///////////////////////////////////////////////////////////////////////////////
///
/// @file CSystemStatisticsTest.cpp
///
/// Tests fixture class for testing CSystemStatistics class functionality.
///
/// @author sandro
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
///////////////////////////////////////////////////////////////////////////////

#include "CSystemStatisticsTest.h"
#include "XmlModel/HostHardwareInfo/CSystemStatistics.h"
#include <prlcommon/Logging/Logging.h>

#ifndef _WIN_
#       ifndef ULLONG_MAX
#          define ULLONG_MAX   18446744073709551615ULL
#       endif
#endif

void CSystemStatisticsTest::testFormingCpusStatistics()
{
	CSystemStatistics _source_stat;
	CCpuStatistics *pCpuStat = new CCpuStatistics;
	pCpuStat->setPercentsUsage(35);
	pCpuStat->setTotalTime(57632);
	pCpuStat->setUserTime(57000);
	pCpuStat->setSystemTime(632);
	_source_stat.m_lstCpusStatistics.append(pCpuStat);

	CSystemStatistics _dest_stat;
	_dest_stat.fromString(_source_stat.toString());
	QCOMPARE(_dest_stat.m_uiRcInit, PRL_ERR_SUCCESS);

	QVERIFY(_dest_stat.m_lstCpusStatistics.size() == _source_stat.m_lstCpusStatistics.size());
	for(int i = 0; i < _source_stat.m_lstCpusStatistics.size(); ++i)
	{
		CCpuStatistics *pSourceCpu = _source_stat.m_lstCpusStatistics.value(i);
		CCpuStatistics *pDestCpu = _dest_stat.m_lstCpusStatistics.value(i);
		QVERIFY(*pSourceCpu == *pDestCpu);
	}
}

void CSystemStatisticsTest::testFormingCpusStatisticsBoundValues()
{
	CSystemStatistics _source_stat;
	CCpuStatistics *pCpuStat = new CCpuStatistics;
	pCpuStat->setPercentsUsage(100);
	pCpuStat->setTotalTime(ULLONG_MAX);
	pCpuStat->setUserTime(ULLONG_MAX);
	pCpuStat->setSystemTime(ULLONG_MAX);
	_source_stat.m_lstCpusStatistics.append(pCpuStat);

	CSystemStatistics _dest_stat;
	_dest_stat.fromString(_source_stat.toString());
	QCOMPARE(_dest_stat.m_uiRcInit, PRL_ERR_SUCCESS);

	QVERIFY(_dest_stat.m_lstCpusStatistics.size() == _source_stat.m_lstCpusStatistics.size());
	for(int i = 0; i < _source_stat.m_lstCpusStatistics.size(); ++i)
	{
		CCpuStatistics *pSourceCpu = _source_stat.m_lstCpusStatistics.value(i);
		CCpuStatistics *pDestCpu = _dest_stat.m_lstCpusStatistics.value(i);
		QVERIFY(*pSourceCpu == *pDestCpu);
	}
}

void CSystemStatisticsTest::testFormingDisksStatistics()
{
	CSystemStatistics _source_stat;
	CDiskStatistics *pDiskStat = new CDiskStatistics;

	CDiskPartStatistics *pDiskPartition = new CDiskPartStatistics;
	pDiskPartition->setDeviceSystemName("/dev/hda1");
	pDiskPartition->setUsageDiskSpace(43563456354LL);
	pDiskPartition->setFreeDiskSpace(454545463LL);
	pDiskStat->m_lstPartitionsStatistics.append(pDiskPartition);

	pDiskStat->setDeviceSystemName("/dev/hda");
	pDiskStat->setUsageDiskSpace(37637463783LL);
	pDiskStat->setFreeDiskSpace(4676473736353LL);
	_source_stat.m_lstDisksStatistics.append(pDiskStat);

	CSystemStatistics _dest_stat;
	_dest_stat.fromString(_source_stat.toString());
	QCOMPARE(_dest_stat.m_uiRcInit, PRL_ERR_SUCCESS);

	QVERIFY(_dest_stat.m_lstDisksStatistics.size() == _source_stat.m_lstDisksStatistics.size());
	for(int i = 0; i < _source_stat.m_lstDisksStatistics.size(); ++i)
	{
		CDiskStatistics *pSourceDisk = _source_stat.m_lstDisksStatistics.value(i);
		CDiskStatistics *pDestDisk = _dest_stat.m_lstDisksStatistics.value(i);
		QVERIFY(*pSourceDisk == *pDestDisk);
	}
}

void CSystemStatisticsTest::testFormingDisksStatisticsBoundValues()
{
	CSystemStatistics _source_stat;
	CDiskStatistics *pDiskStat = new CDiskStatistics;

	CDiskPartStatistics *pDiskPartition = new CDiskPartStatistics;
	pDiskPartition->setDeviceSystemName("/dev/hda1");
	pDiskPartition->setUsageDiskSpace(ULLONG_MAX);
	pDiskPartition->setFreeDiskSpace(ULLONG_MAX);
	pDiskStat->m_lstPartitionsStatistics.append(pDiskPartition);

	pDiskStat->setDeviceSystemName("/dev/hda");
	pDiskStat->setUsageDiskSpace(ULLONG_MAX);
	pDiskStat->setFreeDiskSpace(ULLONG_MAX);
	_source_stat.m_lstDisksStatistics.append(pDiskStat);

	CSystemStatistics _dest_stat;
	_dest_stat.fromString(_source_stat.toString());
	QCOMPARE(_dest_stat.m_uiRcInit, PRL_ERR_SUCCESS);

	QVERIFY(_dest_stat.m_lstDisksStatistics.size() == _source_stat.m_lstDisksStatistics.size());
	for(int i = 0; i < _source_stat.m_lstDisksStatistics.size(); ++i)
	{
		CDiskStatistics *pSourceDisk = _source_stat.m_lstDisksStatistics.value(i);
		CDiskStatistics *pDestDisk = _dest_stat.m_lstDisksStatistics.value(i);
		QVERIFY(*pSourceDisk == *pDestDisk);
	}
}

void CSystemStatisticsTest::testFormingMemoryStatistics()
{
	CSystemStatistics _source_stat;
	_source_stat.getMemoryStatistics()->setTotalSize(43876234876347638LL);
	_source_stat.getMemoryStatistics()->setUsageSize(37637463783LL);
	_source_stat.getMemoryStatistics()->setFreeSize(4676473736353LL);
	_source_stat.getMemoryStatistics()->setRealSize(3456456733243LL);

	CSystemStatistics _dest_stat(_source_stat.toString());
	QCOMPARE(_dest_stat.m_uiRcInit, PRL_ERR_SUCCESS);

	QVERIFY(*_dest_stat.getMemoryStatistics() == *_source_stat.getMemoryStatistics());
}

void CSystemStatisticsTest::testFormingMemoryStatisticsBoundValues()
{
	CSystemStatistics _source_stat;
	_source_stat.getMemoryStatistics()->setTotalSize(ULLONG_MAX);
	_source_stat.getMemoryStatistics()->setUsageSize(ULLONG_MAX);
	_source_stat.getMemoryStatistics()->setFreeSize(ULLONG_MAX);
	_source_stat.getMemoryStatistics()->setRealSize(ULLONG_MAX);

	CSystemStatistics _dest_stat(_source_stat.toString());
	QCOMPARE(_dest_stat.m_uiRcInit, PRL_ERR_SUCCESS);

	QVERIFY(*_dest_stat.getMemoryStatistics() == *_source_stat.getMemoryStatistics());
}

void CSystemStatisticsTest::testFormingSwapStatistics()
{
	CSystemStatistics _source_stat;
	_source_stat.getSwapStatistics()->setTotalSize(43876234876347638LL);
	_source_stat.getSwapStatistics()->setUsageSize(37637463783LL);
	_source_stat.getSwapStatistics()->setFreeSize(4676473736353LL);

	CSystemStatistics _dest_stat(_source_stat.toString());
	QCOMPARE(_dest_stat.m_uiRcInit, PRL_ERR_SUCCESS);

	QVERIFY(*_dest_stat.getSwapStatistics() == *_source_stat.getSwapStatistics());
}

void CSystemStatisticsTest::testFormingSwapStatisticsBoundValues()
{
	CSystemStatistics _source_stat;
	_source_stat.getSwapStatistics()->setTotalSize(ULLONG_MAX);
	_source_stat.getSwapStatistics()->setUsageSize(ULLONG_MAX);
	_source_stat.getSwapStatistics()->setFreeSize(ULLONG_MAX);

	CSystemStatistics _dest_stat(_source_stat.toString());
	QCOMPARE(_dest_stat.m_uiRcInit, PRL_ERR_SUCCESS);

	QVERIFY(*_dest_stat.getSwapStatistics() == *_source_stat.getSwapStatistics());
}

void CSystemStatisticsTest::testFormingUptimeStatistics()
{
	CSystemStatistics _source_stat;
	_source_stat.getUptimeStatistics()->setOsUptime(43876234876347638LL);
	_source_stat.getUptimeStatistics()->setDispatcherUptime(37637463783LL);

	CSystemStatistics _dest_stat(_source_stat.toString());
	QCOMPARE(_dest_stat.m_uiRcInit, PRL_ERR_SUCCESS);

	QVERIFY(*_dest_stat.getUptimeStatistics() == *_source_stat.getUptimeStatistics());
}

void CSystemStatisticsTest::testFormingUptimeStatisticsBoundValues()
{
	CSystemStatistics _source_stat;
	_source_stat.getUptimeStatistics()->setOsUptime(ULLONG_MAX);
	_source_stat.getUptimeStatistics()->setDispatcherUptime(ULLONG_MAX);

	CSystemStatistics _dest_stat(_source_stat);

	QVERIFY(*_dest_stat.getUptimeStatistics() == *_source_stat.getUptimeStatistics());
}

void CSystemStatisticsTest::testFormingProcInfoStatistics()
{
	CSystemStatistics _source_stat;
	CProcInfoStatistics *pProcInfoStat = new CProcInfoStatistics;
	pProcInfoStat->setCommandName("prl_disp_service -e --mode server");
	pProcInfoStat->setProcId(1024);
	pProcInfoStat->setOwnerUser("root");
	pProcInfoStat->setTotalMemUsage(47645646465654LL);
	pProcInfoStat->setRealMemUsage(47645646465654LL);
	pProcInfoStat->setVirtualMemUsage(47645646465654LL);
	pProcInfoStat->setStartTime(47645646465654LL);
	pProcInfoStat->setTotalTime(47645646465654LL);
	pProcInfoStat->setUserTime(47645646465654LL);
	pProcInfoStat->setSystemTime(47645646465654LL);
	pProcInfoStat->setState(PPS_PROC_RUN);
	_source_stat.m_lstProcessesStatistics.append(pProcInfoStat);

	CSystemStatistics _dest_stat(_source_stat.toString());
	QCOMPARE(_dest_stat.m_uiRcInit, PRL_ERR_SUCCESS);

	QVERIFY(_dest_stat.m_lstProcessesStatistics.size() == _source_stat.m_lstProcessesStatistics.size());
	for(int i = 0; i < _source_stat.m_lstProcessesStatistics.size(); ++i)
	{
		CProcInfoStatistics *pSourceProcInfo = _source_stat.m_lstProcessesStatistics.value(i);
		CProcInfoStatistics *pDestProcInfo = _dest_stat.m_lstProcessesStatistics.value(i);
		QVERIFY(*pSourceProcInfo == *pDestProcInfo);
	}
}

void CSystemStatisticsTest::testFormingProcInfoStatisticsBoundValues()
{
	CSystemStatistics _source_stat;
	CProcInfoStatistics *pProcInfoStat = new CProcInfoStatistics;
	pProcInfoStat->setCommandName("prl_disp_service -e --mode server");
	pProcInfoStat->setProcId(UINT_MAX);
	pProcInfoStat->setOwnerUser("prl_unit_test_user@parallels.ru");
	pProcInfoStat->setTotalMemUsage(ULLONG_MAX);
	pProcInfoStat->setRealMemUsage(ULLONG_MAX);
	pProcInfoStat->setVirtualMemUsage(ULLONG_MAX);
	pProcInfoStat->setStartTime(ULLONG_MAX);
	pProcInfoStat->setTotalTime(ULLONG_MAX);
	pProcInfoStat->setUserTime(ULLONG_MAX);
	pProcInfoStat->setSystemTime(ULLONG_MAX);
	pProcInfoStat->setState(PPS_PROC_IDLE);
	_source_stat.m_lstProcessesStatistics.append(pProcInfoStat);

	CSystemStatistics _dest_stat(_source_stat.toString());
	QCOMPARE(_dest_stat.m_uiRcInit, PRL_ERR_SUCCESS);

	QVERIFY(_dest_stat.m_lstProcessesStatistics.size() == _source_stat.m_lstProcessesStatistics.size());
	for(int i = 0; i < _source_stat.m_lstProcessesStatistics.size(); ++i)
	{
		CProcInfoStatistics *pSourceProcInfo = _source_stat.m_lstProcessesStatistics.value(i);
		CProcInfoStatistics *pDestProcInfo = _dest_stat.m_lstProcessesStatistics.value(i);
		QVERIFY(*pSourceProcInfo == *pDestProcInfo);
	}
}

void CSystemStatisticsTest::testFormingNetIfacesStatistics()
{
	CSystemStatistics _source_stat;
	CNetIfaceStatistics *pNetIfaceStat = new CNetIfaceStatistics;
	pNetIfaceStat->setIfaceSystemName("/dev/hda");
	pNetIfaceStat->setInDataSize(37637463783LL);
	pNetIfaceStat->setOutDataSize(4676473736353LL);
	pNetIfaceStat->setInPkgsCount(345634563LL);
	pNetIfaceStat->setOutPkgsCount(2323232LL);
	_source_stat.m_lstNetIfacesStatistics.append(pNetIfaceStat);

	CSystemStatistics _dest_stat;
	_dest_stat.fromString(_source_stat.toString());
	QCOMPARE(_dest_stat.m_uiRcInit, PRL_ERR_SUCCESS);

	QVERIFY(_dest_stat.m_lstNetIfacesStatistics.size() == _source_stat.m_lstNetIfacesStatistics.size());
	for(int i = 0; i < _source_stat.m_lstNetIfacesStatistics.size(); ++i)
	{
		CNetIfaceStatistics *pSourceNetIface = _source_stat.m_lstNetIfacesStatistics.value(i);
		CNetIfaceStatistics *pDestNetIface = _dest_stat.m_lstNetIfacesStatistics.value(i);
		QVERIFY(*pSourceNetIface == *pDestNetIface);
	}
}

void CSystemStatisticsTest::testFormingNetIfacesStatisticsBoundValues()
{
	CSystemStatistics _source_stat;
	CNetIfaceStatistics *pNetIfaceStat = new CNetIfaceStatistics;
	pNetIfaceStat->setIfaceSystemName("eth0");
	pNetIfaceStat->setInDataSize(ULLONG_MAX);
	pNetIfaceStat->setOutDataSize(ULLONG_MAX);
	pNetIfaceStat->setInPkgsCount(ULLONG_MAX);
	pNetIfaceStat->setOutPkgsCount(ULLONG_MAX);
	_source_stat.m_lstNetIfacesStatistics.append(pNetIfaceStat);

	CSystemStatistics _dest_stat;
	_dest_stat.fromString(_source_stat.toString());
	QCOMPARE(_dest_stat.m_uiRcInit, PRL_ERR_SUCCESS);

	QVERIFY(_dest_stat.m_lstNetIfacesStatistics.size() == _source_stat.m_lstNetIfacesStatistics.size());
	for(int i = 0; i < _source_stat.m_lstNetIfacesStatistics.size(); ++i)
	{
		CNetIfaceStatistics *pSourceNetIface = _source_stat.m_lstNetIfacesStatistics.value(i);
		CNetIfaceStatistics *pDestNetIface = _dest_stat.m_lstNetIfacesStatistics.value(i);
		QVERIFY(*pSourceNetIface == *pDestNetIface);
	}
}

void CSystemStatisticsTest::testFormingUsersStatistics()
{
	CSystemStatistics _source_stat;
	CUserStatistics *pUserStat = new CUserStatistics;
	pUserStat->setUserName("root");
	pUserStat->setServiceName("prl_disp_service");
	pUserStat->setHostName("bla.bla.ru");
	pUserStat->setSessionTime(3536663LL);
	_source_stat.m_lstUsersStatistics.append(pUserStat);

	CSystemStatistics _dest_stat;
	_dest_stat.fromString(_source_stat.toString());
	QCOMPARE(_dest_stat.m_uiRcInit, PRL_ERR_SUCCESS);

	QVERIFY(_dest_stat.m_lstUsersStatistics.size() == _source_stat.m_lstUsersStatistics.size());
	for(int i = 0; i < _source_stat.m_lstUsersStatistics.size(); ++i)
	{
		CUserStatistics *pSourceUser = _source_stat.m_lstUsersStatistics.value(i);
		CUserStatistics *pDestUser = _dest_stat.m_lstUsersStatistics.value(i);
		QVERIFY(*pSourceUser == *pDestUser);
	}
}

void CSystemStatisticsTest::testFormingUsersStatisticsBoundValues()
{
	CSystemStatistics _source_stat;
	CUserStatistics *pUserStat = new CUserStatistics;
	pUserStat->setUserName("root");
	pUserStat->setServiceName("prl_disp_service");
	pUserStat->setHostName("bla.bla.ru");
	pUserStat->setSessionTime(ULLONG_MAX);
	_source_stat.m_lstUsersStatistics.append(pUserStat);

	CSystemStatistics _dest_stat;
	_dest_stat.fromString(_source_stat.toString());
	QCOMPARE(_dest_stat.m_uiRcInit, PRL_ERR_SUCCESS);

	QVERIFY(_dest_stat.m_lstUsersStatistics.size() == _source_stat.m_lstUsersStatistics.size());
	for(int i = 0; i < _source_stat.m_lstUsersStatistics.size(); ++i)
	{
		CUserStatistics *pSourceUser = _source_stat.m_lstUsersStatistics.value(i);
		CUserStatistics *pDestUser = _dest_stat.m_lstUsersStatistics.value(i);
		QVERIFY(*pSourceUser == *pDestUser);
	}
}

QTEST_MAIN(CSystemStatisticsTest)
