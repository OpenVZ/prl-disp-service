/*
 * @file utf82wstr.h
 *
 * @brief Routines to convert utf8 string to wchar_t string
 *
 * Routines implementation located in file utf82wstr_imp.h
 *
 * @author andreyp@
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

////////////////////////////////////////////////////////////////////////////////
// includes
#include <string.h>
#include <string>


////////////////////////////////////////////////////////////////////////////////
// functions
extern inline std::wstring utf82wstr(const char *const utf8, const size_t sz);
extern inline std::wstring utf82wstr(const char *const utf8)
{ return utf82wstr(utf8, strlen(utf8)); }
extern inline std::wstring utf82wstr(const std::string &utf8)
{ return utf82wstr(utf8.c_str(), utf8.size()); }
#define UTF82WSTR(...) (utf82wstr(__VA_ARGS__).c_str())
