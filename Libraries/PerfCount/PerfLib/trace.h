//////////////////////////////////////////////////////////////////////////
///
/// @file tarce.h
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

#ifndef __TRACE_H__
#define __TRACE_H__

#include <stdarg.h>
#include <stdio.h>

#ifdef _WIN_
#include <windows.h>
#include <process.h>

#define ERROR_CODE GetLastError()

#define getpid() _getpid()
#else

#include <unistd.h>
#include <errno.h>
#include <string.h>
#define ERROR_CODE errno

#endif

#undef WRITE_TRACE
#define WRITE_TRACE notrace

inline void notrace(int, const char *, ...){}

#include <prlcommon/Logging/LoggingConfig.h>
#include <prlcommon/Std/AtomicOps.h>

#ifndef WRITE_TRACE
inline void WRITE_TRACE(int, const char *format, ...)
#if defined(__GNUC__)
	__attribute__ ((format(printf, 2, 3)))
#endif
{
    va_list vargs ;
    va_start(vargs, format) ;
    printf("[0x%x]: ", ::getpid()) ;
    vprintf(format, vargs) ;
    va_end(vargs) ;
}
#endif // WRITE_TRACE

inline const char* value2str(bool value, char *buff)
{
    strcpy(buff, value ? "True" : "False") ;
    return buff ;
}


inline const char* value2str(const char *value, char *buff)
{
    strcpy(buff, value) ;
    return buff ;
}

inline const char* value2str(long long int value, char *buff)
{
    sprintf(buff, "%lli", value) ;
    return buff ;
}

inline const char* value2str(unsigned long value, char *buff)
{
    sprintf(buff, "%lu", value) ;
    return buff ;
}

inline const char* value2str(long value, char *buff)
{
    sprintf(buff, "%ld", value) ;
    return buff ;
}

inline const char* value2str(unsigned int value, char *buff)
{
    sprintf(buff, "%u", value) ;
    return buff ;
}

inline const char* value2str(int value, char *buff)
{
    sprintf(buff, "%d", value) ;
    return buff ;
}

#ifdef PERF_COUNT_ATOMIC_GET
inline const char* value2str(const counter_t &value, char *buff)
{
    sprintf(buff, "counter_t(%s, %lu)", value.name,
            (unsigned long)PERF_COUNT_ATOMIC_GET((counter_t*)&value)) ;
    return buff ;
}

inline const char* value2str(const counter_t *value, char *buff)
{
    if (value)
        return value2str(*value, buff) ;
    sprintf(buff, "counter_t( NULL ) !!! ") ;
    return buff ;
}
#endif //PERF_COUNT_ATOMIC_GET

inline const char* value2str(const void *value, char *buff)
{
    sprintf(buff, "%p", value) ;
    return buff ;
}

template<typename R>
inline R call_trace(R ret_val, const char *call, const char *func=NULL,
                       const char *file=NULL, int line=0)
{
    char buff[2024] ;
    WRITE_TRACE(DBG_FATAL, "Method called: '%s' return: %s, from '%s', '%s:%d'.\n",
                call, value2str(ret_val, buff), func ? func : "???", file ? file : "???", file ? line : -1) ;

    return ret_val ;
}

inline int check_sys_result(int ret_val)
{
    if (ret_val<0)
        WRITE_TRACE(DBG_FATAL, "Errno:  %d'.\n", ERROR_CODE) ;
    return ret_val ;
}

#define CALL_TRACE(call) call_trace(call, #call, __FUNCTION__, __FILE__, __LINE__)

#define SYS_CALL_TRACE(call) check_sys_result(call_trace(call, #call, __FUNCTION__, __FILE__, __LINE__))

#endif //__TRACE_H__
