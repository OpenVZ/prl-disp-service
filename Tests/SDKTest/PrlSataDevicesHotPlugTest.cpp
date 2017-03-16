/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlSataDevicesHotPlugTest.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing SATA devices hot plug feature.
///
///	@author sandro
///
/// Copyright (c) 1999-2017, Parallels International GmbH
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

#include "PrlSataDevicesHotPlugTest.h"
#include "SimpleServerWrapper.h"
#include "Tests/CommonTestsUtils.h"
#include <prlcommon/Interfaces/ParallelsQt.h>
#include <prlcommon/PrlUuid/Uuid.h>

#include <QFile>
#include <QDir>
#include <QFileInfo>

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
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig( hVm, hSrvConfig, PVS_GUEST_VER_WIN_2008, create_with_devices ))\
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName( hVm, QTest::currentTestFunction() ))\
	QVERIFY(_connection.CreateTestVm( hVm ));

#define CHECK_WHETHER_HDD_BUNDLE_EXISTS(device, exists_sign)\
	{\
		QString sHddBundlePath;\
		PRL_EXTRACT_STRING_VALUE(sHddBundlePath, device, PrlVmDev_GetFriendlyName)\
		CHECK_RET_CODE_EXP(PrlVmDev_Remove( device ))\
		if ( exists_sign )\
			CHECK_ASYNC_OP_FAILED(PrlVm_CommitEx( hVm, PVCF_DESTROY_HDD_BUNDLE ), PRL_ERR_VM_MUST_BE_STOPPED_FOR_CHANGE_DEVICES)\
		else\
			CHECK_ASYNC_OP_SUCCEEDED(PrlVm_CommitEx( hVm, PVCF_DESTROY_HDD_BUNDLE ))\
		QVERIFY(exists_sign == QFile::exists( sHddBundlePath ));\
	}

void PrlSataDevicesHotPlugTest::cleanup()
{
	if ( QFile::exists( m_sImageFilePath ) )
		QFile::remove( m_sImageFilePath );
}

void PrlSataDevicesHotPlugTest::testAddSataHddOnRunningVm()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Start( hVm ))
	{
		stop_vm_on_exit stop_vm( hVm );
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		SdkHandleWrap hHddDev;
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev.GetHandlePtr() ))
		CHECK_HANDLE_TYPE(hHddDev, PHT_VIRTUAL_DEV_HARD_DISK)
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev, PRL_FALSE, PRL_TRUE ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_Disconnect( hHddDev ))
	}
}

void PrlSataDevicesHotPlugTest::testAddSeveralSataHddsOnRunningVm()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Start( hVm ))
	{
		stop_vm_on_exit stop_vm( hVm );
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		SdkHandleWrap hHddDev1, hHddDev2;
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev1.GetHandlePtr() ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev1, PRL_FALSE, PRL_TRUE ))
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev2.GetHandlePtr() ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev2, PRL_FALSE, PRL_TRUE ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_Disconnect( hHddDev1 ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_Disconnect( hHddDev2 ))
	}
}

void PrlSataDevicesHotPlugTest::testAddSeveralSataHddsThenRemoveThemOnRunningVm()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Start( hVm ))
	{
		stop_vm_on_exit stop_vm( hVm );
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		SdkHandleWrap hHddDev1, hHddDev2, hHddDev3;
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev1.GetHandlePtr() ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev1, PRL_FALSE, PRL_TRUE ))
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev2.GetHandlePtr() ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev2, PRL_FALSE, PRL_TRUE ))
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev3.GetHandlePtr() ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev3, PRL_FALSE, PRL_TRUE ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_Disconnect( hHddDev1 ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_Disconnect( hHddDev2 ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_Disconnect( hHddDev3 ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		CHECK_WHETHER_HDD_BUNDLE_EXISTS(hHddDev1, false)
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		CHECK_WHETHER_HDD_BUNDLE_EXISTS(hHddDev2, false)
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		CHECK_WHETHER_HDD_BUNDLE_EXISTS(hHddDev3, false)
	}
}

void PrlSataDevicesHotPlugTest::testAddAndRemoveSataHddsSimultaneouslyOnRunningVm()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Start( hVm ))
	{
		stop_vm_on_exit stop_vm( hVm );
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		SdkHandleWrap hHddDev1, hHddDev2;
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev1.GetHandlePtr() ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev1, PRL_FALSE, PRL_TRUE ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_Disconnect( hHddDev1 ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev2.GetHandlePtr() ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev2, PRL_FALSE, PRL_TRUE ))
		CHECK_WHETHER_HDD_BUNDLE_EXISTS(hHddDev1, false)
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_Disconnect( hHddDev2 ))
	}
}

