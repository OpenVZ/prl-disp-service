/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlVmValidateConfigTest.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing VM manipulating SDK API.
///
///	@author myakhin
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
#include "PrlVmValidateConfigTest.h"
#include "Tests/CommonTestsUtils.h"

#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlxmlmodel/HostHardwareInfo/CHwGenericPciDevice.h>
#include "Libraries/DiskImage/DiskImage.h"
#include <prlcommon/HostUtils/HostUtils.h>
#include "Interfaces/Config.h"
#include <prlcommon/Interfaces/ApiDevNums.h>


#define READ_VM_CONFIG_INTO_BUF(vm_config_path)\
	QFile _file(vm_config_path);\
	QVERIFY(_file.open(QIODevice::ReadOnly));\
	QTextStream _stream(&_file);\
	QString _config = _stream.readAll();

#define INITIALIZE_VM(vm_config_path)\
	READ_VM_CONFIG_INTO_BUF(vm_config_path)\
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _config.toUtf8().data()))\
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, QTest::currentTestFunction()))

#define COMPATIBLE_ADAPTER_TYPE (IsSharedNetworkingTypeEnabled() ? PNA_SHARED : PNA_HOST_ONLY)

void PrlVmValidateConfigTest::init()
{
	m_ServerHandle.reset();
	m_JobHandle.reset();
	m_VmHandle.reset();
	QVERIFY(PrlSrv_Create(m_ServerHandle.GetHandlePtr()) == PRL_ERR_SUCCESS);
	m_JobHandle.reset(PrlSrv_Login(m_ServerHandle, TestConfig::getRemoteHostName(),
		TestConfig::getUserLogin(), TestConfig::getUserPassword(), NULL, 0, 0, PSL_HIGH_SECURITY));
	CHECK_JOB_RET_CODE(m_JobHandle);
	INITIALIZE_VM("./TestDspCmdDirValidateVmConfig_vm_config.xml");

	m_JobHandle.reset(PrlVm_Stop(m_VmHandle, PRL_FALSE));
	PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
	m_JobHandle.reset(PrlVm_Delete(m_VmHandle, PRL_INVALID_HANDLE));
	PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
	m_VmHandle.reset();
	m_ResultHandle.reset();
}

void PrlVmValidateConfigTest::cleanup()
{
	m_lstVmHandles += m_VmHandle;
	m_VmHandle.reset();

	foreach(SdkHandleWrap hVm, m_lstVmHandles)
	{
		if (hVm)
		{
			m_JobHandle.reset(PrlVm_Delete(hVm, PRL_INVALID_HANDLE));
			PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
		}
	}

	m_JobHandle.reset(PrlSrv_Logoff(m_ServerHandle));
	PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
	m_ServerHandle.reset();

	if (!m_qsFile.isEmpty())
	{
		QFile::remove(m_qsFile);
		m_qsFile.clear();
	}
}

void PrlVmValidateConfigTest::testCreateVmFromConfig()
{
	INITIALIZE_VM("./TestDspCmdDirValidateVmConfig_vm_config.xml");

	m_JobHandle.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(m_JobHandle);
}

#define CHECK_VMCONF_VALIDATION_FAILED(hJob)\
{\
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT));\
	PRL_RESULT nValidationResult = PRL_ERR_UNINITIALIZED;\
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nValidationResult));\
	QVERIFY(nValidationResult == PRL_ERR_VMCONF_VALIDATION_FAILED);\
}

#define CHECK_ONE_ERROR_VALIDATION(section, error)\
{\
	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, section));\
	CHECK_VMCONF_VALIDATION_FAILED(hJob);\
		SdkHandleWrap hResult;\
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));\
			CHECK_PARAMS_COUNT(hResult, 1)\
				SdkHandleWrap hProblemDescription;\
				CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, hProblemDescription.GetHandlePtr()));\
				CHECK_HANDLE_TYPE(hProblemDescription, PHT_EVENT)\
					PRL_RESULT nProblemErrCode = PRL_ERR_UNINITIALIZED;\
					CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hProblemDescription, &nProblemErrCode));\
					QVERIFY(nProblemErrCode == error);\
}

#define CHECK_TWO_ERRORS_VALIDATION(section, error1, error2)\
{\
	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, section));\
	CHECK_VMCONF_VALIDATION_FAILED(hJob);\
		SdkHandleWrap hResult;\
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));\
			CHECK_PARAMS_COUNT(hResult, 2)\
				SdkHandleWrap hProblemDescription;\
				CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, hProblemDescription.GetHandlePtr()));\
				CHECK_HANDLE_TYPE(hProblemDescription, PHT_EVENT);\
					PRL_RESULT nProblemErrCode = PRL_ERR_UNINITIALIZED;\
					CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hProblemDescription, &nProblemErrCode));\
					QVERIFY(nProblemErrCode == error1);\
				CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 1, hProblemDescription.GetHandlePtr()));\
				CHECK_HANDLE_TYPE(hProblemDescription, PHT_EVENT);\
					CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hProblemDescription, &nProblemErrCode));\
					QVERIFY(nProblemErrCode == error2);\
}

#define CHECK_THREE_ERRORS_VALIDATION(section, error1, error2, error3)\
{\
	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, section));\
	CHECK_VMCONF_VALIDATION_FAILED(hJob);\
		SdkHandleWrap hResult;\
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));\
			CHECK_PARAMS_COUNT(hResult, 3)\
				SdkHandleWrap hProblemDescription;\
				CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, hProblemDescription.GetHandlePtr()));\
				CHECK_HANDLE_TYPE(hProblemDescription, PHT_EVENT);\
					PRL_RESULT nProblemErrCode = PRL_ERR_UNINITIALIZED;\
					CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hProblemDescription, &nProblemErrCode));\
					QVERIFY(nProblemErrCode == error1);\
				CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 1, hProblemDescription.GetHandlePtr()));\
				CHECK_HANDLE_TYPE(hProblemDescription, PHT_EVENT);\
					CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hProblemDescription, &nProblemErrCode));\
					QVERIFY(nProblemErrCode == error2);\
				CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 2, hProblemDescription.GetHandlePtr()));\
				CHECK_HANDLE_TYPE(hProblemDescription, PHT_EVENT);\
					CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hProblemDescription, &nProblemErrCode));\
					QVERIFY(nProblemErrCode == error3);\
}

void PrlVmValidateConfigTest::testValidateConfigGeneralParametersOnValid()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));
	// Unique name as uuid
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetOsVersion(m_VmHandle, PVS_GUEST_VER_OTH_OTHER));
	// Disable default auto-compress
	CHECK_RET_CODE_EXP(PrlVmCfg_SetAutoCompressEnabled(m_VmHandle, PRL_FALSE));

	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_GENERAL_PARAMETERS));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigGeneralParametersOnEmptyVmName()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	// Note: avoiding OS info errors
	CHECK_RET_CODE_EXP(PrlVmCfg_SetOsVersion(m_VmHandle, PVS_GUEST_VER_OTH_OTHER));

	CHECK_ONE_ERROR_VALIDATION(PVC_GENERAL_PARAMETERS, PRL_ERR_VMCONF_VM_NAME_IS_EMPTY);
}

static QString wrong_symbols = ":*?\"<>|";

void PrlVmValidateConfigTest::testValidateConfigGeneralParametersOnInvalidSymbolInVmName()
{
	QString wrong_symbols2 = "\\/" + wrong_symbols;

	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	// Note: avoiding OS info errors
	CHECK_RET_CODE_EXP(PrlVmCfg_SetOsVersion(m_VmHandle, PVS_GUEST_VER_OTH_OTHER));

	for(int i = 0; i < wrong_symbols2.size() ; ++i)
	{
		QString qsVmName = "VM Name ";
		qsVmName[3] = wrong_symbols2.at(i);
		CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, QSTR2UTF8(qsVmName)));

		CHECK_ONE_ERROR_VALIDATION(PVC_GENERAL_PARAMETERS, PRL_ERR_VMCONF_VM_NAME_HAS_INVALID_SYMBOL);
	}
}

struct TestOsParams
{
	PRL_UINT32 nOsType;
	PRL_UINT32 nLowOsVersion;
	PRL_UINT32 nHighOsVersion;
};

static TestOsParams os_params[]=
{
	{PVS_GUEST_TYPE_MACOS, PVS_GUEST_VER_MACOS_TIGER, PVS_GUEST_VER_MACOS_LAST},
	{PVS_GUEST_TYPE_WINDOWS, PVS_GUEST_VER_WIN_311, PVS_GUEST_VER_WIN_LAST},
	{PVS_GUEST_TYPE_LINUX, PVS_GUEST_VER_LIN_REDHAT, PVS_GUEST_VER_LIN_LAST},
	{PVS_GUEST_TYPE_FREEBSD, PVS_GUEST_VER_BSD_4X, PVS_GUEST_VER_BSD_LAST},
	{PVS_GUEST_TYPE_OS2, PVS_GUEST_VER_OS2_WARP3, PVS_GUEST_VER_OS2_LAST},
	{PVS_GUEST_TYPE_MSDOS, PVS_GUEST_VER_DOS_MS622, PVS_GUEST_VER_DOS_LAST},
	{PVS_GUEST_TYPE_NETWARE, PVS_GUEST_VER_NET_4X, PVS_GUEST_VER_NET_LAST},
	{PVS_GUEST_TYPE_SOLARIS, PVS_GUEST_VER_SOL_9, PVS_GUEST_VER_SOL_LAST},
	{PVS_GUEST_TYPE_CHROMEOS, PVS_GUEST_VER_CHROMEOS_1x, PVS_GUEST_VER_CHROMEOS_LAST},
	{PVS_GUEST_TYPE_OTHER, PVS_GUEST_VER_OTH_QNX, PVS_GUEST_VER_OTH_LAST}
};

void PrlVmValidateConfigTest::testValidateConfigGeneralParametersOnValidOs()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));
	// Note: avoiding VM name empty error
	// Unique name as uuid
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, Uuid::createUuid().toString().toUtf8().constData()));

	for(unsigned int i = 0; i < sizeof(os_params)/sizeof(os_params[0]); i++)
	{
		PRL_UINT32 nOsType = os_params[i].nOsType;

		PRL_UINT32 nOsVersion = 0;
		for(nOsVersion = os_params[i].nLowOsVersion; nOsVersion <= os_params[i].nHighOsVersion; nOsVersion++)
		{
			CHECK_RET_CODE_EXP(PrlVmCfg_SetOsVersion(m_VmHandle, nOsVersion));
            QCOMPARE(PVS_GET_GUEST_TYPE(nOsVersion), nOsType);

			PRL_UINT32 vm_os_type;
			CHECK_RET_CODE_EXP(PrlVmCfg_GetOsType(m_VmHandle, &vm_os_type));
			QCOMPARE(vm_os_type, nOsType);

			SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_GENERAL_PARAMETERS));
			CHECK_JOB_RET_CODE(hJob)
		}

		if (nOsType != PVS_GUEST_TYPE_MACOS)
		{
			// Other OS
			nOsVersion |= 0xFF;

			CHECK_RET_CODE_EXP(PrlVmCfg_SetOsVersion(m_VmHandle, nOsVersion));

			SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_GENERAL_PARAMETERS));
			CHECK_JOB_RET_CODE(hJob)
		}
	}
}

void PrlVmValidateConfigTest::testValidateConfigGeneralParametersOnInvalidOsType()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	CHECK_TWO_ERRORS_VALIDATION(PVC_GENERAL_PARAMETERS,
								PRL_ERR_VMCONF_VM_NAME_IS_EMPTY,
								PRL_ERR_VMCONF_UNKNOWN_OS_TYPE);
}

void PrlVmValidateConfigTest::testValidateConfigGeneralParametersOnNoSmartGuardWithBootCamp()
{
	testValidateConfigGeneralParametersOnValid();

// Smart guard is on

	CHECK_RET_CODE_EXP(PrlVmCfg_SetSmartGuardEnabled(m_VmHandle, PRL_TRUE));

// 1 Add hard disk
	SdkHandleWrap hDevice1;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice1.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice1, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice1, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice1, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice1, 0));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice1, PDT_USE_IMAGE_FILE));
// 2 Add hard disk
	SdkHandleWrap hDevice2;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice2.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice2, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice2, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice2, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice2, 1));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice2, PDT_USE_IMAGE_FILE));
////
	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_GENERAL_PARAMETERS));
	CHECK_JOB_RET_CODE(hJob);

// Not connected
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice2, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice2, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice2, PDT_USE_BOOTCAMP));
////
	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_GENERAL_PARAMETERS));
	CHECK_JOB_RET_CODE(hJob);

// Not enabled
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice2, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice2, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice2, PDT_USE_BOOTCAMP));
////
	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_GENERAL_PARAMETERS));
	CHECK_JOB_RET_CODE(hJob);

// Not allow boot camp disks
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice1, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice1, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice1, PDT_USE_BOOTCAMP));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice2, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice2, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice2, PDT_USE_BOOTCAMP));
////
	CHECK_TWO_ERRORS_VALIDATION(PVC_GENERAL_PARAMETERS,
								PRL_ERR_VMCONF_BOOTCAMP_HARD_DISK_SMART_GUARD_NOT_ALLOW,
								PRL_ERR_VMCONF_BOOTCAMP_HARD_DISK_SMART_GUARD_NOT_ALLOW);
// Smart guard is off
	CHECK_RET_CODE_EXP(PrlVmCfg_SetSmartGuardEnabled(m_VmHandle, PRL_FALSE));
////
	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_GENERAL_PARAMETERS));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigGeneralParametersOnNoSmartGuardWithIncompatDisks()
{
	testCreateVmFromConfig();

	SdkHandleWrap hJob;

	hJob.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

// Prepare

	QString qsVmDir;
	PRL_EXTRACT_STRING_VALUE(qsVmDir, m_VmHandle, PrlVmCfg_GetHomePath);
	qsVmDir = QFileInfo(qsVmDir).path();

	QString qsDir1 = qsVmDir + "/hdd_1";
	QString qsDir2 = qsVmDir + "/hdd_2";

	CHECK_RET_CODE_EXP(PrlVmCfg_SetOsVersion(m_VmHandle, PVS_GUEST_VER_WIN_2K));

// Smart guard mode

	CHECK_RET_CODE_EXP(PrlVmCfg_SetSmartGuardEnabled(m_VmHandle, PRL_TRUE));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetAutoCompressEnabled(m_VmHandle, PRL_FALSE));

