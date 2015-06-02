///////////////////////////////////////////////////////////////////////////////
///
/// @file CyclicListTest.h
///
///	Tests fixture class for testing Monitor/Std/ functionality.
///
/// @author vtatarinov
/// @owner alexeyk
///
/// Copyright (c) 2007-2015 Parallels IP Holdings GmbH
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
///////////////////////////////////////////////////////////////////////////////

#ifndef __MONITOR_STD_H__
#define __MONITOR_STD_H__

#include <QtTest/QtTest>
#include "Tests/MonitorStdTest/Monitor.h"

/**
* Class that contains unit-tests for testing Monitor/Std/ functionality.
*
* @author vtatarinov
*/
class MonitorStdTest
	: public QObject
{
	Q_OBJECT

	public:
		MonitorStdTest();

	private slots:
		void init();
		void cleanup();
		void CyclicList_TestIsEmpty();
		void CyclicList_TestInit();
		void CyclicList_TestAdd();
		void CyclicList_TestLoop();
		void CyclicList_TestLoopRev();
		void CyclicList_TestMoveLoop();
		void CyclicList_TestGetMostUsed();
		void CyclicList_TestGetOldest();
		void CyclicList_TestRemove();
		void CyclicList_TestPopup();
		void CyclicList_TestPushDown();
		void SimpleLinkedHashTest();
		void StdLibraryTest();
		void BidirListTest();
		void CacheLruTest();
		void DirectHashTableTest();
		void DmmTest();
		void HashTreeTest();
		void ListMemManTest();
		void LookasideTest();
		void MemManTest();

	private:

#ifdef _WIN_
		/// Type of elements for testing Lookaside with dependent pages.
		struct ValOffs
		{
			int v;
			void* ptr1;
			void* ptr2;
			void* ptr3;
		};

		/**
		* Lookaside element destructor.
		* @see DESTRUCT_ELEM in Monitor/Std/Lookaside.h
		*/
		static void TestLookasideDestructor( ValOffs* pEntry, UINT uWhere );
		/**
		* Lookaside element action.
		* @see LOOKASIDE_ELEM_ACTION in Monitor/Std/Lookaside.h
		*/
		static void TestLookasideElemAction( void * pEntry );
		/**
		* Dmm chunk destructor.
		* @see DESTRUCTOR_PAGE in Monitor/Std/Dmm.h
		*/
		static void TestDmmDestructor(PVOID pOwner, PVOID pPage, UINT uwhere);

		/// Sizes of DMM zones. Each zone is monitor buffer actually.
		/// One zone per attribute.
		const static size_t s_dmmZoneSizes[DMM_ATTR_MAX];
		/// Makers of DMM zones. Marker - first byte of page.
		/// Each attribute has it's own marker.
		const static char s_zoneMarkers[DMM_ATTR_MAX];
		/// Total size of zones.
		size_t m_totalZonesSize;

		/// Descriptors of monitor buffers. One per each attribute.
		MON_BUFF_DESCR  m_aMonBuf[DMM_ATTR_MAX];
		/// Pointers to monitor buffers. One per each attribute.
		PVOID           m_pBufPtr[DMM_ATTR_MAX];
		/// Table of pointers to monitor buffers' descriptors.
		/// One per each attribute.
		MON_BUFF_DESCR* m_pPtrs[DMM_ATTR_MAX];
		PVOID m_pBuf;
#endif // _WIN_
};

#endif	// __MONITOR_STD_H__
