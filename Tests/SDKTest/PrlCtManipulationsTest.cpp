/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2006-2017, Parallels International GmbH
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
/// @file
///		DispCtFunctionalityTest.cpp
///
/// @author
///		sergeyt
///
/// @brief
///		Test dispatcher common functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////

#include "PrlCtManipulationsTest.h"
#include "SimpleServerWrapper.h"

#include "Tests/CommonTestsUtils.h"
#include "Tests/SDKTest/AutoHelpers.h"
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlcommon/PrlUuid/Uuid.h>
#include <prlcommon/Std/SmartPtr.h>
#include <prlcommon/Std/PrlAssert.h>
#include <prlcommon/Messaging/CVmEvent.h>
#include <prlcommon/HostUtils/HostUtils.h>
#include <prlcommon/Std/PrlTime.h>
#include <prlcommon/Std/SmartPtr.h>
#include "Libraries/PrlCommonUtils/CAuthHelper.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/NonQtUtils/CQuestionHelper.h"

#include <prlxmlmodel/DispConfig/CDispCommonPreferences.h>

#include <prlsdk/PrlApiDeprecated.h>
#include <prlcommon/Interfaces/ParallelsSdkPrivate.h>



class AutoDirRemover;
class AutoRestorer;


#define EXTRACT_CONFIG_AS_XML_MODEL_OBJECT(hVm, pOutVmConfig)\
{ \
	PRL_VOID_PTR pVmConfig = 0;\
	QVERIFY(PRL_SUCCEEDED(PrlVm_ToString(hVm, &pVmConfig)));\
	QString sVmConfig = UTF8_2QSTR((const char *)pVmConfig);\
	PrlBuffer_Free(pVmConfig);\
	pOutVmConfig->fromString(sVmConfig);\
	CHECK_RET_CODE_EXP(pOutVmConfig->m_uiRcInit) \
}


#define EXTRACT_HANDLE_AS_XML_MODEL_OBJECT( _hVmInfo, _outXmlObj ) \
{ \
	PRL_VOID_PTR pXml = 0; \
	CHECK_RET_CODE_EXP(PrlHandle_ToString(_hVmInfo, &pXml)); \
	CHECK_RET_CODE_EXP( _outXmlObj.fromString( UTF8_2QSTR((PRL_CONST_STR )pXml) ) ); \
	PrlBuffer_Free(pXml); \
}

#define FILL_DEFAULT_VM_FOLDER(oldVmRootDir, hUserProfile) \
{ \
	char buff[2048]; \
	PRL_UINT32 szBuff=sizeof(buff); \
	QVERIFY( PRL_SUCCEEDED(PrlUsrCfg_GetDefaultVmFolder( hUserProfile, buff, &szBuff ) ) ); \
	oldVmRootDir = UTF8_2QSTR( buff );	\
}

#define PRL_CHECK_STRINGS_LIST(handle, extract_list_method, expected_list_values)\
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
                        QCOMPARE(sItem, expected_list_values.at(i));\
                }\
        }

//////////////////////////////////////////////////////////////////////////
PrlCtManipulationsTest::PrlCtManipulationsTest()
{

        m_ServerHandle.reset();
		m_JobHandle.reset();
		m_hVm.reset();
}

void PrlCtManipulationsTest::initTestCase()
{
	test_login();
}

void PrlCtManipulationsTest::cleanupTestCase()
{
	WRITE_TRACE(DBG_FATAL, "Cleanup All!");
	if (m_hVm != PRL_INVALID_HANDLE)
		m_lstVmHandles += SdkHandleWrap(m_hVm);

	cleanup();

	m_JobHandle.reset(PrlSrv_Logoff(m_ServerHandle));
	PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
	m_ServerHandle.reset();
}

void PrlCtManipulationsTest::init()
{
}

