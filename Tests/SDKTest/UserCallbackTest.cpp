/////////////////////////////////////////////////////////////////////////////
///
///	@file UserCallbackTest.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing server login SDK API.
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
#include "Libraries/Logging/Logging.h"

#include "UserCallbackTest.h"
#include "Tests/CommonTestsUtils.h"

#include <QFile>
#include <QTextStream>
#include <QMutex>

#define ITERATIONS 10

namespace {
PRL_RESULT UserCallback (PRL_HANDLE handle, void *data);

class CSimpleListener
{
public:
	CSimpleListener(PRL_HANDLE handle)
	: m_Handle(handle), m_EventHandle(PRL_INVALID_HANDLE), m_Mutex(QMutex::Recursive), m_bNotified(false)
	{
		PRL_HANDLE_TYPE _type = PHT_ERROR;
		PrlHandle_GetType(m_Handle, &_type);
		if (_type == PHT_SERVER)
			PrlSrv_RegEventHandler(m_Handle, UserCallback, this);
		else if (_type == PHT_VIRTUAL_MACHINE)
			PrlVm_RegEventHandler(m_Handle, UserCallback, this);
		else
			QWARN("Unknown handle type in CSimpleListener constructor");
	}

	~CSimpleListener()
	{
		PRL_HANDLE_TYPE _type = PHT_ERROR;
		PrlHandle_GetType(m_Handle, &_type);
		if (_type == PHT_SERVER)
			PrlSrv_UnregEventHandler(m_Handle, UserCallback, this);
		else if (_type == PHT_VIRTUAL_MACHINE)
			PrlVm_UnregEventHandler(m_Handle, UserCallback, this);
		else
			QWARN("Unknown handle type in CSimpleListener destructor");
	}

	void Notify(SdkHandleWrap event)
	{
		QMutexLocker _lock(&m_Mutex);
		m_EventHandle = event;
		QMutexLocker _wait_lock(&m_WaitMutex);
		m_bNotified = true;
		m_Condition.wakeAll();
	}

	PRL_HANDLE GetEventHandle()
	{
		QMutexLocker _lock(&m_Mutex);
		return (m_EventHandle);
	}

	bool CheckNotified(SdkHandleWrap hExpectedHandle)
	{
		QMutexLocker _wait_lock(&m_WaitMutex);
		if (!m_bNotified || m_EventHandle != hExpectedHandle)
			m_Condition.wait(&m_WaitMutex, PRL_JOB_WAIT_TIMEOUT);
		return (m_bNotified);
	}

private:
	PRL_HANDLE m_Handle;
	SdkHandleWrap m_EventHandle;
	QMutex m_Mutex;
	bool m_bNotified;
	QMutex m_WaitMutex;
	QWaitCondition m_Condition;
};

PRL_RESULT UserCallback (PRL_HANDLE _handle, void *data)
{
	SdkHandleWrap handle(_handle);
	PRL_HANDLE_TYPE _type = PHT_ERROR;
	PrlHandle_GetType(handle, &_type);
	if (_type == PHT_JOB)
	{
		CSimpleListener *pListener = reinterpret_cast<CSimpleListener *>(data);
		if (pListener)
			pListener->Notify(handle);
	}
	return PRL_ERR_SUCCESS;
}

}

void UserCallbackTest::init()
{
	CHECK_RET_CODE_EXP(PrlSrv_Create(m_ServerHandle.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VM1Handle.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VM2Handle.GetHandlePtr()))
	m_JobHandle.reset(PrlSrv_LoginLocal(m_ServerHandle, NULL, 0, PSL_HIGH_SECURITY));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))
}

void UserCallbackTest::cleanup()
{
	m_JobHandle.reset(PrlVm_Delete(m_VM1Handle,PRL_INVALID_HANDLE));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))
	m_JobHandle.reset(PrlVm_Delete(m_VM2Handle,PRL_INVALID_HANDLE));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))
	m_JobHandle.reset(PrlSrv_Logoff(m_ServerHandle));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))
	m_VM1Handle.reset();
	m_VM2Handle.reset();
	m_ServerHandle.reset();
	m_VmDevHandle.reset();
	m_JobHandle.reset();
}

