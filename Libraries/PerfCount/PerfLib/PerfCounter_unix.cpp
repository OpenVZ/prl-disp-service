//////////////////////////////////////////////////////////////////////////
///
/// @file PerfCounter_unix.cpp
///
/// @brief Linux specific implemetation
///
/// @author Vadim Hohlov (vhohlov@)
///
/// Copyright (c) 2008-2015 Parallels IP Holdings GmbH
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
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
//////////////////////////////////////////////////////////////////////////

#include "platform_spec.h"
#include "trace.h"
#include "PerfCounter.h"

#include <dirent.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

#ifdef  _MAC_
#	include <sys/sysctl.h>
#	include <sys/stat.h>
#endif

#ifdef _LIN_
#	include <sys/types.h>
#	include <sys/stat.h>
#endif

#define INVALID_FILE_HANDLE() ((file_handle_t)-1)

static const char MEMMAP_FN_PREFIX[] =
/*
 * Apple Sandbox Design Guide requires (see section "IPC and POSIX Semaphores
 * and Shared Memory") to use Group/Name notation for shared memory objects
 * naming. Note that the full name length is limited to 31 character.
 */
#ifdef _MAC_
	"IPC.parallels/"; // FIXME: use ParallelsDirs::getIPCPath()
#else
	"/";
#endif

inline char* get_mapping_filename(const char *fname, char *buff)
{
    strcpy(buff, MEMMAP_FN_PREFIX) ;
    strcpy(buff + sizeof(MEMMAP_FN_PREFIX) - 1, fname) ;
    return buff ;
}

#ifdef  _MAC_

bool processes_alive(pid_t p_id)
{
    static const int name[] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, p_id} ;
    kinfo_proc proc ;
    size_t length = sizeof(proc) ;

    int result = sysctl((int *)name, ARRAY_SIZE(name), (void*)&proc, &length, NULL, 0) ;
    WRITE_TRACE(DBG_FATAL, "## sysctl(CTL_KERN, KERN_PROC, KERN_PROC_PID, %d) return %d, errno: %d, kp_proc.p_id: %d\n",
                p_id, result, errno, proc.kp_proc.p_pid) ;

    return result==0 ;
}

int get_processes_list(pid_t *p_list, unsigned int size)
{
	int name[] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
	kinfo_proc *procs;
	size_t procs_size = sizeof(kinfo_proc) * size;
	size_t i;

	procs = (kinfo_proc*) malloc(procs_size);
	if (procs == NULL)
		return ERR_ALLOCATE_MEM;

	if (sysctl(name, ARRAY_SIZE(name) - 1, (void*) procs, &procs_size, NULL, 0) < 0)
	{
		free(procs);
		return ERR_UNKNOWN;
	}

	for (i = 0; i < procs_size / sizeof(kinfo_proc); i++)
		p_list[i] = procs[i].kp_proc.p_pid;

	free(procs);
	return i;
}

#else // not _MAC_

bool processes_alive(pid_t p_id)
{
    char buff[64] ;
    sprintf(buff, "/proc/%d", p_id) ;
    DIR *proc_dir = opendir(buff) ;
    if (proc_dir)
        closedir(proc_dir) ;
    return proc_dir ;
}

int get_processes_list(pid_t *p_list, unsigned int size)
{
    DIR *proc_dir = opendir("/proc/") ;
    if (!proc_dir)
        return 0 ;

    unsigned int count = 0 ;
        struct dirent *de ;
    pid_t *p_id = p_list ;
    while( (de = readdir(proc_dir)) != NULL && count<size) {
        unsigned int pid ;
        if (sscanf(de->d_name, "%u", &pid)!=1)
            continue ;
        *p_id++ = pid ;
        ++count ;
    }
    closedir(proc_dir) ;

    return p_id - p_list ;
}

#endif // not _MAC_

ULONG64 get_process_starttime( pid_t )
{
	return (ULONG64)-1;
}


int remove_shared_file(const char *fname)
{
    char buff[PATH_MAX] ;
    shm_unlink(get_mapping_filename(fname, buff)) ;
    //WRITE_TRACE(DBG_FATAL, "Unlink share file '%s'\n", buff) ;
    return 0 ;
}

struct umask_saver_t {
        umask_saver_t(mode_t new_value)
            :_old(umask(new_value)), _new(new_value)
        {
        }
        ~umask_saver_t() { umask(_old) ; }

        mode_t old_value() { return _old ; }
        mode_t new_value() { return _new ; }

    private:
        mode_t _old ;
        mode_t _new ;
} ;

file_handle_t open_shared_file(const char *fname, unsigned int mem_size)
{
    bool create = mem_size!=0 ;
    // we always recreate shared memory
    if (create)
        remove_shared_file(fname) ;

    char mmap_filename[PATH_MAX] ;

    umask_saver_t umask_saver(S_IXGRP | S_IXOTH) ;
    int flags = O_RDWR | (create ? O_CREAT | O_EXCL : 0) ;
    int fd = shm_open(get_mapping_filename(fname, mmap_filename), flags, S_IRWXU | S_IRWXG | S_IRWXO) ;
    if (fd < 0)
    {
        //WRITE_TRACE(DBG_FATAL, "Failed on %s shared file '%s', errno: %d\n",
        //            create? "creating": "opening", mmap_filename, errno) ;
        return INVALID_FILE_HANDLE() ;
    }
    if (mem_size>0 && (SYS_CALL_TRACE(ftruncate(fd, mem_size)) < 0))
    {
        close(fd) ;
        WRITE_TRACE(DBG_FATAL, "Failed allocate shared memory %d for file '%s', errno: %d\n",
                    mem_size, mmap_filename, errno) ;
        return INVALID_FILE_HANDLE() ;
    }
    return fd ;
}

void* memory_map(file_handle_t fd, unsigned int mem_size)
{
	struct stat x;
	void* output = NULL;
	if (-1 != fstat(fd, &x))
	{
		if (x.st_size == mem_size)
		{
			output = mmap(NULL, mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
			if (MAP_FAILED == output)
				output = NULL;
		}
		else
		{
			WRITE_TRACE(DBG_FATAL, "The shared segment size %lld differs from the required size %u\n",
				int64_t(x.st_size), mem_size);
		}
	}
	close(fd);
	return output;
}

int memory_unmap(void *mem, file_handle_t, unsigned int mem_size)
{
    munmap(mem, mem_size) ;
    return 0 ;
}
