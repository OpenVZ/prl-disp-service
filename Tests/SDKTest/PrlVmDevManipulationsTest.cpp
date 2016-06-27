/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlVmDevManipulationsTest.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing VM devices manipulating SDK API.
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
///Cfg////////////////////////////////////////////////////////////////////////

#include "PrlVmDevManipulationsTest.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlxmlmodel/VmConfig/CVmSoundInputs.h>
#include <prlxmlmodel/VmConfig/CVmSoundOutputs.h>
#include <prlxmlmodel/VmConfig/CVmHardDisk.h>
#include <prlcommon/Logging/Logging.h>
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include <prlcommon/Interfaces/ApiDevNums.h>

#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QHostAddress>

#include "Tests/CommonTestsUtils.h"
#include "Tests/SDKTest/AutoHelpers.h"

#define READ_VM_CONFIG_INTO_BUF(sFileName)\
	QFile _file(sFileName);\
	QVERIFY(_file.open(QIODevice::ReadOnly));\
	QTextStream _stream(&_file);\
	QString _config = _stream.readAll();

#define INITIALIZE_VM_FROM_FILE(sFileName)\
	READ_VM_CONFIG_INTO_BUF(sFileName)\
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _config.toUtf8().data()))\
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, QTest::currentTestFunction()))

#define CREATE_TEST_VM_CONFIGURATION\
	CVmConfiguration _vm_config(_config);\
	QVERIFY(PRL_SUCCEEDED(_vm_config.m_uiRcInit));\
	CVmHardware *pHardware = _vm_config.getVmHardwareList();\
	QVERIFY(pHardware);

#define PRL_CHECK_NET_ADDRESS_LIST(handle, extract_list_method, expected_list_values)\
	{\
		SdkHandleWrap hStringsList;\
		CHECK_RET_CODE_EXP(extract_list_method(handle, hStringsList.GetHandlePtr()))\
		CHECK_HANDLE_TYPE(hStringsList, PHT_STRINGS_LIST)\
		PRL_UINT32 nItemsCount = 0;\
		CHECK_RET_CODE_EXP(PrlStrList_GetItemsCount(hStringsList, &nItemsCount))\
		QCOMPARE(quint32(nItemsCount), quint32(expected_list_values.size()));\
		for (PRL_UINT32 i = 0; i < nItemsCount; ++i)\
		{\
			QString sItem;\
			PRL_EXTRACT_STRING_VALUE_BY_INDEX(sItem, hStringsList, i, PrlStrList_GetItem)\
			QCOMPARE(QHostAddress(sItem), QHostAddress(expected_list_values.at(i)));\
		}\
	}

namespace
{
int GetExpectedDevicesCount(CVmHardware *pHardware)
{
	return (pHardware->m_lstUsbDevices.count() + pHardware->m_lstSoundDevices.count() +
					pHardware->m_lstNetworkAdapters.count() + pHardware->m_lstParallelPorts.count() +
					pHardware->m_lstSerialPorts.count() + pHardware->m_lstHardDisks.count() +
					pHardware->m_lstOpticalDisks.count() + pHardware->m_lstFloppyDisks.count() +
					pHardware->m_lstGenericScsiDevices.count() + pHardware->m_lstGenericPciDevices.count());
}

}

void PrlVmDevManipulationsTest::clearTestPaths()
{
	QStringList lstTestPaths = QStringList()
		<< m_qsTestPath << m_qsTestPath2;

	foreach(QString qsTestPath, lstTestPaths)
		if ( ! qsTestPath.isEmpty() && QFile::exists( qsTestPath ) )
			CFileHelper::ClearAndDeleteDir( qsTestPath );

	m_qsTestPath.clear();
	m_qsTestPath2.clear();
}

void PrlVmDevManipulationsTest::init()
{
	CHECK_RET_CODE_EXP(PrlSrv_Create(m_ServerHandle.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))
	m_pDevicesBuf = NULL;
	SdkHandleWrap hJob;
	hJob.reset(PrlSrv_Login(m_ServerHandle, TestConfig::getRemoteHostName(),
					TestConfig::getUserLogin(),	TestConfig::getUserPassword(), NULL, 0, 0, PSL_HIGH_SECURITY));
	CHECK_JOB_RET_CODE(hJob)

	clearTestPaths();
}

void PrlVmDevManipulationsTest::cleanup()
{
	if (m_VmHandle)
		DELETE_VM(m_VmHandle)

	SdkHandleWrap hJob(PrlSrv_Logoff(m_ServerHandle));
	PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT);
	m_VmDevHandles.clear();
	m_VmHandle.reset();
	m_ServerHandle.reset();
	free(m_pDevicesBuf);

	clearTestPaths();
}

bool PrlVmDevManipulationsTest::CheckDevicesCount(int nExpectedCount, PRL_HANDLE_TYPE _dev_type, PRL_UINT32 nAllCount)
{
	int nActualCount = 0;
	for (PRL_UINT32 i = 0; i < nAllCount; i++)
	{
		PRL_HANDLE hDev = m_pDevicesBuf[i];
		PRL_HANDLE_TYPE _type = PHT_ERROR;
		PrlHandle_GetType(hDev, &_type);
		if (_type == _dev_type)
			nActualCount++;
	}
	return (nActualCount == nExpectedCount);
}

void PrlVmDevManipulationsTest::testGetDeviceList()
{
	INITIALIZE_VM_FROM_FILE("./CVmConfigurationTest_valid_vm_config.xml")
	CREATE_TEST_VM_CONFIGURATION
	PRL_UINT32 nDevCount = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDevsCount(m_VmHandle, &nDevCount))
	QCOMPARE(quint32(nDevCount), quint32(GetExpectedDevicesCount(pHardware)));
	m_pDevicesBuf = (PRL_HANDLE_PTR)malloc(sizeof(PRL_HANDLE)*nDevCount);
	QVERIFY(m_pDevicesBuf);
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDevsList(m_VmHandle, m_pDevicesBuf, &nDevCount))
	for (PRL_UINT32 i = 0; i < nDevCount; i++)
		m_VmDevHandles.append(SdkHandleWrap(m_pDevicesBuf[i]));
	QCOMPARE(quint32(nDevCount), quint32(GetExpectedDevicesCount(pHardware)));
	QVERIFY(CheckDevicesCount(pHardware->m_lstFloppyDisks.count(), PHT_VIRTUAL_DEV_FLOPPY, nDevCount));
	QVERIFY(CheckDevicesCount(pHardware->m_lstOpticalDisks.count(), PHT_VIRTUAL_DEV_OPTICAL_DISK, nDevCount));
	QVERIFY(CheckDevicesCount(pHardware->m_lstHardDisks.count(), PHT_VIRTUAL_DEV_HARD_DISK, nDevCount));
	QVERIFY(CheckDevicesCount(pHardware->m_lstSerialPorts.count(), PHT_VIRTUAL_DEV_SERIAL_PORT, nDevCount));
	QVERIFY(CheckDevicesCount(pHardware->m_lstParallelPorts.count(), PHT_VIRTUAL_DEV_PARALLEL_PORT, nDevCount));
	QVERIFY(CheckDevicesCount(pHardware->m_lstNetworkAdapters.count(), PHT_VIRTUAL_DEV_NET_ADAPTER, nDevCount));
	QVERIFY(CheckDevicesCount(pHardware->m_lstSoundDevices.count(), PHT_VIRTUAL_DEV_SOUND, nDevCount));
	QVERIFY(CheckDevicesCount(pHardware->m_lstUsbDevices.count(), PHT_VIRTUAL_DEV_USB_DEVICE, nDevCount));
	QVERIFY(CheckDevicesCount(pHardware->m_lstGenericPciDevices.count(), PHT_VIRTUAL_DEV_GENERIC_PCI, nDevCount));
	QVERIFY(CheckDevicesCount(pHardware->m_lstGenericScsiDevices.count(), PHT_VIRTUAL_DEV_GENERIC_SCSI, nDevCount));
}

void PrlVmDevManipulationsTest::testGetDeviceCountInvalidVmHandle()
{
	PRL_UINT32 nDevCount = 0;
	QVERIFY(!PRL_SUCCEEDED(PrlVmCfg_GetDevsCount(PRL_INVALID_HANDLE, &nDevCount)));
}

void PrlVmDevManipulationsTest::testGetDeviceCountNonVmHandle()
{
	PRL_UINT32 nDevCount = 0;
	QVERIFY(!PRL_SUCCEEDED(PrlVmCfg_GetDevsCount(m_ServerHandle, &nDevCount)));
}

void PrlVmDevManipulationsTest::testGetDeviceCountInvalidPtr()
{
	QVERIFY(!PRL_SUCCEEDED(PrlVmCfg_GetDevsCount(m_VmHandle, 0)));
}

void PrlVmDevManipulationsTest::testGetDeviceListInvalidVmHandle()
{
	INITIALIZE_VM_FROM_FILE("./CVmConfigurationTest_valid_vm_config.xml")
	CREATE_TEST_VM_CONFIGURATION
	PRL_UINT32 nDevCount = GetExpectedDevicesCount(pHardware);
	m_pDevicesBuf = (PRL_HANDLE_PTR)malloc(sizeof(PRL_HANDLE)*nDevCount);
	QVERIFY(!PRL_SUCCEEDED(PrlVmCfg_GetDevsList(PRL_INVALID_HANDLE, m_pDevicesBuf, &nDevCount)));
}

void PrlVmDevManipulationsTest::testGetDeviceListNonVmHandle()
{
	INITIALIZE_VM_FROM_FILE("./CVmConfigurationTest_valid_vm_config.xml")
	CREATE_TEST_VM_CONFIGURATION
	PRL_UINT32 nDevCount = GetExpectedDevicesCount(pHardware);
	m_pDevicesBuf = (PRL_HANDLE_PTR)malloc(sizeof(PRL_HANDLE)*nDevCount);
	QVERIFY(!PRL_SUCCEEDED(PrlVmCfg_GetDevsList(m_ServerHandle, m_pDevicesBuf, &nDevCount)));
}

void PrlVmDevManipulationsTest::testGetDeviceListInvalidPtr()
{
	INITIALIZE_VM_FROM_FILE("./CVmConfigurationTest_valid_vm_config.xml")
	CREATE_TEST_VM_CONFIGURATION
	PRL_UINT32 nDevCount = GetExpectedDevicesCount(pHardware);
	QVERIFY(!PRL_SUCCEEDED(PrlVmCfg_GetDevsList(m_VmHandle, 0, &nDevCount)));
}

void PrlVmDevManipulationsTest::testGetDeviceListInvalidPtr2()
{
	INITIALIZE_VM_FROM_FILE("./CVmConfigurationTest_valid_vm_config.xml")
	CREATE_TEST_VM_CONFIGURATION
	m_pDevicesBuf = (PRL_HANDLE_PTR)malloc(sizeof(PRL_HANDLE)*GetExpectedDevicesCount(pHardware));
	QVERIFY(!PRL_SUCCEEDED(PrlVmCfg_GetDevsList(m_VmHandle, m_pDevicesBuf, 0)));
}

void PrlVmDevManipulationsTest::testGetDeviceOnNonEnoughBuffer()
{
	INITIALIZE_VM_FROM_FILE("./CVmConfigurationTest_valid_vm_config.xml")
	CREATE_TEST_VM_CONFIGURATION
	if (GetExpectedDevicesCount(pHardware) < 2)
	{
		QSKIP("No valid tests input data. This test requires 2 and more virtual devices in VM configuration", SkipAll);
	}
	PRL_UINT32 nDevCount = GetExpectedDevicesCount(pHardware)/2;
	m_pDevicesBuf = (PRL_HANDLE_PTR)malloc(sizeof(PRL_HANDLE)*nDevCount);
	QVERIFY(PrlVmCfg_GetDevsList(m_VmHandle, m_pDevicesBuf, &nDevCount) == PRL_ERR_BUFFER_OVERRUN);
	for (PRL_UINT32 i = 0; i < nDevCount; i++)
		m_VmDevHandles.append(SdkHandleWrap(m_pDevicesBuf[i]));
	QCOMPARE(quint32(nDevCount), quint32(GetExpectedDevicesCount(pHardware)/2));
}

void PrlVmDevManipulationsTest::testGetDeviceOnLargerBuffer()
{
	INITIALIZE_VM_FROM_FILE("./CVmConfigurationTest_valid_vm_config.xml")
	CREATE_TEST_VM_CONFIGURATION
	if (!GetExpectedDevicesCount(pHardware))
	{
		QSKIP("No valid tests input data. This test requires at least one or more virtual devices in VM configuration", SkipAll);
	}
	PRL_UINT32 nDevCount = GetExpectedDevicesCount(pHardware)*2;
	m_pDevicesBuf = (PRL_HANDLE_PTR)malloc(sizeof(PRL_HANDLE)*nDevCount);
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDevsList(m_VmHandle, m_pDevicesBuf, &nDevCount))
	for (PRL_UINT32 i = 0; i < nDevCount; i++)
		m_VmDevHandles.append(SdkHandleWrap(m_pDevicesBuf[i]));
	QCOMPARE(quint32(nDevCount), quint32(GetExpectedDevicesCount(pHardware)));
}

static const PRL_DEVICE_TYPE _devices_types[] = {
					PDE_FLOPPY_DISK, PDE_HARD_DISK, PDE_GENERIC_NETWORK_ADAPTER,
					PDE_PARALLEL_PORT, PDE_SERIAL_PORT, PDE_OPTICAL_DISK,
					PDE_USB_DEVICE, PDE_SOUND_DEVICE
};

void PrlVmDevManipulationsTest::testCreateVmDev()
{
	for (size_t i = 0; i < sizeof(_devices_types)/sizeof(_devices_types[0]); i++)
	{
		PRL_HANDLE hVmDev = PRL_INVALID_HANDLE;
		PRL_RESULT _rc = PrlVmCfg_CreateVmDev(m_VmHandle, _devices_types[i], &hVmDev);
		m_VmDevHandles.append(SdkHandleWrap(hVmDev));
		CHECK_RET_CODE_EXP(_rc)
	}
}

void PrlVmDevManipulationsTest::testCreateVmDevOnInvalidVmHandle()
{
	PRL_HANDLE hVmDev = PRL_INVALID_HANDLE;
	QVERIFY(!PRL_SUCCEEDED(PrlVmCfg_CreateVmDev(PRL_INVALID_HANDLE, PDE_SOUND_DEVICE, &hVmDev)));
}

void PrlVmDevManipulationsTest::testCreateVmDevOnNonVmHandle()
{
	PRL_HANDLE hVmDev = PRL_INVALID_HANDLE;
	QVERIFY(!PRL_SUCCEEDED(PrlVmCfg_CreateVmDev(m_ServerHandle, PDE_SOUND_DEVICE, &hVmDev)));
}

void PrlVmDevManipulationsTest::testCreateVmDevOnInvalidVmDevType()
{
	PRL_HANDLE hVmDev = PRL_INVALID_HANDLE;
	QVERIFY(!PRL_SUCCEEDED(PrlVmCfg_CreateVmDev(m_VmHandle, (PRL_DEVICE_TYPE )PHT_SERVER, &hVmDev)));
}

void PrlVmDevManipulationsTest::testCreateVmDevOnInvalidPtr()
{
	QVERIFY(!PRL_SUCCEEDED(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SOUND_DEVICE, 0)));
}

void PrlVmDevManipulationsTest::testCreateVmDevNotBoundToVm()
{
	for (size_t i = 0; i < sizeof(_devices_types)/sizeof(_devices_types[0]); i++)
	{
		PRL_HANDLE hVmDev = PRL_INVALID_HANDLE;
		PRL_RESULT _rc = PrlVmDev_Create(_devices_types[i], &hVmDev);
		m_VmDevHandles.append(SdkHandleWrap(hVmDev));
		CHECK_RET_CODE_EXP(_rc)
	}
}

void PrlVmDevManipulationsTest::testCreateVmDevNotBoundToVmOnInvalidVmDevType()
{
	PRL_HANDLE hVmDev = PRL_INVALID_HANDLE;
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_Create((PRL_DEVICE_TYPE )PHT_SERVER, &hVmDev)));
}

void PrlVmDevManipulationsTest::testCreateVmDevNotBoundToVmOnInvalidPtr()
{
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_Create(PDE_SOUND_DEVICE, 0)));
}

void PrlVmDevManipulationsTest::testCreateVmDevNotBoundToVmTryToCallAsyncMeths()
{
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmDev_Create(PDE_HARD_DISK, hVmDev.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVmDev_Connect(hVmDev), PRL_ERR_UNINITIALIZED)
	CHECK_ASYNC_OP_FAILED(PrlVmDev_Disconnect(hVmDev), PRL_ERR_UNINITIALIZED)
	CHECK_ASYNC_OP_FAILED(PrlVmDev_CreateImage(hVmDev, PRL_FALSE, PRL_TRUE), PRL_ERR_UNINITIALIZED)
}

void PrlVmDevManipulationsTest::testCreateVmDevNotBoundToVmTryToCallRemove()
{
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmDev_Create(PDE_HARD_DISK, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hVmDev))
}

void PrlVmDevManipulationsTest::testGetDeviceIndex()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hVmDev.GetHandlePtr()))
	PRL_UINT32 nIndex;
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hVmDev, &nIndex))
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmFloppyDisk _floppy;
	QVERIFY(StringToElement<CVmFloppyDisk*>(&_floppy, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QVERIFY(PRL_UINT32(_floppy.getIndex()) == nIndex);
}

void PrlVmDevManipulationsTest::testSetDeviceIndex()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hVmDev.GetHandlePtr()))
	PRL_UINT32 nIndex;
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hVmDev, &nIndex))
	PRL_UINT32 nNewIndex = (nIndex == 0 ? 1 : 0);
	CHECK_RET_CODE_EXP(PrlVmDev_SetIndex(hVmDev, nNewIndex))
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hVmDev, &nIndex))
	QVERIFY(nNewIndex == nIndex);
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmFloppyDisk _floppy;
	QVERIFY(StringToElement<CVmFloppyDisk*>(&_floppy, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QVERIFY(PRL_UINT32(_floppy.getIndex()) == nNewIndex);
}

void PrlVmDevManipulationsTest::testCreateNewDevice()
{
	INITIALIZE_VM_FROM_FILE("./TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hVmDev.GetHandlePtr()))
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmSerialPort _port;
	QVERIFY(StringToElement<CVmSerialPort*>(&_port, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	PVE::DeviceEnabledState nSetting = _port.getEnabled() ? PVE::DeviceDisabled : PVE::DeviceEnabled;
	_port.setEnabled(nSetting);
	QString sVmSerial = ElementToString<CVmSerialPort*>(&_port, XML_VM_CONFIG_EL_HARDWARE);
	CHECK_RET_CODE_EXP(PrlVmDev_FromString(hVmDev, sVmSerial.toUtf8().data()))
	PRL_VOID_PTR pVmConfig = 0;
	CHECK_RET_CODE_EXP(PrlVm_ToString(m_VmHandle, &pVmConfig))
	QString sVmConfig = UTF8_2QSTR((const char *)pVmConfig);
	CVmConfiguration _vm_conf(sVmConfig);
	PrlBuffer_Free(pVmConfig);
	CHECK_RET_CODE_EXP(_vm_conf.m_uiRcInit)
	CVmHardware *pHardware = _vm_conf.getVmHardwareList();
	QVERIFY(pHardware);
	for (size_t i = 0; i < size_t(pHardware->m_lstSerialPorts.size()); i++)
	{
		CVmSerialPort *pPort = pHardware->m_lstSerialPorts.value(i);
		if (nSetting == pPort->getEnabled())
			return;
	}
	QFAIL("Newly created VM device couldn't be found!");
}

#define EXTRACT_VM_CONFIG\
	PRL_VOID_PTR pVmConfig = 0;\
	QVERIFY(PRL_SUCCEEDED(PrlVm_ToString(m_VmHandle, &pVmConfig)));\
	CVmConfiguration _vm_conf(UTF8_2QSTR((const char *)pVmConfig));\
	PrlBuffer_Free(pVmConfig);\
	QVERIFY(PRL_SUCCEEDED(_vm_conf.m_uiRcInit));\
	CVmHardware *pHardware = _vm_conf.getVmHardwareList();\
	QVERIFY(pHardware);

#define CHECK_ON_TRY_USAGE_REMOVED_VM_DEV\
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_Remove(hVmDev)));\
	PRL_BOOL bConnected;\
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_IsConnected(hVmDev, &bConnected)));\
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_SetConnected(hVmDev, bConnected)));\
	PRL_BOOL bEnabled;\
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_IsEnabled(hVmDev, &bEnabled)));\
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_SetEnabled(hVmDev, bEnabled)));\
	PRL_BOOL bRemote;\
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_IsRemote(hVmDev, &bRemote)));\
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_SetRemote(hVmDev, bRemote)));\
	PRL_VM_DEV_EMULATION_TYPE nEmulatedType = PDT_USE_REAL_DEVICE;\
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_GetEmulatedType(hVmDev, &nEmulatedType)));\
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_SetEmulatedType(hVmDev, nEmulatedType)));\
	PRL_CHAR sImagePath[STR_BUF_LENGTH];\
	PRL_UINT32 nImagePathLength = STR_BUF_LENGTH;\
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_GetImagePath(hVmDev, sImagePath, &nImagePathLength)));\
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_SetImagePath(hVmDev, "image path")));\
	PRL_CHAR sSysName[STR_BUF_LENGTH];\
	PRL_UINT32 nSysNameLength = STR_BUF_LENGTH;\
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_GetSysName(hVmDev, sSysName, &nSysNameLength)));\
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_SetSysName(hVmDev, "system name")));\
	PRL_MASS_STORAGE_INTERFACE_TYPE nIfaceType = PMS_IDE_DEVICE;\
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_GetIfaceType(hVmDev, &nIfaceType)));\
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_SetIfaceType(hVmDev, nIfaceType)));\
	PRL_UINT32 nStackIndex = 0;\
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_GetStackIndex(hVmDev, &nStackIndex)));\
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_SetStackIndex(hVmDev, nStackIndex)));\
	PRL_CHAR sOutputFile[STR_BUF_LENGTH];\
	PRL_UINT32 nOutputFileLength = STR_BUF_LENGTH;\
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_GetOutputFile(hVmDev, sOutputFile, &nOutputFileLength)));\
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_SetOutputFile(hVmDev, "output file")));

