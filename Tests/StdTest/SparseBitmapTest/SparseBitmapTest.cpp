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
///		SparseBitmapTest.cpp
///
/// @author
///		kmalanin@
///
/// @brief
///		Sparsed bitmap test suite.
///
/////////////////////////////////////////////////////////////////////////////
#include	<limits.h>
#include	<errno.h>

#include <Libraries/Std/sparse_bitmap.h>
#include "SparseBitmapTest.h"

#define		PAGE_SIZE_BITS		(PAGE_SIZE << 3llu)
#define		TEST_BITMAP_SIZE_BITS	(PAGE_SIZE_BITS + 100) // two pages

#define		TEST_BITMAP_2_PAGES	(PAGE_SIZE_BITS * 2)
#define		TEST_BITMAP_5_PAGES	(PAGE_SIZE_BITS * 5)

#define		UINT64_BITS		64

void SparseBitmapTest::create()
{
	// less than PAGE_SIZE
	struct sp_bitmap* sbm = sp_bitmap_create(6);
	QVERIFY(sbm != 0);
	sp_bitmap_destroy(sbm);
	//
	sbm = sp_bitmap_create(TEST_BITMAP_SIZE_BITS);
	QVERIFY(sbm != 0);
	sp_bitmap_destroy(sbm);
	// very huge - overflow
	sbm = sp_bitmap_create(UINT_MAX - 100U);
	QVERIFY(0 == sbm);
	// btw check for nill pointer
	sp_bitmap_destroy(sbm);
	// ignore zero size
	sbm = sp_bitmap_create(0);
	QVERIFY(0 == sbm);
}

void SparseBitmapTest::setBits()
{	// Use two pages
	struct sp_bitmap* sbm = sp_bitmap_create(TEST_BITMAP_SIZE_BITS);
	QVERIFY( sbm != 0);
	int	rv;
	// first in first page
	rv = sp_bitmap_set(sbm, 0);
	QVERIFY(0 == rv);
	// last in first page
	rv = sp_bitmap_set(sbm, PAGE_SIZE_BITS - 1);
	QVERIFY(0 == rv);
	// first in second page
	rv = sp_bitmap_set(sbm, PAGE_SIZE_BITS);
	QVERIFY(0 == rv);
	// fill second page
	for (int i = PAGE_SIZE_BITS; i < TEST_BITMAP_SIZE_BITS; i++)
	{
		rv = sp_bitmap_set(sbm, i);
		QVERIFY(0 == rv);
	}
	sp_bitmap_destroy(sbm);
}

void	SparseBitmapTest::clearBits()
{
	int	rv;
	struct sp_bitmap* sbm = sp_bitmap_create(TEST_BITMAP_SIZE_BITS);
	QVERIFY( sbm != 0);
	// By default pages are zeroed.
	// Check sparsed pages
	rv = sp_bitmap_clear(sbm, PAGE_SIZE_BITS - 1);
	QVERIFY(0 == rv);
	rv = sp_bitmap_clear(sbm, PAGE_SIZE_BITS);
	QVERIFY(0 == rv);
	//
	rv = sp_bitmap_set_all(sbm);
	QVERIFY(0 == rv);
	//
	rv = sp_bitmap_clear(sbm, 0);
	QVERIFY(0 == rv);
	rv = sp_bitmap_clear(sbm, 1);
	QVERIFY(0 == rv);
	rv = sp_bitmap_clear(sbm, 2);
	QVERIFY(0 == rv);
	//
	rv = sp_bitmap_clear(sbm, 10);
	QVERIFY(0 == rv);
	rv = sp_bitmap_clear(sbm, 14);
	QVERIFY(0 == rv);
	// last in first page
	rv = sp_bitmap_clear(sbm, PAGE_SIZE_BITS - 1);
	QVERIFY(0 == rv);
	//
	sp_bitmap_destroy(sbm);
}

void	SparseBitmapTest::checkBitSet()
{
	int	rv;
	struct sp_bitmap* sbm = sp_bitmap_create(TEST_BITMAP_SIZE_BITS);
	QVERIFY(sbm != 0);
	// case: all are zero
	rv = sp_bitmap_is_set(sbm, PAGE_SIZE_BITS - 1);
	QVERIFY(0 == rv);
	// case: all are one
	rv = sp_bitmap_set_all(sbm);
	QVERIFY(0 == rv);
	//
	rv = sp_bitmap_is_set(sbm, PAGE_SIZE_BITS + 11);
	QVERIFY(1 == rv);
	// case: mixed
	rv = sp_bitmap_clear(sbm, 64*5 + 17);
	QVERIFY(0 == rv);
	rv = sp_bitmap_clear(sbm, 64*6 + 19);
	QVERIFY(0 == rv);
	// two bits are 0, another -1
	rv = sp_bitmap_is_set(sbm, 64);
	QVERIFY(1 == rv);
	rv = sp_bitmap_is_set(sbm, 64*5 + 17);
	QVERIFY(0 == rv);
	//
	sp_bitmap_destroy(sbm);
}

