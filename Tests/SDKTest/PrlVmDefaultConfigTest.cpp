/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlVmDefaultConfigTest.cpp
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
#include "PrlVmDefaultConfigTest.h"
#include "Tests/CommonTestsUtils.h"

#include <prlcommon/Interfaces/ParallelsSdkPrivate.h>
#include <prlcommon/Interfaces/ApiDevNums.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>

#include <prlcommon/ProtoSerializer/CProtoCommands.h>

using namespace Parallels;

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

void PrlVmDefaultConfigTest::init()
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

void PrlVmDefaultConfigTest::cleanup()
{
	if (m_VmHandle)
	{
		m_JobHandle.reset(PrlVm_Delete(m_VmHandle, PRL_INVALID_HANDLE));
		PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
		m_VmHandle.reset();
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

static const struct VmDefaultCfgValues
{
        unsigned short os_type ;
        unsigned short os_version ;
        PRL_UINT32 mem_size ;
        PRL_UINT32 hdd_size ;
        PRL_UINT32 video_mem_size ;
} def_cfg_os_param[] = {

    // 0 - means use default value

    // firts is the default value for all
    {0xFFFF, 0xFFFF,                                    256, 8192, 16},

    {PVS_GUEST_TYPE_WINDOWS, 0,                         256, 65536, 0},
    { PVS_GUEST_TYPE_WINDOWS, PVS_GUEST_VER_WIN_311,     64, 2048, 0},
    { PVS_GUEST_TYPE_WINDOWS, PVS_GUEST_VER_WIN_95,     128, 2048, 0},
    { PVS_GUEST_TYPE_WINDOWS, PVS_GUEST_VER_WIN_98,       0, 8192, 0},
    { PVS_GUEST_TYPE_WINDOWS, PVS_GUEST_VER_WIN_ME,       0, 8192, 0},
    { PVS_GUEST_TYPE_WINDOWS, PVS_GUEST_VER_WIN_NT,     256, 4096, 0},

    {PVS_GUEST_TYPE_LINUX, 0,                           512, 65536, 3},
    { PVS_GUEST_TYPE_LINUX, PVS_GUEST_VER_LIN_FEDORA_5,   0, 0, 0},

    {PVS_GUEST_TYPE_FREEBSD, 0,                           0, 65536, 3},
    { PVS_GUEST_TYPE_FREEBSD, PVS_GUEST_VER_BSD_4X,       0, 0, 0},
    { PVS_GUEST_TYPE_FREEBSD, PVS_GUEST_VER_BSD_5X,       0, 0, 0},
    { PVS_GUEST_TYPE_FREEBSD, PVS_GUEST_VER_BSD_6X,       0, 0, 0},
    { PVS_GUEST_TYPE_FREEBSD, PVS_GUEST_VER_BSD_7X,       0, 0, 0},
    { PVS_GUEST_TYPE_FREEBSD, PVS_GUEST_VER_BSD_OTHER,    0, 0, 0},

    {PVS_GUEST_TYPE_OS2, 0,                             128, 2048, 0},
    { PVS_GUEST_TYPE_OS2, PVS_GUEST_VER_OS2_WARP3,        0, 0, 0},
    { PVS_GUEST_TYPE_OS2, PVS_GUEST_VER_OS2_WARP4,        0, 0, 0},
    { PVS_GUEST_TYPE_OS2, PVS_GUEST_VER_OS2_WARP45,     256, 0, 0},
    { PVS_GUEST_TYPE_OS2, PVS_GUEST_VER_OS2_ECS11,      256, 0, 0},
    { PVS_GUEST_TYPE_OS2, PVS_GUEST_VER_OS2_ECS12,      256, 0, 0},
    { PVS_GUEST_TYPE_OS2, PVS_GUEST_VER_OS2_OTHER,        0, 0, 0},

    {PVS_GUEST_TYPE_MSDOS, 0,                            32, 2048, 0},
    { PVS_GUEST_TYPE_MSDOS, PVS_GUEST_VER_DOS_MS622,      0, 0, 0},
    { PVS_GUEST_TYPE_MSDOS, PVS_GUEST_VER_DOS_OTHER,      0, 0, 0},

    {PVS_GUEST_TYPE_NETWARE, 0,                           0, 8192, 0},
    { PVS_GUEST_TYPE_NETWARE, PVS_GUEST_VER_NET_4X,       0, 0, 0},
    { PVS_GUEST_TYPE_NETWARE, PVS_GUEST_VER_NET_5X,       0, 0, 0},
    { PVS_GUEST_TYPE_NETWARE, PVS_GUEST_VER_NET_6X,       0, 0, 0},
    { PVS_GUEST_TYPE_NETWARE, PVS_GUEST_VER_NET_OTHER,    0, 0, 0},

    {PVS_GUEST_TYPE_SOLARIS, 0,                           1024, 65536, 0},
    { PVS_GUEST_TYPE_SOLARIS, PVS_GUEST_VER_SOL_9,        0, 0, 0},
	{ PVS_GUEST_TYPE_SOLARIS, PVS_GUEST_VER_SOL_10,       1536, 0, 0},
	{ PVS_GUEST_TYPE_SOLARIS, PVS_GUEST_VER_SOL_11,       768, 0, 0},
    { PVS_GUEST_TYPE_SOLARIS, PVS_GUEST_VER_SOL_OTHER,    0, 0, 0},

    {PVS_GUEST_TYPE_OTHER, 0,                             0, 0, 0},
    { PVS_GUEST_TYPE_OTHER, PVS_GUEST_VER_OTH_QNX,        0, 0, 0},
    { PVS_GUEST_TYPE_OTHER, PVS_GUEST_VER_OTH_OPENSTEP,   0, 0, 0},
    { PVS_GUEST_TYPE_OTHER, PVS_GUEST_VER_OTH_OTHER,      0, 0, 0},

    {0, 0, 0, 0, 0} // last
} ;

#define GET_DEF_VALUE(v_name) \
    (it->v_name ? it->v_name : (os_def->v_name ? os_def->v_name : base_def->v_name))

void PrlVmDefaultConfigTest::testCheckDefaultConfigurationValues()
{
    const VmDefaultCfgValues *base_def = def_cfg_os_param ;
    Q_ASSERT(base_def->os_type == 0xFFFF) ;
    const VmDefaultCfgValues *os_def = NULL ;
    for(const VmDefaultCfgValues *it=(def_cfg_os_param+1); it->os_type; ++it)
    {
        if (!it->os_version) {
            os_def = it ;
            continue ;
        }

        Q_ASSERT(os_def) ;
        // checking map
        QCOMPARE((PRL_UINT32)it->os_type, (PRL_UINT32)os_def->os_type) ;
        QCOMPARE((PRL_UINT32)it->os_type, (PRL_UINT32)PVS_GET_GUEST_TYPE(it->os_version)) ;

        PRL_UINT32 sdk_value = 0xFFFFFFFF ;
        CHECK_RET_CODE_EXP(PrlVmCfg_GetDefaultMemSize(it->os_version, 0, &sdk_value)) ;
        QCOMPARE(sdk_value, GET_DEF_VALUE(mem_size)) ;

        sdk_value = 0xFFFFFFFF ;
        CHECK_RET_CODE_EXP(PrlVmCfg_GetDefaultHddSize(it->os_version, &sdk_value)) ;
        QCOMPARE(sdk_value, GET_DEF_VALUE(hdd_size)) ;
    }
}
#undef GET_DEF_VALUE

#define SET_DEFAULT_CONFIG(os_version, create_devices)\
	SdkHandleWrap hVm;\
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()));\
	SdkHandleWrap hJob(PrlSrv_GetSrvConfig(m_ServerHandle));\
	CHECK_JOB_RET_CODE(hJob)\
	SdkHandleWrap hResult;\
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));\
	SdkHandleWrap hSrvConfig;\
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hSrvConfig.GetHandlePtr()));\
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(hVm, hSrvConfig, os_version, create_devices))\

#define CHECK_DEVICES_BY_TYPE(device_name1, device_name2)\
	{\
		PRL_UINT32 nOrigDevsCount = 0, nGeneratedConfDevsCount = 0;\
		CHECK_RET_CODE_EXP(PrlSrvCfg_Get##device_name1##sCount(hSrvConfig, &nOrigDevsCount));\
		CHECK_RET_CODE_EXP(PrlVmCfg_Get##device_name2##sCount(hVm, &nGeneratedConfDevsCount));\
		if (nOrigDevsCount >= nGeneratedConfDevsCount && \
				nOrigDevsCount != 0 && nGeneratedConfDevsCount != 0)\
		{\
			SdkHandleWrap hOrigDevice;\
			SdkHandleWrap hGeneratedDevice;\
			CHECK_RET_CODE_EXP(PrlSrvCfg_Get##device_name1(hSrvConfig, 0, hOrigDevice.GetHandlePtr()));\
			CHECK_RET_CODE_EXP(PrlVmCfg_Get##device_name2(hVm, 0, hGeneratedDevice.GetHandlePtr()));\
			QString sOrigName, sOrigId, sGeneratedName, sGeneratedId;\
			PRL_EXTRACT_STRING_VALUE(sOrigName, hOrigDevice, PrlSrvCfgDev_GetName);\
			PRL_EXTRACT_STRING_VALUE(sOrigId, hOrigDevice, PrlSrvCfgDev_GetId);\
			PRL_EXTRACT_STRING_VALUE(sGeneratedName, hGeneratedDevice, PrlVmDev_GetFriendlyName);\
			PRL_EXTRACT_STRING_VALUE(sGeneratedId, hGeneratedDevice, PrlVmDev_GetSysName);\
			QCOMPARE(sOrigName, sGeneratedName);\
			QCOMPARE(sOrigId, sGeneratedId);\
		}\
	}

void PrlVmDefaultConfigTest::testSetDefaultConfigCheckAllDevicesWinVista()
{
	SET_DEFAULT_CONFIG(PVS_GUEST_VER_WIN_VISTA, PRL_TRUE)

	CHECK_DEVICES_BY_TYPE(FloppyDisk, FloppyDisk)
	CHECK_DEVICES_BY_TYPE(SerialPort, SerialPort)
	CHECK_DEVICES_BY_TYPE(Printer, ParallelPort)

	SdkHandleWrap hOrigOpticalDisk, hGeneratedOpticalDisk;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetOpticalDisk(hSrvConfig, 0, hOrigOpticalDisk.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_GetOpticalDisk(hVm, 0, hGeneratedOpticalDisk.GetHandlePtr()));
	QString sOrigOpticalName, sOrigOpticalId, sGeneratedOpticalName, sGeneratedOpticalId;
	PRL_EXTRACT_STRING_VALUE(sOrigOpticalId, hOrigOpticalDisk, PrlSrvCfgDev_GetId);
	PRL_EXTRACT_STRING_VALUE(sGeneratedOpticalName, hGeneratedOpticalDisk, PrlVmDev_GetFriendlyName);
	PRL_EXTRACT_STRING_VALUE(sGeneratedOpticalId, hGeneratedOpticalDisk, PrlVmDev_GetSysName);
	QCOMPARE(sGeneratedOpticalName, QString(PRL_DVD_DEFAULT_DEVICE_NAME));
	QCOMPARE(sGeneratedOpticalId, sOrigOpticalId);
}

#define SET_DEFAULT_CONFIG_AND_GET_SOUND_DEVICE(os_version, create_devices)\
	SET_DEFAULT_CONFIG(os_version, PRL_TRUE)\
	PRL_UINT32 nSoundDevsCount = 0;\
	CHECK_RET_CODE_EXP(PrlVmCfg_GetSoundDevsCount(hVm, &nSoundDevsCount))\
	QCOMPARE(quint32(nSoundDevsCount), quint32(1));\
	SdkHandleWrap hVmDevSound;\
	CHECK_RET_CODE_EXP(PrlVmCfg_GetSoundDev(hVm, 0, hVmDevSound.GetHandlePtr()))\
	CHECK_HANDLE_TYPE(hVmDevSound, PHT_VIRTUAL_DEV_SOUND)\
	PRL_VM_DEV_EMULATION_TYPE nEmulType = PDT_ANY_TYPE;\
	CHECK_RET_CODE_EXP(PrlVmDev_GetEmulatedType(hVmDevSound, &nEmulType))

#define SET_DEFAULT_CONFIG_AND_GET_PARALLEL_PORT_DEVICE(os_version, create_devices)\
	SET_DEFAULT_CONFIG(os_version, PRL_TRUE)\
	PRL_UINT32 nParPortCount = 0;\
	CHECK_RET_CODE_EXP(PrlVmCfg_GetParallelPortsCount(hVm, &nParPortCount))\
	QCOMPARE(quint32(nParPortCount), quint32(1));\
	SdkHandleWrap hVmDevPort;\
	CHECK_RET_CODE_EXP(PrlVmCfg_GetParallelPort(hVm, 0, hVmDevPort.GetHandlePtr()))\
	PRL_VM_DEV_EMULATION_TYPE nEmulType = PDT_ANY_TYPE;\
	CHECK_RET_CODE_EXP(PrlVmDev_GetEmulatedType(hVmDevPort, &nEmulType))

#define SET_DEFAULT_CONFIG_AND_GET_USB_DEVICE(os_version, create_devices)\
	SET_DEFAULT_CONFIG(os_version, PRL_TRUE)\
	PRL_UINT32 nUsbDevCount = 0;\
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUsbDevicesCount(hVm, &nUsbDevCount))\
	QCOMPARE(quint32(nUsbDevCount), quint32(1));\
	SdkHandleWrap hVmDevUsb;\
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUsbDevice(hVm, 0, hVmDevUsb.GetHandlePtr()))

void PrlVmDefaultConfigTest::testSetDefaultConfigForParallelPortDeviceFreeBSD()
{
	SET_DEFAULT_CONFIG_AND_GET_PARALLEL_PORT_DEVICE(PVS_GUEST_VER_BSD_7X, PRL_TRUE)
	QVERIFY(nEmulType == PDT_USE_OUTPUT_FILE);
}

void PrlVmDefaultConfigTest::testSetDefaultConfigForSerailPortDeviceOS2()
{
	SET_DEFAULT_CONFIG(PVS_GUEST_VER_OS2_WARP4, PRL_TRUE)
	PRL_UINT32 nSerPortCount = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetSerialPortsCount(hVm, &nSerPortCount))
	QVERIFY(nSerPortCount == 1 );
}

void PrlVmDefaultConfigTest::testSetDefaultConfigForSerailPortDeviceEcs()
{
	SET_DEFAULT_CONFIG(PVS_GUEST_VER_OS2_ECS12, PRL_TRUE)
	PRL_UINT32 nSerPortCount = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetSerialPortsCount(hVm, &nSerPortCount))
	QVERIFY(nSerPortCount == 1 );
}

