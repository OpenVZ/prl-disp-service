///////////////////////////////////////////////////////////////////////////////
///
/// @file condwait.cpp
///
/// Lightweight cross platform condition variable helper
///
/// @author olegv@
///
/// Copyright (c) 2010-2015 Parallels IP Holdings GmbH
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

#include "condwait.h"

#ifndef _WIN_
#include <sys/time.h>

void CPrlWaitCondition::timedwait(unsigned long ms_tout)
{
	struct timeval tv;
	gettimeofday(&tv, 0);

	timespec ts;
	ts.tv_sec  =  tv.tv_sec;
	ts.tv_nsec = (tv.tv_usec + ms_tout * 1000) * 1000;

	unsigned long secs = ts.tv_nsec / 1000000000;
	ts.tv_sec  += secs;
	ts.tv_nsec -= secs * 1000000000;

	pthread_cond_timedwait(&m_cond, &m_mutex, &ts);
}

#endif