void PrlCtManipulationsTest::cleanup()
{
	if (!m_lstVmHandles.isEmpty())
	{
		// create list to delete for sdk
		for(int i = 0; i < m_lstVmHandles.size(); ++i)
		{
			if (m_lstVmHandles[i] != PRL_INVALID_HANDLE)
			{

				m_JobHandle.reset(PrlVm_StopEx(m_lstVmHandles[i], PSM_KILL, 0));
				PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
				m_JobHandle.reset(PrlVm_Delete(m_lstVmHandles[i], PRL_INVALID_HANDLE));
				PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
			}
		}
		m_lstVmHandles.clear();
	}
	if (m_hVm != PRL_INVALID_HANDLE)
	{
		m_JobHandle.reset(PrlVm_StopEx(m_hVm, PSM_KILL, 0));
		PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
	}
	m_JobHandle.reset();
}

void PrlCtManipulationsTest::test_login()
{
	CHECK_RET_CODE_EXP(PrlSrv_Create(m_ServerHandle.GetHandlePtr()));
        QVERIFY(PrlSrv_Create(m_ServerHandle.GetHandlePtr()) == PRL_ERR_SUCCESS);
	m_JobHandle.reset(PrlSrv_LoginLocal(m_ServerHandle, "" , 0, PSL_HIGH_SECURITY));
	QVERIFY(m_JobHandle != PRL_INVALID_HANDLE);
	PRL_HANDLE_TYPE _handle_type;
	QVERIFY(PrlHandle_GetType(m_JobHandle, &_handle_type) == PRL_ERR_SUCCESS);
	QVERIFY(_handle_type == PHT_JOB);
	CHECK_JOB_RET_CODE(m_JobHandle);

	m_JobHandle.reset(PrlSrv_SetNonInteractiveSession(m_ServerHandle, PRL_TRUE, 0));
	CHECK_JOB_RET_CODE(m_JobHandle)

	//////////////////////////////////////////////////////////////////////////
	// get server uuid
	SdkHandleWrap hServerInfo;
	CHECK_RET_CODE_EXP( PrlSrv_GetServerInfo( m_ServerHandle, hServerInfo.GetHandlePtr() ) );
	QVERIFY( hServerInfo.valid() );

	char buff[STR_BUF_LENGTH];
	PRL_UINT32 nLen = sizeof( buff );
	CHECK_RET_CODE_EXP( PrlSrvInfo_GetServerUuid(hServerInfo, buff, &nLen) );
	m_sServerUuid = buff;

	GET_SRV_CONFIG( m_ServerHandle, m_hSrvConfig );
}

void PrlCtManipulationsTest::getCachedOstemplate(QString &sOstemplate)
{
	static QString _sOstemplate;

	if (!_sOstemplate.isEmpty())
	{
		sOstemplate = _sOstemplate;
		return;
	}

	SdkHandleWrap hJob(PrlSrv_GetCtTemplateList(m_ServerHandle, 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	PRL_UINT32 nParamsCount;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))
	QVERIFY(nParamsCount > 0);

	for (PRL_UINT32 i = 0; i < nParamsCount; ++i)
	{
		SdkHandleWrap hCtTmplInfo;
		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, i, hCtTmplInfo.GetHandlePtr()));
		CHECK_HANDLE_TYPE(hCtTmplInfo, PHT_CT_TEMPLATE);

		PRL_BOOL bCached;
		CHECK_RET_CODE_EXP(PrlCtTemplate_IsCached(hCtTmplInfo, &bCached))
		if (!bCached)
			continue;

                PRL_CHAR sName[STR_BUF_LENGTH];
                PRL_UINT32 nNameBufLength = STR_BUF_LENGTH;
                CHECK_RET_CODE_EXP(PrlCtTemplate_GetName(hCtTmplInfo, sName, &nNameBufLength))

		_sOstemplate = sName;
		break;
	}
	sOstemplate = _sOstemplate;
}

void PrlCtManipulationsTest::createVm(const QString& sVmName,
		bool& bRes,
		SdkHandleWrap& hVm)
{
	bRes = false;
        SdkHandleWrap hJob;

	WRITE_TRACE( DBG_FATAL, "Create Vm: %s", QSTR2UTF8(sVmName));
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(hVm, m_hSrvConfig, PVS_GUEST_VER_WIN_XP, PRL_TRUE));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(hVm, QSTR2UTF8(sVmName)));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetRamSize(hVm, 256));

        hJob.reset(PrlVm_Reg(hVm, "", PRL_TRUE));
        CHECK_JOB_RET_CODE(hJob);

	bRes = true;
}

