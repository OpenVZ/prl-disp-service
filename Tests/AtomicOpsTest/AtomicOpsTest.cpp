/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (C) 2008-2015 Parallels IP Holdings GmbH
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
/// @file AtomicOpsTest.cpp
///
/// @author maximk
///
/// @brief Tests for the AtomicOps functionality
///
/////////////////////////////////////////////////////////////////////////////

#include "AtomicOpsTest.h"
#include "Tests/CommonTestsUtils.h"
#include "Libraries/Std/AtomicOps.h"


CAtomicOpsTest::CAtomicOpsTest()
{
}

CAtomicOpsTest::~CAtomicOpsTest()
{
}

void CAtomicOpsTest::init()
{
}

void CAtomicOpsTest::cleanup()
{
}


void CAtomicOpsTest::test8()
{
    /**
     * 8-bit atomic operations
     */
    UINT8 u8_1 = 0, u8_2 = 0;

    u8_1 = 0x1c;
    u8_2 = AtomicOr8U( &u8_1, 0x01);
    QVERIFY( u8_1 == 0x1d );
    QVERIFY( u8_2 == 0x1c );

    u8_1 = 0x11;
    u8_2 = AtomicOr8U( &u8_1, 0x2e);
    QVERIFY( u8_1 == 0x3f );
    QVERIFY( u8_2 == 0x11 );

    u8_1 = 0xa8;
    u8_2 = AtomicOr8U( &u8_1, 0x3e);
    QVERIFY( u8_1 == 0xbe );
    QVERIFY( u8_2 == 0xa8 );

    u8_1 = 0xa8;
    u8_2 = AtomicAnd8U( &u8_1, 0xff);
    QVERIFY( u8_1 == 0xa8 );
    QVERIFY( u8_2 == 0xa8 );

    u8_1 = 0xa8;
    u8_2 = AtomicAnd8U( &u8_1, 0x0);
    QVERIFY( u8_1 == 0x0 );
    QVERIFY( u8_2 == 0xa8 );
}


void CAtomicOpsTest::test32()
{
    /**
     * 32-bit atomic operations
     */
    UINT32 u32_1 = 0, u32_2 = 0;

    /////////////////////////////////////////////////////////////////////////////////
    // AtomicInc32
    u32_1 = 0;
    u32_2 = AtomicInc( (int*)&u32_1 );
    QVERIFY( u32_1 == 1 );
    QVERIFY( u32_2 == 0 );

    u32_1 = (UINT32)-1;
    u32_2 = AtomicInc( (int*) &u32_1 );
    QVERIFY( u32_1 == 0 );
    QVERIFY( u32_2 == (UINT32)-1 );

    /////////////////////////////////////////////////////////////////////////////////
    // AtomicDec32
    u32_1 = 0;
    u32_2 = AtomicDec( (int*)&u32_1 );
    QVERIFY( u32_1 == (UINT32)-1 );
    QVERIFY( u32_2 == 0 );

    u32_1 = (UINT32)-1;
    u32_2 = AtomicDec( (int*) &u32_1 );
    QVERIFY( u32_1 == (UINT32)(-2) );
    QVERIFY( u32_2 == (UINT32)-1 );

    /////////////////////////////////////////////////////////////////////////////////
    // AtomicAdd32
    u32_1 = 0;
    u32_2 = AtomicAdd( (int*)&u32_1, (UINT32)-1 );
    QVERIFY( u32_1 == (UINT32)-1 );
    QVERIFY( u32_2 == 0 );

    u32_1 = (UINT32)-1;
    u32_2 = AtomicAdd( (int*) &u32_1, 1 );
    QVERIFY( u32_1 == 0 );
    QVERIFY( u32_2 == (UINT32)-1 );

    /////////////////////////////////////////////////////////////////////////////////
    // AtomicSwap32
    u32_1 = 123;
    u32_2 = AtomicSwap( (int*)&u32_1, 456 );
    QVERIFY( u32_1 == 456 );
    QVERIFY( u32_2 == 123 );

    /////////////////////////////////////////////////////////////////////////////////
    // AtomicIncAndTest32
    u32_1 = 0;
    u32_2 = AtomicIncAndTest( (int*)&u32_1 );
    QVERIFY( u32_1 == 1 );
    QVERIFY( u32_2 == 0 );

    u32_1 = (UINT32)-1;
    u32_2 = AtomicIncAndTest( (int*) &u32_1 );
    QVERIFY( u32_1 == 0 );
    QVERIFY( u32_2 != 0 );

    /////////////////////////////////////////////////////////////////////////////////
    // AtomicDecAndTest32
    u32_1 = 0;
    u32_2 = AtomicDecAndTest( (int*)&u32_1 );
    QVERIFY( u32_1 == (UINT32)-1 );
    QVERIFY( u32_2 == 0 );

    u32_1 = (UINT32)-1;
    u32_2 = AtomicDecAndTest( (int*) &u32_1 );
    QVERIFY( u32_1 == (UINT32)-2 );
    QVERIFY( u32_2 == 0 );

    u32_1 = (UINT32)1;
    u32_2 = AtomicDecAndTest( (int*) &u32_1 );
    QVERIFY( u32_1 == 0 );
    QVERIFY( u32_2 != 0 );

    /////////////////////////////////////////////////////////////////////////////////
    // AtomicRead32
    u32_1 = 0xDEADBABA;
    u32_2 = AtomicRead( (int*)&u32_1 );
    QVERIFY( u32_1 == 0xDEADBABA );
    QVERIFY( u32_2 == 0xDEADBABA );

    /////////////////////////////////////////////////////////////////////////////////
    // AtomicCompareSwap32
    u32_1 = 0xDEADBABA;
    u32_2 = AtomicCompareSwap( (int*)&u32_1, 0xDEADBABA, 0xDEAD123 );
    QVERIFY( u32_1 == 0xDEAD123 );
    QVERIFY( u32_2 == 0xDEADBABA );

    u32_1 = 0xDEADBABA;
    u32_2 = AtomicCompareSwap( (int*)&u32_1, 123, 0xDEAD123 );
    QVERIFY( u32_1 == 0xDEADBABA );
    QVERIFY( u32_2 == 0xDEADBABA );
}

