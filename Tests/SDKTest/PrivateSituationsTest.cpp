/////////////////////////////////////////////////////////////////////////////
///
///	@file PrivateSituationsTest.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing private situations of SDK API usage.
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

#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"
#include <prlcommon/Interfaces/ParallelsSdkPrivate.h>
#include "PrivateSituationsTest.h"
#include <prlcommon/PrlUuid/Uuid.h>
#include <prlcommon/Logging/Logging.h>
#include "Tests/CommonTestsUtils.h"
#include <set>
#include "XmlModel/Messaging/CVmEvent.h"
#include "XmlModel/Messaging/CVmEventParameter.h"
#include "Build/Current.ver"
#include "Libraries/ProtoSerializer/CProtoSerializer.h"

using std::set;
using namespace Parallels;

namespace {
PRL_RESULT UserCallback (PRL_HANDLE _handle, void *_data)
{
	SdkHandleWrap handle(_handle);
	PRL_HANDLE_TYPE _type = PHT_ERROR;
	PrlHandle_GetType(handle, &_type);
	if (_type == PHT_JOB)
	{
		PrivateSituationsTest *pTestSuite = static_cast<PrivateSituationsTest *>(_data);
		pTestSuite->m_Mutex.lock();
		SdkHandleWrap hServer = pTestSuite->m_ServerHandle;
		pTestSuite->m_Mutex.unlock();
		PrlSrv_UnregEventHandler(hServer, UserCallback, (void *)pTestSuite);
		SdkHandleWrap hJob(PrlSrv_GetUserProfile(hServer));
		if (PRL_SUCCEEDED(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT)))
		{
			PRL_RESULT nRetCode;
			if (PRL_SUCCEEDED(PrlJob_GetRetCode(hJob, &nRetCode)))
			{
				if (!PRL_SUCCEEDED(nRetCode))
					LOG_MESSAGE(DBG_FATAL, "Non success result of PrlSrv_GetUserProfile() execution");
				else
				{
					QMutexLocker _lock(&pTestSuite->m_Mutex);
					pTestSuite->m_bExecutionSuccessful = true;
				}
			}
			else
				LOG_MESSAGE(DBG_FATAL, "Non success result of PrlJob_GetRetCode() execution");
		}
		else
			LOG_MESSAGE(DBG_FATAL, "Non success result of PrlJob_Wait() execution");
		QMutexLocker _lock(&pTestSuite->m_Mutex);
		pTestSuite->m_Condition.wakeAll();
	}
	return PRL_ERR_SUCCESS;
}

class AsyncMethsExecutor : public QThread
{
public:
	AsyncMethsExecutor(bool &bExecutionResult, size_t nThreadNum)
	: m_cancelled(), m_bExecutionResult(bExecutionResult), m_nThreadNum(nThreadNum)
	{
		QMutexLocker _lock(&g_ObjectsCountMutex);
		g_nObjectsCount++;
		g_ObjsSet.insert(this);
	}

	virtual ~AsyncMethsExecutor()
	{
		QMutexLocker _lock(&g_ObjectsCountMutex);
		g_ObjsSet.erase(this);
	}

