///////////////////////////////////////////////////////////////////////////////
///
/// @file CDspStatCollector_win.cpp
///
/// Statistics platform dependent collector implementation for Windows
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

#ifndef UNICODE
#define UNICODE
#endif

#undef _WIN32_WINNT
#define _WIN32_WINNT (_WIN32_WINNT_WS03)
#undef NTDDI_VERSION
#define NTDDI_VERSION (NTDDI_WS03)

#pragma message("ATTENTION: Forced CDspStatCollector_win.cpp compatibility with Windows 7+ SDK headers.")

#include "CDspStatCollector.h"
#include <prlcommon/Logging/Logging.h>
#include <windows.h>
#include <psapi.h>
#include <aclapi.h>
#include <ntsecapi.h>
#include <lm.h>
#include <set>

// By adding this interface we enable allocations tracing in the module
#include "Interfaces/Debug.h"


namespace {
	typedef std::set<std::pair<DWORD, std::string> > ErrorsMap;
	ErrorsMap g_NotifiedErrors;

#define REPORT_ERROR_CODE_ONCE(error_message)\
	{\
		DWORD nErrCode = GetLastError();\
		ErrorsMap::const_iterator _it = g_NotifiedErrors.find(std::make_pair(nErrCode, error_message));\
		if (_it == g_NotifiedErrors.end())\
		{\
			WRITE_TRACE(DBG_FATAL, error_message, nErrCode);\
			g_NotifiedErrors.insert(std::make_pair(nErrCode, error_message));\
		}\
	}

#define REPORT_ERROR_CODE_ONCE_ERR_CODE_SPECIFIED(error_message, error_code)\
	{\
		ErrorsMap::const_iterator _it = g_NotifiedErrors.find(std::make_pair(error_code, error_message));\
		if (_it == g_NotifiedErrors.end())\
		{\
			WRITE_TRACE(DBG_FATAL, error_message, error_code);\
			g_NotifiedErrors.insert(std::make_pair(error_code, error_message));\
		}\
	}

#define REPORT_ERROR_CODE_ONCE_WITH_ONE_ADDITIONAL_PARAM(error_message, param)\
	{\
		DWORD nErrCode = GetLastError();\
		ErrorsMap::const_iterator _it = g_NotifiedErrors.find(std::make_pair(nErrCode, error_message));\
		if (_it == g_NotifiedErrors.end())\
		{\
			WRITE_TRACE(DBG_FATAL, error_message, param, nErrCode);\
			g_NotifiedErrors.insert(std::make_pair(nErrCode, error_message));\
		}\
	}

#define REPORT_ERROR_CODE_ONCE_WITH_TWO_ADDITIONAL_PARAMS(error_message, param1, param2)\
	{\
		DWORD nErrCode = GetLastError();\
		ErrorsMap::const_iterator _it = g_NotifiedErrors.find(std::make_pair(nErrCode, error_message));\
		if (_it == g_NotifiedErrors.end())\
		{\
			WRITE_TRACE(DBG_FATAL, error_message, param1, param2, nErrCode);\
			g_NotifiedErrors.insert(std::make_pair(nErrCode, error_message));\
		}\
	}

}

