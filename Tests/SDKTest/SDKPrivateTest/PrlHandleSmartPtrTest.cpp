/*
 * Copyright (c) 2015-2017, Parallels International GmbH
 *
 * This file is part of Virtuozzo Core. Virtuozzo Core is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation;
 * either version 2 of the License, or (at your option) any later
 * version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Our contact details: Parallels International GmbH, Vordergasse 59, 8200
 * Schaffhausen, Switzerland.
 */

#include "PrlHandleSmartPtrTest.h"

namespace {
class PrlHandleTest : public PrlHandleBase {
public:
	PrlHandleTest() : PrlHandleBase(PHT_SERVER) {}
};
}

typedef PrlHandleSmartPtr<PrlHandleTest> PrlHandleTestPtr;

PrlHandleSmartPtrTest::PrlHandleSmartPtrTest()
: m_pHandle(NULL)
{}

void PrlHandleSmartPtrTest::init()
{
	m_pHandle = new PrlHandleTest;
	m_pHandle2 = new PrlHandleTest;
}

void PrlHandleSmartPtrTest::cleanup()
{
	if (m_pHandle)
	{
		QVERIFY(!m_pHandle->Release());
		m_pHandle = NULL;
	}
	if (m_pHandle2)
	{
		QVERIFY(!m_pHandle2->Release());
		m_pHandle2 = NULL;
	}
}

void PrlHandleSmartPtrTest::testConstructorDestructor()
{
	{
		PrlHandleTestPtr _ptr((PrlHandleTest *)m_pHandle);
		QCOMPARE(quint32(m_pHandle->GetRefCount()), quint32(2));
	}
	QCOMPARE(quint32(m_pHandle->GetRefCount()), quint32(1));
}

void PrlHandleSmartPtrTest::testCopyConstructor()
{
	PrlHandleTestPtr _ptr1((PrlHandleTest *)m_pHandle);
	PrlHandleTestPtr _ptr2 = _ptr1;
	QCOMPARE(quint32(m_pHandle->GetRefCount()), quint32(3));
}

void PrlHandleSmartPtrTest::testAssignOperator()
{
	PrlHandleTestPtr _ptr1((PrlHandleTest *)m_pHandle), _ptr2((PrlHandleTest *)m_pHandle2);
	QCOMPARE(quint32(m_pHandle->GetRefCount()), quint32(2));
	QCOMPARE(quint32(m_pHandle2->GetRefCount()), quint32(2));
	_ptr2 = _ptr1;
	QCOMPARE(quint32(m_pHandle->GetRefCount()), quint32(3));
	QCOMPARE(quint32(m_pHandle2->GetRefCount()), quint32(1));
}

void PrlHandleSmartPtrTest::testPointerAssignOperator()
{
	PrlHandleTestPtr _ptr((PrlHandleTest *)m_pHandle);
	QCOMPARE(quint32(m_pHandle->GetRefCount()), quint32(2));
	_ptr = (PrlHandleTest *)m_pHandle2;
	QCOMPARE(quint32(m_pHandle->GetRefCount()), quint32(1));
	QCOMPARE(quint32(m_pHandle2->GetRefCount()), quint32(2));
}

void PrlHandleSmartPtrTest::testConstructorDestructorOnNullPointer()
{
	PrlHandleTestPtr _ptr(NULL);
}

void PrlHandleSmartPtrTest::testCopyConstructorOnNullPointer()
{
	PrlHandleTestPtr _ptr1(NULL);
	PrlHandleTestPtr _ptr2 = _ptr1;
}

void PrlHandleSmartPtrTest::testAssignOperatorOnNullPointer()
{
	PrlHandleTestPtr _ptr1((PrlHandleTest *)m_pHandle), _ptr2(NULL);
	QCOMPARE(quint32(m_pHandle->GetRefCount()), quint32(2));
	_ptr1 = _ptr2;
	QCOMPARE(quint32(m_pHandle->GetRefCount()), quint32(1));
}

void PrlHandleSmartPtrTest::testAssignOperatorOnNullPointer2()
{
	PrlHandleTestPtr _ptr1((PrlHandleTest *)m_pHandle), _ptr2(NULL);
	QCOMPARE(quint32(m_pHandle->GetRefCount()), quint32(2));
	_ptr2 = _ptr1;
	QCOMPARE(quint32(m_pHandle->GetRefCount()), quint32(3));
}

void PrlHandleSmartPtrTest::testAssignOperatorOnNullPointer3()
{
	PrlHandleTestPtr _ptr1(NULL), _ptr2(NULL);
	_ptr2 = _ptr1;
}

void PrlHandleSmartPtrTest::testPointerAssignOperatorOnNullPointer()
{
	PrlHandleTestPtr _ptr((PrlHandleTest *)m_pHandle);
	QCOMPARE(quint32(m_pHandle->GetRefCount()), quint32(2));
	_ptr = NULL;
	QCOMPARE(quint32(m_pHandle->GetRefCount()), quint32(1));
}

void PrlHandleSmartPtrTest::testPointerAssignOperatorOnNullPointer2()
{
	PrlHandleTestPtr _ptr(NULL);
	_ptr = (PrlHandleTest *)m_pHandle;
	QCOMPARE(quint32(m_pHandle->GetRefCount()), quint32(2));
}

void PrlHandleSmartPtrTest::testPointerAssignOperatorOnNullPointer3()
{
	PrlHandleTestPtr _ptr(NULL);
	_ptr = NULL;
}

void PrlHandleSmartPtrTest::testPointerAssignOperatorOnSameHandle()
{
	PrlHandleTestPtr _ptr1((PrlHandleTest *)m_pHandle), _ptr2((PrlHandleTest *)m_pHandle);
	QCOMPARE(quint32(m_pHandle->GetRefCount()), quint32(3));
	_ptr2 = _ptr1;
	QCOMPARE(quint32(m_pHandle->GetRefCount()), quint32(3));
}

void PrlHandleSmartPtrTest::testPointerAssignOperatorOnSameHandleAfterReleaseOccuredFromOutside()
{
	PrlHandleTestPtr _ptr1((PrlHandleTest *)m_pHandle), _ptr2((PrlHandleTest *)m_pHandle);
	m_pHandle->Release();
	PrlHandleBase *pHandle = m_pHandle;
	m_pHandle = NULL;
	QCOMPARE(quint32(pHandle->GetRefCount()), quint32(2));
	_ptr2 = _ptr1;
	QCOMPARE(quint32(pHandle->GetRefCount()), quint32(2));
}