// 1 Add hard disk
	SdkHandleWrap hDevice1;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice1.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice1, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice1, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice1, QSTR2UTF8(qsDir1)));
	CHECK_RET_CODE_EXP(PrlVmDev_SetFriendlyName(hDevice1, QSTR2UTF8(qsDir1)));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice1, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice1, 0));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice1, PDT_USE_IMAGE_FILE));
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskSize(hDevice1, 1024 * 1024));
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskType(hDevice1, PHD_EXPANDING_HARD_DISK));

	hJob.reset(PrlVmDev_CreateImage(hDevice1, PRL_TRUE, PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);
// 2 Add hard disk
	SdkHandleWrap hDevice2;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice2.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice2, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice2, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice2, QSTR2UTF8(qsDir2)));
	CHECK_RET_CODE_EXP(PrlVmDev_SetFriendlyName(hDevice2, QSTR2UTF8(qsDir2)));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice2, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice2, 1));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice2, PDT_USE_IMAGE_FILE));
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskSize(hDevice2, 1024 * 1024));
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskType(hDevice2, PHD_EXPANDING_HARD_DISK));

	hJob.reset(PrlVmDev_CreateImage(hDevice2, PRL_TRUE, PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);
////
	hJob.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);
////
	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_GENERAL_PARAMETERS));
	CHECK_JOB_RET_CODE(hJob);

// Set old level

	QFile DiskDescriptor(qsDir2 + "/"DISK_DESCRIPTOR_XML);
	QVERIFY(DiskDescriptor.open(QIODevice::ReadWrite));
	QByteArray ba = DiskDescriptor.readAll();
	QVERIFY(!ba.isEmpty());

	ba.replace(COMPAT_LEVEL_PARAM_VAL_LEVEL2, COMPAT_LEVEL_PARAM_VAL_LEVEL1);

	DiskDescriptor.seek(0);
	QVERIFY(DiskDescriptor.write(ba) != 0);
	DiskDescriptor.close();
////
	CHECK_ONE_ERROR_VALIDATION(PVC_GENERAL_PARAMETERS, PRL_ERR_VMCONF_INCOMPAT_HARD_DISK_SMART_GUARD_NOT_ALLOW);
// Old windows versions
	CHECK_RET_CODE_EXP(PrlVmCfg_SetOsVersion(m_VmHandle, PVS_GUEST_VER_WIN_NT));
////
	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_GENERAL_PARAMETERS));
	CHECK_JOB_RET_CODE(hJob);
// another OS
	CHECK_RET_CODE_EXP(PrlVmCfg_SetOsVersion(m_VmHandle, PVS_GUEST_VER_LIN_FEDORA));
////
	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_GENERAL_PARAMETERS));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigGeneralParametersOnVideoNotEnabled()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml");

	CVmConfiguration _vm_conf(_config);
	CVmHardware* pHardwareList = _vm_conf.getVmHardwareList();

	_vm_conf.getVmSettings()->getVmCommonOptions()->setOsVersion(PVS_GUEST_VER_OTH_OTHER);

	pHardwareList->getVideo()->setEnabled(false);

	pHardwareList->m_lstPciVideoAdapters += new CVmPciVideoAdapter;
	pHardwareList->m_lstPciVideoAdapters += new CVmPciVideoAdapter;

	foreach(CVmPciVideoAdapter* pDev, pHardwareList->m_lstPciVideoAdapters)
	{
		pDev->setEnabled(PVE::DeviceDisabled);
	}
	CHECK_RET_CODE_EXP( PrlVm_FromString(m_VmHandle, QSTR2UTF8(_vm_conf.toString())) );

	CHECK_ONE_ERROR_VALIDATION(PVC_GENERAL_PARAMETERS, PRL_ERR_VMCONF_VIDEO_NOT_ENABLED);

	pHardwareList->m_lstPciVideoAdapters[0]->setEnabled(PVE::DeviceEnabled);
	CHECK_RET_CODE_EXP( PrlVm_FromString(m_VmHandle, QSTR2UTF8(_vm_conf.toString())) );

	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_GENERAL_PARAMETERS));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigGeneralParametersOnNoAutoCompressWithSmartGuard()
{
	testValidateConfigGeneralParametersOnValid();

	CHECK_RET_CODE_EXP(PrlVmCfg_SetSmartGuardEnabled(m_VmHandle, PRL_TRUE));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetAutoCompressEnabled(m_VmHandle, PRL_TRUE));

	CHECK_ONE_ERROR_VALIDATION(PVC_GENERAL_PARAMETERS, PRL_ERR_VMCONF_NO_AUTO_COMPRESS_WITH_SMART_GUARD);

	CHECK_RET_CODE_EXP(PrlVmCfg_SetAutoCompressEnabled(m_VmHandle, PRL_FALSE));

	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_GENERAL_PARAMETERS));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigBootOptionOnValid()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	SdkHandleWrap hBootDevice;
	PRL_UINT32 nIndex = 0;

	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hDevice, &nIndex));
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()));
			CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, 3));
			CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, nIndex));
			CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_FLOPPY_DISK));

	hDevice.reset();
	hBootDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hDevice, &nIndex));
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()));
			CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, 2));
			CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, nIndex));
			CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_GENERIC_NETWORK_ADAPTER));

	hDevice.reset();
	hBootDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hDevice, &nIndex));
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()));
			CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, 1));
			CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, nIndex));
			CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_HARD_DISK));

	hDevice.reset();
	hBootDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hDevice, &nIndex));
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()));
			CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, 0));
			CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, nIndex));
			CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_OPTICAL_DISK));

// Check booting devices order at sorting them by sequence index

	CVmConfiguration _vm_conf;
	VM_CONFIG_TO_XML_OBJECT;

	QList<PRL_DEVICE_TYPE > lstDevTypes;
	lstDevTypes << PDE_OPTICAL_DISK << PDE_HARD_DISK << PDE_GENERIC_NETWORK_ADAPTER << PDE_FLOPPY_DISK;

	foreach(CVmBootDeviceBase* pBD,
			_vm_conf.getVmSettings()->getVmStartupOptions()->getBootingOrder()->m_lstBootDevice)
	{
		QCOMPARE(pBD->getType(), lstDevTypes.first());
		lstDevTypes.removeFirst();
	}

////
	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_BOOT_OPTION));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigBootOptionOnInvalidType()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	SdkHandleWrap hBootDevice;
	PRL_UINT32 nIndex = 0;
// Floppy disk is valid boot device
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hDevice, &nIndex));
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()));
			CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, 0));
			CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, nIndex));
			CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_FLOPPY_DISK));
// Parallel port is invalid boot device
	hDevice.reset();
	hBootDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hDevice, &nIndex));
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()));
			CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, 1));
			CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, nIndex));
			CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_PARALLEL_PORT));

	CHECK_ONE_ERROR_VALIDATION(PVC_BOOT_OPTION, PRL_ERR_VMCONF_BOOT_OPTION_INVALID_DEVICE_TYPE);
}

void PrlVmValidateConfigTest::testValidateConfigBootOptionOnDuplicateDevice()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	SdkHandleWrap hBootDevice;
	PRL_UINT32 nIndex = 0;
// Add floppy disk as boot device
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hDevice, &nIndex));
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()));
			CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, 3));
			CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, nIndex));
			CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_FLOPPY_DISK));
// Add optical disk as boot device
	hDevice.reset();
	hBootDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hDevice, &nIndex));
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()));
			CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, 2));
			CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, nIndex));
			CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_OPTICAL_DISK));
// Add optical disk as boot device
	hDevice.reset();
	hBootDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hDevice, &nIndex));
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()));
			CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, 1));
			CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, nIndex));
			CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_OPTICAL_DISK));
	PRL_UINT32 nIndexDup = nIndex;
// Add hard disk as boot device
	hDevice.reset();
	hBootDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hDevice, &nIndex));
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()));
			CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, 0));
			CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, nIndex));
			CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_HARD_DISK));
// Add duplicated optical disk as boot device
	hDevice.reset();
	hBootDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()));
			CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, 4));
			CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, nIndexDup));
			CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_OPTICAL_DISK));

	CHECK_ONE_ERROR_VALIDATION(PVC_BOOT_OPTION, PRL_ERR_VMCONF_BOOT_OPTION_DUPLICATE_DEVICE);
}

void PrlVmValidateConfigTest::testValidateConfigBootOptionOnDeviceNotExists()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	SdkHandleWrap hBootDevice;
	PRL_UINT32 nIndex = 0;
//// (1) Add absent floppy disk as boot device
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()));
			CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, 5));
			CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, 0));
			CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_FLOPPY_DISK));
//// (2) Add absent network adapter as boot device
		hBootDevice.reset();
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()));
			CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, 2));
			CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, 0));
			CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_GENERIC_NETWORK_ADAPTER));
//// Add existing hard disk as boot device
	hBootDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hDevice, &nIndex));
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()));
			CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, 1));
			CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, nIndex));
			CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_HARD_DISK));
//// Add existing hard disk as boot device
	hDevice.reset();
	hBootDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hDevice, &nIndex));
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()));
			CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, 4));
			CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, nIndex));
			CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_HARD_DISK));
//// (3) Add absent hard disk as boot device
		hBootDevice.reset();
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()));
			CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, 3));
			CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, 2));
			CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_HARD_DISK));
//// Add existing optical disk as boot device
	hDevice.reset();
	hBootDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hDevice, &nIndex));
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()));
			CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, 0));
			CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, nIndex));
			CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_OPTICAL_DISK));
//// (4) Add absent optical disk as boot device
		hBootDevice.reset();
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()));
			CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, 6));
			CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, 1));
			CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_OPTICAL_DISK));
////
	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_BOOT_OPTION));
	CHECK_VMCONF_VALIDATION_FAILED(hJob);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));

	CHECK_PARAMS_COUNT(hResult, 4)

	SdkHandleWrap hProblemDescription;
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, hProblemDescription.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hProblemDescription, PHT_EVENT);

	PRL_RESULT nProblemErrCode = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hProblemDescription, &nProblemErrCode));
	QVERIFY(nProblemErrCode == PRL_ERR_VMCONF_BOOT_OPTION_DEVICE_NOT_EXISTS);

	hProblemDescription.reset();
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 1, hProblemDescription.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hProblemDescription, PHT_EVENT);

	CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hProblemDescription, &nProblemErrCode));
	QVERIFY(nProblemErrCode == PRL_ERR_VMCONF_BOOT_OPTION_DEVICE_NOT_EXISTS);

	hProblemDescription.reset();
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 2, hProblemDescription.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hProblemDescription, PHT_EVENT);

	CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hProblemDescription, &nProblemErrCode));
	QVERIFY(nProblemErrCode == PRL_ERR_VMCONF_BOOT_OPTION_DEVICE_NOT_EXISTS);

	hProblemDescription.reset();
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 3, hProblemDescription.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hProblemDescription, PHT_EVENT);

	CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hProblemDescription, &nProblemErrCode));
	QVERIFY(nProblemErrCode == PRL_ERR_VMCONF_BOOT_OPTION_DEVICE_NOT_EXISTS);
}

void PrlVmValidateConfigTest::testValidateConfigRemoteDisplayOnValid()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

// Zero address as ANY address is valid

	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCPassword(m_VmHandle, "12345"));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCPort(m_VmHandle, 5678));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCHostName(m_VmHandle, "0.0.0.0"));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCMode(m_VmHandle, PRD_MANUAL));

	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_REMOTE_DISPLAY));
	CHECK_JOB_RET_CODE(hJob)

// Normal address

	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCPort(m_VmHandle, 1234));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCHostName(m_VmHandle, "192.168.20.30"));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCMode(m_VmHandle, PRD_MANUAL));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_REMOTE_DISPLAY));
	CHECK_JOB_RET_CODE(hJob)

// Incorrect port and address but remote display is not enabled

	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCPassword(m_VmHandle, ""));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCPort(m_VmHandle, 0));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCHostName(m_VmHandle, "307.555.1.2"));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCMode(m_VmHandle, PRD_DISABLED));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_REMOTE_DISPLAY));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlVmValidateConfigTest::testValidateConfigRemoteDisplayOnZeroPortValue()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCPassword(m_VmHandle, "12345"));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCPort(m_VmHandle, 0));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCHostName(m_VmHandle, "1.2.3.4"));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCMode(m_VmHandle, PRD_MANUAL));

	CHECK_ONE_ERROR_VALIDATION(PVC_REMOTE_DISPLAY, PRL_ERR_VMCONF_REMOTE_DISPLAY_PORT_NUMBER_IS_ZERO);
}

void PrlVmValidateConfigTest::testValidateConfigRemoteDisplayOnInvalidIpAddress()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCPassword(m_VmHandle, "12345"));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCHostName(m_VmHandle, "256.3.444.0"))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCMode(m_VmHandle, PRD_AUTO));

	CHECK_ONE_ERROR_VALIDATION(PVC_REMOTE_DISPLAY, PRL_ERR_VMCONF_REMOTE_DISPLAY_INVALID_HOST_IP_ADDRESS);
}

void PrlVmValidateConfigTest::testValidateConfigRemoteDisplayOnEmptyPassword()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

// Empty password

	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCPassword(m_VmHandle, ""));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCMode(m_VmHandle, PRD_MANUAL));

	CHECK_ONE_ERROR_VALIDATION(PVC_REMOTE_DISPLAY, PRL_ERR_VMCONF_REMOTE_DISPLAY_EMPTY_PASSWORD);

// Not empty password

	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCPassword(m_VmHandle, "12345"));

	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_REMOTE_DISPLAY));
	CHECK_JOB_RET_CODE(hJob);

// Empty password but remote display is disabled

	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCPassword(m_VmHandle, ""));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCMode(m_VmHandle, PRD_DISABLED));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_REMOTE_DISPLAY));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigSharedFoldersOnValid()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml");

	CVmConfiguration _vm_conf(_config);
	CVmHostSharing* pHostSharing =
		_vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing();
	pHostSharing->setUserDefinedFoldersEnabled(true);

	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()));

	SdkHandleWrap hShare;