void PrlSataDevicesHotPlugTest::testDisconnectSataHddOnRunningVm()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	SdkHandleWrap hHddDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev.GetHandlePtr() ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev, PRL_FALSE, PRL_TRUE ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Start( hVm ))
	{
		stop_vm_on_exit stop_vm( hVm );
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_Disconnect( hHddDev ))
	}
}

void PrlSataDevicesHotPlugTest::testDisconnectSataHddOnRunningVmViaEditConfig()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	SdkHandleWrap hHddDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev.GetHandlePtr() ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev, PRL_FALSE, PRL_TRUE ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Start( hVm ))
	{
		stop_vm_on_exit stop_vm( hVm );
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		CHECK_RET_CODE_EXP(PrlVmDev_SetConnected( hHddDev, PRL_FALSE ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	}
}

void PrlSataDevicesHotPlugTest::testTryDisconnectIdeHddOnRunningVm()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	SdkHandleWrap hHddDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev.GetHandlePtr() ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev, PRL_FALSE, PRL_TRUE ))
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType( hHddDev, PMS_IDE_DEVICE ))
	CHECK_RET_CODE_EXP(PrlVmDev_SetDefaultStackIndex( hHddDev ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Start( hVm ))
	{
		stop_vm_on_exit stop_vm( hVm );
		CHECK_ASYNC_OP_FAILED(PrlVmDev_Disconnect( hHddDev ), PRL_ERR_VM_DEV_DISCONNECT_FAILED)
	}
}

void PrlSataDevicesHotPlugTest::testTryDisconnectScsiHddOnRunningVm()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	SdkHandleWrap hHddDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev.GetHandlePtr() ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev, PRL_FALSE, PRL_TRUE ))
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType( hHddDev, PMS_SCSI_DEVICE ))
	CHECK_RET_CODE_EXP(PrlVmDev_SetDefaultStackIndex( hHddDev ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Start( hVm ))
	{
		stop_vm_on_exit stop_vm( hVm );
		CHECK_ASYNC_OP_FAILED(PrlVmDev_Disconnect( hHddDev ), PRL_ERR_VM_DEV_DISCONNECT_FAILED)
	}
}

void PrlSataDevicesHotPlugTest::testTryDisconnectIdeHddOnRunningVmViaEditConfig()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	SdkHandleWrap hHddDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev.GetHandlePtr() ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev, PRL_FALSE, PRL_TRUE ))
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType( hHddDev, PMS_IDE_DEVICE ))
	CHECK_RET_CODE_EXP(PrlVmDev_SetDefaultStackIndex( hHddDev ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Start( hVm ))
	{
		stop_vm_on_exit stop_vm( hVm );
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		CHECK_RET_CODE_EXP(PrlVmDev_SetConnected( hHddDev, PRL_FALSE ))
		CHECK_ASYNC_OP_FAILED(PrlVm_Commit( hVm ), PRL_ERR_VM_MUST_BE_STOPPED_FOR_CHANGE_DEVICES)
	}
}

