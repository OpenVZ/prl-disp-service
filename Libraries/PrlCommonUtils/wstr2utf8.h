/*
 * @file wstr2utf8.h
 *
 * @brief Routines to convert wchar_t string to utf8 string
 *
 * Routines implementation located in file wstr2utf8_imp.h
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
extern inline int wstr2utf8(const wchar_t *const wstrSrc, const size_t szSrc,
			    char **const utf8Dst, size_t *const szDst);
extern inline std::string wstr2utf8(const wchar_t *const wstr, const size_t sz);
extern inline std::string wstr2utf8(const wchar_t *const wstr)
{ return wstr2utf8(wstr, wcslen(wstr)); }
extern inline std::string wstr2utf8(const std::wstring &wstr)
{ return wstr2utf8(wstr.c_str(), wstr.size()); }
#define WSTR2UTF8(...) (wstr2utf8(__VA_ARGS__).c_str())