void PrlVmDefaultConfigTest::testSetDefaultConfigForParallelPortDeviceOtherLinux()
{
	SET_DEFAULT_CONFIG_AND_GET_PARALLEL_PORT_DEVICE(PVS_GUEST_VER_LIN_OTHER, PRL_TRUE)
	QVERIFY(nEmulType == PDT_USE_OUTPUT_FILE);
}

void PrlVmDefaultConfigTest::testSetDefaultConfigForUsbControllerDeviceOtherLinux()
{
	SET_DEFAULT_CONFIG_AND_GET_USB_DEVICE(PVS_GUEST_VER_LIN_OTHER, PRL_TRUE)
	CHECK_HANDLE_TYPE(hVmDevUsb, PHT_VIRTUAL_DEV_USB_DEVICE)
}

#define ADD_DEFAULT_DEVICE_AND_GET_SOUND_DEVICE(os_version)\
	SET_DEFAULT_CONFIG(os_version, PRL_FALSE)\
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDevice(hVm, hSrvConfig, PDE_SOUND_DEVICE))\
	PRL_UINT32 nSoundDevsCount = 0;\
	CHECK_RET_CODE_EXP(PrlVmCfg_GetSoundDevsCount(hVm, &nSoundDevsCount))\
	QCOMPARE(quint32(nSoundDevsCount), quint32(1));\
	SdkHandleWrap hVmDevSound;\
	CHECK_RET_CODE_EXP(PrlVmCfg_GetSoundDev(hVm, 0, hVmDevSound.GetHandlePtr()))\
	CHECK_HANDLE_TYPE(hVmDevSound, PHT_VIRTUAL_DEV_SOUND)\
	PRL_VM_DEV_EMULATION_TYPE nEmulType = PDT_ANY_TYPE;\
	CHECK_RET_CODE_EXP(PrlVmDev_GetEmulatedType(hVmDevSound, &nEmulType))

void PrlVmDefaultConfigTest::testAddDefaultDeviceForSoundDeviceWinVista()
{
	ADD_DEFAULT_DEVICE_AND_GET_SOUND_DEVICE(PVS_GUEST_VER_WIN_VISTA)
	QVERIFY(nEmulType == PDT_USE_AC97_SOUND);
}

void PrlVmDefaultConfigTest::testAddDefaultDeviceForSoundDeviceWin311()
{
	ADD_DEFAULT_DEVICE_AND_GET_SOUND_DEVICE(PVS_GUEST_VER_WIN_311)
	QVERIFY(nEmulType == PDT_USE_CREATIVE_SB16_SOUND);
}

void PrlVmDefaultConfigTest::testAddDefaultDeviceForSoundDeviceMSDos622()
{
	ADD_DEFAULT_DEVICE_AND_GET_SOUND_DEVICE(PVS_GUEST_VER_DOS_MS622)
	QVERIFY(nEmulType == PDT_USE_CREATIVE_SB16_SOUND);
}

void PrlVmDefaultConfigTest::testAddDefaultDeviceForSoundDeviceMSDosOther()
{
	ADD_DEFAULT_DEVICE_AND_GET_SOUND_DEVICE(PVS_GUEST_VER_DOS_OTHER)
	QVERIFY(nEmulType == PDT_USE_CREATIVE_SB16_SOUND);
}

void PrlVmDefaultConfigTest::testSetDefaultConfigCheckDefaultCpusNumber()
{
	SET_DEFAULT_CONFIG(PVS_GUEST_VER_WIN_VISTA, PRL_TRUE)
	PRL_UINT32 nHostCpusCount = 0;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetCpuCount(hSrvConfig, &nHostCpusCount))
	PRL_UINT32 nVmCpusCount = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetCpuCount(hVm, &nVmCpusCount))
	{
		if (nHostCpusCount < 4)
			QCOMPARE(quint32(nVmCpusCount), quint32(1));
		else
			QCOMPARE(quint32(nVmCpusCount), quint32(2));
	}
}

void PrlVmDefaultConfigTest::testSetDefaultConfigOnInvalidServerConfig1()
{
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(hVm, PRL_INVALID_HANDLE, PVS_GUEST_VER_WIN_2003, PRL_FALSE))
}

void PrlVmDefaultConfigTest::testSetDefaultConfigOnInvalidServerConfig2()
{
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(hVm, PRL_INVALID_HANDLE, PVS_GUEST_VER_WIN_2003, PRL_TRUE))
}

void PrlVmDefaultConfigTest::testSetDefaultConfigCheckShareMacOSXFoldersToWindowsSignSwitchedOn()
{
	SET_DEFAULT_CONFIG(PVS_GUEST_VER_WIN_VISTA, PRL_TRUE)
	PRL_VOID_PTR pBuffer = NULL;
	CHECK_RET_CODE_EXP(PrlVm_ToString(hVm, &pBuffer))
	QString sVmConfig = UTF8_2QSTR((const char *)pBuffer);
	PrlBuffer_Free(pBuffer);
	CVmConfiguration _vm_config(sVmConfig);
	QVERIFY(PRL_SUCCEEDED(_vm_config.m_uiRcInit));
	QVERIFY( ! _vm_config.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->isEnabled() );
	QVERIFY(_vm_config.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->isShareUserHomeDir());
	QVERIFY(!_vm_config.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->isShareAllMacDisks());

	QVERIFY( ! _vm_config.getVmSettings()->getVmTools()->getVmSharing()->getGuestSharing()->isEnabled() );
}

void PrlVmDefaultConfigTest::testSetDefaultConfig_CheckSharedCameraEnabled()
{
	SET_DEFAULT_CONFIG(PVS_GUEST_VER_WIN_XP, PRL_FALSE);
	PRL_VOID_PTR pBuffer = NULL;
	CHECK_RET_CODE_EXP(PrlVm_ToString(hVm, &pBuffer))
	QString sVmConfig = UTF8_2QSTR((const char *)pBuffer);
	PrlBuffer_Free(pBuffer);
	CVmConfiguration _vm_config(sVmConfig);
	QVERIFY(PRL_SUCCEEDED(_vm_config.m_uiRcInit));
	QCOMPARE( true, _vm_config.getVmSettings()->getSharedCamera()->isEnabled() );
}

void PrlVmDefaultConfigTest::testSetDefaultConfig_CheckSharedCameraDisabled()
{
	SET_DEFAULT_CONFIG(PVS_GUEST_VER_WIN_2K, PRL_FALSE);
	PRL_VOID_PTR pBuffer = NULL;
	CHECK_RET_CODE_EXP(PrlVm_ToString(hVm, &pBuffer))
		QString sVmConfig = UTF8_2QSTR((const char *)pBuffer);
	PrlBuffer_Free(pBuffer);
	CVmConfiguration _vm_config(sVmConfig);
	QVERIFY(PRL_SUCCEEDED(_vm_config.m_uiRcInit));
	QCOMPARE( false, _vm_config.getVmSettings()->getSharedCamera()->isEnabled() );
}

#define SET_DEFAULT_CONFIG_WITH_SPECIFIC_HOST_RAM_SIZE(os_version, host_ram_size)\
	SdkHandleWrap hVm;\
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))\
	SdkHandleWrap hJob(PrlSrv_GetSrvConfig(m_ServerHandle));\
	CHECK_JOB_RET_CODE(hJob)\
	SdkHandleWrap hResult;\
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))\
	QString sHostHardwareInfo;\
	PRL_EXTRACT_STRING_VALUE(sHostHardwareInfo, hResult, PrlResult_GetParamAsString)\
	CHostHardwareInfo _hw_info(sHostHardwareInfo);\
	CHECK_RET_CODE_EXP(_hw_info.m_uiRcInit)\
	_hw_info.getMemorySettings()->setHostRamSize(host_ram_size);\
	CProtoCommandDspWsResponse _response_cmd(PVE::DspCmdUserGetHostHwInfo, PRL_ERR_SUCCESS);\
	_response_cmd.SetHostHardwareInfo(_hw_info.toString());\
	CHECK_RET_CODE_EXP(PrlResult_FromString(hResult, _response_cmd.GetCommand()->toString().toUtf8().constData()))\
	SdkHandleWrap hSrvConfig;\
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hSrvConfig.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(hVm, hSrvConfig, os_version, PRL_FALSE))\
	PRL_VOID_PTR pBuffer = 0;\
	CHECK_RET_CODE_EXP(PrlVm_ToString(hVm, &pBuffer))\
	CVmConfiguration _vm_cfg(UTF8_2QSTR((const char *)pBuffer));\
	PrlBuffer_Free(pBuffer);\
	CHECK_RET_CODE_EXP(_vm_cfg.m_uiRcInit)

