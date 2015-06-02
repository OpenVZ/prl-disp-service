///////////////////////////////////////////////////////////////////////////////
///
/// @file BitOps.h
///
/// Some basic bit array operations
///
/// @author antonz@
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
///////////////////////////////////////////////////////////////////////////////
#ifndef _STD_BITOPS_H_
#define _STD_BITOPS_H_


#include <Interfaces/ParallelsTypes.h>
#include <Libraries/Std/AtomicOps.h>


// Get UINT-aligned size of a bitmap in bytes
#define BMAP_SZ(bits)		((((bits) + 31) >> 5) << 2)

// Get a bit of a bitmap
static __inline int BMAP_GET(void const* bmap, unsigned int bit)
{
	return !!(((unsigned int const*)bmap)[bit >> 5] & (1 << (bit & 31)));
}

// Set a bit of a bitmap
static __inline void BMAP_SET(void* bmap, unsigned int bit)
{
	((unsigned int*)bmap)[bit >> 5] |= (1 << (bit & 31));
}
static __inline int BMAP_ATOMIC_SET(void* bmap, unsigned int bit)
{
	return AtomicOr((int*)bmap + bit / 32, 1u << (bit & 31));
}
static __inline void BMAP_SET_BLOCK(void* bmap, unsigned int Start, unsigned int Size)
{
	unsigned int End;
	int Len, ByteSize;

	for (End = Start + Size; Start < End && Start & 0x7; Start++)
		BMAP_SET(bmap, Start);
	// now - Start is byte-aligned

	Len = End - Start;
	if (Len <= 0)
		return;

	ByteSize = Len & ~0x7;
	if (ByteSize)
	{
		// Avoid using stdlib functions for better compatibility
		unsigned char* p = (unsigned char*)bmap + Start / 8;
		for (Len = ByteSize / 8; Len != 0; Len--, p++)
			*p = 0xFF;
		Start += ByteSize;
	}

	for (; Start < End; Start++)
		BMAP_SET(bmap, Start);
}

// Clear a bit of a bitmap
static __inline void BMAP_CLR(void* bmap, unsigned int bit)
{
	((unsigned int*)bmap)[bit >> 5] &= ~(1 << (bit & 31));
}
static __inline int BMAP_ATOMIC_CLR(void* bmap, unsigned int bit)
{
	return AtomicAnd((int*)bmap + bit / 32, ~(1u << (bit & 31)));
}

static __inline unsigned char* BMAP_COUNT_TBL(void)
{
	static unsigned char t[0x100], *pt = 0;
	if (!pt)
	{
		int i;
		for (i = 0; i < 0x100; ++i)
		{
			unsigned char m, cnt = 0;
			for (m = 1; m; m <<= 1)
				if (m & i)
					++cnt;
			t[i] = cnt;
		}
		pt = t;
	}
	return pt;
}

// Count the number of set bits in a bitmap of the given size (in bytes)
static __inline SIZE_T BMAP_COUNT_IN_BYTES(void const* bmap, SIZE_T len)
{
	SIZE_T cnt = 0;
	unsigned char const* ptr = (unsigned char const*)bmap, *end = ptr + len;
	for (; ptr < end; ++ptr)
		cnt += BMAP_COUNT_TBL()[*ptr];
	return cnt;
}

// Count the number of set/cleared bits in a bitmap of the given size (in bits)
#define BMAP_COUNT(bmap, bits)	BMAP_COUNT_IN_BYTES(bmap, ((bits) + 7) >> 3)
static __inline SIZE_T BMAP_COUNT_ZERO(void const* bmap, unsigned int bits)
{
	return bits - BMAP_COUNT(bmap, bits);
}


#if defined(_MSC_VER)
// suppress "no return value"
// assemply inlines do return [edx:]eax per register convention
#pragma warning(push)
#pragma warning(disable:4035)

#if defined(_AMD64_) && !defined(__INTEL_COMPILER)
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_byteswap_ushort)
#pragma intrinsic(_byteswap_ulong)
#pragma intrinsic(_BitScanForward64)
#endif
#endif	// defined(_MSC_VER)

/**
 * @brief
 *	Find the least significant bit
 *
 * @param
 *	original unsigned int
 *
 * @return
 *	position of the least significant bit set,
 *	and -1 if all the bits are 0
 */
