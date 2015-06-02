//////////////////////////////////////////////////////////////////////////
///
/// @file PerfCounter.cpp
///
/// @brief Performance counters library implementation
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


#include "trace.h"
#include "PerfCounter.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>

typedef unsigned int refcount_t ;

#define FILENAME_STORAGE_HINT		"prl.perf_"
#define INVALID_FILE_HANDLE() ((file_handle_t)-1)
#ifdef _WIN_
#define snprintf _snprintf
#endif

static const char *FILENAME_STORAGE_FMT
		= FILENAME_STORAGE_HINT"%u_%u"
#ifdef	_WIN_
		".%8llx" ;
#else
		"" ;
#endif

static const char STORAGE_MARK[8] = "PRLSTRG" ;
static const int SHM_FILENAME_SIZE = PERFCOUNT_STORAGE_WIDTH - sizeof(STORAGE_MARK) -
    sizeof(unsigned int) - sizeof(unsigned int) -
    sizeof(refcount_t) - sizeof(refcount_t) ;

#define CREATER_PID_UNDEFINED() ((unsigned int)-1)

#include <Interfaces/packed.h>
typedef struct _storage_mem_t {

        char          mark[sizeof(STORAGE_MARK)] ;   // mark to identify that memory initialized
        unsigned int  storage_no ;                   // number of storage in created process
        unsigned int  creater_pid ;                  // process creater
        refcount_t    creater_ref_count ;            // creater ref count, identify when release
                                                     //    storage_no for creater process
        refcount_t    ref_count ;                    // global ref count, identify when
                                                     // delete shared mem file
		char          filename[SHM_FILENAME_SIZE] ;  // shared mem file name

		#define STORAGE_MEM_HEADER_SIZE ( \
			sizeof(STORAGE_MARK) + sizeof(unsigned int) * 2 + \
			sizeof(refcount_t) * 2 + SHM_FILENAME_SIZE )

		/** Performance counters storage MUST be aligned
		 * to the page border in order to be shared between
		 * VMM/VMX/Hypervisor
		 */
		char          align_to_page[PAGE_SIZE - STORAGE_MEM_HEADER_SIZE];
        char          counters_storage[sizeof(counters_storage_t)] ;  // real counter storage memory returned for use
} storage_mem_t;
#include <Interfaces/unpacked.h>
typedef storage_mem_t * storage_mem_ptr ;


#define ALIGN_TO(value, align) ((((value) + align - 1)/align)*align)

inline unsigned aligned_storage_size()
{
    static const unsigned result = ALIGN_TO(sizeof(storage_mem_t), GET_MEMORY_PAGE_SIZE()) ;
    return result ;
}
#define ALIGNED_STORAGE_SIZE aligned_storage_size()

static lock_object_t *storage_lock = NULL;
static unsigned long used_storage_no = 0 ;
static signed int lock_ref_count = 0;

// this operations (xxx_available_fileno) should be called
// in locked creation section
static int get_available_storage_no()
{
    unsigned long value = used_storage_no ;
    int i=0 ;
    while(i<PERFCOUNT_STORAGES_PER_PID_MAX) {
        if (!(value & 0x1))
            return i ;
        value >>= 1 ;
        ++i ;
    }
    return -1 ;
}

static void set_available_storage_no(int storage_no, bool available)
{
    used_storage_no = available ? used_storage_no & ~(1 << storage_no) :
        used_storage_no | (1 << storage_no) ;
    WRITE_TRACE(DBG_FATAL, "## available storages 0x%lx\n", used_storage_no) ;
}

class creation_locker {
        lock_object_t * _obj ;
    public:
        creation_locker(lock_object_t * obj)
            :_obj(obj)
        {
            if (_obj)
                obj->lock() ;
        }
        ~creation_locker() { unlock() ; }

        void unlock() {
            if (_obj)
                _obj->unlock() ;
            _obj = NULL ;
        }
} ;


