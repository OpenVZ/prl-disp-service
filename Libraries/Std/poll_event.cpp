//////////////////////////////////////////////////////////////////////////
///
/// @file poll_event.cpp
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
#if defined(_MAC_)
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#elif defined(_WIN_)

#include <windows.h>

#elif defined(_LIN_)

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#endif

#include <errno.h>

#include <Libraries/Logging/Logging.h>
#include "poll_event.h"

#if defined(_MAC_)
static int __setup_pipe(int socks[2])
{
	int flags;
	// OSX doesn't support socketpair on AF_INET. how nice.
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, socks) < 0)
		return -1;

	if ((flags = fcntl(socks[0], F_GETFL, 0)) < 0
		|| fcntl(socks[0], F_SETFL, flags | O_NONBLOCK) < 0
		|| (flags = fcntl(socks[1], F_GETFL, 0)) < 0
		|| fcntl(socks[1], F_SETFL, flags | O_NONBLOCK) < 0)
	{
		close(socks[0]);
		close(socks[1]);
		return -1;
	}
	return 0;
}


bool pollev_init(struct poll_event *ev, bool ManualReset)
{
	(void)ManualReset;
	ev->read_sock = -1;
	ev->write_sock = -1;

	int socks[2];
	if (__setup_pipe(socks) < 0) {
		WRITE_TRACE(DBG_FATAL, "pollev_init: failed to create a pipe");
		return false;
	}
	ev->read_sock = socks[0];
	ev->write_sock = socks[1];
	return true;
}


void pollev_close(struct poll_event *ev)
{
	if (ev->read_sock >= 0)
		::close(ev->read_sock);
	if (ev->write_sock >= 0)
		::close(ev->write_sock);
	ev->read_sock = -1;
	ev->write_sock = -1;
}


void pollev_signal(struct poll_event *ev)
{
	int bytes = ::write(ev->write_sock, "1", 1);
	(void)bytes;
}


/* Reset event to non-signalled state */
void pollev_reset(struct poll_event *ev)
{
	char buf[64];
	ssize_t bytes;
	do {
		bytes = ::read(ev->read_sock, buf, sizeof(buf));
	} while (bytes == sizeof(buf));
}


#elif defined(_WIN_)


bool pollev_init(struct poll_event *ev, bool ManualReset)
{
	ev->event_handle = ::CreateEvent(NULL,
		ManualReset ? TRUE : FALSE /* auto-reset */, FALSE, NULL);
	if (NULL == ev->event_handle) {
		WRITE_TRACE(DBG_FATAL, "pollev_init failed: error 0x%x",
			::GetLastError());
		return false;
	}
	return true;
}


void pollev_close(struct poll_event *ev)
{
	if (ev->event_handle) {
		::CloseHandle(ev->event_handle);
		ev->event_handle = NULL;
	}
}

void pollev_signal(struct poll_event *ev)
{
	::SetEvent(ev->event_handle);
}


void pollev_reset(struct poll_event *ev)
{
	::ResetEvent(ev->event_handle);
}


#elif defined(_LIN_)


#ifndef __NR_eventfd
#  if defined(_AMD64_)
#    define __NR_eventfd 284
#  else
#    define __NR_eventfd 323
#  endif
#endif

#ifndef O_CLOEXEC
#define O_CLOEXEC 02000000
#endif

#ifndef __NR_eventfd2
#  if defined(_AMD64_)
#    define __NR_eventfd2 290
#  else
#    define __NR_eventfd2 328
#  endif
#endif

#ifndef EFD_NONBLOCK
#  define EFD_NONBLOCK O_NONBLOCK
#endif

#ifndef EFD_CLOEXEC
#  define EFD_CLOEXEC O_CLOEXEC
#endif

static int __fd_set_no_block(int fd)
{
	int flags;
	if ((flags = fcntl(fd, F_GETFL, 0)) < 0
			|| fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
		WRITE_TRACE(DBG_FATAL, "Failed to set O_NONBLOCK on fd %d: err %d",
				fd, errno);
		return -1;
	}
	return 0;
}

int prl_open_eventfd(int count)
{
	int ev = syscall(__NR_eventfd2, count, EFD_NONBLOCK | EFD_CLOEXEC);
	if (ev >= 0)
		return ev;
	if (errno != ENOSYS)
		return -1;

	static bool msg_logged = false;
	if (!msg_logged) {
		msg_logged = true;
		WRITE_TRACE(DBG_FATAL, "eventfd2 is not supported by kernel. Trying eventfd.");
	}

	ev = syscall(__NR_eventfd, count);
	if (ev < 0)
		return ev;
	if (__fd_set_no_block(ev) < 0) {
		int err = errno;
		::close(ev);
		errno = err;
		return -1;
	}
	return ev;
}


static int __setup_pipe(int fds[2])
{
	if (pipe(fds) < 0)
		return -1;
	if (__fd_set_no_block(fds[0]) < 0 || __fd_set_no_block(fds[1]) < 0) {
		::close(fds[0]);
		::close(fds[1]);
		return -1;
	}
	return 0;
}



bool pollev_init(struct poll_event *ev, bool ManualReset)
{
	(void)ManualReset;
	// when eventfd is not supported by kernel, read/write socks are used
	ev->read_sock = -1;
	ev->write_sock = -1;

	int eventfd = prl_open_eventfd(0);
	if (eventfd >= 0) {
		ev->read_sock = eventfd;
		ev->write_sock = eventfd;
	} else {
		if (errno != ENOSYS) {
			WRITE_TRACE(DBG_FATAL, "Failed to create eventfd: error %d", errno);
			return false;
		}

		static bool msg_logged = false;
		if (!msg_logged)
			WRITE_TRACE(DBG_FATAL,
				"eventfd-syscall is not supported by kernel. Using pipes.");

		int pipe_fds[2];
		if (__setup_pipe(pipe_fds) < 0) {
			WRITE_TRACE(DBG_FATAL, "pollev_init: failed to create a pipe");
			return false;
		}

		ev->read_sock = pipe_fds[0];
		ev->write_sock = pipe_fds[1];
	}

	return true;
}


void pollev_close(struct poll_event *ev)
{
	// when eventfd-syscal is supported, read_sock == write_sock.
	if (ev->read_sock >= 0)
		::close(ev->read_sock);
	if (ev->write_sock != ev->read_sock && ev->write_sock >= 0)
		::close(ev->write_sock);
	ev->read_sock = -1;
	ev->write_sock = -1;
}


void pollev_signal(struct poll_event *ev)
{
	UINT64 val = 1;
	// note: eventfd expects UINT64. UINT64 also works with pipes
	int bytes = ::write(ev->write_sock, &val, sizeof(UINT64));
	(void)bytes;
}


void pollev_reset(struct poll_event *ev)
{
	char buf[64];
	int bytes = ::read(ev->read_sock, buf, sizeof(buf));
	(void)bytes;
}

#endif