void PrlCtManipulationsTest::createCt(const QString& sVmName
	, const QString &sSample
	, bool& bRes /*out*/
	, SdkHandleWrap& hVm/*out*/)
{
	bRes = false;
	SdkHandleWrap hJob;

	QString sOsTemplate;
	getCachedOstemplate(sOsTemplate);

	WRITE_TRACE( DBG_FATAL, "Create Container: %s %s",
			QSTR2UTF8(sVmName), QSTR2UTF8(sOsTemplate));

	if (sSample.isEmpty()) {
		CHECK_RET_CODE_EXP( PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()) );
	} else {
		PRL_GET_VM_CONFIG_PARAM_DATA param;

		param.nVmType = PVT_CT;
		param.sConfigSample = "";
		param.nOsVersion = 0;

		SdkHandleWrap hResult;
		hJob.reset(PrlSrv_GetDefaultVmConfig(m_ServerHandle, &param, 0));
		CHECK_JOB_RET_CODE(hJob);
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
		CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVm.GetHandlePtr()))
	}
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(hVm, QSTR2UTF8(sVmName) ) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetVmType(hVm, PVT_CT ) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetOsTemplate(hVm, QSTR2UTF8(sOsTemplate)) );
	//CHECK_RET_CODE_EXP( PrlVmCfg_SetRamSize(hVm, 260 ) );

	QString sHomePath, sVmUuid;

	PRL_EXTRACT_STRING_VALUE(sVmUuid, hVm, PrlVmCfg_GetUuid);

	hJob.reset(PrlVm_Reg(hVm, "", PRL_TRUE));
	PRL_RESULT nRetCode = PrlJob_Wait(hJob, 10*PRL_JOB_WAIT_TIMEOUT);
	PrlJob_GetRetCode(hJob, &nRetCode);
	if (PRL_FAILED(nRetCode)) {
		WRITE_TRACE(DBG_FATAL, "_ret_code=%.8X '%s'",
			 nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		return;
	}

	PRL_EXTRACT_STRING_VALUE(sVmUuid, hVm, PrlVmCfg_GetUuid);
	PRL_EXTRACT_STRING_VALUE(sHomePath, hVm, PrlVmCfg_GetHomePath);
	WRITE_TRACE( DBG_FATAL, "after create Uuid=%s name=%s home=%s",
			QSTR2UTF8(sVmUuid),
			QSTR2UTF8(sVmName),
			QSTR2UTF8(sHomePath) );

	bRes = true;
}

void PrlCtManipulationsTest::testCreateCtFromSample()
{
	SdkHandleWrap hJob;
	SdkHandleWrap hResult;
	SdkHandleWrap hVmInfo;
	QString sSample("basic");

	bool bRes;

	// Create
	createCt(GEN_VM_NAME_BY_TEST_FUNCTION(), sSample, bRes, m_hVm);
	QVERIFY( bRes );

	PRL_EXTRACT_STRING_VALUE(m_sHomePath, m_hVm, PrlVmCfg_GetHomePath);
	QVERIFY(!m_sHomePath.isEmpty());

	hJob.reset(PrlVm_RefreshConfig(m_hVm));
	CHECK_JOB_RET_CODE(hJob);

	QString sVmUuid;
	PRL_EXTRACT_STRING_VALUE(sVmUuid, m_hVm, PrlVmCfg_GetUuid);

	// Get Info
	hJob.reset(PrlVm_GetState(m_hVm));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))

	VIRTUAL_MACHINE_STATE nVmState;
	CHECK_RET_CODE_EXP(PrlVmInfo_GetState(hVmInfo, &nVmState))
	QVERIFY(nVmState == VMS_STOPPED);

	////////////////////////////////////////////////////
	// get Ct from GetVmList
	hJob.reset(PrlSrv_GetVmListEx(m_ServerHandle, PVTF_VM|PVTF_CT));
	CHECK_JOB_RET_CODE(hJob);

	SdkHandleWrap hFoundVm;
	TRY_GET_VM_AFTER_GET_VM_LIST( hJob, sVmUuid, hFoundVm );
	QVERIFY(hFoundVm != PRL_INVALID_HANDLE);
	CHECK_RET_CODE_EXP(PrlVmCfg_GetVmInfo(hFoundVm, hVmInfo.GetHandlePtr() ) );
	CHECK_RET_CODE_EXP(PrlVmInfo_GetState(hVmInfo, &nVmState))
	QVERIFY(nVmState == VMS_STOPPED);

	m_hVm = hFoundVm;
}