static storage_mem_t* get_mem_ptr(counters_storage_t *storage)
{
	/* Local storage do not have storage_mem_t */
	PERFC_ASSERT(!storage->local) ;
	if (storage->local)
		return NULL;

    return (storage_mem_t*)((char*)storage - offsetof(struct _storage_mem_t, counters_storage)) ;
}

static counters_storage_t* get_storage_ptr(storage_mem_t *mem_ptr)
{
    if (!mem_ptr)
        return NULL ;
    return (counters_storage_t*)(mem_ptr->counters_storage) ;
}

inline bool is_valid_storage(storage_mem_t *mem_ptr)
{
    return mem_ptr && memcmp(&mem_ptr->mark, STORAGE_MARK, sizeof(STORAGE_MARK))==0 &&
        (mem_ptr->creater_pid != CREATER_PID_UNDEFINED()) ;
}

struct create_params {
        const char * filename ;
        int          storage_no ;
        const char * name ;
} ;
static int open_storage(file_handle_t fd, counters_storage_ptr *storage_ptr,
                 refcount_t *old_ref_count, create_params *create_prms = NULL)
{
    *old_ref_count = 0 ;
    *storage_ptr = NULL ;
    storage_mem_t *mem_ptr = (storage_mem_t*)memory_map(fd, ALIGNED_STORAGE_SIZE) ;
    if (!mem_ptr) {
        WRITE_TRACE(DBG_FATAL, "FAILED mmap for fd: %d, errno: %d\n", fd, errno) ;
        return ERR_MAP_FAILED ;
    }
    if (!create_prms && !is_valid_storage(mem_ptr))
    {
        char tmp[30] ;
        strncpy(tmp, (char*)mem_ptr, sizeof(tmp)-1) ;
        tmp[sizeof(tmp)-1] = 0 ;
        WRITE_TRACE(DBG_FATAL, "FAILED: wrong mark of mapped mem: %s\n", (char*)tmp) ;
        (void)tmp ;
        memory_unmap(mem_ptr, fd, ALIGNED_STORAGE_SIZE) ;
        return ERR_WRONG_MEM ;
    }

    *storage_ptr = get_storage_ptr(mem_ptr) ;
    if (create_prms)
    {
        WRITE_TRACE(DBG_FATAL, "New shared mem created, zero size: %u!\n",
					ALIGNED_STORAGE_SIZE) ;
        memset(mem_ptr, 0, ALIGNED_STORAGE_SIZE) ;
        memcpy(mem_ptr, STORAGE_MARK, sizeof(STORAGE_MARK)) ;
        WRITE_TRACE(DBG_FATAL, "mem map after zero: %s, in use: %lu\n", (char*)mem_ptr, (unsigned long)mem_ptr->ref_count) ;

        mem_ptr->storage_no = create_prms->storage_no;
        mem_ptr->creater_pid = (unsigned int)getpid() ;
        strcpy(mem_ptr->filename, create_prms->filename) ;
        strcpy((*storage_ptr)->descriptor.name, create_prms->name) ;

		// We use 'magic' to ease debugging of this memory block
		(*storage_ptr)->magic = PERF_COUNTERS_STORAGE_MAGIC;
    }

    if (mem_ptr->creater_pid == (unsigned int)getpid())
        AtomicInc(&mem_ptr->creater_ref_count) ;

    *old_ref_count = AtomicInc(&mem_ptr->ref_count) ;

    if (create_prms)
        SpinLockInit(&(*storage_ptr)->spinlock) ;

    WRITE_TRACE(DBG_FATAL, "mem map successfully opened: %p [%p] (%s), no: %d, in use: %u, name: %s, counters: %Lu\n",
                mem_ptr, *storage_ptr, (char*)mem_ptr, mem_ptr->storage_no, AtomicRead(&mem_ptr->ref_count),
                (*storage_ptr)->descriptor.name, PERF_COUNT_ATOMIC_GET(&(*storage_ptr)->descriptor)) ;

    return 0 ;
}

