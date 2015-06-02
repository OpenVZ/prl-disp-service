//////////////////////////////////////////////////////////////////////////
///
/// @file Config.h
///
/// @brief Definitions for compilation
///
/// @author ?
///
///	All definitions needed to application/driver/monitor are
/// included here
///
/// Copyright (c) 2006-2015 Parallels IP Holdings GmbH
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

#ifndef __CONFIG_H__
#define __CONFIG_H__

// Logging configuration is placed in the separated file
#include "Libraries/Logging/LoggingConfig.h"

// Maximum guest physical memory size (in megabytes)
#define VM_MAX_MEM				(128 * 1024)

/**
 * Constant used to define start address, PCI prefetchable space
 * Bios/SysBios/PCI/PCI.8
 * P_MEMP0 equ 0xB000
 */
#define MAX_MIDDLE_MEMORY	  0x0B0000000
#define DEVICES_PHY_SPACE_TOP 0x100000000ULL

// maximum physical address space for devices
#define DEVICES_PHY_SPACE (DEVICES_PHY_SPACE_TOP - MAX_MIDDLE_MEMORY)

// Accessible physical memory maximum size by guest OS
// (including shared device physical memory)
#define GUEST_PHY_SPACE(mem_size) \
	((mem_size) <= MAX_MIDDLE_MEMORY \
		? DEVICES_PHY_SPACE_TOP \
		: DEVICES_PHY_SPACE + (mem_size))

#define GUEST_PHY_SPACE_MAX GUEST_PHY_SPACE(1ULL*VM_MAX_MEM*SIZE_1MB)

#define VM_MIN_MEM              4
#define VM_MIN_VIDEO_MEM		2
#define VM_MAX_VIDEO_MEM		1024

#include "Interfaces/ParallelsTypes.h"

// Align size "var" at specified "align"
#define ALIGNAT(var,align) ( ( ((var) + (align) - 1)/(align) ) * (align) )

// QString file name to char* conversion routine
// This conversion is intended to make unicode transitions consistent
// Previous version used QString.toUtf8().data(), but it is not really correct
#define QFLN2CH( qstr )		(	QFile::encodeName( qstr ).data()	)

#endif // __CONFIG_H__