#define TEST_DEFAULT_RAM_SIZE(os_version, expected_ram_size1, expected_ram_size2, host_ram_size, expected_video_ram_size, expected_video_ram_size2)\
	{\
		SET_DEFAULT_CONFIG_WITH_SPECIFIC_HOST_RAM_SIZE(os_version, host_ram_size)\
		PRL_UINT32 nVmRamSize = 0;\
		CHECK_RET_CODE_EXP(PrlVmCfg_GetRamSize(hVm, &nVmRamSize))\
		QCOMPARE(quint32(nVmRamSize), quint32(expected_ram_size2));\
		PRL_UINT32 nVmVideoRamSize = 0;\
		CHECK_RET_CODE_EXP(PrlVmCfg_GetVideoRamSize(hVm, &nVmVideoRamSize))\
		if ((PHO_MAC == _hw_info.getOsVersion()->getOsType() || PHO_LIN == _hw_info.getOsVersion()->getOsType())\
			&& _vm_cfg.getVmHardwareList()->getVideo()->getEnable3DAcceleration() != P3D_DISABLED )\
			QCOMPARE(quint32(nVmVideoRamSize), quint32(expected_video_ram_size));\
		else\
			QCOMPARE(quint32(nVmVideoRamSize), quint32(expected_video_ram_size2));\
		PRL_UINT32 nDefaultVideoRamSize = 0;\
		CHECK_RET_CODE_EXP(PrlVmCfg_GetDefaultVideoRamSize(os_version, hSrvConfig, PRL_FALSE, &nDefaultVideoRamSize))\
		QCOMPARE(quint32(nDefaultVideoRamSize), quint32(nVmVideoRamSize));\
	}

void PrlVmDefaultConfigTest::testSetDefaultConfigCheckDefaultRamSizeForWinOses()
{
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_2012, 1024, 1024, 3073, 128, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_2012, 1024, 1024, 2049, 128, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_2012, 1024, 1024, 2048, 128, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_WINDOWS8, 1024, 1024, 3073, 256, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_WINDOWS8, 768, 768, 2049, 256, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_WINDOWS8, 512, 512, 2048, 128, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_WINDOWS8_1, 1024, 1024, 3073, 256, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_WINDOWS8_1, 768, 768, 2049, 256, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_WINDOWS8_1, 512, 512, 2048, 128, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_VISTA, 1024, 1024, 3073, 256, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_VISTA, 768, 768, 2049, 256, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_VISTA, 512, 512, 2048, 128, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_2008, 1024, 1024, 3073, 128, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_2008, 768, 768, 2049, 128, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_2008, 512, 512, 2048, 128, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_2003, 1024, 1024, 3073, 64, 64)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_2003, 768, 768, 2049, 64, 64)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_2003, 512, 512, 2048, 64, 64)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_XP, 1024, 1024, 3073, 256, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_XP, 768, 768, 2049, 256, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_XP, 512, 512, 2048, 128, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_2K, 256, 256, 2049, 256, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_2K, 256, 256, 2048, 128, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_2K, 256, 256, 1024, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_OTHER, 1024, 1024, 3073, 128, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_OTHER, 768, 768, 2049, 128, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_WIN_OTHER, 512, 512, 2048, 128, 32)
}

void PrlVmDefaultConfigTest::testSetDefaultConfigCheckDefaultRamSizeForMacOses()
{
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_MACOS_TIGER, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_MACOS_TIGER, 768, 768, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_MACOS_TIGER, 512, 512, 1024, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_MACOS_LEOPARD, 2048, 2048, 4096, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_MACOS_LEOPARD, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_MACOS_LEOPARD, 512, 512, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_MACOS_SNOW_LEOPARD, 2048, 2048, 4096, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_MACOS_SNOW_LEOPARD, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_MACOS_SNOW_LEOPARD, 512, 512, 2048, 32, 32)
}

void PrlVmDefaultConfigTest::testSetDefaultConfigCheckDefaultRamSizeForLinOses()
{
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_FEDORA, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_FEDORA, 1024, 1024, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_FEDORA, 1024, 1024, 1024, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_REDHAT, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_REDHAT, 768, 768, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_REDHAT, 512, 512, 1024, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_REDHAT_7, 2048, 2048, 3073, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_REDHAT_7, 2048, 2048, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_REDHAT_7, 2048, 2048, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_SUSE, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_SUSE, 768, 768, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_SUSE, 512, 512, 1024, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_MANDRAKE, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_MANDRAKE, 768, 768, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_MANDRAKE, 512, 512, 1024, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_KRNL_24, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_KRNL_24, 768, 768, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_KRNL_24, 512, 512, 1024, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_KRNL_26, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_KRNL_26, 768, 768, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_KRNL_26, 512, 512, 1024, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_DEBIAN, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_DEBIAN, 768, 768, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_DEBIAN, 512, 512, 1024, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_XANDROS, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_XANDROS, 768, 768, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_XANDROS, 512, 512, 1024, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_UBUNTU, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_UBUNTU, 768, 768, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_UBUNTU, 512, 512, 1024, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_MAGEIA, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_MAGEIA, 768, 768, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_MAGEIA, 512, 512, 1024, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_MINT, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_MINT, 768, 768, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_MINT, 512, 512, 1024, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_SLES9, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_SLES9, 768, 768, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_SLES9, 512, 512, 1024, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_RHLES3, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_RHLES3, 768, 768, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_RHLES3, 512, 512, 1024, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_CENTOS, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_CENTOS, 768, 768, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_CENTOS, 512, 512, 1024, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_CENTOS_7, 2048, 2048, 3073, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_CENTOS_7, 2048, 2048, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_CENTOS_7, 2048, 2048, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_VZLINUX, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_VZLINUX, 768, 768, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_VZLINUX, 512, 512, 1024, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_VZLINUX_7, 2048, 2048, 3073, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_VZLINUX_7, 2048, 2048, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_VZLINUX_7, 2048, 2048, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_PSBM, 2048, 2048, 3073, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_PSBM, 2048, 2048, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_PSBM, 2048, 2048, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_RH_LEGACY, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_RH_LEGACY, 768, 768, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_RH_LEGACY, 512, 512, 1024, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_OPENSUSE, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_OPENSUSE, 768, 768, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_OPENSUSE, 512, 512, 1024, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_OTHER, 1024, 1024, 2049, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_OTHER, 768, 768, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_LIN_OTHER, 512, 512, 1024, 32, 32)

	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_BSD_7X, 256, 256, 2048, 32, 32)
	TEST_DEFAULT_RAM_SIZE(PVS_GUEST_VER_BSD_7X, 256, 256, 1024, 32, 32)
}

#define TEST_DEFAULT_HDD_SIZE(os_version, expected_hdd_size1, expected_hdd_size2)\
	{\
		SET_DEFAULT_CONFIG(os_version, PRL_TRUE)\
		SdkHandleWrap hHardDisk;\
		CHECK_RET_CODE_EXP(PrlVmCfg_GetHardDisk(hVm, 0, hHardDisk.GetHandlePtr()))\
		CHECK_HANDLE_TYPE(hHardDisk, PHT_VIRTUAL_DEV_HARD_DISK)\
		PRL_UINT32 nHddSize = 0;\
		CHECK_RET_CODE_EXP(PrlVmDevHd_GetDiskSize(hHardDisk, &nHddSize))\
		QCOMPARE(quint32(nHddSize), quint32(expected_hdd_size2));\
	}

void PrlVmDefaultConfigTest::testSetDefaultConfigCheckDefaultHddSizeForWinOses()
{
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_WIN_VISTA, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_WIN_2008, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_WIN_2003, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_WIN_XP, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_WIN_WINDOWS7, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_WIN_WINDOWS8, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_WIN_WINDOWS8_1, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_WIN_2012, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_WIN_OTHER, 65536, 65536)
}

void PrlVmDefaultConfigTest::testSetDefaultConfigCheckDefaultHddSizeForMacOses()
{
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_MACOS_TIGER, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_MACOS_LEOPARD, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_MACOS_SNOW_LEOPARD, 65536, 65536)
}

void PrlVmDefaultConfigTest::testSetDefaultConfigCheckDefaultHddSizeForLinOses()
{
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_LIN_REDHAT, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_LIN_REDHAT_7, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_LIN_SUSE, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_LIN_MANDRAKE, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_LIN_KRNL_24, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_LIN_KRNL_26, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_LIN_DEBIAN, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_LIN_FEDORA, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_LIN_XANDROS, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_LIN_UBUNTU, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_LIN_MAGEIA, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_LIN_MINT, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_LIN_SLES9, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_LIN_RHLES3, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_LIN_CENTOS, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_LIN_CENTOS_7, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_LIN_PSBM, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_LIN_RH_LEGACY, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_LIN_OPENSUSE, 65536, 65536)
	TEST_DEFAULT_HDD_SIZE(PVS_GUEST_VER_LIN_OTHER, 65536, 65536)
}

#define TEST_HOST_SHARING_AND_SHARED_PROFILE_DEFAULTS(os_version, host_sharing_enabled, shared_profile_enabled)\
	{\
		SET_DEFAULT_CONFIG(os_version, PRL_TRUE)\
		PRL_VOID_PTR pBuffer = NULL;\
		CHECK_RET_CODE_EXP(PrlVm_ToString(hVm, &pBuffer))\
		QString sVmConfig = UTF8_2QSTR((const char *)pBuffer);\
		PrlBuffer_Free(pBuffer);\
		CVmConfiguration _vm_config(sVmConfig);\
		QVERIFY(PRL_SUCCEEDED(_vm_config.m_uiRcInit));\
		QVERIFY(_vm_config.getVmSettings()->getVmTools()->getVmSharedProfile()->isEnabled() == shared_profile_enabled);\
		QVERIFY(_vm_config.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->isEnabled() ==\
					host_sharing_enabled);\
		if (host_sharing_enabled)\
		{\
			QVERIFY(!_vm_config.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->isShareAllMacDisks());\
			QVERIFY(_vm_config.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->isShareUserHomeDir());\
		}\
	}

void PrlVmDefaultConfigTest::testSetDefaultConfigCheckSharedProfileEnabled()
{
	{
		TEST_HOST_SHARING_AND_SHARED_PROFILE_DEFAULTS(PVS_GUEST_VER_WIN_VISTA, false, false)
		TEST_HOST_SHARING_AND_SHARED_PROFILE_DEFAULTS(PVS_GUEST_VER_LIN_REDHAT, false, false)
		TEST_HOST_SHARING_AND_SHARED_PROFILE_DEFAULTS(PVS_GUEST_VER_MACOS_LEOPARD, false, false)
	}
	TEST_HOST_SHARING_AND_SHARED_PROFILE_DEFAULTS(PVS_GUEST_VER_BSD_5X, false, false)
	TEST_HOST_SHARING_AND_SHARED_PROFILE_DEFAULTS(PVS_GUEST_VER_OS2_ECS11, false, false)
	TEST_HOST_SHARING_AND_SHARED_PROFILE_DEFAULTS(PVS_GUEST_VER_DOS_MS622, false, false)
	TEST_HOST_SHARING_AND_SHARED_PROFILE_DEFAULTS(PVS_GUEST_VER_SOL_10, false, false)
	TEST_HOST_SHARING_AND_SHARED_PROFILE_DEFAULTS(PVS_GUEST_VER_NET_5X, false, false)
	TEST_HOST_SHARING_AND_SHARED_PROFILE_DEFAULTS(PVS_GUEST_VER_OTH_QNX, false, false)
}