// 1 Add shared folder with valid name and path
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(m_VmHandle, hShare.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlShare_SetName(hShare, "Name 1"));
	CHECK_RET_CODE_EXP(PrlShare_SetPath(hShare, QDir::currentPath().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlShare_SetEnabled(hShare, PRL_TRUE));
// 2 Add shared folder with valid name and path
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(m_VmHandle, hShare.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlShare_SetName(hShare, "Name 2"));
	CHECK_RET_CODE_EXP(PrlShare_SetPath(hShare, QDir::rootPath().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlShare_SetEnabled(hShare, PRL_TRUE));
// 3 Add shared folder with valid name and path
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(m_VmHandle, hShare.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlShare_SetName(hShare, "Name 3"));
	CHECK_RET_CODE_EXP(PrlShare_SetPath(hShare, QDir::tempPath().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlShare_SetEnabled(hShare, PRL_TRUE));
// 4 Add shared folder with duplicate name but folder is not enabled
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(m_VmHandle, hShare.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlShare_SetName(hShare, "Name 1"));
	CHECK_RET_CODE_EXP(PrlShare_SetPath(hShare, QDir::currentPath().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlShare_SetEnabled(hShare, PRL_FALSE));
////
	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_SHARED_FOLDERS));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigSharedFoldersOnEmptyFolderName()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml");

	CVmConfiguration _vm_conf(_config);
	CVmHostSharing* pHostSharing =
		_vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing();
	pHostSharing->setUserDefinedFoldersEnabled(true);

	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()));

	SdkHandleWrap hShare;
// 1 Add shared folder with empty name but with valid path
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(m_VmHandle, hShare.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlShare_SetName(hShare, ""));
	CHECK_RET_CODE_EXP(PrlShare_SetPath(hShare, QDir::currentPath().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlShare_SetEnabled(hShare, PRL_TRUE));
// 2 Add shared folder with empty name but with valid path
	hShare.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(m_VmHandle, hShare.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlShare_SetName(hShare, ""));
	CHECK_RET_CODE_EXP(PrlShare_SetPath(hShare, QDir::tempPath().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlShare_SetEnabled(hShare, PRL_TRUE));
////
	CHECK_TWO_ERRORS_VALIDATION(PVC_SHARED_FOLDERS,
								PRL_ERR_VMCONF_SHARED_FOLDERS_EMPTY_FOLDER_NAME,
								PRL_ERR_VMCONF_SHARED_FOLDERS_EMPTY_FOLDER_NAME);
}

void PrlVmValidateConfigTest::testValidateConfigSharedFoldersOnDuplicateFolderName()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml");

	CVmConfiguration _vm_conf(_config);
	CVmHostSharing* pHostSharing =
		_vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing();
	pHostSharing->setUserDefinedFoldersEnabled(true);

	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()));

	SdkHandleWrap hShare;
// 1 Add shared folder with valid name and path
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(m_VmHandle, hShare.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlShare_SetName(hShare, "Name dup"));
	CHECK_RET_CODE_EXP(PrlShare_SetPath(hShare, QDir::currentPath().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlShare_SetEnabled(hShare, PRL_TRUE));
// 2 Add shared folder with valid name and path
	hShare.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(m_VmHandle, hShare.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlShare_SetName(hShare, "Name 2"));
	CHECK_RET_CODE_EXP(PrlShare_SetPath(hShare, QDir::rootPath().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlShare_SetEnabled(hShare, PRL_TRUE));
// 3 Add shared folder with duplicated name and valid path
	hShare.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(m_VmHandle, hShare.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlShare_SetName(hShare, "NaMe DUp"));
	CHECK_RET_CODE_EXP(PrlShare_SetPath(hShare, QDir::tempPath().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlShare_SetEnabled(hShare, PRL_TRUE));
////
	CHECK_ONE_ERROR_VALIDATION(PVC_SHARED_FOLDERS, PRL_ERR_VMCONF_SHARED_FOLDERS_DUPLICATE_FOLDER_NAME);
}

void PrlVmValidateConfigTest::testValidateConfigSharedFoldersOnInvalidFolderPath()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml")

	CVmConfiguration _vm_conf(_config);
	CVmHostSharing* pHostSharing =
		_vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing();
	pHostSharing->setUserDefinedFoldersEnabled(true);

	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()));

	SdkHandleWrap hShare;
// 1 Add shared folder with valid name and path
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(m_VmHandle, hShare.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlShare_SetName(hShare, "Name 1"));
	CHECK_RET_CODE_EXP(PrlShare_SetPath(hShare, QDir::currentPath().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlShare_SetEnabled(hShare, PRL_TRUE));
// 2 Add shared folder with valid name and invalid path
	hShare.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(m_VmHandle, hShare.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlShare_SetName(hShare, "Name 2"));
	CHECK_RET_CODE_EXP(PrlShare_SetPath(hShare, (QDir::currentPath() + "zxy").toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlShare_SetEnabled(hShare, PRL_TRUE));
// 3 Add shared folder with valid name and path
	hShare.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(m_VmHandle, hShare.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlShare_SetName(hShare, "Name 3"));
	CHECK_RET_CODE_EXP(PrlShare_SetPath(hShare, QDir::tempPath().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlShare_SetEnabled(hShare, PRL_TRUE));
////
	CHECK_ONE_ERROR_VALIDATION(PVC_SHARED_FOLDERS, PRL_ERR_VMCONF_SHARED_FOLDERS_INVALID_FOLDER_PATH);
}

void PrlVmValidateConfigTest::testValidateConfigSharedFoldersOnDuplicateFolderPath()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml")

	CVmConfiguration _vm_conf(_config);
	CVmHostSharing* pHostSharing =
		_vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing();
	pHostSharing->setUserDefinedFoldersEnabled(true);

	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()));

	SdkHandleWrap hShare;
// 1 Add shared folder with valid name and path
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(m_VmHandle, hShare.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlShare_SetName(hShare, "Name 1"));
	CHECK_RET_CODE_EXP(PrlShare_SetPath(hShare, QDir::currentPath().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlShare_SetEnabled(hShare, PRL_TRUE));
// 2 Add shared folder with valid name and duplicated path
	hShare.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(m_VmHandle, hShare.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlShare_SetName(hShare, "Name 2"));
	CHECK_RET_CODE_EXP(PrlShare_SetPath(hShare, QDir::currentPath().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlShare_SetEnabled(hShare, PRL_TRUE));
// 3 Add shared folder with valid name and path
	hShare.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(m_VmHandle, hShare.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlShare_SetName(hShare, "Name 3"));
	CHECK_RET_CODE_EXP(PrlShare_SetPath(hShare, QDir::tempPath().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlShare_SetEnabled(hShare, PRL_TRUE));
////
	CHECK_ONE_ERROR_VALIDATION(PVC_SHARED_FOLDERS, PRL_ERR_VMCONF_SHARED_FOLDERS_DUPLICATE_FOLDER_PATH);
}

void PrlVmValidateConfigTest::testValidateConfigSharedFoldersOnNotUserDefinedFolders()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml");

	CVmConfiguration _vm_conf(_config);
	CVmHostSharing* pHostSharing =
		_vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing();
	pHostSharing->setUserDefinedFoldersEnabled(false);

	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()));

	SdkHandleWrap hShare;
// 1 Add shared folder with duplicate name and invalid path
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(m_VmHandle, hShare.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlShare_SetName(hShare, "Name dup"));
	CHECK_RET_CODE_EXP(PrlShare_SetPath(hShare, (QDir::currentPath() + "zxy1").toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlShare_SetEnabled(hShare, PRL_TRUE));
// 2 Add shared folder with duplicate name and invalid path
	hShare.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(m_VmHandle, hShare.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlShare_SetName(hShare, "Name dup"));
	CHECK_RET_CODE_EXP(PrlShare_SetPath(hShare, (QDir::currentPath() + "zxy2").toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlShare_SetEnabled(hShare, PRL_TRUE));
////
	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_SHARED_FOLDERS));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlVmValidateConfigTest::testValidateConfigCpuOnValid()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hJob(PrlSrv_GetSrvConfig(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));

	SdkHandleWrap hServerConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hServerConfig.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hServerConfig, PHT_SERVER_CONFIG);

	PRL_UINT32 nHostCpuCount = 0;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetCpuCount(hServerConfig, &nHostCpuCount));

	PRL_UINT32 nMaxCpuCount = nHostCpuCount < PRL_MAX_CPU_COUNT ? nHostCpuCount : PRL_MAX_CPU_COUNT;
	for(PRL_UINT32 nCpuCount = 1; nCpuCount <= nMaxCpuCount; ++nCpuCount)
	{
		CHECK_RET_CODE_EXP(PrlVmCfg_SetCpuCount(m_VmHandle, nCpuCount));

		hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_CPU));
		CHECK_JOB_RET_CODE(hJob);
	}
}

void PrlVmValidateConfigTest::testValidateConfigCpuOnZeroCount()
{
	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml");
	CVmConfiguration _vm_conf(_config);
	_vm_conf.getVmHardwareList()->getCpu()->setNumber(0);
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()));

	CHECK_ONE_ERROR_VALIDATION(PVC_CPU, PRL_ERR_VMCONF_CPU_ZERO_COUNT);
}

void PrlVmValidateConfigTest::testValidateConfigCpuOnInvalidCount()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hJob(PrlSrv_GetSrvConfig(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));

	SdkHandleWrap hServerConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hServerConfig.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hServerConfig, PHT_SERVER_CONFIG);

	PRL_UINT32 nCpuCount = 0;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetCpuCount(hServerConfig, &nCpuCount));

	nCpuCount = (nCpuCount * 2) + (PRL_MAX_CPU_COUNT-1);
	CHECK_RET_CODE_EXP(PrlVmCfg_SetCpuCount(m_VmHandle, nCpuCount));
////
	CHECK_TWO_ERRORS_VALIDATION(PVC_CPU,
								PRL_ERR_VMCONF_CPU_COUNT_MORE_MAX_CPU_COUNT,
								PRL_ERR_VMCONF_CPU_COUNT_MORE_HOST_CPU_COUNT);
}

void PrlVmValidateConfigTest::testValidateConfigMainMemoryOnValid()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hJob(PrlSrv_GetCommonPrefs(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));

	SdkHandleWrap hDispConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hDispConfig.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hDispConfig, PHT_DISP_CONFIG);

	PRL_UINT32 nRecomnededMemSize = 0;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetRecommendMaxVmMem(hDispConfig, &nRecomnededMemSize));
	nRecomnededMemSize = (nRecomnededMemSize & 0xFFFFFFFCUL);

	CHECK_RET_CODE_EXP(PrlVmCfg_SetRamSize(m_VmHandle, nRecomnededMemSize));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_MAIN_MEMORY));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigMainMemoryOnZero()
{
	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml");
	CVmConfiguration _vm_conf(_config);
	_vm_conf.getVmHardwareList()->getMemory()->setRamSize(0);
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()));

	CHECK_ONE_ERROR_VALIDATION(PVC_MAIN_MEMORY, PRL_ERR_VMCONF_MAIN_MEMORY_ZERO_SIZE);
}

void PrlVmValidateConfigTest::testValidateConfigMainMemoryOnOutOfRange()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmCfg_SetRamSize(m_VmHandle, VM_MAX_MEM + 4));

	CHECK_ONE_ERROR_VALIDATION(PVC_MAIN_MEMORY, PRL_ERR_VMCONF_MAIN_MEMORY_OUT_OF_RANGE);
}

void PrlVmValidateConfigTest::testValidateConfigMainMemoryOnRatio4()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	PRL_UINT32 nSize = 501;
	for(int i = 0; i < 3; i++)
	{
		CHECK_RET_CODE_EXP(PrlVmCfg_SetRamSize(m_VmHandle, nSize + i));

		CHECK_ONE_ERROR_VALIDATION(PVC_MAIN_MEMORY, PRL_ERR_VMCONF_MAIN_MEMORY_NOT_4_RATIO_SIZE);
	}
}

void PrlVmValidateConfigTest::testValidateConfigMainMemoryOnSizeNotEqualRecommended()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmCfg_SetOsVersion(m_VmHandle, PVS_GUEST_VER_WIN_NT));

// Highest recommend memory boundary

	SdkHandleWrap hJob(PrlSrv_GetCommonPrefs(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));

	SdkHandleWrap hDispConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hDispConfig.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hDispConfig, PHT_DISP_CONFIG);

	PRL_UINT32 nRecomnededMemSize = 0;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetRecommendMaxVmMem(hDispConfig, &nRecomnededMemSize));
	nRecomnededMemSize = (nRecomnededMemSize & 0xFFFFFFFCUL);

	CHECK_RET_CODE_EXP(PrlVmCfg_SetRamSize(m_VmHandle, nRecomnededMemSize + 4));

	CHECK_ONE_ERROR_VALIDATION(PVC_MAIN_MEMORY, PRL_ERR_VMCONF_MAIN_MEMORY_SIZE_NOT_EAQUAL_RECOMMENDED);

// Lowest recommend memory boundary

	nRecomnededMemSize = 64;

	CHECK_RET_CODE_EXP(PrlVmCfg_SetRamSize(m_VmHandle, nRecomnededMemSize - 4));

	CHECK_ONE_ERROR_VALIDATION(PVC_MAIN_MEMORY, PRL_ERR_VMCONF_MAIN_MEMORY_SIZE_NOT_EAQUAL_RECOMMENDED);
}

void PrlVmValidateConfigTest::testValidateConfigMainMemoryOnMemQuotaSettings()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmCfg_SetOsVersion(m_VmHandle, PVS_GUEST_VER_WIN_NT));
// Prepare memory size
	PRL_UINT32 nRamSize = 512;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetRamSize(m_VmHandle, nRamSize));
	PRL_UINT32 nVideoRamSize = 64;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVideoRamSize(m_VmHandle, nVideoRamSize));
////
	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemAutoQuota(m_VmHandle, PRL_FALSE));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetMaxBalloonSize(m_VmHandle, 101));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemQuotaPriority(m_VmHandle, 0));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemQuotaMin(m_VmHandle, 200));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemQuotaMax(m_VmHandle, 100));

	CHECK_THREE_ERRORS_VALIDATION(PVC_MAIN_MEMORY,
		PRL_ERR_VMCONF_MAIN_MEMORY_MAX_BALLOON_SIZE_MORE_100_PERCENT,
		PRL_ERR_VMCONF_MAIN_MEMORY_MQ_PRIOR_ZERO,
		PRL_ERR_VMCONF_MAIN_MEMORY_MQ_INVALID_RANGE);

	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemAutoQuota(m_VmHandle, PRL_TRUE));
	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_MAIN_MEMORY));
	CHECK_JOB_RET_CODE(hJob)
////
	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemAutoQuota(m_VmHandle, PRL_FALSE));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetMaxBalloonSize(m_VmHandle, 90));
