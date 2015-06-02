//////////////////////////////////////////////////////////////////////////
///
/// @file RingBuffer.c
///
/// @brief Ring Buffer with size of pow of 2
///
/// @author kozerkov@
///
/// Description of the structures and constants required
/// for logging data exchange between Application and Monitor
/// that implements concept of single consumer and multiple producers
///
/// Copyright (c) 2006-2015 Parallels IP Holdings GmbH
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

#include "Libraries/Std/RingBuffer.h"

#define rbuf_memcpy(d,s,c) memcpy(d,s,c)

#define rbuf_data(rb)	((UINT8*)((rb) + 1))

#include <string.h>
#include "Libraries/Logging/Logging.h"


static UINT32 round_buffer_size(UINT32 buffer_size)
{
	if (!((buffer_size - 1) & buffer_size))
		return buffer_size;

	//
	// round size to near highest 2^N not greater than original
	//
	buffer_size--;
	buffer_size |= buffer_size >> 1;
	buffer_size |= buffer_size >> 2;
	buffer_size |= buffer_size >> 4;
	buffer_size |= buffer_size >> 8;
	buffer_size |= buffer_size >> 16;
#if 0
	if (sizeof(unsigned) > 4)
		buffer_size |= buffer_size >> 32; // for 64-bit unsigned
#endif
	buffer_size = (buffer_size + 1) >> 1;

	return buffer_size;
}

//
// initialize struct ring_buffer structure
//   buffer_size is adjusted to near highest 2^N (buffer size is 2^N elements,
//   but not greater than original size)
//   element_size is element sizeof (in bytes), for byte buffers can be 1 or 0
//
UINT32 rbuf_init(struct ring_buffer *rbuf, UINT32 buffer_size,
				 UINT32 element_size)
{
	// element_size == 0 is same as 1 (byte buffer)
	if (element_size < 1)
		element_size = 1;

	rbuf->buffer_size = round_buffer_size(buffer_size);
	rbuf->element_size = element_size;
	if (rbuf->buffer_size == 0)
			rbuf->small_mask = rbuf->big_mask = 0;
	else {
		rbuf->small_mask = rbuf->buffer_size - 1;
		rbuf->big_mask = (rbuf->buffer_size * 2) - 1;
	}

	rbuf->read_index = rbuf->write_index = 0;
	// no write/read_ptr assignment here to save context synchronization
	return rbuf->buffer_size;
}

//
// get current write region of requested size or smaller if buffer full
// NOTE: this call return abstract indexes/sizes neutral from element size
//   and memory pointers
//
static UINT32 rbuf_write_region(const struct ring_buffer *rbuf,
								UINT32 region_size, UINT32 *offset,
								UINT32 *size, UINT32 *wrapped_size)
{
	UINT32 free_size = rbuf_free_size(rbuf);

	// adjust size to available
	if (region_size > free_size)
		region_size = free_size;

	*offset = rbuf->write_index & rbuf->small_mask;

	if ((*offset + region_size) > rbuf->buffer_size) {
		*size = rbuf->buffer_size - (*offset);
		*wrapped_size = region_size - (*size);
	} else {
		*size = region_size;
		*wrapped_size = 0;
	}

	return region_size;
}

//
// get current read region of requested size or smaller if no data in buffer
// NOTE: this call return abstract indexes/sizes neutral from element size
//   and memory pointers
//
static UINT32 rbuf_read_region(const struct ring_buffer *rbuf,
							   UINT32 region_size, UINT32 *offset,
							   UINT32 *size, UINT32 *wrapped_size)
{
	UINT32 used_size = rbuf_used_size(rbuf);

	if (region_size > used_size)
		region_size = used_size;

	*offset = rbuf->read_index & rbuf->small_mask;

	if ((*offset + region_size) > rbuf->buffer_size) {
		*size = rbuf->buffer_size - (*offset);
		*wrapped_size = region_size - (*size);
	} else {
		*size = region_size;
		*wrapped_size = 0;
	}

	return region_size;
}

//
// get write region as memory pointers
// NOTE: all sizes are in elements
//
UINT32 rbuf_write_ptr(const struct ring_buffer *rbuf, UINT32 size,
					  void** p1, UINT32 *sz1, void** p2, UINT32 *sz2)
{
	UINT32 offset, ret_size;

	ret_size = rbuf_write_region(rbuf, size, &offset, sz1, sz2);

	offset *= rbuf->element_size;
	*p1 = rbuf_data(rbuf) + offset;

	if (*sz2 != 0)
		*p2 = rbuf_data(rbuf);
	else
		*p2 = NULL;

	return ret_size;
}

