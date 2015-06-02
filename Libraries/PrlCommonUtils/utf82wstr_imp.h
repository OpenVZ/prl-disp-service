/**
* @file utf82wstr_imp.h
*
* @brief Routines to convert utf8 string to wchar_t string
*
* @author andreyp@
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


////////////////////////////////////////////////////////////////////////////////
// includes
#include <windows.h>
#include "utf82wstr.h"


////////////////////////////////////////////////////////////////////////////////
// functions


extern inline std::wstring utf82wstr(const char *const utf8, const size_t sz)
{
#ifdef _WIN_
	const DWORD lerr = GetLastError();
	int c;
	wchar_t *s;
	std::wstring wstr;

	if (0 == sz) goto conv_fin;

	c = MultiByteToWideChar(CP_UTF8, 0, utf8, (int)sz, 0, 0);
	if (0 >= c) goto conv_fin;

	s = (wchar_t *)malloc(c * sizeof(wchar_t));
	if (0 == s) goto conv_fin;

	c = MultiByteToWideChar(CP_UTF8, 0, utf8, (int)sz, s, c);
	if (0 >= c) goto conv_free;

	wstr.assign(s, c);

conv_free:
	free(s);
conv_fin:
	SetLastError(lerr);
	return wstr;
#else
	#error Platform not supported
#endif
}