void PrlVmDevManipulationsTest::testRemoveDeviceOnFloppy()
{
	INITIALIZE_VM_FROM_FILE("./TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hVmDev))
	EXTRACT_VM_CONFIG
	QVERIFY(pHardware->m_lstFloppyDisks.size() == 0);
	CHECK_ON_TRY_USAGE_REMOVED_VM_DEV
}

void PrlVmDevManipulationsTest::testRemoveDeviceOnHardDisk()
{
	INITIALIZE_VM_FROM_FILE("./TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hVmDev))
	EXTRACT_VM_CONFIG
	QVERIFY(pHardware->m_lstHardDisks.size() == 0);
	CHECK_ON_TRY_USAGE_REMOVED_VM_DEV
}

void PrlVmDevManipulationsTest::testRemoveDeviceOnOpticalDisk()
{
	INITIALIZE_VM_FROM_FILE("./TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hVmDev))
	EXTRACT_VM_CONFIG
	QVERIFY(pHardware->m_lstOpticalDisks.size() == 0);
	CHECK_ON_TRY_USAGE_REMOVED_VM_DEV
}

void PrlVmDevManipulationsTest::testRemoveDeviceOnSerialPort()
{
	INITIALIZE_VM_FROM_FILE("./TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hVmDev))
	EXTRACT_VM_CONFIG
	QVERIFY(pHardware->m_lstSerialPorts.size() == 0);
	CHECK_ON_TRY_USAGE_REMOVED_VM_DEV
}

void PrlVmDevManipulationsTest::testRemoveDeviceOnParallelPort()
{
	INITIALIZE_VM_FROM_FILE("./TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hVmDev))
	EXTRACT_VM_CONFIG
	QVERIFY(pHardware->m_lstParallelPorts.size() == 0);
	CHECK_ON_TRY_USAGE_REMOVED_VM_DEV
}

void PrlVmDevManipulationsTest::testRemoveDeviceOnUsbDevice()
{
	INITIALIZE_VM_FROM_FILE("./TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_USB_DEVICE, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hVmDev))
	EXTRACT_VM_CONFIG
	QVERIFY(pHardware->m_lstUsbDevices.size() == 0);
	CHECK_ON_TRY_USAGE_REMOVED_VM_DEV
}

void PrlVmDevManipulationsTest::testRemoveDeviceOnSound()
{
	INITIALIZE_VM_FROM_FILE("./TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SOUND_DEVICE, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hVmDev))
	EXTRACT_VM_CONFIG
	QVERIFY(pHardware->m_lstSoundDevices.size() == 0);
	CHECK_ON_TRY_USAGE_REMOVED_VM_DEV
}

void PrlVmDevManipulationsTest::testRemoveDeviceOnNetworkAdapter()
{
	INITIALIZE_VM_FROM_FILE("./TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hVmDev))
	EXTRACT_VM_CONFIG
	QVERIFY(pHardware->m_lstNetworkAdapters.size() == 0);
	CHECK_ON_TRY_USAGE_REMOVED_VM_DEV
}

void PrlVmDevManipulationsTest::testIsConnected()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDev.GetHandlePtr()))
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmGenericNetworkAdapter _net_card;
	QVERIFY(StringToElement<CVmGenericNetworkAdapter*>(&_net_card, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	PRL_BOOL bConnected;
	CHECK_RET_CODE_EXP(PrlVmDev_IsConnected(hVmDev, &bConnected))
	QVERIFY(PRL_BOOL(_net_card.getConnected()) == bConnected);
}

void PrlVmDevManipulationsTest::testSetConnected()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDev.GetHandlePtr()))
	PRL_BOOL bConnected;
	CHECK_RET_CODE_EXP(PrlVmDev_IsConnected(hVmDev, &bConnected))
	bConnected = !bConnected;
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hVmDev, bConnected))
	PRL_BOOL bNewConnected;
	CHECK_RET_CODE_EXP(PrlVmDev_IsConnected(hVmDev, &bNewConnected))
	QVERIFY(bNewConnected == bConnected);
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmGenericNetworkAdapter _net_card;
	QVERIFY(StringToElement<CVmGenericNetworkAdapter*>(&_net_card, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QVERIFY(PRL_BOOL(_net_card.getConnected()) == bConnected);
}

void PrlVmDevManipulationsTest::testIsEnabled()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hVmDev.GetHandlePtr()))
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmParallelPort _port;
	QVERIFY(StringToElement<CVmParallelPort*>(&_port, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	PRL_BOOL bEnabled;
	CHECK_RET_CODE_EXP(PrlVmDev_IsEnabled(hVmDev, &bEnabled))
	QVERIFY(PRL_BOOL(_port.getEnabled()) == bEnabled);
}

void PrlVmDevManipulationsTest::testSetEnabled()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hVmDev.GetHandlePtr()))
	PRL_BOOL bEnabled;
	CHECK_RET_CODE_EXP(PrlVmDev_IsEnabled(hVmDev, &bEnabled))
	bEnabled = !bEnabled;
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hVmDev, bEnabled))
	PRL_BOOL bNewEnabled;
	CHECK_RET_CODE_EXP(PrlVmDev_IsEnabled(hVmDev, &bNewEnabled))
	QVERIFY(bNewEnabled == bEnabled);
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmParallelPort _port;
	QVERIFY(StringToElement<CVmParallelPort*>(&_port, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QVERIFY(PRL_BOOL(_port.getEnabled()) == bEnabled);
}

void PrlVmDevManipulationsTest::testGetEmulatedType()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hVmDev.GetHandlePtr()))
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmFloppyDisk _floppy;
	QVERIFY(StringToElement<CVmFloppyDisk*>(&_floppy, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	PRL_VM_DEV_EMULATION_TYPE nEmulatedType;
	CHECK_RET_CODE_EXP(PrlVmDev_GetEmulatedType(hVmDev, &nEmulatedType))
	QVERIFY(PRL_VM_DEV_EMULATION_TYPE(_floppy.getEmulatedType()) == nEmulatedType);
}

void PrlVmDevManipulationsTest::testSetEmulatedType()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hVmDev.GetHandlePtr()))
	PRL_VM_DEV_EMULATION_TYPE nEmulatedType;
	CHECK_RET_CODE_EXP(PrlVmDev_GetEmulatedType(hVmDev, &nEmulatedType))
	nEmulatedType = nEmulatedType == PDT_USE_REAL_DEVICE ? PDT_USE_IMAGE_FILE : PDT_USE_REAL_DEVICE;
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hVmDev, nEmulatedType))
	PRL_VM_DEV_EMULATION_TYPE nNewEmulatedType;
	CHECK_RET_CODE_EXP(PrlVmDev_GetEmulatedType(hVmDev, &nNewEmulatedType))
	QVERIFY(nNewEmulatedType == nEmulatedType);
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmFloppyDisk _floppy;
	QVERIFY(StringToElement<CVmFloppyDisk*>(&_floppy, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QVERIFY(PRL_VM_DEV_EMULATION_TYPE(_floppy.getEmulatedType()) == nEmulatedType);
}

#define PREPARE_VM_DEV_CDROM\
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")\
	QString sExpectedImagePath = "image path";\
	QString sExpectedSysName = "system name";\
	QString sExpectedDescription = "some device description";\
	SdkHandleWrap hVmDev;\
	QVERIFY(PRL_SUCCEEDED(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hVmDev.GetHandlePtr())));\
	CVmOpticalDisk _cdrom;\
	_cdrom.setUserFriendlyName(sExpectedImagePath);\
	_cdrom.setSystemName(sExpectedSysName);\
	_cdrom.setDescription(sExpectedDescription);\
	QString sCdromXml = ElementToString<CVmOpticalDisk*>(&_cdrom, XML_VM_CONFIG_EL_HARDWARE);\
	CHECK_RET_CODE_EXP(PrlVmDev_FromString(hVmDev, sCdromXml.toUtf8().data()))

void PrlVmDevManipulationsTest::testGetImagePath()
{
	PREPARE_VM_DEV_CDROM
	PRL_CHAR sActualImagePath[STR_BUF_LENGTH];
	PRL_UINT32 nActualImagePathLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDev_GetImagePath(hVmDev, sActualImagePath, &nActualImagePathLength))
	QCOMPARE(UTF8_2QSTR(sActualImagePath), sExpectedImagePath);
}

void PrlVmDevManipulationsTest::testGetImagePathNotEnoughBufSize()
{
	PREPARE_VM_DEV_CDROM
	PRL_CHAR sActualImagePath[1];
	PRL_UINT32 nActualImagePathLength = 1;
	QVERIFY(PrlVmDev_GetImagePath(hVmDev, sActualImagePath, &nActualImagePathLength) == PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmDevManipulationsTest::testGetImagePathNullBufSize()
{
	PREPARE_VM_DEV_CDROM
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hVmDev, PrlVmDev_GetImagePath)
}

void PrlVmDevManipulationsTest::testSetImagePath()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	QString sExpectedImagePath = "image path";
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_SetImagePath(hVmDev, sExpectedImagePath.toUtf8().data()))
	PRL_CHAR sActualImagePath[STR_BUF_LENGTH];
	PRL_UINT32 nActualImagePathLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDev_GetImagePath(hVmDev, sActualImagePath, &nActualImagePathLength))
	QCOMPARE(UTF8_2QSTR(sActualImagePath), sExpectedImagePath);
}

void PrlVmDevManipulationsTest::testSetImagePathTryToSetEmptyStringValue()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	QString sImagePath = "";
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_SetImagePath(hVmDev, sImagePath.toUtf8().data()))
	PRL_CHAR sActualImagePath[STR_BUF_LENGTH];
	PRL_UINT32 nActualImagePathLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDev_GetImagePath(hVmDev, sActualImagePath, &nActualImagePathLength))
	QCOMPARE(UTF8_2QSTR(sActualImagePath), sImagePath);
}

void PrlVmDevManipulationsTest::testSetImagePathOnImageDeviceEmulatedType()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	QString sExpectedImagePath = "some image path";
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hVmDev, PDT_USE_IMAGE_FILE))
	CHECK_RET_CODE_EXP(PrlVmDev_SetImagePath(hVmDev, sExpectedImagePath.toUtf8().data()))
	QString sActualImagePath, sActualSystemName;
	PRL_EXTRACT_STRING_VALUE(sActualImagePath, hVmDev, PrlVmDev_GetImagePath)
	PRL_EXTRACT_STRING_VALUE(sActualSystemName, hVmDev, PrlVmDev_GetSysName)
	QCOMPARE(sExpectedImagePath, sActualImagePath);
	QCOMPARE(sExpectedImagePath, sActualSystemName);
}

void PrlVmDevManipulationsTest::testGetIfaceType()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hVmDev.GetHandlePtr()))
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmHardDisk _hdd;
	QVERIFY(StringToElement<CVmHardDisk*>(&_hdd, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	PRL_MASS_STORAGE_INTERFACE_TYPE nIfaceType;
	CHECK_RET_CODE_EXP(PrlVmDev_GetIfaceType(hVmDev, &nIfaceType))
	QVERIFY(_hdd.getInterfaceType() == nIfaceType);
}

void PrlVmDevManipulationsTest::testSetIfaceType()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hVmDev.GetHandlePtr()))
	PRL_MASS_STORAGE_INTERFACE_TYPE nIfaceType;
	CHECK_RET_CODE_EXP(PrlVmDev_GetIfaceType(hVmDev, &nIfaceType))
	nIfaceType = (nIfaceType == PMS_IDE_DEVICE ? PMS_SCSI_DEVICE : PMS_IDE_DEVICE);
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev, nIfaceType))
	PRL_MASS_STORAGE_INTERFACE_TYPE nNewIfaceType;
	CHECK_RET_CODE_EXP(PrlVmDev_GetIfaceType(hVmDev, &nNewIfaceType))
	QVERIFY(nNewIfaceType == nIfaceType);
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmHardDisk _hdd;
	QVERIFY(StringToElement<CVmHardDisk*>(&_hdd, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QVERIFY(_hdd.getInterfaceType() == nIfaceType);
}

void PrlVmDevManipulationsTest::testGetSubType()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hVmDev.GetHandlePtr()))

	PRL_CLUSTERED_DEVICE_SUBTYPE nSubType;
	CHECK_RET_CODE_EXP(PrlVmDev_GetSubType(hVmDev, &nSubType));

	HANDLE_TO_STRING(hVmDev);
	CVmHardDisk hdd;
	CHECK_RET_CODE_EXP(hdd.fromString(_str_object));

	QCOMPARE(quint32(nSubType), quint32(hdd.getSubType()));
}

void PrlVmDevManipulationsTest::testSetSubType()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hVmDev.GetHandlePtr()))

	PRL_CLUSTERED_DEVICE_SUBTYPE nSubType;
	CHECK_RET_CODE_EXP(PrlVmDev_GetSubType(hVmDev, &nSubType))
	nSubType = (PRL_CLUSTERED_DEVICE_SUBTYPE )(1 + nSubType);
	CHECK_RET_CODE_EXP(PrlVmDev_SetSubType(hVmDev, nSubType))
	PRL_CLUSTERED_DEVICE_SUBTYPE nActualSubType;
	CHECK_RET_CODE_EXP(PrlVmDev_GetSubType(hVmDev, &nActualSubType))
	QCOMPARE(quint32(nActualSubType), quint32(nSubType));
}

void PrlVmDevManipulationsTest::testSubTypeOnWrongParams()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hVmDev.GetHandlePtr()))

	// Wrong handle
	PRL_CLUSTERED_DEVICE_SUBTYPE nSubType;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetSubType(m_ServerHandle, &nSubType),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetSubType(m_ServerHandle, PCD_LSI_SPI),
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetSubType(hVmDev, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmDevManipulationsTest::testGetDeviceStackIndex()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hVmDev.GetHandlePtr()))
	PRL_UINT32 nStackIndex;
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev, &nStackIndex))
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmOpticalDisk _cdrom;
	QVERIFY(StringToElement<CVmOpticalDisk*>(&_cdrom, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QVERIFY(PRL_UINT32(_cdrom.getStackIndex()) == nStackIndex);
}

void PrlVmDevManipulationsTest::testSetDeviceStackIndex()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hVmDev.GetHandlePtr()))
	PRL_UINT32 nStackIndex;
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev, &nStackIndex))
	PRL_UINT32 nNewStackIndex = (nStackIndex == 0 ? 1 : 0);
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hVmDev, nNewStackIndex))
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev, &nStackIndex))
	QVERIFY(nNewStackIndex == nStackIndex);
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmOpticalDisk _cdrom;
	QVERIFY(StringToElement<CVmOpticalDisk*>(&_cdrom, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QVERIFY(PRL_UINT32(_cdrom.getStackIndex()) == nNewStackIndex);
}

#define PREPARE_VM_DEV_SERIAL_PORT\
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")\
	QString sExpectedOutputFile = "output file";\
	SdkHandleWrap hVmDev;\
	QVERIFY(PRL_SUCCEEDED(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hVmDev.GetHandlePtr())));\
	CVmSerialPort _port;\
	_port.setUserFriendlyName(sExpectedOutputFile);\
	QString sPortXml = ElementToString<CVmSerialPort*>(&_port, XML_VM_CONFIG_EL_HARDWARE);\
	CHECK_RET_CODE_EXP(PrlVmDev_FromString(hVmDev, sPortXml.toUtf8().data()))

void PrlVmDevManipulationsTest::testGetOutputFile()
{
	PREPARE_VM_DEV_SERIAL_PORT
	PRL_CHAR sActualOutputFile[STR_BUF_LENGTH];
	PRL_UINT32 nActualOutputFileLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDev_GetOutputFile(hVmDev, sActualOutputFile, &nActualOutputFileLength))
	QCOMPARE(UTF8_2QSTR(sActualOutputFile), sExpectedOutputFile);
}

void PrlVmDevManipulationsTest::testGetOutputFileNotEnoughBufSize()
{
	PREPARE_VM_DEV_SERIAL_PORT
	PRL_CHAR sActualOutputFile[1];
	PRL_UINT32 nActualOutputFileLength = 1;
	QVERIFY(PrlVmDev_GetOutputFile(hVmDev, sActualOutputFile, &nActualOutputFileLength) == PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmDevManipulationsTest::testGetOutputFileNullBufSize()
{
	PREPARE_VM_DEV_SERIAL_PORT
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hVmDev, PrlVmDev_GetOutputFile)
}

void PrlVmDevManipulationsTest::testSetOutputFile()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	QString sExpectedOutputFile = "output file";
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_SetOutputFile(hVmDev, sExpectedOutputFile.toUtf8().data()))
	PRL_CHAR sActualOutputFile[STR_BUF_LENGTH];
	PRL_UINT32 nActualOutputFileLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDev_GetOutputFile(hVmDev, sActualOutputFile, &nActualOutputFileLength))
	QCOMPARE(UTF8_2QSTR(sActualOutputFile), sExpectedOutputFile);
}

void PrlVmDevManipulationsTest::testSetOutputFileTryToSetEmptyStringValue()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	QString sOutputFile = "";
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_SetOutputFile(hVmDev, sOutputFile.toUtf8().data()))
	PRL_CHAR sActualOutputFile[STR_BUF_LENGTH];
	PRL_UINT32 nActualOutputFileLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDev_GetOutputFile(hVmDev, sActualOutputFile, &nActualOutputFileLength))
	QCOMPARE(UTF8_2QSTR(sActualOutputFile), sOutputFile);
}

#define CREATE_HARD_DISK_DEVICE\
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")\
	SdkHandleWrap hVmDev;\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hVmDev.GetHandlePtr()))

void PrlVmDevManipulationsTest::testHardDiskGetType()
{
	CREATE_HARD_DISK_DEVICE
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmHardDisk _hdd;
	_hdd.setDiskType(_hdd.getDiskType() == PHD_PLAIN_HARD_DISK ? PHD_EXPANDING_HARD_DISK : PHD_PLAIN_HARD_DISK);
	QVERIFY(StringToElement<CVmHardDisk*>(&_hdd, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	PRL_HARD_DISK_INTERNAL_FORMAT nDiskType;
	CHECK_RET_CODE_EXP(PrlVmDevHd_GetDiskType(hVmDev, &nDiskType))
	QVERIFY(_hdd.getDiskType() == nDiskType);
}

void PrlVmDevManipulationsTest::testHardDiskSetType()
{
	CREATE_HARD_DISK_DEVICE
	PRL_HARD_DISK_INTERNAL_FORMAT nDiskType;
	CHECK_RET_CODE_EXP(PrlVmDevHd_GetDiskType(hVmDev, &nDiskType))
	nDiskType = (nDiskType == PHD_PLAIN_HARD_DISK ? PHD_EXPANDING_HARD_DISK : PHD_PLAIN_HARD_DISK);
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskType(hVmDev, nDiskType))
	PRL_HARD_DISK_INTERNAL_FORMAT nNewDiskType;
	CHECK_RET_CODE_EXP(PrlVmDevHd_GetDiskType(hVmDev, &nNewDiskType))
	QVERIFY(nNewDiskType == nDiskType);
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmHardDisk _hdd;
	QVERIFY(StringToElement<CVmHardDisk*>(&_hdd, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QVERIFY(_hdd.getDiskType() == nDiskType);
}

void PrlVmDevManipulationsTest::testHardDiskIsSplitted()
{
	CREATE_HARD_DISK_DEVICE
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmHardDisk _hdd;
	_hdd.setSplitted(!_hdd.isSplitted());
	QVERIFY(StringToElement<CVmHardDisk*>(&_hdd, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	PRL_BOOL bSplitted;
	CHECK_RET_CODE_EXP(PrlVmDevHd_IsSplitted(hVmDev, &bSplitted))
	QVERIFY(_hdd.isSplitted() == (bool) bSplitted);
}

void PrlVmDevManipulationsTest::testHardDiskSetSplitted()
{
	CREATE_HARD_DISK_DEVICE
	PRL_BOOL bSplitted;
	CHECK_RET_CODE_EXP(PrlVmDevHd_IsSplitted(hVmDev, &bSplitted))
	bSplitted = !bSplitted;
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetSplitted(hVmDev, bSplitted))
	PRL_BOOL bNewSplitted;
	CHECK_RET_CODE_EXP(PrlVmDevHd_IsSplitted(hVmDev, &bNewSplitted))
	QVERIFY(bNewSplitted == bSplitted);
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmHardDisk _hdd;
	QVERIFY(StringToElement<CVmHardDisk*>(&_hdd, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QVERIFY(_hdd.isSplitted() == (bool) bSplitted);
}

void PrlVmDevManipulationsTest::testHardDiskGetSize()
{
	CREATE_HARD_DISK_DEVICE
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmHardDisk _hdd;
	_hdd.setSize(8192);
	QVERIFY(StringToElement<CVmHardDisk*>(&_hdd, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	PRL_UINT32 nDiskSize;
	CHECK_RET_CODE_EXP(PrlVmDevHd_GetDiskSize(hVmDev, &nDiskSize))
	QVERIFY(_hdd.getSize() == nDiskSize);
}

void PrlVmDevManipulationsTest::testHardDiskSetSize()
{
	CREATE_HARD_DISK_DEVICE
	PRL_UINT32 nDiskSize;
	CHECK_RET_CODE_EXP(PrlVmDevHd_GetDiskSize(hVmDev, &nDiskSize))
	nDiskSize = (nDiskSize == 8192 ? 4096 : 8192);
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskSize(hVmDev, nDiskSize))
	PRL_UINT32 nNewDiskSize;
	CHECK_RET_CODE_EXP(PrlVmDevHd_GetDiskSize(hVmDev, &nNewDiskSize))
	QVERIFY(nNewDiskSize == nDiskSize);
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmHardDisk _hdd;
	QVERIFY(StringToElement<CVmHardDisk*>(&_hdd, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QVERIFY(_hdd.getSize() == nDiskSize);
}

void PrlVmDevManipulationsTest::testHardDiskGetSizeOnDisk()
{
	CREATE_HARD_DISK_DEVICE
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmHardDisk _hdd;
	_hdd.setSizeOnDisk(1024*1024*800);
	QVERIFY(StringToElement<CVmHardDisk*>(&_hdd, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	PRL_UINT32 nDiskSize;
	CHECK_RET_CODE_EXP(PrlVmDevHd_GetSizeOnDisk(hVmDev, &nDiskSize))
	QVERIFY(_hdd.getSize() == nDiskSize);
}

void PrlVmDevManipulationsTest::testIsPassthrough()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hVmDev.GetHandlePtr()))
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmOpticalDisk _cdrom;
	_cdrom.setPassthrough(PVE::DevicePassthroughMode(!_cdrom.getPassthrough()));
	QVERIFY(StringToElement<CVmOpticalDisk*>(&_cdrom, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	PRL_BOOL bPassthrough;
	CHECK_RET_CODE_EXP(PrlVmDev_IsPassthrough(hVmDev, &bPassthrough))
	QVERIFY(PRL_BOOL(_cdrom.getPassthrough()) == bPassthrough);
}

void PrlVmDevManipulationsTest::testSetPassthrough()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hVmDev.GetHandlePtr()))
	PRL_BOOL bPassthrough;
	CHECK_RET_CODE_EXP(PrlVmDev_IsPassthrough(hVmDev, &bPassthrough))
	bPassthrough = !bPassthrough;
	CHECK_RET_CODE_EXP(PrlVmDev_SetPassthrough(hVmDev, bPassthrough))
	PRL_BOOL bNewPassthrough;
	CHECK_RET_CODE_EXP(PrlVmDev_IsPassthrough(hVmDev, &bNewPassthrough))
	QVERIFY(bNewPassthrough == bPassthrough);
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmOpticalDisk _cdrom;
	QVERIFY(StringToElement<CVmOpticalDisk*>(&_cdrom, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QVERIFY(PRL_BOOL(_cdrom.getPassthrough()) == bPassthrough);
}

#define CREATE_NET_ADAPTER_DEVICE\
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")\
	SdkHandleWrap hVmDev;\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDev.GetHandlePtr()))

void PrlVmDevManipulationsTest::testNetGetBoundAdapterIndex()
{
	CREATE_NET_ADAPTER_DEVICE
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmGenericNetworkAdapter _net;
	_net.setBoundAdapterIndex(_net.getBoundAdapterIndex() == 1 ? 0 : 1);
	QVERIFY(StringToElement<CVmGenericNetworkAdapter*>(&_net, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	PRL_INT32 nIndex;
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetBoundAdapterIndex(hVmDev, &nIndex))
	QVERIFY(PRL_INT32(_net.getBoundAdapterIndex()) == nIndex);
}

void PrlVmDevManipulationsTest::testNetSetBoundAdapterIndex()
{
	CREATE_NET_ADAPTER_DEVICE
	PRL_INT32 nIndex;
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetBoundAdapterIndex(hVmDev, &nIndex))
	nIndex = (nIndex == 1 ? 0 : 1);
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetBoundAdapterIndex(hVmDev, nIndex))
	PRL_INT32 nNewIndex;
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetBoundAdapterIndex(hVmDev, &nNewIndex))
	QVERIFY(nNewIndex == nIndex);
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmGenericNetworkAdapter _net;
	QVERIFY(StringToElement<CVmGenericNetworkAdapter*>(&_net, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QVERIFY(PRL_INT32(_net.getBoundAdapterIndex()) == nIndex);
}

#define PREPARE_VM_DEV_NET_ADAPTER\
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")\
	QString sExpectedBoundAdapterName = "eth0";\
	QString sExpectedMacAddress = TEST_MAC_ADDR;\
	QString sExpectedDefaultGateway = TEST_DEFAULT_GATEWAY;\
	QString sExpectedDefaultGatewayIPv6 = TEST_DEFAULT_GATEWAY_IPv6;\
	QStringList lstNetAddresses = QStringList()<<"1.2.3.4/1.2.3.0:24"<<"5.6.7.8/5.6.7.0:24"<<"9.10.11.12/9.10.11.0:24";\
	QStringList lstDnsServers = QStringList()<<"192.168.1.254"<<"10.30.8.254";\
	QStringList lstSearchDomains = QStringList()<<"sw.ru"<<"parallels.com";\
	QString sExpectedVirtualNetworkId = Uuid::createUuid().toString();\
	SdkHandleWrap hVmDev;\
	QVERIFY(PRL_SUCCEEDED(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDev.GetHandlePtr())));\
	CVmGenericNetworkAdapter _net;\
	_net.setBoundAdapterName(sExpectedBoundAdapterName);\
	_net.setMacAddress(sExpectedMacAddress);\
	_net.setNetAddresses(lstNetAddresses);\
	_net.setDnsIPAddresses(lstDnsServers);\
	_net.setSearchDomains(lstSearchDomains);\
	_net.setDefaultGateway(sExpectedDefaultGateway);\
	_net.setDefaultGatewayIPv6(sExpectedDefaultGatewayIPv6);\
	_net.setVirtualNetworkID(sExpectedVirtualNetworkId);\
	QString sNetXml = ElementToString<CVmGenericNetworkAdapter*>(&_net, XML_VM_CONFIG_EL_HARDWARE);\
	CHECK_RET_CODE_EXP(PrlVmDev_FromString(hVmDev, sNetXml.toUtf8().data()))

void PrlVmDevManipulationsTest::testNetGetBoundAdapterName()
{
	PREPARE_VM_DEV_NET_ADAPTER
	PRL_CHAR sActualBoundAdapterName[STR_BUF_LENGTH];
	PRL_UINT32 nActualBoundAdapterNameLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetBoundAdapterName(hVmDev, sActualBoundAdapterName, &nActualBoundAdapterNameLength))
	QCOMPARE(UTF8_2QSTR(sActualBoundAdapterName), sExpectedBoundAdapterName);
}

void PrlVmDevManipulationsTest::testNetGetBoundAdapterNameNotEnoughBufSize()
{
	PREPARE_VM_DEV_NET_ADAPTER
	PRL_CHAR sActualBoundAdapterName[1];
	PRL_UINT32 nActualBoundAdapterNameLength = 1;
	QVERIFY(PrlVmDevNet_GetBoundAdapterName(hVmDev, sActualBoundAdapterName, &nActualBoundAdapterNameLength) == PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmDevManipulationsTest::testNetGetBoundAdapterNameNullBufSize()
{
	PREPARE_VM_DEV_NET_ADAPTER
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hVmDev, PrlVmDevNet_GetBoundAdapterName)
}

void PrlVmDevManipulationsTest::testNetSetBoundAdapterName()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	QString sExpectedBoundAdapterName = "eth0";
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetBoundAdapterName(hVmDev, sExpectedBoundAdapterName.toUtf8().data()))
	PRL_CHAR sActualBoundAdapterName[STR_BUF_LENGTH];
	PRL_UINT32 nActualBoundAdapterNameLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetBoundAdapterName(hVmDev, sActualBoundAdapterName, &nActualBoundAdapterNameLength))
	QCOMPARE(UTF8_2QSTR(sActualBoundAdapterName), sExpectedBoundAdapterName);
}

void PrlVmDevManipulationsTest::testNetSetBoundAdapterNameTryToSetEmptyStringValue()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	QString sBoundAdapterName = "";
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetBoundAdapterName(hVmDev, sBoundAdapterName.toUtf8().data()))
	PRL_CHAR sActualBoundAdapterName[STR_BUF_LENGTH];
	PRL_UINT32 nActualBoundAdapterNameLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetBoundAdapterName(hVmDev, sActualBoundAdapterName, &nActualBoundAdapterNameLength))
	QCOMPARE(UTF8_2QSTR(sActualBoundAdapterName), sBoundAdapterName);
}

void PrlVmDevManipulationsTest::testNetGetMacAddress()
{
	PREPARE_VM_DEV_NET_ADAPTER
	PRL_CHAR sActualMacAddress[STR_BUF_LENGTH];
	PRL_UINT32 nActualMacAddressLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetMacAddress(hVmDev, sActualMacAddress, &nActualMacAddressLength))
	QCOMPARE(UTF8_2QSTR(sActualMacAddress), sExpectedMacAddress);
}

void PrlVmDevManipulationsTest::testNetGetMacAddressNotEnoughBufSize()
{
	PREPARE_VM_DEV_NET_ADAPTER
	PRL_CHAR sActualMacAddress[1];
	PRL_UINT32 nActualMacAddressLength = 1;
	QVERIFY(PrlVmDevNet_GetMacAddress(hVmDev, sActualMacAddress, &nActualMacAddressLength) == PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmDevManipulationsTest::testNetGetMacAddressNullBufSize()
{
	PREPARE_VM_DEV_NET_ADAPTER
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hVmDev, PrlVmDevNet_GetMacAddress)
}

void PrlVmDevManipulationsTest::testNetSetMacAddress()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	QString sExpectedMacAddress = TEST_MAC_ADDR;
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetMacAddress(hVmDev, sExpectedMacAddress.toUtf8().data()))
	PRL_CHAR sActualMacAddress[STR_BUF_LENGTH];
	PRL_UINT32 nActualMacAddressLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetMacAddress(hVmDev, sActualMacAddress, &nActualMacAddressLength))
	QCOMPARE(UTF8_2QSTR(sActualMacAddress), sExpectedMacAddress);
}

