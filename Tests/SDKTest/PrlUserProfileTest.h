/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlUserProfileTest.h
///
/// User profile related api test
///
///	@author andreydanin
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
#ifndef PrlUserProfileTest_H
#define PrlUserProfileTest_H

#include <QtTest/QtTest>
#include <SDK/Wrappers/SdkWrap/SdkHandleWrap.h>

class PrlUserProfileTest : public QObject
{

Q_OBJECT

private slots:
	void initTestCase();
	void cleanupTestCase();

	void testGetUserProfile();
	void testSetProxy();
	void testSetProxy2();

private:
	void GetUserProfile();

private:
	SdkHandleWrap m_hServer;
	SdkHandleWrap m_hUserProfile;
};

#endif // PrlUserProfileTest_H

