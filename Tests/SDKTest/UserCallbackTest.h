/////////////////////////////////////////////////////////////////////////////
///
///	@file UserCallbackTest.h
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing server login SDK API.
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
#ifndef UserCallbackTest_H
#define UserCallbackTest_H

#include <QtTest/QtTest>

#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"

class UserCallbackTest : public QObject
{

Q_OBJECT

private slots:
	void init();
	void cleanup();
	void testUserCallbackServerOnLoginLogoff();
	void testUserCallbackServerOnGetVmList();
	void testUserCallbackServerOnNetServiceStart();
	void testUserCallbackServerOnNetServiceStop();
	void testUserCallbackServerOnNetServiceRestart();
//	void testUserCallbackServerOnNetServiceRestoreDefaults();
	void testUserCallbackServerOnGetNetServiceStatus();
	void testUserCallbackServerOnGetSrvConfig();
	void testUserCallbackServerOnJobCancel();
	void testUserCallbackServerOnVmReg();
	void testUserCallbackServerOnVmUnreg();
	void testUserCallbackServerOnVmDelete();
	void testUserCallbackServerOnVmStart();
	void testUserCallbackServerOnVmStop();
	void testUserCallbackServerOnVmPause();
	void testUserCallbackServerOnVmResume();
	void testUserCallbackServerOnVmSuspend();
	void testUserCallbackServerOnVmReset();
	void testUserCallbackServerOnVmClone();
	void testUserCallbackServerOnVmGetState();
	void testUserCallbackServerOnVmGetConfig();
	void testUserCallbackServerOnVmBeginEdit();
	void testUserCallbackServerOnVmCommit();
	void testUserCallbackServerOnVmDevConnect();
	void testUserCallbackServerOnVmDevDisconnect();
	void testUserCallbackServerOnVmDevCreateImage();
	void testUserCallbackServerOnVmInitiateDevStateNotifications();

	void testUserCallbackServerWithNullData();

	void testUserCallbackVM();

private:
	SdkHandleWrap m_ServerHandle;
	SdkHandleWrap m_JobHandle;
	SdkHandleWrap m_VM1Handle;
	SdkHandleWrap m_VM2Handle;
	SdkHandleWrap m_VmDevHandle;
};

#endif