void PrlVmDevManipulationsTest::testNetGetDefaultGateway()
{
	PREPARE_VM_DEV_NET_ADAPTER
	QString sActualDefaultGateway;
	PRL_EXTRACT_STRING_VALUE(sActualDefaultGateway, hVmDev, PrlVmDevNet_GetDefaultGateway)
	QCOMPARE(sActualDefaultGateway, sExpectedDefaultGateway);
}

void PrlVmDevManipulationsTest::testNetGetDefaultGatewayNotEnoughBufSize()
{
	PREPARE_VM_DEV_NET_ADAPTER
	PRL_CHAR sActualDefaultGateway[1];
	PRL_UINT32 nActualDefaultGatewayLength = 1;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetDefaultGateway(hVmDev, sActualDefaultGateway, &nActualDefaultGatewayLength),\
		PRL_ERR_BUFFER_OVERRUN)
}

void PrlVmDevManipulationsTest::testNetGetDefaultGatewayNullBufSize()
{
	PREPARE_VM_DEV_NET_ADAPTER
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hVmDev, PrlVmDevNet_GetDefaultGateway)
}

void PrlVmDevManipulationsTest::testNetSetDefaultGateway()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	QString sExpectedDefaultGateway = TEST_DEFAULT_GATEWAY;
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetDefaultGateway(hVmDev, QSTR2UTF8(sExpectedDefaultGateway)))
	QString sActualDefaultGateway;
	PRL_EXTRACT_STRING_VALUE(sActualDefaultGateway, hVmDev, PrlVmDevNet_GetDefaultGateway)
	QCOMPARE(sActualDefaultGateway, sExpectedDefaultGateway);
}

void PrlVmDevManipulationsTest::testNetGetDefaultGatewayIPv6()
{
	PREPARE_VM_DEV_NET_ADAPTER;

	QString sActualDefaultGatewayIPv6;
	PRL_EXTRACT_STRING_VALUE(sActualDefaultGatewayIPv6, hVmDev, PrlVmDevNet_GetDefaultGatewayIPv6);
	QCOMPARE(sActualDefaultGatewayIPv6, sExpectedDefaultGatewayIPv6);
}

void PrlVmDevManipulationsTest::testNetGetDefaultGatewayIPv6OnWrongParams()
{
	PREPARE_VM_DEV_NET_ADAPTER;

	// Invalid handle
	PRL_UINT32 nBufSize = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetDefaultGatewayIPv6(m_VmHandle, 0, &nBufSize),
		PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetDefaultGatewayIPv6(hVmDev, 0, 0),
		PRL_ERR_INVALID_ARG);
	// Null buffer size
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hVmDev, PrlVmDevNet_GetDefaultGatewayIPv6);
	// Not enough buffer size
	PRL_CHAR sDefaultGatewayIPv6[3];
	PRL_UINT32 nLength = 3;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetDefaultGateway(hVmDev, sDefaultGatewayIPv6, &nLength),
		PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmDevManipulationsTest::testNetSetDefaultGatewayIPv6()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml");

	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDev.GetHandlePtr()));

	QString sExpectedDefaultGatewayIPv6 = TEST_DEFAULT_GATEWAY_IPv6;
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetDefaultGatewayIPv6(hVmDev, QSTR2UTF8(sExpectedDefaultGatewayIPv6)));
	QString sActualDefaultGatewayIPv6;
	PRL_EXTRACT_STRING_VALUE(sActualDefaultGatewayIPv6, hVmDev, PrlVmDevNet_GetDefaultGatewayIPv6);
	QCOMPARE(sActualDefaultGatewayIPv6, sExpectedDefaultGatewayIPv6);
}

void PrlVmDevManipulationsTest::testNetSetDefaultGatewayIPv6OnWrongParams()
{
	PREPARE_VM_DEV_NET_ADAPTER;

	// Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
		PrlVmDevNet_SetDefaultGatewayIPv6(m_VmHandle, QSTR2UTF8(sExpectedDefaultGatewayIPv6)),
		PRL_ERR_INVALID_ARG);
}

void PrlVmDevManipulationsTest::testNetGetVirtualNetworkId()
{
	PREPARE_VM_DEV_NET_ADAPTER
	QString sActualVirtualNetworkId;
	PRL_EXTRACT_STRING_VALUE(sActualVirtualNetworkId, hVmDev, PrlVmDevNet_GetVirtualNetworkId)
	QCOMPARE(sActualVirtualNetworkId, sExpectedVirtualNetworkId);
}

void PrlVmDevManipulationsTest::testNetGetVirtualNetworkIdNotEnoughBufSize()
{
	PREPARE_VM_DEV_NET_ADAPTER
	PRL_CHAR sActualVirtualNetworkId[1];
	PRL_UINT32 nActualVirtualNetworkIdLength = 1;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetVirtualNetworkId(hVmDev, sActualVirtualNetworkId, &nActualVirtualNetworkIdLength),\
		PRL_ERR_BUFFER_OVERRUN)
}

void PrlVmDevManipulationsTest::testNetGetVirtualNetworkIdNullBufSize()
{
	PREPARE_VM_DEV_NET_ADAPTER
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hVmDev, PrlVmDevNet_GetVirtualNetworkId)
}

void PrlVmDevManipulationsTest::testNetGetVirtualNetworkIdOnWrongParams()
{
	PRL_UINT32 nBufSize = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetVirtualNetworkId(PRL_INVALID_HANDLE, 0, &nBufSize), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetVirtualNetworkId(m_ServerHandle, 0, &nBufSize), PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testNetSetVirtualNetworkId()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	QString sExpectedVirtualNetworkId = TEST_DEFAULT_GATEWAY;
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetVirtualNetworkId(hVmDev, QSTR2UTF8(sExpectedVirtualNetworkId)))
	QString sActualVirtualNetworkId;
	PRL_EXTRACT_STRING_VALUE(sActualVirtualNetworkId, hVmDev, PrlVmDevNet_GetVirtualNetworkId)
	QCOMPARE(sActualVirtualNetworkId, sExpectedVirtualNetworkId);
	//Check on empty values
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetVirtualNetworkId(hVmDev, 0))
	PRL_EXTRACT_STRING_VALUE(sActualVirtualNetworkId, hVmDev, PrlVmDevNet_GetVirtualNetworkId)
	QVERIFY(sActualVirtualNetworkId.isEmpty());
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetVirtualNetworkId(hVmDev, ""))
	PRL_EXTRACT_STRING_VALUE(sActualVirtualNetworkId, hVmDev, PrlVmDevNet_GetVirtualNetworkId)
	QVERIFY(sActualVirtualNetworkId.isEmpty());
}

void PrlVmDevManipulationsTest::testNetSetVirtualNetworkIdOnWrongParams()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetVirtualNetworkId(PRL_INVALID_HANDLE, ""), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetVirtualNetworkId(m_ServerHandle, ""), PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testNetGetNetAddresses()
{
	PREPARE_VM_DEV_NET_ADAPTER
	PRL_CHECK_STRINGS_LIST(hVmDev, PrlVmDevNet_GetNetAddresses, lstNetAddresses)
}

void PrlVmDevManipulationsTest::testNetGetNetAddressesOnNullPtr()
{
	PREPARE_VM_DEV_NET_ADAPTER
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetNetAddresses(hVmDev, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testNetGetNetAddressesOnNullNetAdapterHandle()
{
	SdkHandleWrap hNetAddressesList;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetNetAddresses(PRL_INVALID_HANDLE, hNetAddressesList.GetHandlePtr()),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testNetGetNetAddressesOnNonNetAdapterHandle()
{
	SdkHandleWrap hNetAddressesList;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetNetAddresses(m_ServerHandle, hNetAddressesList.GetHandlePtr()),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testSetNetAddresses()
{
	PREPARE_VM_DEV_NET_ADAPTER
	QStringList lstNewNetAddresses = QStringList()<<"192.168.1.1/255.255.255.0"<<"10.30.1.1/255.255.255.0";
	SdkHandleWrap hNewNetAddressesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hNewNetAddressesList.GetHandlePtr()))
	foreach(QString sNetAddress, lstNewNetAddresses)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewNetAddressesList, QSTR2UTF8(sNetAddress)))
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetNetAddresses(hVmDev, hNewNetAddressesList))
	PRL_CHECK_STRINGS_LIST(hVmDev, PrlVmDevNet_GetNetAddresses, lstNewNetAddresses)
}

void PrlVmDevManipulationsTest::testSetNetAddressesOnNullNetAdapterHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetNetAddresses(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE),\
			PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testSetNetAddressesOnNonNetAdapterHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetNetAddresses(m_ServerHandle, PRL_INVALID_HANDLE),\
			PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testSetNetAddressesSetEmptyListAsNullListHandle()
{
	PREPARE_VM_DEV_NET_ADAPTER

	SdkHandleWrap hNetAddressesList;
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetNetAddresses(hVmDev, hNetAddressesList.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hNetAddressesList, PHT_STRINGS_LIST)
	PRL_UINT32 nItemsCount = 0;
	CHECK_RET_CODE_EXP(PrlStrList_GetItemsCount(hNetAddressesList, &nItemsCount))
	QCOMPARE(quint32(nItemsCount), quint32(lstNetAddresses.size()));

	CHECK_RET_CODE_EXP(PrlVmDevNet_SetNetAddresses(hVmDev, PRL_INVALID_HANDLE))

	CHECK_RET_CODE_EXP(PrlVmDevNet_GetNetAddresses(hVmDev, hNetAddressesList.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hNetAddressesList, PHT_STRINGS_LIST)
	CHECK_RET_CODE_EXP(PrlStrList_GetItemsCount(hNetAddressesList, &nItemsCount))
	QCOMPARE(quint32(nItemsCount), quint32(0));
}

void PrlVmDevManipulationsTest::testNetGetDnsServers()
{
	PREPARE_VM_DEV_NET_ADAPTER
	PRL_CHECK_STRINGS_LIST(hVmDev, PrlVmDevNet_GetDnsServers, lstDnsServers)
}

void PrlVmDevManipulationsTest::testNetGetDnsServersOnNullPtr()
{
	PREPARE_VM_DEV_NET_ADAPTER
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetDnsServers(hVmDev, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testNetGetDnsServersOnNullNetAdapterHandle()
{
	SdkHandleWrap hDnsServersList;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetDnsServers(PRL_INVALID_HANDLE, hDnsServersList.GetHandlePtr()),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testNetGetDnsServersOnNonNetAdapterHandle()
{
	SdkHandleWrap hDnsServersList;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetDnsServers(m_ServerHandle, hDnsServersList.GetHandlePtr()),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testSetDnsServers()
{
	PREPARE_VM_DEV_NET_ADAPTER
	QStringList lstNewDnsServers = QStringList()<<"192.168.1.1/192.168.1.0:24"<<"10.30.1.1/10.30.1.0:24";
	SdkHandleWrap hNewDnsServersList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hNewDnsServersList.GetHandlePtr()))
	foreach(QString sNetAddress, lstNewDnsServers)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewDnsServersList, QSTR2UTF8(sNetAddress)))
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetDnsServers(hVmDev, hNewDnsServersList))
	PRL_CHECK_STRINGS_LIST(hVmDev, PrlVmDevNet_GetDnsServers, lstNewDnsServers)
}

void PrlVmDevManipulationsTest::testSetDnsServersOnNullNetAdapterHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetDnsServers(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE),\
			PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testSetDnsServersOnNonNetAdapterHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetDnsServers(m_ServerHandle, PRL_INVALID_HANDLE),\
			PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testSetDnsServersSetEmptyListAsNullListHandle()
{
	PREPARE_VM_DEV_NET_ADAPTER

	SdkHandleWrap hDnsServersList;
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetDnsServers(hVmDev, hDnsServersList.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hDnsServersList, PHT_STRINGS_LIST)
	PRL_UINT32 nItemsCount = 0;
	CHECK_RET_CODE_EXP(PrlStrList_GetItemsCount(hDnsServersList, &nItemsCount))
	QCOMPARE(quint32(nItemsCount), quint32(lstDnsServers.size()));

	CHECK_RET_CODE_EXP(PrlVmDevNet_SetDnsServers(hVmDev, PRL_INVALID_HANDLE))

	CHECK_RET_CODE_EXP(PrlVmDevNet_GetDnsServers(hVmDev, hDnsServersList.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hDnsServersList, PHT_STRINGS_LIST)
	CHECK_RET_CODE_EXP(PrlStrList_GetItemsCount(hDnsServersList, &nItemsCount))
	QCOMPARE(quint32(nItemsCount), quint32(0));
}

void PrlVmDevManipulationsTest::testNetGetSearchDomains()
{
	PREPARE_VM_DEV_NET_ADAPTER
	PRL_CHECK_STRINGS_LIST(hVmDev, PrlVmDevNet_GetSearchDomains, lstSearchDomains)
}

void PrlVmDevManipulationsTest::testNetGetSearchDomainsOnNullPtr()
{
	PREPARE_VM_DEV_NET_ADAPTER
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetSearchDomains(hVmDev, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testNetGetSearchDomainsOnNullNetAdapterHandle()
{
	SdkHandleWrap hSearchDomainsList;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetSearchDomains(PRL_INVALID_HANDLE, hSearchDomainsList.GetHandlePtr()),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testNetGetSearchDomainsOnNonNetAdapterHandle()
{
	SdkHandleWrap hSearchDomainsList;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetSearchDomains(m_ServerHandle, hSearchDomainsList.GetHandlePtr()),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testSetSearchDomains()
{
	PREPARE_VM_DEV_NET_ADAPTER
	QStringList lstNewSearchDomains = QStringList()<<"192.168.1.1/192.168.1.0:24"<<"10.30.1.1/10.30.1.0:24";
	SdkHandleWrap hNewSearchDomainsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hNewSearchDomainsList.GetHandlePtr()))
	foreach(QString sNetAddress, lstNewSearchDomains)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewSearchDomainsList, QSTR2UTF8(sNetAddress)))
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetSearchDomains(hVmDev, hNewSearchDomainsList))
	PRL_CHECK_STRINGS_LIST(hVmDev, PrlVmDevNet_GetSearchDomains, lstNewSearchDomains)
}

void PrlVmDevManipulationsTest::testSetSearchDomainsOnNullNetAdapterHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetSearchDomains(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE),\
			PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testSetSearchDomainsOnNonNetAdapterHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetSearchDomains(m_ServerHandle, PRL_INVALID_HANDLE),\
			PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testSetSearchDomainsSetEmptyListAsNullListHandle()
{
	PREPARE_VM_DEV_NET_ADAPTER

	SdkHandleWrap hSearchDomainsList;
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetSearchDomains(hVmDev, hSearchDomainsList.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hSearchDomainsList, PHT_STRINGS_LIST)
	PRL_UINT32 nItemsCount = 0;
	CHECK_RET_CODE_EXP(PrlStrList_GetItemsCount(hSearchDomainsList, &nItemsCount))
	QCOMPARE(quint32(nItemsCount), quint32(lstSearchDomains.size()));

	CHECK_RET_CODE_EXP(PrlVmDevNet_SetSearchDomains(hVmDev, PRL_INVALID_HANDLE))

	CHECK_RET_CODE_EXP(PrlVmDevNet_GetSearchDomains(hVmDev, hSearchDomainsList.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hSearchDomainsList, PHT_STRINGS_LIST)
	CHECK_RET_CODE_EXP(PrlStrList_GetItemsCount(hSearchDomainsList, &nItemsCount))
	QCOMPARE(quint32(nItemsCount), quint32(0));
}

void PrlVmDevManipulationsTest::testNetIsConfigureWithDhcp()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDev.GetHandlePtr()))
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmGenericNetworkAdapter _adapter;
	QVERIFY(StringToElement<CVmGenericNetworkAdapter*>(&_adapter, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	PRL_BOOL bConfigureWithDhcp;
	CHECK_RET_CODE_EXP(PrlVmDevNet_IsConfigureWithDhcp(hVmDev, &bConfigureWithDhcp))
	QVERIFY(PRL_BOOL(_adapter.isConfigureWithDhcp()) == bConfigureWithDhcp);
}

void PrlVmDevManipulationsTest::testNetSetConfigureWithDhcp()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDev.GetHandlePtr()))
	PRL_BOOL bConfigureWithDhcp;
	CHECK_RET_CODE_EXP(PrlVmDevNet_IsConfigureWithDhcp(hVmDev, &bConfigureWithDhcp))
	bConfigureWithDhcp = !bConfigureWithDhcp;
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetConfigureWithDhcp(hVmDev, bConfigureWithDhcp))
	PRL_BOOL bNewConfigureWithDhcp;
	CHECK_RET_CODE_EXP(PrlVmDevNet_IsConfigureWithDhcp(hVmDev, &bNewConfigureWithDhcp))
	QVERIFY(bNewConfigureWithDhcp == bConfigureWithDhcp);
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmGenericNetworkAdapter _adapter;
	QVERIFY(StringToElement<CVmGenericNetworkAdapter*>(&_adapter, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QVERIFY(PRL_BOOL(_adapter.isConfigureWithDhcp()) == bConfigureWithDhcp);

	//check clean NetAddress
	bConfigureWithDhcp = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetConfigureWithDhcp(hVmDev, bConfigureWithDhcp));

	QStringList lstNetAddressesIPv6 = QStringList()<<"c001::1/70"<<"ee::aa:0:1/124";
	QStringList lstNetAddressesAll = QStringList()<<"192.168.1.1/255.255.255.0"
				<<"c001::1/70"<<"10.30.1.1/255.255.255.0"<<"ee::aa:0:1/124";

	SdkHandleWrap hNewNetAddressesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hNewNetAddressesList.GetHandlePtr()))
	foreach(QString sNetAddress, lstNetAddressesAll)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewNetAddressesList, QSTR2UTF8(sNetAddress)))
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetNetAddresses(hVmDev, hNewNetAddressesList))
	PRL_CHECK_NET_ADDRESS_LIST(hVmDev, PrlVmDevNet_GetNetAddresses, lstNetAddressesAll)

	bConfigureWithDhcp = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetConfigureWithDhcp(hVmDev, bConfigureWithDhcp));
	//ipv4 should be removed
	PRL_CHECK_NET_ADDRESS_LIST(hVmDev, PrlVmDevNet_GetNetAddresses, lstNetAddressesIPv6)
}

void PrlVmDevManipulationsTest::testNetIsConfigureWithDhcpIPv6()
{
	PREPARE_VM_DEV_NET_ADAPTER;

	HANDLE_TO_STRING(hVmDev);
	CVmGenericNetworkAdapter _adapter;
	_adapter.fromString(_str_object);

	PRL_BOOL bConfigureWithDhcpIPv6;
	CHECK_RET_CODE_EXP(PrlVmDevNet_IsConfigureWithDhcpIPv6(hVmDev, &bConfigureWithDhcpIPv6))
	QVERIFY(PRL_BOOL(_adapter.isConfigureWithDhcpIPv6()) == bConfigureWithDhcpIPv6);
}

void PrlVmDevManipulationsTest::testNetIsConfigureWithDhcpIPv6OnWrongParams()
{
	PREPARE_VM_DEV_NET_ADAPTER;

	PRL_BOOL bConfigureWithDhcpIPv6;
	// Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_IsConfigureWithDhcpIPv6(m_VmHandle, &bConfigureWithDhcpIPv6),
		PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_IsConfigureWithDhcpIPv6(hVmDev, 0),
		PRL_ERR_INVALID_ARG);
}

void PrlVmDevManipulationsTest::testNetSetConfigureWithDhcpIPv6()
{
	PREPARE_VM_DEV_NET_ADAPTER;

	PRL_BOOL bConfigureWithDhcp;
	CHECK_RET_CODE_EXP(PrlVmDevNet_IsConfigureWithDhcpIPv6(hVmDev, &bConfigureWithDhcp));
	bConfigureWithDhcp = ! bConfigureWithDhcp;

	CHECK_RET_CODE_EXP(PrlVmDevNet_SetConfigureWithDhcpIPv6(hVmDev, bConfigureWithDhcp));
	PRL_BOOL bNewConfigureWithDhcp;
	CHECK_RET_CODE_EXP(PrlVmDevNet_IsConfigureWithDhcpIPv6(hVmDev, &bNewConfigureWithDhcp));
	QVERIFY(bNewConfigureWithDhcp == bConfigureWithDhcp);

	//check clean NetAddress
	bConfigureWithDhcp = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetConfigureWithDhcpIPv6(hVmDev, bConfigureWithDhcp));

	QStringList lstNetAddressesIPv4 = QStringList()<<"192.168.1.1/255.255.255.0"<<"10.30.1.1/255.255.255.0";
	QStringList lstNetAddressesAll = QStringList()<<"aaaa:aa::1/3"<<"192.168.1.1/255.255.255.0"
				<<"c001::1/70"<<"10.30.1.1/255.255.255.0"<<"ee::aa:0:1/124";

	SdkHandleWrap hNewNetAddressesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hNewNetAddressesList.GetHandlePtr()))
	foreach(QString sNetAddress, lstNetAddressesAll)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewNetAddressesList, QSTR2UTF8(sNetAddress)))
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetNetAddresses(hVmDev, hNewNetAddressesList))
	PRL_CHECK_NET_ADDRESS_LIST(hVmDev, PrlVmDevNet_GetNetAddresses, lstNetAddressesAll)

	bConfigureWithDhcp = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetConfigureWithDhcpIPv6(hVmDev, bConfigureWithDhcp));
	//ipv6 should be removed
	PRL_CHECK_NET_ADDRESS_LIST(hVmDev, PrlVmDevNet_GetNetAddresses, lstNetAddressesIPv4)
}

void PrlVmDevManipulationsTest::testNetSetConfigureWithDhcpIPv6OnWrongParams()
{
	PREPARE_VM_DEV_NET_ADAPTER;

	// Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetConfigureWithDhcpIPv6(m_VmHandle, PRL_TRUE),
		PRL_ERR_INVALID_ARG);
}