namespace {
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define GET_SYSTEM_CALL_BODY(ModuleName, FuncType, FuncName, FuncStub) \
  static FuncType instance = NULL; \
  if (instance == NULL) { \
    HMODULE hmod;\
    hmod = GetModuleHandle(ModuleName); \
    if (hmod) \
      instance = (FuncType)GetProcAddress(hmod, FuncName);\
		else\
			REPORT_ERROR_CODE_ONCE_WITH_ONE_ADDITIONAL_PARAM( "An error occured on try to get handle module '%s'. Error code: %d", ModuleName)\
    if (!instance) \
		{\
			REPORT_ERROR_CODE_ONCE_WITH_ONE_ADDITIONAL_PARAM( "An error occured on try to get method '%s' symbol. Error code: %d", FuncName)\
      instance = FuncStub; \
		}\
  }

#define TO_ULONGLONG(large_var, high_part, low_part)\
	(((unsigned long long) large_var.high_part) << 32) +	large_var.low_part

/** Stub for non compat OSes */
BOOL WINAPI GetSystemTimesStub(LPFILETIME lpIdleTime, LPFILETIME lpKernelTime,	LPFILETIME lpUserTime)
{
	lpIdleTime->dwLowDateTime = 0;
	lpIdleTime->dwHighDateTime = 0;
	lpKernelTime->dwLowDateTime = 0;
	lpKernelTime->dwHighDateTime = 0;
	lpUserTime->dwLowDateTime = 0;
	lpUserTime->dwHighDateTime = 0;
	return 1;
}
/** Returns system CPU time (valid values just can be got for >= WinXP SP1 workstation and >= 2003 server) */
BOOL GetSystemTimesCompat(LPFILETIME lpIdleTime, LPFILETIME lpKernelTime,	LPFILETIME lpUserTime)
{
	typedef BOOL (WINAPI *LPFNGETSYSTEMTIMES)(LPFILETIME, LPFILETIME, LPFILETIME);
	GET_SYSTEM_CALL_BODY(L"kernel32.dll", LPFNGETSYSTEMTIMES, "GetSystemTimes", GetSystemTimesStub);
  return instance(lpIdleTime, lpKernelTime, lpUserTime);
}

/** Converts specified 100-nanosecond intervals to milliseconds */
unsigned long long to_msecs(unsigned long long n100Nanoseconds)
{
	return (n100Nanoseconds/10000);
}

/** Fills corresponding disk statistics object with necessary disk space values */
void FillDiskInfo(CDiskStatInfo *pDisk, LPCTSTR lpszVolumeName)
{
	QString sVolumeName = UTF16_2QSTR(lpszVolumeName);
	TCHAR szVolumePath[BUFSIZ];
	DWORD cchBufferSize = BUFSIZ;
	QString sVolumePath = "?";
	if ( GetVolumePathName(lpszVolumeName, szVolumePath, cchBufferSize) )
		sVolumePath = UTF16_2QSTR(szVolumePath);
	else
		REPORT_ERROR_CODE_ONCE_WITH_ONE_ADDITIONAL_PARAM(  "Coldn't to get volume '%s' path. Error code: %d", sVolumeName.toUtf8().constData())
	ULARGE_INTEGER TotalNumberOfBytes, TotalNumberOfFreeBytes;
	pDisk->m_sSystemName = sVolumePath;
	if (GetDiskFreeSpaceEx(lpszVolumeName, 0, &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
	{
		pDisk->m_nTotalBytes = TO_ULONGLONG(TotalNumberOfBytes, HighPart, LowPart);
		pDisk->m_nFreeBytes = TO_ULONGLONG(TotalNumberOfFreeBytes, HighPart, LowPart);
	}
	else
		REPORT_ERROR_CODE_ONCE_WITH_TWO_ADDITIONAL_PARAMS(  "Couldn't to get volume '%s' info (path '%s'). Error code: %d", sVolumeName.toUtf8().constData(), sVolumePath.toUtf8().constData())
}

/** Adds disk statistics */
void AddDiskStatistics(LPCTSTR lpszVolumeName, SmartPtr<CDisksStatInfo> pDisksStatInfo)
{
	if (GetDriveType(lpszVolumeName) == DRIVE_FIXED)
	{
		SmartPtr<CDiskStatInfo> pDisk( new CDiskStatInfo );
		FillDiskInfo(pDisk.getImpl(), lpszVolumeName);
		pDisksStatInfo->m_lstDisksStatInfo.append(pDisk);
	}
}

/** Returns current system time in UTC format (num of seconds since January 1, 1601)*/
quint64 GetSystemUtcTime()
{
	SYSTEMTIME SystemTime;
	GetSystemTime(&SystemTime);
	FILETIME ConvertedSystemTime;
	if (SystemTimeToFileTime(&SystemTime, &ConvertedSystemTime))
		return (to_msecs(TO_ULONGLONG(ConvertedSystemTime, dwHighDateTime, dwLowDateTime))/1000);
	else
		REPORT_ERROR_CODE_ONCE("Couldn't to convert system time. Error code: %d")
	return 0;
}

}

void CDspStatCollector::GetCpusStatInfo(SmartPtr<CCpusStatInfo> pCpusStatInfo)
{
	SmartPtr<CCpuStatInfo> pCpu( new CCpuStatInfo );
	pCpusStatInfo->m_lstCpusStatInfo.append(pCpu);
  FILETIME        IdleTime, KernelTime, UserTime;
  BOOL            bret;
  bret = GetSystemTimesCompat(&IdleTime, &KernelTime, &UserTime);
  if (!bret)
	{
		REPORT_ERROR_CODE_ONCE("Couldn't to get system times info. Error code: %d")
    return;
	}
	pCpu->m_nIdle = TO_ULONGLONG(IdleTime, dwHighDateTime, dwLowDateTime);
  //"System Idle" process time is included in kernel time
	pCpu->m_nSystem = to_msecs(TO_ULONGLONG(KernelTime, dwHighDateTime, dwLowDateTime) - pCpu->m_nIdle);
  pCpu->m_nIdle = to_msecs(pCpu->m_nIdle);
  pCpu->m_nUser = to_msecs(TO_ULONGLONG(UserTime, dwHighDateTime, dwLowDateTime));
}

void CDspStatCollector::GetDisksStatInfo(SmartPtr<CDisksStatInfo> pDisksStatInfo)
{
	TCHAR szVolumeSystemName[BUFSIZ];
	DWORD cchBufferSize = BUFSIZ;
	HANDLE hFindHandle = FindFirstVolume(szVolumeSystemName, cchBufferSize);
	if (hFindHandle != INVALID_HANDLE_VALUE)
	{
		AddDiskStatistics(szVolumeSystemName, pDisksStatInfo);
		while (FindNextVolume(hFindHandle, szVolumeSystemName, cchBufferSize))
			AddDiskStatistics(szVolumeSystemName, pDisksStatInfo);
		FindVolumeClose(hFindHandle);
	}
}

void CDspStatCollector::GetRamStatInfo(SmartPtr<CRamStatInfo> pRamStatInfo)
{
	MEMORYSTATUSEX MemoryStatus;
	MemoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
	if (GlobalMemoryStatusEx(&MemoryStatus))
	{
		pRamStatInfo->m_nTotalSize = MemoryStatus.ullTotalPhys;
		pRamStatInfo->m_nFreeSize = MemoryStatus.ullAvailPhys;
	}
	else
		REPORT_ERROR_CODE_ONCE("Couldn't to get memory statistics. Error code: %d")
}

void CDspStatCollector::GetSwapStatInfo(SmartPtr<CSwapStatInfo> pSwapStatInfo)
{
	MEMORYSTATUSEX MemoryStatus;
	MemoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
	if (GlobalMemoryStatusEx(&MemoryStatus))
	{
		pSwapStatInfo->m_nTotalSize = MemoryStatus.ullTotalPageFile;
		pSwapStatInfo->m_nFreeSize = MemoryStatus.ullAvailPageFile;
	}
	else
		REPORT_ERROR_CODE_ONCE("Couldn't to get memory statistics. Error code: %d")
}

void CDspStatCollector::GetUptimeStatInfo(SmartPtr<CUptimeStatInfo> pUptimeStatInfo)
{
	static quint32 g_nPreviousOsUptime = 0;
	static quint64 g_nProcessCreationTime = 0;
	quint32 nCurrentOsUptime = GetTickCount()/1000;
	if (g_nPreviousOsUptime)
	{
		//Overflow was occured
		if (nCurrentOsUptime < g_nPreviousOsUptime)
			pUptimeStatInfo->m_nOsUptimeDelta = nCurrentOsUptime + (UINT_MAX - g_nPreviousOsUptime);
		else
			pUptimeStatInfo->m_nOsUptimeDelta = nCurrentOsUptime - g_nPreviousOsUptime;
	}
	else
		pUptimeStatInfo->m_nOsUptimeDelta = nCurrentOsUptime;
	g_nPreviousOsUptime = nCurrentOsUptime;
	//Retrieve this value once in view it's const through out of process live
	if (!g_nProcessCreationTime)
	{
		FILETIME CreationTime, ExitTime, KernelTime, UserTime;
		if (GetProcessTimes(GetCurrentProcess(), &CreationTime, &ExitTime, &KernelTime, &UserTime))
			g_nProcessCreationTime = to_msecs(TO_ULONGLONG(CreationTime, dwHighDateTime, dwLowDateTime))/1000;
		else
		{
			REPORT_ERROR_CODE_ONCE("An error occured on getting process creation time. Error code: %d")
			return;
		}
	}
	pUptimeStatInfo->m_nProcessUptime = GetSystemUtcTime() - g_nProcessCreationTime;
}

namespace {
/** Fills specified user name */
void GetUserName(QString& userName, PSID pSID)
{
	TCHAR user[MAX_PATH], domain[MAX_PATH];
	DWORD sizeUser = sizeof(user), sizeDomain = sizeof(domain);
	SID_NAME_USE snu;

	if (LookupAccountSid(NULL, pSID, user, &sizeUser, domain, &sizeDomain, &snu))
	{
		if (sizeDomain > 1)
		{
			userName = UTF16_2QSTR(domain);
			userName += "\\";
		}
		else
			userName = "";
		userName += UTF16_2QSTR(user);
	}
	else
		REPORT_ERROR_CODE_ONCE("Couldn't to lookup process owner user SID. Error code: %d")
}
/** Fills process command name info */
void FillProcessCommandName(SmartPtr<CProcStatInfo> pProcStatInfo, HANDLE hProcess)
{
	if (pProcStatInfo->m_nId == 0 || pProcStatInfo->m_nId == 4)//Special case for System Idle and System processes
	{
		if (pProcStatInfo->m_nId == 0)
			pProcStatInfo->m_sCommandName = "System Idle Process";
		else
			pProcStatInfo->m_sCommandName = "System";
		return;
	}
	TCHAR szProcessName[MAX_PATH];
	HMODULE hMod;
	DWORD cbNeeded;
	if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
	{
		if (GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName)/sizeof(TCHAR)))
			pProcStatInfo->m_sCommandName = UTF16_2QSTR(szProcessName);
		else
			REPORT_ERROR_CODE_ONCE_WITH_ONE_ADDITIONAL_PARAM("Couldn't to extract process (id=%u) comand name. Error code: %d", pProcStatInfo->m_nId)
	}
	else
		REPORT_ERROR_CODE_ONCE_WITH_ONE_ADDITIONAL_PARAM("Couldn't to enumerate process (id=%u) modules. Error code: %d", pProcStatInfo->m_nId)
}
/** Fills process owner user name info */
void FillProcessOwnerUser(SmartPtr<CProcStatInfo> pProcStatInfo, HANDLE hProcess)
{
	HANDLE hTkn;
	if (OpenProcessToken(hProcess, TOKEN_QUERY, &hTkn))
	{
		//token user
		TOKEN_USER* tknUser = 0;
		DWORD needed = 0;
		GetTokenInformation(hTkn, TokenUser, tknUser, 0, &needed);
		if (0 != needed)
		{
			tknUser = new TOKEN_USER[needed];
			if (GetTokenInformation(hTkn, TokenUser, tknUser, needed, &needed))
				GetUserName(pProcStatInfo->m_sOwnerUser, tknUser->User.Sid);
			else
				REPORT_ERROR_CODE_ONCE_WITH_ONE_ADDITIONAL_PARAM("Couldn't to request owner token info for process (id=%u). Error code: %d", pProcStatInfo->m_nId)
		}
		else
			REPORT_ERROR_CODE_ONCE_WITH_ONE_ADDITIONAL_PARAM("Couldn't to request necessary token size for process (id=%u) owner. Error code: %d", pProcStatInfo->m_nId)
		delete[] tknUser;
		CloseHandle(hTkn);
	}
	else
			REPORT_ERROR_CODE_ONCE_WITH_ONE_ADDITIONAL_PARAM("Couldn't to open process (id=%u) token. Error code: %d", pProcStatInfo->m_nId)
}
/** Fills process memory usage info */
void FillProcessMemUsage(SmartPtr<CProcStatInfo> pProcStatInfo, HANDLE hProcess)
{
	PROCESS_MEMORY_COUNTERS psmemCounters;
	if (GetProcessMemoryInfo(hProcess, &psmemCounters, sizeof(psmemCounters)))
	{
		pProcStatInfo->m_nRealMemUsage = psmemCounters.WorkingSetSize;
		pProcStatInfo->m_nVirtMemUsage = psmemCounters.PagefileUsage;
		pProcStatInfo->m_nTotalMemUsage = pProcStatInfo->m_nRealMemUsage + pProcStatInfo->m_nVirtMemUsage;
	}
	else
		REPORT_ERROR_CODE_ONCE_WITH_ONE_ADDITIONAL_PARAM("Couldn't to get process (id=%u) memory info. Error code: %d", pProcStatInfo->m_nId)
}
/** Fills process CPU usage and process creation time info */
void FillProcessCpuUsageAndStartTimes(SmartPtr<CProcStatInfo> pProcStatInfo, HANDLE hProcess)
{
	FILETIME CreationTime, ExitTime, KernelTime, UserTime;
	if (GetProcessTimes(hProcess, &CreationTime, &ExitTime, &KernelTime, &UserTime))
	{
		pProcStatInfo->m_nUserTime = to_msecs(TO_ULONGLONG(UserTime, dwHighDateTime, dwLowDateTime));
		pProcStatInfo->m_nSystemTime = to_msecs(TO_ULONGLONG(KernelTime, dwHighDateTime, dwLowDateTime));
		pProcStatInfo->m_nTotalTime = pProcStatInfo->m_nUserTime + pProcStatInfo->m_nSystemTime;
		//XXX: need to investigate possibility of convert Win UTC time to UTC GNU extension time (num of seconds from '00:00:00 1970-01-01')
		pProcStatInfo->m_nStartTime = to_msecs(TO_ULONGLONG(CreationTime, dwHighDateTime, dwLowDateTime));
	}
	else
		REPORT_ERROR_CODE_ONCE_WITH_ONE_ADDITIONAL_PARAM("An error occured on getting process (id=%u) creation time. Error code: %d", pProcStatInfo->m_nId)
}

}

