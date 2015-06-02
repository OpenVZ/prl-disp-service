/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (C) 2006-2015 Parallels IP Holdings GmbH
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
/// @file
///		BitOpsTest.cpp
///
/// @author
///		kmalanin@
///
/// @brief
///		BitOps and BitOps64 test suite.
///
/////////////////////////////////////////////////////////////////////////////
#include	<limits.h>

#include <Libraries/Std/BitOps.h>

#include	"BitOpsTest.h"

#define		PAGE_SIZE_BITS		(PAGE_SIZE << 3llu)
#define		TEST_BITMAP_SIZE_BITS	(PAGE_SIZE_BITS + 100) // two pages

#define		TEST_BITMAP_2_PAGES	(PAGE_SIZE_BITS * 2)
#define		TEST_BITMAP_5_PAGES	(PAGE_SIZE_BITS * 5)

#define		UINT64_BITS		64

void	BitOpsTest::findLowers64()
{
	//
	LONG64	rv = BitFindLowestSet64(0x0100);
	QVERIFY(8 == rv);

	rv = BitFindLowestSet64(0);
	QVERIFY(-1 == rv);
	// 1111 1110 1111 1111
	rv = BitFindLowestClear64(0xFEFF);
	QVERIFY(8 == rv);

	rv = BitFindLowestClear64((~0llu));
	QVERIFY(-1 == rv);
}

void	BitOpsTest::findNextSetSingle64()
{
	// FF FF FF FF FF FF FF 00
	UINT64	buff = 0xFFllu << 8;

	LONG64	rv = BitFindNextSet64(&buff, 64, 0);
	QString	msg = QString("Wait %1, got %2").arg(8).arg(rv);
	QVERIFY2(8 == rv, qPrintable(msg));

	rv = BitFindNextSet64(&buff, 8, 0);
	msg = QString("Wait %1, got %2").arg(-1).arg(rv);
	QVERIFY2(-1 == rv, qPrintable(msg));

	// FF 00 00 00 00 00 00 00
	buff = (0xFFllu) << 56;

	rv = BitFindNextSet64(&buff, 64, 0);
	msg = QString("Wait %1, got %2").arg(56).arg(rv);
	QVERIFY2(56 == rv, qPrintable(msg));

	rv = BitFindNextSet64(&buff, 64, 56);
	msg = QString("Wait %1, got %2").arg(56).arg(rv);
	QVERIFY2(56 == rv, qPrintable(msg));

	// 00 00 00 00 FF 00 00 00
	buff = (0xFFllu) << 24;

	rv = BitFindNextSet64(&buff, 25, 20);
	msg = QString("Wait %1, got %2").arg(24).arg(rv);
	QVERIFY2(24 == rv, qPrintable(msg));

	// all zeroes
	buff = 0;
	rv = BitFindNextSet64(&buff, 64, 0);

	msg = QString("Wait %1, got %2").arg(-1).arg(rv);
	QVERIFY2(-1 == rv, qPrintable(msg));
}

void	BitOpsTest::findNextClearSingle64()
{
	// FF 00 00 00 00 00 00 00
	UINT64	buff = (0xFFllu) << 56;
	LONG64	rv = BitFindNextClear64(&buff, 64, 0);

	QString	msg = QString("Wait %1, got %2").arg(0).arg(rv);
	QVERIFY2(0 == rv, qPrintable(msg));

	rv = BitFindNextClear64(&buff, 64, 56);

	msg = QString("Wait %1, got %2").arg(-1).arg(rv);
	QVERIFY2(-1 == rv, qPrintable(msg));

	// 00 00 00 00 00 00 00 FF
	buff = 0xFFllu;
	// range 0...7 bits - no bits clear
	rv = BitFindNextClear64(&buff, 8, 0);
	msg = QString("Wait %1, got %2").arg(-1).arg(rv);
	QVERIFY2(-1 == rv, qPrintable(msg));

	// range 4...8 bits - bit 8 clear
	rv = BitFindNextClear64(&buff, 9, 4);
	msg = QString("Wait %1, got %2").arg(8).arg(rv);
	QVERIFY2(8 == rv, qPrintable(msg));

	// 00 00 00 00 00 00 FF 00
	buff = (0x0FFllu) << 8;
	rv = BitFindNextClear64(&buff, 64, 10);
	msg = QString("Wait %1, got %2").arg(16).arg(rv);
	QVERIFY2(16 == rv, qPrintable(msg));
}

void	BitOpsTest::findNextSet64()
{
	UINT64*	buff = new UINT64[3];
	QVERIFY(buff != 0);
	//	FF 00 00 .... 00
	buff[0] = buff[1] = 0;
	buff[2] = (0xFFllu) << 56;

	LONG64	rv = BitFindNextSet64(buff, 3*64, 0);

	QString	msg = QString("Wait %1, got %2")
			.arg((2*64 + 56))
			.arg(rv);
	QVERIFY2((2*64 + 56) == rv, qPrintable(msg));
	// all zeroes
	buff[2] = 0;

	rv = BitFindNextSet64(buff, 3*64, 0);

	msg = QString("Wait -1, got %1").arg(rv);
	QVERIFY2(-1 == rv, qPrintable(msg));
	// 00 ... 00 07, start from bit 3
	buff[0] = 0x07;

	rv = BitFindNextSet64(buff, 64 - 3, 3);

	msg = QString("Wait -1, got %1").arg(rv);
	QVERIFY2(-1 == rv, qPrintable(msg));
	//
	delete buff;
}

void BitOpsTest::findNextClear64()
{
	UINT64*	buff = new UINT64[3];
	QVERIFY(buff != 0);
	// [FF...FF] [FF...FF] [00 FF ... FF]
	buff[0] = buff[1] = buff[2] = (~0llu);
	buff[2] &= ~((0xFFlu) << 56);

	LONG64	rv = BitFindNextClear64(buff, 3*64, 0);
	QString msg = QString("Wait %1, got %2")
			.arg((2*64 + 56))
			.arg(rv);
	QVERIFY2((2*64 + 56) == rv, qPrintable(msg));
	// [FF...FF] [FF...FF] [FF... FF]
	buff[2] = (~0llu);

	rv = BitFindNextClear64(buff, 3*64, 0);
	msg = QString("Wait -1, got %1").arg(rv);
	QVERIFY2(-1 == rv, qPrintable(msg));

	// [00..00FF] [FF...FF] [FF... FF]
	buff[0] = 0xFFlu;

	rv = BitFindNextClear64(buff, 64 - 3, 3);
	msg = QString("Wait 8, got %1").arg(rv);
	QVERIFY2(8 == rv, qPrintable(msg));
}

QTEST_MAIN(BitOpsTest)