void PrlVmDevManipulationsTest::testNetIsPktFilterPreventMacSpoof()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDev.GetHandlePtr()))
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmGenericNetworkAdapter _adapter;
	QVERIFY(StringToElement<CVmGenericNetworkAdapter*>(&_adapter, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	PRL_BOOL bPktFilterPreventMacSpoof;
	CHECK_RET_CODE_EXP(PrlVmDevNet_IsPktFilterPreventMacSpoof(hVmDev, &bPktFilterPreventMacSpoof))
	QCOMPARE(PRL_BOOL(_adapter.getPktFilter()->isPreventMacSpoof()), bPktFilterPreventMacSpoof);
}

void PrlVmDevManipulationsTest::testNetSetPktFilterPreventMacSpoof()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDev.GetHandlePtr()))
	PRL_BOOL bPktFilterPreventMacSpoof;
	CHECK_RET_CODE_EXP(PrlVmDevNet_IsPktFilterPreventMacSpoof(hVmDev, &bPktFilterPreventMacSpoof))
	bPktFilterPreventMacSpoof = !bPktFilterPreventMacSpoof;
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetPktFilterPreventMacSpoof(hVmDev, bPktFilterPreventMacSpoof))
	PRL_BOOL bNewPktFilterPreventMacSpoof;
	CHECK_RET_CODE_EXP(PrlVmDevNet_IsPktFilterPreventMacSpoof(hVmDev, &bNewPktFilterPreventMacSpoof))
	QCOMPARE(bNewPktFilterPreventMacSpoof, bPktFilterPreventMacSpoof);
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmGenericNetworkAdapter _adapter;
	QVERIFY(StringToElement<CVmGenericNetworkAdapter*>(&_adapter, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QCOMPARE(PRL_BOOL(_adapter.getPktFilter()->isPreventMacSpoof()), bPktFilterPreventMacSpoof);
}


void PrlVmDevManipulationsTest::testNetIsPktFilterPreventPromisc()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDev.GetHandlePtr()))
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmGenericNetworkAdapter _adapter;
	QVERIFY(StringToElement<CVmGenericNetworkAdapter*>(&_adapter, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	PRL_BOOL bPktFilterPreventPromisc;
	CHECK_RET_CODE_EXP(PrlVmDevNet_IsPktFilterPreventPromisc(hVmDev, &bPktFilterPreventPromisc))
	QCOMPARE(PRL_BOOL(_adapter.getPktFilter()->isPreventPromisc()), bPktFilterPreventPromisc);
}

void PrlVmDevManipulationsTest::testNetSetPktFilterPreventPromisc()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDev.GetHandlePtr()))
	PRL_BOOL bPktFilterPreventPromisc;
	CHECK_RET_CODE_EXP(PrlVmDevNet_IsPktFilterPreventPromisc(hVmDev, &bPktFilterPreventPromisc))
	bPktFilterPreventPromisc = !bPktFilterPreventPromisc;
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetPktFilterPreventPromisc(hVmDev, bPktFilterPreventPromisc))
	PRL_BOOL bNewPktFilterPreventPromisc;
	CHECK_RET_CODE_EXP(PrlVmDevNet_IsPktFilterPreventPromisc(hVmDev, &bNewPktFilterPreventPromisc))
	QCOMPARE(bNewPktFilterPreventPromisc, bPktFilterPreventPromisc);
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmGenericNetworkAdapter _adapter;
	QVERIFY(StringToElement<CVmGenericNetworkAdapter*>(&_adapter, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QCOMPARE(PRL_BOOL(_adapter.getPktFilter()->isPreventPromisc()), bPktFilterPreventPromisc);
}



void PrlVmDevManipulationsTest::testNetIsPktFilterPreventIpSpoof()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDev.GetHandlePtr()))
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmGenericNetworkAdapter _adapter;
	QVERIFY(StringToElement<CVmGenericNetworkAdapter*>(&_adapter, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	PRL_BOOL bPktFilterPreventIpSpoof;
	CHECK_RET_CODE_EXP(PrlVmDevNet_IsPktFilterPreventIpSpoof(hVmDev, &bPktFilterPreventIpSpoof))
	QCOMPARE(PRL_BOOL(_adapter.getPktFilter()->isPreventIpSpoof()), bPktFilterPreventIpSpoof);
}

void PrlVmDevManipulationsTest::testNetSetPktFilterPreventIpSpoof()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDev.GetHandlePtr()))
	PRL_BOOL bPktFilterPreventIpSpoof;
	CHECK_RET_CODE_EXP(PrlVmDevNet_IsPktFilterPreventIpSpoof(hVmDev, &bPktFilterPreventIpSpoof))
	bPktFilterPreventIpSpoof = !bPktFilterPreventIpSpoof;
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetPktFilterPreventIpSpoof(hVmDev, bPktFilterPreventIpSpoof))
	PRL_BOOL bNewPktFilterPreventIpSpoof;
	CHECK_RET_CODE_EXP(PrlVmDevNet_IsPktFilterPreventIpSpoof(hVmDev, &bNewPktFilterPreventIpSpoof))
	QCOMPARE(bNewPktFilterPreventIpSpoof, bPktFilterPreventIpSpoof);
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmGenericNetworkAdapter _adapter;
	QVERIFY(StringToElement<CVmGenericNetworkAdapter*>(&_adapter, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QCOMPARE(PRL_BOOL(_adapter.getPktFilter()->isPreventIpSpoof()), bPktFilterPreventIpSpoof);
}

void PrlVmDevManipulationsTest::testNetPktFilterOnWrongParams()
{
	PRL_BOOL bPktFilter = PRL_TRUE;
	//check set
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetPktFilterPreventMacSpoof(PRL_INVALID_HANDLE, bPktFilter), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetPktFilterPreventPromisc(PRL_INVALID_HANDLE, bPktFilter), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetPktFilterPreventIpSpoof(PRL_INVALID_HANDLE, bPktFilter), PRL_ERR_INVALID_ARG)

	//check get
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_IsPktFilterPreventMacSpoof(PRL_INVALID_HANDLE, &bPktFilter), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_IsPktFilterPreventPromisc(PRL_INVALID_HANDLE, &bPktFilter), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_IsPktFilterPreventIpSpoof(PRL_INVALID_HANDLE, &bPktFilter), PRL_ERR_INVALID_ARG)

	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDev.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_IsPktFilterPreventMacSpoof(hVmDev, NULL), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_IsPktFilterPreventPromisc(hVmDev, NULL), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_IsPktFilterPreventIpSpoof(hVmDev, NULL), PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testUsbGetAutoconnectOption()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_USB_DEVICE, hVmDev.GetHandlePtr()))
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmUsbDevice _usb;
	_usb.setAutoconnectDevices(_usb.getAutoconnectDevices() == PUD_CONNECT_TO_GUEST_OS ? PUD_ASK_USER_WHAT_TODO : PUD_CONNECT_TO_GUEST_OS);
	QVERIFY(StringToElement<CVmUsbDevice*>(&_usb, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	PRL_USB_DEVICE_AUTO_CONNECT_OPTION nAutoconnectOption;
	CHECK_RET_CODE_EXP(PrlVmDevUsb_GetAutoconnectOption(hVmDev, &nAutoconnectOption))
	QVERIFY(_usb.getAutoconnectDevices() == nAutoconnectOption);
}

void PrlVmDevManipulationsTest::testUsbSetAutoconnectOption()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_USB_DEVICE, hVmDev.GetHandlePtr()))
	PRL_USB_DEVICE_AUTO_CONNECT_OPTION nAutoconnectOption;
	CHECK_RET_CODE_EXP(PrlVmDevUsb_GetAutoconnectOption(hVmDev, &nAutoconnectOption));
	nAutoconnectOption = (nAutoconnectOption == PUD_CONNECT_TO_GUEST_OS ? PUD_ASK_USER_WHAT_TODO : PUD_CONNECT_TO_GUEST_OS);
	CHECK_RET_CODE_EXP(PrlVmDevUsb_SetAutoconnectOption(hVmDev, nAutoconnectOption))
	PRL_USB_DEVICE_AUTO_CONNECT_OPTION nNewAutoconnectOption;
	CHECK_RET_CODE_EXP(PrlVmDevUsb_GetAutoconnectOption(hVmDev, &nNewAutoconnectOption))
	QVERIFY(nNewAutoconnectOption == nAutoconnectOption);
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmUsbDevice _usb;
	QVERIFY(StringToElement<CVmUsbDevice*>(&_usb, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QVERIFY(_usb.getAutoconnectDevices() == nAutoconnectOption);
}

#define PREPARE_VM_DEV_SOUND\
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")\
	QString sExpectedOutputDev = "output device";\
	QString sExpectedMixerDev = "mixer device";\
	SdkHandleWrap hVmDev;\
	QVERIFY(PRL_SUCCEEDED(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SOUND_DEVICE, hVmDev.GetHandlePtr())));\
	CVmSoundDevice _sound;\
	_sound.getSoundOutputs()->m_lstSoundDevices += new CVmSoundDevice;\
	_sound.getSoundOutputs()->m_lstSoundDevices[0]->setUserFriendlyName(sExpectedOutputDev);\
	_sound.getSoundInputs()->m_lstSoundDevices += new CVmSoundDevice;\
	_sound.getSoundInputs()->m_lstSoundDevices[0]->setUserFriendlyName(sExpectedMixerDev);\
	QString sSoundXml = ElementToString<CVmSoundDevice*>(&_sound, XML_VM_CONFIG_EL_HARDWARE);\
	CHECK_RET_CODE_EXP(PrlVmDev_FromString(hVmDev, sSoundXml.toUtf8().data()))

void PrlVmDevManipulationsTest::testSoundGetOutputDev()
{
	PREPARE_VM_DEV_SOUND
	PRL_CHAR sActualOutputDev[STR_BUF_LENGTH];
	PRL_UINT32 nActualOutputDevLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDevSound_GetOutputDev(hVmDev, sActualOutputDev, &nActualOutputDevLength))
	QCOMPARE(UTF8_2QSTR(sActualOutputDev), sExpectedOutputDev);
}

void PrlVmDevManipulationsTest::testSoundGetOutputDevNotEnoughBufSize()
{
	PREPARE_VM_DEV_SOUND
	PRL_CHAR sActualOutputDev[1];
	PRL_UINT32 nActualOutputDevLength = 1;
	QVERIFY(PrlVmDevSound_GetOutputDev(hVmDev, sActualOutputDev, &nActualOutputDevLength) == PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmDevManipulationsTest::testSoundGetOutputDevNullBufSize()
{
	PREPARE_VM_DEV_SOUND
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hVmDev, PrlVmDevSound_GetOutputDev)
}

void PrlVmDevManipulationsTest::testSoundSetOutputDev()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	QString sExpectedOutputDev = "output device";
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SOUND_DEVICE, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDevSound_SetOutputDev(hVmDev, sExpectedOutputDev.toUtf8().data()))
	PRL_CHAR sActualOutputDev[STR_BUF_LENGTH];
	PRL_UINT32 nActualOutputDevLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDevSound_GetOutputDev(hVmDev, sActualOutputDev, &nActualOutputDevLength))
	QCOMPARE(UTF8_2QSTR(sActualOutputDev), sExpectedOutputDev);
}

void PrlVmDevManipulationsTest::testSoundSetOutputDevTryToSetEmptyStringValue()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	QString sOutputDev = "";
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SOUND_DEVICE, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDevSound_SetOutputDev(hVmDev, sOutputDev.toUtf8().data()))
	PRL_CHAR sActualOutputDev[STR_BUF_LENGTH];
	PRL_UINT32 nActualOutputDevLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDevSound_GetOutputDev(hVmDev, sActualOutputDev, &nActualOutputDevLength))
	QCOMPARE(UTF8_2QSTR(sActualOutputDev), sOutputDev);
}

void PrlVmDevManipulationsTest::testSoundGetMixerDev()
{
	PREPARE_VM_DEV_SOUND
	PRL_CHAR sActualMixerDev[STR_BUF_LENGTH];
	PRL_UINT32 nActualMixerDevLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDevSound_GetMixerDev(hVmDev, sActualMixerDev, &nActualMixerDevLength))
	QCOMPARE(UTF8_2QSTR(sActualMixerDev), sExpectedMixerDev);
}

void PrlVmDevManipulationsTest::testSoundGetMixerDevNotEnoughBufSize()
{
	PREPARE_VM_DEV_SOUND
	PRL_CHAR sActualMixerDev[1];
	PRL_UINT32 nActualMixerDevLength = 1;
	QVERIFY(PrlVmDevSound_GetMixerDev(hVmDev, sActualMixerDev, &nActualMixerDevLength) == PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmDevManipulationsTest::testSoundGetMixerDevNullBufSize()
{
	PREPARE_VM_DEV_SOUND
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hVmDev, PrlVmDevSound_GetMixerDev)
}

void PrlVmDevManipulationsTest::testSoundSetMixerDev()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	QString sExpectedMixerDev = "output device";
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SOUND_DEVICE, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDevSound_SetMixerDev(hVmDev, sExpectedMixerDev.toUtf8().data()))
	PRL_CHAR sActualMixerDev[STR_BUF_LENGTH];
	PRL_UINT32 nActualMixerDevLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDevSound_GetMixerDev(hVmDev, sActualMixerDev, &nActualMixerDevLength))
	QCOMPARE(UTF8_2QSTR(sActualMixerDev), sExpectedMixerDev);
}

void PrlVmDevManipulationsTest::testSoundSetMixerDevTryToSetEmptyStringValue()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	QString sMixerDev = "";
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SOUND_DEVICE, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDevSound_SetMixerDev(hVmDev, sMixerDev.toUtf8().data()))
	PRL_CHAR sActualMixerDev[STR_BUF_LENGTH];
	PRL_UINT32 nActualMixerDevLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDevSound_GetMixerDev(hVmDev, sActualMixerDev, &nActualMixerDevLength))
	QCOMPARE(UTF8_2QSTR(sActualMixerDev), sMixerDev);
}

#define CREATE_SERIAL_PORT_ELEM\
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")\
	SdkHandleWrap hVmDev;\
	QVERIFY(PRL_SUCCEEDED(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hVmDev.GetHandlePtr())));\
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hVmDev, PDT_USE_OTHER))

void PrlVmDevManipulationsTest::testSerialGetSocketMode()
{
	CREATE_SERIAL_PORT_ELEM
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmSerialPort _port;
	QVERIFY(StringToElement<CVmSerialPort*>(&_port, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	PRL_SERIAL_PORT_SOCKET_OPERATION_MODE nSocketMode;
	CHECK_RET_CODE_EXP(PrlVmDevSerial_GetSocketMode(hVmDev, &nSocketMode))
	QVERIFY(_port.getSocketMode() == nSocketMode);
}

void PrlVmDevManipulationsTest::testSerialSetSocketMode()
{
	CREATE_SERIAL_PORT_ELEM
	PRL_SERIAL_PORT_SOCKET_OPERATION_MODE nSocketMode;
	CHECK_RET_CODE_EXP(PrlVmDevSerial_GetSocketMode(hVmDev, &nSocketMode))
	nSocketMode = (nSocketMode == PSP_SERIAL_SOCKET_CLIENT ? PSP_SERIAL_SOCKET_SERVER : PSP_SERIAL_SOCKET_CLIENT);
	CHECK_RET_CODE_EXP(PrlVmDevSerial_SetSocketMode(hVmDev, nSocketMode))
	PRL_SERIAL_PORT_SOCKET_OPERATION_MODE nNewSocketMode;
	CHECK_RET_CODE_EXP(PrlVmDevSerial_GetSocketMode(hVmDev, &nNewSocketMode))
	QVERIFY(nNewSocketMode == nSocketMode);
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmSerialPort _port;
	QVERIFY(StringToElement<CVmSerialPort*>(&_port, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QVERIFY(_port.getSocketMode() == nSocketMode);
}

void PrlVmDevManipulationsTest::testVmDevsHandlesNonMadeInvalidOnVmFromString()
{
	testGetDeviceList();
	READ_VM_CONFIG_INTO_BUF("./CVmConfigurationTest_valid_vm_config.xml")
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _config.toUtf8().data()))
	for (int i = 0; i < m_VmDevHandles.size(); ++i)
	{
		SdkHandleWrap _dev_handle(m_VmDevHandles.value(i));
		PRL_BOOL bEnabled;
		CHECK_RET_CODE_EXP(PrlVmDev_IsEnabled(_dev_handle, &bEnabled))
	}
}

#define TEST_GET_DEVICES_LIST(sdk_dev_name, xml_model_dev_name)\
	INITIALIZE_VM_FROM_FILE("./CVmConfigurationTest_valid_vm_config.xml")\
	EXTRACT_VM_CONFIG\
	PRL_UINT32 n##sdk_dev_name##sCount = 0;\
	CHECK_RET_CODE_EXP(PrlVmCfg_Get##sdk_dev_name##sCount(m_VmHandle, &n##sdk_dev_name##sCount))\
	QCOMPARE(n##sdk_dev_name##sCount, PRL_UINT32(_vm_conf.getVmHardwareList()->m_lst##xml_model_dev_name##s.size()));

#define TEST_GET_VM_DEVICE(dev_name, handle_type)\
	INITIALIZE_VM_FROM_FILE("./CVmConfigurationTest_valid_vm_config.xml")\
	PRL_UINT32 n##dev_name##sCount = 0;\
	CHECK_RET_CODE_EXP(PrlVmCfg_Get##dev_name##sCount(m_VmHandle, &n##dev_name##sCount))\
	if (!n##dev_name##sCount)\
		QSKIP("Skipping test due no necessary test data", SkipAll);\
	SdkHandleWrap hVm##dev_name;\
	CHECK_RET_CODE_EXP(PrlVmCfg_Get##dev_name(m_VmHandle, 0, hVm##dev_name.GetHandlePtr()))\
	CHECK_HANDLE_TYPE(hVm##dev_name, handle_type)

#define TEST_DEVICE_HANDLES_VALIDITY_AFTER_REMOVE(dev_name, handle_type)\
	INITIALIZE_VM_FROM_FILE("./TestDspCmdDirValidateVmConfig_vm_config.xml")\
	SdkHandleWrap hVmDevice1_1, hVmDevice2_1;\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, handle_type, hVmDevice1_1.GetHandlePtr()))\
	if (handle_type != PDE_SOUND_DEVICE && handle_type != PDE_USB_DEVICE)\
	{\
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, handle_type, hVmDevice2_1.GetHandlePtr()))\
	}\
	SdkHandleWrap hVmDevice1_2, hVmDevice2_2;\
	CHECK_RET_CODE_EXP(PrlVmCfg_Get##dev_name(m_VmHandle, 0, hVmDevice1_2.GetHandlePtr()))\
	if (handle_type != PDE_SOUND_DEVICE && handle_type != PDE_USB_DEVICE)\
	{\
		CHECK_RET_CODE_EXP(PrlVmCfg_Get##dev_name(m_VmHandle, 1, hVmDevice2_2.GetHandlePtr()))\
	}\
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hVmDevice1_1))\
	PRL_BOOL bEnabled;\
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsEnabled(hVmDevice1_1, &bEnabled), PRL_ERR_OBJECT_WAS_REMOVED)\
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsEnabled(hVmDevice1_2, &bEnabled), PRL_ERR_OBJECT_WAS_REMOVED)\
	if (handle_type != PDE_SOUND_DEVICE && handle_type != PDE_USB_DEVICE)\
	{\
		CHECK_RET_CODE_EXP(PrlVmDev_IsEnabled(hVmDevice2_1, &bEnabled))\
		CHECK_RET_CODE_EXP(PrlVmDev_IsEnabled(hVmDevice2_2, &bEnabled))\
	}

void PrlVmDevManipulationsTest::testVmGetFloppyDisksCount()
{
	TEST_GET_DEVICES_LIST(FloppyDisk, FloppyDisk)
}

void PrlVmDevManipulationsTest::testVmGetFloppyDisk()
{
	TEST_GET_VM_DEVICE(FloppyDisk, PHT_VIRTUAL_DEV_FLOPPY)
}

void PrlVmDevManipulationsTest::testVmDevsHandlesMadeInvalidOnRemoveFloppyDisk()
{
	TEST_DEVICE_HANDLES_VALIDITY_AFTER_REMOVE(FloppyDisk, PDE_FLOPPY_DISK)
}

void PrlVmDevManipulationsTest::testVmGetHardDisksCount()
{
	TEST_GET_DEVICES_LIST(HardDisk, HardDisk)
}

void PrlVmDevManipulationsTest::testVmGetHardDisk()
{
	TEST_GET_VM_DEVICE(HardDisk, PHT_VIRTUAL_DEV_HARD_DISK)
}

void PrlVmDevManipulationsTest::testVmDevsHandlesMadeInvalidOnRemoveHardDisk()
{
	TEST_DEVICE_HANDLES_VALIDITY_AFTER_REMOVE(HardDisk, PDE_HARD_DISK)
}

void PrlVmDevManipulationsTest::testVmGetOpticalDisksCount()
{
	TEST_GET_DEVICES_LIST(OpticalDisk, OpticalDisk)
}

void PrlVmDevManipulationsTest::testVmGetOpticalDisk()
{
	TEST_GET_VM_DEVICE(OpticalDisk, PHT_VIRTUAL_DEV_OPTICAL_DISK)
}

void PrlVmDevManipulationsTest::testVmDevsHandlesMadeInvalidOnRemoveOpticalDisk()
{
	TEST_DEVICE_HANDLES_VALIDITY_AFTER_REMOVE(OpticalDisk, PDE_OPTICAL_DISK)
}

void PrlVmDevManipulationsTest::testVmGetParallelPortsCount()
{
	TEST_GET_DEVICES_LIST(ParallelPort, ParallelPort)
}

void PrlVmDevManipulationsTest::testVmGetParallelPort()
{
	TEST_GET_VM_DEVICE(ParallelPort, PHT_VIRTUAL_DEV_PARALLEL_PORT)
}

void PrlVmDevManipulationsTest::testVmDevsHandlesMadeInvalidOnRemoveParallelPort()
{
	TEST_DEVICE_HANDLES_VALIDITY_AFTER_REMOVE(ParallelPort, PDE_PARALLEL_PORT)
}

void PrlVmDevManipulationsTest::testVmGetSerialPortsCount()
{
	TEST_GET_DEVICES_LIST(SerialPort, SerialPort)
}

void PrlVmDevManipulationsTest::testVmGetSerialPort()
{
	TEST_GET_VM_DEVICE(SerialPort, PHT_VIRTUAL_DEV_SERIAL_PORT)
}

void PrlVmDevManipulationsTest::testVmDevsHandlesMadeInvalidOnRemoveSerialPort()
{
	TEST_DEVICE_HANDLES_VALIDITY_AFTER_REMOVE(SerialPort, PDE_SERIAL_PORT)
}

void PrlVmDevManipulationsTest::testVmGetSoundDevsCount()
{
	TEST_GET_DEVICES_LIST(SoundDev, SoundDevice)
}

void PrlVmDevManipulationsTest::testVmGetSoundDev()
{
	TEST_GET_VM_DEVICE(SoundDev, PHT_VIRTUAL_DEV_SOUND)
}

void PrlVmDevManipulationsTest::testVmDevsHandlesMadeInvalidOnRemoveSoundDev()
{
	TEST_DEVICE_HANDLES_VALIDITY_AFTER_REMOVE(SoundDev, PDE_SOUND_DEVICE)
}

void PrlVmDevManipulationsTest::testVmGetUsbDevicesCount()
{
	TEST_GET_DEVICES_LIST(UsbDevice, UsbDevice)
}

void PrlVmDevManipulationsTest::testVmGetUsbDevice()
{
	TEST_GET_VM_DEVICE(UsbDevice, PHT_VIRTUAL_DEV_USB_DEVICE)
}

void PrlVmDevManipulationsTest::testVmDevsHandlesMadeInvalidOnRemoveUsbDevice()
{
	TEST_DEVICE_HANDLES_VALIDITY_AFTER_REMOVE(UsbDevice, PDE_USB_DEVICE)
}

void PrlVmDevManipulationsTest::testVmGetNetAdaptersCount()
{
	TEST_GET_DEVICES_LIST(NetAdapter, NetworkAdapter)
}

void PrlVmDevManipulationsTest::testVmGetNetAdapter()
{
	TEST_GET_VM_DEVICE(NetAdapter, PHT_VIRTUAL_DEV_NET_ADAPTER)
}

void PrlVmDevManipulationsTest::testVmDevsHandlesMadeInvalidOnRemoveNetAdapter()
{
	TEST_DEVICE_HANDLES_VALIDITY_AFTER_REMOVE(NetAdapter, PDE_GENERIC_NETWORK_ADAPTER)
}

void PrlVmDevManipulationsTest::testNetGenerateMacAddress()
{
	CREATE_NET_ADAPTER_DEVICE
	CHECK_RET_CODE_EXP(PrlVmDevNet_GenerateMacAddr(hVmDev))
	PRL_CHAR sMacAddress1[STR_BUF_LENGTH];
	PRL_UINT32 nMacAddressLength1 = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetMacAddress(hVmDev, sMacAddress1, &nMacAddressLength1))
	QVERIFY(!UTF8_2QSTR(sMacAddress1).isEmpty());
	CHECK_RET_CODE_EXP(PrlVmDevNet_GenerateMacAddr(hVmDev))
	PRL_CHAR sMacAddress2[STR_BUF_LENGTH];
	PRL_UINT32 nMacAddressLength2 = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetMacAddress(hVmDev, sMacAddress2, &nMacAddressLength2))
	QVERIFY(!UTF8_2QSTR(sMacAddress2).isEmpty());
	QVERIFY(UTF8_2QSTR(sMacAddress1) != UTF8_2QSTR(sMacAddress2));
}

void PrlVmDevManipulationsTest::testNetDevCreateMacAddressAutoGenerated()
{
	CREATE_NET_ADAPTER_DEVICE
	PRL_CHAR sMacAddress[STR_BUF_LENGTH];
	PRL_UINT32 nMacAddressLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetMacAddress(hVmDev, sMacAddress, &nMacAddressLength))
	QVERIFY(!UTF8_2QSTR(sMacAddress).isEmpty());
}

void PrlVmDevManipulationsTest::testNetDevGetDublicateMacAddressNotChanged()
{
	CREATE_NET_ADAPTER_DEVICE
	PRL_CHAR sMacAddress[STR_BUF_LENGTH];
	PRL_UINT32 nMacAddressLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetMacAddress(hVmDev, sMacAddress, &nMacAddressLength))
	PRL_UINT32 nNetAdaptersCount;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetNetAdaptersCount(m_VmHandle, &nNetAdaptersCount))
	QVERIFY(nNetAdaptersCount != 0);
	SdkHandleWrap hVmDev2;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetNetAdapter(m_VmHandle, nNetAdaptersCount-1, hVmDev2.GetHandlePtr()))
	PRL_CHAR sMacAddress2[STR_BUF_LENGTH];
	PRL_UINT32 nMacAddressLength2 = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetMacAddress(hVmDev2, sMacAddress2, &nMacAddressLength2))
	QCOMPARE(UTF8_2QSTR(sMacAddress2), UTF8_2QSTR(sMacAddress));
}

void PrlVmDevManipulationsTest::testGetSysName()
{
	PREPARE_VM_DEV_CDROM
	PRL_CHAR sActualSysName[STR_BUF_LENGTH];
	PRL_UINT32 nActualSysNameLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDev_GetSysName(hVmDev, sActualSysName, &nActualSysNameLength))
	QCOMPARE(UTF8_2QSTR(sActualSysName), sExpectedSysName);
}

void PrlVmDevManipulationsTest::testSetSysName()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	QString sExpectedSysName = "system name";
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hVmDev, sExpectedSysName.toUtf8().data()))
	PRL_CHAR sActualSysName[STR_BUF_LENGTH];
	PRL_UINT32 nActualSysNameLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDev_GetSysName(hVmDev, sActualSysName, &nActualSysNameLength))
	QCOMPARE(UTF8_2QSTR(sActualSysName), sExpectedSysName);
}

void PrlVmDevManipulationsTest::testIsRemote()
{
	PREPARE_VM_DEV_CDROM
	PRL_BOOL bRemote;
	CHECK_RET_CODE_EXP(PrlVmDev_IsRemote(hVmDev, &bRemote))
	QVERIFY( (PRL_BOOL) _cdrom.isRemote() == bRemote);
}

void PrlVmDevManipulationsTest::testSetRemote()
{
	PREPARE_VM_DEV_CDROM
	PRL_BOOL bRemote = !_cdrom.isRemote();
	CHECK_RET_CODE_EXP(PrlVmDev_SetRemote(hVmDev, bRemote))
	PRL_BOOL bActualRemote;
	CHECK_RET_CODE_EXP(PrlVmDev_IsRemote(hVmDev, &bActualRemote))
	QVERIFY(bActualRemote == bRemote);
}

void PrlVmDevManipulationsTest::testGetFriendlyName()
{
	PREPARE_VM_DEV_CDROM
	PRL_CHAR sActualFriendlyName[STR_BUF_LENGTH];
	PRL_UINT32 nActualFriendlyNameLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDev_GetFriendlyName(hVmDev, sActualFriendlyName, &nActualFriendlyNameLength))
	QCOMPARE(UTF8_2QSTR(sActualFriendlyName), sExpectedImagePath);
}

void PrlVmDevManipulationsTest::testSetFriendlyName()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	QString sExpectedFriendlyName = "friendly name";
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_SetFriendlyName(hVmDev, sExpectedFriendlyName.toUtf8().data()))
	PRL_CHAR sActualFriendlyName[STR_BUF_LENGTH];
	PRL_UINT32 nActualFriendlyNameLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDev_GetFriendlyName(hVmDev, sActualFriendlyName, &nActualFriendlyNameLength))
	QCOMPARE(UTF8_2QSTR(sActualFriendlyName), sExpectedFriendlyName);
}