#define	BIT_FIRST	(64*3 + 7)
#define	BIT_COUNT	(64*3 + 50)

void	SparseBitmapTest::setRange()
{
	int	rv;
	struct sp_bitmap* sbm = sp_bitmap_create(TEST_BITMAP_5_PAGES);
	QVERIFY(sbm != 0);
	// range: inside one 64 bits entry
	// from bit 0 to bit 56 (first entry)
	rv = sp_bitmap_set_range(sbm, 0, 57);
	QVERIFY(0 == rv);
	for (UINT32 i = 0; i < 57; i++)
	{
		rv = sp_bitmap_is_set(sbm, i);
		QVERIFY(1 == rv);
	}
	// from bit 70 to bit 127 (second entry)
	rv = sp_bitmap_set_range(sbm, 70, 57);
	QVERIFY(0 == rv);
	for (UINT32 i = 70; i < 127; i++)
	{
		rv = sp_bitmap_is_set(sbm, i);
		QVERIFY(1 == rv);
	}
	// from bit 130 to 180 (third entry)
	rv = sp_bitmap_set_range(sbm, 130, 50);	//192
	QVERIFY(0 == rv);
	for (UINT32 i = 130; i < 180; i++)
	{
		rv = sp_bitmap_is_set(sbm, i);
		QVERIFY(1 == rv);
	}
	// range: inside first page (from fourth entry)
	rv = sp_bitmap_set_range(sbm, BIT_FIRST, BIT_COUNT);
	QVERIFY(0 == rv);
	for (UINT32 i = BIT_FIRST; i < (BIT_FIRST+BIT_COUNT); i++)
	{
		rv = sp_bitmap_is_set(sbm, i);
		QVERIFY(1 == rv);
	}
	//
	sp_bitmap_destroy(sbm);
	sbm = sp_bitmap_create(TEST_BITMAP_5_PAGES);
	QVERIFY(sbm != 0);
	// range: from middle of first page to middle of last page
	// all bits are zero
	rv = sp_bitmap_set_range(sbm, BIT_FIRST, TEST_BITMAP_5_PAGES - 200);
	QVERIFY(0 == rv);
	for (UINT32 i = BIT_FIRST; i < (TEST_BITMAP_5_PAGES - 200); i++)
	{
		rv = sp_bitmap_is_set(sbm, i);
		QVERIFY(1 == rv);
	}
	// all bits are set
	rv = sp_bitmap_set_all(sbm);
	QVERIFY(0 == rv);

	rv = sp_bitmap_set_range(sbm, BIT_FIRST, TEST_BITMAP_5_PAGES - 200);
	QVERIFY(0 == rv);
	for (UINT32 i = BIT_FIRST; i < (TEST_BITMAP_5_PAGES - 200); i++)
	{
		rv = sp_bitmap_is_set(sbm, i);
		QVERIFY(1 == rv);
	}
	//
	sp_bitmap_destroy(sbm);
}

void	SparseBitmapTest::clearRange()
{
	int	rv;
	struct sp_bitmap* sbm = sp_bitmap_create(TEST_BITMAP_5_PAGES);
	QVERIFY(sbm != 0);
	rv = sp_bitmap_set_all(sbm);
	QVERIFY(0 == rv);
	// range: inside one 64 bits entry
	// from bit 0 to bit 56 (first entry)
	rv = sp_bitmap_clear_range(sbm, 0, 57);
	QVERIFY(0 == rv);
	for (UINT32 i = 0; i < 57; i++)
	{
		rv = sp_bitmap_is_set(sbm, i);
		QVERIFY(0 == rv);
	}
	// from bit 70 to bit 127 (second entry)
	rv = sp_bitmap_clear_range(sbm, 70, 57);
	QVERIFY(0 == rv);
	for (UINT32 i = 70; i < 127; i++)
	{
		rv = sp_bitmap_is_set(sbm, i);
		QVERIFY(0 == rv);
	}
	// from bit 130 to 180 (third entry)
	rv = sp_bitmap_clear_range(sbm, 130, 50);	//192
	QVERIFY(0 == rv);
	for (UINT32 i = 130; i < 180; i++)
	{
		rv = sp_bitmap_is_set(sbm, i);
		QVERIFY(0 == rv);
	}
	// range: inside first page (from fourth entry)
	rv = sp_bitmap_clear_range(sbm, BIT_FIRST, BIT_COUNT);
	QVERIFY(0 == rv);
	for (UINT32 i = BIT_FIRST; i < (BIT_FIRST+BIT_COUNT); i++)
	{
		rv = sp_bitmap_is_set(sbm, i);
		QVERIFY(0 == rv);
	}
	//
	sp_bitmap_destroy(sbm);
	sbm = sp_bitmap_create(TEST_BITMAP_5_PAGES);
	QVERIFY(sbm != 0);
	// range: from middle of first page to middle of last page
	// all bits are zero
	rv = sp_bitmap_clear_range(sbm, BIT_FIRST, TEST_BITMAP_5_PAGES - 200);
	QVERIFY(0 == rv);
	for (UINT32 i = BIT_FIRST; i < (TEST_BITMAP_5_PAGES - 200); i++)
	{
		rv = sp_bitmap_is_set(sbm, i);
		QVERIFY(0 == rv);
	}
	// all bits are set
	rv = sp_bitmap_set_all(sbm);
	QVERIFY(0 == rv);

	rv = sp_bitmap_clear_range(sbm, BIT_FIRST, TEST_BITMAP_5_PAGES - 200);
	QVERIFY(0 == rv);
	for (UINT32 i = BIT_FIRST; i < (TEST_BITMAP_5_PAGES - 200); i++)
	{
		rv = sp_bitmap_is_set(sbm, i);
		QVERIFY(0 == rv);
	}
	//
	sp_bitmap_destroy(sbm);
}