static __inline int BitFindLowestSet(unsigned int uVal)
{
#if defined (_MSC_VER)
#if defined(_AMD64_) && !defined(__INTEL_COMPILER)
	unsigned long r;
	return _BitScanForward(&r, uVal) ? r : -1;
#else
	__asm
	{
		bsf eax, uVal
		jnz _ret
		mov eax, -1
		_ret:
	}
#endif
#else
	int r;
	__asm__(
		"bsfl %1, %0\n\t"
		"jnz 1f\n\t"
		"movl $-1, %0\n"
		"1:"
		: "=r"(r)
		: "r"(uVal));
	return r;
#endif
}

/**
 * @brief
 *	Find the most significant bit
 *
 * @param
 *	original unsigned int
 *
 * @return
 *	position of the most significant bit set,
 *	and -1 if all bits are 0
 */
static __inline int BitFindHighestSet(unsigned int uVal)
{
#if defined (_MSC_VER)
#if defined(_AMD64_) && !defined(__INTEL_COMPILER)
	unsigned long r;
	return _BitScanReverse(&r, uVal) ? r : -1;
#else
	__asm
	{
		bsr eax, uVal
		jnz _ret
		mov eax, -1
		_ret:
	}
#endif
#else
	int r;
	__asm__(
		"bsrl %1, %0\n\t"
		"jnz 1f\n\t"
		"movl $-1, %0\n"
		"1:"
		: "=r"(r)
		: "r"(uVal));
	return r;
#endif
}

/**
 * @brief
 *	Find the least bit is set in uVal
 *
 * @param
 *
 *
 * @return
 *	position of the least significant bit set,
 *	and -1 if all the bits are 0
 */
static __inline int BitFindLowestSet64(UINT64 uVal)
{
#if defined(__GNUC__) || defined(__clang__)
	return __builtin_ffsll(uVal) - 1;
#else
	// not-optimized fallback variant
	// XXX: little-endian specific code
	UINT32* v = (UINT32*)&uVal;
	int     rv = BitFindLowestSet(v[0]);
	if (-1 != rv)
		return rv;
	rv = BitFindLowestSet(v[1]);
	if (-1 != rv)
		return (rv + 32);
	return -1;
#endif
}

/**
 * @brief
 *	Find the least bit is clear in uVal
 *
 * @param
 *
 *
 * @return
 *	position of the least significant bit clear,
 *	and -1 if all the bits are 1
 */
static __inline int BitFindLowestClear64(UINT64 uVal)
{
	return BitFindLowestSet64(~uVal);
}

#define BITS_PER_UINT64	(sizeof(UINT64) << 3)
#define BIT_MASK_UINT64	((UINT64)0x3F)
#define BIT2POS64(pos)	((pos) / BITS_PER_UINT64)
#define BIT_ALL_SET64	(~(UINT64)0)
#define BMAP_SZ64(bits)		((((bits) + 63) >> 6) << 3)

/**
 * @brief
 *	Find the least significant bit in a bitmap
 *
 * @param bmap	pointer to a bit array
 * @param size	size of a bitmap (in bits!)
 * @param off	position to start from (in bits!)
 *
 * @return
 *	position of the least significant bit set,
 *	and -1 if all bits are 0
 */
static __inline LONG64 BitFindNextSet64(UINT64 const* bmap,
										UINT32 size,
										UINT32 off)
{
	UINT64 const* p = bmap + BIT2POS64(off);
	UINT32 pos = off & ~BIT_MASK_UINT64;
	UINT64 val;

	if (NULL == bmap)
		return -2;
	if (off >= size)
		return -2;

	size -= pos;
	off &= BIT_MASK_UINT64;
	if (off != 0) {
		val = *(p++);
		val &= (BIT_ALL_SET64 << off);
		if (size < BITS_PER_UINT64)
			goto check_tail;
		if (val != 0)
			goto found;
		size -= BITS_PER_UINT64;
		pos += BITS_PER_UINT64;
	}

	while (size & ~BIT_MASK_UINT64) {
		val = *(p++);
		if (val != 0)
			goto found;
		pos += BITS_PER_UINT64;
		size -= BITS_PER_UINT64;
	}

	if (!size)
		return -1;
	val = *p;

check_tail:
	val &= BIT_ALL_SET64 >> (BITS_PER_UINT64 - size);
	if (val == 0)
		return -1;	// no bits set

found:
	// do not care about -1, at least one bit is always set here
	return pos + BitFindLowestSet64(val);
}