#define CHECK_DEFAULT_PARALLEL_PORT_ON_TYPE(type, force_add_dev)\
{\
	PRL_VOID_PTR pBuffer = NULL;\
	CHECK_RET_CODE_EXP(PrlVm_ToString(m_VmHandle, &pBuffer));\
	CVmConfiguration _vm_config(UTF8_2QSTR((const char *)pBuffer));\
	PrlBuffer_Free(pBuffer);\
	CHECK_RET_CODE_EXP(_vm_config.m_uiRcInit);\
	CVmHardware* pHW = _vm_config.getVmHardwareList();\
	QVERIFY(!pHW->m_lstParallelPorts.isEmpty());\
	CVmParallelPort* pPP = pHW->m_lstParallelPorts[0];\
	QVERIFY(pPP->getEmulatedType() == type);\
}

#define GET_HOST_HRDWARE_INFO\
	m_JobHandle.reset(PrlSrv_GetSrvConfig(m_ServerHandle));\
	CHECK_JOB_RET_CODE(m_JobHandle);\
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()));\
	QString sHostHardwareInfo;\
	PRL_EXTRACT_STRING_VALUE(sHostHardwareInfo, m_ResultHandle, PrlResult_GetParamAsString)\
	CHostHardwareInfo _hw_info(sHostHardwareInfo);\
	CHECK_RET_CODE_EXP(_hw_info.m_uiRcInit)

#define UPDATE_SERVER_CONFIG\
	CProtoCommandDspWsResponse _response_cmd(PVE::DspCmdUserGetHostHwInfo, PRL_ERR_SUCCESS);\
	_response_cmd.SetHostHardwareInfo(_hw_info.toString());\
	CHECK_RET_CODE_EXP(PrlResult_FromString(m_ResultHandle, QSTR2UTF8(_response_cmd.GetCommand()->toString())));\
	SdkHandleWrap hSrvConfig;\
	CHECK_RET_CODE_EXP(PrlResult_GetParam(m_ResultHandle, hSrvConfig.GetHandlePtr()))

void PrlVmDefaultConfigTest::testSetDefaultConfigPrinterOnNoServerConfig()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(m_VmHandle, PRL_INVALID_HANDLE, PVS_GUEST_VER_WIN_XP, PRL_TRUE));

	CHECK_DEFAULT_PARALLEL_PORT_ON_TYPE(PVE::ParallelOutputFile, false);
}

void PrlVmDefaultConfigTest::testSetDefaultConfigPrinterOnNoParallelPortsInSrvCfg()
{
	GET_HOST_HRDWARE_INFO;

	_hw_info.getPrinters()->ClearLists();
	_hw_info.getParallelPorts()->ClearLists();

	UPDATE_SERVER_CONFIG;

	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(m_VmHandle, hSrvConfig, PVS_GUEST_VER_WIN_XP, PRL_TRUE));

	CHECK_DEFAULT_PARALLEL_PORT_ON_TYPE(PVE::ParallelOutputFile, false);
}

void PrlVmDefaultConfigTest::testSetDefaultConfigPrinterOnRealParallelPortsInSrvCfg()
{
	GET_HOST_HRDWARE_INFO;

	_hw_info.getPrinters()->ClearLists();
	_hw_info.getParallelPorts()->ClearLists();

	CHwGenericDevice* pHwPP = new CHwGenericDevice(	PDE_PARALLEL_PORT,
													"Parallel port",
													Uuid::createUuid().toString());
	_hw_info.getParallelPorts()->m_lstParallelPort.append(pHwPP);

	UPDATE_SERVER_CONFIG;

	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(m_VmHandle, hSrvConfig, PVS_GUEST_VER_WIN_XP, PRL_TRUE));

	CHECK_DEFAULT_PARALLEL_PORT_ON_TYPE(PVE::RealParallelPort, false);
}

void PrlVmDefaultConfigTest::testSetDefaultConfigPrinterOnPrintersInSrvCfg()
{
	GET_HOST_HRDWARE_INFO;

	_hw_info.getPrinters()->ClearLists();
	_hw_info.getParallelPorts()->ClearLists();

	CHwPrinter* pHwP = new CHwPrinter;
	pHwP->setDeviceName("Printer");
	pHwP->setDeviceId(Uuid::createUuid().toString());

	_hw_info.getPrinters()->m_lstPrinter.append(pHwP);

	UPDATE_SERVER_CONFIG;

	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(m_VmHandle, hSrvConfig, PVS_GUEST_VER_WIN_XP, PRL_TRUE));

	CHECK_DEFAULT_PARALLEL_PORT_ON_TYPE(PVE::ParallelPrinter, false);
}

void PrlVmDefaultConfigTest::testAddDefaultConfigPrinter()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(m_VmHandle, PRL_INVALID_HANDLE, PVS_GUEST_VER_WIN_XP, PRL_FALSE));

	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDevice(m_VmHandle, PRL_INVALID_HANDLE, PDE_PRINTER));

	CHECK_DEFAULT_PARALLEL_PORT_ON_TYPE(PVE::ParallelOutputFile, true);
}

void PrlVmDefaultConfigTest::testGetDefaultVideoRamSizeOnWrongPtr()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetDefaultVideoRamSize(PVS_GUEST_VER_WIN_XP, PRL_INVALID_HANDLE, PRL_TRUE, 0),\
		PRL_ERR_INVALID_ARG)
}

#define TEST_GET_DEFAULT_VIDEO_MEM_ON_NULL_SRV_CONFIG(os_version, expected_video_ram_size)\
	{\
		PRL_UINT32 nVideoRamSize = 0;\
		CHECK_RET_CODE_EXP(PrlVmCfg_GetDefaultVideoRamSize(os_version, PRL_INVALID_HANDLE, PRL_TRUE, &nVideoRamSize))\
		QCOMPARE(quint32(nVideoRamSize), quint32(expected_video_ram_size));\
	}

void PrlVmDefaultConfigTest::testGetDefaultVideoRamSizeOnNullSrvConfig()
{
	TEST_GET_DEFAULT_VIDEO_MEM_ON_NULL_SRV_CONFIG(PVS_GUEST_VER_WIN_XP, 32)
	TEST_GET_DEFAULT_VIDEO_MEM_ON_NULL_SRV_CONFIG(PVS_GUEST_VER_MACOS_LEOPARD, 32)
	TEST_GET_DEFAULT_VIDEO_MEM_ON_NULL_SRV_CONFIG(PVS_GUEST_VER_LIN_REDHAT, 32)
	TEST_GET_DEFAULT_VIDEO_MEM_ON_NULL_SRV_CONFIG(PVS_GUEST_VER_BSD_5X, 32)
}

void PrlVmDefaultConfigTest::testGetDefaultVideoRamSizeOnNonSrvConfig()
{
	PRL_UINT32 nVideoRamSize = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetDefaultVideoRamSize(PVS_GUEST_VER_WIN_XP, m_ServerHandle, PRL_TRUE, &nVideoRamSize),\
		PRL_ERR_INVALID_ARG)
}

namespace {
template <typename T>
bool compare_lists(const QList<T *> &list1, const QList<T *> &list2)
{
	if (list1.size() != list2.size())
		return (false);

	for(int i = 0; i < list1.size(); ++i)
	{
		if (list1.at(i)->toString() != list2.at(i)->toString())
			return (false);
	}
	return (true);
}

bool CompareHardwareLists(const CVmConfiguration &_vm1, const CVmConfiguration &_vm2)
{
	if (!compare_lists(_vm1.getVmHardwareList()->m_lstFloppyDisks, _vm2.getVmHardwareList()->m_lstFloppyDisks)) return (false);
	if (!compare_lists(_vm1.getVmHardwareList()->m_lstOpticalDisks, _vm2.getVmHardwareList()->m_lstOpticalDisks)) return (false);
	if (!compare_lists(_vm1.getVmHardwareList()->m_lstHardDisks, _vm2.getVmHardwareList()->m_lstHardDisks)) return (false);
	if (!compare_lists(_vm1.getVmHardwareList()->m_lstSerialPorts, _vm2.getVmHardwareList()->m_lstSerialPorts)) return (false);
	if (!compare_lists(_vm1.getVmHardwareList()->m_lstParallelPorts, _vm2.getVmHardwareList()->m_lstParallelPorts)) return (false);
	if (!compare_lists(_vm1.getVmHardwareList()->m_lstNetworkAdapters, _vm2.getVmHardwareList()->m_lstNetworkAdapters)) return (false);
	if (!compare_lists(_vm1.getVmHardwareList()->m_lstSoundDevices, _vm2.getVmHardwareList()->m_lstSoundDevices)) return (false);
	if (!compare_lists(_vm1.getVmHardwareList()->m_lstUsbDevices, _vm2.getVmHardwareList()->m_lstUsbDevices)) return (false);
	if (!compare_lists(_vm1.getVmHardwareList()->m_lstPciVideoAdapters, _vm2.getVmHardwareList()->m_lstPciVideoAdapters)) return (false);
	if (!compare_lists(_vm1.getVmHardwareList()->m_lstGenericDevices, _vm2.getVmHardwareList()->m_lstGenericDevices)) return (false);
	if (!compare_lists(_vm1.getVmHardwareList()->m_lstGenericPciDevices, _vm2.getVmHardwareList()->m_lstGenericPciDevices)) return (false);
	if (!compare_lists(_vm1.getVmHardwareList()->m_lstGenericScsiDevices, _vm2.getVmHardwareList()->m_lstGenericScsiDevices)) return (false);

	return (true);
}

}

void PrlVmDefaultConfigTest::testSetDefaultConfigNotDroppingDevicesIfFalseSpecifiedForCreateDevicesFlag()
{
	//Create default VM config
	SET_DEFAULT_CONFIG(PVS_GUEST_VER_WIN_VISTA, PRL_TRUE)

	//Extract optical disk and change friendly name with own one
	SdkHandleWrap hOpticalDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetOpticalDisk(hVm, 0, hOpticalDisk.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetFriendlyName(hOpticalDisk, QSTR2UTF8(Uuid::createUuid().toString())))

	//Store VM hardware list for further comparision
	PRL_VOID_PTR pBuffer = NULL;
	CHECK_RET_CODE_EXP(PrlVm_ToString(hVm, &pBuffer))
	CVmConfiguration _vm_etalon(UTF8_2QSTR((const char *)pBuffer));
	PrlBuffer_Free(pBuffer);

	//Set default values just for common options
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(hVm, PRL_INVALID_HANDLE, PVS_GUEST_VER_WIN_XP, PRL_FALSE));

	//Check that devices weren't touched
	pBuffer = NULL;
	CHECK_RET_CODE_EXP(PrlVm_ToString(hVm, &pBuffer))
	CVmConfiguration _vm_changed_one(UTF8_2QSTR((const char *)pBuffer));
	PrlBuffer_Free(pBuffer);
	QVERIFY(CompareHardwareLists(_vm_etalon, _vm_changed_one));
}

