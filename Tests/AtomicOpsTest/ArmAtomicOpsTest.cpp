///////////////////////////////////////////////////////////////////////////////
///
/// @file AtomicOpsTest.cpp
///
/// Tests for atomic operations. Unfortunately couldn't find another way how to
/// test atomic ops on ARM. In order to add unit tests to project just uncomment
/// correspond lines at PrlCommonUtils.pro
///
/// @author sandro@
///
/// Copyright (c) 1999-2015 Parallels IP Holdings GmbH
///
/// This file is part of Virtuozzo Core. Virtuozzo Core is free
/// software; you can redistribute it and/or modify it under the terms
/// of the GNU General Public License as published by the Free Software
/// Foundation; either version 2 of the License, or (at your option) any
/// later version.
/// 
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
/// 
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
/// 02110-1301, USA.
///
/// Our contact details: Parallels IP Holdings GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////////

#include "Tests/AtomicOpsTest/ArmAtomicOpsTest.h"
#include "Monitor/Interfaces/AtomicOps.h"

#if defined(_ARM_)
	#if _ARMV_ == 7
	#define PrlMethodPrefix QString("armv7 %1: ").arg(__FUNCTION__)
	#else
	#error Unknown ARM version
	#endif
#elif defined(_AMD64_)
	#define PrlMethodPrefix QString("amd64 %1: ").arg(__FUNCTION__)
#elif defined(_X86_)
	#define PrlMethodPrefix QString("i386 %1: ").arg(__FUNCTION__)
#else
	#error Unknown platform
#endif

QString AtomicOpsTest::testAtomicRead64()
{
    atomicInt64 value = 65363546356766565LL;
	atomicInt64 readValue = AtomicRead64(&value);
	return PrlMethodPrefix + QString("value=%1 readValue=%2").arg(value).arg(readValue);
}

QString AtomicOpsTest::testAtomicWrite64()
{
    atomicInt64 value = 65363546356766565LL, target = 0LL;
	AtomicWrite64(&target, value);
	return PrlMethodPrefix + QString("value=%1 target=%2").arg(value).arg(target);
}

QString AtomicOpsTest::testAtomicAdd64()
{
    atomicInt64 value = 10LL, target = 20LL, resultValue = 0LL;
	resultValue = AtomicAdd64(&target, value);
	return PrlMethodPrefix + QString("value=%1 target=%2 resultValue=%3").arg(value).arg(target).arg(resultValue);
}

QString AtomicOpsTest::testAtomicAdd64HighPart()
{
    atomicInt64 value = 10000000000LL, target = 10000000000LL, resultValue = 0LL;
	resultValue = AtomicAdd64(&target, value);
	return PrlMethodPrefix + QString("value=%1 target=%2 resultValue=%3").arg(value).arg(target).arg(resultValue);
}

QString AtomicOpsTest::testAtomicAdd64Overflow()
{
    atomicInt64 value = 10LL, target = 0xFFFFFFFFFFFFFFFFLL, resultValue = 0LL;
	resultValue = AtomicAdd64(&target, value);
	return PrlMethodPrefix + QString("value=%1 target=%2 resultValue=%3").arg(value).arg(target).arg(resultValue);
}

QString AtomicOpsTest::testAtomicAdd64Overflow2()
{
    atomicInt64 value = 10LL, target = 0xFFFFFFFFLL, resultValue = 0LL;
	resultValue = AtomicAdd64(&target, value);
	return PrlMethodPrefix + QString("value=%1 target=%2 resultValue=%3").arg(value).arg(target).arg(resultValue);
}

QString AtomicOpsTest::testAtomicAdd64Overflow3()
{
    atomicInt64 value = 0x1000000000LL, target = 0xFFFFFFFF00000000LL, resultValue = 0LL;
	resultValue = AtomicAdd64(&target, value);
	return PrlMethodPrefix + QString("value=%1 target=%2 resultValue=%3").arg(value).arg(target).arg(resultValue);
}

QString AtomicOpsTest::testAtomicInc64()
{
    atomicInt64 target = 20LL, resultValue = 0LL;
	resultValue = AtomicInc64(&target);
	return PrlMethodPrefix + QString("target=%1 resultValue=%2").arg(target).arg(resultValue);
}

QString AtomicOpsTest::testAtomicSwap64()
{
	atomicInt64 target = 0x1234567890ABCDEFLL, swap = 0xFEDCBA0987654321LL, resultValue = 0LL;
	resultValue = AtomicSwap64(&target, swap);
	return PrlMethodPrefix + QString("target=%1 resultValue=%2 swap=%3").arg(target).arg(resultValue).arg(swap);
}

QString AtomicOpsTest::testAtomicCompareSwap64()
{
    atomicInt64 target = 20LL, compare = 20LL, swap = 30LL, resultValue = 0LL;
	resultValue = AtomicCompareSwap64(&target, compare, swap);
	return PrlMethodPrefix + QString("target=%1 resultValue=%2 compare=%3 swap=%4").arg(target).arg(resultValue).arg(compare).arg(swap);
}

QString AtomicOpsTest::testAtomicCompareSwap64NonEqual()
{
    atomicInt64 target = 20LL, compare = 25LL, swap = 30LL, resultValue = 0LL;
	resultValue = AtomicCompareSwap64(&target, compare, swap);
	return PrlMethodPrefix + QString("target=%1 resultValue=%2 compare=%3 swap=%4").arg(target).arg(resultValue).arg(compare).arg(swap);
}

