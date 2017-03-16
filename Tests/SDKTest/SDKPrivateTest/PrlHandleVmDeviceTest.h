/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlHandleVmDeviceTest.h
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
#ifndef PrlHandleVmDeviceTest_H
#define PrlHandleVmDeviceTest_H

#include <QtTest/QtTest>
#include <prlsdk/Parallels.h>
#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"

class PrlHandleVmDeviceTest : public QObject
{

Q_OBJECT

public:
	PrlHandleVmDeviceTest();

private slots:
	void init();
	void cleanup();
	void testfromStringFloppyDisk();
	void testfromStringHardDisk();
	void testfromStringOnNonVmDeviceHandle();
	void testfromStringOnWrongStrPtr();
	void testfromStringOnWrongXmlString();
	void testConnectDeviceOnInvalidDeviceHandle();
	void testConnectDeviceOnWrongDeviceHandle();
	void testDisconnectDeviceOnInvalidDeviceHandle();
	void testDisconnectDeviceOnWrongDeviceHandle();
	void testCreateImageDeviceOnInvalidDeviceHandle();
	void testCreateImageDeviceOnWrongDeviceHandle();

private:
	SdkHandleWrap m_ServerHandle;
	SdkHandleWrap m_VmHandle;
	SdkHandleWrap m_Handle;
	SdkHandleWrap m_JobHandle;
};

#endif
