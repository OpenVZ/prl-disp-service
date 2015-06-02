/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlUsbDevicesHotPlugTest.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing USB devices hot plug feature.
///
///	@author myakhin
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

#include "PrlUsbDevicesHotPlugTest.h"
#include "SimpleServerWrapper.h"
#include "Tests/CommonTestsUtils.h"

#define CHECK_THAT_USB_PRINTER_SUPPORTED\
	if ( TestConfig::isServerModePSBM() )\
		QSKIP("Skipping due usb printers is not supported at server mode", SkipAll);

#define CREATE_VM(create_with_devices)\
	SimpleServerWrapper _connection( NULL );\
	QVERIFY(_connection.IsConnected());\
	SdkHandleWrap hVm;\
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm( _connection.GetServerHandle(), hVm.GetHandlePtr() ))\
	SdkHandleWrap hJob(PrlSrv_GetSrvConfig( _connection.GetServerHandle() ));\
	CHECK_JOB_RET_CODE(hJob)\
	SdkHandleWrap hResult;\
	CHECK_RET_CODE_EXP(PrlJob_GetResult( hJob, hResult.GetHandlePtr() ))\
	SdkHandleWrap hSrvConfig;\
	CHECK_RET_CODE_EXP(PrlResult_GetParam( hResult, hSrvConfig.GetHandlePtr() ));\
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig( hVm, hSrvConfig, PVS_GUEST_VER_WIN_VISTA, create_with_devices ))\
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName( hVm, QTest::currentTestFunction() ))\
	QVERIFY(_connection.CreateTestVm( hVm ));

void PrlUsbDevicesHotPlugTest::testAddPrinterOnRunningVm()
{
	CHECK_THAT_USB_PRINTER_SUPPORTED

	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Start( hVm ))
	{
		stop_vm_on_exit stop_vm( hVm );
		SdkHandleWrap hPrinter;

		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_PARALLEL_PORT, hPrinter.GetHandlePtr() ))
		CHECK_HANDLE_TYPE(hPrinter, PHT_VIRTUAL_DEV_PARALLEL_PORT)
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	}
}

void PrlUsbDevicesHotPlugTest::testAddSeveralPrintersOnRunningVm()
{
	CHECK_THAT_USB_PRINTER_SUPPORTED

	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Start( hVm ))
	{
		stop_vm_on_exit stop_vm( hVm );
		SdkHandleWrap hPrinter1, hPrinter2;

		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_PARALLEL_PORT, hPrinter1.GetHandlePtr() ))
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_PARALLEL_PORT, hPrinter2.GetHandlePtr() ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	}
}

void PrlUsbDevicesHotPlugTest::testAddSeveralPrintersThenRemoveThemOnRunningVm()
{
	CHECK_THAT_USB_PRINTER_SUPPORTED

	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Start( hVm ))
	{
		stop_vm_on_exit stop_vm( hVm );
		SdkHandleWrap hPrinter1, hPrinter2, hPrinter3;

		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_PARALLEL_PORT, hPrinter1.GetHandlePtr() ))
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_PARALLEL_PORT, hPrinter2.GetHandlePtr() ))
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_PARALLEL_PORT, hPrinter3.GetHandlePtr() ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))

		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		CHECK_RET_CODE_EXP(PrlVmDev_Remove( hPrinter1 ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		CHECK_RET_CODE_EXP(PrlVmDev_Remove( hPrinter2 ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		CHECK_RET_CODE_EXP(PrlVmDev_Remove( hPrinter3 ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	}
}

void PrlUsbDevicesHotPlugTest::testAddAndRemovePrintersSimultaneouslyOnRunningVm()
{
	CHECK_THAT_USB_PRINTER_SUPPORTED

	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Start( hVm ))
	{
		stop_vm_on_exit stop_vm( hVm );
		SdkHandleWrap hPrinter1, hPrinter2;

		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_PARALLEL_PORT, hPrinter1.GetHandlePtr() ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))

		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_PARALLEL_PORT, hPrinter2.GetHandlePtr() ))
		CHECK_RET_CODE_EXP(PrlVmDev_Remove( hPrinter1 ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	}
}

void PrlUsbDevicesHotPlugTest::testDisconnectPrinterOnRunningVm()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	SdkHandleWrap hPrinter;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_PARALLEL_PORT, hPrinter.GetHandlePtr() ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Start( hVm ))
	{
		stop_vm_on_exit stop_vm( hVm );

		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_Disconnect( hPrinter ))
	}
}

void PrlUsbDevicesHotPlugTest::testDisconnectPrinterOnRunningVmViaEditConfig()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	SdkHandleWrap hPrinter;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_PARALLEL_PORT, hPrinter.GetHandlePtr() ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Start( hVm ))
	{
		stop_vm_on_exit stop_vm( hVm );

		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		CHECK_RET_CODE_EXP(PrlVmDev_SetConnected( hPrinter, PRL_FALSE ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	}
}
