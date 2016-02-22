/////////////////////////////////////////////////////////////////////////////
///
///	@file Main.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	SDK API tests entry point.
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
#include <QCoreApplication>
#include <QtTest/QtTest>
#include <QTimer>

#include "Main.h"

#include <prlcommon/Logging/Logging.h>
#include "Tests/CommonTestsUtils.h"

#include "HandlesManipulationsTest.h"
#include "PrlSrvManipulationsTest.h"
#include "PrlVirtualNetworkTest.h"
#include "JobsManipulationsTest.h"
#include "PrlVmManipulationsTest.h"
#include "PrlCtManipulationsTest.h"
#include "PrlVmValidateConfigTest.h"
#include "PrlVmDefaultConfigTest.h"
#include "PrlVmDevManipulationsTest.h"
#include "UserCallbackTest.h"
#include "PrivateSituationsTest.h"
#include "PrlApiBasicsTest.h"
#include "PrlUserProfileTest.h"
#include "DispFunctionalityTest.h"
#include "PrlVmExecFunctionalityTest.h"
#include "PrlExclusiveVmLockTest.h"
#include "PrlVmUptimeTest.h"
#include "PrlNetworkShapingTest.h"
#include "PrlSataDevicesHotPlugTest.h"
#include "PrlIPPrivateNetworkTest.h"
#include "PrlAutoReportsTest.h"
#include "PrlUsbDevicesHotPlugTest.h"

#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"
#include <prlcommon/HostUtils/PrlMiscellaneous.h>

#ifdef _LIN_
#include "Libraries/Virtuozzo/CVzHelper.h"
#endif

void TestsExecuter::PushTestsExecution()
{
#ifdef DYN_API_WRAP
	if ( !SdkWrap_LoadLibFromStdPaths(true) )
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't to initialize SDK lib");
		QCoreApplication::exit(-1);
		return;
	}
#endif

	nRet = 0;
	PrlApi_InitEx(PARALLELS_API_VER, TestConfig::getApplicationMode(), TestConfig::getSdkInitFlags(), 0);
	EXECUTE_TESTS_SUITE(HandlesManipulationsTest)
	EXECUTE_TESTS_SUITE(PrlSrvManipulationsTest)
	EXECUTE_TESTS_SUITE(PrlVirtualNetworkTest)
	EXECUTE_TESTS_SUITE(JobsManipulationsTest)
	EXECUTE_TESTS_SUITE(PrlVmManipulationsTest)
	EXECUTE_TESTS_SUITE(PrlVmValidateConfigTest)
	EXECUTE_TESTS_SUITE(PrlVmDefaultConfigTest)
	EXECUTE_TESTS_SUITE(UserCallbackTest)
	EXECUTE_TESTS_SUITE(PrlVmDevManipulationsTest)
	EXECUTE_TESTS_SUITE(PrivateSituationsTest)
	EXECUTE_TESTS_SUITE(PrlApiBasicsTest)
	EXECUTE_TESTS_SUITE(PrlUserProfileTest)
	EXECUTE_TESTS_SUITE(PrlVmExecFunctionalityTest)
	EXECUTE_TESTS_SUITE(PrlExclusiveVmLockTest)
	EXECUTE_TESTS_SUITE(PrlVmUptimeTest)
#ifdef _LIN_
	if (TestConfig::isServerMode() &&
		CVzHelper::is_vz_running() &&
		0 == CVzHelper::init_lib())
	{
		EXECUTE_TESTS_SUITE(PrlNetworkShapingTest)
		// Run test for Container
		TestConfig::g_CtMode = true;
		EXECUTE_TESTS_SUITE(PrlVmManipulationsTest)
		TestConfig::g_CtMode = false;
		EXECUTE_TESTS_SUITE(PrlCtManipulationsTest);
	}
#endif
	EXECUTE_TESTS_SUITE(PrlSataDevicesHotPlugTest)
	EXECUTE_TESTS_SUITE(PrlUsbDevicesHotPlugTest)
	EXECUTE_TESTS_SUITE(PrlIPPrivateNetworkTest)
	EXECUTE_TESTS_SUITE(PrlAutoReportsTest)
	EXECUTE_TESTS_SUITE(DispFunctionalityTest);
	QCoreApplication::exit(nRet);
}

void DeinitSdk()
{
	// Note: Move PrlApi_Deinit before SdkWrap_Unload to know the handles' leakage
	PrlApi_Deinit();
	PRL_UINT32 nHandlesNum = 0;
	PrlDbg_GetHandlesNum(&nHandlesNum, PHT_ERROR);
	LOG_MESSAGE(DBG_FATAL, "Non cleaned handles instances %d", nHandlesNum);
	for (PRL_HANDLE_TYPE i = PHT_SERVER; i <= PHT_LAST; ++(unsigned int &)i)
		OUTPUT_HANDLES_NUM_FOR_TYPE(i);
#ifdef DYN_API_WRAP
	SdkWrap_Unload();
#endif
}

int main(int argc, char *argv[])
{
	TestConfig::readTestParameters();
	int ret = 0;
	QCoreApplication a(argc, argv);

	TestsExecuter _tests_executer(argc, argv);
	QTimer::singleShot(0, &_tests_executer, SLOT(PushTestsExecution()));
	ret = a.exec();
	//Proper way of deiniting SDK in case of reusing QCoreApplication to do it
	//after exit from exec() of main event loop
	//see https://bugzilla.sw.ru/show_bug.cgi?id=448820 for more information
	DeinitSdk();
	return (ret);
}