#define CHECK_MULTIPLE_DEVICES_CREATION(device_type)\
	SdkHandleWrap hVmDev1;\
	SdkHandleWrap hVmDev2;\
	SdkHandleWrap hVmDev3;\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, device_type, hVmDev1.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, device_type, hVmDev2.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, device_type, hVmDev3.GetHandlePtr()))\
	PRL_UINT32 nVmDevIndex1;\
	PRL_UINT32 nVmDevIndex2;\
	PRL_UINT32 nVmDevIndex3;\
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hVmDev1, &nVmDevIndex1))\
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hVmDev2, &nVmDevIndex2))\
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hVmDev3, &nVmDevIndex3))\
	QCOMPARE(nVmDevIndex1, quint32(0));\
	QCOMPARE(nVmDevIndex2, quint32(1));\
	QCOMPARE(nVmDevIndex3, quint32(2));

void PrlVmDevManipulationsTest::testCreateVmDevAutoIndexAssignedForHardDisks()
{
	CHECK_MULTIPLE_DEVICES_CREATION(PDE_HARD_DISK)
}

void PrlVmDevManipulationsTest::testCreateVmDevAutoIndexAssignedForOpticalDisks()
{
	CHECK_MULTIPLE_DEVICES_CREATION(PDE_OPTICAL_DISK)
}

void PrlVmDevManipulationsTest::testCreateVmDevAutoIndexAssignedForFloppyDisks()
{
	CHECK_MULTIPLE_DEVICES_CREATION(PDE_FLOPPY_DISK)
}

void PrlVmDevManipulationsTest::testCreateVmDevAutoIndexAssignedForSerialPorts()
{
	CHECK_MULTIPLE_DEVICES_CREATION(PDE_SERIAL_PORT)
}

void PrlVmDevManipulationsTest::testCreateVmDevAutoIndexAssignedForParallelPorts()
{
	CHECK_MULTIPLE_DEVICES_CREATION(PDE_PARALLEL_PORT)
}

void PrlVmDevManipulationsTest::testCreateVmDevAutoIndexAssignedForNetworkAdapters()
{
	CHECK_MULTIPLE_DEVICES_CREATION(PDE_GENERIC_NETWORK_ADAPTER)
}

#define CHECK_MULTIPLE_DEVICES_CREATION_ON_INTERRUPTED_RANGE(device_type)\
	SdkHandleWrap hVmDev1;\
	SdkHandleWrap hVmDev2;\
	SdkHandleWrap hVmDev3;\
	SdkHandleWrap hVmDev4;\
	SdkHandleWrap hVmDev5;\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, device_type, hVmDev1.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, device_type, hVmDev2.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, device_type, hVmDev3.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hVmDev2))\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, device_type, hVmDev4.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, device_type, hVmDev5.GetHandlePtr()))\
	PRL_UINT32 nVmDevIndex4;\
	PRL_UINT32 nVmDevIndex5;\
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hVmDev4, &nVmDevIndex4))\
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hVmDev5, &nVmDevIndex5))\
	QCOMPARE(nVmDevIndex4, quint32(1));\
	QCOMPARE(nVmDevIndex5, quint32(3));

void PrlVmDevManipulationsTest::testCreateVmDevAutoIndexAssignedOnInterruptedRangeForHardDisks()
{
	CHECK_MULTIPLE_DEVICES_CREATION_ON_INTERRUPTED_RANGE(PDE_HARD_DISK)
}

void PrlVmDevManipulationsTest::testCreateVmDevAutoIndexAssignedOnInterruptedRangeForOpticalDisks()
{
	CHECK_MULTIPLE_DEVICES_CREATION_ON_INTERRUPTED_RANGE(PDE_OPTICAL_DISK)
}

void PrlVmDevManipulationsTest::testCreateVmDevAutoIndexAssignedOnInterruptedRangeForFloppyDisks()
{
	CHECK_MULTIPLE_DEVICES_CREATION_ON_INTERRUPTED_RANGE(PDE_FLOPPY_DISK)
}

void PrlVmDevManipulationsTest::testCreateVmDevAutoIndexAssignedOnInterruptedRangeForSerialPorts()
{
	CHECK_MULTIPLE_DEVICES_CREATION_ON_INTERRUPTED_RANGE(PDE_SERIAL_PORT)
}

void PrlVmDevManipulationsTest::testCreateVmDevAutoIndexAssignedOnInterruptedRangeForParallelPorts()
{
	CHECK_MULTIPLE_DEVICES_CREATION_ON_INTERRUPTED_RANGE(PDE_PARALLEL_PORT)
}

void PrlVmDevManipulationsTest::testCreateVmDevAutoIndexAssignedOnInterruptedRangeForNetworkAdapters()
{
	CHECK_MULTIPLE_DEVICES_CREATION_ON_INTERRUPTED_RANGE(PDE_GENERIC_NETWORK_ADAPTER)
}

#define TEST_SET_DEFAULT_STACK_INDEX(interface_type)\
	SdkHandleWrap hVmDev1;\
	SdkHandleWrap hVmDev2;\
	SdkHandleWrap hVmDev3;\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hVmDev1.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hVmDev2.GetHandlePtr()))\
	if (interface_type == PMS_SCSI_DEVICE)\
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_SCSI_DEVICE, hVmDev3.GetHandlePtr()))\
	else\
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hVmDev3.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev1, interface_type))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev2, interface_type))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev3, interface_type))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetDefaultStackIndex(hVmDev1))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetDefaultStackIndex(hVmDev2))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetDefaultStackIndex(hVmDev3))\
	PRL_UINT32 nVmDevStackIndex1;\
	PRL_UINT32 nVmDevStackIndex2;\
	PRL_UINT32 nVmDevStackIndex3;\
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev1, &nVmDevStackIndex1))\
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev2, &nVmDevStackIndex2))\
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev3, &nVmDevStackIndex3))\
	QVERIFY(nVmDevStackIndex1 != nVmDevStackIndex2 && nVmDevStackIndex1 != nVmDevStackIndex3 &&\
			nVmDevStackIndex3 != nVmDevStackIndex2);\
	QVERIFY(nVmDevStackIndex1 <= 2);\
	QVERIFY(nVmDevStackIndex2 <= 2);\
	QVERIFY(nVmDevStackIndex3 <= 2);

void PrlVmDevManipulationsTest::testSetDefaultStackIndexForIdeInterface()
{
	TEST_SET_DEFAULT_STACK_INDEX(PMS_IDE_DEVICE)
}

void PrlVmDevManipulationsTest::testSetDefaultStackIndexForScsiInterface()
{
	TEST_SET_DEFAULT_STACK_INDEX(PMS_SCSI_DEVICE)
}

void PrlVmDevManipulationsTest::testSetDefaultStackIndexForSataInterface()
{
	TEST_SET_DEFAULT_STACK_INDEX(PMS_SATA_DEVICE)
}

#define TEST_SET_DEFAULT_STACK_INDEX_ON_INTERRUPTED_RANGE(interface_type)\
	SdkHandleWrap hVmDev1;\
	SdkHandleWrap hVmDev2;\
	SdkHandleWrap hVmDev3;\
	SdkHandleWrap hVmDev4;\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hVmDev1.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hVmDev2.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hVmDev3.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev1, interface_type))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev2, interface_type))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev3, interface_type))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetDefaultStackIndex(hVmDev1))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetDefaultStackIndex(hVmDev2))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetDefaultStackIndex(hVmDev3))\
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hVmDev2))\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hVmDev4.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev4, interface_type))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetDefaultStackIndex(hVmDev4))\
	PRL_UINT32 nVmDevStackIndex1;\
	PRL_UINT32 nVmDevStackIndex4;\
	PRL_UINT32 nVmDevStackIndex3;\
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev1, &nVmDevStackIndex1))\
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev4, &nVmDevStackIndex4))\
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev3, &nVmDevStackIndex3))\
	QVERIFY(nVmDevStackIndex1 != nVmDevStackIndex4 && nVmDevStackIndex1 != nVmDevStackIndex3 &&\
			nVmDevStackIndex3 != nVmDevStackIndex4);\
	QVERIFY(nVmDevStackIndex1 <= 2);\
	QVERIFY(nVmDevStackIndex4 <= 2);\
	QVERIFY(nVmDevStackIndex3 <= 2);

void PrlVmDevManipulationsTest::testSetDefaultStackIndexForIdeInterfaceOnInterruptedRange()
{
	TEST_SET_DEFAULT_STACK_INDEX_ON_INTERRUPTED_RANGE(PMS_IDE_DEVICE)
}

void PrlVmDevManipulationsTest::testSetDefaultStackIndexForScsiInterfaceOnInterruptedRange()
{
	TEST_SET_DEFAULT_STACK_INDEX_ON_INTERRUPTED_RANGE(PMS_SCSI_DEVICE)
}

void PrlVmDevManipulationsTest::testSetDefaultStackIndexForSataInterfaceOnInterruptedRange()
{
	TEST_SET_DEFAULT_STACK_INDEX_ON_INTERRUPTED_RANGE(PMS_SATA_DEVICE)
}

#define TEST_SET_DEFAULT_STACK_INDEX_ON_NO_FREE_SLOTS(interface_type, max_devices_num)\
	for (size_t i = 0; i < max_devices_num; ++i)\
	{\
		SdkHandleWrap hVmDev;\
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle,\
		(i%2 ? PDE_HARD_DISK : PDE_OPTICAL_DISK), hVmDev.GetHandlePtr()))\
		CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev, interface_type))\
		CHECK_RET_CODE_EXP(PrlVmDev_SetDefaultStackIndex(hVmDev))\
	}\
	SdkHandleWrap hVmDev;\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hVmDev.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev, interface_type))\
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetDefaultStackIndex(hVmDev), PRL_ERR_NO_MORE_FREE_INTERFACE_SLOTS)

void PrlVmDevManipulationsTest::testSetDefaultStackIndexForIdeInterfaceOnNoFreeSlots()
{
	TEST_SET_DEFAULT_STACK_INDEX_ON_NO_FREE_SLOTS(PMS_IDE_DEVICE, PRL_MAX_IDE_DEVICES_NUM)
}

void PrlVmDevManipulationsTest::testSetDefaultStackIndexForScsiInterfaceOnNoFreeSlots()
{
	TEST_SET_DEFAULT_STACK_INDEX_ON_NO_FREE_SLOTS(PMS_SCSI_DEVICE, PRL_MAX_SCSI_DEVICES_NUM -1 )
}

void PrlVmDevManipulationsTest::testSetDefaultStackIndexForSataInterfaceOnNoFreeSlots()
{
	TEST_SET_DEFAULT_STACK_INDEX_ON_NO_FREE_SLOTS(PMS_SATA_DEVICE, PRL_MAX_SATA_DEVICES_NUM)
}

void PrlVmDevManipulationsTest::testSetDefaultStackIndexForScsiInterfaceOnScsiController()
{
	for (size_t i = 0; i < 7; ++i)
	{
		SdkHandleWrap hVmDev;
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle,
		(i%2 ? PDE_HARD_DISK : PDE_OPTICAL_DISK), hVmDev.GetHandlePtr()))
		CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev, PMS_SCSI_DEVICE))
		CHECK_RET_CODE_EXP(PrlVmDev_SetDefaultStackIndex(hVmDev))
	}
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev, PMS_SCSI_DEVICE))
	CHECK_RET_CODE_EXP(PrlVmDev_SetDefaultStackIndex(hVmDev))
	PRL_UINT32 nStackIndex;
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex( hVmDev, &nStackIndex ) )
	QVERIFY(nStackIndex == 8);
}

#define TEST_SET_DEFAULT_STACK_INDEX_ON_NO_FREE_SLOTS_GENERIC_SCSI_PRESENTS(interface_type, max_devices_num)\
	PRL_VOID_PTR pBuffer;\
	CHECK_RET_CODE_EXP(PrlVm_ToString(m_VmHandle, &pBuffer))\
	CVmConfiguration _vm_conf(UTF8_2QSTR((const char *)pBuffer));\
	CHECK_RET_CODE_EXP(PrlBuffer_Free(pBuffer))\
	CVmGenericScsiDevice *pDevice = new CVmGenericScsiDevice;\
	pDevice->setInterfaceType(interface_type);\
	pDevice->setStackIndex(0);\
	_vm_conf.getVmHardwareList()->m_lstGenericScsiDevices.append(pDevice);\
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().constData()))\
	for (size_t i = 0; i < max_devices_num-1; ++i)\
	{\
		SdkHandleWrap hVmDev;\
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle,\
			(i%2 ? PDE_HARD_DISK : PDE_OPTICAL_DISK), hVmDev.GetHandlePtr()))\
		CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev, interface_type))\
		CHECK_RET_CODE_EXP(PrlVmDev_SetDefaultStackIndex(hVmDev))\
	}\
	SdkHandleWrap hVmDev;\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hVmDev.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev, interface_type))\
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetDefaultStackIndex(hVmDev), PRL_ERR_NO_MORE_FREE_INTERFACE_SLOTS)

void PrlVmDevManipulationsTest::testSetDefaultStackIndexForIdeInterfaceOnNoFreeSlotsGenericScsiPresents()
{
	TEST_SET_DEFAULT_STACK_INDEX_ON_NO_FREE_SLOTS_GENERIC_SCSI_PRESENTS(PMS_IDE_DEVICE, PRL_MAX_IDE_DEVICES_NUM)
}

void PrlVmDevManipulationsTest::testSetDefaultStackIndexForScsiInterfaceOnNoFreeSlotsGenericScsiPresents()
{
	TEST_SET_DEFAULT_STACK_INDEX_ON_NO_FREE_SLOTS_GENERIC_SCSI_PRESENTS(PMS_SCSI_DEVICE, PRL_MAX_SCSI_DEVICES_NUM-1)
}

void PrlVmDevManipulationsTest::testSetDefaultStackIndexForSataInterfaceOnNoFreeSlotsGenericScsiPresents()
{
	TEST_SET_DEFAULT_STACK_INDEX_ON_NO_FREE_SLOTS_GENERIC_SCSI_PRESENTS(PMS_SATA_DEVICE, PRL_MAX_SATA_DEVICES_NUM)
}

void PrlVmDevManipulationsTest::testSetDefaultStackIndexFailedOnNullVmDevHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetDefaultStackIndex(PRL_INVALID_HANDLE), PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testSetDefaultStackIndexFailedOnNonVmDevHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetDefaultStackIndex(m_VmHandle), PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testSetDefaultStackIndexFailedOnNonHardOrOpticalDiskVmDevHandle()
{
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hVmDev.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetDefaultStackIndex(hVmDev), PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testSetDefaultStackIndexFailedOnRemovedVmDevHandle()
{
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hVmDev))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetDefaultStackIndex(hVmDev), PRL_ERR_OBJECT_WAS_REMOVED)
}

void PrlVmDevManipulationsTest::testSetDefaultStackIndexOnVmDevHandleThatNotBoundToVm()
{
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmDev_Create(PDE_HARD_DISK, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE(PrlVmDev_SetDefaultStackIndex(hVmDev))
	PRL_UINT32 nVmDevStackIndex;
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev, &nVmDevStackIndex))
	QVERIFY(nVmDevStackIndex == 0);
}

#define CHECK_THAT_BOOT_DEVICE_IS_ABSENT(device_type, device_index, sequence_index)\
	{\
		PRL_UINT32 nBootDevsCount = 0;\
		CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDevCount(m_VmHandle, &nBootDevsCount))\
		for (PRL_UINT32 i = 0; i < nBootDevsCount; ++i)\
		{\
			PRL_DEVICE_TYPE nBootDevType;\
			PRL_UINT32 nBootDevIndex, nBootDevSequenceIndex;\
			SdkHandleWrap hBootDev;\
			CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDev(m_VmHandle, i, hBootDev.GetHandlePtr()))\
			CHECK_RET_CODE_EXP(PrlBootDev_GetIndex(hBootDev, &nBootDevIndex))\
			CHECK_RET_CODE_EXP(PrlBootDev_GetSequenceIndex(hBootDev, &nBootDevSequenceIndex))\
			CHECK_RET_CODE_EXP(PrlBootDev_GetType(hBootDev, &nBootDevType))\
			if (nBootDevType == device_type && nBootDevIndex == device_index && nBootDevSequenceIndex == sequence_index)\
				QFAIL(QString("Unexpected boot device presents with type %1 index %2 sequence index %3").arg(device_type).arg(device_index).arg(sequence_index).toUtf8().constData());\
		}\
	}

#define CHECK_THAT_BOOT_DEVICE_IS_PRESENTS(device_type, device_index, sequence_index)\
	{\
		try\
		{\
			PRL_UINT32 nBootDevsCount = 0;\
			CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDevCount(m_VmHandle, &nBootDevsCount))\
			for (PRL_UINT32 i = 0; i < nBootDevsCount; ++i)\
			{\
				PRL_DEVICE_TYPE nBootDevType;\
				PRL_UINT32 nBootDevIndex, nBootDevSequenceIndex;\
				SdkHandleWrap hBootDev;\
				CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDev(m_VmHandle, i, hBootDev.GetHandlePtr()))\
				CHECK_RET_CODE_EXP(PrlBootDev_GetIndex(hBootDev, &nBootDevIndex))\
				CHECK_RET_CODE_EXP(PrlBootDev_GetSequenceIndex(hBootDev, &nBootDevSequenceIndex))\
				CHECK_RET_CODE_EXP(PrlBootDev_GetType(hBootDev, &nBootDevType))\
				if (nBootDevType == device_type && nBootDevIndex == device_index && nBootDevSequenceIndex == sequence_index)\
					throw int(0);\
			}\
			QFAIL(QString("Specified boot device with type %1 index %2 sequence index %3 wasn't found").arg(device_type).arg(device_index).arg(sequence_index).toUtf8().constData());\
		} catch(int) {}\
	}

void PrlVmDevManipulationsTest::testRemoveDevAlsoRemovesCorrespondingBootDeviceOnFloppyDisk()
{
	PRL_UINT32 nDevIndex = 0;
	PRL_UINT32 nSequenceIndex = 0;
	SdkHandleWrap hFloppyDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hFloppyDisk.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_SetIndex(hFloppyDisk, nDevIndex))
	SdkHandleWrap hBootDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_FLOPPY_DISK))
	CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, nDevIndex))
	CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, nSequenceIndex))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hFloppyDisk))
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_FLOPPY_DISK, nDevIndex, nSequenceIndex)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlBootDev_GetIndex(hBootDevice, &nDevIndex), PRL_ERR_OBJECT_WAS_REMOVED)
}

void PrlVmDevManipulationsTest::testRemoveDevAlsoRemovesCorrespondingBootDeviceOnHardDisk()
{
	SdkHandleWrap hHardDisk1, hHardDisk2, hHardDisk3;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hHardDisk1.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hHardDisk2.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hHardDisk3.GetHandlePtr()))
	SdkHandleWrap hBootDevice1, hBootDevice2, hBootDevice3;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice1.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice2.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice3.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice1, PDE_HARD_DISK))
	CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice1, 0))
	CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice1, 0))
	CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice2, PDE_HARD_DISK))
	CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice2, 1))
	CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice2, 1))
	CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice3, PDE_HARD_DISK))
	CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice3, 2))
	CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice3, 2))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hHardDisk2))
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_HARD_DISK, 1, 1)
	CHECK_THAT_BOOT_DEVICE_IS_PRESENTS(PDE_HARD_DISK, 0, 0)
	CHECK_THAT_BOOT_DEVICE_IS_PRESENTS(PDE_HARD_DISK, 2, 2)
	PRL_UINT32 nDevIndex = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlBootDev_GetIndex(hBootDevice2, &nDevIndex), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hHardDisk1))
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_HARD_DISK, 1, 1)
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_HARD_DISK, 0, 0)
	CHECK_THAT_BOOT_DEVICE_IS_PRESENTS(PDE_HARD_DISK, 2, 2)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlBootDev_GetIndex(hBootDevice1, &nDevIndex), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hHardDisk3))
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_HARD_DISK, 1, 1)
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_HARD_DISK, 0, 0)
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_HARD_DISK, 2, 2)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlBootDev_GetIndex(hBootDevice3, &nDevIndex), PRL_ERR_OBJECT_WAS_REMOVED)
}

void PrlVmDevManipulationsTest::testRemoveDevAlsoRemovesCorrespondingBootDeviceOnOpticalDisk()
{
	SdkHandleWrap hOpticalDisk1, hOpticalDisk2, hOpticalDisk3;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hOpticalDisk1.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hOpticalDisk2.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hOpticalDisk3.GetHandlePtr()))
	SdkHandleWrap hBootDevice1, hBootDevice2, hBootDevice3;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice1.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice2.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice3.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice1, PDE_OPTICAL_DISK))
	CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice1, 0))
	CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice1, 0))
	CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice2, PDE_OPTICAL_DISK))
	CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice2, 1))
	CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice2, 1))
	CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice3, PDE_OPTICAL_DISK))
	CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice3, 2))
	CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice3, 2))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hOpticalDisk2))
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_OPTICAL_DISK, 1, 1)
	CHECK_THAT_BOOT_DEVICE_IS_PRESENTS(PDE_OPTICAL_DISK, 0, 0)
	CHECK_THAT_BOOT_DEVICE_IS_PRESENTS(PDE_OPTICAL_DISK, 2, 2)
	PRL_UINT32 nDevIndex = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlBootDev_GetIndex(hBootDevice2, &nDevIndex), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hOpticalDisk1))
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_OPTICAL_DISK, 1, 1)
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_OPTICAL_DISK, 0, 0)
	CHECK_THAT_BOOT_DEVICE_IS_PRESENTS(PDE_OPTICAL_DISK, 2, 2)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlBootDev_GetIndex(hBootDevice1, &nDevIndex), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hOpticalDisk3))
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_OPTICAL_DISK, 1, 1)
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_OPTICAL_DISK, 0, 0)
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_OPTICAL_DISK, 2, 2)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlBootDev_GetIndex(hBootDevice3, &nDevIndex), PRL_ERR_OBJECT_WAS_REMOVED)
}

void PrlVmDevManipulationsTest::testRemoveNetAdapterDevNotRemovesNetworkBootDevice()
{
	SdkHandleWrap hNetAdapter1, hNetAdapter2, hNetAdapter3;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hNetAdapter1.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hNetAdapter2.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hNetAdapter3.GetHandlePtr()))
	SdkHandleWrap hBootDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hBootDevice.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDevice, PDE_GENERIC_NETWORK_ADAPTER))
	CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDevice, 0))
	CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDevice, 0))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hNetAdapter2))
	CHECK_THAT_BOOT_DEVICE_IS_PRESENTS(PDE_GENERIC_NETWORK_ADAPTER, 0, 0)
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hNetAdapter1))
	CHECK_THAT_BOOT_DEVICE_IS_PRESENTS(PDE_GENERIC_NETWORK_ADAPTER, 0, 0)
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hNetAdapter3))
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_GENERIC_NETWORK_ADAPTER, 0, 0)
	PRL_UINT32 nDevIndex = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlBootDev_GetIndex(hBootDevice, &nDevIndex), PRL_ERR_OBJECT_WAS_REMOVED)
}

void PrlVmDevManipulationsTest::testRemoveDevOnMixedDevicesTypes()
{
	SdkHandleWrap hFloppyDisk, hHardDisk, hOpticalDisk, hNetAdapter, hNetAdapter2;
	SdkHandleWrap hFloppyDiskBootDev, hHardDiskBootDev, hOpticalDiskBootDev, hNetAdapterBootDev;
	PRL_UINT32 nDevIndex;

	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hFloppyDisk.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hHardDisk.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hOpticalDisk.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hNetAdapter.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hNetAdapter2.GetHandlePtr()))

	CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hFloppyDiskBootDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hHardDiskBootDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hOpticalDiskBootDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(m_VmHandle, hNetAdapterBootDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlBootDev_SetType(hFloppyDiskBootDev, PDE_FLOPPY_DISK))
	CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hFloppyDiskBootDev, 0))
	CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hFloppyDiskBootDev, 0))
	CHECK_RET_CODE_EXP(PrlBootDev_SetType(hHardDiskBootDev, PDE_HARD_DISK))
	CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hHardDiskBootDev, 0))
	CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hHardDiskBootDev, 1))
	CHECK_RET_CODE_EXP(PrlBootDev_SetType(hOpticalDiskBootDev, PDE_OPTICAL_DISK))
	CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hOpticalDiskBootDev, 0))
	CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hOpticalDiskBootDev, 2))
	CHECK_RET_CODE_EXP(PrlBootDev_SetType(hNetAdapterBootDev, PDE_GENERIC_NETWORK_ADAPTER))
	CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hNetAdapterBootDev, 0))
	CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hNetAdapterBootDev, 3))

	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hNetAdapter))
	CHECK_THAT_BOOT_DEVICE_IS_PRESENTS(PDE_FLOPPY_DISK, 0, 0)
	CHECK_THAT_BOOT_DEVICE_IS_PRESENTS(PDE_HARD_DISK, 0, 1)
	CHECK_THAT_BOOT_DEVICE_IS_PRESENTS(PDE_OPTICAL_DISK, 0, 2)
	CHECK_THAT_BOOT_DEVICE_IS_PRESENTS(PDE_GENERIC_NETWORK_ADAPTER, 0, 3)
	CHECK_RET_CODE_EXP(PrlBootDev_GetIndex(hNetAdapterBootDev, &nDevIndex))

	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hFloppyDisk))
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_FLOPPY_DISK, 0, 0)
	CHECK_THAT_BOOT_DEVICE_IS_PRESENTS(PDE_HARD_DISK, 0, 1)
	CHECK_THAT_BOOT_DEVICE_IS_PRESENTS(PDE_OPTICAL_DISK, 0, 2)
	CHECK_THAT_BOOT_DEVICE_IS_PRESENTS(PDE_GENERIC_NETWORK_ADAPTER, 0, 3)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlBootDev_GetIndex(hFloppyDiskBootDev, &nDevIndex), PRL_ERR_OBJECT_WAS_REMOVED)

	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hHardDisk))
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_FLOPPY_DISK, 0, 0)
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_HARD_DISK, 0, 1)
	CHECK_THAT_BOOT_DEVICE_IS_PRESENTS(PDE_OPTICAL_DISK, 0, 2)
	CHECK_THAT_BOOT_DEVICE_IS_PRESENTS(PDE_GENERIC_NETWORK_ADAPTER, 0, 3)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlBootDev_GetIndex(hHardDiskBootDev, &nDevIndex), PRL_ERR_OBJECT_WAS_REMOVED)

	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hOpticalDisk))
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_FLOPPY_DISK, 0, 0)
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_HARD_DISK, 0, 1)
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_OPTICAL_DISK, 0, 2)
	CHECK_THAT_BOOT_DEVICE_IS_PRESENTS(PDE_GENERIC_NETWORK_ADAPTER, 0, 3)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlBootDev_GetIndex(hOpticalDiskBootDev, &nDevIndex), PRL_ERR_OBJECT_WAS_REMOVED)

	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hNetAdapter2))
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_FLOPPY_DISK, 0, 0)
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_HARD_DISK, 0, 1)
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_OPTICAL_DISK, 0, 2)
	CHECK_THAT_BOOT_DEVICE_IS_ABSENT(PDE_GENERIC_NETWORK_ADAPTER, 0, 3)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlBootDev_GetIndex(hNetAdapterBootDev, &nDevIndex), PRL_ERR_OBJECT_WAS_REMOVED)
}