void PrlSataDevicesHotPlugTest::testTryDisconnectScsiHddOnRunningVmViaEditConfig()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	SdkHandleWrap hHddDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev.GetHandlePtr() ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev, PRL_FALSE, PRL_TRUE ))
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType( hHddDev, PMS_SCSI_DEVICE ))
	CHECK_RET_CODE_EXP(PrlVmDev_SetDefaultStackIndex( hHddDev ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Start( hVm ))
	{
		stop_vm_on_exit stop_vm( hVm );
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		CHECK_RET_CODE_EXP(PrlVmDev_SetConnected( hHddDev, PRL_FALSE ))
		CHECK_ASYNC_OP_FAILED(PrlVm_Commit( hVm ), PRL_ERR_VM_MUST_BE_STOPPED_FOR_CHANGE_DEVICES)
	}
}

void PrlSataDevicesHotPlugTest::testAddSataHddOnStoppedVm()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	SdkHandleWrap hHddDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev.GetHandlePtr() ))
	CHECK_HANDLE_TYPE(hHddDev, PHT_VIRTUAL_DEV_HARD_DISK)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev, PRL_FALSE, PRL_TRUE ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
}

void PrlSataDevicesHotPlugTest::testAddSeveralSataHddsOnStoppedVm()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	SdkHandleWrap hHddDev1, hHddDev2;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev1.GetHandlePtr() ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev1, PRL_FALSE, PRL_TRUE ))
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev2.GetHandlePtr() ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev2, PRL_FALSE, PRL_TRUE ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
}

void PrlSataDevicesHotPlugTest::testDisconnectSataHddOnStoppedVmViaEditConfig()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	SdkHandleWrap hHddDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev.GetHandlePtr() ))
	CHECK_HANDLE_TYPE(hHddDev, PHT_VIRTUAL_DEV_HARD_DISK)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev, PRL_FALSE, PRL_TRUE ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected( hHddDev, PRL_FALSE ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
}

void PrlSataDevicesHotPlugTest::testTryDisconnectIdeHddOnStoppedVmViaEditConfig()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	SdkHandleWrap hHddDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev.GetHandlePtr() ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev, PRL_FALSE, PRL_TRUE ))
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType( hHddDev, PMS_IDE_DEVICE ))
	CHECK_RET_CODE_EXP(PrlVmDev_SetDefaultStackIndex( hHddDev ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected( hHddDev, PRL_FALSE ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
}

void PrlSataDevicesHotPlugTest::testTryDisconnectScsiHddOnStoppedVmViaEditConfig()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	SdkHandleWrap hHddDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev.GetHandlePtr() ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev, PRL_FALSE, PRL_TRUE ))
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType( hHddDev, PMS_SCSI_DEVICE ))
	CHECK_RET_CODE_EXP(PrlVmDev_SetDefaultStackIndex( hHddDev ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected( hHddDev, PRL_FALSE ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
}

void PrlSataDevicesHotPlugTest::testRemoveSataHddOnRunningVm()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Start( hVm ))
	{
		stop_vm_on_exit stop_vm( hVm );
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		SdkHandleWrap hHddDev;
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev.GetHandlePtr() ))
		CHECK_HANDLE_TYPE(hHddDev, PHT_VIRTUAL_DEV_HARD_DISK)
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev, PRL_FALSE, PRL_TRUE ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_Disconnect( hHddDev ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		CHECK_WHETHER_HDD_BUNDLE_EXISTS(hHddDev, false)
	}
}

void PrlSataDevicesHotPlugTest::testTryToRemoveNotDisconnectedSataHddOnRunningVm()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Start( hVm ))
	{
		stop_vm_on_exit stop_vm( hVm );
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		SdkHandleWrap hHddDev;
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev.GetHandlePtr() ))
		CHECK_HANDLE_TYPE(hHddDev, PHT_VIRTUAL_DEV_HARD_DISK)
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev, PRL_FALSE, PRL_TRUE ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		CHECK_WHETHER_HDD_BUNDLE_EXISTS(hHddDev, true)
	}
}