char* get_storage_filename(int pid, int i, char *result)
{
    snprintf(result, PATH_MAX-1, FILENAME_STORAGE_FMT, pid, i, get_process_starttime((pid_t)pid) ) ;
    return result ;
}
char* get_storage_filename(int i, char *result)
{
    return get_storage_filename((int)::getpid(), i, result) ;
}

int perf_create_storage(const char *name, storage_descriptor_t *result, lock_object_t *lock)
{
    result->storage = NULL ;
    result->internal = (void*)INVALID_FILE_HANDLE() ;

    if (strlen(name) > PERF_COUNT_NAME_LENGTH) {
        WRITE_TRACE(DBG_FATAL, "Storage name is too long '%s'\n", name) ;
        return ERR_LONG_NAME ;
    }

    creation_locker locker(lock);

    create_params create_prms ;
    create_prms.storage_no = get_available_storage_no() ;

    if (create_prms.storage_no<0) {
        WRITE_TRACE(DBG_FATAL, "Failed create storage: all %d storages is used.\n", PERFCOUNT_STORAGES_PER_PID_MAX) ;
        return ERR_OUTOF_LIMIT ;
    }

    char filename[PATH_MAX] ;
    file_handle_t fd = open_shared_file(get_storage_filename(create_prms.storage_no, filename), ALIGNED_STORAGE_SIZE) ;
    if (fd == INVALID_FILE_HANDLE())
    {
        WRITE_TRACE(DBG_FATAL, "## FAILED on creating shared file (%d): %s.\n", create_prms.storage_no,
                    get_storage_filename(create_prms.storage_no, filename)) ;
        return ERR_ACCESS_STORAGE ;
    }

    create_prms.filename = filename ;
    create_prms.name = name ;

    WRITE_TRACE(DBG_FATAL, "shared mem created: %s (%d)\n", filename, fd) ;
    refcount_t old_ref_count ;
    if (int res = open_storage(fd, &result->storage, &old_ref_count, &create_prms))
        return res ;

    result->internal = (void*)(ULONG_PTR)fd ;
    set_available_storage_no(create_prms.storage_no, false) ;
    return 0 ;
}

int perf_release_storage(storage_descriptor_t *s_descriptor, lock_object_t *lock)
{
    PERFC_ASSERT(s_descriptor) ;
    if (!s_descriptor->storage)
        return 0 ;

    creation_locker locker(lock);

    // if storage already deleted by another thread - do nothing
    if (!s_descriptor->storage)
        return 0 ;

	// Do not need to release local storage
	if (s_descriptor->storage->local)
		return 0;

    storage_mem_t * mem_ptr = get_mem_ptr(s_descriptor->storage) ;
    char del_filename[PATH_MAX] = "" ;

    if (AtomicDec(&mem_ptr->ref_count)<=1) {
		// we should release and delete file
        strcpy(del_filename, mem_ptr->filename) ;
        // clean storage mark for file wrongly reused
        memset(mem_ptr, 0, sizeof(mem_ptr->mark)) ;
    }

    WRITE_TRACE(DBG_FATAL, "## Release storage %p [%p] with no %d, shared file %s, name: %s.\n",
                mem_ptr, s_descriptor->storage, mem_ptr->storage_no,
                mem_ptr->filename, s_descriptor->storage->descriptor.name) ;

	s_descriptor->storage = NULL ;

	if (mem_ptr->creater_pid == (unsigned int)getpid() &&
        AtomicDec(&mem_ptr->creater_ref_count)<=1)
    {
        set_available_storage_no(mem_ptr->storage_no, true) ;
        mem_ptr->creater_pid = CREATER_PID_UNDEFINED() ;
    }

    memory_unmap(mem_ptr, *(file_handle_t*)&(s_descriptor->internal), ALIGNED_STORAGE_SIZE) ;

    s_descriptor->internal = (void*)INVALID_FILE_HANDLE() ;

    if (*del_filename) {
        WRITE_TRACE(DBG_FATAL, "## Delete shared file '%s'.\n", del_filename) ;
        remove_shared_file(del_filename) ;
    }

    return 0 ;
}

