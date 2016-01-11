//////////////////////////////////////////////////////////////////////////
///
/// @file platform_spec.h
///
/// @brief Internal header file that declare platform dependent
/// implemetation methods for PerfCounter library
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

#ifndef __PLATFORM_SPEC_H__
#define __PLATFORM_SPEC_H__

#include <prlcommon/Interfaces/ParallelsTypes.h>

#ifdef _LIN_
#elif  defined(_MAC_)
#endif

#if defined(_LIN_) || defined(_MAC_)

#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

typedef int file_handle_t ;

#define GET_MEMORY_PAGE_SIZE() (::getpagesize())

#elif defined(_WIN_)

#include <Windows.h>

typedef HANDLE file_handle_t ;
typedef DWORD  pid_t ;

#define GET_MEMORY_PAGE_SIZE() 0x1000
#define PATH_MAX MAX_PATH

#endif

bool processes_alive(pid_t p_id) ;
int get_processes_list(pid_t *p_list, unsigned int size) ;

/*  Function to get process unique parameter.
	return (ULONG64)(-1) if pid doesn't exist.
	NOTE: Implemented only on for Windows ( for other platform return (ULONG64)(-1) )
*/
ULONG64 get_process_starttime( pid_t p_id );

/*
  create or open shared memory depending on @mem_size parameter
  if mem_size>0 - shared memory created O_EXCL flag
 */
file_handle_t open_shared_file(const char *fname, unsigned int mem_size) ;
int remove_shared_file(const char *fname) ;

void* memory_map(file_handle_t fd, unsigned int mem_size) ;
int memory_unmap(void *mem, file_handle_t fd, unsigned int mem_size) ;

#if defined(_LIN_) || defined(_MAC_)

struct lock_object_t {

        lock_object_t() {
            const pthread_mutex_t tmp = PTHREAD_MUTEX_INITIALIZER ;
            mutex = tmp ;
        }

        void lock() { pthread_mutex_lock(&mutex) ; }
        void unlock() { pthread_mutex_unlock(&mutex) ; }
    private:
        pthread_mutex_t mutex ;
} ;

#elif defined(_WIN_)

struct lock_object_t {

        lock_object_t() {
            InitializeCriticalSection(&critical_section) ;
        }
        ~lock_object_t() {
            DeleteCriticalSection(&critical_section) ;
        }

        void lock() { EnterCriticalSection(&critical_section) ; }
        void unlock() { LeaveCriticalSection(&critical_section) ; }
    private:
        CRITICAL_SECTION critical_section ;
} ;

#endif

#if defined(_LIN_) || defined(_MAC_) || defined(_WIN_)
struct lock_object_t_locker {
		lock_object_t_locker(lock_object_t &object) : m_object(object) {
			m_object.lock();
			m_locked = true;
		};

		~lock_object_t_locker() {
			m_object.unlock();
		}

		void unlock() {
			if (m_locked)
				m_object.unlock();
			m_locked = false;
		}

	private:
		lock_object_t &m_object;
		bool m_locked;
} ;
#endif

#endif // __PLATFORM_SPEC_H__