// FIXME(dguryanov)
//	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemQuotaPriority(m_VmHandle, PMM_MEMPRIO_MAX + 1));

// FIXME(dguryanov)
//	PRL_UINT32 nSwapMem = nRamSize + nVideoRamSize;
//	PRL_UINT32 nVmmOverhead = CALC_APPROXIMATE_VMM_OVERHEAD(nSwapMem);
//	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemQuotaMin(m_VmHandle, nVmmOverhead - 4));
//	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemQuotaMax(m_VmHandle, nVmmOverhead + 4));

	CHECK_ONE_ERROR_VALIDATION(PVC_MAIN_MEMORY,
		PRL_ERR_VMCONF_MAIN_MEMORY_MQ_PRIOR_OUT_OF_RANGE);

	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemAutoQuota(m_VmHandle, PRL_TRUE));
	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_MAIN_MEMORY));
	CHECK_JOB_RET_CODE(hJob)
////
	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemAutoQuota(m_VmHandle, PRL_FALSE));
// FIXME(dguryanov)
//	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemQuotaPriority(m_VmHandle, PMM_MEMPRIO_MAX - 1));
//	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemQuotaMin(m_VmHandle, nSwapMem + nVmmOverhead + 4));
//	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemQuotaMax(m_VmHandle, nSwapMem + nVmmOverhead + 8));

	CHECK_ONE_ERROR_VALIDATION(PVC_MAIN_MEMORY, PRL_ERR_VMCONF_MAIN_MEMORY_MQ_MIN_OUT_OF_RANGE);

	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemAutoQuota(m_VmHandle, PRL_TRUE));
	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_MAIN_MEMORY));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlVmValidateConfigTest::testValidateConfigMainMemoryOnMemQuotaTryToEdit()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmCfg_SetOsVersion(m_VmHandle, PVS_GUEST_VER_WIN_NT));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, QTest::currentTestFunction()));
// Prepare memory size
	PRL_UINT32 nRamSize = 512;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetRamSize(m_VmHandle, nRamSize));
	PRL_UINT32 nVideoRamSize = 64;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVideoRamSize(m_VmHandle, nVideoRamSize));
// Auto memory quota enabled
	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemAutoQuota(m_VmHandle, PRL_TRUE));
// Invalid minimum quota value
// FIXME(dguryanov)
//	PRL_UINT32 nSwapMem = nRamSize + nVideoRamSize;
//	PRL_UINT32 nVmmOverhead = CALC_APPROXIMATE_VMM_OVERHEAD(nSwapMem);
//	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemQuotaMin(m_VmHandle, nSwapMem + nVmmOverhead + 4));
//	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemQuotaMax(m_VmHandle, nSwapMem + nVmmOverhead + 8));
////
	m_JobHandle.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(m_JobHandle);

// Auto memory quota disabled

	m_JobHandle.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle);

	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemAutoQuota(m_VmHandle, PRL_FALSE));

	m_JobHandle.reset(PrlVm_Commit(m_VmHandle));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT));
	PRL_RESULT nEditResult = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(m_JobHandle, &nEditResult));
	QVERIFY(nEditResult == PRL_ERR_INCONSISTENCY_VM_CONFIG);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()));
		CHECK_PARAMS_COUNT(hResult, 1);
			SdkHandleWrap hProblemDescription;
			CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, hProblemDescription.GetHandlePtr()));
			CHECK_HANDLE_TYPE(hProblemDescription, PHT_EVENT);
				PRL_RESULT nProblemErrCode = PRL_ERR_UNINITIALIZED;
				CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hProblemDescription, &nProblemErrCode));
				QVERIFY(nProblemErrCode == PRL_ERR_VMCONF_MAIN_MEMORY_MQ_MIN_OUT_OF_RANGE);
}

void PrlVmValidateConfigTest::testValidateConfigVideoMemoryOnValid()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmCfg_SetVideoRamSize(m_VmHandle, 23));

	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_VIDEO_MEMORY));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlVmValidateConfigTest::testValidateConfigVideoMemoryOnLess2()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmCfg_SetVideoRamSize(m_VmHandle, 1));

	CHECK_ONE_ERROR_VALIDATION(PVC_VIDEO_MEMORY, PRL_ERR_VMCONF_VIDEO_MEMORY_OUT_OF_RANGE);
}

void PrlVmValidateConfigTest::testValidateConfigVideoMemoryOnGreaterMax()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmCfg_SetVideoRamSize(m_VmHandle, VM_MAX_VIDEO_MEM + 1));

	CHECK_ONE_ERROR_VALIDATION(PVC_VIDEO_MEMORY, PRL_ERR_VMCONF_VIDEO_MEMORY_OUT_OF_RANGE);
}

void PrlVmValidateConfigTest::testValidateConfigFloppyDiskOnValid()
{
	testCreateVmFromConfig();

	SdkHandleWrap hJob;

	hJob.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

// Prepare

	QString qsVmDir;
	PRL_EXTRACT_STRING_VALUE(qsVmDir, m_VmHandle, PrlVmCfg_GetHomePath);
	qsVmDir = QFileInfo(qsVmDir).path();

	QString qsFile = qsVmDir + "/floppy.fdd";

// Add floppy device

	SdkHandleWrap hDevice;
	if ( PRL_FAILED(PrlVmCfg_GetDevByType(m_VmHandle, PDE_FLOPPY_DISK, 0, hDevice.GetHandlePtr())) )
	{
		hDevice.reset();
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hDevice.GetHandlePtr()));
	}

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8(qsFile)));
	CHECK_RET_CODE_EXP(PrlVmDev_SetFriendlyName(hDevice, QSTR2UTF8(qsFile)));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));

	hJob.reset(PrlVmDev_CreateImage(hDevice, PRL_FALSE, PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);
////
	hJob.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);
////
	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
// Disabled
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
// Disconnected
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);

// Remote
	{
		CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, "any path"));
		CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetRemote(hDevice, TRUE));

		hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
		CHECK_JOB_RET_CODE(hJob);
	}
}

void PrlVmValidateConfigTest::testValidateConfigFloppyDiskOnEmptySysName()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));

	CHECK_ONE_ERROR_VALIDATION(PVC_FLOPPY_DISK, PRL_ERR_VMCONF_FLOPPY_DISK_SYS_NAME_IS_EMPTY);
// Disable floppy device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));

	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
// Disconnect floppy device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigFloppyDiskOnInvalidSymbolInSysName()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hDevice.GetHandlePtr()));

	for(int i = 0; i < wrong_symbols.size() ; ++i)
	{
		CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));
		QString qsSysName = "floppy.fdd";
		qsSysName[3] = wrong_symbols.at(i);
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8(qsSysName)));
////////
		CHECK_ONE_ERROR_VALIDATION(PVC_FLOPPY_DISK, PRL_ERR_VMCONF_FLOPPY_DISK_SYS_NAME_HAS_INVALID_SYMBOL);
// Disable floppy device
		CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));

		SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
		CHECK_JOB_RET_CODE(hJob);
// Disconnect floppy device
		CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));

		hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
		CHECK_JOB_RET_CODE(hJob);
	}
}

void PrlVmValidateConfigTest::testValidateConfigFloppyDiskOnNotExistImageFile()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hDevice.GetHandlePtr()));

	QString qsNotExistFile = QDir::tempPath() + "/floppy.fdd";
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, qsNotExistFile.toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));

	CHECK_ONE_ERROR_VALIDATION(PVC_FLOPPY_DISK, PRL_ERR_VMCONF_FLOPPY_DISK_IMAGE_IS_NOT_EXIST);
// Disable floppy device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));

	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
// Disconnect floppy device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigFloppyDiskOnInvalidImageFile()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

// Create file
	m_qsFile = QDir::tempPath() + "/floppy.fdd";
	QFile file(m_qsFile);
	QVERIFY(file.open(QIODevice::ReadWrite));
	QByteArray ba(1234, 'B');
	QVERIFY(file.write(ba));

// Add floppy device
	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, m_qsFile.toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));

	CHECK_ONE_ERROR_VALIDATION(PVC_FLOPPY_DISK, PRL_ERR_VMCONF_FLOPPY_DISK_IMAGE_IS_NOT_VALID);
// Disable floppy device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));

	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
// Disconnect floppy device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigFloppyDiskOnRealFloppyDisk()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hJob(PrlSrv_GetSrvConfig(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));

	SdkHandleWrap hServerConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hServerConfig.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hServerConfig, PHT_SERVER_CONFIG);

	PRL_UINT32 nFloppyCount = 0;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetFloppyDisksCount(hServerConfig, &nFloppyCount));

	if (nFloppyCount != 0)
	{
		QSKIP("Floppy disk presents on the host, but it has to be absent for this test!", SkipAll);
	}

	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, "Floppy disk"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));

	CHECK_ONE_ERROR_VALIDATION(PVC_FLOPPY_DISK, PRL_ERR_VMCONF_FLOPPY_DISK_IS_NOT_ACCESSIBLE);
// Disable floppy device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
// Disconnect floppy device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
}

#define SYS_NAMES_IN_URL_FORMAT "ftp://my.net" << "http://my.net" << "https://my.net" \
	<< "smb://my.net" << "nfs://"

void PrlVmValidateConfigTest::testValidateConfigFloppyDiskOnUrlSysName()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));

	QStringList lstSysNames;
	lstSysNames << SYS_NAMES_IN_URL_FORMAT;
	foreach(QString qsSysName, lstSysNames)
	{
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8(qsSysName)));

		CHECK_ONE_ERROR_VALIDATION(PVC_FLOPPY_DISK, PRL_ERR_VMCONF_FLOPPY_DISK_URL_FORMAT_SYS_NAME);
	}

// Real device
	SdkHandleWrap hJob(PrlSrv_GetSrvConfig(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));

	SdkHandleWrap hServerConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hServerConfig.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hServerConfig, PHT_SERVER_CONFIG);

	PRL_UINT32 nFloppyCount = 0;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetFloppyDisksCount(hServerConfig, &nFloppyCount));

	if (nFloppyCount)//Check just for hosts with floppy disk hardware present
	{
		CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));

		hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
		CHECK_JOB_RET_CODE(hJob);
	}
// Disable device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
// Disconnect device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigCdDvdRomOnValid()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

// Create file
	m_qsFile = QDir::tempPath() + "/image.iso";
	QFile file(m_qsFile);
	QVERIFY(file.open(QIODevice::ReadWrite));
	QByteArray ba(100 * 1024, 'B');
	QVERIFY(file.write(ba));

// Add CD/DVD-ROM device
	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, m_qsFile.toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));

	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_CD_DVD_ROM));
	CHECK_JOB_RET_CODE(hJob);
// Disabled
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_CD_DVD_ROM));
	CHECK_JOB_RET_CODE(hJob);
// Disconnected
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_CD_DVD_ROM));
	CHECK_JOB_RET_CODE(hJob);
// Remote
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, "any path"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetRemote(hDevice, TRUE));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigCdDvdRomOnEmptySysName()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
// 1 Add optical disk with valid name
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 0));
// 2 Add optical disk with empty name
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 1));
// 3 Add optical disk with empty name but device is not enabled
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
// 4 Add optical disk with empty name but device is not connected
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
////
	CHECK_ONE_ERROR_VALIDATION(PVC_CD_DVD_ROM, PRL_ERR_VMCONF_CD_DVD_ROM_SYS_NAME_IS_EMPTY);
}

void PrlVmValidateConfigTest::testValidateConfigCdDvdRomOnDuplicateStackIndex()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
// 1 Add IDE optical disk
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 3));
// 2 Add IDE optical disk with duplicated stack index
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 3));
// 3 Add SCSI optical disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
// 4 Add SCSI optical disk with duplicated stack index
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
////
	CHECK_TWO_ERRORS_VALIDATION(PVC_CD_DVD_ROM,
								PRL_ERR_VMCONF_IDE_DEVICES_DUPLICATE_STACK_INDEX,
								PRL_ERR_VMCONF_SCSI_DEVICES_DUPLICATE_STACK_INDEX);
}

void PrlVmValidateConfigTest::testValidateConfigCdDvdRomOnDuplicateSysName()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
// 1 Add IDE optical disk
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, "Name dup"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 4));
// 2 Add IDE optical disk with duplicated sys name
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, "Name dup"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
// 3 Add SCSI optical disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, "Another name"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 1));
// 4 Add SCSI optical disk with duplicated sys name but device is not enabled
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, "Name dup"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 3));
// 5 Add SCSI optical disk with duplicated sys name but device is not connected
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, "Name dup"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 3));
////
	CHECK_ONE_ERROR_VALIDATION(PVC_CD_DVD_ROM, PRL_ERR_VMCONF_CD_DVD_ROM_DUPLICATE_SYS_NAME);
}

void PrlVmValidateConfigTest::testValidateConfigCdDvdRomOnNotExistImageFile()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

	QString qsNotExistFile = QDir::tempPath() + "/image.iso";
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, qsNotExistFile.toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));

	CHECK_ONE_ERROR_VALIDATION(PVC_CD_DVD_ROM, PRL_ERR_VMCONF_CD_DVD_ROM_IMAGE_IS_NOT_EXIST);
// Disable CD/DVD-ROM device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));

	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_CD_DVD_ROM));
	CHECK_JOB_RET_CODE(hJob);
// Disconnect CD/DVD-ROM device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_CD_DVD_ROM));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigCdDvdRomOnUrlSysName()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));

	QStringList lstSysNames;
	lstSysNames << SYS_NAMES_IN_URL_FORMAT;
	foreach(QString qsSysName, lstSysNames)
	{
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8(qsSysName)));

		CHECK_ONE_ERROR_VALIDATION(PVC_CD_DVD_ROM, PRL_ERR_VMCONF_CD_DVD_ROM_URL_FORMAT_SYS_NAME);
	}

// Real device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));

	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
// Disable device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
// Disconnect device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigHardDiskOnValid()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));

	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_HARD_DISK));
	CHECK_JOB_RET_CODE(hJob);

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_HARD_DISK));
	CHECK_JOB_RET_CODE(hJob);

	//Remote device
	{
		CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, "any path"));
		CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetRemote(hDevice, TRUE));

		hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_HARD_DISK));
		CHECK_JOB_RET_CODE(hJob);
	}
}

void PrlVmValidateConfigTest::testValidateConfigHardDiskOnEmptySysName()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
// 1 Add hard disk with valid name
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 0));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 2 Add hard disk with empty name
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 1));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 3 Add hard disk with empty name but device is not enabled
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
////
	CHECK_ONE_ERROR_VALIDATION(PVC_HARD_DISK, PRL_ERR_VMCONF_HARD_DISK_SYS_NAME_IS_EMPTY);
}