bool perf_storage_alive(storage_descriptor_t *s_descriptor, lock_object_t *lock)
{
    PERFC_ASSERT(s_descriptor) ;
    if (!s_descriptor->storage)
        return false ;

	creation_locker locker(lock);

    if (!s_descriptor->storage)
        return false ;

	// local storage is always alive
	if (s_descriptor->storage->local)
		return true;

    storage_mem_t * mem_ptr = get_mem_ptr(s_descriptor->storage) ;
    if (mem_ptr->creater_pid == CREATER_PID_UNDEFINED())
        return false ;

    if (processes_alive(mem_ptr->creater_pid))
        return true ;

    WRITE_TRACE(DBG_FATAL, "## Dead process %d shared mem found. Mark it as not alive.\n",
                mem_ptr->creater_pid) ;
    memset(mem_ptr, 0, sizeof(mem_ptr->mark)) ;
    mem_ptr->creater_pid = CREATER_PID_UNDEFINED() ;
    return false ;
}

class storage_helper {
        storage_descriptor_t _descriptor ;
        refcount_t           _ref_count ;
		lock_object_t		*_lock;
    public:
        storage_helper(file_handle_t fd, lock_object_t *lock):_ref_count(0), _lock(lock)
        {
            _descriptor.internal = (void*)(ULONG_PTR)fd ;
			_lock->lock();
            if (open_storage(fd, &_descriptor.storage, &_ref_count, NULL))
				_descriptor.internal = (void*)INVALID_FILE_HANDLE();
			_lock->unlock();
        }
        ~storage_helper()
        {
            if (_descriptor.storage) {
                perf_release_storage(&_descriptor, _lock) ;
            }
        }
        const storage_descriptor_t& descriptor() { return _descriptor ; }
        counters_storage_t * storage() { return descriptor().storage ; }
        bool ref_count() { return _ref_count >= 1 ; }

        counters_storage_t * zero() {
            counters_storage_t * result = storage() ;
			_descriptor.internal = (void*)INVALID_FILE_HANDLE();
            _descriptor.storage = NULL ;
            _ref_count = 0 ;
            return result ;
        }
} ;

#if _LIN_

#include <sys/types.h>
#include <dirent.h>

static int perf_process_shared_storage(enum_storage_proc proc, const char *fname, void *user_data, lock_object_t *lock)
{
    file_handle_t fd = open_shared_file(fname, 0) ;
    if ((int)fd < 0)
        return ENUM_CONTINUE ;

    WRITE_TRACE(DBG_FATAL, "shared mem found: %s (%d)\n", fname, fd) ;
    storage_helper storage_helper(fd, lock) ;
    /* descriptor was mapped - we can close it */
    close(fd) ;
    if (!storage_helper.ref_count()) {
        WRITE_TRACE(DBG_FATAL, "skip not used shared mem: %s (%d)\n", fname, fd) ;
        return ENUM_CONTINUE ;
    }
    int result = proc(&storage_helper.descriptor(), user_data) ;
    if (result & ENUM_DONOT_RELEASE_STORAGE)
        storage_helper.zero() ;
    return result ;
}

static void perf_process_pid_storages(enum_storage_proc proc, pid_t pid, void *user_data, lock_object_t *lock)
{
    char buff[PATH_MAX] ;
    int result;

    for(unsigned int i=0; i<PERFCOUNT_STORAGES_PER_PID_MAX; ++i)
    {
        result = perf_process_shared_storage(proc, get_storage_filename(pid, i, buff), user_data, lock);
        if (result & ENUM_BREAK)
            break;
    }
}

