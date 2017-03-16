//////////////////////////////////////////////////////////////////////////
///
/// @file perfcounter_test.cpp
///
/// @brief Performance counters library that could be used around all
/// system components (Host UserSpace + Host Kernel + VMM)
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <typeinfo>

#include "PerfLib/PerfCounter.h"

#include "PerfLib/trace.h"

#ifdef _WIN_

#define sleep(msec) Sleep(msec)
#include <conio.h>

#else

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <dirent.h>
#include <unistd.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>

inline int _getch() { return getchar() ; }

#endif // #ifdef _WIN_


#define TRACE_INPLACE(text) printf("<-- %s -->", text) ;

lock_object_t *perf_storage_lock;


template<typename T>
struct value_compare {

        static bool cmp(const T &v1, const T &v2) {
            TRACE_INPLACE("default compare") ;
            return v1 == v2 ;
        }
} ;

template<>
struct value_compare<const char*> {
        typedef char * castT ;

        static bool cmp(const char *v1, const char *v2) {
            TRACE_INPLACE("char* compare") ;
            return strcmp(v1, v2) == 0 ;
        }
} ;

template<const int size>
struct value_compare<const char[size]> {

        static bool cmp(const char *v1, const char *v2) {
            return value_compare<const char*>::cmp(v1, v2) ;
        }
} ;


template<>
struct value_compare<const counter_t*> {

        static bool cmp(const counter_t * v1, const counter_t * v2) {
            TRACE_INPLACE("counter_t* compare") ;
            if (!v1 || !v2)
                return v1==v2 ;
            return v1==v2 ||
                (strcmp(v1->name, v2->name) == 0 && PERF_COUNT_GET(v1)==PERF_COUNT_GET(v2)) ;
        }
} ;

template<>
struct value_compare<counter_t*> {

        static bool cmp(const counter_t * v1, const counter_t * v2) {
            return value_compare<const counter_t*>::cmp(v1, v2) ;
        }
} ;

#define TEST_FLUSH()  //fflush(stdout)

inline void TEST_LOG(const char *format, ...)
{
    va_list vargs ;
    va_start(vargs, format) ;

    vprintf( format, vargs) ;
    TEST_FLUSH() ;
    va_end(vargs) ;
}

#define TEST_START()                                                    \
    TEST_LOG("\n------------- %s ----------------\n", __FUNCTION__) ;

#define TEST_FINISHED()                                                 \
    TEST_LOG("\n------------- %s  successfully finished----\n", __FUNCTION__) ;