void PrlVmValidateConfigTest::testValidateConfigHardDiskOnInvalidSymbolInSysName()
{
	for(int i = 0; i < wrong_symbols.size() ; ++i)
	{
		QString qsSysName = "hard_disk.hdd";
		qsSysName[5] = wrong_symbols.at(i);

		m_VmHandle.reset();
		CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

		SdkHandleWrap hDevice;
// 1 Add hard disk with invalid name
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

		CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8(qsSysName)));
		CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 0));
		CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));
// 2 Add hard disk with invalid name but real device
		hDevice.reset();
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

		CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8(qsSysName)));
		CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 1));
		CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 3 Add hard disk with invalid name but device is not enabled
		hDevice.reset();
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

		CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8(qsSysName)));
		CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
		CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));
////////
		CHECK_ONE_ERROR_VALIDATION(PVC_HARD_DISK, PRL_ERR_VMCONF_HARD_DISK_SYS_NAME_HAS_INVALID_SYMBOL);
	}
}

void PrlVmValidateConfigTest::testValidateConfigHardDiskOnSysNameAsHddImageIsInvalid()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

// Create file
	m_qsFile = QDir::tempPath() + "/harddisk.hdd";
	QFile file(m_qsFile);
	QVERIFY(file.open(QIODevice::ReadWrite));
	QByteArray ba(100 * 1024, 'B');
	QVERIFY(file.write(ba));

	SdkHandleWrap hDevice;
// 1 Add hard disk with existing sys name as path
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, m_qsFile.toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 0));
// 2 Add hard disk with not existing sys name as path
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 1));
// 3 Add hard disk with not existing sys name as path but device is not enabled
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
// 4 Add hard disk with not existing sys name as path but device is real
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 3));
////
	CHECK_TWO_ERRORS_VALIDATION(PVC_HARD_DISK,
								PRL_ERR_VMCONF_HARD_DISK_IMAGE_IS_NOT_VALID,
								PRL_ERR_VMCONF_HARD_DISK_IMAGE_IS_NOT_EXIST);
}

void PrlVmValidateConfigTest::testValidateConfigHardDiskOnDuplicateStackIndex()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
// 1 Add IDE hard disk
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 3));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 2 Add IDE hard disk with duplicated stack index
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 3));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 3 Add SCSI hard disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 4 Add SCSI hard disk with duplicated stack index
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
////
	CHECK_TWO_ERRORS_VALIDATION(PVC_HARD_DISK,
								PRL_ERR_VMCONF_IDE_DEVICES_DUPLICATE_STACK_INDEX,
								PRL_ERR_VMCONF_SCSI_DEVICES_DUPLICATE_STACK_INDEX);
}

void PrlVmValidateConfigTest::testValidateConfigHardDiskOnDuplicateSysName()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
// 1 Add IDE hard disk
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, "Name dup"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 3));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 2 Add IDE hard disk with duplicated sys name
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, "Name dup"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 4));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 3 Add SCSI hard disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, "Another name"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 4 Add SCSI hard disk with duplicated sys name but device is not enabled
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, "Name dup"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 1));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
////
	CHECK_ONE_ERROR_VALIDATION(PVC_HARD_DISK, PRL_ERR_VMCONF_HARD_DISK_DUPLICATE_SYS_NAME);
}

void PrlVmValidateConfigTest::testValidateConfigHardDiskOnUrlSysName()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));

	QStringList lstSysNames;
	lstSysNames << SYS_NAMES_IN_URL_FORMAT;
	foreach(QString qsSysName, lstSysNames)
	{
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8(qsSysName)));

		CHECK_ONE_ERROR_VALIDATION(PVC_HARD_DISK, PRL_ERR_VMCONF_HARD_DISK_URL_FORMAT_SYS_NAME);
	}

// Real device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));

	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
// Disable device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
// Disconnect device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigNetworkAdapterOnValid()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hJob(PrlSrv_GetSrvConfig(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));

	SdkHandleWrap hServerConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hServerConfig.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hServerConfig, PHT_SERVER_CONFIG);

	PRL_UINT32 nAdapterCount = 0;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetNetAdaptersCount(hServerConfig, &nAdapterCount));

	for(PRL_UINT32 i = 0;  i < nAdapterCount + 3; i++)
	{
		SdkHandleWrap hDevice;
		if (i < nAdapterCount)
		{
			CHECK_RET_CODE_EXP(PrlSrvCfg_GetNetAdapter(hServerConfig, i, hDevice.GetHandlePtr()));
			CHECK_HANDLE_TYPE(hDevice, PHT_HW_NET_ADAPTER);

			PRL_UINT32 nSysIndex = 0;
			CHECK_RET_CODE_EXP(PrlSrvCfgNet_GetSysIndex(hDevice, &nSysIndex));

			char adapterName[512];
			PRL_UINT32 nameSize = sizeof(adapterName);
			CHECK_RET_CODE_EXP(PrlSrvCfgDev_GetName(hDevice, adapterName, &nameSize));

			CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

			CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )PNA_BRIDGED_ETHERNET));
			CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
			CHECK_RET_CODE_EXP(PrlVmDevNet_GenerateMacAddr(hDevice));
			CHECK_RET_CODE_EXP(PrlVmDevNet_SetBoundAdapterIndex(hDevice,nSysIndex));
			CHECK_RET_CODE_EXP(PrlVmDevNet_SetBoundAdapterName(hDevice,adapterName));
		}
		else if (i == nAdapterCount )
		{
			// add default adapter;
			CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

			CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )PNA_BRIDGED_ETHERNET));
			CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
			CHECK_RET_CODE_EXP(PrlVmDevNet_GenerateMacAddr(hDevice));
			CHECK_RET_CODE_EXP(PrlVmDevNet_SetBoundAdapterName(hDevice, "AdapterXXX"));
			CHECK_RET_CODE_EXP(PrlVmDevNet_SetBoundAdapterIndex(hDevice,-1));
		}
		else
		{
			CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

			CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )PNA_SHARED));
			CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
			CHECK_RET_CODE_EXP(PrlVmDevNet_SetMacAddress(hDevice, "000000000000"));
		}
	}

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_NETWORK_ADAPTER));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::CheckWhetherSharedModeEnabled( bool &bSharedModeEnabled )
{
	SdkHandleWrap hJob(PrlSrv_GetCommonPrefs( m_ServerHandle ));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult( hJob, hResult.GetHandlePtr() ))
	SdkHandleWrap hDispConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam( hResult, hDispConfig.GetHandlePtr() ))
	CHECK_HANDLE_TYPE(hDispConfig, PHT_DISP_CONFIG)
	PRL_UINT32 nAdaptersCount = 0;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetDispNetCount( hDispConfig, &nAdaptersCount ))
	for ( PRL_UINT32 i = 0; i < nAdaptersCount; ++i )
	{
		SdkHandleWrap hDispNet;
		CHECK_RET_CODE_EXP(PrlDispCfg_GetDispNet( hDispConfig, i, hDispNet.GetHandlePtr() ))
		CHECK_HANDLE_TYPE(hDispNet, PHT_DISP_NET_ADAPTER)
		PRL_NET_ADAPTER_EMULATED_TYPE nAdapterType = PNA_HOST_ONLY;
		CHECK_RET_CODE_EXP(PrlDispNet_GetNetworkType( hDispNet, &nAdapterType ))
		if ( PNA_SHARED == nAdapterType )
		{
			bSharedModeEnabled = true;
			break;
		}
	}
}

bool PrlVmValidateConfigTest::IsSharedNetworkingTypeEnabled()
{
	static bool g_bFirstTime = true;
	static bool g_bSharedModeEnabled = false;
	if ( g_bFirstTime )
	{
		g_bFirstTime = false;
		CheckWhetherSharedModeEnabled( g_bSharedModeEnabled );
	}
	return g_bSharedModeEnabled;
}

void PrlVmValidateConfigTest::testValidateConfigNetworkAdapterOnInvalidAdapter()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;

	// 1 Add bridged ethernet network adapter with invalid adapter name
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )PNA_BRIDGED_ETHERNET));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDevNet_GenerateMacAddr(hDevice));
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetBoundAdapterName(hDevice, "AdapterXXX 1"));

	// 2 Add bridged ethernet network adapter with invalid adapter name
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )PNA_BRIDGED_ETHERNET));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDevNet_GenerateMacAddr(hDevice));
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetBoundAdapterName(hDevice, "AdapterXXX 2"));

	// 3 Add shared network adapter with invalid adapter name
	if ( IsSharedNetworkingTypeEnabled() )
	{
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

		CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )PNA_SHARED));
		CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDevNet_GenerateMacAddr(hDevice));
		CHECK_RET_CODE_EXP(PrlVmDevNet_SetBoundAdapterName(hDevice, "AdapterXXX Shared"));
	}

	// 4 Add bridged ethernet network adapter with invalid index but device is not enabled
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )PNA_BRIDGED_ETHERNET));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDevNet_GenerateMacAddr(hDevice));
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetBoundAdapterName(hDevice, "AdapterXXX 3"));

	// 5 Add bridged ethernet network adapter with invalid index but device is direct assign
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_DIRECT_ASSIGN));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDevNet_GenerateMacAddr(hDevice));
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetBoundAdapterName(hDevice, "AdapterXXX 4"));
////
	CHECK_TWO_ERRORS_VALIDATION(PVC_NETWORK_ADAPTER,
								PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_BOUND_INDEX,
								PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_BOUND_INDEX);
}

void PrlVmValidateConfigTest::testValidateConfigNetworkAdapterOnDefaultSysIndex()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )PNA_BRIDGED_ETHERNET));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDevNet_GenerateMacAddr(hDevice));
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetBoundAdapterIndex(hDevice, (PRL_UINT32 )-1));

	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_NETWORK_ADAPTER));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigNetworkAdapterOnInvalidMacAddress()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));
	// Wrong symbol
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetMacAddress(hDevice, "000CALB00756"),
						PRL_ERR_INVALID_ARG);
	// Wrong size (< 12)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetMacAddress(hDevice, "000a07d92E"),
						PRL_ERR_INVALID_ARG);
	// Wrong size (> 12)
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetMacAddress(hDevice, "000a07d923450F"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )PNA_HOST_ONLY));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 1 Add network adapter with invalid MAC address
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

	// Null address
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetMacAddress(hDevice, "000000000000"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )PNA_HOST_ONLY));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 2 Add network adapter with invalid MAC address
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));
	// Multicast address (1st bit of 1st byte is on)
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetMacAddress(hDevice, "ab230ab000c0"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )PNA_HOST_ONLY));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 3 Add network adapter with invalid MAC address but device is direct assign
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDevNet_SetMacAddress(hDevice, "000000000000"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_DIRECT_ASSIGN));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 4 Add network adapter with valid MAC address but not belongs to Parallels interval
// https://bugzilla.sw.ru/show_bug.cgi?id=437789
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetMacAddress(hDevice, "0007E909DCC6"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE)PNA_HOST_ONLY));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
////
	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_NETWORK_ADAPTER));
	CHECK_VMCONF_VALIDATION_FAILED(hJob);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));

	CHECK_PARAMS_COUNT(hResult, 2)

	SdkHandleWrap hProblemDescription;
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, hProblemDescription.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hProblemDescription, PHT_EVENT);
	PRL_RESULT nProblemErrCode = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hProblemDescription, &nProblemErrCode));
	CHECK_CONCRETE_EXPRESSION_RET_CODE(nProblemErrCode, PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_MAC_ADDRESS)

	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 1, hProblemDescription.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hProblemDescription, PHT_EVENT);
	CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hProblemDescription, &nProblemErrCode));
	CHECK_CONCRETE_EXPRESSION_RET_CODE(nProblemErrCode, PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_MAC_ADDRESS)
}

void PrlVmValidateConfigTest::testValidateConfigNetworkAdapterOnDuplicateMacAddress()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
// 1 Add network adapter with valid MAC address
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )COMPATIBLE_ADAPTER_TYPE));
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetMacAddress(hDevice, "001C42025667"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 2 Add network adapter with valid MAC address
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )COMPATIBLE_ADAPTER_TYPE));
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetMacAddress(hDevice, "001c4223aB03"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 3 Add network adapter with valid MAC address
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )COMPATIBLE_ADAPTER_TYPE));
	CHECK_RET_CODE_EXP(PrlVmDevNet_GenerateMacAddr(hDevice));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 4 (1) Add network adapter with duplicated MAC address
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )COMPATIBLE_ADAPTER_TYPE));
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetMacAddress(hDevice, "001c4223aB03"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 5 (2) Add network adapter with duplicated MAC address
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )COMPATIBLE_ADAPTER_TYPE));
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetMacAddress(hDevice, "001c4223aB03"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 6 (3) Add network adapter with duplicated MAC address
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )COMPATIBLE_ADAPTER_TYPE));
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetMacAddress(hDevice, "001C42025667"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 7 Add network adapter with valid MAC address
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )COMPATIBLE_ADAPTER_TYPE));
	CHECK_RET_CODE_EXP(PrlVmDevNet_GenerateMacAddr(hDevice));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 8 Add network adapter with duplicated MAC address but device is direct assign
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_DIRECT_ASSIGN));
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetMacAddress(hDevice, "001C42025667"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
////
	CHECK_THREE_ERRORS_VALIDATION(PVC_NETWORK_ADAPTER,
								  PRL_ERR_VMCONF_NETWORK_ADAPTER_DUPLICATE_MAC_ADDRESS,
								  PRL_ERR_VMCONF_NETWORK_ADAPTER_DUPLICATE_MAC_ADDRESS,
								  PRL_ERR_VMCONF_NETWORK_ADAPTER_DUPLICATE_MAC_ADDRESS);
}

void PrlVmValidateConfigTest::testValidateConfigNetworkAdapterOnDuplicateIpAddress()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	QStringList lstNewNetAddresses;
	SdkHandleWrap hNewNetAddressesList;