	static PRL_RESULT Callback(PRL_HANDLE _handle, void *pData)
	{
		SdkHandleWrap handle(_handle);
		PRL_HANDLE_TYPE _type = PHT_ERROR;
		PrlHandle_GetType(handle, &_type);
		if (_type == PHT_JOB)
		{
			AsyncMethsExecutor *pExecutor = static_cast<AsyncMethsExecutor *>(pData);
			if (pExecutor)
			{
				QMutexLocker _lock(&pExecutor->m_Mutex);
				if (pExecutor->m_hJob.GetHandle() != _handle)
					return (PRL_ERR_SUCCESS);
				PrlSrv_UnregEventHandler(pExecutor->m_ServerHandle, Callback, pExecutor);
				PRL_RESULT nRetCode;
				if (PRL_SUCCEEDED(PrlJob_GetRetCode(handle, &nRetCode)))
				{
					if (PRL_SUCCEEDED(nRetCode))
					{
						SdkHandleWrap hJob(PrlVm_Delete(pExecutor->m_VmHandle,PRL_INVALID_HANDLE));
						PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT);
						hJob.reset(PrlSrv_Logoff(pExecutor->m_ServerHandle));
						PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT);
						pExecutor->m_bExecutionResult = true;
					}
					else
						WRITE_TRACE(DBG_FATAL, "VM registration failed with code %.8X!", nRetCode);
				}
				else
					WRITE_TRACE(DBG_FATAL, "Couldn't to get job return code!");
				pExecutor->m_Condition.wakeAll();
			}
		}
		return (PRL_ERR_SUCCESS);
	}

	void run()
	{
		LOG_MESSAGE(DBG_DEBUG, "Thread %d begin work", m_nThreadNum);
		m_bExecutionResult = false;
		if (PRL_SUCCEEDED(PrlSrv_Create(m_ServerHandle.GetHandlePtr())))
		{
			m_hJob.reset(PrlSrv_LoginLocal(m_ServerHandle, "", 0, PSL_HIGH_SECURITY));
			PRL_RESULT nRetCode = PRL_ERR_OPERATION_WAS_CANCELED;
			if (!m_cancelled && PrlJob_Wait(m_hJob, PRL_JOB_WAIT_TIMEOUT) == PRL_ERR_TIMEOUT)
			{
				for (int i = 0; i < 10 && !m_cancelled; ++i)
					PrlJob_Wait(m_hJob, PRL_JOB_WAIT_TIMEOUT);
			}
			if (!m_cancelled)
				PrlJob_GetRetCode(m_hJob, &nRetCode);
			if (PRL_SUCCEEDED(nRetCode))
			{
				if (PRL_SUCCEEDED(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr())))
				{
					if (PRL_SUCCEEDED(PrlVmCfg_SetName(m_VmHandle, QString("%1.%2").arg(QTest::currentTestFunction()).arg(Uuid::createUuid().toString()).toUtf8().data())))
					{
						QMutexLocker _lock(&m_Mutex);
						PrlSrv_RegEventHandler(m_ServerHandle, Callback, this);
						m_hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
						if (!m_cancelled)
							m_Condition.wait(&m_Mutex);
					}
					else
						WRITE_TRACE(DBG_FATAL, "VM name setting failed!");
				}
				else
					WRITE_TRACE(DBG_FATAL, "VM handle creation failed!");
			}
			else
				WRITE_TRACE(DBG_FATAL, "Login local command failed with code %.8X!", nRetCode);
		}
		else
			WRITE_TRACE(DBG_FATAL, "Server handle creation failed!");
		LOG_MESSAGE(DBG_DEBUG, "Thread %d finalizing work", m_nThreadNum);
		QMutexLocker _lock(&g_ObjectsCountMutex);
		g_nObjectsCount--;
	}

	void cancel()
	{
		if (isFinished() || m_cancelled)
			return;

		QMutexLocker g(&m_Mutex);
		if (!m_cancelled)
		{
			m_cancelled = true;
			m_Condition.wakeAll();
		}
	}

	static size_t GetObjectsCount()
	{
		QMutexLocker _lock(&g_ObjectsCountMutex);
		return (g_nObjectsCount);
	}

public:
	static set<AsyncMethsExecutor *> g_ObjsSet;
	SdkHandleWrap m_hJob;

protected:
	bool m_cancelled;
	bool &m_bExecutionResult;
	size_t m_nThreadNum;
	QMutex m_Mutex;
	QWaitCondition m_Condition;
	SdkHandleWrap m_ServerHandle;
	SdkHandleWrap m_VmHandle;

protected:
	static size_t g_nObjectsCount;
	static QMutex g_ObjectsCountMutex;
};

size_t AsyncMethsExecutor::g_nObjectsCount = 0;
QMutex AsyncMethsExecutor::g_ObjectsCountMutex(QMutex::Recursive);
set<AsyncMethsExecutor *> AsyncMethsExecutor::g_ObjsSet;

