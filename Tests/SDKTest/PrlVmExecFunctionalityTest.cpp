/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlVmExecFunctionalityTest.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing VM manipulating SDK API.
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

#include "Build/Current.ver"
#include "PrlVmExecFunctionalityTest.h"
#include "Tests/CommonTestsUtils.h"
#include "SimpleServerWrapper.h"
#include <prlcommon/Messaging/CVmEvent.h>
#include <prlcommon/Messaging/CVmEventParameter.h>
#include <prlcommon/Interfaces/ParallelsNamespaceTests.h>

#include <fcntl.h>
#ifdef _WIN_
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif

void PrlVmExecFunctionalityTest::testVmLoginInGuestByAdminUserOnNonOwnerAndNonHostAdministrator()
{
	SimpleServerWrapper _connection1(TestConfig::getUserLogin());
	SimpleServerWrapper _connection2(TestConfig::getUserLogin2());

	QVERIFY(_connection1.IsConnected());
	QVERIFY(_connection2.IsConnected());

	QVERIFY(_connection1.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_GetState(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	SdkHandleWrap hVmAccessRights;
	CHECK_RET_CODE_EXP(PrlVmInfo_GetAccessRights(hVmInfo, hVmAccessRights.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlAcl_SetAccessForOthers(hVmAccessRights, PAO_VM_SHARED_ON_VIEW_AND_RUN))

	hJob.reset(PrlVm_UpdateSecurity(_connection1.GetTestVm(), hVmAccessRights));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_Start(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection1.GetTestVm());

	CHECK_ASYNC_OP_FAILED(PrlVm_LoginInGuest(_connection2.GetTestVm(), PRL_PRIVILEGED_GUEST_OS_SESSION, 0, 0),\
		PRL_ERR_ONLY_ADMIN_OR_VM_OWNER_CAN_OPEN_THIS_SESSION)
}

void PrlVmExecFunctionalityTest::testVmLoginInGuestByCurrentActiveSessionOnNonOwnerAndNonHostAdministrator()
{
	SimpleServerWrapper _connection1(TestConfig::getUserLogin());
	SimpleServerWrapper _connection2(TestConfig::getUserLogin2());

	QVERIFY(_connection1.IsConnected());
	QVERIFY(_connection2.IsConnected());

	QVERIFY(_connection1.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_GetState(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	SdkHandleWrap hVmAccessRights;
	CHECK_RET_CODE_EXP(PrlVmInfo_GetAccessRights(hVmInfo, hVmAccessRights.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlAcl_SetAccessForOthers(hVmAccessRights, PAO_VM_SHARED_ON_VIEW_AND_RUN))

	hJob.reset(PrlVm_UpdateSecurity(_connection1.GetTestVm(), hVmAccessRights));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_Start(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection1.GetTestVm());

	CHECK_ASYNC_OP_FAILED(PrlVm_LoginInGuest(_connection2.GetTestVm(), PRL_CURRENT_GUEST_OS_SESSION, 0, 0),\
		PRL_ERR_ONLY_ADMIN_OR_VM_OWNER_CAN_OPEN_THIS_SESSION)
}

void PrlVmExecFunctionalityTest::testVmLoginInGuestByAdminUserOnVmOwner()
{
	SimpleServerWrapper _connection(TestConfig::getUserLogin());

	QVERIFY(_connection.IsConnected());

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	CHECK_ASYNC_OP_FAILED(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_PRIVILEGED_GUEST_OS_SESSION, 0, 0),\
		PRL_ERR_VM_EXEC_GUEST_TOOL_NOT_AVAILABLE)
}

void PrlVmExecFunctionalityTest::testVmLoginInGuestByCurrentActiveSessionOnVmOwner()
{
	SimpleServerWrapper _connection(TestConfig::getUserLogin());

	QVERIFY(_connection.IsConnected());

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	CHECK_ASYNC_OP_FAILED(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_CURRENT_GUEST_OS_SESSION, 0, 0),\
		PRL_ERR_VM_EXEC_GUEST_TOOL_NOT_AVAILABLE)
}

void PrlVmExecFunctionalityTest::testVmLoginInGuestByAdminUserOnHostAdministrator()
{
	SimpleServerWrapper _connection1(TestConfig::getUserLogin());
	SimpleServerWrapper _connection2(NULL);//Login local mode

	QVERIFY(_connection1.IsConnected());
	QVERIFY(_connection2.IsConnected());

	QVERIFY(_connection1.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_GetState(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	SdkHandleWrap hVmAccessRights;
	CHECK_RET_CODE_EXP(PrlVmInfo_GetAccessRights(hVmInfo, hVmAccessRights.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlAcl_SetAccessForOthers(hVmAccessRights, PAO_VM_SHARED_ON_VIEW))

	hJob.reset(PrlVm_UpdateSecurity(_connection1.GetTestVm(), hVmAccessRights));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_Start(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection1.GetTestVm());

	CHECK_ASYNC_OP_FAILED(PrlVm_LoginInGuest(_connection2.GetTestVm(), PRL_PRIVILEGED_GUEST_OS_SESSION, 0, 0),\
		PRL_ERR_VM_EXEC_GUEST_TOOL_NOT_AVAILABLE)
}

void PrlVmExecFunctionalityTest::testVmLoginInGuestByCurrentActiveSessionOnHostAdministrator()
{
	SimpleServerWrapper _connection1(TestConfig::getUserLogin());
	SimpleServerWrapper _connection2(NULL);//Login local mode

	QVERIFY(_connection1.IsConnected());
	QVERIFY(_connection2.IsConnected());

	QVERIFY(_connection1.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_GetState(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	SdkHandleWrap hVmAccessRights;
	CHECK_RET_CODE_EXP(PrlVmInfo_GetAccessRights(hVmInfo, hVmAccessRights.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlAcl_SetAccessForOthers(hVmAccessRights, PAO_VM_SHARED_ON_VIEW))

	hJob.reset(PrlVm_UpdateSecurity(_connection1.GetTestVm(), hVmAccessRights));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_Start(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection1.GetTestVm());

	CHECK_ASYNC_OP_FAILED(PrlVm_LoginInGuest(_connection2.GetTestVm(), PRL_CURRENT_GUEST_OS_SESSION, 0, 0),\
		PRL_ERR_VM_EXEC_GUEST_TOOL_NOT_AVAILABLE)
}

void PrlVmExecFunctionalityTest::testVmLoginInGuestByCustomCredsOnUserWithInsufficientRights()
{
	SimpleServerWrapper _connection1(TestConfig::getUserLogin());
	SimpleServerWrapper _connection2(TestConfig::getUserLogin2());

	QVERIFY(_connection1.IsConnected());
	QVERIFY(_connection2.IsConnected());

	QVERIFY(_connection1.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_GetState(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	SdkHandleWrap hVmAccessRights;
	CHECK_RET_CODE_EXP(PrlVmInfo_GetAccessRights(hVmInfo, hVmAccessRights.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlAcl_SetAccessForOthers(hVmAccessRights, PAO_VM_SHARED_ON_VIEW))

	hJob.reset(PrlVm_UpdateSecurity(_connection1.GetTestVm(), hVmAccessRights));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_Start(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	stop_vm_on_exit stop_vm(_connection1.GetTestVm());

	CHECK_ASYNC_OP_FAILED(PrlVm_LoginInGuest(_connection2.GetTestVm(), "some user", "some password", 0),\
			PRL_ERR_ACCESS_DENIED)
}

void PrlVmExecFunctionalityTest::testVmLoginInGuestByCustomCredsOnUserWithInsufficientRights2()
{
	SimpleServerWrapper _connection1(TestConfig::getUserLogin());
	SimpleServerWrapper _connection2(TestConfig::getUserLogin2());

	QVERIFY(_connection1.IsConnected());
	QVERIFY(_connection2.IsConnected());

	QVERIFY(_connection1.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_GetState(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	SdkHandleWrap hVmAccessRights;
	CHECK_RET_CODE_EXP(PrlVmInfo_GetAccessRights(hVmInfo, hVmAccessRights.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlAcl_SetAccessForOthers(hVmAccessRights, PAO_VM_SHARED_ON_VIEW_AND_RUN))

	hJob.reset(PrlVm_UpdateSecurity(_connection1.GetTestVm(), hVmAccessRights));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_Start(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	stop_vm_on_exit stop_vm(_connection1.GetTestVm());

	CHECK_ASYNC_OP_FAILED(PrlVm_LoginInGuest(_connection2.GetTestVm(), "some user", "some password", 0),\
		PRL_ERR_ACCESS_DENIED)
}

void PrlVmExecFunctionalityTest::testVmLoginInGuestByCustomCredsOnUserWithSufficientRights()
{
	SimpleServerWrapper _connection1(TestConfig::getUserLogin());
	SimpleServerWrapper _connection2(TestConfig::getUserLogin2());

	QVERIFY(_connection1.IsConnected());
	QVERIFY(_connection2.IsConnected());

	QVERIFY(_connection1.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_GetState(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	SdkHandleWrap hVmAccessRights;
	CHECK_RET_CODE_EXP(PrlVmInfo_GetAccessRights(hVmInfo, hVmAccessRights.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlAcl_SetAccessForOthers(hVmAccessRights, PAO_VM_SHARED_ON_FULL_ACCESS))

	hJob.reset(PrlVm_UpdateSecurity(_connection1.GetTestVm(), hVmAccessRights));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_Start(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection1.GetTestVm());

	CHECK_ASYNC_OP_FAILED(PrlVm_LoginInGuest(_connection2.GetTestVm(), "some user", "some password", 0),\
		PRL_ERR_VM_EXEC_GUEST_TOOL_NOT_AVAILABLE)
}

void PrlVmExecFunctionalityTest::testVmLoginInGuestByCustomCredsOnVmOwner()
{
	SimpleServerWrapper _connection(TestConfig::getUserLogin());

	QVERIFY(_connection.IsConnected());

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	CHECK_ASYNC_OP_FAILED(PrlVm_LoginInGuest(_connection.GetTestVm(), "some user", "some password", 0),\
		PRL_ERR_VM_EXEC_GUEST_TOOL_NOT_AVAILABLE)
}

void PrlVmExecFunctionalityTest::testVmLoginInGuestByCustomCredsOnHostAdministrator()
{
	SimpleServerWrapper _connection1(TestConfig::getUserLogin());
	SimpleServerWrapper _connection2(NULL);//Login local mode

	QVERIFY(_connection1.IsConnected());
	QVERIFY(_connection2.IsConnected());

	QVERIFY(_connection1.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_GetState(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	SdkHandleWrap hVmAccessRights;
	CHECK_RET_CODE_EXP(PrlVmInfo_GetAccessRights(hVmInfo, hVmAccessRights.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlAcl_SetAccessForOthers(hVmAccessRights, PAO_VM_SHARED_ON_VIEW))

	hJob.reset(PrlVm_UpdateSecurity(_connection1.GetTestVm(), hVmAccessRights));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_Start(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection1.GetTestVm());

	CHECK_ASYNC_OP_FAILED(PrlVm_LoginInGuest(_connection2.GetTestVm(), "some user", "some password", 0),\
		PRL_ERR_VM_EXEC_GUEST_TOOL_NOT_AVAILABLE)
}

void PrlVmExecFunctionalityTest::testVmLoginInGuestOnNullVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_LoginInGuest(PRL_INVALID_HANDLE, "some user", "some password", 0),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmExecFunctionalityTest::testVmLoginInGuestOnNonVmHandle()
{
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVm_LoginInGuest(hServer, "some user", "some password", 0),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmExecFunctionalityTest::testVmLoginInGuestOnNullUserLogin()
{
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVm_LoginInGuest(hServer, 0, "some password", 0),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmExecFunctionalityTest::testVmLoginInGuestOnEmptyUserLogin()
{
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVm_LoginInGuest(hServer, "", "some password", 0),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmExecFunctionalityTest::testVmGuestLogoutOnNullVmSessionHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVmGuest_Logout(PRL_INVALID_HANDLE, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmExecFunctionalityTest::testVmGuestLogoutOnNonVmSessionHandle()
{
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVmGuest_Logout(hServer, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmExecFunctionalityTest::testVmGuestLogoutOnFakeGuestSession()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmGuest;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

	//Try another one method of result extraction
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, hVmGuest.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

	hJob.reset(PrlVmGuest_Logout(hVmGuest, 0));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlVmExecFunctionalityTest::testVmGuestLogoutTryToLogoutOnAlreadyClosedSession()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmGuest;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

	hJob.reset(PrlVmGuest_Logout(hVmGuest, 0));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_ASYNC_OP_FAILED(PrlVmGuest_Logout(hVmGuest, 0), PRL_ERR_VM_GUEST_SESSION_EXPIRED)
}

void PrlVmExecFunctionalityTest::testVmGuestLogoutTryToUseSessionAfterLostConnection()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmGuest;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

	QVERIFY(_connection.Logoff());
	QVERIFY(_connection.Login(0));

	CHECK_ASYNC_OP_FAILED(PrlVmGuest_Logout(hVmGuest, 0), PRL_ERR_VM_GUEST_SESSION_EXPIRED)
}

void PrlVmExecFunctionalityTest::testLoginInGuestOnStoppedVm()
{
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	CHECK_ASYNC_OP_FAILED(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0),\
			PRL_ERR_DISP_VM_IS_NOT_STARTED)
}

void PrlVmExecFunctionalityTest::testVmGuestLogoutOnStoppedVm()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hVmGuest;
	{
		stop_vm_on_exit stop_vm(_connection.GetTestVm());

		hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
		CHECK_JOB_RET_CODE(hJob)

		SdkHandleWrap hResult;
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
		CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
		CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)
	}

	CHECK_ASYNC_OP_FAILED(PrlVmGuest_Logout(hVmGuest, 0), PRL_ERR_DISP_VM_IS_NOT_STARTED)
}

void PrlVmExecFunctionalityTest::testVmGuestRunProgramOnOrdinalCase()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmGuest;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

	hJob.reset(PrlVmGuest_RunProgram(hVmGuest, "exists_app_name", PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, PFD_ALL,
									PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	PRL_UINT32 nParamsCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))
	QCOMPARE(quint32(nParamsCount), quint32(1));

	QString sParamValue;
	PRL_EXTRACT_STRING_VALUE(sParamValue, hResult, PrlResult_GetParamAsString)

	CVmEvent _result(sParamValue);
	CHECK_RET_CODE_EXP(_result.m_uiRcInit)

	CVmEventParameter *pParam = _result.getEventParameter(EVT_PARAM_VM_EXEC_APP_RET_CODE);
	QVERIFY(pParam);
	QCOMPARE(quint32(pParam->getParamValue().toUInt()), quint32(0));

	pParam = _result.getEventParameter(EVT_PARAM_VM_EXEC_STDOUT);
	QVERIFY(pParam);
	QCOMPARE(pParam->getParamValue(), QString("stdout value"));

	pParam = _result.getEventParameter(EVT_PARAM_VM_EXEC_STDERR);
	QVERIFY(pParam);
	QCOMPARE(pParam->getParamValue(), QString("stderr value"));

	hJob.reset(PrlVmGuest_Logout(hVmGuest, 0));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlVmExecFunctionalityTest::testVmGuestRunProgramOnOrdinalCase2()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmGuest;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

	hJob.reset(PrlVmGuest_RunProgram(hVmGuest, "exists_app_name", PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, PFD_ALL,
									PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	PRL_UINT32 nParamsCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))
	QCOMPARE(quint32(nParamsCount), quint32(1));

	SdkHandleWrap hVmEvent;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmEvent.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmEvent, PHT_EVENT)

	//Check another way of extracting handle from result
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, hVmEvent.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmEvent, PHT_EVENT)

	SdkHandleWrap hRetCodeParam;
	CHECK_RET_CODE_EXP(PrlEvent_GetParamByName(hVmEvent, EVT_PARAM_VM_EXEC_APP_RET_CODE, hRetCodeParam.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hRetCodeParam, PHT_EVENT_PARAMETER)
	PRL_UINT32 nAppRetCode = 0xffffffff;
	CHECK_RET_CODE_EXP(PrlEvtPrm_ToUint32(hRetCodeParam, &nAppRetCode))
	QCOMPARE(quint32(nAppRetCode), quint32(0));

	SdkHandleWrap hStdoutParam;
	CHECK_RET_CODE_EXP(PrlEvent_GetParamByName(hVmEvent, EVT_PARAM_VM_EXEC_STDOUT, hStdoutParam.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hStdoutParam, PHT_EVENT_PARAMETER)
	QString sActualStdout;
	PRL_EXTRACT_STRING_VALUE(sActualStdout, hStdoutParam, PrlEvtPrm_ToString)
	QCOMPARE(sActualStdout, QString("stdout value"));

	SdkHandleWrap hStderrParam;
	CHECK_RET_CODE_EXP(PrlEvent_GetParamByName(hVmEvent, EVT_PARAM_VM_EXEC_STDERR, hStderrParam.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hStderrParam, PHT_EVENT_PARAMETER)
	QString sActualStderr;
	PRL_EXTRACT_STRING_VALUE(sActualStderr, hStderrParam, PrlEvtPrm_ToString)
	QCOMPARE(sActualStderr, QString("stderr value"));

	hJob.reset(PrlVmGuest_Logout(hVmGuest, 0));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlVmExecFunctionalityTest::testVmGuestRunProgramOnJustStdoutRequested()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmGuest;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

	hJob.reset(PrlVmGuest_RunProgram(hVmGuest, "exists_app_name", PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, PFD_STDOUT,
									PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	PRL_UINT32 nParamsCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))
	QCOMPARE(quint32(nParamsCount), quint32(1));

	QString sParamValue;
	PRL_EXTRACT_STRING_VALUE(sParamValue, hResult, PrlResult_GetParamAsString)

	CVmEvent _result(sParamValue);
	CHECK_RET_CODE_EXP(_result.m_uiRcInit)

	CVmEventParameter *pParam = _result.getEventParameter(EVT_PARAM_VM_EXEC_APP_RET_CODE);
	QVERIFY(pParam);
	QCOMPARE(quint32(pParam->getParamValue().toUInt()), quint32(0));

	pParam = _result.getEventParameter(EVT_PARAM_VM_EXEC_STDOUT);
	QVERIFY(pParam);
	QCOMPARE(pParam->getParamValue(), QString("stdout value"));

	pParam = _result.getEventParameter(EVT_PARAM_VM_EXEC_STDERR);
	QVERIFY(!pParam);

	hJob.reset(PrlVmGuest_Logout(hVmGuest, 0));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlVmExecFunctionalityTest::testVmGuestRunProgramOnJustStderrRequested()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmGuest;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

	hJob.reset(PrlVmGuest_RunProgram(hVmGuest, "exists_app_name", PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, PFD_STDERR,
									PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	PRL_UINT32 nParamsCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))
	QCOMPARE(quint32(nParamsCount), quint32(1));

	QString sParamValue;
	PRL_EXTRACT_STRING_VALUE(sParamValue, hResult, PrlResult_GetParamAsString)

	CVmEvent _result(sParamValue);
	CHECK_RET_CODE_EXP(_result.m_uiRcInit)

	CVmEventParameter *pParam = _result.getEventParameter(EVT_PARAM_VM_EXEC_APP_RET_CODE);
	QVERIFY(pParam);
	QCOMPARE(quint32(pParam->getParamValue().toUInt()), quint32(0));

	pParam = _result.getEventParameter(EVT_PARAM_VM_EXEC_STDOUT);
	QVERIFY(!pParam);

	pParam = _result.getEventParameter(EVT_PARAM_VM_EXEC_STDERR);
	QVERIFY(pParam);
	QCOMPARE(pParam->getParamValue(), QString("stderr value"));

	hJob.reset(PrlVmGuest_Logout(hVmGuest, 0));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlVmExecFunctionalityTest::testVmGuestRunProgramOnNoOutputRequested()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmGuest;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

	hJob.reset(PrlVmGuest_RunProgram(hVmGuest, "exists_app_name", PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, 0,
									PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	PRL_UINT32 nParamsCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))
	QCOMPARE(quint32(nParamsCount), quint32(1));

	QString sParamValue;
	PRL_EXTRACT_STRING_VALUE(sParamValue, hResult, PrlResult_GetParamAsString)

	CVmEvent _result(sParamValue);
	CHECK_RET_CODE_EXP(_result.m_uiRcInit)

	CVmEventParameter *pParam = _result.getEventParameter(EVT_PARAM_VM_EXEC_APP_RET_CODE);
	QVERIFY(pParam);
	QCOMPARE(quint32(pParam->getParamValue().toUInt()), quint32(0));

	pParam = _result.getEventParameter(EVT_PARAM_VM_EXEC_STDOUT);
	QVERIFY(pParam);
	QCOMPARE(pParam->getParamValue(), QString("stdout value"));

	pParam = _result.getEventParameter(EVT_PARAM_VM_EXEC_STDERR);
	QVERIFY(pParam);
	QCOMPARE(pParam->getParamValue(), QString("stderr value"));

	hJob.reset(PrlVmGuest_Logout(hVmGuest, 0));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlVmExecFunctionalityTest::testVmGuestRunProgramOnJustProgramStartRequested()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmGuest;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

	hJob.reset(PrlVmGuest_RunProgram(hVmGuest, "exists_app_name", PRL_INVALID_HANDLE, PRL_INVALID_HANDLE,
				PRPM_RUN_PROGRAM_AND_RETURN_IMMEDIATELY, PRL_INVALID_FILE_DESCRIPTOR,
				PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	PRL_UINT32 nParamsCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))
	QCOMPARE(quint32(nParamsCount), quint32(0));

	hJob.reset(PrlVmGuest_Logout(hVmGuest, 0));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlVmExecFunctionalityTest::testVmGuestRunProgramOnStoppedVm()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hVmGuest;
	{
		stop_vm_on_exit stop_vm(_connection.GetTestVm());

		hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
		CHECK_JOB_RET_CODE(hJob)

		SdkHandleWrap hResult;
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
		CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
		CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)
	}

	CHECK_ASYNC_OP_FAILED(PrlVmGuest_RunProgram(hVmGuest, "exists_app_name", PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, 0,\
									PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR),\
		PRL_ERR_DISP_VM_IS_NOT_STARTED)
}

void PrlVmExecFunctionalityTest::testVmGuestRunProgramOnInvalidVmGuestHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVmGuest_RunProgram(PRL_INVALID_HANDLE, "exists_app_name", PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, 0,\
									PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmExecFunctionalityTest::testVmGuestRunProgramOnNonVmGuestHandle()
{
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVmGuest_RunProgram(hServer, "exists_app_name", PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, 0,\
									PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmExecFunctionalityTest::testVmGuestRunProgramOnNullAppString()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmGuest;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

	CHECK_ASYNC_OP_FAILED(PrlVmGuest_RunProgram(hVmGuest, 0, PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, 0,\
				PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR),\
		PRL_ERR_INVALID_ARG)

	hJob.reset(PrlVmGuest_Logout(hVmGuest, 0));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlVmExecFunctionalityTest::testVmGuestRunProgramOnEmptyAppString()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmGuest;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

	CHECK_ASYNC_OP_FAILED(PrlVmGuest_RunProgram(hVmGuest, "", PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, 0,\
				PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR),\
		PRL_ERR_INVALID_ARG)

	hJob.reset(PrlVmGuest_Logout(hVmGuest, 0));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlVmExecFunctionalityTest::testVmGuestRunProgramOnNonStringsListHandleForArgs()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmGuest;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

	CHECK_ASYNC_OP_FAILED(PrlVmGuest_RunProgram(hVmGuest, "exists_app_name", hResult, PRL_INVALID_HANDLE, 0,\
				PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR),\
		PRL_ERR_INVALID_ARG)

	hJob.reset(PrlVmGuest_Logout(hVmGuest, 0));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlVmExecFunctionalityTest::testVmGuestRunProgramOnNonStringsListHandleForEnvs()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmGuest;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

	CHECK_ASYNC_OP_FAILED(PrlVmGuest_RunProgram(hVmGuest, "exists_app_name", PRL_INVALID_HANDLE, hVmGuest, 0,\
				PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR),\
		PRL_ERR_INVALID_ARG)

	hJob.reset(PrlVmGuest_Logout(hVmGuest, 0));
	CHECK_JOB_RET_CODE(hJob)
}

#ifndef _WIN_
#define STDIN_FILE_DESC fileno(stdin)
#define STDOUT_FILE_DESC fileno(stdout)
#define STDERR_FILE_DESC fileno(stderr)
#else
#define STDIN_FILE_DESC  GetStdHandle(STD_INPUT_HANDLE)
#define STDOUT_FILE_DESC GetStdHandle(STD_OUTPUT_HANDLE)
#define STDERR_FILE_DESC GetStdHandle(STD_ERROR_HANDLE)
#endif

void PrlVmExecFunctionalityTest::testVmGuestRunProgramOnFileDescriptorsUsageIoChannelAbsent()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmGuest;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

	CHECK_ASYNC_OP_FAILED(PrlVmGuest_RunProgram(hVmGuest, "exists_app_name", PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, 0,\
				STDIN_FILE_DESC, PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR),\
		PRL_ERR_UNINITIALIZED)
	CHECK_ASYNC_OP_FAILED(PrlVmGuest_RunProgram(hVmGuest, "exists_app_name", PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, 0,\
				PRL_INVALID_FILE_DESCRIPTOR, STDOUT_FILE_DESC, PRL_INVALID_FILE_DESCRIPTOR),\
		PRL_ERR_UNINITIALIZED)
	CHECK_ASYNC_OP_FAILED(PrlVmGuest_RunProgram(hVmGuest, "exists_app_name", PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, 0,\
				PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR, STDERR_FILE_DESC),\
		PRL_ERR_UNINITIALIZED)

	hJob.reset(PrlVmGuest_Logout(hVmGuest, 0));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlVmExecFunctionalityTest::testVmGuestRunProgramCheckPassedArgsAndEnvs()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	QStringList _args_list = QStringList()<<"arg1"<<"arg 2"<<"arg3 3 3";
	QStringList _envs_list = QStringList()<<"env1=value1"<<"env2=value2"<<"env3=value3";

	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmGuest;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

	SdkHandleWrap hArgsList, hEnvsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hArgsList.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hEnvsList.GetHandlePtr()))

	foreach(QString sArg, _args_list)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hArgsList, sArg.toUtf8().constData()))

	foreach(QString sEnv, _envs_list)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hEnvsList, sEnv.toUtf8().constData()))

	hJob.reset(PrlVmGuest_RunProgram(hVmGuest, "args_and_envs_print_app", hArgsList, hEnvsList, PFD_ALL,
							PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	PRL_UINT32 nParamsCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))
	QCOMPARE(quint32(nParamsCount), quint32(1));

	QString sParamValue;
	PRL_EXTRACT_STRING_VALUE(sParamValue, hResult, PrlResult_GetParamAsString)

	CVmEvent _result(sParamValue);
	CHECK_RET_CODE_EXP(_result.m_uiRcInit)

	CVmEventParameter *pParam = _result.getEventParameter(EVT_PARAM_VM_EXEC_STDOUT);
	QVERIFY(pParam);
	foreach(QString sArg, _args_list)
		QVERIFY(pParam->getParamValue().contains(sArg));

	pParam = _result.getEventParameter(EVT_PARAM_VM_EXEC_STDERR);
	QVERIFY(pParam);
	foreach(QString sEnv, _envs_list)
		QVERIFY(pParam->getParamValue().contains(sEnv));

	hJob.reset(PrlVmGuest_Logout(hVmGuest, 0));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlVmExecFunctionalityTest::testVmGuestRunProgramOnNonExistsAppName()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmGuest;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

	CHECK_ASYNC_OP_FAILED(PrlVmGuest_RunProgram(hVmGuest, "non_exists_app_name", PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, 0,\
				PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR),\
		PRL_ERR_VM_EXEC_PROGRAM_NOT_FOUND)

	hJob.reset(PrlVmGuest_Logout(hVmGuest, 0));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlVmExecFunctionalityTest::testVmGuestGetNetworkSettings()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmGuest;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

	hJob.reset(PrlVmGuest_GetNetworkSettings(hVmGuest, 0));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hSrvConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hSrvConfig.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hSrvConfig, PHT_SERVER_CONFIG)

//Check common network settings
	QString sHostname;
	PRL_EXTRACT_STRING_VALUE(sHostname, hSrvConfig, PrlSrvCfg_GetHostname)
	QCOMPARE(sHostname, QString("test hostname"));

	QStringList lstExpectedDnsServers = QStringList()<<"192.168.1.254"<<"10.30.8.254";
	QStringList lstExpectedSearchDomains = QStringList()<<"sw.ru"<<"parallels.com";
	QStringList lstExpectedNetAddresses = QStringList()<<"192.168.1.1/255.255.255.0"<<"10.30.8.245/255.255.255.0";

	PRL_CHECK_STRINGS_LIST(hSrvConfig, PrlSrvCfg_GetDnsServers, lstExpectedDnsServers)
	PRL_CHECK_STRINGS_LIST(hSrvConfig, PrlSrvCfg_GetSearchDomains, lstExpectedSearchDomains)

//Check network adapter settings
	PRL_UINT32 nNetAdaptersCount = 0;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetNetAdaptersCount(hSrvConfig, &nNetAdaptersCount))
	QCOMPARE(quint32(nNetAdaptersCount), quint32(1));

	SdkHandleWrap hNetAdapter;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetNetAdapter(hSrvConfig, 0, hNetAdapter.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hNetAdapter, PHT_HW_NET_ADAPTER)

	QString sNetAdapterName;
	PRL_EXTRACT_STRING_VALUE(sNetAdapterName, hNetAdapter, PrlSrvCfgDev_GetName)
	QCOMPARE(sNetAdapterName, QString("eth0"));

	QString sDefaultGateway;
	PRL_EXTRACT_STRING_VALUE(sDefaultGateway, hNetAdapter, PrlSrvCfgNet_GetDefaultGateway)
	QCOMPARE(sDefaultGateway, QString(TEST_DEFAULT_GATEWAY));

	PRL_EXTRACT_STRING_VALUE(sDefaultGateway, hSrvConfig, PrlSrvCfg_GetDefaultGateway)
	QCOMPARE(sDefaultGateway, QString(TEST_DEFAULT_GATEWAY));

	QString sDefaultGatewayIPv6;
	PRL_EXTRACT_STRING_VALUE(sDefaultGatewayIPv6, hNetAdapter, PrlSrvCfgNet_GetDefaultGatewayIPv6)
	QCOMPARE(sDefaultGatewayIPv6, QString(TEST_DEFAULT_GATEWAY_IPv6));

	PRL_EXTRACT_STRING_VALUE(sDefaultGatewayIPv6, hSrvConfig, PrlSrvCfg_GetDefaultGatewayIPv6)
	QCOMPARE(sDefaultGatewayIPv6, QString(TEST_DEFAULT_GATEWAY_IPv6));

	QString sMacAddress;
	PRL_EXTRACT_STRING_VALUE(sMacAddress, hNetAdapter, PrlSrvCfgNet_GetMacAddress)
	QCOMPARE(sMacAddress, QString(TEST_MAC_ADDR));

	PRL_UINT16 nVlanTag = 0;
	CHECK_RET_CODE_EXP(PrlSrvCfgNet_GetVlanTag(hNetAdapter, &nVlanTag))
	QCOMPARE(quint32(nVlanTag), quint32(TEST_VLAN_TAG));
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfgNet_GetVlanTag(hNetAdapter, 0), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfgNet_GetVlanTag(hSrvConfig, &nVlanTag), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfgNet_GetVlanTag(PRL_INVALID_HANDLE, &nVlanTag), PRL_ERR_INVALID_ARG)

	PRL_BOOL bIsConfigureWithDhcp = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlSrvCfgNet_IsConfigureWithDhcp(hNetAdapter, &bIsConfigureWithDhcp))
	QVERIFY(PRL_FALSE == bIsConfigureWithDhcp);

	PRL_BOOL bIsConfigureWithDhcpIPv6 = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlSrvCfgNet_IsConfigureWithDhcpIPv6(hNetAdapter, &bIsConfigureWithDhcpIPv6))
	QVERIFY(PRL_FALSE == bIsConfigureWithDhcpIPv6);

	PRL_CHECK_STRINGS_LIST(hNetAdapter, PrlSrvCfgNet_GetNetAddresses, lstExpectedNetAddresses)
	PRL_CHECK_STRINGS_LIST(hNetAdapter, PrlSrvCfgNet_GetDnsServers, lstExpectedDnsServers)
	PRL_CHECK_STRINGS_LIST(hNetAdapter, PrlSrvCfgNet_GetSearchDomains, lstExpectedSearchDomains)
}

void PrlVmExecFunctionalityTest::testVmGuestGetNetworkSettingsOnStoppedVm()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hVmGuest;
	{
		stop_vm_on_exit stop_vm(_connection.GetTestVm());

		hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
		CHECK_JOB_RET_CODE(hJob)

		SdkHandleWrap hResult;
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
		CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
		CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)
	}

	CHECK_ASYNC_OP_FAILED(PrlVmGuest_GetNetworkSettings(hVmGuest, 0), PRL_ERR_DISP_VM_IS_NOT_STARTED)
}

void PrlVmExecFunctionalityTest::testVmGuestGetNetworkSettingsOnInvalidVmGuestHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVmGuest_GetNetworkSettings(PRL_INVALID_HANDLE, 0),	PRL_ERR_INVALID_ARG)
}

void PrlVmExecFunctionalityTest::testVmGuestGetNetworkSettingsOnNonVmGuestHandle()
{
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVmGuest_GetNetworkSettings(hServer, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmExecFunctionalityTest::testVmGuestSetUserPasswd()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmGuest;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

	hJob.reset(PrlVmGuest_SetUserPasswd(hVmGuest, "valid_user", "new_password", 0));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlVmExecFunctionalityTest::testVmGuestSetUserPasswdOnStoppedVm()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hVmGuest;
	{
		stop_vm_on_exit stop_vm(_connection.GetTestVm());

		hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
		CHECK_JOB_RET_CODE(hJob)

		SdkHandleWrap hResult;
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
		CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
		CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)
	}

	CHECK_ASYNC_OP_FAILED(PrlVmGuest_SetUserPasswd(hVmGuest, 0, 0, 0), PRL_ERR_DISP_VM_IS_NOT_STARTED)
}

void PrlVmExecFunctionalityTest::testVmGuestSetUserPasswdOnInvalidVmGuestHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVmGuest_SetUserPasswd(PRL_INVALID_HANDLE, 0, 0, 0),	PRL_ERR_INVALID_ARG)
}

void PrlVmExecFunctionalityTest::testVmGuestSetUserPasswdOnNonVmGuestHandle()
{
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVmGuest_SetUserPasswd(hServer, 0, 0, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmExecFunctionalityTest::testVmGuestSetUserPasswdOnNonKnownUserName()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	{
		stop_vm_on_exit stop_vm(_connection.GetTestVm());

		hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
		CHECK_JOB_RET_CODE(hJob)

		SdkHandleWrap hResult;
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

		SdkHandleWrap hVmGuest;
		CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
		CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

		hJob.reset(PrlVmGuest_SetUserPasswd(hVmGuest, "unknown_user", "new_password", 0));
		CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT))

		SdkHandleWrap hError;
		CHECK_RET_CODE_EXP(PrlJob_GetError(hJob, hError.GetHandlePtr()))
		PRL_RESULT nErrorCode = PRL_ERR_UNINITIALIZED;
		CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hError, &nErrorCode))
		CHECK_CONCRETE_EXPRESSION_RET_CODE(nErrorCode, PRL_ERR_GUEST_PROGRAM_EXECUTION_FAILED)

		QByteArray _buffer;
		PRL_UINT32 nBufLength = 0;
		CHECK_RET_CODE_EXP(PrlEvent_GetErrString(hError, PRL_FALSE, PRL_FALSE, 0, &nBufLength))
		QVERIFY(nBufLength != 0);
		_buffer.resize(nBufLength);
		CHECK_RET_CODE_EXP(PrlEvent_GetErrString(hError, PRL_FALSE, PRL_FALSE, _buffer.data(), &nBufLength))

		QString sError = UTF8_2QSTR(_buffer);
		QVERIFY(sError.contains("1301"));
		QVERIFY(sError.contains("Some mapping between account names and security IDs was not done."));
	}
}

namespace {
#ifndef _WIN_
void prl_read_output(QString &sBuffer, int file_desc, size_t data_size)
{
	char nChar = 0;
	int nCharNums = read(file_desc, &nChar, sizeof(nChar));
	const PRL_UINT32 nMaxAttempts = 30;
	PRL_UINT32 nCurrAttempt = 0;
	while (size_t(sBuffer.toUtf8().size()) != data_size && nCurrAttempt < nMaxAttempts)
	{
		while (nCharNums == sizeof(nChar))
		{
			sBuffer += nChar;
			nCharNums = read(file_desc, &nChar, sizeof(nChar));
		}
		nCurrAttempt++;
		QTest::qSleep(1000);
	}
}
#else
void prl_read_output(QString &sBuffer, HANDLE file_desc, size_t data_size)
{
	char nChar = 0;
	DWORD nCharNums = 0;
	if (PeekNamedPipe(file_desc, &nChar, sizeof(nChar), &nCharNums, 0, 0) && nCharNums)
		ReadFile(file_desc, &nChar, sizeof(nChar), &nCharNums, 0);
	const PRL_UINT32 nMaxAttempts = 30;
	PRL_UINT32 nCurrAttempt = 0;
	while (sBuffer.toUtf8().size() != data_size && nCurrAttempt < nMaxAttempts)
	{
		while (nCharNums == sizeof(nChar))
		{
			sBuffer += nChar;
			if (PeekNamedPipe(file_desc, &nChar, sizeof(nChar), &nCharNums, 0, 0) && nCharNums)
				ReadFile(file_desc, &nChar, sizeof(nChar), &nCharNums, 0);
		}
		nCurrAttempt++;
		QTest::qSleep(1000);
	}
}
#endif

#ifndef _WIN_
void prl_close_pipe(int *pipe_pair)
{
	QVERIFY(close(pipe_pair[0]) == 0);
	QVERIFY(close(pipe_pair[1]) == 0);
}
#else
void prl_close_pipe(HANDLE *pipe_pair)
{
	QVERIFY(CloseHandle(pipe_pair[0]));
	QVERIFY(CloseHandle(pipe_pair[1]));
}
#endif

}//namespace

void PrlVmExecFunctionalityTest::testVmGuestRunProgramUsingFileDescriptors()
{
#ifdef EXTERNALLY_AVAILABLE_BUILD
		QSKIP("Skipping due fake stub not presents at external builds", SkipAll);
#endif
	QString sTestData = Uuid::createUuid().toString();

	SimpleServerWrapper _connection(0);//Login local case

	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

    stop_vm_on_exit stop_vm(_connection.GetTestVm());

	hJob.reset(PrlVm_LoginInGuest(_connection.GetTestVm(), PRL_TEST_FAKE_GUEST_SESSION, "123", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmGuest;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmGuest, PHT_VM_GUEST_SESSION)

	SdkHandleWrap hArgsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hArgsList.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlStrList_AddItem(hArgsList, sTestData.toUtf8().constData()))

#ifndef _WIN_
	int stdin_pipe[2];
	int stdout_pipe[2];
	int stderr_pipe[2];
#else
	HANDLE stdin_pipe[2];
	HANDLE stdout_pipe[2];
	HANDLE stderr_pipe[2];
#endif

#ifndef _WIN_
	QVERIFY(pipe(stdin_pipe) == 0);
	QVERIFY(pipe(stdout_pipe) == 0);
	QVERIFY(pipe(stderr_pipe) == 0);
	QVERIFY(fcntl(stdout_pipe[0], F_SETFL, O_NONBLOCK) == 0);
	QVERIFY(fcntl(stderr_pipe[0], F_SETFL, O_NONBLOCK) == 0);
#else
	QVERIFY(CreatePipe(&stdin_pipe[0], &stdin_pipe[1], 0, 0));
	QVERIFY(CreatePipe(&stdout_pipe[0], &stdout_pipe[1], 0, 0));
	QVERIFY(CreatePipe(&stderr_pipe[0], &stderr_pipe[1], 0, 0));
#endif

	hJob.reset(PrlVmGuest_RunProgram(hVmGuest, "test_file_descriptors_app", hArgsList, PRL_INVALID_HANDLE, PFD_ALL, stdin_pipe[0], stdout_pipe[1], stderr_pipe[1]));
	QByteArray sTestDataUtf8 = sTestData.toUtf8();
#ifndef _WIN_
	QVERIFY(write(stdin_pipe[1], sTestDataUtf8.constData(), sTestDataUtf8.size() ) == sTestDataUtf8.size());
#else
	DWORD nWrittenBytesNum = 0;
	WriteFile(stdin_pipe[1], sTestDataUtf8.constData(), sTestDataUtf8.size(), &nWrittenBytesNum, 0);
	QVERIFY(nWrittenBytesNum == sTestDataUtf8.size());
#endif
	CHECK_JOB_RET_CODE(hJob)

	QString sStdout, sStderr;

	prl_read_output(sStdout, stdout_pipe[0], sTestDataUtf8.size());
	prl_read_output(sStderr, stderr_pipe[0], sTestDataUtf8.size());

	prl_close_pipe(stdin_pipe);
	prl_close_pipe(stdout_pipe);
	prl_close_pipe(stderr_pipe);

	QVERIFY(sStdout.contains(sTestData));
	QVERIFY(sStderr.contains(sTestData));

	hJob.reset(PrlVmGuest_Logout(hVmGuest, 0));
	CHECK_JOB_RET_CODE(hJob)
}

