/////////////////////////////////////////////////////////////////////////////
///
///	@file HandlesManipulationsTest.h
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing handles manipulations SDK API.
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
#ifndef HandlesManipulationsTest_H
#define HandlesManipulationsTest_H

#include <QtTest/QtTest>
#include <prlsdk/helpers/SdkWrap/SdkHandleWrap.h>

class HandlesManipulationsTest : public QObject
{

Q_OBJECT

public:
	HandlesManipulationsTest();

private slots:
	void cleanup();
	void testCreateHandleOnInvalidHandlePtr();
	void testCreateHandleOnValidParams();
	void testGetHandleTypeOnInvalidHandle();
	void testGetHandleTypeOnValidHandle();
	void testGetHandleTypeOnInvalidHandleTypePtr();

private:
	SdkHandleWrap m_Handle;
};

#endif