class SyncVmRegister : public AsyncMethsExecutor
{
public:

	SyncVmRegister(QString qsVmName, bool &bExecutionResult, size_t nThreadNum)
		: AsyncMethsExecutor(bExecutionResult, nThreadNum),
		  m_qsVmName(qsVmName)
			{}

	virtual ~SyncVmRegister() {}

	virtual void run()
	{
		LOG_MESSAGE(DBG_DEBUG, "Thread %d begin work", m_nThreadNum);
		if (PRL_SUCCEEDED(PrlSrv_Create(m_ServerHandle.GetHandlePtr())))
		{
			m_hJob.reset(PrlSrv_LoginLocal(m_ServerHandle, "", 0, PSL_HIGH_SECURITY));
			if (PrlJob_Wait(m_hJob, PRL_JOB_WAIT_TIMEOUT) == PRL_ERR_TIMEOUT)
				PrlJob_Wait(m_hJob, 10*PRL_JOB_WAIT_TIMEOUT);
			PRL_RESULT nRetCode;
			PrlJob_GetRetCode(m_hJob, &nRetCode);
			if (PRL_SUCCEEDED(nRetCode))
			{
				if (PRL_SUCCEEDED(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr())))
				{
					if (PRL_SUCCEEDED(PrlVmCfg_SetName( m_VmHandle, QSTR2UTF8(m_qsVmName) )))
					{
						m_hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
						if (PrlJob_Wait(m_hJob, PRL_JOB_WAIT_TIMEOUT) == PRL_ERR_TIMEOUT)
							PrlJob_Wait(m_hJob, 10*PRL_JOB_WAIT_TIMEOUT);
						PrlJob_GetRetCode(m_hJob, &nRetCode);
						if (PRL_SUCCEEDED(nRetCode))
						{
							m_hJob.reset(PrlVm_Delete(m_VmHandle,PRL_INVALID_HANDLE));
							PrlJob_Wait(m_hJob, PRL_JOB_WAIT_TIMEOUT);
							m_hJob.reset(PrlSrv_Logoff(m_ServerHandle));
							PrlJob_Wait(m_hJob, PRL_JOB_WAIT_TIMEOUT);

							m_bExecutionResult = true;
						}
						else
							WRITE_TRACE(DBG_FATAL, "VM registration failed with code %.8X!", nRetCode);
					}
					else
						WRITE_TRACE(DBG_FATAL, "VM name setting failed!");
				}
				else
					WRITE_TRACE(DBG_FATAL, "VM handle creation failed!");
			}
			else
				WRITE_TRACE(DBG_FATAL, "Login local command failed with code %.8Xd!", nRetCode);
		}
		else
			WRITE_TRACE(DBG_FATAL, "Server handle creation failed!");
		LOG_MESSAGE(DBG_DEBUG, "Thread %d finalizing work", m_nThreadNum);
		QMutexLocker _lock(&g_ObjectsCountMutex);
		g_nObjectsCount--;
	}

private:

	QString m_qsVmName;

};

}//namespace

void PrivateSituationsTest::init()
{
	CHECK_RET_CODE_EXP(PrlSrv_Create(m_ServerHandle.GetHandlePtr()))
}

void PrivateSituationsTest::cleanup()
{
	if (m_sVmUuid.size())
	{
		SdkHandleWrap hVm;
		CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))
		CHECK_RET_CODE_EXP(PrlVmCfg_SetName(hVm, m_sVmName.toUtf8().data()))
		CHECK_RET_CODE_EXP(PrlVmCfg_SetUuid(hVm, m_sVmUuid.toUtf8().data()))
		SdkHandleWrap hJob(PrlVm_Delete(hVm,PRL_INVALID_HANDLE));
		CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT))
		m_sVmUuid = m_sVmName = "";
	}
	SdkHandleWrap hJob(PrlSrv_Logoff(m_ServerHandle));
	PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT);
	m_ServerHandle.reset();
	while (AsyncMethsExecutor::g_ObjsSet.size())
	{
		AsyncMethsExecutor* x = *AsyncMethsExecutor::g_ObjsSet.begin();
		x->cancel();
		x->wait();
		delete x;
	}

}

