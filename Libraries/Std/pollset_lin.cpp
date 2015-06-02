//////////////////////////////////////////////////////////////////////////
///
/// @file pollset_lin.cpp
///
/// @author sdmitry
///
/// Abstraction for platform-independent polling
/// of file-descriptors/events and timers.
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

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <Libraries/Logging/Logging.h>
#include <Libraries/Std/PrlTime.h>
#include <Libraries/Std/PrlAssert.h>

BUILD_BUG_ON(PRL_POLLIN != (int)EPOLLIN);
BUILD_BUG_ON(PRL_POLLOUT != (int)EPOLLOUT);
BUILD_BUG_ON(PRL_POLLHUP != (int)EPOLLHUP);
BUILD_BUG_ON(PRL_POLLERR != (int)EPOLLERR);

bool pollset_init(pollset_t *pollset)
{
	// code is written in assumption that one tick is one millisecond
	PRL_ASSERT(PrlGetTicksPerSecond() == 1000);

	pollset->size = PRL_POLLSET_INITIAL_SIZE;
	pollset->count = 0;
	pollset->ep_events_signalled = 0;
	pollset->curr_event = 0;
	pollset->ep_events = NULL;
	pollset->epfd = epoll_create(PRL_POLLSET_INITIAL_SIZE);
	if (pollset->epfd < 0) {
		WRITE_TRACE(DBG_FATAL, "Failed to epoll_create(): %d", errno);
		return false;
	}

	pollset->ep_events = (struct epoll_event *)malloc(
		pollset->size * sizeof(struct epoll_event));
	if (NULL == pollset->ep_events) {
		::close(pollset->epfd);
		pollset->epfd = -1;
		return false;
	}

	cd_list_init(&pollset->timers);
	__pollset_refresh_time(pollset);
	return true;
}


bool pollset_reserve(pollset_t *p, int size)
{
	if (p->size >= size)
		return true;

	if (NULL == p->ep_events)
		return false;

	struct epoll_event *new_events = (struct epoll_event *)malloc(
		size * sizeof(struct epoll_event));
	if (NULL == new_events)
		return false;

	memcpy(new_events, p->ep_events, p->count * sizeof(struct epoll_event));
	free(p->ep_events);

	p->ep_events = new_events;
	p->size = size;

	return true;
}


void pollset_deinit(pollset_t *pollset)
{
	free(pollset->ep_events);
	pollset->ep_events = NULL;
	if (pollset->epfd >= 0)
		::close(pollset->epfd);
	pollset->epfd = -1;
}


bool pollset_add(pollset_t *pollset,
		pollset_entry_t *entry, prl_handle h, UINT16 events)
{
	if (0 == (events & ~(PRL_POLLHUP | PRL_POLLERR)))
		return true; // nothing to poll for

	PRL_ASSERT(entry->pollset == NULL);

	if ((pollset->count + 1) >= pollset->size) {
		int new_size = pollset->size + PRL_POLLSET_SIZE_INCREMENT;
		struct epoll_event *tmp = (struct epoll_event *)realloc(
			pollset->ep_events, new_size * sizeof(struct epoll_event));
		if (!tmp)
			return false;
		pollset->ep_events = tmp;
		pollset->size = new_size;
		LOG_MESSAGE(DBG_WARNING,
			"pollset size was increased to %d entries", new_size);
	}
	entry->events = events;
	entry->fd = h;

	struct epoll_event ev;
	ev.data.ptr = (void *)entry;
	ev.events = 0;
	if (events & PRL_POLLIN)
		ev.events |= EPOLLIN;
	if (events & PRL_POLLOUT)
		ev.events |= EPOLLOUT;
	int err = epoll_ctl(pollset->epfd, EPOLL_CTL_ADD, h, &ev);
	if (unlikely(err < 0)) {
		WRITE_TRACE_RL(20, DBG_FATAL,
				"Failed to add event to epoll: %d", errno);
		return false;
	}
	++pollset->count;

	entry->pollset = pollset;
	return true;
}


void pollset_remove(pollset_t *pollset, pollset_entry_t *entry)
{
	if (NULL == entry->pollset)
		return;

	if (entry->pollset != pollset) {
		WRITE_TRACE(DBG_FATAL, "pollset_remove: entry is not initialized!");
		PRL_ASSERT(entry->pollset == pollset);
		return;
	}

	if (pollset->count <= 0) {
		WRITE_TRACE(DBG_FATAL,
				"invalid pollset_remove: no entries in pollset");
		PRL_ASSERT(pollset->count > 0);
		return;
	}

	entry->pollset = NULL;

	// Scan currently-signalled events and fix this entry
	// Note: nothing to scan if we are not in pollset_poll() curently.
	// Need to scan only unprocessed yet in pollset_poll() events.
	for (int i = pollset->curr_event; i < pollset->ep_events_signalled; ++i) {
		struct epoll_event *ev = &pollset->ep_events[i];
		if (ev->data.ptr == entry)
			ev->data.ptr = NULL;
	}

	// protect from buggy-kernel. Pass empty event.
	// see man epoll_ctl
	struct epoll_event epe;
	int err = epoll_ctl(pollset->epfd, EPOLL_CTL_DEL, entry->fd, &epe);
	if (unlikely(err < 0)) {
		WRITE_TRACE(DBG_FATAL,
					"Failed to pollset_remove: EPOLL_CTL_DEL: %d", errno);
	}
	--pollset->count;
}

int pollset_poll(pollset_t *pollset, int timeout_ms)
{
	pollset->ep_events_signalled = 0;

	int timer_timeout_ms = __pollset_poll_timers(pollset);
	int t = timeout_ms;
	if ((UINT32)timer_timeout_ms < (UINT32)timeout_ms)
		t = timer_timeout_ms;

	int count = epoll_wait(pollset->epfd,
						pollset->ep_events, pollset->count, t);
	if (unlikely(count < 0)) {
		int err = errno;
		__pollset_refresh_time(pollset); // may cleanup errno
		return -err;
	}

	pollset->curr_event = 0;
	pollset->ep_events_signalled = count;

	// some of callbacks may want to add timer. Need
	// to either refresh pollset->now here or each time in timer_set
	__pollset_poll_timers(pollset);

	if (count == 0)
		return 0;

	for (int i = 0; i < count; ++i) {
		struct epoll_event *ev = &pollset->ep_events[i];
		pollset_entry_t *entry = (pollset_entry_t *)ev->data.ptr;
		if (entry == NULL)
			continue; // the event was deleted
		int revents = 0;
		if (ev->events & EPOLLHUP)
			revents |= PRL_POLLHUP;
		if (ev->events & EPOLLERR)
			revents |= PRL_POLLERR;
		if (ev->events & EPOLLIN)
			revents |= (entry->events & PRL_POLLIN);
		if (ev->events & EPOLLOUT)
			revents |= (entry->events & PRL_POLLOUT);

		// Signal ERR and HUP even if not requested
		if (revents) {
			pollset->curr_event = i;
			entry->wake(entry, revents);
		}
		else {
			WRITE_TRACE_RL(60, DBG_FATAL,
						"nowake, ep_event 0x%x, entry_events 0x%x",
						revents, entry->events);
			PRL_ASSERT(0 != revents);
		}
	}
	pollset->ep_events_signalled = 0;
	return count;
}
