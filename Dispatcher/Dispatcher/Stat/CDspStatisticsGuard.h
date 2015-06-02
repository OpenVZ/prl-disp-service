///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspStatisticsGuard.h
///
/// Statistics guardian implementation
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

#ifndef _DSP_STATISTICS_GUARDIAN_H_
#define _DSP_STATISTICS_GUARDIAN_H_

#include "XmlModel/HostHardwareInfo/CSystemStatistics.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"
#include <QMutex>
#include <QList>
#include "Libraries/Std/SmartPtr.h"

class CDspVm;

/** CPU statistics info structure */
struct CCpuStatInfo
{
	/** Class default constructor */
	CCpuStatInfo()
	: m_nUser(0), m_nNice(0), m_nSystem(0), m_nIdle(0), m_nWait(0)
	{}
	/** Class constructor with specified init data */
	CCpuStatInfo(quint64 nUser, quint64 nNice, quint64 nSystem, quint64 nIdle, quint64 nWait)
	: m_nUser(nUser), m_nNice(nNice), m_nSystem(nSystem), m_nIdle(nIdle), m_nWait(nWait)
	{}
	/** CPU user mode time in milliseconds */
	quint64 m_nUser;
	/** CPU user mode with low priority time in milliseconds */
	quint64 m_nNice;
	/** CPU system mode time in milliseconds */
	quint64 m_nSystem;
	/** CPU idle time in milliseconds */
	quint64 m_nIdle;
	/** CPU wait I/O operations competion time in milliseconds */
	quint64 m_nWait;
};

/** CPUs statistics set */
struct CCpusStatInfo
{
	/** CPUs statistics list */
	QList<SmartPtr<CCpuStatInfo> > m_lstCpusStatInfo;
};

/** Disk partion statistics info structure */
struct CDiskPartStatInfo
{
	/** Default class constructor */
	CDiskPartStatInfo()
	: m_nTotalBytes(0), m_nFreeBytes(0)
	{}
	/** Class constructor with specified init data */
	CDiskPartStatInfo(const QString &sSystemName, quint64 nTotalBytes, quint64 nFreeBytes)
	: m_sSystemName(sSystemName), m_nTotalBytes(nTotalBytes), m_nFreeBytes(nFreeBytes)
	{}
	/** Partition system name */
	QString m_sSystemName;
	/** Num of total bytes on partition */
	quint64 m_nTotalBytes;
	/** Num of free bytes on partition */
	quint64 m_nFreeBytes;
};

/** Disk statistics info structure */
struct CDiskStatInfo : public CDiskPartStatInfo
{
	/** Class default constructor */
	CDiskStatInfo()
	: CDiskPartStatInfo()
	{}
	/** Class constructor with specified init data */
	CDiskStatInfo(const QString &sSystemName, quint64 nTotalBytes, quint64 nFreeBytes)
	: CDiskPartStatInfo(sSystemName, nTotalBytes, nFreeBytes)
	{}
	/** Disk partitions statistics info */
	QList<SmartPtr<CDiskPartStatInfo> > m_lstPartsStatInfo;
};

/** Disks statistics info set */
struct CDisksStatInfo
{
	/** Disks statistics info */
	QList<SmartPtr<CDiskStatInfo> > m_lstDisksStatInfo;
};

/** Memory statistics info structure */
struct CMemoryStatInfo
{
	/** Default  class constructor */
	CMemoryStatInfo()
	: m_nTotalSize(0), m_nFreeSize(0)
	{}
	/** Class constructor with specified init data */
	CMemoryStatInfo( quint64 nTotalSize, quint64 nFreeSize)
	: m_nTotalSize(nTotalSize), m_nFreeSize(nFreeSize)
	{}
	/** Total memory size in bytes */
	quint64 m_nTotalSize;
	/** Free memory size in bytes */
	quint64 m_nFreeSize;
};

typedef struct CMemoryStatInfo CRamStatInfo;
typedef struct CMemoryStatInfo CSwapStatInfo;