void PrlVmDevManipulationsTest::testGetDeviceTypeOnValid()
{
	SdkHandleWrap hDevice;
	for(unsigned int i = 0; i < sizeof(_devices_types)/sizeof(_devices_types[0]); i++)
	{
		hDevice.reset();
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, _devices_types[i], hDevice.GetHandlePtr()));

		PRL_DEVICE_TYPE nDeviceType = PDE_MAX;
		CHECK_RET_CODE_EXP(PrlVmDev_GetType(hDevice, &nDeviceType));
		QVERIFY(nDeviceType == _devices_types[i]);
	}
}

void PrlVmDevManipulationsTest::testGetDeviceTypeOnInvalidArguments()
{
	SdkHandleWrap hDevice;
	PRL_DEVICE_TYPE nDeviceType = PDE_MAX;

// Wrong handle
	PRL_RESULT nResult =  PrlVmDev_GetType(m_ServerHandle, &nDeviceType);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(nResult, PRL_ERR_INVALID_ARG);

// Invalid handle
	nResult =  PrlVmDev_GetType(hDevice, &nDeviceType);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(nResult, PRL_ERR_INVALID_ARG);

// Null result pointer
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));
	nResult =  PrlVmDev_GetType(hDevice, 0);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(nResult, PRL_ERR_INVALID_ARG);
}

struct DeviceTypeHandle
{
	PRL_DEVICE_TYPE type;
	PRL_HANDLE_TYPE	handle;
};

void PrlVmDevManipulationsTest::testCreateVmDeviceByType()
{
	DeviceTypeHandle deviceTypeHandles[]=
	{
		{PDE_HARD_DISK, PHT_VIRTUAL_DEV_HARD_DISK},
		{PDE_FLOPPY_DISK, PHT_VIRTUAL_DEV_FLOPPY},
		{PDE_OPTICAL_DISK, PHT_VIRTUAL_DEV_OPTICAL_DISK},
		{PDE_SERIAL_PORT, PHT_VIRTUAL_DEV_SERIAL_PORT},
		{PDE_PARALLEL_PORT, PHT_VIRTUAL_DEV_PARALLEL_PORT},
		{PDE_USB_DEVICE, PHT_VIRTUAL_DEV_USB_DEVICE},
		{PDE_GENERIC_NETWORK_ADAPTER, PHT_VIRTUAL_DEV_NET_ADAPTER},
		{PDE_SOUND_DEVICE, PHT_VIRTUAL_DEV_SOUND}
	};

	SdkHandleWrap hDevice;
	for(unsigned int i = 0; i < sizeof(deviceTypeHandles)/sizeof(deviceTypeHandles[0]); i++)
	{
		hDevice.reset();
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, deviceTypeHandles[i].type, hDevice.GetHandlePtr()));

		CHECK_HANDLE_TYPE(hDevice, deviceTypeHandles[i].handle)
	}
}

void PrlVmDevManipulationsTest::testCreateVmDeviceOnInvalidArguments()
{
	SdkHandleWrap hDevice;

// Invalid handle
	PRL_RESULT nResult = PrlVmCfg_CreateVmDev(m_ServerHandle, PDE_SERIAL_PORT, hDevice.GetHandlePtr());
	CHECK_CONCRETE_EXPRESSION_RET_CODE(nResult, PRL_ERR_INVALID_ARG);

// Null result pointer
	nResult = PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, 0);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(nResult, PRL_ERR_INVALID_ARG);

// Type is out of range
	hDevice.reset();
	nResult = PrlVmCfg_CreateVmDev(m_VmHandle, PDE_MAX, hDevice.GetHandlePtr());
	CHECK_CONCRETE_EXPRESSION_RET_CODE(nResult, PRL_ERR_INVALID_ARG);

// Invalid device type
	hDevice.reset();
	nResult = PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PRINTER, hDevice.GetHandlePtr());
	CHECK_CONCRETE_EXPRESSION_RET_CODE(nResult, PRL_ERR_INVALID_ARG);
}

void PrlVmDevManipulationsTest::testCheckValidRetCodesOnRemovedOrInvalidVmDeviceOnFloppy()
{
	SdkHandleWrap hRemovedFloppy, hVmDevOfNonValidType;

	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hRemovedFloppy.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVmDevOfNonValidType.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hRemovedFloppy))

	PRL_DEVICE_TYPE nDevType;
	PRL_BOOL bValue;
	PRL_VM_DEV_EMULATION_TYPE nEmulType;
	PRL_UINT32 nValue;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetType(hRemovedFloppy, &nDevType), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsConnected(hRemovedFloppy, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetConnected(hRemovedFloppy, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsEnabled(hRemovedFloppy, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetEnabled(hRemovedFloppy, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsRemote(hRemovedFloppy, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetRemote(hRemovedFloppy, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetEmulatedType(hRemovedFloppy, &nEmulType),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetEmulatedType(hRemovedFloppy, nEmulType),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetSysName(hRemovedFloppy, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetSysName(hRemovedFloppy, ""), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetFriendlyName(hRemovedFloppy, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetFriendlyName(hRemovedFloppy, ""), PRL_ERR_OBJECT_WAS_REMOVED)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetImagePath(hRemovedFloppy, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetImagePath(hVmDevOfNonValidType, 0, &nValue),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetImagePath(hRemovedFloppy, ""), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetImagePath(hVmDevOfNonValidType, ""), PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testCheckValidRetCodesOnRemovedOrInvalidVmDeviceOnHardDisk()
{
	SdkHandleWrap hRemovedHardDisk, hVmDevOfNonValidType;

	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hRemovedHardDisk.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVmDevOfNonValidType.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hRemovedHardDisk))

	PRL_DEVICE_TYPE nDevType;
	PRL_BOOL bValue;
	PRL_VM_DEV_EMULATION_TYPE nEmulType;
	PRL_UINT32 nValue;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetType(hRemovedHardDisk, &nDevType), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsConnected(hRemovedHardDisk, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetConnected(hRemovedHardDisk, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsEnabled(hRemovedHardDisk, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetEnabled(hRemovedHardDisk, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsRemote(hRemovedHardDisk, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetRemote(hRemovedHardDisk, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetEmulatedType(hRemovedHardDisk, &nEmulType),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetEmulatedType(hRemovedHardDisk, nEmulType),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetSysName(hRemovedHardDisk, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetSysName(hRemovedHardDisk, ""), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetFriendlyName(hRemovedHardDisk, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetFriendlyName(hRemovedHardDisk, ""), PRL_ERR_OBJECT_WAS_REMOVED)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetImagePath(hRemovedHardDisk, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetImagePath(hVmDevOfNonValidType, 0, &nValue),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetImagePath(hRemovedHardDisk, ""), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetImagePath(hVmDevOfNonValidType, ""), PRL_ERR_INVALID_ARG)

	PRL_MASS_STORAGE_INTERFACE_TYPE nIfaceType;
	PRL_HARD_DISK_INTERNAL_FORMAT nDiskType;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetIfaceType(hRemovedHardDisk, &nIfaceType),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetIfaceType(hVmDevOfNonValidType, &nIfaceType),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetIfaceType(hRemovedHardDisk, nIfaceType),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetIfaceType(hVmDevOfNonValidType, nIfaceType),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetStackIndex(hRemovedHardDisk, &nValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetStackIndex(hVmDevOfNonValidType, &nValue), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetStackIndex(hRemovedHardDisk, nValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetStackIndex(hVmDevOfNonValidType, nValue), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetDefaultStackIndex(hRemovedHardDisk), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetDefaultStackIndex(hVmDevOfNonValidType), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_GetDiskType(hRemovedHardDisk, &nDiskType),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_GetDiskType(hVmDevOfNonValidType, &nDiskType),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_SetDiskType(hRemovedHardDisk, nDiskType),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_SetDiskType(hVmDevOfNonValidType, nDiskType),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_IsSplitted(hRemovedHardDisk, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_IsSplitted(hVmDevOfNonValidType, &bValue), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_SetSplitted(hRemovedHardDisk, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_SetSplitted(hVmDevOfNonValidType, bValue), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_GetDiskSize(hRemovedHardDisk, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_GetDiskSize(hVmDevOfNonValidType, &nValue),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_SetDiskSize(hRemovedHardDisk, nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_SetDiskSize(hVmDevOfNonValidType, nValue),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_GetSizeOnDisk(hRemovedHardDisk, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_GetSizeOnDisk(hVmDevOfNonValidType, &nValue),\
										PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testCheckValidRetCodesOnRemovedOrInvalidVmDeviceOnOpticalDisk()
{
	SdkHandleWrap hRemovedOpticalDisk, hVmDevOfNonValidType;

	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hRemovedOpticalDisk.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVmDevOfNonValidType.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hRemovedOpticalDisk))

	PRL_DEVICE_TYPE nDevType;
	PRL_BOOL bValue;
	PRL_VM_DEV_EMULATION_TYPE nEmulType;
	PRL_UINT32 nValue;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetType(hRemovedOpticalDisk, &nDevType), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsConnected(hRemovedOpticalDisk, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetConnected(hRemovedOpticalDisk, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsEnabled(hRemovedOpticalDisk, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetEnabled(hRemovedOpticalDisk, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsRemote(hRemovedOpticalDisk, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetRemote(hRemovedOpticalDisk, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetEmulatedType(hRemovedOpticalDisk, &nEmulType),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetEmulatedType(hRemovedOpticalDisk, nEmulType),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetSysName(hRemovedOpticalDisk, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetSysName(hRemovedOpticalDisk, ""), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetFriendlyName(hRemovedOpticalDisk, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetFriendlyName(hRemovedOpticalDisk, ""), PRL_ERR_OBJECT_WAS_REMOVED)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetImagePath(hRemovedOpticalDisk, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetImagePath(hVmDevOfNonValidType, 0, &nValue),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetImagePath(hRemovedOpticalDisk, ""), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetImagePath(hVmDevOfNonValidType, ""), PRL_ERR_INVALID_ARG)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsPassthrough(hRemovedOpticalDisk, &bValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsPassthrough(hVmDevOfNonValidType, &bValue),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetPassthrough(hRemovedOpticalDisk, bValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetPassthrough(hVmDevOfNonValidType, bValue),\
										PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testCheckValidRetCodesOnRemovedOrInvalidVmDeviceOnParallelPort()
{
	SdkHandleWrap hRemovedParallelPort, hVmDevOfNonValidType;

	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hRemovedParallelPort.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVmDevOfNonValidType.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hRemovedParallelPort))

	PRL_DEVICE_TYPE nDevType;
	PRL_BOOL bValue;
	PRL_VM_DEV_EMULATION_TYPE nEmulType;
	PRL_UINT32 nValue;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetType(hRemovedParallelPort, &nDevType), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsConnected(hRemovedParallelPort, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetConnected(hRemovedParallelPort, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsEnabled(hRemovedParallelPort, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetEnabled(hRemovedParallelPort, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsRemote(hRemovedParallelPort, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetRemote(hRemovedParallelPort, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetEmulatedType(hRemovedParallelPort, &nEmulType),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetEmulatedType(hRemovedParallelPort, nEmulType),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetSysName(hRemovedParallelPort, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetSysName(hRemovedParallelPort, ""), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetFriendlyName(hRemovedParallelPort, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetFriendlyName(hRemovedParallelPort, ""), PRL_ERR_OBJECT_WAS_REMOVED)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetOutputFile(hRemovedParallelPort, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetOutputFile(hVmDevOfNonValidType, 0, &nValue),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetOutputFile(hRemovedParallelPort, ""), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetOutputFile(hVmDevOfNonValidType, ""), PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testCheckValidRetCodesOnRemovedOrInvalidVmDeviceOnSerialPort()
{
	SdkHandleWrap hRemovedSerialPort, hVmDevOfNonValidType;

	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hRemovedSerialPort.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVmDevOfNonValidType.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hRemovedSerialPort))

	PRL_DEVICE_TYPE nDevType;
	PRL_BOOL bValue;
	PRL_VM_DEV_EMULATION_TYPE nEmulType;
	PRL_UINT32 nValue;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetType(hRemovedSerialPort, &nDevType), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsConnected(hRemovedSerialPort, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetConnected(hRemovedSerialPort, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsEnabled(hRemovedSerialPort, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetEnabled(hRemovedSerialPort, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsRemote(hRemovedSerialPort, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetRemote(hRemovedSerialPort, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetEmulatedType(hRemovedSerialPort, &nEmulType),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetEmulatedType(hRemovedSerialPort, nEmulType),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetSysName(hRemovedSerialPort, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetSysName(hRemovedSerialPort, ""), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetFriendlyName(hRemovedSerialPort, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetFriendlyName(hRemovedSerialPort, ""), PRL_ERR_OBJECT_WAS_REMOVED)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetOutputFile(hRemovedSerialPort, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetOutputFile(hVmDevOfNonValidType, 0, &nValue),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetOutputFile(hRemovedSerialPort, ""), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetOutputFile(hVmDevOfNonValidType, ""), PRL_ERR_INVALID_ARG)

	PRL_SERIAL_PORT_SOCKET_OPERATION_MODE nSocketMode;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevSerial_GetSocketMode(hRemovedSerialPort, &nSocketMode),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevSerial_GetSocketMode(hVmDevOfNonValidType, &nSocketMode),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevSerial_SetSocketMode(hRemovedSerialPort, nSocketMode),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevSerial_SetSocketMode(hVmDevOfNonValidType, nSocketMode),\
										PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testCheckValidRetCodesOnRemovedOrInvalidVmDeviceOnNetAdapter()
{
	SdkHandleWrap hRemovedNetAdapter, hVmDevOfNonValidType;

	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hRemovedNetAdapter.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_USB_DEVICE, hVmDevOfNonValidType.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hRemovedNetAdapter))

	PRL_DEVICE_TYPE nDevType;
	PRL_BOOL bValue;
	PRL_VM_DEV_EMULATION_TYPE nEmulType;
	PRL_UINT32 nValue;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetType(hRemovedNetAdapter, &nDevType), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsConnected(hRemovedNetAdapter, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetConnected(hRemovedNetAdapter, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsEnabled(hRemovedNetAdapter, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetEnabled(hRemovedNetAdapter, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsRemote(hRemovedNetAdapter, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetRemote(hRemovedNetAdapter, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetEmulatedType(hRemovedNetAdapter, &nEmulType),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetEmulatedType(hRemovedNetAdapter, nEmulType),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetSysName(hRemovedNetAdapter, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetSysName(hRemovedNetAdapter, ""), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetFriendlyName(hRemovedNetAdapter, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetFriendlyName(hRemovedNetAdapter, ""), PRL_ERR_OBJECT_WAS_REMOVED)

	PRL_INT32 uiValue;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetBoundAdapterIndex(hRemovedNetAdapter, &uiValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetBoundAdapterIndex(hVmDevOfNonValidType, &uiValue),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetBoundAdapterIndex(hRemovedNetAdapter, uiValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetBoundAdapterIndex(hVmDevOfNonValidType, uiValue),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetBoundAdapterName(hRemovedNetAdapter, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetBoundAdapterName(hVmDevOfNonValidType, 0, &nValue),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetBoundAdapterName(hRemovedNetAdapter, ""),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetBoundAdapterName(hVmDevOfNonValidType, ""),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetMacAddress(hRemovedNetAdapter, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetMacAddress(hVmDevOfNonValidType, 0, &nValue),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetMacAddress(hRemovedNetAdapter, ""), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetMacAddress(hVmDevOfNonValidType, ""), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GenerateMacAddr(hRemovedNetAdapter), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GenerateMacAddr(hVmDevOfNonValidType), PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testCheckValidRetCodesOnRemovedOrInvalidVmDeviceOnSound()
{
	SdkHandleWrap hRemovedSound, hVmDevOfNonValidType;

	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SOUND_DEVICE, hRemovedSound.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_USB_DEVICE, hVmDevOfNonValidType.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hRemovedSound))

	PRL_DEVICE_TYPE nDevType;
	PRL_BOOL bValue;
	PRL_VM_DEV_EMULATION_TYPE nEmulType;
	PRL_UINT32 nValue;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetType(hRemovedSound, &nDevType), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsConnected(hRemovedSound, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetConnected(hRemovedSound, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsEnabled(hRemovedSound, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetEnabled(hRemovedSound, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsRemote(hRemovedSound, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetRemote(hRemovedSound, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetEmulatedType(hRemovedSound, &nEmulType),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetEmulatedType(hRemovedSound, nEmulType),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetSysName(hRemovedSound, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetSysName(hRemovedSound, ""), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetFriendlyName(hRemovedSound, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetFriendlyName(hRemovedSound, ""), PRL_ERR_OBJECT_WAS_REMOVED)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevSound_GetOutputDev(hRemovedSound, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevSound_GetOutputDev(hVmDevOfNonValidType, 0, &nValue),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevSound_SetOutputDev(hRemovedSound, ""), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevSound_SetOutputDev(hVmDevOfNonValidType, ""), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevSound_GetMixerDev(hRemovedSound, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevSound_GetMixerDev(hVmDevOfNonValidType, 0, &nValue),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevSound_SetMixerDev(hRemovedSound, ""), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevSound_SetMixerDev(hVmDevOfNonValidType, ""), PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testCheckValidRetCodesOnRemovedOrInvalidVmDeviceOnUsbController()
{
	SdkHandleWrap hRemovedUsbCtrl, hVmDevOfNonValidType;

	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_USB_DEVICE, hRemovedUsbCtrl.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SOUND_DEVICE, hVmDevOfNonValidType.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hRemovedUsbCtrl))

	PRL_DEVICE_TYPE nDevType;
	PRL_BOOL bValue;
	PRL_VM_DEV_EMULATION_TYPE nEmulType;
	PRL_UINT32 nValue;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetType(hRemovedUsbCtrl, &nDevType), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsConnected(hRemovedUsbCtrl, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetConnected(hRemovedUsbCtrl, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsEnabled(hRemovedUsbCtrl, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetEnabled(hRemovedUsbCtrl, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_IsRemote(hRemovedUsbCtrl, &bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetRemote(hRemovedUsbCtrl, bValue), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetEmulatedType(hRemovedUsbCtrl, &nEmulType),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetEmulatedType(hRemovedUsbCtrl, nEmulType),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetSysName(hRemovedUsbCtrl, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetSysName(hRemovedUsbCtrl, ""), PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_GetFriendlyName(hRemovedUsbCtrl, 0, &nValue),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetFriendlyName(hRemovedUsbCtrl, ""), PRL_ERR_OBJECT_WAS_REMOVED)

	PRL_USB_DEVICE_AUTO_CONNECT_OPTION nAutoOption;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevUsb_GetAutoconnectOption(hRemovedUsbCtrl, &nAutoOption),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevUsb_GetAutoconnectOption(hVmDevOfNonValidType, &nAutoOption),\
										PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevUsb_SetAutoconnectOption(hRemovedUsbCtrl, nAutoOption),\
										PRL_ERR_OBJECT_WAS_REMOVED)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevUsb_SetAutoconnectOption(hVmDevOfNonValidType, nAutoOption),\
										PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testHardDiskPartitionOnAdd()
{
	SdkHandleWrap hHardDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hHardDisk.GetHandlePtr()));

	SdkHandleWrap hPartition;
	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hPartition, PHT_VIRTUAL_DEV_HD_PARTITION);
}

void PrlVmDevManipulationsTest::testHardDiskPartitionOnAddWrongParams()
{
	SdkHandleWrap hHardDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hHardDisk.GetHandlePtr()));

	// Wrong handle
	SdkHandleWrap hPartition;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_AddPartition(m_VmHandle, hPartition.GetHandlePtr()),
										PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_AddPartition(hHardDisk, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmDevManipulationsTest::testHardDiskPartitionOnGetPartitionsCount()
{
	SdkHandleWrap hHardDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hHardDisk.GetHandlePtr()));

	PRL_UINT32 nCount = 5;
	CHECK_RET_CODE_EXP(PrlVmDevHd_GetPartitionsCount(hHardDisk, &nCount));
	QVERIFY(nCount == 0);

	SdkHandleWrap hPartition;
	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition.GetHandlePtr()));

	nCount = 0;
	CHECK_RET_CODE_EXP(PrlVmDevHd_GetPartitionsCount(hHardDisk, &nCount));
	QVERIFY(nCount == 2);

	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition.GetHandlePtr()));

	nCount = 0;
	CHECK_RET_CODE_EXP(PrlVmDevHd_GetPartitionsCount(hHardDisk, &nCount));
	QVERIFY(nCount == 3);
}

void PrlVmDevManipulationsTest::testHardDiskPartitionOnGetPartitionsCountWrongParams()
{
	SdkHandleWrap hHardDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hHardDisk.GetHandlePtr()));

	// Wrong handle
	PRL_UINT32 nCount = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_GetPartitionsCount(m_VmHandle, &nCount),
										PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_GetPartitionsCount(hHardDisk, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmDevManipulationsTest::testHardDiskPartitionOnGetPartition()
{
	SdkHandleWrap hHardDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hHardDisk.GetHandlePtr()));

	SdkHandleWrap hPartition;
	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDevHd_GetPartition(hHardDisk, 1, hPartition.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hPartition, PHT_VIRTUAL_DEV_HD_PARTITION);
}

void PrlVmDevManipulationsTest::testHardDiskPartitionOnGetPartitionWrongParams()
{
	SdkHandleWrap hHardDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hHardDisk.GetHandlePtr()));

	// Wrong handle
	SdkHandleWrap hPartition;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_GetPartition(m_VmHandle, 0, hPartition.GetHandlePtr()),
										PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_GetPartition(hHardDisk, 0, NULL), PRL_ERR_INVALID_ARG);
	// Index out of range
	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition.GetHandlePtr()));
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_GetPartition(hHardDisk, 1, hPartition.GetHandlePtr()),
										PRL_ERR_INVALID_ARG);
	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition.GetHandlePtr()));
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_GetPartition(hHardDisk, 4, hPartition.GetHandlePtr()),
										PRL_ERR_INVALID_ARG);
}

void PrlVmDevManipulationsTest::testHardDiskPartitionOnRemove()
{
	SdkHandleWrap hHardDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hHardDisk.GetHandlePtr()));

	SdkHandleWrap hPartition;
	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition.GetHandlePtr()));

	PRL_UINT32 nCount = 0;
	CHECK_RET_CODE_EXP(PrlVmDevHd_GetPartitionsCount(hHardDisk, &nCount));
	QVERIFY(nCount == 3);

	CHECK_RET_CODE_EXP(PrlVmDevHdPart_Remove(hPartition));

	CHECK_RET_CODE_EXP(PrlVmDevHd_GetPartitionsCount(hHardDisk, &nCount));
	QVERIFY(nCount == 2);
}

void PrlVmDevManipulationsTest::testHardDiskPartitionOnRemoveWrongParams()
{
	SdkHandleWrap hHardDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hHardDisk.GetHandlePtr()));

	SdkHandleWrap hPartition;
	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition.GetHandlePtr()));

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHdPart_Remove(m_VmHandle), PRL_ERR_INVALID_ARG);
	// Removed object
	CHECK_RET_CODE_EXP(PrlVmDevHdPart_Remove(hPartition));
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHdPart_Remove(hPartition),
										PRL_ERR_OBJECT_WAS_REMOVED);
}

void PrlVmDevManipulationsTest::testHardDiskPartitionOnGetSysName()
{
	SdkHandleWrap hHardDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hHardDisk.GetHandlePtr()));

	SdkHandleWrap hPartition;
	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition.GetHandlePtr()));

	QString qsSysName;
	PRL_EXTRACT_STRING_VALUE(qsSysName, hPartition, PrlVmDevHdPart_GetSysName);
}

void PrlVmDevManipulationsTest::testHardDiskPartitionOnGetSysNameWrongParams()
{
	SdkHandleWrap hHardDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hHardDisk.GetHandlePtr()));

	SdkHandleWrap hPartition;
	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition.GetHandlePtr()));

	PRL_CHAR buf[32];
	PRL_UINT32 nSize = sizeof(buf);
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHdPart_GetSysName(m_VmHandle, 0, &nSize),
										PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHdPart_GetSysName(hPartition, buf, NULL),
										PRL_ERR_INVALID_ARG);
	// Removed object
	CHECK_RET_CODE_EXP(PrlVmDevHdPart_Remove(hPartition));
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHdPart_GetSysName(hPartition, buf, &nSize),
										PRL_ERR_OBJECT_WAS_REMOVED);
}

void PrlVmDevManipulationsTest::testHardDiskPartitionOnSetSysName()
{
	SdkHandleWrap hHardDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hHardDisk.GetHandlePtr()));

	SdkHandleWrap hPartition;
	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition.GetHandlePtr()));

	PRL_CHAR buf[32] = "Partition system name";
	CHECK_RET_CODE_EXP(PrlVmDevHdPart_SetSysName(hPartition, buf));

	QString qsSysName;
	PRL_EXTRACT_STRING_VALUE(qsSysName, hPartition, PrlVmDevHdPart_GetSysName);

	QVERIFY(qsSysName == buf);
}

void PrlVmDevManipulationsTest::testHardDiskPartitionOnSetSysNameWrongParams()
{
	SdkHandleWrap hHardDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hHardDisk.GetHandlePtr()));

	SdkHandleWrap hPartition;
	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition.GetHandlePtr()));

	PRL_CHAR buf[32] = "Partition long system name";
	PRL_UINT32 nSize = 5;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHdPart_SetSysName(m_VmHandle, buf),
										PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHdPart_SetSysName(hPartition, NULL),
										PRL_ERR_INVALID_ARG);
	// Not enough buffer size
	CHECK_RET_CODE_EXP(PrlVmDevHdPart_SetSysName(hPartition, buf));
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHdPart_GetSysName(hPartition, buf, &nSize),
										PRL_ERR_BUFFER_OVERRUN);
	// Removed object
	CHECK_RET_CODE_EXP(PrlVmDevHdPart_Remove(hPartition));
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHdPart_SetSysName(hPartition, buf),
										PRL_ERR_OBJECT_WAS_REMOVED);
}

void PrlVmDevManipulationsTest::testHardDiskPartitionOnRemovedHardDisk()
{
	SdkHandleWrap hHardDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hHardDisk.GetHandlePtr()));

	SdkHandleWrap hPartition1;
	SdkHandleWrap hPartition2;
	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition1.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDevHd_AddPartition(hHardDisk, hPartition2.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hHardDisk));

	PRL_UINT32 nCount = 0;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_GetPartitionsCount(hHardDisk, &nCount),
										PRL_ERR_OBJECT_WAS_REMOVED);
	SdkHandleWrap hPartition;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_GetPartition(hHardDisk, 0, hPartition.GetHandlePtr()),
										PRL_ERR_OBJECT_WAS_REMOVED);

	PRL_UINT32 nSize = 0;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHdPart_Remove(hPartition1),
										PRL_ERR_OBJECT_WAS_REMOVED);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHdPart_GetSysName(hPartition1, 0, &nSize),
										PRL_ERR_OBJECT_WAS_REMOVED);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHdPart_SetSysName(hPartition1, "abc"),
										PRL_ERR_OBJECT_WAS_REMOVED);

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHdPart_Remove(hPartition2),
										PRL_ERR_OBJECT_WAS_REMOVED);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHdPart_GetSysName(hPartition2, 0, &nSize),
										PRL_ERR_OBJECT_WAS_REMOVED);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHdPart_SetSysName(hPartition2, "abc"),
										PRL_ERR_OBJECT_WAS_REMOVED);
}

#define TEST_AUTO_ASSIGN_STACK_INDEX_ON_IFACE_SETTING(interface_type)\
	SdkHandleWrap hVmDev1;\
	SdkHandleWrap hVmDev2;\
	SdkHandleWrap hVmDev3;\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hVmDev1.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hVmDev2.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hVmDev3.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev1, interface_type))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev2, interface_type))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev3, interface_type))\
	PRL_UINT32 nVmDevStackIndex1;\
	PRL_UINT32 nVmDevStackIndex2;\
	PRL_UINT32 nVmDevStackIndex3;\
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev1, &nVmDevStackIndex1))\
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev2, &nVmDevStackIndex2))\
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev3, &nVmDevStackIndex3))\
	QVERIFY(nVmDevStackIndex1 != nVmDevStackIndex2 && nVmDevStackIndex1 != nVmDevStackIndex3 &&\
			nVmDevStackIndex3 != nVmDevStackIndex2);\
	QVERIFY(nVmDevStackIndex1 <= 2);\
	QVERIFY(nVmDevStackIndex2 <= 2);\
	QVERIFY(nVmDevStackIndex3 <= 2);