// 1 Add network adapter with duplicate IPs
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )COMPATIBLE_ADAPTER_TYPE));
	CHECK_RET_CODE_EXP(PrlVmDevNet_GenerateMacAddr(hDevice));

	lstNewNetAddresses = QStringList()<<"192.168.1.1/255.255.255.0"<<"10.30.1.1/22"<<"192.168.1.1/16";
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hNewNetAddressesList.GetHandlePtr()))
	foreach(QString sNetAddress, lstNewNetAddresses)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewNetAddressesList, QSTR2UTF8(sNetAddress)))
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetNetAddresses(hDevice, hNewNetAddressesList))

	CHECK_RET_CODE_EXP(PrlVmDevNet_SetAutoApply(hDevice,TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 2 Add network adapter with dublicates from first adapter
	hNewNetAddressesList.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )COMPATIBLE_ADAPTER_TYPE));
	CHECK_RET_CODE_EXP(PrlVmDevNet_GenerateMacAddr(hDevice));

	lstNewNetAddresses = QStringList()<<"192.168.1.2/255.255.0.0"<<"10.30.1.1/255.0.0.0"<<"10.30.1.4";
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hNewNetAddressesList.GetHandlePtr()))
	foreach(QString sNetAddress, lstNewNetAddresses)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewNetAddressesList, QSTR2UTF8(sNetAddress)))
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetNetAddresses(hDevice, hNewNetAddressesList))

	CHECK_RET_CODE_EXP(PrlVmDevNet_SetAutoApply(hDevice,TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
////
	CHECK_TWO_ERRORS_VALIDATION(PVC_NETWORK_ADAPTER,
				PRL_ERR_VMCONF_NETWORK_ADAPTER_DUPLICATE_IP_ADDRESS,
				PRL_ERR_VMCONF_NETWORK_ADAPTER_DUPLICATE_IP_ADDRESS);
}

void PrlVmValidateConfigTest::testValidateConfigNetworkAdapterOnInvalidIpAddress()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	QStringList lstNewNetAddresses;
	CVmConfiguration _vm_conf;

	PRL_VOID_PTR pVmConfData = 0;
	CHECK_RET_CODE_EXP(PrlVm_ToString(m_VmHandle, &pVmConfData))
	QString	_config = UTF8_2QSTR((const char *)pVmConfData);
	PrlBuffer_Free(pVmConfData);
	CHECK_RET_CODE_EXP(_vm_conf.fromString(_config))

	_vm_conf.getVmHardwareList()->m_lstNetworkAdapters.clear();

// Add network adapter with invalid IPs
	lstNewNetAddresses = QStringList()<<"192.168.1.1.1/255.255.255.0"<<"10.30.1.1/22"<<"192.168.1.1/33";
	CVmGenericNetworkAdapter *pNetAdapter = new CVmGenericNetworkAdapter();
	pNetAdapter->setMacAddress(HostUtils::generateMacAddress());
	pNetAdapter->setNetAddresses(lstNewNetAddresses);
	pNetAdapter->setEmulatedType( (PRL_VM_DEV_EMULATION_TYPE )COMPATIBLE_ADAPTER_TYPE);
	pNetAdapter->setAutoApply( TRUE );
	pNetAdapter->setEnabled( TRUE );

	_vm_conf.getVmHardwareList()->addNetworkAdapter(pNetAdapter);
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()))
////
	CHECK_TWO_ERRORS_VALIDATION(PVC_NETWORK_ADAPTER,
				PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_IP_ADDRESS,
				PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_IP_ADDRESS);

// Check invalid v6 IPs
	lstNewNetAddresses = QStringList()<<"ccc::1:1/200"<<"1::h:1/33"<<"ee::1/64";
	pNetAdapter->setNetAddresses(lstNewNetAddresses);

	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()))
	CHECK_TWO_ERRORS_VALIDATION(PVC_NETWORK_ADAPTER,
				PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_IP_ADDRESS,
				PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_IP_ADDRESS);
}

void PrlVmValidateConfigTest::testValidateConfigNetworkAdapterOnMulticastAddress()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	QStringList lstNewNetAddresses;
	SdkHandleWrap hNewNetAddressesList;
// Add network adapter with multicast IPs
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )COMPATIBLE_ADAPTER_TYPE));
	CHECK_RET_CODE_EXP(PrlVmDevNet_GenerateMacAddr(hDevice));

	lstNewNetAddresses = QStringList()
		<< "224.1.2.3"					// MC
		<< "240.0.0.1/255.255.255.0"	// no MC
		<< "237.27.255.255/16";			// MC
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hNewNetAddressesList.GetHandlePtr()))
	foreach(QString sNetAddress, lstNewNetAddresses)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewNetAddressesList, QSTR2UTF8(sNetAddress)))
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetNetAddresses(hDevice, hNewNetAddressesList))

	CHECK_RET_CODE_EXP(PrlVmDevNet_SetAutoApply(hDevice,TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
////
	CHECK_TWO_ERRORS_VALIDATION(PVC_NETWORK_ADAPTER,
				PRL_ERR_VMCONF_NETWORK_ADAPTER_MULTICAST_IP_ADDRESS,
				PRL_ERR_VMCONF_NETWORK_ADAPTER_MULTICAST_IP_ADDRESS);
}

void PrlVmValidateConfigTest::testValidateConfigNetworkAdapterOnBroadcastAddress()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	QStringList lstNewNetAddresses;
	SdkHandleWrap hNewNetAddressesList;
// Add network adapter with broadcasst IPs
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )COMPATIBLE_ADAPTER_TYPE));
	CHECK_RET_CODE_EXP(PrlVmDevNet_GenerateMacAddr(hDevice));

	lstNewNetAddresses = QStringList()
		<< "10.43.255.255/255.255.0.0"	// BC
		<< "176.25.2.3/20"				// no BC
		<< "208.137.15.255/20";			// BC
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hNewNetAddressesList.GetHandlePtr()))
	foreach(QString sNetAddress, lstNewNetAddresses)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewNetAddressesList, QSTR2UTF8(sNetAddress)))
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetNetAddresses(hDevice, hNewNetAddressesList))

	CHECK_RET_CODE_EXP(PrlVmDevNet_SetAutoApply(hDevice,TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
////
	CHECK_TWO_ERRORS_VALIDATION(PVC_NETWORK_ADAPTER,
				PRL_ERR_VMCONF_NETWORK_ADAPTER_BROADCAST_IP_ADDRESS,
				PRL_ERR_VMCONF_NETWORK_ADAPTER_BROADCAST_IP_ADDRESS);
}

void PrlVmValidateConfigTest::testValidateConfigNetworkAdapterOnInvalidGatewayIpAddress()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	CVmConfiguration _vm_conf;

	PRL_VOID_PTR pVmConfData = 0;
	CHECK_RET_CODE_EXP(PrlVm_ToString(m_VmHandle, &pVmConfData))
	QString	_config = UTF8_2QSTR((const char *)pVmConfData);
	PrlBuffer_Free(pVmConfData);
	CHECK_RET_CODE_EXP(_vm_conf.fromString(_config))

	_vm_conf.getVmHardwareList()->m_lstNetworkAdapters.clear();

	int i = 0;
// 1 Add network adapter with invalid default gateway IP
	CVmGenericNetworkAdapter *pNetAdapter = new CVmGenericNetworkAdapter();
	pNetAdapter->setEmulatedType( (PRL_VM_DEV_EMULATION_TYPE )COMPATIBLE_ADAPTER_TYPE);
	pNetAdapter->setMacAddress(HostUtils::generateMacAddress());
	pNetAdapter->setIndex(i++);
	pNetAdapter->setDefaultGateway("a.b.20.34");
	pNetAdapter->setAutoApply( TRUE );
	pNetAdapter->setEnabled( TRUE );
	_vm_conf.getVmHardwareList()->addNetworkAdapter(pNetAdapter);

// 2 Add network adapter with valid default gateway IP
	pNetAdapter = new CVmGenericNetworkAdapter();
	pNetAdapter->setEmulatedType( (PRL_VM_DEV_EMULATION_TYPE )COMPATIBLE_ADAPTER_TYPE);
	pNetAdapter->setMacAddress(HostUtils::generateMacAddress());
	pNetAdapter->setIndex(i++);
	pNetAdapter->setDefaultGateway("10.20.30.40");
	pNetAdapter->setAutoApply( TRUE );
	pNetAdapter->setEnabled( TRUE );
	_vm_conf.getVmHardwareList()->addNetworkAdapter(pNetAdapter);

// 3 Add network adapter with invalid default gateway IP
	pNetAdapter = new CVmGenericNetworkAdapter();
	pNetAdapter->setEmulatedType( (PRL_VM_DEV_EMULATION_TYPE )COMPATIBLE_ADAPTER_TYPE);
	pNetAdapter->setMacAddress(HostUtils::generateMacAddress());
	pNetAdapter->setIndex(i++);
	pNetAdapter->setDefaultGateway("1.344.15.2");
	pNetAdapter->setAutoApply( TRUE );
	pNetAdapter->setEnabled( TRUE );
	_vm_conf.getVmHardwareList()->addNetworkAdapter(pNetAdapter);

// 4 Add network adapter with invalid default gateway IP but not enabled
	pNetAdapter = new CVmGenericNetworkAdapter();
	pNetAdapter->setEmulatedType( (PRL_VM_DEV_EMULATION_TYPE )COMPATIBLE_ADAPTER_TYPE);
	pNetAdapter->setMacAddress(HostUtils::generateMacAddress());
	pNetAdapter->setIndex(i++);
	pNetAdapter->setDefaultGateway("37.z.67.211");
	pNetAdapter->setAutoApply( TRUE );
	pNetAdapter->setEnabled( FALSE );
	_vm_conf.getVmHardwareList()->addNetworkAdapter(pNetAdapter);

// 5 Add network adapter with empty default gateway IP
	pNetAdapter = new CVmGenericNetworkAdapter();
	pNetAdapter->setEmulatedType( (PRL_VM_DEV_EMULATION_TYPE )COMPATIBLE_ADAPTER_TYPE);
	pNetAdapter->setMacAddress(HostUtils::generateMacAddress());
	pNetAdapter->setIndex(i++);
	pNetAdapter->setDefaultGateway("");
	pNetAdapter->setAutoApply( TRUE );
	pNetAdapter->setEnabled( TRUE );
	_vm_conf.getVmHardwareList()->addNetworkAdapter(pNetAdapter);

	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()))
////
	CHECK_TWO_ERRORS_VALIDATION(PVC_NETWORK_ADAPTER,
				PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_GATEWAY_IP_ADDRESS,
				PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_GATEWAY_IP_ADDRESS);
}

void PrlVmValidateConfigTest::testValidateConfigNetworkAdapterOnInvalidGatewayIpAddress2()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetDefaultGateway(hDevice, "a.b.20.34"), PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetDefaultGateway(hDevice, "1.344.15.2"), PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetDefaultGateway(hDevice, "37.z.67.211"), PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetDefaultGateway(hDevice, "2a00::1"), PRL_ERR_INVALID_ARG);
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetDefaultGateway(hDevice, "10.10.20.34"));
}

void PrlVmValidateConfigTest::testValidateConfigNetworkAdapterOnGatewayNotInAnySubnet()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	QStringList lstNewNetAddresses;
	SdkHandleWrap hNewNetAddressesList;
// Add network adapter with default gateway IP in ton any subnet
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )COMPATIBLE_ADAPTER_TYPE));
	CHECK_RET_CODE_EXP(PrlVmDevNet_GenerateMacAddr(hDevice));

	lstNewNetAddresses = QStringList()
		<< "10.43.34.15/255.255.255.0"
		<< "75.25.2.3/16"
		<< "208.137.15.45/8";
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hNewNetAddressesList.GetHandlePtr()))
	foreach(QString sNetAddress, lstNewNetAddresses)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewNetAddressesList, QSTR2UTF8(sNetAddress)))
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetNetAddresses(hDevice, hNewNetAddressesList))

	CHECK_RET_CODE_EXP(PrlVmDevNet_SetDefaultGateway(hDevice, "192.131.0.1"));

	CHECK_RET_CODE_EXP(PrlVmDevNet_SetAutoApply(hDevice,TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
////
	CHECK_ONE_ERROR_VALIDATION(PVC_NETWORK_ADAPTER,
				PRL_ERR_VMCONF_NETWORK_ADAPTER_GATEWAY_NOT_IN_SUBNET);
////
// Gateway in subnet
	CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewNetAddressesList, "192.140.17.25/255.128.0.0"));
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetNetAddresses(hDevice, hNewNetAddressesList))
////
	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_NETWORK_ADAPTER));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigNetworkAdapterOnInvalidDnsIpAddress()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	QStringList lstNewDnsAddresses;
	SdkHandleWrap hNewDnsAddressesList;
// Add network adapter with invalid DNS IPs
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )COMPATIBLE_ADAPTER_TYPE));
	CHECK_RET_CODE_EXP(PrlVmDevNet_GenerateMacAddr(hDevice));

	lstNewDnsAddresses = QStringList()
		<< "10.20.AB.40"
		<< "75.76.77.78"
		<< "192.168.444.1";
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hNewDnsAddressesList.GetHandlePtr()))
	foreach(QString sDnsAddress, lstNewDnsAddresses)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewDnsAddressesList, QSTR2UTF8(sDnsAddress)))
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetDnsServers(hDevice, hNewDnsAddressesList))

	CHECK_RET_CODE_EXP(PrlVmDevNet_SetAutoApply(hDevice,TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
////
	CHECK_TWO_ERRORS_VALIDATION(PVC_NETWORK_ADAPTER,
				PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_DNS_IP_ADDRESS,
				PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_DNS_IP_ADDRESS);
}

void PrlVmValidateConfigTest::testValidateConfigNetworkAdapterOnInvalidSearchDomainName()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	QStringList lstNewSearchDomains;
	SdkHandleWrap hNewSearchDomainsList;
// Add network adapter with invalid search domain names
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, (PRL_VM_DEV_EMULATION_TYPE )COMPATIBLE_ADAPTER_TYPE));
	CHECK_RET_CODE_EXP(PrlVmDevNet_GenerateMacAddr(hDevice));

	lstNewSearchDomains = QStringList()
		<< "ab+/adsk@f11.com"			// INV
		<< "234566-dfnjsdnfjn.net"		// VALID
		<< "hasdgh"						// VALID
		<< "mydomain."					// INV
		<< "a32.b647.c87.z23-4a-sd7"	// VALID
		<< ""							// VALID
		<< ".ASDA.SDSD";				// INV
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hNewSearchDomainsList.GetHandlePtr()))
	foreach(QString sDomain, lstNewSearchDomains)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewSearchDomainsList, QSTR2UTF8(sDomain)))
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetSearchDomains(hDevice, hNewSearchDomainsList))

	CHECK_RET_CODE_EXP(PrlVmDevNet_SetAutoApply(hDevice,TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
////
	CHECK_THREE_ERRORS_VALIDATION(PVC_NETWORK_ADAPTER,
				PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_SEARCH_DOMAIN_NAME,
				PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_SEARCH_DOMAIN_NAME,
				PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_SEARCH_DOMAIN_NAME);
}

void PrlVmValidateConfigTest::testValidateConfigSerialPortOnValid()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));
// Add serial port device
	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));

	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_SERIAL_PORT));
	CHECK_JOB_RET_CODE(hJob);