void PrlSataDevicesHotPlugTest::testRemoveSataHddOnStoppedVm()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	SdkHandleWrap hHddDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev.GetHandlePtr() ))
	CHECK_HANDLE_TYPE(hHddDev, PHT_VIRTUAL_DEV_HARD_DISK)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev, PRL_FALSE, PRL_TRUE ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected( hHddDev, PRL_FALSE ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	CHECK_WHETHER_HDD_BUNDLE_EXISTS(hHddDev, false)
}

void PrlSataDevicesHotPlugTest::testTryToRemoveNotDisconnectedSataHddOnStoppedVm()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	SdkHandleWrap hHddDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev.GetHandlePtr() ))
	CHECK_HANDLE_TYPE(hHddDev, PHT_VIRTUAL_DEV_HARD_DISK)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev, PRL_FALSE, PRL_TRUE ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	CHECK_WHETHER_HDD_BUNDLE_EXISTS(hHddDev, false)
}

void PrlSataDevicesHotPlugTest::testAddSeveralSataHddsThenRemoveThemOnStoppedVm()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	SdkHandleWrap hHddDev1, hHddDev2, hHddDev3;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev1.GetHandlePtr() ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev1, PRL_FALSE, PRL_TRUE ))
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev2.GetHandlePtr() ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev2, PRL_FALSE, PRL_TRUE ))
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev3.GetHandlePtr() ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev3, PRL_FALSE, PRL_TRUE ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	CHECK_WHETHER_HDD_BUNDLE_EXISTS(hHddDev1, false)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	CHECK_WHETHER_HDD_BUNDLE_EXISTS(hHddDev2, false)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	CHECK_WHETHER_HDD_BUNDLE_EXISTS(hHddDev3, false)
}

void PrlSataDevicesHotPlugTest::testAddAndRemoveSataHddsSimultaneouslyOnStoppedVm()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	SdkHandleWrap hHddDev1, hHddDev2;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev1.GetHandlePtr() ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev1, PRL_FALSE, PRL_TRUE ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_HARD_DISK, hHddDev2.GetHandlePtr() ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_CreateImage( hHddDev2, PRL_FALSE, PRL_TRUE ))
	CHECK_WHETHER_HDD_BUNDLE_EXISTS(hHddDev1, false)
}

void PrlSataDevicesHotPlugTest::testChangeSystemAndFriendlyNamesForCdromOnRunningVm()
{
	CREATE_VM(PRL_FALSE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	SdkHandleWrap hCdrom;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_OPTICAL_DISK, hCdrom.GetHandlePtr() ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Start( hVm ))
	{
		stop_vm_on_exit stop_vm( hVm );
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_Disconnect( hCdrom ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
		m_sImageFilePath = QDir::tempPath() + '/' + Uuid::createUuid().toString();
		QFile _file( m_sImageFilePath );
		QVERIFY(_file.open( QIODevice::WriteOnly ));
		CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType( hCdrom, PDT_USE_IMAGE_FILE ))
		CHECK_RET_CODE_EXP(PrlVmDev_SetFriendlyName( hCdrom, QSTR2UTF8(m_sImageFilePath) ))
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName( hCdrom, QSTR2UTF8(m_sImageFilePath) ))
		CHECK_RET_CODE_EXP(PrlVmDev_SetConnected( hCdrom, PRL_TRUE ))
		CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	}
}

void PrlSataDevicesHotPlugTest::testDisconnectSerialPortOnRunningVm()
{
	CREATE_VM(PRL_TRUE)
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_BeginEdit( hVm ))
	SdkHandleWrap hSerialPortDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVm, hSrvConfig, PDE_SERIAL_PORT, hSerialPortDev.GetHandlePtr() ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Commit( hVm ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Start( hVm ))
	{
		stop_vm_on_exit stop_vm( hVm );
		CHECK_ASYNC_OP_SUCCEEDED(PrlVmDev_Disconnect( hSerialPortDev ))
	}
}