void PrivateSituationsTest::testCallAsyncMethodFromCallback()
{
	CHECK_RET_CODE_EXP(PrlSrv_RegEventHandler(m_ServerHandle, UserCallback, (void *)this))
	QMutexLocker _lock(&m_Mutex);
	m_bExecutionSuccessful = false;
	SdkHandleWrap hJob;
	if (TestConfig::isServerMode())
	{
		hJob.reset(PrlSrv_Login(m_ServerHandle, TestConfig::getRemoteHostName(), TestConfig::getUserLogin(),
								TestConfig::getUserPassword(), NULL, 0, 0, PSL_HIGH_SECURITY));
	}
	else
	{
		hJob.reset(PrlSrv_LoginLocal(m_ServerHandle, NULL, 0, PSL_HIGH_SECURITY));
	}
	QVERIFY(m_Condition.wait(&m_Mutex, 10*PRL_JOB_WAIT_TIMEOUT));
	QVERIFY(m_bExecutionSuccessful);
}

void PrivateSituationsTest::testOftenCallsOfPrlJobWait()
{
	const size_t nTestTimeout = 100;
	SdkHandleWrap hJob;
	if (TestConfig::isServerMode())
	{
		hJob.reset(PrlSrv_Login(m_ServerHandle, TestConfig::getRemoteHostName(),	TestConfig::getUserLogin(),
								TestConfig::getUserPassword(), NULL, 0, 0, PSL_HIGH_SECURITY));
	}
	else
	{
		hJob.reset(PrlSrv_LoginLocal(m_ServerHandle, NULL, 0, PSL_HIGH_SECURITY));
	}
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))
	m_sVmName = QString("%1.%2").arg(QTest::currentTestFunction()).arg(Uuid::createUuid().toString());
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(hVm, m_sVmName.toUtf8().data()))
	char sVmUuid[STR_BUF_LENGTH];
	PRL_UINT32 nVmUuidLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUuid(hVm, sVmUuid, &nVmUuidLength))
	m_sVmUuid = UTF8_2QSTR(sVmUuid);
	hJob.reset(PrlVm_Reg(hVm, "", PRL_TRUE));
	size_t nMaxPossibleIterations = 100000;
	while (--nMaxPossibleIterations)
	{
		if(PRL_SUCCEEDED(PrlJob_Wait(hJob, nTestTimeout)))
		{
			PRL_RESULT nJobRetCode;
			CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nJobRetCode))
			CHECK_RET_CODE_EXP(nJobRetCode)
			return;//All fine so finalizing tests execution
		}
	}
	QFAIL("Job wasn't completed!");
}

void PrivateSituationsTest::testSimultaneouslyAsyncOperationsCall()
{
	const size_t nTestsObjsNum = 10;
	bool bExecutorResults[nTestsObjsNum];
	for (size_t i = 0; i < nTestsObjsNum; ++i)
	{
		AsyncMethsExecutor *pExecutor = new AsyncMethsExecutor(bExecutorResults[i], i);
		pExecutor->start();
	}
	const size_t nTimeout = 100;
	size_t nMaxPossibleIterationsNum = 10000;
	while (AsyncMethsExecutor::GetObjectsCount() && --nMaxPossibleIterationsNum)
		QTest::qWait(nTimeout);
	QVERIFY(!AsyncMethsExecutor::GetObjectsCount());
	for (size_t i = 0; i < nTestsObjsNum; ++i)
		QVERIFY(bExecutorResults[i]);
}