void PrlVmDefaultConfigTest::testCheckDefaultSATAHardDisk()
{
	QList< QPair<PRL_UINT32, PRL_UINT32> > lstOsVersions
		= QList< QPair<PRL_UINT32, PRL_UINT32> >()
		<< QPair<PRL_UINT32, PRL_UINT32>(PVS_GUEST_VER_MACOS_TIGER, PVS_GUEST_VER_MACOS_LAST)
		<< QPair<PRL_UINT32, PRL_UINT32>(PVS_GUEST_VER_LIN_REDHAT, PVS_GUEST_VER_LIN_MANDRAKE)
		<< QPair<PRL_UINT32, PRL_UINT32>(PVS_GUEST_VER_LIN_KRNL_26, PVS_GUEST_VER_LIN_LAST)
		<< QPair<PRL_UINT32, PRL_UINT32>(PVS_GUEST_VER_WIN_VISTA, PVS_GUEST_VER_WIN_2012);

	QPair<PRL_UINT32, PRL_UINT32> osRange;
	foreach(osRange, lstOsVersions)
	{
		for(PRL_UINT32 nOsVersion = osRange.first; nOsVersion <= osRange.second; nOsVersion++)
		{
			SET_DEFAULT_CONFIG(nOsVersion, PRL_TRUE);
			PRL_UINT32 nCount = 0;
			CHECK_RET_CODE_EXP(PrlVmCfg_GetHardDisksCount(hVm, &nCount));
			if (nCount == 0)
				continue;

			SdkHandleWrap hHdd;
			CHECK_RET_CODE_EXP(PrlVmCfg_GetHardDisk(hVm, 0, hHdd.GetHandlePtr()));

			PRL_MASS_STORAGE_INTERFACE_TYPE nIfaceType;
			CHECK_RET_CODE_EXP(PrlVmDev_GetIfaceType(hHdd, &nIfaceType));
			if ( PMS_SATA_DEVICE != nIfaceType )
				WRITE_TRACE(DBG_FATAL, "nIfaceType=%u for nOsVersion=0x%.4X", nIfaceType, nOsVersion);
			QCOMPARE(quint32(nIfaceType), quint32(PMS_SATA_DEVICE));
		}
	}
}

typedef QPair<PRL_DEVICE_TYPE , PRL_HANDLE_TYPE > pair_dev_type_handle;

void PrlVmDefaultConfigTest::testAddDefaultDeviceWithReturnedDeviceHandle()
{
	SET_DEFAULT_CONFIG(PVS_GUEST_VER_WIN_WINDOWS7, PRL_FALSE);

	QList< pair_dev_type_handle > lstDevTypeHandles
		= QList< pair_dev_type_handle >()
			<< pair_dev_type_handle(PDE_FLOPPY_DISK, PHT_VIRTUAL_DEV_FLOPPY)
			<< pair_dev_type_handle(PDE_OPTICAL_DISK, PHT_VIRTUAL_DEV_OPTICAL_DISK)
			<< pair_dev_type_handle(PDE_HARD_DISK, PHT_VIRTUAL_DEV_HARD_DISK)
			<< pair_dev_type_handle(PDE_GENERIC_NETWORK_ADAPTER, PHT_VIRTUAL_DEV_NET_ADAPTER)
			<< pair_dev_type_handle(PDE_SERIAL_PORT, PHT_VIRTUAL_DEV_SERIAL_PORT)
			<< pair_dev_type_handle(PDE_PARALLEL_PORT, PHT_VIRTUAL_DEV_PARALLEL_PORT)
			<< pair_dev_type_handle(PDE_SOUND_DEVICE, PHT_VIRTUAL_DEV_SOUND)
			<< pair_dev_type_handle(PDE_USB_DEVICE, PHT_VIRTUAL_DEV_USB_DEVICE)
			<< pair_dev_type_handle(PDE_ATTACHED_BACKUP_DISK, PHT_VIRTUAL_DEV_HARD_DISK);

	foreach(pair_dev_type_handle pairDevTypeHandle, lstDevTypeHandles)
	{
		SdkHandleWrap hDevice;
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx(
							hVm, hSrvConfig, pairDevTypeHandle.first, hDevice.GetHandlePtr()));
		CHECK_HANDLE_TYPE(hDevice, pairDevTypeHandle.second);
		//https://jira.sw.ru:9443/browse/PDFM-20077
		#define dev_handle_type pairDevTypeHandle.second
		if ( PHT_VIRTUAL_DEV_OPTICAL_DISK == dev_handle_type || PHT_VIRTUAL_DEV_HARD_DISK == dev_handle_type )
		{
			PRL_MASS_STORAGE_INTERFACE_TYPE nIfaceType = PMS_UNKNOWN_DEVICE;
			CHECK_RET_CODE_EXP(PrlVmDev_GetIfaceType( hDevice, &nIfaceType ))
			QCOMPARE(quint32(PMS_SATA_DEVICE), quint32(nIfaceType));
		}
		#undef dev_handle_type
	}
}

void PrlVmDefaultConfigTest::testAddDefaultDeviceExOnWrongParams()
{
	SET_DEFAULT_CONFIG(PVS_GUEST_VER_WIN_WINDOWS7, PRL_FALSE);

	SdkHandleWrap hDevice;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_AddDefaultDeviceEx(hSrvConfig, hSrvConfig,
																	PDE_HARD_DISK,
																	hDevice.GetHandlePtr()),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_AddDefaultDeviceEx(hSrvConfig, PRL_INVALID_HANDLE,
																	PDE_HARD_DISK,
																	hDevice.GetHandlePtr()),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_AddDefaultDeviceEx(hVm, hVm,
																	PDE_HARD_DISK,
																	hDevice.GetHandlePtr()),
										PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_AddDefaultDeviceEx(hVm, hSrvConfig,
																	PDE_HARD_DISK, NULL),
										PRL_ERR_INVALID_ARG);
}

void PrlVmDefaultConfigTest::testCheckDefaultBootOrder()
{
	QList<int> lstDevOrder;
	lstDevOrder << PDE_FLOPPY_DISK << PDE_HARD_DISK << PDE_OPTICAL_DISK
				<< PDE_USB_DEVICE << PDE_GENERIC_NETWORK_ADAPTER;
	for(PRL_UINT32 nOsVersion = PVS_GUEST_VER_OS2_WARP3; nOsVersion <= PVS_GUEST_VER_OS2_WARP45; nOsVersion++)
	{
		SET_DEFAULT_CONFIG(nOsVersion, PRL_TRUE);
		PRL_UINT32 nCount = 0;
		CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDevCount(hVm, &nCount));
		QCOMPARE( quint32(nCount), quint32(lstDevOrder.size()) );

		for( int i = 0 ; i < lstDevOrder.size() ; i++ )
		{
			SdkHandleWrap hBootDev;
			CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDev(hVm, i, hBootDev.GetHandlePtr()));

			PRL_DEVICE_TYPE nDevType;
			CHECK_RET_CODE_EXP(PrlBootDev_GetType(hBootDev, &nDevType));
			QCOMPARE(quint32(nDevType), quint32(lstDevOrder[i]));
		}
	}
}

void PrlVmDefaultConfigTest::testShareGuestAppsWithHost()
{
	SET_DEFAULT_CONFIG(PVS_GUEST_VER_WIN_VISTA, PRL_TRUE)

	HANDLE_TO_STRING(hVm);

	CVmConfiguration vm_config;
	vm_config.fromString(_str_object);

	QCOMPARE(vm_config.getVmSettings()->getVmTools()->getVmSharedApplications()->isWinToMac()
					, true);
}

void PrlVmDefaultConfigTest::testVirtualPrintersInfo()
{
	int nWinVersion = -1;
	// Win 3.11 <=> Win NT
	for(nWinVersion = PVS_GUEST_VER_WIN_311; nWinVersion <= PVS_GUEST_VER_WIN_NT; nWinVersion++)
	{
		SET_DEFAULT_CONFIG(nWinVersion, PRL_FALSE)
		HANDLE_TO_STRING(hVm);
		CVmConfiguration vm_config;
		vm_config.fromString(_str_object);

		QCOMPARE(vm_config.getVmSettings()->getVirtualPrintersInfo()->isUseHostPrinters(),
					false);
		QCOMPARE(vm_config.getVmSettings()->getVirtualPrintersInfo()->isSyncDefaultPrinter(),
					false);
	}
	// Other Windows
	{
		SET_DEFAULT_CONFIG(PVS_GUEST_VER_WIN_OTHER, PRL_FALSE)
		HANDLE_TO_STRING(hVm);
		CVmConfiguration vm_config;
		vm_config.fromString(_str_object);

		QCOMPARE(vm_config.getVmSettings()->getVirtualPrintersInfo()->isUseHostPrinters(),
					false);
		QCOMPARE(vm_config.getVmSettings()->getVirtualPrintersInfo()->isSyncDefaultPrinter(),
					false);
	}
	// Windows with host printers
	for(nWinVersion = PVS_GUEST_VER_WIN_2K; nWinVersion <= PVS_GUEST_VER_WIN_LAST; nWinVersion++)
	{
		SET_DEFAULT_CONFIG(nWinVersion, PRL_FALSE)
		HANDLE_TO_STRING(hVm);
		CVmConfiguration vm_config;
		vm_config.fromString(_str_object);

		QCOMPARE(vm_config.getVmSettings()->getVirtualPrintersInfo()->isUseHostPrinters(),
					false);
		QCOMPARE(vm_config.getVmSettings()->getVirtualPrintersInfo()->isSyncDefaultPrinter(),
					false);
	}

	// Any other OS (let's switch off or it doesn't matter?)
	{
		SET_DEFAULT_CONFIG(PVS_GUEST_VER_LIN_CENTOS, PRL_FALSE)
		HANDLE_TO_STRING(hVm);
		CVmConfiguration vm_config;
		vm_config.fromString(_str_object);

		QCOMPARE(vm_config.getVmSettings()->getVirtualPrintersInfo()->isUseHostPrinters(),
					false);
		QCOMPARE(vm_config.getVmSettings()->getVirtualPrintersInfo()->isSyncDefaultPrinter(),
					false);
	}
}

void PrlVmDefaultConfigTest::testDefaultStickyMouseValue()
{
	SET_DEFAULT_CONFIG(PVS_GUEST_VER_WIN_WINDOWS8, PRL_FALSE)
	HANDLE_TO_STRING(hVm);

	CVmConfiguration vm_config;
	QCOMPARE(vm_config.getVmSettings()->getVmRuntimeOptions()->isStickyMouse(),
			false );

	vm_config.fromString(_str_object);

	QCOMPARE(vm_config.getVmSettings()->getVmRuntimeOptions()->isStickyMouse(), false);
}

