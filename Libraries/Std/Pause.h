//////////////////////////////////////////////////////////////////////////
///
/// @file Pause.h
///
/// @brief CpuPause operation.
///
/// @author Parallels, Denis V. Lunev <den@parallels.com>
///
/// Copyright (c) 2006-2012 Parallels IP Holdings GmbH. All rights reserved.
///
/// This file is licensed for use in the Linux Kernel under the
/// terms of the GNU General Public License version 2.
///
//////////////////////////////////////////////////////////////////////////

#ifndef __PAUSE_H__
#define __PAUSE_H__

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER) && !defined(_KERNEL_)
#include <intrin.h>
#pragma intrinsic(_mm_pause)
#endif

static __always_inline void CpuPause(void)
{
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
	_mm_pause();
#elif defined(SUPPORT_ASM_MS)
	__asm pause
#else
	asm volatile("pause": : :"memory");
#endif
}

#endif