void PrlCtManipulationsTest::testCreateCt()
{
	SdkHandleWrap hJob;
	SdkHandleWrap hResult;
	SdkHandleWrap hVmInfo;
	SdkHandleWrap hVm;
	QString sVmUuid;
	QString sSample;
	bool bRes;

	// Create
	createCt(GEN_VM_NAME_BY_TEST_FUNCTION(), sSample, bRes, hVm);
	PRL_EXTRACT_STRING_VALUE(sVmUuid, hVm, PrlVmCfg_GetUuid);
	m_lstVmHandles += SdkHandleWrap(hVm);
	QVERIFY( bRes );

	////////////////////////////////////////////////////
	// get Ct from GetVmList
	hJob.reset(PrlSrv_GetVmListEx(m_ServerHandle, PVTF_VM|PVTF_CT));
	CHECK_JOB_RET_CODE(hJob);

	VIRTUAL_MACHINE_STATE nVmState;
	SdkHandleWrap hFoundVm;
	TRY_GET_VM_AFTER_GET_VM_LIST( hJob, sVmUuid, hFoundVm );
	QVERIFY(hFoundVm != PRL_INVALID_HANDLE);
	CHECK_RET_CODE_EXP(PrlVmCfg_GetVmInfo(hFoundVm, hVmInfo.GetHandlePtr() ) );
	CHECK_RET_CODE_EXP(PrlVmInfo_GetState(hVmInfo, &nVmState))
	QVERIFY(nVmState == VMS_STOPPED);
}

void PrlCtManipulationsTest::testUnRegisterCt()
{
	SdkHandleWrap hJob;

	WRITE_TRACE( DBG_FATAL, "Unregister running Container: %s", QSTR2UTF8(m_sHomePath));
	hJob.reset(PrlVm_Start(m_hVm));
	CHECK_JOB_RET_CODE(hJob);

	CHECK_ASYNC_OP_FAILED(PrlVm_Unreg(m_hVm), PRL_ERR_DISP_VM_COMMAND_CANT_BE_EXECUTED)

	WRITE_TRACE( DBG_FATAL, "Unregister stopped Container: %s", QSTR2UTF8(m_sHomePath));
	hJob.reset(PrlVm_StopEx(m_hVm, PSM_KILL, 0));
	CHECK_JOB_RET_CODE(hJob);

	hJob.reset(PrlVm_Unreg(m_hVm));
	CHECK_JOB_RET_CODE(hJob);
}

void PrlCtManipulationsTest::testRegisterCt()
{
	SdkHandleWrap hJob;
	SdkHandleWrap hResult;

	WRITE_TRACE( DBG_FATAL, "Register Container: %s", QSTR2UTF8(m_sHomePath));
	PRL_UINT32 nFlags = PACF_NON_INTERACTIVE_MODE ;
	hJob.reset(PrlSrv_RegisterVmEx( m_ServerHandle, QSTR2UTF8(m_sHomePath), nFlags ));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_hVm.GetHandlePtr()));
}

void PrlCtManipulationsTest::testCreateCtWithSameName()
{
	SdkHandleWrap hJob;
	SdkHandleWrap hVm;
	SdkHandleWrap hResult;
	QString sName;
	bool bRes;

	sName = GEN_VM_NAME_BY_TEST_FUNCTION();

	// Create
	createCt(sName, QString(), bRes, hVm);
	m_lstVmHandles += SdkHandleWrap(hVm);
	QVERIFY( bRes );

	createCt(sName, QString(), bRes, hVm);
	m_lstVmHandles += SdkHandleWrap(hVm);
	QVERIFY( !bRes );

}