#define TEST_ASSERT(call)                                   \
    printf("... Test '%s' ", #call) ;                       \
    TEST_FLUSH() ;                                          \
    check_call((call), __FUNCTION__, __FILE__, __LINE__)

#define TEST_COMPARE(call, expected)                                \
    printf("... Test compare '%s' == ", #call) ;                    \
    TEST_FLUSH() ;                                                  \
    {                                                               \
        char buff[1024] ;                                           \
        printf("'%s' ", value2str(expected, buff)) ;                \
    }                                                               \
    check_call((call), expected, __FUNCTION__, __FILE__, __LINE__)

#define TEST_LOG_CALL(call)                     \
    TEST_LOG( "... Test run: '%s'", #call ) ;   \
    TEST_FLUSH() ;                              \
    call ;                                      \
    TEST_LOG( " OK\n")

bool inline check_value(bool result) { return result ; }

bool inline check_value(int result) { return result==0 ; }

bool inline check_value(void * object) { return !!object ; }

template<typename R, typename T>
bool inline compare_values(const R &value, const T &expected)
{
    return value_compare<T>::cmp(value, expected) ;
}

template<typename R>
inline R check_call(R value, const char *func, const char *file, int line_no)
{
    if (!check_value(value))
    {
        char buff_val[2048] ;
        printf("FAILED!!! returns: '%s'; (%s, %s:%d)\n",
               value2str(value, buff_val), func, file, line_no) ;
        exit(1);
    }
    printf("OK\n");
    return value ;
}

template<typename R, typename T>
inline R check_call(R value, T expected, const char *func, const char *file, int line_no)
{
    char buff[2048] ;
    if (!compare_values(value, expected))
    {
        printf("FAILED!!! Got: '%s'; (%s, %s:%d)\n", value2str(value, buff), func, file, line_no) ;
        exit(1);
    }
    printf("OK\n");
    return value ;
}

#define STORAGE_NAME_PREFIX "storage_test_"
#define STORAGE_NAME(suffix) STORAGE_NAME_PREFIX#suffix

struct count_params_t {
        const char *prefix ;
        size_t      prefix_len ;
        size_t      count  ;
} ;
inline count_params_t* init_count(count_params_t *prm)
{
    prm->prefix_len = strlen(prm->prefix) ;
    prm->count = 0 ;
    return prm ;
}

int count_test_storages(const storage_descriptor_t *sd, void *count_prm)
{
    count_params_t *prm = (count_params_t*)count_prm ;
    WRITE_TRACE(DBG_FATAL, "count_test_storages: %d, prefix: %s (%d), compare with: '%s', res: %d\n",
                prm->count, prm->prefix, prm->prefix_len,
                sd->storage->descriptor.name,
                strncmp(sd->storage->descriptor.name, prm->prefix, prm->prefix_len)) ;

    if (strncmp(sd->storage->descriptor.name, prm->prefix, prm->prefix_len)==0)
        ++prm->count ;
    return ENUM_CONTINUE ;
}

int count_test_counters(counters_storage_t *, counter_t *counter, void *count_prm)
{
    count_params_t *prm = (count_params_t*)count_prm ;
    //printf("...\n--------- TEST COUNTER: '%s', prefix '%s' (%d)\n...",
    //       counter->name, prm->prefix, prm->prefix_len) ;
    if (strncmp(counter->name, prm->prefix, prm->prefix_len)==0)
        ++prm->count ;
    return ENUM_CONTINUE ;
}


void perf_unit_test_storage()
{
    TEST_START() ;

    storage_descriptor_t tmp_sd = {0,0} ;
    storage_descriptor_t sd[PERFCOUNT_STORAGES_PER_PID_MAX+1] ;
    memset(sd, 0, sizeof(sd)) ;
    count_params_t st_count = { STORAGE_NAME(st_), 0, 0 } ;

    TEST_LOG_CALL( perf_enum_storages(count_test_storages, init_count(&st_count), perf_storage_lock) ) ;
    TEST_COMPARE( st_count.count, 0U ) ;

    TEST_ASSERT( perf_create_storage(STORAGE_NAME(st_0), &sd[0], perf_storage_lock) ) ;
    TEST_ASSERT( sd[0].storage ) ;
    TEST_COMPARE( sd[0].storage->descriptor.name, STORAGE_NAME(st_0) ) ;

    TEST_LOG_CALL( perf_enum_storages(count_test_storages, init_count(&st_count), perf_storage_lock) ) ;
    TEST_COMPARE( st_count.count, 1U ) ;

    TEST_ASSERT( perf_create_storage(STORAGE_NAME(st_1), &sd[1], perf_storage_lock) ) ;
    TEST_ASSERT( sd[1].storage ) ;
    TEST_ASSERT( sd[1].storage != sd[0].storage ) ;

    TEST_LOG_CALL( perf_enum_storages(count_test_storages, init_count(&st_count), perf_storage_lock) ) ;
    TEST_COMPARE( st_count.count, 2U ) ;

    TEST_ASSERT( (tmp_sd = perf_find_storage(STORAGE_NAME(st_0), perf_storage_lock)).storage != NULL ) ;
    TEST_LOG( "storage [%p] (%s) found.\n", tmp_sd.storage, tmp_sd.storage->descriptor.name ) ;
    TEST_LOG_CALL( perf_release_storage(&tmp_sd, perf_storage_lock) ) ;

    TEST_LOG_CALL( perf_enum_storages(count_test_storages, init_count(&st_count), perf_storage_lock) ) ;
    TEST_COMPARE( st_count.count, 2U ) ;

    TEST_ASSERT( perf_create_storage(STORAGE_NAME(st_2), &sd[2], perf_storage_lock) ) ;
    TEST_ASSERT( sd[2].storage ) ;

    TEST_LOG_CALL( perf_enum_storages(count_test_storages, init_count(&st_count), perf_storage_lock) ) ;
    TEST_COMPARE( st_count.count, 3U ) ;

    for(unsigned int i=st_count.count; i<PERFCOUNT_STORAGES_PER_PID_MAX; ++i)
    {
        TEST_ASSERT( perf_create_storage(STORAGE_NAME(st_tmp), &sd[i], perf_storage_lock) ) ;
        TEST_ASSERT( sd[i].storage ) ;
    }
    for(unsigned int i=PERFCOUNT_STORAGES_PER_PID_MAX; i-->1; ) {
        for(int j=i; j--; ) {
            TEST_ASSERT( sd[i].storage!=sd[j].storage ) ;
        }
    }

    TEST_LOG_CALL( perf_enum_storages(count_test_storages, init_count(&st_count), perf_storage_lock) ) ;
    TEST_COMPARE( st_count.count, PERFCOUNT_STORAGES_PER_PID_MAX ) ;

    TEST_ASSERT( perf_release_storage(&sd[1], perf_storage_lock) ) ;
    TEST_ASSERT( sd[1].storage == NULL ) ;

    TEST_LOG_CALL( perf_enum_storages(count_test_storages, init_count(&st_count), perf_storage_lock) ) ;
    TEST_COMPARE( st_count.count, PERFCOUNT_STORAGES_PER_PID_MAX-1 ) ;

    TEST_ASSERT( perf_create_storage(STORAGE_NAME(st_new_1), &sd[1], perf_storage_lock) ) ;
    TEST_ASSERT( sd[1].storage ) ;

    TEST_LOG_CALL( perf_enum_storages(count_test_storages, init_count(&st_count), perf_storage_lock) ) ;
    TEST_COMPARE( st_count.count, PERFCOUNT_STORAGES_PER_PID_MAX ) ;

    TEST_ASSERT( !!(sd[PERFCOUNT_STORAGES_PER_PID_MAX].storage = sd[1].storage) ) ;
    TEST_ASSERT( perf_create_storage(STORAGE_NAME(st_more),
                                     &sd[PERFCOUNT_STORAGES_PER_PID_MAX], perf_storage_lock)==ERR_OUTOF_LIMIT ) ;
    TEST_ASSERT( sd[PERFCOUNT_STORAGES_PER_PID_MAX].storage==NULL ) ;

    TEST_LOG_CALL( perf_enum_storages(count_test_storages, init_count(&st_count), perf_storage_lock) ) ;
    TEST_COMPARE( st_count.count, PERFCOUNT_STORAGES_PER_PID_MAX ) ;

    for(int i=PERFCOUNT_STORAGES_PER_PID_MAX; i--; )
    {
        TEST_ASSERT( perf_release_storage(&sd[i], perf_storage_lock) ) ;
        TEST_ASSERT( sd[i].storage==NULL ) ;
    }

    TEST_LOG_CALL( perf_enum_storages(count_test_storages, init_count(&st_count), perf_storage_lock) ) ;
    TEST_COMPARE( st_count.count, 0U ) ;

    TEST_FINISHED() ;
}

/*
  !!!!!!!!!!!!!!!!!
  these CONSTANT values should be declared as NON const
  because of on comparing this counter_t and from storage
  AtomicRead operation will SEGFAULT on constant memory
*/
static counter_t st0_counter_vals[] = {
    {"counter0", 999},
    {"counter1", 333},
    {"counter2", 0}
} ;
static counter_t st1_counter_vals[] = {
    {"couNTer0", 223},
    {"couNter1", 222},
    {"counter2", 555}
} ;

void perf_unit_test_writer()
{
    TEST_ASSERT( (perf_unit_test_storage(), true) ) ;

    TEST_START() ;

    storage_descriptor_t sd[2] ;
    count_params_t st_count = { STORAGE_NAME(wrt_), 0, 0 } ;
    count_params_t c_count = { "cou", 0, 0 } ;

    TEST_LOG_CALL( perf_enum_storages(count_test_storages, init_count(&st_count), perf_storage_lock) ) ;
    TEST_COMPARE( st_count.count, 0U ) ;

    TEST_ASSERT( perf_create_storage(STORAGE_NAME(wrt_0), &sd[0], perf_storage_lock) ) ;
    TEST_ASSERT( sd[0].storage ) ;

    TEST_ASSERT( perf_create_storage(STORAGE_NAME(wrt_1), &sd[1], perf_storage_lock) ) ;
    TEST_ASSERT( sd[1].storage ) ;

    TEST_LOG_CALL( perf_enum_storages(count_test_storages, init_count(&st_count), perf_storage_lock) ) ;
    TEST_COMPARE( st_count.count, 2U ) ;

    counter_t *s0_counter[5], *s1_counter[5] ;

    TEST_ASSERT( perf_add_counter(sd[0].storage, "counter0", &s0_counter[0]) ) ;

    TEST_LOG_CALL( perf_enum_counters(sd[0].storage, count_test_counters, init_count(&c_count)) ) ;
    TEST_COMPARE( c_count.count, 1U ) ;

    TEST_ASSERT( perf_add_counter(sd[0].storage, "counter0", &s0_counter[1])==ERR_EXISTS ) ;
    TEST_ASSERT( s0_counter[0] == s0_counter[1] ) ;

    TEST_LOG_CALL( perf_enum_counters(sd[0].storage, count_test_counters, init_count(&c_count)) ) ;
    TEST_COMPARE( c_count.count, 1U ) ;

    TEST_ASSERT( perf_add_counter(sd[0].storage, "counter1", &s0_counter[1]) ) ;
    TEST_ASSERT( s0_counter[1] ) ;
    TEST_ASSERT( s0_counter[0] != s0_counter[1] ) ;

    TEST_LOG_CALL( perf_enum_counters(sd[0].storage, count_test_counters, init_count(&c_count)) ) ;
    TEST_COMPARE( c_count.count, 2U ) ;

    TEST_COMPARE( (unsigned int) PERF_COUNT_ATOMIC_SET(s0_counter[1], 333), 0 ) ;

    TEST_ASSERT( perf_find_counter(sd[0].storage, "counter0") == s0_counter[0] ) ;

    T_COUNTER old_value ;
    TEST_ASSERT( (old_value = PERF_COUNT_ATOMIC_GET(s0_counter[0])) == 0 ) ;
    TEST_COMPARE( PERF_COUNT_ATOMIC_SET(s0_counter[0], 1234), old_value ) ;
    TEST_COMPARE( (unsigned int) PERF_COUNT_ATOMIC_GET(s0_counter[0]), 1234 ) ;

    TEST_ASSERT( perf_add_counter(sd[0].storage, "counter2", &s0_counter[2]) ) ;
    TEST_ASSERT( s0_counter[2] ) ;

    TEST_LOG_CALL( perf_enum_counters(sd[0].storage, count_test_counters, init_count(&c_count)) ) ;
    TEST_COMPARE( c_count.count, 3U ) ;

    TEST_COMPARE( (unsigned int) (old_value = PERF_COUNT_ATOMIC_GET(s0_counter[0])), 1234 ) ;

    TEST_COMPARE( PERF_COUNT_ATOMIC_INC(s0_counter[0]), old_value ) ;
    TEST_COMPARE( PERF_COUNT_ATOMIC_GET(s0_counter[0]), old_value+1 ) ;
    TEST_COMPARE( PERF_COUNT_ATOMIC_DEC(s0_counter[0]), old_value+1 ) ;
    TEST_COMPARE( PERF_COUNT_ATOMIC_GET(s0_counter[0]), old_value ) ;

    TEST_COMPARE( PERF_COUNT_ATOMIC_ADD(s0_counter[0], 356), old_value ) ;
    TEST_COMPARE( PERF_COUNT_ATOMIC_GET(s0_counter[0]), old_value+356 ) ;
    TEST_COMPARE( PERF_COUNT_ATOMIC_ADD(s0_counter[0], -591), old_value+356 ) ;
    TEST_COMPARE( PERF_COUNT_ATOMIC_GET(s0_counter[0]), old_value+356-591 ) ;
    TEST_ASSERT( s0_counter[0] ) ;
    TEST_ASSERT( &st0_counter_vals[0] ) ;
    TEST_COMPARE( s0_counter[0], &st0_counter_vals[0] ) ; // 999

    TEST_COMPARE( s0_counter[1], &st0_counter_vals[1] ) ; // 333
    TEST_COMPARE( s0_counter[2], &st0_counter_vals[2] ) ; // 0

    TEST_ASSERT( perf_add_counter(sd[1].storage, "couNTer0", &s1_counter[0]) ) ;
    TEST_ASSERT( perf_add_counter(sd[1].storage, "couNter1", &s1_counter[1]) ) ;
    TEST_ASSERT( perf_add_counter(sd[1].storage, "counter2", &s1_counter[2]) ) ;

    TEST_LOG_CALL( perf_enum_counters(sd[1].storage, count_test_counters, init_count(&c_count)) ) ;
    TEST_COMPARE( c_count.count, 3U ) ;

    TEST_COMPARE( (unsigned int) PERF_COUNT_ATOMIC_SET(s1_counter[2], 555), 0 ) ;

    TEST_COMPARE( (unsigned int) PERF_COUNT_ATOMIC_SET(s1_counter[0], 222), 0 ) ;
    TEST_COMPARE( (unsigned int) PERF_COUNT_ATOMIC_SET(s1_counter[1], PERF_COUNT_ATOMIC_INC(s1_counter[0])), 0 ) ;
    TEST_COMPARE( s1_counter[0], &st1_counter_vals[0] ) ; // 223
    TEST_COMPARE( s1_counter[1], &st1_counter_vals[1] ) ; // 222
    TEST_COMPARE( s1_counter[2], &st1_counter_vals[2] ) ; // 555

    TEST_LOG( "wait for newline char (Enter) to exit\n" ) ;
    int ch = 0 ;
    while(ch!=0x0A && ch!=0x0D && ch!=0x03) {
        ch = _getch();
        printf("\\x%x,", ch) ;
    }
    printf("\n") ;
    //sleep(2000000);

    TEST_LOG_CALL( perf_enum_counters(sd[0].storage, count_test_counters, init_count(&c_count)) ) ;
    TEST_COMPARE( c_count.count, 3U ) ;

    TEST_LOG_CALL( perf_enum_counters(sd[1].storage, count_test_counters, init_count(&c_count)) ) ;
    TEST_COMPARE( c_count.count, 3U ) ;

    TEST_LOG_CALL( perf_enum_storages(count_test_storages, init_count(&st_count), perf_storage_lock) ) ;
    TEST_COMPARE( st_count.count, ARRAY_SIZE(sd) ) ;

    for(int i=ARRAY_SIZE(sd); i--;)
    {
        TEST_ASSERT( perf_release_storage(&sd[i], perf_storage_lock) ) ;
        TEST_ASSERT( sd[i].storage==NULL ) ;
    }

    TEST_LOG_CALL( perf_enum_storages(count_test_storages, init_count(&st_count), perf_storage_lock) ) ;
    TEST_COMPARE( st_count.count, 0U ) ;

    TEST_FINISHED() ;
}

struct compare_data {
        counter_t *src ;
        int        src_count ;
        int        src_ndx ;
        int        enum_count ;
} ;
int compare_counters(counters_storage_t *, counter_t *counter, void *user_data)
{
    compare_data *data = (compare_data*)user_data ;
    ++data->enum_count ;
    if (data->src_ndx < data->src_count)
    {
        TEST_LOG( "try compare %d\n", data->src_ndx ) ;
        TEST_FLUSH() ;
        TEST_COMPARE( &data->src[data->src_ndx++], counter ) ;
    }
    return ENUM_CONTINUE ;
}

void perf_unit_test_reader()
{
    //TEST_ASSERT( (perf_unit_test_storage(), true) ) ;

    TEST_START() ;

    storage_descriptor_t sd[2], tmp_sd ;
    count_params_t st_count = { STORAGE_NAME(wrt_), 0, 0 } ;
    counter_t *s0_counter[5], *s1_counter[5] ;
    T_COUNTER old_value = 0;

    TEST_LOG_CALL( perf_enum_storages(count_test_storages, init_count(&st_count), perf_storage_lock) ) ;
    TEST_COMPARE( st_count.count, 0U ) ;

    TEST_ASSERT( perf_create_storage(STORAGE_NAME(wrt_0), &sd[0], perf_storage_lock) ) ;
    TEST_ASSERT( sd[0].storage ) ;

    TEST_ASSERT( perf_create_storage(STORAGE_NAME(wrt_1), &sd[1], perf_storage_lock) ) ;
    TEST_ASSERT( sd[1].storage ) ;
    TEST_ASSERT( (tmp_sd = perf_find_storage(STORAGE_NAME(wrt_0), perf_storage_lock)).storage != NULL ) ;
    TEST_LOG( "storage [%p] (%s) found.\n", tmp_sd.storage, tmp_sd.storage->descriptor.name ) ;
    {
        compare_data data = {st0_counter_vals, ARRAY_SIZE(st0_counter_vals), 0, 0} ;
	    TEST_ASSERT( perf_add_counter(sd[0].storage, "counter0", &s0_counter[0]) ) ;
	    TEST_COMPARE( PERF_COUNT_ATOMIC_SET(s0_counter[0], 999), old_value ) ;
	    TEST_ASSERT( perf_add_counter(sd[0].storage, "counter1", &s0_counter[1]) ) ;
	    TEST_COMPARE( PERF_COUNT_ATOMIC_SET(s0_counter[1], 333), old_value ) ;
	    TEST_ASSERT( perf_add_counter(sd[0].storage, "counter2", &s0_counter[2]) ) ;
	    TEST_COMPARE( PERF_COUNT_ATOMIC_SET(s0_counter[2], 0), old_value ) ;

        TEST_LOG_CALL( perf_enum_counters(tmp_sd.storage, compare_counters, &data) ) ;
        TEST_COMPARE( data.enum_count, data.src_count ) ;
        TEST_COMPARE( data.src_ndx, data.src_count ) ;
    }
    TEST_LOG_CALL( perf_release_storage(&tmp_sd, perf_storage_lock) ) ;

    TEST_ASSERT( (tmp_sd = perf_find_storage(STORAGE_NAME(wrt_1), perf_storage_lock)).storage != NULL ) ;
    TEST_LOG( "storage [%p] (%s) found.\n", tmp_sd.storage, tmp_sd.storage->descriptor.name ) ;
    {
        compare_data data = {st1_counter_vals, ARRAY_SIZE(st1_counter_vals), 0, 0} ;
	    TEST_ASSERT( perf_add_counter(sd[1].storage, "couNTer0", &s1_counter[0]) ) ;
	    TEST_COMPARE( PERF_COUNT_ATOMIC_SET(s1_counter[0], 223), old_value ) ;
	    TEST_ASSERT( perf_add_counter(sd[1].storage, "couNter1", &s1_counter[1]) ) ;
	    TEST_COMPARE( PERF_COUNT_ATOMIC_SET(s1_counter[1], 222), old_value ) ;
	    TEST_ASSERT( perf_add_counter(sd[1].storage, "counter2", &s1_counter[2]) ) ;
	    TEST_COMPARE( PERF_COUNT_ATOMIC_SET(s1_counter[2], 555), old_value ) ;

        TEST_LOG_CALL( perf_enum_counters(tmp_sd.storage, compare_counters, &data) ) ;
        TEST_COMPARE( data.enum_count, data.src_count ) ;
        TEST_COMPARE( data.src_ndx, data.src_count ) ;
    }
    TEST_LOG_CALL( perf_release_storage(&tmp_sd, perf_storage_lock) ) ;

    TEST_FINISHED() ;
}

int print_counter(counters_storage_t*, counter_t *counter, void *prefix)
{
    if (!prefix || strncmp(counter->name, (const char*)prefix, strlen((const char*)prefix))==0)
        printf("  %s: %lu\n", counter->name, (unsigned long)PERF_COUNT_ATOMIC_GET(counter)) ;
    return ENUM_CONTINUE ;
}

int print_storage(const storage_descriptor_t *sd, void *prefix)
{
    const char * c_prefix = (const char*)prefix ;
    int cmp_len = c_prefix ? strlen(c_prefix) : 0 ;
    if (prefix) {
        c_prefix = strchr(c_prefix, '.') ;
        if (c_prefix) {
            cmp_len = c_prefix - (const char*)prefix ;
            ++c_prefix ;
        }
    }
    if (!prefix || (strncmp(sd->storage->descriptor.name, (const char*)prefix, cmp_len)==0
                    && (!c_prefix || sd->storage->descriptor.name[cmp_len+1]==0)))
    {
        printf("%s:\n", sd->storage->descriptor.name) ;
        perf_enum_counters(sd->storage, print_counter, (void*)c_prefix) ;
    }
    return ENUM_CONTINUE ;
}

void perf_print(const char * prefix)
{
    TEST_START() ;

    perf_enum_storages(print_storage, (void*)prefix, perf_storage_lock) ;
}


#ifndef _WIN_

static const char PRL_SHARED_PERFCOUNTERS[] = "PRL" ;
struct memmap_data {
        char      prl_mark[sizeof(PRL_SHARED_PERFCOUNTERS)] ;
        T_COUNTER counter ;
        char      data[] ;
} ;

#define ALIGN_TO(value, align) ((((value) + align - 1)/align)*align)

static const char TEST_TEXT[] = "we should test right access" ;

inline size_t aligned_storage_size()
{
    static const size_t result = ALIGN_TO(sizeof(counters_storage_t) + PERFCOUNT_RESERVED_SIZE,
                                          ::getpagesize()) ;
    return result ;
}
#define TEST_STORAGE_SIZE aligned_storage_size()

void simple_test()
{
    TEST_START() ;

    const char name[] = "/tmp/test_6789_123456789_123456789" ;
    int result ;
    //TEST_LOG( "SHM_NAME_MAX: %d\n", SHM_NAME_MAX) ;
    TEST_LOG( "unlink(%s) return %d, errno: %d\n", name, result=unlink(name), errno) ;
    //TEST_ASSERT( result==0 || errno==ENOENT) ;
    int fd = open(name, O_RDWR | O_CREAT | O_EXCL, S_IRWXU | S_IRWXG | S_IRWXO) ;
    if (fd<0) {
        printf("failed open: %d\n", errno) ;
        exit(1) ;
    }
    if (ftruncate(fd, TEST_STORAGE_SIZE)!=0) {
        printf("failed ftruncate %d to size %lu, errno: %d\n",
               fd, (unsigned long int)TEST_STORAGE_SIZE, errno) ;
        exit(1) ;
    }
    memmap_data * mem_ptr ;
    TEST_ASSERT( (mem_ptr = (memmap_data*)mmap(NULL, TEST_STORAGE_SIZE,
                                               PROT_READ | PROT_WRITE,
                                               MAP_SHARED, fd, 0)) != MAP_FAILED ) ;
    close(fd) ;
    if (memcmp(mem_ptr->prl_mark, PRL_SHARED_PERFCOUNTERS, sizeof(mem_ptr->prl_mark))) {
        memset(mem_ptr, 0, TEST_STORAGE_SIZE) ;
        memcpy(mem_ptr->prl_mark, PRL_SHARED_PERFCOUNTERS, sizeof(mem_ptr->prl_mark)) ;
        printf("mem_ptr->counter: '%llu'\n", (long long int)mem_ptr->counter) ;
        AtomicInc64(&mem_ptr->counter);
    }
    memcpy(mem_ptr->data, TEST_TEXT, sizeof(TEST_TEXT)) ;
    printf("copied text: '%s', counter: '%llu'\n", (char*)mem_ptr->data, (long long int)mem_ptr->counter) ;

    int fd1 = open(name, O_RDONLY, S_IRWXU | S_IRWXG | S_IRWXO) ;
    if (fd1<0) {
        printf("failed open: %d\n", errno) ;
        exit(1) ;
    }
    memmap_data *mem_ptr1 ;
    TEST_ASSERT( (mem_ptr1 = (memmap_data*)mmap(NULL, TEST_STORAGE_SIZE,
                                                PROT_READ, MAP_SHARED, fd1, 0)) != MAP_FAILED ) ;
    close(fd1) ;
    TEST_COMPARE( mem_ptr->prl_mark, PRL_SHARED_PERFCOUNTERS ) ;
    printf("read from second: '%s', counter: '%lu'\n",
           mem_ptr1->data, (unsigned long)mem_ptr->counter) ;

    TEST_LOG( "mem_ptr: %p, mem_ptr1: %p\n", mem_ptr, mem_ptr1 ) ;

    TEST_ASSERT( munmap(mem_ptr, TEST_STORAGE_SIZE) == 0 ) ;
    TEST_LOG( "unlink(%s) return %d, errno: %d\n", name, result=unlink(name), errno) ;

    printf("after unlink mem->data: '%s', counter: '%llu'\n",
           (char*)mem_ptr1->data, (long long int)mem_ptr1->counter) ;
    TEST_ASSERT( munmap(mem_ptr1, TEST_STORAGE_SIZE) == 0 ) ;

    TEST_LOG( "try second unlink(%s) return %d, errno: %d\n", name, result=unlink(name), errno) ;

    TEST_FINISHED() ;
}
#else

void simple_test()
{
    TEST_START() ;
    TEST_LOG("No simple test for windows platform") ;
    TEST_FINISHED() ;
}
#endif // not _WIN_

int main(int argc, char* argv[])
{
	perf_storage_lock = perf_get_storage_lock();
    simple_test() ; // return 0 ;
    if (argc==2 && !strcmp(argv[1], "reader")) {
        perf_unit_test_reader();
        perf_release_storage_lock();
        return 0 ;
    }
    else if (argc==2 && !strcmp(argv[1], "writer")) {
        perf_unit_test_writer();
        perf_release_storage_lock();
        return 0 ;
    }
    else if (argc<=2) {
        perf_print(argc==2 ? argv[1] : NULL);
        perf_release_storage_lock();
        return 0 ;
    }

    printf( "wrong usage: %s [writer|reader|<prefix>]\n", argv[0] );
    return -1;
}