void PrlVmDevManipulationsTest::testAutoAssignStackIndexOnIfaceSettingForIdeInterface()
{
	TEST_AUTO_ASSIGN_STACK_INDEX_ON_IFACE_SETTING(PMS_IDE_DEVICE)
}

void PrlVmDevManipulationsTest::testAutoAssignStackIndexOnIfaceSettingForScsiInterface()
{
	TEST_AUTO_ASSIGN_STACK_INDEX_ON_IFACE_SETTING(PMS_SCSI_DEVICE)
}

#define TEST_AUTO_ASSIGN_STACK_INDEX_ON_IFACE_SETTING_ON_INTERRUPTED_RANGE(interface_type)\
	SdkHandleWrap hVmDev1;\
	SdkHandleWrap hVmDev2;\
	SdkHandleWrap hVmDev3;\
	SdkHandleWrap hVmDev4;\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hVmDev1.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hVmDev2.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hVmDev3.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev1, interface_type))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev2, interface_type))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev3, interface_type))\
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hVmDev2))\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hVmDev4.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hVmDev4, interface_type))\
	PRL_UINT32 nVmDevStackIndex1;\
	PRL_UINT32 nVmDevStackIndex4;\
	PRL_UINT32 nVmDevStackIndex3;\
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev1, &nVmDevStackIndex1))\
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev4, &nVmDevStackIndex4))\
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev3, &nVmDevStackIndex3))\
	QVERIFY(nVmDevStackIndex1 != nVmDevStackIndex4 && nVmDevStackIndex1 != nVmDevStackIndex3 &&\
			nVmDevStackIndex3 != nVmDevStackIndex4);\
	QVERIFY(nVmDevStackIndex1 <= 2);\
	QVERIFY(nVmDevStackIndex4 <= 2);\
	QVERIFY(nVmDevStackIndex3 <= 2);

void PrlVmDevManipulationsTest::testAutoAssignStackIndexOnIfaceSettingForIdeInterfaceOnInterruptedRange()
{
	TEST_AUTO_ASSIGN_STACK_INDEX_ON_IFACE_SETTING_ON_INTERRUPTED_RANGE(PMS_IDE_DEVICE)
}

void PrlVmDevManipulationsTest::testAutoAssignStackIndexOnIfaceSettingForScsiInterfaceOnInterruptedRange()
{
	TEST_AUTO_ASSIGN_STACK_INDEX_ON_IFACE_SETTING_ON_INTERRUPTED_RANGE(PMS_SCSI_DEVICE)
}

void PrlVmDevManipulationsTest::testSetIfaceTypeOnIfaceSettingFailedOnNullVmDevHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetIfaceType(PRL_INVALID_HANDLE, PMS_SCSI_DEVICE), PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testSetIfaceTypeOnIfaceSettingFailedOnNonVmDevHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetIfaceType(m_VmHandle, PMS_IDE_DEVICE), PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testSetIfaceTypeOnIfaceSettingFailedOnNonHardOrOpticalDiskVmDevHandle()
{
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hVmDev.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetIfaceType(hVmDev, PMS_SCSI_DEVICE), PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testSetIfaceTypeOnIfaceSettingFailedOnRemovedVmDevHandle()
{
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hVmDev))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetIfaceType(hVmDev, PMS_IDE_DEVICE), PRL_ERR_OBJECT_WAS_REMOVED)
}

void PrlVmDevManipulationsTest::testSetIfaceTypeOnIfaceSettingOnVmDevHandleThatNotBoundToVm()
{
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmDev_Create(PDE_HARD_DISK, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE(PrlVmDev_SetIfaceType(hVmDev, PMS_IDE_DEVICE))
	PRL_UINT32 nVmDevStackIndex;
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev, &nVmDevStackIndex))
	QVERIFY(nVmDevStackIndex == 0);
}

#define CHECK_GENERAL_DEVICE_PROPS(handle_type, device_type)\
	{\
		CHECK_HANDLE_TYPE(hVmDev, handle_type)\
		PRL_DEVICE_TYPE nActualDeviceType = PDE_GENERIC_DEVICE;\
		CHECK_RET_CODE_EXP(PrlVmDev_GetType(hVmDev, &nActualDeviceType))\
		QCOMPARE(quint32(nActualDeviceType), quint32(device_type));\
		PRL_UINT32 nIndex = 1;\
		PRL_BOOL bEnabled = PRL_TRUE;\
		PRL_BOOL bConnected = PRL_FALSE;\
		PRL_VM_DEV_EMULATION_TYPE nEmulType = PDT_USE_REAL_DEVICE;\
		QString sSystemName = "device system name";\
		QString sFriendlyName = "device friendly name";\
		PRL_UINT32 nStackIndex = 2;\
		QString sDescription = "some device description";\
		\
		CHECK_RET_CODE_EXP(PrlVmDev_SetIndex(hVmDev, nIndex))\
		CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hVmDev, bEnabled))\
		CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hVmDev, bConnected))\
		CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hVmDev, nEmulType))\
		CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hVmDev, QSTR2UTF8(sSystemName)))\
		CHECK_RET_CODE_EXP(PrlVmDev_SetFriendlyName(hVmDev, QSTR2UTF8(sFriendlyName)))\
		CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hVmDev, nStackIndex))\
		CHECK_RET_CODE_EXP(PrlVmDev_SetDescription(hVmDev, QSTR2UTF8(sDescription)))\
		\
		PRL_UINT32 nActualIndex = 0, nActualStackIndex = 0;\
		PRL_BOOL bActualEnabled = PRL_FALSE, bActualConnected = PRL_FALSE;\
		PRL_VM_DEV_EMULATION_TYPE nActualEmulType = PDT_USE_IMAGE_FILE;\
		QString sActualSystemName, sActualFriendlyName, sActualDescription;\
		\
		CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hVmDev, &nActualIndex))\
		CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev, &nActualStackIndex))\
		CHECK_RET_CODE_EXP(PrlVmDev_IsEnabled(hVmDev, &bActualEnabled))\
		CHECK_RET_CODE_EXP(PrlVmDev_IsConnected(hVmDev, &bActualConnected))\
		CHECK_RET_CODE_EXP(PrlVmDev_GetEmulatedType(hVmDev, &nActualEmulType))\
		PRL_EXTRACT_STRING_VALUE(sActualSystemName, hVmDev, PrlVmDev_GetSysName)\
		PRL_EXTRACT_STRING_VALUE(sActualFriendlyName, hVmDev, PrlVmDev_GetFriendlyName)\
		PRL_EXTRACT_STRING_VALUE(sActualDescription, hVmDev, PrlVmDev_GetDescription)\
		\
		QCOMPARE(quint32(nActualIndex), quint32(nIndex));\
		QCOMPARE(quint32(nActualStackIndex), quint32(nStackIndex));\
		QCOMPARE(quint32(nActualEmulType), quint32(nEmulType));\
		QCOMPARE(quint32(bActualEnabled), quint32(bEnabled));\
		QCOMPARE(quint32(bActualConnected), quint32(bConnected));\
		QCOMPARE(sActualSystemName, sSystemName);\
		QCOMPARE(sActualFriendlyName, sFriendlyName);\
		QCOMPARE(sActualDescription, sDescription);\
	}

void PrlVmDevManipulationsTest::testCreateGenericPciDevice()
{
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmDev_Create(PDE_GENERIC_PCI_DEVICE, hVmDev.GetHandlePtr()))
	CHECK_GENERAL_DEVICE_PROPS(PHT_VIRTUAL_DEV_GENERIC_PCI, PDE_GENERIC_PCI_DEVICE)
}

void PrlVmDevManipulationsTest::testCreateGenericScsiDevice()
{
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmDev_Create(PDE_GENERIC_SCSI_DEVICE, hVmDev.GetHandlePtr()))
	CHECK_GENERAL_DEVICE_PROPS(PHT_VIRTUAL_DEV_GENERIC_SCSI, PDE_GENERIC_SCSI_DEVICE)
}

void PrlVmDevManipulationsTest::testSetDefaultStackIndexForGenericPci()
{
	SdkHandleWrap hVmDev1;
	SdkHandleWrap hVmDev2;
	SdkHandleWrap hVmDev3;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_PCI_DEVICE, hVmDev1.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_PCI_DEVICE, hVmDev2.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_PCI_DEVICE, hVmDev3.GetHandlePtr()))
	PRL_UINT32 nVmDevStackIndex1;
	PRL_UINT32 nVmDevStackIndex2;
	PRL_UINT32 nVmDevStackIndex3;
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev1, &nVmDevStackIndex1))
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev2, &nVmDevStackIndex2))
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev3, &nVmDevStackIndex3))
	QVERIFY(nVmDevStackIndex1 != nVmDevStackIndex2 && nVmDevStackIndex1 != nVmDevStackIndex3 &&
			nVmDevStackIndex3 != nVmDevStackIndex2);
	QVERIFY(nVmDevStackIndex1 <= 2);
	QVERIFY(nVmDevStackIndex2 <= 2);
	QVERIFY(nVmDevStackIndex3 <= 2);

	CHECK_RET_CODE_EXP(PrlVmDev_SetDefaultStackIndex(hVmDev1))
	CHECK_RET_CODE_EXP(PrlVmDev_SetDefaultStackIndex(hVmDev2))
	CHECK_RET_CODE_EXP(PrlVmDev_SetDefaultStackIndex(hVmDev3))
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev1, &nVmDevStackIndex1))
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev2, &nVmDevStackIndex2))
	CHECK_RET_CODE_EXP(PrlVmDev_GetStackIndex(hVmDev3, &nVmDevStackIndex3))
	QVERIFY(nVmDevStackIndex1 != nVmDevStackIndex2 && nVmDevStackIndex1 != nVmDevStackIndex3 &&
			nVmDevStackIndex3 != nVmDevStackIndex2);
	QVERIFY(nVmDevStackIndex1 <= 2);
	QVERIFY(nVmDevStackIndex2 <= 2);
	QVERIFY(nVmDevStackIndex3 <= 2);
}

void PrlVmDevManipulationsTest::testSetDefaultStackIndexForGenericPciNoMoreFreeSlots()
{
	for (size_t i = 0; i < PRL_MAX_GENERIC_PCI_DEVICES; ++i)
	{
		SdkHandleWrap hVmDev;
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_PCI_DEVICE, hVmDev.GetHandlePtr()))
	}
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_PCI_DEVICE, hVmDev.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetDefaultStackIndex(hVmDev), PRL_ERR_NO_MORE_FREE_INTERFACE_SLOTS)
}

void PrlVmDevManipulationsTest::testGetIfaceTypeOnGenericScsi()
{
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmDev_Create(PDE_GENERIC_SCSI_DEVICE, hVmDev.GetHandlePtr()))
	PRL_MASS_STORAGE_INTERFACE_TYPE nIfaceType = PMS_IDE_DEVICE;
	CHECK_RET_CODE_EXP(PrlVmDev_GetIfaceType(hVmDev, &nIfaceType))
	QCOMPARE(quint32(nIfaceType), quint32(PMS_SCSI_DEVICE));
}

void PrlVmDevManipulationsTest::testSetIfaceTypeOnGenericScsiWithAttemptToApplyIdeIface()
{
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmDev_Create(PDE_GENERIC_SCSI_DEVICE, hVmDev.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDev_SetIfaceType(hVmDev, PMS_IDE_DEVICE), PRL_ERR_INVALID_ARG)
}

void PrlVmDevManipulationsTest::testGetDevsListForGenericPci()
{
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_PCI_DEVICE, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_PCI_DEVICE, hVmDev.GetHandlePtr()))

	PRL_UINT32 nDevCount = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDevsCount(m_VmHandle, &nDevCount))
	m_pDevicesBuf = (PRL_HANDLE_PTR)malloc(sizeof(PRL_HANDLE)*nDevCount);
	QVERIFY(m_pDevicesBuf);
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDevsList(m_VmHandle, m_pDevicesBuf, &nDevCount))
	quint32 nActualDevicesCount = 0;
	for (PRL_UINT32 i = 0; i < nDevCount; i++)
	{
		m_VmDevHandles.append(SdkHandleWrap(m_pDevicesBuf[i]));
		PRL_HANDLE_TYPE nHandleType = PHT_ERROR;
		CHECK_RET_CODE_EXP(PrlHandle_GetType(m_pDevicesBuf[i], &nHandleType))
		if (PHT_VIRTUAL_DEV_GENERIC_PCI == nHandleType)
			nActualDevicesCount++;
	}

	QCOMPARE(nActualDevicesCount, quint32(2));
}

void PrlVmDevManipulationsTest::testGetDevsListForGenericScsi()
{
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_SCSI_DEVICE, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_SCSI_DEVICE, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_SCSI_DEVICE, hVmDev.GetHandlePtr()))

	PRL_UINT32 nDevCount = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDevsCount(m_VmHandle, &nDevCount))
	m_pDevicesBuf = (PRL_HANDLE_PTR)malloc(sizeof(PRL_HANDLE)*nDevCount);
	QVERIFY(m_pDevicesBuf);
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDevsList(m_VmHandle, m_pDevicesBuf, &nDevCount))
	quint32 nActualDevicesCount = 0;
	for (PRL_UINT32 i = 0; i < nDevCount; i++)
	{
		m_VmDevHandles.append(SdkHandleWrap(m_pDevicesBuf[i]));
		PRL_HANDLE_TYPE nHandleType = PHT_ERROR;
		CHECK_RET_CODE_EXP(PrlHandle_GetType(m_pDevicesBuf[i], &nHandleType))
		if (PHT_VIRTUAL_DEV_GENERIC_SCSI == nHandleType)
			nActualDevicesCount++;
	}

	QCOMPARE(nActualDevicesCount, quint32(3));
}

void PrlVmDevManipulationsTest::testGetDevByTypeForGenericPci()
{
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_PCI_DEVICE, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_PCI_DEVICE, hVmDev.GetHandlePtr()))

	PRL_UINT32 nDevCount = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDevsCountByType(m_VmHandle, PDE_GENERIC_PCI_DEVICE, &nDevCount))
	QCOMPARE(nDevCount, quint32(2));
	for (PRL_UINT32 i = 0; i < nDevCount; i++)
	{
		CHECK_RET_CODE_EXP(PrlVmCfg_GetDevByType(m_VmHandle, PDE_GENERIC_PCI_DEVICE, i, hVmDev.GetHandlePtr()))
		CHECK_HANDLE_TYPE(hVmDev, PHT_VIRTUAL_DEV_GENERIC_PCI)
	}
}

void PrlVmDevManipulationsTest::testGetDevByTypeForGenericScsi()
{
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_SCSI_DEVICE, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_SCSI_DEVICE, hVmDev.GetHandlePtr()))

	PRL_UINT32 nDevCount = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDevsCountByType(m_VmHandle, PDE_GENERIC_SCSI_DEVICE, &nDevCount))
	QCOMPARE(nDevCount, quint32(2));
	for (PRL_UINT32 i = 0; i < nDevCount; i++)
	{
		CHECK_RET_CODE_EXP(PrlVmCfg_GetDevByType(m_VmHandle, PDE_GENERIC_SCSI_DEVICE, i, hVmDev.GetHandlePtr()))
		CHECK_HANDLE_TYPE(hVmDev, PHT_VIRTUAL_DEV_GENERIC_SCSI)
	}
}

void PrlVmDevManipulationsTest::testGetDescription()
{
	PREPARE_VM_DEV_CDROM
	QString sActualDescription;
	PRL_EXTRACT_STRING_VALUE(sActualDescription, hVmDev, PrlVmDev_GetDescription)
	QCOMPARE(sActualDescription, sExpectedDescription);
}

void PrlVmDevManipulationsTest::testGetDescriptionNotEnoughBufSize()
{
	PREPARE_VM_DEV_CDROM
	PRL_CHAR sActualDescription[1];
	PRL_UINT32 nActualDescriptionLength = 1;
	QVERIFY(PrlVmDev_GetDescription(hVmDev, sActualDescription, &nActualDescriptionLength) == PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmDevManipulationsTest::testGetDescriptionNullBufSize()
{
	PREPARE_VM_DEV_CDROM
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hVmDev, PrlVmDev_GetDescription)
}

void PrlVmDevManipulationsTest::testSetDescription()
{
	INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")
	QString sExpectedDescription = "new device description";
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_SetDescription(hVmDev, sExpectedDescription.toUtf8().constData()))
	QString sActualDescription;
	PRL_EXTRACT_STRING_VALUE(sActualDescription, hVmDev, PrlVmDev_GetDescription)
	QCOMPARE(sActualDescription, sExpectedDescription);
}

#define TEST_GET_SET_DESCRIPTION(device_type, xml_object_type)\
	{\
		INITIALIZE_VM_FROM_FILE("TestDspCmdDirValidateVmConfig_vm_config.xml")\
		QString sExpectedDescription = "some device description";\
		SdkHandleWrap hVmDev;\
		QVERIFY(PRL_SUCCEEDED(PrlVmCfg_CreateVmDev(m_VmHandle, device_type, hVmDev.GetHandlePtr())));\
		xml_object_type _xml_dev;\
		_xml_dev.setDescription(sExpectedDescription);\
		QString sXml = _xml_dev.toString();\
		CHECK_RET_CODE_EXP(PrlHandle_FromString(hVmDev, QSTR2UTF8(sXml)))\
		QString sActualDescription;\
		PRL_EXTRACT_STRING_VALUE(sActualDescription, hVmDev, PrlVmDev_GetDescription)\
		QCOMPARE(sActualDescription, sExpectedDescription);\
		QString sNewDescription = "new device description";\
		CHECK_RET_CODE_EXP(PrlVmDev_SetDescription(hVmDev, QSTR2UTF8(sNewDescription)))\
		PRL_VOID_PTR pDevice = 0;\
		CHECK_RET_CODE_EXP(PrlHandle_ToString(hVmDev, &pDevice))\
		QVERIFY( PRL_SUCCEEDED(_xml_dev.fromString(UTF8_2QSTR((const char *)pDevice))) );\
		PrlBuffer_Free(pDevice);\
		QCOMPARE(_xml_dev.getDescription(), sNewDescription);\
	}

void PrlVmDevManipulationsTest::testSetGetDescriptionForAllDevices()
{
	TEST_GET_SET_DESCRIPTION(PDE_FLOPPY_DISK, CVmFloppyDisk)
	TEST_GET_SET_DESCRIPTION(PDE_OPTICAL_DISK, CVmOpticalDisk)
	TEST_GET_SET_DESCRIPTION(PDE_HARD_DISK, CVmHardDisk)
	TEST_GET_SET_DESCRIPTION(PDE_GENERIC_NETWORK_ADAPTER, CVmGenericNetworkAdapter)
	TEST_GET_SET_DESCRIPTION(PDE_SERIAL_PORT, CVmSerialPort)
	TEST_GET_SET_DESCRIPTION(PDE_PARALLEL_PORT, CVmParallelPort)
	TEST_GET_SET_DESCRIPTION(PDE_SOUND_DEVICE, CVmSoundDevice)
	TEST_GET_SET_DESCRIPTION(PDE_USB_DEVICE	, CVmUsbDevice)
	TEST_GET_SET_DESCRIPTION(PDE_GENERIC_PCI_DEVICE, CVmGenericPciDevice)
	TEST_GET_SET_DESCRIPTION(PDE_GENERIC_SCSI_DEVICE, CVmGenericScsiDevice)
	TEST_GET_SET_DESCRIPTION(PDE_PCI_VIDEO_ADAPTER, CVmPciVideoAdapter)
}

void PrlVmDevManipulationsTest::testCreateVmDeviceDisplay()
{
	CVmConfiguration vm_config;
	{
		HANDLE_TO_STRING(m_VmHandle);
		vm_config.fromString(_str_object);

		QVERIFY(vm_config.getVmHardwareList()->m_lstPciVideoAdapters.isEmpty());
	}

// 1 Add display device
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PCI_VIDEO_ADAPTER, hVmDev.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hVmDev, PHT_VIRTUAL_DEV_DISPLAY);
// 2 Add display device
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PCI_VIDEO_ADAPTER, hVmDev.GetHandlePtr()));
////
	{
		HANDLE_TO_STRING(m_VmHandle);
		vm_config.fromString(_str_object);

		QVERIFY(2 == vm_config.getVmHardwareList()->m_lstPciVideoAdapters.size());
	}
}

void PrlVmDevManipulationsTest::testRemoveDeviceOnDisplay()
{
	INITIALIZE_VM_FROM_FILE("./TestDspCmdDirValidateVmConfig_vm_config.xml")
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PCI_VIDEO_ADAPTER, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hVmDev))
	EXTRACT_VM_CONFIG
	QVERIFY(pHardware->m_lstPciVideoAdapters.size() == 0);
	CHECK_ON_TRY_USAGE_REMOVED_VM_DEV
}

void PrlVmDevManipulationsTest::testCreateVmDeviceDisplayOnCommonDeviceList()
{
	PRL_UINT32 nCount = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDevsCount(m_VmHandle, &nCount));
	QVERIFY(nCount == 0);

	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PCI_VIDEO_ADAPTER, hVmDev.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PCI_VIDEO_ADAPTER, hVmDev.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PCI_VIDEO_ADAPTER, hVmDev.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmCfg_GetDevsCount(m_VmHandle, &nCount));
	QVERIFY(nCount == 3);

	PRL_HANDLE m_pDevices[3] = {PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, PRL_INVALID_HANDLE};

	CHECK_RET_CODE_EXP(PrlVmCfg_GetDevsList(m_VmHandle, m_pDevices, &nCount));
	QVERIFY(nCount == 3);

	for(PRL_UINT32 i = 0; i < nCount; i++)
	{
		CHECK_HANDLE_TYPE(m_pDevices[i], PHT_VIRTUAL_DEV_DISPLAY);
	}
}

void PrlVmDevManipulationsTest::testGetDisplayDeviceByType()
{
	PRL_UINT32 nCount = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDevsCountByType(m_VmHandle, PDE_PCI_VIDEO_ADAPTER, &nCount));
	QVERIFY(nCount == 0);

	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PCI_VIDEO_ADAPTER, hVmDev.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PCI_VIDEO_ADAPTER, hVmDev.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PCI_VIDEO_ADAPTER, hVmDev.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmCfg_GetDevsCountByType(m_VmHandle, PDE_PCI_VIDEO_ADAPTER, &nCount));
	QVERIFY(nCount == 3);

	for(PRL_UINT32 i = 0; i < nCount; i++)
	{
		CHECK_RET_CODE_EXP(PrlVmCfg_GetDevByType(m_VmHandle, PDE_PCI_VIDEO_ADAPTER, i, hVmDev.GetHandlePtr()));
		CHECK_HANDLE_TYPE(hVmDev, PHT_VIRTUAL_DEV_DISPLAY);
	}
}