void PrlCtManipulationsTest::testChangeCtName()
{
	SdkHandleWrap hJob;

	QString sNewVmName = GEN_VM_NAME_BY_TEST_FUNCTION();

	WRITE_TRACE( DBG_FATAL, "Change CT name: %s", QSTR2UTF8(sNewVmName));
	hJob.reset(PrlVm_BeginEdit(m_hVm));
	CHECK_JOB_RET_CODE(hJob);

	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_hVm, QSTR2UTF8(sNewVmName)));

	hJob.reset(PrlVm_Commit(m_hVm));
	CHECK_JOB_RET_CODE(hJob);

	hJob.reset(PrlVm_RefreshConfig(m_hVm));
	CHECK_JOB_RET_CODE(hJob);

	QString qsVmName;

	PRL_EXTRACT_STRING_VALUE(qsVmName, m_hVm, PrlVmCfg_GetName);
	QCOMPARE(qsVmName, sNewVmName);
}

void PrlCtManipulationsTest::testSetUserPasswd()
{
	SdkHandleWrap hJob;
	SdkHandleWrap hResult;
	SdkHandleWrap hVmGuest;

	WRITE_TRACE( DBG_FATAL, "Set user password on running Ct");
	hJob.reset(PrlVm_Start(m_hVm));
	CHECK_JOB_RET_CODE(hJob);

	hJob.reset(PrlVm_LoginInGuest(m_hVm, PRL_PRIVILEGED_GUEST_OS_SESSION, 0, 0));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmGuest.GetHandlePtr()));
	hJob.reset(PrlVmGuest_SetUserPasswd(hVmGuest, "user1", "passwd1", 0));
	CHECK_JOB_RET_CODE(hJob);

	hJob.reset(PrlVm_AuthWithGuestSecurityDb(m_hVm, "user1", "passwd1", 0));
	CHECK_JOB_RET_CODE(hJob);

	WRITE_TRACE( DBG_FATAL, "Set user password on stopped Ct");
	hJob.reset(PrlVm_StopEx(m_hVm, PSM_KILL, 0));
	CHECK_JOB_RET_CODE(hJob);

	hJob.reset(PrlVmGuest_SetUserPasswd(hVmGuest, "user2", "passwd2", 0));
	CHECK_JOB_RET_CODE(hJob);
	hJob.reset(PrlVm_AuthWithGuestSecurityDb(m_hVm, "user2", "passwd2", 0));
	CHECK_JOB_RET_CODE(hJob);

}

void PrlCtManipulationsTest::testStartStop()
{
	int i, j, stage;
	QString sUuid;
	QString sSample("slm.512Mb");
	bool bRes;
	const int nNumIter = 5;

QSKIP("Skip test for Container", SkipAll);
	WRITE_TRACE( DBG_FATAL, "StartStop");
	for (i = 0; i < nNumIter; i++)
	{
		SdkHandleWrap hVm;
		createCt(GEN_VM_NAME_BY_TEST_FUNCTION(), sSample, bRes, hVm);
		m_lstVmHandles += SdkHandleWrap(hVm);
		if (!(i % 4))
		{
			createVm(GEN_VM_NAME_BY_TEST_FUNCTION(), bRes, hVm);
			if (bRes)
				m_lstVmHandles += SdkHandleWrap(hVm);
		}
	}

	for (i = 0; i < nNumIter; i++)
	{
		for (stage = 0; stage < 2; stage++)
		{
			QList<SdkHandleWrap> lstJobHandles;
			// Run in parallel
			for (j = 0; j < m_lstVmHandles.size(); ++j)
			{
				PRL_EXTRACT_STRING_VALUE(sUuid, m_lstVmHandles[j], PrlVmCfg_GetUuid);
				if (stage == 0)
				{
					WRITE_TRACE(DBG_FATAL, "Start %s", QSTR2UTF8(sUuid));
					lstJobHandles += SdkHandleWrap(PrlVm_Start(m_lstVmHandles[j]));
				}
				else
				{
					if (!(j % 2))
					{
						WRITE_TRACE(DBG_FATAL, "Suspend %s", QSTR2UTF8(sUuid));
						lstJobHandles += SdkHandleWrap(PrlVm_Suspend(m_lstVmHandles[j]));
					}
					else
					{
						WRITE_TRACE(DBG_FATAL, "Stop %s", QSTR2UTF8(sUuid));
						lstJobHandles += SdkHandleWrap(PrlVm_StopEx(m_lstVmHandles[j], PSM_KILL, 0));
					}
				}
			}
			QTest::qSleep(1000);
			// Wait for complition
			for (j = 0; j < lstJobHandles.size(); ++j)
			{
				PRL_EXTRACT_STRING_VALUE(sUuid, m_lstVmHandles[j], PrlVmCfg_GetUuid);
				PRL_RESULT ret_code = PRL_ERR_UNINITIALIZED;
				PrlJob_Wait(lstJobHandles[j], PRL_JOB_WAIT_TIMEOUT);
				PrlJob_GetRetCode(lstJobHandles[j], &ret_code);
				if (PRL_FAILED(ret_code)) {
					WRITE_TRACE(DBG_FATAL, "%s ret_code=%.8X '%s'",
						QSTR2UTF8(sUuid), ret_code, PRL_RESULT_TO_STRING(ret_code));
				}
			}
		}
	}
}