/** Uptime statistics info structure */
struct CUptimeStatInfo
{
	/** Default class constructor */
	CUptimeStatInfo()
	: m_nOsUptimeDelta(0), m_nProcessUptime(0)
	{}
	/** Class constructor with specified init data */
	CUptimeStatInfo(quint64 nOsUptimeDelta, quint64 nProcessUptime)
	: m_nOsUptimeDelta(nOsUptimeDelta), m_nProcessUptime(nProcessUptime)
	{}
	/** OS uptime delta (difference since previous statistics processing) in seconds */
	quint64 m_nOsUptimeDelta;
	/** Current process (appologize that it will be dispatcher process) uptime in seconds */
	quint64 m_nProcessUptime;
};

/** Process statistics info structure */
struct CProcStatInfo
{
	/** Default class constructor */
	CProcStatInfo()
	: m_nId(0), m_nTotalMemUsage(0), m_nRealMemUsage(0), m_nVirtMemUsage(0), m_nStartTime(0), m_nTotalTime(0), m_nUserTime(0), m_nSystemTime(0), m_nState(PPS_PROC_STOP)
	{}
	/** Class constructor with specified init data */
	CProcStatInfo(const QString &sCommandName, quint32 nId, const QString &sOwnerUser,
								quint64 nTotalMemUsage, quint64 nRealMemUsage, quint64 nVirtMemUsage,
								quint64 nStartTime, quint64 nTotalTime, quint64 nUserTime,
								quint64 nSystemTime, PRL_PROCESS_STATE_TYPE nState)
	:	m_sCommandName(sCommandName), m_nId(nId), m_sOwnerUser(sOwnerUser),
		m_nTotalMemUsage(nTotalMemUsage), m_nRealMemUsage(nRealMemUsage),
		m_nVirtMemUsage(nVirtMemUsage), m_nStartTime(nStartTime),
		m_nTotalTime(nTotalTime), m_nUserTime(nUserTime),
		m_nSystemTime(nSystemTime), m_nState(nState)
	{}
	/** Process command name */
	QString m_sCommandName;
	/** Process system id */
	quint32 m_nId;
	/** Process owner name */
	QString m_sOwnerUser;
	/** Process total memory usage size in bytes */
	quint64 m_nTotalMemUsage;
	/** Process real memory usage size in bytes */
	quint64 m_nRealMemUsage;
	/** Process virtual memory usage size in bytes */
	quint64 m_nVirtMemUsage;
	/** Process start time in secs (num of seconds since January 1, 1601 (UTC)) */
	quint64 m_nStartTime;
	/** Process total CPU usage time in msecs */
	quint64 m_nTotalTime;
	/** Process user space time in msecs */
	quint64 m_nUserTime;
	/** Process system space time in msecs */
	quint64 m_nSystemTime;
	/** Process state */
	PRL_PROCESS_STATE_TYPE m_nState;
};

/** Processes statistics info set */
struct CProcsStatInfo
{
	/** List of processes statistics */
	QList<SmartPtr<CProcStatInfo> > m_lstProcsStatInfo;
};

/** Users statistics info structure */
struct CUserStatInfo
{
	/** Default class constructor */
	CUserStatInfo()
	: m_nDurationTime(0)
	{}
	/** Class constructor with specified init data */
	CUserStatInfo(const QString &sUserName, const QString &sServiceName, const QString &sHostName, quint64 nDurationTime)
	:	m_sUserName(sUserName), m_sServiceName(sServiceName), m_sHostName(sHostName),	m_nDurationTime(nDurationTime)
	{}
	/** Session user name */
	QString m_sUserName;
	/** Session service name */
	QString m_sServiceName;
	/** Session host name */
	QString m_sHostName;
	/** Session duration time in seconds */
	quint64 m_nDurationTime;
};

/** Useresses statistics info set */
struct CUsersStatInfo
{
	/** List of users logon sessions statistics */
	QList<SmartPtr<CUserStatInfo> > m_lstUsersStatInfo;
};