void PrlVmDefaultConfigTest::testScsiCdRomOnSubTypeValueLsiSpi()
{
	for(PRL_UINT32 i = PVS_GUEST_VER_LIN_REDHAT; i <= PVS_GUEST_VER_LIN_OTHER; i++)
	{
		SdkHandleWrap hVm;
		CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()));
		CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(hVm, PRL_INVALID_HANDLE, i, PRL_FALSE));

		for(int j = 0; j < 7; j++)
		{
			SdkHandleWrap hDevice;
			CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx(
								hVm, PRL_INVALID_HANDLE, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

			HANDLE_TO_STRING(hDevice);
			CVmOpticalDisk vmCdRom;
			QVERIFY(PRL_SUCCEEDED(vmCdRom.fromString(_str_object)));

			if (vmCdRom.getInterfaceType() == PMS_SCSI_DEVICE)
				QCOMPARE(vmCdRom.getSubType(), PCD_LSI_SPI);
			else
				QCOMPARE(vmCdRom.getSubType(), PCD_BUSLOGIC);
		}

		if (i == PVS_GUEST_VER_LIN_LAST)
			i = PVS_GUEST_VER_LIN_OTHER;
	}
}

void PrlVmDefaultConfigTest::testScsiHddOnSubTypeValueLsiSpi()
{
	for(PRL_UINT32 i = PVS_GUEST_VER_LIN_REDHAT; i <= PVS_GUEST_VER_LIN_OTHER; i++)
	{
		SdkHandleWrap hVm;
		CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()));
		CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(hVm, PRL_INVALID_HANDLE, i, PRL_FALSE));

		for(int j = 0; j < 7; j++)
		{
			SdkHandleWrap hDevice;
			CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx(
								hVm, PRL_INVALID_HANDLE, PDE_HARD_DISK, hDevice.GetHandlePtr()));

			HANDLE_TO_STRING(hDevice);
			CVmHardDisk vmHdd;
			QVERIFY(PRL_SUCCEEDED(vmHdd.fromString(_str_object)));

			if (vmHdd.getInterfaceType() == PMS_SCSI_DEVICE)
				QCOMPARE(vmHdd.getSubType(), PCD_LSI_SPI);
			else
				QCOMPARE(vmHdd.getSubType(), PCD_BUSLOGIC);
		}

		if (i == PVS_GUEST_VER_LIN_LAST)
			i = PVS_GUEST_VER_LIN_OTHER;
	}
}


void PrlVmDefaultConfigTest::testScsiCdRomOnSubTypeValueLsiSas()
{
	for(PRL_UINT32 i = PVS_GUEST_VER_WIN_311; i <= PVS_GUEST_VER_WIN_OTHER; i++)
	{
		SdkHandleWrap hVm;
		CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()));
		CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(hVm, PRL_INVALID_HANDLE, i, PRL_FALSE));

		for(int j = 0; j < 7; j++)
		{
			SdkHandleWrap hDevice;
			CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx(
								hVm, PRL_INVALID_HANDLE, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

			HANDLE_TO_STRING(hDevice);
			CVmOpticalDisk vmCdRom;
			QVERIFY(PRL_SUCCEEDED(vmCdRom.fromString(_str_object)));

			if (vmCdRom.getInterfaceType() == PMS_SCSI_DEVICE
				&& PVS_GUEST_SCSI_LSI_SAS_SUPPORTED(i))
				QCOMPARE(vmCdRom.getSubType(), PCD_LSI_SAS);
			else
				QCOMPARE(vmCdRom.getSubType(), PCD_BUSLOGIC);
		}

		if (i == PVS_GUEST_VER_WIN_LAST)
			i = PVS_GUEST_VER_WIN_OTHER;
	}
}

void PrlVmDefaultConfigTest::testScsiHddOnSubTypeValueLsiSas()
{
	for(PRL_UINT32 i = PVS_GUEST_VER_WIN_311; i <= PVS_GUEST_VER_WIN_OTHER; i++)
	{
		SdkHandleWrap hVm;
		CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()));
		CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(hVm, PRL_INVALID_HANDLE, i, PRL_FALSE));

		for(int j = 0; j < 7; j++)
		{
			SdkHandleWrap hDevice;
			CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx(
								hVm, PRL_INVALID_HANDLE, PDE_HARD_DISK, hDevice.GetHandlePtr()));

			HANDLE_TO_STRING(hDevice);
			CVmHardDisk vmHdd;
			QVERIFY(PRL_SUCCEEDED(vmHdd.fromString(_str_object)));

			if (vmHdd.getInterfaceType() == PMS_SCSI_DEVICE
				&& PVS_GUEST_SCSI_LSI_SAS_SUPPORTED(i))
				QCOMPARE(vmHdd.getSubType(), PCD_LSI_SAS);
			else
				QCOMPARE(vmHdd.getSubType(), PCD_BUSLOGIC);
		}

		if (i == PVS_GUEST_VER_WIN_LAST)
			i = PVS_GUEST_VER_WIN_OTHER;
	}
}

void PrlVmDefaultConfigTest::testAddDefaultCdRomForMacOsGuest()
{
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(
						hVm, PRL_INVALID_HANDLE, PVS_GUEST_VER_MACOS_UNIVERSAL, PRL_FALSE));

	int i = -1;
	for(i = 0; i < SATA_DEVICES; i++)
	{
		SdkHandleWrap hDevice;
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx(
							hVm, PRL_INVALID_HANDLE, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

		PRL_MASS_STORAGE_INTERFACE_TYPE nIfaceType = PMS_UNKNOWN_DEVICE;
		CHECK_RET_CODE_EXP(PrlVmDev_GetIfaceType( hDevice, &nIfaceType ))
		QCOMPARE(quint32(PMS_SATA_DEVICE), quint32(nIfaceType));
	}

	for(i = 0; i < BUSLOGIC_SCSI_MAXIMUM_TARGETS - 1; i++)
	{
		SdkHandleWrap hDevice;
		CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx(
							hVm, PRL_INVALID_HANDLE, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()));

		PRL_MASS_STORAGE_INTERFACE_TYPE nIfaceType = PMS_UNKNOWN_DEVICE;
		CHECK_RET_CODE_EXP(PrlVmDev_GetIfaceType( hDevice, &nIfaceType ))
		QCOMPARE(quint32(PMS_SCSI_DEVICE), quint32(nIfaceType));
	}

	for(i = 0; i < IDE_DEVICE_LAST; i++)
	{
		SdkHandleWrap hDevice;
		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_AddDefaultDeviceEx(
							hVm, PRL_INVALID_HANDLE, PDE_OPTICAL_DISK, hDevice.GetHandlePtr()),
			PRL_ERR_OPERATION_FAILED);
	}
}

void PrlVmDefaultConfigTest::testAddUsbToBoot()
{
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(
						hVm, PRL_INVALID_HANDLE, PVS_GUEST_VER_WIN_WINDOWS8, PRL_TRUE));

	bool bUsbInBoot = false;
	PRL_UINT32 nDvdBootNum = (PRL_UINT32 )-1;
	PRL_UINT32 nCount;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDevCount(hVm, &nCount));

	for(PRL_UINT32 i = 0; i < nCount; i++)
	{
		SdkHandleWrap hBootDev;
		CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDev(hVm, i, hBootDev.GetHandlePtr()));
		PRL_DEVICE_TYPE nDevType;
		CHECK_RET_CODE_EXP(PrlBootDev_GetType(hBootDev, &nDevType));

		if (nDevType == PDE_OPTICAL_DISK)
		{
			CHECK_RET_CODE_EXP(PrlBootDev_GetSequenceIndex(hBootDev, &nDvdBootNum));
		}

		if (nDevType == PDE_USB_DEVICE)
		{
			bUsbInBoot = true;
			PRL_UINT32 nBootNum;
			CHECK_RET_CODE_EXP(PrlBootDev_GetSequenceIndex(hBootDev, &nBootNum));
			QCOMPARE(nBootNum, nDvdBootNum + 1);
		}
	}

	QVERIFY(bUsbInBoot);
}

void PrlVmDefaultConfigTest::testEnableHiResDrawing()
{
	for(PRL_UINT32 i = PVS_GUEST_VER_WIN_311; i <= PVS_GUEST_VER_WIN_OTHER; i++)
	{
		SdkHandleWrap hVm;
		CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()));
		CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(hVm, PRL_INVALID_HANDLE, i, PRL_FALSE));

		HANDLE_TO_STRING(hVm);
		CVmConfiguration vm_cfg;
		vm_cfg.fromString(_str_object);
		QCOMPARE(i >= PVS_GUEST_VER_WIN_WINDOWS7 && i < PVS_GUEST_VER_WIN_OTHER,
				 vm_cfg.getVmHardwareList()->getVideo()->isEnableHiResDrawing());

		if (i == PVS_GUEST_VER_WIN_LAST)
			i = PVS_GUEST_VER_WIN_OTHER;
	}
}

void PrlVmDefaultConfigTest::testUsbControllerDefaults()
{
	SdkHandleWrap hVm;
	CVmConfiguration vm_cfg;
	QString qsVmCfg;

	// Mac OS
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(
						hVm, PRL_INVALID_HANDLE, PVS_GUEST_VER_MACOS_UNIVERSAL, PRL_FALSE));

	HANDLE_TO_STRING_EX(hVm, qsVmCfg);
	vm_cfg.fromString(qsVmCfg);
	QVERIFY(vm_cfg.getVmSettings()->getUsbController()->isXhcEnabled());

	// Linux
	for(PRL_UINT32 i = PVS_GUEST_VER_LIN_REDHAT; i <= PVS_GUEST_VER_LIN_OTHER; i++)
	{
		CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()));
		CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(hVm, PRL_INVALID_HANDLE, i, PRL_FALSE));

		HANDLE_TO_STRING_EX(hVm, qsVmCfg);
		vm_cfg.fromString(qsVmCfg);
		QVERIFY(vm_cfg.getVmSettings()->getUsbController()->isXhcEnabled());

		if (i == PVS_GUEST_VER_LIN_LAST)
			i = PVS_GUEST_VER_LIN_OTHER;
	}

	// Windows
	for(PRL_UINT32 i = PVS_GUEST_VER_WIN_311; i <= PVS_GUEST_VER_WIN_OTHER; i++)
	{
		CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()));
		CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(hVm, PRL_INVALID_HANDLE, i, PRL_FALSE));

		HANDLE_TO_STRING_EX(hVm, qsVmCfg);
		vm_cfg.fromString(qsVmCfg);
		QCOMPARE(i >= PVS_GUEST_VER_WIN_WINDOWS8 && i < PVS_GUEST_VER_WIN_OTHER,
				 vm_cfg.getVmSettings()->getUsbController()->isXhcEnabled());

		if (i == PVS_GUEST_VER_WIN_LAST)
			i = PVS_GUEST_VER_WIN_OTHER;
	}

	// OS2
	for(PRL_UINT32 i = PVS_GUEST_VER_OS2_WARP3; i <= PVS_GUEST_VER_OS2_OTHER; i++)
	{
		CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()));
		CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(hVm, PRL_INVALID_HANDLE, i, PRL_FALSE));

		HANDLE_TO_STRING_EX(hVm, qsVmCfg);
		vm_cfg.fromString(qsVmCfg);
		QVERIFY( ! vm_cfg.getVmSettings()->getUsbController()->isEhcEnabled() );

		if (i == PVS_GUEST_VER_OS2_LAST)
			i = PVS_GUEST_VER_OS2_OTHER;
	}
}

void PrlVmDefaultConfigTest::testEnableEfiByDefaultForWindows2012()
{
	SET_DEFAULT_CONFIG(PVS_GUEST_VER_WIN_2012, PRL_FALSE)
	HANDLE_TO_STRING(hVm);
	CVmConfiguration vm_config;
	vm_config.fromString(_str_object);

	QVERIFY(vm_config.getVmSettings()->getVmStartupOptions()->getBios()->isEfiEnabled());
}