void PrlCtManipulationsTest::testSubscribeToPerfStats()
{
	SdkHandleWrap hJob(PrlVm_Start(m_hVm));
	CHECK_JOB_RET_CODE(hJob) ;

	hJob.reset(PrlVm_GetPerfStats(m_hVm, ""));
	CHECK_JOB_RET_CODE(hJob) ;
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hEvent;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hEvent.GetHandlePtr()))
	QVERIFY(hEvent.valid()) ;

	PRL_UINT32 nCountersCount = 0;
	CHECK_RET_CODE_EXP(PrlEvent_GetParamsCount(hEvent, &nCountersCount))
}

void PrlCtManipulationsTest::testGetPerfStats()
{
	SdkHandleWrap hJob(PrlVm_Start(m_hVm));
	CHECK_JOB_RET_CODE(hJob);

	hJob.reset(PrlVm_GetPerfStats(m_hVm, ""));
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

void PrlCtManipulationsTest::testSetDnsServers()
{
	SdkHandleWrap hJob;

	// Check for invalid parameters
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetDnsServers(m_hVm, m_hVm), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetDnsServers(m_hVm, 0), PRL_ERR_INVALID_ARG)

	// 1) Check for valid parameter
	hJob.reset(PrlVm_BeginEdit(m_hVm));
	CHECK_JOB_RET_CODE(hJob)

        QStringList lstNewDnsServers = QStringList()<<"192.168.1.1"<<"10.30.1.1";
        SdkHandleWrap hNewDnsServersList;
        CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hNewDnsServersList.GetHandlePtr()))
        foreach(QString sNetAddress, lstNewDnsServers)
                CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewDnsServersList, QSTR2UTF8(sNetAddress)))
        CHECK_RET_CODE_EXP(PrlVmCfg_SetDnsServers(m_hVm, hNewDnsServersList))
        PRL_CHECK_STRINGS_LIST(m_hVm, PrlVmCfg_GetDnsServers, lstNewDnsServers)

	// Check from config
	hJob.reset(PrlVm_Commit(m_hVm));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_RefreshConfig(m_hVm));
	CHECK_JOB_RET_CODE(hJob);

	PRL_CHECK_STRINGS_LIST(m_hVm, PrlVmCfg_GetDnsServers, lstNewDnsServers)

	// 2) Check for empty parameters
	hJob.reset(PrlVm_BeginEdit(m_hVm));
	CHECK_JOB_RET_CODE(hJob)

        CHECK_RET_CODE_EXP(PrlVmCfg_SetDnsServers(m_hVm, 0))
        PRL_CHECK_STRINGS_LIST(m_hVm, PrlVmCfg_GetDnsServers, QStringList())

	// Check from config
	hJob.reset(PrlVm_Commit(m_hVm));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_RefreshConfig(m_hVm));
	CHECK_JOB_RET_CODE(hJob);

	PRL_CHECK_STRINGS_LIST(m_hVm, PrlVmCfg_GetDnsServers, QStringList())
}

