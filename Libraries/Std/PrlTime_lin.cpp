/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (C) 2007-2015 Parallels IP Holdings GmbH
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
/// @file
///		PrlTime_lin.h
///
/// @author
///		kozerkov@
///
/// @brief
///		Timing functions (Linux implementation)
///
/////////////////////////////////////////////////////////////////////////////

#include "PrlTime.h"
#include "Interfaces/ParallelsTypes.h"
#include "Libraries/Logging/Logging.h"

#include <time.h>
#include <errno.h>

// funny constant
#define PRL_UI64_MAX 0xFFFFFFFFFFFFFFFFLL

PRL_UINT64 PrlTicks()
{
	struct timespec ts;

	if (likely(!clock_gettime(CLOCK_MONOTONIC, &ts)))
		return ((PRL_UINT64)ts.tv_sec * 1000000000LL) + ts.tv_nsec;

	WRITE_TRACE(DBG_FATAL, "[PrlTicks] clock_gettime() failed: %d", errno);
	return 1;
}

PRL_UINT64 PrlTicksDelta(PRL_UINT64 start, PRL_UINT64 *new_start)
{
	PRL_UINT64 tick;
	if (!new_start)
		new_start = &tick;

	*new_start = PrlTicks();

	if (likely(*new_start >= start))
		return *new_start - start;

	return PRL_UI64_MAX - (*new_start) + start + 1;
}

PRL_UINT64 PrlTicksToMicro(PRL_UINT64 tick)
{
	return tick / 1000;
}

PRL_UINT64 PrlTicksToMilli(PRL_UINT64 tick)
{
	return tick / 1000000;
}

PRL_UINT64 PrlTicksToSeconds(PRL_UINT64 tick)
{
	return tick / 1000000000LL;
}

PRL_UINT64 PrlTicksFrequency()
{
	return 1000000000LL;
}
