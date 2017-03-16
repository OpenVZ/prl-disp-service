///////////////////////////////////////////////////////////////////////////////
///
/// @file SimpleStorage.h
///
/// Easy to use performance counters module
///
/// @author maximk
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
///////////////////////////////////////////////////////////////////////////////

#ifndef __SIMPLE_STORAGE_H__
#define __SIMPLE_STORAGE_H__

#include "Libraries/PerfCount/PerfLib/PerfCounter.h"

/**
 * Initialize existing performance counters pointers
 * with real values to be used in the VMM.
 * @param name of the performance storage
 * @return boolean success status
 */
BOOL PerfCounters_Init( const char* name );

/**
 * Returns pointer to the performance counters storage,
 * NULL in case of some failure/unitialized yet.
 */
counters_storage_t* PerfCounters_GetBuffer();

/**
 * Register new performance counter by name
 * and return pointer to it
 * @param name of the performance counter
 */
counter_ptr PerfCounters_Add( const char* name );

#endif // __SIMPLE_STORAGE_H__
