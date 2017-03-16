//////////////////////////////////////////////////////////////////////////
///
/// @file PerfCounter_unix.cpp
///
/// @brief Linux specific implemetation
///
/// @author Vadim Hohlov (vhohlov@)
///
/// Copyright (c) 2008-2017, Parallels International GmbH
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
//////////////////////////////////////////////////////////////////////////

#include "platform_spec.h"
#include "trace.h"
#include "PerfCounter.h"

#include <Psapi.h>
#include <sddl.h>

#define INVALID_FILE_HANDLE() ((file_handle_t)-1)

inline char* get_mapping_filename(const char *fname, char *buff)
{
    static char MEMMAP_FN_PREFIX[PATH_MAX] = "\0" ; //"c:\\temp\\" ; //"Global\\" ;
    if (!*MEMMAP_FN_PREFIX) {
        char temp[PATH_MAX] ;
        UINT len = GetSystemWindowsDirectoryA(temp, PATH_MAX) ;
        if (!len) {
            WRITE_TRACE(DBG_FATAL, "Could not get SystemWindowsDirectory, error: %d", GetLastError()) ;
            PERFC_ASSERT(false) ;
        }
        PERFC_ASSERT(len < PATH_MAX) ;
        if (temp[len-1] == '\\')
            --len ;
        strcpy(temp+len, "\\Temp\\") ;
        DWORD f_attr = GetFileAttributesA(temp) ;
        if (f_attr==INVALID_FILE_ATTRIBUTES || !( f_attr & FILE_ATTRIBUTE_DIRECTORY ))
        {
            WRITE_TRACE(DBG_FATAL, "Could not get '%s' path, error: %d", GetLastError()) ;
            PERFC_ASSERT(false) ;
        }
        // may be another thread already initialize this static value
        if (!*MEMMAP_FN_PREFIX)
            strcpy(MEMMAP_FN_PREFIX, temp) ;
    }
    static const size_t MEMMAP_FN_PREFIX_len = strlen(MEMMAP_FN_PREFIX) ;
    strcpy(buff, MEMMAP_FN_PREFIX) ;
    strcpy(buff + MEMMAP_FN_PREFIX_len, fname) ;
    return buff ;
}

bool processes_alive(pid_t p_id)
{
    pid_t p_list[1024] ;
    int len = get_processes_list(ARRAY_AND_SIZE(p_list)) ;
    if (len<=0)
        return false ;
    for(; len--;)
        if (p_list[len]==p_id)
            return true ;
    return false ;
}

int get_processes_list(pid_t *p_list, unsigned int size)
{

    DWORD len ;
    if (EnumProcesses(p_list, size*sizeof(*p_list), &len)==0)
        return ERR_UNKNOWN ;

    return len/sizeof(*p_list) ;
}