void PrlVmDevManipulationsTest::testGetDisplayDevices()
{
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PCI_VIDEO_ADAPTER, hVmDev.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PCI_VIDEO_ADAPTER, hVmDev.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PCI_VIDEO_ADAPTER, hVmDev.GetHandlePtr()));

	PRL_UINT32 nCount = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDisplayDevsCount(m_VmHandle, &nCount));
	QVERIFY(nCount == 3);

	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hVmDev));
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDisplayDevsCount(m_VmHandle, &nCount));
	QVERIFY(nCount == 2);

	for(PRL_UINT32 i = 0; i < nCount; i++)
	{
		CHECK_RET_CODE_EXP(PrlVmCfg_GetDisplayDev(m_VmHandle, i, hVmDev.GetHandlePtr()));
		CHECK_HANDLE_TYPE(hVmDev, PHT_VIRTUAL_DEV_DISPLAY);
	}
}

void PrlVmDevManipulationsTest::testGetDisplayDevicesOnWrongParams()
{
	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PCI_VIDEO_ADAPTER, hVmDev.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PCI_VIDEO_ADAPTER, hVmDev.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PCI_VIDEO_ADAPTER, hVmDev.GetHandlePtr()));

	PRL_UINT32 nCount = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetDisplayDevsCount(m_ServerHandle, &nCount),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetDisplayDev(m_ServerHandle, 0, hVmDev.GetHandlePtr()),
										PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetDisplayDevsCount(m_VmHandle, NULL),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetDisplayDev(m_VmHandle, 0, NULL),
										PRL_ERR_INVALID_ARG);
	// Out of range
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetDisplayDev(m_VmHandle, 3, hVmDev.GetHandlePtr()),
										PRL_ERR_INVALID_ARG);
}

void PrlVmDevManipulationsTest::testAddDefaultDeviceExOnIndexOrder()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

// 1. Add default devices

	SdkHandleWrap hDevice1;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx(
						m_VmHandle, PRL_INVALID_HANDLE, PDE_OPTICAL_DISK, hDevice1.GetHandlePtr()));
	SdkHandleWrap hDevice2;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx(
						m_VmHandle, PRL_INVALID_HANDLE, PDE_OPTICAL_DISK, hDevice2.GetHandlePtr()));
	SdkHandleWrap hDevice3;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx(
						m_VmHandle, PRL_INVALID_HANDLE, PDE_OPTICAL_DISK, hDevice3.GetHandlePtr()));
	SdkHandleWrap hDevice4;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx(
						m_VmHandle, PRL_INVALID_HANDLE, PDE_OPTICAL_DISK, hDevice4.GetHandlePtr()));
	SdkHandleWrap hDevice5;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx(
						m_VmHandle, PRL_INVALID_HANDLE, PDE_OPTICAL_DISK, hDevice5.GetHandlePtr()));

// 2. Delete devices

	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hDevice3));
	CHECK_RET_CODE_EXP(PrlVmDev_Remove(hDevice4));

// 3. Check index again added device

	SdkHandleWrap hNewDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx(
						m_VmHandle, PRL_INVALID_HANDLE, PDE_OPTICAL_DISK, hNewDevice.GetHandlePtr()));

	PRL_UINT32 nIndex = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_GetIndex(hNewDevice, &nIndex));
	QCOMPARE(quint32(nIndex), quint32(2));
}

void PrlVmDevManipulationsTest::testNetIsFirewallEnabled()
{
	PREPARE_VM_DEV_NET_ADAPTER;

	HANDLE_TO_STRING(hVmDev);
	CVmGenericNetworkAdapter _adapter;
	_adapter.fromString(_str_object);

	PRL_BOOL bFirewallEnabled;
	CHECK_RET_CODE_EXP(PrlVmDevNet_IsFirewallEnabled(hVmDev, &bFirewallEnabled));
	QVERIFY(PRL_BOOL(_adapter.getFirewall()->isEnabled()) == bFirewallEnabled);
}

void PrlVmDevManipulationsTest::testNetIsFirewallEnabledOnWrongParams()
{
	PREPARE_VM_DEV_NET_ADAPTER;

	PRL_BOOL bFirewallEnabled;
	// Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_IsFirewallEnabled(m_VmHandle, &bFirewallEnabled),
		PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_IsFirewallEnabled(hVmDev, 0),
		PRL_ERR_INVALID_ARG);
}

void PrlVmDevManipulationsTest::testNetSetFirewallEnabled()
{
	PREPARE_VM_DEV_NET_ADAPTER;

	PRL_BOOL bFirewallEnabled;
	CHECK_RET_CODE_EXP(PrlVmDevNet_IsFirewallEnabled(hVmDev, &bFirewallEnabled));
	bFirewallEnabled = ! bFirewallEnabled;

	CHECK_RET_CODE_EXP(PrlVmDevNet_SetFirewallEnabled(hVmDev, bFirewallEnabled));
	PRL_BOOL bNewFirewallEnabled;
	CHECK_RET_CODE_EXP(PrlVmDevNet_IsFirewallEnabled(hVmDev, &bNewFirewallEnabled));
	QVERIFY(bNewFirewallEnabled == bFirewallEnabled);
}

void PrlVmDevManipulationsTest::testNetSetFirewallEnabledOnWrongParams()
{
	PREPARE_VM_DEV_NET_ADAPTER;

	// Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetFirewallEnabled(m_VmHandle, PRL_TRUE),
		PRL_ERR_INVALID_ARG);
}

void PrlVmDevManipulationsTest::testNetGetFirewallDefaultPolicy()
{
	PREPARE_VM_DEV_NET_ADAPTER;

	HANDLE_TO_STRING(hVmDev);
	CVmGenericNetworkAdapter _adapter;
	_adapter.fromString(_str_object);

	PRL_FIREWALL_POLICY nFirewallDefaultPolicy;
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetFirewallDefaultPolicy(hVmDev, PFD_INCOMING, &nFirewallDefaultPolicy));
	QVERIFY(_adapter.getFirewall()->getIncoming()->getDirection()->getDefaultPolicy() == nFirewallDefaultPolicy);
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetFirewallDefaultPolicy(hVmDev, PFD_OUTGOING, &nFirewallDefaultPolicy));
	QVERIFY(_adapter.getFirewall()->getOutgoing()->getDirection()->getDefaultPolicy() == nFirewallDefaultPolicy);
}

void PrlVmDevManipulationsTest::testNetGetFirewallDefaultPolicyOnWrongParams()
{
	PREPARE_VM_DEV_NET_ADAPTER;

	PRL_FIREWALL_POLICY nFirewallDefaultPolicy;
	// Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetFirewallDefaultPolicy(m_VmHandle,
		PFD_INCOMING, &nFirewallDefaultPolicy), PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetFirewallDefaultPolicy(hVmDev, PFD_INCOMING, 0),
		PRL_ERR_INVALID_ARG);
}

void PrlVmDevManipulationsTest::testNetSetFirewallDefaultPolicy()
{
	PREPARE_VM_DEV_NET_ADAPTER;

	PRL_FIREWALL_POLICY nFirewallDefaultPolicy;
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetFirewallDefaultPolicy(hVmDev, PFD_INCOMING, &nFirewallDefaultPolicy));
	nFirewallDefaultPolicy = (PRL_FIREWALL_POLICY )(nFirewallDefaultPolicy ^ PFP_DENY);

	CHECK_RET_CODE_EXP(PrlVmDevNet_SetFirewallDefaultPolicy(hVmDev, PFD_INCOMING, nFirewallDefaultPolicy));
	PRL_FIREWALL_POLICY nNewFirewallDefaultPolicy;
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetFirewallDefaultPolicy(hVmDev, PFD_INCOMING, &nNewFirewallDefaultPolicy));
	QVERIFY(nNewFirewallDefaultPolicy == nFirewallDefaultPolicy);

	CHECK_RET_CODE_EXP(PrlVmDevNet_GetFirewallDefaultPolicy(hVmDev, PFD_OUTGOING, &nFirewallDefaultPolicy));
	nFirewallDefaultPolicy = (PRL_FIREWALL_POLICY )(nFirewallDefaultPolicy ^ PFP_DENY);

	CHECK_RET_CODE_EXP(PrlVmDevNet_SetFirewallDefaultPolicy(hVmDev, PFD_OUTGOING, nFirewallDefaultPolicy));
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetFirewallDefaultPolicy(hVmDev, PFD_OUTGOING, &nNewFirewallDefaultPolicy));
	QVERIFY(nNewFirewallDefaultPolicy == nFirewallDefaultPolicy);
}

void PrlVmDevManipulationsTest::testNetSetFirewallDefaultPolicyOnWrongParams()
{
	PREPARE_VM_DEV_NET_ADAPTER;

	// Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetFirewallDefaultPolicy(m_VmHandle, PFD_OUTGOING, PFP_ACCEPT),
		PRL_ERR_INVALID_ARG);
}

void PrlVmDevManipulationsTest::testCreateFirewallRuleEntry()
{
	SdkHandleWrap hRule;
	CHECK_RET_CODE_EXP(PrlFirewallRule_Create(hRule.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hRule, PHT_FIREWALL_RULE);
}

void PrlVmDevManipulationsTest::testCreateFirewallRuleEntryOnWrongParams()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_Create(NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmDevManipulationsTest::testFirewallDirectionRuleList(PRL_FIREWALL_DIRECTION nDirection)
{
	PREPARE_VM_DEV_NET_ADAPTER;

	SdkHandleWrap hRuleList;

// Empty list by default

	CHECK_RET_CODE_EXP(PrlVmDevNet_GetFirewallRuleList(hVmDev, nDirection, hRuleList.GetHandlePtr()));
	PRL_UINT32 nCount = 0;
	CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hRuleList, &nCount));
	QVERIFY( ! nCount );

	CHECK_RET_CODE_EXP(PrlVmDevNet_GetFirewallRuleList(hVmDev, nDirection, hRuleList.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hRuleList, &nCount));
	QVERIFY( ! nCount );

// Set not empty list

	for(int i = 0; i < 5; i++)
	{
		SdkHandleWrap hRule;
		CHECK_RET_CODE_EXP(PrlFirewallRule_Create(hRule.GetHandlePtr()));
		CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hRuleList, hRule));
	}
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetFirewallRuleList(hVmDev, nDirection, hRuleList));
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetFirewallRuleList(hVmDev, nDirection, hRuleList.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hRuleList, &nCount));
	QVERIFY( nCount == 5 );
	{
		HANDLE_TO_STRING(hVmDev);
		CVmGenericNetworkAdapter _adapter;
		_adapter.fromString(_str_object);

		CVmNetFirewallDirection *pDirection;
		if (nDirection == PFD_INCOMING)
			pDirection = _adapter.getFirewall()->getIncoming()->getDirection();
		else
			pDirection = _adapter.getFirewall()->getOutgoing()->getDirection();
		QVERIFY(pDirection->getFirewallRules()->m_lstFirewallRules.size() == 5);
	}

// Remove entries

	CHECK_RET_CODE_EXP(PrlHndlList_RemoveItem(hRuleList, 0));
	CHECK_RET_CODE_EXP(PrlHndlList_RemoveItem(hRuleList, 0));
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetFirewallRuleList(hVmDev, nDirection, hRuleList));
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetFirewallRuleList(hVmDev, nDirection, hRuleList.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hRuleList, &nCount));
	QVERIFY( nCount == 3 );
	{
		HANDLE_TO_STRING(hVmDev);
		CVmGenericNetworkAdapter _adapter;
		_adapter.fromString(_str_object);

		CVmNetFirewallDirection *pDirection;
		if (nDirection == PFD_INCOMING)
			pDirection = _adapter.getFirewall()->getIncoming()->getDirection();
		else
			pDirection = _adapter.getFirewall()->getOutgoing()->getDirection();
		QVERIFY(pDirection->getFirewallRules()->m_lstFirewallRules.size() == 3);
	}

// Invalid handles list as empty list (reset firewall rules)

	hRuleList.reset();
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetFirewallRuleList(hVmDev, nDirection, hRuleList));
	CHECK_RET_CODE_EXP(PrlVmDevNet_GetFirewallRuleList(hVmDev, nDirection, hRuleList.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hRuleList, &nCount));
	QVERIFY( ! nCount );
}

void PrlVmDevManipulationsTest::testFirewallRuleList()
{
	testFirewallDirectionRuleList(PFD_INCOMING);
	testFirewallDirectionRuleList(PFD_OUTGOING);
}

void PrlVmDevManipulationsTest::testFirewallRuleListOnWrongParams()
{
	PREPARE_VM_DEV_NET_ADAPTER;

	SdkHandleWrap hRuleList;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetFirewallRuleList(
					m_ServerHandle, PFD_OUTGOING, hRuleList.GetHandlePtr()),
								PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetFirewallRuleList(m_ServerHandle,
				PFD_OUTGOING, hRuleList), PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetFirewallRuleList(hVmDev,
				PFD_OUTGOING, m_ServerHandle), PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_GetFirewallRuleList(hVmDev,
				PFD_OUTGOING, NULL), PRL_ERR_INVALID_ARG);
	// Wrong handles list
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hRuleList.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hRuleList, m_VmHandle));
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hRuleList, hVmDev));
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevNet_SetFirewallRuleList(hVmDev,
				PFD_OUTGOING, hRuleList), PRL_ERR_INVALID_ARG);
}

#define FIREWALL_RULE_TO_XML_OBJECT \
	PRL_VOID_PTR pXml = 0; \
	CHECK_RET_CODE_EXP(PrlHandle_ToString(hRule, &pXml)); \
	CVmNetFirewallRule rule; \
	rule.fromString( UTF8_2QSTR((PRL_CONST_STR )pXml) ); \
	PrlBuffer_Free(pXml);

#define FIREWALL_RULE_FROM_XML_OBJECT \
	CHECK_RET_CODE_EXP(PrlHandle_FromString( hRule, QSTR2UTF8(rule.toString()) ));

void PrlVmDevManipulationsTest::testFirewallRuleLocalPort()
{
	SdkHandleWrap hRule;
	CHECK_RET_CODE_EXP(PrlFirewallRule_Create(hRule.GetHandlePtr()));

	FIREWALL_RULE_TO_XML_OBJECT;
	rule.setLocalPort(7);
	FIREWALL_RULE_FROM_XML_OBJECT;

	PRL_UINT32 nLocalPort = 10;
	CHECK_RET_CODE_EXP(PrlFirewallRule_GetLocalPort(hRule, &nLocalPort));
	QVERIFY(nLocalPort == 7);

	PRL_UINT32 nLocalPortExpected = 15;
	CHECK_RET_CODE_EXP(PrlFirewallRule_SetLocalPort(hRule, nLocalPortExpected));
	CHECK_RET_CODE_EXP(PrlFirewallRule_GetLocalPort(hRule, &nLocalPort));
	QCOMPARE(nLocalPort, nLocalPortExpected);
}

void PrlVmDevManipulationsTest::testFirewallRuleLocalPortOnWrongParams()
{
	SdkHandleWrap hRule;
	CHECK_RET_CODE_EXP(PrlFirewallRule_Create(hRule.GetHandlePtr()));

	PRL_UINT32 nLocalPort = 0;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_GetLocalPort(m_ServerHandle, &nLocalPort),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_SetLocalPort(m_ServerHandle, nLocalPort),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_GetLocalPort(hRule, NULL),
									PRL_ERR_INVALID_ARG);
}

void PrlVmDevManipulationsTest::testFirewallRuleRemotePort()
{
	SdkHandleWrap hRule;
	CHECK_RET_CODE_EXP(PrlFirewallRule_Create(hRule.GetHandlePtr()));

	FIREWALL_RULE_TO_XML_OBJECT;
	rule.setRemotePort(7);
	FIREWALL_RULE_FROM_XML_OBJECT;

	PRL_UINT32 nRemotePort = 10;
	CHECK_RET_CODE_EXP(PrlFirewallRule_GetRemotePort(hRule, &nRemotePort));
	QVERIFY(nRemotePort == 7);

	PRL_UINT32 nRemotePortExpected = 15;
	CHECK_RET_CODE_EXP(PrlFirewallRule_SetRemotePort(hRule, nRemotePortExpected));
	CHECK_RET_CODE_EXP(PrlFirewallRule_GetRemotePort(hRule, &nRemotePort));
	QCOMPARE(nRemotePort, nRemotePortExpected);
}

void PrlVmDevManipulationsTest::testFirewallRuleRemotePortOnWrongParams()
{
	SdkHandleWrap hRule;
	CHECK_RET_CODE_EXP(PrlFirewallRule_Create(hRule.GetHandlePtr()));

	PRL_UINT32 nRemotePort = 0;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_GetRemotePort(m_ServerHandle, &nRemotePort),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_SetRemotePort(m_ServerHandle, nRemotePort),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_GetRemotePort(hRule, NULL),
									PRL_ERR_INVALID_ARG);
}

void PrlVmDevManipulationsTest::testFirewallRuleProtocol()
{
	SdkHandleWrap hRule;
	CHECK_RET_CODE_EXP(PrlFirewallRule_Create(hRule.GetHandlePtr()));

	FIREWALL_RULE_TO_XML_OBJECT;
	rule.setProtocol("TCP");
	FIREWALL_RULE_FROM_XML_OBJECT;

	QString qsProtocol = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsProtocol, hRule, PrlFirewallRule_GetProtocol);
	QVERIFY(qsProtocol == "TCP");

	QString qsProtocolExpected = "UDP";
	CHECK_RET_CODE_EXP(PrlFirewallRule_SetProtocol(hRule, QSTR2UTF8(qsProtocolExpected)));
	PRL_EXTRACT_STRING_VALUE(qsProtocol, hRule, PrlFirewallRule_GetProtocol);

	QCOMPARE(qsProtocol, qsProtocolExpected);
}

void PrlVmDevManipulationsTest::testFirewallRuleProtocolOnWrongParams()
{
	SdkHandleWrap hRule;
	CHECK_RET_CODE_EXP(PrlFirewallRule_Create(hRule.GetHandlePtr()));

	QString qsStr = "ISAKMP";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_GetProtocol(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_SetProtocol(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_GetProtocol(hRule, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_SetProtocol(hRule, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlFirewallRule_SetProtocol(hRule, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_GetProtocol(hRule, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmDevManipulationsTest::testFirewallRuleLocalNetAddress()
{
	SdkHandleWrap hRule;
	CHECK_RET_CODE_EXP(PrlFirewallRule_Create(hRule.GetHandlePtr()));

	FIREWALL_RULE_TO_XML_OBJECT;
	rule.setLocalNetAddress("90.80.70.60");
	FIREWALL_RULE_FROM_XML_OBJECT;

	QString qsLocalNetAddress = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsLocalNetAddress, hRule, PrlFirewallRule_GetLocalNetAddress);
	QVERIFY(qsLocalNetAddress == "90.80.70.60");

	QString qsLocalNetAddressExpected = "192.168.2.3";
	CHECK_RET_CODE_EXP(PrlFirewallRule_SetLocalNetAddress(hRule, QSTR2UTF8(qsLocalNetAddressExpected)));
	PRL_EXTRACT_STRING_VALUE(qsLocalNetAddress, hRule, PrlFirewallRule_GetLocalNetAddress);

	QCOMPARE(qsLocalNetAddress, qsLocalNetAddressExpected);
}

void PrlVmDevManipulationsTest::testFirewallRuleLocalNetAddressOnWrongParams()
{
	SdkHandleWrap hRule;
	CHECK_RET_CODE_EXP(PrlFirewallRule_Create(hRule.GetHandlePtr()));

	QString qsStr = "192.168.2.3";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_GetLocalNetAddress(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_SetLocalNetAddress(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_GetLocalNetAddress(hRule, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_SetLocalNetAddress(hRule, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlFirewallRule_SetLocalNetAddress(hRule, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_GetLocalNetAddress(hRule, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmDevManipulationsTest::testFirewallRuleRemoteNetAddress()
{
	SdkHandleWrap hRule;
	CHECK_RET_CODE_EXP(PrlFirewallRule_Create(hRule.GetHandlePtr()));

	FIREWALL_RULE_TO_XML_OBJECT;
	rule.setRemoteNetAddress("90.80.70.60");
	FIREWALL_RULE_FROM_XML_OBJECT;

	QString qsRemoteNetAddress = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsRemoteNetAddress, hRule, PrlFirewallRule_GetRemoteNetAddress);
	QVERIFY(qsRemoteNetAddress == "90.80.70.60");

	QString qsRemoteNetAddressExpected = "192.168.2.3";
	CHECK_RET_CODE_EXP(PrlFirewallRule_SetRemoteNetAddress(hRule, QSTR2UTF8(qsRemoteNetAddressExpected)));
	PRL_EXTRACT_STRING_VALUE(qsRemoteNetAddress, hRule, PrlFirewallRule_GetRemoteNetAddress);

	QCOMPARE(qsRemoteNetAddress, qsRemoteNetAddressExpected);
}

void PrlVmDevManipulationsTest::testFirewallRuleRemoteNetAddressOnWrongParams()
{
	SdkHandleWrap hRule;
	CHECK_RET_CODE_EXP(PrlFirewallRule_Create(hRule.GetHandlePtr()));

	QString qsStr = "192.168.2.3";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_GetRemoteNetAddress(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_SetRemoteNetAddress(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_GetRemoteNetAddress(hRule, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_SetRemoteNetAddress(hRule, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlFirewallRule_SetRemoteNetAddress(hRule, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlFirewallRule_GetRemoteNetAddress(hRule, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmDevManipulationsTest::testCopyImage()
{
	INITIALIZE_VM_FROM_FILE("./TestDspCmdDirValidateVmConfig_vm_config.xml")

	SdkHandleWrap hJob(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);

// Add hard disk

	hJob.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlVmCfg_SetOsVersion(m_VmHandle, PVS_GUEST_VER_WIN_WINDOWS7));

	QString qsVmDir;
	PRL_EXTRACT_STRING_VALUE(qsVmDir, m_VmHandle, PrlVmCfg_GetHomePath);
	qsVmDir = QFileInfo(qsVmDir).path();

	SdkHandleWrap hHdd;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hHdd.GetHandlePtr()));

// Create hard disk image

	QString qsHddDir = qsVmDir + "/vm_hdd.hdd";

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hHdd, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hHdd, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hHdd, QSTR2UTF8(qsHddDir)));
	CHECK_RET_CODE_EXP(PrlVmDev_SetFriendlyName(hHdd, QSTR2UTF8(qsHddDir)));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hHdd, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hHdd, 0));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hHdd, PDT_USE_IMAGE_FILE));
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskSize(hHdd, 1024 * 1024));
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskType(hHdd, PHD_EXPANDING_HARD_DISK));

	hJob.reset(PrlVmDev_CreateImage(hHdd, PRL_TRUE, PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);

	hJob.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);
////
	m_qsTestPath = ParallelsDirs::getSystemTempDir();
	hJob.reset(PrlVmDev_CopyImage(hHdd, "copied_vm_hdd", QSTR2UTF8(m_qsTestPath), 0));
	CHECK_JOB_RET_CODE(hJob);

	m_qsTestPath += QString("/%1.hdd").arg("copied_vm_hdd");

	quint64 nOrigSize;
	CHECK_RET_CODE_EXP(CSimpleFileHelper::GetDirSize( qsHddDir, &nOrigSize ));

	quint64 nCopiedSize;
	CHECK_RET_CODE_EXP(CSimpleFileHelper::GetDirSize( m_qsTestPath, &nCopiedSize ));

	QCOMPARE(nCopiedSize, nOrigSize);
}

void PrlVmDevManipulationsTest::testCopyImageOnWrongParams()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hHdd;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hHdd.GetHandlePtr()));

	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlVmDev_CopyImage(m_VmHandle, "", "../", 0),
						PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_ASYNC_OP_FAILED(PrlVmDev_CopyImage(hHdd, "", NULL, 0),
						PRL_ERR_INVALID_ARG);

	// Real device
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hHdd, PDT_USE_REAL_DEVICE));
	CHECK_ASYNC_OP_FAILED(PrlVmDev_CopyImage(hHdd, NULL, "../", 0),
						PRL_ERR_CI_DEVICE_IS_NOT_VIRTUAL);

}

void PrlVmDevManipulationsTest::testHardDiskGetStorageURL()
{
	CREATE_HARD_DISK_DEVICE
	PRL_CHAR buf[STR_BUF_LENGTH];
	PRL_UINT32 len = STR_BUF_LENGTH;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_GetStorageURL(m_ServerHandle, buf, &len),
		PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_GetStorageURL(hVmDev, buf, 0), PRL_ERR_INVALID_ARG);
	CHECK_RET_CODE_EXP(PrlVmDevHd_GetStorageURL(hVmDev, 0, &len));

	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmHardDisk hdd;
	QVERIFY(StringToElement<CVmHardDisk*>(&hdd, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QUrl url("backup://server:12345/{BACKUP}/diskname");
	hdd.setStorageURL(url);
	QString s = ElementToString<CVmHardDisk*>(&hdd, XML_VM_CONFIG_EL_HARD_DISK);
	QVERIFY(!s.isEmpty());
	CHECK_RET_CODE_EXP(PrlVmDev_FromString(hVmDev, QSTR2UTF8(s)));
	len = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmDevHd_GetStorageURL(hVmDev, buf, &len))
	QCOMPARE(UTF8_2QSTR(buf), url.toString());
}

void PrlVmDevManipulationsTest::testHardDiskSetStorageURL()
{
	CREATE_HARD_DISK_DEVICE
	PRL_CHAR buf[STR_BUF_LENGTH] = "backup://server:12345/{BACKUP}/diskname";

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_SetStorageURL(m_ServerHandle, buf), PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmDevHd_SetStorageURL(hVmDev, 0), PRL_ERR_INVALID_ARG);

	CHECK_RET_CODE_EXP(PrlVmDevHd_SetStorageURL(hVmDev, buf))
	PRL_VOID_PTR pDevice = 0;
	CHECK_RET_CODE_EXP(PrlVmDev_ToString(hVmDev, &pDevice))
	CVmHardDisk hdd;
	QVERIFY(StringToElement<CVmHardDisk*>(&hdd, UTF8_2QSTR((const char *)pDevice)));
	PrlBuffer_Free(pDevice);
	QCOMPARE(hdd.getStorageURL().toString(), UTF8_2QSTR(buf));
}
