/**
* @file wstr2utf8_imp.h
*
* @brief Routines to convert wchar_t string to utf8 string
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
#include <prlcommon/Std/ParallelsErrors.h>
#include "wstr2utf8.h"


////////////////////////////////////////////////////////////////////////////////
// functions


extern inline int wstr2utf8(const wchar_t *const wstrSrc, const size_t szSrc,
			    char **const utf8Dst, size_t *const szDst)
{
#ifdef _WIN_
	int c;
	char *s;
	int err = 0;

	if (0 == szSrc)
	{
		c = 0;
		s = (char *)malloc(c + 1);
		if (0 == s)
		{
			err = PRL_E_NO_MEMORY;
			goto failed_fin;
		}
		else
		{
			goto success;
		}
	}

	c = WideCharToMultiByte(CP_UTF8, 0, wstrSrc, (int)szSrc, 0, 0, 0, 0);
	if (0 >= c)
	{
		err = PRL_E_FAILURE;
		goto failed_fin;
	}

	s = (char *)malloc(c + 1);
	if (0 == s)
	{
		err = PRL_E_NO_MEMORY;
		goto failed_fin;
	}

	c = WideCharToMultiByte(CP_UTF8, 0, wstrSrc, (int)szSrc, s, c, 0, 0);
	if (0 >= c)
	{
		err = PRL_E_FAILURE;
		goto failed_free;
	}

success:
	s[c] = 0;
	*utf8Dst = s;
	if (szDst)
		*szDst = c;
	return err;

failed_free:
	free(s);
failed_fin:
	return err;
#else
	#error Platform not supported
#endif
}


extern inline std::string wstr2utf8(const wchar_t *const wstr, const size_t sz)
{
#ifdef _WIN_
	const DWORD lerr = GetLastError();
	int c;
	char *s;
	std::string utf8;

	if (0 == sz) goto conv_fin;

	c = WideCharToMultiByte(CP_UTF8, 0, wstr, (int)sz, 0, 0, 0, 0);
	if (0 >= c) goto conv_fin;

	s = (char *)malloc(c);
	if (0 == s) goto conv_fin;

	c = WideCharToMultiByte(CP_UTF8, 0, wstr, (int)sz, s, c, 0, 0);
	if (0 >= c) goto conv_free;

	utf8.assign(s, c);

conv_free:
	free(s);
conv_fin:
	SetLastError(lerr);
	return utf8;
#else
	#error Platform not supported
#endif
}
