/*
 * Copyright (c) 2015 Parallels IP Holdings GmbH
 *
 * This file is part of Virtuozzo Core Libraries. Virtuozzo Core
 * Libraries is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/> or write to Free Software Foundation,
 * 51 Franklin Street, Fifth Floor Boston, MA 02110, USA.
 *
 * Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#ifndef __MUTEX_H__
#define __MUTEX_H__

#define MUTEX_BLOCK_USEC 1000		/* usec */

/* Note: XXXIrqDisable/XXXIrqRestore versions for user space MUST NOT exist */

static __always_inline int RawMutexTryLock(SPINLOCK* slock)
{
	int res;

	res = RawSpinLockTryLock(slock);
	/* We don't set owner in user-space...
	 * And there is a reason for this:
	 * we can't take lock and set owner atomically.
	 * In monitor implementation of RawMutexTryLock() we simply use cli/sti.
	 *
	 * So instead we do another trick: SPINLOCK_NO_OWNER means
	 * that lock might have been taken from user-space and monitor reacts
	 * accordinly (reschedules).
	 */

	return res;
}

#ifndef _KERNEL_
#ifdef _WIN_
#include <windows.h>
#else
#include <time.h>
#endif

static void MutexYield(void)
{
#ifdef _WIN_
	Sleep(MUTEX_BLOCK_USEC / 1000);
#else
	struct timespec ts;

	ts.tv_sec = 0;
	ts.tv_nsec = MUTEX_BLOCK_USEC * 1000;
	nanosleep(&ts, NULL);
#endif
}
#endif /* !_KERNEL_ */

static __always_inline void MutexPause(SPINLOCK* slock)
{
#ifdef _KERNEL_
	/* always spin in hypervisor. FIXME: to be fixed */
	CpuPause();
#else
	int i;
	/* spin 1000 times (~0.01ms on 2.4Ghz Intel Dual Core) before blocking */
	for (i = 0; i < 1000; i++) {
		if (!SpinIsLocked(slock))
			return;
		CpuPause();
	}

	/* always yield in user-space */
	MutexYield();
#endif
	(void) slock;
}


static __always_inline void RawMutexLock(SPINLOCK* slock)
{
	while (!RawMutexTryLock(slock))
		while (SpinIsLocked(slock))
			MutexPause(slock);
}

static __always_inline void RawMutexUnlock(SPINLOCK* slock)
{
	slock->owner = SPINLOCK_NO_OWNER;
	RawSpinUnlock(slock);
}

#endif /* __MUTEX_H__ */
