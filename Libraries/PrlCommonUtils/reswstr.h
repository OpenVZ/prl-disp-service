/**
* @file reswstr.h
*
* @brief Routines to load resource string and convert it to std::wstring
*
* Routines implementation located in file reswstr_imp.h
*
* @author deno@
* @author owner is alexg@
*
* Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
*/

#pragma once

////////////////////////////////////////////////////////////////////////////////
// includes
#include <windows.h>
#include <string>


////////////////////////////////////////////////////////////////////////////////
// functions
extern inline std::wstring resourceString(const HINSTANCE hinst, const UINT id);
extern inline std::wstring resourceString(const UINT id)
{ return resourceString(GetModuleHandleW(0), id); }
#define RESWSTR(...) (resourceString(__VA_ARGS__).c_str())
