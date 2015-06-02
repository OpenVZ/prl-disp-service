///////////////////////////////////////////////////////////////////////////////
///
/// @file SimpleStorage.cpp
///
/// Easy to use performance counters module
///
/// @author maximk
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
///////////////////////////////////////////////////////////////////////////////

#include "SimpleStorage.h"

// Perfomance counters storage
static CounterStorageT s_CounterStorage;


/**
 * Initialize existing performance counters pointers
 * with real values to be used in the VMM.
 * @return boolean success status
 */
BOOL PerfCounters_Init( const char* name )
{
	// It's impossible to initialize it twice
	if (s_CounterStorage.valid())
		return TRUE;

	s_CounterStorage.recreate( name ) ;

	return TRUE;
}


/**
 * Returns pointer to the performance counters storage,
 * NULL in case of some failure/unitialized yet.
 */
counters_storage_t* PerfCounters_GetBuffer()
{
	return s_CounterStorage.storage();
}


/**
 * Register new performance counter by name
 * and return pointer to it
 * @param name of the performance counter
 */
counter_ptr PerfCounters_Add( const char* name )
{
    counter_ptr ret = 0;

	// It's impossible to initialize it twice
	if (!s_CounterStorage.valid())
		return 0;

	ret = s_CounterStorage.add_counter(name);
	if (!ret)
		return 0;

    PERF_COUNT_SET(ret, 0);
    return ret;
}
