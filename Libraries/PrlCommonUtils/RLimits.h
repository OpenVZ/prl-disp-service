///////////////////////////////////////////////////////////////////////////////
///
/// @file RLimits.h
///
/// System resouces limits configuration.
///
/// @author artemk
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
///////////////////////////////////////////////////////////////////////////////

#ifndef VI_COMMON_UTILS_RLIMITS_H
#define VI_COMMON_UTILS_RLIMITS_H

#if defined(_MAC_)
#include <sys/syslimits.h> // For OPEN_MAX
#endif

#if defined(_LIN_) || defined(_MAC_)
#include <sys/resource.h>
#include <errno.h>
#endif // _LIN_ or _MAC_

#if defined(_LIN_) || defined(_MAC_)

inline bool SetMax_RLIMIT_NOFILE( rlim_t *pnAppliedValue = NULL )
{
	struct rlimit rl;

	// Get RLIMIT_NOFILE value
	if (!getrlimit(RLIMIT_NOFILE, &rl))
	{
		// Set RLIMIT_NOFILE value to maximum
#ifdef _MAC_
		// https://bugzilla.sw.ru/show_bug.cgi?id=460645
		rl.rlim_cur = ((unsigned)OPEN_MAX < (unsigned)rl.rlim_max ? OPEN_MAX : rl.rlim_max);
#else
		rl.rlim_cur = rl.rlim_max;
#endif
		if ( pnAppliedValue )
			*pnAppliedValue = rl.rlim_cur;
		return !setrlimit(RLIMIT_NOFILE, &rl);
	}

	return false;
}

#endif // _LIN_ or _MAC_

#endif // VI_COMMON_UTILS_RLIMITS_H
