///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspStatCollector_lin.cpp
///
/// Statistics platform dependent collector implementation for Linux
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
#include <prlcommon/Logging/Logging.h>

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"

#include <unistd.h>
#include <asm/param.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>

#ifndef HZ
#define HZ 100
#endif

#ifndef ULLONG_MAX
# define ULLONG_MAX ULONG_MAX
#endif

void CDspStatCollector::GetCpusStatInfo(SmartPtr<CCpusStatInfo> pCpusStatInfo)
{
	FILE *stat;
	char buf[8];
	unsigned long long user = 0, nice = 0, system = 0, idle = 0;

	stat = fopen("/proc/stat", "r");
	if(!stat) {
		LOG_MESSAGE(DBG_FATAL, "Can't open /proc/stat");
		return;
	}
	SmartPtr<CCpuStatInfo> pCpu( new CCpuStatInfo );
	while(fscanf(stat, " %7s", buf) != EOF) {
		if(buf[0] == 'c' && buf[1] == 'p' && buf[2] == 'u') {
			if (fscanf(stat, "%llu %llu %llu %llu", &user, &nice, &system, &idle) < 0)
				LOG_MESSAGE(DBG_FATAL, "Error on reading /proc/stat");
			break;
		}
		while(fgetc(stat) != '\n')
		{}
	}
	fclose(stat);

	pCpu->m_nUser = user;
	pCpu->m_nNice = nice;
	pCpu->m_nSystem = system;
	pCpu->m_nIdle = idle;
	pCpu->m_nWait = 0;

	pCpusStatInfo->m_lstCpusStatInfo.append(pCpu);
}

void CDspStatCollector::GetDisksStatInfo(SmartPtr<CDisksStatInfo> pDisksStatInfo)
{
	Q_UNUSED(pDisksStatInfo);
	LOG_MESSAGE(DBG_DEBUG, "Not implemented yet");
}

void CDspStatCollector::GetRamStatInfo(SmartPtr<CRamStatInfo> pRamStatInfo)
{
	FILE *stat;
	char buf[32];
	unsigned long long int tmp = 0;
	pRamStatInfo->m_nFreeSize = 0;
	pRamStatInfo->m_nTotalSize = 0;

	stat = fopen("/proc/meminfo", "r");
	if(!stat)
	{
		WRITE_TRACE_RL(1, DBG_FATAL, "Unable to open /proc/meminfo by error %d", errno );
		return;
	}
	while(fscanf(stat, "%31s %llu", buf, &tmp) != EOF) {
		if(!strcmp(buf, "MemFree:") || !strcmp(buf, "Cached:") || !strcmp(buf, "Buffers:")) {
			pRamStatInfo->m_nFreeSize += tmp;
		}
		if(!strcmp(buf, "MemTotal:")) {
			pRamStatInfo->m_nTotalSize = tmp * 1024;
		}
		while(fgetc(stat) != '\n')
		{}
	}
	pRamStatInfo->m_nFreeSize *= 1024;
	fclose(stat);
	if(pRamStatInfo->m_nTotalSize == 0)
		LOG_MESSAGE(DBG_FATAL, "Couldn't read RAM statistics information");
}

void CDspStatCollector::GetSwapStatInfo(SmartPtr<CSwapStatInfo> pSwapStatInfo)
{
	Q_UNUSED(pSwapStatInfo);
	LOG_MESSAGE(DBG_DEBUG, "Not implemented yet");
}

void CDspStatCollector::GetUptimeStatInfo(SmartPtr<CUptimeStatInfo> pUptimeStatInfo)
{
	Q_UNUSED(pUptimeStatInfo);
	LOG_MESSAGE(DBG_DEBUG, "Not implemented yet");
}

void CDspStatCollector::GetProcsStatInfo(SmartPtr<CProcsStatInfo> pProcsStatInfo, Q_PID tpid )
{
	DIR *proc;
	FILE *fp;
	int pid;
	char name[64];
	struct dirent *pde;
	unsigned long long user, system, starttime, vsize, rsize;

	proc = opendir("/proc");
	if(!proc)
	{
		WRITE_TRACE_RL(1, DBG_FATAL, "Unable to open /proc by error %d", errno );
		return;
	}

	while( (pde = readdir(proc)) != NULL) {
		if (!isdigit(pde->d_name[0])) continue;

		if ( tpid && (int )tpid != UTF8_2QSTR(pde->d_name).toInt() )
			continue;

		sprintf(name, "/proc/%s/stat", pde->d_name);
		fp = fopen(name, "r");
		if(!fp) continue;
		SmartPtr<CProcStatInfo> pProcStatInfo( new CProcStatInfo );

		QString fileName = name;

		if (fscanf(fp, "%d %31s %*c %*d %*d %*d %*d %*d %*d %*d %*d %*d %*d %llu %llu %*d %*d %*d %*d %*d %*d %llu %llu %llu",
				&pid,
				name,
				&user,
				&system,
				&starttime,
				&vsize,
				&rsize) < 0) {
			fclose(fp);
			continue;
		}

		pProcStatInfo->m_nId = pid;
		pProcStatInfo->m_sCommandName = QString::fromAscii(name);
		pProcStatInfo->m_nUserTime = quint64(user) * 1000 / HZ;
		pProcStatInfo->m_nSystemTime = quint64(system) * 1000 / HZ;
		pProcStatInfo->m_nTotalTime = pProcStatInfo->m_nUserTime + pProcStatInfo->m_nSystemTime;
		pProcStatInfo->m_nStartTime = quint64(starttime) * 1000 / HZ;
		pProcStatInfo->m_nRealMemUsage =  quint64(rsize) * 4096;
		pProcStatInfo->m_nVirtMemUsage = vsize;
		pProcStatInfo->m_nTotalMemUsage = pProcStatInfo->m_nRealMemUsage + pProcStatInfo->m_nVirtMemUsage;
		pProcsStatInfo->m_lstProcsStatInfo.append(pProcStatInfo);
		fclose(fp);

		WRITE_TRACE( DBG_TRACE, "TMP>>> %d %llu %llu %llu %llu\n"
			, pid, user, system, starttime, pProcStatInfo->m_nTotalTime);
		WRITE_TRACE( DBG_TRACE, "TMP>>> PID = %d  m_nStartTime = %llu "
			, pProcStatInfo->m_nId, pProcStatInfo->m_nStartTime );

		if (tpid)
			break;
	}
	closedir(proc);
}

void CDspStatCollector::GetSystemUsersSessions(SmartPtr<CUsersStatInfo> pUsersStatInfo)
{
	Q_UNUSED(pUsersStatInfo);
	LOG_MESSAGE(DBG_DEBUG, "Not implemented yet");
}