void PrivateSituationsTest::testSimultaneouslyVmCreationWithTheSameName()
{
	QSKIP( "Obsolete because we can't create vm with same name (bug #267227)", SkipAll );
	const size_t nTestsObjsNum = 10;
	bool bExecutorResults[nTestsObjsNum];
	for (size_t i = 0; i < nTestsObjsNum; ++i)
	{
		SyncVmRegister *pExecutor = new SyncVmRegister("virtual machine", bExecutorResults[i], i);
		pExecutor->start();
	}
	const size_t nTimeout = 100;
	size_t nMaxPossibleIterationsNum = 10000;
	while (SyncVmRegister::GetObjectsCount() && --nMaxPossibleIterationsNum)
		QTest::qWait(nTimeout);
	QVERIFY(!SyncVmRegister::GetObjectsCount());
	for (size_t i = 0; i < nTestsObjsNum; ++i)
		QVERIFY(bExecutorResults[i]);
}

namespace {
#define CHECK_SERVER_HOST_NAME\
	{\
		PRL_CHAR sBuffer[STR_BUF_LENGTH];\
		PRL_UINT32 nBufferLength = STR_BUF_LENGTH;\
		PRL_RESULT _ret_code = PrlSrvInfo_GetHostName(hServerInfo, sBuffer, &nBufferLength);\
		if (PRL_FAILED(_ret_code))\
		{\
			WRITE_TRACE(DBG_FATAL, "Failed to extract server host name. Error code: %.8X", _ret_code);\
			return (PRL_ERR_UNEXPECTED);\
		}\
		sHostName = UTF8_2QSTR(sBuffer);\
		if (sHostName.isEmpty())\
		{\
			WRITE_TRACE(DBG_FATAL, "Server hostname is empty");\
			return (PRL_ERR_UNEXPECTED);\
		}\
	}

#define CHECK_SERVER_OS_VERSION\
	{\
		PRL_CHAR sBuffer[STR_BUF_LENGTH];\
		PRL_UINT32 nBufferLength = STR_BUF_LENGTH;\
		PRL_RESULT _ret_code = PrlSrvInfo_GetOsVersion(hServerInfo, sBuffer, &nBufferLength);\
		if (PRL_FAILED(_ret_code))\
		{\
			WRITE_TRACE(DBG_FATAL, "Failed to extract server OS version. Error code: %.8X", _ret_code);\
			return (PRL_ERR_UNEXPECTED);\
		}\
		if (UTF8_2QSTR(sBuffer).isEmpty())\
		{\
			WRITE_TRACE(DBG_FATAL, "Server OS version is empty");\
			return (PRL_ERR_UNEXPECTED);\
		}\
	}

#define CHECK_SERVER_MANAGEMENT_PORT\
	{\
		PRL_UINT32 nPort = 0;\
		PRL_RESULT _ret_code = PrlSrvInfo_GetCmdPort(hServerInfo, &nPort);\
		if (PRL_FAILED(_ret_code))\
		{\
			WRITE_TRACE(DBG_FATAL, "Failed to extract server management port. Error code: %.8X '%s'. Hostname: '%s'",\
										_ret_code, PRL_RESULT_TO_STRING(_ret_code), sHostName.toUtf8().constData());\
			return (PRL_ERR_UNEXPECTED);\
		}\
		if (!nPort)\
		{\
			WRITE_TRACE(DBG_FATAL, "Null management port received");\
			return (PRL_ERR_UNEXPECTED);\
		}\
	}

#define CHECK_SERVER_UUID\
	{\
		PRL_CHAR sBuffer[STR_BUF_LENGTH];\
		PRL_UINT32 nBufferLength = STR_BUF_LENGTH;\
		PRL_RESULT _ret_code = PrlSrvInfo_GetServerUuid(hServerInfo, sBuffer, &nBufferLength);\
		if (PRL_FAILED(_ret_code))\
		{\
			WRITE_TRACE(DBG_FATAL, "Failed to extract server UUID. Error code: %.8X", _ret_code);\
			return (PRL_ERR_UNEXPECTED);\
		}\
		if (UTF8_2QSTR(sBuffer).isEmpty())\
		{\
			WRITE_TRACE(DBG_FATAL, "Server UUID is empty");\
			return (PRL_ERR_UNEXPECTED);\
		}\
	}

PRL_RESULT onParallelsServerFound(PRL_HANDLE hServerInfo, PRL_VOID_PTR pData)
{
	SdkHandleWrap _handle(hServerInfo);
	PRL_HANDLE_TYPE _type;
	PRL_RESULT _res = PrlHandle_GetType(_handle, &_type);
	if (PRL_SUCCEEDED(_res))
	{
		if (PHT_SERVER_INFO == _type)
		{
			QString sHostName;
			CHECK_SERVER_HOST_NAME
			CHECK_SERVER_MANAGEMENT_PORT
			CHECK_SERVER_OS_VERSION
			CHECK_SERVER_UUID
			static_cast<QStringList *>(pData)->append(sHostName);
		}
		else
			WRITE_TRACE(DBG_FATAL, "Wrong handle type was received at server found handler with type: %.8X", _type);
	}
	else
		WRITE_TRACE(DBG_FATAL, "PrlHandle_GetType() was failed with code: %.8X", _res);

	return (PRL_ERR_SUCCESS);
}

}