void PrlCtManipulationsTest::testApplyIpOnly()
{
	SdkHandleWrap hJob;
	PRL_BOOL bIpOnly;

	// Check for invalid parameters
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_IsAutoApplyIpOnly(m_hVm, NULL), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetAutoApplyIpOnly(m_hSrvConfig, true), PRL_ERR_INVALID_ARG)

	// Check for valid parameter
	hJob.reset(PrlVm_BeginEdit(m_hVm));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlVmCfg_SetAutoApplyIpOnly(m_hVm, false))
	CHECK_RET_CODE_EXP(PrlVmCfg_IsAutoApplyIpOnly(m_hVm, &bIpOnly))
	QVERIFY(!bIpOnly);

	CHECK_RET_CODE_EXP(PrlVmCfg_SetAutoApplyIpOnly(m_hVm, true))
	CHECK_RET_CODE_EXP(PrlVmCfg_IsAutoApplyIpOnly(m_hVm, &bIpOnly))
	QVERIFY(bIpOnly);

	// Check from config
	hJob.reset(PrlVm_Commit(m_hVm));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_RefreshConfig(m_hVm));
	CHECK_JOB_RET_CODE(hJob);

	CHECK_RET_CODE_EXP(PrlVmCfg_IsAutoApplyIpOnly(m_hVm, &bIpOnly))
	QVERIFY(bIpOnly);
}

void PrlCtManipulationsTest::testSetSearchDomains()
{
	SdkHandleWrap hJob;

	// Check for invalid parameters
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetSearchDomains(m_hVm, m_hVm), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetSearchDomains(m_hVm, 0), PRL_ERR_INVALID_ARG)

	// 1) Check for valid parameter
	hJob.reset(PrlVm_BeginEdit(m_hVm));
	CHECK_JOB_RET_CODE(hJob)

	QStringList lstNewSearchDomains = QStringList()<<"exmaple.com"<<"example.ru";
	SdkHandleWrap hNewSearchDomainsList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hNewSearchDomainsList.GetHandlePtr()))
	foreach(QString sNetAddress, lstNewSearchDomains)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewSearchDomainsList, QSTR2UTF8(sNetAddress)))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetSearchDomains(m_hVm, hNewSearchDomainsList))
	PRL_CHECK_STRINGS_LIST(m_hVm, PrlVmCfg_GetSearchDomains, lstNewSearchDomains)

	// Check from config
	hJob.reset(PrlVm_Commit(m_hVm));
	CHECK_JOB_RET_CODE(hJob)
	hJob.reset(PrlVm_RefreshConfig(m_hVm));
	CHECK_JOB_RET_CODE(hJob);

	PRL_CHECK_STRINGS_LIST(m_hVm, PrlVmCfg_GetSearchDomains, lstNewSearchDomains)

	// 2) Check for empty parameters
	hJob.reset(PrlVm_BeginEdit(m_hVm));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlVmCfg_SetSearchDomains(m_hVm, 0))
	PRL_CHECK_STRINGS_LIST(m_hVm, PrlVmCfg_GetSearchDomains, QStringList())

        // Check from config
        hJob.reset(PrlVm_Commit(m_hVm));
        CHECK_JOB_RET_CODE(hJob)
        hJob.reset(PrlVm_RefreshConfig(m_hVm));
        CHECK_JOB_RET_CODE(hJob);

        PRL_CHECK_STRINGS_LIST(m_hVm, PrlVmCfg_GetSearchDomains, QStringList())
}

void PrlCtManipulationsTest::testClone()
{
	SdkHandleWrap hJob;
	bool bTempl = false;

	for(int i = 0; i < 2; i++)
	{
		hJob.reset(PrlVm_CloneEx(m_hVm, QSTR2UTF8(GEN_VM_NAME_BY_TEST_FUNCTION()), "",
			bTempl ? PCVF_CLONE_TO_TEMPLATE : 0));
		CHECK_JOB_RET_CODE(hJob);

		SdkHandleWrap hResult;
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

		SdkHandleWrap hVm;
		CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVm.GetHandlePtr()))
		m_lstVmHandles += SdkHandleWrap(hVm);

		QString sNewUuid;
		PRL_EXTRACT_STRING_VALUE(sNewUuid, hVm, PrlVmCfg_GetUuid);

		SdkHandleWrap hFoundVm;
		TRY_GET_VM_AFTER_GET_VM_LIST( hJob, sNewUuid, hFoundVm );
		QVERIFY(hFoundVm != PRL_INVALID_HANDLE);

		PRL_BOOL IsTmpl;
		CHECK_RET_CODE_EXP(PrlVmCfg_IsTemplate(hVm, &IsTmpl))
		QVERIFY((bool)IsTmpl == bTempl);

		bTempl = true;
	}
}

