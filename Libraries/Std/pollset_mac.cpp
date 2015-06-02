//////////////////////////////////////////////////////////////////////////
///
/// @file pollset_mac.cpp
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

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/poll.h>
#include <string.h>

#include <Libraries/Logging/Logging.h>
#include <Libraries/Std/PrlTime.h>
#include <Libraries/Std/PrlAssert.h>

BUILD_BUG_ON(PRL_POLLIN != POLLIN);
BUILD_BUG_ON(PRL_POLLOUT != POLLOUT);
BUILD_BUG_ON(PRL_POLLHUP != POLLHUP);
BUILD_BUG_ON(PRL_POLLERR != POLLERR);

bool pollset_init(pollset_t *pollset)
{
	// code is written in assumption that one tick is one millisecond
	PRL_ASSERT(PrlGetTicksPerSecond() == 1000);

	pollset->size = PRL_POLLSET_INITIAL_SIZE;
	pollset->count = 0;
	pollset->kq_events_signalled = 0;
	pollset->curr_event = 0;
	pollset->kq_events = NULL;
	pollset->kq = kqueue();
	if (pollset->kq < 0) {
		WRITE_TRACE(DBG_FATAL, "Failed to open kqueue(): %d", errno);
		return false;
	}

	pollset->kq_events = (struct kevent *)malloc(
				pollset->size * sizeof(struct kevent));
	if (NULL == pollset->kq_events) {
		::close(pollset->kq);
		pollset->kq = -1;
		return false;
	}

	cd_list_init(&pollset->timers);
	__pollset_refresh_time(pollset);
	return true;
}


bool pollset_reserve(pollset_t *p, int size)
{
	size = 2 * size; // usually need 2 events for one FD
	if (p->size >= size)
		return true;
	if (NULL == p->kq_events)
		return false;

	struct kevent *new_events = (struct kevent *)malloc(
			size * sizeof(struct kevent));
	if (NULL == new_events)
		return false;

	memcpy(new_events, p->kq_events, p->count * sizeof(struct kevent));
	free(p->kq_events);

	p->kq_events = new_events;
	p->size = size;

	return true;
}


void pollset_deinit(pollset_t *pollset)
{
	free(pollset->kq_events);
	pollset->kq_events = NULL;
	if (pollset->kq >= 0)
		::close(pollset->kq);
	pollset->kq = -1;
}


static bool
__pollset_remove(pollset_t *pollset, int fd, UINT32 ev_filt, const char *op)
{
	if (pollset->count <= 0) {
		WRITE_TRACE(DBG_FATAL,
					"invalid pollset_remove (%s): no entries in pollset",
					op);
		PRL_ASSERT(pollset->count > 0);
		return false;
	}
	struct kevent ev;
	EV_SET(&ev, fd, ev_filt, EV_DELETE, 0, 0, NULL);
	int err = kevent(pollset->kq, &ev, 1, NULL, 0, NULL);
	if (unlikely(err < 0)) {
		WRITE_TRACE_RL(20, DBG_FATAL,
				"Failed to del kevent (%s): %d", op, errno);
		PRL_ASSERT(err >= 0);
	}
	--pollset->count;
	return true;
}


