//////////////////////////////////////////////////////////////////////////
///
/// @file PerfCounter.h
///
/// @brief Performance counters library that could be used around all
/// system components (Host UserSpace + Host Kernel + VMM)
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

#ifndef __PREF_COUNTER_H__
#define __PREF_COUNTER_H__

#include "Interfaces/ParallelsTypes.h"

#include <prlcommon/Std/SpinLock.h>
#include <prlcommon/Std/AtomicOps.h>

#include <limits.h>
#include <string.h>
#include "Libraries/PerfCount/PerfLib/platform_spec.h"

#ifndef PERFC_ASSERT
#include <assert.h>
#define PERFC_ASSERT( test ) assert(test)
#endif

/**
 * Default counter type definition
 */
typedef LONG64 T_COUNTER;

/**
 * Performance counters name consists of 2 parts:
 * meta_info + @ +counter_name
 * Graphical tools should ignore meta information part and print
 * only name after "@" symbol
 * Meta-Symbols:
 *   [A]bsolute value (represents some absolute value, like current CPU usage
 *   [I]ncremental value (some value that constantly grows, like I/O bytes count
 */
#define PERF_COUNT_TYPE_ABS "A@"
#define PERF_COUNT_TYPE_INC "I@"

/**
 * Counter accessors. All users must use these macros to
 * manipulate perf counters.
 */
#define PERF_COUNT_GET(counter)					PERF_COUNT_VAL(counter)
#define PERF_COUNT_SET(counter, val)			do {PERF_COUNT_VAL(counter) = (val);} while(0)
#define PERF_COUNT_ADD(counter, val)			do {PERF_COUNT_VAL(counter) += (val);} while(0)
#define PERF_COUNT_SUB(counter, val)			do {PERF_COUNT_VAL(counter) -= (val);} while(0)
#define PERF_COUNT_INC(counter)					do {PERF_COUNT_VAL(counter)++;} while(0)
#define PERF_COUNT_DEC(counter)					do {PERF_COUNT_VAL(counter)--;} while(0)
#define PERF_COUNT_ATOMIC_GET(counter)          AtomicRead64(&PERF_COUNT_VAL(counter))
#define PERF_COUNT_ATOMIC_SET(counter, val)     AtomicSwap64(&PERF_COUNT_VAL(counter), val)
#define PERF_COUNT_ATOMIC_INC(counter)          AtomicInc64 (&PERF_COUNT_VAL(counter))
#define PERF_COUNT_ATOMIC_DEC(counter)          AtomicDec64 (&PERF_COUNT_VAL(counter))
#define PERF_COUNT_ATOMIC_ADD(counter, val)     AtomicAdd64 (&PERF_COUNT_VAL(counter), val)

#define PERFCOUNT_STORAGES_PER_PID_MAX (5)
#define PERFCOUNT_PAGES_PER_STORAGE (140)
#define PERFCOUNT_STORAGE_WIDTH (256) /* 16 counters x 140 pages = 2240 counters limit */

/*
  Performance counters error codes
 */
enum perf_errors {
    ERR_LONG_NAME = -20,
    ERR_OUTOF_LIMIT,
    ERR_EXISTS,
    ERR_MAP_FAILED,
    ERR_WRONG_MEM,
    ERR_ACCESS_STORAGE,
    ERR_STORAGE_LOCK,
    ERR_ALLOCATE_MEM,
    ERR_NOT_OPENED,

    ERR_NOT_FOUND,

    ERR_UNKNOWN = -1,
    ERR_NO_ERROR = 0
} ;

#include <Interfaces/packed.h>

/*
  Performance counter enumeration proc result values
 */
enum perf_enum_proc_result {
    ENUM_CONTINUE = 0,
    ENUM_BREAK    = 0x1,

    // only for storage enumeration, to return storage from it
    ENUM_DONOT_RELEASE_STORAGE = 0x80
} ;

#define PERF_COUNT_NAME_LENGTH	(PERFCOUNT_STORAGE_WIDTH - sizeof(T_COUNTER))
#define PERF_COUNT_VAL(c)		((c)->value__dont_use_directly)
/*
  counter_t struct aligned to STORAGE_WIDTH
 */
typedef struct _counter_t {
        char      name[PERF_COUNT_NAME_LENGTH] ;
        T_COUNTER value__dont_use_directly;
} counter_t;
typedef counter_t* counter_ptr ;

struct lock_object_t;

/*
  calculate counters count per storage

  struct counters_storage_t should be aligned to memory page
  but we also reserve RESERVED_SIZE in this memory for internal use
 */
#define PERFCOUNT_RESERVED_SIZE (PERFCOUNT_STORAGE_WIDTH)
#define PERFCOUNT_PAGE_SIZE (0x1000)

