/*
 * @file wildcards_imp.h
 *
 * @brief Routine for check wildcards matching
 *
 * @author vasilyz
 * @owner alexg
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
#include "wildcards.h"

/**
 * Checks whether given string matches to given pattern
 *
 * @param s[in] - string
 * @param p[in] - pattern; '*' and '?' wildcards supported
 * @return non-zero if the string matches to the pattern
 */
#define _WILDCARDMATCH(wmtype, s, p)			\
{												\
	if (s == 0 || p == 0)						\
		return s == p;							\
												\
	for (; *s != '\0' && *p != '*'; ++s, ++p)	\
		if (*s != *p && *p != '?')				\
			return 0;							\
												\
	const wmtype *pm = 0, *sm = 0;				\
												\
	while (*s != '\0')							\
	{											\
		if (*p == '*')							\
		{										\
			if (*++p == '\0')					\
				return 1;						\
			pm = p;								\
			sm = s + 1;							\
		}										\
		else if (*s == *p || *p == '?')			\
			++s, ++p;							\
		else									\
			p = pm, s = sm++;					\
	}											\
												\
	while (*p == '*')							\
		++p;									\
												\
	return *p == 0;								\
}


int wildcardMatch(const char *s, const char *p)
	_WILDCARDMATCH(char, s, p)

int wildcardMatchUnicode(const unsigned short *s, const unsigned short *p)
	_WILDCARDMATCH(unsigned short, s, p)

#undef _WILDCARDMATCH

