/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlExclusiveVmLockTest.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing VM exclusive lock functionality.
///
///	@author sandro
///
/// Copyright (c) 1999-2015 Parallels IP Holdings GmbH
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

#include "PrlExclusiveVmLockTest.h"
#include "SimpleServerWrapper.h"

void PrlExclusiveVmLockTest::testVmLockUnlock()
{
	SimpleServerWrapper _connection1, _connection2;
	QVERIFY(_connection1.IsConnected());
	QVERIFY(_connection2.IsConnected());
	QVERIFY(_connection1.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Lock(_connection1.GetTestVm(), 0));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_ASYNC_OP_FAILED(PrlVm_BeginEdit(_connection2.GetTestVm()), PRL_ERR_VM_IS_EXCLUSIVELY_LOCKED)

	hJob.reset(PrlVm_GetState(_connection2.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))

	PRL_BOOL bVmIsInvalid = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlVmInfo_IsInvalid(hVmInfo, &bVmIsInvalid))
	QVERIFY(PRL_TRUE == bVmIsInvalid);

	PRL_RESULT nVmValidityCode = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetConfigValidity(_connection2.GetTestVm(), &nVmValidityCode))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(nVmValidityCode, PRL_ERR_VM_IS_EXCLUSIVELY_LOCKED)

	hJob.reset(PrlVm_BeginEdit(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_Commit(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_Unlock(_connection1.GetTestVm(), 0));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_BeginEdit(_connection2.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_Commit(_connection2.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlExclusiveVmLockTest::testVmLockTryToLockVmTwice()
{
	SimpleServerWrapper _connection1, _connection2;
	QVERIFY(_connection1.IsConnected());
	QVERIFY(_connection2.IsConnected());
	QVERIFY(_connection1.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Lock(_connection1.GetTestVm(), 0));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_ASYNC_OP_FAILED(PrlVm_Lock(_connection2.GetTestVm(), 0), PRL_ERR_VM_IS_EXCLUSIVELY_LOCKED)
	CHECK_ASYNC_OP_FAILED(PrlVm_Lock(_connection1.GetTestVm(), 0), PRL_ERR_VM_IS_EXCLUSIVELY_LOCKED)

	hJob.reset(PrlVm_Unlock(_connection1.GetTestVm(), 0));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlExclusiveVmLockTest::testVmLockTryToUnlockVmFromAnotherSession()
{
	SimpleServerWrapper _connection1, _connection2;
	QVERIFY(_connection1.IsConnected());
	QVERIFY(_connection2.IsConnected());
	QVERIFY(_connection1.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Lock(_connection1.GetTestVm(), 0));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_ASYNC_OP_FAILED(PrlVm_Unlock(_connection2.GetTestVm(), 0), PRL_ERR_NOT_LOCK_OWNER_SESSION_TRIES_TO_UNLOCK)

	hJob.reset(PrlVm_Unlock(_connection1.GetTestVm(), 0));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlExclusiveVmLockTest::testVmLockAutoUnlockingOnSessionClosed()
{
	SimpleServerWrapper _connection2;
	QVERIFY(_connection2.IsConnected());
	QVERIFY(_connection2.CreateTestVm());
	{
		SimpleServerWrapper _connection1;
		QVERIFY(_connection1.IsConnected());
		SdkHandleWrap hJob(PrlVm_Lock(_connection1.GetTestVm(), 0));
		CHECK_JOB_RET_CODE(hJob)
		CHECK_ASYNC_OP_FAILED(PrlVm_Lock(_connection2.GetTestVm(), 0), PRL_ERR_VM_IS_EXCLUSIVELY_LOCKED)
	}

	SdkHandleWrap hJob(PrlVm_Lock(_connection2.GetTestVm(), 0));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_Unlock(_connection2.GetTestVm(), 0));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlExclusiveVmLockTest::testCheckPermissionsOnVmLockUnlockOperations()
{
	SimpleServerWrapper _connection1, _connection2(TestConfig::getUserLogin2());
	QVERIFY(_connection1.IsConnected());
	QVERIFY(_connection2.IsConnected());
	QVERIFY(_connection1.CreateTestVm());

	PRL_SHARE_VM(_connection1.GetTestVm(), PAO_VM_SHARED_ON_VIEW)

	PRL_CHECK_PERMISSION(_connection1.GetTestVm(), PAR_VM_LOCK_ACCESS, PRL_TRUE)
	PRL_CHECK_PERMISSION(_connection1.GetTestVm(), PAR_VM_UNLOCK_ACCESS, PRL_TRUE)
	PRL_CHECK_PERMISSION(_connection2.GetTestVm(), PAR_VM_LOCK_ACCESS, PRL_FALSE)
	PRL_CHECK_PERMISSION(_connection2.GetTestVm(), PAR_VM_UNLOCK_ACCESS, PRL_FALSE)

	PRL_SHARE_VM(_connection1.GetTestVm(), PAO_VM_SHARED_ON_VIEW_AND_RUN)

	PRL_CHECK_PERMISSION(_connection1.GetTestVm(), PAR_VM_LOCK_ACCESS, PRL_TRUE)
	PRL_CHECK_PERMISSION(_connection1.GetTestVm(), PAR_VM_UNLOCK_ACCESS, PRL_TRUE)
	PRL_CHECK_PERMISSION(_connection2.GetTestVm(), PAR_VM_LOCK_ACCESS, PRL_FALSE)
	PRL_CHECK_PERMISSION(_connection2.GetTestVm(), PAR_VM_UNLOCK_ACCESS, PRL_FALSE)

	PRL_SHARE_VM(_connection1.GetTestVm(), PAO_VM_SHARED_ON_FULL_ACCESS)

	PRL_CHECK_PERMISSION(_connection1.GetTestVm(), PAR_VM_LOCK_ACCESS, PRL_TRUE)
	PRL_CHECK_PERMISSION(_connection1.GetTestVm(), PAR_VM_UNLOCK_ACCESS, PRL_TRUE)
	PRL_CHECK_PERMISSION(_connection2.GetTestVm(), PAR_VM_LOCK_ACCESS, PRL_TRUE)
	PRL_CHECK_PERMISSION(_connection2.GetTestVm(), PAR_VM_UNLOCK_ACCESS, PRL_TRUE)
}

void PrlExclusiveVmLockTest::testVmLockOnWrongParams()
{
	SimpleServerWrapper _connection1;
	CHECK_ASYNC_OP_FAILED(PrlVm_Lock(PRL_INVALID_HANDLE, 0), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlVm_Lock(_connection1, 0), PRL_ERR_INVALID_ARG)
}

void PrlExclusiveVmLockTest::testVmUnlockOnWrongParams()
{
	SimpleServerWrapper _connection1;
	CHECK_ASYNC_OP_FAILED(PrlVm_Unlock(PRL_INVALID_HANDLE, 0), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlVm_Unlock(_connection1, 0), PRL_ERR_INVALID_ARG)
}

void PrlExclusiveVmLockTest::testStartVmThenLockThisThenCloseCurrentSession()
{
	{
		SimpleServerWrapper _connection1;
		QVERIFY(_connection1.IsConnected());
		QVERIFY(_connection1.CreateTestVm());

		{
			SimpleServerWrapper _connection2;
			QVERIFY(_connection2.IsConnected());
			SdkHandleWrap hJob(PrlVm_Start(_connection2.GetTestVm()));
			CHECK_JOB_RET_CODE(hJob)
			hJob.reset(PrlVm_Lock(_connection2.GetTestVm(), 0));
			CHECK_JOB_RET_CODE(hJob)
		}

		stop_vm_on_exit stop_vm(_connection1.GetTestVm());
	}
	SimpleServerWrapper _connection3;
	QVERIFY(_connection3.IsConnected());
	QVERIFY(PRL_INVALID_HANDLE == _connection3.GetTestVm().GetHandle());
}

void PrlExclusiveVmLockTest::testStartVmFromOneSessionAndLockItFromAnotherOne()
{
	SimpleServerWrapper _connection1;
	QVERIFY(_connection1.IsConnected());
	QVERIFY(_connection1.CreateTestVm());

	SimpleServerWrapper _connection2;
	QVERIFY(_connection2.IsConnected());
	SdkHandleWrap hJob(PrlVm_Start(_connection2.GetTestVm()));
	stop_vm_on_exit stop_vm(_connection1.GetTestVm());
	CHECK_JOB_RET_CODE(hJob)
	while(true)
	{
		hJob.reset(PrlVm_Lock(_connection1.GetTestVm(), 0));
		PRL_RESULT e = PRL_ERR_UNINITIALIZED;
		GET_JOB_RET_CODE_EX(hJob, e);
		if (PRL_SUCCEEDED(e))
			break;
		if (PRL_ERR_VM_LOCKED_FOR_DELETE_TO_SNAPSHOT == e)
			continue;
		QCOMPARE(e, PRL_ERR_SUCCESS);
	}
}

