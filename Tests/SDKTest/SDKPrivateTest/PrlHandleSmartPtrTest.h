/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlHandleSmartPtrTest.h
///
///	This file is the part of parallels public SDK library private tests suite.
///	Tests fixture class for testing handle base object smart pointer.
///
///	@author sandro
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/// Our contact details: Parallels International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/////////////////////////////////////////////////////////////////////////////
#ifndef PrlHandleSmartPtrTest_H
#define PrlHandleSmartPtrTest_H

#include <QtTest/QtTest>
#include "SDK/Handles/Core/PrlHandleBase.h"

class PrlHandleSmartPtrTest : public QObject
{

Q_OBJECT

public:
	PrlHandleSmartPtrTest();

private slots:
	void init();
	void cleanup();
	void testConstructorDestructor();
	void testCopyConstructor();
	void testAssignOperator();
	void testPointerAssignOperator();
	void testConstructorDestructorOnNullPointer();
	void testCopyConstructorOnNullPointer();
	void testAssignOperatorOnNullPointer();
	void testAssignOperatorOnNullPointer2();
	void testAssignOperatorOnNullPointer3();
	void testPointerAssignOperatorOnNullPointer();
	void testPointerAssignOperatorOnNullPointer2();
	void testPointerAssignOperatorOnNullPointer3();
	void testPointerAssignOperatorOnSameHandle();
	void testPointerAssignOperatorOnSameHandleAfterReleaseOccuredFromOutside();

private:
	PrlHandleBase *m_pHandle, *m_pHandle2;
};

#endif
