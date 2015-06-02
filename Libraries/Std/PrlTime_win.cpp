/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (C) 2007-2015 Parallels IP Holdings GmbH
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
/// @file
///		PrlTime_win.h
///
/// @author
///		kozerkov@
///
/// @brief
///		Timing functions (Windows implementation)
///
/////////////////////////////////////////////////////////////////////////////

#include "PrlTime.h"
#include "Interfaces/ParallelsTypes.h"
#include "Libraries/Logging/Logging.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <limits.h>
#include <intrin.h>
#pragma intrinsic(__rdtsc)

// this macro used to enable/disable some tests in init_PrlTicks()
// by default DO_ACCURACY_TEST is disabled because this test can take long
// time (~1 s) and this can confuse callers
// #define DO_MONOTONIC_TEST
// #define DO_ACCURACY_TEST

// run once function group for lazy initialization
static PRL_UINT64 init_PrlTicks();
static PRL_UINT64 init_PrlTicksDelta(PRL_UINT64 start, PRL_UINT64 *new_start);
static PRL_UINT64 init_PrlTicksToMicro(PRL_UINT64 tick);
static PRL_UINT64 init_PrlTicksToMilli(PRL_UINT64 tick);
static PRL_UINT64 init_PrlTicksToSeconds(PRL_UINT64 tick);
static PRL_UINT64 init_PrlTicksFrequency();

// QueryPerformanceCounter (QPC) function group
static PRL_UINT64 qpc_PrlTicks();
static PRL_UINT64 qpc_PrlTicksDelta(PRL_UINT64 start, PRL_UINT64 *new_start);

// RDTSC function group
static PRL_UINT64 rdtsc_PrlTicks();
static PRL_UINT64 rdtsc_PrlTicksDelta(PRL_UINT64 start, PRL_UINT64 *new_start);

// scale functions for various timer frequency ( >10MHz, ..., <10KHz)
static PRL_UINT64 fast_PrlTicksToMicro(PRL_UINT64 tick);
static PRL_UINT64 normal_PrlTicksToMicro(PRL_UINT64 tick);
static PRL_UINT64 slow_PrlTicksToMicro(PRL_UINT64 tick);

// scale functions for various timer frequency
static PRL_UINT64 normal_PrlTicksToMilli(PRL_UINT64 tick);
static PRL_UINT64 slow_PrlTicksToMilli(PRL_UINT64 tick);

// real (runtime) functions
static PRL_UINT64 real_PrlTicksToSeconds(PRL_UINT64 tick);
static PRL_UINT64 real_PrlTicksFrequency();

// simplified variant of HostUtils::GetCPUMhz(), placed here to avoid
// project libraries cross dependence
static PRL_UINT64 get_cpu_frequency();

// this constant used as fallback cpu frequency value in get_cpu_frequency()
#define FALLBACK_CPU_FREQUENCY 1000000000i64

static PRL_UINT64 (*s_PrlTicks)() = init_PrlTicks;
static PRL_UINT64 (*s_PrlTicksDelta)(PRL_UINT64, PRL_UINT64*) = init_PrlTicksDelta;
static PRL_UINT64 (*s_PrlTicksToMicro)(PRL_UINT64) = init_PrlTicksToMicro;
static PRL_UINT64 (*s_PrlTicksToMilli)(PRL_UINT64) = init_PrlTicksToMilli;
static PRL_UINT64 (*s_PrlTicksToSeconds)(PRL_UINT64) = init_PrlTicksToSeconds;
static PRL_UINT64 (*s_PrlTicksFrequency)() = init_PrlTicksFrequency;

static PRL_UINT64 s_freq = 1;
static PRL_UINT64 s_freq_mhz = 1;
static PRL_UINT64 s_freq_khz = 1;

PRL_UINT64 PrlTicks()
{
	return s_PrlTicks();
}

PRL_UINT64 PrlTicksDelta(PRL_UINT64 start, PRL_UINT64 *new_start)
{
	return s_PrlTicksDelta(start, new_start);
}

PRL_UINT64 PrlTicksToMicro(PRL_UINT64 tick)
{
	return s_PrlTicksToMicro(tick);
}