#define CREATE_BOOTABLE_HDD(hVmDev, bootIndex) \
{ \
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx(hVm, hSrvConfig, \
		PDE_HARD_DISK, hVmDev.GetHandlePtr())) \
	SdkHandleWrap hBootDevice; \
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(hVm, hBootDevice.GetHandlePtr())) \
	CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_HARD_DISK)) \
	PRL_UINT32 index; \
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hVmDev, &index)) \
	CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, index)) \
	CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, (bootIndex))) \
	CHECK_RET_CODE_EXP(PrlBootDev_SetInUse(hBootDevice, PRL_TRUE)) \
}

#define CREATE_ATTACHED_BACKUP(hVmDev, seqno) \
{ \
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx(hVm, hSrvConfig, \
		PDE_ATTACHED_BACKUP_DISK, hVmDev.GetHandlePtr())) \
	QString url = QString("backup://server:12345/{BACKUP}.%1/diskname").arg(seqno); \
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetStorageURL(hVmDev, QSTR2UTF8(url))) \
}

#define FILL_IFACE(iface, limit, backup, seqno) \
{ \
	for (unsigned i = 0; i < (limit); ++i, ++(seqno)) { \
		SdkHandleWrap hVmDev; \
		if (backup) \
			CREATE_ATTACHED_BACKUP(hVmDev, (seqno)) \
		else \
			CREATE_BOOTABLE_HDD(hVmDev, (seqno)) \
		PRL_MASS_STORAGE_INTERFACE_TYPE type; \
		CHECK_RET_CODE_EXP(PrlVmDev_GetIfaceType(hVmDev, &type)) \
		QVERIFY(type == (iface)); \
	} \
}

#define CHECK_DEFAULT_IFACE_TYPE(sata, backup) \
{ \
	/* SATA is not supported: IDE -> SCSI, SATA is supported: SATA -> SCSI -> IDE */ \
	PRL_UINT32 seqno = 0; \
	if (sata) { \
		FILL_IFACE(PMS_SATA_DEVICE, PRL_MAX_SATA_DEVICES_NUM, backup, seqno) \
		FILL_IFACE(PMS_SCSI_DEVICE, PRL_MAX_SCSI_DEVICES_NUM - 1, backup, seqno) \
		FILL_IFACE(PMS_IDE_DEVICE, PRL_MAX_IDE_DEVICES_NUM, backup, seqno) \
	} else { \
		FILL_IFACE(PMS_IDE_DEVICE, PRL_MAX_IDE_DEVICES_NUM, backup, seqno) \
		FILL_IFACE(PMS_SCSI_DEVICE, PRL_MAX_SCSI_DEVICES_NUM - 1, backup, seqno) \
	} \
	SdkHandleWrap hVmDev; \
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(hVm, PDE_HARD_DISK, hVmDev.GetHandlePtr())) \
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetDefaultStackIndex(hVmDev), PRL_ERR_NO_MORE_FREE_INTERFACE_SLOTS) \
}

#define SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(version) \
	SET_DEFAULT_CONFIG((version), PRL_FALSE) \
	PRL_UINT32 count = 0; \
	CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDevCount(hVm, &count)); \
	for( unsigned int i = count ; i > 0 ; --i ) \
	{ \
		SdkHandleWrap hBootDev; \
		CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDev(hVm, i - 1, hBootDev.GetHandlePtr())); \
		CHECK_RET_CODE_EXP(PrlBootDev_Remove(hBootDev)); \
	}

void PrlVmDefaultConfigTest::testCreateDefaultHddForWindowsDistroWithoutSataSupport()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2003)
	CHECK_DEFAULT_IFACE_TYPE(false, false)
}

void PrlVmDefaultConfigTest::testCreateDefaultHddForWindowsDistroWithSataSupport()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)
	CHECK_DEFAULT_IFACE_TYPE(true, false)
}

void PrlVmDefaultConfigTest::testCreateDefaultHddForLinuxDistroWithoutSataSupport()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_LIN_KRNL_24)
	CHECK_DEFAULT_IFACE_TYPE(false, false)
}

void PrlVmDefaultConfigTest::testCreateDefaultHddForLinuxDistroWithSataSupport()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_LIN_CENTOS)
	CHECK_DEFAULT_IFACE_TYPE(true, false)
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithoutSataSupport()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2003)
	CHECK_DEFAULT_IFACE_TYPE(false, true)
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithSataSupport()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)
	CHECK_DEFAULT_IFACE_TYPE(true, true)
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForLinuxDistroWithoutSataSupport()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_LIN_KRNL_24)
	CHECK_DEFAULT_IFACE_TYPE(false, true)
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForLinuxDistroWithSataSupport()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_LIN_CENTOS)
	CHECK_DEFAULT_IFACE_TYPE(true, true)
}

#define CHECK_DEFAULT_IFACE_TYPE_SHIFTED(sata, backup) \
{ \
	/* SATA is not supported: IDE -> SCSI, SATA is supported: SATA -> SCSI -> IDE */ \
	PRL_UINT32 seqno = 3; \
	if (sata) { \
		FILL_IFACE(PMS_SATA_DEVICE, PRL_MAX_SATA_DEVICES_NUM - 1, backup, seqno) \
		FILL_IFACE(PMS_SCSI_DEVICE, PRL_MAX_SCSI_DEVICES_NUM - 2, backup, seqno) \
		FILL_IFACE(PMS_IDE_DEVICE, PRL_MAX_IDE_DEVICES_NUM - 1, backup, seqno) \
	} else { \
		FILL_IFACE(PMS_IDE_DEVICE, PRL_MAX_IDE_DEVICES_NUM - 1, backup, seqno) \
		FILL_IFACE(PMS_SCSI_DEVICE, PRL_MAX_SCSI_DEVICES_NUM - 2, backup, seqno) \
	} \
	SdkHandleWrap hVmDev; \
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(hVm, PDE_HARD_DISK, hVmDev.GetHandlePtr())) \
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetDefaultStackIndex(hVmDev), PRL_ERR_NO_MORE_FREE_INTERFACE_SLOTS) \
}

#define CHECK_DEFAULT_IFACE_TYPE_WITH_EXISTING_BOOTABLE_HDD(sata, backup) \
{ \
	SdkHandleWrap hVmDev1, hVmDev2; \
	CREATE_BOOTABLE_HDD(hVmDev1, 0) \
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev1, PMS_IDE_DEVICE)) \
	CREATE_BOOTABLE_HDD(hVmDev2, 1) \
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev2, PMS_SCSI_DEVICE)) \
	if (sata) { \
		SdkHandleWrap hVmDev; \
		CREATE_BOOTABLE_HDD(hVmDev, 2) \
		CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev, PMS_SATA_DEVICE)) \
	} \
	CHECK_DEFAULT_IFACE_TYPE_SHIFTED(sata, backup) \
}

#define CHECK_DEFAULT_IFACE_TYPE_WITH_EXISTING_ATTACHED_BACKUP(sata, backup) \
{ \
	SdkHandleWrap hVmDev1, hVmDev2; \
	CREATE_ATTACHED_BACKUP(hVmDev1, 0) \
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev1, PMS_IDE_DEVICE)) \
	CREATE_ATTACHED_BACKUP(hVmDev2, 1) \
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev2, PMS_SCSI_DEVICE)) \
	if (sata) { \
		SdkHandleWrap hVmDev; \
		CREATE_ATTACHED_BACKUP(hVmDev, 2) \
		CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev, PMS_SATA_DEVICE)) \
	} \
	CHECK_DEFAULT_IFACE_TYPE_SHIFTED(sata, backup) \
}

void PrlVmDefaultConfigTest::testCreateDefaultHddForWindowsDistroWithoutSataSupportWithExistingHdd()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2003)
	CHECK_DEFAULT_IFACE_TYPE_WITH_EXISTING_BOOTABLE_HDD(false, false)
}

void PrlVmDefaultConfigTest::testCreateDefaultHddForWindowsDistroWithSataSupportWithExistingHdd()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)
	CHECK_DEFAULT_IFACE_TYPE_WITH_EXISTING_BOOTABLE_HDD(true, false)
}

void PrlVmDefaultConfigTest::testCreateDefaultHddForLinuxDistroWithoutSataSupportWithExistingHdd()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_LIN_KRNL_24)
	CHECK_DEFAULT_IFACE_TYPE_WITH_EXISTING_BOOTABLE_HDD(false, false)
}

void PrlVmDefaultConfigTest::testCreateDefaultHddForLinuxDistroWithSataSupportWithExistingHdd()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_LIN_CENTOS)
	CHECK_DEFAULT_IFACE_TYPE_WITH_EXISTING_BOOTABLE_HDD(true, false)
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForLinuxDistroWithoutSataSupportWithExistingHdd()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_LIN_KRNL_24)
	CHECK_DEFAULT_IFACE_TYPE_WITH_EXISTING_BOOTABLE_HDD(false, true)
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForLinuxDistroWithSataSupportWithExistingHdd()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_LIN_CENTOS)
	CHECK_DEFAULT_IFACE_TYPE_WITH_EXISTING_BOOTABLE_HDD(true, true)
}

void PrlVmDefaultConfigTest::testCreateDefaultHddForWindowsDistroWithoutSataSupportWithExistingAttachedBackup()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2003)
	CHECK_DEFAULT_IFACE_TYPE_WITH_EXISTING_ATTACHED_BACKUP(false, false)
}

void PrlVmDefaultConfigTest::testCreateDefaultHddForWindowsDistroWithSataSupportWithExistingAttachedBackup()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)
	CHECK_DEFAULT_IFACE_TYPE_WITH_EXISTING_ATTACHED_BACKUP(true, false)
}

void PrlVmDefaultConfigTest::testCreateDefaultHddForLinuxDistroWithoutSataSupportWithExistingAttachedBackup()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_LIN_KRNL_24)
	CHECK_DEFAULT_IFACE_TYPE_WITH_EXISTING_ATTACHED_BACKUP(false, false)
}

void PrlVmDefaultConfigTest::testCreateDefaultHddForLinuxDistroWithSataSupportWithExistingAttachedBackup()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_LIN_CENTOS)
	CHECK_DEFAULT_IFACE_TYPE_WITH_EXISTING_ATTACHED_BACKUP(true, false)
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithoutSataSupportWithExistingAttachedBackup()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2003)
	CHECK_DEFAULT_IFACE_TYPE_WITH_EXISTING_ATTACHED_BACKUP(false, true)
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingAttachedBackup()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)
	CHECK_DEFAULT_IFACE_TYPE_WITH_EXISTING_ATTACHED_BACKUP(true, true)
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForLinuxDistroWithoutSataSupportWithExistingAttachedBackup()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_LIN_KRNL_24)
	CHECK_DEFAULT_IFACE_TYPE_WITH_EXISTING_ATTACHED_BACKUP(false, true)
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForLinuxDistroWithSataSupportWithExistingAttachedBackup()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_LIN_CENTOS)
	CHECK_DEFAULT_IFACE_TYPE_WITH_EXISTING_ATTACHED_BACKUP(true, true)
}

