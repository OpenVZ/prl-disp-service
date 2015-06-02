/////////////////////////////////////////////////////////////////////////////
///
///	@file HandlesManipulationsTest.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing handles manipulations SDK API.
///
///	@author sandro
///
/// Copyright (c) 2005-2015 Parallels IP Holdings GmbH
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
/////////////////////////////////////////////////////////////////////////////
#include "HandlesManipulationsTest.h"

HandlesManipulationsTest::HandlesManipulationsTest()
{}

void HandlesManipulationsTest::cleanup()
{
	m_Handle.reset();
}

void HandlesManipulationsTest::testCreateHandleOnInvalidHandlePtr()
{
	PRL_RESULT _res = PrlSrv_Create(0);
	QVERIFY(_res == PRL_ERR_INVALID_ARG);
}

void HandlesManipulationsTest::testCreateHandleOnValidParams()
{
	QVERIFY(PrlSrv_Create(m_Handle.GetHandlePtr()) == PRL_ERR_SUCCESS);
}

void HandlesManipulationsTest::testGetHandleTypeOnInvalidHandle()
{
	PRL_HANDLE_TYPE _handle_type;
	QVERIFY(PrlHandle_GetType(m_Handle, &_handle_type) == PRL_ERR_INVALID_ARG);
}

void HandlesManipulationsTest::testGetHandleTypeOnInvalidHandleTypePtr()
{
	QVERIFY(PrlSrv_Create(m_Handle.GetHandlePtr()) == PRL_ERR_SUCCESS);
	PRL_HANDLE_TYPE_PTR pHandleType = 0;
	QVERIFY(PrlHandle_GetType(m_Handle, pHandleType));
}

void HandlesManipulationsTest::testGetHandleTypeOnValidHandle()
{
	QVERIFY(PrlSrv_Create(m_Handle.GetHandlePtr()) == PRL_ERR_SUCCESS);
	PRL_HANDLE_TYPE _handle_type;
	QVERIFY(PrlHandle_GetType(m_Handle, &_handle_type) == PRL_ERR_SUCCESS);
	QVERIFY(_handle_type == PHT_SERVER);
}
