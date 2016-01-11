/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlVmManipulationsTest.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing VM manipulating SDK API.
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
#include "PrlVmManipulationsTest.h"
#include "Tests/CommonTestsUtils.h"
#include "SimpleServerWrapper.h"
#include "TestCallbackCommon.h"

#include <QFile>
#include <QTextStream>
#include <QImage>

#include <prlcommon/PrlUuid/Uuid.h>
#include "XmlModel/Messaging/CVmEvent.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"
#include "XmlModel/VmConfig/CVmGuestSharing.h"
#include "XmlModel/VmConfig/CVmHostSharing.h"
#include <prlcommon/Logging/Logging.h>
#include <prlcommon/Std/SmartPtr.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include "Libraries/DiskImage/DiskImage.h"
#include "XmlModel/Messaging/CVmEventParameter.h"
#include "XmlModel/HostHardwareInfo/CSystemStatistics.h"
#include "XmlModel/HostHardwareInfo/CHostHardwareInfo.h"
#include "XmlModel/NetworkConfig/CVirtualNetwork.h"
#include "XmlModel/NetworkConfig/CParallelsNetworkConfig.h"
#include "XmlModel/VmDataStatistic/CVmDataStatistic.h"

#include "Libraries/PrlCommonUtils/CAuthHelper.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#ifdef _LIN_
#include "Libraries/Virtuozzo/CVzHelper.h"
#endif

#include "Libraries/ProtoSerializer/CProtoCommands.h"
#include "Libraries/StatesStore/SavedStateTree.h"
#include "Libraries/PrlNetworking/netconfig.h"

#include <prlcommon/Interfaces/ParallelsSdkPrivate.h>
#ifdef _MAC_
#include "Libraries/TimeMachineHelper/CTimeMachineHelper.h"
#endif

using namespace Parallels;

#define SKIP_TEST_IN_CT_MODE \
	if (TestConfig::isCtMode()) \
		QSKIP("Skip test for Container", SkipAll);

#define SKIP_TEST_IN_CT_MODE_unimp \
	if (TestConfig::isCtMode()) \
		QSKIP("Skip testing unimplemeted functionality for Container", SkipAll);

#define READ_VM_CONFIG_INTO_BUF(vm_config_path)\
	QFile _file(vm_config_path);\
	QVERIFY(_file.open(QIODevice::ReadOnly));\
	QTextStream _stream(&_file);\
	QString _config = _stream.readAll();

#define SET_VM_TYPE \
	if (TestConfig::isCtMode()) \
		CHECK_RET_CODE_EXP(PrlVmCfg_SetVmType(m_VmHandle, PVT_CT))

#define INITIALIZE_VM(vm_config_path)\
	READ_VM_CONFIG_INTO_BUF(vm_config_path)\
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _config.toUtf8().data()))\
	APPLY_UNIQUE_VM_TEST_NAME \
	SET_VM_TYPE

#define APPLY_UNIQUE_VM_TEST_NAME\
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, QTest::currentTestFunction()))

extern qint32 PrlGetCurrentUserId();

void PrlVmManipulationsTest::initTestCase()
{
	if (TestConfig::isCtMode())
	{
		test_login();

		createCacheForDefaultTemplate();

		test_logout();
	}
}

void PrlVmManipulationsTest::init()
{
	test_login();

	m_VmHandle.reset();
	INITIALIZE_VM("./TestDspCmdDirValidateVmConfig_vm_config.xml")

	m_JobHandle.reset(PrlVm_Stop(m_VmHandle, PRL_FALSE));
	PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
	m_JobHandle.reset(PrlVm_Delete(m_VmHandle,PRL_INVALID_HANDLE));
	PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
	m_JobHandle.reset(PrlVm_Unreg(m_VmHandle));
	PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
	m_VmHandle.reset();
	m_ResultHandle.reset();
}

void PrlVmManipulationsTest::cleanup()
{
	if (m_VmHandle)
		DELETE_VM(m_VmHandle)

	if (m_ClonedVmHandle)
		DELETE_VM(m_ClonedVmHandle)

	if (m_MovedVmHandle)
		DELETE_VM(m_MovedVmHandle)

	test_logout();

	if (!m_qsFile.isEmpty())
	{
		QFile::remove(m_qsFile);
		m_qsFile.clear();
	}
}

void PrlVmManipulationsTest::testCreateVmFromConfig()
{
	PRL_UINT32 nTimeout = TestConfig::isCtMode() ? PRL_JOB_WAIT_TIMEOUT * 10 :
							PRL_JOB_WAIT_TIMEOUT;
	INITIALIZE_VM("./TestDspCmdDirValidateVmConfig_vm_config.xml")

	m_JobHandle.reset(PrlVm_RegEx(m_VmHandle, "", PACF_NON_INTERACTIVE_MODE));
	CHECK_JOB_RET_CODE_TM(m_JobHandle, nTimeout)
}

void PrlVmManipulationsTest::testStartOnNonValidVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_Start(m_VmHandle), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testStopOnNonValidVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_Stop(m_VmHandle, PRL_FALSE), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testInitiateDevStateNotificationsOnNonValidVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_InitiateDevStateNotifications(m_VmHandle), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testResetOnNonValidVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_Reset(m_VmHandle), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testPauseOnNonValidVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_Pause(m_VmHandle, PRL_FALSE), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSuspendOnNonValidVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_Suspend(m_VmHandle), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testResumeOnNonValidVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_Resume(m_VmHandle), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testDeleteOnNonValidVmHandle()
{
	m_JobHandle.reset(PrlVm_Delete(m_VmHandle,PRL_INVALID_HANDLE));
	QVERIFY(m_JobHandle != PRL_INVALID_HANDLE);
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))
	PRL_RESULT _ret_code = PRL_ERR_SUCCESS;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(m_JobHandle, &_ret_code))
	QVERIFY(_ret_code == PRL_ERR_INVALID_ARG);

	//CHECK_ASYNC_OP_FAILED(PrlVm_Delete(m_VmHandle), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testUnregOnNonValidVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_Unreg(m_VmHandle), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetConfigOnNonValidVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_RefreshConfig(m_VmHandle), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetStateOnNonValidVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_GetState(m_VmHandle), PRL_ERR_INVALID_ARG)
}

#define NEW_VM_NAME "new_vm_name"
#define NEW_VM_PATH "./new_vm_dir/vm_config.xml"

void PrlVmManipulationsTest::testCloneOnNonValidVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_Clone(m_VmHandle, NEW_VM_NAME, NEW_VM_PATH, PRL_FALSE), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testCloneOnNullVmName()
{
	QVERIFY(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()) == PRL_ERR_SUCCESS);
	CHECK_ASYNC_OP_FAILED(PrlVm_Clone(m_VmHandle, NULL, NEW_VM_PATH, PRL_FALSE), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testCloneOnNullVmPath()
{
	QVERIFY(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()) == PRL_ERR_SUCCESS);
	CHECK_ASYNC_OP_FAILED(PrlVm_Clone(m_VmHandle, NEW_VM_NAME, NULL, PRL_FALSE), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetState()
{
	testCreateVmFromConfig();
	m_JobHandle.reset(PrlVm_GetState(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(m_ResultHandle, hVmInfo.GetHandlePtr()))
	VIRTUAL_MACHINE_STATE nVmState;
	CHECK_RET_CODE_EXP(PrlVmInfo_GetState(hVmInfo, &nVmState))
	QVERIFY(nVmState == VMS_STOPPED);
}

void PrlVmManipulationsTest::testVmInfoIsInvalid_FailOnInvalidHandle()
{
	PRL_BOOL bVmIsInvalid;
	QVERIFY( PrlVmInfo_IsInvalid( m_ServerHandle, &bVmIsInvalid) == PRL_ERR_INVALID_ARG);
	QVERIFY( PrlVmInfo_IsInvalid( PRL_INVALID_HANDLE, &bVmIsInvalid) == PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testVmInfoIsInvalid()
{
	testCreateVmFromConfig();
	m_JobHandle.reset(PrlVm_GetState(m_VmHandle));

	CHECK_JOB_RET_CODE(m_JobHandle);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()));

	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(m_ResultHandle, hVmInfo.GetHandlePtr()));

	PRL_BOOL bVmIsInvalid;
	CHECK_RET_CODE_EXP(PrlVmInfo_IsInvalid( hVmInfo, &bVmIsInvalid))
	QVERIFY(bVmIsInvalid == PRL_FALSE);

	QVERIFY( PrlVmInfo_IsInvalid( hVmInfo, NULL ) == PRL_ERR_INVALID_ARG);

	PRL_RESULT nRes;
	CHECK_RET_CODE_EXP( PrlVmInfo_IsInvalidEx( hVmInfo, &bVmIsInvalid, &nRes ) );
	QVERIFY(bVmIsInvalid == PRL_FALSE);
	QCOMPARE( nRes , PRL_ERR_SUCCESS );

	QVERIFY( PrlVmInfo_IsInvalidEx( hVmInfo, &bVmIsInvalid, NULL ) == PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testVmInfoIsVmWaitingForAnswer()
{
	testCreateVmFromConfig();
	m_JobHandle.reset(PrlVm_GetState(m_VmHandle));

	CHECK_JOB_RET_CODE(m_JobHandle);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()));

	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(m_ResultHandle, hVmInfo.GetHandlePtr()));

	PRL_BOOL bIsVmWaitingForAnswer;
	CHECK_RET_CODE_EXP(PrlVmInfo_IsVmWaitingForAnswer( hVmInfo, &bIsVmWaitingForAnswer))
	QVERIFY(bIsVmWaitingForAnswer == PRL_FALSE);

	// Wrong params
	QVERIFY( PrlVmInfo_IsVmWaitingForAnswer( hVmInfo, NULL ) == PRL_ERR_INVALID_ARG);
	QVERIFY( PrlVmInfo_IsVmWaitingForAnswer( m_ServerHandle, &bIsVmWaitingForAnswer ) == PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::test_login()
{
	m_ServerHandle.reset();
	m_JobHandle.reset();
	QVERIFY(PrlSrv_Create(m_ServerHandle.GetHandlePtr()) == PRL_ERR_SUCCESS);
	if (TestConfig::isServerMode() && !TestConfig::isCtMode())
	{
		m_JobHandle.reset(PrlSrv_Login(m_ServerHandle, TestConfig::getRemoteHostName(),
			TestConfig::getUserLogin(), TestConfig::getUserPassword(), NULL, 0, 0, PSL_HIGH_SECURITY));
	}
	else
	{
		m_JobHandle.reset(PrlSrv_LoginLocal(m_ServerHandle, NULL, 0, PSL_HIGH_SECURITY));
	}
	CHECK_JOB_RET_CODE(m_JobHandle);
	CHECK_ASYNC_OP_SUCCEEDED(PrlSrv_SetNonInteractiveSession( m_ServerHandle, PRL_TRUE, 0 ))
}

void PrlVmManipulationsTest::test_logout()
{
	m_JobHandle.reset(PrlSrv_Logoff(m_ServerHandle));
	PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
	m_ServerHandle.reset();
}

bool PrlVmManipulationsTest::CheckOwnVmHandlePresentsInResult()
{
	PRL_UINT32 nParamsCount = 0;
	if (PRL_SUCCEEDED(PrlResult_GetParamsCount(m_ResultHandle, &nParamsCount)))
	{
		for (PRL_UINT32 i = 0; i < nParamsCount; ++i)
		{
			SdkHandleWrap hVm;
			if (PRL_SUCCEEDED(PrlResult_GetParamByIndex(m_ResultHandle, i, hVm.GetHandlePtr())))
				if (m_VmHandle == hVm)
					return (true);
		}
	}
	return (false);
}

void PrlVmManipulationsTest::testGettingResultOfGetVmList()
{
	testCreateVmFromConfig();
	m_JobHandle.reset(PrlSrv_GetVmListEx(m_ServerHandle, PVTF_VM|PVTF_CT));
	CHECK_JOB_RET_CODE(m_JobHandle)
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	QVERIFY(CheckOwnVmHandlePresentsInResult());
}

void PrlVmManipulationsTest::testEditVmConfig()
{
	testCreateVmFromConfig();
	PRL_VOID_PTR pVmConfData = 0;
	CHECK_RET_CODE_EXP(PrlVm_ToString(m_VmHandle, &pVmConfData))
	QString _config = UTF8_2QSTR((const char *)pVmConfData);
	PrlBuffer_Free(pVmConfData);
	CVmConfiguration _vm_conf(_config);
	QCOMPARE(_vm_conf.m_uiRcInit, PRL_ERR_SUCCESS);
	QString sNewVmName = GEN_VM_NAME_BY_TEST_FUNCTION();
	QVERIFY(sNewVmName != _vm_conf.getVmIdentification()->getVmName());
	m_JobHandle.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)
	_vm_conf.getVmIdentification()->setVmName(sNewVmName);
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()))
	m_JobHandle.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)
	m_JobHandle.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)
	pVmConfData = 0;
	CHECK_RET_CODE_EXP(PrlVm_ToString(m_VmHandle, &pVmConfData))
	_config = UTF8_2QSTR((const char *)pVmConfData);
	PrlBuffer_Free(pVmConfData);
	CHECK_RET_CODE_EXP(_vm_conf.fromString(_config))
	QCOMPARE(_vm_conf.getVmIdentification()->getVmName(), sNewVmName);
}

void PrlVmManipulationsTest::testEditVmConfigWithCritcalErrorConfigValidate()
{
	SKIP_TEST_IN_CT_MODE_unimp

	testCreateVmFromConfig();

	PRL_VOID_PTR pVmConfData = 0;
	CHECK_RET_CODE_EXP(PrlVm_ToString(m_VmHandle, &pVmConfData))
	QString _config = UTF8_2QSTR((const char *)pVmConfData);
	PrlBuffer_Free(pVmConfData);

	CVmConfiguration _vm_conf(_config);
	QCOMPARE(_vm_conf.m_uiRcInit, PRL_ERR_SUCCESS);

	m_JobHandle.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)

	_vm_conf.getVmHardwareList()->getCpu()->setNumber(0);

	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()))

	m_JobHandle.reset(PrlVm_Commit(m_VmHandle));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT));
	PRL_RESULT nValidationResult = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(m_JobHandle, &nValidationResult))

	QVERIFY(nValidationResult == PRL_ERR_INCONSISTENCY_VM_CONFIG);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()));

	CHECK_PARAMS_COUNT(hResult, 1)

	SdkHandleWrap hProblemDescription;
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, hProblemDescription.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hProblemDescription, PHT_EVENT)

	PRL_RESULT nProblemErrCode = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hProblemDescription, &nProblemErrCode))
	QVERIFY(nProblemErrCode == PRL_ERR_VMCONF_CPU_ZERO_COUNT);
}

#define INIT_VM_CONF_FROM_FILE\
	QFile _file("./TestDspCmdDirValidateVmConfig_vm_config.xml");\
	QVERIFY(_file.open(QIODevice::ReadOnly));\
	CVmConfiguration _vm_conf(&_file);\
	QVERIFY(_vm_conf.getVmIdentification());\
	_vm_conf.getVmIdentification()->setVmName(QTest::currentTestFunction());

void PrlVmManipulationsTest::testGetVmName()
{
	testCreateVmFromConfig();
	PRL_CHAR sVmName[STR_BUF_LENGTH];
	PRL_UINT32 nVmNameLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetName(m_VmHandle, sVmName, &nVmNameLength))
	INIT_VM_CONF_FROM_FILE
	QCOMPARE(UTF8_2QSTR(sVmName), _vm_conf.getVmIdentification()->getVmName());
	QVERIFY(nVmNameLength == size_t(_vm_conf.getVmIdentification()->getVmName().toUtf8().size()+1));
}

void PrlVmManipulationsTest::testGetVmNameNotEnoughBufSize()
{
	testCreateVmFromConfig();
	PRL_CHAR sVmName[STR_BUF_LENGTH];
	INIT_VM_CONF_FROM_FILE
	PRL_UINT32 nVmNameLength = _vm_conf.getVmIdentification()->getVmName().size();
	PRL_RESULT nRetCode = PrlVmCfg_GetName(m_VmHandle, sVmName, &nVmNameLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmManipulationsTest::testGetVmNameNullBufSize()
{
	testCreateVmFromConfig();
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(m_VmHandle, PrlVmCfg_GetName)
}

void PrlVmManipulationsTest::testSetVmName()
{
	testCreateVmFromConfig();
	QString sNewVmName = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, sNewVmName.toUtf8().data()))
	PRL_CHAR sVmName[STR_BUF_LENGTH];
	PRL_UINT32 nVmNameLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetName(m_VmHandle, sVmName, &nVmNameLength))
	QCOMPARE(UTF8_2QSTR(sVmName), sNewVmName);
}

void PrlVmManipulationsTest::testSetVmNameTryToSetEmptyStringValue()
{
	testCreateVmFromConfig();
	QString sNewVmName = "";
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, sNewVmName.toUtf8().data()))
	PRL_CHAR sVmName[STR_BUF_LENGTH];
	PRL_UINT32 nVmNameLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetName(m_VmHandle, sVmName, &nVmNameLength))
	QCOMPARE(UTF8_2QSTR(sVmName), sNewVmName);
}

void PrlVmManipulationsTest::testGetVmHostname()
{
	testCreateVmFromConfig();
	QString sVmHostname;
	PRL_EXTRACT_STRING_VALUE(sVmHostname, m_VmHandle, PrlVmCfg_GetHostname)
	INIT_VM_CONF_FROM_FILE
	QCOMPARE(sVmHostname, _vm_conf.getVmSettings()->getGlobalNetwork()->getHostName());
}

void PrlVmManipulationsTest::testGetVmHostnameNullBufSize()
{
	testCreateVmFromConfig();
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(m_VmHandle, PrlVmCfg_GetHostname)
}

void PrlVmManipulationsTest::testSetVmHostname()
{
	testCreateVmFromConfig();
	QString sNewVmHostname = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostname(m_VmHandle, sNewVmHostname.toUtf8().data()))
	QString sVmHostname;
	PRL_EXTRACT_STRING_VALUE(sVmHostname, m_VmHandle, PrlVmCfg_GetHostname)
	QCOMPARE(sVmHostname, sNewVmHostname);
}

void PrlVmManipulationsTest::testGetVmDefaultGateway()
{
	testCreateVmFromConfig();
	QString sVmDefaultGateway;
	PRL_EXTRACT_STRING_VALUE(sVmDefaultGateway, m_VmHandle, PrlVmCfg_GetDefaultGateway)
	INIT_VM_CONF_FROM_FILE
	QCOMPARE(sVmDefaultGateway, _vm_conf.getVmSettings()->getGlobalNetwork()->getDefaultGateway());
}

void PrlVmManipulationsTest::testGetVmDefaultGatewayNullBufSize()
{
	testCreateVmFromConfig();
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(m_VmHandle, PrlVmCfg_GetDefaultGateway)
}

void PrlVmManipulationsTest::testGetVmDefaultGatewayInvalidHandle()
{
	PRL_UINT32 nBufSize = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetDefaultGateway(PRL_INVALID_HANDLE, 0, &nBufSize), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetVmDefaultGatewayNonVmCfgHandle()
{
	PRL_UINT32 nBufSize = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetDefaultGateway(m_ServerHandle, 0, &nBufSize), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetVmDefaultGateway()
{
	testCreateVmFromConfig();
	QString sNewVmDefaultGateway = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultGateway(m_VmHandle, sNewVmDefaultGateway.toUtf8().data()))
	QString sVmDefaultGateway;
	PRL_EXTRACT_STRING_VALUE(sVmDefaultGateway, m_VmHandle, PrlVmCfg_GetDefaultGateway)
	QCOMPARE(sVmDefaultGateway, sNewVmDefaultGateway);
}

void PrlVmManipulationsTest::testSetVmDefaultGatewayInvalidHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetDefaultGateway(PRL_INVALID_HANDLE, "default gateway value"), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetVmDefaultGatewayNonVmCfgHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetDefaultGateway(m_ServerHandle, "default gateway value"), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetVmDefaultGatewayIPv6()
{
	testCreateVmFromConfig();

	QString sVmDefaultGatewayIPv6 = "abc";
	PRL_EXTRACT_STRING_VALUE(sVmDefaultGatewayIPv6, m_VmHandle, PrlVmCfg_GetDefaultGatewayIPv6)
	INIT_VM_CONF_FROM_FILE;
	QCOMPARE(sVmDefaultGatewayIPv6, _vm_conf.getVmSettings()->getGlobalNetwork()->getDefaultGatewayIPv6());
}

void PrlVmManipulationsTest::testGetVmDefaultGatewayIPv6OnWrongParams()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultGatewayIPv6(m_VmHandle, TEST_DEFAULT_GATEWAY_IPv6));

	// Invalid handle
	PRL_UINT32 nBufSize = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetDefaultGatewayIPv6(m_ServerHandle, 0, &nBufSize),
		PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetDefaultGatewayIPv6(m_VmHandle, 0, 0),
		PRL_ERR_INVALID_ARG);
	// Null buffer size
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(m_VmHandle, PrlVmCfg_GetDefaultGatewayIPv6);
	// Not enough buffer size
	PRL_CHAR sDefaultGatewayIPv6[3];
	PRL_UINT32 nLength = 3;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetDefaultGatewayIPv6(m_VmHandle, sDefaultGatewayIPv6, &nLength),
		PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmManipulationsTest::testSetVmDefaultGatewayIPv6()
{
	testCreateVmFromConfig();

	QString sNewVmDefaultGatewayIPv6 = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultGatewayIPv6(m_VmHandle, QSTR2UTF8(sNewVmDefaultGatewayIPv6)));
	QString sVmDefaultGatewayIPv6;
	PRL_EXTRACT_STRING_VALUE(sVmDefaultGatewayIPv6, m_VmHandle, PrlVmCfg_GetDefaultGatewayIPv6);
	QCOMPARE(sVmDefaultGatewayIPv6, sNewVmDefaultGatewayIPv6);
}

void PrlVmManipulationsTest::testSetVmDefaultGatewayIPv6OnWrongParams()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	// Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
		PrlVmDevNet_SetDefaultGatewayIPv6(m_ServerHandle, TEST_DEFAULT_GATEWAY_IPv6),
		PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testGetVmUuid()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();
	PRL_CHAR sVmUuid[STR_BUF_LENGTH];
	PRL_UINT32 nVmUuidLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUuid(m_VmHandle, sVmUuid, &nVmUuidLength))
	INIT_VM_CONF_FROM_FILE
	QCOMPARE(UTF8_2QSTR(sVmUuid), _vm_conf.getVmIdentification()->getVmUuid());
	QVERIFY(nVmUuidLength == size_t(_vm_conf.getVmIdentification()->getVmUuid().toUtf8().size()+1));
}

void PrlVmManipulationsTest::testSetVmUuid()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();
	QString sNewVmUuid = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP(PrlVmCfg_SetUuid(m_VmHandle, sNewVmUuid.toUtf8().data()))
	PRL_CHAR sVmUuid[STR_BUF_LENGTH];
	PRL_UINT32 nVmUuidLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUuid(m_VmHandle, sVmUuid, &nVmUuidLength))
	QCOMPARE(UTF8_2QSTR(sVmUuid), sNewVmUuid);
}

void PrlVmManipulationsTest::testSetVmUuidTryToSetEmptyStringValue()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();
	QString sNewVmUuid = "";
	CHECK_RET_CODE_EXP(PrlVmCfg_SetUuid(m_VmHandle, sNewVmUuid.toUtf8().data()))
	PRL_CHAR sVmUuid[STR_BUF_LENGTH];
	PRL_UINT32 nVmUuidLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUuid(m_VmHandle, sVmUuid, &nVmUuidLength))
	QCOMPARE(UTF8_2QSTR(sVmUuid), sNewVmUuid);
}

void PrlVmManipulationsTest::testGetVmUuidNotEnoughBufSize()
{
	testCreateVmFromConfig();
	PRL_CHAR sVmUuid[STR_BUF_LENGTH];
	INIT_VM_CONF_FROM_FILE
	PRL_UINT32 nVmUuidLength = _vm_conf.getVmIdentification()->getVmUuid().size();
	PRL_RESULT nRetCode = PrlVmCfg_GetUuid(m_VmHandle, sVmUuid, &nVmUuidLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmManipulationsTest::testGetVmUuidNullBufSize()
{
	testCreateVmFromConfig();
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(m_VmHandle, PrlVmCfg_GetUuid)
}

void PrlVmManipulationsTest::testGetVmOsType()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();
	PRL_UINT32 nOsType = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetOsType(m_VmHandle, &nOsType))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(nOsType == _vm_conf.getVmSettings()->getVmCommonOptions()->getOsType());
}

#define CREATE_TEST_VM\
	SdkHandleWrap hVm;\
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))


void PrlVmManipulationsTest::testGetVmOsVersion()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();
	PRL_UINT32 nOsVersion = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetOsVersion(m_VmHandle, &nOsVersion))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(nOsVersion == _vm_conf.getVmSettings()->getVmCommonOptions()->getOsVersion());
}

#define TEST_SET_OS_VERSION(expected_os_version)\
	CREATE_TEST_VM\
	PRL_UINT32 nGuestOsVersion = expected_os_version;\
	QVERIFY(PRL_SUCCEEDED(PrlVmCfg_SetOsVersion(hVm, nGuestOsVersion)));\
	PRL_UINT32 nActualGuestOsVersion = PVS_GUEST_TYPE_OTHER;\
	QVERIFY(PRL_SUCCEEDED(PrlVmCfg_GetOsVersion(hVm, &nActualGuestOsVersion)));\
	QCOMPARE(nActualGuestOsVersion, nGuestOsVersion);

void PrlVmManipulationsTest::testSetVmGuestOsVersion()
{
	TEST_SET_OS_VERSION(PVS_GUEST_VER_LIN_REDHAT)
	QVERIFY(PVS_GET_GUEST_TYPE(nActualGuestOsVersion) == PVS_GUEST_TYPE_LINUX);
}

void PrlVmManipulationsTest::testSetVmGuestOsVersionOnMacosTiger()
{
	TEST_SET_OS_VERSION(PVS_GUEST_VER_MACOS_TIGER)
	QVERIFY(PVS_GET_GUEST_TYPE(nActualGuestOsVersion) == PVS_GUEST_TYPE_MACOS);
}

void PrlVmManipulationsTest::testSetVmGuestOsVersionOnMacosLeopard()
{
	TEST_SET_OS_VERSION(PVS_GUEST_VER_MACOS_LEOPARD)
	QVERIFY(PVS_GET_GUEST_TYPE(nActualGuestOsVersion) == PVS_GUEST_TYPE_MACOS);
}

void PrlVmManipulationsTest::testSetVmGuestOsVersionWrongOsVersion()
{
	CREATE_TEST_VM
	QVERIFY(!PRL_SUCCEEDED(PrlVmCfg_SetOsVersion(hVm, 0xFFFFFFFF)));
}

void PrlVmManipulationsTest::testGetVmRamSize()
{
	testCreateVmFromConfig();
	PRL_UINT32 nVmRamSize = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetRamSize(m_VmHandle, &nVmRamSize))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(nVmRamSize == _vm_conf.getVmHardwareList()->getMemory()->getRamSize());
}

void PrlVmManipulationsTest::testSetVmRamSize()
{
	CREATE_TEST_VM
	PRL_UINT32 nVmRamSize = 4096;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetRamSize(hVm, nVmRamSize))
	PRL_UINT32 nActualVmRamSize = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetRamSize(hVm, &nActualVmRamSize))
	QCOMPARE(nActualVmRamSize, nVmRamSize);
}

void PrlVmManipulationsTest::testSetVmRamSizeNullRamSize()
{
	CREATE_TEST_VM
	QVERIFY(!PRL_SUCCEEDED(PrlVmCfg_SetRamSize(hVm, 0)));
}

void PrlVmManipulationsTest::testCreateVmUuidAutoGenerated()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))
	PRL_CHAR sVmUuid[STR_BUF_LENGTH];
	PRL_UINT32 nVmUuidLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUuid(m_VmHandle, sVmUuid, &nVmUuidLength))
	QVERIFY(!UTF8_2QSTR(sVmUuid).isEmpty());
}

void PrlVmManipulationsTest::testGetVmVideoRamSize()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();
	PRL_UINT32 nVmVideoRamSize = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetVideoRamSize(m_VmHandle, &nVmVideoRamSize))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(nVmVideoRamSize == _vm_conf.getVmHardwareList()->getVideo()->getMemorySize());
}

void PrlVmManipulationsTest::testSetVmVideoRamSize()
{
	CREATE_TEST_VM
	PRL_UINT32 nVmVideoRamSize = 4096;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVideoRamSize(hVm, nVmVideoRamSize))
	PRL_UINT32 nActualVmVideoRamSize = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetVideoRamSize(hVm, &nActualVmVideoRamSize))
	QCOMPARE(nActualVmVideoRamSize, nVmVideoRamSize);
}

void PrlVmManipulationsTest::testSetVmVideoRamSizeNullVideoRamSize()
{
	CREATE_TEST_VM
	QVERIFY(!PRL_SUCCEEDED(PrlVmCfg_SetVideoRamSize(hVm, 0)));
}

void PrlVmManipulationsTest::testGetHostMemQuotaMin()
{
	testCreateVmFromConfig();
	PRL_UINT32 nHostMemQuotaMin = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHostMemQuotaMin(m_VmHandle, &nHostMemQuotaMin))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(nHostMemQuotaMin == _vm_conf.getVmHardwareList()->getMemory()->getHostMemQuotaMin());
}

void PrlVmManipulationsTest::testSetHostMemQuotaMin()
{
	CREATE_TEST_VM
	PRL_UINT32 nHostMemQuotaMin = 4096;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemQuotaMin(hVm, nHostMemQuotaMin))
	PRL_UINT32 nActualHostMemQuotaMin = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHostMemQuotaMin(hVm, &nActualHostMemQuotaMin))
	QCOMPARE(nActualHostMemQuotaMin, nHostMemQuotaMin);
}

void PrlVmManipulationsTest::testHostMemQuotaMinOnWrongParams()
{
	testCreateVmFromConfig();
	PRL_UINT32 nHostMemQuotaMin;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetHostMemQuotaMin(PRL_INVALID_HANDLE,
				&nHostMemQuotaMin), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetHostMemQuotaMin(m_ServerHandle,
				&nHostMemQuotaMin), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetHostMemQuotaMin(m_VmHandle, 0), PRL_ERR_INVALID_ARG)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetHostMemQuotaMin(PRL_INVALID_HANDLE, 4096), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetHostMemQuotaMin(m_ServerHandle, 4096), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetHostMemQuotaMax()
{
	testCreateVmFromConfig();
	PRL_UINT32 nHostMemQuotaMax = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHostMemQuotaMax(m_VmHandle, &nHostMemQuotaMax))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(nHostMemQuotaMax == _vm_conf.getVmHardwareList()->getMemory()->getHostMemQuotaMax());
}

void PrlVmManipulationsTest::testSetHostMemQuotaMax()
{
	CREATE_TEST_VM
	PRL_UINT32 nHostMemQuotaMax = 4096;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemQuotaMax(hVm, nHostMemQuotaMax))
	PRL_UINT32 nActualHostMemQuotaMax = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHostMemQuotaMax(hVm, &nActualHostMemQuotaMax))
	QCOMPARE(nActualHostMemQuotaMax, nHostMemQuotaMax);
}

void PrlVmManipulationsTest::testHostMemQuotaMaxOnWrongParams()
{
	testCreateVmFromConfig();
	PRL_UINT32 nHostMemQuotaMax;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetHostMemQuotaMax(PRL_INVALID_HANDLE,
				&nHostMemQuotaMax), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetHostMemQuotaMax(m_ServerHandle,
				&nHostMemQuotaMax), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetHostMemQuotaMax(m_VmHandle, 0), PRL_ERR_INVALID_ARG)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetHostMemQuotaMax(PRL_INVALID_HANDLE, 4096), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetHostMemQuotaMax(m_ServerHandle, 4096), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetHostMemQuotaPriority()
{
	testCreateVmFromConfig();
	PRL_UINT32 nHostMemQuotaPriority = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHostMemQuotaPriority(m_VmHandle, &nHostMemQuotaPriority))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(nHostMemQuotaPriority == _vm_conf.getVmHardwareList()->getMemory()->getHostMemQuotaPriority());
}

void PrlVmManipulationsTest::testSetHostMemQuotaPriority()
{
	CREATE_TEST_VM
	PRL_UINT32 nHostMemQuotaPriority = 4096;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemQuotaPriority(hVm, nHostMemQuotaPriority))
	PRL_UINT32 nActualHostMemQuotaPriority = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHostMemQuotaPriority(hVm, &nActualHostMemQuotaPriority))
	QCOMPARE(nActualHostMemQuotaPriority, nHostMemQuotaPriority);
}

void PrlVmManipulationsTest::testHostMemQuotaPriorityOnWrongParams()
{
	testCreateVmFromConfig();
	PRL_UINT32 nHostMemQuotaPriority;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetHostMemQuotaPriority(PRL_INVALID_HANDLE,
				&nHostMemQuotaPriority), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetHostMemQuotaPriority(m_ServerHandle,
				&nHostMemQuotaPriority), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetHostMemQuotaPriority(m_VmHandle, 0), PRL_ERR_INVALID_ARG)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetHostMemQuotaPriority(PRL_INVALID_HANDLE, 4096), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetHostMemQuotaPriority(m_ServerHandle, 4096), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testIsHostMemAutoQuota()
{
	testCreateVmFromConfig();
	PRL_BOOL bHostMemAutoQuota = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsHostMemAutoQuota(m_VmHandle, &bHostMemAutoQuota))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bHostMemAutoQuota == PRL_BOOL(_vm_conf.getVmHardwareList()->getMemory()->isAutoQuota()));
}

void PrlVmManipulationsTest::testSetHostMemAutoQuota()
{
	CREATE_TEST_VM
	PRL_BOOL bHostMemAutoQuota = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsHostMemAutoQuota(hVm, &bHostMemAutoQuota))
	bHostMemAutoQuota = !bHostMemAutoQuota;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostMemAutoQuota(hVm, bHostMemAutoQuota))
	PRL_UINT32 bActualHostMemAutoQuota = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsHostMemAutoQuota(hVm, &bActualHostMemAutoQuota))
	QCOMPARE(bActualHostMemAutoQuota, bHostMemAutoQuota);
}

void PrlVmManipulationsTest::testHostMemAutoQuotaOnWrongParams()
{
	testCreateVmFromConfig();
	PRL_BOOL bHostMemAutoQuota;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsHostMemAutoQuota(PRL_INVALID_HANDLE,
				&bHostMemAutoQuota), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsHostMemAutoQuota(m_ServerHandle,
				&bHostMemAutoQuota), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsHostMemAutoQuota(m_VmHandle, 0), PRL_ERR_INVALID_ARG)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetHostMemAutoQuota(PRL_INVALID_HANDLE,
				PRL_FALSE), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetHostMemAutoQuota(m_ServerHandle,
				PRL_FALSE), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetMaxBalloonSize()
{
	testCreateVmFromConfig();
	PRL_UINT32 nMaxBalloonSize = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetMaxBalloonSize(m_VmHandle, &nMaxBalloonSize))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(nMaxBalloonSize == _vm_conf.getVmHardwareList()->getMemory()->getMaxBalloonSize());
}

void PrlVmManipulationsTest::testSetMaxBalloonSize()
{
	CREATE_TEST_VM
	PRL_UINT32 nMaxBalloonSize = 55;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetMaxBalloonSize(hVm, nMaxBalloonSize))
	PRL_UINT32 nActualMaxBalloonSize = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetMaxBalloonSize(hVm, &nActualMaxBalloonSize))
	QCOMPARE(nActualMaxBalloonSize, nMaxBalloonSize);
}

void PrlVmManipulationsTest::testMaxBalloonSizeOnWrongParams()
{
	testCreateVmFromConfig();
	PRL_UINT32 nMaxBalloonSize;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetMaxBalloonSize(PRL_INVALID_HANDLE,
				&nMaxBalloonSize), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetMaxBalloonSize(m_ServerHandle,
				&nMaxBalloonSize), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetMaxBalloonSize(m_VmHandle, 0), PRL_ERR_INVALID_ARG)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetMaxBalloonSize(PRL_INVALID_HANDLE, 77), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetMaxBalloonSize(m_ServerHandle, 77), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetVmCpuCount()
{
	testCreateVmFromConfig();
	PRL_UINT32 nVmCpuCount = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetCpuCount(m_VmHandle, &nVmCpuCount))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(nVmCpuCount == _vm_conf.getVmHardwareList()->getCpu()->getNumber());
}

void PrlVmManipulationsTest::testSetVmCpuCount()
{
	CREATE_TEST_VM
	PRL_UINT32 nVmCpuCount = 2;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetCpuCount(hVm, nVmCpuCount))
	PRL_UINT32 nActualVmCpuCount = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetCpuCount(hVm, &nActualVmCpuCount))
	QCOMPARE(nActualVmCpuCount, nVmCpuCount);
}

void PrlVmManipulationsTest::testGetVmCpuMode()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();
	PRL_CPU_MODE nVmCpuMode;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetCpuMode(m_VmHandle, &nVmCpuMode))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(nVmCpuMode == _vm_conf.getVmHardwareList()->getCpu()->getMode());
}

void PrlVmManipulationsTest::testSetVmCpuMode()
{
	CREATE_TEST_VM
	PRL_CPU_MODE nVmCpuMode = PCM_CPU_MODE_64;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetCpuMode(hVm, nVmCpuMode))
	PRL_CPU_MODE nActualVmCpuMode;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetCpuMode(hVm, &nActualVmCpuMode))
	QCOMPARE(nActualVmCpuMode, nVmCpuMode);
}

void PrlVmManipulationsTest::testGetVmCpuAccelLevel()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();
	PRL_VM_ACCELERATION_LEVEL nVmCpuAccelLevel;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetCpuAccelLevel(m_VmHandle, &nVmCpuAccelLevel))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(nVmCpuAccelLevel == _vm_conf.getVmHardwareList()->getCpu()->getAccelerationLevel());
}

void PrlVmManipulationsTest::testSetVmCpuAccelLevel()
{
	CREATE_TEST_VM
	PRL_VM_ACCELERATION_LEVEL nVmCpuAccelLevel = PVA_ACCELERATION_NORMAL;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetCpuAccelLevel(hVm, nVmCpuAccelLevel))
	PRL_VM_ACCELERATION_LEVEL nActualVmCpuAccelLevel;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetCpuAccelLevel(hVm, &nActualVmCpuAccelLevel))
	QCOMPARE(nActualVmCpuAccelLevel, nVmCpuAccelLevel);
}

void PrlVmManipulationsTest::testIsVmCpuVtxEnabled()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();
	PRL_BOOL bVmCpuVtxEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsCpuVtxEnabled(m_VmHandle, &bVmCpuVtxEnabled))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bVmCpuVtxEnabled == PRL_BOOL(_vm_conf.getVmHardwareList()->getCpu()->getEnableVTxSupport()));
}

void PrlVmManipulationsTest::testSetVmCpuVtxEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bVmCpuVtxEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsCpuVtxEnabled(hVm, &bVmCpuVtxEnabled))
	bVmCpuVtxEnabled = !bVmCpuVtxEnabled;
	CHECK_RET_CODE_EXP(PrlVm_SetCpuVtxEnabled(hVm, bVmCpuVtxEnabled))
	PRL_BOOL bActualVmCpuVtxEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsCpuVtxEnabled(hVm, &bActualVmCpuVtxEnabled))
	QCOMPARE(bActualVmCpuVtxEnabled, bVmCpuVtxEnabled);
}

void PrlVmManipulationsTest::testVmCpuVtxEnabledOnWrongParams()
{
	testCreateVmFromConfig();
	PRL_BOOL bVmCpuVtxEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsCpuVtxEnabled(PRL_INVALID_HANDLE, &bVmCpuVtxEnabled), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsCpuVtxEnabled(m_ServerHandle, &bVmCpuVtxEnabled), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsCpuVtxEnabled(m_VmHandle, 0), PRL_ERR_INVALID_ARG)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVm_SetCpuVtxEnabled(PRL_INVALID_HANDLE, PRL_FALSE), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVm_SetCpuVtxEnabled(m_ServerHandle, PRL_FALSE), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testIsVmCpuHotplugEnabled()
{
	testCreateVmFromConfig();
	PRL_BOOL bCpuHotplugEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsCpuHotplugEnabled(m_VmHandle, &bCpuHotplugEnabled))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bCpuHotplugEnabled == PRL_BOOL(_vm_conf.getVmHardwareList()->getCpu()->isEnableHotplug()));
}

void PrlVmManipulationsTest::testSetVmCpuHotplugEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bCpuHotplugEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsCpuHotplugEnabled(hVm, &bCpuHotplugEnabled))
	bCpuHotplugEnabled = !bCpuHotplugEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetCpuHotplugEnabled(hVm, bCpuHotplugEnabled))
	PRL_BOOL bActualCpuHotplugEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsCpuHotplugEnabled(hVm, &bActualCpuHotplugEnabled))
	QCOMPARE(bActualCpuHotplugEnabled, bCpuHotplugEnabled);
}

void PrlVmManipulationsTest::testVmCpuHotplugEnabledOnWrongParams()
{
	testCreateVmFromConfig();
	PRL_BOOL bCpuHotplugEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
		PrlVmCfg_IsCpuHotplugEnabled(PRL_INVALID_HANDLE, &bCpuHotplugEnabled),
		PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
		PrlVmCfg_IsCpuHotplugEnabled(m_ServerHandle, &bCpuHotplugEnabled),
		PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
		PrlVmCfg_IsCpuHotplugEnabled(m_VmHandle, 0),
		PRL_ERR_INVALID_ARG)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(
		PrlVmCfg_SetCpuHotplugEnabled(PRL_INVALID_HANDLE, PRL_FALSE),
		PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
		PrlVmCfg_SetCpuHotplugEnabled(m_ServerHandle, PRL_FALSE),
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testIs3DAccelerationEnabled()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();
	PRL_BOOL b3DAccelerationEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_Is3DAccelerationEnabled(m_VmHandle, &b3DAccelerationEnabled))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(b3DAccelerationEnabled
		== PRL_BOOL(_vm_conf.getVmHardwareList()->getVideo()->getEnable3DAcceleration() != P3D_DISABLED));
}

void PrlVmManipulationsTest::testSet3DAccelerationEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL b3DAccelerationEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_Is3DAccelerationEnabled(hVm, &b3DAccelerationEnabled))
	b3DAccelerationEnabled = !b3DAccelerationEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_Set3DAccelerationEnabled(hVm, b3DAccelerationEnabled))
	PRL_BOOL bActual3DAccelerationEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_Is3DAccelerationEnabled(hVm, &bActual3DAccelerationEnabled))
	QCOMPARE(bActual3DAccelerationEnabled, b3DAccelerationEnabled);
}

void PrlVmManipulationsTest::test3DAccelerationEnabledOnWrongParams()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();
	PRL_BOOL b3DAccelerationEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_Is3DAccelerationEnabled(PRL_INVALID_HANDLE, &b3DAccelerationEnabled), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_Is3DAccelerationEnabled(m_ServerHandle, &b3DAccelerationEnabled), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_Is3DAccelerationEnabled(m_VmHandle, 0), PRL_ERR_INVALID_ARG)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_Set3DAccelerationEnabled(PRL_INVALID_HANDLE, PRL_FALSE), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_Set3DAccelerationEnabled(m_ServerHandle, PRL_FALSE), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGet3DAccelerationMode()
{
	testCreateVmFromConfig();
	PRL_VIDEO_3D_ACCELERATION n3DAccelerationMode;
	CHECK_RET_CODE_EXP(PrlVmCfg_Get3DAccelerationMode(m_VmHandle, &n3DAccelerationMode))
	INIT_VM_CONF_FROM_FILE
	QCOMPARE(quint32(n3DAccelerationMode),
			 quint32(_vm_conf.getVmHardwareList()->getVideo()->getEnable3DAcceleration()));
}

void PrlVmManipulationsTest::testSet3DAccelerationMode()
{
	CREATE_TEST_VM
	PRL_VIDEO_3D_ACCELERATION n3DAccelerationMode;
	CHECK_RET_CODE_EXP(PrlVmCfg_Get3DAccelerationMode(hVm, &n3DAccelerationMode))
	n3DAccelerationMode = (PRL_VIDEO_3D_ACCELERATION )(1 + n3DAccelerationMode);
	CHECK_RET_CODE_EXP(PrlVmCfg_Set3DAccelerationMode(hVm, n3DAccelerationMode))
	PRL_VIDEO_3D_ACCELERATION nActual3DAccelerationMode;
	CHECK_RET_CODE_EXP(PrlVmCfg_Get3DAccelerationMode(hVm, &nActual3DAccelerationMode))
	QCOMPARE(quint32(nActual3DAccelerationMode), quint32(n3DAccelerationMode));
}

void PrlVmManipulationsTest::test3DAccelerationModeOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_VIDEO_3D_ACCELERATION n3DAccelerationMode;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_Get3DAccelerationMode(m_ServerHandle, &n3DAccelerationMode),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_Set3DAccelerationMode(m_ServerHandle, P3D_DISABLED),
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_Get3DAccelerationMode(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

#define RECEIVE_UPDATED_VM_CONFIG\
	testCreateVmFromConfig();\
	m_JobHandle.reset(PrlVm_BeginEdit(m_VmHandle));\
	CHECK_JOB_RET_CODE(m_JobHandle)\
	QString sVmConfiguration;\
	{\
		PRL_VOID_PTR pVmConfig = 0;\
		QVERIFY(PRL_SUCCEEDED(PrlVm_ToString(m_VmHandle, &pVmConfig)));\
		CVmConfiguration _vm_conf(UTF8_2QSTR((const char *)pVmConfig));\
		PrlBuffer_Free(pVmConfig);\
		QVERIFY(PRL_SUCCEEDED(_vm_conf.m_uiRcInit));\
		_vm_conf.getVmIdentification()->setServerUuid(Uuid().createUuid().toString());\
		_vm_conf.getVmIdentification()->setServerHost(Uuid().createUuid().toString());\
		_vm_conf.getVmSettings()->getVmCommonOptions()->setIcon("some VM icon");\
		_vm_conf.getVmSettings()->getVmCommonOptions()->setVmDescription("some VM description");\
		_vm_conf.getVmSettings()->getVmRuntimeOptions()->setSystemFlags("value1=magic_num");\
		CVmSharedFolder *pShare = new CVmSharedFolder;\
		pShare->setName("share1");\
		pShare->setPath("some share path");\
		pShare->setDescription("some share description");\
		_vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->addSharedFolder(pShare);\
		CVmScreenResolution *pScrRes = new CVmScreenResolution;\
		pScrRes->setWidth(1260);\
		pScrRes->setHeight(800);\
		_vm_conf.getVmHardwareList()->getVideo()->getVmScreenResolutions()->addScreenResolution(pScrRes);\
		CVmStartupOptions::CVmBootDevice *pBootDev = new CVmStartupOptions::CVmBootDevice;\
		_vm_conf.getVmSettings()->getVmStartupOptions()->m_lstBootDeviceList.append(pBootDev);\
		QVERIFY(PRL_SUCCEEDED(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data())));\
		sVmConfiguration = _vm_conf.toString();\
	}\
	m_JobHandle.reset(PrlVm_Commit(m_VmHandle));\
	CHECK_JOB_RET_CODE(m_JobHandle)

#define EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(hVm)\
	PRL_VOID_PTR pVmConfig = 0;\
	QVERIFY(PRL_SUCCEEDED(PrlVm_ToString(hVm, &pVmConfig)));\
	QString sVmConfig = UTF8_2QSTR((const char *)pVmConfig);\
	PrlBuffer_Free(pVmConfig);\
	CVmConfiguration _vm_conf(sVmConfig);\
	CHECK_RET_CODE_EXP(_vm_conf.m_uiRcInit)

void PrlVmManipulationsTest::testGetServerUuid()
{
	RECEIVE_UPDATED_VM_CONFIG
	PRL_CHAR sServerUuid[STR_BUF_LENGTH];
	PRL_UINT32 nServerUuidLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetServerUuid(m_VmHandle, sServerUuid, &nServerUuidLength))
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	QCOMPARE(UTF8_2QSTR(sServerUuid), _vm_conf.getVmIdentification()->getServerUuid());
	QVERIFY(nServerUuidLength == size_t(_vm_conf.getVmIdentification()->getServerUuid().toUtf8().size()+1));
}

void PrlVmManipulationsTest::testGetServerUuidNotEnoughBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG
	PRL_CHAR sServerUuid[STR_BUF_LENGTH];
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	PRL_UINT32 nServerUuidLength = _vm_conf.getVmIdentification()->getServerUuid().size();
	PRL_RESULT nRetCode = PrlVmCfg_GetServerUuid(m_VmHandle, sServerUuid, &nServerUuidLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmManipulationsTest::testGetServerUuidNullBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG;
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(m_VmHandle, PrlVmCfg_GetServerUuid)
}

void PrlVmManipulationsTest::testGetServerHost()
{
	RECEIVE_UPDATED_VM_CONFIG
	PRL_CHAR sServerHost[STR_BUF_LENGTH];
	PRL_UINT32 nServerHostLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetServerHost(m_VmHandle, sServerHost, &nServerHostLength))
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	QCOMPARE(UTF8_2QSTR(sServerHost), _vm_conf.getVmIdentification()->getServerHost());
	QVERIFY(nServerHostLength == size_t(_vm_conf.getVmIdentification()->getServerHost().toUtf8().size()+1));
}

void PrlVmManipulationsTest::testGetServerHostNotEnoughBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG
	PRL_CHAR sServerHost[STR_BUF_LENGTH];
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	PRL_UINT32 nServerHostLength = _vm_conf.getVmIdentification()->getServerHost().size();
	PRL_RESULT nRetCode = PrlVmCfg_GetServerHost(m_VmHandle, sServerHost, &nServerHostLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmManipulationsTest::testGetServerHostNullBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG;
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(m_VmHandle, PrlVmCfg_GetServerHost)
}

void PrlVmManipulationsTest::testGetHomePath()
{
	RECEIVE_UPDATED_VM_CONFIG
	PRL_CHAR sHomePath[STR_BUF_LENGTH];
	PRL_UINT32 nHomePathLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHomePath(m_VmHandle, sHomePath, &nHomePathLength))
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	QCOMPARE(UTF8_2QSTR(sHomePath), _vm_conf.getVmIdentification()->getHomePath());
	QVERIFY(nHomePathLength == size_t(_vm_conf.getVmIdentification()->getHomePath().toUtf8().size()+1));
}

void PrlVmManipulationsTest::testGetHomePathNotEnoughBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG
	PRL_CHAR sHomePath[STR_BUF_LENGTH];
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	PRL_UINT32 nHomePathLength = _vm_conf.getVmIdentification()->getHomePath().size();
	PRL_RESULT nRetCode = PrlVmCfg_GetHomePath(m_VmHandle, sHomePath, &nHomePathLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmManipulationsTest::testGetHomePathNullBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG;
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(m_VmHandle, PrlVmCfg_GetHomePath)
}

void PrlVmManipulationsTest::testGetVmIcon()
{
	RECEIVE_UPDATED_VM_CONFIG
	PRL_CHAR sIcon[STR_BUF_LENGTH];
	PRL_UINT32 nIconLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetIcon(m_VmHandle, sIcon, &nIconLength))
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	QCOMPARE(UTF8_2QSTR(sIcon), _vm_conf.getVmSettings()->getVmCommonOptions()->getIcon());
	QVERIFY(nIconLength == size_t(_vm_conf.getVmSettings()->getVmCommonOptions()->getIcon().toUtf8().size()+1));
}

void PrlVmManipulationsTest::testGetVmIconNotEnoughBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG
	PRL_CHAR sIcon[STR_BUF_LENGTH];
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	PRL_UINT32 nIconLength = _vm_conf.getVmSettings()->getVmCommonOptions()->getIcon().size();
	PRL_RESULT nRetCode = PrlVmCfg_GetIcon(m_VmHandle, sIcon, &nIconLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmManipulationsTest::testGetVmIconNullBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG;
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(m_VmHandle, PrlVmCfg_GetIcon)
}

void PrlVmManipulationsTest::testSetVmIcon()
{
	testCreateVmFromConfig();
	QString sNewVmIcon = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP(PrlVmCfg_SetIcon(m_VmHandle, sNewVmIcon.toUtf8().data()))
	PRL_CHAR sVmIcon[STR_BUF_LENGTH];
	PRL_UINT32 nVmIconLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetIcon(m_VmHandle, sVmIcon, &nVmIconLength))
	QCOMPARE(UTF8_2QSTR(sVmIcon), sNewVmIcon);
}

void PrlVmManipulationsTest::testSetVmIconTryToSetEmptyStringValue()
{
	testCreateVmFromConfig();
	QString sNewVmIcon = "";
	CHECK_RET_CODE_EXP(PrlVmCfg_SetIcon(m_VmHandle, sNewVmIcon.toUtf8().data()))
	PRL_CHAR sVmIcon[STR_BUF_LENGTH];
	PRL_UINT32 nVmIconLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetIcon(m_VmHandle, sVmIcon, &nVmIconLength))
	QCOMPARE(UTF8_2QSTR(sVmIcon), sNewVmIcon);
}

void PrlVmManipulationsTest::testGetVmDescription()
{
	RECEIVE_UPDATED_VM_CONFIG
	PRL_CHAR sDescription[STR_BUF_LENGTH];
	PRL_UINT32 nDescriptionLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDescription(m_VmHandle, sDescription, &nDescriptionLength))
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	QCOMPARE(UTF8_2QSTR(sDescription), _vm_conf.getVmSettings()->getVmCommonOptions()->getVmDescription());
	QVERIFY(nDescriptionLength == size_t(_vm_conf.getVmSettings()->getVmCommonOptions()->getVmDescription().toUtf8().size()+1));
}

void PrlVmManipulationsTest::testGetVmDescriptionNotEnoughBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG
	PRL_CHAR sDescription[STR_BUF_LENGTH];
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	PRL_UINT32 nDescriptionLength = _vm_conf.getVmSettings()->getVmCommonOptions()->getVmDescription().size();
	PRL_RESULT nRetCode = PrlVmCfg_GetDescription(m_VmHandle, sDescription, &nDescriptionLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmManipulationsTest::testGetVmDescriptionNullBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG;
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(m_VmHandle, PrlVmCfg_GetDescription)
}

void PrlVmManipulationsTest::testSetVmDescription()
{
	testCreateVmFromConfig();
	QString sNewVmDescription = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDescription(m_VmHandle, sNewVmDescription.toUtf8().data()))
	PRL_CHAR sVmDescription[STR_BUF_LENGTH];
	PRL_UINT32 nVmDescriptionLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDescription(m_VmHandle, sVmDescription, &nVmDescriptionLength))
	QCOMPARE(UTF8_2QSTR(sVmDescription), sNewVmDescription);
}

void PrlVmManipulationsTest::testSetVmDescriptionTryToSetEmptyStringValue()
{
	testCreateVmFromConfig();
	QString sNewVmDescription = "";
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDescription(m_VmHandle, sNewVmDescription.toUtf8().data()))
	PRL_CHAR sVmDescription[STR_BUF_LENGTH];
	PRL_UINT32 nVmDescriptionLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDescription(m_VmHandle, sVmDescription, &nVmDescriptionLength))
	QCOMPARE(UTF8_2QSTR(sVmDescription), sNewVmDescription);
}

void PrlVmManipulationsTest::testGetVmCustomProperty()
{
	RECEIVE_UPDATED_VM_CONFIG
	PRL_CHAR sCustomProperty[STR_BUF_LENGTH];
	PRL_UINT32 nCustomPropertyLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetCustomProperty(m_VmHandle, sCustomProperty, &nCustomPropertyLength))
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	QCOMPARE(UTF8_2QSTR(sCustomProperty), _vm_conf.getVmSettings()->getVmCommonOptions()->getCustomProperty());
	QVERIFY(nCustomPropertyLength == size_t(_vm_conf.getVmSettings()->getVmCommonOptions()->getCustomProperty().size()+1));
}

void PrlVmManipulationsTest::testGetVmCustomPropertyNotEnoughBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG
	PRL_CHAR sCustomProperty[STR_BUF_LENGTH];
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	PRL_UINT32 nCustomPropertyLength = _vm_conf.getVmSettings()->getVmCommonOptions()->getCustomProperty().size();
	// check on empty string. if it is empty, testGetVmCustomPropertyNullBufSize() will test it
	if (nCustomPropertyLength == 0)
		return;
	PRL_RESULT nRetCode = PrlVmCfg_GetCustomProperty(m_VmHandle, sCustomProperty, &nCustomPropertyLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmManipulationsTest::testGetVmCustomPropertyNullBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG;
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(m_VmHandle, PrlVmCfg_GetCustomProperty)
}

void PrlVmManipulationsTest::testSetVmCustomProperty()
{
	testCreateVmFromConfig();
	QString sNewVmCustomProperty = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP(PrlVmCfg_SetCustomProperty(m_VmHandle, sNewVmCustomProperty.toUtf8().data()))
	PRL_CHAR sVmCustomProperty[STR_BUF_LENGTH];
	PRL_UINT32 nVmCustomPropertyLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetCustomProperty(m_VmHandle, sVmCustomProperty, &nVmCustomPropertyLength))
	QCOMPARE(UTF8_2QSTR(sVmCustomProperty), sNewVmCustomProperty);
}

void PrlVmManipulationsTest::testSetVmCustomPropertyTryToSetEmptyStringValue()
{
	testCreateVmFromConfig();
	QString sNewVmCustomProperty = "";
	CHECK_RET_CODE_EXP(PrlVmCfg_SetCustomProperty(m_VmHandle, sNewVmCustomProperty.toUtf8().data()))
	PRL_CHAR sVmCustomProperty[STR_BUF_LENGTH];
	PRL_UINT32 nVmCustomPropertyLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetCustomProperty(m_VmHandle, sVmCustomProperty, &nVmCustomPropertyLength))
	QCOMPARE(UTF8_2QSTR(sVmCustomProperty), sNewVmCustomProperty);
}

void PrlVmManipulationsTest::testGetVmAutoStart()
{
	testCreateVmFromConfig();
	PRL_VM_AUTOSTART_OPTION nVmAutoStart;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAutoStart(m_VmHandle, &nVmAutoStart))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(nVmAutoStart == _vm_conf.getVmSettings()->getVmStartupOptions()->getAutoStart());
}

void PrlVmManipulationsTest::testSetVmAutoStart()
{
	CREATE_TEST_VM
	PRL_VM_AUTOSTART_OPTION nVmAutoStart = PAO_VM_START_ON_RELOAD;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetAutoStart(hVm, nVmAutoStart))
	PRL_VM_AUTOSTART_OPTION nActualVmAutoStart = PAO_VM_START_MANUAL;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAutoStart(hVm, &nActualVmAutoStart))
	QCOMPARE(nActualVmAutoStart, nVmAutoStart);
}

void PrlVmManipulationsTest::testVmAutoStartWrongParams()
{
	PRL_VM_AUTOSTART_OPTION nVmAutoStart = PAO_VM_START_MANUAL;

// Wrong handle

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetAutoStart(m_ServerHandle, PAO_VM_START_ON_LOAD),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetAutoStart(m_ServerHandle, &nVmAutoStart),
										PRL_ERR_INVALID_ARG);

// Invalid handle

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetAutoStart(m_VmHandle, PAO_VM_START_ON_RELOAD),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetAutoStart(m_VmHandle, &nVmAutoStart),
										PRL_ERR_INVALID_ARG);

// Null pointer

	CREATE_TEST_VM

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetAutoStart(hVm, 0),
										PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testGetVmAutoStartDelay()
{
	testCreateVmFromConfig();
	PRL_UINT32 nVmAutoStartDelay = 25;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAutoStartDelay(m_VmHandle, &nVmAutoStartDelay))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(nVmAutoStartDelay == _vm_conf.getVmSettings()->getVmStartupOptions()->getAutoStartDelay());
}

void PrlVmManipulationsTest::testSetVmAutoStartDelay()
{
	CREATE_TEST_VM
	PRL_UINT32 nVmAutoStartDelay = 17;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetAutoStartDelay(hVm, nVmAutoStartDelay))
	PRL_UINT32 nActualVmAutoStartDelay = 6;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAutoStartDelay(hVm, &nActualVmAutoStartDelay))
	QCOMPARE(nActualVmAutoStartDelay, nVmAutoStartDelay);
}

void PrlVmManipulationsTest::testVmAutoStartDelayWrongParams()
{
	PRL_UINT32 nVmAutoStartDelay = 34;

// Wrong handle

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetAutoStartDelay(m_ServerHandle, 12),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetAutoStartDelay(m_ServerHandle, &nVmAutoStartDelay),
										PRL_ERR_INVALID_ARG);

// Invalid handle

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetAutoStartDelay(m_VmHandle, 41),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetAutoStartDelay(m_VmHandle, &nVmAutoStartDelay),
										PRL_ERR_INVALID_ARG);

// Null pointer

	CREATE_TEST_VM

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetAutoStartDelay(hVm, 0),
										PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testGetVmStartLoginMode()
{
	testCreateVmFromConfig();
	PRL_VM_START_LOGIN_MODE nVmStartLoginMode = PLM_USER_ACCOUNT;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetStartLoginMode(m_VmHandle, &nVmStartLoginMode))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(nVmStartLoginMode == _vm_conf.getVmSettings()->getVmStartupOptions()->getVmStartLoginMode());
}

void PrlVmManipulationsTest::testSetVmStartLoginMode()
{
	CREATE_TEST_VM
	PRL_VM_START_LOGIN_MODE nVmStartLoginMode = PLM_ROOT_ACCOUNT;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetStartLoginMode(hVm, nVmStartLoginMode))
	PRL_VM_START_LOGIN_MODE nActualVmStartLoginMode = PLM_START_ACCOUNT;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetStartLoginMode(hVm, &nActualVmStartLoginMode))
	QCOMPARE(nActualVmStartLoginMode, nVmStartLoginMode);
}

void PrlVmManipulationsTest::testVmStartLoginModeWrongParams()
{
	PRL_VM_START_LOGIN_MODE nVmStartLoginMode = PLM_ROOT_ACCOUNT;

// Wrong handle

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetStartLoginMode(m_ServerHandle, PLM_USER_ACCOUNT),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetStartLoginMode(m_ServerHandle, &nVmStartLoginMode),
										PRL_ERR_INVALID_ARG);

// Invalid handle

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetStartLoginMode(m_VmHandle, PLM_START_ACCOUNT),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetStartLoginMode(m_VmHandle, &nVmStartLoginMode),
										PRL_ERR_INVALID_ARG);

// Null pointer

	CREATE_TEST_VM

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetStartLoginMode(hVm, 0),
										PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testGetVmStartUserLogin()
{
	testCreateVmFromConfig();
	QString qsStartUserLogin = "abc";
	PRL_EXTRACT_STRING_VALUE(qsStartUserLogin, m_VmHandle, PrlVmCfg_GetStartUserLogin);
	INIT_VM_CONF_FROM_FILE
	QVERIFY(qsStartUserLogin == _vm_conf.getVmSettings()->getVmStartupOptions()->getVmStartAsUser());
}

void PrlVmManipulationsTest::testSetVmStartUserCreds()
{
	CREATE_TEST_VM
	QString qsStartUserLogin = "MyUser";
	QString qsPassword = "qawsedrf";
	CHECK_RET_CODE_EXP(PrlVmCfg_SetStartUserCreds(hVm, qsStartUserLogin.toUtf8().constData(),
												qsPassword.toUtf8().constData()));
	QString qsActualStartUserLogin = "AnyUser";
	PRL_EXTRACT_STRING_VALUE(qsActualStartUserLogin, hVm, PrlVmCfg_GetStartUserLogin);
	QCOMPARE(qsActualStartUserLogin, qsStartUserLogin);

	PRL_STR pVmConfig = 0;
	CHECK_RET_CODE_EXP(PrlVm_ToString(hVm, (PRL_VOID_PTR_PTR )&pVmConfig));

	CVmConfiguration _vm_config;
	_vm_config.fromString((const char* )pVmConfig);
	CHECK_RET_CODE_EXP(PrlBuffer_Free(pVmConfig));

	QCOMPARE(qsPassword, _vm_config.getVmSettings()->getVmStartupOptions()->getVmStartAsPassword());
	QVERIFY(_vm_config.getVmSettings()->getVmStartupOptions()->isChangedPassword());
}

void PrlVmManipulationsTest::testVmStartUserCredsWrongParams()
{
	QString qsVmStartUserLogin;
	QString qsPassword;
	QByteArray buf(32, 'B');
	PRL_UINT32 nSize = 0;

// Wrong handle

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetStartUserCreds( m_ServerHandle,
																qsVmStartUserLogin.toUtf8().constData(),
																qsPassword.toUtf8().constData()),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetStartUserLogin(m_ServerHandle, buf.data(), &nSize),
										PRL_ERR_INVALID_ARG);

// Invalid handle

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetStartUserCreds( m_VmHandle,
																qsVmStartUserLogin.toUtf8().constData(),
																qsPassword.toUtf8().constData()),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetStartUserLogin(m_VmHandle, buf.data(), &nSize),
										PRL_ERR_INVALID_ARG);

// Null pointer

	CREATE_TEST_VM

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetStartUserLogin(hVm, 0, 0),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetStartUserCreds( hVm,
																0,
																qsPassword.toUtf8().constData()),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetStartUserCreds( hVm,
																qsVmStartUserLogin.toUtf8().constData(),
																0),
										PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testGetVmAutoStop()
{
	testCreateVmFromConfig();
	PRL_VM_AUTOSTOP_OPTION nVmAutoStop;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAutoStop(m_VmHandle, &nVmAutoStop))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(nVmAutoStop == _vm_conf.getVmSettings()->getShutdown()->getAutoStop());
}

void PrlVmManipulationsTest::testSetVmAutoStop()
{
	CREATE_TEST_VM
	PRL_VM_AUTOSTOP_OPTION nVmAutoStop = PAO_VM_SUSPEND;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetAutoStop(hVm, nVmAutoStop))
	PRL_VM_AUTOSTOP_OPTION nActualVmAutoStop;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAutoStop(hVm, &nActualVmAutoStop))
	QCOMPARE(nActualVmAutoStop, nVmAutoStop);
}

void PrlVmManipulationsTest::testVmAutoStopWrongParams()
{
	PRL_VM_AUTOSTOP_OPTION nVmAutoStop = PAO_VM_STOP;

// Wrong handle

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetAutoStop(m_ServerHandle, PAO_VM_STOP),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetAutoStop(m_ServerHandle, &nVmAutoStop),
										PRL_ERR_INVALID_ARG);

// Invalid handle

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetAutoStop(m_VmHandle, PAO_VM_SUSPEND),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetAutoStop(m_VmHandle, &nVmAutoStop),
										PRL_ERR_INVALID_ARG);

// Null pointer

	CREATE_TEST_VM

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetAutoStop(hVm, 0),
										PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testGetVmWindowMode()
{
	testCreateVmFromConfig();
	PRL_VM_WINDOW_MODE nVmWindowMode;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetWindowMode(m_VmHandle, &nVmWindowMode))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(nVmWindowMode == _vm_conf.getVmSettings()->getVmStartupOptions()->getWindowMode());
}

void PrlVmManipulationsTest::testSetStartInDetachedWindowEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bStartInDetachedWindowEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsStartInDetachedWindowEnabled(hVm, &bStartInDetachedWindowEnabled))
	bStartInDetachedWindowEnabled = !bStartInDetachedWindowEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetStartInDetachedWindowEnabled(hVm, bStartInDetachedWindowEnabled))
	PRL_BOOL bActualStartInDetachedWindowEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsStartInDetachedWindowEnabled(hVm, &bActualStartInDetachedWindowEnabled))
	QCOMPARE(bActualStartInDetachedWindowEnabled, bStartInDetachedWindowEnabled);
}

void PrlVmManipulationsTest::testStartInDetachedWindowEnabledOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bStartInDetachedWindowEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsStartInDetachedWindowEnabled(m_ServerHandle, &bStartInDetachedWindowEnabled),\
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetStartInDetachedWindowEnabled(m_ServerHandle, PRL_FALSE),\
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsStartInDetachedWindowEnabled(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testGetVmLastModifiedDate()
{
	RECEIVE_UPDATED_VM_CONFIG
	PRL_CHAR sLastModifiedDate[STR_BUF_LENGTH];
	PRL_UINT32 nLastModifiedDateLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetLastModifiedDate(m_VmHandle, sLastModifiedDate, &nLastModifiedDateLength))
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	QString sActualDate = _vm_conf.getVmIdentification()->getLastModifDate().toString(XML_DATETIME_FORMAT);
	QCOMPARE(UTF8_2QSTR(sLastModifiedDate), sActualDate);
	QVERIFY(nLastModifiedDateLength == size_t(sActualDate.toUtf8().size()+1));
}

void PrlVmManipulationsTest::testGetVmLastModifiedDateNotEnoughBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG
	PRL_CHAR sLastModifiedDate[STR_BUF_LENGTH];
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	PRL_UINT32 nLastModifiedDateLength = _vm_conf.getVmIdentification()->getLastModifDate().toString(XML_DATETIME_FORMAT).size();
	PRL_RESULT nRetCode = PrlVmCfg_GetLastModifiedDate(m_VmHandle, sLastModifiedDate, &nLastModifiedDateLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmManipulationsTest::testGetVmLastModifiedDateNullBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG;
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(m_VmHandle, PrlVmCfg_GetLastModifiedDate)
}

void PrlVmManipulationsTest::testGetVmLastModifierName()
{
	RECEIVE_UPDATED_VM_CONFIG
	PRL_CHAR sLastModifierName[STR_BUF_LENGTH];
	PRL_UINT32 nLastModifierNameLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetLastModifierName(m_VmHandle, sLastModifierName, &nLastModifierNameLength))
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	QString sActualName = _vm_conf.getVmIdentification()->getModifierName();
	QCOMPARE(UTF8_2QSTR(sLastModifierName), sActualName);
	QVERIFY(nLastModifierNameLength == size_t(sActualName.toUtf8().size()+1));
}

void PrlVmManipulationsTest::testGetVmLastModifierNameNotEnoughBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG
	PRL_CHAR sLastModifierName[STR_BUF_LENGTH];
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	PRL_UINT32 nLastModifierNameLength = _vm_conf.getVmIdentification()->getModifierName().size();
	if (nLastModifierNameLength != 0) {
		PRL_RESULT nRetCode = PrlVmCfg_GetLastModifierName(m_VmHandle, sLastModifierName, &nLastModifierNameLength);
		QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
	}
}

void PrlVmManipulationsTest::testGetVmLastModifierNameNullBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG;
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(m_VmHandle, PrlVmCfg_GetLastModifierName)
}

void PrlVmManipulationsTest::testIsVmGuestSharingEnabled()
{
	testCreateVmFromConfig();
	PRL_BOOL bVmGuestSharingEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsGuestSharingEnabled(m_VmHandle, &bVmGuestSharingEnabled))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bVmGuestSharingEnabled == (PRL_BOOL) _vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getGuestSharing()->isEnabled());
}

void PrlVmManipulationsTest::testSetVmGuestSharingEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bVmGuestSharingEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsGuestSharingEnabled(hVm, &bVmGuestSharingEnabled))
	bVmGuestSharingEnabled = !bVmGuestSharingEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetGuestSharingEnabled(hVm, bVmGuestSharingEnabled))
	PRL_BOOL bActualVmGuestSharingEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsGuestSharingEnabled(hVm, &bActualVmGuestSharingEnabled))
	QCOMPARE(bActualVmGuestSharingEnabled, bVmGuestSharingEnabled);
}

void PrlVmManipulationsTest::testIsVmGuestSharingAutoMount()
{
	testCreateVmFromConfig();
	PRL_BOOL bVmGuestSharingAutoMount;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsGuestSharingAutoMount(m_VmHandle, &bVmGuestSharingAutoMount))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bVmGuestSharingAutoMount == (PRL_BOOL) _vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getGuestSharing()->isAutoMount());
}

void PrlVmManipulationsTest::testSetVmGuestSharingAutoMount()
{
	CREATE_TEST_VM
	PRL_BOOL bVmGuestSharingAutoMount;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsGuestSharingAutoMount(hVm, &bVmGuestSharingAutoMount))
	bVmGuestSharingAutoMount = !bVmGuestSharingAutoMount;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetGuestSharingAutoMount(hVm, bVmGuestSharingAutoMount))
	PRL_BOOL bActualVmGuestSharingAutoMount;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsGuestSharingAutoMount(hVm, &bActualVmGuestSharingAutoMount))
	QCOMPARE(bActualVmGuestSharingAutoMount, bVmGuestSharingAutoMount);
}

void PrlVmManipulationsTest::testIsVmGuestSharingEnableSpotlight()
{
	testCreateVmFromConfig();
	PRL_BOOL bVmGuestSharingEnableSpotlight;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsGuestSharingEnableSpotlight(m_VmHandle, &bVmGuestSharingEnableSpotlight))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bVmGuestSharingEnableSpotlight ==
		(PRL_BOOL) _vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getGuestSharing()->isEnableSpotlight());
}

void PrlVmManipulationsTest::testIsVmGuestSharingEnableSpotlightOnNonVmHandle()
{
	PRL_BOOL bVmGuestSharingEnableSpotlight;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(\
		PrlVmCfg_IsGuestSharingEnableSpotlight(m_ServerHandle, &bVmGuestSharingEnableSpotlight),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testIsVmGuestSharingEnableSpotlightOnInvalidVmHandle()
{
	PRL_BOOL bVmGuestSharingEnableSpotlight;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(\
		PrlVmCfg_IsGuestSharingEnableSpotlight(PRL_INVALID_HANDLE, &bVmGuestSharingEnableSpotlight),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testIsVmGuestSharingEnableSpotlightOnWrongBuffer()
{
	CREATE_TEST_VM
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsGuestSharingEnableSpotlight(hVm, NULL), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetVmGuestSharingEnableSpotlight()
{
	CREATE_TEST_VM
	PRL_BOOL bVmGuestSharingEnableSpotlight;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsGuestSharingEnableSpotlight(hVm, &bVmGuestSharingEnableSpotlight))
	bVmGuestSharingEnableSpotlight = !bVmGuestSharingEnableSpotlight;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetGuestSharingEnableSpotlight(hVm, bVmGuestSharingEnableSpotlight))
	PRL_BOOL bActualVmGuestSharingEnableSpotlight;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsGuestSharingEnableSpotlight(hVm, &bActualVmGuestSharingEnableSpotlight))
	QCOMPARE(bActualVmGuestSharingEnableSpotlight, bVmGuestSharingEnableSpotlight);
}

void PrlVmManipulationsTest::testSetVmGuestSharingEnableSpotlightOnWrongVmHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetGuestSharingEnableSpotlight(m_ServerHandle, PRL_FALSE),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetVmGuestSharingEnableSpotlightOnInvalidVmHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetGuestSharingEnableSpotlight(PRL_INVALID_HANDLE, PRL_FALSE),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testIsVmHostSharingEnabled()
{
	VM_CONFIG_INIT
	PRL_BOOL bVmHostSharingEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsHostSharingEnabled(m_VmHandle, &bVmHostSharingEnabled))
	VM_CONFIG_TO_XML_OBJECT
	QVERIFY(bVmHostSharingEnabled == (PRL_BOOL) _vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->isEnabled());
}

void PrlVmManipulationsTest::testSetVmHostSharingEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bVmHostSharingEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsHostSharingEnabled(hVm, &bVmHostSharingEnabled))
	bVmHostSharingEnabled = !bVmHostSharingEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetHostSharingEnabled(hVm, bVmHostSharingEnabled))
	PRL_BOOL bActualVmHostSharingEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsHostSharingEnabled(hVm, &bActualVmHostSharingEnabled))
	QCOMPARE(bActualVmHostSharingEnabled, bVmHostSharingEnabled);
}

void PrlVmManipulationsTest::testIsUserDefinedSharedFoldersEnabled()
{
	VM_CONFIG_INIT
	PRL_BOOL bUserDefinedSharedFoldersEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUserDefinedSharedFoldersEnabled(m_VmHandle, &bUserDefinedSharedFoldersEnabled))
	VM_CONFIG_TO_XML_OBJECT
	QVERIFY(bUserDefinedSharedFoldersEnabled == \
		(PRL_BOOL) _vm_conf.getVmSettings()->getVmTools()->getVmSharing()->\
			getHostSharing()->isUserDefinedFoldersEnabled());
}

void PrlVmManipulationsTest::testSetUserDefinedSharedFoldersEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bUserDefinedSharedFoldersEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUserDefinedSharedFoldersEnabled(hVm, &bUserDefinedSharedFoldersEnabled))
	bUserDefinedSharedFoldersEnabled = !bUserDefinedSharedFoldersEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetUserDefinedSharedFoldersEnabled(hVm, bUserDefinedSharedFoldersEnabled))
	PRL_BOOL bActualUserDefinedSharedFoldersEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUserDefinedSharedFoldersEnabled(hVm, &bActualUserDefinedSharedFoldersEnabled))
	QCOMPARE(bActualUserDefinedSharedFoldersEnabled, bUserDefinedSharedFoldersEnabled);
}

void PrlVmManipulationsTest::testIsShareAllHostDisks()
{
	VM_CONFIG_INIT
	PRL_BOOL bShareAllHostDisks;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsShareAllHostDisks(m_VmHandle, &bShareAllHostDisks))
	VM_CONFIG_TO_XML_OBJECT
	QVERIFY(bShareAllHostDisks == \
		(PRL_BOOL) _vm_conf.getVmSettings()->getVmTools()->getVmSharing()->\
			getHostSharing()->isShareAllMacDisks());
}

void PrlVmManipulationsTest::testIsShareAllHostDisksOnNonVmHandle()
{
	PRL_BOOL bShareAllHostDisks;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsShareAllHostDisks(m_ServerHandle, &bShareAllHostDisks),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testIsShareAllHostDisksInvalidVmHandle()
{
	PRL_BOOL bShareAllHostDisks;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsShareAllHostDisks(PRL_INVALID_HANDLE, &bShareAllHostDisks),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testIsShareAllHostDisksOnWrongBuffer()
{
	testCreateVmFromConfig();
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsShareAllHostDisks(m_VmHandle, NULL), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetShareAllHostDisks()
{
	CREATE_TEST_VM
	PRL_BOOL bShareAllHostDisks;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsShareAllHostDisks(hVm, &bShareAllHostDisks))
	bShareAllHostDisks = !bShareAllHostDisks;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetShareAllHostDisks(hVm, bShareAllHostDisks))
	PRL_BOOL bActualShareAllHostDisks;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsShareAllHostDisks(hVm, &bActualShareAllHostDisks))
	QCOMPARE(bActualShareAllHostDisks, bShareAllHostDisks);
}

void PrlVmManipulationsTest::testSetShareAllHostDisksOnWrongVmHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetShareAllHostDisks(m_ServerHandle, PRL_FALSE),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetShareAllHostDisksOnInvalidVmHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetShareAllHostDisks(PRL_INVALID_HANDLE, PRL_FALSE),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testIsShareUserHomeDir()
{
	testCreateVmFromConfig();
	PRL_BOOL bShareUserHomeDir;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsShareUserHomeDir(m_VmHandle, &bShareUserHomeDir))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bShareUserHomeDir == \
		(PRL_BOOL) _vm_conf.getVmSettings()->getVmTools()->getVmSharing()->\
			getHostSharing()->isShareUserHomeDir());
}

void PrlVmManipulationsTest::testIsShareUserHomeDirOnNonVmHandle()
{
	PRL_BOOL bShareUserHomeDir;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsShareUserHomeDir(m_ServerHandle, &bShareUserHomeDir),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testIsShareUserHomeDirOnInvalidHandle()
{
	PRL_BOOL bShareUserHomeDir;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsShareUserHomeDir(PRL_INVALID_HANDLE, &bShareUserHomeDir),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testIsShareUserHomeDirOnWrongBuffer()
{
	CREATE_TEST_VM
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsShareUserHomeDir(hVm, NULL), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetShareUserHomeDir()
{
	CREATE_TEST_VM
	PRL_BOOL bShareUserHomeDir;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsShareUserHomeDir(hVm, &bShareUserHomeDir))
	bShareUserHomeDir = !bShareUserHomeDir;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetShareUserHomeDir(hVm, bShareUserHomeDir))
	PRL_BOOL bActualShareUserHomeDir;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsShareUserHomeDir(hVm, &bActualShareUserHomeDir))
	QCOMPARE(bActualShareUserHomeDir, bShareUserHomeDir);
}

void PrlVmManipulationsTest::testSetShareUserHomeDirOnNonVmHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetShareUserHomeDir(m_ServerHandle, PRL_FALSE),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetShareUserHomeDirOnInvalidHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetShareUserHomeDir(PRL_INVALID_HANDLE, PRL_FALSE),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testIsMapSharedFoldersOnLetters()
{
	testCreateVmFromConfig();
	PRL_BOOL bMapSharedFoldersOnLetters;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsMapSharedFoldersOnLetters(m_VmHandle, &bMapSharedFoldersOnLetters))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bMapSharedFoldersOnLetters == \
		(PRL_BOOL) _vm_conf.getVmSettings()->getVmTools()->getVmSharing()->\
			getHostSharing()->isMapSharedFoldersOnLetters());
}

void PrlVmManipulationsTest::testIsMapSharedFoldersOnLettersOnNonVmHandle()
{
	PRL_BOOL bMapSharedFoldersOnLetters;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(\
		PrlVmCfg_IsMapSharedFoldersOnLetters(m_ServerHandle, &bMapSharedFoldersOnLetters),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testIsMapSharedFoldersOnLettersOnInvalidVmHandle()
{
	PRL_BOOL bMapSharedFoldersOnLetters;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(\
		PrlVmCfg_IsMapSharedFoldersOnLetters(PRL_INVALID_HANDLE, &bMapSharedFoldersOnLetters),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testIsMapSharedFoldersOnLettersOnWrongBuffer()
{
	CREATE_TEST_VM
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsMapSharedFoldersOnLetters(hVm, NULL), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetMapSharedFoldersOnLetters()
{
	CREATE_TEST_VM
	PRL_BOOL bMapSharedFoldersOnLetters;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsMapSharedFoldersOnLetters(hVm, &bMapSharedFoldersOnLetters))
	bMapSharedFoldersOnLetters = !bMapSharedFoldersOnLetters;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetMapSharedFoldersOnLetters(hVm, bMapSharedFoldersOnLetters))
	PRL_BOOL bActualMapSharedFoldersOnLetters;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsMapSharedFoldersOnLetters(hVm, &bActualMapSharedFoldersOnLetters))
	QCOMPARE(bActualMapSharedFoldersOnLetters, bMapSharedFoldersOnLetters);
}

void PrlVmManipulationsTest::testSetMapSharedFoldersOnLettersOnNonVmHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetMapSharedFoldersOnLetters(m_ServerHandle, PRL_FALSE),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetMapSharedFoldersOnLettersOnInvalidVmHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetMapSharedFoldersOnLetters(PRL_INVALID_HANDLE, PRL_FALSE),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetVmIoLimit()
{
	testCreateVmFromConfig();
	PRL_IOLIMIT_DATA data;
	data.type = PRL_IOLIMIT_BS;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetIoLimit(m_VmHandle, &data))
	QVERIFY(data.type == PRL_IOLIMIT_BS && data.value == 0);
}

void PrlVmManipulationsTest::testSetVmIoLimit()
{
	CREATE_TEST_VM
	PRL_IOLIMIT_DATA data;
	data.type = PRL_IOLIMIT_BS;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetIoLimit(hVm, &data))
	data.value++;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetIoLimit(hVm, &data))
	PRL_IOLIMIT_DATA data2;
	data2.type = PRL_IOLIMIT_BS;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetIoLimit(hVm, &data2))
	QVERIFY(data2.type == data.type && data2.value == data.value);
}

void PrlVmManipulationsTest::testVmIoLimitOnWrongParams()
{
	testCreateVmFromConfig();
	PRL_IOLIMIT_DATA data;
	data.type = PRL_IOLIMIT_BS;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
		PrlVmCfg_GetIoLimit(PRL_INVALID_HANDLE, &data),
		PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
		PrlVmCfg_GetIoLimit(m_ServerHandle, &data),
		PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
		PrlVmCfg_GetIoLimit(m_VmHandle, 0),
		PRL_ERR_INVALID_ARG)
	data.type = (PRL_IOLIMIT_TYPE)(PRL_IOLIMIT_BS + 1);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
		PrlVmCfg_GetIoLimit(m_VmHandle, &data),
		PRL_ERR_INVALID_ARG)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(
		PrlVmCfg_SetIoLimit(m_VmHandle, &data),
		PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
		PrlVmCfg_SetIoLimit(PRL_INVALID_HANDLE, &data),
		PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
		PrlVmCfg_SetIoLimit(m_ServerHandle, &data),
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testIsVmDiskCacheWriteBack()
{
	testCreateVmFromConfig();
	PRL_BOOL bVmDiskCacheWriteBack;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsDiskCacheWriteBack(m_VmHandle, &bVmDiskCacheWriteBack))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bVmDiskCacheWriteBack == PRL_BOOL(_vm_conf.getVmSettings()->getVmRuntimeOptions()->getDiskCachePolicy()));
}

void PrlVmManipulationsTest::testSetVmDiskCacheWriteBack()
{
	CREATE_TEST_VM
	PRL_BOOL bVmDiskCacheWriteBack;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsDiskCacheWriteBack(hVm, &bVmDiskCacheWriteBack))
	bVmDiskCacheWriteBack = !bVmDiskCacheWriteBack;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDiskCacheWriteBack(hVm, bVmDiskCacheWriteBack))
	PRL_BOOL bActualVmDiskCacheWriteBack;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsDiskCacheWriteBack(hVm, &bActualVmDiskCacheWriteBack))
	QCOMPARE(bActualVmDiskCacheWriteBack, bVmDiskCacheWriteBack);
}

void PrlVmManipulationsTest::testIsVmOsResInFullScrMode()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();
	PRL_BOOL bVmOsResInFullScrMode;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsOsResInFullScrMode(m_VmHandle, &bVmOsResInFullScrMode))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bVmOsResInFullScrMode == (PRL_BOOL) _vm_conf.getVmSettings()->getVmRuntimeOptions()->isOsResolutionInFullScreen());
}

void PrlVmManipulationsTest::testSetVmOsResInFullScrMode()
{
	CREATE_TEST_VM
	PRL_BOOL bVmOsResInFullScrMode;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsOsResInFullScrMode(hVm, &bVmOsResInFullScrMode))
	bVmOsResInFullScrMode = !bVmOsResInFullScrMode;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetOsResInFullScrMode(hVm, bVmOsResInFullScrMode))
	PRL_BOOL bActualVmOsResInFullScrMode;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsOsResInFullScrMode(hVm, &bActualVmOsResInFullScrMode))
	QCOMPARE(bActualVmOsResInFullScrMode, bVmOsResInFullScrMode);
}

void PrlVmManipulationsTest::testIsVmCloseAppOnShutdown()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();
	PRL_BOOL bVmCloseAppOnShutdown;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsCloseAppOnShutdown(m_VmHandle, &bVmCloseAppOnShutdown))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bVmCloseAppOnShutdown == (PRL_BOOL) _vm_conf.getVmSettings()->getVmRuntimeOptions()->isCloseAppOnShutdown());
}

void PrlVmManipulationsTest::testSetVmCloseAppOnShutdown()
{
	CREATE_TEST_VM
	PRL_BOOL bVmCloseAppOnShutdown;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsCloseAppOnShutdown(hVm, &bVmCloseAppOnShutdown))
	bVmCloseAppOnShutdown = !bVmCloseAppOnShutdown;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetCloseAppOnShutdown(hVm, bVmCloseAppOnShutdown))
	PRL_BOOL bActualVmCloseAppOnShutdown;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsCloseAppOnShutdown(hVm, &bActualVmCloseAppOnShutdown))
	QCOMPARE(bActualVmCloseAppOnShutdown, bVmCloseAppOnShutdown);
}

void PrlVmManipulationsTest::testGetVmSystemFlags()
{
	RECEIVE_UPDATED_VM_CONFIG
	PRL_CHAR sSystemFlags[STR_BUF_LENGTH];
	PRL_UINT32 nSystemFlagsLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetSystemFlags(m_VmHandle, sSystemFlags, &nSystemFlagsLength))
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	QCOMPARE(UTF8_2QSTR(sSystemFlags), _vm_conf.getVmSettings()->getVmRuntimeOptions()->getSystemFlags());
	QVERIFY(nSystemFlagsLength == (PRL_BOOL) size_t(_vm_conf.getVmSettings()->getVmRuntimeOptions()->getSystemFlags().toUtf8().size()+1));
}

void PrlVmManipulationsTest::testGetVmSystemFlagsNotEnoughBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG
	PRL_CHAR sSystemFlags[STR_BUF_LENGTH];
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	PRL_UINT32 nSystemFlagsLength = _vm_conf.getVmSettings()->getVmRuntimeOptions()->getSystemFlags().size();
	PRL_RESULT nRetCode = PrlVmCfg_GetSystemFlags(m_VmHandle, sSystemFlags, &nSystemFlagsLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmManipulationsTest::testGetVmSystemFlagsNullBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG;
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(m_VmHandle, PrlVmCfg_GetSystemFlags)
}

void PrlVmManipulationsTest::testSetVmSystemFlags()
{
	testCreateVmFromConfig();
	QString sNewVmSystemFlags = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP(PrlVmCfg_SetSystemFlags(m_VmHandle, sNewVmSystemFlags.toUtf8().data()))
	PRL_CHAR sVmSystemFlags[STR_BUF_LENGTH];
	PRL_UINT32 nVmSystemFlagsLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetSystemFlags(m_VmHandle, sVmSystemFlags, &nVmSystemFlagsLength))
	QCOMPARE(UTF8_2QSTR(sVmSystemFlags), sNewVmSystemFlags);
}

void PrlVmManipulationsTest::testSetVmSystemFlagsTryToSetEmptyStringValue()
{
	testCreateVmFromConfig();
	QString sNewVmSystemFlags = "";
	CHECK_RET_CODE_EXP(PrlVmCfg_SetSystemFlags(m_VmHandle, sNewVmSystemFlags.toUtf8().data()))
	PRL_CHAR sVmSystemFlags[STR_BUF_LENGTH];
	PRL_UINT32 nVmSystemFlagsLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetSystemFlags(m_VmHandle, sVmSystemFlags, &nVmSystemFlagsLength))
	QCOMPARE(UTF8_2QSTR(sVmSystemFlags), sNewVmSystemFlags);
}

void PrlVmManipulationsTest::testIsVmDisableAPIC()
{
	testCreateVmFromConfig();
	PRL_BOOL bIsDisableAPIC = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsDisableAPIC(m_VmHandle, &bIsDisableAPIC))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bIsDisableAPIC == PRL_BOOL(_vm_conf.getVmSettings()->getVmRuntimeOptions()->isDisableAPIC()));
}

void PrlVmManipulationsTest::testSetVmDisableAPICSign()
{
	CREATE_TEST_VM
	PRL_BOOL bIsDisableAPIC = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsDisableAPIC(hVm, &bIsDisableAPIC))
	bIsDisableAPIC = !bIsDisableAPIC;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDisableAPICSign(hVm, bIsDisableAPIC))
	PRL_BOOL bActualIsDisableAPIC = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsDisableAPIC(hVm, &bActualIsDisableAPIC))
	QCOMPARE(bActualIsDisableAPIC, bIsDisableAPIC);
}

void PrlVmManipulationsTest::testIsVmDisableAPICOnWrongParams()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

// Wrong handle
	PRL_BOOL bIsDisableAPIC = PRL_TRUE;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsDisableAPIC(m_ServerHandle, &bIsDisableAPIC),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetDisableAPICSign(m_ServerHandle, PRL_TRUE), PRL_ERR_INVALID_ARG);

// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsDisableAPIC(m_VmHandle, NULL),
										PRL_ERR_INVALID_ARG);
}


void PrlVmManipulationsTest::testIsVmDisableSpeaker()
{
	testCreateVmFromConfig();
	PRL_BOOL bIsDisableSpeaker = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsDisableSpeaker(m_VmHandle, &bIsDisableSpeaker))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bIsDisableSpeaker == PRL_BOOL(_vm_conf.getVmSettings()->getVmRuntimeOptions()->isDisableSpeaker()));
}

void PrlVmManipulationsTest::testSetVmDisableSpeakerSign()
{
	CREATE_TEST_VM
	PRL_BOOL bIsDisableSpeaker = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsDisableSpeaker(hVm, &bIsDisableSpeaker))
	bIsDisableSpeaker = !bIsDisableSpeaker;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDisableSpeakerSign(hVm, bIsDisableSpeaker))
	PRL_BOOL bActualIsDisableSpeaker = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsDisableSpeaker(hVm, &bActualIsDisableSpeaker))
	QCOMPARE(bActualIsDisableSpeaker, bIsDisableSpeaker);
}

void PrlVmManipulationsTest::testIsVmDisableSpeakerOnWrongParams()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

// Wrong handle
	PRL_BOOL bIsDisableSpeaker = PRL_TRUE;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsDisableSpeaker(m_ServerHandle, &bIsDisableSpeaker),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetDisableSpeakerSign(m_ServerHandle, PRL_TRUE), PRL_ERR_INVALID_ARG);

// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsDisableSpeaker(m_VmHandle, NULL),
										PRL_ERR_INVALID_ARG);
}


void PrlVmManipulationsTest::testGetUndoDisksMode()
{
	testCreateVmFromConfig();
	PRL_UNDO_DISKS_MODE nUndoDisksMode = PUD_PROMPT_BEHAVIOUR;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUndoDisksMode(m_VmHandle, &nUndoDisksMode));
	INIT_VM_CONF_FROM_FILE
	QVERIFY(nUndoDisksMode == (PRL_UNDO_DISKS_MODE )_vm_conf.getVmSettings()
										->getVmRuntimeOptions()->getUndoDisksMode());
}

void PrlVmManipulationsTest::testSetUndoDisksMode()
{
	CREATE_TEST_VM
	PRL_UNDO_DISKS_MODE nUndoDisksMode = PUD_PROMPT_BEHAVIOUR;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetUndoDisksMode(hVm, nUndoDisksMode));
	PRL_UNDO_DISKS_MODE nActualUndoDisksMode;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUndoDisksMode(hVm, &nActualUndoDisksMode));
	QCOMPARE(nActualUndoDisksMode, nUndoDisksMode);
}

void PrlVmManipulationsTest::testUndoDisksModeOnWrongParams()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

// Wrong handle
	PRL_UNDO_DISKS_MODE nUndoDisksMode = PUD_PROMPT_BEHAVIOUR;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetUndoDisksMode(m_ServerHandle, &nUndoDisksMode),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetUndoDisksMode(m_ServerHandle, PUD_COMMIT_CHANGES),
										PRL_ERR_INVALID_ARG);

// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetUndoDisksMode(m_VmHandle, NULL),
										PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testGetVmForegroundPriority()
{
	testCreateVmFromConfig();
	PRL_VM_PRIORITY nVmForegroundPriority;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetForegroundPriority(m_VmHandle, &nVmForegroundPriority))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(nVmForegroundPriority == _vm_conf.getVmSettings()->getVmRuntimeOptions()->getForegroundPriority());
}

void PrlVmManipulationsTest::testSetVmForegroundPriority()
{
	CREATE_TEST_VM
	PRL_VM_PRIORITY nVmForegroundPriority = PVR_PRIORITY_HIGH;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetForegroundPriority(hVm, nVmForegroundPriority))
	PRL_VM_PRIORITY nActualVmForegroundPriority;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetForegroundPriority(hVm, &nActualVmForegroundPriority))
	QCOMPARE(nActualVmForegroundPriority, nVmForegroundPriority);
}

void PrlVmManipulationsTest::testGetVmBackgroundPriority()
{
	testCreateVmFromConfig();
	PRL_VM_PRIORITY nVmBackgroundPriority;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetBackgroundPriority(m_VmHandle, &nVmBackgroundPriority))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(nVmBackgroundPriority == _vm_conf.getVmSettings()->getVmRuntimeOptions()->getBackgroundPriority());
}

void PrlVmManipulationsTest::testSetVmBackgroundPriority()
{
	CREATE_TEST_VM
	PRL_VM_PRIORITY nVmBackgroundPriority = PVR_PRIORITY_HIGH;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetBackgroundPriority(hVm, nVmBackgroundPriority))
	PRL_VM_PRIORITY nActualVmBackgroundPriority;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetBackgroundPriority(hVm, &nActualVmBackgroundPriority))
	QCOMPARE(nActualVmBackgroundPriority, nVmBackgroundPriority);
}

void PrlVmManipulationsTest::testCreateShare()
{
	CREATE_TEST_VM
	SdkHandleWrap hShare;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(hVm, hShare.GetHandlePtr()))
	PRL_HANDLE_TYPE _type = PHT_ERROR;
	CHECK_RET_CODE_EXP(PrlHandle_GetType(hShare, &_type))
	QVERIFY(_type == PHT_SHARE);
}

void PrlVmManipulationsTest::testGetSharesCount()
{
	CREATE_TEST_VM
	SdkHandleWrap hShare;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(hVm, hShare.GetHandlePtr()))
	PRL_UINT32 nSharesCount = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetSharesCount(hVm, &nSharesCount))
	QVERIFY(nSharesCount != 0);
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(hVm)
	QCOMPARE(nSharesCount, PRL_UINT32(_vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->m_lstSharedFolders.size()));
}

void PrlVmManipulationsTest::testGetShare()
{
	CREATE_TEST_VM
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(hVm)
	CVmSharedFolder *pShare = new CVmSharedFolder;
	_vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->addSharedFolder(pShare);
	CHECK_RET_CODE_EXP(PrlVm_FromString(hVm, _vm_conf.toString().toUtf8().data()))
	SdkHandleWrap hShare;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetShare(hVm, 0, hShare.GetHandlePtr()))
	PRL_HANDLE_TYPE _type = PHT_ERROR;
	CHECK_RET_CODE_EXP(PrlHandle_GetType(hShare, &_type))
	QVERIFY(_type == PHT_SHARE);
}

void PrlVmManipulationsTest::testGetShareNonValidIndex()
{
	CREATE_TEST_VM
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(hVm)
	PRL_UINT32 nShareIndex = _vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->m_lstSharedFolders.size();
	SdkHandleWrap hShare;
	QVERIFY(!PRL_SUCCEEDED(PrlVmCfg_GetShare(hVm, nShareIndex, hShare.GetHandlePtr())));
}

#define CHECK_TRY_TO_USE_ALREADY_REMOVED_SHARE\
	QVERIFY(!PRL_SUCCEEDED(PrlShare_Remove(hShare)));\
	PRL_CHAR sShareName[STR_BUF_LENGTH];\
	PRL_UINT32 nShareNameLength = STR_BUF_LENGTH;\
	QVERIFY(!PRL_SUCCEEDED(PrlShare_GetName(hShare, sShareName, &nShareNameLength)));\
	QVERIFY(!PRL_SUCCEEDED(PrlShare_SetName(hShare, "some share name")));\
	PRL_CHAR sSharePath[STR_BUF_LENGTH];\
	PRL_UINT32 nSharePathLength = STR_BUF_LENGTH;\
	QVERIFY(!PRL_SUCCEEDED(PrlShare_GetPath(hShare, sSharePath, &nSharePathLength)));\
	QVERIFY(!PRL_SUCCEEDED(PrlShare_SetPath(hShare, "some share path")));\
	PRL_CHAR sShareDescription[STR_BUF_LENGTH];\
	PRL_UINT32 nShareDescriptionLength = STR_BUF_LENGTH;\
	QVERIFY(!PRL_SUCCEEDED(PrlShare_GetDescription(hShare, sShareDescription, &nShareDescriptionLength)));\
	QVERIFY(!PRL_SUCCEEDED(PrlShare_SetDescription(hShare, "some share description")));\
	PRL_BOOL bEnabled;\
	QVERIFY(!PRL_SUCCEEDED(PrlShare_IsEnabled(hShare, &bEnabled)));\
	QVERIFY(!PRL_SUCCEEDED(PrlShare_SetEnabled(hShare, bEnabled)));\
	PRL_BOOL bReadOnly;\
	QVERIFY(!PRL_SUCCEEDED(PrlShare_IsReadOnly(hShare, &bReadOnly)));\
	QVERIFY(!PRL_SUCCEEDED(PrlShare_SetReadOnly(hShare, bReadOnly)));

void PrlVmManipulationsTest::testShareRemove()
{
	CREATE_TEST_VM
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(hVm)
	CVmSharedFolder *pShare = new CVmSharedFolder;
	_vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->addSharedFolder(pShare);
	CHECK_RET_CODE_EXP(PrlVm_FromString(hVm, _vm_conf.toString().toUtf8().data()))
	SdkHandleWrap hShare;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetShare(hVm, 0, hShare.GetHandlePtr()))
	PRL_UINT32 nSharesCount1 = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetSharesCount(hVm, &nSharesCount1))
	CHECK_RET_CODE_EXP(PrlShare_Remove(hShare))
	PRL_UINT32 nSharesCount2 = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetSharesCount(hVm, &nSharesCount2))
	QVERIFY((nSharesCount1 - nSharesCount2) == 1);
	CHECK_TRY_TO_USE_ALREADY_REMOVED_SHARE
}

#define GET_VM_SHARED_FOLDER\
	SdkHandleWrap hShare;\
	CHECK_RET_CODE_EXP(PrlVmCfg_GetShare(m_VmHandle, 0, hShare.GetHandlePtr()))

void PrlVmManipulationsTest::testGetShareName()
{
	RECEIVE_UPDATED_VM_CONFIG
	GET_VM_SHARED_FOLDER
	PRL_CHAR sShareName[STR_BUF_LENGTH];
	PRL_UINT32 nShareNameLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlShare_GetName(hShare, sShareName, &nShareNameLength))
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	CVmSharedFolder *pSharedFolder = _vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->m_lstSharedFolders.at(0);
	QCOMPARE(UTF8_2QSTR(sShareName), pSharedFolder->getName());
	QVERIFY(nShareNameLength == size_t(pSharedFolder->getName().toUtf8().size()+1));
}

void PrlVmManipulationsTest::testGetShareNameNotEnoughBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG
	GET_VM_SHARED_FOLDER
	PRL_CHAR sShareName[STR_BUF_LENGTH];
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	PRL_UINT32 nShareNameLength = _vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->m_lstSharedFolders.at(0)->getName().toUtf8().size();
	PRL_RESULT nRetCode = PrlShare_GetName(hShare, sShareName, &nShareNameLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmManipulationsTest::testGetShareNameNullBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG;
	GET_VM_SHARED_FOLDER
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hShare, PrlShare_GetName)
}

void PrlVmManipulationsTest::testSetShareName()
{
	CREATE_TEST_VM;
	SdkHandleWrap hShare;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(hVm, hShare.GetHandlePtr()))
	QString sNewShareName = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP(PrlShare_SetName(hShare, sNewShareName.toUtf8().data()))
	PRL_CHAR sShareName[STR_BUF_LENGTH];
	PRL_UINT32 nShareNameLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlShare_GetName(hShare, sShareName, &nShareNameLength))
	QCOMPARE(UTF8_2QSTR(sShareName), sNewShareName);
}

void PrlVmManipulationsTest::testGetSharePath()
{
	RECEIVE_UPDATED_VM_CONFIG
	GET_VM_SHARED_FOLDER
	PRL_CHAR sSharePath[STR_BUF_LENGTH];
	PRL_UINT32 nSharePathLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlShare_GetPath(hShare, sSharePath, &nSharePathLength))
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	CVmSharedFolder *pSharedFolder = _vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->m_lstSharedFolders.at(0);
	QCOMPARE(UTF8_2QSTR(sSharePath), pSharedFolder->getPath());
	QVERIFY(nSharePathLength == size_t(pSharedFolder->getPath().toUtf8().size()+1));
}

void PrlVmManipulationsTest::testGetSharePathNotEnoughBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG
	GET_VM_SHARED_FOLDER
	PRL_CHAR sSharePath[STR_BUF_LENGTH];
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	PRL_UINT32 nSharePathLength = _vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->m_lstSharedFolders.at(0)->getPath().toUtf8().size();
	PRL_RESULT nRetCode = PrlShare_GetPath(hShare, sSharePath, &nSharePathLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmManipulationsTest::testGetSharePathNullBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG;
	GET_VM_SHARED_FOLDER
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hShare, PrlShare_GetPath)
}

void PrlVmManipulationsTest::testSetSharePath()
{
	CREATE_TEST_VM;
	SdkHandleWrap hShare;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(hVm, hShare.GetHandlePtr()))
	QString sNewSharePath = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP(PrlShare_SetPath(hShare, sNewSharePath.toUtf8().data()))
	PRL_CHAR sSharePath[STR_BUF_LENGTH];
	PRL_UINT32 nSharePathLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlShare_GetPath(hShare, sSharePath, &nSharePathLength))
	QCOMPARE(UTF8_2QSTR(sSharePath), sNewSharePath);
}

void PrlVmManipulationsTest::testGetShareDescription()
{
	RECEIVE_UPDATED_VM_CONFIG
	GET_VM_SHARED_FOLDER
	PRL_CHAR sShareDescription[STR_BUF_LENGTH];
	PRL_UINT32 nShareDescriptionLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlShare_GetDescription(hShare, sShareDescription, &nShareDescriptionLength))
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	CVmSharedFolder *pSharedFolder = _vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->m_lstSharedFolders.at(0);
	QCOMPARE(UTF8_2QSTR(sShareDescription), pSharedFolder->getDescription());
	QVERIFY(nShareDescriptionLength == size_t(pSharedFolder->getDescription().toUtf8().size()+1));
}

void PrlVmManipulationsTest::testGetShareDescriptionNotEnoughBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG
	GET_VM_SHARED_FOLDER
	PRL_CHAR sShareDescription[STR_BUF_LENGTH];
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	PRL_UINT32 nShareDescriptionLength = _vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->m_lstSharedFolders.at(0)->getDescription().toUtf8().size();
	PRL_RESULT nRetCode = PrlShare_GetDescription(hShare, sShareDescription, &nShareDescriptionLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmManipulationsTest::testGetShareDescriptionNullBufSize()
{
	RECEIVE_UPDATED_VM_CONFIG;
	GET_VM_SHARED_FOLDER
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hShare, PrlShare_GetDescription)
}

void PrlVmManipulationsTest::testSetShareDescription()
{
	CREATE_TEST_VM;
	SdkHandleWrap hShare;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(hVm, hShare.GetHandlePtr()))
	QString sNewShareDescription = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP(PrlShare_SetDescription(hShare, sNewShareDescription.toUtf8().data()))
	PRL_CHAR sShareDescription[STR_BUF_LENGTH];
	PRL_UINT32 nShareDescriptionLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlShare_GetDescription(hShare, sShareDescription, &nShareDescriptionLength))
	QCOMPARE(UTF8_2QSTR(sShareDescription), sNewShareDescription);
}

void PrlVmManipulationsTest::testShareIsEnabled()
{
	RECEIVE_UPDATED_VM_CONFIG
	GET_VM_SHARED_FOLDER
	PRL_BOOL bEnabled = (PRL_BOOL) false;
	CHECK_RET_CODE_EXP(PrlShare_IsEnabled(hShare, &bEnabled))
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	QVERIFY(bEnabled == (PRL_BOOL) _vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->m_lstSharedFolders.at(0)->isEnabled());
}

void PrlVmManipulationsTest::testShareSetEnabled()
{
	RECEIVE_UPDATED_VM_CONFIG
	GET_VM_SHARED_FOLDER
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	PRL_BOOL bNewEnabled = !_vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->m_lstSharedFolders.at(0)->isEnabled();
	CHECK_RET_CODE_EXP(PrlShare_SetEnabled(hShare, bNewEnabled))
	PRL_BOOL bActualEnabled;
	CHECK_RET_CODE_EXP(PrlShare_IsEnabled(hShare, &bActualEnabled))
	QVERIFY(bNewEnabled == bActualEnabled);
}

void PrlVmManipulationsTest::testShareIsReadOnly()
{
	RECEIVE_UPDATED_VM_CONFIG
	GET_VM_SHARED_FOLDER
	PRL_BOOL bReadOnly = false;
	CHECK_RET_CODE_EXP(PrlShare_IsReadOnly(hShare, &bReadOnly))
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	QVERIFY(bReadOnly == (PRL_BOOL) _vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->m_lstSharedFolders.at(0)->isReadOnly());
}

void PrlVmManipulationsTest::testShareSetReadOnly()
{
	RECEIVE_UPDATED_VM_CONFIG
	GET_VM_SHARED_FOLDER
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	PRL_BOOL bNewReadOnly = !_vm_conf.getVmSettings()->getVmTools()->getVmSharing()->getHostSharing()->m_lstSharedFolders.at(0)->isReadOnly();
	CHECK_RET_CODE_EXP(PrlShare_SetReadOnly(hShare, bNewReadOnly))
	PRL_BOOL bActualReadOnly;
	CHECK_RET_CODE_EXP(PrlShare_IsReadOnly(hShare, &bActualReadOnly))
	QVERIFY(bNewReadOnly == bActualReadOnly);
}

namespace {
bool FillVmList(SdkHandleWrap hServer, QList<SdkHandleWrap> &_vm_list)
{
	SdkHandleWrap hJob(PrlSrv_GetVmListEx(hServer, PVTF_VM|PVTF_CT));
	if (PRL_SUCCEEDED(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT)))
	{
		PRL_RESULT nRetCode;
		if (PRL_SUCCEEDED(PrlJob_GetRetCode(hJob, &nRetCode)))
		{
			if (PRL_SUCCEEDED(nRetCode))
			{
				SdkHandleWrap hResult;
				if (PRL_SUCCEEDED(PrlJob_GetResult(hJob, hResult.GetHandlePtr())))
				{
					PRL_UINT32 nParamsCount;
					if (PRL_SUCCEEDED(PrlResult_GetParamsCount(hResult, &nParamsCount)))
					{
						for (size_t i = 0; i < nParamsCount; ++i)
						{
							PRL_HANDLE hVm;
							if (!PRL_SUCCEEDED(PrlResult_GetParamByIndex(hResult, i, &hVm)))
							{
								LOG_MESSAGE(DBG_FATAL, "Failed to get param token with index %d", i);
								return (false);
							}
							_vm_list.append(SdkHandleWrap(hVm));
						}
						return (true);
					}
					else
						LOG_MESSAGE(DBG_FATAL, "Failed to get result params count");
				}
				else
					LOG_MESSAGE(DBG_FATAL, "Failed to get job result");
			}
			else
				LOG_MESSAGE(DBG_FATAL, "Failed to get VM list from server");
		}
		else
			LOG_MESSAGE(DBG_FATAL, "Failed to get job return code");
	}
	else
		LOG_MESSAGE(DBG_FATAL, "Job wait failed");
	return (false);
}

}

void PrlVmManipulationsTest::testGetVmHandlesFromDifferentServerHandles()
{
	testCreateVmFromConfig();
	QList<SdkHandleWrap> _vm_list1, _vm_list2;
	SimpleServerWrapper hServer1, hServer2;
	QVERIFY(hServer1.IsConnected());
	QVERIFY(hServer2.IsConnected());
	QVERIFY(FillVmList(hServer1, _vm_list1));
	QVERIFY(FillVmList(hServer2, _vm_list2));
	QCOMPARE(_vm_list1.size(), _vm_list2.size());
	foreach(SdkHandleWrap _vm_handle, _vm_list1)
		QVERIFY(_vm_list2.indexOf(_vm_handle) == -1);
}

void PrlVmManipulationsTest::testVmDevsHandlesNonMadeInvalidOnGetVmConfigCall()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hVmDevFloppyDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hVmDevFloppyDisk.GetHandlePtr()))
	SdkHandleWrap hVmDevHardDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hVmDevHardDisk.GetHandlePtr()))
	SdkHandleWrap hVmDevOpticalDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, hVmDevOpticalDisk.GetHandlePtr()))
	SdkHandleWrap hVmDevParallelPort;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_PARALLEL_PORT, hVmDevParallelPort.GetHandlePtr()))
	SdkHandleWrap hVmDevSerialPort;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SERIAL_PORT, hVmDevSerialPort.GetHandlePtr()))
	SdkHandleWrap hVmDevUsbDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_USB_DEVICE, hVmDevUsbDevice.GetHandlePtr()))
	SdkHandleWrap hVmDevSoundDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_SOUND_DEVICE, hVmDevSoundDev.GetHandlePtr()))
	SdkHandleWrap hVmDevNetAdapter;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, hVmDevNetAdapter.GetHandlePtr()))

	hJob.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	PRL_BOOL bEnabled;
	CHECK_RET_CODE_EXP(PrlVmDev_IsEnabled(hVmDevFloppyDisk, &bEnabled))
	CHECK_RET_CODE_EXP(PrlVmDev_IsEnabled(hVmDevHardDisk, &bEnabled))
	CHECK_RET_CODE_EXP(PrlVmDev_IsEnabled(hVmDevOpticalDisk, &bEnabled))
	CHECK_RET_CODE_EXP(PrlVmDev_IsEnabled(hVmDevParallelPort, &bEnabled))
	CHECK_RET_CODE_EXP(PrlVmDev_IsEnabled(hVmDevSerialPort, &bEnabled))
	CHECK_RET_CODE_EXP(PrlVmDev_IsEnabled(hVmDevNetAdapter, &bEnabled))
	CHECK_RET_CODE_EXP(PrlVmDev_IsEnabled(hVmDevUsbDevice, &bEnabled))
	CHECK_RET_CODE_EXP(PrlVmDev_IsEnabled(hVmDevSoundDev, &bEnabled))
}

void PrlVmManipulationsTest::testVmShareHandleNonMadeInvalidOnFromStringCall()
{
	SKIP_TEST_IN_CT_MODE

	RECEIVE_UPDATED_VM_CONFIG
	SdkHandleWrap hVmShare;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetShare(m_VmHandle, 0, hVmShare.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, sVmConfiguration.toUtf8().data()))
	PRL_BOOL bEnabled;
	CHECK_RET_CODE_EXP(PrlShare_IsEnabled(hVmShare, &bEnabled))
	//Do it twice to check properly mech work
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, sVmConfiguration.toUtf8().data()))
	CHECK_RET_CODE_EXP(PrlShare_IsEnabled(hVmShare, &bEnabled))
}

void PrlVmManipulationsTest::testVmShareHandleMadeInvalidOnRemove()
{
	SKIP_TEST_IN_CT_MODE

	RECEIVE_UPDATED_VM_CONFIG
	SdkHandleWrap hVmShare1, hVmShare2;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetShare(m_VmHandle, 0, hVmShare1.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_GetShare(m_VmHandle, 0, hVmShare2.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlShare_Remove(hVmShare1))
	PRL_BOOL bEnabled;
	QVERIFY(PRL_FAILED(PrlShare_IsEnabled(hVmShare1, &bEnabled)));
	QVERIFY(PRL_FAILED(PrlShare_IsEnabled(hVmShare2, &bEnabled)));
}

void PrlVmManipulationsTest::testVmShareHandleNonMadeInvalidOnGetVmConfigCall()
{
	SKIP_TEST_IN_CT_MODE

	RECEIVE_UPDATED_VM_CONFIG
	SdkHandleWrap hVmShare;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetShare(m_VmHandle, 0, hVmShare.GetHandlePtr()))
	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)
	PRL_BOOL bEnabled;
	CHECK_RET_CODE_EXP(PrlShare_IsEnabled(hVmShare, &bEnabled))
}

void PrlVmManipulationsTest::testVmShareHandleMadeInvalidOnRemoveFromAnotherSession()
{
	SimpleServerWrapper _server1, _server2;
	QVERIFY(_server1.IsConnected());
	QVERIFY(_server2.IsConnected());

	//Create test VM
	QVERIFY(_server1.CreateTestVm());
	SdkHandleWrap hVm1(_server1.GetTestVm());
	CHECK_HANDLE_TYPE(hVm1, PHT_VIRTUAL_MACHINE)
	SdkHandleWrap hVm2(_server2.GetTestVm());
	CHECK_HANDLE_TYPE(hVm2, PHT_VIRTUAL_MACHINE)

	//Create share object
	SdkHandleWrap hJob(PrlVm_BeginEdit(hVm1));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hShare1, hShare2;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateShare(hVm1, hShare1.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlShare_SetName(hShare1, "share1"))
	hJob.reset(PrlVm_Commit(hVm1));
	CHECK_JOB_RET_CODE(hJob)
	hJob.reset(PrlVm_RefreshConfig(hVm2));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlVmCfg_GetShare(hVm2, 0, hShare2.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hShare1, PHT_SHARE)
	CHECK_HANDLE_TYPE(hShare2, PHT_SHARE)

	//Remove share at first connection
	hJob.reset(PrlVm_BeginEdit(hVm1));
	CHECK_JOB_RET_CODE(hJob)
	CHECK_RET_CODE_EXP(PrlShare_Remove(hShare1))
	hJob.reset(PrlVm_Commit(hVm1));
	CHECK_JOB_RET_CODE(hJob)
	PRL_BOOL bEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlShare_IsEnabled(hShare1, &bEnabled), PRL_ERR_OBJECT_WAS_REMOVED)
	//Another object still alive
	CHECK_RET_CODE_EXP(PrlShare_IsEnabled(hShare2, &bEnabled))

	//Update VM config at second connection and check that share object is absent now
	hJob.reset(PrlVm_RefreshConfig(hVm2));
	CHECK_JOB_RET_CODE(hJob)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlShare_IsEnabled(hShare2, &bEnabled), PRL_ERR_OBJECT_WAS_REMOVED)
}

void PrlVmManipulationsTest::testCreateUnattendedWinFloppy()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();
	CHECK_ASYNC_OP_FAILED(PrlVm_CreateUnattendedFloppy(m_VmHandle, PGD_WINDOWS_VISTA, "some_user", "some_company", "some_serial_key"), PRL_ERR_FLOPPY_DRIVE_INVALID)
}

void PrlVmManipulationsTest::testCreateBootDev()
{
	SKIP_TEST_IN_CT_MODE

	CREATE_TEST_VM
	SdkHandleWrap hBootDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(hVm, hBootDev.GetHandlePtr()))
	PRL_HANDLE_TYPE _type = PHT_ERROR;
	CHECK_RET_CODE_EXP(PrlHandle_GetType(hBootDev, &_type))
	QVERIFY(_type == PHT_BOOT_DEVICE);
}

void PrlVmManipulationsTest::testGetBootDevCount()
{
	CREATE_TEST_VM
	SdkHandleWrap hBootDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(hVm, hBootDev.GetHandlePtr()))
	PRL_UINT32 nBootDevCount = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDevCount(hVm, &nBootDevCount))
	QVERIFY(nBootDevCount != 0);
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(hVm)
	QCOMPARE(nBootDevCount, PRL_UINT32(_vm_conf.getVmSettings()->getVmStartupOptions()->m_lstBootDeviceList.size()));
}

void PrlVmManipulationsTest::testGetBootDev()
{
	CREATE_TEST_VM
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(hVm)
	CVmStartupOptions::CVmBootDevice *pBootDev = new CVmStartupOptions::CVmBootDevice;
	_vm_conf.getVmSettings()->getVmStartupOptions()->m_lstBootDeviceList.append(pBootDev);
	CHECK_RET_CODE_EXP(PrlVm_FromString(hVm, _vm_conf.toString().toUtf8().data()))
	SdkHandleWrap hBootDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDev(hVm, 0, hBootDev.GetHandlePtr()))
	PRL_HANDLE_TYPE _type = PHT_ERROR;
	CHECK_RET_CODE_EXP(PrlHandle_GetType(hBootDev, &_type))
	QVERIFY(_type == PHT_BOOT_DEVICE);
}

void PrlVmManipulationsTest::testGetBootDevNonValidIndex()
{
	CREATE_TEST_VM
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(hVm)
	PRL_UINT32 nBootDevIndex = _vm_conf.getVmSettings()->getVmStartupOptions()->m_lstBootDeviceList.size();
	SdkHandleWrap hBootDev;
	QVERIFY(!PRL_SUCCEEDED(PrlVmCfg_GetBootDev(hVm, nBootDevIndex, hBootDev.GetHandlePtr())));
}

#define CHECK_TRY_TO_USE_ALREADY_REMOVED_BOOT_DEVICE(hBootDev)\
	QVERIFY(!PRL_SUCCEEDED(PrlBootDev_Remove(hBootDev)));\
	{\
		PRL_BOOL bInUse;\
		QVERIFY(PRL_FAILED(PrlBootDev_IsInUse(hBootDev, &bInUse)));\
		QVERIFY(PRL_FAILED(PrlBootDev_SetInUse(hBootDev, bInUse)));\
		PRL_UINT32 nIndex;\
		QVERIFY(PRL_FAILED(PrlBootDev_GetIndex(hBootDev, &nIndex)));\
		QVERIFY(PRL_FAILED(PrlBootDev_SetIndex(hBootDev, nIndex)));\
		PRL_UINT32 nSequenceIndex;\
		QVERIFY(PRL_FAILED(PrlBootDev_GetSequenceIndex(hBootDev, &nSequenceIndex)));\
		QVERIFY(PRL_FAILED(PrlBootDev_SetSequenceIndex(hBootDev, nSequenceIndex)));\
		PRL_DEVICE_TYPE nType;\
		QVERIFY(PRL_FAILED(PrlBootDev_GetType(hBootDev, &nType)));\
		QVERIFY(PRL_FAILED(PrlBootDev_SetType(hBootDev, nType)));\
	}

void PrlVmManipulationsTest::testBootDevRemove()
{
	CREATE_TEST_VM
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(hVm)
	CVmStartupOptions::CVmBootDevice *pBootDev = new CVmStartupOptions::CVmBootDevice;
	_vm_conf.getVmSettings()->getVmStartupOptions()->m_lstBootDeviceList.append(pBootDev);
	CHECK_RET_CODE_EXP(PrlVm_FromString(hVm, _vm_conf.toString().toUtf8().data()))
	SdkHandleWrap hBootDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDev(hVm, 0, hBootDev.GetHandlePtr()))
	PRL_UINT32 nBootDevCount1 = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDevCount(hVm, &nBootDevCount1))
	CHECK_RET_CODE_EXP(PrlBootDev_Remove(hBootDev))
	PRL_UINT32 nBootDevCount2 = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDevCount(hVm, &nBootDevCount2))
	QVERIFY((nBootDevCount1 - nBootDevCount2) == 1);
	CHECK_TRY_TO_USE_ALREADY_REMOVED_BOOT_DEVICE(hBootDev)
}

void PrlVmManipulationsTest::testVmBootDevHandleNonMadeInvalidOnFromStringCall()
{
	RECEIVE_UPDATED_VM_CONFIG
	SdkHandleWrap hVmBootDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDev(m_VmHandle, 0, hVmBootDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, sVmConfiguration.toUtf8().data()))
	PRL_BOOL bInUse;
	CHECK_RET_CODE_EXP(PrlBootDev_IsInUse(hVmBootDev, &bInUse))
	//Do it twice to check that mech working properly
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, sVmConfiguration.toUtf8().data()))
	CHECK_RET_CODE_EXP(PrlBootDev_IsInUse(hVmBootDev, &bInUse))
}

void PrlVmManipulationsTest::testVmBootDevHandleMadeInvalidOnRemove()
{
	RECEIVE_UPDATED_VM_CONFIG
	SdkHandleWrap hVmBootDev1, hVmBootDev2;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDev(m_VmHandle, 0, hVmBootDev1.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDev(m_VmHandle, 0, hVmBootDev2.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlBootDev_Remove(hVmBootDev1))
	CHECK_TRY_TO_USE_ALREADY_REMOVED_BOOT_DEVICE(hVmBootDev1)
	CHECK_TRY_TO_USE_ALREADY_REMOVED_BOOT_DEVICE(hVmBootDev2)
}

void PrlVmManipulationsTest::testVmBootDevHandleNonMadeInvalidOnGetVmConfigCall()
{
	SKIP_TEST_IN_CT_MODE

	RECEIVE_UPDATED_VM_CONFIG
	SdkHandleWrap hVmBootDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDev(m_VmHandle, 0, hVmBootDev.GetHandlePtr()))
	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)
	PRL_BOOL bInUse;
	CHECK_RET_CODE_EXP(PrlBootDev_IsInUse(hVmBootDev, &bInUse))
}

void PrlVmManipulationsTest::testVmBootDevHandleMadeInvalidOnRemoveFromAnotherSession()
{
	SimpleServerWrapper _server1, _server2;
	QVERIFY(_server1.IsConnected());
	QVERIFY(_server2.IsConnected());

	//Create test VM
	QVERIFY(_server1.CreateTestVm());
	SdkHandleWrap hVm1(_server1.GetTestVm());
	CHECK_HANDLE_TYPE(hVm1, PHT_VIRTUAL_MACHINE)
	SdkHandleWrap hVm2(_server2.GetTestVm());
	CHECK_HANDLE_TYPE(hVm2, PHT_VIRTUAL_MACHINE)

	//Create BootDev object
	SdkHandleWrap hJob(PrlVm_BeginEdit(hVm1));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hBootDev1, hBootDev2;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateBootDev(hVm1, hBootDev1.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDev1, PDE_HARD_DISK))
	CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDev1, 0))
	hJob.reset(PrlVm_Commit(hVm1));
	CHECK_JOB_RET_CODE(hJob)
	hJob.reset(PrlVm_RefreshConfig(hVm2));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDev(hVm2, 0, hBootDev2.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hBootDev1, PHT_BOOT_DEVICE)
	CHECK_HANDLE_TYPE(hBootDev2, PHT_BOOT_DEVICE)

	//Remove BootDev at first connection
	hJob.reset(PrlVm_BeginEdit(hVm1));
	CHECK_JOB_RET_CODE(hJob)
	CHECK_RET_CODE_EXP(PrlBootDev_Remove(hBootDev1))
	hJob.reset(PrlVm_Commit(hVm1));
	CHECK_JOB_RET_CODE(hJob)
	PRL_BOOL bInUse;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlBootDev_IsInUse(hBootDev1, &bInUse), PRL_ERR_OBJECT_WAS_REMOVED)
	//Another object still alive
	CHECK_RET_CODE_EXP(PrlBootDev_IsInUse(hBootDev2, &bInUse))

	//Update VM config at second connection and check that BootDev object is absent now
	hJob.reset(PrlVm_RefreshConfig(hVm2));
	CHECK_JOB_RET_CODE(hJob)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlBootDev_IsInUse(hBootDev2, &bInUse), PRL_ERR_OBJECT_WAS_REMOVED)
}

#define GET_VM_BOOT_DEV\
	SdkHandleWrap hBootDev;\
	CHECK_RET_CODE_EXP(PrlVmCfg_GetBootDev(m_VmHandle, 0, hBootDev.GetHandlePtr()))

void PrlVmManipulationsTest::testBootDevGetType()
{
	RECEIVE_UPDATED_VM_CONFIG
	GET_VM_BOOT_DEV
	PRL_DEVICE_TYPE nDeviceType = PDE_GENERIC_DEVICE;
	CHECK_RET_CODE_EXP(PrlBootDev_GetType(hBootDev, &nDeviceType))
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	QVERIFY(nDeviceType == _vm_conf.getVmSettings()->getVmStartupOptions()->m_lstBootDeviceList.at(0)->deviceType);
}

void PrlVmManipulationsTest::testBootDevSetType()
{
	RECEIVE_UPDATED_VM_CONFIG
	GET_VM_BOOT_DEV
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	PRL_DEVICE_TYPE nOldDeviceType = _vm_conf.getVmSettings()->getVmStartupOptions()->m_lstBootDeviceList.at(0)->deviceType;
	PRL_DEVICE_TYPE nNewDeviceType = (nOldDeviceType == PDE_FLOPPY_DISK ? PDE_OPTICAL_DISK : PDE_FLOPPY_DISK);
	CHECK_RET_CODE_EXP(PrlBootDev_SetType(hBootDev, nNewDeviceType))
	PRL_DEVICE_TYPE nActualDeviceType = PDE_GENERIC_DEVICE;
	CHECK_RET_CODE_EXP(PrlBootDev_GetType(hBootDev, &nActualDeviceType))
	QVERIFY(nNewDeviceType == nActualDeviceType);
}

void PrlVmManipulationsTest::testBootDevGetIndex()
{
	RECEIVE_UPDATED_VM_CONFIG
	GET_VM_BOOT_DEV
	PRL_UINT32 nDeviceIndex = 0;
	CHECK_RET_CODE_EXP(PrlBootDev_GetIndex(hBootDev, &nDeviceIndex))
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	QVERIFY(nDeviceIndex == _vm_conf.getVmSettings()->getVmStartupOptions()->m_lstBootDeviceList.at(0)->deviceIndex);
}

void PrlVmManipulationsTest::testBootDevSetIndex()
{
	RECEIVE_UPDATED_VM_CONFIG
	GET_VM_BOOT_DEV
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	PRL_UINT32 nOldDeviceIndex = _vm_conf.getVmSettings()->getVmStartupOptions()->m_lstBootDeviceList.at(0)->deviceIndex;
	PRL_UINT32 nNewDeviceIndex = (nOldDeviceIndex == 0 ? 1 : 0);
	CHECK_RET_CODE_EXP(PrlBootDev_SetIndex(hBootDev, nNewDeviceIndex))
	PRL_UINT32 nActualDeviceIndex = 0;
	CHECK_RET_CODE_EXP(PrlBootDev_GetIndex(hBootDev, &nActualDeviceIndex))
	QVERIFY(nNewDeviceIndex == nActualDeviceIndex);
}

void PrlVmManipulationsTest::testBootDevGetSequenceIndex()
{
	RECEIVE_UPDATED_VM_CONFIG
	GET_VM_BOOT_DEV
	PRL_UINT32 nSequenceIndex = 0;
	CHECK_RET_CODE_EXP(PrlBootDev_GetSequenceIndex(hBootDev, &nSequenceIndex))
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	QVERIFY(nSequenceIndex == _vm_conf.getVmSettings()->getVmStartupOptions()->m_lstBootDeviceList.at(0)->sequenceNumber);
}

void PrlVmManipulationsTest::testBootDevSetSequenceIndex()
{
	RECEIVE_UPDATED_VM_CONFIG
	GET_VM_BOOT_DEV
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	PRL_UINT32 nOldSequenceIndex = _vm_conf.getVmSettings()->getVmStartupOptions()->m_lstBootDeviceList.at(0)->sequenceNumber;
	PRL_UINT32 nNewSequenceIndex = (nOldSequenceIndex == 0 ? 1 : 0);
	CHECK_RET_CODE_EXP(PrlBootDev_SetSequenceIndex(hBootDev, nNewSequenceIndex))
	PRL_UINT32 nActualSequenceIndex = 0;
	CHECK_RET_CODE_EXP(PrlBootDev_GetSequenceIndex(hBootDev, &nActualSequenceIndex))
	QVERIFY(nNewSequenceIndex == nActualSequenceIndex);
}

void PrlVmManipulationsTest::testBootDevIsInUse()
{
	RECEIVE_UPDATED_VM_CONFIG
	GET_VM_BOOT_DEV
	PRL_BOOL bIsInUse = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlBootDev_IsInUse(hBootDev, &bIsInUse))
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	QVERIFY(bIsInUse == (PRL_BOOL) _vm_conf.getVmSettings()->getVmStartupOptions()->m_lstBootDeviceList.at(0)->inUseStatus);
}

void PrlVmManipulationsTest::testBootDevSetInUse()
{
	RECEIVE_UPDATED_VM_CONFIG
	GET_VM_BOOT_DEV
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(m_VmHandle)
	PRL_BOOL bOldInUse = (PRL_BOOL)_vm_conf.getVmSettings()->getVmStartupOptions()->m_lstBootDeviceList.at(0)->inUseStatus;
	PRL_BOOL bNewInUse = (bOldInUse == PRL_TRUE ? PRL_FALSE : PRL_TRUE);
	CHECK_RET_CODE_EXP(PrlBootDev_SetInUse(hBootDev, bNewInUse))
	PRL_BOOL bActualInUse = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlBootDev_IsInUse(hBootDev, &bActualInUse))
	QVERIFY(bNewInUse == bActualInUse);
}

void PrlVmManipulationsTest::testGetParamByIndexAsStringOnGetVmListRequest()
{
	testCreateVmFromConfig();
	SdkHandleWrap hJob(PrlSrv_GetVmListEx(m_ServerHandle, PVTF_VM|PVTF_CT));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	//Request necessary string buffer size
	PRL_UINT32 nParamBufSize = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndexAsString(hResult, 0, 0, &nParamBufSize))
	QVERIFY(nParamBufSize != 0);
	PRL_STR sParamBuf = (PRL_STR)malloc(nParamBufSize*sizeof(PRL_CHAR));
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndexAsString(hResult, 0, sParamBuf, &nParamBufSize))
	QString sVmConfig = UTF8_2QSTR(sParamBuf);
	free(sParamBuf);
	SmartPtr<CVmConfiguration> pVmConfig( new CVmConfiguration );
	CHECK_RET_CODE_EXP(pVmConfig->fromString(sVmConfig))
}

void PrlVmManipulationsTest::testGetParamByIndexAsStringOnGetVmListRequestWrongParamIndex()
{
	SdkHandleWrap hJob(PrlSrv_GetVmListEx(m_ServerHandle, PVTF_VM|PVTF_CT));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	PRL_UINT32 nParamsCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))
	PRL_CHAR sParamBuf[STR_BUF_LENGTH];
	PRL_UINT32 nParamBufSize = sizeof(sParamBuf);
	QVERIFY(PRL_FAILED(PrlResult_GetParamByIndexAsString(hResult, nParamBufSize, sParamBuf, &nParamBufSize)));
}

void PrlVmManipulationsTest::testGetParamAsStringOnGetVmListRequest()
{
	testCreateVmFromConfig();
	SdkHandleWrap hJob(PrlSrv_GetVmListEx(m_ServerHandle, PVTF_VM|PVTF_CT));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	//Request necessary string buffer size
	PRL_UINT32 nParamBufSize = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, 0, &nParamBufSize))
	QVERIFY(nParamBufSize != 0);
	PRL_STR sParamBuf = (PRL_STR)malloc(nParamBufSize*sizeof(PRL_CHAR));
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, sParamBuf, &nParamBufSize))
	QString sVmConfig = UTF8_2QSTR(sParamBuf);
	free(sParamBuf);
	SmartPtr<CVmConfiguration> pVmConfig( new CVmConfiguration );
	CHECK_RET_CODE_EXP(pVmConfig->fromString(sVmConfig))
}

#define CHECK_ANSWER_EVENT\
	PRL_VOID_PTR pVoidBuf = NULL;\
	CHECK_RET_CODE_EXP(PrlEvent_GetDataPtr(hAnswerEvent, &pVoidBuf))\
	QString sVmEvent = UTF8_2QSTR((const char *)pVoidBuf);\
	PrlBuffer_Free(pVoidBuf);\
	SmartPtr<CVmEvent> pEvent( new CVmEvent );\
	CHECK_RET_CODE_EXP(pEvent->fromString(sVmEvent))\
	QVERIFY(pEvent->getEventCode() == PET_DSP_EVT_VM_QUESTION);\
	QVERIFY(pEvent->m_lstEventParameters.size() == 2);\
	PRL_CHAR sVmUuid[STR_BUF_LENGTH];\
	PRL_UINT32 nVmUuidSize = sizeof(sVmUuid);\
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUuid(m_VmHandle, sVmUuid, &nVmUuidSize))\
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_UUID, PVE::String, UTF8_2QSTR(sVmUuid))\
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MESSAGE_CHOICE_0, PVE::UnsignedInt, QString("%1").arg(PET_ANSWER_YES))

void PrlVmManipulationsTest::testCreateAnswerEvent()
{
	testCreateVmFromConfig();
	SdkHandleWrap hAnswerEvent;
	CHECK_RET_CODE_EXP(PrlVm_CreateAnswerEvent(m_VmHandle, hAnswerEvent.GetHandlePtr(), PET_ANSWER_YES))
	CHECK_HANDLE_TYPE(hAnswerEvent, PHT_EVENT)
	CHECK_ANSWER_EVENT
}

void PrlVmManipulationsTest::testCreateAnswerEventFromVmEvent()
{
	testCreateVmFromConfig();
	SdkHandleWrap hQuestionEvent;
	CHECK_RET_CODE_EXP(PrlVm_CreateEvent(m_VmHandle, hQuestionEvent.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hQuestionEvent, PHT_EVENT)
	{
		SmartPtr<CVmEvent> pEvent( new CVmEvent );
		pEvent->setEventCode(PET_DSP_EVT_VM_QUESTION);
		PRL_CHAR sVmUuid[STR_BUF_LENGTH];
		PRL_UINT32 nVmUuidSize = sizeof(sVmUuid);
		CHECK_RET_CODE_EXP(PrlVmCfg_GetUuid(m_VmHandle, sVmUuid, &nVmUuidSize))
		pEvent->setEventIssuerId(UTF8_2QSTR(sVmUuid));
		CHECK_RET_CODE_EXP(PrlEvent_FromString(hQuestionEvent, pEvent->toString().toUtf8().constData()))
	}
	SdkHandleWrap hAnswerEvent;
	CHECK_RET_CODE_EXP(PrlEvent_CreateAnswerEvent(hQuestionEvent, hAnswerEvent.GetHandlePtr(), PET_ANSWER_YES))
	CHECK_HANDLE_TYPE(hAnswerEvent, PHT_EVENT)
	CHECK_ANSWER_EVENT
}

namespace {
struct StatisticsInfoStruct
{
	QWaitCondition m_Condition;
	QMutex m_Mutex;
	SdkHandleWrap m_hStatistics;
	SdkHandleWrap m_hVm;
	QString m_sVmUuid;
};

PRL_RESULT StatisticsCallback(PRL_HANDLE _handle, void *pData)
{
	SdkHandleWrap hEvent(_handle);
	PRL_HANDLE_TYPE _type;
	PRL_RESULT nRetCode = PrlHandle_GetType(hEvent, &_type);
	if (PRL_SUCCEEDED(nRetCode))
	{
		if (_type == PHT_EVENT)
		{
			PRL_EVENT_TYPE _event_type;
			nRetCode = PrlEvent_GetType(hEvent, &_event_type);
			if (PRL_SUCCEEDED(nRetCode))
			{
				if (_event_type == PET_DSP_EVT_VM_STATISTICS_UPDATED)
				{
					StatisticsInfoStruct *pStatisticsStruct = static_cast<StatisticsInfoStruct *>(pData);
					QMutexLocker _lock(&pStatisticsStruct->m_Mutex);
					nRetCode = PrlVm_UnregEventHandler(pStatisticsStruct->m_hVm, StatisticsCallback, pData);
					if (PRL_FAILED(nRetCode))
						WRITE_TRACE(DBG_FATAL, "PrlSrv_UnregEventHandler() call failed. Error code: %.8X", nRetCode);
					PRL_UINT32 nParamsCount = 0;
					nRetCode = PrlEvent_GetParamsCount(hEvent, &nParamsCount);
					if (PRL_SUCCEEDED(nRetCode))
					{
						PRL_CHAR sIssuerIdBuf[STR_BUF_LENGTH];
						PRL_UINT32 nIssuerIdBufLength = sizeof(sIssuerIdBuf);
						nRetCode = PrlEvent_GetIssuerId(hEvent, sIssuerIdBuf, &nIssuerIdBufLength);
						if (PRL_SUCCEEDED(nRetCode))
							pStatisticsStruct->m_sVmUuid = UTF8_2QSTR(sIssuerIdBuf);
						else
							WRITE_TRACE(DBG_FATAL, "PrlEvent_GetIssuerId() call failed. Error code: %.8X", nRetCode);

						for (PRL_UINT32 i = 0; i < nParamsCount; ++i)
						{
							SdkHandleWrap hEventParameter;
							nRetCode = PrlEvent_GetParam(hEvent, i, hEventParameter.GetHandlePtr());
							if (PRL_SUCCEEDED(nRetCode))
							{
								PRL_CHAR sParamNameBuf[STR_BUF_LENGTH];
								PRL_UINT32 nParamNameBufLength = sizeof(sParamNameBuf);
								nRetCode = PrlEvtPrm_GetName(hEventParameter, sParamNameBuf, &nParamNameBufLength);
								if (PRL_SUCCEEDED(nRetCode))
								{
									if (UTF8_2QSTR(sParamNameBuf) == UTF8_2QSTR(EVT_PARAM_STATISTICS))
									{
										nRetCode = PrlEvtPrm_ToHandle(hEventParameter,
																pStatisticsStruct->m_hStatistics.GetHandlePtr());
										if (PRL_SUCCEEDED(nRetCode))
										{
											pStatisticsStruct->m_Condition.wakeAll();
											break;
										}
										else
											WRITE_TRACE(DBG_FATAL, "PrlEvtPrm_ToHandle() call failed. Error code: %.8X", nRetCode);
									}
								}
								else
									WRITE_TRACE(DBG_FATAL, "PrlEvtPrm_GetName() call failed. Error code: %.8X", nRetCode);
							}
							else
								WRITE_TRACE(DBG_FATAL, "PrlEvent_GetParam() call failed. Error code: %.8X", nRetCode);
						}
					}
					else
						WRITE_TRACE(DBG_FATAL, "PrlEvent_GetParamsCount() call failed. Error code: %.8X", nRetCode);
				}
			}
			else
				WRITE_TRACE(DBG_FATAL, "PrlEvent_GetType() call failed. Error: %.8X", nRetCode);
		}
	}
	else
		WRITE_TRACE(DBG_FATAL, "PrlHandle_GetType() call failed. Error code: %.8X", nRetCode);

	return PRL_ERR_SUCCESS;
}

}

void PrlVmManipulationsTest::testSubscribeToGuestStatistics()
{
	testCreateVmFromConfig();
	SmartPtr<StatisticsInfoStruct> pStatInfo( new StatisticsInfoStruct );
	pStatInfo->m_hVm = m_VmHandle;
	CHECK_RET_CODE_EXP(PrlVm_RegEventHandler(m_VmHandle, StatisticsCallback, pStatInfo.getImpl()))
	SdkHandleWrap hJob;
	bool bRes = false;
	{
		QMutexLocker _lock(&pStatInfo->m_Mutex);
		hJob.reset(PrlVm_SubscribeToGuestStatistics(m_VmHandle));
		bRes = pStatInfo->m_Condition.wait(&pStatInfo->m_Mutex, PRL_JOB_WAIT_TIMEOUT);
	}
	CHECK_JOB_RET_CODE(hJob)
	hJob.reset(PrlVm_UnsubscribeFromGuestStatistics(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)
	QVERIFY(bRes);
	QMutexLocker _lock(&pStatInfo->m_Mutex);
	CHECK_HANDLE_TYPE(pStatInfo->m_hStatistics, PHT_SYSTEM_STATISTICS)
	PRL_CHAR sVmUuidBuf[STR_BUF_LENGTH];
	PRL_UINT32 nVmUuidBufLength = sizeof(sVmUuidBuf);
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUuid(m_VmHandle, sVmUuidBuf, &nVmUuidBufLength))
	QCOMPARE(pStatInfo->m_sVmUuid, UTF8_2QSTR(sVmUuidBuf));
}

void PrlVmManipulationsTest::testGetStatistics()
{
	SKIP_TEST_IN_CT_MODE_unimp
	testCreateVmFromConfig();
	SdkHandleWrap hJob(PrlVm_GetStatistics(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	QByteArray sBuf;
	PRL_UINT32 nBufSize = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, 0, &nBufSize))
	sBuf.resize(nBufSize);
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, sBuf.data(), &nBufSize))
	CSystemStatistics _vm_stat;
	CHECK_RET_CODE_EXP(_vm_stat.fromString(UTF8_2QSTR(sBuf)))
}

void PrlVmManipulationsTest::testCreateVmWithOsDiv2Name()
{
	SKIP_TEST_IN_CT_MODE

	INITIALIZE_VM("./TestDspCmdDirValidateVmConfig_vm_config.xml")

	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, "OS/2 Warp 4"))

	m_JobHandle.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(m_JobHandle)
}

void PrlVmManipulationsTest::testCreateVmWithOsDiv2Name2()
{
	SKIP_TEST_IN_CT_MODE

	INITIALIZE_VM("./TestDspCmdDirValidateVmConfig_vm_config.xml")

	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, "OS\\2 Warp 4"))

	m_JobHandle.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(m_JobHandle)
}

void PrlVmManipulationsTest::testChangeVmNameOnValueWithIncorrectPathSymbols()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, "OS/2 Warp 4"))

	hJob.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	QByteArray sVmName;
	PRL_UINT32 nBufSize = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetName(m_VmHandle, 0, &nBufSize))
	sVmName.resize(nBufSize);
	CHECK_RET_CODE_EXP(PrlVmCfg_GetName(m_VmHandle, sVmName.data(), &nBufSize))

	QCOMPARE(UTF8_2QSTR(sVmName), UTF8_2QSTR("OS2 Warp 4"));

	QByteArray sVmHomePath;
	nBufSize = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHomePath(m_VmHandle, 0, &nBufSize))
	sVmHomePath.resize(nBufSize);
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHomePath(m_VmHandle, sVmHomePath.data(), &nBufSize))

	QVERIFY(!UTF8_2QSTR(sVmHomePath).contains("OS/2 Warp 4"));
	QVERIFY(UTF8_2QSTR(sVmHomePath).contains("OS2 Warp 4"));
}

void PrlVmManipulationsTest::testChangeVmNameOnValueWithIncorrectPathSymbols2()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, "OS\\2 Warp 4"))

	hJob.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	QByteArray sVmName;
	PRL_UINT32 nBufSize = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetName(m_VmHandle, 0, &nBufSize))
	sVmName.resize(nBufSize);
	CHECK_RET_CODE_EXP(PrlVmCfg_GetName(m_VmHandle, sVmName.data(), &nBufSize))

	QCOMPARE(UTF8_2QSTR(sVmName), UTF8_2QSTR("OS2 Warp 4"));

	QByteArray sVmHomePath;
	nBufSize = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHomePath(m_VmHandle, 0, &nBufSize))
	sVmHomePath.resize(nBufSize);
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHomePath(m_VmHandle, sVmHomePath.data(), &nBufSize))

	QVERIFY(!UTF8_2QSTR(sVmHomePath).contains("OS\\2 Warp 4"));
	QVERIFY(UTF8_2QSTR(sVmHomePath).contains("OS2 Warp 4"));
}

void PrlVmManipulationsTest::testChangeVmNameOnValueWithIncorrectPathSymbols3()
{
	SKIP_TEST_IN_CT_MODE_unimp

	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	QString qsVmName;
	PRL_EXTRACT_STRING_VALUE(qsVmName, m_VmHandle, PrlVmCfg_GetName);
	QString qsVmHomePath;
	PRL_EXTRACT_STRING_VALUE(qsVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath);

	hJob.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	QByteArray sNewVmName = (qsVmName + "  \\\\/").toUtf8();
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, sNewVmName.data()))

	hJob.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	QString qsNewVmName;
	PRL_EXTRACT_STRING_VALUE(qsNewVmName, m_VmHandle, PrlVmCfg_GetName);
	QString qsNewVmHomePath;
	PRL_EXTRACT_STRING_VALUE(qsNewVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath);

	QCOMPARE(qsVmName, qsNewVmName);
	QCOMPARE(qsVmHomePath, qsNewVmHomePath);
}

void PrlVmManipulationsTest::testCloneVmOnValueWithIncorrectPathSymbols()
{
	SKIP_TEST_IN_CT_MODE_unimp

	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_Clone(m_VmHandle, "OS/2 Warp 4", "", PRL_FALSE));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	m_ClonedVmHandle.reset();

	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_ClonedVmHandle.GetHandlePtr()))

	QVERIFY(m_ClonedVmHandle != PRL_INVALID_HANDLE);
	CHECK_HANDLE_TYPE(m_ClonedVmHandle, PHT_VIRTUAL_MACHINE)

	QString sVmName;
	PRL_EXTRACT_STRING_VALUE(sVmName, m_ClonedVmHandle, PrlVmCfg_GetName)
	QCOMPARE(sVmName, UTF8_2QSTR("OS2 Warp 4"));
}

void PrlVmManipulationsTest::testCloneVmOnValueWithIncorrectPathSymbols2()
{
	SKIP_TEST_IN_CT_MODE_unimp
	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_Clone(m_VmHandle, "OS\\2 Warp 4", "", PRL_FALSE));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	m_ClonedVmHandle.reset();

	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_ClonedVmHandle.GetHandlePtr()))

	QVERIFY(m_ClonedVmHandle != PRL_INVALID_HANDLE);
	CHECK_HANDLE_TYPE(m_ClonedVmHandle, PHT_VIRTUAL_MACHINE)

	QString sVmName;
	PRL_EXTRACT_STRING_VALUE(sVmName, m_ClonedVmHandle, PrlVmCfg_GetName)
	QCOMPARE(sVmName, UTF8_2QSTR("OS2 Warp 4"));
}

void PrlVmManipulationsTest::testCreateVmWithPercentageSymbolInName()
{
	SKIP_TEST_IN_CT_MODE_unimp
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm( m_ServerHandle, m_VmHandle.GetHandlePtr() ))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig( m_VmHandle, PRL_INVALID_HANDLE, PVS_GUEST_VER_WIN_2003, PRL_FALSE ))
	QString sTestVmName = "QtSpecName%2";
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName( m_VmHandle, QSTR2UTF8(sTestVmName) ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Reg( m_VmHandle, "", PRL_TRUE ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_RefreshConfig( m_VmHandle ))
	QString sVmHomePath;
	PRL_EXTRACT_STRING_VALUE(sVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath)
	QVERIFY(!sVmHomePath.contains( sTestVmName ));
	QVERIFY(sVmHomePath.contains( CFileHelper::ReplaceNonValidPathSymbols( sTestVmName ) ));
}

void PrlVmManipulationsTest::testCreateImageOnFloppyDevice()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	CAuthHelper _auth_helper(TestConfig::getUserLogin());
	if (TestConfig::isServerMode())
		QVERIFY(_auth_helper.AuthUser(TestConfig::getUserPassword()));
	else
		QVERIFY(_auth_helper.AuthUser(PrlGetCurrentUserId()));

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	QByteArray sVmHomePath;
	PRL_UINT32 nVmHomePathBufSize = 0;

	CHECK_RET_CODE_EXP(PrlVmCfg_GetHomePath(m_VmHandle, 0, &nVmHomePathBufSize))
	sVmHomePath.resize(nVmHomePathBufSize);
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHomePath(m_VmHandle, sVmHomePath.data(), &nVmHomePathBufSize))

	QString sFloppyImagePath = QFileInfo(UTF8_2QSTR(sVmHomePath)).path() + "/floppy.img";

	CVmFloppyDisk floppy;
	floppy.setIndex( 0 );
	floppy.setEmulatedType( PVE::FloppyDiskImage );
	floppy.setEnabled( PVE::DeviceEnabled );
	floppy.setConnected( PVE::DeviceConnected );
	floppy.setUserFriendlyName( sFloppyImagePath );
	floppy.setSystemName( sFloppyImagePath );
	QString floppyXMLstr = ElementToString<CVmFloppyDisk*>(&floppy, XML_VM_CONFIG_EL_HARDWARE );

	hJob.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hVmDev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, hVmDev.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmDev_FromString(hVmDev, floppyXMLstr.toUtf8().data()))
	hJob.reset(PrlVmDev_CreateImage(hVmDev, 0, PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)
	QVERIFY(CFileHelper::FileExists(sFloppyImagePath, &_auth_helper));

	hJob.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlVmManipulationsTest::testIsTemplate()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();
	PRL_BOOL bVmIsTemplate;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsTemplate(m_VmHandle, &bVmIsTemplate))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bVmIsTemplate == PRL_BOOL(_vm_conf.getVmSettings()->getVmCommonOptions()->isTemplate()));
}

void PrlVmManipulationsTest::testSetTemplateSign()
{
	CREATE_TEST_VM
	PRL_BOOL bVmIsTemplate;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsTemplate(hVm, &bVmIsTemplate))
	bVmIsTemplate = !bVmIsTemplate;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetTemplateSign(hVm, bVmIsTemplate))
	PRL_BOOL bActualVmIsTemplate;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsTemplate(hVm, &bActualVmIsTemplate))
	QCOMPARE(bActualVmIsTemplate, bVmIsTemplate);
}

#define CHECK_EVENT_PARAMETER2(param_name, param_type, param_value)\
	{\
		SdkHandleWrap hEventParam;\
		CHECK_RET_CODE_EXP(PrlEvent_GetParamByName(_event_handle, param_name, hEventParam.GetHandlePtr()))\
		CHECK_HANDLE_TYPE(hEventParam, PHT_EVENT_PARAMETER)\
		PRL_PARAM_FIELD_DATA_TYPE nParamType;\
		CHECK_RET_CODE_EXP(PrlEvtPrm_GetType(hEventParam, &nParamType))\
		QVERIFY(nParamType == param_type);\
		QByteArray sParamValue;\
		PRL_UINT32 nParamValueBufSize = 0;\
		CHECK_RET_CODE_EXP(PrlEvtPrm_ToString(hEventParam, 0, &nParamValueBufSize))\
		QVERIFY(nParamValueBufSize != 0);\
		sParamValue.resize(nParamValueBufSize);\
		CHECK_RET_CODE_EXP(PrlEvtPrm_ToString(hEventParam, sParamValue.data(), &nParamValueBufSize))\
		QCOMPARE(UTF8_2QSTR(sParamValue), UTF8_2QSTR(param_value));\
	}

void PrlVmManipulationsTest::testVmEventProcessing1()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))
	SdkHandleWrap _event_handle;
	CHECK_RET_CODE_EXP(PrlVm_CreateAnswerEvent(m_VmHandle, _event_handle.GetHandlePtr(), PRL_ERR_SUCCESS))
	QFile _file("./CVmEventTest_test_data1.xml");
	QVERIFY(_file.open(QIODevice::ReadOnly));
	QDomDocument _xmldoc;
	QVERIFY(_xmldoc.setContent(&_file));
	_file.close();
	CHECK_RET_CODE_EXP(PrlEvent_FromString(_event_handle, _xmldoc.toString().toUtf8().constData()))
	PRL_UINT32 nParamsCount = 0;
	CHECK_RET_CODE_EXP(PrlEvent_GetParamsCount(_event_handle, &nParamsCount))
	QCOMPARE(quint32(nParamsCount), quint32(5));
	CHECK_EVENT_PARAMETER2("vm_message", PFD_UINT32, "1002")
	CHECK_EVENT_PARAMETER2("vm_message_choice_0", PFD_UINT32, "2002")
	CHECK_EVENT_PARAMETER2("vm_message_choice_1", PFD_UINT32, "2003")
	CHECK_EVENT_PARAMETER2("vm_message_param_0", PFD_STRING, "C:\\Parallels Virtual Machines\\DOS\\lpt.txt")
	CHECK_EVENT_PARAMETER2("executive_server", PFD_STRING, "localhost")
}

void PrlVmManipulationsTest::testCreateVmWithDevices()
{
	SKIP_TEST_IN_CT_MODE

	INITIALIZE_VM("./SDKTest_vm_with_devices_config.xml")

	QList<SdkHandleWrap> _vm_devices;
	QByteArray _buffer;
	PRL_UINT32 nDevCount = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDevsCount(m_VmHandle, &nDevCount))
	_buffer.resize(sizeof(PRL_HANDLE)*nDevCount);
	PRL_HANDLE_PTR pDevicesList = (PRL_HANDLE_PTR)_buffer.data();
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDevsList(m_VmHandle, pDevicesList, &nDevCount))
	for (PRL_UINT32 i = 0; i < nDevCount; i++)
		_vm_devices.append(SdkHandleWrap(pDevicesList[i]));

	QStringList _vm_devices_sys_names;
	foreach(SdkHandleWrap _handle, _vm_devices)
	{
		PRL_VM_DEV_EMULATION_TYPE _vm_dev_type;
		CHECK_RET_CODE_EXP(PrlVmDev_GetEmulatedType(_handle, &_vm_dev_type))
		if (_vm_dev_type == PDT_USE_IMAGE_FILE)
		{
			QByteArray sDeviceSystemName;
			PRL_UINT32 nDeviceSystemNameBufSize = 0;
			CHECK_RET_CODE_EXP(PrlVmDev_GetSysName(_handle, 0, &nDeviceSystemNameBufSize));
			QVERIFY(nDeviceSystemNameBufSize > 0);
			sDeviceSystemName.resize(nDeviceSystemNameBufSize);
			CHECK_RET_CODE_EXP(PrlVmDev_GetSysName(_handle, sDeviceSystemName.data(), &nDeviceSystemNameBufSize))
			_vm_devices_sys_names.append(UTF8_2QSTR(sDeviceSystemName));
		}
	}
	QVERIFY(_vm_devices_sys_names.size() > 0);

	m_JobHandle.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(m_JobHandle)

	m_JobHandle.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)

	QByteArray sVmHomeDir;
	PRL_UINT32 nVmHomeDirBufSize = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHomePath(m_VmHandle, 0, &nVmHomeDirBufSize))
	sVmHomeDir.resize(nVmHomeDirBufSize);
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHomePath(m_VmHandle, sVmHomeDir.data(), &nVmHomeDirBufSize))

	foreach (QString sDeviceSystemName, _vm_devices_sys_names)
	{
		QString sDeviceImagePath = QFileInfo(UTF8_2QSTR(sVmHomeDir)).path() + '/' + sDeviceSystemName;
		bool bRes = QFile::exists(sDeviceImagePath);
		if (!bRes)
			WRITE_TRACE(DBG_FATAL, "Couldn't to find image file by path '%s'", sDeviceImagePath.toUtf8().constData());
		QVERIFY(bRes);
	}
}

#define TEST_VM_CREATION_AND_EDIT\
	SdkHandleWrap hJob(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));\
	CHECK_JOB_RET_CODE(hJob)\
	hJob.reset(PrlVm_BeginEdit(m_VmHandle));\
	CHECK_JOB_RET_CODE(hJob)\
	SdkHandleWrap _hdd_dev;\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, _hdd_dev.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(_hdd_dev, "harddisk.hdd"))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetFriendlyName(_hdd_dev, "harddisk.hdd"))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(_hdd_dev, PRL_TRUE))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(_hdd_dev, PRL_TRUE))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(_hdd_dev, PDT_USE_IMAGE_FILE))\
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskSize(_hdd_dev, 20))\
	hJob.reset(PrlVmDev_CreateImage(_hdd_dev, PRL_FALSE, PRL_TRUE));\
	CHECK_JOB_RET_CODE(hJob)\
	hJob.reset(PrlVm_Commit(m_VmHandle));\
	CHECK_JOB_RET_CODE(hJob)

void PrlVmManipulationsTest::testEditVmWithCdromWhichImageNotExists()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, Uuid::createUuid().toString().toUtf8().constData()))

	SdkHandleWrap _cdrom_dev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_OPTICAL_DISK, _cdrom_dev.GetHandlePtr()))
	QString sImagePath = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(_cdrom_dev, sImagePath.toUtf8().constData()))
	CHECK_RET_CODE_EXP(PrlVmDev_SetFriendlyName(_cdrom_dev, sImagePath.toUtf8().constData()))
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(_cdrom_dev, PRL_TRUE))
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(_cdrom_dev, PRL_TRUE))
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(_cdrom_dev, PDT_USE_IMAGE_FILE))

	TEST_VM_CREATION_AND_EDIT
}

void PrlVmManipulationsTest::testEditVmWithFloppyWhichImageNotExists()
{
	SKIP_TEST_IN_CT_MODE

	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, Uuid::createUuid().toString().toUtf8().constData()))

	SdkHandleWrap _floppy_dev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_FLOPPY_DISK, _floppy_dev.GetHandlePtr()))
	QString sImagePath = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(_floppy_dev, sImagePath.toUtf8().constData()))
	CHECK_RET_CODE_EXP(PrlVmDev_SetFriendlyName(_floppy_dev, sImagePath.toUtf8().constData()))
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(_floppy_dev, PRL_TRUE))
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(_floppy_dev, PRL_TRUE))
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(_floppy_dev, PDT_USE_IMAGE_FILE))

	TEST_VM_CREATION_AND_EDIT
}

void PrlVmManipulationsTest::testEditVmWithHardWhichImageNotExists()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, Uuid::createUuid().toString().toUtf8().constData()))

	SdkHandleWrap _harddisk_dev;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, _harddisk_dev.GetHandlePtr()))
	QString sImagePath = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(_harddisk_dev, sImagePath.toUtf8().constData()))
	CHECK_RET_CODE_EXP(PrlVmDev_SetFriendlyName(_harddisk_dev, sImagePath.toUtf8().constData()))
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(_harddisk_dev, PRL_TRUE))
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(_harddisk_dev, PRL_TRUE))
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(_harddisk_dev, PDT_USE_IMAGE_FILE))
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskSize(_harddisk_dev, 20))

	TEST_VM_CREATION_AND_EDIT
}

#define LOAD_CONFIG_AND_SET_PERMS(perms)\
	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml")\
	CVmConfiguration _vm_conf(_config);\
	QList<PRL_ALLOWED_VM_COMMAND> _access_rights_list;\
	_vm_conf.getVmSecurity()->getAccessControlList()->setAccessControl(_access_rights_list<<perms);\
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()))\
	APPLY_UNIQUE_VM_TEST_NAME\
	SdkHandleWrap hVmAccessRights;\
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAccessRights(m_VmHandle, hVmAccessRights.GetHandlePtr()))\
	CHECK_HANDLE_TYPE(hVmAccessRights, PHT_ACCESS_RIGHTS)

#define CHECK_PERM_ALLOWED(perm, bExpectedValue)\
	{\
		PRL_BOOL bActualValue = PRL_FALSE;\
		CHECK_RET_CODE_EXP(PrlAcl_IsAllowed(hVmAccessRights, perm, &bActualValue))\
		QVERIFY(bActualValue == bExpectedValue);\
	}

void PrlVmManipulationsTest::testVmAccessRightsOnAllPermissionsGranted()
{
	LOAD_CONFIG_AND_SET_PERMS(PAR_VM_START_ACCESS<<PAR_VM_STOP_ACCESS<<PAR_VM_PAUSE_ACCESS<<PAR_VM_RESET_ACCESS<<PAR_VM_SUSPEND_ACCESS<<PAR_VM_RESUME_ACCESS<<PAR_VM_DROPSUSPENDEDSTATE_ACCESS<<PAR_VM_CLONE_ACCESS<<PAR_VM_DELETE_ACCESS<<PAR_VM_GETPROBLEMREPORT_ACCESS<<PAR_VM_GETCONFIG_ACCESS<<PAR_VM_GETSTATISTICS_ACCESS<<PAR_VM_SUBSCRIBETOGUESTSTATISTICS_ACCESS<<PAR_VM_UNSUBSCRIBEFROMGUESTSTATISTICS_ACCESS<<PAR_VM_UNREG_ACCESS<<PAR_VM_BEGINEDIT_ACCESS<<PAR_VM_COMMIT_ACCESS<<PAR_VMDEV_CONNECT_ACCESS<<PAR_VMDEV_DISCONNECT_ACCESS<<PAR_VMDEV_CREATEIMAGE_ACCESS<<PAR_VM_SEND_ANSWER_ACCESS<<PAR_VM_GET_VMINFO_ACCESS<<PAR_VM_INSTALL_TOOLS_ACCESS<<PAR_VM_INITIATE_DEV_STATE_NOTIFICATIONS_ACCESS)

	CHECK_PERM_ALLOWED(PAR_VM_START_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_STOP_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_PAUSE_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_RESET_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_SUSPEND_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_RESUME_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_DROPSUSPENDEDSTATE_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_CLONE_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_DELETE_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_GETPROBLEMREPORT_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_GETCONFIG_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_GETSTATISTICS_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_SUBSCRIBETOGUESTSTATISTICS_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_UNSUBSCRIBEFROMGUESTSTATISTICS_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_UNREG_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_BEGINEDIT_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_COMMIT_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VMDEV_CONNECT_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VMDEV_DISCONNECT_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VMDEV_CREATEIMAGE_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_SEND_ANSWER_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_GET_VMINFO_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_INSTALL_TOOLS_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_INITIATE_DEV_STATE_NOTIFICATIONS_ACCESS, PRL_TRUE)
}

void PrlVmManipulationsTest::testVmAccessRightsOnReadOnly()
{
	LOAD_CONFIG_AND_SET_PERMS(PAR_VM_CLONE_ACCESS<<PAR_VM_GETPROBLEMREPORT_ACCESS<<PAR_VM_GETCONFIG_ACCESS<<PAR_VM_GETSTATISTICS_ACCESS<<PAR_VM_SUBSCRIBETOGUESTSTATISTICS_ACCESS<<PAR_VM_UNSUBSCRIBEFROMGUESTSTATISTICS_ACCESS<<PAR_VM_GET_VMINFO_ACCESS<<PAR_VM_INITIATE_DEV_STATE_NOTIFICATIONS_ACCESS)

	CHECK_PERM_ALLOWED(PAR_VM_START_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_STOP_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_PAUSE_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_RESET_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_SUSPEND_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_RESUME_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_DROPSUSPENDEDSTATE_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_CLONE_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_DELETE_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_GETPROBLEMREPORT_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_GETCONFIG_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_GETSTATISTICS_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_SUBSCRIBETOGUESTSTATISTICS_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_UNSUBSCRIBEFROMGUESTSTATISTICS_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_UNREG_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_BEGINEDIT_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_COMMIT_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VMDEV_CONNECT_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VMDEV_DISCONNECT_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VMDEV_CREATEIMAGE_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_SEND_ANSWER_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_GET_VMINFO_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_INSTALL_TOOLS_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_INITIATE_DEV_STATE_NOTIFICATIONS_ACCESS, PRL_TRUE)
}

void PrlVmManipulationsTest::testVmAccessRightsOnReadWrite()
{
	LOAD_CONFIG_AND_SET_PERMS(PAR_VM_DROPSUSPENDEDSTATE_ACCESS<<PAR_VM_CLONE_ACCESS<<PAR_VM_DELETE_ACCESS<<PAR_VM_GETPROBLEMREPORT_ACCESS<<PAR_VM_GETCONFIG_ACCESS<<PAR_VM_GETSTATISTICS_ACCESS<<PAR_VM_SUBSCRIBETOGUESTSTATISTICS_ACCESS<<PAR_VM_UNSUBSCRIBEFROMGUESTSTATISTICS_ACCESS<<PAR_VM_UNREG_ACCESS<<PAR_VM_BEGINEDIT_ACCESS<<PAR_VM_COMMIT_ACCESS<<PAR_VMDEV_CREATEIMAGE_ACCESS<<PAR_VM_GET_VMINFO_ACCESS<<PAR_VM_INITIATE_DEV_STATE_NOTIFICATIONS_ACCESS)

	CHECK_PERM_ALLOWED(PAR_VM_START_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_STOP_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_PAUSE_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_RESET_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_SUSPEND_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_RESUME_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_DROPSUSPENDEDSTATE_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_CLONE_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_DELETE_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_GETPROBLEMREPORT_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_GETCONFIG_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_GETSTATISTICS_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_SUBSCRIBETOGUESTSTATISTICS_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_UNSUBSCRIBEFROMGUESTSTATISTICS_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_UNREG_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_BEGINEDIT_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_COMMIT_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VMDEV_CONNECT_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VMDEV_DISCONNECT_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VMDEV_CREATEIMAGE_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_SEND_ANSWER_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_GET_VMINFO_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_INSTALL_TOOLS_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_INITIATE_DEV_STATE_NOTIFICATIONS_ACCESS, PRL_TRUE)
}

void PrlVmManipulationsTest::testVmAccessRightsOnReadExecute()
{
	LOAD_CONFIG_AND_SET_PERMS(PAR_VM_START_ACCESS<<PAR_VM_STOP_ACCESS<<PAR_VM_PAUSE_ACCESS<<PAR_VM_RESET_ACCESS<<PAR_VM_SUSPEND_ACCESS<<PAR_VM_RESUME_ACCESS<<PAR_VM_CLONE_ACCESS<<PAR_VM_GETPROBLEMREPORT_ACCESS<<PAR_VM_GETCONFIG_ACCESS<<PAR_VM_GETSTATISTICS_ACCESS<<PAR_VM_SUBSCRIBETOGUESTSTATISTICS_ACCESS<<PAR_VM_UNSUBSCRIBEFROMGUESTSTATISTICS_ACCESS<<PAR_VM_SEND_ANSWER_ACCESS<<PAR_VM_GET_VMINFO_ACCESS<<PAR_VM_INITIATE_DEV_STATE_NOTIFICATIONS_ACCESS)

	CHECK_PERM_ALLOWED(PAR_VM_START_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_STOP_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_PAUSE_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_RESET_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_SUSPEND_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_RESUME_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_DROPSUSPENDEDSTATE_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_CLONE_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_DELETE_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_GETPROBLEMREPORT_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_GETCONFIG_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_GETSTATISTICS_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_SUBSCRIBETOGUESTSTATISTICS_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_UNSUBSCRIBEFROMGUESTSTATISTICS_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_UNREG_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_BEGINEDIT_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_COMMIT_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VMDEV_CONNECT_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VMDEV_DISCONNECT_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VMDEV_CREATEIMAGE_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_SEND_ANSWER_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_GET_VMINFO_ACCESS, PRL_TRUE)
	CHECK_PERM_ALLOWED(PAR_VM_INSTALL_TOOLS_ACCESS, PRL_FALSE)
	CHECK_PERM_ALLOWED(PAR_VM_INITIATE_DEV_STATE_NOTIFICATIONS_ACCESS, PRL_TRUE)
}

#define CHECK_THAT_NO_ALLOWED_PERMISSIONS\
	CHECK_PERM_ALLOWED(PAR_VM_START_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VM_STOP_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VM_PAUSE_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VM_RESET_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VM_SUSPEND_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VM_RESUME_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VM_DROPSUSPENDEDSTATE_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VM_CLONE_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VM_DELETE_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VM_GETPROBLEMREPORT_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VM_GETCONFIG_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VM_GETSTATISTICS_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VM_SUBSCRIBETOGUESTSTATISTICS_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VM_UNSUBSCRIBEFROMGUESTSTATISTICS_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VM_UNREG_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VM_BEGINEDIT_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VM_COMMIT_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VMDEV_CONNECT_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VMDEV_DISCONNECT_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VMDEV_CREATEIMAGE_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VM_SEND_ANSWER_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VM_GET_VMINFO_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VM_INSTALL_TOOLS_ACCESS, PRL_FALSE)\
	CHECK_PERM_ALLOWED(PAR_VM_INITIATE_DEV_STATE_NOTIFICATIONS_ACCESS, PRL_FALSE)

void PrlVmManipulationsTest::testVmAccessRightsOnNoPermissions()
{
	LOAD_CONFIG_AND_SET_PERMS(QList<PRL_ALLOWED_VM_COMMAND>())
	CHECK_THAT_NO_ALLOWED_PERMISSIONS
}

void PrlVmManipulationsTest::testVmGetAccessRightsFailedOnNonVmHandle()
{
	SdkHandleWrap hVmAcl;
	QVERIFY(PRL_FAILED(PrlVmCfg_GetAccessRights(m_ServerHandle, hVmAcl.GetHandlePtr())));
}

void PrlVmManipulationsTest::testVmGetAccessRightsFailedOnNullVmHandle()
{
	SdkHandleWrap hVmAcl;
	QVERIFY(PRL_FAILED(PrlVmCfg_GetAccessRights(PRL_INVALID_HANDLE, hVmAcl.GetHandlePtr())));
}

void PrlVmManipulationsTest::testVmGetAccessRightsFailedOnNullResultBuffer()
{
	LOAD_CONFIG_AND_SET_PERMS(QList<PRL_ALLOWED_VM_COMMAND>())
	QVERIFY(PRL_FAILED(PrlVmCfg_GetAccessRights(m_VmHandle, 0)));
}

void PrlVmManipulationsTest::testAclIsAllowedFailedOnNonAclHandle()
{
	PRL_BOOL bValue;
	QVERIFY(PRL_FAILED(PrlAcl_IsAllowed(m_ServerHandle, PAR_VM_START_ACCESS, &bValue)));
}

void PrlVmManipulationsTest::testAclIsAllowedFailedOnNullAclHandle()
{
	PRL_BOOL bValue;
	QVERIFY(PRL_FAILED(PrlAcl_IsAllowed(PRL_INVALID_HANDLE, PAR_VM_START_ACCESS, &bValue)));
}

void PrlVmManipulationsTest::testAclIsAllowedFailedOnNullResultBuffer()
{
	LOAD_CONFIG_AND_SET_PERMS(QList<PRL_ALLOWED_VM_COMMAND>())
	QVERIFY(PRL_FAILED(PrlAcl_IsAllowed(hVmAccessRights, PAR_VM_START_ACCESS, 0)));
}

void PrlVmManipulationsTest::testGetVmAccessRightsFromVmInfo()
{
	SKIP_TEST_IN_CT_MODE_unimp

	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));//Get actual VM config from server
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_GetState(m_VmHandle));//Retrieve actual VM state
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmInfo, PHT_VM_INFO)

	SdkHandleWrap hVmAcl1;
	CHECK_RET_CODE_EXP(PrlVmInfo_GetAccessRights(hVmInfo, hVmAcl1.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmAcl1, PHT_ACCESS_RIGHTS)

	SdkHandleWrap hVmAcl2;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAccessRights(m_VmHandle, hVmAcl2.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmAcl2, PHT_ACCESS_RIGHTS)

	PRL_ALLOWED_VM_COMMAND _access_rights[] = {	PAR_VM_START_ACCESS, PAR_VM_STOP_ACCESS
		, PAR_VM_PAUSE_ACCESS, PAR_VM_RESET_ACCESS, PAR_VM_SUSPEND_ACCESS,
		PAR_VM_RESUME_ACCESS, PAR_VM_DROPSUSPENDEDSTATE_ACCESS, PAR_VM_CLONE_ACCESS, PAR_VM_DELETE_ACCESS,
		PAR_VM_GETPROBLEMREPORT_ACCESS, PAR_VM_GETCONFIG_ACCESS, PAR_VM_GETSTATISTICS_ACCESS,
		PAR_VM_SUBSCRIBETOGUESTSTATISTICS_ACCESS, PAR_VM_UNSUBSCRIBEFROMGUESTSTATISTICS_ACCESS,
		PAR_VM_UNREG_ACCESS, PAR_VM_BEGINEDIT_ACCESS, PAR_VM_COMMIT_ACCESS, PAR_VMDEV_CONNECT_ACCESS,
		PAR_VMDEV_DISCONNECT_ACCESS, PAR_VMDEV_CREATEIMAGE_ACCESS, PAR_VM_SEND_ANSWER_ACCESS,
		PAR_VM_GET_VMINFO_ACCESS, PAR_VM_INSTALL_TOOLS_ACCESS, PAR_VM_INITIATE_DEV_STATE_NOTIFICATIONS_ACCESS,
		PAR_VM_UPDATE_SECURITY_ACCESS
											};
	for (size_t i = 0; i < sizeof(_access_rights)/sizeof(PRL_ALLOWED_VM_COMMAND); ++i)
	{
		PRL_ALLOWED_VM_COMMAND nPermission = _access_rights[i];
		PRL_BOOL bIsAllowed1, bIsAllowed2;
		CHECK_RET_CODE_EXP(PrlAcl_IsAllowed(hVmAcl1, nPermission, &bIsAllowed1))
		CHECK_RET_CODE_EXP(PrlAcl_IsAllowed(hVmAcl2, nPermission, &bIsAllowed2))
		QVERIFY(bIsAllowed1 == bIsAllowed2);
	}

	PRL_VM_ACCESS_FOR_OTHERS nAccessForOthers1 = PAO_VM_NOT_SHARED, nAccessForOthers2 = PAO_VM_NOT_SHARED;
	CHECK_RET_CODE_EXP(PrlAcl_GetAccessForOthers(hVmAcl1, &nAccessForOthers1))
	CHECK_RET_CODE_EXP(PrlAcl_GetAccessForOthers(hVmAcl2, &nAccessForOthers2))
	QVERIFY(nAccessForOthers1 == nAccessForOthers2);

	QString sOwnerName1, sOwnerName2;
	PRL_EXTRACT_STRING_VALUE(sOwnerName1, hVmAcl1, PrlAcl_GetOwnerName)
	PRL_EXTRACT_STRING_VALUE(sOwnerName2, hVmAcl2, PrlAcl_GetOwnerName)
	QCOMPARE(sOwnerName1, sOwnerName2);

	PRL_BOOL bIsOwner1 = PRL_FALSE, bIsOwner2 = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlAcl_IsCurrentSessionOwner(hVmAcl1, &bIsOwner1))
	CHECK_RET_CODE_EXP(PrlAcl_IsCurrentSessionOwner(hVmAcl2, &bIsOwner2))
	QVERIFY(bIsOwner1 == bIsOwner2);
}

void PrlVmManipulationsTest::testGetVmAccessRightsFromVmInfoFailedOnNullVmInfoHandle()
{
	SdkHandleWrap hVmAcl;
	QVERIFY(PRL_FAILED(PrlVmInfo_GetAccessRights(PRL_INVALID_HANDLE, hVmAcl.GetHandlePtr())));
}

void PrlVmManipulationsTest::testGetVmAccessRightsFromVmInfoFailedOnNonVmInfoHandle()
{
	SdkHandleWrap hVmAcl;
	QVERIFY(PRL_FAILED(PrlVmInfo_GetAccessRights(m_ServerHandle, hVmAcl.GetHandlePtr())));
}

void PrlVmManipulationsTest::testGetVmAccessRightsFromVmInfoFailedOnNullResultBuffer()
{
	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_GetState(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmInfo, PHT_VM_INFO)

	QVERIFY(PRL_FAILED(PrlVmInfo_GetAccessRights(hVmInfo, 0)));
}

#define LOAD_CONFIG_AND_SET_PERMS_FOR_OTHERS(perms)\
	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml")\
	CVmConfiguration _vm_conf(_config);\
	_vm_conf.getVmSecurity()->setAccessForOthers(perms);\
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()))\
	APPLY_UNIQUE_VM_TEST_NAME\
	SdkHandleWrap hVmAccessRights;\
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAccessRights(m_VmHandle, hVmAccessRights.GetHandlePtr()))\
	CHECK_HANDLE_TYPE(hVmAccessRights, PHT_ACCESS_RIGHTS)

void PrlVmManipulationsTest::testVmGetAccessRightsGetAccessForOthers()
{
	PRL_VM_ACCESS_FOR_OTHERS nExpectedValue = PAO_VM_SHARED_ON_VIEW;
	LOAD_CONFIG_AND_SET_PERMS_FOR_OTHERS(nExpectedValue)
	PRL_VM_ACCESS_FOR_OTHERS nActualValue = PAO_VM_NOT_SHARED;
	CHECK_RET_CODE_EXP(PrlAcl_GetAccessForOthers(hVmAccessRights, &nActualValue))
	QCOMPARE(quint32(nActualValue), quint32(nExpectedValue));
}

void PrlVmManipulationsTest::testAclGetAccessForOthersFailedOnNonAclHandle()
{
	PRL_VM_ACCESS_FOR_OTHERS nValue;
	QVERIFY(PRL_FAILED(PrlAcl_GetAccessForOthers(m_ServerHandle, &nValue)));
}

void PrlVmManipulationsTest::testAclGetAccessForOthersFailedOnNullAclHandle()
{
	PRL_VM_ACCESS_FOR_OTHERS nValue;
	QVERIFY(PRL_FAILED(PrlAcl_GetAccessForOthers(PRL_INVALID_HANDLE, &nValue)));
}

void PrlVmManipulationsTest::testAclGetAccessForOthersFailedOnNullResultBuffer()
{
	LOAD_CONFIG_AND_SET_PERMS_FOR_OTHERS(PAO_VM_SHARED_ON_VIEW)
	QVERIFY(PRL_FAILED(PrlAcl_GetAccessForOthers(hVmAccessRights, 0)));
}

void PrlVmManipulationsTest::testVmGetAccessRightsSetAccessForOthers()
{
	PRL_VM_ACCESS_FOR_OTHERS nExpectedValue = PAO_VM_SHARED_ON_VIEW;
	LOAD_CONFIG_AND_SET_PERMS_FOR_OTHERS(PAO_VM_NOT_SHARED)
	CHECK_RET_CODE_EXP(PrlAcl_SetAccessForOthers(hVmAccessRights, nExpectedValue))
	PRL_VM_ACCESS_FOR_OTHERS nActualValue = PAO_VM_NOT_SHARED;
	CHECK_RET_CODE_EXP(PrlAcl_GetAccessForOthers(hVmAccessRights, &nActualValue))
	QCOMPARE(quint32(nActualValue), quint32(nExpectedValue));
}

void PrlVmManipulationsTest::testAclSetAccessForOthersFailedOnNonAclHandle()
{
	QVERIFY(PRL_FAILED(PrlAcl_SetAccessForOthers(m_ServerHandle, PAO_VM_NOT_SHARED)));
}

void PrlVmManipulationsTest::testAclSetAccessForOthersFailedOnNullAclHandle()
{
	QVERIFY(PRL_FAILED(PrlAcl_SetAccessForOthers(PRL_INVALID_HANDLE, PAO_VM_NOT_SHARED)));
}

#define LOAD_CONFIG_AND_SET_OWNER(owner)\
	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml")\
	CVmConfiguration _vm_conf(_config);\
	_vm_conf.getVmSecurity()->setOwner(owner);\
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()))\
	APPLY_UNIQUE_VM_TEST_NAME\
	SdkHandleWrap hVmAccessRights;\
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAccessRights(m_VmHandle, hVmAccessRights.GetHandlePtr()))\
	CHECK_HANDLE_TYPE(hVmAccessRights, PHT_ACCESS_RIGHTS)

void PrlVmManipulationsTest::testVmGetAccessRightsGetOwnerName()
{
	QString sOwnerName = "some owner name";
	LOAD_CONFIG_AND_SET_OWNER(sOwnerName)
	QByteArray sOwnerNameBuf;
	PRL_UINT32 nOwnerNameBufSize = 0;
	CHECK_RET_CODE_EXP(PrlAcl_GetOwnerName(hVmAccessRights, 0, &nOwnerNameBufSize))
	QVERIFY(nOwnerNameBufSize != 0);
	sOwnerNameBuf.resize(nOwnerNameBufSize);
	CHECK_RET_CODE_EXP(PrlAcl_GetOwnerName(hVmAccessRights, sOwnerNameBuf.data(), &nOwnerNameBufSize))
	QCOMPARE(UTF8_2QSTR(sOwnerNameBuf), sOwnerName);
}

void PrlVmManipulationsTest::testAclGetOwnerNameFailedOnNonAclHandle()
{
	PRL_UINT32 nOwnerNameBufSize = 0;
	QVERIFY(PRL_FAILED(PrlAcl_GetOwnerName(m_ServerHandle, 0, &nOwnerNameBufSize)));
}

void PrlVmManipulationsTest::testAclGetOwnerNameFailedOnNullAclHandle()
{
	PRL_UINT32 nOwnerNameBufSize = 0;
	QVERIFY(PRL_FAILED(PrlAcl_GetOwnerName(PRL_INVALID_HANDLE, 0, &nOwnerNameBufSize)));
}

void PrlVmManipulationsTest::testAclGetOwnerNameFailedOnNullResultBuffer()
{
	LOAD_CONFIG_AND_SET_OWNER("some owner name")
	QVERIFY(PRL_FAILED(PrlAcl_GetOwnerName(hVmAccessRights, 0, 0)));
}

#define LOAD_CONFIG_AND_SET_IS_OWNER_SIGN(owner_sign)\
	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml")\
	CVmConfiguration _vm_conf(_config);\
	_vm_conf.getVmSecurity()->setOwnerPresent(owner_sign);\
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))\
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()))\
	APPLY_UNIQUE_VM_TEST_NAME\
	SdkHandleWrap hVmAccessRights;\
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAccessRights(m_VmHandle, hVmAccessRights.GetHandlePtr()))\
	CHECK_HANDLE_TYPE(hVmAccessRights, PHT_ACCESS_RIGHTS)

void PrlVmManipulationsTest::testVmGetAccessRightsIsCurrentSessionOwner()
{
	LOAD_CONFIG_AND_SET_IS_OWNER_SIGN(true)
	PRL_BOOL bIsOwnerSign = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlAcl_IsCurrentSessionOwner(hVmAccessRights, &bIsOwnerSign))
	QVERIFY(bIsOwnerSign == PRL_TRUE);
}

void PrlVmManipulationsTest::testAclIsCurrentSessionOwnerFailedOnNonAclHandle()
{
	PRL_BOOL bIsOwnerSign = PRL_FALSE;
	QVERIFY(PRL_FAILED(PrlAcl_IsCurrentSessionOwner(m_ServerHandle, &bIsOwnerSign)));
}

void PrlVmManipulationsTest::testAclIsCurrentSessionOwnerFailedOnNullAclHandle()
{
	PRL_BOOL bIsOwnerSign = PRL_FALSE;
	QVERIFY(PRL_FAILED(PrlAcl_IsCurrentSessionOwner(PRL_INVALID_HANDLE, &bIsOwnerSign)));
}

void PrlVmManipulationsTest::testAclIsCurrentSessionOwnerFailedOnNullResultBuffer()
{
	LOAD_CONFIG_AND_SET_IS_OWNER_SIGN(true)
	QVERIFY(PRL_FAILED(PrlAcl_IsCurrentSessionOwner(hVmAccessRights, 0)));
}

void PrlVmManipulationsTest::testGenerateVmDevFilename()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmWithDevices();

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_GenerateVmDevFilename(m_VmHandle, "Winxp", ".hdd", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QString sGeneratedVmDevFilename;
	PRL_EXTRACT_STRING_VALUE(sGeneratedVmDevFilename, hResult, PrlResult_GetParamAsString)

	QVERIFY(sGeneratedVmDevFilename != "Winxp.hdd");
	QVERIFY(sGeneratedVmDevFilename.startsWith("Winxp"));
	QVERIFY(sGeneratedVmDevFilename.endsWith(".hdd"));
}

void PrlVmManipulationsTest::testGenerateVmDevFilenameWithDelimiter()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmWithDevices();

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_GenerateVmDevFilename(m_VmHandle, "Winxp", ".hdd", " "));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QString sGeneratedVmDevFilename;
	PRL_EXTRACT_STRING_VALUE(sGeneratedVmDevFilename, hResult, PrlResult_GetParamAsString)

	QVERIFY(sGeneratedVmDevFilename != "Winxp.hdd");
	QVERIFY(sGeneratedVmDevFilename.contains(" "));
	QVERIFY(sGeneratedVmDevFilename.startsWith("Winxp"));
	QVERIFY(sGeneratedVmDevFilename.endsWith(".hdd"));
}

void PrlVmManipulationsTest::testGenerateVmDevFilenameOnEmptyFilenamePrefix()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_GenerateVmDevFilename(m_VmHandle, "", ".hdd", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QByteArray sGeneratedVmDevFilename;
	PRL_UINT32 nGeneratedVmDevFilenameBufSize = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, 0, &nGeneratedVmDevFilenameBufSize))
	QVERIFY(nGeneratedVmDevFilenameBufSize != 0);
	sGeneratedVmDevFilename.resize(nGeneratedVmDevFilenameBufSize);
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, sGeneratedVmDevFilename.data(), &nGeneratedVmDevFilenameBufSize))

	QVERIFY(UTF8_2QSTR(sGeneratedVmDevFilename).startsWith("tmpfile"));
	QVERIFY(UTF8_2QSTR(sGeneratedVmDevFilename).endsWith(".hdd"));
}

void PrlVmManipulationsTest::testGenerateVmDevFilenameOnNullFilenamePrefix()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_GenerateVmDevFilename(m_VmHandle, 0, ".hdd", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QByteArray sGeneratedVmDevFilename;
	PRL_UINT32 nGeneratedVmDevFilenameBufSize = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, 0, &nGeneratedVmDevFilenameBufSize))
	QVERIFY(nGeneratedVmDevFilenameBufSize != 0);
	sGeneratedVmDevFilename.resize(nGeneratedVmDevFilenameBufSize);
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, sGeneratedVmDevFilename.data(), &nGeneratedVmDevFilenameBufSize))

	QVERIFY(UTF8_2QSTR(sGeneratedVmDevFilename).startsWith("tmpfile"));
	QVERIFY(UTF8_2QSTR(sGeneratedVmDevFilename).endsWith(".hdd"));
}

void PrlVmManipulationsTest::testGenerateVmDevFilenameOnEmptyFilenameSuffix()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_GenerateVmDevFilename(m_VmHandle, "harddisk", "", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QByteArray sGeneratedVmDevFilename;
	PRL_UINT32 nGeneratedVmDevFilenameBufSize = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, 0, &nGeneratedVmDevFilenameBufSize))
	QVERIFY(nGeneratedVmDevFilenameBufSize != 0);
	sGeneratedVmDevFilename.resize(nGeneratedVmDevFilenameBufSize);
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, sGeneratedVmDevFilename.data(), &nGeneratedVmDevFilenameBufSize))

	QVERIFY(UTF8_2QSTR(sGeneratedVmDevFilename).startsWith("harddisk"));
}

void PrlVmManipulationsTest::testGenerateVmDevFilenameOnNullFilenameSuffix()
{
	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_GenerateVmDevFilename(m_VmHandle, "harddisk", 0, 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QByteArray sGeneratedVmDevFilename;
	PRL_UINT32 nGeneratedVmDevFilenameBufSize = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, 0, &nGeneratedVmDevFilenameBufSize))
	QVERIFY(nGeneratedVmDevFilenameBufSize != 0);
	sGeneratedVmDevFilename.resize(nGeneratedVmDevFilenameBufSize);
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, sGeneratedVmDevFilename.data(), &nGeneratedVmDevFilenameBufSize))

	QVERIFY(UTF8_2QSTR(sGeneratedVmDevFilename).startsWith("harddisk"));
}

void PrlVmManipulationsTest::testGenerateVmDevFilenameOnBothEmptyFilenamePrefixAndSuffix()
{
	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_GenerateVmDevFilename(m_VmHandle, "", "", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QByteArray sGeneratedVmDevFilename;
	PRL_UINT32 nGeneratedVmDevFilenameBufSize = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, 0, &nGeneratedVmDevFilenameBufSize))
	QVERIFY(nGeneratedVmDevFilenameBufSize != 0);
	sGeneratedVmDevFilename.resize(nGeneratedVmDevFilenameBufSize);
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, sGeneratedVmDevFilename.data(), &nGeneratedVmDevFilenameBufSize))

	QVERIFY(UTF8_2QSTR(sGeneratedVmDevFilename).startsWith("tmpfile"));
}

void PrlVmManipulationsTest::testGenerateVmDevFilenameOnBothNullFilenamePrefixAndSuffix()
{
	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_GenerateVmDevFilename(m_VmHandle, 0, 0, 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QByteArray sGeneratedVmDevFilename;
	PRL_UINT32 nGeneratedVmDevFilenameBufSize = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, 0, &nGeneratedVmDevFilenameBufSize))
	QVERIFY(nGeneratedVmDevFilenameBufSize != 0);
	sGeneratedVmDevFilename.resize(nGeneratedVmDevFilenameBufSize);
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, sGeneratedVmDevFilename.data(), &nGeneratedVmDevFilenameBufSize))

	QVERIFY(UTF8_2QSTR(sGeneratedVmDevFilename).startsWith("tmpfile"));
}

void PrlVmManipulationsTest::testGenerateVmDevFilenameOnNullVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_GenerateVmDevFilename(PRL_INVALID_HANDLE, "harddisk", ".hdd", 0), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGenerateVmDevFilenameOnNonVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_GenerateVmDevFilename(m_ServerHandle, "harddisk", ".hdd", 0), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGenerateVmDevFilenameOnNewlyCreatedVmHandle()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVm_GenerateVmDevFilename(m_VmHandle, "harddisk", ".hdd", 0), PRL_ERR_NO_DATA)
}

void PrlVmManipulationsTest::testTryToStartVMTemplate()
{
	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml")
	CVmConfiguration _vm_conf(_config);
	_vm_conf.getVmSettings()->getVmCommonOptions()->setTemplate(true);
	_vm_conf.getVmHardwareList()->getCpu()->setEnableVTxSupport(PVE::VTxSupportEnabled);
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()));
	APPLY_UNIQUE_VM_TEST_NAME

	SdkHandleWrap hJob(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_ASYNC_OP_FAILED(PrlVm_Start(m_VmHandle), PRL_ERR_CANT_TO_START_VM_TEMPLATE)
}

void PrlVmManipulationsTest::testTryToStartVMTemplateViaStartEx()
{
	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml")
	CVmConfiguration _vm_conf(_config);
	_vm_conf.getVmSettings()->getVmCommonOptions()->setTemplate(true);
	_vm_conf.getVmHardwareList()->getCpu()->setEnableVTxSupport(PVE::VTxSupportEnabled);
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()));
	APPLY_UNIQUE_VM_TEST_NAME

	SdkHandleWrap hJob(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_ASYNC_OP_FAILED(PrlVm_StartEx(m_VmHandle, PSM_VM_START, 0), PRL_ERR_CANT_TO_START_VM_TEMPLATE)
}

void PrlVmManipulationsTest::testTryToStartVMWithCritcalErrorConfigValidate()
{
	SKIP_TEST_IN_CT_MODE_unimp

	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml")
	CVmConfiguration _vm_conf(_config);
	_vm_conf.getVmHardwareList()->getCpu()->setEnableVTxSupport(PVE::VTxSupportEnabled);
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()));
	APPLY_UNIQUE_VM_TEST_NAME

	SdkHandleWrap hJob(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)

	//This 'Sleep' guarantee that the timestamp value
	//of changed config.pvs (see changes below) will be different
	//from timestamp value of just-now cashed config.pvs
	//(config watcher (in Dispatcher) refresh config files cash
	//according to timestamp values (with one second precision)).
	HostUtils::Sleep( 1000 );

	// path config to invalid values
	{
		char buff[ 2048 ];
		PRL_UINT32 buff_sz = sizeof( buff );
		CHECK_RET_CODE_EXP( PrlVmCfg_GetHomePath( m_VmHandle, buff, &buff_sz ) );

		QFile fConf( UTF8_2QSTR( buff ) );
		CVmConfiguration vmConf;
		QVERIFY( 0 == vmConf.loadFromFile( &fConf ) );
		vmConf.getVmHardwareList()->getCpu()->setNumber( 0 );
		QVERIFY( 0 == vmConf.saveToFile( &fConf ) );
	}

	hJob.reset(PrlVm_RefreshConfig( m_VmHandle ));
	CHECK_JOB_RET_CODE(hJob);

	hJob.reset(PrlVm_Start(m_VmHandle));
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT));
	PRL_RESULT nValidationResult = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nValidationResult))

	QVERIFY(nValidationResult == PRL_ERR_INCONSISTENCY_VM_CONFIG);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));

	PRL_UINT32 nCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nCount));
	QVERIFY(nCount == 1);

	SdkHandleWrap hProblemDescription;
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, hProblemDescription.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hProblemDescription, PHT_EVENT)

	PRL_RESULT nProblemErrCode = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hProblemDescription, &nProblemErrCode))
	QVERIFY(nProblemErrCode == PRL_ERR_VMCONF_CPU_ZERO_COUNT);
}

void PrlVmManipulationsTest::testTryToCreateVMWithCritcalErrorConfigValidate()
{
	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml")
		CVmConfiguration _vm_conf(_config);
	_vm_conf.getVmHardwareList()->getCpu()->setEnableVTxSupport(PVE::VTxSupportEnabled);
	_vm_conf.getVmHardwareList()->getCpu()->setNumber( 0 );
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()));
	APPLY_UNIQUE_VM_TEST_NAME

	SdkHandleWrap hJob(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT));
	PRL_RESULT nValidationResult = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nValidationResult))

	QVERIFY(nValidationResult == PRL_ERR_INCONSISTENCY_VM_CONFIG);
}


namespace {
struct VmSecurityStruct
{
	QWaitCondition m_Condition;
	QMutex m_Mutex;
	SdkHandleWrap m_hVmSecurity;
	SdkHandleWrap m_hServer;
	QString m_sVmUuid;
};

PRL_RESULT VmSecurityCallback(PRL_HANDLE _handle, void *pData)
{
	SdkHandleWrap hEvent(_handle);
	PRL_HANDLE_TYPE _type;
	PRL_RESULT nRetCode = PrlHandle_GetType(hEvent, &_type);
	if (PRL_SUCCEEDED(nRetCode))
	{
		if (_type == PHT_EVENT)
		{
			PRL_EVENT_TYPE _event_type;
			nRetCode = PrlEvent_GetType(hEvent, &_event_type);
			if (PRL_SUCCEEDED(nRetCode))
			{
				if (_event_type == PET_DSP_EVT_VM_SECURITY_CHANGED)
				{
					VmSecurityStruct *pVmSecurityStruct = static_cast<VmSecurityStruct *>(pData);
					QMutexLocker _lock(&pVmSecurityStruct->m_Mutex);
					nRetCode = PrlSrv_UnregEventHandler(pVmSecurityStruct->m_hServer, VmSecurityCallback, pData);
					if (PRL_FAILED(nRetCode))
						WRITE_TRACE(DBG_FATAL, "PrlVm_UnregEventHandler() call failed. Error code: %.8X", nRetCode);
					PRL_UINT32 nParamsCount = 0;
					nRetCode = PrlEvent_GetParamsCount(hEvent, &nParamsCount);
					if (PRL_SUCCEEDED(nRetCode) && nParamsCount > 0)
					{
						PRL_CHAR sIssuerIdBuf[STR_BUF_LENGTH];
						PRL_UINT32 nIssuerIdBufLength = sizeof(sIssuerIdBuf);
						nRetCode = PrlEvent_GetIssuerId(hEvent, sIssuerIdBuf, &nIssuerIdBufLength);
						if (PRL_SUCCEEDED(nRetCode))
							pVmSecurityStruct->m_sVmUuid = UTF8_2QSTR(sIssuerIdBuf);
						else
							WRITE_TRACE(DBG_FATAL, "PrlEvent_GetIssuerId() call failed. Error code: %.8X", nRetCode);

						SdkHandleWrap hEventParameter;
						nRetCode = PrlEvent_GetParam(hEvent, 0, hEventParameter.GetHandlePtr());
						if (PRL_SUCCEEDED(nRetCode))
						{
							nRetCode = PrlEvtPrm_ToHandle(hEventParameter,
															pVmSecurityStruct->m_hVmSecurity.GetHandlePtr());
							if (PRL_SUCCEEDED(nRetCode))
								pVmSecurityStruct->m_Condition.wakeAll();
							else
								WRITE_TRACE(DBG_FATAL, "PrlEvtPrm_ToHandle() call failed. Error code: %.8X", nRetCode);
						}
						else
							WRITE_TRACE(DBG_FATAL, "PrlEvent_GetParam() call failed. Error code: %.8X", nRetCode);
					}
					else
						WRITE_TRACE(DBG_FATAL, "PrlEvent_GetParamsCount() call failed. Error code: %.8X. Params count: %d",\
									nRetCode, nParamsCount);
				}
			}
			else
				WRITE_TRACE(DBG_FATAL, "PrlEvent_GetType() call failed. Error: %.8X", nRetCode);
		}
	}
	else
		WRITE_TRACE(DBG_FATAL, "PrlHandle_GetType() call failed. Error code: %.8X", nRetCode);

	return PRL_ERR_SUCCESS;
}

}

void PrlVmManipulationsTest::testVmUpdateSecurity()
{
	SKIP_TEST_IN_CT_MODE_unimp

	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hAccessRights;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAccessRights(m_VmHandle, hAccessRights.GetHandlePtr()))

	PRL_BOOL bIsOwner = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlAcl_IsCurrentSessionOwner(hAccessRights, &bIsOwner))
	QVERIFY(bIsOwner == PRL_TRUE);

	PRL_BOOL bIsAllowed = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlAcl_IsAllowed(hAccessRights, PAR_VM_UPDATE_SECURITY_ACCESS, &bIsAllowed))
	QVERIFY(bIsAllowed == PRL_TRUE);

	PRL_VM_ACCESS_FOR_OTHERS nExpectedAccessForOthers;
	CHECK_RET_CODE_EXP(PrlAcl_GetAccessForOthers(hAccessRights, &nExpectedAccessForOthers))

	SmartPtr<VmSecurityStruct> pVmSecurity( new VmSecurityStruct );
	pVmSecurity->m_hServer = m_ServerHandle;
	CHECK_RET_CODE_EXP(PrlSrv_RegEventHandler(m_ServerHandle, VmSecurityCallback, pVmSecurity.getImpl()))
	bool bRes = false;
	{
		QMutexLocker _lock(&pVmSecurity->m_Mutex);
		hJob.reset(PrlVm_UpdateSecurity(m_VmHandle, hAccessRights));
		bRes = pVmSecurity->m_Condition.wait(&pVmSecurity->m_Mutex, PRL_JOB_WAIT_TIMEOUT);
	}
	CHECK_JOB_RET_CODE(hJob)
	QVERIFY(bRes);
	QMutexLocker _lock(&pVmSecurity->m_Mutex);
	CHECK_HANDLE_TYPE(pVmSecurity->m_hVmSecurity, PHT_ACCESS_RIGHTS)
	QString sVmUuid;
	PRL_EXTRACT_STRING_VALUE(sVmUuid, m_VmHandle, PrlVmCfg_GetUuid)
	QCOMPARE(pVmSecurity->m_sVmUuid, sVmUuid);

	hAccessRights.reset();
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAccessRights(m_VmHandle, hAccessRights.GetHandlePtr()))
	PRL_VM_ACCESS_FOR_OTHERS nActualAccessForOthers;
	CHECK_RET_CODE_EXP(PrlAcl_GetAccessForOthers(hAccessRights, &nActualAccessForOthers))

	QCOMPARE(quint32(nActualAccessForOthers), quint32(nExpectedAccessForOthers));

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	hAccessRights.reset();
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hAccessRights.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hAccessRights, PHT_ACCESS_RIGHTS)

	QString sVmSecurity;
	PRL_EXTRACT_STRING_VALUE(sVmSecurity, hResult, PrlResult_GetParamAsString)
	CVmSecurity _vm_security;
	CHECK_RET_CODE_EXP(_vm_security.fromString(sVmSecurity))
}

void PrlVmManipulationsTest::testVmUpdateSecurityFailedOnNonVmHandle()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))
	SdkHandleWrap hAccessRights;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAccessRights(m_VmHandle, hAccessRights.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVm_UpdateSecurity(m_ServerHandle, hAccessRights), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmUpdateSecurityFailedOnNullVmHandle()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))
	SdkHandleWrap hAccessRights;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAccessRights(m_VmHandle, hAccessRights.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVm_UpdateSecurity(PRL_INVALID_HANDLE, hAccessRights), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmUpdateSecurityFailedOnNonAccessRightsHandle()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVm_UpdateSecurity(m_VmHandle, m_ServerHandle), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmUpdateSecurityFailedOnNullAccessRightsHandle()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVm_UpdateSecurity(m_VmHandle, PRL_INVALID_HANDLE), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmUpdateSecurityFailedOnAttemptToCallByNonOwnerSession()
{
	SKIP_TEST_IN_CT_MODE_unimp

	if (!TestConfig::isServerMode())
		QSKIP("Skip test due functionality not supported at desktop mode", SkipAll);

	testCreateVmFromConfig();

	SdkHandleWrap hAccessRights;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAccessRights(m_VmHandle, hAccessRights.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlAcl_SetAccessForOthers(hAccessRights, PAO_VM_SHARED_ON_FULL_ACCESS))
	SdkHandleWrap hJob(PrlVm_UpdateSecurity(m_VmHandle, hAccessRights));
	CHECK_JOB_RET_CODE(hJob)

	QString sVmUuid;
	PRL_EXTRACT_STRING_VALUE(sVmUuid, m_VmHandle, PrlVmCfg_GetUuid)

	SdkHandleWrap hServer2;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer2.GetHandlePtr()))
	hJob.reset(PrlSrv_Login(hServer2, TestConfig::getRemoteHostName(), TestConfig::getUserLogin2(),
										TestConfig::getUserPassword(), NULL, 0, 0, PSL_HIGH_SECURITY));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(hServer2, hVm.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetUuid(hVm, sVmUuid.toUtf8().constData()))

	CHECK_RET_CODE_EXP(PrlVmCfg_GetAccessRights(hVm, hAccessRights.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVm_UpdateSecurity(hVm, hAccessRights), PRL_ERR_ACCESS_DENIED_TO_CHANGE_PERMISSIONS)
}

void PrlVmManipulationsTest::testVmUpdateSecurityOnLocalAdministratorAccount()
{
	SKIP_TEST_IN_CT_MODE_unimp

	if (!TestConfig::isServerMode())
		QSKIP("Skip test due functionality not supported at desktop mode", SkipAll);

	testCreateVmFromConfig();

	SdkHandleWrap hAccessRights;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAccessRights(m_VmHandle, hAccessRights.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlAcl_SetAccessForOthers(hAccessRights, PAO_VM_SHARED_ON_FULL_ACCESS))
	SdkHandleWrap hJob(PrlVm_UpdateSecurity(m_VmHandle, hAccessRights));
	CHECK_JOB_RET_CODE(hJob)

	QString sVmUuid;
	PRL_EXTRACT_STRING_VALUE(sVmUuid, m_VmHandle, PrlVmCfg_GetUuid)

	SdkHandleWrap hServer2;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer2.GetHandlePtr()))
	hJob.reset(PrlSrv_LoginLocal(hServer2, NULL, 0, PSL_HIGH_SECURITY));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(hServer2, hVm.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetUuid(hVm, sVmUuid.toUtf8().constData()))

	CHECK_RET_CODE_EXP(PrlVmCfg_GetAccessRights(hVm, hAccessRights.GetHandlePtr()))
	PRL_VM_ACCESS_FOR_OTHERS nExpectedAccess = PAO_VM_SHARED_ON_VIEW;
	CHECK_RET_CODE_EXP(PrlAcl_SetAccessForOthers(hAccessRights, nExpectedAccess))
	hJob.reset(PrlVm_UpdateSecurity(hVm, hAccessRights));
	CHECK_JOB_RET_CODE(hJob)
	hJob.reset(PrlVm_RefreshConfig(hVm));
	CHECK_JOB_RET_CODE(hJob)
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAccessRights(hVm, hAccessRights.GetHandlePtr()))
	PRL_VM_ACCESS_FOR_OTHERS nActualAccess = PAO_VM_NOT_SHARED;
	CHECK_RET_CODE_EXP(PrlAcl_GetAccessForOthers(hAccessRights, &nActualAccess))
	QCOMPARE(quint32(nActualAccess), quint32(nExpectedAccess));
}

void PrlVmManipulationsTest::testConcurentExecutionOfBeginEdit()
{
	//Create two concurrent users sessions
	SimpleServerWrapper _s1, _s2;
	QVERIFY(_s1.IsConnected());
	QVERIFY(_s2.IsConnected());

	//Create test VM
	QVERIFY(_s1.CreateTestVm());
	SdkHandleWrap hVm1 = _s1.GetTestVm();
	SdkHandleWrap hVm2 = _s2.GetTestVm();
	CHECK_HANDLE_TYPE(hVm1, PHT_VIRTUAL_MACHINE)
	CHECK_HANDLE_TYPE(hVm2, PHT_VIRTUAL_MACHINE)

	//Change VM at first session
	SdkHandleWrap hJob(PrlVm_BeginEdit(hVm1));
	CHECK_JOB_RET_CODE(hJob)
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(hVm1, Uuid::createUuid().toString().toUtf8().constData()))
	hJob.reset(PrlVm_Commit(hVm1));
	CHECK_JOB_RET_CODE(hJob)

	//Begin edit transaction at second session
	hJob.reset(PrlVm_BeginEdit(hVm2));
	CHECK_JOB_RET_CODE(hJob)

	//Compare sessions VM names now - they should match to each other
	QString sVmName1, sVmName2;
	PRL_EXTRACT_STRING_VALUE(sVmName1, hVm1, PrlVmCfg_GetName)
	PRL_EXTRACT_STRING_VALUE(sVmName2, hVm2, PrlVmCfg_GetName)
	QCOMPARE(sVmName1, sVmName2);
}

void PrlVmManipulationsTest::testGetQuestionsList()
{
	testCreateVmFromConfig();

	SdkHandleWrap hQuestionsList;
	CHECK_RET_CODE_EXP(PrlVm_GetQuestions(m_VmHandle, hQuestionsList.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hQuestionsList, PHT_HANDLES_LIST)

	PRL_UINT32 nItemsCount = 0;
	CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hQuestionsList, &nItemsCount))
	for (PRL_UINT32 i = 0; i < nItemsCount; ++i)
	{
		SdkHandleWrap hQuestion;
		CHECK_RET_CODE_EXP(PrlHndlList_GetItem(hQuestionsList, i, hQuestion.GetHandlePtr()))
		CHECK_HANDLE_TYPE(hQuestion, PHT_EVENT)
	}
}

void PrlVmManipulationsTest::testGetQuestionsListOnInvalidHandle()
{
	SdkHandleWrap hQuestionsList;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVm_GetQuestions(PRL_INVALID_HANDLE, hQuestionsList.GetHandlePtr()),\
										PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetQuestionsListOnWrongTypeHandle()
{
	SdkHandleWrap hQuestionsList1, hQuestionsList2;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hQuestionsList1.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVm_GetQuestions(hQuestionsList1, hQuestionsList2.GetHandlePtr()),\
										PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetQuestionsListOnNullResultBuffer()
{
	testCreateVmFromConfig();

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVm_GetQuestions(m_VmHandle, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testProcessResultOfRefreshConfigOperation()
{
	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	PRL_UINT32 nResParamsCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nResParamsCount))
	QCOMPARE(quint32(1), quint32(nResParamsCount));

	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, hVm.GetHandlePtr()))

	QVERIFY(m_VmHandle.GetHandle() == hVm.GetHandle());
}

void PrlVmManipulationsTest::testVmMigrateOnWrongVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_Migrate(m_ServerHandle, m_ServerHandle, "", PVMT_WARM_MIGRATION, 0, PRL_TRUE),\
							PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlVm_MigrateWithRename(m_ServerHandle, m_ServerHandle, "", "", PVMT_WARM_MIGRATION, 0, PRL_TRUE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmMigrateOnInvalidVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_Migrate(PRL_INVALID_HANDLE, m_ServerHandle, "", PVMT_WARM_MIGRATION, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlVm_MigrateWithRename(PRL_INVALID_HANDLE, m_ServerHandle, "", "", PVMT_WARM_MIGRATION, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmMigrateOnWrongTargetServerHandle()
{
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVm_Migrate(hVm, hVm, "", PVMT_WARM_MIGRATION, 0, PRL_TRUE), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlVm_MigrateWithRename(hVm, hVm, "", "", PVMT_WARM_MIGRATION, 0, PRL_TRUE), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmMigrateOnInvalidTargetServerHandle()
{
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVm_Migrate(hVm, PRL_INVALID_HANDLE, "", PVMT_WARM_MIGRATION, 0, PRL_TRUE),\
							PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlVm_MigrateWithRename(hVm, PRL_INVALID_HANDLE, "", "", PVMT_WARM_MIGRATION, 0, PRL_TRUE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmMigrateOnNullTargetVmName()
{
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVm_MigrateWithRename(hVm, m_ServerHandle, 0, "", PVMT_WARM_MIGRATION, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmMigrateOnNullTargetVmHomePath()
{
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVm_Migrate(hVm, m_ServerHandle, 0, PVMT_WARM_MIGRATION, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlVm_MigrateWithRename(hVm, m_ServerHandle, "", 0, PVMT_WARM_MIGRATION, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmMigrateExOnWrongVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_MigrateEx(m_ServerHandle, "some host", 0, "some session id",\
											"", PVMT_WARM_MIGRATION, 0, PRL_TRUE), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlVm_MigrateWithRenameEx(m_ServerHandle, "some host", 0, "some session id",\
											"", "", PVMT_WARM_MIGRATION, 0, PRL_TRUE), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmMigrateExOnInvalidVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_MigrateEx(PRL_INVALID_HANDLE, "some host", 0, "some session id",\
											"", PVMT_WARM_MIGRATION, 0, PRL_FALSE), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlVm_MigrateWithRenameEx(PRL_INVALID_HANDLE, "some host", 0, "some session id",\
											"", "", PVMT_WARM_MIGRATION, 0, PRL_FALSE), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmMigrateExOnNullTargetServerHost()
{
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVm_MigrateEx(hVm, 0, 0, "some session id",\
											"", PVMT_WARM_MIGRATION, 0, PRL_TRUE), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlVm_MigrateWithRenameEx(hVm, 0, 0, "some session id",\
											"", "", PVMT_WARM_MIGRATION, 0, PRL_TRUE), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmMigrateExOnNullTargetServerSessionId()
{
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVm_MigrateEx(hVm, "some host", 0, 0, "", PVMT_WARM_MIGRATION, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlVm_MigrateWithRenameEx(hVm, "some host", 0, 0, "", "", PVMT_WARM_MIGRATION, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmMigrateExOnNullTargetVmName()
{
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVm_MigrateWithRenameEx(hVm, "some host", 0, "some session id", 0, "", PVMT_WARM_MIGRATION, 0, PRL_TRUE),\
											PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmMigrateExOnNullTargetVmHomePath()
{
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlVm_MigrateEx(hVm, "some host", 0, "some session id", 0, PVMT_WARM_MIGRATION, 0, PRL_TRUE),\
											PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlVm_MigrateWithRenameEx(hVm, "some host", 0, "some session id", "", 0, PVMT_WARM_MIGRATION, 0, PRL_TRUE),\
											PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSubscribeToPerfStats()
{
	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_Start(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob) ;

    stop_vm_on_exit stop_vm(m_VmHandle) ;

	hJob.reset(PrlVm_GetPerfStats(m_VmHandle, ""));
	CHECK_JOB_RET_CODE(hJob) ;
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hEvent;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hEvent.GetHandlePtr()))
    QVERIFY(hEvent.valid()) ;

	PRL_UINT32 nCountersCount = 0;
	CHECK_RET_CODE_EXP(PrlEvent_GetParamsCount(hEvent, &nCountersCount))
	if (0 == nCountersCount)
		QSKIP("No public performance counters. skipping test.", SkipAll);

	DEFINE_CHECK_CALLBACK(check_callback, m_VmHandle, PET_DSP_EVT_VM_PERFSTATS, PrlVm_UnregEventHandler, PJOC_UNKNOWN) ;

	CHECK_RET_CODE_EXP(PrlVm_RegEventHandler(m_VmHandle, event_callback, &check_callback)) ;
	{
		QMutexLocker _lock(&check_callback.mutex);
		hJob.reset(PrlVm_SubscribeToPerfStats(m_VmHandle, ""));
		check_callback.condition.wait(&check_callback.mutex, PRL_JOB_WAIT_TIMEOUT) ;
	}

    CHECK_JOB_RET_CODE(hJob) ;
    hJob.reset(PrlVm_UnsubscribeFromPerfStats(m_VmHandle));
    CHECK_JOB_RET_CODE(hJob) ;

	QMutexLocker _lock(&check_callback.mutex) ;

    QVERIFY(check_callback.got_event.valid()) ;
	QVERIFY(check_callback.u_result > 0);

	PRL_CHAR sVmUuidBuf[STR_BUF_LENGTH] ;
	PRL_UINT32 nVmUuidBufLength = sizeof(sVmUuidBuf);
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUuid(m_VmHandle, sVmUuidBuf, &nVmUuidBufLength))
	QCOMPARE(check_callback.s_result, UTF8_2QSTR(sVmUuidBuf));
}

void PrlVmManipulationsTest::testGetPerfStats()
{
	INITIALIZE_VM("./SDKTest_vm_with_devices_config.xml")

	m_JobHandle.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(m_JobHandle)

	SdkHandleWrap hJob(PrlVm_Start(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

    stop_vm_on_exit stop_vm(m_VmHandle) ;

	hJob.reset(PrlVm_GetPerfStats(m_VmHandle, ""));
	CHECK_JOB_RET_CODE(hJob);
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hEvent;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hEvent.GetHandlePtr()))
    QVERIFY(hEvent.valid());

    PRL_EVENT_TYPE event_type;
    CHECK_RET_CODE_EXP( PrlEvent_GetType(hEvent, &event_type) );
    QCOMPARE(event_type, PET_DSP_EVT_VM_PERFSTATS);

	PRL_UINT32 nCountersCount = 0;
	CHECK_RET_CODE_EXP(PrlEvent_GetParamsCount(hEvent, &nCountersCount))
	if (0 == nCountersCount)
		QSKIP("No public performance counters. skipping test.", SkipAll);

	QRegExp _reg_exp(QString(PRL_NET_CLASSFUL_TRAFFIC_PTRN));
	bool bCounterFound = false;
	for (PRL_UINT32 i = 0; i < nCountersCount; ++i)
	{
		SdkHandleWrap hCounter;
		CHECK_RET_CODE_EXP(PrlEvent_GetParam(hEvent, i, hCounter.GetHandlePtr()))
		QString sCounterName;
		PRL_EXTRACT_STRING_VALUE(sCounterName, hCounter, PrlEvtPrm_GetName)
		if (_reg_exp.exactMatch(sCounterName))
		{
			bCounterFound = true;
			break;
		}
	}
	QVERIFY(bCounterFound);
}

void PrlVmManipulationsTest::testGetPerfStatsForBinaryParam()
{
	INITIALIZE_VM("./SDKTest_vm_with_devices_config.xml")

	m_JobHandle.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(m_JobHandle)

	SdkHandleWrap hJob(PrlVm_Start(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

    stop_vm_on_exit stop_vm(m_VmHandle) ;

	hJob.reset(PrlVm_GetPerfStats(m_VmHandle, PRL_NET_CLASSFUL_TRAFFIC_PTRN));
	CHECK_JOB_RET_CODE(hJob);
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hEvent;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hEvent.GetHandlePtr()))
    QVERIFY(hEvent.valid());

    PRL_EVENT_TYPE event_type;
    CHECK_RET_CODE_EXP( PrlEvent_GetType(hEvent, &event_type) );
    QCOMPARE(quint32(event_type), quint32(PET_DSP_EVT_VM_PERFSTATS));

	PRL_UINT32 nCountersCount = 0;
	CHECK_RET_CODE_EXP(PrlEvent_GetParamsCount(hEvent, &nCountersCount))
	QCOMPARE(quint32(1), quint32(nCountersCount));

	SdkHandleWrap hTrafficInfo;
	CHECK_RET_CODE_EXP(PrlEvent_GetParamByName(hEvent, PRL_NET_CLASSFUL_TRAFFIC_PTRN, hTrafficInfo.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hTrafficInfo, PHT_EVENT_PARAMETER)

	PRL_PARAM_FIELD_DATA_TYPE nFieldType = PFD_UNKNOWN;
	CHECK_RET_CODE_EXP(PrlEvtPrm_GetType(hTrafficInfo, &nFieldType))
	QCOMPARE(quint32(PFD_BINARY), quint32(nFieldType));

	QByteArray _buffer;
	PRL_UINT32 nBufferSize = 0;
	CHECK_RET_CODE_EXP(PrlEvtPrm_GetBuffer(hTrafficInfo, 0, &nBufferSize))

	QCOMPARE(quint32(sizeof(PRL_STAT_NET_TRAFFIC)), nBufferSize);

	_buffer.resize(nBufferSize);
	CHECK_RET_CODE_EXP(PrlEvtPrm_GetBuffer(hTrafficInfo, (PRL_VOID_PTR)_buffer.data(), &nBufferSize))

	PRL_STAT_NET_TRAFFIC_PTR pNetTraffic = (PRL_STAT_NET_TRAFFIC_PTR)_buffer.data();
	for (quint32 i = 0; i < PRL_TC_CLASS_MAX; ++i)
	{
		QVERIFY(pNetTraffic->incoming[i] == 0 || pNetTraffic->incoming[i] > 0);
		QVERIFY(pNetTraffic->outgoing[i] == 0 || pNetTraffic->outgoing[i] > 0);
		QVERIFY(pNetTraffic->incoming_pkt[i] == 0 || pNetTraffic->incoming_pkt[i] > 0);
		QVERIFY(pNetTraffic->outgoing_pkt[i] == 0 || pNetTraffic->outgoing_pkt[i] > 0);
	}

	//Make additional checks of binary result behaviour
	PRL_UINT32 nParamsCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))
	QCOMPARE(quint32(1), quint32(nParamsCount));

	nBufferSize = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlResult_GetParamByIndexAsString(hResult, 0, 0, &nBufferSize), PRL_ERR_INVALID_ARG)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlResult_GetParamByIndex(hResult, 1, hEvent.GetHandlePtr()), PRL_ERR_INVALID_ARG)

	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, hEvent.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hEvent, PHT_EVENT)
}

void PrlVmManipulationsTest::testSubscribeToPerfStatsForBinaryParam()
{
	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_Start(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob) ;

    stop_vm_on_exit stop_vm(m_VmHandle) ;

	DEFINE_CHECK_CALLBACK(check_callback, m_VmHandle, PET_DSP_EVT_VM_PERFSTATS, PrlVm_UnregEventHandler, PJOC_UNKNOWN) ;

	CHECK_RET_CODE_EXP(PrlVm_RegEventHandler(m_VmHandle, event_callback, &check_callback)) ;

	bool bRes = false;
	{
		QMutexLocker _lock(&check_callback.mutex);
		hJob.reset(PrlVm_SubscribeToPerfStats(m_VmHandle, PRL_NET_CLASSFUL_TRAFFIC_PTRN));
		bRes = check_callback.condition.wait(&check_callback.mutex, PRL_JOB_WAIT_TIMEOUT) ;
	}

	CHECK_RET_CODE_EXP(PrlVm_UnregEventHandler(m_VmHandle, event_callback, &check_callback)) ;
    CHECK_JOB_RET_CODE(hJob) ;
    hJob.reset(PrlVm_UnsubscribeFromPerfStats(m_VmHandle));
    CHECK_JOB_RET_CODE(hJob) ;
	QVERIFY(bRes);

	SdkHandleWrap hEvent;
	{
		QMutexLocker _lock(&check_callback.mutex) ;

		QVERIFY(check_callback.got_event.valid()) ;
		QVERIFY(check_callback.u_result > 0);
		hEvent = check_callback.got_event;
	}

	PRL_UINT32 nCountersCount = 0;
	CHECK_RET_CODE_EXP(PrlEvent_GetParamsCount(hEvent, &nCountersCount))
	QCOMPARE(quint32(1), quint32(nCountersCount));

	SdkHandleWrap hTrafficInfo;
	CHECK_RET_CODE_EXP(PrlEvent_GetParamByName(hEvent, PRL_NET_CLASSFUL_TRAFFIC_PTRN, hTrafficInfo.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hTrafficInfo, PHT_EVENT_PARAMETER)

	PRL_PARAM_FIELD_DATA_TYPE nFieldType = PFD_UNKNOWN;
	CHECK_RET_CODE_EXP(PrlEvtPrm_GetType(hTrafficInfo, &nFieldType))
	QCOMPARE(quint32(PFD_BINARY), quint32(nFieldType));

	QByteArray _buffer;
	PRL_UINT32 nBufferSize = 0;
	CHECK_RET_CODE_EXP(PrlEvtPrm_GetBuffer(hTrafficInfo, 0, &nBufferSize))

	QCOMPARE(quint32(sizeof(PRL_STAT_NET_TRAFFIC)), nBufferSize);

	_buffer.resize(nBufferSize);
	CHECK_RET_CODE_EXP(PrlEvtPrm_GetBuffer(hTrafficInfo, (PRL_VOID_PTR)_buffer.data(), &nBufferSize))

	PRL_STAT_NET_TRAFFIC_PTR pNetTraffic = (PRL_STAT_NET_TRAFFIC_PTR)_buffer.data();
	for (quint32 i = 0; i < PRL_TC_CLASS_MAX; ++i)
	{
		QVERIFY(pNetTraffic->incoming[i] == 0 || pNetTraffic->incoming[i] > 0);
		QVERIFY(pNetTraffic->outgoing[i] == 0 || pNetTraffic->outgoing[i] > 0);
		QVERIFY(pNetTraffic->incoming_pkt[i] == 0 || pNetTraffic->incoming_pkt[i] > 0);
		QVERIFY(pNetTraffic->outgoing_pkt[i] == 0 || pNetTraffic->outgoing_pkt[i] > 0);
	}
}

void PrlVmManipulationsTest::testVmGetSuspendedScreenOnWrongVmState()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	CHECK_ASYNC_OP_FAILED(PrlVm_GetSuspendedScreen(m_VmHandle), PRL_ERR_VM_IS_NOT_SUSPENDED);
}

void PrlVmManipulationsTest::testVmGetSuspendedScreenOnInvalidHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_GetSuspendedScreen(m_ServerHandle), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testVmGetSuspendedScreenOnSuccess()
{
	SKIP_TEST_IN_CT_MODE
// Prepare suspend state

	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_Start(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	stop_vm_on_exit stop_vm(m_VmHandle);

	hJob.reset(PrlVm_Suspend(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	HostUtils::Sleep(3000);

// Get suspend screen

	hJob.reset(PrlVm_GetSuspendedScreen(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));

	PRL_UINT32 nCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nCount));
	QVERIFY(nCount == 1);

	SdkHandleWrap hVmEvent;
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, hVmEvent.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hVmEvent, PHT_EVENT);

	CHECK_RET_CODE_EXP(PrlEvent_GetParamsCount(hVmEvent, &nCount));
	QVERIFY(nCount == 1);

	SdkHandleWrap hParam;
	CHECK_RET_CODE_EXP(PrlEvent_GetParam(hVmEvent, 0, hParam.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hParam, PHT_EVENT_PARAMETER);

	QString qsVmScreen;
	PRL_EXTRACT_STRING_VALUE(qsVmScreen, hParam, PrlEvtPrm_ToString);

// Check image

	QImage imgVmScreen;
	imgVmScreen.loadFromData(QByteArray::fromBase64(qsVmScreen.toAscii()));
	QVERIFY(!imgVmScreen.isNull());
}

void PrlVmManipulationsTest::testRunCompressorOnWrongVmState()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	CHECK_ASYNC_OP_FAILED(PrlVm_RunCompressor(m_VmHandle), PRL_ERR_DISP_VM_IS_NOT_STARTED);
}

void PrlVmManipulationsTest::testRunCompressorOnInvalidHandle()
{
	SKIP_TEST_IN_CT_MODE

	CHECK_ASYNC_OP_FAILED(PrlVm_RunCompressor(m_ServerHandle), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testCancelCompressorOnWrongVmState()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	CHECK_ASYNC_OP_FAILED(PrlVm_CancelCompressor(m_VmHandle), PRL_ERR_DISP_VM_IS_NOT_STARTED);
}

void PrlVmManipulationsTest::testCancelCompressorOnInvalidHandle()
{
	SKIP_TEST_IN_CT_MODE

	CHECK_ASYNC_OP_FAILED(PrlVm_CancelCompressor(m_ServerHandle), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testCheckSourceVmUuidOnNewVmCreation()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();
	m_JobHandle.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)

	CVmConfiguration _vm_conf;
	VM_CONFIG_TO_XML_OBJECT

	QCOMPARE(_vm_conf.getVmIdentification()->getVmUuid(), _vm_conf.getVmIdentification()->getSourceVmUuid());
}

void PrlVmManipulationsTest::testSpecifyAbsolutePathAtHardDiskSettings()
{
	SKIP_TEST_IN_CT_MODE

	INITIALIZE_VM("./SDKTest_vm_with_devices_config.xml")

	m_JobHandle.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(m_JobHandle)

	m_JobHandle.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)

	QString sVmHomePath;
	PRL_EXTRACT_STRING_VALUE(sVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath)
	QVERIFY(QFile::exists(sVmHomePath));

	CVmConfiguration _test_config;
	{
		READ_VM_CONFIG_INTO_BUF(sVmHomePath)
		_test_config.fromString(_config);
		QVERIFY(PRL_SUCCEEDED(_test_config.m_uiRcInit));
	}
	QVERIFY(_test_config.getVmHardwareList()->m_lstHardDisks.size());
	QString sUserFriendlyName = _test_config.getVmHardwareList()->m_lstHardDisks.at(0)->getUserFriendlyName();
	QString sSystemName = _test_config.getVmHardwareList()->m_lstHardDisks.at(0)->getSystemName();
	QVERIFY(!QFileInfo(sUserFriendlyName).isAbsolute());
	QVERIFY(!QFileInfo(sSystemName).isAbsolute());

	m_JobHandle.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)

	SdkHandleWrap hVmHardDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHardDisk(m_VmHandle, 0, hVmHardDisk.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmHardDisk, PHT_VIRTUAL_DEV_HARD_DISK)
	PRL_BOOL bConnected = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlVmDev_IsConnected(hVmHardDisk, &bConnected))
	QVERIFY(bConnected == PRL_TRUE);

	QString sAbsolutePathValue = QString("%1/%2/%3")
									.arg(QFileInfo(sVmHomePath).absolutePath())
									.arg(sUserFriendlyName)
									.arg(sUserFriendlyName);
	CHECK_RET_CODE_EXP(PrlVmDev_SetFriendlyName(hVmHardDisk, sAbsolutePathValue.toUtf8().constData()))
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hVmHardDisk, sAbsolutePathValue.toUtf8().constData()))

	m_JobHandle.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)

	{
		READ_VM_CONFIG_INTO_BUF(sVmHomePath)
		_test_config.fromString(_config);
		QVERIFY(PRL_SUCCEEDED(_test_config.m_uiRcInit));
	}

	sUserFriendlyName = _test_config.getVmHardwareList()->m_lstHardDisks.at(0)->getUserFriendlyName();
	sSystemName = _test_config.getVmHardwareList()->m_lstHardDisks.at(0)->getSystemName();
	QVERIFY(!QFileInfo(sUserFriendlyName).isAbsolute());
	QVERIFY(!QFileInfo(sSystemName).isAbsolute());

	m_JobHandle.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)

	CHECK_RET_CODE_EXP(PrlVmDev_IsConnected(hVmHardDisk, &bConnected))
	QVERIFY(bConnected == PRL_TRUE);
}

void PrlVmManipulationsTest::testRenameVmCheckInternalDiskImagePathValid()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmWithDevices();

	QString sOldVmHomePath;
	PRL_EXTRACT_STRING_VALUE(sOldVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath)

	SdkHandleWrap hVmHardDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHardDisk(m_VmHandle, 0, hVmHardDisk.GetHandlePtr()))

	QString sFriendlyName, sSystemName;
	PRL_EXTRACT_STRING_VALUE(sFriendlyName, hVmHardDisk, PrlVmDev_GetFriendlyName)
	PRL_EXTRACT_STRING_VALUE(sSystemName, hVmHardDisk, PrlVmDev_GetSysName)

	QVERIFY(sFriendlyName.startsWith(QFileInfo(sOldVmHomePath).absolutePath()));
	QVERIFY(sSystemName.startsWith(QFileInfo(sOldVmHomePath).absolutePath()));

	m_JobHandle.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)

	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, Uuid::createUuid().toString().toUtf8().constData()))

	m_JobHandle.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)

	m_JobHandle.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)

	QString sNewVmHomePath;
	PRL_EXTRACT_STRING_VALUE(sNewVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath)
	QVERIFY(sNewVmHomePath != sOldVmHomePath);

	PRL_EXTRACT_STRING_VALUE(sFriendlyName, hVmHardDisk, PrlVmDev_GetFriendlyName)
	PRL_EXTRACT_STRING_VALUE(sSystemName, hVmHardDisk, PrlVmDev_GetSysName)

	QVERIFY(sFriendlyName.startsWith(QFileInfo(sNewVmHomePath).absolutePath()));
	QVERIFY(sSystemName.startsWith(QFileInfo(sNewVmHomePath).absolutePath()));
}

void PrlVmManipulationsTest::testVmStartExOnWrongParams()
{
	if (TestConfig::isServerModePSBM())
	{
		QSKIP("Safe mode is not available in PSBM !", SkipAll);
		return;
	}

	testCreateVmFromConfig();

	// Wrong handle
	PRL_VM_START_MODE nVmStartMode = PSM_VM_SAFE_START;
	CHECK_ASYNC_OP_FAILED(PrlVm_StartEx(m_ServerHandle, nVmStartMode, 0), PRL_ERR_INVALID_ARG);

	// Invalid VM start mode
	CHECK_ASYNC_OP_FAILED(PrlVm_StartEx(m_VmHandle, 1 + (PRL_UINT32 )PSM_VM_START_LAST_ITEM , 0), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testUnableCreateSnapshotWithUndoDisks()
{
	if (TestConfig::isServerModePSBM())
	{
		QSKIP("Undo disks is not available in PSBM !", SkipAll);
		return;
	}

	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	m_JobHandle.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)

	CHECK_RET_CODE_EXP(PrlVmCfg_SetUndoDisksMode(m_VmHandle, PUD_REVERSE_CHANGES));

	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 0));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));

	m_JobHandle.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)

	CHECK_ASYNC_OP_FAILED(PrlVm_CreateSnapshot(m_VmHandle, "snapshot", ""),
												PRL_ERR_VM_SNAPSHOT_IN_UNDO_DISKS_MODE);
}

void PrlVmManipulationsTest::testUnableSwitchToSnapshotWithUndoDisks()
{
	if (TestConfig::isServerModePSBM())
	{
		QSKIP("Undo disks is not available in PSBM !", SkipAll);
		return;
	}

	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	m_JobHandle.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)

	CHECK_RET_CODE_EXP(PrlVmCfg_SetUndoDisksMode(m_VmHandle, PUD_REVERSE_CHANGES));

	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 0));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));

	m_JobHandle.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)

	CHECK_ASYNC_OP_FAILED(PrlVm_SwitchToSnapshot(m_VmHandle, QSTR2UTF8(Uuid::createUuid().toString())),
												PRL_ERR_VM_SNAPSHOT_IN_UNDO_DISKS_MODE);
}

void PrlVmManipulationsTest::testUnableDeleteSnapshotWithUndoDisks()
{
	if (TestConfig::isServerModePSBM())
	{
		QSKIP("Undo disks is not available in PSBM !", SkipAll);
		return;
	}

	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	m_JobHandle.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)

	CHECK_RET_CODE_EXP(PrlVmCfg_SetUndoDisksMode(m_VmHandle, PUD_REVERSE_CHANGES));

	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, Uuid::createUuid().toString().toUtf8().constData()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 0));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));

	m_JobHandle.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)

	CHECK_ASYNC_OP_FAILED(PrlVm_DeleteSnapshot(m_VmHandle, QSTR2UTF8(Uuid::createUuid().toString()), PRL_FALSE),
												PRL_ERR_VM_SNAPSHOT_IN_UNDO_DISKS_MODE);
}

#define SET_DEFAULT_VM_CONFIG(out_hVm, os_version, create_devices) \
	{	\
		SdkHandleWrap hVm;\
		CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))\
		SdkHandleWrap hJob(PrlSrv_GetSrvConfig(m_ServerHandle));\
		CHECK_JOB_RET_CODE(hJob)\
		SdkHandleWrap hResult;\
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))\
		SdkHandleWrap hSrvConfig;\
		CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hSrvConfig.GetHandlePtr()))\
		CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(hVm, hSrvConfig, os_version, create_devices))\
		CHECK_RET_CODE_EXP(PrlVmCfg_SetName(hVm, QTest::currentTestFunction()))\
		out_hVm = hVm; \
	}

void PrlVmManipulationsTest::testDenyToCreateVmWithAlreadyExistsVmName()
{
	SKIP_TEST_IN_CT_MODE_unimp

	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	QString sVmName;
	PRL_EXTRACT_STRING_VALUE(sVmName, m_VmHandle, PrlVmCfg_GetName)

	SET_DEFAULT_VM_CONFIG( m_ClonedVmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE );
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName( m_ClonedVmHandle, QSTR2UTF8(sVmName) ));
	CHECK_ASYNC_OP_FAILED( PrlVm_Reg( m_ClonedVmHandle, "", PRL_TRUE) , PRL_ERR_VM_ALREADY_REGISTERED_VM_NAME );
}

void PrlVmManipulationsTest::testCreateVmWithAlreadyExistsVmNameCheckHardImagePath()
{
	QSKIP( "Obsolete because we can't create vm with same name (bug #267227)", SkipAll );

	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	QString sVmName;
	PRL_EXTRACT_STRING_VALUE(sVmName, m_VmHandle, PrlVmCfg_GetName)

	QFile _file("./SDKTest_vm_with_devices_config.xml");
	QVERIFY(_file.open(QIODevice::ReadOnly));
	QTextStream _stream(&_file);
	QString _config = _stream.readAll();
	CVmConfiguration _vm_config(_config);
	_vm_config.getVmIdentification()->setVmName(sVmName);

	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_ClonedVmHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_ClonedVmHandle, _vm_config.toString().toUtf8().constData()));

	hJob.reset(PrlVm_Reg(m_ClonedVmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_RefreshConfig(m_ClonedVmHandle));
	CHECK_JOB_RET_CODE(hJob)

	QString sVmHomePath;
	PRL_EXTRACT_STRING_VALUE(sVmHomePath, m_ClonedVmHandle, PrlVmCfg_GetHomePath)

	SdkHandleWrap hVmHardDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHardDisk(m_ClonedVmHandle, 0, hVmHardDisk.GetHandlePtr()))

	QString sFriendlyName, sSystemName;
	PRL_EXTRACT_STRING_VALUE(sFriendlyName, hVmHardDisk, PrlVmDev_GetFriendlyName)
	PRL_EXTRACT_STRING_VALUE(sSystemName, hVmHardDisk, PrlVmDev_GetSysName)

	QVERIFY(sFriendlyName.startsWith(QFileInfo(sVmHomePath).absolutePath()));
	QVERIFY(sSystemName.startsWith(QFileInfo(sVmHomePath).absolutePath()));

	QVERIFY(QFile::exists(sFriendlyName));
	QVERIFY(QFile::exists(sSystemName));
}

void PrlVmManipulationsTest::testVmRestartOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlVm_Restart(m_ServerHandle), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testIsSmartGuardEnabled()
{
	testCreateVmFromConfig();
	PRL_BOOL bSmartGuardEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsSmartGuardEnabled(m_VmHandle, &bSmartGuardEnabled))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bSmartGuardEnabled == (PRL_BOOL )_vm_conf.getVmSettings()->getVmAutoprotect()->isEnabled());
}

void PrlVmManipulationsTest::testSetSmartGuardEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bSmartGuardEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsSmartGuardEnabled(hVm, &bSmartGuardEnabled))
	bSmartGuardEnabled = !bSmartGuardEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetSmartGuardEnabled(hVm, bSmartGuardEnabled))
	PRL_BOOL bActualSmartGuardEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsSmartGuardEnabled(hVm, &bActualSmartGuardEnabled))
	QCOMPARE(bActualSmartGuardEnabled, bSmartGuardEnabled);
}

void PrlVmManipulationsTest::testSmartGuardEnabledOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bSmartGuardEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsSmartGuardEnabled(m_ServerHandle, &bSmartGuardEnabled),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetSmartGuardEnabled(m_ServerHandle, PRL_FALSE),
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsSmartGuardEnabled(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testIsSmartGuardNotifyBeforeCreation()
{
	testCreateVmFromConfig();
	PRL_BOOL bSmartGuardNotifyBeforeCreation;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsSmartGuardNotifyBeforeCreation(m_VmHandle, &bSmartGuardNotifyBeforeCreation))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bSmartGuardNotifyBeforeCreation == (PRL_BOOL )_vm_conf.getVmSettings()->getVmAutoprotect()->isNotifyBeforeCreation());
}

void PrlVmManipulationsTest::testSetSmartGuardNotifyBeforeCreation()
{
	CREATE_TEST_VM
	PRL_BOOL bSmartGuardNotifyBeforeCreation;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsSmartGuardNotifyBeforeCreation(hVm, &bSmartGuardNotifyBeforeCreation))
	bSmartGuardNotifyBeforeCreation = !bSmartGuardNotifyBeforeCreation;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetSmartGuardNotifyBeforeCreation(hVm, bSmartGuardNotifyBeforeCreation))
	PRL_BOOL bActualSmartGuardNotifyBeforeCreation;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsSmartGuardNotifyBeforeCreation(hVm, &bActualSmartGuardNotifyBeforeCreation))
	QCOMPARE(bActualSmartGuardNotifyBeforeCreation, bSmartGuardNotifyBeforeCreation);
}

void PrlVmManipulationsTest::testSmartGuardNotifyBeforeCreationOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bSmartGuardNotifyBeforeCreation;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsSmartGuardNotifyBeforeCreation(m_ServerHandle, &bSmartGuardNotifyBeforeCreation),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetSmartGuardNotifyBeforeCreation(m_ServerHandle, PRL_FALSE),
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsSmartGuardNotifyBeforeCreation(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testGetSmartGuardInterval()
{
	testCreateVmFromConfig();
	PRL_UINT32 nSmartGuardInterval;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetSmartGuardInterval(m_VmHandle, &nSmartGuardInterval))
	INIT_VM_CONF_FROM_FILE
	QCOMPARE(quint32(nSmartGuardInterval), quint32(_vm_conf.getVmSettings()->getVmAutoprotect()->getPeriod()));
}

void PrlVmManipulationsTest::testSetSmartGuardInterval()
{
	CREATE_TEST_VM
	PRL_UINT32 nSmartGuardInterval;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetSmartGuardInterval(hVm, &nSmartGuardInterval))
	nSmartGuardInterval = nSmartGuardInterval+1;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetSmartGuardInterval(hVm, nSmartGuardInterval))
	PRL_UINT32 nActualSmartGuardInterval;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetSmartGuardInterval(hVm, &nActualSmartGuardInterval))
	QCOMPARE(quint32(nActualSmartGuardInterval), quint32(nSmartGuardInterval));
}

void PrlVmManipulationsTest::testSmartGuardIntervalOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_UINT32 nSmartGuardInterval;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetSmartGuardInterval(m_ServerHandle, &nSmartGuardInterval),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetSmartGuardInterval(m_ServerHandle, 0),
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetSmartGuardInterval(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testGetSmartGuardMaxSnapshotsCount()
{
	testCreateVmFromConfig();
	PRL_UINT32 nSmartGuardMaxSnapshotsCount;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetSmartGuardMaxSnapshotsCount(m_VmHandle, &nSmartGuardMaxSnapshotsCount))
	INIT_VM_CONF_FROM_FILE
	QCOMPARE(quint32(nSmartGuardMaxSnapshotsCount), quint32(_vm_conf.getVmSettings()->getVmAutoprotect()->getTotalSnapshots()));
}

void PrlVmManipulationsTest::testSetSmartGuardMaxSnapshotsCount()
{
	CREATE_TEST_VM
	PRL_UINT32 nSmartGuardMaxSnapshotsCount;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetSmartGuardMaxSnapshotsCount(hVm, &nSmartGuardMaxSnapshotsCount))
	nSmartGuardMaxSnapshotsCount = nSmartGuardMaxSnapshotsCount+1;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetSmartGuardMaxSnapshotsCount(hVm, nSmartGuardMaxSnapshotsCount))
	PRL_UINT32 nActualSmartGuardMaxSnapshotsCount;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetSmartGuardMaxSnapshotsCount(hVm, &nActualSmartGuardMaxSnapshotsCount))
	QCOMPARE(quint32(nActualSmartGuardMaxSnapshotsCount), quint32(nSmartGuardMaxSnapshotsCount));
}

void PrlVmManipulationsTest::testSmartGuardMaxSnapshotsCountOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_UINT32 nSmartGuardMaxSnapshotsCount;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetSmartGuardMaxSnapshotsCount(m_ServerHandle, &nSmartGuardMaxSnapshotsCount),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetSmartGuardMaxSnapshotsCount(m_ServerHandle, 0),
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetSmartGuardMaxSnapshotsCount(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testConfigBackupFileOnExisting()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	m_JobHandle.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle);

	QString sVmConfig;
	PRL_EXTRACT_STRING_VALUE(sVmConfig, m_VmHandle, PrlVmCfg_GetHomePath);
	QVERIFY(QFile::remove(sVmConfig + VMDIR_DEFAULT_VM_BACKUP_SUFFIX));

	m_JobHandle.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle);

	QVERIFY(QFile::exists(sVmConfig + VMDIR_DEFAULT_VM_BACKUP_SUFFIX));
}

void PrlVmManipulationsTest::testIsSharedProfileEnabled()
{
	testCreateVmFromConfig();
	PRL_BOOL bSharedProfileEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsSharedProfileEnabled(m_VmHandle, &bSharedProfileEnabled))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bSharedProfileEnabled == (PRL_BOOL )_vm_conf.getVmSettings()->getVmTools()
											->getVmSharedProfile()->isEnabled());
}

void PrlVmManipulationsTest::testSetSharedProfileEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bSharedProfileEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsSharedProfileEnabled(hVm, &bSharedProfileEnabled))
	bSharedProfileEnabled = !bSharedProfileEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetSharedProfileEnabled(hVm, bSharedProfileEnabled))
	PRL_BOOL bActualSharedProfileEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsSharedProfileEnabled(hVm, &bActualSharedProfileEnabled))
	QCOMPARE(bActualSharedProfileEnabled, bSharedProfileEnabled);
}

void PrlVmManipulationsTest::testSharedProfileEnabledOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bSharedProfileEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsSharedProfileEnabled(m_ServerHandle, &bSharedProfileEnabled),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetSharedProfileEnabled(m_ServerHandle, PRL_FALSE),
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsSharedProfileEnabled(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testIsUseDesktop()
{
	testCreateVmFromConfig();
	PRL_BOOL bUseDesktop;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUseDesktop(m_VmHandle, &bUseDesktop))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bUseDesktop == (PRL_BOOL )_vm_conf.getVmSettings()->getVmTools()
											->getVmSharedProfile()->isUseDesktop());
}

void PrlVmManipulationsTest::testSetUseDesktop()
{
	CREATE_TEST_VM
	PRL_BOOL bUseDesktop;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUseDesktop(hVm, &bUseDesktop))
	bUseDesktop = !bUseDesktop;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetUseDesktop(hVm, bUseDesktop))
	PRL_BOOL bActualUseDesktop;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUseDesktop(hVm, &bActualUseDesktop))
	QCOMPARE(bActualUseDesktop, bUseDesktop);
}

void PrlVmManipulationsTest::testUseDesktopOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bUseDesktop;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsUseDesktop(m_ServerHandle, &bUseDesktop),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetUseDesktop(m_ServerHandle, PRL_FALSE),
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsUseDesktop(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testIsUseDocuments()
{
	testCreateVmFromConfig();
	PRL_BOOL bUseDocuments;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUseDocuments(m_VmHandle, &bUseDocuments))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bUseDocuments == (PRL_BOOL )_vm_conf.getVmSettings()->getVmTools()
											->getVmSharedProfile()->isUseDocuments());
}

void PrlVmManipulationsTest::testSetUseDocuments()
{
	CREATE_TEST_VM
	PRL_BOOL bUseDocuments;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUseDocuments(hVm, &bUseDocuments))
	bUseDocuments = !bUseDocuments;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetUseDocuments(hVm, bUseDocuments))
	PRL_BOOL bActualUseDocuments;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUseDocuments(hVm, &bActualUseDocuments))
	QCOMPARE(bActualUseDocuments, bUseDocuments);
}

void PrlVmManipulationsTest::testUseDocumentsOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bUseDocuments;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsUseDocuments(m_ServerHandle, &bUseDocuments),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetUseDocuments(m_ServerHandle, PRL_FALSE),
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsUseDocuments(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testIsUsePictures()
{
	testCreateVmFromConfig();
	PRL_BOOL bUsePictures;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUsePictures(m_VmHandle, &bUsePictures))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bUsePictures == (PRL_BOOL )_vm_conf.getVmSettings()->getVmTools()
											->getVmSharedProfile()->isUsePictures());
}

void PrlVmManipulationsTest::testSetUsePictures()
{
	CREATE_TEST_VM
	PRL_BOOL bUsePictures;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUsePictures(hVm, &bUsePictures))
	bUsePictures = !bUsePictures;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetUsePictures(hVm, bUsePictures))
	PRL_BOOL bActualUsePictures;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUsePictures(hVm, &bActualUsePictures))
	QCOMPARE(bActualUsePictures, bUsePictures);
}

void PrlVmManipulationsTest::testUsePicturesOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bUsePictures;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsUsePictures(m_ServerHandle, &bUsePictures),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetUsePictures(m_ServerHandle, PRL_FALSE),
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsUsePictures(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testIsUseMusic()
{
	testCreateVmFromConfig();
	PRL_BOOL bUseMusic;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUseMusic(m_VmHandle, &bUseMusic))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bUseMusic == (PRL_BOOL )_vm_conf.getVmSettings()->getVmTools()
											->getVmSharedProfile()->isUseMusic());
}

void PrlVmManipulationsTest::testSetUseMusic()
{
	CREATE_TEST_VM
	PRL_BOOL bUseMusic;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUseMusic(hVm, &bUseMusic))
	bUseMusic = !bUseMusic;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetUseMusic(hVm, bUseMusic))
	PRL_BOOL bActualUseMusic;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUseMusic(hVm, &bActualUseMusic))
	QCOMPARE(bActualUseMusic, bUseMusic);
}

void PrlVmManipulationsTest::testUseMusicOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bUseMusic;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsUseMusic(m_ServerHandle, &bUseMusic),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetUseMusic(m_ServerHandle, PRL_FALSE),
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsUseMusic(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testIsUseDownloads()
{
	testCreateVmFromConfig();
	PRL_BOOL bUseDownloads;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUseDownloads(m_VmHandle, &bUseDownloads))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bUseDownloads == (PRL_BOOL )_vm_conf.getVmSettings()->getVmTools()
											->getVmSharedProfile()->isUseDownloads());
}

void PrlVmManipulationsTest::testSetUseDownloads()
{
	CREATE_TEST_VM
	PRL_BOOL bUseDownloads;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUseDownloads(hVm, &bUseDownloads))
	bUseDownloads = !bUseDownloads;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetUseDownloads(hVm, bUseDownloads))
	PRL_BOOL bActualUseDownloads;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUseDownloads(hVm, &bActualUseDownloads))
	QCOMPARE(bActualUseDownloads, bUseDownloads);
}

void PrlVmManipulationsTest::testUseDownloadsOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bUseDownloads;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsUseDownloads(m_ServerHandle, &bUseDownloads),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetUseDownloads(m_ServerHandle, PRL_FALSE),
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsUseDownloads(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testIsUseMovies()
{
	testCreateVmFromConfig();
	PRL_BOOL bUseMovies;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUseMovies(m_VmHandle, &bUseMovies))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bUseMovies == (PRL_BOOL )_vm_conf.getVmSettings()->getVmTools()
											->getVmSharedProfile()->isUseMovies());
}

void PrlVmManipulationsTest::testSetUseMovies()
{
	CREATE_TEST_VM
	PRL_BOOL bUseMovies;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUseMovies(hVm, &bUseMovies))
	bUseMovies = !bUseMovies;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetUseMovies(hVm, bUseMovies))
	PRL_BOOL bActualUseMovies;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUseMovies(hVm, &bActualUseMovies))
	QCOMPARE(bActualUseMovies, bUseMovies);
}

void PrlVmManipulationsTest::testUseMoviesOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bUseMovies;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsUseMovies(m_ServerHandle, &bUseMovies),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetUseMovies(m_ServerHandle, PRL_FALSE),
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsUseMovies(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testRestoreVm()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	QString qsVmHomePath;
	PRL_EXTRACT_STRING_VALUE(qsVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath);

// Remove config

	QVERIFY(QFile::remove(qsVmHomePath));

	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()))

// Check config state

	PRL_RESULT nResult = PRL_ERR_SUCCESS;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetConfigValidity(m_VmHandle, &nResult));
	QVERIFY(nResult == PRL_ERR_VM_CONFIG_CAN_BE_RESTORED);

// Restore config

	hJob.reset(PrlVm_Restore(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()))
	CHECK_HANDLE_TYPE(m_VmHandle, PHT_VIRTUAL_MACHINE);

	QVERIFY(QFile::exists(qsVmHomePath));

// Check VM state

	hJob.reset(PrlVm_GetState(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hVmInfo, PHT_VM_INFO);

	PRL_BOOL bValid = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlVmInfo_IsInvalid(hVmInfo, &bValid));
	QVERIFY(bValid == PRL_FALSE);
}

void PrlVmManipulationsTest::testRestoreVmOnWrongParams()
{
	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlVm_Restore(m_ServerHandle), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testGetConfigValidity()
{
	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	PRL_RESULT nResult = PRL_ERR_FAILURE;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetConfigValidity(m_VmHandle, &nResult));
	QVERIFY(PRL_SUCCEEDED(nResult));
}

void PrlVmManipulationsTest::testGetConfigValidityOnWrongParams()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	// Wrong handle
	PRL_RESULT nResult;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetConfigValidity(m_ServerHandle, &nResult),
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetConfigValidity(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

#define SET_UNDO_DISKS_MODE(mode) \
	m_JobHandle.reset(PrlVm_BeginEdit(m_VmHandle));\
	CHECK_JOB_RET_CODE(m_JobHandle);\
	CHECK_RET_CODE_EXP(PrlVmCfg_SetUndoDisksMode(m_VmHandle, mode));\
	if (mode != PUD_DISABLE_UNDO_DISKS)\
	{\
		QString qsVmDir;\
		PRL_EXTRACT_STRING_VALUE(qsVmDir, m_VmHandle, PrlVmCfg_GetHomePath);\
		qsVmDir = QFileInfo(qsVmDir).path();\
		QString qsDir = qsVmDir + "/hdd";\
		SdkHandleWrap hDevice;\
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));\
			CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));\
			CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));\
			CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8(qsDir)));\
			CHECK_RET_CODE_EXP(PrlVmDev_SetFriendlyName(hDevice, QSTR2UTF8(qsDir)));\
			CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));\
			CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 0));\
			CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));\
			CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskSize(hDevice, 1024 * 1024));\
			CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskType(hDevice, PHD_EXPANDING_HARD_DISK));\
		m_JobHandle.reset(PrlVmDev_CreateImage(hDevice, PRL_TRUE, PRL_TRUE));\
		CHECK_JOB_RET_CODE(m_JobHandle);\
	}\
	m_JobHandle.reset(PrlVm_Commit(m_VmHandle));\
	CHECK_JOB_RET_CODE(m_JobHandle);

#define START_AND_STOP_VM \
	m_JobHandle.reset(PrlVm_Start(m_VmHandle));\
	CHECK_JOB_RET_CODE(m_JobHandle);\
	m_JobHandle.reset(PrlVm_Stop(m_VmHandle, PRL_FALSE));\
	CHECK_JOB_RET_CODE(m_JobHandle);

#define WAIT_UNTIL_VM_STOPPED\
	{\
		const quint32 nMaxTimeout = 60000;\
		const quint32 nDelay = 500;\
		bool bVmWasStopped = false;\
		for (quint32 i = 0; i < nMaxTimeout/nDelay; ++i)\
		{\
			HostUtils::Sleep(nDelay);\
			SdkHandleWrap hJob(PrlVm_GetState(m_VmHandle));\
			CHECK_JOB_RET_CODE(hJob)\
			SdkHandleWrap hResult;\
			CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))\
			SdkHandleWrap hVmInfo;\
			CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))\
			CHECK_HANDLE_TYPE(hVmInfo, PHT_VM_INFO)\
			VIRTUAL_MACHINE_STATE nVmState = VMS_UNKNOWN;\
			CHECK_RET_CODE_EXP(PrlVmInfo_GetState(hVmInfo, &nVmState))\
			if (VMS_STOPPED == nVmState)\
			{\
				bVmWasStopped = true;\
				break;\
			}\
		}\
		QVERIFY(bVmWasStopped);\
	}

void PrlVmManipulationsTest::testStartVmAfterUndoDisksApply()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

// Start/stop with apply changes

	SET_UNDO_DISKS_MODE(PUD_COMMIT_CHANGES);

	START_AND_STOP_VM;

// Start/stop without undo disks

	SET_UNDO_DISKS_MODE(PUD_DISABLE_UNDO_DISKS);

	START_AND_STOP_VM;
}

void PrlVmManipulationsTest::testStartVmAfterUndoDisksDiscard()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

// Start/stop with discrad changes

	SET_UNDO_DISKS_MODE(PUD_REVERSE_CHANGES);

	START_AND_STOP_VM;

// Start/stop without undo disks

	SET_UNDO_DISKS_MODE(PUD_DISABLE_UNDO_DISKS);

	START_AND_STOP_VM;
}

void PrlVmManipulationsTest::testStartVncServerOnNotStartedVm()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_GetState(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	SdkHandleWrap hVmAccessRights;
	CHECK_RET_CODE_EXP(PrlVmInfo_GetAccessRights(hVmInfo, hVmAccessRights.GetHandlePtr()))

	PRL_BOOL bIsAllowed = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlAcl_IsAllowed(hVmAccessRights, PAR_VM_START_STOP_VNC_SERVER, &bIsAllowed))
	QVERIFY(bIsAllowed == PRL_TRUE);

	CHECK_ASYNC_OP_FAILED(PrlVm_StartVncServer(m_VmHandle, 0), PRL_ERR_DISP_VM_IS_NOT_STARTED)
}

void PrlVmManipulationsTest::testStartVncServerOnNotEnoughUserRights()
{
	SKIP_TEST_IN_CT_MODE

	if (!TestConfig::isServerMode())
		QSKIP("Skipping due remote login not supporting in non server products", SkipAll);

	SimpleServerWrapper _connection1(TestConfig::getUserLogin());
	SimpleServerWrapper _connection2(TestConfig::getUserLogin2());

	QVERIFY(_connection1.IsConnected());
	QVERIFY(_connection2.IsConnected());

	QVERIFY(_connection1.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_GetState(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	SdkHandleWrap hVmAccessRights;
	CHECK_RET_CODE_EXP(PrlVmInfo_GetAccessRights(hVmInfo, hVmAccessRights.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlAcl_SetAccessForOthers(hVmAccessRights, PAO_VM_SHARED_ON_VIEW_AND_RUN))

	hJob.reset(PrlVm_UpdateSecurity(_connection1.GetTestVm(), hVmAccessRights));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_GetState(_connection2.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmInfo_GetAccessRights(hVmInfo, hVmAccessRights.GetHandlePtr()))

	PRL_BOOL bIsAllowed = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlAcl_IsAllowed(hVmAccessRights, PAR_VM_START_STOP_VNC_SERVER, &bIsAllowed))
	QVERIFY(bIsAllowed == PRL_FALSE);

	hJob.reset(PrlVm_Start(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	{
		stop_vm_on_exit stop_vm(_connection1.GetTestVm());

		CHECK_ASYNC_OP_FAILED(PrlVm_StartVncServer(_connection2.GetTestVm(), 0), PRL_ERR_ACCESS_TO_VM_DENIED)
	}
}

void PrlVmManipulationsTest::testStartVncServerOnNotEnoughUserRights2()
{
	SKIP_TEST_IN_CT_MODE

	if (!TestConfig::isServerMode())
		QSKIP("Skipping due remote login not supporting in non server products", SkipAll);

	SimpleServerWrapper _connection1(TestConfig::getUserLogin());
	SimpleServerWrapper _connection2(TestConfig::getUserLogin2());

	QVERIFY(_connection1.IsConnected());
	QVERIFY(_connection2.IsConnected());

	QVERIFY(_connection1.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_GetState(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	SdkHandleWrap hVmAccessRights;
	CHECK_RET_CODE_EXP(PrlVmInfo_GetAccessRights(hVmInfo, hVmAccessRights.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlAcl_SetAccessForOthers(hVmAccessRights, PAO_VM_SHARED_ON_VIEW))

	hJob.reset(PrlVm_UpdateSecurity(_connection1.GetTestVm(), hVmAccessRights));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_GetState(_connection2.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmInfo_GetAccessRights(hVmInfo, hVmAccessRights.GetHandlePtr()))

	PRL_BOOL bIsAllowed = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlAcl_IsAllowed(hVmAccessRights, PAR_VM_START_STOP_VNC_SERVER, &bIsAllowed))
	QVERIFY(bIsAllowed == PRL_FALSE);

	hJob.reset(PrlVm_Start(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	{
		stop_vm_on_exit stop_vm(_connection1.GetTestVm());

		CHECK_ASYNC_OP_FAILED(PrlVm_StartVncServer(_connection2.GetTestVm(), 0), PRL_ERR_ACCESS_TO_VM_DENIED)
	}
}

void PrlVmManipulationsTest::testStartVncServerOnNullVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_StartVncServer(PRL_INVALID_HANDLE, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testStartVncServerOnNonVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_StartVncServer(m_ServerHandle, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testStopVncServerOnNotStartedVm()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_GetState(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	SdkHandleWrap hVmAccessRights;
	CHECK_RET_CODE_EXP(PrlVmInfo_GetAccessRights(hVmInfo, hVmAccessRights.GetHandlePtr()))

	PRL_BOOL bIsAllowed = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlAcl_IsAllowed(hVmAccessRights, PAR_VM_START_STOP_VNC_SERVER, &bIsAllowed))
	QVERIFY(bIsAllowed == PRL_TRUE);

	CHECK_ASYNC_OP_FAILED(PrlVm_StopVncServer(m_VmHandle, 0), PRL_ERR_DISP_VM_IS_NOT_STARTED)
}

void PrlVmManipulationsTest::testStopVncServerOnNotEnoughUserRights()
{
	SKIP_TEST_IN_CT_MODE

	if (!TestConfig::isServerMode())
		QSKIP("Skipping due remote login not supporting in non server products", SkipAll);

	SimpleServerWrapper _connection1(TestConfig::getUserLogin());
	SimpleServerWrapper _connection2(TestConfig::getUserLogin2());

	QVERIFY(_connection1.IsConnected());
	QVERIFY(_connection2.IsConnected());

	QVERIFY(_connection1.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_GetState(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	SdkHandleWrap hVmAccessRights;
	CHECK_RET_CODE_EXP(PrlVmInfo_GetAccessRights(hVmInfo, hVmAccessRights.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlAcl_SetAccessForOthers(hVmAccessRights, PAO_VM_SHARED_ON_VIEW_AND_RUN))

	hJob.reset(PrlVm_UpdateSecurity(_connection1.GetTestVm(), hVmAccessRights));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_GetState(_connection2.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmInfo_GetAccessRights(hVmInfo, hVmAccessRights.GetHandlePtr()))

	PRL_BOOL bIsAllowed = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlAcl_IsAllowed(hVmAccessRights, PAR_VM_START_STOP_VNC_SERVER, &bIsAllowed))
	QVERIFY(bIsAllowed == PRL_FALSE);

	hJob.reset(PrlVm_Start(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	{
		stop_vm_on_exit stop_vm(_connection1.GetTestVm());

		CHECK_ASYNC_OP_FAILED(PrlVm_StopVncServer(_connection2.GetTestVm(), 0), PRL_ERR_ACCESS_TO_VM_DENIED)
	}
}

void PrlVmManipulationsTest::testStopVncServerOnNotEnoughUserRights2()
{
	SKIP_TEST_IN_CT_MODE

	if (!TestConfig::isServerMode())
		QSKIP("Skipping due remote login not supporting in non server products", SkipAll);

	SimpleServerWrapper _connection1(TestConfig::getUserLogin());
	SimpleServerWrapper _connection2(TestConfig::getUserLogin2());

	QVERIFY(_connection1.IsConnected());
	QVERIFY(_connection2.IsConnected());

	QVERIFY(_connection1.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_GetState(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	SdkHandleWrap hVmAccessRights;
	CHECK_RET_CODE_EXP(PrlVmInfo_GetAccessRights(hVmInfo, hVmAccessRights.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlAcl_SetAccessForOthers(hVmAccessRights, PAO_VM_SHARED_ON_VIEW))

	hJob.reset(PrlVm_UpdateSecurity(_connection1.GetTestVm(), hVmAccessRights));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_GetState(_connection2.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmInfo_GetAccessRights(hVmInfo, hVmAccessRights.GetHandlePtr()))

	PRL_BOOL bIsAllowed = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlAcl_IsAllowed(hVmAccessRights, PAR_VM_START_STOP_VNC_SERVER, &bIsAllowed))
	QVERIFY(bIsAllowed == PRL_FALSE);

	hJob.reset(PrlVm_Start(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	{
		stop_vm_on_exit stop_vm(_connection1.GetTestVm());

		CHECK_ASYNC_OP_FAILED(PrlVm_StopVncServer(_connection2.GetTestVm(), 0), PRL_ERR_ACCESS_TO_VM_DENIED)
	}
}

void PrlVmManipulationsTest::testStopVncServerOnNullVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_StopVncServer(PRL_INVALID_HANDLE, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testStopVncServerOnNonVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_StopVncServer(m_ServerHandle, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testCheckVncServerStatusOnNotStartedVm()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_GetState(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))

	PRL_BOOL bVncServerStarted = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlVmInfo_IsVncServerStarted(hVmInfo, &bVncServerStarted))

	QVERIFY(PRL_FALSE == bVncServerStarted);
}

void PrlVmManipulationsTest::testVmInfoIsVncServerStartedOnNullPointer()
{
	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_GetState(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hVmInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmInfo_IsVncServerStarted(hVmInfo, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmInfoIsVncServerStartedOnNullVmInfoHandle()
{
	PRL_BOOL bVncServerStarted = PRL_FALSE;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmInfo_IsVncServerStarted(PRL_INVALID_HANDLE, &bVncServerStarted), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmInfoIsVncServerStartedOnNonVmInfoHandle()
{
	testCreateVmFromConfig();
	PRL_BOOL bVncServerStarted = PRL_FALSE;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmInfo_IsVncServerStarted(m_VmHandle, &bVncServerStarted), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmGetSearchDomains()
{
	testCreateVmFromConfig();
	INIT_VM_CONF_FROM_FILE
	PRL_CHECK_STRINGS_LIST(m_VmHandle, PrlVmCfg_GetSearchDomains,\
		_vm_conf.getVmSettings()->getGlobalNetwork()->getSearchDomains())
}

void PrlVmManipulationsTest::testVmGetSearchDomainsOnNullPtr()
{
	testCreateVmFromConfig();
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetSearchDomains(m_VmHandle, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmGetSearchDomainsOnNullVmHandle()
{
	SdkHandleWrap hSearchDomainsList;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetSearchDomains(PRL_INVALID_HANDLE, hSearchDomainsList.GetHandlePtr()),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmGetSearchDomainsOnNonVmHandle()
{
	SdkHandleWrap hSearchDomainsList;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetSearchDomains(m_ServerHandle, hSearchDomainsList.GetHandlePtr()),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetSearchDomains()
{
	testCreateVmFromConfig();
	QStringList lstNewSearchDomains = QStringList()<<"192.168.1.1/192.168.1.0:24"<<"10.30.1.1/10.30.1.0:24";
	SdkHandleWrap hNewSearchDomainsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hNewSearchDomainsList.GetHandlePtr()))
	foreach(QString sNetAddress, lstNewSearchDomains)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewSearchDomainsList, QSTR2UTF8(sNetAddress)))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetSearchDomains(m_VmHandle, hNewSearchDomainsList))
	PRL_CHECK_STRINGS_LIST(m_VmHandle, PrlVmCfg_GetSearchDomains, lstNewSearchDomains)
}

void PrlVmManipulationsTest::testSetSearchDomainsOnNullVmHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetSearchDomains(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE),\
			PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetSearchDomainsOnNonVmHandle()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetSearchDomains(m_ServerHandle, PRL_INVALID_HANDLE),\
			PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetSearchDomainsOnNonStringsListHandle()
{
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetSearchDomains(hVm, m_ServerHandle),\
			PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetSearchDomainsSetEmptyListAsNullListHandle()
{
	testCreateVmFromConfig();

	QStringList lstSearchDomains = QStringList()<<"192.168.1.1/192.168.1.0:24"<<"10.30.1.1/10.30.1.0:24";
	SdkHandleWrap hNewSearchDomainsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hNewSearchDomainsList.GetHandlePtr()))
	foreach(QString sNetAddress, lstSearchDomains)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewSearchDomainsList, QSTR2UTF8(sNetAddress)))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetSearchDomains(m_VmHandle, hNewSearchDomainsList))
	PRL_CHECK_STRINGS_LIST(m_VmHandle, PrlVmCfg_GetSearchDomains, lstSearchDomains)

	CHECK_RET_CODE_EXP(PrlVmCfg_SetSearchDomains(m_VmHandle, PRL_INVALID_HANDLE))
	lstSearchDomains = QStringList();
	PRL_CHECK_STRINGS_LIST(m_VmHandle, PrlVmCfg_GetSearchDomains, lstSearchDomains)
}

void PrlVmManipulationsTest::testAuthWithGuestSecurityDb()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	CHECK_ASYNC_OP_FAILED(PrlVm_AuthWithGuestSecurityDb(m_VmHandle, "some user", "some passwd", 0),\
		PRL_ERR_VM_USER_AUTHENTICATION_FAILED)
	CHECK_ASYNC_OP_FAILED(PrlVm_AuthWithGuestSecurityDb(m_VmHandle, "some user", "", 0),\
		PRL_ERR_VM_USER_AUTHENTICATION_FAILED)
	CHECK_ASYNC_OP_FAILED(PrlVm_AuthWithGuestSecurityDb(m_VmHandle, "some user", 0, 0),\
		PRL_ERR_VM_USER_AUTHENTICATION_FAILED)
}

void PrlVmManipulationsTest::testAuthWithGuestSecurityDbOnNullVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_AuthWithGuestSecurityDb(PRL_INVALID_HANDLE, "some user", "some passwd", 0),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testAuthWithGuestSecurityDbOnNonVmHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_AuthWithGuestSecurityDb(m_ServerHandle, "some user", "some passwd", 0),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testAuthWithGuestSecurityDbOnEmptyOrNullUserLogin()
{
	testCreateVmFromConfig();

	CHECK_ASYNC_OP_FAILED(PrlVm_AuthWithGuestSecurityDb(m_VmHandle, 0, "some passwd", 0),\
		PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlVm_AuthWithGuestSecurityDb(m_VmHandle, "", "some passwd", 0),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetOptimizeModifiersMode()
{
	CREATE_TEST_VM
	PRL_OPTIMIZE_MODIFIERS_MODE nOptimizeModifiersMode;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetOptimizeModifiersMode(hVm, &nOptimizeModifiersMode))
	nOptimizeModifiersMode = (PRL_OPTIMIZE_MODIFIERS_MODE )(nOptimizeModifiersMode ^ POD_OPTIMIZE_FOR_GAMES);
	CHECK_RET_CODE_EXP(PrlVmCfg_SetOptimizeModifiersMode(hVm, nOptimizeModifiersMode))
	PRL_OPTIMIZE_MODIFIERS_MODE nActualOptimizeModifiersMode;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetOptimizeModifiersMode(hVm, &nActualOptimizeModifiersMode))
	QCOMPARE(nActualOptimizeModifiersMode, nOptimizeModifiersMode);
}

void PrlVmManipulationsTest::testOptimizeModifiersModeOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_OPTIMIZE_MODIFIERS_MODE nOptimizeModifiersMode;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetOptimizeModifiersMode(m_ServerHandle, &nOptimizeModifiersMode),\
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetOptimizeModifiersMode(m_ServerHandle, POD_STANDARD),\
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetOptimizeModifiersMode(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testIsShareClipboard()
{
	testCreateVmFromConfig();
	PRL_BOOL bShareClipboard;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsShareClipboard(m_VmHandle, &bShareClipboard))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bShareClipboard == (PRL_BOOL )_vm_conf.getVmSettings()->getVmTools()\
											->getClipboardSync()->isEnabled());
}

void PrlVmManipulationsTest::testSetShareClipboard()
{
	CREATE_TEST_VM
	PRL_BOOL bShareClipboard;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsShareClipboard(hVm, &bShareClipboard))
	bShareClipboard = !bShareClipboard;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetShareClipboard(hVm, bShareClipboard))
	PRL_BOOL bActualShareClipboard;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsShareClipboard(hVm, &bActualShareClipboard))
	QCOMPARE(bActualShareClipboard, bShareClipboard);
}

void PrlVmManipulationsTest::testShareClipboardOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bShareClipboard;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsShareClipboard(m_ServerHandle, &bShareClipboard),\
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetShareClipboard(m_ServerHandle, PRL_FALSE),\
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsShareClipboard(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testIsTimeSynchronizationEnabled()
{
	testCreateVmFromConfig();
	PRL_BOOL bTimeSynchronizationEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsTimeSynchronizationEnabled(m_VmHandle, &bTimeSynchronizationEnabled))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bTimeSynchronizationEnabled == (PRL_BOOL )_vm_conf.getVmSettings()->getVmTools()\
											->getTimeSync()->isEnabled());
}

void PrlVmManipulationsTest::testSetTimeSynchronizationEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bTimeSynchronizationEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsTimeSynchronizationEnabled(hVm, &bTimeSynchronizationEnabled))
	bTimeSynchronizationEnabled = !bTimeSynchronizationEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetTimeSynchronizationEnabled(hVm, bTimeSynchronizationEnabled))
	PRL_BOOL bActualTimeSynchronizationEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsTimeSynchronizationEnabled(hVm, &bActualTimeSynchronizationEnabled))
	QCOMPARE(bActualTimeSynchronizationEnabled, bTimeSynchronizationEnabled);
}

void PrlVmManipulationsTest::testTimeSynchronizationEnabledOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bTimeSynchronizationEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsTimeSynchronizationEnabled(m_ServerHandle, &bTimeSynchronizationEnabled),\
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetTimeSynchronizationEnabled(m_ServerHandle, PRL_FALSE),\
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsTimeSynchronizationEnabled(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testIsTimeSyncSmartModeEnabled()
{
	testCreateVmFromConfig();
	PRL_BOOL bTimeSyncSmartModeEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsTimeSyncSmartModeEnabled(m_VmHandle, &bTimeSyncSmartModeEnabled))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bTimeSyncSmartModeEnabled == (PRL_BOOL )_vm_conf.getVmSettings()->getVmTools()\
											->getTimeSync()->isKeepTimeDiff());
}

void PrlVmManipulationsTest::testSetTimeSyncSmartModeEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bTimeSyncSmartModeEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsTimeSyncSmartModeEnabled(hVm, &bTimeSyncSmartModeEnabled))
	bTimeSyncSmartModeEnabled = !bTimeSyncSmartModeEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetTimeSyncSmartModeEnabled(hVm, bTimeSyncSmartModeEnabled))
	PRL_BOOL bActualTimeSyncSmartModeEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsTimeSyncSmartModeEnabled(hVm, &bActualTimeSyncSmartModeEnabled))
	QCOMPARE(bActualTimeSyncSmartModeEnabled, bTimeSyncSmartModeEnabled);
}

void PrlVmManipulationsTest::testTimeSyncSmartModeEnabledOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bTimeSyncSmartModeEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsTimeSyncSmartModeEnabled(m_ServerHandle, &bTimeSyncSmartModeEnabled),\
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetTimeSyncSmartModeEnabled(m_ServerHandle, PRL_FALSE),\
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsTimeSyncSmartModeEnabled(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testGetTimeSyncInterval()
{
	testCreateVmFromConfig();
	PRL_UINT32 nTimeSyncInterval;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetTimeSyncInterval(m_VmHandle, &nTimeSyncInterval))
	INIT_VM_CONF_FROM_FILE
	QCOMPARE(quint32(nTimeSyncInterval), quint32(_vm_conf.getVmSettings()->getVmTools()\
											->getTimeSync()->getSyncInterval()));
}

void PrlVmManipulationsTest::testSetTimeSyncInterval()
{
	CREATE_TEST_VM
	PRL_UINT32 nTimeSyncInterval;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetTimeSyncInterval(hVm, &nTimeSyncInterval))
	nTimeSyncInterval = nTimeSyncInterval+1;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetTimeSyncInterval(hVm, nTimeSyncInterval))
	PRL_UINT32 nActualTimeSyncInterval;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetTimeSyncInterval(hVm, &nActualTimeSyncInterval))
	QCOMPARE(nActualTimeSyncInterval, nTimeSyncInterval);
}

void PrlVmManipulationsTest::testTimeSyncIntervalOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_UINT32 nTimeSyncInterval;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetTimeSyncInterval(m_ServerHandle, &nTimeSyncInterval),\
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetTimeSyncInterval(m_ServerHandle, 0),\
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetTimeSyncInterval(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testEditDescriptionStartupOptionsNotAffected()
{
	SimpleServerWrapper _connection(0);
	QVERIFY(_connection.CreateTestVm());

	PRL_VM_AUTOSTART_OPTION nExpectedVmOnStartAction = PAO_VM_START_ON_LOAD;
	PRL_VM_START_LOGIN_MODE nExpectedVmStartAccount = PLM_ROOT_ACCOUNT;

	SdkHandleWrap hJob(PrlVm_BeginEdit(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

		CHECK_RET_CODE_EXP(PrlVmCfg_SetAutoStart(_connection.GetTestVm(), nExpectedVmOnStartAction))
		CHECK_RET_CODE_EXP(PrlVmCfg_SetStartLoginMode(_connection.GetTestVm(), nExpectedVmStartAccount))

		hJob.reset(PrlVm_Commit(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

		hJob.reset(PrlVm_BeginEdit(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

		CHECK_RET_CODE_EXP(PrlVmCfg_SetDescription(_connection.GetTestVm(), "Some VM description"))

		hJob.reset(PrlVm_Commit(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

		hJob.reset(PrlVm_RefreshConfig(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

		PRL_VM_AUTOSTART_OPTION nActualVmOnStartAction = PAO_VM_START_MANUAL;
	PRL_VM_START_LOGIN_MODE nActualVmStartAccount = PLM_START_ACCOUNT;

	CHECK_RET_CODE_EXP(PrlVmCfg_GetAutoStart(_connection.GetTestVm(), &nActualVmOnStartAction))
		CHECK_RET_CODE_EXP(PrlVmCfg_GetStartLoginMode(_connection.GetTestVm(), &nActualVmStartAccount))

		QCOMPARE(quint32(nActualVmStartAccount), quint32(nExpectedVmStartAccount));
	QCOMPARE(quint32(nActualVmOnStartAction), quint32(nExpectedVmOnStartAction));
}

void PrlVmManipulationsTest::testIsAllowSelectBootDevice()
{
	testCreateVmFromConfig();
	PRL_BOOL bAllowSelectBootDevice = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsAllowSelectBootDevice(m_VmHandle, &bAllowSelectBootDevice))
		INIT_VM_CONF_FROM_FILE
		QVERIFY(bAllowSelectBootDevice == (PRL_BOOL) _vm_conf.getVmSettings()->getVmStartupOptions()->isAllowSelectBootDevice());
}

void PrlVmManipulationsTest::testIsAllowSelectBootDeviceOnWrongParams()
{
	PRL_BOOL bAllowSelectBootDevice = PRL_FALSE;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsAllowSelectBootDevice(PRL_INVALID_HANDLE, &bAllowSelectBootDevice), PRL_ERR_INVALID_ARG)
		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsAllowSelectBootDevice(m_ServerHandle, &bAllowSelectBootDevice), PRL_ERR_INVALID_ARG)
		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsAllowSelectBootDevice(m_VmHandle, NULL), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetAllowSelectBootDevice()
{
	CREATE_TEST_VM
		PRL_BOOL bAllowSelectBootDevice = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsAllowSelectBootDevice(hVm, &bAllowSelectBootDevice))
		bAllowSelectBootDevice = !bAllowSelectBootDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetAllowSelectBootDevice(hVm, bAllowSelectBootDevice))
		PRL_BOOL bActualAllowSelectBootDevice = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsAllowSelectBootDevice(hVm, &bActualAllowSelectBootDevice))
		QCOMPARE(quint32(bActualAllowSelectBootDevice), quint32(bAllowSelectBootDevice));
}

void PrlVmManipulationsTest::testSetAllowSelectBootDeviceOnWrongParams()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetAllowSelectBootDevice(PRL_INVALID_HANDLE, PRL_FALSE), PRL_ERR_INVALID_ARG)
		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetAllowSelectBootDevice(m_ServerHandle, PRL_FALSE), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetAllDevices()
{
	// Create VM without devices

	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(m_VmHandle,
		PRL_INVALID_HANDLE,
		PVS_GUEST_VER_WIN_XP,
		PRL_FALSE));
	PRL_UINT32 nCount;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDevsCount(m_VmHandle, &nCount));
	QVERIFY(nCount == 0);

	SdkHandleWrap hDevsList;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAllDevices(m_VmHandle, hDevsList.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hDevsList, PHT_HANDLES_LIST);
	CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hDevsList, &nCount));
	QVERIFY(nCount == 0);

	// Add devices

	QList<PRL_DEVICE_TYPE >	lstDevTypes;
	lstDevTypes << PDE_FLOPPY_DISK << PDE_OPTICAL_DISK << PDE_HARD_DISK
		<< PDE_SERIAL_PORT << PDE_PARALLEL_PORT << PDE_USB_DEVICE
		<< PDE_GENERIC_NETWORK_ADAPTER << PDE_SOUND_DEVICE << PDE_GENERIC_PCI_DEVICE
		<< PDE_GENERIC_SCSI_DEVICE << PDE_PCI_VIDEO_ADAPTER;

	foreach(PRL_DEVICE_TYPE nDevType, lstDevTypes)
	{
		SdkHandleWrap hDevice;
		CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, nDevType, hDevice.GetHandlePtr()));
	}

	// Get all devices

	CHECK_RET_CODE_EXP(PrlVmCfg_GetDevsCount(m_VmHandle, &nCount));
	QCOMPARE(nCount, (PRL_UINT32 )lstDevTypes.size());

	CHECK_RET_CODE_EXP(PrlVmCfg_GetAllDevices(m_VmHandle, hDevsList.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hDevsList, PHT_HANDLES_LIST);
	CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hDevsList, &nCount));
	QCOMPARE(nCount, (PRL_UINT32 )lstDevTypes.size());

	for(PRL_UINT32 i = 0; i < nCount; i++)
	{
		SdkHandleWrap hDevice;
		CHECK_RET_CODE_EXP(PrlHndlList_GetItem(hDevsList, i, hDevice.GetHandlePtr()));

		PRL_DEVICE_TYPE nDevType;
		CHECK_RET_CODE_EXP(PrlVmDev_GetType(hDevice, &nDevType));
		lstDevTypes.removeAll(nDevType);
	}

	QVERIFY(lstDevTypes.isEmpty());
}

void PrlVmManipulationsTest::testGetAllDevicesOnWrongParams()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	// Wrong handle
	SdkHandleWrap hDevice;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetAllDevices(m_ServerHandle, hDevice.GetHandlePtr()),
		PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetAllDevices(m_VmHandle, NULL),
		PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testIsLockInFullScreenMode()
{
	testCreateVmFromConfig();
	PRL_BOOL bLockInFullScreenMode;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsLockInFullScreenMode(m_VmHandle, &bLockInFullScreenMode))
		INIT_VM_CONF_FROM_FILE
		QVERIFY(bLockInFullScreenMode == PRL_BOOL(_vm_conf.getVmSettings()->getVmStartupOptions()->isLockInFullScreenMode()));
}

void PrlVmManipulationsTest::testIsLockInFullScreenModeOnWrongParams()
{
	testCreateVmFromConfig();
	PRL_BOOL bLockInFullScreenMode;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsLockInFullScreenMode(m_ServerHandle, &bLockInFullScreenMode), PRL_ERR_INVALID_ARG)
		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsLockInFullScreenMode(PRL_INVALID_HANDLE, &bLockInFullScreenMode), PRL_ERR_INVALID_ARG)
		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsLockInFullScreenMode(m_VmHandle, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetLockInFullScreenMode()
{
	CREATE_TEST_VM
		PRL_BOOL bLockInFullScreenMode;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsLockInFullScreenMode(hVm, &bLockInFullScreenMode))
		bLockInFullScreenMode = !bLockInFullScreenMode;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetLockInFullScreenMode(hVm, bLockInFullScreenMode))
		PRL_BOOL bActualLockInFullScreenMode;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsLockInFullScreenMode(hVm, &bActualLockInFullScreenMode))
		QCOMPARE(bActualLockInFullScreenMode, bLockInFullScreenMode);
}

void PrlVmManipulationsTest::testSetLockInFullScreenModeOnWrongParams()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetLockInFullScreenMode(m_ServerHandle, PRL_TRUE), PRL_ERR_INVALID_ARG)
		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetLockInFullScreenMode(PRL_INVALID_HANDLE, PRL_TRUE), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetActionOnWindowClose()
{
	testCreateVmFromConfig();
	PRL_VM_ACTION_ON_WINDOW_CLOSE nActionOnWindowClose = PWC_VM_STOP;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetActionOnWindowClose(m_VmHandle, &nActionOnWindowClose))
		INIT_VM_CONF_FROM_FILE
		QCOMPARE(quint32(nActionOnWindowClose), quint32(_vm_conf.getVmSettings()->getShutdown()->getOnVmWindowClose()));
}

void PrlVmManipulationsTest::testGetActionOnWindowCloseOnWrongParams()
{
	testCreateVmFromConfig();
	PRL_VM_ACTION_ON_WINDOW_CLOSE nActionOnWindowClose = PWC_VM_STOP;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetActionOnWindowClose(m_ServerHandle, &nActionOnWindowClose), PRL_ERR_INVALID_ARG)
		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetActionOnWindowClose(PRL_INVALID_HANDLE, &nActionOnWindowClose), PRL_ERR_INVALID_ARG)
		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetActionOnWindowClose(m_VmHandle, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetActionOnWindowClose()
{
	CREATE_TEST_VM
		PRL_VM_ACTION_ON_WINDOW_CLOSE nActionOnWindowClose = PWC_VM_STOP;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetActionOnWindowClose(hVm, &nActionOnWindowClose))
		nActionOnWindowClose = (PWC_VM_STOP == nActionOnWindowClose ? PWC_VM_DO_NOTHING : PWC_VM_STOP);
	CHECK_RET_CODE_EXP(PrlVmCfg_SetActionOnWindowClose(hVm, nActionOnWindowClose))
		PRL_VM_ACTION_ON_WINDOW_CLOSE nActualActionOnWindowClose = PWC_VM_STOP;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetActionOnWindowClose(hVm, &nActualActionOnWindowClose))
		QCOMPARE(quint32(nActualActionOnWindowClose), quint32(nActionOnWindowClose));
}

void PrlVmManipulationsTest::testSetActionOnWindowCloseOnWrongParams()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetActionOnWindowClose(m_ServerHandle, PWC_VM_STOP), PRL_ERR_INVALID_ARG)
		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetActionOnWindowClose(PRL_INVALID_HANDLE, PWC_VM_STOP), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetActionOnStopMode()
{
	CREATE_TEST_VM
	PRL_VM_ACTION_ON_STOP nActionOnStopMode;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetActionOnStopMode(hVm, &nActionOnStopMode))
	nActionOnStopMode = (PRL_VM_ACTION_ON_STOP )(nActionOnStopMode ^ POD_OPTIMIZE_FOR_GAMES);
	CHECK_RET_CODE_EXP(PrlVmCfg_SetActionOnStopMode(hVm, nActionOnStopMode))
	PRL_VM_ACTION_ON_STOP nActualActionOnStopMode;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetActionOnStopMode(hVm, &nActualActionOnStopMode))
	QCOMPARE(nActualActionOnStopMode, nActionOnStopMode);
}

void PrlVmManipulationsTest::testActionOnStopModeOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_VM_ACTION_ON_STOP nActionOnStopMode;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetActionOnStopMode(m_ServerHandle, &nActionOnStopMode),\
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetActionOnStopMode(m_ServerHandle, PAS_QUIT_APPLICATION),\
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetActionOnStopMode(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testGetVmLocation()
{
	testCreateVmFromConfig();
	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

		QString sVmHomePath;
	PRL_EXTRACT_STRING_VALUE(sVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath)
		QVERIFY(!sVmHomePath.isEmpty());

	PRL_VM_LOCATION nVmLocation = PVL_UNKNOWN;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetLocation(m_VmHandle, &nVmLocation))

		if (CFileHelper::isLocalPath(sVmHomePath))
			QCOMPARE(quint32(PVL_LOCAL_FS), quint32(nVmLocation));
		else if (CFileHelper::isRemotePath(sVmHomePath))
			QCOMPARE(quint32(PVL_REMOTE_FS), quint32(nVmLocation));
		else if (CFileHelper::isUsbDrive(sVmHomePath))
			QCOMPARE(quint32(PVL_USB_DRIVE), quint32(nVmLocation));
		else if (CFileHelper::isFireWireDrive(sVmHomePath))
			QCOMPARE(quint32(PVL_FIREWIRE_DRIVE), quint32(nVmLocation));
		else
			QFAIL("Abnormal situation: VM location unknown!");
}

void PrlVmManipulationsTest::testGetVmLocationOnWrongParams()
{
	testCreateVmFromConfig();

	PRL_VM_LOCATION nVmLocation = PVL_UNKNOWN;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetLocation(PRL_INVALID_HANDLE, &nVmLocation), PRL_ERR_INVALID_ARG)
		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetLocation(m_ServerHandle, &nVmLocation), PRL_ERR_INVALID_ARG)
		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetLocation(m_VmHandle, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetVmLocationOnInvalidVm()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	// Make VM invalid
	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

		QString sVmHomePath;
	PRL_EXTRACT_STRING_VALUE(sVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath);

	QVERIFY(QFile::remove(sVmHomePath));

	// Get VM config with old name
	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

		PRL_VM_LOCATION nVmLocation = PVL_LOCAL_FS;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetLocation(m_VmHandle, &nVmLocation))

		QCOMPARE(quint32(PVL_UNKNOWN), quint32(nVmLocation));

	//Check that VM location not cached after VM delete
	hJob.reset(PrlVm_Restore(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)
		DELETE_VM(m_VmHandle)
		testCreateVmFromConfig();
	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)
		CHECK_RET_CODE_EXP(PrlVmCfg_GetLocation(m_VmHandle, &nVmLocation))

		if (CFileHelper::isLocalPath(sVmHomePath))
			QCOMPARE(quint32(PVL_LOCAL_FS), quint32(nVmLocation));
		else if (CFileHelper::isRemotePath(sVmHomePath))
			QCOMPARE(quint32(PVL_REMOTE_FS), quint32(nVmLocation));
		else if (CFileHelper::isUsbDrive(sVmHomePath))
			QCOMPARE(quint32(PVL_USB_DRIVE), quint32(nVmLocation));
		else if (CFileHelper::isFireWireDrive(sVmHomePath))
			QCOMPARE(quint32(PVL_FIREWIRE_DRIVE), quint32(nVmLocation));
		else
			QFAIL("Abnormal situation: VM location unknown!");
}

void PrlVmManipulationsTest::testIsUseDefaultAnswers()
{
	testCreateVmFromConfig();
	PRL_BOOL bUseDefaultAnswers = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUseDefaultAnswers(m_VmHandle, &bUseDefaultAnswers))
		INIT_VM_CONF_FROM_FILE
		QVERIFY(bUseDefaultAnswers == PRL_BOOL(_vm_conf.getVmSettings()->getVmRuntimeOptions()->isUseDefaultAnswers()));

	//By default mechanism should be switched off!!! - check it now
	QVERIFY(PRL_FALSE == bUseDefaultAnswers);
}

void PrlVmManipulationsTest::testIsUseDefaultAnswersOnWrongParams()
{
	testCreateVmFromConfig();
	PRL_BOOL bUseDefaultAnswers = PRL_FALSE;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsUseDefaultAnswers(PRL_INVALID_HANDLE, &bUseDefaultAnswers), PRL_ERR_INVALID_ARG)
		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsUseDefaultAnswers(m_ServerHandle, &bUseDefaultAnswers), PRL_ERR_INVALID_ARG)
		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsUseDefaultAnswers(m_VmHandle, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetUseDefaultAnswers()
{
	CREATE_TEST_VM
		PRL_BOOL bUseDefaultAnswers;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUseDefaultAnswers(hVm, &bUseDefaultAnswers))
		bUseDefaultAnswers = !bUseDefaultAnswers;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetUseDefaultAnswers(hVm, bUseDefaultAnswers))
		PRL_BOOL bActualUseDefaultAnswers = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsUseDefaultAnswers(hVm, &bActualUseDefaultAnswers))
		QCOMPARE(quint32(bActualUseDefaultAnswers), quint32(bUseDefaultAnswers));
}

void PrlVmManipulationsTest::testSetUseDefaultAnswersOnWrongParams()
{
	testCreateVmFromConfig();
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetUseDefaultAnswers(PRL_INVALID_HANDLE, PRL_TRUE), PRL_ERR_INVALID_ARG)
		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetUseDefaultAnswers(m_ServerHandle, PRL_TRUE), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testCancelCompactOnWrongParams()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_CancelCompact(m_ServerHandle), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testCompactOnWrongParams()
{
	SKIP_TEST_IN_CT_MODE

	CHECK_ASYNC_OP_FAILED(PrlVm_Compact(m_ServerHandle, 1, 0), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testCompactOnInvalidArgFromVm()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_Start(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	stop_vm_on_exit stop_vm(m_VmHandle);

	CHECK_ASYNC_OP_FAILED(PrlVm_Compact(m_VmHandle, 0, PACF_NON_INTERACTIVE_MODE), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testCompactOnInvalidVmState()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_Start(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	stop_vm_on_exit stop_vm(m_VmHandle);

	hJob.reset(PrlVm_Pause(m_VmHandle, PRL_FALSE));
	CHECK_JOB_RET_CODE(hJob);

	CHECK_ASYNC_OP_FAILED(PrlVm_Compact(m_VmHandle, PIM_IDE_0_0, PACF_NON_INTERACTIVE_MODE),
		PRL_ERR_COMPACT_WRONG_VM_STATE);
}

void PrlVmManipulationsTest::testStartVmInCompactModeOnSuspendState()
{
	SKIP_TEST_IN_CT_MODE

	// Prepare suspend state
	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_Start(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	stop_vm_on_exit stop_vm(m_VmHandle);

	hJob.reset(PrlVm_Suspend(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	WAIT_FOR_VM_STATE(m_VmHandle, VMS_SUSPENDED);

	// Start VM compacting with error

	CHECK_ASYNC_OP_FAILED(PrlVm_StartEx(m_VmHandle, PSM_VM_START_FOR_COMPACT, PIM_IDE_0_0),
		PRL_ERR_COMPACT_WRONG_VM_STATE);
}

void PrlVmManipulationsTest::testVmCompactOnSuspendState()
{
	SKIP_TEST_IN_CT_MODE

	// Prepare suspend state
	testCreateVmFromConfig();

	SdkHandleWrap hJob(PrlVm_Start(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	stop_vm_on_exit stop_vm(m_VmHandle);

	hJob.reset(PrlVm_Suspend(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	WAIT_FOR_VM_STATE(m_VmHandle, VMS_SUSPENDED);

	// Start VM compacting with error

	CHECK_ASYNC_OP_FAILED(PrlVm_Compact(m_VmHandle, PIM_IDE_0_0, 0),
		PRL_ERR_COMPACT_WRONG_VM_STATE);
}

void PrlVmManipulationsTest::testAutoCompressEnabled()
{
	VM_CONFIG_INIT;

	VM_CONFIG_TO_XML_OBJECT;
	_vm_conf.getVmSettings()->getVmAutoCompress()->setEnabled(false);
	VM_CONFIG_FROM_XML_OBJECT;

	PRL_BOOL bEnabled = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsAutoCompressEnabled(m_VmHandle, &bEnabled));
	QVERIFY(bEnabled == PRL_FALSE);

	PRL_BOOL bEnabledExpected = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetAutoCompressEnabled(m_VmHandle, bEnabledExpected));
	CHECK_RET_CODE_EXP(PrlVmCfg_IsAutoCompressEnabled(m_VmHandle, &bEnabled));
	QCOMPARE(bEnabled, bEnabledExpected);
}

void PrlVmManipulationsTest::testAutoCompressEnabledOnWrongParams()
{
	VM_CONFIG_INIT;

	PRL_BOOL bEnabled = PRL_TRUE;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsAutoCompressEnabled(m_ServerHandle, &bEnabled),
		PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetAutoCompressEnabled(m_ServerHandle, bEnabled),
		PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsAutoCompressEnabled(m_VmHandle, NULL),
		PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testAutoCompressInterval()
{
	VM_CONFIG_INIT;

	VM_CONFIG_TO_XML_OBJECT;
	_vm_conf.getVmSettings()->getVmAutoCompress()->setPeriod(600);
	VM_CONFIG_FROM_XML_OBJECT;

	PRL_UINT32 nInterval = 300;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAutoCompressInterval(m_VmHandle, &nInterval));
	QVERIFY(nInterval == 600);

	PRL_UINT32 nIntervalExpected = 450;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetAutoCompressInterval(m_VmHandle, nIntervalExpected));
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAutoCompressInterval(m_VmHandle, &nInterval));
	QCOMPARE(nInterval, nIntervalExpected);
}

void PrlVmManipulationsTest::testAutoCompressIntervalOnWrongParams()
{
	VM_CONFIG_INIT;

	PRL_UINT32 nInterval = 200;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetAutoCompressInterval(m_ServerHandle, &nInterval),
		PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetAutoCompressInterval(m_ServerHandle, nInterval),
		PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetAutoCompressInterval(m_VmHandle, NULL),
		PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testFreeDiskSpaceRatio()
{
	VM_CONFIG_INIT;

	VM_CONFIG_TO_XML_OBJECT;
	_vm_conf.getVmSettings()->getVmAutoCompress()->setFreeDiskSpaceRatio(55.7);
	VM_CONFIG_FROM_XML_OBJECT;

	PRL_DOUBLE dFreeDiskSpaceRatio = 23.8;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetFreeDiskSpaceRatio(m_VmHandle, &dFreeDiskSpaceRatio));
	QVERIFY(dFreeDiskSpaceRatio == 55.7);

	PRL_DOUBLE dFreeDiskSpaceRatioExpected = 34.0;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetFreeDiskSpaceRatio(m_VmHandle, dFreeDiskSpaceRatioExpected));
	CHECK_RET_CODE_EXP(PrlVmCfg_GetFreeDiskSpaceRatio(m_VmHandle, &dFreeDiskSpaceRatio));
	QCOMPARE(dFreeDiskSpaceRatio, dFreeDiskSpaceRatioExpected);
}

void PrlVmManipulationsTest::testFreeDiskSpaceRatioOnWrongParams()
{
	VM_CONFIG_INIT;

	PRL_DOUBLE dFreeDiskSpaceRatio = 10.0;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetFreeDiskSpaceRatio(m_ServerHandle, &dFreeDiskSpaceRatio),
		PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetFreeDiskSpaceRatio(m_ServerHandle, dFreeDiskSpaceRatio),
		PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetFreeDiskSpaceRatio(m_VmHandle, NULL),
		PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testStartAndStopVmInSafeModeForNonInteractiveSession()
{
	if (TestConfig::isServerModePSBM())
	{
		QSKIP("Safe mode is not available in PSBM !", SkipAll);
		return;
	}

	SdkHandleWrap hJob(PrlSrv_SetNonInteractiveSession(m_ServerHandle, PRL_TRUE, 0));
	CHECK_JOB_RET_CODE(hJob);

	INITIALIZE_VM("./SDKTest_vm_with_devices_config.xml")

		hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)

		hJob.reset(PrlVm_StartEx(m_VmHandle, PSM_VM_SAFE_START, 0));
	CHECK_JOB_RET_CODE(hJob)

		hJob.reset(PrlVm_Stop(m_VmHandle, PRL_FALSE));
	CHECK_JOB_RET_CODE(hJob)

		WAIT_FOR_VM_STATE(m_VmHandle, VMS_STOPPED)
}

void PrlVmManipulationsTest::testChangeSid()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();

	CHECK_ASYNC_OP_FAILED(PrlVm_ChangeSid(m_VmHandle, 0), PRL_ERR_CHANGESID_NOT_SUPPORTED)
}

void PrlVmManipulationsTest::testChangeSid2()
{
	SKIP_TEST_IN_CT_MODE

	SET_DEFAULT_VM_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE );
	SdkHandleWrap hJob(PrlVm_Reg( m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)

		CHECK_ASYNC_OP_FAILED(PrlVm_ChangeSid(m_VmHandle, 0), PRL_ERR_CHANGESID_GUEST_TOOLS_NOT_AVAILABLE)
}

void PrlVmManipulationsTest::testChangeSidOnWrongParams()
{
	SKIP_TEST_IN_CT_MODE

	CHECK_ASYNC_OP_FAILED(PrlVm_ChangeSid(PRL_INVALID_HANDLE, 0), PRL_ERR_INVALID_ARG)
		CHECK_ASYNC_OP_FAILED(PrlVm_ChangeSid(m_ServerHandle, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetAndAssemblyPackedReportForRunningVm()
{
	SKIP_TEST_IN_CT_MODE_unimp

	testCreateVmFromConfig();
	CHECK_HANDLE_TYPE(m_VmHandle, PHT_VIRTUAL_MACHINE)
		SdkHandleWrap hJob(PrlVm_Start(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

		stop_vm_on_exit stop_vm(m_VmHandle);

	hJob.reset(PrlVm_GetPackedProblemReport(m_VmHandle, 0));
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT*10))
		PRL_RESULT nRetCode = PRL_ERR_UNINITIALIZED;

		CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nRetCode))
		if (PRL_ERR_UNRECOGNIZED_REQUEST == nRetCode)
			QSKIP("New problem report scheme not supported on server", SkipAll);
		else
			CHECK_RET_CODE_EXP(nRetCode)

			SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

		SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hProblemReport.GetHandlePtr()))
		CHECK_HANDLE_TYPE(hProblemReport, PHT_PROBLEM_REPORT)

		hJob.reset(PrlReport_Assembly(hProblemReport, PPRF_ADD_CLIENT_PART));
	CHECK_JOB_RET_CODE(hJob)
}

//////////////

void PrlVmManipulationsTest::testVmGetVmInfo()
{
	SdkHandleWrap hVmInfo;
	testCreateVmFromConfig();
	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)
	CHECK_RET_CODE_EXP(PrlVmCfg_GetVmInfo(m_VmHandle, hVmInfo.GetHandlePtr()));
	CHECK_HANDLE_TYPE( hVmInfo, PHT_VM_INFO);
}

void PrlVmManipulationsTest::testVmGetVmInfoOnNullPtr()
{
	testCreateVmFromConfig();
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetVmInfo(m_VmHandle, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmGetVmInfoOnNullVmHandle()
{
	SdkHandleWrap hVmInfo;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetVmInfo(PRL_INVALID_HANDLE, hVmInfo.GetHandlePtr()),\
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmGetVmInfoOnNonVmHandle()
{
	SdkHandleWrap hVmInfo;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetVmInfo(m_ServerHandle, hVmInfo.GetHandlePtr()),\
	PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testLinkedCloneVm()
{
	SKIP_TEST_IN_CT_MODE

	SdkHandleWrap hJob(PrlSrv_GetSrvConfig(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hSrvConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hSrvConfig.GetHandlePtr()))

	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(m_VmHandle, hSrvConfig, PVS_GUEST_VER_WIN_WINDOWS7, PRL_TRUE))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, QTest::currentTestFunction()))

	hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_CloneEx(m_VmHandle, QSTR2UTF8(QString("%1_cloned").arg(QTest::currentTestFunction())), "", PACF_NON_INTERACTIVE_MODE | PCVF_LINKED_CLONE));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_ClonedVmHandle.GetHandlePtr()))
	CHECK_HANDLE_TYPE(m_ClonedVmHandle, PHT_VIRTUAL_MACHINE)

	hJob.reset(PrlVm_GetSnapshotsTree(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QString sSnapshotsTree;
	PRL_EXTRACT_STRING_VALUE(sSnapshotsTree, hResult, PrlResult_GetParamAsString)

	QDomDocument doc;
	QVERIFY(doc.setContent(sSnapshotsTree));

	QDomElement docElem = doc.documentElement();

	QCOMPARE(docElem.tagName(), QString(XML_SS_CONFIG_EL_ROOT));

	QDomNode docNodeItem = docElem.firstChild();
	QVERIFY(!docNodeItem.isNull());
	QVERIFY(docNodeItem.isElement());
	QDomElement docElemItem = docNodeItem.toElement();
	CSavedStateTree _tree;
	QCOMPARE(quint32(_tree.ReadXml(&docElemItem)), quint32(SnapshotParser::RcSuccess));
	QCOMPARE(quint32(1), quint32(_tree.GetChildCount()));

	QString sOriginalVmUuid, sLinkedVmUuid;
	PRL_EXTRACT_STRING_VALUE(sOriginalVmUuid, m_VmHandle, PrlVmCfg_GetUuid)
	PRL_EXTRACT_STRING_VALUE(sLinkedVmUuid, m_ClonedVmHandle, PrlVmCfg_GetLinkedVmUuid)
	QCOMPARE(sOriginalVmUuid, sLinkedVmUuid);
}

void PrlVmManipulationsTest::testTryToLinkedCloneVmWithoutSnapshotsOnReadOnlyAccess()
{
	SKIP_TEST_IN_CT_MODE

	if (!TestConfig::isServerMode())
		QSKIP("This test doesn't make sense in non server mode", SkipAll);

	SimpleServerWrapper _connection1, _connection2;
	QVERIFY(_connection1.Login(TestConfig::getUserLogin()));
	QVERIFY(_connection2.Login(TestConfig::getUserLogin2()));

	SdkHandleWrap hJob(PrlSrv_GetSrvConfig(_connection1.GetServerHandle()));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hSrvConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hSrvConfig.GetHandlePtr()))

	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(_connection1.GetServerHandle(), hVm.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(hVm, hSrvConfig, PVS_GUEST_VER_WIN_WINDOWS7, PRL_TRUE))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(hVm, QTest::currentTestFunction()))

	QVERIFY(_connection1.CreateTestVm(hVm));

	hJob.reset(PrlVm_RefreshConfig(_connection1.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hAcl;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetAccessRights(_connection1.GetTestVm(), hAcl.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlAcl_SetAccessForOthers(hAcl, PAO_VM_SHARED_ON_VIEW))

	hJob.reset(PrlVm_UpdateSecurity(_connection1.GetTestVm(), hAcl));
	CHECK_JOB_RET_CODE(hJob)

	QString sVmUuid;
	PRL_EXTRACT_STRING_VALUE(sVmUuid, _connection1.GetTestVm(), PrlVmCfg_GetUuid)

	CHECK_ASYNC_OP_FAILED(PrlVm_CloneEx(_connection2.GetTestVm(sVmUuid), QSTR2UTF8(Uuid::createUuid().toString()), "", PACF_NON_INTERACTIVE_MODE | PCVF_LINKED_CLONE),
		PRL_ERR_NOT_ENOUGH_RIGHTS_FOR_LINKED_CLONE)
}

void PrlVmManipulationsTest::testVmConvertDisksOnWrongParams()
{
#ifndef _WIN_
	QSKIP("Only on windows host this test will always be passed !", SkipAll);
#endif

	testCreateVmWithDevices();

	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlVm_ConvertDisks(m_ServerHandle, PIM_SCSI_0_0, PCVD_TO_PLAIN_DISK),
							PRL_ERR_INVALID_ARG);
	CHECK_ASYNC_OP_FAILED(PrlVm_CancelConvertDisks(m_ServerHandle, 0),
							PRL_ERR_INVALID_ARG);

	// Conversion mode does not define
	CHECK_ASYNC_OP_FAILED(PrlVm_ConvertDisks(m_VmHandle, PIM_IDE_0_0, 0),
							PRL_ERR_INVALID_ARG);
	// Wrong mix flags
	CHECK_ASYNC_OP_FAILED(PrlVm_ConvertDisks(m_VmHandle, PIM_IDE_0_0,
												(PCVD_TO_SPLIT_DISK | PCVD_MERGE_ALL_SNAPSHOTS)),
							PRL_ERR_INVALID_ARG);

	// Wrong conversion
	CHECK_ASYNC_OP_FAILED(PrlVm_ConvertDisks(m_VmHandle, PIM_IDE_0_0,
												(PCVD_TO_EXPANDING_DISK | PCVD_TO_NON_SPLIT_DISK)),
							PRL_ERR_CONV_HD_NO_ONE_DISK_FOR_CONVERSION);
	// Wrong disk mask
	CHECK_ASYNC_OP_FAILED(PrlVm_ConvertDisks(m_VmHandle, PIM_SCSI_4_0, PCVD_TO_PLAIN_DISK),
							PRL_ERR_CONV_HD_NO_ONE_DISK_FOR_CONVERSION);

	// Conversion conflict
	CHECK_ASYNC_OP_FAILED(PrlVm_ConvertDisks(m_VmHandle, PIM_IDE_0_0,
												(PCVD_TO_PLAIN_DISK | PCVD_TO_EXPANDING_DISK)),
							PRL_ERR_CONV_HD_CONFLICT);
	CHECK_ASYNC_OP_FAILED(PrlVm_ConvertDisks(m_VmHandle, PIM_IDE_0_0,
												(PCVD_TO_SPLIT_DISK | PCVD_TO_NON_SPLIT_DISK)),
							PRL_ERR_CONV_HD_CONFLICT);

}

void PrlVmManipulationsTest::testIsEncrypted()
{
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm( m_ServerHandle, hVm.GetHandlePtr() ))
	PRL_BOOL bEncrypted = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsEncrypted( hVm, &bEncrypted ))
	QVERIFY(PRL_FALSE == bEncrypted);
	CVmConfiguration _vm_cfg;
	_vm_cfg.getVmSettings()->getVmEncryption()->setEnabled( true );
	CHECK_RET_CODE_EXP(PrlHandle_FromString( hVm, QSTR2UTF8(_vm_cfg.toString()) ))
	CHECK_RET_CODE_EXP(PrlVmCfg_IsEncrypted( hVm, &bEncrypted ))
	QVERIFY(PRL_TRUE == bEncrypted);
}

void PrlVmManipulationsTest::testIsEncryptedOnWrongParams()
{
	PRL_BOOL bEncrypted = PRL_FALSE;
	//Invalid handle case
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsEncrypted( PRL_INVALID_HANDLE, &bEncrypted ), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsEncrypted( m_ServerHandle, &bEncrypted ), PRL_ERR_INVALID_ARG)

	//NULL pointer case
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm( m_ServerHandle, hVm.GetHandlePtr() ))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsEncrypted( hVm, NULL ), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmAuthoriseOnWrongParams()
{
	//Wrong handle case
	CHECK_ASYNC_OP_FAILED(PrlVm_Authorise( PRL_INVALID_HANDLE, 0, 0 ), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlVm_Authorise( m_ServerHandle, 0, 0 ), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmChangePasswordOnWrongParams()
{
	//Wrong handle case
	CHECK_ASYNC_OP_FAILED(PrlVm_ChangePassword( PRL_INVALID_HANDLE, 0, 0, 0 ), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlVm_ChangePassword( m_ServerHandle, 0, 0, 0 ), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmEncryptOnWrongParams()
{
	//Wrong handle case
	CHECK_ASYNC_OP_FAILED(PrlVm_Encrypt( PRL_INVALID_HANDLE, 0, 0, 0 ), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlVm_Encrypt( m_ServerHandle, 0, 0, 0 ), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmDecryptOnWrongParams()
{
	//Wrong handle case
	CHECK_ASYNC_OP_FAILED(PrlVm_Decrypt( PRL_INVALID_HANDLE, 0, 0 ), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlVm_Decrypt( m_ServerHandle, 0, 0 ), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmAuthoriseOnValidPassword()
{
	SKIP_TEST_IN_CT_MODE

	QSKIP("Skip till VM encrypt functionality will be finished", SkipAll);
	testCreateVmFromConfig();
	QString sPassword = "1q2w3e";
	SdkHandleWrap hJob(PrlVm_Encrypt( m_VmHandle, QSTR2UTF8(sPassword), 0, 0 ));
	CHECK_JOB_RET_CODE(hJob)
	QString sVmUuid;
	PRL_EXTRACT_STRING_VALUE(sVmUuid, m_VmHandle, PrlVmCfg_GetUuid)

	SimpleServerWrapper _another_session;
	if ( TestConfig::isServerMode() )
	{
		QVERIFY(_another_session.Login( TestConfig::getUserLogin() ));
	}
	else
	{
		QVERIFY(_another_session.LoginLocal());
	}
	QVERIFY(_another_session.IsConnected());
	SdkHandleWrap hVm = _another_session.GetTestVm( sVmUuid );
	QVERIFY(PRL_INVALID_HANDLE != hVm.GetHandle());
	hJob.reset(PrlVm_Authorise( hVm, QSTR2UTF8(sPassword), 0 ));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlVmManipulationsTest::testVmAuthoriseOnWrongPassword()
{
	SKIP_TEST_IN_CT_MODE

	QSKIP("Skip till VM encrypt functionality will be finished", SkipAll);
	testCreateVmFromConfig();
	QString sPassword = "1q2w3e";
	SdkHandleWrap hJob(PrlVm_Encrypt( m_VmHandle, QSTR2UTF8(sPassword), 0, 0 ));
	CHECK_JOB_RET_CODE(hJob)
	QString sVmUuid;
	PRL_EXTRACT_STRING_VALUE(sVmUuid, m_VmHandle, PrlVmCfg_GetUuid)

	SimpleServerWrapper _another_session;
	if ( TestConfig::isServerMode() )
	{
		QVERIFY(_another_session.Login( TestConfig::getUserLogin() ));
	}
	else
	{
		QVERIFY(_another_session.LoginLocal());
	}
	QVERIFY(_another_session.IsConnected());
	SdkHandleWrap hVm = _another_session.GetTestVm( sVmUuid );
	QVERIFY(PRL_INVALID_HANDLE != hVm.GetHandle());
	CHECK_ASYNC_OP_FAILED(PrlVm_Authorise( hVm, "wrong password", 0 ), PRL_ERR_WRONG_PASSWORD_TO_ENCRYPTED_VM)
}

void PrlVmManipulationsTest::testVmEncrypt()
{
	SKIP_TEST_IN_CT_MODE

	QSKIP("Skip till VM encrypt functionality will be finished", SkipAll);
	//FIXME: temporarily test which should be replace with proper one
	//when correspond functionality will be implemented on dispatcher
	//side
	testCreateVmFromConfig();
	CHECK_ASYNC_OP_FAILED(PrlVm_Encrypt( m_VmHandle, 0, 0, 0 ), PRL_ERR_UNIMPLEMENTED)
}

void PrlVmManipulationsTest::testVmDecrypt()
{
	SKIP_TEST_IN_CT_MODE

	QSKIP("Skip till VM encrypt functionality will be finished", SkipAll);
	//FIXME: temporarily test which should be replace with proper one
	//when correspond functionality will be implemented on dispatcher
	//side
	testCreateVmFromConfig();
	CHECK_ASYNC_OP_FAILED(PrlVm_Decrypt( m_VmHandle, 0, 0 ), PRL_ERR_UNIMPLEMENTED)
}


void PrlVmManipulationsTest::testHardDiskCheckPassword()
{
	CHECK_WHETHER_ENCRYPTION_DISABLED

	SKIP_TEST_IN_CT_MODE

	testCreateVmWithDevices();

	m_JobHandle.reset(PrlVm_Encrypt(m_VmHandle, "12345", 0, 0 ));
	CHECK_JOB_RET_CODE(m_JobHandle)

	PRL_UINT32 nHddCount = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDevsCountByType(m_VmHandle, PDE_HARD_DISK, &nHddCount));
	QVERIFY(nHddCount > 0);

	SdkHandleWrap hHdd;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetDevByType(m_VmHandle, PDE_HARD_DISK, 0, hHdd.GetHandlePtr()));

// 1. Wrong password
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetPassword( hHdd, "abc" ));
	CHECK_ASYNC_OP_FAILED(PrlVmDevHd_CheckPassword( hHdd, 0 ), PRL_ERR_WRONG_PASSWORD_TO_ENCRYPTED_HDD);

// 2. Right password
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetPassword( hHdd, "12345" ));
	m_JobHandle.reset(PrlVmDevHd_CheckPassword( hHdd, 0 ));
	CHECK_JOB_RET_CODE(m_JobHandle)
}

void PrlVmManipulationsTest::testAddHardDiskToEncryptedVm()
{
	CHECK_WHETHER_ENCRYPTION_DISABLED

	SKIP_TEST_IN_CT_MODE

	testCreateVmWithDevices();

	m_JobHandle.reset(PrlVm_Encrypt(m_VmHandle, "12345", 0, 0 ));
	CHECK_JOB_RET_CODE(m_JobHandle)

	m_JobHandle.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle);

// Add hard disk

	QString qsVmDir;
	PRL_EXTRACT_STRING_VALUE(qsVmDir, m_VmHandle, PrlVmCfg_GetHomePath);
	qsVmDir = QFileInfo(qsVmDir).path();

	QString qsDir = qsVmDir + "/hdd_encrypted.hdd";

	SdkHandleWrap hHdd;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hHdd.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hHdd, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hHdd, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hHdd, QSTR2UTF8(qsDir)));
	CHECK_RET_CODE_EXP(PrlVmDev_SetFriendlyName(hHdd, QSTR2UTF8(qsDir)));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hHdd, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hHdd, 1));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hHdd, PDT_USE_IMAGE_FILE));
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskSize(hHdd, 1024 * 1024));
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskType(hHdd, PHD_EXPANDING_HARD_DISK));

	m_JobHandle.reset(PrlVmDev_CreateImage(hHdd, PRL_TRUE, PRL_TRUE));
	CHECK_JOB_RET_CODE(m_JobHandle);

	m_JobHandle.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle);

// Check password for encrypted (has to be) hard disk
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetPassword( hHdd, "12345" ));
	m_JobHandle.reset(PrlVmDevHd_CheckPassword( hHdd, 0 ));
	CHECK_JOB_RET_CODE(m_JobHandle)
}

void PrlVmManipulationsTest::testGetVmHddsSize()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmWithDevices();
	const PRL_UINT64 nExpectedSize = PRL_UINT64(5)*1024*1024*1024;
	SdkHandleWrap hJob(PrlVm_StoreValueByKey( m_VmHandle
		, PRL_KEY_TO_REQUEST_VM_CLONE_SIZE
		, QSTR2UTF8(QString::number( nExpectedSize ))
		, 0 ));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	QString sValue;
	PRL_EXTRACT_STRING_VALUE(sValue, hResult, PrlResult_GetParamAsString)
	bool bOk = false;
	PRL_UINT64 nActualSize = sValue.toULongLong( &bOk );
	QVERIFY(bOk);
	QCOMPARE(nExpectedSize, nActualSize);
}

void PrlVmManipulationsTest::testGetAppTemplateList()
{
	testCreateVmFromConfig();
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVmType(m_VmHandle, PVT_CT))
	INIT_VM_CONF_FROM_FILE
	PRL_CHECK_STRINGS_LIST(m_VmHandle, PrlVmCfg_GetAppTemplateList,\
			_vm_conf.getCtSettings()->getAppTemplate())
}

void PrlVmManipulationsTest::testSetAppTemplateList()
{
	testCreateVmFromConfig();
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVmType(m_VmHandle, PVT_CT))

	QStringList lstNewAppTemplate = QStringList()<<"app1"<<"app2";
	SdkHandleWrap hNewAppTemplateList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hNewAppTemplateList.GetHandlePtr()))
	foreach(QString sTmpl, lstNewAppTemplate)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewAppTemplateList,
					QSTR2UTF8(sTmpl)))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetAppTemplateList(m_VmHandle, hNewAppTemplateList))
	PRL_CHECK_STRINGS_LIST(m_VmHandle, PrlVmCfg_GetAppTemplateList, lstNewAppTemplate)
}

void PrlVmManipulationsTest::testGetAppTemplateListOnWrongParams()
{
	testCreateVmFromConfig();
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetAppTemplateList(m_VmHandle, 0), \
		PRL_ERR_INVALID_ARG)

	SdkHandleWrap hAppTemplateList;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetAppTemplateList(PRL_INVALID_HANDLE, \
		hAppTemplateList.GetHandlePtr()), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetAppTemplateList(m_ServerHandle, \
		hAppTemplateList.GetHandlePtr()), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetAppTemplateListOnWrongParams()
{
	testCreateVmFromConfig();
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetAppTemplateList(PRL_INVALID_HANDLE, \
		PRL_INVALID_HANDLE), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetAppTemplateList(m_ServerHandle, \
		PRL_INVALID_HANDLE), PRL_ERR_INVALID_ARG)

	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetAppTemplateList(hVm, m_ServerHandle),\
			PRL_ERR_INVALID_ARG)

	/* prepare list of templates */
	QStringList lstAppTemplate = QStringList()<<"app1"<<"app2";
	SdkHandleWrap hNewAppTemplateList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hNewAppTemplateList.GetHandlePtr()))
	foreach(QString sTmpl, lstAppTemplate)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewAppTemplateList, QSTR2UTF8(sTmpl)))

	PRL_VM_TYPE nVmType;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetVmType(m_VmHandle, &nVmType))
	/* prohibited for Vms, allowed only for Containers */
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetAppTemplateList(m_VmHandle, hNewAppTemplateList),\
			(nVmType == PVT_VM ? PRL_ERR_INVALID_ARG : PRL_ERR_SUCCESS))
}

#define CREATE_NET_DEVICE(device, net_type)\
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev( m_VmHandle, PDE_GENERIC_NETWORK_ADAPTER, device.GetHandlePtr() ))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType( device, (PRL_VM_DEV_EMULATION_TYPE )net_type ))\
	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled( device, TRUE ))\
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetBoundAdapterName( device, "Default" ))\
	CHECK_RET_CODE_EXP(PrlVmDevNet_SetBoundAdapterIndex(device, -1) )\

#define CHECK_ADAPTER_DISCONNECTED(adapter)\
	{\
		PRL_BOOL bConnected = PRL_FALSE;\
		CHECK_RET_CODE_EXP(PrlVmDev_IsConnected( adapter, &bConnected ))\
		QVERIFY(PRL_FALSE == bConnected);\
	}

void PrlVmManipulationsTest::testAutoVirtualNetworksConfigureDuringVmCreation()
{
	if ( ! TestConfig::isServerMode() )
		QSKIP("Skipping test due it doesn't make sense in non server mode", SkipAll);

	QFile _file( ParallelsDirs::getNetworkConfigFilePath() );
	QVERIFY(_file.open( QIODevice::ReadOnly ));
	CParallelsNetworkConfig _netcfg( &_file );
	CHECK_RET_CODE_EXP(_netcfg.m_uiRcInit)
	_file.close();

	CVirtualNetwork *pHostOnlyNet = PrlNet::GetHostOnlyNetwork( &_netcfg, PRL_DEFAULT_HOSTONLY_INDEX );
	CVirtualNetwork *pBridgedNet = PrlNet::GetBridgedNetwork( &_netcfg );

	CHECK_RET_CODE_EXP(PrlSrv_CreateVm( m_ServerHandle, m_VmHandle.GetHandlePtr() ))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig( m_VmHandle, PRL_INVALID_HANDLE, PVS_GUEST_VER_WIN_2003, PRL_FALSE ))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName( m_VmHandle, QTest::currentTestFunction() ))

	SdkHandleWrap hSharedNet, hHostOnlyNet, hBridgedNet;

	CREATE_NET_DEVICE(hSharedNet, PNA_SHARED)
	CREATE_NET_DEVICE(hHostOnlyNet, PNA_HOST_ONLY)
	CREATE_NET_DEVICE(hBridgedNet, PNA_BRIDGED_ETHERNET)

	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_Reg( m_VmHandle, "", PRL_FALSE ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_RefreshConfig( m_VmHandle ))

	QString sSharedVirtId, sHostOnlyVirtId, sBridgedVirtId;
	PRL_EXTRACT_STRING_VALUE(sSharedVirtId, hSharedNet, PrlVmDevNet_GetVirtualNetworkId)
	PRL_EXTRACT_STRING_VALUE(sHostOnlyVirtId, hHostOnlyNet, PrlVmDevNet_GetVirtualNetworkId)
	PRL_EXTRACT_STRING_VALUE(sBridgedVirtId, hBridgedNet, PrlVmDevNet_GetVirtualNetworkId)

	if ( pHostOnlyNet )
	{
		QCOMPARE(pHostOnlyNet->getNetworkID(), sSharedVirtId);
		QCOMPARE(pHostOnlyNet->getNetworkID(), sHostOnlyVirtId);
	}
	else
	{
		CHECK_ADAPTER_DISCONNECTED(hSharedNet)
		CHECK_ADAPTER_DISCONNECTED(hHostOnlyNet)
	}

	if ( pBridgedNet )
	{
		QCOMPARE(pBridgedNet->getNetworkID(), sBridgedVirtId);
	}
	else
	{
		CHECK_ADAPTER_DISCONNECTED(hBridgedNet)
	}
}

void PrlVmManipulationsTest::testAutoVirtualNetworksConfigureDuringVmRegistration()
{
	if ( ! TestConfig::isServerMode() )
		QSKIP("Skipping test due it doesn't make sense in non server mode", SkipAll);

	QFile _file( ParallelsDirs::getNetworkConfigFilePath() );
	QVERIFY(_file.open( QIODevice::ReadOnly ));
	CParallelsNetworkConfig _netcfg( &_file );
	CHECK_RET_CODE_EXP(_netcfg.m_uiRcInit)
	_file.close();

	CVirtualNetwork *pHostOnlyNet = PrlNet::GetHostOnlyNetwork( &_netcfg, PRL_DEFAULT_HOSTONLY_INDEX);
	CVirtualNetwork *pBridgedNet = PrlNet::GetBridgedNetwork( &_netcfg );

	CHECK_RET_CODE_EXP(PrlSrv_CreateVm( m_ServerHandle, m_VmHandle.GetHandlePtr() ))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig( m_VmHandle, PRL_INVALID_HANDLE, PVS_GUEST_VER_WIN_2003, PRL_FALSE ))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName( m_VmHandle, QTest::currentTestFunction() ))

	SdkHandleWrap hSharedNet, hHostOnlyNet, hBridgedNet;

	CREATE_NET_DEVICE(hSharedNet, PNA_SHARED)
	CREATE_NET_DEVICE(hHostOnlyNet, PNA_HOST_ONLY)
	CREATE_NET_DEVICE(hBridgedNet, PNA_BRIDGED_ETHERNET)

	CAuthHelper _auth( TestConfig::getUserLogin() );
	QVERIFY(_auth.AuthUser( TestConfig::getUserPassword() ));
	QString sVmHomePath = ParallelsDirs::getSystemTempDir() + '/' + QTest::currentTestFunction();
	QVERIFY(CFileHelper::WriteDirectory( sVmHomePath, &_auth ));
	QString sVmConfigPath = QString( "%1/%2" ).arg( sVmHomePath ).arg( VMDIR_DEFAULT_VM_CONFIG_FILE );
	QVERIFY(CFileHelper::CreateBlankFile( sVmConfigPath, &_auth ));

	PRL_VOID_PTR pBuffer = NULL;
	CHECK_RET_CODE_EXP(PrlHandle_ToString( m_VmHandle, &pBuffer ))
	QString sVmConfig( UTF8_2QSTR((PRL_CONST_STR)pBuffer) );
	PrlBuffer_Free( pBuffer );
	CVmConfiguration _vm_conf;
	CHECK_RET_CODE_EXP(_vm_conf.fromString( sVmConfig ))
	CHECK_RET_CODE_EXP(_vm_conf.saveToFile( sVmConfigPath, true, true ))

	CHECK_ASYNC_OP_SUCCEEDED(PrlSrv_RegisterVm( m_ServerHandle, QSTR2UTF8(sVmHomePath), 0 ))
	CHECK_ASYNC_OP_SUCCEEDED(PrlVm_RefreshConfig( m_VmHandle ))

	QString sSharedVirtId, sHostOnlyVirtId, sBridgedVirtId;
	PRL_EXTRACT_STRING_VALUE(sSharedVirtId, hSharedNet, PrlVmDevNet_GetVirtualNetworkId)
	PRL_EXTRACT_STRING_VALUE(sHostOnlyVirtId, hHostOnlyNet, PrlVmDevNet_GetVirtualNetworkId)
	PRL_EXTRACT_STRING_VALUE(sBridgedVirtId, hBridgedNet, PrlVmDevNet_GetVirtualNetworkId)

	if ( pHostOnlyNet )
	{
		QCOMPARE(pHostOnlyNet->getNetworkID(), sSharedVirtId);
		QCOMPARE(pHostOnlyNet->getNetworkID(), sHostOnlyVirtId);
	}
	else
	{
		CHECK_ADAPTER_DISCONNECTED(hSharedNet)
		CHECK_ADAPTER_DISCONNECTED(hHostOnlyNet)
	}

	if ( pBridgedNet )
	{
		QCOMPARE(pBridgedNet->getNetworkID(), sBridgedVirtId);
	}
	else
	{
		CHECK_ADAPTER_DISCONNECTED(hBridgedNet)
	}
}

void PrlVmManipulationsTest::testCheckAutoStartModeAtRegistrationVm()
{
	INITIALIZE_VM("./TestDspCmdDirValidateVmConfig_vm_config.xml")

	CHECK_RET_CODE_EXP(PrlVmCfg_SetStartLoginMode(m_VmHandle, PLM_ROOT_ACCOUNT));

	m_JobHandle.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	GET_JOB_RET_CODE(m_JobHandle);

	if (nRetCode == PRL_ERR_ONLY_ADMIN_CAN_SET_PARAMETER_STARTLOGINMODE_ROOT)
		// Not an admin user start test
		return;

	QVERIFY(PRL_SUCCEEDED(nRetCode));

	m_JobHandle.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle);

	PRL_VM_START_LOGIN_MODE nAutoMode = PLM_START_ACCOUNT;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetStartLoginMode(m_VmHandle, &nAutoMode));

	if ( TestConfig::isCtMode() )
		QCOMPARE(nAutoMode, PLM_START_ACCOUNT);
	else
		QCOMPARE(nAutoMode, PLM_ROOT_ACCOUNT);
}

void PrlVmManipulationsTest::testIsVmRamHotplugEnabled()
{
	testCreateVmFromConfig();
	PRL_BOOL bRamHotplugEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsRamHotplugEnabled(m_VmHandle, &bRamHotplugEnabled))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bRamHotplugEnabled == PRL_BOOL(_vm_conf.getVmHardwareList()->getMemory()->isEnableHotplug()));
}

void PrlVmManipulationsTest::testSetVmRamHotplugEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bRamHotplugEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsRamHotplugEnabled(hVm, &bRamHotplugEnabled))
	bRamHotplugEnabled = !bRamHotplugEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetRamHotplugEnabled(hVm, bRamHotplugEnabled))
	PRL_BOOL bActualRamHotplugEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsRamHotplugEnabled(hVm, &bActualRamHotplugEnabled))
	QCOMPARE(bActualRamHotplugEnabled, bRamHotplugEnabled);
}

void PrlVmManipulationsTest::testVmRamHotplugEnabledOnWrongParams()
{
	testCreateVmFromConfig();
	PRL_BOOL bRamHotplugEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
		PrlVmCfg_IsRamHotplugEnabled(PRL_INVALID_HANDLE, &bRamHotplugEnabled),
		PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
		PrlVmCfg_IsRamHotplugEnabled(m_ServerHandle, &bRamHotplugEnabled),
		PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
		PrlVmCfg_IsRamHotplugEnabled(m_VmHandle, 0),
		PRL_ERR_INVALID_ARG)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(
		PrlVmCfg_SetRamHotplugEnabled(PRL_INVALID_HANDLE, PRL_FALSE),
		PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
		PrlVmCfg_SetRamHotplugEnabled(m_ServerHandle, PRL_FALSE),
		PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testCreateBackupOnWrongServerHandle()
{
	SdkHandleWrap hHandle;
	CHECK_ASYNC_OP_FAILED(PrlSrv_CreateVmBackup(hHandle, "", "", 0, "", "", 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testCreateBackupOnInvalidServerHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_CreateVmBackup(PRL_INVALID_HANDLE, "", "", 0, "", "", 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testCreateBackupOnNullVmUuid()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_CreateVmBackup(m_ServerHandle, NULL, "", 0, "", "", 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testCreateBackupOnNullTargetHost()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_CreateVmBackup(m_ServerHandle, "", NULL, 0, "", "", 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testCreateBackupOnNullTargetSessionId()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_CreateVmBackup(m_ServerHandle, "", "", 0, NULL, "", 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testRestoreBackupOnWrongServerHandle()
{
	SdkHandleWrap hHandle;
	CHECK_ASYNC_OP_FAILED(PrlSrv_RestoreVmBackup(hHandle, "", "", "", 0, "", "", "", 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testRestoreBackupOnInvalidServerHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_RestoreVmBackup(PRL_INVALID_HANDLE, "", "", "", 0, "", "", "", 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testRestoreBackupOnNullVmUuid()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_RestoreVmBackup(m_ServerHandle, NULL, "", "", 0, "", "", "", 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testRestoreBackupOnNullBackupUuid()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_RestoreVmBackup(m_ServerHandle, "", NULL, "", 0, "", "", "", 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testRestoreBackupOnNullTargetHost()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_RestoreVmBackup(m_ServerHandle, "", "", NULL, 0, "", "", "", 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testRestoreBackupOnNullTargetSessionId()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_RestoreVmBackup(m_ServerHandle, "", "", "", 0, NULL, "", "", 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetBackupTreeOnWrongServerHandle()
{
	SdkHandleWrap hHandle;
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetBackupTree(hHandle, "", "", 0, "", 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetBackupTreeOnInvalidServerHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetBackupTree(PRL_INVALID_HANDLE, "", "", 0, "", 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetBackupTreeOnNullVmUuid()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetBackupTree(m_ServerHandle, NULL, "", 0, "", 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetBackupTreeOnTargetHost()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetBackupTree(m_ServerHandle, "", NULL, 0, "", 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetBackupTreeOnTargetSessionId()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetBackupTree(m_ServerHandle, "", "", 0, NULL, 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

#define CHECK_BACKUP_TREE_EMPTY(backupTree)\
{\
	QDomDocument __doc;\
	QVERIFY(__doc.setContent(backupTree));\
	QDomElement __docElem = __doc.documentElement();\
	QCOMPARE(__docElem.tagName(), QString("BackupTree"));\
	QDomElement __vmItemElem = __docElem.firstChildElement(QString("VmItem"));\
	QVERIFY(__vmItemElem.isNull());\
}

#define CREATE_VM_FOR_BACKUP_TEST(vmUuid)\
{\
	testCreateVmFromConfig();\
	PRL_EXTRACT_STRING_VALUE(vmUuid, m_VmHandle, PrlVmCfg_GetUuid)\
	m_JobHandle.reset(PrlSrv_RemoveVmBackup(m_ServerHandle, QSTR2UTF8(vmUuid),\
										"", "", 0, "", 0, 0, PRL_FALSE));\
	GET_JOB_RET_CODE(m_JobHandle);\
	QVERIFY(PRL_SUCCEEDED(nRetCode) || (nRetCode == PRL_ERR_BACKUP_BACKUP_NOT_FOUND));\
}

#define CREATE_BACKUP(vmUuid, backupId, flags)\
{\
	m_JobHandle.reset(PrlSrv_CreateVmBackup(m_ServerHandle, QSTR2UTF8(vmUuid),\
										"", 0, "", "", flags, 0, PRL_FALSE));\
	CHECK_JOB_RET_CODE(m_JobHandle);\
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()));\
	CHECK_PARAMS_COUNT(m_ResultHandle, 2);\
	SdkHandleWrap __paramHandle;\
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(m_ResultHandle, 1, __paramHandle.GetHandlePtr()));\
	PRL_EXTRACT_STRING_VALUE(backupId, __paramHandle, PrlBackupResult_GetBackupUuid)\
}

#define GET_BACKUP_TREE(backupId, flags, backupTree)\
{\
	m_JobHandle.reset(PrlSrv_GetBackupTree(m_ServerHandle, QSTR2UTF8(backupId), "", 0, "",\
		flags | (TestConfig::isCtMode() ? PBT_CT : PBT_VM), 0, PRL_FALSE));\
	CHECK_JOB_RET_CODE(m_JobHandle);\
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()));\
	PRL_EXTRACT_STRING_VALUE(backupTree, m_ResultHandle, PrlResult_GetParamAsString);\
}

void PrlVmManipulationsTest::verifyBackupTree(const QString &backupTree,
											  const QStringList &backupIds)
{
	QStringList remainingIds(backupIds);
	QDomDocument doc;
	QVERIFY(doc.setContent(backupTree));
	QDomElement docElem = doc.documentElement();
	QCOMPARE(docElem.tagName(), QString("BackupTree"));
	QDomElement vmItemElem = docElem.firstChildElement(QString("VmItem"));
	QVERIFY(!vmItemElem.isNull());
	for (QDomElement backupElem = vmItemElem.firstChildElement("BackupItem");
		!backupElem.isNull();
		backupElem = backupElem.nextSiblingElement("BackupItem"))
	{
		QDomElement idElem = backupElem.firstChildElement(QString("Id"));
		QVERIFY(!idElem.isNull());
		if (!backupIds.contains(idElem.text()))
			QFAIL(QSTR2UTF8(QString("The backup tree contains an unnecessary item with ID %1."
									"\nBackup tree:\n%2").arg(idElem.text()).arg(backupTree)));
		remainingIds.removeOne(idElem.text());
		for (QDomElement partialBackupElem = backupElem.firstChildElement("PartialBackupItem");
			!partialBackupElem.isNull();
			partialBackupElem = partialBackupElem.nextSiblingElement("PartialBackupItem"))
		{
			QDomElement idElem = partialBackupElem.firstChildElement(QString("Id"));
			QVERIFY(!idElem.isNull());
			if (!backupIds.contains(idElem.text()))
				QFAIL(QSTR2UTF8(QString("The backup tree contains an unnecessary item with ID %1."
										"\nBackup tree:\n%2").arg(idElem.text()).arg(backupTree)));
			remainingIds.removeOne(idElem.text());
		}
	}
	if (!remainingIds.isEmpty())
	{
		foreach(QString id, remainingIds)
			QWARN(QSTR2UTF8(QString("The backup tree lacks a necessary item with ID %1.").arg(id)));
		QFAIL(QSTR2UTF8(QString("The expected backup items are not found in the backup tree.\n"
								"Backup tree:\n%1").arg(backupTree)));
	}
}

void PrlVmManipulationsTest::testGetBackupTree()
{
	// 01. Create a VM
	// 02. Create a full backup
	// 03. Get backup tree and check that the full backup info presents in the tree
	// 04. Create an incremental backup
	// 05. Get backup tree and check that the incremental backup info presents in the tree
	///////////////////////////////////////////////////////////////////////////

	QString vmUuid;
	QString fullBackupId, incrementalBackupId;
	QString backupTree;

	CREATE_VM_FOR_BACKUP_TEST(vmUuid)
	CREATE_BACKUP(vmUuid, fullBackupId, PBT_FULL)
	GET_BACKUP_TREE(QString(), 0, backupTree)
	verifyBackupTree(backupTree, QStringList() << fullBackupId);
	CREATE_BACKUP(vmUuid, incrementalBackupId, PBT_INCREMENTAL)
	GET_BACKUP_TREE(QString(), 0, backupTree)
	verifyBackupTree(backupTree, QStringList() << fullBackupId << incrementalBackupId);
}

void PrlVmManipulationsTest::testGetBackupTreeFlagCombinations()
{
	// 01. Create a VM
	// 02. Try to get backup tree with PBT_BACKUP_ID and empty ID
	// 03. Try to get backup tree with PBT_CHAIN and empty ID
	// 04. Try to get backup tree with PBT_BACKUP_ID | PBT_CHAIN
	///////////////////////////////////////////////////////////////////////////

	QString vmUuid;
	QString backupId;

	// 01. Create a VM
	testCreateVmFromConfig();
	PRL_EXTRACT_STRING_VALUE(vmUuid, m_VmHandle, PrlVmCfg_GetUuid)
	// 02. Try to get backup tree with PBT_BACKUP_ID and empty UUID
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetBackupTree(m_ServerHandle, "", "", 0, "",
						  PBT_BACKUP_ID, 0, PRL_FALSE), PRL_ERR_INVALID_PARAM);
	// 03. Try to get backup tree with PBT_CHAIN and empty UUID
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetBackupTree(m_ServerHandle, "", "", 0, "",
						  PBT_CHAIN, 0, PRL_FALSE), PRL_ERR_INVALID_PARAM);
	// 04. Try to get backup tree with PBT_BACKUP_ID | PBT_CHAIN
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetBackupTree(m_ServerHandle, QSTR2UTF8(vmUuid), "", 0, "",
						  PBT_BACKUP_ID | PBT_CHAIN, 0, PRL_FALSE), PRL_ERR_INVALID_PARAM);
}

void PrlVmManipulationsTest::testGetBackupTreeForInexistingBackup()
{
	// 01. Create a VM
	// 02. Create a full backup
	// 03. Create an incremental backup
	// 04. Get backup tree for an inexisting full backup id and check that it's empty
	// 05. Get backup tree for an inexisting full backup chain and check that it's empty
	// 06. Get backup tree for an inexisting incremental backup id and check that it's empty
	// 07. Get backup tree for an inexisting incremental backup chain and check that it's empty
	///////////////////////////////////////////////////////////////////////////

	QString vmUuid;
	QString fullBackupId, incrementalBackupId;
	QString backupTree;
	QString inexistingFullBackupId = Uuid::createUuid().toString();
	QString inexistingIncrementalBackupId = Uuid::createUuid().toString() + ".2";

	CREATE_VM_FOR_BACKUP_TEST(vmUuid)
	CREATE_BACKUP(vmUuid, fullBackupId, PBT_FULL)
	CREATE_BACKUP(vmUuid, incrementalBackupId, PBT_INCREMENTAL)
	GET_BACKUP_TREE(inexistingFullBackupId, PBT_BACKUP_ID, backupTree)
	CHECK_BACKUP_TREE_EMPTY(backupTree)
	GET_BACKUP_TREE(inexistingFullBackupId, PBT_CHAIN, backupTree)
	CHECK_BACKUP_TREE_EMPTY(backupTree)
	GET_BACKUP_TREE(inexistingIncrementalBackupId, PBT_BACKUP_ID, backupTree)
	CHECK_BACKUP_TREE_EMPTY(backupTree)
	GET_BACKUP_TREE(inexistingIncrementalBackupId, PBT_CHAIN, backupTree)
	CHECK_BACKUP_TREE_EMPTY(backupTree)
}

void PrlVmManipulationsTest::testGetBackupTreeForSingleFullBackup()
{
	// 01. Create a VM
	// 02. Create a full backup
	// 03. Create an incremental backup
	// 04. Create another full backup
	// 05. Get backup tree for the first full backup and check that the first
	//     full backup info presents in the tree
	///////////////////////////////////////////////////////////////////////////

	QString vmUuid;
	QString fullBackupId[2], incrementalBackupId[1];
	QString backupTree;

	CREATE_VM_FOR_BACKUP_TEST(vmUuid)
	CREATE_BACKUP(vmUuid, fullBackupId[0], PBT_FULL)
	CREATE_BACKUP(vmUuid, incrementalBackupId[0], PBT_INCREMENTAL)
	CREATE_BACKUP(vmUuid, fullBackupId[1], PBT_FULL)
	GET_BACKUP_TREE(fullBackupId[0], PBT_BACKUP_ID, backupTree)
	verifyBackupTree(backupTree, QStringList() << fullBackupId[0]);
}

void PrlVmManipulationsTest::testGetBackupTreeForSingleIncrementalBackup()
{
	// 01. Create a VM
	// 02. Remove all backups for the VM (they could still exist after previous runs)
	// 03. Create a full backup
	// 04. Create another full backup
	// 05. Create an incremental backup
	// 06. Create another incremental backup
	// 07. Get backup tree for the first incremental backup and check that
	//     the first incremental backup info and second full backup info
	//     present in the tree
	///////////////////////////////////////////////////////////////////////////

	QString vmUuid;
	QString fullBackupId[2], incrementalBackupId[2];
	QString backupTree;

	CREATE_VM_FOR_BACKUP_TEST(vmUuid)
	CREATE_BACKUP(vmUuid, fullBackupId[0], PBT_FULL)
	CREATE_BACKUP(vmUuid, fullBackupId[1], PBT_FULL)
	CREATE_BACKUP(vmUuid, incrementalBackupId[0], PBT_INCREMENTAL)
	CREATE_BACKUP(vmUuid, incrementalBackupId[1], PBT_INCREMENTAL)
	GET_BACKUP_TREE(incrementalBackupId[0], PBT_BACKUP_ID, backupTree)
	verifyBackupTree(backupTree, QStringList() << fullBackupId[1] << incrementalBackupId[0]);
}

void PrlVmManipulationsTest::testGetBackupTreeForFullBackupChain()
{
	// 01. Create a VM
	// 02. Remove all backups for the VM (they could still exist after previous runs)
	// 03. Create a full backup
	// 04. Create an incremental backup
	// 05. Create another full backup
	// 06. Get backup tree for the first full backup chain and check that the first
	//     full and the first incremental backup info present in the tree
	///////////////////////////////////////////////////////////////////////////

	QString vmUuid;
	QString fullBackupId[2], incrementalBackupId[1];
	QString backupTree;

	CREATE_VM_FOR_BACKUP_TEST(vmUuid)
	CREATE_BACKUP(vmUuid, fullBackupId[0], PBT_FULL)
	CREATE_BACKUP(vmUuid, incrementalBackupId[0], PBT_INCREMENTAL)
	CREATE_BACKUP(vmUuid, fullBackupId[1], PBT_FULL)
	GET_BACKUP_TREE(fullBackupId[0], PBT_CHAIN, backupTree)
	verifyBackupTree(backupTree, QStringList() << fullBackupId[0] << incrementalBackupId[0]);
}

void PrlVmManipulationsTest::testGetBackupTreeForIncrementalBackupChain()
{
	// 01. Create a VM
	// 02. Remove all backups for the VM (they could still exist after previous runs)
	// 03. Create a full backup
	// 04. Create another full backup
	// 05. Create an incremental backup
	// 06. Create second incremental backup
	// 07. Create third incremental backup
	// 08. Get backup tree for the second incremental backup chain and check that
	//     the second and the third incremental backup info and the second full
	//     backup info present in the tree
	///////////////////////////////////////////////////////////////////////////

	QString vmUuid;
	QString fullBackupId[2], incrementalBackupId[3];
	QString backupTree;

	CREATE_VM_FOR_BACKUP_TEST(vmUuid)
	CREATE_BACKUP(vmUuid, fullBackupId[0], PBT_FULL)
	CREATE_BACKUP(vmUuid, fullBackupId[1], PBT_FULL)
	CREATE_BACKUP(vmUuid, incrementalBackupId[0], PBT_INCREMENTAL)
	CREATE_BACKUP(vmUuid, incrementalBackupId[1], PBT_INCREMENTAL)
	CREATE_BACKUP(vmUuid, incrementalBackupId[2], PBT_INCREMENTAL)
	GET_BACKUP_TREE(incrementalBackupId[1], PBT_CHAIN, backupTree)
	verifyBackupTree(backupTree, QStringList()
		<< incrementalBackupId[1] << incrementalBackupId[2] << fullBackupId[1]);
}

void PrlVmManipulationsTest::testRemoveBackupOnWrongServerHandle()
{
	SdkHandleWrap hHandle;
	CHECK_ASYNC_OP_FAILED(PrlSrv_RemoveVmBackup(hHandle, "", "", "", 0, "", 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testRemoveBackupOnInvalidServerHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_RemoveVmBackup(PRL_INVALID_HANDLE, "", "", "", 0, "", 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testRemoveBackupOnNullVmUuid()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_RemoveVmBackup(m_ServerHandle, NULL, "", "", 0, "", 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testRemoveBackupOnNullBackupUuid()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_RemoveVmBackup(m_ServerHandle, "", NULL, "", 0, "", 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testRemoveBackupOnNullTargetHost()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_RemoveVmBackup(m_ServerHandle, "", "", NULL, 0, "", 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testRemoveBackupOnNullTargetSessionId()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_RemoveVmBackup(m_ServerHandle, "", "", "", 0, NULL, 0, 0, PRL_FALSE),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testBackupResultOnWrongServerHandle()
{
	SdkHandleWrap hHandle;
	PRL_UINT32 nLength;
	char buf[128];
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlBackupResult_GetBackupUuid(hHandle, buf, &nLength),
				PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testBackupResultOnInvalidServerHandle()
{
	PRL_UINT32 nLength;
	char buf[128];
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlBackupResult_GetBackupUuid(PRL_INVALID_HANDLE, buf, &nLength),\
				PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testBackupResultOnNullBackupUuidBufLength()
{
	char buf[128];
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlBackupResult_GetBackupUuid(m_ServerHandle, buf, NULL),\
				PRL_ERR_INVALID_ARG)
}

#ifdef _MAC_

namespace
{

class TMCleaner
{
public:

	TMCleaner(const QString& path) : m_path(path)
	{
		CTimeMachineHelper::RemovePathFromTimeMachineExcludeList(path);
	}

	~TMCleaner()
	{
		CTimeMachineHelper::RemovePathFromTimeMachineExcludeList(m_path);
	}

private:
	QString m_path;
};

}

#endif

void PrlVmManipulationsTest::testVmWithTimeMachine()
{
#ifdef _MAC_
	if ( ! CTimeMachineHelper::IsTimeMachineApiSupported() )
	{
		QSKIP("No time machine supported !", SkipAll);
		return;
	}

	testCreateVmFromConfig();
	QString qsVmHomePath;
	PRL_EXTRACT_STRING_VALUE(qsVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath);

	TMCleaner tm_cleaner(qsVmHomePath);

	QVERIFY( ! CTimeMachineHelper::IsPathPresentsAtTimeMachineExcludeList(qsVmHomePath) );
	QVERIFY( CTimeMachineHelper::AddPathToTimeMachineExcludeList(qsVmHomePath) );
	QVERIFY( CTimeMachineHelper::IsPathPresentsAtTimeMachineExcludeList(qsVmHomePath) );
	QVERIFY( CTimeMachineHelper::RemovePathFromTimeMachineExcludeList(qsVmHomePath) );
	QVERIFY( ! CTimeMachineHelper::IsPathPresentsAtTimeMachineExcludeList(qsVmHomePath) );

#else
	QSKIP("Time machine is available on Mac only !", SkipAll);
#endif
}

void PrlVmManipulationsTest::testVmGetStatisticsExOnWrongParams()
{
	CHECK_ASYNC_OP_FAILED(PrlVm_GetStatisticsEx(m_ServerHandle, 0),\
							PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testVmDataStatisticOnNoData()
{
	SKIP_TEST_IN_CT_MODE_unimp

	testCreateVmFromConfig();

	m_JobHandle.reset(PrlVm_GetStatisticsEx(m_VmHandle, 0));
	CHECK_JOB_RET_CODE(m_JobHandle);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()));
	SdkHandleWrap hVmStat;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmStat.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hVmStat, PHT_SYSTEM_STATISTICS);

	SdkHandleWrap hVmDataStat;
	CHECK_RET_CODE_EXP(PrlStat_GetVmDataStat(hVmStat, hVmDataStat.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hVmDataStat, PHT_SYSTEM_STATISTICS_VM_DATA);

	PRL_VOID_PTR pBuffer = NULL;
	CHECK_RET_CODE_EXP(PrlHandle_ToString( hVmDataStat, &pBuffer ))
	QString qsVmDataStat( UTF8_2QSTR((PRL_CONST_STR )pBuffer) );
	PrlBuffer_Free( pBuffer );

	CVmDataStatistic vmDataStat;
	CHECK_RET_CODE_EXP(vmDataStat.fromString(qsVmDataStat));
	QVERIFY(vmDataStat.getVmDataSegments()->m_lstVmDataSegments.isEmpty());

	PRL_UINT64	nCapacity = 0;
	for(int i = PDSS_VM_MISCELLANEOUS_SPACE; i <= PDSS_VM_FULL_SPACE; i++)
	{
		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStatVmData_GetSegmentCapacity(
			hVmDataStat, (PRL_DATA_STATISTIC_SEGMENTS )i, &nCapacity),\
				PRL_ERR_NO_DATA);
	}

}

void PrlVmManipulationsTest::testVmDataStatisticOnFilledData()
{
	SKIP_TEST_IN_CT_MODE_unimp

	testCreateVmFromConfig();

	m_JobHandle.reset(PrlVm_GetStatisticsEx(m_VmHandle, PVMSF_HOST_DISK_SPACE_USAGE_ONLY));
	CHECK_JOB_RET_CODE(m_JobHandle);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()));
	SdkHandleWrap hVmStat;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmStat.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hVmStat, PHT_SYSTEM_STATISTICS);

	SdkHandleWrap hVmDataStat;
	CHECK_RET_CODE_EXP(PrlStat_GetVmDataStat(hVmStat, hVmDataStat.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hVmDataStat, PHT_SYSTEM_STATISTICS_VM_DATA);

	PRL_UINT64	nCapacity = 0;
	PRL_UINT64	nTotal = 0;
	PRL_UINT64	nFull = 0;
	for(int i = PDSS_VM_MISCELLANEOUS_SPACE; i <= PDSS_VM_FULL_SPACE; i++)
	{
		CHECK_RET_CODE_EXP(PrlStatVmData_GetSegmentCapacity(
			hVmDataStat, (PRL_DATA_STATISTIC_SEGMENTS )i, &nCapacity));

		if (i == PDSS_VM_FULL_SPACE)
			nFull = nCapacity;
		else
			nTotal += nCapacity;
	}

	QCOMPARE(nTotal, nFull);
}

void PrlVmManipulationsTest::testVmDataStatisticOnWrongParams()
{
	SKIP_TEST_IN_CT_MODE_unimp

	// Wrong handle
	SdkHandleWrap hVmDataStat;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStat_GetVmDataStat(m_ServerHandle, hVmDataStat.GetHandlePtr()),\
			PRL_ERR_INVALID_ARG);

	// Null pointer
	testCreateVmFromConfig();

	m_JobHandle.reset(PrlVm_GetStatistics(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()));
	SdkHandleWrap hVmStat;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmStat.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hVmStat, PHT_SYSTEM_STATISTICS);

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStat_GetVmDataStat(hVmStat, NULL),\
			PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testVmDataSegmentsOnWrongParams()
{
	SKIP_TEST_IN_CT_MODE_unimp

	testCreateVmFromConfig();

	m_JobHandle.reset(PrlVm_GetStatistics(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()));
	SdkHandleWrap hVmStat;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmStat.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hVmStat, PHT_SYSTEM_STATISTICS);

	SdkHandleWrap hVmDataStat;
	CHECK_RET_CODE_EXP(PrlStat_GetVmDataStat(hVmStat, hVmDataStat.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hVmDataStat, PHT_SYSTEM_STATISTICS_VM_DATA);

	PRL_UINT64	nCapacity = 0;
	// Incorrect segment data enum values
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStatVmData_GetSegmentCapacity(hVmDataStat, PDSS_UNKNOWN, &nCapacity),\
			PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStatVmData_GetSegmentCapacity(\
		hVmDataStat, (PRL_DATA_STATISTIC_SEGMENTS )(1 + PDSS_VM_FULL_SPACE), &nCapacity),\
			PRL_ERR_INVALID_ARG);

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStatVmData_GetSegmentCapacity(m_ServerHandle, PDSS_VM_FULL_SPACE, &nCapacity),\
			PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStatVmData_GetSegmentCapacity(hVmDataStat, PDSS_VM_MISCELLANEOUS_SPACE, NULL),\
			PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testMove()
{
	testCreateVmFromConfig();

	const char *sNewHome = "/tmp/test_move_home";
	SdkHandleWrap hJob(PrlVm_Move(m_VmHandle, sNewHome, 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	m_MovedVmHandle.reset();

	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_MovedVmHandle.GetHandlePtr()))

	QVERIFY(m_MovedVmHandle != PRL_INVALID_HANDLE);
	CHECK_HANDLE_TYPE(m_MovedVmHandle, PHT_VIRTUAL_MACHINE)

	QString sHome;
	PRL_EXTRACT_STRING_VALUE(sHome, m_MovedVmHandle, PrlVmCfg_GetHomePath)
	QCOMPARE(sHome, UTF8_2QSTR(sNewHome));
}

void PrlVmManipulationsTest::testMoveOnWrongParams()
{
	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlVm_Move(PRL_INVALID_HANDLE, NEW_VM_PATH, 0), PRL_ERR_INVALID_ARG)

	// Wrong path
	QVERIFY(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()) == PRL_ERR_SUCCESS);
	CHECK_ASYNC_OP_FAILED(PrlVm_Move(m_VmHandle, NULL, 0), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlVm_Move(m_VmHandle, "", 0), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::checkGetVmConfigValid(QString sVmName, quint32 nFlags,
												QString sOrigName, QString sOrigUuid)
{
	SdkHandleWrap hJob(PrlSrv_GetVmConfig(m_ServerHandle, QSTR2UTF8(sVmName), nFlags));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap foundHandle;

	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, foundHandle.GetHandlePtr()))
	QVERIFY(foundHandle != PRL_INVALID_HANDLE);
	CHECK_HANDLE_TYPE(foundHandle, PHT_VIRTUAL_MACHINE)

	QString sVmName2;
	PRL_EXTRACT_STRING_VALUE(sVmName2, foundHandle, PrlVmCfg_GetName)
	QString sVmUuid2;
	PRL_EXTRACT_STRING_VALUE(sVmUuid2, foundHandle, PrlVmCfg_GetUuid)

	QVERIFY(sOrigName == sVmName2);
	QVERIFY(sOrigUuid == sVmUuid2);
}

void PrlVmManipulationsTest::getDefaultOsTemplate(QString &sTemplate)
{
	#ifndef _LIN_
		return;
	#endif

	QFile vz_conf("/etc/sysconfig/vz");
	QVERIFY( vz_conf.open(QFile::ReadOnly) );

	QRegExp rx("(^DEF_OSTEMPLATE=\".)([^\"]*)");

	while (!vz_conf.atEnd())
	{
		QByteArray line = vz_conf.readLine();

		if (rx.indexIn(line) != (-1))
		{
			sTemplate = rx.cap(2);
			break;
		}
	}

	vz_conf.close();
}

void PrlVmManipulationsTest::createCacheForDefaultTemplate()
{
	#ifndef _LIN_
		return;
	#endif

	SdkHandleWrap hJob(PrlSrv_GetCtTemplateList(m_ServerHandle, 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	PRL_UINT32 nParamsCount;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))
	QVERIFY(nParamsCount > 0);

	QString sDefaultTemplateName;
	getDefaultOsTemplate(sDefaultTemplateName);
	QVERIFY( !sDefaultTemplateName.isEmpty() );

	for (PRL_UINT32 i = 0; i < nParamsCount; ++i)
	{
		SdkHandleWrap hCtTmplInfo;
		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, i, hCtTmplInfo.GetHandlePtr()));
		CHECK_HANDLE_TYPE(hCtTmplInfo, PHT_CT_TEMPLATE);

		QString sTemplateName;
		PRL_EXTRACT_STRING_VALUE(sTemplateName, hCtTmplInfo, PrlCtTemplate_GetName)
		if (sTemplateName != sDefaultTemplateName)
			continue;

		PRL_BOOL bCached;
		CHECK_RET_CODE_EXP(PrlCtTemplate_IsCached(hCtTmplInfo, &bCached))

		if (!bCached)
		{
			QString sCmd = "vzpkg create cache " + sTemplateName;
			QVERIFY( system( QSTR2UTF8(sCmd) ) == 0 );
		}

		break;
	}
}

void PrlVmManipulationsTest::testGetVmConfig()
{
	QVERIFY_EXPRESSION(testCreateVmFromConfig());
	QString sVmName;
	PRL_EXTRACT_STRING_VALUE(sVmName, m_VmHandle, PrlVmCfg_GetName)
	QString sVmUuid;
	PRL_EXTRACT_STRING_VALUE(sVmUuid, m_VmHandle, PrlVmCfg_GetUuid)

	WRITE_TRACE(DBG_FATAL, "name=%s", QSTR2UTF8(sVmName));
	WRITE_TRACE(DBG_FATAL, "uuid=%s", QSTR2UTF8(sVmUuid));

	QVERIFY_EXPRESSION(checkGetVmConfigValid(sVmName, PGVC_SEARCH_BY_NAME, sVmName, sVmUuid));
	QVERIFY_EXPRESSION(checkGetVmConfigValid(sVmName, PGVC_SEARCH_BY_NAME|PGVC_SEARCH_BY_UUID, sVmName, sVmUuid));
	QVERIFY_EXPRESSION(checkGetVmConfigValid(sVmUuid, 0, sVmName, sVmUuid));
	QVERIFY_EXPRESSION(checkGetVmConfigValid(sVmUuid, PGVC_SEARCH_BY_UUID, sVmName, sVmUuid));
	QVERIFY_EXPRESSION(checkGetVmConfigValid(sVmUuid, PGVC_SEARCH_BY_NAME|PGVC_SEARCH_BY_UUID, sVmName, sVmUuid));
}

void PrlVmManipulationsTest::testGetVmConfigOnWrongParams()
{
	QVERIFY_EXPRESSION(testCreateVmFromConfig());
	QString sVmName;
	PRL_EXTRACT_STRING_VALUE(sVmName, m_VmHandle, PrlVmCfg_GetName)
	QString sVmUuid;
	PRL_EXTRACT_STRING_VALUE(sVmUuid, m_VmHandle, PrlVmCfg_GetUuid)
	QString newUuid = Uuid::createUuid().toString();

	WRITE_TRACE(DBG_FATAL, "name=%s", QSTR2UTF8(sVmName));
	WRITE_TRACE(DBG_FATAL, "uuid=%s", QSTR2UTF8(sVmUuid));

	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetVmConfig(PRL_INVALID_HANDLE, "", 0), PRL_ERR_INVALID_ARG)

	// Not approriate flag
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetVmConfig(m_ServerHandle, QSTR2UTF8(sVmName),
					PGVC_SEARCH_BY_UUID), PRL_ERR_VM_UUID_NOT_FOUND)
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetVmConfig(m_ServerHandle, QSTR2UTF8(sVmUuid),
					PGVC_SEARCH_BY_NAME), PRL_ERR_VM_UUID_NOT_FOUND)

	// Unexistent name or uuid
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetVmConfig(m_ServerHandle, QSTR2UTF8(sVmName + "test"),
					PGVC_SEARCH_BY_NAME), PRL_ERR_VM_UUID_NOT_FOUND)
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetVmConfig(m_ServerHandle, QSTR2UTF8(newUuid),
					PGVC_SEARCH_BY_UUID), PRL_ERR_VM_UUID_NOT_FOUND)
}


void PrlVmManipulationsTest::testIsEfiEnabled()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();
	PRL_BOOL bEfiEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsEfiEnabled(m_VmHandle, &bEfiEnabled))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bEfiEnabled
		== PRL_BOOL(_vm_conf.getVmSettings()->getVmStartupOptions()->getBios()->isEfiEnabled()));
}

void PrlVmManipulationsTest::testSetEfiEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bEfiEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsEfiEnabled(hVm, &bEfiEnabled))
	bEfiEnabled = !bEfiEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetEfiEnabled(hVm, bEfiEnabled))
	PRL_BOOL bActualEfiEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsEfiEnabled(hVm, &bActualEfiEnabled))
	QCOMPARE(bActualEfiEnabled, bEfiEnabled);
}

void PrlVmManipulationsTest::testEfiEnabledOnWrongParams()
{
	SKIP_TEST_IN_CT_MODE

	testCreateVmFromConfig();
	PRL_BOOL bEfiEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsEfiEnabled(PRL_INVALID_HANDLE, &bEfiEnabled), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsEfiEnabled(m_ServerHandle, &bEfiEnabled), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsEfiEnabled(m_VmHandle, 0), PRL_ERR_INVALID_ARG)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetEfiEnabled(PRL_INVALID_HANDLE, PRL_FALSE), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetEfiEnabled(m_ServerHandle, PRL_FALSE), PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testGetExternalBootDevice()
{
	testCreateVmFromConfig();

	QString sExternalBootDevice = "some text";
	PRL_EXTRACT_STRING_VALUE(sExternalBootDevice, m_VmHandle, PrlVmCfg_GetExternalBootDevice)
	INIT_VM_CONF_FROM_FILE;
	QCOMPARE(sExternalBootDevice,
			 _vm_conf.getVmSettings()->getVmStartupOptions()->getExternalDeviceSystemName());
}

void PrlVmManipulationsTest::testGetExternalBootDeviceOnWrongParams()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetExternalBootDevice(m_VmHandle, "some text"));

	// Invalid handle
	PRL_UINT32 nBufSize = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetExternalBootDevice(m_ServerHandle, 0, &nBufSize),
		PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetExternalBootDevice(m_VmHandle, 0, 0),
		PRL_ERR_INVALID_ARG);
	// Null buffer size
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(m_VmHandle, PrlVmCfg_GetExternalBootDevice);
	// Not enough buffer size
	PRL_CHAR sExternalBootDevice[3];
	PRL_UINT32 nLength = 3;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetExternalBootDevice(m_VmHandle, sExternalBootDevice, &nLength),
		PRL_ERR_BUFFER_OVERRUN);
}

void PrlVmManipulationsTest::testSetExternalBootDevice()
{
	testCreateVmFromConfig();

	QString sNewExternalBootDevice = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP(PrlVmCfg_SetExternalBootDevice(m_VmHandle, QSTR2UTF8(sNewExternalBootDevice)));
	QString sExternalBootDevice;
	PRL_EXTRACT_STRING_VALUE(sExternalBootDevice, m_VmHandle, PrlVmCfg_GetExternalBootDevice);
	QCOMPARE(sExternalBootDevice, sNewExternalBootDevice);
}

void PrlVmManipulationsTest::testSetExternalBootDeviceOnWrongParams()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	// Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
		PrlVmCfg_SetExternalBootDevice(m_ServerHandle, "some text"),
		PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testIsHighAvailabilityEnabled()
{
	testCreateVmFromConfig();
	PRL_BOOL bHAEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsHighAvailabilityEnabled(m_VmHandle, &bHAEnabled))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bHAEnabled
		== PRL_BOOL(_vm_conf.getVmSettings()->getHighAvailability()->isEnabled()));
}

void PrlVmManipulationsTest::testSetHighAvailabilityEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bHAEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsHighAvailabilityEnabled(hVm, &bHAEnabled))
	bHAEnabled = !bHAEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetHighAvailabilityEnabled(hVm, bHAEnabled))
	PRL_BOOL bActualHAEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsHighAvailabilityEnabled(hVm, &bActualHAEnabled))
	QCOMPARE(bActualHAEnabled, bHAEnabled);
}

void PrlVmManipulationsTest::testHighAvailabilityEnabledOnWrongParams()
{
	testCreateVmFromConfig();
	PRL_BOOL bHAEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsHighAvailabilityEnabled(PRL_INVALID_HANDLE, &bHAEnabled),
									   PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsHighAvailabilityEnabled(m_ServerHandle, &bHAEnabled),
									   PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsHighAvailabilityEnabled(m_VmHandle, 0),
									   PRL_ERR_INVALID_ARG)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetHighAvailabilityEnabled(PRL_INVALID_HANDLE, PRL_FALSE),
									   PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetHighAvailabilityEnabled(m_ServerHandle, PRL_FALSE),
									   PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testHighAvailabilityPriority()
{
	VM_CONFIG_INIT;

	VM_CONFIG_TO_XML_OBJECT;
	_vm_conf.getVmSettings()->getHighAvailability()->setPriority(600);
	VM_CONFIG_FROM_XML_OBJECT;

	PRL_UINT32 nPriority = 300;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHighAvailabilityPriority(m_VmHandle, &nPriority));
	QVERIFY(nPriority == 600);

	PRL_UINT32 nPriorityExpected = 450;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetHighAvailabilityPriority(m_VmHandle, nPriorityExpected));
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHighAvailabilityPriority(m_VmHandle, &nPriority));
	QCOMPARE(nPriority, nPriorityExpected);
}

void PrlVmManipulationsTest::testHighAvailabilityPriorityOnWrongParams()
{
	VM_CONFIG_INIT;

	PRL_UINT32 nPriority;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetHighAvailabilityPriority(PRL_INVALID_HANDLE, &nPriority),
		PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetHighAvailabilityPriority(m_ServerHandle, &nPriority),
		PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetHighAvailabilityPriority(m_VmHandle, NULL),
		PRL_ERR_INVALID_ARG);

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetHighAvailabilityPriority(PRL_INVALID_HANDLE, nPriority),
		PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetHighAvailabilityPriority(m_ServerHandle, nPriority),
		PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testIsVerticalSynchronizationEnabled()
{
	testCreateVmFromConfig();
	PRL_BOOL bVSEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsVerticalSynchronizationEnabled(m_VmHandle, &bVSEnabled))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bVSEnabled
		== PRL_BOOL(_vm_conf.getVmHardwareList()->getVideo()->isEnableVSync()));
}

void PrlVmManipulationsTest::testSetVerticalSynchronizationEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bVSEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsVerticalSynchronizationEnabled(hVm, &bVSEnabled))
	bVSEnabled = !bVSEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetVerticalSynchronizationEnabled(hVm, bVSEnabled))
	PRL_BOOL bActualVSEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsVerticalSynchronizationEnabled(hVm, &bActualVSEnabled))
	QCOMPARE(bActualVSEnabled, bVSEnabled);
}

void PrlVmManipulationsTest::testVerticalSynchronizationEnabledOnWrongParams()
{
	testCreateVmFromConfig();
	PRL_BOOL bVSEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsVerticalSynchronizationEnabled(PRL_INVALID_HANDLE, &bVSEnabled),
									   PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsVerticalSynchronizationEnabled(m_ServerHandle, &bVSEnabled),
									   PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsVerticalSynchronizationEnabled(m_VmHandle, 0),
									   PRL_ERR_INVALID_ARG)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetVerticalSynchronizationEnabled(PRL_INVALID_HANDLE, PRL_FALSE),
									   PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetVerticalSynchronizationEnabled(m_ServerHandle, PRL_FALSE),
									   PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testIsHighResolutionEnabled()
{
	testCreateVmFromConfig();
	PRL_BOOL bEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsHighResolutionEnabled(m_VmHandle, &bEnabled))
	INIT_VM_CONF_FROM_FILE
	QVERIFY(bEnabled
		== PRL_BOOL(_vm_conf.getVmHardwareList()->getVideo()->isEnableHiResDrawing()));
}

void PrlVmManipulationsTest::testSetHighResolutionEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsHighResolutionEnabled(hVm, &bEnabled))
	bEnabled = !bEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetHighResolutionEnabled(hVm, bEnabled))
	PRL_BOOL bActualEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsHighResolutionEnabled(hVm, &bActualEnabled))
	QCOMPARE(bActualEnabled, bEnabled);
}

void PrlVmManipulationsTest::testHighResolutionEnabledOnWrongParams()
{
	testCreateVmFromConfig();
	PRL_BOOL bEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsHighResolutionEnabled(PRL_INVALID_HANDLE, &bEnabled),
									   PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsHighResolutionEnabled(m_ServerHandle, &bEnabled),
									   PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsHighResolutionEnabled(m_VmHandle, 0),
									   PRL_ERR_INVALID_ARG)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetHighResolutionEnabled(PRL_INVALID_HANDLE, PRL_FALSE),
									   PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetHighResolutionEnabled(m_ServerHandle, PRL_FALSE),
									   PRL_ERR_INVALID_ARG)
}

void PrlVmManipulationsTest::testSetAdaptiveHypervisorEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bAdaptiveHypervisorEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsAdaptiveHypervisorEnabled(hVm, &bAdaptiveHypervisorEnabled))
	bAdaptiveHypervisorEnabled = !bAdaptiveHypervisorEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetAdaptiveHypervisorEnabled(hVm, bAdaptiveHypervisorEnabled))
	PRL_BOOL bActualAdaptiveHypervisorEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsAdaptiveHypervisorEnabled(hVm, &bActualAdaptiveHypervisorEnabled))
	QCOMPARE(bActualAdaptiveHypervisorEnabled, bAdaptiveHypervisorEnabled);
}

void PrlVmManipulationsTest::testAdaptiveHypervisorEnabledOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bAdaptiveHypervisorEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsAdaptiveHypervisorEnabled(m_ServerHandle, &bAdaptiveHypervisorEnabled),\
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetAdaptiveHypervisorEnabled(m_ServerHandle, PRL_FALSE),\
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsAdaptiveHypervisorEnabled(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testSetSwitchOffWindowsLogoEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bSwitchOffWindowsLogoEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsSwitchOffWindowsLogoEnabled(hVm, &bSwitchOffWindowsLogoEnabled))
	bSwitchOffWindowsLogoEnabled = !bSwitchOffWindowsLogoEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetSwitchOffWindowsLogoEnabled(hVm, bSwitchOffWindowsLogoEnabled))
	PRL_BOOL bActualSwitchOffWindowsLogoEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsSwitchOffWindowsLogoEnabled(hVm, &bActualSwitchOffWindowsLogoEnabled))
	QCOMPARE(bActualSwitchOffWindowsLogoEnabled, bSwitchOffWindowsLogoEnabled);
}

void PrlVmManipulationsTest::testSwitchOffWindowsLogoEnabledOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bSwitchOffWindowsLogoEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsSwitchOffWindowsLogoEnabled(m_ServerHandle, &bSwitchOffWindowsLogoEnabled),\
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetSwitchOffWindowsLogoEnabled(m_ServerHandle, PRL_FALSE),\
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsSwitchOffWindowsLogoEnabled(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testSetLongerBatteryLifeEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bLongerBatteryLifeEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsLongerBatteryLifeEnabled(hVm, &bLongerBatteryLifeEnabled))
	bLongerBatteryLifeEnabled = !bLongerBatteryLifeEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetLongerBatteryLifeEnabled(hVm, bLongerBatteryLifeEnabled))
	PRL_BOOL bActualLongerBatteryLifeEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsLongerBatteryLifeEnabled(hVm, &bActualLongerBatteryLifeEnabled))
	QCOMPARE(bActualLongerBatteryLifeEnabled, bLongerBatteryLifeEnabled);
}

void PrlVmManipulationsTest::testLongerBatteryLifeEnabledOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bLongerBatteryLifeEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsLongerBatteryLifeEnabled(m_ServerHandle, &bLongerBatteryLifeEnabled),\
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetLongerBatteryLifeEnabled(m_ServerHandle, PRL_FALSE),\
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsLongerBatteryLifeEnabled(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testSetBatteryStatusEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bBatteryStatusEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsBatteryStatusEnabled(hVm, &bBatteryStatusEnabled))
	bBatteryStatusEnabled = !bBatteryStatusEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetBatteryStatusEnabled(hVm, bBatteryStatusEnabled))
	PRL_BOOL bActualBatteryStatusEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsBatteryStatusEnabled(hVm, &bActualBatteryStatusEnabled))
	QCOMPARE(bActualBatteryStatusEnabled, bBatteryStatusEnabled);
}

void PrlVmManipulationsTest::testBatteryStatusEnabledOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bBatteryStatusEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsBatteryStatusEnabled(m_ServerHandle, &bBatteryStatusEnabled),\
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetBatteryStatusEnabled(m_ServerHandle, PRL_FALSE),\
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsBatteryStatusEnabled(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testSetNestedVirtualizationEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bNestedVirtualizationEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsNestedVirtualizationEnabled(hVm, &bNestedVirtualizationEnabled))
	bNestedVirtualizationEnabled = !bNestedVirtualizationEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetNestedVirtualizationEnabled(hVm, bNestedVirtualizationEnabled))
	PRL_BOOL bActualNestedVirtualizationEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsNestedVirtualizationEnabled(hVm, &bActualNestedVirtualizationEnabled))
	QCOMPARE(bActualNestedVirtualizationEnabled, bNestedVirtualizationEnabled);
}

void PrlVmManipulationsTest::testNestedVirtualizationEnabledOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bNestedVirtualizationEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsNestedVirtualizationEnabled(m_ServerHandle, &bNestedVirtualizationEnabled),\
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetNestedVirtualizationEnabled(m_ServerHandle, PRL_FALSE),\
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsNestedVirtualizationEnabled(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testSetPMUVirtualizationEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bPMUVirtualizationEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsPMUVirtualizationEnabled(hVm, &bPMUVirtualizationEnabled))
	bPMUVirtualizationEnabled = !bPMUVirtualizationEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetPMUVirtualizationEnabled(hVm, bPMUVirtualizationEnabled))
	PRL_BOOL bActualPMUVirtualizationEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsPMUVirtualizationEnabled(hVm, &bActualPMUVirtualizationEnabled))
	QCOMPARE(bActualPMUVirtualizationEnabled, bPMUVirtualizationEnabled);
}

void PrlVmManipulationsTest::testPMUVirtualizationEnabledOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bPMUVirtualizationEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsPMUVirtualizationEnabled(m_ServerHandle, &bPMUVirtualizationEnabled),\
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetPMUVirtualizationEnabled(m_ServerHandle, PRL_FALSE),\
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsPMUVirtualizationEnabled(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testSetLockGuestOnSuspendEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bLockGuestOnSuspendEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsLockGuestOnSuspendEnabled(hVm, &bLockGuestOnSuspendEnabled))
	bLockGuestOnSuspendEnabled = !bLockGuestOnSuspendEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetLockGuestOnSuspendEnabled(hVm, bLockGuestOnSuspendEnabled))
	PRL_BOOL bActualLockGuestOnSuspendEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsLockGuestOnSuspendEnabled(hVm, &bActualLockGuestOnSuspendEnabled))
	QCOMPARE(bActualLockGuestOnSuspendEnabled, bLockGuestOnSuspendEnabled);
}

void PrlVmManipulationsTest::testLockGuestOnSuspendEnabledOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bLockGuestOnSuspendEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsLockGuestOnSuspendEnabled(m_ServerHandle, &bLockGuestOnSuspendEnabled),\
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetLockGuestOnSuspendEnabled(m_ServerHandle, PRL_FALSE),\
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsLockGuestOnSuspendEnabled(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

void PrlVmManipulationsTest::testSetIsolatedVmEnabled()
{
	CREATE_TEST_VM
	PRL_BOOL bIsolatedVmEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsIsolatedVmEnabled(hVm, &bIsolatedVmEnabled))
	bIsolatedVmEnabled = !bIsolatedVmEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_SetIsolatedVmEnabled(hVm, bIsolatedVmEnabled))
	PRL_BOOL bActualIsolatedVmEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsIsolatedVmEnabled(hVm, &bActualIsolatedVmEnabled))
	QCOMPARE(bActualIsolatedVmEnabled, bIsolatedVmEnabled);
}

void PrlVmManipulationsTest::testIsolatedVmEnabledOnWrongParams()
{
	testCreateVmFromConfig();

	// Wrong handle
	PRL_BOOL bIsolatedVmEnabled;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsIsolatedVmEnabled(m_ServerHandle, &bIsolatedVmEnabled),\
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetIsolatedVmEnabled(m_ServerHandle, PRL_FALSE),\
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsIsolatedVmEnabled(m_VmHandle, NULL), PRL_ERR_INVALID_ARG);
}

