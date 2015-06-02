///////////////////////////////////////////////////////////////////////////////
///
/// @file MonitorBuffers.h
///
///	MonitorBuffers header stub file.
/// Emulates functionality Monitor/Source/Core/MonitorBuffers.h.
/// For testing Monitor/Std only.
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

#ifndef __MONITOR_STD_TEST_MONITOR_BUFFERS_H__
#define __MONITOR_STD_TEST_MONITOR_BUFFERS_H__


struct _MON_BUFF_DESCR;


// Function type returned the size/attribute of the monitor buffer
typedef ULONG_PTR (*MBUFF_DESCR_FUNC)(struct _MON_BUFF_DESCR* pMonBuffDescr);


// Size types
enum
{
	// Static size - constant value in bytes
	MBUFF_PARAM_STATIC,
	// Pointer to the variable included the size of the buffer
	MBUFF_PARAM_PTR,
	// Pointer to the function calculated the buffer size
	MBUFF_PARAM_FUNC,
};


/**
* @brief
*		Moniotor buffer descriptor.
*/
typedef struct _MON_BUFF_DESCR
{
	// Buffer type
	UINT	mtType;

#ifdef LOGGING_ON
	// Buffer type string
	const char* sTypeStr;
#endif

	// Flags of the buffer may also be changed on init stage
	UINT	uFlagsType;

	union
	{
		// Static flags
		ULONG_PTR	uFlag;

		//  Flags field pointer
		ULONG_PTR*	pFlagsPtr;

		// Flags getting function
		MBUFF_DESCR_FUNC  pGetFlagFunc;
	}Flags;

	// Buffer size type
	UINT	uSizeType;

	// Buffer size union (static/pointer/get function)
	union
	{
		// Static size
		ULONG_PTR		uSize;

		// Size field pointer
		ULONG_PTR*		pSizePtr;

		// Size getting function
		MBUFF_DESCR_FUNC	pGetSizeFunc;
	} Size;


	// Physical (real) page index array associated with buffer
	ULONG64*	pPhyPageArr;

	// Pointer to buffer pointer
	void**		ppBuf;

	UINT	uArraySize;

	// Additionally initializing fields

	// Pointer to Primary OS buffer pointer
	UINT	uMapOffsFromVmmBase;

	ULONG64	uHypMemDescr;

} MON_BUFF_DESCR;




#endif // __MONITOR_STD_TEST_MONITOR_BUFFERS_H__