//
// get write region as memory pointers
// NOTE: all sizes are in elements
//
UINT32 rbuf_write_ptr_simple(const struct ring_buffer *rbuf,
							 UINT32 size, void** p1)
{
	UINT32 offset, ret_size, sz2;

	rbuf_write_region(rbuf, size, &offset, &ret_size, &sz2);

	offset *= rbuf->element_size;
	*p1 = rbuf_data(rbuf) + offset;

	return ret_size;
}

//
// get read region as memory pointers
// NOTE: all sizes are in elements
//
UINT32 rbuf_read_ptr(const struct ring_buffer *rbuf, UINT32 size,
					 void** p1, UINT32 *sz1, void** p2, UINT32 *sz2)
{
	UINT32 offset, ret_size;

	ret_size = rbuf_read_region(rbuf, size, &offset, sz1, sz2);

	offset *= rbuf->element_size;
	*p1 = rbuf_data(rbuf) + offset;

	if (*sz2 != 0)
		*p2 = rbuf_data(rbuf);
	else
		*p2 = NULL;

	return ret_size;
}

//
// get read region as memory pointers
// NOTE: all sizes are in elements
//
UINT32 rbuf_read_ptr_simple(const struct ring_buffer *rbuf,
							UINT32 size, void** p1)
{
	UINT32 offset, ret_size, sz2;

	rbuf_read_region(rbuf, size, &offset, &ret_size, &sz2);

	offset *= rbuf->element_size;
	*p1 = rbuf_data(rbuf) + offset;

	return ret_size;
}

//
// one step write to buffer
// NOTE: size is in elements
//
UINT32 rbuf_write(struct ring_buffer *rbuf, const void *src_data, UINT32 size)
{
	void *p1, *p2;
	UINT32 sz1, sz2, transfered;

	transfered = rbuf_write_ptr(rbuf, size, &p1, &sz1, &p2, &sz2);

	sz1 *= rbuf->element_size;
	sz2 *= rbuf->element_size;

	if (sz1 != 0)
		rbuf_memcpy(p1, src_data, sz1);
	if (sz2 != 0)
		rbuf_memcpy(p2, (UINT8 *)src_data + sz1, sz2);

	return rbuf_write_advance(rbuf, transfered);
}


//
// one step read from buffer
// NOTE: size is in elements
//
UINT32 rbuf_read(struct ring_buffer *rbuf, void *dst_data, UINT32 size)
{
	void *p1, *p2;
	UINT32 sz1, sz2, transfered;

	transfered = rbuf_read_ptr(rbuf, size, &p1, &sz1, &p2, &sz2);

	sz1 *= rbuf->element_size;
	sz2 *= rbuf->element_size;

	if (sz1 != 0)
		rbuf_memcpy(dst_data, p1, sz1);
	if (sz2 != 0)
		rbuf_memcpy((UINT8 *)dst_data + sz1, p2, sz2);

	return rbuf_read_advance(rbuf, transfered);
}


//
// special variant for network code (iovec based API)
// include  <sys/socket.h> or <sys/uio.h> before
//
#ifdef _STRUCT_IOVEC

//
// return write region as iovec structure
//
UINT32 rbuf_write_iov(const struct ring_buffer *rbuf,
					  struct iovec iov[2], UINT32 *niov)
{
	UINT32 ret_size, sz1, sz2;

	ret_size = rbuf_write_ptr(rbuf, rbuf->buffer_size,
								&iov[0].iov_base, &sz1,
								&iov[1].iov_base, &sz2);

	iov[0].iov_len = sz1 * rbuf->element_size;
	iov[1].iov_len = sz2 * rbuf->element_size;

	if (sz2 != 0)
		*niov = 2;
	else if (ret_size != 0)
		*niov = 1;
	else
		*niov = 0;

	return ret_size;
}

//
// return read region as iovec structure
//
UINT32 rbuf_read_iov(const struct ring_buffer *rbuf,
					 struct iovec iov[2], UINT32 *niov)
{
	UINT32 ret_size, sz1, sz2;

