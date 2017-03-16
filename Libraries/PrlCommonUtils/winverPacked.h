/*
 * @file winverPacked.h
 *
 * @brief Routine to get packed Windows version
 *
 * @author deno@
 * @author owner is alexg@
 *
 * Copyright (c) 2005-2017, Parallels International GmbH
 *
 * This file is part of Virtuozzo Core Libraries. Virtuozzo Core
 * Libraries is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#pragma once

#include <windows.h>

#define WINVER_PACKED_8_1 0x0603
#define WINVER_PACKED_8 0x0602
#define WINVER_PACKED_7 0x0601
#define WINVER_PACKED_VISTA 0x0600
#define WINVER_PACKED_XP64 0x0502
#define WINVER_PACKED_2K3 WINVER_PACKED_XP64
#define WINVER_PACKED_XP 0x0501
#define WINVER_PACKED_2K 0x0500

extern inline
WORD winverPacked()
{
	DWORD v = GetVersion();
	return (LOBYTE(LOWORD(v)) << 8) | HIBYTE(LOWORD(v));
};
