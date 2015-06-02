//////////////////////////////////////////////////////////////////////////
///
/// @file RingBuffer.h
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

#ifndef __LOG_RING_BUFFER_H__
#define __LOG_RING_BUFFER_H__

#include "Interfaces/ParallelsCompiler.h"

#include "Libraries/Std/AtomicOps.h"
#include "Libraries/Std/MemBarrier.h"

//
// Simple flexible lock less ring buffer logic (manager).
// Can be shared between monitor and VM app
// Read and write pointers can be used in different contextes with
// different pointer size (32 bit vs. 64 bit)
//
struct ring_buffer
{
	UINT32 buffer_size;
	UINT32 element_size;
	UINT32 read_index;
	UINT32 write_index;
	UINT32 small_mask;
	UINT32 big_mask;
};


#ifdef __cplusplus
extern "C" {
#endif

//
// initialize struct ring_buffer structure
//   buffer_size is adjusted to near highest 2^N (buffer size is 2^N elements,
//   but not greater than original size)
//   element_size is element sizeof (in bytes), for byte buffers can be 1 or 0
//
UINT32 rbuf_init(struct ring_buffer *rbuf, UINT32 buffer_size,
				 UINT32 element_size);

//
// return buffer used (available for read) size in elements
//
static __inline UINT32 rbuf_used_size(const struct ring_buffer *rbuf)
{
	ReadMemoryBarrier();
	return ((rbuf->write_index - rbuf->read_index) & rbuf->big_mask);
}

//
// return buffer free (available for write) size in elements
//
static __inline UINT32 rbuf_free_size(const struct ring_buffer *rbuf)
{
	// NOTE: memory barrier called in rbuf_used_size()
	return rbuf->buffer_size - rbuf_used_size(rbuf);
}

//
// return total buffer size in elements
//
static __inline UINT32 rbuf_total_size(const struct ring_buffer *rbuf)
{
	return rbuf->buffer_size;
}

//
// return element size in bytes
//
static __inline UINT32 rbuf_element_size(const struct ring_buffer *rbuf)
{
	return rbuf->element_size;
}

//
// discard (clear) buffer
// Must be called in reader or when reader is completely disabled
//
static __inline void rbuf_discard(struct ring_buffer *rbuf)
{
	WriteMemoryBarrier();
	rbuf->read_index = rbuf->write_index;
}


//
// advance (move, commit) write pointer, size in elements
// NOTE: for performance reasons no any checks for size here: i.e. check size
//   before this call by rbuf_used_size() for example
//
static __inline UINT32 rbuf_write_advance(struct ring_buffer *rbuf, UINT32 size)
{
	WriteMemoryBarrier();
	rbuf->write_index = (rbuf->write_index + size) & rbuf->big_mask;
	return size;
}

//
// advance (move, commit) read pointer, size in elements
// NOTE: for performance reasons no any checks for size here: i.e. check size
//   before this call by rbuf_free_size() for example
//
static __inline UINT32 rbuf_read_advance(struct ring_buffer *rbuf, UINT32 size)
{
	WriteMemoryBarrier();
	rbuf->read_index = (rbuf->read_index + size) & rbuf->big_mask;
	return size;
}

//
// get write region as memory pointers
// NOTE: all sizes are in elements
//
UINT32 rbuf_write_ptr(const struct ring_buffer *rbuf, UINT32 size,
					  void** p1, UINT32 *sz1, void** p2, UINT32 *sz2);

//
// get write region as memory pointers
// NOTE: all sizes are in elements
//
UINT32 rbuf_write_ptr_simple(const struct ring_buffer *rbuf,
							 UINT32 size, void** p1);

//
// get read region as memory pointers
// NOTE: all sizes are in elements
//
UINT32 rbuf_read_ptr(const struct ring_buffer *rbuf, UINT32 size,
					 void** p1, UINT32 *sz1, void** p2, UINT32 *sz2);

//
// get read region as memory pointers
// NOTE: all sizes are in elements
//
UINT32 rbuf_read_ptr_simple(const struct ring_buffer *rbuf,
							UINT32 size, void** p1);

//
// one step write to buffer
// NOTE: size is in elements
//
UINT32 rbuf_write(struct ring_buffer *rbuf, const void *src_data, UINT32 size);


//
// one step read from buffer
// NOTE: size is in elements
//
UINT32 rbuf_read(struct ring_buffer *rbuf, void *dst_data, UINT32 size);


//
// special variant for network code (iovec based API)
// include  <sys/socket.h> or <sys/uio.h> before
//
#ifdef _STRUCT_IOVEC

//
// return write region as iovec structure
//
UINT32 rbuf_write_iov(const struct ring_buffer *rbuf,
					  struct iovec iov[2], UINT32 *niov);

//
// return read region as iovec structure
//
UINT32 rbuf_read_iov(const struct ring_buffer *rbuf,
					 struct iovec iov[2], UINT32 *niov);

#endif // _STRUCT_IOVEC


//
// helpers to dump ring_buffer state to log
//
#define RBUF_FMT "W: [%u], R: [%u], U/F/S/E: %u/%u/%u/%u"
#define RBUF_FMT_ARGS(rbuf) \
		(unsigned)(rbuf)->write_index, (unsigned)(rbuf)->read_index,		\
		(unsigned)rbuf_used_size(rbuf), (unsigned)rbuf_free_size(rbuf),		\
		(unsigned)(rbuf)->buffer_size, (unsigned)(rbuf)->element_size



//
// Lockless ringbuffer with true lockless writer semantic.
// It allows arbitrary amount of writers in arbitrary amount of contextes
//

union shadow_tail {
	struct {
		UINT32 tail_idx;
		UINT32 users;
	} s;
	UINT64 u;
};

struct lrbuf {
	UINT32	read_idx;
	UINT32	write_idx;
	union	shadow_tail	tail;
	UINT32	small_mask;
	UINT32	big_mask;
	UINT32	buffer_size;
	UINT32  __pad;
};
BUILD_BUG_ON(offsetof(struct lrbuf, tail) & 7);


//
// initialize struct lrbuf structure
//   buffer_size is adjusted to near highest 2^N (buffer size is 2^N elements,
//   but not greater than original size)
//
UINT32 lrb_init(struct lrbuf *rb, UINT32 buffer_size);

//
// return buffer used (available for read) size in elements
//
static __inline UINT32 lrb_used_size(const struct lrbuf *rb)
{
	ReadMemoryBarrier();
	return (rb->write_idx - rb->read_idx) & rb->big_mask;
}

//
// return buffer free (available for write) size in elements
//
static __inline UINT32 lrb_free_size(const struct lrbuf *rb)
{
	ReadMemoryBarrier();
	return
		rb->buffer_size - ((rb->tail.s.tail_idx - rb->read_idx) & rb->big_mask);
}

//
// one step write to buffer
//
UINT32 lrb_write(struct lrbuf *rb, const void *src_data, UINT32 size);

//
// return single read region as
//
UINT32 lrb_read_ptr(const struct lrbuf *rb, UINT8 **ptr);

//
// one step read from buffer
//
UINT32 lrb_read(struct lrbuf *rb, void *dst, UINT32 size);

//
// advance (move, commit) read pointer
// NOTE: there are no checks for size for a sake of performance
//
static __inline UINT32 lrb_read_advance(struct lrbuf *rb, UINT32 size)
{
	WriteMemoryBarrier();
	rb->read_idx = (rb->read_idx + size) & rb->big_mask;
	return size;
}

#define LRBUF_FMT "W: [%u], R: [%u], U/F/S: %u/%u/%u"
#define LRBUF_FMT_ARGS(lrb) \
		(unsigned)(lrb)->write_idx, (unsigned)(lrb)->read_idx,				\
		(unsigned)lrb_used_size(lrb), (unsigned)lrb_free_size(lrb),			\
		(unsigned)(lrb)->buffer_size

#ifdef __cplusplus
}
#endif

//
// this is special type of buffer for monitor logger logic
//
typedef struct _MON_LOG_BUFFER {
	INT32				LogLevel;
	UINT				Overflow;
	INT32				sync;
	INT32				__pad;
	struct lrbuf		RBuf;
} MON_LOG_BUFFER;
BUILD_BUG_ON(offsetof(MON_LOG_BUFFER, RBuf) & 7);

#endif // __LOG_RING_BUFFER_H__