#define FILL_IFACE_WITH_ATTACHED_BACKUP(iface, start, limit, seqno) \
{ \
	for (unsigned i = (start); i < (start) + (limit); ++i, ++(seqno)) { \
		SdkHandleWrap hVmDev; \
		CREATE_ATTACHED_BACKUP(hVmDev, (seqno)) \
		PRL_MASS_STORAGE_INTERFACE_TYPE type; \
		CHECK_RET_CODE_EXP(PrlVmDev_GetIfaceType(hVmDev, &type)) \
		QVERIFY(type == (iface)); \
		PRL_UINT32 index; \
		CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev, &index)) \
		if (((iface) == PMS_SCSI_DEVICE) && i >= 7) \
			QVERIFY(index == i + 1); /* scsi:7 is reserved for SCSI controller */ \
		else \
			QVERIFY(index == i); \
	} \
}

#define CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_PROLOGUE(iface, index) \
{ \
	SdkHandleWrap hVmDev; \
	CREATE_BOOTABLE_HDD(hVmDev, 0) \
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev, (iface))) \
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hVmDev, (index))) \
}

#define CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE() \
{ \
	SdkHandleWrap hVmDev2; \
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_AddDefaultDeviceEx(hVm, hSrvConfig, \
		PDE_ATTACHED_BACKUP_DISK, hVmDev2.GetHandlePtr()), PRL_ERR_OPERATION_FAILED) \
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithoutSataSupportWithExistingHddOnIde0()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2003)

	PRL_UINT32 seqno = 1;
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_PROLOGUE(PMS_IDE_DEVICE, 0)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_IDE_DEVICE, 1, PRL_MAX_IDE_DEVICES_NUM - 1, seqno)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 0, PRL_MAX_SCSI_DEVICES_NUM - 1, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithoutSataSupportWithExistingHddOnIde2()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2003)

	PRL_UINT32 seqno = 1;
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_PROLOGUE(PMS_IDE_DEVICE, 2)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_IDE_DEVICE, 3, PRL_MAX_IDE_DEVICES_NUM - 3, seqno)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 0, PRL_MAX_SCSI_DEVICES_NUM - 1, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithoutSataSupportWithExistingHddOnIdeMax()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2003)

	PRL_UINT32 seqno = 1;
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_PROLOGUE(PMS_IDE_DEVICE, PRL_MAX_IDE_DEVICES_NUM - 1)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 0, PRL_MAX_SCSI_DEVICES_NUM - 1, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithoutSataSupportWithExistingHddOnScsi0()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2003)

	PRL_UINT32 seqno = 1;
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_PROLOGUE(PMS_SCSI_DEVICE, 0)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 1, PRL_MAX_SCSI_DEVICES_NUM - 2, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithoutSataSupportWithExistingHddOnScsi2()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2003)

	PRL_UINT32 seqno = 1;
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_PROLOGUE(PMS_SCSI_DEVICE, 2)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 3, PRL_MAX_SCSI_DEVICES_NUM - 4, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithoutSataSupportWithExistingHddOnScsiMax()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2003)

	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_PROLOGUE(PMS_SCSI_DEVICE, PRL_MAX_SCSI_DEVICES_NUM - 1)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithoutSataSupportWithExistingHddOnIde2AndScsi2()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2003)

	SdkHandleWrap hVmDev;
	CREATE_BOOTABLE_HDD(hVmDev, 0)
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev, PMS_IDE_DEVICE))
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hVmDev, 2))
	SdkHandleWrap hVmDev2;
	CREATE_BOOTABLE_HDD(hVmDev2, 1)
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev2, PMS_SCSI_DEVICE))
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hVmDev2, 2))

	PRL_UINT32 seqno = 2;
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 3, PRL_MAX_SCSI_DEVICES_NUM - 4, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnSata0()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)

	PRL_UINT32 seqno = 1;
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_PROLOGUE(PMS_SATA_DEVICE, 0)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SATA_DEVICE, 1, PRL_MAX_SATA_DEVICES_NUM - 1, seqno)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 0, PRL_MAX_SCSI_DEVICES_NUM - 1, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnSata2()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)

	PRL_UINT32 seqno = 1;
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_PROLOGUE(PMS_SATA_DEVICE, 2)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SATA_DEVICE, 3, PRL_MAX_SATA_DEVICES_NUM - 3, seqno)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 0, PRL_MAX_SCSI_DEVICES_NUM - 1, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnSataMax()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)

	PRL_UINT32 seqno = 1;
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_PROLOGUE(PMS_SATA_DEVICE, PRL_MAX_SATA_DEVICES_NUM - 1)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 0, PRL_MAX_SCSI_DEVICES_NUM - 1, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnScsi0()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)

	PRL_UINT32 seqno = 1;
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_PROLOGUE(PMS_SCSI_DEVICE, 0)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 1, PRL_MAX_SCSI_DEVICES_NUM - 2, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnScsi2()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)

	PRL_UINT32 seqno = 1;
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_PROLOGUE(PMS_SCSI_DEVICE, 2)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 3, PRL_MAX_SCSI_DEVICES_NUM - 4, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnScsiMax()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)

	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_PROLOGUE(PMS_SCSI_DEVICE, PRL_MAX_SCSI_DEVICES_NUM - 1)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnIde0()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)

	PRL_UINT32 seqno = 1;
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_PROLOGUE(PMS_IDE_DEVICE, 0)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SATA_DEVICE, 0, PRL_MAX_SATA_DEVICES_NUM, seqno)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 0, PRL_MAX_SCSI_DEVICES_NUM - 1, seqno)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_IDE_DEVICE, 1, PRL_MAX_IDE_DEVICES_NUM - 1, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnIde2()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)

	PRL_UINT32 seqno = 1;
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_PROLOGUE(PMS_IDE_DEVICE, 2)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SATA_DEVICE, 0, PRL_MAX_SATA_DEVICES_NUM, seqno)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 0, PRL_MAX_SCSI_DEVICES_NUM - 1, seqno)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_IDE_DEVICE, 3, PRL_MAX_IDE_DEVICES_NUM - 3, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnIdeMax()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)

	PRL_UINT32 seqno = 1;
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_PROLOGUE(PMS_IDE_DEVICE, PRL_MAX_IDE_DEVICES_NUM - 1)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SATA_DEVICE, 0, PRL_MAX_SATA_DEVICES_NUM, seqno)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 0, PRL_MAX_SCSI_DEVICES_NUM - 1, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

#define CREATE_BOOTABLE_HDD_WITH_STACK_INDEX(iface, index) \
{ \
	SdkHandleWrap hVmDev; \
	CREATE_BOOTABLE_HDD(hVmDev, seqno) \
	++seqno; \
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev, (iface))) \
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hVmDev, (index))) \
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnIde2AndScsi2()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)

	PRL_UINT32 seqno = 0;
	CREATE_BOOTABLE_HDD_WITH_STACK_INDEX(PMS_IDE_DEVICE, 2)
	CREATE_BOOTABLE_HDD_WITH_STACK_INDEX(PMS_SCSI_DEVICE, 2)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 3, PRL_MAX_SCSI_DEVICES_NUM - 4, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnIde2AndSata2()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)

	PRL_UINT32 seqno = 0;
	CREATE_BOOTABLE_HDD_WITH_STACK_INDEX(PMS_IDE_DEVICE, 2)
	CREATE_BOOTABLE_HDD_WITH_STACK_INDEX(PMS_SATA_DEVICE, 2)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SATA_DEVICE, 3, PRL_MAX_SATA_DEVICES_NUM - 3, seqno)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 0, PRL_MAX_SCSI_DEVICES_NUM - 1, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnSata2AndScsi2()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)

	PRL_UINT32 seqno = 0;
	CREATE_BOOTABLE_HDD_WITH_STACK_INDEX(PMS_SCSI_DEVICE, 2)
	CREATE_BOOTABLE_HDD_WITH_STACK_INDEX(PMS_SATA_DEVICE, 2)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 3, PRL_MAX_SCSI_DEVICES_NUM - 4, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnIde2AndSata2AndScsi2()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)

	PRL_UINT32 seqno = 0;
	CREATE_BOOTABLE_HDD_WITH_STACK_INDEX(PMS_SCSI_DEVICE, 2)
	CREATE_BOOTABLE_HDD_WITH_STACK_INDEX(PMS_SATA_DEVICE, 2)
	CREATE_BOOTABLE_HDD_WITH_STACK_INDEX(PMS_IDE_DEVICE, 2)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 3, PRL_MAX_SCSI_DEVICES_NUM - 4, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

#define CREATE_DISABLED_BOOTABLE_HDD_WITH_STACK_INDEX(iface, index) \
{ \
	SdkHandleWrap hVmDev; \
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx(hVm, hSrvConfig, \
		PDE_HARD_DISK, hVmDev.GetHandlePtr())) \
	SdkHandleWrap hBootDevice; \
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(hVm, hBootDevice.GetHandlePtr())) \
	CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_HARD_DISK)) \
	PRL_UINT32 idx; \
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hVmDev, &idx)) \
	CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, idx)) \
	CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, (index))) \
	CHECK_RET_CODE_EXP(PrlBootDev_SetInUse(hBootDevice, PRL_FALSE)) \
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev, (iface))) \
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hVmDev, (index))) \
	++seqno; \
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnDisabledScsi2()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)

	PRL_UINT32 seqno = 0;
	CREATE_DISABLED_BOOTABLE_HDD_WITH_STACK_INDEX(PMS_SCSI_DEVICE, 2)
	CREATE_BOOTABLE_HDD_WITH_STACK_INDEX(PMS_SATA_DEVICE, 2)
	CREATE_BOOTABLE_HDD_WITH_STACK_INDEX(PMS_IDE_DEVICE, 2)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SATA_DEVICE, 3, PRL_MAX_SATA_DEVICES_NUM - 3, seqno)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 0, 2, seqno)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 3, PRL_MAX_SCSI_DEVICES_NUM - 4, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnDisabledScsi2AndSata2()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)

	PRL_UINT32 seqno = 0;
	CREATE_DISABLED_BOOTABLE_HDD_WITH_STACK_INDEX(PMS_SCSI_DEVICE, 2)
	CREATE_DISABLED_BOOTABLE_HDD_WITH_STACK_INDEX(PMS_SATA_DEVICE, 2)
	CREATE_BOOTABLE_HDD_WITH_STACK_INDEX(PMS_IDE_DEVICE, 2)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SATA_DEVICE, 0, 2, seqno)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SATA_DEVICE, 3, PRL_MAX_SATA_DEVICES_NUM - 3, seqno)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 0, 2, seqno)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 3, PRL_MAX_SCSI_DEVICES_NUM - 4, seqno)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_IDE_DEVICE, 3, PRL_MAX_IDE_DEVICES_NUM - 3, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}

void PrlVmDefaultConfigTest::testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnDisabledSata2()
{
	SET_DEFAULT_CONFIG_WITHOUT_BOOT_DEVICES(PVS_GUEST_VER_WIN_2008)

	PRL_UINT32 seqno = 0;
	CREATE_BOOTABLE_HDD_WITH_STACK_INDEX(PMS_SCSI_DEVICE, 2)
	CREATE_DISABLED_BOOTABLE_HDD_WITH_STACK_INDEX(PMS_SATA_DEVICE, 2)
	CREATE_BOOTABLE_HDD_WITH_STACK_INDEX(PMS_IDE_DEVICE, 2)
	FILL_IFACE_WITH_ATTACHED_BACKUP(PMS_SCSI_DEVICE, 3, PRL_MAX_SCSI_DEVICES_NUM - 4, seqno)
	CHECK_DEFAULT_IFACE_TYPE_AND_STACK_INDEX_EPILOGUE()
}
