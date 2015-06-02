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
///		TestDspCmdUserLogin.h
///
/// @author
///		sergeyt
///
/// @brief
///		Tests fixture class for testing DspCmdUserLogin dispatcher API command functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#ifndef H_TestDspCmdUserLogin_H
#define H_TestDspCmdUserLogin_H

#include "TestDispatcherBase.h"

class TestDspCmdUserLogin : public TestDispatcherBase
{
   Q_OBJECT

public:
	virtual ~TestDspCmdUserLogin() {}

private slots:
	void init();
	void cleanup();
	void cleanupTestCase();
	void TestWithValidUser2();
	void TestWithValidUser();
	void TestWithWrongUser();
	void TestWithWrongPassword();
	void TestWithRootUserWithoutPassword();
	void TestWithEmptyUser();
	void TestWithEmptyPassword();
	void TestWithBothUserAndPasswordEmpty();
	void TestWithTooLongStringUser();
	void TestCaseSensivityUserName();

private:
	QString getUserId_byUserName( const QString& loginName );
};

#endif //H_TestDspCmdUserLogin_H