PRL_UINT64 PrlTicksToMilli(PRL_UINT64 tick)
{
	return s_PrlTicksToMilli(tick);
}

PRL_UINT64 PrlTicksToSeconds(PRL_UINT64 tick)
{
	return s_PrlTicksToSeconds(tick);
}

PRL_UINT64 PrlTicksFrequency()
{
	return s_PrlTicksFrequency();
}

static PRL_UINT64 init_PrlTicks()
{
	PRL_UINT64 tick;
	PRL_UINT64 freq;

	if (QueryPerformanceCounter((LARGE_INTEGER*)&tick) &&
		QueryPerformanceFrequency((LARGE_INTEGER*)&freq)) {
		s_freq = freq;
		s_PrlTicks = qpc_PrlTicks;
		s_PrlTicksDelta = qpc_PrlTicksDelta;

		WRITE_TRACE(DBG_FATAL,
			"[PrlTicks] Init (method: QPC, frequency: %llu)", s_freq);
	} else {
		tick = rdtsc_PrlTicks();
		s_freq = get_cpu_frequency();
		s_PrlTicks = rdtsc_PrlTicks;
		s_PrlTicksDelta = rdtsc_PrlTicksDelta;

		WRITE_TRACE(DBG_FATAL,
			"[PrlTicks] Init (method: RDTSC, frequency: %llu)", s_freq);
	}

	s_freq_mhz = s_freq / 1000000;
	s_freq_khz = s_freq / 1000;

	// select better scaling functions (precision optimized)
	if (s_freq_mhz > 10) {
		s_PrlTicksToMicro = fast_PrlTicksToMicro;
		s_PrlTicksToMilli = normal_PrlTicksToMilli;
	} else if (s_freq_khz > 10) {
		s_PrlTicksToMicro = normal_PrlTicksToMicro;
		s_PrlTicksToMilli = normal_PrlTicksToMilli;
	} else {
		WRITE_TRACE(DBG_FATAL, "[PrlTicks] Slow timer");
		s_PrlTicksToMicro = slow_PrlTicksToMicro;
		s_PrlTicksToMicro = slow_PrlTicksToMilli;
	}

	s_PrlTicksToSeconds = real_PrlTicksToSeconds;
	s_PrlTicksFrequency = real_PrlTicksFrequency;

#ifdef DO_MONOTONIC_TEST
	// small monotonic timer test (SMP aware)
	// NOTE: do not use PrlTicksDelta below because it have timer overlap support
	DWORD_PTR	affinity = SetThreadAffinityMask(GetCurrentThread(), 1);
	PRL_UINT64	start = s_PrlTicks();
	int			fails = 0;
	for (int i = 0; i < 320; i++) {
		SetThreadAffinityMask(GetCurrentThread(), 1 << (i % 32));
		Sleep(1); // force reschedule
		PRL_UINT64 next = s_PrlTicks();
		if (next < start)
			fails++;
		start = next;
	}
	SetThreadAffinityMask(GetCurrentThread(), affinity);
	// skip one failure (allow one timer overlap)
	if (fails > 1)
		WRITE_TRACE(DBG_FATAL,
			"[PrlTicks] Monotonic test failed (fails: %u)", fails);
#endif

#ifdef DO_ACCURACY_TEST
	// small timer accuracy test
	start = s_PrlTicks();
	Sleep(1000);
	PRL_UINT64 delay = s_PrlTicksToMicro(s_PrlTicksDelta(start, NULL));
	// test failed if more than 10 ms error present
	if ((delay / 10000) != 100) {
		WRITE_TRACE(DBG_FATAL,
			"[PrlTicks] Accuracy test failed (%lld us)", 1000000 - delay);
	}
#endif

	return tick;
}

static PRL_UINT64 init_PrlTicksDelta(PRL_UINT64 start, PRL_UINT64 *new_start)
{
	init_PrlTicks();
	return s_PrlTicksDelta(start, new_start);
}

static PRL_UINT64 init_PrlTicksToMicro(PRL_UINT64 tick)
{
	init_PrlTicks();
	return s_PrlTicksToMicro(tick);
}

static PRL_UINT64 init_PrlTicksToMilli(PRL_UINT64 tick)
{
	init_PrlTicks();
	return s_PrlTicksToMilli(tick);
}

