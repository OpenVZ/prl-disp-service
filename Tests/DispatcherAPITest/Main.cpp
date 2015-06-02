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
///		Main.cpp
///
/// @author
///		sergeyt
///
/// @brief
///		Dispatcher API tests entry point.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#include <QCoreApplication>
#include <QtTest/QtTest>

#include "Main.h"

#include "Tests/CommonTestsUtils.h"

#include "TestDspCmdUserLogin.h"
#include "TestDspCmdDirGetVmList.h"
#include "TestDspCmdVmGetConfig.h"
#include "TestDspFSCommands.h"
#include "TestDspCmdUserGetProfile.h"
#include "TestDspCmdUserGetHostHwInfo.h"
#include "TestDspCmdDirVmCreate.h"
#include "TestDspCmdDirVmDelete.h"
#include "TestDspCmdDirVmClone.h"

void TestsExecuter::PushTestsExecution()
{
	int nRet = 0;
	EXECUTE_TESTS_SUITE(TestDspCmdUserLogin)
	EXECUTE_TESTS_SUITE(TestDspCmdDirGetVmList)
	EXECUTE_TESTS_SUITE(TestDspCmdVmGetConfig)
	EXECUTE_TESTS_SUITE(TestDspFSCommands)
	EXECUTE_TESTS_SUITE(TestDspCmdUserGetProfile)
	EXECUTE_TESTS_SUITE(TestDspCmdUserGetHostHwInfo)
	EXECUTE_TESTS_SUITE(TestDspCmdDirVmCreate)
	EXECUTE_TESTS_SUITE(TestDspCmdDirVmDelete)
	EXECUTE_TESTS_SUITE(TestDspCmdDirVmClone)

	QCoreApplication::exit(nRet);
}

int main(int argc, char *argv[])
{
	//https://bugzilla.sw.ru/show_bug.cgi?id=456391
	WRITE_TRACE(DBG_FATAL, "Skipping tests suite due numerous of problems: #456391, #455553");
	return 0;

	QCoreApplication a(argc, argv);
	TestConfig::readTestParameters(argv[0]);

	if (!TestConfig::isServerMode())
	{
		WRITE_TRACE(DBG_FATAL, "Skipping tests suite due desktop mode not supported");
		return (0);
	}

	TestsExecuter _tests_executer(argc, argv);
	QTimer::singleShot(0, &_tests_executer, SLOT(PushTestsExecution()));
	return (a.exec());
}