/**
 * @brief
 *	Find the least bit is clear in a bitmap
 *
 * @param bmap	pointer to a bit array
 * @param size	size of a bitmap (in bits!)
 * @param off	position to start from (in bits!)
 *
 * @return
 *	position of the least bit is clear,
 *	and -1 if all bits are 1
 */
static __inline LONG64 BitFindNextClear64(UINT64 const* bmap,
										  UINT32 size,
										  UINT32 off)
{
	UINT64 const* p = bmap + BIT2POS64(off);
	UINT32	pos = off & ~BIT_MASK_UINT64;
	UINT64	val;

	if (NULL == bmap)
		return -1;
	if (off >= size)
		return -1;

	size -= pos;
	off &= BIT_MASK_UINT64;
	if (off != 0) {
		val = *(p++);
		val |= BIT_ALL_SET64 >> (BITS_PER_UINT64 - off); // set all bits before desired
		if (size < BITS_PER_UINT64)
			goto check_tail;
		if (val != BIT_ALL_SET64)
			goto found;
		size -= BITS_PER_UINT64;
		pos += BITS_PER_UINT64;
	}

	while (size & ~BIT_MASK_UINT64) {
		val = *(p++);
		if (val != BIT_ALL_SET64)
			goto found;
		pos += BITS_PER_UINT64;
		size -= BITS_PER_UINT64;
	}

	if (!size)
		return -1;

	val = *p;

check_tail:
	val |= BIT_ALL_SET64 << size;
	if (BIT_ALL_SET64 == val)
		return -1;	// no bits cleared

found:
	return pos + BitFindLowestClear64(val);
}

/*
 * Swap bytes in the 16 bit value
 *
 * @param 16 bit value to swap
 *
 * @return Swapped 16 bit value
 */
#undef bswap_16
static __inline UINT16 bswap_16(UINT16 x) {
#if defined(_MSC_VER)
#if defined(_AMD64_) && !defined(__INTEL_COMPILER)
	return _byteswap_ushort(x);
#else		/* defined(_AMD64_) && !defined(__INTEL_COMPILER) */
	UINT32 u = x;
	__asm {
		movzx eax, word ptr u
		xchg ah, al
	}
	// return eax per register convention
#endif		/* defined(_AMD64_) && !defined(__INTEL_COMPILER) */
#else		/* defined(_MSC_VER) */
	UINT32 r = x;
	__asm__ __volatile__ (
		"xchg %%ah, %%al\n\t"
		: "=a"(r) : "0"(r));
	return r;
#endif		/* defined(_MSC_VER) */
}

/*
 * Swap bytes in the 32 bit value
 *
 * @param 32 bit value to swap
 *
 * @return Swapped 32 bit value
 */
#undef bswap_32
static __inline UINT32 bswap_32(UINT32 u) {
#if defined(_MSC_VER)
#if defined(_AMD64_) && !defined(__INTEL_COMPILER)
	return _byteswap_ulong((unsigned long)u);
#else		/* defined(_AMD64_) && !defined(__INTEL_COMPILER) */
	__asm {
		mov eax, u
		bswap eax
	}
	// return eax per register convention
#endif		/* defined(_AMD64_) && !defined(__INTEL_COMPILER) */
#else		/* defined(_MSC_VER) */
	__asm__ __volatile__ (
		"bswap %%eax\n\t"
		: "=a"(u) : "0"(u));
	return u;
#endif		/* defined(_MSC_VER) */
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif


/*
 * Swap bytes in the 64 bit value
 *
 * @param 64 bit value to swap
 *
 * @return Swapped 64 bit value
 */
#undef bswap_64
static __inline UINT64 bswap_64(UINT64 x) {
	return (((UINT64)bswap_32((UINT32)(x&0xffffffffull)))<<32) |
			(bswap_32((UINT32)(x>>32)));
}

#endif
