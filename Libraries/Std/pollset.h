//////////////////////////////////////////////////////////////////////////
///
/// @file pollset.h
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
#ifndef prl_pollset_h__
#define prl_pollset_h__

#include <Interfaces/ParallelsTypes.h>
#include <Libraries/Std/list.h>

typedef struct pollset_entry pollset_entry_t;
typedef struct pollset pollset_t;

struct pollset
{
#if defined(_MAC_)
	int kq; // kqueue descriptor
	int size; // maximum size of the pollset
	int count; // current num of events

	struct kevent *kq_events;
	int kq_events_signalled;
	int curr_event; // index of evt currently-processed in pollset_poll()
#elif defined(_LIN_)
	int epfd; // epoll descriptor
	int size; // maximum size of the pollset
	int count; // current num of events

	struct epoll_event *ep_events;
	int ep_events_signalled;
	int curr_event;
#elif defined(_WIN_)
	int size; // maximum size of the pollset
	int count; // current num of events
	void **events;
	pollset_entry_t **entries;
#else
#error "Undefined platform"
#endif

	UINT32 now; // curr-time
	struct cd_list timers;
};

#if defined(_WIN_)
typedef void *prl_handle;
#else
typedef int prl_handle;
#endif

/** Note: constants matches the values from sys/poll.h */
enum POLLSET_EVENTS_MASK {
	PRL_POLLIN  = 0x0001,
	PRL_POLLOUT = 0x0004,
	PRL_POLLHUP = 0x0010,
	PRL_POLLERR = 0x0008,
};

struct pollset_entry
{
	// Callback on pending-event on the entry
	void (*wake)(pollset_entry *entry, UINT16 revents);
#if !defined(_WIN_)
	pollset_t *pollset;
	prl_handle fd;
	UINT16 events; // requested events (POLLSET_EVENTS_MASK)
#endif
};

static inline void pollset_entry_init(
	pollset_entry_t *e, void (*wake)(pollset_entry *entry, UINT16 revents))
{
#if !defined(_WIN_)
	e->pollset = NULL;
#endif
	e->wake = wake;
}

/**
 * Initialize pollset.
 * @return false if failed
 */
bool pollset_init(pollset_t *p);
void pollset_deinit(pollset_t *p);

/**
 * Ensure that pollset-size is at least size.
 * Note: it is not necessary to call this func.
 * Pollset automatically resizes itself when needed.
 *
 * The function may not be called from inside of the poll-callback.
 */
bool pollset_reserve(pollset_t *p, int size);

/**
 * add event to poll.
 * @param events - set of POLLSET_EVENTS_MASK events to poll for. Ignored on Win32
 * @return true on success
 */
bool pollset_add(pollset_t *p, pollset_entry_t *entry, prl_handle h, UINT16 events);

/*
 * remove event from pollset
 */
void pollset_remove(pollset_t *p, pollset_entry_t *entry);

/**
 * poll events and timers and call their callbacks
 * return false on system-error or signal.
 * always returns when some event-callback returned false
 * @return num of fired events if any
 *         0 on timeout
 *         -errno on system-error
 */
int pollset_poll(pollset_t *pollset, int timeout_ms);

/**
 * Timers
 */
typedef struct pollset_timer {
	struct cd_list list;
	// internal, absolute deadline
	UINT32 expiry;
	//< Expiration callback
	void (*expired)(struct pollset_timer *timer);
} pollset_timer_t;

/**
 * Initialize entry
 */
void pollset_timer_init(pollset_timer_t *timer,
	void (*expired)(struct pollset_timer *timer));

/**
 * Stop timer
 */
void pollset_timer_kill(pollset_timer_t *timer);

/**
 * Setup timer.
 * timeout_ms < 0 is treated as 0.
 * it is safe to add timer twice
 */
void pollset_timer_set(pollset_t *pollset,
				pollset_timer_t *timer, int timeout_ms);

/**
 * Should be called only from timer-callback.
 * Looks at pollset->curr_time and timer->expiry and sets up
 * timer to expiry at (timer->expiry - pollset->curr_time + timeout_ms)
 * Provides ability to high-precision timer.
 */
void pollset_timer_restart(pollset_t *pollset,
				pollset_timer_t *timer, int timeout_ms);

#endif // prl_pollset_h__