void CAtomicOpsTest::test64()
{
    /**
     * 64-bit atomic operations
     */
    UINT64 u64_1 = 0, u64_2 = 0;

    /////////////////////////////////////////////////////////////////////////////////
    // AtomicInc64
    u64_1 = 0;
    u64_2 = AtomicInc( (long long int*)&u64_1 );
    QVERIFY( u64_1 == 1 );
    QVERIFY( u64_2 == 0 );

    u64_1 = (UINT64)-1;
    u64_2 = AtomicInc( (long long int*) &u64_1 );
    QVERIFY( u64_1 == 0 );
    QVERIFY( u64_2 == (UINT64)-1 );

    /////////////////////////////////////////////////////////////////////////////////
    // AtomicDec64
    u64_1 = 0;
    u64_2 = AtomicDec( (long long int*)&u64_1 );
    QVERIFY( u64_1 == (UINT64)-1 );
    QVERIFY( u64_2 == 0 );

    u64_1 = (UINT64)-1;
    u64_2 = AtomicDec( (long long int*) &u64_1 );
    QVERIFY( u64_1 == (UINT64)(-2) );
    QVERIFY( u64_2 == (UINT64)-1 );

    /////////////////////////////////////////////////////////////////////////////////
    // AtomicAdd64
    u64_1 = 0;
    u64_2 = AtomicAdd( (long long int*)&u64_1, (UINT64)-1 );
    QVERIFY( u64_1 == (UINT64)-1 );
    QVERIFY( u64_2 == 0 );

    u64_1 = (UINT64)-1;
    u64_2 = AtomicAdd( (long long int*) &u64_1, 1 );
    QVERIFY( u64_1 == 0 );
    QVERIFY( u64_2 == (UINT64)-1 );

    /////////////////////////////////////////////////////////////////////////////////
    // AtomicSwap64
    u64_1 = 0xDEADBABA1LL;
    u64_2 = AtomicSwap( (long long int*)&u64_1, 0xDEADBABA2LL );
    QVERIFY( u64_1 == 0xDEADBABA2LL );
    QVERIFY( u64_2 == 0xDEADBABA1LL );

    /////////////////////////////////////////////////////////////////////////////////
    // AtomicIncAndTest64
    u64_1 = 0;
    u64_2 = AtomicIncAndTest( (long long int*)&u64_1 );
    QVERIFY( u64_1 == 1 );
    QVERIFY( u64_2 == 0 );

    u64_1 = (UINT64)-1;
    u64_2 = AtomicIncAndTest( (long long int*) &u64_1 );
    QVERIFY( u64_1 == 0 );
    QVERIFY( u64_2 != 0 );

    /////////////////////////////////////////////////////////////////////////////////
    // AtomicDecAndTest64
    u64_1 = 0;
    u64_2 = AtomicDecAndTest( (long long int*)&u64_1 );
    QVERIFY( u64_1 == (UINT64)-1 );
    QVERIFY( u64_2 == 0 );

    u64_1 = (UINT64)-1;
    u64_2 = AtomicDecAndTest( (long long int*) &u64_1 );
    QVERIFY( u64_1 == (UINT64)-2 );
    QVERIFY( u64_2 == 0 );

    u64_1 = (UINT64)1;
    u64_2 = AtomicDecAndTest( (long long int*) &u64_1 );
    QVERIFY( u64_1 == 0 );
    QVERIFY( u64_2 != 0 );

    /////////////////////////////////////////////////////////////////////////////////
    // AtomicRead64
    u64_1 = 0xDEADBABA123LL;
    u64_2 = AtomicRead( (long long int*)&u64_1 );
    QVERIFY( u64_1 == 0xDEADBABA123LL );
    QVERIFY( u64_2 == 0xDEADBABA123LL );

    /////////////////////////////////////////////////////////////////////////////////
    // AtomicCompareSwap64
    u64_1 = 0xDEADBABA123LL;
    u64_2 = AtomicCompareSwap( (long long int*)&u64_1, 0xDEADBABA123LL, 0xDEAD1230000LL );
    QVERIFY( u64_1 == 0xDEAD1230000LL );
    QVERIFY( u64_2 == 0xDEADBABA123LL );

    u64_1 = 0xDEADBABA123LL;
    u64_2 = AtomicCompareSwap( (long long int*)&u64_1, 123, 0xDEAD1230000LL );
    QVERIFY( u64_1 == 0xDEADBABA123LL );
    QVERIFY( u64_2 == 0xDEADBABA123LL );
}


QTEST_MAIN(CAtomicOpsTest)