#define SERVERS_SEARCH_TIMEOUT 3000

#define TEST_PARAMS_COUNT(cmd_type, init_cmd, nExpectedParamsCount)\
{\
	SdkHandleWrap hJob(PrlSrv_GetUserProfile(PRL_INVALID_HANDLE));\
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT))\
	SdkHandleWrap hResult;\
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))\
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(cmd_type, PRL_ERR_SUCCESS);\
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);\
	pResponseCmd->init_cmd;\
	CHECK_RET_CODE_EXP(PrlResult_FromString(hResult, pCmd->GetCommand()->toString().toUtf8().constData()))\
	PRL_UINT32 nActualParamsCount = 0;\
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nActualParamsCount))\
	QCOMPARE(quint32(nExpectedParamsCount), quint32(nActualParamsCount));\
}

void PrivateSituationsTest::testGetParamsCountForCallsWithOneStandardParam()
{
	TEST_PARAMS_COUNT(PVE::DspCmdUserLogin, SetLoginResponse("some data"), 1)
		TEST_PARAMS_COUNT(PVE::DspCmdUserLoginLocalStage2, SetLoginResponse("some data"), 1)
		TEST_PARAMS_COUNT(PVE::DspCmdUserGetHostHwInfo, SetHostHardwareInfo("some data"), 1)
		TEST_PARAMS_COUNT(PVE::DspCmdGetHostCommonInfo, SetHostCommonInfo("some data", "some data2"), 1)
	TEST_PARAMS_COUNT(PVE::DspCmdUserGetProfile, SetUserProfile("some data"), 1)
	TEST_PARAMS_COUNT(PVE::DspCmdGetHostStatistics, SetSystemStatistics("some data"), 1)
	TEST_PARAMS_COUNT(PVE::DspCmdDirRegVm, SetVmConfig("some data"), 1)
	TEST_PARAMS_COUNT(PVE::DspCmdFsGetDiskList, SetHwFileSystemInfo("some data"), 1)
	TEST_PARAMS_COUNT(PVE::DspCmdFsGetDirectoryEntries, SetHwFileSystemInfo("some data"), 1)
	TEST_PARAMS_COUNT(PVE::DspCmdFsCreateDirectory, SetHwFileSystemInfo("some data"), 1)
	TEST_PARAMS_COUNT(PVE::DspCmdFsRemoveEntry, SetHwFileSystemInfo("some data"), 1)
	TEST_PARAMS_COUNT(PVE::DspCmdFsRenameEntry, SetHwFileSystemInfo("some data"), 1)
	TEST_PARAMS_COUNT(PVE::DspCmdUserGetLicenseInfo, SetVmEvent("some data"), 1)
	TEST_PARAMS_COUNT(PVE::DspCmdGetNetServiceStatus, SetNetServiceStatus("some data"), 1)
	TEST_PARAMS_COUNT(PVE::DspCmdGetVmInfo, SetVmEvent("some data"), 1)
	TEST_PARAMS_COUNT(PVE::DspCmdGetVmToolsInfo, SetVmEvent("some data"), 1)
	TEST_PARAMS_COUNT(PVE::DspCmdVmGetConfig, SetVmConfig("some data"), 1)
	TEST_PARAMS_COUNT(PVE::DspCmdVmGetStatistics, SetSystemStatistics("some data"), 1)
	TEST_PARAMS_COUNT(PVE::DspCmdVmUpdateSecurity, GetCommand()->addEventParameter(new CVmEventParameter(PVE::String, "some data", EVT_PARAM_WS_RESPONSE_CMD_SECURITY_INFO)), 1)
	TEST_PARAMS_COUNT(PVE::DspCmdSMCGetDispatcherRTInfo, SetVmEvent("some data"), 1)
	TEST_PARAMS_COUNT(PVE::DspCmdSMCGetCommandHistoryByVm, SetVmEvent("some data"), 1)
	TEST_PARAMS_COUNT(PVE::DspCmdSMCGetCommandHistoryByUser, SetVmEvent("some data"), 1)
	TEST_PARAMS_COUNT(PVE::DspCmdVmGetProblemReport, SetProblemReport("some data"), 1)
}

