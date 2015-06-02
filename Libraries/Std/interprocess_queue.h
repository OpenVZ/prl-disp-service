//////////////////////////////////////////////////////////////////////////
///
/// @file interprocess_queue.h
///
/// @brief
/// Interprocess concurrent queue implementation based on single linked list
///
/// This code described in the following paper:
///		Simple, Fast, and Practical Non-Blocking and Blocking
///		Concurrent Queue Algorithms \Lambda
///		Maged M. Michael Michael L. Scott
///		Department of Computer Science
///		University of Rochester
///		Rochester, NY 14627-0226
///		fmichael,scottg@cs.rochester.edu
///
/// With some modifications, allowed skipping node allocation/deallocation in
/// enqueue and dequeue functions.
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
#ifndef __STD_INTERPROCESS_QUEUE__
#define __STD_INTERPROCESS_QUEUE__

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

/*
 * This may be redo via s_list, but i decide to logically
 * separate those items
 */
typedef struct _ipcq_entry
{
	LONG32 next;
} ipcq_entry;

/*
 * Special structure to link classes to queue. offsetof is not working for nonPOD
 * members, so class must store its reference in the structure.
 * Note: you can safely cast pointer from ipcq_nonpod_entry to pointer to ipcq_entry (if
 * nobody will not change link placement)
 */
typedef struct _ipcq_nonpod_entry
{
	ipcq_entry link;
	LONG64 data;
} ipcq_nonpod_entry;

/*
 * REMEMBER:
 * The node can't use NULL pointer directly or point to dummy node as null. Really,
 * imagine two threads push new node.
 *      1                         2
 * ------------------------------------------------------
 *   try to push             ...
 *    preempted             pop node thread 1 trying to link to
 *                          push to another queue
 *   ok, CAS succeeded
 *   because node still
 *   ending with NULL
 *   so, node linked now
 *   not in right queue
 *
 * To avoid it, use special variable as null.
 */
typedef struct _ipcq_queue {
	LONG32 volatile head;
	LONG32 volatile tail;
	LONG32 pseudo_null;
	ipcq_entry dummy;
} ipcq_queue;

#define SUB_BASE(val, base) ((LONG32)((ULONG_PTR)(val) - (ULONG_PTR)(base)))
#define ADD_BASE(val, base) ((ULONG_PTR)(base) + (ULONG_PTR)(val))

static FORCEINLINE void ipcq_init(ipcq_queue *q, void *base)
{
	q->pseudo_null = SUB_BASE(q, base);
	q->dummy.next = q->pseudo_null;
	q->head = SUB_BASE(&q->dummy, base);
	q->tail = SUB_BASE(&q->dummy, base);
}

static FORCEINLINE void ipcq_enqueue(ipcq_queue *q, ipcq_entry *in, void *base)
{
	ipcq_entry *tail;
	// Cache entry to avoid useless dereferences
	LONG32 p_null = q->pseudo_null;
	LONG32 in_base = SUB_BASE(in, base);

	in->next = p_null;

	while (1) {
		tail = (ipcq_entry*)ADD_BASE(q->tail, base);

		// Last node always points to dummy node. So next should be checked for it.
		if (AtomicCompareSwap(&tail->next, p_null, in_base) == p_null)
			break;

		/*
		 * Looks like some other thread just updated tail,
		 * so we need to update it and recharge enque, if we failed - thus
		 * it is updated already and next iteration (hopefully) will do
		 * the work
		 */
		AtomicCompareSwap((LONG32*)&q->tail, SUB_BASE(tail, base), tail->next);
	}

	/*
	 * Update tail pointer. If we do not succeeded - leave it as is,
	 * other thread in progress and update it at next enqueue
	 */
	AtomicCompareSwap((LONG32*)&q->tail, SUB_BASE(tail, base), in_base);
}

static FORCEINLINE ipcq_entry *ipcq_dequeue(ipcq_queue *q, void *base)
{
	LONG32 p_null = q->pseudo_null;
	LONG32 head, tail, next;
	ipcq_entry *head_p;

	while (1) {
		head = q->head;
		head_p = (ipcq_entry*)ADD_BASE(head, base);
		next = head_p->next;
		tail = q->tail;

		if (head != tail) {
			// Try to own head pointer
			if (AtomicCompareSwap((LONG32*)&q->head, head, next) != head)
				continue;

			// Is it real node or our dummy one?
			if (head_p != &q->dummy)
				return head_p;

			/*
			 * Dummy node should be pushed back
			 */
			ipcq_enqueue(q, &q->dummy, base);
			continue;
		}

		/*
		 * Is there is no any data in list for this reader
		 */
		if (next == p_null)
			return NULL;

		/*
		 * Tail should be advanced, enque in middle of operation
		 * Note: the tail->next == head->next == next.
		 */
		AtomicCompareSwap((LONG32*)&q->tail, tail, next);
	}

	// Can't came here
	return NULL;
}

#endif