static void perf_process_all_storages(enum_storage_proc proc, void *user_data, lock_object_t *lock)
{
    int result;
    struct dirent *de ;

    DIR *shm_dir = opendir("/dev/shm") ;
    if (!shm_dir)
        return;

    while( (de = readdir(shm_dir)) != NULL) {
        if (strncmp(de->d_name, FILENAME_STORAGE_HINT, strlen(FILENAME_STORAGE_HINT)))
            continue;
        result = perf_process_shared_storage(proc, de->d_name, user_data, lock);
        if (result & ENUM_BREAK)
            break;
    }
    closedir(shm_dir);
}

void perf_enum_process_storages(enum_storage_proc proc, unsigned int pid, void *user_data, lock_object_t *lock)
{
    if (pid != (unsigned int)-1)
	perf_process_pid_storages(proc, pid, user_data, lock);
    else
	perf_process_all_storages(proc, user_data, lock);
}
#else
void perf_enum_process_storages(enum_storage_proc proc, unsigned int pid, void *user_data, lock_object_t *lock)
{
    char buff[PATH_MAX] ;
    pid_t p_list[1024] ;
    int p_count ;

    if ((p_count=get_processes_list(ARRAY_AND_SIZE(p_list)))<=0)
        return ;

    //WRITE_TRACE(DBG_FATAL, "Got %d processes\n", p_count) ;
    if (pid != (unsigned int)-1)
    {
        // try find specified process and make it first and single in list
        for(int i=p_count; i--; )
            if ((unsigned int)p_list[i] == pid) {
                p_list[0] = p_list[i] ;
                p_count = 1 ;
                break ;
            }
        if ((unsigned int)p_list[0] != pid)
            return ; // specified process not found
    }

    for(; p_count--; )
    {
        for(unsigned int i=0; i<PERFCOUNT_STORAGES_PER_PID_MAX; ++i)
        {
            file_handle_t fd = open_shared_file(get_storage_filename(p_list[p_count], i, buff), 0) ;
            if ((int)fd < 0)
                continue ;
            WRITE_TRACE(DBG_FATAL, "shared mem found: %s (%d)\n", buff, fd) ;
            storage_helper storage_helper(fd, lock) ;
            if (!storage_helper.ref_count()) {
                WRITE_TRACE(DBG_FATAL, "skip not used shared mem: %s (%d)\n", buff, fd) ;
                continue ;
            }
            int result = proc(&storage_helper.descriptor(), user_data) ;
            if (result & ENUM_DONOT_RELEASE_STORAGE)
                storage_helper.zero() ;
            if (result & ENUM_BREAK)
                break ;
        }
    }
}
#endif

int s_find_by_name(const storage_descriptor_t *sd, void *user_data)
{
    pvt_perf_find_named_value *named_value = (pvt_perf_find_named_value*)user_data ;
    if (strcmp(sd->storage->descriptor.name, named_value->name)==0) {
        named_value->result.sd = *sd ;
        return ENUM_BREAK | ENUM_DONOT_RELEASE_STORAGE ;
    }
    return ENUM_CONTINUE ;
}

storage_descriptor_t perf_find_process_storage(const char *name, unsigned int pid, lock_object_t *lock)
{
    pvt_perf_find_named_value result = pvt_perf_FIND_NAMED_VALUE_INITIALIZER(result, name) ;
    perf_enum_process_storages(s_find_by_name, pid, &result, lock) ;
    return result.result.sd ;
}

lock_object_t *perf_get_storage_lock()
{
	if (storage_lock == NULL)
		storage_lock = new lock_object_t();
	AtomicInc(&lock_ref_count);
	return storage_lock;
}

void perf_release_storage_lock()
{
	if (AtomicDec(&lock_ref_count) <= 0)
	{
		delete storage_lock;
		storage_lock = NULL;
	}
}