#define PERFCOUNT_COUNTERS_PER_STORAGE \
    ((PERFCOUNT_PAGE_SIZE*PERFCOUNT_PAGES_PER_STORAGE - \
      PERFCOUNT_RESERVED_SIZE - sizeof(SPINLOCK) - \
      sizeof(counter_t)) / sizeof(counter_t))

/*
  counters storage memory struct
  descriptor identify storage name and counters count
*/
#define PERF_COUNTERS_STORAGE_MAGIC (0xDEAD7770)
typedef struct _counters_storage_t {
	unsigned int magic;
	SPINLOCK  spinlock;
	unsigned int __pad;
	counter_t descriptor ;
	counter_t counters[ PERFCOUNT_COUNTERS_PER_STORAGE ] ;
	/*
	 * Local storage lives in this process, it's always active
	 * it doesn't have pid, shared memory file, etc. For example
	 * host statistics can be implemented as local storage.
	 */
	int local;
} counters_storage_t;
typedef counters_storage_t* counters_storage_ptr ;

BUILD_BUG_ON(offsetof(counters_storage_t, descriptor) & 7);

#define PERF_COUNTERS_STORAGE_PAGES (BYTES2PAGES(sizeof(counters_storage_t)))

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
/*
  storage holder
*/
typedef struct {
        void               *internal ;
        counters_storage_t *storage ;
} storage_descriptor_t ;

#include <Interfaces/unpacked.h>


/*
  Creates new storage with specified name and return it in @result
  Returns 0 on success else one of errors
*/
int perf_create_storage(const char *name, storage_descriptor_t *result, lock_object_t *lock) ;
/*
  Release storage
  Returns 0 on success else one of errors

  if storage_descriptor->storage==NULL do nothing

  on success storage_descriptor->storage will be filled with NULL
*/
int perf_release_storage(storage_descriptor_t *storage_descriptor, lock_object_t *lock) ;
/*
  Checks that storage is actual
*/
bool perf_storage_alive(storage_descriptor_t *storage_descriptor, lock_object_t *lock) ;

typedef int (*enum_storage_proc)(const storage_descriptor_t *storage, void *user_data) ;
/*
  Enumerates shared storages
  @proc - should return combination of enum_proc_result values
  @pid  - process id, for all should be (unsigned int)-1

  enumeration open storage and passes it to @proc,
  if @proc result contains ENUM_DONOT_RELEASE_STORAGE
  the storage will NOT be released, it should be released manually
*/
void perf_enum_process_storages(enum_storage_proc proc, unsigned int pid, void *user_data, lock_object_t *lock) ;

__inline void perf_enum_storages(enum_storage_proc proc, void *user_data, lock_object_t *lock)
{
    perf_enum_process_storages(proc, (unsigned int)-1, user_data, lock) ;
}

/*
  Find storage by @name and @pid
  @pid  - process id, for all should be (unsigned int)-1

  if nothing found returned value of storage==NULL

  returned storage should be released
*/
storage_descriptor_t perf_find_process_storage(const char *name, unsigned int pid, lock_object_t *lock) ;

__inline storage_descriptor_t perf_find_storage(const char *name, lock_object_t *lock)
{
    return perf_find_process_storage(name, (unsigned int)-1, lock) ;
}

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#ifndef _PARAVIRT_

/*
 private methods
*/

typedef struct {
        const char *name ;
        union {
                counter_t *counter ;
                storage_descriptor_t sd ;
        } result ;
} pvt_perf_find_named_value ;
#define pvt_perf_FIND_NAMED_VALUE_INITIALIZER(value, name) \
    {name, {0}}; memset(&value.result, 0, sizeof(value.result))

__inline int pvt_perf_counter_finder(counters_storage_t*, counter_t *counter, void *user_data)
{
    pvt_perf_find_named_value *named_value = (pvt_perf_find_named_value*)user_data ;
    if (strcmp(counter->name, named_value->name)==0) {
        named_value->result.counter = counter ;
        return ENUM_BREAK ;
    }
    return ENUM_CONTINUE ;
}


typedef int (*enum_counter_proc)(counters_storage_t *storage, counter_t *counter, void *user_data) ;
/*
  Enumerates counters in @storage
  @proc should return ENUM_BREAK to break enumaration
*/
__inline void perf_enum_counters(counters_storage_t *storage, enum_counter_proc proc, void *user_data)
{
    PERFC_ASSERT(storage) ;
    { // msvc c compile compatibility
    T_COUNTER count = PERF_COUNT_ATOMIC_GET(&storage->descriptor) ;

	// #425829 TEMPORALLY  WORKARROUND ONLY
	// PERFC_ASSERT( count < PERFCOUNT_COUNTERS_PER_STORAGE );
	if( !(count<(T_COUNTER)PERFCOUNT_COUNTERS_PER_STORAGE) )
		return;

    counter_t *counter ;
    for(counter=storage->counters; count--;)
        if (proc(storage, counter++, user_data) & ENUM_BREAK)
            break ;
    }
}

