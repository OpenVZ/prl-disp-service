/*
 * @file reswstr_imp.h
 *
 * @brief Routines to load resource string and convert it to std::wstring
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


////////////////////////////////////////////////////////////////////////////////
// includes
#include "reswstr.h"


////////////////////////////////////////////////////////////////////////////////
// functions


extern inline std::wstring resourceString(const HINSTANCE hinst, const UINT id)
{
#ifdef _WIN_
	int wstr_len;
	const wchar_t *wstr;
	std::wstring ret;

	wstr_len = LoadStringW(hinst, id, (LPWSTR)&wstr, 0);
	if (0 < wstr_len)
	{
		ret.assign(wstr, wstr_len);
	}

	return ret;
#else
	#error Platform not supported
#endif
}