#define VERIFY_RESULT\
		{\
		QVERIFY(PRL_SUCCEEDED(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT)));\
		PRL_RESULT _ret_code = PRL_ERR_UNEXPECTED;\
		QVERIFY(PRL_SUCCEEDED(PrlJob_GetRetCode(m_JobHandle, &_ret_code)));\
		if (!PRL_SUCCEEDED(_ret_code)) {\
			WRITE_TRACE(DBG_FATAL, "_ret_code=%.8X", _ret_code);\
		}\
		QVERIFY(_l1.CheckNotified(m_JobHandle));\
		QVERIFY(_l2.CheckNotified(m_JobHandle));\
		}

void UserCallbackTest::testUserCallbackServerOnLoginLogoff()
{
	{
		CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
		m_JobHandle.reset(PrlSrv_Logoff(m_ServerHandle));
		VERIFY_RESULT
	}
	m_ServerHandle.reset();
	CHECK_RET_CODE_EXP(PrlSrv_Create(m_ServerHandle.GetHandlePtr()))
	{
		CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
		m_JobHandle.reset(PrlSrv_Login(m_ServerHandle, TestConfig::getRemoteHostName(), TestConfig::getUserLogin(),
			TestConfig::getUserPassword(), NULL, 0, 0, PSL_HIGH_SECURITY));
		VERIFY_RESULT
	}
}

void UserCallbackTest::testUserCallbackServerOnGetVmList()
{
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	for (size_t i = 0; i < ITERATIONS; i++)
	{
		m_JobHandle.reset(PrlSrv_GetVmList(m_ServerHandle));
		VERIFY_RESULT
	}
}

void UserCallbackTest::testUserCallbackServerOnNetServiceStart()
{
//	QSKIP("Skipping test until necessary functionality will be implemented at dispatcher", SkipAll);
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlSrv_NetServiceStart(m_ServerHandle));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnNetServiceStop()
{
//	QSKIP("Skipping test until necessary functionality will be implemented at dispatcher", SkipAll);
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlSrv_NetServiceStop(m_ServerHandle));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnNetServiceRestart()
{
//	QSKIP("Skipping test until necessary functionality will be implemented at dispatcher", SkipAll);
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlSrv_NetServiceRestart(m_ServerHandle));
	VERIFY_RESULT
}
/*
void UserCallbackTest::testUserCallbackServerOnNetServiceRestoreDefaults()
{
//	QSKIP("Skipping test until necessary functionality will be implemented at dispatcher", SkipAll);
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlSrv_NetServiceRestoreDefaults(m_ServerHandle));
	VERIFY_RESULT
}
*/
void UserCallbackTest::testUserCallbackServerOnGetNetServiceStatus()
{
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlSrv_GetNetServiceStatus(m_ServerHandle));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnGetSrvConfig()
{
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlSrv_GetSrvConfig(m_ServerHandle));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnJobCancel()
{
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlSrv_GetVmList(m_ServerHandle));
	m_JobHandle.reset(PrlJob_Cancel(m_JobHandle));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnVmReg()
{
	if (!TestConfig::isServerMode())
	{
		QSKIP("Skipping due need refactoring with preparation of VM root dir", SkipAll);
	}
	QFile _file("./TestDspCmdDirValidateVmConfig_vm_config.xml");
	QVERIFY(_file.open(QIODevice::ReadOnly));
	QTextStream _stream(&_file);
	QString _config = _stream.readAll();
	QVERIFY(PrlVm_FromString(m_VM1Handle, _config.toUtf8().data()) == PRL_ERR_SUCCESS);
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlVm_Reg(m_VM1Handle, "", PRL_TRUE));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnVmUnreg()
{
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlVm_Unreg(m_VM1Handle));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnVmDelete()
{
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlVm_Delete(m_VM1Handle,PRL_INVALID_HANDLE));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnVmStart()
{
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlVm_Start(m_VM1Handle));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnVmStop()
{
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlVm_Stop(m_VM1Handle, PRL_FALSE));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnVmPause()
{
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlVm_Pause(m_VM1Handle, PRL_FALSE));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnVmResume()
{
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlVm_Resume(m_VM1Handle));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnVmSuspend()
{
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlVm_Suspend(m_VM1Handle));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnVmReset()
{
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlVm_Reset(m_VM1Handle));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnVmClone()
{
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlVm_Clone(m_VM1Handle, "new VM name", "new VM config path", PRL_FALSE));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnVmGetState()
{
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlVm_GetState(m_VM1Handle));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnVmGetConfig()
{
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlVm_RefreshConfig(m_VM1Handle));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnVmBeginEdit()
{
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlVm_BeginEdit(m_VM1Handle));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnVmCommit()
{
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlVm_Commit(m_VM1Handle));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnVmInitiateDevStateNotifications()
{
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlVm_InitiateDevStateNotifications(m_VM1Handle));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnVmDevConnect()
{
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VM1Handle, PDE_FLOPPY_DISK, m_VmDevHandle.GetHandlePtr()))
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlVmDev_Connect(m_VmDevHandle));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnVmDevDisconnect()
{
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VM1Handle, PDE_FLOPPY_DISK, m_VmDevHandle.GetHandlePtr()))
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlVmDev_Disconnect(m_VmDevHandle));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerOnVmDevCreateImage()
{
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VM1Handle, PDE_FLOPPY_DISK, m_VmDevHandle.GetHandlePtr()))
	CSimpleListener _l1(m_ServerHandle), _l2(m_ServerHandle);
	m_JobHandle.reset(PrlVmDev_CreateImage(m_VmDevHandle, 0, PRL_TRUE));
	VERIFY_RESULT
}