/*
  Find counter in storage by name
  Return NULL if nothing found
*/
__inline counter_t* perf_find_counter(counters_storage_t *storage, const char *name)
{
    pvt_perf_find_named_value result = pvt_perf_FIND_NAMED_VALUE_INITIALIZER(result, name) ;
    perf_enum_counters(storage, pvt_perf_counter_finder, &result) ;
    return result.result.counter ;
}

/*
  Add counter with specified name and return it in @counter_ptr
  If counter with specified name already exists @counter_ptr will point on it
  and method returns ERR_EXISTS.
  Returns 0 on success.
*/
__inline int perf_add_counter(counters_storage_t *storage,
                            const char *name,
                            counter_ptr *counter_ptr)
{
	int result = 0;
	T_COUNTER count;

    PERFC_ASSERT( storage ) ;
    PERFC_ASSERT( name ) ;
    PERFC_ASSERT( counter_ptr ) ;

    if (strlen(name) > (sizeof((*counter_ptr)->name)-1))
        return ERR_LONG_NAME ;

	SpinLockLock(&storage->spinlock);

    *counter_ptr = NULL ;

    count = PERF_COUNT_ATOMIC_GET(&storage->descriptor) ;
    if (count < (unsigned) PERFCOUNT_COUNTERS_PER_STORAGE) {


        *counter_ptr = perf_find_counter(storage, name) ;
        if (!*counter_ptr) {
            *counter_ptr = &storage->counters[count] ;
            strcpy((*counter_ptr)->name, name) ;
            PERF_COUNT_ATOMIC_INC(&storage->descriptor) ;
        }
        else
            result = ERR_EXISTS ;
    }
    else
        result = ERR_OUTOF_LIMIT ;

	SpinLockUnlock(&storage->spinlock);

    return result ;
}


#if defined(__cplusplus)

/*
  Create or returns pointer to global storage lock
*/
lock_object_t *perf_get_storage_lock();

/*
  Release or destroy global storage lock
*/
void perf_release_storage_lock();

    class CounterStorageT {
            storage_descriptor_t _storage ;
            int                  _error ;
            int					 _storage_error;
			lock_object_t		*_lock;

            inline void init()
            {
                _error = ERR_NO_ERROR ;
                _storage_error = ERR_NOT_OPENED;
                _lock = perf_get_storage_lock();
                memset(&_storage, 0, sizeof(_storage)) ;
            }

            CounterStorageT(const CounterStorageT&) ;
            void operator=(const CounterStorageT&) ;
        public:

			inline counters_storage_t* storage() { return _storage.storage ; }

			CounterStorageT() { init() ; }

            /*
              create or open storage with @name depending on @create flag
             */
            explicit CounterStorageT(const char * name, bool create = true)
            {
                init() ;
                create ? recreate(name) : open_storage(name) ;
            }

            /*
              opens storage with @name of process @pid
             */
            explicit CounterStorageT(const char * name, unsigned int pid)
            {
                init() ;
                open_storage(name, pid) ;
            }

            ~CounterStorageT()
            {
            	release() ;
            	perf_release_storage_lock() ;
            }

            /*
              recreate storage with specified @name

              !ATTENTION! the old one opened or created storage will be released
             */
            int recreate(const char *name)
            {
                release() ;
                _storage_error = perf_create_storage(name, &_storage, _lock) ;
                return _storage_error ;
            }

            /*
              opens storage by @name and @pid

              !ATTENTION! the old one opened or created storage will be released

              @pid - enum only storages for specified process id
             */
            int open_storage(const char *name, unsigned int pid = (unsigned int)-1)
            {
                release() ;
                _storage = perf_find_process_storage(name, pid, _lock) ;
                _storage_error = !_storage.storage ? ERR_NOT_FOUND : 0 ;
                return _storage_error ;
            }


            /*
              release previously created or opened storage
              do nothing if no one was openned earlier
             */
            void release()
            {
                if (valid())
                    perf_release_storage(&_storage, _lock) ;
            }

            bool valid()
            {
            	return (_storage_error == ERR_NO_ERROR) && !!storage() ;
            }

            int error() { return _error ; }

            inline counter_t* add_counter(const char *name)
            {
                PERFC_ASSERT(valid()) ;
                counter_t *result = NULL;
                if (!valid())
                    return NULL;

                _error = perf_add_counter(storage(), name, &result) ;
                return result ;
            }

            void enum_counters(enum_counter_proc proc, void *user_data)
            {
                PERFC_ASSERT(valid()) ;
                if (!valid())
                    return ;
                perf_enum_counters(storage(), proc, user_data) ;
            }

            counter_t* find(const char *counter_name)
            {
                PERFC_ASSERT(valid()) ;
                if (!valid())
                    return NULL ;
                return perf_find_counter(storage(), counter_name) ;
            }
    } ;