void PrivateSituationsTest::testGetParamsCountForCallsWithOneStandardParamDataAbsent()
{
	TEST_PARAMS_COUNT(PVE::DspCmdUserLogin, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdUserLoginLocalStage2, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdUserGetHostHwInfo, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdGetHostCommonInfo, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdUserGetProfile, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdGetHostStatistics, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdDirRegVm, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdFsGetDiskList, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdFsGetDirectoryEntries, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdFsCreateDirectory, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdFsRemoveEntry, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdFsRenameEntry, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdUserGetLicenseInfo, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdGetNetServiceStatus, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdGetVmInfo, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdGetVmToolsInfo, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdVmGetConfig, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdVmGetStatistics, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdVmUpdateSecurity, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdSMCGetDispatcherRTInfo, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdSMCGetCommandHistoryByVm, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdSMCGetCommandHistoryByUser, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdVmGetProblemReport, GetCommand(), 0)
}

void PrivateSituationsTest::testGetParamsCountForCallsWithMultipleArgs()
{
	TEST_PARAMS_COUNT(PVE::DspCmdLookupParallelsServers, SetParamsList(QStringList()<<"item1"), 1)
	TEST_PARAMS_COUNT(PVE::DspCmdDirGetVmList, SetParamsList(QStringList()<<"item1"<<"item2"), 2)
	TEST_PARAMS_COUNT(PVE::DspCmdStartSearchConfig, SetParamsList(QStringList()<<"item1"<<"item2"<<"item3"), 3)
	TEST_PARAMS_COUNT(PVE::DspCmdVmSectionValidateConfig, SetParamsList(QStringList()<<"item1"<<"item2"<<"item3"<<"item4"), 4)
	TEST_PARAMS_COUNT(PVE::DspCmdFsGenerateEntryName, SetParamsList(QStringList()<<"item1"), 1)
}

void PrivateSituationsTest::testGetParamsCountForCallsWithMultipleArgsDataAbsent()
{
	TEST_PARAMS_COUNT(PVE::DspCmdLookupParallelsServers, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdDirGetVmList, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdStartSearchConfig, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdVmSectionValidateConfig, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdFsGenerateEntryName, GetCommand(), 0)
}

void PrivateSituationsTest::testGetParamsCountForCallsWithoutParamsAtResponse()
{
	TEST_PARAMS_COUNT(PVE::DspCmdUserLogoff, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdDirVmEditBegin, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdSMCShutdownDispatcher, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdUserProfileBeginEdit, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdHostCommonInfoCommit, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdVmStart, GetCommand(), 0)
	TEST_PARAMS_COUNT(PVE::DspCmdVmSuspend, GetCommand(), 0)
}