bool pollset_add(pollset_t *pollset, pollset_entry_t *entry,
		prl_handle h, UINT16 events)
{
	if (0 == (events & ~(PRL_POLLHUP | PRL_POLLERR)))
		return true; // nothing to poll for

	PRL_ASSERT(entry->pollset == NULL);

	// usually need to add 2 events (one for read and one for write)
	if ((pollset->count + 2) >= pollset->size) {
		int new_size = pollset->size + PRL_POLLSET_SIZE_INCREMENT;
		struct kevent *tmp = (struct kevent *)realloc(
			pollset->kq_events, new_size * sizeof(struct kevent));
		if (!tmp)
			return false;
		pollset->kq_events = tmp;
		pollset->size = new_size;
		LOG_MESSAGE(DBG_WARNING,
				"pollset size was increased to %d entries", new_size);
	}
	entry->events = events;
	entry->fd = h;

	struct kevent ev;
	if (events & PRL_POLLIN) {
		EV_SET(&ev, entry->fd, EVFILT_READ, EV_ADD, 0, 0, entry);
		int err = kevent(pollset->kq, &ev, 1, NULL, 0, NULL);
		if (unlikely(err < 0)) {
			WRITE_TRACE_RL(20, DBG_FATAL,
				"Failed to add kevent to kqueue: %d", errno);
			PRL_ASSERT(err >= 0);
			return false;
		}
		++pollset->count;
	}

	if (events & PRL_POLLOUT) {
		EV_SET(&ev, entry->fd, EVFILT_WRITE, EV_ADD, 0, 0, entry);
		int err = kevent(pollset->kq, &ev, 1, NULL, 0, NULL);
		if (unlikely(err < 0)) {
			WRITE_TRACE_RL(20, DBG_FATAL,
				"Failed to add kevent to kqueue: %d", errno);
			// remove read-event
			if (events & PRL_POLLIN)
				__pollset_remove(pollset, h, EVFILT_READ, "POLLIN");
			return false;
		}
		++pollset->count;
	}

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

	entry->pollset = NULL;

	if (entry->events & PRL_POLLIN) {
		if (unlikely(!__pollset_remove(
				pollset, entry->fd, EVFILT_READ, "POLLIN")))
			return;
	}

	if (entry->events & PRL_POLLOUT) {
		if (unlikely(!__pollset_remove(
				pollset, entry->fd, EVFILT_WRITE, "POLLOUT")))
			return;
	}

	// Scan currently-signalled events and fix this entry
	// Note: nothing to scan if we are not in pollset_poll() curently.
	// Need to scan only unprocessed yet in pollset_poll() events.
	for (int i = pollset->curr_event; i < pollset->kq_events_signalled; ++i) {
		struct kevent *kev = &pollset->kq_events[i];
		if (kev->udata == entry)
			kev->udata = NULL;
	}
}


int pollset_poll(pollset_t *pollset, int timeout_ms)
{
	pollset->kq_events_signalled = 0;

	int timer_timeout_ms = __pollset_poll_timers(pollset);
	int t = timeout_ms;
	if ((UINT32)timer_timeout_ms < (UINT32)timeout_ms)
		t = timer_timeout_ms;

	struct timespec ts;
	struct timespec *ptime = NULL;
	if (t >= 0) {
		ts.tv_sec = t / 1000;
		ts.tv_nsec = (t % 1000) * 1000000;
		ptime = &ts;
	}

	int count = kevent(pollset->kq, NULL, 0,
					   pollset->kq_events, pollset->count, ptime);
	if (unlikely(count < 0)) {
		int err = errno;
		__pollset_refresh_time(pollset); // may cleanup errno
		return -err;
	}

	pollset->curr_event = 0;
	pollset->kq_events_signalled = count;

	// some of callbacks may want to add timer. Need
	// to either refresh pollset->now here or each time in timer_set
	__pollset_poll_timers(pollset);

	if (count == 0)
		return 0;

	for (int i = 0; i < count; ++i) {
		struct kevent *ev = &pollset->kq_events[i];
		pollset_entry_t *entry = (pollset_entry_t *)ev->udata;
		if (entry == NULL)
			continue; // the event was deleted
		int revents = 0;
		if (ev->flags & EV_EOF)
			revents |= PRL_POLLHUP;
		if (ev->flags & EV_ERROR)
			revents |= PRL_POLLERR;
		int mask = 0;
		if (ev->filter == EVFILT_READ) {
			if (revents & PRL_POLLHUP)
				mask |= PRL_POLLIN;
			else if (ev->data != 0)
				mask |= PRL_POLLIN;
			revents |= (entry->events & mask);
		}
		else if (ev->filter == EVFILT_WRITE) {
			if (0 == (revents & PRL_POLLHUP))
				revents |= (entry->events & PRL_POLLOUT);
		}

		if (likely(0 != revents)) {
			pollset->curr_event = i;
			int fd = (int)ev->ident;
			if (fd < 0 || entry->fd != fd) {
				WRITE_TRACE(DBG_FATAL,
					"pollset_poll: entry->fd != fd of fd < 0: entry->fd = %d, fd = %d",
					entry->fd, fd);
				WRITE_TRACE(DBG_FATAL,
					"pollset_size = %d, count = %d, curr_event = %d, event_signalled = %d",
					(int)pollset->size, (int)count, i, (int)pollset->kq_events_signalled);
			}
			entry->wake(entry, revents);
		}
		else {
			WRITE_TRACE_RL(60, DBG_FATAL,
				"nowake, filter %d, flags 0x%x, data %ld, entry_revents 0x%x",
				ev->filter, ev->flags, ev->data, entry->events);
			PRL_ASSERT(0 != revents);
		}
	}
	pollset->kq_events_signalled = 0;
	return count;
}
