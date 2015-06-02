//////////////////////////////////////////////////////////////////////////
///
/// @file poll_event.h
///
/// @brief event that can be added to pollset
///
/// Copyright (c) 2012-2015 Parallels IP Holdings GmbH
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
#ifndef PollableEvent_h__
#define PollableEvent_h__

#include "pollset.h"

struct poll_event
{
#if defined(_MAC_)
	int read_sock;
	int write_sock;
#elif defined(_LIN_)
	/* Note: when eventfd-syscall is supported by kernel,
	 * read_sock == write_sock == eventfd
	 * Otherwise pipe is used
	 */
	int read_sock;
	int write_sock;
#elif defined(_WIN_)
	prl_handle event_handle;
#endif
};

/*
 * Construct event and fill it with default invalid values
 * This avoid pipe creations and failures in constructors.
 */
static inline void pollev_construct(struct poll_event *ev)
{
#ifdef _WIN_
	ev->event_handle = NULL;
#else
	ev->read_sock = -1;
	ev->write_sock = -1;
#endif
}

/* Init event.
 * @param AutoReset has meaning only in Windows,
 *		means then event is cleared as soon as WaitForSingleObject
 *		detects that event is signaled
 */
bool pollev_init(struct poll_event *ev, bool ManualReset);

/* Close */
void pollev_close(struct poll_event *ev);

/* Signal event */
void pollev_signal(struct poll_event *ev);

/* Reset event to non-signalled state */
void pollev_reset(struct poll_event *ev);

static inline prl_handle
pollev_get_readfd(struct poll_event *ev)
{
#if !defined(_WIN_)
	return ev->read_sock;
#else
	return ev->event_handle;
#endif
}


static inline prl_handle
pollev_get_writefd(struct poll_event *ev)
{
#if !defined(_WIN_)
	return ev->write_sock;
#else
	return ev->event_handle;
#endif
}

#if defined(_LIN_)
int prl_open_eventfd(int count);
#endif

#endif // PollableEvent_h__
