/////////////////////////////////////////////////////////////////////////////
///
///	@file NonInitializedSdkTest.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests suite for checking attempts on SDK methods call when PrlApi_Init wasn't called before.
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

#include "NonInitializedSdkTest.h"

#include "Interfaces/ParallelsTypes.h"
#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"
#include <prlcommon/Logging/Logging.h>
#include "SDK/Wrappers/SdkWrap/SdkWrap.h"

#define CHECK_ASYNC_OP_FAILED(operation, expectedErrCode)\
	{\
		SdkHandleWrap hJob = operation;\
		QVERIFY(hJob != PRL_INVALID_HANDLE);\
		PRL_RESULT _ret_code = PRL_ERR_SUCCESS;\
		QVERIFY(PRL_SUCCEEDED(PrlJob_GetRetCode(hJob, &_ret_code)));\
		QVERIFY(!PRL_SUCCEEDED(_ret_code));\
		SdkHandleWrap hError;\
		QVERIFY(PRL_SUCCEEDED(PrlJob_GetError(hJob, hError.GetHandlePtr())));\
		QVERIFY(PRL_SUCCEEDED(PrlEvent_GetErrCode(hError, &_ret_code)));\
		if (expectedErrCode != _ret_code)\
			WRITE_TRACE(DBG_FATAL, "expression '%s' expectedErrCode=%.8X '%s' _ret_code=%.8X '%s'", #operation,\
				expectedErrCode, prl_result_to_string(expectedErrCode), _ret_code, prl_result_to_string(_ret_code));\
		QVERIFY(expectedErrCode == _ret_code);\
	}

#define CHECK_SYNC_OP_FAILED(operation, expectedErrCode)\
	{\
		PRL_RESULT _ret_code = operation;\
		QVERIFY(!PRL_SUCCEEDED(_ret_code));\
		if (expectedErrCode != _ret_code)\
			WRITE_TRACE(DBG_FATAL, "expression '%s' expectedErrCode=%.8X '%s' _ret_code=%.8X '%s'", #operation,\
				expectedErrCode, prl_result_to_string(expectedErrCode), _ret_code, prl_result_to_string(_ret_code));\
		QVERIFY(expectedErrCode == _ret_code);\
	}

namespace {
UNUSED PRL_RESULT CallbackStub(PRL_HANDLE, PRL_VOID_PTR)
{
	return PRL_ERR_UNIMPLEMENTED;
}

}

void NonInitializedSdkTest::test()
{
	PRL_HANDLE aHandle = NULL;
	PRL_UINT32 anUintValue = 0;
	PRL_IMAGE_FORMAT anImageFormat = PIF_RAW;
	PRL_INT32 anIntValue = 0;
	PRL_UINT16 anUint16Value = 0;

	CHECK_SYNC_OP_FAILED(PrlDevDisplay_SyncCaptureScreenRegionToFile(aHandle, "", anImageFormat, anIntValue, anUintValue, anUintValue, anUintValue, anUintValue), PRL_ERR_API_WASNT_INITIALIZED)
	CHECK_SYNC_OP_FAILED(PrlJob_Wait(aHandle, anUintValue), PRL_ERR_API_WASNT_INITIALIZED)
	CHECK_SYNC_OP_FAILED(PrlSrv_Create(&aHandle), PRL_ERR_API_WASNT_INITIALIZED)
	CHECK_SYNC_OP_FAILED(PrlSrv_CreateVm(aHandle, &aHandle), PRL_ERR_API_WASNT_INITIALIZED)
	CHECK_SYNC_OP_FAILED(PrlUsr_GetClientConfig(&aHandle), PRL_ERR_API_WASNT_INITIALIZED)
	CHECK_SYNC_OP_FAILED(PrlApi_Deinit(), PRL_ERR_API_WASNT_INITIALIZED)
	CHECK_SYNC_OP_FAILED(PrlApi_GetSupportedOsesTypes(PHO_UNKNOWN, &aHandle), PRL_ERR_API_WASNT_INITIALIZED)
	CHECK_SYNC_OP_FAILED(PrlApi_GetSupportedOsesVersions(PHO_UNKNOWN, PVS_GUEST_TYPE_WINDOWS, &aHandle), PRL_ERR_API_WASNT_INITIALIZED)
	CHECK_SYNC_OP_FAILED(PrlApi_GetDefaultOsVersion(PVS_GUEST_TYPE_WINDOWS, &anUint16Value), PRL_ERR_API_WASNT_INITIALIZED)
}

int main(int argc, char *argv[])
{
	if ( !SdkWrap_LoadLibFromStdPaths(true) )
	{
		WRITE_TRACE(DBG_FATAL, "Couldn't to initialize SDK lib");
		return (-1);
	}

	NonInitializedSdkTest _tests_suite;
	int nRet = QTest::qExec(&_tests_suite, argc, argv);

	SdkWrap_Unload();
	return (nRet);
}