void PrlCtManipulationsTest::testGetCapabilities()
{
	bool bRes;
	PRL_UINT32 nCapMask = 0;

	// Check for invalid parameters
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetCapabilitiesMask(m_hVm, NULL), PRL_ERR_INVALID_ARG)

	createCt(GEN_VM_NAME_BY_TEST_FUNCTION(), QString(), bRes, m_hVm);
	QVERIFY( bRes );

	CHECK_RET_CODE_EXP(PrlVmCfg_GetCapabilitiesMask(m_hVm, &nCapMask))
	QVERIFY(nCapMask == (PRL_UINT32)CAPDEFAULTMASK);
}

void PrlCtManipulationsTest::testSetCapabilities()
{
	SdkHandleWrap hJob;
	bool bRes;

	createCt(GEN_VM_NAME_BY_TEST_FUNCTION(), QString(), bRes, m_hVm);
	QVERIFY( bRes );

	//Create cap mask
	PRL_UINT32 nMaskOn = (PCC_SYS_ADMIN | PCC_SYS_TIME | PCC_SYS_PACCT);
	PRL_UINT32 nMaskOff = (PCC_LEASE | PCC_NET_BIND_SERVICE | PCC_SYS_TTY_CONFIG);
	PRL_UINT32 nCapMask = ((CAPDEFAULTMASK | nMaskOn)& ~nMaskOff);

	//Set cap mask to Ct
	hJob.reset(PrlVm_BeginEdit(m_hVm));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlVmCfg_SetCapabilitiesMask(m_hVm, nCapMask))

	hJob.reset(PrlVm_Commit(m_hVm));
	CHECK_JOB_RET_CODE(hJob)
	hJob.reset(PrlVm_RefreshConfig(m_hVm));
	CHECK_JOB_RET_CODE(hJob);

	// Verify results
	PRL_UINT32 nNewCapMask = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetCapabilitiesMask(m_hVm, &nNewCapMask))
	QVERIFY(nCapMask == nNewCapMask);
}

void PrlCtManipulationsTest::testGetNetfilterMode()
{
	bool bRes;
	PRL_NETFILTER_MODE nMode;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetNetfilterMode(NULL, &nMode), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetNetfilterMode(m_hVm, NULL), PRL_ERR_INVALID_ARG)

	createCt(GEN_VM_NAME_BY_TEST_FUNCTION(), QString(), bRes, m_hVm);
	QVERIFY( bRes );

	CHECK_RET_CODE_EXP(PrlVmCfg_GetNetfilterMode(m_hVm, &nMode))
	QVERIFY( nMode == PCNM_NOT_SET );
}

void PrlCtManipulationsTest::testSetNetfilterMode()
{
	bool bRes;
	const PRL_NETFILTER_MODE ModesList[] = {
			PCNM_DISABLED, PCNM_STATELESS, PCNM_STATEFUL, PCNM_FULL};

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_SetNetfilterMode(NULL, PCNM_DISABLED), PRL_ERR_INVALID_ARG)

	createCt(GEN_VM_NAME_BY_TEST_FUNCTION(), QString(), bRes, m_hVm);
	QVERIFY( bRes );

	for (PRL_UINT32 i = 0; i < sizeof(ModesList)/sizeof(PRL_NETFILTER_MODE); ++i)
	{
		SdkHandleWrap hJob;

		hJob.reset(PrlVm_BeginEdit(m_hVm));
		CHECK_JOB_RET_CODE(hJob)

		CHECK_RET_CODE_EXP(PrlVmCfg_SetNetfilterMode(m_hVm, ModesList[i]))

		hJob.reset(PrlVm_Commit(m_hVm));
		CHECK_JOB_RET_CODE(hJob)
		hJob.reset(PrlVm_RefreshConfig(m_hVm));
		CHECK_JOB_RET_CODE(hJob);

		PRL_NETFILTER_MODE nNewMode;
		CHECK_RET_CODE_EXP(PrlVmCfg_GetNetfilterMode(m_hVm, &nNewMode))
		QVERIFY( ModesList[i] == nNewMode );
	}
}

