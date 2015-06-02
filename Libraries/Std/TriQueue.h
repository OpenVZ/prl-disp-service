/*
* Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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

#pragma once

#include <assert.h>
#include <Libraries/Std/AtomicOps.h>


#ifdef _MSC_VER
#if (_MSC_VER >= 1200)
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE __inline
#endif
#else
#define FORCEINLINE inline
#endif

// Number of items in queue
#define TRIQUEUE_SIZE 3

typedef struct _TRIQUEUE
{
	// Stores index of the most recent item.
	// Reader atomic-swap it with "size".
	// Writer atomic-swap it with wToPush:
	UINT32 rwPushed;
	// Writer stores index of last pushed item here.
	UINT32 wLastPushed;
	// Index of item that writer is currently using and that will be
	// pushed next.
	UINT32 wToPush;
	// Index of item that previously was pushed and in some cases
	// can be still used by reader.
	UINT32 wBusy;
	// Index of the last item that was retrieved by reader. Used
	// to always have index of last item even when there is no
	// new item in rwPushed (still set to "size").
	UINT32 rLastPushed;
} TRIQUEUE;

// Initialize queue for first use.
FORCEINLINE void TriQueueReset(TRIQUEUE *const q)
{
	q->rwPushed = 0;
	q->rLastPushed = 0;
	q->wLastPushed = 0;
	q->wToPush = 1;
	q->wBusy = 2;
}

// Returns index of last pushed item (used by reader).
FORCEINLINE UINT32 TriQueueGetPushed(TRIQUEUE *const q)
{
	// By setting "TRIQUEUE_SIZE" reader tells writer, that wLastPushed
	// already can be in use by reader.
	// This also indicates that wBusy is no longer used by reader
	// and can be used by writer.
	const UINT32 k = AtomicSwap(&q->rwPushed, TRIQUEUE_SIZE);
	if (TRIQUEUE_SIZE != k)
	{
		q->rLastPushed = k;
	}
	// rLastPushed now contains the most recent item.
	return q->rLastPushed;
}

// Returns index of item that will be pushed next (used by writer).
FORCEINLINE UINT32 TriQueueGetToPush(TRIQUEUE *const q)
{
	return q->wToPush;
}

// Pushes item returned by getToPush() (used by writer).
FORCEINLINE void TriQueuePush(TRIQUEUE *const q)
{
	const UINT32 k = AtomicSwap(&q->rwPushed, q->wToPush);
	if (TRIQUEUE_SIZE == k)
	{
		// 1. Since wToPush is already in rwPushed it already
		//    can be used by reader.
		// 2. wLastPushed can be still used by reader.
		// 3. wBusy is not used by reader (because k is "TRIQUEUE_SIZE"),
		//    therefore can be used by writer in next
		//    iteration.
		const UINT32 pushed = q->wLastPushed;
		q->wLastPushed = q->wToPush;
		q->wToPush = q->wBusy;
		q->wBusy = pushed;
	}
	else
	{
		// 1. wLastPushed was never used by reader. And
		//    will not, since it was replaced with wToPush.
		//    Therefore wLastPushed can be used by writer in
		//    next iteration.
		// 2. Since wToPush is already in rwPushed it already
		//    can be used by reader.
		// 3. wBusy can be still in use by reader, since rwPushed
		//    was not "size".
		const UINT32 pushed = q->wToPush;
		// Value of k must be wLastPushed here.
		// If not, there is an error in reader or writer.
		assert(k == q->wLastPushed);
		q->wToPush = q->wLastPushed;
		q->wLastPushed = pushed;
	}
}
