//////////////////////////////////////////////////////////////////////////
///
/// @file pollset_win.cpp
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
#include <Windows.h>
#include <Libraries/Logging/Logging.h>
#include <Libraries/Std/PrlTime.h>
#include <Libraries/Std/PrlAssert.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>

bool pollset_init(pollset_t *pollset)
{
	// code is written in assumption that one tick is one millisecond
	PRL_ASSERT(PrlGetTicksPerSecond() == 1000);

	pollset->size = PRL_POLLSET_INITIAL_SIZE;
	pollset->count = 0;
	pollset->entries = NULL;
	pollset->events = (HANDLE *)malloc(pollset->size * sizeof(HANDLE));
	if (NULL == pollset->events)
		return false;

	pollset->entries = (pollset_entry_t **)malloc(
				pollset->size * sizeof(pollset_entry_t *));
	if (NULL == pollset->entries) {
		::free(pollset->events);
		pollset->events = NULL;
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

	if (NULL == p->events || p->size >= MAXIMUM_WAIT_OBJECTS)
		return false;

	HANDLE *new_events = (HANDLE *)malloc(size * sizeof(HANDLE));
	if (NULL == new_events)
		return false;

	pollset_entry_t **new_entries = (pollset_entry_t **)malloc(
				size * sizeof(pollset_entry_t *));
	if (NULL == new_entries) {
		free(new_events);
		return false;
	}

	memcpy(new_events, p->events, p->count * sizeof(HANDLE));
	memcpy(new_entries, p->entries, p->count * sizeof(pollset_entry_t *));

	free(p->events);
	free(p->entries);

	p->events = new_events;
	p->entries = new_entries;

	p->size = size;

	return true;
}


void pollset_deinit(pollset_t *pollset)
{
	::free(pollset->events);
	::free(pollset->entries);
	pollset->events = NULL;
	pollset->entries = NULL;
}


bool pollset_add(pollset_t *pollset,
		pollset_entry_t *entry, prl_handle h, UINT16 events)
{
	(void)events;
	if (pollset->count >= MAXIMUM_WAIT_OBJECTS) {
		WRITE_TRACE(DBG_FATAL, "pollset_add: max num of events reached: %d",
					pollset->count);
		return false;
	}
	if (pollset->count >= pollset->size) {
		int new_size = pollset->size + PRL_POLLSET_SIZE_INCREMENT;
		if (new_size > MAXIMUM_WAIT_OBJECTS)
			new_size = MAXIMUM_WAIT_OBJECTS;
		HANDLE *events = (HANDLE *)realloc(
				pollset->events, new_size * sizeof(HANDLE));
		if (events == NULL)
			return false;
		pollset->events = events;
		pollset_entry_t **entries = (pollset_entry_t **)realloc(
				pollset->entries, new_size * sizeof(pollset_entry_t *));
		if (NULL == entries)
			return false; // note: no need to free events!
		pollset->entries = entries;
		pollset->size = new_size;
	}
	// add to the end to not give to the handle priority on more
	// recent entries,
	// because (WaitFor...) always returns the entry with min idx)
	pollset->events[pollset->count] = h;
	pollset->entries[pollset->count] = entry;
	pollset->count++;
	return true;
}


void pollset_remove(pollset_t *pollset, pollset_entry_t *entry)
{
	// scan starting at end of array, because
	// if pollset_remove is called from callback,
	// the entry is the last in the array.
	for (int i = pollset->count - 1; i >= 0; --i) {
		if (pollset->entries[i] != entry)
			continue;
		--pollset->count;
		if (pollset->count == 0 || i > pollset->count)
			return;
		memmove(pollset->events + i, pollset->events + i + 1,
				(pollset->count - i) * sizeof(*pollset->events));
		memmove(pollset->entries + i, pollset->entries + i + 1,
				(pollset->count - i) * sizeof(*pollset->entries));
		return;
	}
}


int pollset_poll(pollset_t *pollset, int timeout_ms)
{
	int timer_timeout_ms = __pollset_poll_timers(pollset);
	int t = timeout_ms;
	if ((UINT32)timer_timeout_ms < (UINT32)timeout_ms)
		t = timer_timeout_ms;
	if (t < 0)
		t = INFINITE;
	DWORD r = WaitForMultipleObjectsEx(
				pollset->count, pollset->events,
				FALSE/*fWaitAll*/, (DWORD)t/*ms*/, TRUE/*fAlertable*/);

	// need to handle situation when event is removed inside
	// __pollset_poll_timers(). To do so, remember corresponding hEvent.
	HANDLE hSignalledEvent = NULL;
	if (likely(r >= WAIT_OBJECT_0 && r < (WAIT_OBJECT_0 + pollset->count))) {
		int idx = r - WAIT_OBJECT_0;
		hSignalledEvent = pollset->events[idx];
	}

	// Need to refresh pollset->now here or each time in timer_set
	// because caller may want to add timers
	__pollset_poll_timers(pollset);

	if (r == WAIT_TIMEOUT)
		return 0;

	if (unlikely(NULL == hSignalledEvent)) {
		if (r == WAIT_IO_COMPLETION)
			return -EINTR;

		// Always return negative;
		int rv = (int)::GetLastError();
		if (rv == 0)
			rv = -1;
		else if (rv > 0)
			rv = -rv;

		WRITE_TRACE_RL(20, DBG_FATAL,
			"pollset_poll failed with %u: %u", r, ::GetLastError());
		__pollset_refresh_time(pollset); // may cleanup errno
		return rv;
	}

	int idx = r - WAIT_OBJECT_0;
	pollset_entry_t *entry = NULL;
	// event position could change while pollset_poll_timers().
	if (unlikely(idx >= pollset->count || hSignalledEvent != pollset->events[idx]))
	{
		// search for new position of entry
		idx = -1;
		for (int i = 0; i<pollset->count; ++i) {
			if (hSignalledEvent == pollset->events[i]) {
				idx = i;
				break;
			}
		}
		if (idx < 0)
			return 1;
	}
	entry = pollset->entries[idx];

	// move fired event to the end of array to equalize
	// events' chances of being detected. this assumes that
	// the kernel indicates lowest index as declared by MSDN
	if (idx != (pollset->count - 1)) {
		HANDLE event = pollset->events[idx];
		memmove(pollset->events + idx, pollset->events + idx + 1,
				(pollset->count - idx - 1) * sizeof(*pollset->events));
		memmove(pollset->entries + idx, pollset->entries + idx + 1,
				(pollset->count - idx - 1) * sizeof(*pollset->entries));
		pollset->events[pollset->count - 1] = event;
		pollset->entries[pollset->count - 1] = entry;
	}
	entry->wake(entry, 0);
	return 1;
}