template<const int Size>
struct simple_container_t {

        typedef storage_descriptor_t * iterator ;

        storage_descriptor_t array[Size] ;
        size_t               size ;

        simple_container_t():size(0) {}

        void push_back(const storage_descriptor_t &storage)
        {
            PERFC_ASSERT(size < Size) ;
            if (size >= Size)
                return ;
            array[size++] = storage ;
        }
        iterator begin() { return array ; }
        iterator end() { return array+size ; }

        iterator erase(iterator b, iterator e)
        {
            PERFC_ASSERT(b <= e) ;
            iterator it_end = end() ;
            PERFC_ASSERT( (b >= begin()) && (e <= it_end)) ;
            if (e==b)
                return e ;

            PERFC_ASSERT(size) ;
            size -= e - b ;
            if (e != it_end)
                memmove(b, e, it_end - e) ;
            return e ;
        }
} ;

template<typename T>
class PerfStoragesContainer_t {
        typedef T ContainerT ;
        typedef typename ContainerT::iterator iterator ;

        typedef PerfStoragesContainer_t<T> SelfT ;

        ContainerT _container ;
        bool       _empty ;
		lock_object_t *_lock;

    public:
        PerfStoragesContainer_t():_empty(true)
        {
        	_lock = perf_get_storage_lock();
        }

        PerfStoragesContainer_t(unsigned int pid):_empty(true)
        {
        	_lock = perf_get_storage_lock();
            Refresh(pid) ;
        }

        ~PerfStoragesContainer_t()
        {
            Clean() ;
            perf_release_storage_lock();
        }

        storage_descriptor_t AddNewStorage(const char *storage_name)
        {
            storage_descriptor_t result ;
            perf_create_storage(storage_name, &result, _lock) ;
            // I do not check for result of push_back,
            //   I respose that size of static, if it used array enough large !
            //   in debug it will failed by assert
            _container.push_back(result) ;
            _empty = false ;
            return result ;
        }

        bool IsEmpty() const { return _empty ; }

        void Refresh(unsigned int pid = (unsigned int)-1)
        {
            Clean() ;
            perf_enum_process_storages(enum_storages, pid, &_container, _lock) ;
        }

        void Clean()
        {
            if (IsEmpty())
                return ;
            iterator beg = _container.begin(),
                end = _container.end() ;
            for(iterator it=beg; it != end; ++it)
                perf_release_storage(&*it, _lock) ;

            if (beg!=end)
                _container.erase(beg, end) ;
            _empty = true ;
        }

        const ContainerT& Container() const { return _container ; }

        /*
          note1: this enumerator does not break enumeration of storages, on proc return ENUM_BREAK
		  note2: if you use "release_dead_counters = true" you should provide syncronized access to this call.
         */
        void enum_counters(enum_counter_proc proc, void *user_data ) const
        {
			enum_counters( proc, user_data, false );
		}

		void release_dead_counters(enum_counter_proc proc, void *user_data ) const
		{
			(void)proc;
			(void)user_data;

			int fake;
			enum_counters( fake_proc, (void*)&fake, true );
		}


    private:
        static int enum_storages(const storage_descriptor_t *storage, void *user_data)
        {
            SelfT * self = (SelfT*)user_data ;
            self->_container.push_back(*storage) ;
            self->_empty = false ;
            return ENUM_CONTINUE | ENUM_DONOT_RELEASE_STORAGE ;
        }

		static int fake_proc( const storage_descriptor_t *, void * )
		{
			return ENUM_CONTINUE;
		}

		void enum_counters(enum_counter_proc proc, void *user_data, bool release_dead_counters ) const
		{
			if (IsEmpty())
				return ;
			SelfT * self = const_cast<SelfT*>(this) ;
			for(iterator it = self->_container.begin(),
				end = self->_container.end(); it!=end; ++it)
			{
				storage_descriptor_t *sd = &*it ;
				if (!sd->storage)
					continue ;
				if (perf_storage_alive(sd, _lock))
					perf_enum_counters(sd->storage, proc, user_data) ;
				else if( release_dead_counters )
					perf_release_storage(sd, _lock) ;
			}
		}

} ;
typedef PerfStoragesContainer_t<simple_container_t<PERFCOUNT_STORAGES_PER_PID_MAX> > ProcPerfStoragesContainer ;

#endif  // __cplusplus

#endif // _PARAVIRT_

#endif //__PREF_COUNTER_H__