void UserCallbackTest::testUserCallbackServerWithNullData()
{
	CHECK_RET_CODE_EXP(PrlSrv_RegEventHandler(m_ServerHandle, UserCallback, 0))
	m_JobHandle.reset(PrlSrv_GetVmList(m_ServerHandle));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))
}

void UserCallbackTest::testUserCallbackVM()
{
	if (!TestConfig::isServerMode())
	{
		QSKIP("Skipping due need refactoring with preparation of VM root dir", SkipAll);
	}
	QFile _file("./TestDspCmdDirValidateVmConfig_vm_config.xml");
	QVERIFY(_file.open(QIODevice::ReadOnly));
	QTextStream _stream(&_file);
	QString _config = _stream.readAll();
	QVERIFY(PrlVm_FromString(m_VM1Handle, _config.toUtf8().data()) == PRL_ERR_SUCCESS);
	QVERIFY(PrlVm_FromString(m_VM2Handle, _config.toUtf8().data()) == PRL_ERR_SUCCESS);
	CSimpleListener _l1(m_VM1Handle), _l2(m_VM2Handle);
	m_JobHandle.reset(PrlVm_Reg(m_VM1Handle, "", PRL_TRUE));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))
	QVERIFY(_l1.CheckNotified(m_JobHandle));
	QVERIFY(_l1.GetEventHandle() != PRL_INVALID_HANDLE && _l2.GetEventHandle() == PRL_INVALID_HANDLE);
	m_JobHandle.reset(PrlVm_Delete(m_VM1Handle,PRL_INVALID_HANDLE));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))
	QVERIFY(_l1.GetEventHandle() != PRL_INVALID_HANDLE && _l2.GetEventHandle() == PRL_INVALID_HANDLE);
	QVERIFY(_l1.CheckNotified(m_JobHandle));
	_l1.Notify(SdkHandleWrap());
	m_JobHandle.reset(PrlVm_Reg(m_VM2Handle, "", PRL_TRUE));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))
	QVERIFY(_l2.CheckNotified(m_JobHandle));
	QVERIFY(_l1.GetEventHandle() == PRL_INVALID_HANDLE && _l2.GetEventHandle() != PRL_INVALID_HANDLE);
}