void CDspStatCollector::GetProcsStatInfo(SmartPtr<CProcsStatInfo> pProcsStatInfo, Q_PID tpid )
{
	DWORD aProcesses[1024], cbNeeded;
	if (EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
	{
		quint32 nProcessNumber = cbNeeded/sizeof(DWORD);
		for (quint32 i = 0; i < nProcessNumber; ++i)
		{
			DWORD nProcId = aProcesses[i];

			if ( tpid && tpid->dwProcessId != nProcId )
				continue;

			if (nProcId == 0 || nProcId == 4)//Special case for system processes
			{
				SmartPtr<CProcStatInfo> pProcStatInfo( new CProcStatInfo );
				pProcStatInfo->m_nId = nProcId;
				pProcStatInfo->m_nState = PPS_PROC_RUN;//Under Win platform processes don't have state info
				FillProcessCommandName(pProcStatInfo, NULL);
				FillProcessOwnerUser(pProcStatInfo, GetCurrentProcess());
				pProcsStatInfo->m_lstProcsStatInfo.append(pProcStatInfo);
				continue;
			}
			HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, nProcId);
			if (hProcess == NULL)//Try to open process with less permisions
				hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, nProcId);
			if (NULL != hProcess)
			{
				SmartPtr<CProcStatInfo> pProcStatInfo( new CProcStatInfo );
				pProcStatInfo->m_nId = nProcId;
				pProcStatInfo->m_nState = PPS_PROC_RUN;//Under Win platform processes don't have state info
				FillProcessCommandName(pProcStatInfo, hProcess);
				//XXX: temporary solution to prevent #2388 issue
				if (!pProcStatInfo->m_sCommandName.size())
				{
					CloseHandle(hProcess);
					continue;
				}
				FillProcessOwnerUser(pProcStatInfo, hProcess);
				FillProcessMemUsage(pProcStatInfo, hProcess);
				FillProcessCpuUsageAndStartTimes(pProcStatInfo, hProcess);
				pProcsStatInfo->m_lstProcsStatInfo.append(pProcStatInfo);
				CloseHandle(hProcess);
			}
			else
				REPORT_ERROR_CODE_ONCE_WITH_ONE_ADDITIONAL_PARAM("Couldn't to open process with id %u. Error code: %d", nProcId)

			if (tpid)
				break;
		}
	}
	else
		REPORT_ERROR_CODE_ONCE("Couldn't to enumerate processes. Error code: %d")
}