static PRL_UINT64 init_PrlTicksToSeconds(PRL_UINT64 tick)
{
	init_PrlTicks();
	return s_PrlTicksToSeconds(tick);
}

static PRL_UINT64 init_PrlTicksFrequency()
{
	init_PrlTicks();
	return s_PrlTicksFrequency();
}

static PRL_UINT64 qpc_PrlTicks()
{
	PRL_UINT64 tick;
	QueryPerformanceCounter((LARGE_INTEGER*)&tick);
	return tick;
}

static PRL_UINT64 qpc_PrlTicksDelta(PRL_UINT64 start, PRL_UINT64 *new_start)
{
	PRL_UINT64 tick;
	if (!new_start)
		new_start = &tick;
	QueryPerformanceCounter((LARGE_INTEGER*)new_start);

	if (*new_start >= start)
		return *new_start - start;

	return _UI64_MAX - (*new_start) + start + 1;
}

static PRL_UINT64 rdtsc_PrlTicks()
{
	return __rdtsc();
}

static PRL_UINT64 rdtsc_PrlTicksDelta(PRL_UINT64 start, PRL_UINT64 *new_start)
{
	PRL_UINT64 tick;
	if (!new_start)
		new_start = &tick;

	*new_start = __rdtsc();

	if (*new_start >= start)
		return *new_start - start;

	return _UI64_MAX - (*new_start) + start + 1;
}

static PRL_UINT64 real_PrlTicksToSeconds(PRL_UINT64 tick)
{
	return tick / s_freq;
}

static PRL_UINT64 real_PrlTicksFrequency()
{
	return s_freq;
}

static PRL_UINT64 fast_PrlTicksToMicro(PRL_UINT64 tick)
{
	return tick / s_freq_mhz;
}

static PRL_UINT64 normal_PrlTicksToMicro(PRL_UINT64 tick)
{
	if (likely(tick < (_UI64_MAX / 1000)))
		return (tick * 1000) / s_freq_khz;

	// we rarely follow here, possibly wrong usage
	WRITE_TRACE(DBG_FATAL, "[PrlTicks] Too large delta value: %llu", tick);
	return (tick / s_freq_khz) * 1000;
}

static PRL_UINT64 slow_PrlTicksToMicro(PRL_UINT64 tick)
{
	if (likely(tick < (_UI64_MAX / 1000000)))
		return (tick * 1000000) / s_freq;

	// we rarely follow here, possibly wrong usage
	WRITE_TRACE(DBG_FATAL, "[PrlTicks] Too large delta value: %llu", tick);
	return (tick / s_freq) * 1000000;
}

static PRL_UINT64 normal_PrlTicksToMilli(PRL_UINT64 tick)
{
	return tick / s_freq_khz;
}

static PRL_UINT64 slow_PrlTicksToMilli(PRL_UINT64 tick)
{
	if (likely(tick < (_UI64_MAX / 1000)))
		return (tick * 1000) / s_freq;

	// we rarely follow here, possibly wrong usage
	WRITE_TRACE(DBG_FATAL, "[PrlTicks] Too large delta value: %llu", tick);
	return (tick / s_freq) * 1000;
}

static PRL_UINT64 get_cpu_frequency()
{
	HKEY	hk;

	LONG rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0,
		KEY_QUERY_VALUE, &hk);

	if (rc != ERROR_SUCCESS) {
		WRITE_TRACE(DBG_FATAL,
			"[PrlTicks] Get CPU frequency failed (RegOpenKeyEx(): 0x%lx", rc);
		return FALLBACK_CPU_FREQUENCY;
	}

	DWORD val;
	DWORD sz = sizeof(val);
	rc = RegQueryValueEx(hk, L"~MHz", NULL, NULL, (LPBYTE)&val, &sz);
	if (rc != ERROR_SUCCESS) {
		WRITE_TRACE(DBG_FATAL,
			"[PrlTicks] Get CPU frequency failed (RegQueryValueEx(): 0x%lx", rc);
		val = FALLBACK_CPU_FREQUENCY / 1000000;
	}

	RegCloseKey(hk);
	return (PRL_UINT64)val * 1000000;
}