	ret_size = rbuf_read_ptr(rbuf, rbuf->buffer_size,
							 &iov[0].iov_base, &sz1,
							 &iov[1].iov_base, &sz2);

	iov[0].iov_len = sz1 * rbuf->element_size;
	iov[1].iov_len = sz2 * rbuf->element_size;

	if (sz2 != 0)
		*niov = 2;
	else if (ret_size != 0)
		*niov = 1;
	else
		*niov = 0;

	return ret_size;
}
#endif // _STRUCT_IOVEC



//
// initialize struct lrbuf structure
//   buffer_size is adjusted to near highest 2^N (buffer size is 2^N elements,
//   but not greater than original size)
//
UINT32 lrb_init(struct lrbuf *rb, UINT32 buffer_size)
{
	rb->buffer_size = round_buffer_size(buffer_size);

	if (rb->buffer_size == 0)
			rb->small_mask = rb->big_mask = 0;
	else {
		rb->small_mask = rb->buffer_size - 1;
		rb->big_mask = (rb->buffer_size * 2) - 1;
	}

	rb->read_idx = rb->write_idx = 0;
	rb->tail.u = 0;

	return rb->buffer_size;
}

//
// return single read region as
//
UINT32 lrb_read_ptr(const struct lrbuf *rb, UINT8 **ptr)
{
	UINT32 offset = rb->read_idx & rb->small_mask;
	UINT32 size = lrb_used_size(rb);

	if (offset + size > rb->buffer_size)
		size = rb->buffer_size - offset;
	*ptr = rbuf_data(rb) + offset;
	return size;
}


static INT32 lrb_write_reserv(struct lrbuf *rb, union shadow_tail *curr,
							  UINT32 size)
{
	union shadow_tail next;
	union shadow_tail prev;
	INT32 off;

	curr->u = AtomicRead64U(&rb->tail.u);
	while (1) {
		UINT32 used;
		ReadMemoryBarrier();
		used = (curr->s.tail_idx - rb->read_idx) & rb->big_mask;
		if (rb->buffer_size - used < size)
			return -1;

		next.s.tail_idx = curr->s.tail_idx + size;
		next.s.users = curr->s.users + 1;
		prev.u = AtomicCompareSwap64U(&rb->tail.u, curr->u, next.u);
		if (prev.u == curr->u)
			break;

		curr->u = prev.u;
	}

	off = curr->s.tail_idx & rb->small_mask;
	curr->u = next.u;

	return off;
}

static void lrb_commit_write(struct lrbuf *rb, union shadow_tail *curr)
{
	union shadow_tail next;
	union shadow_tail prev;

	while (1) {
		next.s.tail_idx = curr->s.tail_idx;
		next.s.users = curr->s.users - 1;
		if (next.s.users == 0)
			*(volatile UINT32 *)&rb->write_idx = curr->s.tail_idx;

		prev.u = AtomicCompareSwap64U(&rb->tail.u, curr->u, next.u);
		if (prev.u == curr->u)
			break;

		curr->u = prev.u;
	}
}

//
// one step write to buffer
//
UINT32 lrb_write(struct lrbuf *rb, const void *src, UINT32 size)
{
	INT32 off;
	union shadow_tail curr;
	UINT32 size1;

	off = lrb_write_reserv(rb, &curr, size);
	if (off < 0)
		return 0;

	size1 = size;
	if (off + size1 > rb->buffer_size) {
		size1 = rb->buffer_size - off;
		rbuf_memcpy(rbuf_data(rb), (UINT8 *)src + size1, size - size1);
	}
	rbuf_memcpy(rbuf_data(rb) + off, src, size1);

	lrb_commit_write(rb, &curr);
	return size;
}

//
// one step read from buffer
//
UINT32 lrb_read(struct lrbuf *rb, void *dst, UINT32 size)
{
	UINT32 offset = rb->read_idx & rb->small_mask;
	UINT32 used_size = lrb_used_size(rb);
	UINT32 size1;

	if (size > used_size)
		size = used_size;

	size1 = size;
	if (offset + size > rb->buffer_size) {
		size1 = rb->buffer_size - offset;
		rbuf_memcpy((UINT8 *)dst + size1, rbuf_data(rb), size - size1);
	}
	rbuf_memcpy((UINT8 *)dst, rbuf_data(rb) + offset, size1);

	return lrb_read_advance(rb, size);
}