/** Network interface statistics info structure */
struct CIfaceStatInfo
{
	/** Default class constructor */
	CIfaceStatInfo()
	: m_nInDataSize(0), m_nInPkgsCount(0), m_nOutDataSize(0), m_nOutPkgsCount(0)
	{}
	/** Class constructor with specified init data */
	CIfaceStatInfo(const QString &sIfaceSystemName, quint64 nInDataSize, quint64 nInPkgsCount, quint64 nOutDataSize, quint64 nOutPkgsCount)
	:	m_sIfaceSystemName(sIfaceSystemName), m_nInDataSize(nInDataSize), m_nInPkgsCount(nInPkgsCount), m_nOutDataSize(nOutDataSize), m_nOutPkgsCount(nOutPkgsCount)
	{}
	/** Network interface system name */
	QString m_sIfaceSystemName;
	/** In data size in bytes */
	quint64 m_nInDataSize;
	/** In packages count */
	quint64 m_nInPkgsCount;
	/** Out data size in bytes */
	quint64 m_nOutDataSize;
	/** Out packages count */
	quint64 m_nOutPkgsCount;
};

/** Network interfaces statistics info set */
struct CIfacesStatInfo
{
	/** List of network interfaces statistics */
	QList<SmartPtr<CIfaceStatInfo> > m_lstIfacesStatInfo;
};

/**
 * Statistics object access guardian wrapper
 */

class CProcInfoStatistics;

class CDspStatisticsGuard
{
public:
	/** Thread safely returns string with XML representation of statistics */
	static QString GetStatisticsStringRepresentation();
	/** Returns pointer to statistics synchronization object */
	static QMutex *GetSynchroObject();
	/** Returns hos statistics object reference for external purposes */
	static CSystemStatistics &GetHostStatistics();
	/**
	 * Stores CPUs statistics
	 * @param previous CPUs statistics
	 * @param setting CPUs statitstics
	 */
	static void StoreCpusStatInfo(SmartPtr<CCpusStatInfo> pPrevStat, SmartPtr<CCpusStatInfo> pCurrStat);

	/**
	 * Stores disks statistics
	 * @param setting disks statitstics
	 */
	static void StoreDisksStatInfo(SmartPtr<CDisksStatInfo> pDisksStatInfo);

	/**
	 * Stores RAM statistics
	 * @param setting RAM statitstics
	 */
	static void StoreRamStatInfo(SmartPtr<CRamStatInfo> pRamStatInfo);

	/**
	 * Stores swap statistics
	 * @param setting swap statitstics
	 */
	static void StoreSwapStatInfo(SmartPtr<CSwapStatInfo> pSwapStatInfo);

	/**
	 * Stores uptime statistics
	 * @param setting uptime statitstics
	 * Note: in view of WINAPI GetTickCount() call overflow issue OS uptime statistics value
	 * expected as delta between previous statistics result and current. Accordingly
	 * in that method OS uptime statistics collecting as summary of passing values.
	 */
	static void StoreUptimeStatInfo(SmartPtr<CUptimeStatInfo> pUptimeStatInfo);

	/**
	 * Stores processes statistics
	 * @param setting processes statitstics
	 */
	static void StoreProcsStatInfo(SmartPtr<CProcsStatInfo> pProcsStatInfo, quint32 nDeltaMs);

	/**
	 * Stores users statistics
	 * @param setting users statitstics
	 */
	static void StoreUsersStatInfo(SmartPtr<CUsersStatInfo> pUsersStatInfo);

	/**
	 * Stores network interfaces statistics
	 * @param setting network interfaces statitstics
	 */
	static void StoreIfacesStatInfo(SmartPtr<CIfacesStatInfo> pIfacesStatInfo);

private:
	/** Global host statistics object */
	static CSystemStatistics g_HostStatistics;
	/** Global host statistics access synchronization object */
	static QMutex g_HostStatisticsMutex;
};

#endif
