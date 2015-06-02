///////////////////////////////////////////////////////////////////////////////
///
/// @file Monitor.h
///
///	Monitor header stub file.
/// Emulates functionality Monitor/Source/Include/Monitor.h.
/// For testing Monitor/Std only.
/// See similar declarations in Monitor/Source/Include/Monitor.h for comments.
///
/// @author vtatarinov
/// @owner alexeyk
///
/// Copyright (c) 2007-2015 Parallels IP Holdings GmbH
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

#ifndef __MONITOR_STD_TEST_MONITOR_H__
#define __MONITOR_STD_TEST_MONITOR_H__

#include <stdio.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <memory>
#include <list>
#include <fstream>

#undef _INTSIZEOF
#undef va_start
#undef va_arg
#undef va_end

#define DMM_TEST

#include "Interfaces/ParallelsTypes.h"

#define BYTES2PAGES(b)	(((b) + (PAGE_SIZE - 1)) / PAGE_SIZE)
#define ALIGNAT(var,align) ( ( ((var) + (align) - 1)/(align) ) * (align) )

#define SHIFT_4KB	(12)
#define SIZE_4KB	(1<<SHIFT_4KB)
#define MASK_4KB	(~(SIZE_4KB-1))


#include "Libraries/Logging/LoggingConfig.h"

inline int EmptyLogMessage(int, char*, ...)
{
	return 0;
}

void LogMessage(int,char*, ...);

#define WRITE_TRACE			LogMessage
#define WRITE_TIME_TRACE	LogMessage
#define LOG_MESSAGE			LogMessage
#define LOG_TIME_MESSAGE	LogMessage

void Abort(char*, ...);
#define CondAbort Abort

inline void TestStatDmmSplit( UINT ){};
inline void TestStatDmmMerge( UINT ){};
inline void TestStatDmmDestruct(){};
inline void TestStatDmmGC(){};
inline void TestStatDmmChangePrior( UINT, UINT ){};

inline void TestStatLookasideDestruct( UINT ){};
inline void TestStatLookasideAlloc( UINT ){};
inline void TestStatLookasideFree( UINT ){};
inline void TestStatLookasideAddPage( UINT, UINT ) {};
inline void TestStatLookasideRemovePage( UINT, UINT ){};

#define PTR_TO_LIN_ADDR(uOffset) ((ULONG_PTR) (uOffset))
inline void DumpLaddrMemory(ULONG_PTR, ULONG_PTR)	{}

typedef void( *DESTRUCTOR_PAGE)( void*, void*, UINT );

void PrintDMMSpaceState();

#include "Tests/MonitorStdTest/Monitor/Source/Core/MonitorBuffers.h"
#include "Libraries/Std/SpinLock.h"
#include "Monitor/Source/Std/StdLibrary.h"
#include "Libraries/Std/list.h"

__inline BOOL HpcCheckIsInHPCcontext() { return FALSE; };

#define ASSERT_LOOKASIDE 1
#define ASSERT_DMM 1

#include "Monitor/Source/Std/HashTree.h"
#include "Monitor/Source/Std/MemManager.h"
#include "Monitor/Source/Std/Hash.h"
#include "Monitor/Source/Std/Dmm.h"
#include "Monitor/Source/Std/Lookaside.h"
#include "Monitor/Source/Std/CacheLru.h"

struct MON_STATE
{
	DYNAMIC_MEMORY_MAN DynMamMem;
};

extern MON_STATE MonState;

#endif	// __MONITOR_STD_TEST_MONITOR_H__
