/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlHandleVmDeviceTest.cpp
///
///	This file is the part of parallels public SDK library private tests suite.
///	Tests fixture class for testing SDK API calls for VM devices.
///
///	@author sandro
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
#include "PrlHandleVmDeviceTest.h"
#include "SDK/Handles/Vm/PrlHandleVmDevice.h"
#include <prlxmlmodel/VmConfig/CVmFloppyDisk.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include "Tests/CommonTestsUtils.h"

PrlHandleVmDeviceTest::PrlHandleVmDeviceTest()
: m_ServerHandle(PRL_INVALID_HANDLE), m_VmHandle(PRL_INVALID_HANDLE),
	m_Handle(PRL_INVALID_HANDLE), m_JobHandle(PRL_INVALID_HANDLE)
{}

void PrlHandleVmDeviceTest::init()
{
	QVERIFY(PRL_SUCCEEDED(PrlSrv_Create(m_ServerHandle.GetHandlePtr())));
	QVERIFY(PRL_SUCCEEDED(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr())));
}

void PrlHandleVmDeviceTest::cleanup()
{
	m_Handle.reset();
   	m_JobHandle.reset();
	m_VmHandle.reset();
	m_ServerHandle.reset();
}

void PrlHandleVmDeviceTest::testfromStringFloppyDisk()
{
	QVERIFY(PRL_SUCCEEDED(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, m_Handle.GetHandlePtr())));
	CVmFloppyDisk _xml_wrapper;
	_xml_wrapper.setEmulatedType(PVE::FloppyDiskImage);
	QString _xml = _xml_wrapper.toString();
	QVERIFY(PRL_SUCCEEDED(PrlHandle_FromString(m_Handle, _xml.toUtf8().data())));
	PrlHandleVmDevicePtr _floppy_device = PRL_OBJECT_BY_HANDLE<PrlHandleVmDevice>(m_Handle);
	QVERIFY(_floppy_device);
	QCOMPARE(_floppy_device->toString(), _xml);
}

void PrlHandleVmDeviceTest::testfromStringHardDisk()
{
	QVERIFY(PRL_SUCCEEDED(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, m_Handle.GetHandlePtr())));
	CVmHardDisk _xml_wrapper;
	_xml_wrapper.setEmulatedType(PVE::HardDiskImage);
	QString _xml = _xml_wrapper.toString();
	QVERIFY(PRL_SUCCEEDED(PrlHandle_FromString(m_Handle, _xml.toUtf8().data())));
	PrlHandleVmDevicePtr _hard_device = PRL_OBJECT_BY_HANDLE<PrlHandleVmDevice>(m_Handle);
	QVERIFY(_hard_device);
	QCOMPARE(_hard_device->toString(), _xml);
}

void PrlHandleVmDeviceTest::testfromStringOnNonVmDeviceHandle()
{
	QVERIFY(PRL_SUCCEEDED(PrlSrv_Create(m_Handle.GetHandlePtr())));
	CVmFloppyDisk _xml_wrapper;
	_xml_wrapper.setEmulatedType(PVE::FloppyDiskImage);
	QString _xml = ElementToString<CVmFloppyDisk *>(&_xml_wrapper, XML_VM_CONFIG_EL_HARDWARE);
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_FromString(m_Handle, _xml.toUtf8().data())));
}

void PrlHandleVmDeviceTest::testfromStringOnWrongStrPtr()
{
	QVERIFY(PRL_SUCCEEDED(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, m_Handle.GetHandlePtr())));
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_FromString(m_Handle, NULL)));
}

void PrlHandleVmDeviceTest::testfromStringOnWrongXmlString()
{
	QVERIFY(PRL_SUCCEEDED(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, m_Handle.GetHandlePtr())));
	CVmHardDisk _xml_wrapper;
	_xml_wrapper.setEmulatedType(PVE::HardDiskImage);
	QString _xml = ElementToString<CVmHardDisk *>(&_xml_wrapper, XML_VM_CONFIG_EL_HARDWARE);
	QVERIFY(!PRL_SUCCEEDED(PrlVmDev_FromString(m_Handle, _xml.toUtf8().data())));
}

void PrlHandleVmDeviceTest::testConnectDeviceOnInvalidDeviceHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVmDev_Connect(PRL_INVALID_HANDLE), PRL_ERR_INVALID_ARG)
}

void PrlHandleVmDeviceTest::testConnectDeviceOnWrongDeviceHandle()
{
	QVERIFY(PRL_SUCCEEDED(PrlSrv_Create(m_Handle.GetHandlePtr())));
	CHECK_ASYNC_OP_FAILED(PrlVmDev_Connect(m_Handle), PRL_ERR_INVALID_ARG)
}

void PrlHandleVmDeviceTest::testDisconnectDeviceOnInvalidDeviceHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVmDev_Disconnect(PRL_INVALID_HANDLE), PRL_ERR_INVALID_ARG)
}

void PrlHandleVmDeviceTest::testDisconnectDeviceOnWrongDeviceHandle()
{
	QVERIFY(PRL_SUCCEEDED(PrlSrv_Create(m_Handle.GetHandlePtr())));
	CHECK_ASYNC_OP_FAILED(PrlVmDev_Disconnect(m_Handle), PRL_ERR_INVALID_ARG)
}

void PrlHandleVmDeviceTest::testCreateImageDeviceOnInvalidDeviceHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVmDev_CreateImage(PRL_INVALID_HANDLE, 0, PRL_TRUE), PRL_ERR_INVALID_ARG)
}

void PrlHandleVmDeviceTest::testCreateImageDeviceOnWrongDeviceHandle()
{
	QVERIFY(PRL_SUCCEEDED(PrlSrv_Create(m_Handle.GetHandlePtr())));
	CHECK_ASYNC_OP_FAILED(PrlVmDev_CreateImage(m_Handle, 0, PRL_TRUE), PRL_ERR_INVALID_ARG)
}
