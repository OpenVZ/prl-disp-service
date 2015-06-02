//////////////////////////////////////////////////////////////////////////
///
/// @file pollset.cpp
///
/// @author sdmitry
///
/// Abstraction for platform-independent polling
/// of file-descriptors/events and timers.
///
/// Common for all platforms functions
///
/// Copyright (c) 2011-2015 Parallels IP Holdings GmbH
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
//////////////////////////////////////////////////////////////////////////
#include "pollset.h"
#include "pollset_private.h"
#include <Libraries/Std/PrlTime.h>

void __pollset_refresh_time(pollset_t *pollset)
{
	pollset->now = PrlGetTickCount();
}


int __pollset_poll_timers(pollset_t *pollset)
{
	__pollset_refresh_time(pollset);

	int timeout = -1/*infinite*/;
	while (!cd_list_empty(&pollset->timers)) {
		pollset_timer_t *timer;
		timer = cd_list_first_entry(&pollset->timers, pollset_timer_t, list);

		if ((int)(timer->expiry - pollset->now) > 0) {
			timeout = (int)(timer->expiry - pollset->now);
			break;
		}
		cd_list_del_init(&timer->list);
		timer->expired(timer);
	}
	return timeout;
}

void pollset_timer_init(pollset_timer_t *timer,
	void (*expired)(struct pollset_timer *timer))
{
	cd_list_init(&timer->list);
	timer->expired = expired;
}


// Note: it is allowed to call pollset_timer_kill
// twice on the same timer since cd_list_del_init reinitializes element
void pollset_timer_kill(pollset_timer_t *timer)
{
	cd_list_del_init(&timer->list);
}


void pollset_timer_set(
	pollset_t *pollset, pollset_timer_t *timer, int timeout_ms)
{
	if (unlikely(timeout_ms < 0)) {
		timeout_ms = 0;
	}
	timer->expiry = pollset->now + timeout_ms;

	// paranoya.
	cd_list_del_init(&timer->list);

	pollset_timer_t *t;
	cd_list_for_each_entry(pollset_timer_t, t, &pollset->timers, list) {
		if ((int)(timer->expiry - t->expiry) < 0) {
			// add before curr. elem
			cd_list_add_tail(&timer->list, &t->list);
			return;
		}
	}
	cd_list_add_tail(&timer->list, &pollset->timers);
}


void pollset_timer_restart(
	pollset_t *pollset, pollset_timer_t *timer, int timeout_ms)
{
	pollset_timer_set(pollset, timer,
		timer->expiry + timeout_ms - pollset->now);
}