namespace {
/** Returns logon type string representation */
QString GetLogonTypeName(SECURITY_LOGON_TYPE LogonType)
{
	QString sLogonTypeName;
	switch (LogonType)
	{
		case Network: sLogonTypeName = UTF8_2QSTR("Network"); break;
		case NetworkCleartext: sLogonTypeName = UTF8_2QSTR("NetworkCleartext"); break;
		case RemoteInteractive: sLogonTypeName = UTF8_2QSTR("RemoteInteractive"); break;
		case CachedRemoteInteractive: sLogonTypeName = UTF8_2QSTR("CachedRemoteInteractive"); break;
		default: sLogonTypeName = UTF8_2QSTR("Unknown");
	}
	return (sLogonTypeName);
}
/**
 * Extracts necessary user logon session statistics
 * @param pointer to logon session identifier
 * @param pointer to buffer for storing result statistics
 */
void ProcessLogonSession(PLUID LogonSessionId, SmartPtr<CUsersStatInfo> pUsersStatInfo)
{
	PSECURITY_LOGON_SESSION_DATA sessionData = NULL;
	NTSTATUS retval = LsaGetLogonSessionData(LogonSessionId, &sessionData);
	if (retval == STATUS_SUCCESS && sessionData != NULL)
	{
		SECURITY_LOGON_TYPE LogonType = (SECURITY_LOGON_TYPE)sessionData->LogonType;
		//Processing just remote logon session
		if (LogonType == RemoteInteractive || LogonType == CachedRemoteInteractive)
		{
			SmartPtr<CUserStatInfo> pUserStatInfo( new CUserStatInfo );
			pUserStatInfo->m_sUserName = UTF16_2QSTR(sessionData->UserName.Buffer);
			pUserStatInfo->m_sServiceName = GetLogonTypeName(LogonType);
			//XXX: need to determine possibility of getting remote host name here
			pUserStatInfo->m_sHostName = UTF16_2QSTR(sessionData->LogonDomain.Buffer); // DnsDomainName
			pUserStatInfo->m_nDurationTime = GetSystemUtcTime() - to_msecs(TO_ULONGLONG(sessionData->LogonTime, HighPart, LowPart))/1000;
			pUsersStatInfo->m_lstUsersStatInfo.append(pUserStatInfo);
		}
	}
	else
		REPORT_ERROR_CODE_ONCE_ERR_CODE_SPECIFIED(  "Couldn't to retrieve logon session data. Error code: %d", LsaNtStatusToWinError(retval))
	if (sessionData)
		LsaFreeReturnBuffer(sessionData);
}

/**
 * Processes network sessions information
 * @param pointer to buffer for storing result
 */
void ProcessNetSessions(SmartPtr<CUsersStatInfo> pUsersStatInfo)
{
	LPSESSION_INFO_10 pBuf = NULL;
	LPSESSION_INFO_10 pTmpBuf;
	DWORD dwLevel = 10;
	DWORD dwPrefMaxLen = MAX_PREFERRED_LENGTH;
	DWORD dwEntriesRead = 0;
	DWORD dwTotalEntries = 0;
	DWORD dwResumeHandle = 0;
	NET_API_STATUS nStatus;
	// Call the NetSessionEnum function, specifying level 10
	do
	{
		nStatus = NetSessionEnum(NULL, NULL, NULL, dwLevel, (LPBYTE*)&pBuf, dwPrefMaxLen, &dwEntriesRead, &dwTotalEntries, &dwResumeHandle);
		if ((nStatus == NERR_Success) || (nStatus == ERROR_MORE_DATA))
		{
			if ((pTmpBuf = pBuf) != NULL)
			{
				// Loop through the entries.
				for (DWORD i = 0; (i < dwEntriesRead); i++)
				{
					if (pTmpBuf == NULL)
					{
						REPORT_ERROR_CODE_ONCE("Wrong session info buffer.")
						break;
					}
					SmartPtr<CUserStatInfo> pUserStatInfo( new CUserStatInfo );
					pUserStatInfo->m_sHostName = UTF16_2QSTR(pTmpBuf->sesi10_cname);
					pUserStatInfo->m_sUserName = UTF16_2QSTR(pTmpBuf->sesi10_username);
					pUserStatInfo->m_sServiceName = UTF8_2QSTR("Network share");
					pUserStatInfo->m_nDurationTime = pTmpBuf->sesi10_time;
					pUsersStatInfo->m_lstUsersStatInfo.append(pUserStatInfo);

					pTmpBuf++;
				}
			}
		}
		else
			REPORT_ERROR_CODE_ONCE_ERR_CODE_SPECIFIED("A system error has occurred: %d\n", nStatus)
		// Free the allocated memory.
		if (pBuf != NULL)
		{
			NetApiBufferFree(pBuf);
			pBuf = NULL;
		}
	}
	// Continue to call NetSessionEnum while there are more entries
	while (nStatus == ERROR_MORE_DATA);

	// Check again for an allocated buffer.
	if (pBuf != NULL)
		NetApiBufferFree(pBuf);
}

}

void CDspStatCollector::GetSystemUsersSessions(SmartPtr<CUsersStatInfo> pUsersStatInfo)
{
/* FIXME: Temporarily commented due issues of Secur32.dll under w2k3 64 bit hosts:
			http://bugzilla.parallels.com/show_bug.cgi?id=7211

	PLUID sessions = NULL;
	ULONG count;

	NTSTATUS retval = LsaEnumerateLogonSessions(&count, &sessions);
	if (retval == STATUS_SUCCESS)
	{
		for (ULONG i = 0; i < count; ++i)
			ProcessLogonSession(&sessions[i], pUsersStatInfo);
		LsaFreeReturnBuffer(sessions);
	}
	else
		REPORT_ERROR_CODE_ONCE_ERR_CODE_SPECIFIED("Couldn't to enumerate logon sessions. Error code: %d", LsaNtStatusToWinError(retval))*/
	ProcessNetSessions(pUsersStatInfo);
}