void	SparseBitmapTest::checkRangeSet()
{
	int	rv;
	struct sp_bitmap* sbm = sp_bitmap_create(TEST_BITMAP_5_PAGES);
	QVERIFY(sbm != 0);
	//
	// range: from middle of first page to middle of last page
	// all bits are zero
	rv = sp_bitmap_set_range(sbm, BIT_FIRST, TEST_BITMAP_5_PAGES - 200);
	QVERIFY(0 == rv);
	//
	rv = sp_bitmap_is_set_range(sbm, BIT_FIRST, TEST_BITMAP_5_PAGES - 200);
	QVERIFY(1 == rv);
	//
	rv = sp_bitmap_is_set_range(sbm, BIT_FIRST, TEST_BITMAP_5_PAGES - 100);
	QVERIFY(-EINVAL == rv);
	//
	sp_bitmap_destroy(sbm);
}

void	SparseBitmapTest::checkRangeClear()
{
	int	rv;
	struct sp_bitmap* sbm = sp_bitmap_create(TEST_BITMAP_5_PAGES);
	QVERIFY(sbm != 0);
	// all bits are set
	sp_bitmap_set_all(sbm);
	//
	// range: from middle of first page to middle of last page
	rv = sp_bitmap_clear_range(sbm, BIT_FIRST, TEST_BITMAP_5_PAGES - 200);
	QVERIFY(0 == rv);
	//
	rv = sp_bitmap_is_clear_range(sbm, BIT_FIRST, TEST_BITMAP_5_PAGES - 200);
	QVERIFY(1 == rv);
	//
	rv = sp_bitmap_is_clear_range(sbm, BIT_FIRST, TEST_BITMAP_5_PAGES - 100);
	QVERIFY(-EINVAL == rv);
	//
	sp_bitmap_destroy(sbm);
}

void SparseBitmapTest::checkFindFirstClear()
{
	LONG64	rv;
	struct sp_bitmap* sbm = sp_bitmap_create(TEST_BITMAP_5_PAGES);
	QVERIFY(sbm != 0);
	// all bits are set
	sp_bitmap_set_all(sbm);
	// range: whole second page
	rv = sp_bitmap_clear_range(sbm, PAGE_SIZE_BITS*2, PAGE_SIZE_BITS);
	QVERIFY(0 == rv);
	// first bit on second page must be clear
	rv = sp_bitmap_find_first_clear(sbm, TEST_BITMAP_5_PAGES, 0);
	QVERIFY(PAGE_SIZE_BITS == rv);
	// bit 31 on second page must be clear
	rv = sp_bitmap_set_range(sbm, PAGE_SIZE_BITS + 31, PAGE_SIZE_BITS);
	QVERIFY(0 == rv);
	// search from start of bitmap to the end of the page
	rv = sp_bitmap_find_first_clear(sbm, PAGE_SIZE_BITS*2, 0);
	QVERIFY(PAGE_SIZE_BITS + 31 == rv);
	// search from middle of bitmap to the end of the page
	rv = sp_bitmap_find_first_clear(sbm, PAGE_SIZE_BITS*2, PAGE_SIZE_BITS + 11);
	QVERIFY(PAGE_SIZE_BITS + 31 == rv);
	// search from middle of bitmap to the middle of the page
	rv = sp_bitmap_find_first_clear(sbm, PAGE_SIZE_BITS + 32, PAGE_SIZE_BITS + 11);
	QVERIFY(PAGE_SIZE_BITS + 31 == rv);
}

QTEST_MAIN(SparseBitmapTest)
