//////////////////////////////////////////////////////////////////////////
///
/// @file SpinLock.h
///
/// @brief spinlocks. Synchornization objects and functions working with it.
///
/// @author Parallels, Korotaev Kirill <dev@parallels.com>
///
/// A common way to use SPINLOCK interface would be:
///
/// 1. Declare SPINLOCK variable or allocate some memory for it:
///		SPINLOCK lock;
///
/// 2. Initialize SPINLOCK variable:
///		SpinLockInit(&lock);
///
///	3. Lock/Unlock spinlock in runtime:
///
///		...
///		SpinLockLock(&lock);
///		/* critical section */
///		SpinLockUnlock(&lock);
///		...
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

#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include "Interfaces/ParallelsTypes.h"
#include "Libraries/Std/Pause.h"

#define CONFIG_BLOCKING_SPINLOCK
//#define DEBUG_SPINLOCK

// A kind of light DEBUG_SPINLOCK
#define CONFIG_GLOBALLIST_SPINLOCK

#if defined(DEBUG_SPINLOCK)
// DEBUG_SPINLOCK assumes defined CONFIG_GLOBALLIST_SPINLOCK also
#define CONFIG_GLOBALLIST_SPINLOCK
#endif

/* spinlock object */
struct _spinlock {
	/*
	 * 1. MUST be int for compatibility of 32/64bit Hpv/Mon?
	 * 2. Be VERY carefull with pointers inside, since 32bit VM app
	 *    client won't work with 64bit monitor then. Sigh...
	 *    Why the hell app has to do smth with these monitor structures at all? :/
	 */
	volatile signed int val;
	volatile unsigned int owner;	/* used by mutex version of spinlock */
	/*
	 * The pointer below is used for locks profiling/deadlock detection.
	 * It should be always compiled in to provide compatibility between
	 * both normal and assert monitor version and hypervisor/user space.
	 */
	void *debug_info;
#ifdef _32BIT_
	/*
	 * in 32bit mode the pointer above is 32bit, so pad to have the same
	 * struct size on x32 and x86_64
	*/
	void *pad1;
#endif
};

typedef struct _spinlock SPINLOCK;

/* 1 - unlocked, <= 0 - locked */
#define SPINLOCK_UNLOCKED	1
#define SPINLOCK_LOCKED		0

#define SPINLOCK_NO_OWNER	0xFFFF

#define SPINLOCK_INITIALIZER {SPINLOCK_UNLOCKED, SPINLOCK_NO_OWNER}

#if defined(_MSC_VER) && defined(_AMD64_) && !defined(__INTEL_COMPILER)
#include <intrin.h>
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_ReadWriteBarrier)
#endif


/**
 * Setup initial SPINLOCK value.
 * It is required to init lock through this function call.
 * Manual initialization should not be done, since in debug mode
 * can be complicated procedure.
 *
 * @see SpinLock
 * @see SpinUnlock
 *
 * @param spin lock pointer
 */
static __always_inline void RawSpinLockInit(SPINLOCK* lock)
{
	lock->debug_info = NULL;
#ifdef _32BIT_
	/* if you don't know why it's zeroed, meditate over spinlock structure and 32/64 compats */
	lock->pad1 = 0;
#endif
	lock->val = SPINLOCK_UNLOCKED;
	lock->owner = SPINLOCK_NO_OWNER;
}


/**
 * Try to lock SPINLOCK synchronization object.
 *
 * Plz avoid calling this function in a loop like this:
 * while (!SpinLockTryLock(&lock)) {wait_a_bit();}
 *
 * Always think why you may need this function at all: if it's a way
 * to avoid deadlocks - then your code is braindamaged and better
 * ask someone clever how to handle it in the correct way.
 *
 * @see SpinLockLock
 * @see SpinLockUnlock
 *
 * @param spin lock pointer
 * @return TRUE if lock is taken, false otherwise
 */
static __always_inline int RawSpinLockTryLock(SPINLOCK* slock)
{
	signed int lockval;

#ifdef SUPPORT_ASM_MS

#if defined (_X86_)
	__asm
	{
		mov ebx, slock
		xor eax,eax
		xchg dword ptr [ebx]SPINLOCK.val, eax
		mov dword ptr lockval, eax
	}
#elif defined(_AMD64_)
#if defined(__INTEL_COMPILER)
	__asm
	{
		mov rbx, slock
		xor eax,eax
		xchg dword ptr [rbx]SPINLOCK.val, eax
		mov dword ptr lockval, eax
	}
#else
	lockval = _InterlockedExchange((volatile long *)&(slock->val), SPINLOCK_LOCKED);
#endif
#endif

#else
	 asm volatile("xchgl %0,%1"
			 :"=q" (lockval), "=m" (slock->val)
			 :"0" (0) : "memory");
#endif

	return lockval > 0;
}


/**
 * Lock SPINLOCK synchronization object.
 *
 * @see RawSpinUnlock
 *
 * @param spin lock pointer
 */