// Disabled
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_SERIAL_PORT));
	CHECK_JOB_RET_CODE(hJob);
// Disconnected
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_SERIAL_PORT));
	CHECK_JOB_RET_CODE(hJob);

	//Remote device
	{
		CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, "any path"));
		CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_OUTPUT_FILE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetRemote(hDevice, TRUE));

		hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_SERIAL_PORT));
		CHECK_JOB_RET_CODE(hJob);
	}
}

void PrlVmValidateConfigTest::testValidateConfigSerialPortOnEmptySysName()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
// 1 Add serial port with valid name
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
// 2 Add serial port with empty name
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));
// 3 Add serial port with empty name but device is not enabled
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));
// 4 Add serial port with empty name
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));
// 5 Add serial port with empty name but device is not connected
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));
////
	CHECK_TWO_ERRORS_VALIDATION(PVC_SERIAL_PORT,
								PRL_ERR_VMCONF_SERIAL_PORT_SYS_NAME_IS_EMPTY,
								PRL_ERR_VMCONF_SERIAL_PORT_SYS_NAME_IS_EMPTY);
}

void PrlVmValidateConfigTest::testValidateConfigSerialPortOnInvalidSymbolInSysName()
{
	for(int i = 0; i < wrong_symbols.size() ; ++i)
	{
		QString qsSysName = "serial port.out";
		qsSysName[7] = wrong_symbols.at(i);

		m_VmHandle.reset();
		CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

		SdkHandleWrap hDevice;
// 1 Add serial port with invalid name
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hDevice.GetHandlePtr()));

		CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8(qsSysName)));
		CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_OUTPUT_FILE));
// 2 Add serial port with invalid name but real device
		hDevice.reset();
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hDevice.GetHandlePtr()));

		CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8(qsSysName)));
		CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 3 Add serial port with invalid name but device is not enabled
		hDevice.reset();
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hDevice.GetHandlePtr()));

		CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8(qsSysName)));
		CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_OUTPUT_FILE));
// 4 Add serial port with invalid name but device is not connected
		hDevice.reset();
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hDevice.GetHandlePtr()));

		CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8(qsSysName)));
		CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_OUTPUT_FILE));
////////
		CHECK_ONE_ERROR_VALIDATION(PVC_SERIAL_PORT, PRL_ERR_VMCONF_SERIAL_PORT_SYS_NAME_HAS_INVALID_SYMBOL);
	}
}

void PrlVmValidateConfigTest::testValidateConfigSerialPortOnNotExistOutputFile()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	// Create file
	m_qsFile = QDir::tempPath() + "/port";
	QFile file(m_qsFile);
	QVERIFY(file.open(QIODevice::ReadWrite));

	SdkHandleWrap hDevice;

	//////////////////////////////////////////////////////////
	// 1 Add port with existing sys name as path: PASS
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_OUTPUT_FILE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, m_qsFile.toUtf8().constData()));

	//////////////////////////////////////////////////////////
	// 2 Add port with not existing sys name as path: FAIL
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_OUTPUT_FILE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8( Uuid::createUuid().toString() )  ) );

	//////////////////////////////////////////////////////////
	// 3 Add port with not existing sys name as path but device is not enabled: PASS
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_OUTPUT_FILE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8( Uuid::createUuid().toString() )  ) );

	//////////////////////////////////////////////////////////
	// 4 Add port with not existing sys name as path but device is not connected: PASS
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_OUTPUT_FILE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8( Uuid::createUuid().toString() )  ) );
////
	CHECK_ONE_ERROR_VALIDATION(PVC_SERIAL_PORT, PRL_ERR_VMCONF_SERIAL_PORT_IMAGE_IS_NOT_EXIST);
}

void PrlVmValidateConfigTest::testValidateConfigSerialPortOnUrlSysName()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_OUTPUT_FILE));

	QStringList lstSysNames;
	lstSysNames << SYS_NAMES_IN_URL_FORMAT;
	foreach(QString qsSysName, lstSysNames)
	{
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8(qsSysName)));

		CHECK_ONE_ERROR_VALIDATION(PVC_SERIAL_PORT, PRL_ERR_VMCONF_SERIAL_PORT_URL_FORMAT_SYS_NAME);
	}

// Real device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));

	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
// Disable device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_OUTPUT_FILE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
// Disconnect device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigParallelPortOnValid()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));
// Add parallel port device
	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));

	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_PARALLEL_PORT));
	CHECK_JOB_RET_CODE(hJob);
// Disabled
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_PARALLEL_PORT));
	CHECK_JOB_RET_CODE(hJob);
// Disconnected
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_PARALLEL_PORT));
	CHECK_JOB_RET_CODE(hJob);
// Remote
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, "any path"));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_OUTPUT_FILE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetRemote(hDevice, TRUE));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_SERIAL_PORT));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigParallelPortOnEmptySysName()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	// 1 Add parallel port with valid name
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	// 2 Add parallel port with empty name
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));
	// 3 Add parallel port with empty name but device is not enabled
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));
	// 4 Add parallel port with empty name
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));
	// 5 Add parallel port with empty name but device is not connected
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));
////
	CHECK_TWO_ERRORS_VALIDATION(PVC_PARALLEL_PORT,
								PRL_ERR_VMCONF_PARALLEL_PORT_SYS_NAME_IS_EMPTY,
								PRL_ERR_VMCONF_PARALLEL_PORT_SYS_NAME_IS_EMPTY);
}

void PrlVmValidateConfigTest::testValidateConfigParallelPortOnInvalidSymbolInSysName()
{
	for(int i = 0; i < wrong_symbols.size() ; ++i)
	{
		QString qsSysName = "parallel port.out";
		qsSysName[7] = wrong_symbols.at(i);

		m_VmHandle.reset();
		CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

		SdkHandleWrap hDevice;
// 1 Add parallel port with invalid name
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hDevice.GetHandlePtr()));

		CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8(qsSysName)));
		CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_OUTPUT_FILE));
// 2 Add parallel port with invalid name but real device
		hDevice.reset();
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hDevice.GetHandlePtr()));

		CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8(qsSysName)));
		CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 3 Add parallel port with invalid name but device is not enabled
		hDevice.reset();
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hDevice.GetHandlePtr()));

		CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8(qsSysName)));
		CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_OUTPUT_FILE));
// 4 Add parallel port with invalid name but device is not connected
		hDevice.reset();
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hDevice.GetHandlePtr()));

		CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8(qsSysName)));
		CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_OUTPUT_FILE));
////////
		CHECK_ONE_ERROR_VALIDATION(PVC_PARALLEL_PORT, PRL_ERR_VMCONF_PARALLEL_PORT_SYS_NAME_HAS_INVALID_SYMBOL);
	}
}

void PrlVmValidateConfigTest::testValidateConfigParallelPortOnNotExistOutputFile()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	// Create file
	m_qsFile = QDir::tempPath() + "/port";
	QFile file(m_qsFile);
	QVERIFY(file.open(QIODevice::ReadWrite));

	SdkHandleWrap hDevice;

	//////////////////////////////////////////////////////////
	// 1 Add port with existing sys name as path: PASS
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_OUTPUT_FILE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, m_qsFile.toUtf8().constData()));

	//////////////////////////////////////////////////////////
	// 2 Add port with not existing sys name as path: FAIL
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_OUTPUT_FILE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8( Uuid::createUuid().toString() )  ) );

	//////////////////////////////////////////////////////////
	// 3 Add port with not existing sys name as path but device is not enabled: PASS
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_OUTPUT_FILE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8( Uuid::createUuid().toString() )  ) );

	//////////////////////////////////////////////////////////
	// 4 Add port with not existing sys name as path but device is not connected: PASS
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_OUTPUT_FILE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8( Uuid::createUuid().toString() )  ) );
////
	CHECK_ONE_ERROR_VALIDATION(PVC_PARALLEL_PORT, PRL_ERR_VMCONF_PARALLEL_PORT_IMAGE_IS_NOT_EXIST);
}

void PrlVmValidateConfigTest::testValidateConfigParallelPortOnUrlSysName()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_OUTPUT_FILE));

	QStringList lstSysNames;
	lstSysNames << SYS_NAMES_IN_URL_FORMAT;
	foreach(QString qsSysName, lstSysNames)
	{
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8(qsSysName)));

		CHECK_ONE_ERROR_VALIDATION(PVC_PARALLEL_PORT, PRL_ERR_VMCONF_PARALLEL_PORT_URL_FORMAT_SYS_NAME);
	}

// Real device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));

	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
// Disable device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_OUTPUT_FILE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
// Disconnect device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, FALSE));

	hJob.reset(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigIdeDevicesOnValid()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
// 1 Add IDE hard disk
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 3));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 2 Add IDE optical disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 0));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 3 Add SCSI hard disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 4 Add IDE hard disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 5 Add IDE optical disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 1));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 6 Add SCSI optical disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 1));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 7 Add IDE optical disk but device is not enabled
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 1));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
// 8 Add IDE hard disk but device is not enabled
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 3));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
////
	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_IDE_DEVICES));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigIdeDevicesOnCountOutOfRange()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
// 1 Add IDE hard disk
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 0));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 2 Add IDE optical disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 1));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 3 Add IDE hard disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 4 Add IDE optical disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 3));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 5 Add IDE hard disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 4));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
////
	CHECK_ONE_ERROR_VALIDATION(PVC_IDE_DEVICES, PRL_ERR_VMCONF_IDE_DEVICES_COUNT_OUT_OF_RANGE);
}

void PrlVmValidateConfigTest::testValidateConfigIdeDevicesOnDuplicateStackIndex()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hDevice;
// 1 Add IDE hard disk
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 3));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 2 Add IDE optical disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 3 Add IDE hard disk with duplicated stack index
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 4 Add IDE optical disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 1));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
////
	CHECK_ONE_ERROR_VALIDATION(PVC_IDE_DEVICES, PRL_ERR_VMCONF_IDE_DEVICES_DUPLICATE_STACK_INDEX);
}

void PrlVmValidateConfigTest::testValidateConfigScsiDevicesOnValid()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml")

	CVmConfiguration _vm_conf(_config);
	CVmHardware* pHardwareList = _vm_conf.getVmHardwareList();
	CVmGenericScsiDevice* pScsiDevice = 0;
// 1 Add SCSI generic device
	pScsiDevice = new CVmGenericScsiDevice;
	pScsiDevice->setStackIndex(5);
	pScsiDevice->setEnabled(PVE::DeviceEnabled);
	pHardwareList->m_lstGenericScsiDevices += pScsiDevice;
// 2 Add SCSI generic device
	pScsiDevice = new CVmGenericScsiDevice;
	pScsiDevice->setStackIndex(1);
	pScsiDevice->setEnabled(PVE::DeviceEnabled);
	pHardwareList->m_lstGenericScsiDevices += pScsiDevice;
// 3 Add SCSI generic device
	pScsiDevice = new CVmGenericScsiDevice;
	pScsiDevice->setStackIndex(5);
	pScsiDevice->setEnabled(PVE::DeviceDisabled);
	pHardwareList->m_lstGenericScsiDevices += pScsiDevice;
////
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()));

	SdkHandleWrap hDevice;
// 4 Add SCSI hard disk
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 3));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 5 Add SCSI optical disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 0));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 6 Add IDE hard disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 7 Add SCSI hard disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 8 Add SCSI optical disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 4));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 9 Add IDE optical disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 4));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 10 Add SCSI optical disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 6));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 11 Add SCSI optical disk but device is not enabled
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
// 12 Add SCSI hard disk but device is not enabled
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 0));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, FALSE));
////
	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_SCSI_DEVICES));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigScsiDevicesOnCountOutOfRange()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml");

	CVmConfiguration _vm_conf(_config);
	CVmHardware* pHardwareList = _vm_conf.getVmHardwareList();
	CVmGenericScsiDevice* pScsiDevice = 0;
// 1 Add SCSI generic device
	pScsiDevice = new CVmGenericScsiDevice;
	pScsiDevice->setStackIndex(5);
	pScsiDevice->setEnabled(PVE::DeviceEnabled);
	pHardwareList->m_lstGenericScsiDevices += pScsiDevice;
// 2 Add SCSI generic device
	pScsiDevice = new CVmGenericScsiDevice;
	pScsiDevice->setStackIndex(1);
	pScsiDevice->setEnabled(PVE::DeviceEnabled);
	pHardwareList->m_lstGenericScsiDevices += pScsiDevice;
// 3 Add SCSI generic device
	pScsiDevice = new CVmGenericScsiDevice;
	pScsiDevice->setStackIndex(0);
	pScsiDevice->setEnabled(PVE::DeviceEnabled);
	pHardwareList->m_lstGenericScsiDevices += pScsiDevice;
// 4 Add SCSI generic device
	pScsiDevice = new CVmGenericScsiDevice;
	pScsiDevice->setStackIndex(6);
	pScsiDevice->setEnabled(PVE::DeviceEnabled);
	pHardwareList->m_lstGenericScsiDevices += pScsiDevice;
// 5 Add SCSI generic device
        pScsiDevice = new CVmGenericScsiDevice;
        pScsiDevice->setStackIndex(9);
        pScsiDevice->setEnabled(PVE::DeviceEnabled);
        pHardwareList->m_lstGenericScsiDevices += pScsiDevice;
// 6 Add SCSI generic device
        pScsiDevice = new CVmGenericScsiDevice;
        pScsiDevice->setStackIndex(10);
        pScsiDevice->setEnabled(PVE::DeviceEnabled);
        pHardwareList->m_lstGenericScsiDevices += pScsiDevice;
// 7 Add SCSI generic device
        pScsiDevice = new CVmGenericScsiDevice;
        pScsiDevice->setStackIndex(14);
        pScsiDevice->setEnabled(PVE::DeviceEnabled);
        pHardwareList->m_lstGenericScsiDevices += pScsiDevice;
// 8 Add SCSI generic device
        pScsiDevice = new CVmGenericScsiDevice;
        pScsiDevice->setStackIndex(13);
        pScsiDevice->setEnabled(PVE::DeviceEnabled);
        pHardwareList->m_lstGenericScsiDevices += pScsiDevice;
