///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspStatCollector.h
///
/// Statistics platform dependent collector implementation
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

#ifndef _DSP_STATISTICS_COLLECTOR_H_
#define _DSP_STATISTICS_COLLECTOR_H_

#include <QMutex>
#include <QList>
#include "CDspStatisticsGuard.h"

class CVmEvent ;

/**
 * Statistics platform dependent collector implementation
 */
class CDspStatCollector
{
public:
	/** Class constructor */
	CDspStatCollector();
	/**
	 * Collects necessary CPUs statistics
	 * @param bufer for storing result
	 */
	void GetCpusStatInfo(SmartPtr<CCpusStatInfo> pCpusStatInfo);
	/**
	 * Collects necessary disks statistics
	 * @param bufer for storing result
	 */
	void GetDisksStatInfo(SmartPtr<CDisksStatInfo> pDisksStatInfo);
	/**
	 * Collects necessary RAM statistics
	 * @param bufer for storing result
	 */
	void GetRamStatInfo(SmartPtr<CRamStatInfo> pRamStatInfo);
	/**
	 * Collects necessary swap statistics
	 * @param bufer for storing result
	 */
	void GetSwapStatInfo(SmartPtr<CSwapStatInfo> pSwapStatInfo);
	/**
	 * Collects necessary uptime statistics
	 * @param bufer for storing result
	 * Note: in view of WINAPI GetTickCount() call overflow issue OS uptime statistics value
	 * expected as delta between previous statistics result and current.
	 */
	void GetUptimeStatInfo(SmartPtr<CUptimeStatInfo> pUptimeStatInfo);
	/**
	 * Collects necessary processes statistics
	 * @param bufer for storing result
	 */
	static void GetProcsStatInfo(SmartPtr<CProcsStatInfo> pProcsStatInfo, Q_PID tpid = 0);
	/**
	 * Collects necessary users sessions statistics
	 * @param bufer for storing result
	 */
	void GetUsersStatInfo(SmartPtr<CUsersStatInfo> pUsersStatInfo);
	/**
	 * Collects necessary network interfaces statistics
	 * @param bufer for storing result
	 */
	void GetIfacesStatInfo(SmartPtr<CIfacesStatInfo> pIfacesStatInfo);

private:
	/**
	 * Common users sessions statistics collecting part: retrieve info about dispatcher users sessions
	 */
	void GetDispatcherUsersSessions(SmartPtr<CUsersStatInfo> pUsersStatInfo);
	/**
	 * Platform dependent part of users sessions statistics collecting
	 */
	void GetSystemUsersSessions(SmartPtr<CUsersStatInfo> pUsersStatInfo);
};

#endif