static __always_inline void RawSpinLock(SPINLOCK* slock)
{
#ifdef SUPPORT_ASM_MS
#if defined (_X86_)
	__asm {
				xor ebx,ebx
				mov eax, slock
		a:		lock dec dword ptr [eax]SPINLOCK.val
				jns c
		b:		pause
				cmp [eax]SPINLOCK.val, ebx
				jg a
				jmp b
		c:
	}
#elif defined(_AMD64_)
#if defined(__INTEL_COMPILER)
	__asm {
				xor ebx,ebx
				mov rax, slock
		a:		lock dec dword ptr [rax]SPINLOCK.val
				jns c
		b:		pause
				cmp dword ptr [rax]SPINLOCK.val, ebx
				jg a
				jmp b
		c:
	}
#else
	while (_InterlockedDecrement((volatile long *)&(slock->val)) < 0) {
		while (slock->val <= 0)
			_mm_pause();
	}
#endif
#endif
#else
	asm volatile(
			"1:\n"
			"lock; decl %0\n"
			"jns 3f\n"
			"2:\n"
			"pause\n"
			"cmpl $0,%0\n"
			"jg 1b\n"
			"jmp 2b\n"
			"3:\n"
				: "=m" (slock->val)
				:
				: "memory");
#endif
}


/**
 * Unlock SPINLOCK synchronization object.
 *
 * @see RawSpinLock
 *
 * @param spin lock pointer
 */
static __always_inline void RawSpinUnlock(SPINLOCK* slock)
{
#ifdef SUPPORT_ASM_MS

#if defined(_X86_)
	__asm
	{
		mov edx, slock
		mov dword ptr [edx]SPINLOCK.val, SPINLOCK_UNLOCKED
	}

#elif defined(_AMD64_)
#if defined(__INTEL_COMPILER)
	__asm
	{
		mov rdx, slock
		mov dword ptr [rdx]SPINLOCK.val, SPINLOCK_UNLOCKED
	}
#else
	_ReadWriteBarrier();
	slock->val = SPINLOCK_UNLOCKED;
	_ReadWriteBarrier();
#endif
#endif

#else
	asm volatile("movl $1,%0"
		: "=m" (slock->val)
		:
		: "memory");
#endif

}

/**
 * Check whether SPINLOCK is locked
 * @param spin lock pointer
 */
static __always_inline int SpinIsLocked(SPINLOCK *slock)
{
	return slock->val <= 0;
}

/*
 * here goes macros mapping for 3 cases: debug, SMP, UP
 */
#define __STRINGIFY(x)			#x
#define STRINGIFY(x)			__STRINGIFY(x)
#define CODE_LINE               __FILE__ ":" STRINGIFY(__LINE__)
#define LOCK_ID(lock)			STRINGIFY(lock) " @ " CODE_LINE


#ifdef CONFIG_BLOCKING_SPINLOCK
#include "Libraries/Std/Mutex.h"
#define SpinLockInit(x)			RawSpinLockInit(x)
#define SpinLockLock(x)			RawMutexLock(x)
#define SpinLockIrqDisable(x,f)	RawMutexLockIrqDisable(x, f)
#define SpinLockUnlock(x)		RawMutexUnlock(x)
#define SpinLockTryLock(x)		RawMutexTryLock(x)
#define SpinUnlockIrqRestore(x,f) RawMutexUnlockIrqRestore(x, f)
#else /* CONFIG_BLOCKING_SPINLOCK */
#define SpinLockInit(x)			RawSpinLockInit(x)
#define SpinLockLock(x)			RawSpinLock(x)
#define SpinLockUnlock(x)		RawSpinUnlock(x)
#define SpinLockTryLock(x)		RawSpinLockTryLock(x)
#define SpinLockIrqDisable(x,f)	RawSpinLockIrqDisable(x, f)
#define SpinUnlockIrqRestore(x,f) RawSpinUnlockIrqRestore(x, f)
#endif

#define SpinLockInitGlobalList()
#define SpinLockCheckGlobalList(x,y)

extern unsigned num_vcpus;

#  define SmpSpinLock(slock)		\
	do {							\
		if (num_vcpus > 1)			\
			SpinLockLock(slock);	\
	} while (0)

#  define SmpSpinUnlock(slock)		\
	do {							\
		if (num_vcpus > 1) 			\
			SpinLockUnlock(slock);	\
	} while (0)

#  define SmpSpinIsLocked(slock) (num_vcpus > 1 ? SpinIsLocked(slock) : 1)


static __always_inline int SmpSpinTryLock(SPINLOCK *slock)
{
	if (num_vcpus > 1)
	{
		if (SpinIsLocked(slock))
			return 0;

		return SpinLockTryLock(slock);
	}
	else
		return 1;
}

// TODO: Fill function content. It should return !0 if any spinlock currently locked
// by specified VCPU.
static __inline UINT IsVcpuInLockedContext(UINT uVcpuNumber)
{
	(void)uVcpuNumber;
	return 0;
}


#endif // __SPINLOCK_H__