////
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()));

	SdkHandleWrap hDevice;
// 9 Add SCSI hard disk
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 3));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 10 Add SCSI optical disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 11 Add SCSI hard disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 4));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 12 Add SCSI optical disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
        CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 8));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 13 Add SCSI hard disk
        CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
        CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
        CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 12));
        CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
        CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 14 Add SCSI optical disk
        hDevice.reset();
        CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
        CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
        CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 11));
        CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 15 Add SCSI hard disk
        CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
        CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
        CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 15));
        CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
        CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 16 Add SCSI optical disk
        hDevice.reset();
        CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
        CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
        CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 16));
        CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
////
	CHECK_ONE_ERROR_VALIDATION(PVC_SCSI_DEVICES, PRL_ERR_VMCONF_SCSI_DEVICES_COUNT_OUT_OF_RANGE);
}

void PrlVmValidateConfigTest::testValidateConfigScsiDevicesOnDuplicateStackIndex()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml");

	CVmConfiguration _vm_conf(_config);
	CVmHardware* pHardwareList = _vm_conf.getVmHardwareList();
	CVmGenericScsiDevice* pScsiDevice = 0;
// 1 Add SCSI generic device
	pScsiDevice = new CVmGenericScsiDevice;
	pScsiDevice->setStackIndex(3);
	pScsiDevice->setEnabled(PVE::DeviceEnabled);
	pHardwareList->m_lstGenericScsiDevices += pScsiDevice;
// 2 Add SCSI generic device
	pScsiDevice = new CVmGenericScsiDevice;
	pScsiDevice->setStackIndex(1);
	pScsiDevice->setEnabled(PVE::DeviceEnabled);
	pHardwareList->m_lstGenericScsiDevices += pScsiDevice;
////
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()));

	SdkHandleWrap hDevice;
// 3 Add SCSI hard disk with duplicated stack index
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 3));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 4 Add SCSI optical disk with duplicated stack index
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 3));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
// 5 Add SCSI hard disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 0));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// 6 Add SCSI optical disk
	hDevice.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
////
	CHECK_TWO_ERRORS_VALIDATION(PVC_SCSI_DEVICES,
								PRL_ERR_VMCONF_SCSI_DEVICES_DUPLICATE_STACK_INDEX,
								PRL_ERR_VMCONF_SCSI_DEVICES_DUPLICATE_STACK_INDEX);
}

void PrlVmValidateConfigTest::testValidateConfigOnDuplicateMainDeviceIndex()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml")

		CVmConfiguration _vm_conf(_config);

	_vm_conf.getVmSettings()->getVmCommonOptions()->setOsType( PVS_GUEST_TYPE_WINDOWS );
	_vm_conf.getVmSettings()->getVmCommonOptions()->setOsVersion( PVS_GUEST_VER_WIN_2003 );

	CVmHardware* pHardwareList = _vm_conf.getVmHardwareList();
	CVmOpticalDisk* pDevice = 0;

	// 1 Add some device
	pDevice = new CVmOpticalDisk;
	pDevice->setIndex(5);
	pHardwareList->addOpticalDisk( pDevice );
	// 2 Add some device
	pDevice = new CVmOpticalDisk;
	pDevice->setIndex(5);
	pHardwareList->addOpticalDisk( pDevice );
////
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()));
////
	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_FLOPPY_DISK));
	CHECK_JOB_RET_CODE(hJob);
////
	CHECK_ONE_ERROR_VALIDATION(PVC_CD_DVD_ROM, PRL_ERR_VMCONF_INVALID_DEVICE_MAIN_INDEX);
////
	CHECK_ONE_ERROR_VALIDATION(PVC_ALL, PRL_ERR_VMCONF_INVALID_DEVICE_MAIN_INDEX);
}

void PrlVmValidateConfigTest::testValidateConfigWithRemoteDevicesInDesktopAndServerMode()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));
	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml")

	CVmConfiguration _vm_conf(_config);

	_vm_conf.getVmSettings()->getVmCommonOptions()->setOsType( PVS_GUEST_TYPE_WINDOWS );
	_vm_conf.getVmSettings()->getVmCommonOptions()->setOsVersion( PVS_GUEST_VER_WIN_2003 );

	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()));

	// Add some remote optical disk
	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetRemote(hDevice, true));

	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 0));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE ));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, "XXX-DEVICE--XX"));

	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_ALL));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlVmValidateConfigTest::testValidateConfigOnAllInvalid()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml");

	CVmConfiguration _vm_conf(_config);
	CVmHostSharing* pHostSharing =
		_vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing();
	pHostSharing->setUserDefinedFoldersEnabled(true);

// Network adapter - PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_MAC_ADDRESS
	CVmGenericNetworkAdapter *pNetAdapter = new CVmGenericNetworkAdapter();
	pNetAdapter->setMacAddress("asdasd");
	pNetAdapter->setEmulatedType( (PRL_VM_DEV_EMULATION_TYPE )PNA_BRIDGED_ETHERNET);
	pNetAdapter->setEnabled( TRUE );
	pNetAdapter->setBoundAdapterIndex(0xAAAAAAAA);
	_vm_conf.getVmHardwareList()->addNetworkAdapter(pNetAdapter);

	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()));

	SdkHandleWrap hDevice;
	SdkHandleWrap hBootDevice;
	SdkHandleWrap hShare;
	PRL_UINT32 nIndex = 0;

// General parameters
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, ""));//PRL_ERR_VMCONF_VM_NAME_IS_EMPTY
	CHECK_RET_CODE_EXP(PrlVmCfg_SetOsVersion(m_VmHandle, PVS_GUEST_VER_OTH_OTHER));
// Boot option - PRL_ERR_VMCONF_BOOT_OPTION_INVALID_DEVICE_TYPE
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));

	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hDevice, &nIndex));
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()));
			CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, 1));
			CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, nIndex));
			CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_PARALLEL_PORT));
// Remote display - PRL_ERR_VMCONF_REMOTE_DISPLAY_PORT_NUMBER_IS_ZERO
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCPassword(m_VmHandle, "12345"));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCPort(m_VmHandle, 0));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCHostName(m_VmHandle, "1.2.3.4"));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVNCMode(m_VmHandle, PRD_MANUAL));
// Shared folders - PRL_ERR_VMCONF_SHARED_FOLDERS_INVALID_FOLDER_PATH
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(m_VmHandle, hShare.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlShare_SetName(hShare, "Name 1"));
	CHECK_RET_CODE_EXP(PrlShare_SetPath(hShare, (QDir::currentPath() + "xyz").toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlShare_SetEnabled(hShare, TRUE));
// Cpu - PRL_ERR_VMCONF_CPU_COUNT_MORE_MAX_CPU_COUNT, PRL_ERR_VMCONF_CPU_COUNT_MORE_HOST_CPU_COUNT
	CHECK_RET_CODE_EXP(PrlVmCfg_SetCpuCount(m_VmHandle, 0xFFFF));
// Main memory - PRL_ERR_VMCONF_MAIN_MEMORY_OUT_OF_RANGE
	CHECK_RET_CODE_EXP(PrlVmCfg_SetRamSize(m_VmHandle, VM_MIN_MEM - 1));
// Video memory - PRL_ERR_VMCONF_VIDEO_MEMORY_OUT_OF_RANGE
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVideoRamSize(m_VmHandle, VM_MAX_VIDEO_MEM + 1));
// Floppy disk - PRL_ERR_VMCONF_FLOPPY_DISK_SYS_NAME_IS_EMPTY
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));
// CD/DVD ROM - PRL_ERR_VMCONF_CD_DVD_ROM_SYS_NAME_IS_EMPTY
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 1));
// Hard disk - PRL_ERR_VMCONF_HARD_DISK_SYS_NAME_IS_EMPTY
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));

// Serial port - PRL_ERR_VMCONF_SERIAL_PORT_SYS_NAME_IS_EMPTY
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));
// Parallel port - PRL_ERR_VMCONF_PARALLEL_PORT_SYS_NAME_IS_EMPTY
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, ""));
// Ide devices - PRL_ERR_VMCONF_IDE_DEVICES_DUPLICATE_STACK_INDEX
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));

	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 2));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
// Scsi devices - PRL_ERR_VMCONF_SCSI_DEVICES_DUPLICATE_STACK_INDEX
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 4));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));

	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_SCSI_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 4));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_REAL_DEVICE));
// PCI generic devices - PRL_ERR_VMCONF_GENERIC_PCI_DEVICE_CANNOT_BE_ADDED
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_PCI_DEVICE, hDevice.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 0));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, PRL_TRUE));
////
	SdkHandleWrap hJob(PrlVm_ValidateConfig(m_VmHandle, PVC_ALL));
	CHECK_VMCONF_VALIDATION_FAILED(hJob);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));

#define CHECK_ERRCODE_PRESENTS(nExpectedErrCode)\
	{\
		SdkHandleWrap hProblemDescription;\
		PRL_RESULT nActualErrCode = PRL_ERR_UNINITIALIZED;\
		PRL_UINT32 nParamsCount = 0;\
		CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))\
		bool bFound = false;\
		for (PRL_UINT32 i = 0; i < nParamsCount; ++i)\
		{\
			CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, i, hProblemDescription.GetHandlePtr()))\
			CHECK_HANDLE_TYPE(hProblemDescription, PHT_EVENT)\
			CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hProblemDescription, &nActualErrCode))\
			if ( nActualErrCode == nExpectedErrCode)\
			{\
				bFound = true;\
				break;\
			}\
		}\
		if (!bFound)\
		{\
			WRITE_TRACE(DBG_FATAL, "Error code: %.8X '%s' wasn't found at validation result", nExpectedErrCode, PRL_RESULT_TO_STRING(nExpectedErrCode));\
			QFAIL("Expecting error code wasn't found at validation result");\
		}\
	}

	CHECK_ERRCODE_PRESENTS(PRL_ERR_VMCONF_VM_NAME_IS_EMPTY)
	CHECK_ERRCODE_PRESENTS(PRL_ERR_VMCONF_BOOT_OPTION_INVALID_DEVICE_TYPE)
	CHECK_ERRCODE_PRESENTS(PRL_ERR_VMCONF_REMOTE_DISPLAY_PORT_NUMBER_IS_ZERO)
	CHECK_ERRCODE_PRESENTS(PRL_ERR_VMCONF_SHARED_FOLDERS_INVALID_FOLDER_PATH)
	CHECK_ERRCODE_PRESENTS(PRL_ERR_VMCONF_CPU_COUNT_MORE_MAX_CPU_COUNT)
	CHECK_ERRCODE_PRESENTS(PRL_ERR_VMCONF_CPU_COUNT_MORE_HOST_CPU_COUNT)
	CHECK_ERRCODE_PRESENTS(PRL_ERR_VMCONF_MAIN_MEMORY_OUT_OF_RANGE)
	CHECK_ERRCODE_PRESENTS(PRL_ERR_VMCONF_VIDEO_MEMORY_OUT_OF_RANGE)
	CHECK_ERRCODE_PRESENTS(PRL_ERR_VMCONF_FLOPPY_DISK_SYS_NAME_IS_EMPTY)
	CHECK_ERRCODE_PRESENTS(PRL_ERR_VMCONF_CD_DVD_ROM_SYS_NAME_IS_EMPTY)
	CHECK_ERRCODE_PRESENTS(PRL_ERR_VMCONF_HARD_DISK_SYS_NAME_IS_EMPTY)
	CHECK_ERRCODE_PRESENTS(PRL_ERR_VMCONF_NETWORK_ADAPTER_INVALID_MAC_ADDRESS)
	CHECK_ERRCODE_PRESENTS(PRL_ERR_VMCONF_SERIAL_PORT_SYS_NAME_IS_EMPTY)
	CHECK_ERRCODE_PRESENTS(PRL_ERR_VMCONF_PARALLEL_PORT_SYS_NAME_IS_EMPTY)
	CHECK_ERRCODE_PRESENTS(PRL_ERR_VMCONF_IDE_DEVICES_DUPLICATE_STACK_INDEX)
	CHECK_ERRCODE_PRESENTS(PRL_ERR_VMCONF_SCSI_DEVICES_DUPLICATE_STACK_INDEX)

	GET_VTD_SUPPORTED_FLAG;
	if ( ! bVtdSupported )
	{
		CHECK_ERRCODE_PRESENTS(PRL_ERR_VMCONF_GENERIC_PCI_DEVICE_CANNOT_BE_ADDED)
	}
	else
	{
		CHECK_ERRCODE_PRESENTS(PRL_ERR_VMCONF_GENERIC_PCI_DEVICE_NOT_FOUND)
	}

#undef CHECK_ERRCODE_PRESENTS
}

void PrlVmValidateConfigTest::testWorkWithComplexError()
{
	testCreateVmFromConfig();

	//Get host CPU count
	SdkHandleWrap hJob(PrlSrv_GetSrvConfig(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hSrvConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hSrvConfig.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hSrvConfig, PHT_SERVER_CONFIG)

	PRL_UINT32 nCpusNum = 0;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetCpuCount(hSrvConfig, &nCpusNum))

	hJob.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlVmCfg_SetCpuCount(m_VmHandle, nCpusNum+1))
	hJob.reset(PrlVm_Commit(m_VmHandle));
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT*10))

	CHECK_ASYNC_OP_FAILED(hJob, PRL_ERR_INCONSISTENCY_VM_CONFIG)

	SdkHandleWrap hError;
	CHECK_RET_CODE_EXP(PrlJob_GetError(hJob, hError.GetHandlePtr()))

	SdkHandleWrap hEvtParam;
	CHECK_RET_CODE_EXP(PrlEvent_GetParam(hError, 0, hEvtParam.GetHandlePtr()))

	SdkHandleWrap hComplexError;
	CHECK_RET_CODE_EXP(PrlEvtPrm_ToHandle(hEvtParam, hComplexError.GetHandlePtr()))

	PRL_RESULT nRetCode = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hComplexError, &nRetCode))
	QCOMPARE(quint32(nRetCode), quint32(PRL_ERR_VMCONF_CPU_COUNT_MORE_MAX_CPU_COUNT));

	PRL_EVENT_TYPE nEventType = PET_VM_INF_UNINITIALIZED_EVENT_CODE;
	CHECK_RET_CODE_EXP(PrlEvent_GetType(hComplexError, &nEventType))
	QCOMPARE(quint32(nEventType), quint32(PET_DSP_EVT_ERROR_MESSAGE));
}