bool set_debug_privilege(void)
{
	TOKEN_PRIVILEGES tp;
	LUID luid;
	bool ret = false;

	if (!LookupPrivilegeValue(NULL, L"SeDebugPrivilege", &luid)) {
		WRITE_TRACE(DBG_FATAL, "LookupPrivilegeValue() failed");
		return ret;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	HANDLE curProc = GetCurrentProcess();
	HANDLE procToken;
	if (!OpenProcessToken(curProc, TOKEN_ADJUST_PRIVILEGES, &procToken)) {
		WRITE_TRACE(DBG_FATAL, "OpenProcessToken() failed");
		CloseHandle(curProc);
		return ret;
	}

	if (!AdjustTokenPrivileges(procToken, FALSE, &tp, sizeof(tp),
				(PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
		WRITE_TRACE(DBG_FATAL, "AdjustTokenPrivileges() failed");
	else
		ret = true;

	CloseHandle(procToken);
	CloseHandle(curProc);
	return ret;
}

ULONG64 get_process_starttime( pid_t p_id )
{
	ULONG64 startTime = (ULONG64)-1;

	/*
	 * Do not raise privileges if we are inspecting ourselves.
	 * This function is used not only by prl_perf_ctl,
	 * but by all PerfCount users.
	 */
	if (::getpid() != p_id) {
		if (!set_debug_privilege())
			WRITE_TRACE(DBG_FATAL, "Failed to set debug privilege");
	}

	/** Converts specified 100-nanosecond intervals to milliseconds */
#define TO_MSECS(n100Nanoseconds) (n100Nanoseconds/10000)
#define TO_ULONGLONG(large_var, high_part, low_part)\
	( (((unsigned long long) large_var.high_part) << 32 ) + large_var.low_part)

	HANDLE hProcess = hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, p_id);
	if (NULL != hProcess)
	{
		FILETIME CreationTime, ExitTime, KernelTime, UserTime;
		if (GetProcessTimes(hProcess, &CreationTime, &ExitTime, &KernelTime, &UserTime))
			startTime = TO_MSECS(TO_ULONGLONG(CreationTime, dwHighDateTime, dwLowDateTime));

		CloseHandle(hProcess);
	}

	return startTime;

#undef TO_ULONGLONG
#undef TO_MSECS
}


#define FULL_RIGHTS(for) TEXT("(A;OICI;GARCSDFA;;;") TEXT(#for) TEXT(")")

BOOL CreateFullRightsDACL(SECURITY_ATTRIBUTES * pSA)
{
     pSA->nLength = sizeof(SECURITY_ATTRIBUTES);
     pSA->bInheritHandle = FALSE;

     // Define the SDDL for the DACL. This example sets
     // the following access:
     //     Built-in guests are denied all access.
     //     Anonymous logon is denied all access.
     //     Authenticated users are allowed
     //     read/write/execute access.
     //     Administrators are allowed full control.
     // Modify these values as needed to generate the proper
     // DACL for your application.
     TCHAR * szSD = TEXT("D:")       // Discretionary ACL
        FULL_RIGHTS(BG)     // Deny access to
                                     // built-in guests
        FULL_RIGHTS(AN)     // Deny access to
                                     // anonymous logon
        FULL_RIGHTS(AU) // Allow
                                     // read/write/execute
                                     // to authenticated
                                     // users
        FULL_RIGHTS(BA);    // Allow full control
                                     // to administrators

    if (NULL == pSA)
        return FALSE;

     return ConvertStringSecurityDescriptorToSecurityDescriptor(
                szSD,
                SDDL_REVISION_1,
                &(pSA->lpSecurityDescriptor),
                NULL);
}

file_handle_t open_shared_file(const char *fname, unsigned int mem_size)
{

    SECURITY_ATTRIBUTES  sa;

    if (!CreateFullRightsDACL(&sa))
    {
         // Error encountered; generate message and exit.
         WRITE_TRACE(DBG_FATAL, "Failed CreateFullRightsDACL, errno: %d\n", GetLastError());
         return INVALID_FILE_HANDLE() ;
    }

    HANDLE hMapFile ;
    HANDLE hSharedFile = INVALID_HANDLE_VALUE ;
    char map_filename[PATH_MAX] = "" ;
    get_mapping_filename(fname, map_filename) ;

    if (mem_size && remove_shared_file(fname)!=0)
         return INVALID_FILE_HANDLE() ;

    hSharedFile = CreateFileA(map_filename,
                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                &sa,
                                mem_size ? CREATE_NEW : OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                NULL) ;

    if (hSharedFile == INVALID_HANDLE_VALUE)
    {
        if (mem_size || GetLastError()!=ERROR_FILE_NOT_FOUND)
            WRITE_TRACE(DBG_FATAL, "Could not %s file '%s', error: %d.\n",
                        mem_size ? "create" : "open", map_filename, GetLastError());
        return INVALID_FILE_HANDLE() ;
    }
    hMapFile = CreateFileMappingA(hSharedFile,             // file to be mapped
                                      &sa,                    // security
                                      PAGE_READWRITE,          // read/write access
                                      0,                       // max. object size
                                      mem_size,                // buffer size
                                      NULL) ; //map_filename) ;          // name of mapping object

    if (hSharedFile != INVALID_HANDLE_VALUE)
        CloseHandle(hSharedFile) ;

    if (hMapFile == NULL)
    {
        if (GetLastError()!=ERROR_FILE_NOT_FOUND)
            WRITE_TRACE(DBG_FATAL, "Could not %s file mapping object '%s', error: %d.\n",
                        mem_size ? "create" : "open", map_filename, GetLastError());
        return INVALID_FILE_HANDLE() ;
    }
    return hMapFile ;
}

int remove_shared_file(const char *fname)
{
    char map_filename[PATH_MAX] = "" ;
    if (DeleteFileA(get_mapping_filename(fname, map_filename))!=0 ||
        GetLastError() == ERROR_FILE_NOT_FOUND)
        return 0 ;

    WRITE_TRACE(DBG_FATAL, "Could not remove file '%s', error: %d.\n", map_filename, GetLastError()) ;
    return -1 ;
}

void* memory_map(file_handle_t fd, unsigned int mem_size)
{

    void *result = MapViewOfFile(fd, FILE_MAP_ALL_ACCESS, 0, 0, mem_size) ;
    if (!result)
    {
        WRITE_TRACE(DBG_FATAL, "Could not MapViewOfFile for file %p, error: %d.\n", fd, GetLastError()) ;
        return NULL ;
    }
    return result ;
}

int memory_unmap(void *mem, file_handle_t fd, unsigned int)
{
    if (UnmapViewOfFile(mem)==0)
        WRITE_TRACE(DBG_FATAL, "Could not UnmapViewOfFile for mem %p, file %p, error: %d.\n",
                    mem, fd, GetLastError()) ;

    if (CloseHandle(fd)==0)
        WRITE_TRACE(DBG_FATAL, "Could not CloseHandle for file %p, error: %d.\n", fd, GetLastError()) ;

    return 0 ;
}
