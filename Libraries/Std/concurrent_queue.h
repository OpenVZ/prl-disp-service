//////////////////////////////////////////////////////////////////////////
///
/// @file concurrent_queue.h
///
/// @brief
/// Concurrent queue implementation based on single linked list
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
#ifndef __STD_CONCURRENT_QUEUE__
#define __STD_CONCURRENT_QUEUE__

#include <Libraries/Std/AtomicOps.h>

/*
 * This may be redo via s_list, but i decide to logically
 * separate those items
 */
struct cq_entry
{
	struct cq_entry *next;
};

/*
 * Special structure to link classes to queue. offsetof is not working for nonPOD
 * members, so class must store its reference in the structure.
 * Note: you can safely cast pointer from cq_nonpod_entry to pointer to cq_entry (if
 * nobody will not change link placement)
 */
struct cq_nonpod_entry
{
	struct cq_entry link;
	void *data;
};

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
struct cq_queue {
	struct cq_entry * volatile head;
	struct cq_entry * volatile tail;
	struct cq_entry *pseudo_null;
	struct cq_entry dummy;
};

static inline void cq_init(struct cq_queue *q)
{
	q->pseudo_null = (struct cq_entry *)q;
	q->dummy.next = q->pseudo_null;
	q->head = &q->dummy;
	q->tail = &q->dummy;
}

static inline void cq_enqueue(struct cq_queue *q, struct cq_entry *in)
{
	struct cq_entry *tail;
	// Cache entry to avoid useless dereferences
	struct cq_entry *p_null = q->pseudo_null;

	in->next = p_null;

	while (1) {
		tail = q->tail;

		// Last node always points to dummy node. So next should be checked for it.
		if (AtomicCompareSwapPv((void**)&tail->next, p_null, in) == p_null)
			break;

		/*
		 * Looks like some other thread just updated tail,
		 * so we need to update it and recharge enque, if we failed - thus
		 * it is updated already and next iteration (hopefully) will do
		 * the work
		 */
		AtomicCompareSwapPv((void**)&q->tail, tail, tail->next);
	}

	/*
	 * Update tail pointer. If we do not succeeded - leave it as is,
	 * other thread in progress and update it at next enqueue
	 */
	AtomicCompareSwapPv((void**)&q->tail, tail, in);
}

static inline struct cq_entry *cq_dequeue(struct cq_queue *q)
{
	struct cq_entry *p_null = q->pseudo_null;
	struct cq_entry *head, *tail, *next;

	while (1) {
		head = q->head;
		next = head->next;
		tail = q->tail;

		if (head != tail) {
			// Try to own head pointer
			if (AtomicCompareSwapPv((void**)&q->head, head, next) != head)
				continue;

			// Is it real node or our dummy one?
			if (head != &q->dummy)
				return head;

			/*
			 * Dummy node should be pushed back
			 */
			cq_enqueue(q, &q->dummy);
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
		AtomicCompareSwapPv((void**)&q->tail, tail, next);
	}

	// Can't came here
	return NULL;
}
#endif
