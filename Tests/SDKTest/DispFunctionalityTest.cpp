/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (C) 2006-2015 Parallels IP Holdings GmbH
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
/// @file
///		DispFunctionalityTest.cpp
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

#include "DispFunctionalityTest.h"
#include "SimpleServerWrapper.h"

#include "Tests/CommonTestsUtils.h"

#include "AutoHelpers.h"

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

#include <prlcommon/ProtoSerializer/CProtoSerializer.h>


#include <prlxmlmodel/DispConfig/CDispCommonPreferences.h>

#include <prlsdk/PrlApiDeprecated.h>
#include <prlcommon/Interfaces/ParallelsSdkPrivate.h>

#include <time.h>
#include <errno.h>
#if defined(_LIN_) || defined(_MAC_)
#include <unistd.h>
#endif


#include "Build/Current.ver"

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class ICheckPatchBase
{
public:
	virtual void cleanupPatchedField(bool& bRes, CBaseNode* pCfg) = 0;
	virtual void checkPatchedField( bool& bRes, CBaseNode* pCfg) = 0;
	virtual ~ICheckPatchBase() {}
};

template<class CFG> class CheckPatchBase: public ICheckPatchBase
{
protected:
	CheckPatchBase( const QVariant& checkValue ): m_checkValue(checkValue) {}

	CFG* getCfg( CBaseNode* pBase ){ return dynamic_cast<CFG*>( pBase ); }
	QVariant getCheckValue() { return m_checkValue; }
private:
	QVariant m_checkValue;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

DispFunctionalityTest::DispFunctionalityTest()
{

}

DispFunctionalityTest::~DispFunctionalityTest()
{

}

void DispFunctionalityTest::init()
{
	CHECK_RET_CODE_EXP(PrlSrv_Create(m_ServerHandle.GetHandlePtr()));
	m_JobHandle.reset();
	m_VmHandle.reset();
}

void DispFunctionalityTest::cleanup()
{
	if (m_VmHandle != PRL_INVALID_HANDLE
		|| !m_lstVmHandles.isEmpty())
	{
		m_lstVmHandles += m_VmHandle;

		// create list to delete for sdk
		for(int i = 0; i < m_lstVmHandles.size(); ++i)
		{
			if (m_lstVmHandles[i] != PRL_INVALID_HANDLE)
			{
				m_JobHandle.reset(PrlVm_Delete(m_lstVmHandles[i], PRL_INVALID_HANDLE));
				PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
			}

			if (m_lstVmHandles[i] != PRL_INVALID_HANDLE)
			{
				// If VM is invalid
				m_JobHandle.reset(PrlVm_Unreg(m_lstVmHandles[i]));
				PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
			}
		}
		m_lstVmHandles.clear();
	}

	if (m_JobHandle)
	{
		m_JobHandle.reset(PrlSrv_Logoff(m_ServerHandle));
		PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
	}
	m_ResultHandle.reset();
	m_JobHandle.reset();
	m_ServerHandle.reset();
}


void DispFunctionalityTest::test_login()
{
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


void DispFunctionalityTest::test_bug117830_SaveAutoStartParams()
{
	test_login();

	const PRL_VM_AUTOSTART_OPTION AUTOSTART_TO_SET_VALUE = PAO_VM_START_MANUAL;
	const PRL_VM_AUTOSTART_OPTION AUTOSTART_TO_EXPECT_VALUE = PAO_VM_START_MANUAL;

	SdkHandleWrap hJob;
	SET_DEFAULT_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);


	enum TestCaseEnum { tcCreate = 0, tcRegister, tcEdit, tcLAST };
	for( int tc = tcCreate; tc < tcLAST; tc++ )
	{
		WRITE_TRACE(DBG_FATAL, "%s: tc=%d", __FUNCTION__, tc );
		CHECK_RET_CODE_EXP(PrlVmCfg_SetAutoStart( m_VmHandle, AUTOSTART_TO_SET_VALUE ) );

		switch( (TestCaseEnum)tc )
		{
		case tcCreate:
			hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
			CHECK_JOB_RET_CODE(hJob);
			break;
		case tcRegister:
			{
				PRL_CHAR sHomePath[STR_BUF_LENGTH];
				PRL_UINT32 nHomePathLength = STR_BUF_LENGTH;
				CHECK_RET_CODE_EXP(PrlVmCfg_GetHomePath(m_VmHandle, sHomePath, &nHomePathLength));

				WRITE_TRACE(DBG_FATAL, "home_path = %s", sHomePath );

				hJob.reset(PrlVm_Unreg(m_VmHandle));
				CHECK_JOB_RET_CODE(hJob);
				m_VmHandle.reset();

				hJob.reset(PrlSrv_RegisterVm( m_ServerHandle, sHomePath, PRL_TRUE ));
				CHECK_JOB_RET_CODE(hJob);
				SdkHandleWrap hResult;
				CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
				CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()));
			}
			break;
		case tcEdit:
			hJob.reset(PrlVm_BeginEdit(m_VmHandle));
			CHECK_JOB_RET_CODE(hJob);
			CHECK_RET_CODE_EXP(PrlVmCfg_SetAutoStart( m_VmHandle, AUTOSTART_TO_SET_VALUE ) );
			hJob.reset(PrlVm_Commit(m_VmHandle));
			CHECK_JOB_RET_CODE(hJob);
			break;
		default:
			QFAIL( "INVALID TEST CASE " );
		}//switch

		hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
		CHECK_JOB_RET_CODE(hJob);

		PRL_VM_AUTOSTART_OPTION nCurrAutoStart;
		//////////////////////////////////////////////////////////////////////////
		// test 1
		CHECK_RET_CODE_EXP(PrlVmCfg_GetAutoStart( m_VmHandle, &nCurrAutoStart ) );
		QCOMPARE( nCurrAutoStart, AUTOSTART_TO_EXPECT_VALUE );

		//////////////////////////////////////////////////////////////////////////
		// test 2  load from config.pvs
		CVmConfiguration vm_conf;
		EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( m_VmHandle, (&vm_conf) );

		nCurrAutoStart = vm_conf.getVmSettings()->getVmStartupOptions()->getAutoStart();
		QCOMPARE( nCurrAutoStart, AUTOSTART_TO_EXPECT_VALUE );
	}//for tc
}

void DispFunctionalityTest::test_RegisterVmWhenVmWasUnregisteredOnSameServer()
{
	QSKIP( "Not implemented yet", SkipAll );
	// test1: lastServerUuid == localServerUuid  --> we havn't  PET_QUESTION_VM_COPY_OR_MOVE
	// test2: lastServerUuid != localServerUuid  --> catch  PET_QUESTION_VM_COPY_OR_MOVE

	// login
	// create  vm
	// unregister vm
	// register_question_callback with global var;
	// register vm
	// test1, test2: wait for job complete and wait for question
	// analize

	/*
	test_login();
	QString serverUuid;

	SdkHandleWrap hJob;
	SET_DEFAULT_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(m_VmHandle, QSTR2UTF8( Uuid::createUuid().toString() ) ) );

	hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);

	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	PRL_CHAR sHomePath[STR_BUF_LENGTH];
	PRL_UINT32 nHomePathLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHomePath(m_VmHandle, sHomePath, &nHomePathLength));

	hJob.reset(PrlVm_Unreg(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);
	m_VmHandle.reset();

	//////////////////////////////////////////////////////////////////////////
	// check that lastServerUuid  was stored in config.pvs
	CVmConfiguration conf;
	QFile f( UTF8_2QSTR( sHomePath ) );
	QVERIFY( 0 == conf.loadFromFile( f ) );
	QCOMPARE( conf.getVmIdentification()->getServerUuid(), "" );
	QCOMPARE( conf.getVmIdentification()->getLastServerUuid(), m_sServerUuid );

	//////////////////////////////////////////////////////////////////////////
	///
	*/
}

// https://bugzilla.sw.ru/show_bug.cgi?id=126119
void DispFunctionalityTest::test_bug126119_RegisterAlreadyRegistredVm()
{
	test_login();

	//////////////////////////////////////////////////////////////////////////
	// create VM
	//
	SdkHandleWrap hJob;
	SET_DEFAULT_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(m_VmHandle, QSTR2UTF8( Uuid::createUuid().toString() ) ) );

	hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);

	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// try to already register same VM
	//
	PRL_CHAR sHomePath[STR_BUF_LENGTH];
	PRL_UINT32 nHomePathLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHomePath(m_VmHandle, sHomePath, &nHomePathLength));

	CHECK_ASYNC_OP_FAILED( PrlSrv_RegisterVm( m_ServerHandle, sHomePath, PRL_TRUE ) \
		,  PRL_ERR_VM_ALREADY_REGISTERED );
}

// https://bugzilla.sw.ru/show_bug.cgi?id=126119
void DispFunctionalityTest::test_bug126119_CreateAlreadyRegistredVm()
{
	test_login();

	//////////////////////////////////////////////////////////////////////////
	// create VM
	//
	SdkHandleWrap hJob;
	SET_DEFAULT_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(m_VmHandle, QSTR2UTF8( Uuid::createUuid().toString() ) ) );

	hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);

	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// try to already create VM with same params
	//
	CHECK_ASYNC_OP_FAILED( PrlVm_Reg(m_VmHandle, "", PRL_TRUE) , PRL_ERR_VM_ALREADY_REGISTERED );
}

#define GENERATE_PATH_TO_TEMP_DIR() \
	QString( "%1/%2.%3" ) \
	.arg( ParallelsDirs::getSystemTempDir() ) \
	.arg( QTest::currentTestFunction() ).arg( qrand() );

#define MAKE_VM_ROOT_DIR_FOR_BUG127473 GENERATE_PATH_TO_TEMP_DIR()

void DispFunctionalityTest::test_bug127473_DenyToCreateUserDefinedVmDir_OnLogin()
{
	AutoRestorer autoDispRestorer;
	AutoDirRemover autoDel;
	SdkHandleWrap hJob;

	// 1. login
	// 2. create folder A
	// 3. register folder A as user defined vm folder
	// 4. logoff
	// 5. delete folder from drive
	// 6. login
	// 7. check : folder doesn't exist.

	//////////////////////////////////////////////////////////////////////////
	// 1. login
	hJob.reset(PrlSrv_LoginLocal(m_ServerHandle, "" , 0, PSL_HIGH_SECURITY));
	CHECK_JOB_RET_CODE( hJob );

	//////////////////////////////////////////////////////////////////////////
	// 2. create folder A
	QString vmRootDir = MAKE_VM_ROOT_DIR_FOR_BUG127473;

	QVERIFY( QDir().mkdir( vmRootDir ) );
	QVERIFY( autoDel.add( vmRootDir ) );

	//////////////////////////////////////////////////////////////////////////
	// 3. register folder A as user defined vm folder
	hJob.reset(PrlSrv_UserProfileBeginEdit( m_ServerHandle ));
	CHECK_JOB_RET_CODE(hJob);

	SdkHandleWrap hUserProfile;
	RECEIVE_USER_PROFILE( m_ServerHandle, hUserProfile);

	QString oldVmRootDir;
	FILL_DEFAULT_VM_FOLDER(oldVmRootDir, hUserProfile);

	QVERIFY( PRL_SUCCEEDED(PrlUsrCfg_SetDefaultVmFolder( hUserProfile, QSTR2UTF8(vmRootDir) ) ) );

	hJob.reset(PrlSrv_UserProfileCommit( m_ServerHandle, hUserProfile ));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// create rollback point
	SmartPtr<CVmEvent> pParams( new CVmEvent );
	pParams->addEventParameter( new CVmEventParameter( PVE::String, oldVmRootDir ) );
	autoDispRestorer.addRollback( qMakePair( &AutoRestorer::restoreUserVmFolder, pParams ) );

	//////////////////////////////////////////////////////////////////////////
	// 4. logoff
	hJob.reset(PrlSrv_Logoff(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 5. delete folder from drive
	QVERIFY( QDir().rmdir( vmRootDir ) );
	QVERIFY( autoDel.del( vmRootDir ) );

	//////////////////////////////////////////////////////////////////////////
	// 6. login
	hJob.reset(PrlSrv_LoginLocal(m_ServerHandle, "" , 0, PSL_HIGH_SECURITY));
	CHECK_JOB_RET_CODE( hJob );

	//////////////////////////////////////////////////////////////////////////
	// 7. check : folder doesn't exist.
	QVERIFY( !QDir( vmRootDir ).exists() );
}

void DispFunctionalityTest::test_bug127473_AllowToCreateDefaultVmDir_OnLogin()
{
	QSKIP( "Not implemented yet", SkipAll );
}

void DispFunctionalityTest::test_bug127473_DenyToCreateVmInUnexistingUserDefinedVmDir()
{
	AutoRestorer autoDispRestorer;
	AutoDirRemover autoDel;
	SdkHandleWrap hJob;

	// 1. login
	// 2. create folder A
	// 3. register folder A as user defined vm folder
	// 4. delete folder from drive
	// 5. try to CREATE Vm in default folder (SHOULD BE ERROR)
	// 5. check : folder doesn't exist.

	//////////////////////////////////////////////////////////////////////////
	// 1. login
	hJob.reset(PrlSrv_LoginLocal(m_ServerHandle, "" , 0, PSL_HIGH_SECURITY));
	CHECK_JOB_RET_CODE( hJob );

	//////////////////////////////////////////////////////////////////////////
	// 2. create folder A
	QString vmRootDir = MAKE_VM_ROOT_DIR_FOR_BUG127473;

	QVERIFY( QDir().mkdir( vmRootDir ) );
	QVERIFY( autoDel.add( vmRootDir ) );

	//////////////////////////////////////////////////////////////////////////
	// 3. register folder A as user defined vm folder
	hJob.reset(PrlSrv_UserProfileBeginEdit( m_ServerHandle ));
	CHECK_JOB_RET_CODE(hJob);

	SdkHandleWrap hUserProfile;
	RECEIVE_USER_PROFILE( m_ServerHandle, hUserProfile);

	QString oldVmRootDir;
	FILL_DEFAULT_VM_FOLDER(oldVmRootDir, hUserProfile);

	QVERIFY( PRL_SUCCEEDED(PrlUsrCfg_SetDefaultVmFolder( hUserProfile, QSTR2UTF8(vmRootDir) ) ) );

	hJob.reset(PrlSrv_UserProfileCommit( m_ServerHandle, hUserProfile ));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// create rollback point
	SmartPtr<CVmEvent> pParams( new CVmEvent );
	pParams->addEventParameter( new CVmEventParameter( PVE::String, oldVmRootDir ) );
	autoDispRestorer.addRollback( qMakePair( &AutoRestorer::restoreUserVmFolder, pParams ) );

	//////////////////////////////////////////////////////////////////////////
	// 4. delete folder from drive
	QVERIFY( QDir().rmdir( vmRootDir ) );
	QVERIFY( autoDel.del( vmRootDir ) );

	//////////////////////////////////////////////////////////////////////////
	// 5. try to CREATE Vm in default folder (SHOULD BE ERROR)
	SET_DEFAULT_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT));

	// 7. check : folder doesn't exist.
	QVERIFY( !QDir( vmRootDir ).exists() );

	CHECK_ASYNC_OP_FAILED( PrlVm_Reg(m_VmHandle, "", PRL_TRUE), PRL_ERR_VM_DIRECTORY_FOLDER_DOESNT_EXIST );

	//////////////////////////////////////////////////////////////////////////
}

void DispFunctionalityTest::test_bug127473_DenyToCloneVmToUnexistingUserDefinedVmDir()
{
	AutoRestorer autoDispRestorer;
	AutoDirRemover autoDel;
	SdkHandleWrap hJob;

	// 1. login
	// 2. create VM-1
	// 3. create folder A
	// 4. register folder A as user defined vm folder
	// 5. delete folder from drive
	// 6. try to CLONE Vm to default folder (SHOULD BE ERROR)
	// 5. check : folder doesn't exist.

	//////////////////////////////////////////////////////////////////////////
	// 1. login
	hJob.reset(PrlSrv_LoginLocal(m_ServerHandle, "" , 0, PSL_HIGH_SECURITY));
	CHECK_JOB_RET_CODE( hJob );

	//////////////////////////////////////////////////////////////////////////
	// 1.1. CREATE Vm in default folder
	SET_DEFAULT_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE( hJob );


	//////////////////////////////////////////////////////////////////////////
	// 2. create folder A
	QString vmRootDir = MAKE_VM_ROOT_DIR_FOR_BUG127473;

	QVERIFY( QDir().mkdir( vmRootDir ) );
	QVERIFY( autoDel.add( vmRootDir ) );

	//////////////////////////////////////////////////////////////////////////
	// 3. register folder A as user defined vm folder
	hJob.reset(PrlSrv_UserProfileBeginEdit( m_ServerHandle ));
	CHECK_JOB_RET_CODE(hJob);

	SdkHandleWrap hUserProfile;
	RECEIVE_USER_PROFILE( m_ServerHandle, hUserProfile);

	QString oldVmRootDir;
	FILL_DEFAULT_VM_FOLDER(oldVmRootDir, hUserProfile);

	QVERIFY( PRL_SUCCEEDED(PrlUsrCfg_SetDefaultVmFolder( hUserProfile, QSTR2UTF8(vmRootDir) ) ) );

	hJob.reset(PrlSrv_UserProfileCommit( m_ServerHandle, hUserProfile ));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// create rollback point
	SmartPtr<CVmEvent> pParams( new CVmEvent );
	pParams->addEventParameter( new CVmEventParameter( PVE::String, oldVmRootDir ) );
	autoDispRestorer.addRollback( qMakePair( &AutoRestorer::restoreUserVmFolder, pParams ) );

	//////////////////////////////////////////////////////////////////////////
	// 4. delete folder from drive
	QVERIFY( QDir().rmdir( vmRootDir ) );
	QVERIFY( autoDel.del( vmRootDir ) );

	//////////////////////////////////////////////////////////////////////////
	// 5. try to CLONE Vm in default folder (SHOULD BE ERROR)
	QString newVmName = QString( "bug127473.%1" ).arg( Uuid::createUuid().toString() ) ;

	hJob.reset(PrlVm_Clone(m_VmHandle, QSTR2UTF8(newVmName), "", PRL_FALSE));
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT))

		// 6. check : folder doesn't exist.
		QVERIFY( !QDir( vmRootDir ).exists() );

	CHECK_ASYNC_OP_FAILED( hJob, PRL_ERR_VM_DIRECTORY_FOLDER_DOESNT_EXIST );
	//////////////////////////////////////////////////////////////////////////
}

void DispFunctionalityTest::test_bug127473_DenyToCreateVmInUnexistingDir()
{
	SdkHandleWrap hJob;

	// 1. login
	// 2. generate path to unexisted folder A
	// 3. try to CREATE Vm to folder A (SHOULD BE ERROR)
	// 4. check : folder doesn't exist.

	//////////////////////////////////////////////////////////////////////////
	// 1. login
	hJob.reset(PrlSrv_LoginLocal(m_ServerHandle, "" , 0, PSL_HIGH_SECURITY));
	CHECK_JOB_RET_CODE( hJob );

	//////////////////////////////////////////////////////////////////////////
	// 2. generate path to unexisted folder A
	QString vmRootDir = MAKE_VM_ROOT_DIR_FOR_BUG127473;

	//////////////////////////////////////////////////////////////////////////
	// 3. try to CREATE Vm in default folder (SHOULD BE ERROR)
	SET_DEFAULT_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	hJob.reset(PrlVm_Reg(m_VmHandle, QSTR2UTF8(vmRootDir), PRL_TRUE));
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT));

	// 4. check : folder doesn't exist.
	QVERIFY( !QDir( vmRootDir ).exists() );
	// 4. check fails
	CHECK_ASYNC_OP_FAILED( hJob, PRL_ERR_DIRECTORY_DOES_NOT_EXIST );
}

void DispFunctionalityTest::test_bug127473_DenyToCloneVmToUnexistingDir()
{
	SdkHandleWrap hJob;

	// 1. login
	// 2 create VM
	// 3. generate path to unexisted folder A
	// 4. try to CREATE Vm to folder A (SHOULD BE ERROR)
	// 5. check : folder doesn't exist.

	//////////////////////////////////////////////////////////////////////////
	// 1. login
	hJob.reset(PrlSrv_LoginLocal(m_ServerHandle, "" , 0, PSL_HIGH_SECURITY));
	CHECK_JOB_RET_CODE( hJob );

	//////////////////////////////////////////////////////////////////////////
	// 2. CREATE Vm in default folder
	SET_DEFAULT_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE( hJob );

	//////////////////////////////////////////////////////////////////////////
	// 3. generate path to unexisted folder A
	QString vmRootDir = MAKE_VM_ROOT_DIR_FOR_BUG127473;

	//////////////////////////////////////////////////////////////////////////
	// 4. try to CLONE Vm in default folder (SHOULD BE ERROR)
	QString newVmName = QString( "bug127473.%1" ).arg( Uuid::createUuid().toString() ) ;

	hJob.reset(PrlVm_Clone(m_VmHandle, QSTR2UTF8(newVmName), QSTR2UTF8(vmRootDir), PRL_FALSE));
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT));

	// 5. check : folder doesn't exist.
	QVERIFY( !QDir( vmRootDir ).exists() );
	CHECK_ASYNC_OP_FAILED( hJob, PRL_ERR_DIRECTORY_DOES_NOT_EXIST );

}

void DispFunctionalityTest::test_VmUptime()
{
	// https://bugzilla.sw.ru/show_bug.cgi?id=127477
	// OS uptime for VM does not reported by Parallels SDK

	// 1. create VM
	// 2. start VM
	// 3. get vm statistic
	// 4. ( print it to out )
	//    or compare UPTIME
	// 5. stop VM
	// 6. delete VM


	{
		SdkHandleWrap hJob;
		// 0. login
		hJob.reset(PrlSrv_LoginLocal(m_ServerHandle, "" , 0, PSL_HIGH_SECURITY));
		CHECK_JOB_RET_CODE( hJob );

		// 1. create VM
		SET_DEFAULT_CONFIG(m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
		CHECK_RET_CODE_EXP(PrlVmCfg_SetRamSize( m_VmHandle, 200 )); // to skip OVERCOMMIT QUESTIONS
		CHECK_RET_CODE_EXP(PrlVmCfg_Set3DAccelerationEnabled(m_VmHandle, PRL_FALSE))//To prevent problems with GL context absense

			hJob.reset(PrlVm_Reg( m_VmHandle, "", PRL_TRUE));
		CHECK_JOB_RET_CODE(hJob);

		AutoDeleteVm autoDeleteVm( m_VmHandle );

		quint32 startTimestamp = PrlGetTickCount();

		// 2. start VM
		hJob.reset(PrlVm_Start(m_VmHandle));
		CHECK_JOB_RET_CODE(hJob);

		AutoStopVm autoVmStop( m_VmHandle );


		HostUtils::Sleep( 1*1000 );

		// 3. get vm statistic
		hJob.reset(PrlVm_GetStatistics(m_VmHandle));
		CHECK_JOB_RET_CODE(hJob);

		SdkHandleWrap hResult;
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
		SdkHandleWrap hStat;
		CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hStat.GetHandlePtr()) );
		CHECK_HANDLE_TYPE( hStat, PHT_SYSTEM_STATISTICS);

		// 4. ( print it to out )
		//    or compare UPTIME
		PRL_UINT64 nUptime = 0;
		CHECK_RET_CODE_EXP( PrlStat_GetOsUptime( hStat, &nUptime ) );

		quint32 currTimestamp = PrlGetTickCount();
		PRL_UINT64 deltaInSecs = qRound64(  (double)(currTimestamp - startTimestamp) / PrlGetTicksPerSecond() );

		WRITE_TRACE(DBG_FATAL, "VM uptime = %llu sec, ( elapsed = %llu sec )", nUptime, deltaInSecs );

		QVERIFY( deltaInSecs >= nUptime );

		/*
		QByteArray sBuf;
		PRL_UINT32 nBufSize = 0;
		CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, 0, &nBufSize));
		sBuf.resize(nBufSize);
		CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, sBuf.data(), &nBufSize));
		CSystemStatistics _vm_stat;
		CHECK_RET_CODE_EXP(_vm_stat.fromString(UTF8_2QSTR(sBuf)));
		*/
	}

}

#define RECIEVE_COMMON_PREFS( hCommonPrefs, outXml ) \
{	\
	SdkHandleWrap hJob(PrlSrv_GetCommonPrefs(m_ServerHandle)); \
	CHECK_JOB_RET_CODE(hJob); \
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr())); \
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hCommonPrefs.GetHandlePtr())); \
	\
	PRL_VOID_PTR pResString = 0; \
	CHECK_RET_CODE_EXP(PrlDispCfg_ToString(hCommonPrefs, &pResString)); \
	QString sCommonPrefs = UTF8_2QSTR((const char *)pResString); \
	PrlBuffer_Free(pResString); \
	\
	CHECK_RET_CODE_EXP(outXml.fromString(sCommonPrefs)); \
}

void DispFunctionalityTest::testCommonPrefsSetReadOnlyValues()
{
	// 0. login
	// 1. get config
	// 2. set read only value
	// 3. commit
	// 4. get config && compare.

	SdkHandleWrap hJob;
	SdkHandleWrap hResult;
	//////////////////////////////////////////////////////////////////////////
	// 0. login
	hJob.reset(PrlSrv_LoginLocal(m_ServerHandle, "" , 0, PSL_HIGH_SECURITY));
	CHECK_JOB_RET_CODE( hJob );

	//////////////////////////////////////////////////////////////////////////
	//Extracting user profile to check 'CanChangeSrvSets'
	hJob.reset(PrlSrv_GetUserProfile(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	SdkHandleWrap hUserProfile;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hUserProfile.GetHandlePtr()) );

	PRL_BOOL bCanChangeSrvSets = false;
	CHECK_RET_CODE_EXP(PrlUsrCfg_CanChangeSrvSets( hUserProfile, &bCanChangeSrvSets ) );
	if( ! bCanChangeSrvSets )
		QSKIP( "Skip test because current test user unable to check server settings", SkipAll );

	//////////////////////////////////////////////////////////////////////////
	// 1. get common prefs config
	SdkHandleWrap hCommonPrefs;

	CDispCommonPreferences tmpCommonPrefs;
	RECIEVE_COMMON_PREFS( hCommonPrefs, tmpCommonPrefs);

	const CDispCommonPreferences cpBefore( tmpCommonPrefs );
	CDispCommonPreferences cpChanged( tmpCommonPrefs );

	AutoRevertCommonPrefsChanges autoRevert( m_ServerHandle, hCommonPrefs, cpBefore );

	//////////////////////////////////////////////////////////////////////////
	// set read-only values
	{
		//workspace prefs
		CDispWorkspacePreferences* pWrkSp = cpChanged.getWorkspacePreferences();
		pWrkSp->setDefaultVmDirectory( Uuid::createUuid().toString() );
		pWrkSp->setDistributedDirectory( ! pWrkSp->isDistributedDirectory() );
		pWrkSp->setDispatcherPort( pWrkSp->getDispatcherPort() / 2 );
		pWrkSp->setDefaultCommandHistorySize( pWrkSp->getDefaultCommandHistorySize() * 2 );
		pWrkSp->setVmTimeoutOnShutdown( pWrkSp->getVmTimeoutOnShutdown() / 2 );
		pWrkSp->setAllowUseNetworkShares( ! pWrkSp->isAllowUseNetworkShares() );
		pWrkSp->getLimits()->setMaxLogonActions( pWrkSp->getLimits()->getMaxLogonActions() * 2 );

		CDispMemoryPreferences* pMem = cpChanged.getMemoryPreferences();
		pMem->setMinReservedMemoryLimit( pMem->getMinReservedMemoryLimit() + 100 );
		pMem->setMinReservedMemoryLimit( pMem->getMaxReservedMemoryLimit() + 100 );
		pMem->setMinVmMemory( pMem->getMinVmMemory() + 100 );
		pMem->setMaxVmMemory( pMem->getMaxVmMemory() + 100 );
		pMem->setRecommendedMaxVmMemory( pMem->getRecommendedMaxVmMemory() + 100 );
		pMem->setHostRamSize( pMem->getHostRamSize() + 100 );

		CDispGenericPciDevices* pPci = cpChanged.getPciPreferences()->getGenericPciDevices();
		pPci->m_lstGenericPciDevices.append( new CDispGenericPciDevice );

		//CDispNetworkPreferences* pNetw = cpChanged.getNetworkPreferences();
		//pNetw->addNetAdapter( new CDispNetAdapter );

		CDspDebug* pDebug = cpChanged.getDebug();
		pDebug->setMonitorCCPath( pDebug->getMonitorCCPath() + "AAAAAAAAAA" );
		pDebug->setVtdSetup( pDebug->getVtdSetup() + 10 );
	}



	//////////////////////////////////////////////////////////////////////////
	//Begin/commit server common preferences
	hJob.reset(PrlSrv_CommonPrefsBeginEdit(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob);

	CHECK_RET_CODE_EXP(PrlDispCfg_FromString(hCommonPrefs, cpChanged.toString().toUtf8().data()));
	hJob.reset(PrlSrv_CommonPrefsCommit(m_ServerHandle, hCommonPrefs));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// get common prefs again
	CDispCommonPreferences cpCurrent;
	RECIEVE_COMMON_PREFS( hCommonPrefs, cpCurrent);

	{
#		define ADV_COMPARE( param ) \
	QCOMPARE( cpCurrent. param, cpBefore. param  );
#		define	ADV_COMPARE_WSP( param ) \
	ADV_COMPARE( getWorkspacePreferences()-> param );
#		define	ADV_COMPARE_MEM( param ) \
	ADV_COMPARE( getMemoryPreferences()-> param );
#		define	ADV_COMPARE_PCI( param ) \
	ADV_COMPARE( getPciPreferences()-> param );
#		define	ADV_COMPARE_NET( param ) \
	ADV_COMPARE( getNetworkPreferences()-> param );
#		define	ADV_COMPARE_DBG( param ) \
	ADV_COMPARE( getDebug()-> param );

		ADV_COMPARE_WSP( getDefaultVmDirectory() );
		ADV_COMPARE_WSP( isDistributedDirectory() );
		ADV_COMPARE_WSP( getDispatcherPort() );
		ADV_COMPARE_WSP( getDefaultCommandHistorySize() );
		ADV_COMPARE_WSP( getVmTimeoutOnShutdown() );
		ADV_COMPARE_WSP( isAllowUseNetworkShares() );
		ADV_COMPARE_WSP( getLimits()->getMaxLogonActions() );

		ADV_COMPARE_MEM( getMinReservedMemoryLimit() );
		ADV_COMPARE_MEM( getMaxReservedMemoryLimit() );
		ADV_COMPARE_MEM( getMinVmMemory() );
		ADV_COMPARE_MEM( getMaxVmMemory() );
		ADV_COMPARE_MEM( getRecommendedMaxVmMemory() );
		ADV_COMPARE_MEM( getHostRamSize() );

		ADV_COMPARE_PCI( toString() );
		ADV_COMPARE_NET( toString() );

		ADV_COMPARE_DBG( getMonitorCCPath() );
		ADV_COMPARE_DBG( getVtdSetup() );

		ADV_COMPARE( toString() );

#undef ADV_COMPARE
#undef ADV_COMPARE_WSP
#undef ADV_COMPARE_IPH
#undef ADV_COMPARE_MEM
#undef ADV_COMPARE_PCI
#undef ADV_COMPARE_NET
#undef ADV_COMPARE_DBG
	}

}

void DispFunctionalityTest::testRegenerateVmUuidOnRegisterVm()
{
	// 1. createVm
	// 2. udpate config and store vm uuid
	// 3. unregiser Vm
	// 4. register VM by default way
	//    -- vm_uuid should be same
	// 5. unregister Vm
	// 6. register Vm with 'generate Uuid' flag
	// 7. check that Vm uuid was changed.

	SdkHandleWrap hJob;
	SdkHandleWrap hResult;

	test_login();

	//////////////////////////////////////////////////////////////////////////
	// 1. createVm
	QString firstVmUuid = Uuid::createUuid().toString();
	SET_DEFAULT_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(m_VmHandle, QSTR2UTF8( firstVmUuid ) ) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetUuid(m_VmHandle, QSTR2UTF8( firstVmUuid ) ) );

	hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 2. udpate config and store vm uuid
	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	QString sVmUuid;
	QString sVmUuidNew;
	QString sVmHomePath;

	PRL_EXTRACT_STRING_VALUE(sVmUuid, m_VmHandle, PrlVmCfg_GetUuid);
	PRL_EXTRACT_STRING_VALUE(sVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath);

	//////////////////////////////////////////////////////////////////////////
	// 3. unregister Vm
	hJob.reset(PrlVm_Unreg(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 4.1.1 register VM by default way && check vm_uuid: should be same
	hJob.reset(PrlSrv_RegisterVm( m_ServerHandle, QSTR2UTF8(sVmHomePath), PRL_TRUE ));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()));

	// 4.1.2 check that Vm uuid doesn't changed.
	PRL_EXTRACT_STRING_VALUE(sVmUuidNew, m_VmHandle, PrlVmCfg_GetUuid);
	QCOMPARE(sVmUuidNew, sVmUuid );

	// 4.1.2 unregister Vm
	hJob.reset(PrlVm_Unreg(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);
	//
	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// 4.2.1 register VM by default way BY NEW METHOD && check vm_uuid: should be same
	PRL_UINT32 nFlags = PACF_NON_INTERACTIVE_MODE ;
	hJob.reset(PrlSrv_RegisterVmEx( m_ServerHandle, QSTR2UTF8(sVmHomePath), nFlags ));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()));

	// 4.2.2 check that Vm uuid doesn't changed.
	PRL_EXTRACT_STRING_VALUE(sVmUuidNew, m_VmHandle, PrlVmCfg_GetUuid);
	QCOMPARE(sVmUuidNew, sVmUuid );

	// 4.2.3 unregister Vm
	hJob.reset(PrlVm_Unreg(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 5.1 register Vm with 'generate Uuid' flag and check that vm_uuid is different
	nFlags = PACF_NON_INTERACTIVE_MODE
		| PRVF_REGENERATE_VM_UUID ;

	hJob.reset(PrlSrv_RegisterVmEx( m_ServerHandle, QSTR2UTF8(sVmHomePath), nFlags ));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()));

	// 5.2 check that Vm uuid was changed.
	PRL_EXTRACT_STRING_VALUE(sVmUuidNew, m_VmHandle, PrlVmCfg_GetUuid);
	QVERIFY( sVmUuid != sVmUuidNew );
}

void DispFunctionalityTest::testRegenerateVmSrcUuidOnRegisterVm()
{
	// 1. createVm
	// 2. udpate config and store vm uuid
	// 3. unregiser Vm
	// 4. register VM by default way
	//    -- vm_uuid should be same
	// 5. unregister Vm
	// 6. register Vm with 'generate Source Uuid' flag
	// 7. check that Vm uuid was changed.

	SdkHandleWrap hJob;
	SdkHandleWrap hResult;

	CVmConfiguration vm_conf;

	test_login();

	//////////////////////////////////////////////////////////////////////////
	// 1. createVm
	QString firstVmUuid = Uuid::createUuid().toString();
	SET_DEFAULT_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(m_VmHandle, QSTR2UTF8( firstVmUuid ) ) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetUuid(m_VmHandle, QSTR2UTF8( firstVmUuid ) ) );

	hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 2. udpate config and store vm uuid
	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	QString sVmUuid;
	QString sVmUuidNew;
	QString sVmSrcUuid;
	QString sVmSrcUuidNew;
	QString sVmHomePath;

	PRL_EXTRACT_STRING_VALUE(sVmUuid, m_VmHandle, PrlVmCfg_GetUuid);
	PRL_EXTRACT_STRING_VALUE(sVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath);
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( m_VmHandle, (&vm_conf) );
	sVmSrcUuid = vm_conf.getVmIdentification()->getSourceVmUuid();

	//////////////////////////////////////////////////////////////////////////
	// 3. unregister Vm
	hJob.reset(PrlVm_Unreg(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 4.1.1 register VM by default way && check vm_uuid: should be same
	hJob.reset(PrlSrv_RegisterVm( m_ServerHandle, QSTR2UTF8(sVmHomePath), PRL_TRUE ));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()));

	// 4.1.2 check that Vm uuid and src uuid doesn't changed.
	PRL_EXTRACT_STRING_VALUE(sVmUuidNew, m_VmHandle, PrlVmCfg_GetUuid);
	QCOMPARE(sVmUuidNew, sVmUuid );
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( m_VmHandle, (&vm_conf) );
	sVmSrcUuidNew = vm_conf.getVmIdentification()->getSourceVmUuid();
	QCOMPARE(sVmSrcUuidNew, sVmSrcUuid );

	// 4.1.2 unregister Vm
	hJob.reset(PrlVm_Unreg(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);
	//
	//////////////////////////////////////////////////////////////////////////


	//////////////////////////////////////////////////////////////////////////
	// 4.2.1 register VM by default way BY NEW METHOD && check vm_uuid: should be same
	PRL_UINT32 nFlags = PACF_NON_INTERACTIVE_MODE ;
	hJob.reset(PrlSrv_RegisterVmEx( m_ServerHandle, QSTR2UTF8(sVmHomePath), nFlags ));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()));

	// 4.2.2 check that Vm uuid and src uuid wasn't changed.
	PRL_EXTRACT_STRING_VALUE(sVmUuidNew, m_VmHandle, PrlVmCfg_GetUuid);
	QCOMPARE(sVmUuidNew, sVmUuid );
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( m_VmHandle, (&vm_conf) );
	sVmSrcUuidNew = vm_conf.getVmIdentification()->getSourceVmUuid();
	QCOMPARE(sVmSrcUuidNew, sVmSrcUuid );

	// 4.2.3 unregister Vm
	hJob.reset(PrlVm_Unreg(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 5.1 register Vm with 'generate Uuid' flag and check that vm_uuid is different
	nFlags = PACF_NON_INTERACTIVE_MODE | PRVF_REGENERATE_SRC_VM_UUID ;

	hJob.reset(PrlSrv_RegisterVmEx( m_ServerHandle, QSTR2UTF8(sVmHomePath), nFlags ));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()));

	// 5.2 check that Vm uuid wasn't changed and src uuid was changed.
	PRL_EXTRACT_STRING_VALUE(sVmUuidNew, m_VmHandle, PrlVmCfg_GetUuid);
	QCOMPARE(sVmUuidNew, sVmUuid );
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( m_VmHandle, (&vm_conf) );
	sVmSrcUuidNew = vm_conf.getVmIdentification()->getSourceVmUuid();
	QVERIFY( sVmSrcUuidNew != sVmSrcUuid );
}

void DispFunctionalityTest::testRegisterVmWithCustomUuid()
{
	SdkHandleWrap hJob;
	SdkHandleWrap hResult;
	PRL_UINT32 nFlags = PACF_NON_INTERACTIVE_MODE ;

	test_login();

	//////////////////////////////////////////////////////////////////////////
	// 1. createVm
	QString firstVmUuid = Uuid::createUuid().toString();
	SET_DEFAULT_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(m_VmHandle, QSTR2UTF8( firstVmUuid ) ) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetUuid(m_VmHandle, QSTR2UTF8( firstVmUuid ) ) );

	hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 2. udpate config and store vm uuid
	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	QString sVmUuid;
	QString sVmUuidNew;
	QString sVmHomePath;

	PRL_EXTRACT_STRING_VALUE(sVmUuid, m_VmHandle, PrlVmCfg_GetUuid);
	PRL_EXTRACT_STRING_VALUE(sVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath);

	//////////////////////////////////////////////////////////////////////////
	// 3. unregister Vm
	hJob.reset(PrlVm_Unreg(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 4.1.1 Register Vm with custom uuid
	QString sCustomVmUuid = Uuid::createUuid().toString();
	hJob.reset(PrlSrv_RegisterVmWithUuid( m_ServerHandle, QSTR2UTF8(sVmHomePath),
				QSTR2UTF8(sCustomVmUuid), nFlags));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()));

	// 4.1.2 check Vm uuid
	PRL_EXTRACT_STRING_VALUE(sVmUuidNew, m_VmHandle, PrlVmCfg_GetUuid);
	QCOMPARE(sVmUuidNew, sCustomVmUuid );

	//////////////////////////////////////////////////////////////////////////
	// 4.2.1 Register registered Vm with changed uuid
	QString sNewVmUuid = Uuid::createUuid().toString();
	CHECK_ASYNC_OP_FAILED( PrlSrv_RegisterVmWithUuid( m_ServerHandle, QSTR2UTF8(sVmHomePath),
				QSTR2UTF8(sNewVmUuid), nFlags ),
				PRL_ERR_VM_ALREADY_REGISTERED_VM_PATH );
}

void DispFunctionalityTest::testUnableToRegisterSecondCopyOfCopiedAndRemovedVm()
{
	/*
	https://bugzilla.sw.ru/show_bug.cgi?id=433740
	> Steps to Reproduce from #433740:
	1. Download VM.pvm from host A to host B;
	2. Registered this VM on host B in PD5;
	3. On question "copied or moved" select "copied";
	4. Unregistered this VM on host B;
	5. Copy from host A VM.pvm/config.pvs and paste it on host B/VM.pvm/;
	6. Try to add updated VM.pvm on host B
	*/

	// 1. create vm
	// 2. unregister vm
	// 3. patch config.pvs to replace LastServerUuid field  to another ( to question 'Copied or Moved' )
	// 4. copy vm.pvm to vm-1.pvm

	// 5. register vm.pvm again in interactive mode
	// 6. catch question "copied or moved" and select "copied";
	// 7. (catch another questions )
	// 8. unregister vm

	// 9. try to register vm-1.pvm
	// 10.should be ok

	SdkHandleWrap hJob;
	SdkHandleWrap hResult;

	test_login();


	//////////////////////////////////////////////////////////////////////////
	// 1. createVm
	QString firstVmUuid = Uuid::createUuid().toString();
	SET_DEFAULT_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(m_VmHandle, QSTR2UTF8( firstVmUuid + QTest::currentTestFunction() ) ) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetUuid(m_VmHandle, QSTR2UTF8( firstVmUuid ) ) );

	hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 1.2 udpate config and store vm uuid
	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	QString sVmUuid;
	QString sVmUuidNew;
	QString sVmHomePath;

	PRL_EXTRACT_STRING_VALUE(sVmUuid, m_VmHandle, PrlVmCfg_GetUuid);
	PRL_EXTRACT_STRING_VALUE(sVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath);

	//////////////////////////////////////////////////////////////////////////
	// 2. unregister Vm
	hJob.reset(PrlVm_Unreg(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 3. patch config.pvs to replace LastServerUuid field  to another ( to question 'Copied or Moved' )
	CVmConfiguration vmConfig;
	QFile f(sVmHomePath);
	QVERIFY( PRL_ERR_SUCCESS == vmConfig.loadFromFile( &f ) );
	vmConfig.getVmIdentification()->setLastServerUuid( Uuid::createUuid().toString() );
	QVERIFY( PRL_ERR_SUCCESS == vmConfig.saveToFile(&f) );

	//////////////////////////////////////////////////////////////////////////
	// 4. copy vm.pvm to vm-1.pvm

	// calculate  vm home for new vm
	QString sNewVmHomePath = sVmHomePath;
	int indexToBeginReplace = sNewVmHomePath.lastIndexOf( VMDIR_DEFAULT_BUNDLE_SUFFIX );
	sNewVmHomePath.replace( indexToBeginReplace, 0, "-tmp" );

	DirCopy::copy( QFileInfo(sVmHomePath).path(), QFileInfo(sNewVmHomePath).path() );

	//////////////////////////////////////////////////////////////////////////
	// 5. register vm.pvm again in interactive mode
	SdkHandleWrap hJobRegister(PrlSrv_RegisterVm( m_ServerHandle, QSTR2UTF8(sVmHomePath), PRL_FALSE ));

	// 6. catch question "copied or moved" and select "copied";
	// 7. (catch another questions )
	PRL_UINT64 lTimestampFinish = PrlGetTickCount64() + PrlGetTicksPerSecond()*15;

	bool bAnswered = false;
	while( !bAnswered && lTimestampFinish > PrlGetTickCount64() )
	{
		SdkHandleWrap hQuestion;
		WAIT_ANY_QUESTION(hQuestion, (3*1000/* timeout */) );
		if( !hQuestion.valid() )
			continue;

		PRL_RESULT nQuestionId = PRL_ERR_UNINITIALIZED;
		CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hQuestion, &nQuestionId));
		PRL_RESULT nAnswer;
		if( nQuestionId == PET_QUESTION_VM_COPY_OR_MOVE )
		{
			bAnswered = true;
			nAnswer = PET_ANSWER_COPIED;
		}
		else
		{
			nAnswer = CQuestionHelper::getDefaultAnswer( nQuestionId, TestConfig::getApplicationMode() );
		}

		SdkHandleWrap hAnswer;
		CHECK_RET_CODE_EXP(PrlEvent_CreateAnswerEvent(hQuestion, hAnswer.GetHandlePtr(), nAnswer));
		hJob.reset(PrlSrv_SendAnswer(m_ServerHandle, hAnswer));
		CHECK_JOB_RET_CODE(hJob)
	}//while

	CHECK_JOB_RET_CODE(hJobRegister);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJobRegister, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()));

	// 7.2 refresh new vm uuid
	PRL_EXTRACT_STRING_VALUE(sVmUuid, m_VmHandle, PrlVmCfg_GetUuid);

	// 8. unregister vm
	hJob.reset(PrlVm_Unreg(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	AutoDirRemover autoDirRemover;
	QVERIFY( autoDirRemover.add(QFileInfo(sVmHomePath).path()) );

	// 9. try to register vm-1.pvm
	// 10.should be ok
	hJob.reset(PrlSrv_RegisterVm( m_ServerHandle, QSTR2UTF8(sNewVmHomePath), PRL_TRUE ));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()));

	// 11. do cleanup
	AutoDeleteVm autoDeleteVm2( m_VmHandle );
}

void DispFunctionalityTest::testUnableToCreateVmIfUnexistingInvalidVmIsPresent()
{
	// 1. create vm-1
	// 2. rename vm-1 folder on hdd to vm-1.renamed
	// 3. try to create new VM

	SdkHandleWrap hJob;
	SdkHandleWrap hResult;

	test_login();

	//////////////////////////////////////////////////////////////////////////
	// 1. createVm
	QString firstVmUuid = Uuid::createUuid().toString();
	SET_DEFAULT_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(m_VmHandle, QSTR2UTF8( firstVmUuid ) ) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetUuid(m_VmHandle, QSTR2UTF8( firstVmUuid ) ) );

	hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 1.2 udpate config and store vm uuid
	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	QString sVmUuid;
	QString sVmUuidNew;
	QString sVmHomePath;

	PRL_EXTRACT_STRING_VALUE(sVmUuid, m_VmHandle, PrlVmCfg_GetUuid);
	PRL_EXTRACT_STRING_VALUE(sVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath);

	AutoDeleteVm autoDeleteVm1( m_VmHandle );

	//////////////////////////////////////////////////////////////////////////
	// 2. rename vm-1 folder on hdd to vm-1.renamed
	QDir vmDir;
	QString sOrigVmDir = QFileInfo(sVmHomePath).path();
	QString sNewVmDir = sOrigVmDir + ".renamed";
	QVERIFY( vmDir.rename( sOrigVmDir, sNewVmDir ) );

	AutoRestorer autoRestorer;
	SmartPtr<CVmEvent> pParams( new CVmEvent );
	pParams->addEventParameter( new CVmEventParameter( PVE::String, sOrigVmDir, "original" ) );
	pParams->addEventParameter( new CVmEventParameter( PVE::String, sNewVmDir, "renamed" ) );
	autoRestorer.addRollback( qMakePair( &AutoRestorer::renameDirBack, pParams  ) );

	//////////////////////////////////////////////////////////////////////////
	// 3. try to create new VM
	firstVmUuid = Uuid::createUuid().toString();
	SET_DEFAULT_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(m_VmHandle, QSTR2UTF8( firstVmUuid ) ) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetUuid(m_VmHandle, QSTR2UTF8( firstVmUuid ) ) );

	hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);

	AutoDeleteVm autoDeleteVm2( m_VmHandle );

}

bool DispFunctionalityTest::tryToGetAdministratorCredentials( QString& outAdminLogin, QString& outAdminPassword )
{
	QString sPassword1q2w3e = "1q2w3e";
	// check if current user consist in admin group with 1q2w3e password
	// check root:1q2w3e
	// check special user 'prl_admin_test_user'

	//////////////////////////////////////////////////////////////////////////
	// check if current user consist in admin group with 1q2w3e password
	do{
		CAuthHelper auth;
		if( !auth.AuthUserBySelfProcessOwner() || !auth.isLocalAdministrator() )
			break;
		CAuthHelper auth2( auth.getUserName() );
		if( !auth2.AuthUser( sPassword1q2w3e ) )
			break;

		outAdminLogin = auth.getUserName();
		outAdminPassword = sPassword1q2w3e;
		return true;
	}while(0);

	//////////////////////////////////////////////////////////////////////////
	// check root:1q2w3e
	do{
		CAuthHelper auth("root");
		if( !auth.AuthUser( sPassword1q2w3e ) )
			break;

		outAdminLogin = auth.getUserName();
		outAdminPassword = sPassword1q2w3e;
		return true;
	}while(0);

	//////////////////////////////////////////////////////////////////////////
	// check special user 'prl_admin_test_user'
	do{
		QString adminUser = "prl_admin_test_user";
		QString adminPasswd = "1q2w3e4r5t";

		CAuthHelper auth( adminUser );
		if( !auth.AuthUser( adminPasswd ) )
			break;

		outAdminLogin = adminUser;
		outAdminPassword = adminPasswd;
		return true;
	}while(0);

	return false;
}

void DispFunctionalityTest::testConfirm()
{
	SdkHandleWrap hJob;
	SdkHandleWrap hResult;
	SdkHandleWrap hDispConfig;
	SdkHandleWrap hDispConfigToRevert;
	SdkHandleWrap hConfirmList;
	SdkHandleWrap hConfirmListNew;
	PRL_ALLOWED_VM_COMMAND nCmd;
	AutoRestorer autoRestorer;

	bool bConfirmationByDefaultIsEnabled = false;

	test_login();

	PRL_BOOL bConfirmationModeEnabled;
	CHECK_RET_CODE_EXP( PrlSrv_IsConfirmationModeEnabled(m_ServerHandle, &bConfirmationModeEnabled ) );
	if( bConfirmationByDefaultIsEnabled )
		QVERIFY( bConfirmationModeEnabled );
	else
	{
		QVERIFY( !bConfirmationModeEnabled );

		PRL_UINT32 nFlags = 0;
		hJob.reset(PrlSrv_EnableConfirmationMode( m_ServerHandle, nFlags));
		CHECK_JOB_RET_CODE(hJob);
	}

	QString sAdminUser;
	QString sAdminPassword;
	PRL_UINT32 nFlags = 0;

	if( !tryToGetAdministratorCredentials(sAdminUser, sAdminPassword) )
		nFlags = ISCMF_INTERNAL_TESTS_ONLY_ACCEPT_WITHOUT_ADMINS_ACCESS_CHECK;

	hJob.reset(PrlSrv_DisableConfirmationMode(
		m_ServerHandle, QSTR2UTF8(sAdminUser), QSTR2UTF8(sAdminPassword), nFlags));
	CHECK_JOB_RET_CODE(hJob);

	CHECK_RET_CODE_EXP( PrlSrv_IsConfirmationModeEnabled(m_ServerHandle, &bConfirmationModeEnabled ) );
	QVERIFY( !bConfirmationModeEnabled );

	//--------------
	GET_RESULT_AFTER_ASYNC_CALL_WITH_COPY( (PrlSrv_GetCommonPrefs(m_ServerHandle)), hDispConfig, hDispConfigToRevert );
	CHECK_RET_CODE_EXP(PrlDispCfg_GetConfirmationsList(hDispConfig, hConfirmList.GetHandlePtr()));

	SmartPtr<RollbackOperation> pRollback( new RestoreCommonPrefs( m_ServerHandle, hDispConfigToRevert));
	autoRestorer.addRollback( pRollback );

	// test edit common prefs
	hJob.reset(PrlSrv_CommonPrefsBeginEdit(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob);

	// add test value
	CHECK_RET_CODE_EXP(PrlApi_CreateOpTypeList(hConfirmListNew.GetHandlePtr(), sizeof(PRL_ALLOWED_VM_COMMAND)));
	nCmd = PAR_VM_LOCK_ACCESS;
	CHECK_RET_CODE_EXP(PrlOpTypeList_AddItem(hConfirmListNew, (PRL_CONST_VOID_PTR)&nCmd))
		CHECK_RET_CODE_EXP(PrlDispCfg_SetConfirmationsList(hDispConfig, hConfirmListNew ));

	hJob.reset(PrlSrv_CommonPrefsCommit(m_ServerHandle, hDispConfig));
	CHECK_JOB_RET_CODE(hJob);

	// check that all values are changed
	GET_RESULT_AFTER_ASYNC_CALL( (PrlSrv_GetCommonPrefs(m_ServerHandle)), hDispConfig );
	CHECK_RET_CODE_EXP(PrlDispCfg_GetConfirmationsList(hDispConfig, hConfirmList.GetHandlePtr()));

	QVERIFY( CompareTwoOpTypeLists( hConfirmListNew, hConfirmList, nCmd ) );


	//////////////////////////////////////////////////////////////////////////
	//TODO: test edit vm prefs
	//////////////////////////////////////////////////////////////////////////

	// 1. createVm
	QString firstVmUuid = Uuid::createUuid().toString();
	SET_DEFAULT_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(m_VmHandle, QSTR2UTF8( firstVmUuid ) ) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetUuid(m_VmHandle, QSTR2UTF8( firstVmUuid ) ) );

	hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);

	// 2. set parameters
	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlVmCfg_GetConfirmationsList(m_VmHandle, hConfirmList.GetHandlePtr()));

	// 3.
	// test edit vm prefs
	hJob.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	// add test value
	CHECK_RET_CODE_EXP(PrlApi_CreateOpTypeList(hConfirmListNew.GetHandlePtr(), sizeof(PRL_ALLOWED_VM_COMMAND)));
	nCmd = PAR_VM_UNLOCK_ACCESS;
	CHECK_RET_CODE_EXP(PrlOpTypeList_AddItem(hConfirmListNew, (PRL_CONST_VOID_PTR)&nCmd));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetConfirmationsList(m_VmHandle, hConfirmListNew ));

	hJob.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	// check that all values are changed
	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlVmCfg_GetConfirmationsList(m_VmHandle, hConfirmList.GetHandlePtr()));

	QVERIFY( CompareTwoOpTypeLists( hConfirmListNew, hConfirmList, nCmd ) );

	//////////////////////////////////////////////////////////////////////////
	// test to enable mode again
	nFlags = 0;
	hJob.reset(PrlSrv_EnableConfirmationMode( m_ServerHandle, nFlags));
	CHECK_JOB_RET_CODE(hJob);

	CHECK_RET_CODE_EXP( PrlSrv_IsConfirmationModeEnabled(m_ServerHandle, &bConfirmationModeEnabled ) );
	QVERIFY( bConfirmationModeEnabled );

}

void DispFunctionalityTest::testSetConfirmMode_UnableToDisableWithWrongAdminCredentials()
{
	QSKIP( "Not implemented yet", SkipAll );
}

void DispFunctionalityTest::testSetConfirmMode_UnableToDisableByNotAdmin()
{
	QSKIP( "Not implemented yet", SkipAll );
}

void DispFunctionalityTest::testSetConfirmMode_AllowToEnableByNotAdmin()
{
	QSKIP( "Not implemented yet", SkipAll );
}

void DispFunctionalityTest::testSetConfirmMode_UnableToAlreadyEnableOrDisable()
{
	QSKIP( "Not implemented yet", SkipAll );
}

void DispFunctionalityTest::testConfirm_UnableExecuteOperationWhich_BlockedInCommonList()
{
	QSKIP( "Not implemented yet", SkipAll );
}

void DispFunctionalityTest::testConfirm_UnableExecuteOperationWhich_BlockedInPerVmList()
{
	QSKIP( "Not implemented yet", SkipAll );
}

void DispFunctionalityTest::testConfirm_UnableExecuteOperationWhich_BlockedInCommonList_ButPermittedInPerVmList()
{
	QSKIP( "Not implemented yet", SkipAll );
}

void DispFunctionalityTest::testUnableToRegisterSameVm_WithRegenerateVmUuidFlag()
{
	// 1. create Vm
	// 2. register same vm with Regenerate VmUUID flag.

	//======
	SdkHandleWrap hJob;
	SdkHandleWrap hResult;

	test_login();

	//////////////////////////////////////////////////////////////////////////
	// 1. createVm
	QString firstVmUuid = Uuid::createUuid().toString();
	SET_DEFAULT_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(m_VmHandle, QSTR2UTF8( firstVmUuid + QTest::currentTestFunction() ) ) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetUuid(m_VmHandle, QSTR2UTF8( firstVmUuid ) ) );

	hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 1.2 udpate config and store vm uuid
	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	QString sVmUuid;
	QString sVmHomePath;

	PRL_EXTRACT_STRING_VALUE(sVmUuid, m_VmHandle, PrlVmCfg_GetUuid);
	PRL_EXTRACT_STRING_VALUE(sVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath);

	//////////////////////////////////////////////////////////////////////////
	// 2. register same vm
	PRL_UINT32 nFlags = PACF_NON_INTERACTIVE_MODE;
	CHECK_ASYNC_OP_FAILED( PrlSrv_RegisterVmEx( m_ServerHandle, QSTR2UTF8(sVmHomePath), nFlags ) \
		, PRL_ERR_VM_ALREADY_REGISTERED);

	//////////////////////////////////////////////////////////////////////////
	// 2.1 register same vm with regenerate VmUuid
	nFlags = PACF_NON_INTERACTIVE_MODE | PRVF_REGENERATE_VM_UUID;
	CHECK_ASYNC_OP_FAILED( PrlSrv_RegisterVmEx( m_ServerHandle, QSTR2UTF8(sVmHomePath), nFlags ) \
		, PRL_ERR_VM_ALREADY_REGISTERED);

}

void DispFunctionalityTest::testRegisterVm_WithSameNameAndSameUuid_ShouldRenameBundleToo()
{
	// https://bugzilla.sw.ru/show_bug.cgi?id=440983
	// testcase:
	// 1. create Vm 'test'
	// 2. create dir and copy this vm to it.
	// 3. register copied vm
	// 4. check that it name changed to 'test (1)' and vm_bundle has same name (  'test (1).pvm')

	SdkHandleWrap hJob;
	SdkHandleWrap hResult;

	test_login();


	//////////////////////////////////////////////////////////////////////////
	// 1. createVm
	QString firstVmUuid = Uuid::createUuid().toString();
	SET_DEFAULT_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(m_VmHandle, QSTR2UTF8( GEN_VM_NAME_BY_TEST_FUNCTION() ) ) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetUuid(m_VmHandle, QSTR2UTF8( firstVmUuid ) ) );

	hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 1.2 udpate config and store vm uuid
	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	QString sVmName;
	QString sVmHomePath;

	PRL_EXTRACT_STRING_VALUE(sVmName, m_VmHandle, PrlVmCfg_GetName);
	PRL_EXTRACT_STRING_VALUE(sVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath);

	//////////////////////////////////////////////////////////////////////////
	// 2. create dir and copy this vm to it.
	SdkHandleWrap hVmHandle2;

	QString tmpDir = QString("%1/%2").arg( QDir::tempPath() ).arg( Uuid::createUuid().toString() );
	QVERIFY( QDir().mkdir(tmpDir) );
	AutoDirRemover autoRemover;
	autoRemover.add( tmpDir );

	QVERIFY( DirCopy::copy( QFileInfo(sVmHomePath).path(), tmpDir) );
	QString pathToSecondVm = QString("%1/%2").arg(tmpDir).arg( QFileInfo(sVmHomePath).dir().dirName() );

	// 3. register copied vm
	hJob.reset(PrlSrv_RegisterVm( m_ServerHandle, QSTR2UTF8(pathToSecondVm), PRL_TRUE ));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmHandle2.GetHandlePtr()));

	AutoDeleteVm autoDeleteVm( hVmHandle2 );

	//////////////////////////////////////////////////////////////////////////
	//3.2 udpate config and store vm uuid
	hJob.reset(PrlVm_RefreshConfig(hVmHandle2));
	CHECK_JOB_RET_CODE(hJob);

	QString sVmName2;
	QString sVmHomePath2;

	PRL_EXTRACT_STRING_VALUE(sVmName2, hVmHandle2, PrlVmCfg_GetName);
	PRL_EXTRACT_STRING_VALUE(sVmHomePath2, hVmHandle2, PrlVmCfg_GetHomePath);

	// 4. check that it name changed to 'test (1)' and vm_bundle has same name (  'test (1).pvm')

	QVERIFY( sVmName2 != sVmName );
	QVERIFY( sVmName2.contains(sVmName) );

	QString sBundleName = QFileInfo( sVmHomePath2 ).dir().dirName();
	QCOMPARE( sBundleName, (sVmName2 + VMDIR_DEFAULT_BUNDLE_SUFFIX) );
}

void DispFunctionalityTest::testConfigWatcher_FileWasChanged()
{
	QSKIP( "Not implemented yet", SkipAll );
}

void DispFunctionalityTest::testConfigWatcher_FileWasChangedThroughReplace()
{
	QSKIP( "Not implemented yet", SkipAll );
}

void DispFunctionalityTest::testConfigWatcher_FileWasMoved()
{
	QSKIP( "Not implemented yet", SkipAll );
}

void DispFunctionalityTest::testConfigWatcher_FileWasRemoved()
{
	QSKIP( "Not implemented yet", SkipAll );
}

void DispFunctionalityTest::testConfigWatcher_VmDirWasMoved()
{
	QSKIP( "Not implemented yet", SkipAll );
}

void DispFunctionalityTest::testConfigWatcher_VmDirWasDeleted()
{
	QSKIP( "Not implemented yet", SkipAll );
}

void DispFunctionalityTest::testConfigWatcher_VmDirWasChanged()
{
	QSKIP( "Not implemented yet", SkipAll );
}

void DispFunctionalityTest::testConfigWatcher_AfterEditVm()
{
	QSKIP( "Not implemented yet", SkipAll );
}

void DispFunctionalityTest::testConfigWatcher_AfterAddNewVm()
{
	QSKIP( "Not implemented yet", SkipAll );
}

void DispFunctionalityTest::testConfigWatcher_AfterDeleteVm()
{
	QSKIP( "Not implemented yet", SkipAll );
}

void DispFunctionalityTest::testLostVmPermissionAfterStartByAdmin()
{
	// 	1. Login as user
	//	2. Create Vm
	//	3. check to start
	//	4. login as admin, check to start
	//	5. start as user again

	QSKIP( "Not implemented yet", SkipAll );
}

#define CHECK_VM_INFO_FIELDS( _hVmInfo ) \
{ \
	CVmEvent vmInfo; \
	EXTRACT_HANDLE_AS_XML_MODEL_OBJECT( _hVmInfo, vmInfo); \
	\
	QVERIFY( vmInfo.getEventParameter( EVT_PARAM_VMINFO_VM_IS_INVALID ) ); \
	QVERIFY( vmInfo.getEventParameter( EVT_PARAM_VMINFO_VM_STATE ) ); \
	QVERIFY( vmInfo.getEventParameter( EVT_PARAM_VMINFO_VM_SECURITY ) ); \
	QVERIFY( vmInfo.getEventParameter( EVT_PARAM_VMINFO_IS_VNC_SERVER_STARTED ) ); \
	QVERIFY( vmInfo.getEventParameter( EVT_PARAM_VMINFO_VM_ADDITION_STATE ) ); \
	QVERIFY( vmInfo.getEventParameter( EVT_PARAM_VMINFO_IS_VM_WAITING_FOR_ANSWER ) ); \
	QVERIFY( vmInfo.getEventParameter( EVT_PARAM_VMINFO_VM_ADDITION_STATE ) ); \
	QVERIFY( vmInfo.getEventParameter( EVT_PARAM_VMINFO_IS_VM_WAITING_FOR_ANSWER ) ); \
}

#define CLEANUP_VM_EVENT_UNIQUE_FIELDS_BEFORE_COMPARE( _vmEvent ) \
{ \
	_vmEvent.setInitRequestId(); \
	_vmEvent.setEventIssuerId(); \
}

void DispFunctionalityTest::testGetVmInfo()
{
	// 1. create Vm
	// 2. getVmInfo / get VmConfig / getVmlist
	// 3. checkVmInfo fields
	SdkHandleWrap hResult;
	SdkHandleWrap hJob;
	SdkHandleWrap hVmInfo;

	// 0. login
	hJob.reset(PrlSrv_LoginLocal(m_ServerHandle, "" , 0, PSL_HIGH_SECURITY));
	CHECK_JOB_RET_CODE( hJob );

	// 1. create VM
	SET_DEFAULT_CONFIG(m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(m_VmHandle, QSTR2UTF8( GEN_VM_NAME_BY_TEST_FUNCTION() ) ) );

	hJob.reset(PrlVm_Reg( m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);

	AutoDeleteVm autoDeleteVm( m_VmHandle );

	//////////////////////////////////////////////////////////////////////////
	// 1.2 udpate config
	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	QString sVmUuid;
	PRL_EXTRACT_STRING_VALUE(sVmUuid, m_VmHandle, PrlVmCfg_GetUuid);
	WRITE_TRACE( DBG_DEBUG, "sVmUuid = \n%s", QSTR2UTF8(sVmUuid) );

	////////////////////////////////////////////////////
	// 2.1 from GetVmInfo
	hJob.reset(PrlVm_GetState(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmInfo.GetHandlePtr()))
		VIRTUAL_MACHINE_STATE nVmState;
	CHECK_RET_CODE_EXP(PrlVmInfo_GetState(hVmInfo, &nVmState))
		QVERIFY(nVmState == VMS_STOPPED);

	CHECK_VM_INFO_FIELDS( hVmInfo );

	CVmEvent evtVmInfoEtalone;
	EXTRACT_HANDLE_AS_XML_MODEL_OBJECT( hVmInfo, evtVmInfoEtalone);
	CLEANUP_VM_EVENT_UNIQUE_FIELDS_BEFORE_COMPARE(evtVmInfoEtalone);

	////////////////////////////////////////////////////
	// 2.2 from GetVmConfig
	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP( PrlVmCfg_GetVmInfo(m_VmHandle, hVmInfo.GetHandlePtr() ) );

	CVmEvent evtVmInfo;
	EXTRACT_HANDLE_AS_XML_MODEL_OBJECT( hVmInfo, evtVmInfo);
	CLEANUP_VM_EVENT_UNIQUE_FIELDS_BEFORE_COMPARE(evtVmInfo);

	if( evtVmInfo.toString() != evtVmInfoEtalone.toString())
	{
		WRITE_TRACE( DBG_DEBUG, "evtVmInfoEtalone.toString() = \n%s", QSTR2UTF8(evtVmInfoEtalone.toString()) );
		WRITE_TRACE( DBG_DEBUG, "evtVmInfo.toString() = \n%s", QSTR2UTF8( evtVmInfo.toString() ));
	}
	QVERIFY( evtVmInfo.toString() == evtVmInfoEtalone.toString() );

	////////////////////////////////////////////////////
	// 2.3 from GetVmList
	hJob.reset(PrlSrv_GetVmList(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob);

	SdkHandleWrap hFoundVm;
	TRY_GET_VM_AFTER_GET_VM_LIST( hJob, sVmUuid, hFoundVm );
	QVERIFY(hFoundVm != PRL_INVALID_HANDLE);
	m_VmHandle.reset(hFoundVm);

	PRL_EXTRACT_STRING_VALUE(sVmUuid, m_VmHandle, PrlVmCfg_GetUuid);
	WRITE_TRACE( DBG_DEBUG, "sVmUuid = \n%s", QSTR2UTF8(sVmUuid) );

	CHECK_RET_CODE_EXP( PrlVmCfg_GetVmInfo(m_VmHandle, hVmInfo.GetHandlePtr() ) );

	EXTRACT_HANDLE_AS_XML_MODEL_OBJECT( hVmInfo, evtVmInfo);
	CLEANUP_VM_EVENT_UNIQUE_FIELDS_BEFORE_COMPARE(evtVmInfo);

	if( evtVmInfo.toString() != evtVmInfoEtalone.toString())
	{
		WRITE_TRACE( DBG_DEBUG, "evtVmInfoEtalone.toString() = \n%s", QSTR2UTF8(evtVmInfoEtalone.toString()) );
		WRITE_TRACE( DBG_DEBUG, "evtVmInfo.toString() = \n%s", QSTR2UTF8( evtVmInfo.toString() ));
	}
	QVERIFY( evtVmInfo.toString() == evtVmInfoEtalone.toString() );
}

void DispFunctionalityTest::testStoreValueByKey_InstalledSoftwareId()
{
	SdkHandleWrap hJob;
	SdkHandleWrap hDispConfig;
	CVmConfiguration vmConf;

	test_login();

	//////////////////////////////////////////////////////////////////////////
	// 1. createVm
	SET_DEFAULT_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(m_VmHandle, QTest::currentTestFunction() ) );

	hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);

	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	CONVERT_VMCONF_HANDLE_TO_XML_OBJECT( m_VmHandle, vmConf );
	unsigned int nExpected = vmConf.getInstalledSoftware() | 1;
	hJob.reset(PrlVm_StoreValueByKey( m_VmHandle, PRL_KEY_TO_STORE_VM_INSTALLED_SOFTWARE_ID
		, QSTR2UTF8( QString("%1").arg( nExpected ) ), 0 ));
	CHECK_JOB_RET_CODE(hJob);

	//check
	hJob.reset(PrlVm_RefreshConfig( m_VmHandle ));
	CHECK_JOB_RET_CODE(hJob)

		CONVERT_VMCONF_HANDLE_TO_XML_OBJECT( m_VmHandle, vmConf );
	QCOMPARE( quint32( nExpected ),\
		quint32( vmConf.getInstalledSoftware() ) );
}

void DispFunctionalityTest::testStoreValueByKey_InstalledSoftwareIdOnWrongParams()
{
	SdkHandleWrap hJob;
	SdkHandleWrap hDispConfig;
	CVmConfiguration vmConf;

	test_login();

	//////////////////////////////////////////////////////////////////////////
	// 1. createVm
	SET_DEFAULT_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(m_VmHandle, QTest::currentTestFunction() ) );

	hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);

	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	CHECK_ASYNC_OP_FAILED(PrlVm_StoreValueByKey( m_ServerHandle, \
		PRL_KEY_TO_STORE_VM_INSTALLED_SOFTWARE_ID \
		, QSTR2UTF8( QString("%1").arg(-1) ), 0 ), PRL_ERR_INVALID_ARG);

	CHECK_ASYNC_OP_FAILED(PrlVm_StoreValueByKey( m_VmHandle, \
		PRL_KEY_TO_STORE_VM_INSTALLED_SOFTWARE_ID, "qqq", 0) \
		, PRL_ERR_INVALID_ARG);
}

void DispFunctionalityTest::createVm(const QString& sVmName
									 , bool& bRes /*out*/
									 , SdkHandleWrap& hVmHandle/*out*/
									 , QString& sVmUuid /*out*/ )
{
	return createVm( m_ServerHandle, sVmName, bRes, hVmHandle, sVmUuid );
}

void DispFunctionalityTest::createVm(const SdkHandleWrap& hServerHandle
									 , const QString& sVmName
									 , bool& bRes /*out*/
									 , SdkHandleWrap& hVmHandle/*out*/
									 , QString& sVmUuid /*out*/ )
{
	bRes = false;
	SdkHandleWrap hJob;

	//////////////////////////////////////////////////////////////////////////
	// 1. createVm
	SET_DEFAULT_CONFIG_EX( hServerHandle, sVmName, hVmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_TRUE );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(hVmHandle, QSTR2UTF8(sVmName) ) );
	// set minimal ram to snapshots
	CHECK_RET_CODE_EXP( PrlVmCfg_SetRamSize(hVmHandle, 260 ) );

	hJob.reset(PrlVm_Reg(hVmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);

	hJob.reset(PrlVm_RefreshConfig(hVmHandle));
	CHECK_JOB_RET_CODE(hJob);
	PRL_EXTRACT_STRING_VALUE(sVmUuid, hVmHandle, PrlVmCfg_GetUuid);

	bRes = true;
}

void DispFunctionalityTest::AddPlainHdd( const SdkHandleWrap& hVmHandle, quint32 nHddSizeMb, bool& bRes /*out*/ )
{
	SdkHandleWrap hVmDev;
	return AddHdd( bRes, hVmHandle, nHddSizeMb, PHD_PLAIN_HARD_DISK, m_hSrvConfig, hVmDev );
}

void DispFunctionalityTest::AddExpandingHdd( const SdkHandleWrap& hVmHandle, quint32 nHddSizeMb, bool& bRes /*out*/ )
{
	SdkHandleWrap hVmDev;
	return AddHdd( bRes, hVmHandle, nHddSizeMb, PHD_EXPANDING_HARD_DISK, m_hSrvConfig, hVmDev );
}

void DispFunctionalityTest::AddHdd( bool& bRes, const SdkHandleWrap& hVmHandle, quint32 nHddSizeMb
								   , PRL_HARD_DISK_INTERNAL_FORMAT type, SdkHandleWrap& hSrvConfig
								   , SdkHandleWrap& hHddHandle )
{
	bRes = false;
	SdkHandleWrap hJob;

	CHECK_JOB_RET_CODE( PrlVm_BeginEdit( hVmHandle ) );

	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( hVmHandle, hSrvConfig, PDE_HARD_DISK, hHddHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskType(hHddHandle, type));
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskSize(hHddHandle, nHddSizeMb ));

	CHECK_JOB_RET_CODE( PrlVmDev_CreateImage( hHddHandle, PRL_TRUE, PRL_TRUE ) );

	// PATCH HDD PATHES to ABSOLUTE
	QString sVmHomePath;
	PRL_EXTRACT_STRING_VALUE(sVmHomePath, hVmHandle, PrlVmCfg_GetHomePath);
	sVmHomePath = QFileInfo(sVmHomePath).absolutePath();

	QString sHddPath;
	PRL_EXTRACT_STRING_VALUE( sHddPath, hHddHandle, PrlVmDev_GetSysName );
	if( !QFileInfo(sHddPath).isAbsolute() )
		sHddPath = sVmHomePath + "/" + sHddPath;

	CHECK_RET_CODE_EXP( PrlVmDev_SetSysName(hHddHandle, QSTR2UTF8( sHddPath )) );
	CHECK_RET_CODE_EXP( PrlVmDev_SetImagePath(hHddHandle, QSTR2UTF8( sHddPath )) );

	CHECK_JOB_RET_CODE( PrlVm_Commit( hVmHandle ) );

	// refresh config to get absolute paths to hdds
	// doesn't work !
	// CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) );

	bRes = true;
}

void DispFunctionalityTest::testGetVmConfig_FromVmEditBeginResponse()
{
	// 1. create vm
	// 2. editBegin/ editCommit
	// 3. get vmConfig from both responses

	SdkHandleWrap hJob;

	test_login();

	//////////////////////////////////////////////////////////////////////////
	// create vm
	bool bRes = false;
	QString sVmUuid;
	SdkHandleWrap hVmHandle;
	createVm( GEN_VM_NAME_BY_TEST_FUNCTION(), bRes, hVmHandle, sVmUuid );
	AutoDeleteVm autoDeleter(hVmHandle);
	QVERIFY( bRes );

	hJob.reset(PrlVm_BeginEdit( hVmHandle ));
	CHECK_JOB_RET_CODE( hJob);

	SdkHandleWrap hResult, hVm;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVm.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hVm, PHT_VIRTUAL_MACHINE);
}

void DispFunctionalityTest::testGetVmConfig_FromVmEditCommitResponse()
{
	// 1. create vm
	// 2. editBegin/ editCommit
	// 3. get vmConfig from both responses

	SdkHandleWrap hJob;

	test_login();

	//////////////////////////////////////////////////////////////////////////
	// create vm
	bool bRes = false;
	QString sVmUuid;
	SdkHandleWrap hVmHandle;
	createVm( GEN_VM_NAME_BY_TEST_FUNCTION(), bRes, hVmHandle, sVmUuid );
	AutoDeleteVm autoDeleter(hVmHandle);
	QVERIFY( bRes );

	hJob.reset(PrlVm_BeginEdit( hVmHandle ));
	CHECK_JOB_RET_CODE( hJob);

	PRL_UINT32 nVmRamSize;
	CHECK_RET_CODE_EXP( PrlVmCfg_GetRamSize(hVmHandle, &nVmRamSize ) );

	PRL_UINT32 nNewVmRamSize = nVmRamSize+4;
	CHECK_RET_CODE_EXP( PrlVmCfg_SetRamSize(hVmHandle, nNewVmRamSize ) );

	hJob.reset(PrlVm_Commit( hVmHandle ));
	CHECK_JOB_RET_CODE( hJob);

	SdkHandleWrap hResult, hVm;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVm.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hVm, PHT_VIRTUAL_MACHINE);

	CHECK_RET_CODE_EXP( PrlVmCfg_GetRamSize(hVmHandle, &nVmRamSize ) );
	QCOMPARE( nVmRamSize, nNewVmRamSize );
}

void DispFunctionalityTest::testGetVmConfig_FromOldVmEditCommitResponse()
{

	SdkHandleWrap hJob, hResult, hVm;

	test_login();

	// check for old versions( without vm config in response)

	// Fake call only to init hResult
	hJob.reset(PrlVm_Commit(PRL_INVALID_HANDLE));
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT));
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));

	using namespace Parallels;
	CProtoCommandPtr pCmd
		= CProtoSerializer::CreateDspWsResponseCommand( PVE::DspCmdDirVmEditCommit, PRL_ERR_SUCCESS );
	CHECK_RET_CODE_EXP( PrlResult_FromString( hResult, QSTR2UTF8(pCmd->GetCommand()->toString()) ) );
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlResult_GetParam(hResult, hVm.GetHandlePtr()), PRL_ERR_INVALID_ARG);
}

void DispFunctionalityTest::testLicenseInfo_FromUpdateLicenseResponse()
{
	// 1. update license
	// 2. get licenseInfo from response as parameter

	SKIP_IF_EXTERNAL_BUILD

	SimpleServerWrapper session(0);
	AutoRestoreLicenseCreds restoreLicenseCreds(session);

	QString username("unit-test");
	QString company("parallels");

	test_login();
	SET_LICENSE_USER_AND_COMPANY(m_ServerHandle, username, company)

	// store license parameters ( name, company )
	GET_LICENSE_INFO

	QString sLicenseKey, sUserName, sCompanyName;
	PRL_EXTRACT_STRING_VALUE(sLicenseKey, hLicense, PrlLic_GetLicenseKey);
	PRL_EXTRACT_STRING_VALUE(sUserName, hLicense, PrlLic_GetUserName);
	PRL_EXTRACT_STRING_VALUE(sCompanyName, hLicense, PrlLic_GetCompanyName);

	// Update license
	SET_LICENSE_USER_AND_COMPANY(m_ServerHandle, sUserName, sCompanyName)

	{
		// get response
		GET_LICENSE_INFO

		// check that parameters is the same
		QString sLicenseKey2, sUserName2, sCompanyName2;
		PRL_EXTRACT_STRING_VALUE(sLicenseKey2, hLicense, PrlLic_GetLicenseKey);
		PRL_EXTRACT_STRING_VALUE(sUserName2, hLicense, PrlLic_GetUserName);
		PRL_EXTRACT_STRING_VALUE(sCompanyName2, hLicense, PrlLic_GetCompanyName);

		QCOMPARE( sLicenseKey2, sLicenseKey );
		QCOMPARE( sUserName2, sUserName );
		QCOMPARE( sCompanyName2, sCompanyName );
	}
}

void DispFunctionalityTest::testLicenseInfo_FromUpdateLicenseResponseOld()
{
	// check for old versions( without licenseInfo in response)

	SdkHandleWrap hJob, hResult, hLicense;

	test_login();

	// Fake call only to init hResult
	hJob.reset(PrlSrv_UpdateLicense(PRL_INVALID_HANDLE, "", "", "" ));
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT));
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));

	using namespace Parallels;
	CProtoCommandPtr pCmd
		= CProtoSerializer::CreateDspWsResponseCommand(PVE::DspCmdUserUpdateLicense, PRL_ERR_SUCCESS );
	CHECK_RET_CODE_EXP( PrlResult_FromString( hResult, QSTR2UTF8(pCmd->GetCommand()->toString()) ) );
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlResult_GetParam(hResult, hLicense.GetHandlePtr()), PRL_ERR_INVALID_ARG);
}


void DispFunctionalityTest::testAtomicEdit_EVT_PARAM_VMCFG_DEVICE_CONFIG_WITH_NEW_STATE()
{
	// 1. create VM  with some device
	// 2. start VM
	// 3.1 call PrlVmDev_Connect() / PrlVmDev_Disonnect()
	// 3.2 get config and check that connected value changed
	// 4.1 set disconnect to device
	// 4.2 start VM
	// 4.3 change device type and call connect
	// 4.4 get config and check that connected value changed
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	// 1. createVm
	SdkHandleWrap hVm, hVmDev;
	SimpleServerWrapper session(0);

	SET_DEFAULT_CONFIG_EX( session, GEN_VM_NAME_BY_TEST_FUNCTION()
		, hVm, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);

	CHECK_RET_CODE_EXP( PrlVmCfg_CreateVmDev(hVm, PDE_SERIAL_PORT, hVmDev.GetHandlePtr()) );
	CHECK_RET_CODE_EXP( PrlVmDev_SetEnabled( hVmDev, PRL_TRUE ) );
	CHECK_RET_CODE_EXP( PrlVmDev_SetConnected( hVmDev, PRL_TRUE ) );
	CHECK_RET_CODE_EXP( PrlVmDev_SetEmulatedType( hVmDev, PDT_USE_OUTPUT_FILE ) );

	QString serialName = QString("serial-%1.txt").arg(qrand() );
	CHECK_RET_CODE_EXP( PrlVmDev_SetImagePath(hVmDev, QSTR2UTF8(serialName)) );

	PRL_UINT32 nIndex;
	CHECK_RET_CODE_EXP( PrlVmDev_GetIndex( hVmDev, &nIndex ) );

	CHECK_JOB_RET_CODE( PrlVm_Reg(hVm, "", PRL_TRUE) );
	AutoDeleteVm rmVm(hVm);

	//////////////////////////////////////////////////////////////////////////
	// 2 start Vm
	CHECK_JOB_RET_CODE( PrlVm_Start(hVm) );
	AutoStopVm autoStop(hVm);

	PRL_BOOL bTmp;
	//////////////////////////////////////////////////////////////////////////
	// 3.1 call PrlVmDev_Connect() / PrlVmDev_Disonnect()
	// update device state
	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVm) );
	CHECK_RET_CODE_EXP( PrlVmCfg_GetDevByType( hVm, PDE_SERIAL_PORT, nIndex, hVmDev.GetHandlePtr() ) );
	CHECK_RET_CODE_EXP( PrlVmDev_IsConnected( hVmDev, &bTmp ) );
	QCOMPARE( bTmp, (PRL_BOOL)PRL_TRUE );

	CHECK_JOB_RET_CODE( PrlVmDev_Disconnect( hVmDev ) );

	// 3.2 get config and check that connected value changed
	// update device state
	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVm) );

	CHECK_RET_CODE_EXP( PrlVmCfg_GetDevByType( hVm, PDE_SERIAL_PORT, nIndex, hVmDev.GetHandlePtr() ) );
	CHECK_RET_CODE_EXP( PrlVmDev_IsConnected( hVmDev, &bTmp ) );
	QCOMPARE( bTmp, (PRL_BOOL)PRL_FALSE );

	// 4. TODO:
	// 4.1 set disconnect to device
	// 4.3 change device type and call connect
	// 4.4 get config and check that connected value changed
}

void DispFunctionalityTest::testRemoteDisplayPasswordLength()
{
	// 1. create Vm.
	//
	// 2. set long VNC password (SHOULD BE ERROR).
	//
	// 3. set VNC password with length equal to PRL_VM_REMOTE_DISPLAY_MAX_PASS_LEN.
	//
	// 4. set VNC password with length less then PRL_VM_REMOTE_DISPLAY_MAX_PASS_LEN.
	//////////////////////////////////////////////////////////////////////////

	SdkHandleWrap hJob;
	SdkHandleWrap hResult;

	test_login();

	//////////////////////////////////////////////////////////////////////////
	// 1. create Vm
	QString firstVmUuid = Uuid::createUuid().toString();
	SET_DEFAULT_CONFIG( m_VmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(m_VmHandle, QSTR2UTF8( GEN_VM_NAME_BY_TEST_FUNCTION() ) ) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetUuid(m_VmHandle, QSTR2UTF8( firstVmUuid ) ) );

	hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 2. set long VNC password.
	hJob.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	QString longPw = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP( PrlVmCfg_SetVNCPassword(m_VmHandle, QSTR2UTF8(longPw) ) );

	hJob.reset(PrlVm_Commit(m_VmHandle));
	CHECK_ASYNC_OP_FAILED(hJob, PRL_ERR_VMCONF_REMOTE_DISPLAY_PASSWORD_TOO_LONG);

	//////////////////////////////////////////////////////////////////////////
	// 3. set VNC pasword with length quial to PRL_VM_REMOTE_DISPLAY_MAX_PASS_LEN.
	QString maxPw = Uuid::createUuid().toString().left(PRL_VM_REMOTE_DISPLAY_MAX_PASS_LEN);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetVNCPassword(m_VmHandle, QSTR2UTF8(maxPw) ) );

	hJob.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	//////////////////////////////////////////////////////////////////////////
	// 4. set VNC pasword with length less then PRL_VM_REMOTE_DISPLAY_MAX_PASS_LEN.
	hJob.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	QString shortPw = Uuid::createUuid().toString().left(PRL_VM_REMOTE_DISPLAY_MAX_PASS_LEN - 1);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetVNCPassword(m_VmHandle, QSTR2UTF8(shortPw) ) );

	hJob.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)
}

void DispFunctionalityTest::testMergeVmConfig_MergeDifferentFields()
{
	// test with two sessions

	// 1. create Vm
	// 2.1 send beginEdit by session 1
	// 2.2 send beginEdit by session 2
	// 3.1 change param-1 and apply by Editcommit by session 1
	// 3.2 change param-1 and apply by Editcommit by session 2 ==> CONFLICT
	// 4.1 check param-1 - should be equal to setted in 3.1
	// 5.1 change param-2 and apply by Editcommit by session 2
	// 6.1. check param-1 - should be equal to setted in 3.1
	// 6.1 check param-2 - should be equal to setted in 5.1

	SdkHandleWrap hJob;
	SdkHandleWrap hResult;
	SdkHandleWrap hVmHandle, hVmHandle2;
	SdkHandleWrap hSession2;
	bool bRes;

	test_login();

	SimpleServerWrapper session2(0);
	QVERIFY( session2.IsConnected() );
	hSession2 = session2.GetServerHandle();

	//////////////////////////////////////////////////////////////////////////
	// 1. create Vm
	QString sVmUuid;
	bRes = false;
	createVm( GEN_VM_NAME_BY_TEST_FUNCTION(), bRes, hVmHandle, sVmUuid );
	AutoDeleteVm autoDeleter(hVmHandle);
	QVERIFY( bRes );
	bRes = false;

	hVmHandle2 = session2.GetVmByUuid( sVmUuid );
	QVERIFY( hVmHandle2!=PRL_INVALID_HANDLE );

	//////////////////////////////////////////////////////////////////////////
	// 2.1 send beginEdit by session 1
	// 2.2 send beginEdit by session 2
	hJob.reset(PrlVm_BeginEdit(hVmHandle));
	CHECK_JOB_RET_CODE(hJob);
	hJob.reset(PrlVm_BeginEdit(hVmHandle2));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 3.1 change param-1 and apply by Editcommit by session 1
	// 3.2 change param-1 and apply by Editcommit by session 2 ==> conflict
	QString param1 = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP( PrlVmCfg_SetDescription(hVmHandle, QSTR2UTF8(param1) ) );
	hJob.reset(PrlVm_Commit(hVmHandle));
	CHECK_JOB_RET_CODE( hJob );

	QString param2 = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP( PrlVmCfg_SetDescription(hVmHandle2, QSTR2UTF8(param2) ) );
	CHECK_ASYNC_OP_FAILED( PrlVm_Commit(hVmHandle2), PRL_ERR_VM_CONFIG_WAS_CHANGED );

	//rollback changes
	CHECK_RET_CODE_EXP( PrlVmCfg_SetDescription(hVmHandle2, QSTR2UTF8(param1) ) );

	//////////////////////////////////////////////////////////////////////////
	// 4.1 check param-1 - should be equal to setted in 3.1
	hJob.reset(PrlVm_RefreshConfig( hVmHandle ));
	CHECK_JOB_RET_CODE(hJob);

	QString sVal;
	PRL_EXTRACT_STRING_VALUE(sVal, hVmHandle, PrlVmCfg_GetDescription);
	QCOMPARE( sVal, param1 );

	//////////////////////////////////////////////////////////////////////////
	// 5.1 change param-2 and apply by Editcommit by session 2
	QString param3 = Uuid::createUuid().toString().left(PRL_VM_REMOTE_DISPLAY_MAX_PASS_LEN);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetVNCPassword(hVmHandle2, QSTR2UTF8(param3) ) );
	hJob.reset(PrlVm_Commit(hVmHandle2));
	CHECK_JOB_RET_CODE( hJob );

	//////////////////////////////////////////////////////////////////////////
	// 6.1. check param-1 - should be equal to setted in 3.
	// 6.2 check param-2 - should be equal to setted in 5.1.
	hJob.reset(PrlVm_RefreshConfig( hVmHandle ));
	CHECK_JOB_RET_CODE(hJob);

	PRL_EXTRACT_STRING_VALUE(sVal, hVmHandle, PrlVmCfg_GetDescription);
	QCOMPARE( sVal, param1 );
	PRL_EXTRACT_STRING_VALUE(sVal, hVmHandle, PrlVmCfg_GetVNCPassword);
	QCOMPARE( sVal, param3 );
}

void DispFunctionalityTest::testMergeVmConfig_MergeWholeSection()
{
	// test with two sessions

	// 1. create Vm
	// 2.1 send beginEdit by session 1
	// 2.2 send beginEdit by session 2
	// 3.1 change section-1 param-1 and apply by Editcommit by session 1
	// 3.2 change section-1 param-2 and apply by Editcommit by session 2 ==> CONFLICT
	// 4.1 check section-1 - should be equal to setted in 3.1
	// 5.1 change section-2 param-3 and apply by Editcommit by session 2
	// 6.1 check section-1 param-1 - should be equal to setted in 3.1
	// 6.1 check section-2 param-3 - should be equal to setted in 5.1

	SdkHandleWrap hJob;
	SdkHandleWrap hResult;
	SdkHandleWrap hVmHandle, hVmHandle2;
	SdkHandleWrap hSession2;
	bool bRes;

	test_login();


	SimpleServerWrapper session2(0);
	QVERIFY( session2.IsConnected() );
	hSession2 = session2.GetServerHandle();

	//////////////////////////////////////////////////////////////////////////
	// 1. create Vm
	QString sVmUuid;
	bRes = false;
	createVm( GEN_VM_NAME_BY_TEST_FUNCTION(), bRes, hVmHandle, sVmUuid );
	AutoDeleteVm autoDeleter(hVmHandle);
	QVERIFY( bRes );
	bRes = false;

	AddExpandingHdd( hVmHandle, 5000, bRes );
	QVERIFY( bRes );
	bRes = false;

	AddExpandingHdd( hVmHandle, 5000, bRes );
	QVERIFY( bRes );
	bRes = false;

	hVmHandle2 = session2.GetVmByUuid( sVmUuid );
	QVERIFY( hVmHandle2!=PRL_INVALID_HANDLE );

	//////////////////////////////////////////////////////////////////////////
	// 2.1 send beginEdit by session 1
	// 2.2 send beginEdit by session 2
	hJob.reset(PrlVm_BeginEdit(hVmHandle));
	CHECK_JOB_RET_CODE(hJob);
	hJob.reset(PrlVm_BeginEdit(hVmHandle2));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 3.1 change section-1 param-1 and apply by Editcommit by session 1
	// 3.2 change section-1 param-2 and apply by Editcommit by session 2 ==> CONFLICT
	CVmConfiguration vmConf;

	QString param1 = Uuid::createUuid().toString();
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( hVmHandle, (&vmConf) );
	vmConf.getVmHardwareList()->m_lstHardDisks.at(0)->setDescription(param1);
	CHECK_RET_CODE_EXP( PrlVm_FromString( hVmHandle, QSTR2UTF8( vmConf.toString() ) ) );
	hJob.reset(PrlVm_Commit(hVmHandle));
	CHECK_JOB_RET_CODE( hJob );

	bool param2;
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( hVmHandle2, (&vmConf) );
	param2 = ( bool )vmConf.getVmHardwareList()->m_lstHardDisks.at(0)->getConnected();
	vmConf.getVmHardwareList()->m_lstHardDisks.at(0)->setConnected( (uint) !param2 );
	CHECK_RET_CODE_EXP( PrlVm_FromString( hVmHandle2, QSTR2UTF8( vmConf.toString() ) ) );
	CHECK_ASYNC_OP_FAILED( PrlVm_Commit(hVmHandle2), PRL_ERR_VM_CONFIG_WAS_CHANGED );

	// rollback conflict changes
	vmConf.getVmHardwareList()->m_lstHardDisks.at(0)->setConnected( param2 );
	CHECK_RET_CODE_EXP( PrlVm_FromString( hVmHandle2, QSTR2UTF8( vmConf.toString() ) ) );


	//////////////////////////////////////////////////////////////////////////
	// 4.1 check section-1 - should be equal to setted in 3.1
	hJob.reset(PrlVm_RefreshConfig( hVmHandle ));
	CHECK_JOB_RET_CODE(hJob);

	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( hVmHandle, (&vmConf) );
	QCOMPARE( vmConf.getVmHardwareList()->m_lstHardDisks.at(0)->getDescription(), param1 );
	QCOMPARE( (bool)vmConf.getVmHardwareList()->m_lstHardDisks.at(0)->getConnected(), param2 );

	//////////////////////////////////////////////////////////////////////////
	// 5.1 change section-2 param-3 and apply by Editcommit by session 2
	QString param3 = Uuid::createUuid().toString();
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( hVmHandle2, (&vmConf) );
	vmConf.getVmHardwareList()->m_lstHardDisks.at(1)->setDescription(param3);
	CHECK_RET_CODE_EXP( PrlVm_FromString( hVmHandle2, QSTR2UTF8( vmConf.toString() ) ) );
	hJob.reset(PrlVm_Commit(hVmHandle2));
	CHECK_JOB_RET_CODE( hJob );

	//////////////////////////////////////////////////////////////////////////
	// 6.1 check section-1 param-1 - should be equal to setted in 3.1
	// 6.1 check section-2 param-3 - should be equal to setted in 5.1
	hJob.reset(PrlVm_RefreshConfig( hVmHandle ));
	CHECK_JOB_RET_CODE(hJob);

	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( hVmHandle, (&vmConf) );
	QCOMPARE( vmConf.getVmHardwareList()->m_lstHardDisks.at(0)->getDescription(), param1 );
	QCOMPARE( (bool)vmConf.getVmHardwareList()->m_lstHardDisks.at(0)->getConnected(), param2 );
	QCOMPARE( vmConf.getVmHardwareList()->m_lstHardDisks.at(1)->getDescription(), param3 );
}

//////////////////////////////////////////////////////////////////////////

void DispFunctionalityTest::testMergeVmConfig_byAtomicChangeBetween()
{
	// test with one session
	// test with two sessions

	// ===== testcase-1 ===
	// 1. create Vm
	// 2. send beginEdit
	// 3. change param-1 by atomic
	// 4. change param-2 and apply by Editcommit
	// 5. check param-1 - should be equal to setted in 3.
	// 6. check param-2 - should be equal to setted in 4.

	SdkHandleWrap hJob;
	SdkHandleWrap hResult;
	CVmConfiguration vmConf;

	test_login();

	//////////////////////////////////////////////////////////////////////////
	// 1. create Vm
	QString sVmUuid;
	bool bRes = false;
	createVm( GEN_VM_NAME_BY_TEST_FUNCTION(), bRes, m_VmHandle, sVmUuid );
	AutoDeleteVm autoDeleter(m_VmHandle);
	QVERIFY( bRes );
	bRes = false;

	//////////////////////////////////////////////////////////////////////////
	// 2. send beginEdit
	hJob.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 3. change param-1 by atomic
	PRL_VM_COLOR nExpectedColor = PVC_COLOR_YELLOW;
	hJob.reset(PrlVm_StoreValueByKey( m_VmHandle
		, PRL_KEY_TO_STORE_VM_COLOR_VALUE, QSTR2UTF8( QString("%1").arg( nExpectedColor ) ), 0 ));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 4. change param-2 and apply by Editcommit
	QString param2 = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP( PrlVmCfg_SetSystemFlags(m_VmHandle, QSTR2UTF8(param2) ) );

	hJob.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	//check
	hJob.reset(PrlVm_RefreshConfig( m_VmHandle ));
	CHECK_JOB_RET_CODE(hJob);

	// 5. check param-1 - should be equal to setted in 3.
	// 6. check param-2 - should be equal to setted in 4.
	CONVERT_VMCONF_HANDLE_TO_XML_OBJECT( m_VmHandle, vmConf );
	// check param1
	QCOMPARE( quint32( nExpectedColor ), quint32( vmConf.getVmSettings()->getVmCommonOptions()->getVmColor() ) );
	// check param2
	QString sVal;
	PRL_EXTRACT_STRING_VALUE(sVal, m_VmHandle, PrlVmCfg_GetSystemFlags);
	QCOMPARE( sVal, param2 );
}

void DispFunctionalityTest::testMergeVmConfig_byAtomicChangeBetween_Conflict()
{
	// test with one session

	// ===== testcase-1 ===
	// 1. create Vm
	// 2. send beginEdit
	// 3. change param-1 to value-1 by atomic
	// 4. change param-1 to value-2 and apply by EditCommit
	// 5. check result for PRL_ERR_VM_CONFIG_WAS_CHANGED
	// 6. check that value of param-1 == value-1

	SdkHandleWrap hJob;
	SdkHandleWrap hResult;
	CVmConfiguration vmConf;

	test_login();

	//////////////////////////////////////////////////////////////////////////
	// 1. create Vm
	QString sVmUuid;
	bool bRes = false;
	createVm( GEN_VM_NAME_BY_TEST_FUNCTION(), bRes, m_VmHandle, sVmUuid );
	AutoDeleteVm autoDeleter(m_VmHandle);
	QVERIFY( bRes );
	bRes = false;


	//////////////////////////////////////////////////////////////////////////
	// 2. send beginEdit
	hJob.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( m_VmHandle, (&vmConf) );
	PRL_VM_COLOR nOriginColor = (PRL_VM_COLOR )vmConf.getVmSettings()->getVmCommonOptions()->getVmColor();

	//////////////////////////////////////////////////////////////////////////
	// 3. change param-1 by atomic
	PRL_VM_COLOR nExpectedColor = nOriginColor == PVC_COLOR_YELLOW ? PVC_COLOR_BLUE : PVC_COLOR_YELLOW;
	hJob.reset(PrlVm_StoreValueByKey( m_VmHandle
		, PRL_KEY_TO_STORE_VM_COLOR_VALUE, QSTR2UTF8( QString("%1").arg( nExpectedColor ) ), 0 ));
	CHECK_JOB_RET_CODE(hJob);

	//////////////////////////////////////////////////////////////////////////
	// 4. change param-1 and apply by Editcommit
	vmConf.getVmSettings()->getVmCommonOptions()->setVmColor( nOriginColor == PVC_COLOR_RED ? PVC_COLOR_GREEN : PVC_COLOR_RED );
	CHECK_RET_CODE_EXP( PrlVm_FromString( m_VmHandle, QSTR2UTF8( vmConf.toString() ) ) );

	// 5. check result for PRL_ERR_VM_CONFIG_WAS_CHANGED
	CHECK_ASYNC_OP_FAILED( PrlVm_Commit(m_VmHandle), PRL_ERR_VM_CONFIG_WAS_CHANGED );

	//check
	hJob.reset(PrlVm_RefreshConfig( m_VmHandle ));
	CHECK_JOB_RET_CODE(hJob);

	// 5. check param-1 - should be equal to setted in 3.
	CONVERT_VMCONF_HANDLE_TO_XML_OBJECT( m_VmHandle, vmConf );
	QCOMPARE( quint32( nExpectedColor )
		, quint32( vmConf.getVmSettings()->getVmCommonOptions()->getVmColor() ) );
}

void DispFunctionalityTest::testMergeVmConfig_case11_AtomicAndAtomic_MERGE()
{
	// (A) - commits in another sessions ( session-1)
	// (B) - commit in this session ( with merging attempt ) (session-2)
	// case and behavior are described in header file

	// 1. make 2 sessions ( session-1 / session-2 )
	// 2. create Vm
	// 3.1 session-1: send beginEdit
	// 3.2 session-2: send beginEdit
	// 4.1 session-1: change param-1 ( atomic accessors )
	// 4.2 session-1: editCommit
	// 5.1 session-2: change param-2 ( atomic accessors )
	// 5.2 session-2: editCommit - should be succeeded.
	// 6. check that param-1 setted successfully
	// 7. check that param-2 setted successfully

	SdkHandleWrap hJob;
	SdkHandleWrap hResult;
	SdkHandleWrap hSession2;
	SdkHandleWrap hVmHandle2;
	CVmConfiguration vmConfOrig;
	CVmConfiguration vmConf;

	//////////////////////////////////////////////////////////////////////////
	// 1. make 2 sessions ( session-1 / session-2 )
	test_login();
	SimpleServerWrapper session2(0);
	QVERIFY( session2.IsConnected() );
	hSession2 = session2.GetServerHandle();

	//////////////////////////////////////////////////////////////////////////
	// 1. create Vm
	QString sVmUuid;
	bool bRes = false;
	createVm( GEN_VM_NAME_BY_TEST_FUNCTION(), bRes, m_VmHandle, sVmUuid );
	AutoDeleteVm autoDeleter(m_VmHandle);
	QVERIFY( bRes );
	bRes = false;

	hVmHandle2 = session2.GetVmByUuid(sVmUuid);

	// 3.1 session-1: send beginEdit
	hJob.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	// 3.2 session-2: send beginEdit
	hJob.reset(PrlVm_BeginEdit(hVmHandle2));
	CHECK_JOB_RET_CODE(hJob);

	// 4.1 session-1: change param-1 ( atomic accessors )
	uint uiParam1;
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( m_VmHandle, (&vmConf) );
	uiParam1 = vmConf.getVmSettings()->getVmAutoprotect()->getTotalSnapshots() + 1;
	vmConf.getVmSettings()->getVmAutoprotect()->setTotalSnapshots( uiParam1 );
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, QSTR2UTF8( vmConf.toString() )));

	// 4.2 session-1: editCommit
	hJob.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE( hJob );

	// 5.1 session-2: change param-2 ( atomic accessors )
	uint uiParam2;
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( hVmHandle2, (&vmConf) );
	uiParam2 = vmConf.getVmSettings()->getVmAutoprotect()->getPeriod() + 1;
	vmConf.getVmSettings()->getVmAutoprotect()->setPeriod( uiParam2 );
	CHECK_RET_CODE_EXP(PrlVm_FromString(hVmHandle2, QSTR2UTF8( vmConf.toString() )));

	// 5.2 session-2: editCommit - should be succeeded.
	hJob.reset(PrlVm_Commit(hVmHandle2));
	CHECK_JOB_RET_CODE( hJob );

	//check
	hJob.reset(PrlVm_RefreshConfig( m_VmHandle ));
	CHECK_JOB_RET_CODE(hJob);

	// 6. check that param-1 setted successfully
	uint uiVal;
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( m_VmHandle, (&vmConf) );

	uiVal = vmConf.getVmSettings()->getVmAutoprotect()->getTotalSnapshots();
	QCOMPARE( uiVal, uiParam1 );

	// 7. check that param-2 setted successfully
	uiVal = vmConf.getVmSettings()->getVmAutoprotect()->getPeriod();
	QCOMPARE( uiVal, uiParam2 );
}

void DispFunctionalityTest::testMergeVmConfig_case12_AtomicAndNotAtomic_MERGE()
{
	// (A) - commits in another sessions ( session-1)
	// (B) - commit in this session ( with merging attempt ) (session-2)
	// case and behavior are described in header file

	// 1. make 2 sessions ( session-1 / session-2 )
	// 2. create Vm
	// 3.1 session-1: send beginEdit
	// 3.2 session-2: send beginEdit
	// 4.1 session-1: change param-1 ( atomic accessors )
	// 4.2 session-1: editCommit
	// 5.1 session-2: change param-2 ( not atomic accessors )
	// 5.2 session-2: editCommit - should be succeeded.
	// 6. check that param-1 setted successfully
	// 7. check that param-2 setted successfully

	SdkHandleWrap hJob;
	SdkHandleWrap hResult;
	SdkHandleWrap hSession2;
	SdkHandleWrap hVmHandle2;
	CVmConfiguration vmConfOrig;
	CVmConfiguration vmConf;

	//////////////////////////////////////////////////////////////////////////
	// 1. make 2 sessions ( session-1 / session-2 )
	test_login();
	SimpleServerWrapper session2(0);
	QVERIFY( session2.IsConnected() );
	hSession2 = session2.GetServerHandle();

	//////////////////////////////////////////////////////////////////////////
	// 1. create Vm
	QString sVmUuid;
	bool bRes = false;
	createVm( GEN_VM_NAME_BY_TEST_FUNCTION(), bRes, m_VmHandle, sVmUuid );
	AutoDeleteVm autoDeleter(m_VmHandle);
	QVERIFY( bRes );
	bRes = false;

	hVmHandle2 = session2.GetVmByUuid(sVmUuid);

	// 3.1 session-1: send beginEdit
	hJob.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	// 3.2 session-2: send beginEdit
	hJob.reset(PrlVm_BeginEdit(hVmHandle2));
	CHECK_JOB_RET_CODE(hJob);

	// 4.1 session-1: change param-1 ( atomic accessors )
	uint uiParam1;
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( m_VmHandle, (&vmConf) );
	uiParam1 = vmConf.getVmSettings()->getVmAutoprotect()->getTotalSnapshots() + 1;
	vmConf.getVmSettings()->getVmAutoprotect()->setTotalSnapshots( uiParam1 );
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, QSTR2UTF8( vmConf.toString() )));

	// 4.2 session-1: editCommit
	hJob.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE( hJob );

	// 5.1 session-2: change param-2 ( not atomic accessors )
	uint uiParam2;
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( hVmHandle2, (&vmConf) );
	uiParam2 = vmConf.getVmSettings()->getVmAutoCompress()->getPeriod()+1;
	vmConf.getVmSettings()->getVmAutoCompress()->setPeriod( uiParam2 );
	CHECK_RET_CODE_EXP(PrlVm_FromString(hVmHandle2, QSTR2UTF8( vmConf.toString() )));

	// 5.2 session-2:	editCommit - should be succeeded.
	hJob.reset(PrlVm_Commit(hVmHandle2));
	CHECK_JOB_RET_CODE( hJob );

	//check
	hJob.reset(PrlVm_RefreshConfig( m_VmHandle ));
	CHECK_JOB_RET_CODE(hJob);

	// 6. check that param-1 setted successfully
	uint uiVal;
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( m_VmHandle, (&vmConf) );

	uiVal = vmConf.getVmSettings()->getVmAutoprotect()->getTotalSnapshots();
	QCOMPARE( uiVal, uiParam1 );

	// 7. check that param-2 setted successfully
	uiVal = vmConf.getVmSettings()->getVmAutoCompress()->getPeriod();
	QCOMPARE( uiVal, uiParam2 );
}

void DispFunctionalityTest::testMergeVmConfig_case21_NotAtomicAndAtomic_MERGE()
{
	// (A) - commits in another sessions ( session-1)
	// (B) - commit in this session ( with merging attempt ) (session-2)
	// case and behavior are described in header file

	// 1. make 2 sessions ( session-1 / session-2 )
	// 2. create Vm
	// 3.1 session-1: send beginEdit
	// 3.2 session-2: send beginEdit
	// 4.1 session-1: change param-1 ( not atomic accessors )
	// 4.2 session-1: editCommit
	// 5.1 session-2: change param-2 ( atomic accessors )
	// 5.2 session-2: editCommit - should be succeeded.
	// 6. check that param-1 setted successfully
	// 7. check that param-2 setted successfully

	SdkHandleWrap hJob;
	SdkHandleWrap hResult;
	SdkHandleWrap hSession2;
	SdkHandleWrap hVmHandle2;
	CVmConfiguration vmConfOrig;
	CVmConfiguration vmConf;

	//////////////////////////////////////////////////////////////////////////
	// 1. make 2 sessions ( session-1 / session-2 )
	test_login();
	SimpleServerWrapper session2(0);
	QVERIFY( session2.IsConnected() );
	hSession2 = session2.GetServerHandle();

	//////////////////////////////////////////////////////////////////////////
	// 1. create Vm
	QString sVmUuid;
	bool bRes = false;
	createVm( GEN_VM_NAME_BY_TEST_FUNCTION(), bRes, m_VmHandle, sVmUuid );
	AutoDeleteVm autoDeleter(m_VmHandle);
	QVERIFY( bRes );
	bRes = false;

	hVmHandle2 = session2.GetVmByUuid(sVmUuid);

	// 3.1 session-1: send beginEdit
	hJob.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	// 3.2 session-2: send beginEdit
	hJob.reset(PrlVm_BeginEdit(hVmHandle2));
	CHECK_JOB_RET_CODE(hJob);

	// 4.1 session-1: change param-1 ( not atomic accessors )
	uint uiParam1;
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( m_VmHandle, (&vmConf) );
	uiParam1 = vmConf.getVmSettings()->getVmAutoCompress()->getPeriod()+1;
	vmConf.getVmSettings()->getVmAutoCompress()->setPeriod( uiParam1 );
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, QSTR2UTF8( vmConf.toString() )));

	// 4.2 session-1: editCommit
	hJob.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE( hJob );

	// 5.1 session-2: change param-2 ( atomic accessors )
	uint uiParam2;
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( hVmHandle2, (&vmConf) );
	uiParam2 = vmConf.getVmSettings()->getVmAutoprotect()->getTotalSnapshots() + 1;
	vmConf.getVmSettings()->getVmAutoprotect()->setTotalSnapshots( uiParam2 );
	CHECK_RET_CODE_EXP(PrlVm_FromString(hVmHandle2, QSTR2UTF8( vmConf.toString() )));

	// 5.2 session-2:	editCommit - should be succeeded.
	hJob.reset(PrlVm_Commit(hVmHandle2));
	CHECK_JOB_RET_CODE( hJob );

	//check
	hJob.reset(PrlVm_RefreshConfig( m_VmHandle ));
	CHECK_JOB_RET_CODE(hJob);

	// 6. check that param-1 setted successfully
	uint uiVal;
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( m_VmHandle, (&vmConf) );

	uiVal = vmConf.getVmSettings()->getVmAutoCompress()->getPeriod();
	QCOMPARE( uiVal, uiParam1 );

	// 7. check that param-2 setted successfully
	uiVal = vmConf.getVmSettings()->getVmAutoprotect()->getTotalSnapshots();
	QCOMPARE( uiVal, uiParam2 );
}

void DispFunctionalityTest::testMergeVmConfig_case22_NotAtomicAndNotAtomic()
{
	// (A) - commits in another sessions ( session-1)
	// (B) - commit in this session ( with merging attempt ) (session-2)
	// case and behavior are described in header file

	// 1. make 2 sessions ( session-1 / session-2 )
	// 2. create Vm
	// 3.1 session-1: send beginEdit
	// 3.2 session-2: send beginEdit
	// 4.1 session-1: change param-1 ( not atomic accessors )
	// 4.2 session-1: editCommit
	// 5.1 session-2: change param-2 ( not atomic accessors )
	// 5.2 session-2: editCommit
	// 6. check that param-1 setted successfully
	// 7. check that param-2 setted successfully

	SdkHandleWrap hJob;
	SdkHandleWrap hResult;
	SdkHandleWrap hSession2;
	SdkHandleWrap hVmHandle2;
	CVmConfiguration vmConfOrig;
	CVmConfiguration vmConf;

	//////////////////////////////////////////////////////////////////////////
	// 1. make 2 sessions ( session-1 / session-2 )
	test_login();
	SimpleServerWrapper session2(0);
	QVERIFY( session2.IsConnected() );
	hSession2 = session2.GetServerHandle();

	//////////////////////////////////////////////////////////////////////////
	// 1. create Vm
	QString sVmUuid;
	bool bRes = false;
	createVm( GEN_VM_NAME_BY_TEST_FUNCTION(), bRes, m_VmHandle, sVmUuid );
	AutoDeleteVm autoDeleter(m_VmHandle);
	QVERIFY( bRes );
	bRes = false;

	hVmHandle2 = session2.GetVmByUuid(sVmUuid);

	// 3.1 session-1: send beginEdit
	hJob.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	// 3.2 session-2: send beginEdit
	hJob.reset(PrlVm_BeginEdit(hVmHandle2));
	CHECK_JOB_RET_CODE(hJob);

	// 4.1 session-1: change param-1 ( not atomic accessors )
	uint uiParam1;
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( m_VmHandle, (&vmConf) );
	uiParam1 = vmConf.getVmSettings()->getVmAutoCompress()->getPeriod()+1;
	vmConf.getVmSettings()->getVmAutoCompress()->setPeriod( uiParam1 );
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, QSTR2UTF8( vmConf.toString() )));

	// 4.2 session-1: editCommit
	hJob.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE( hJob );

	// 5.1 session-2: change param-2 ( atomic accessors )
	uint uiParam2, uiParam2Original;
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( hVmHandle2, (&vmConf) );
	uiParam2Original = vmConf.getVmSettings()->getVmAutoprotect()->getTotalSnapshots();
	uiParam2 = uiParam2Original + 1;
	vmConf.getVmSettings()->getVmAutoprotect()->setTotalSnapshots( uiParam2 );
	CHECK_RET_CODE_EXP(PrlVm_FromString(hVmHandle2, QSTR2UTF8( vmConf.toString() )));
	hJob.reset(PrlVm_Commit(hVmHandle2));
	CHECK_JOB_RET_CODE(hJob);

	//check
	hJob.reset(PrlVm_RefreshConfig( m_VmHandle ));
	CHECK_JOB_RET_CODE(hJob);

	// 6. check that param-1 setted successfully
	uint uiVal;
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( m_VmHandle, (&vmConf) );

	uiVal = vmConf.getVmSettings()->getVmAutoCompress()->getPeriod();
	QCOMPARE( uiVal, uiParam1 );

	// 7. check that param-2 setted successfully
	uiVal = vmConf.getVmSettings()->getVmAutoprotect()->getTotalSnapshots();
	QCOMPARE( uiVal, uiParam2 );
}

void DispFunctionalityTest::testMergeVmConfig_case11_AtomicAndAtomic_TheSame_CONFLICT()
{
	// (A) - commits in another sessions ( session-1)
	// (B) - commit in this session ( with merging attempt ) (session-2)
	// case and behavior are described in header file

	// 1. make 2 sessions ( session-1 / session-2 )
	// 2. create Vm
	// 3.1 session-1: send beginEdit
	// 3.2 session-2: send beginEdit
	// 4.1 session-1: change param-1 ( atomic accessors )
	// 4.2 session-1: editCommit
	// 5.1 session-2: change param-1 ( atomic accessors )
	// 5.2 session-2: editCommit - should be succeeded.
	// 6. check that param-1 setted successfully
	// 7. check that param-2 wasn't setted

	SdkHandleWrap hJob;
	SdkHandleWrap hResult;
	SdkHandleWrap hSession2;
	SdkHandleWrap hVmHandle2;
	CVmConfiguration vmConfOrig;
	CVmConfiguration vmConf;

	//////////////////////////////////////////////////////////////////////////
	// 1. make 2 sessions ( session-1 / session-2 )
	test_login();
	SimpleServerWrapper session2(0);
	QVERIFY( session2.IsConnected() );
	hSession2 = session2.GetServerHandle();

	//////////////////////////////////////////////////////////////////////////
	// 1. create Vm
	QString sVmUuid;
	bool bRes = false;
	createVm( GEN_VM_NAME_BY_TEST_FUNCTION(), bRes, m_VmHandle, sVmUuid );
	AutoDeleteVm autoDeleter(m_VmHandle);
	QVERIFY( bRes );
	bRes = false;

	hVmHandle2 = session2.GetVmByUuid(sVmUuid);

	// 3.1 session-1: send beginEdit
	hJob.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob);

	// 3.2 session-2: send beginEdit
	hJob.reset(PrlVm_BeginEdit(hVmHandle2));
	CHECK_JOB_RET_CODE(hJob);

	// 4.1 session-1: change param-1 ( atomic accessors )
	uint uiParam1;
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( m_VmHandle, (&vmConf) );
	uiParam1 = vmConf.getVmSettings()->getVmAutoprotect()->getTotalSnapshots() + 1;
	vmConf.getVmSettings()->getVmAutoprotect()->setTotalSnapshots( uiParam1 );
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, QSTR2UTF8( vmConf.toString() )));

	// 4.2 session-1: editCommit
	hJob.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE( hJob );

	// 5.1 session-2: change param-2 ( atomic accessors )
	uint uiParam2;
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( hVmHandle2, (&vmConf) );
	uiParam2 = vmConf.getVmSettings()->getVmAutoprotect()->getTotalSnapshots() + 110;
	vmConf.getVmSettings()->getVmAutoprotect()->setTotalSnapshots( uiParam2 );
	CHECK_RET_CODE_EXP(PrlVm_FromString(hVmHandle2, QSTR2UTF8( vmConf.toString() )));

	// 5.2 session-2:	editCommit should be conflict because this atomic value was changed already.
	//					with error PRL_ERR_VM_CONFIG_WAS_CHANGED
	CHECK_ASYNC_OP_FAILED( PrlVm_Commit(hVmHandle2), PRL_ERR_VM_CONFIG_WAS_CHANGED );

	//check
	hJob.reset(PrlVm_RefreshConfig( m_VmHandle ));
	CHECK_JOB_RET_CODE(hJob);

	// 6. check that param-1 setted successfully
	uint uiVal;
	EXTRACT_CONFIG_AS_XML_MODEL_OBJECT( m_VmHandle, (&vmConf) );

	uiVal = vmConf.getVmSettings()->getVmAutoprotect()->getTotalSnapshots();
	QCOMPARE( uiVal, uiParam1 );

	// 7. check that param-2 wasn't setted
	QVERIFY( uiVal != uiParam2 );
}

void DispFunctionalityTest::testCloneVm_withExternalHdd()
{
	// 1. create vm1, vm2 with hdd
	// 2. add link to vm1/hdd1 to vm2
	// 3.1 clone vm2 --> vm3 with flag "drop-external-hdd"
	// 3.2 check that vm3 hasn't vm1/hdd
	// 3.3 check that vm3 has hdd
	//
	// 4.1 clone vm2 --> vm4 without flag "drop-external-hdd"
	// 4.2 check that vm4 has vm1/hdd
	// 3.4 check that vm4 has hdd

	QString sVmNamePrefix = GEN_VM_NAME_BY_TEST_FUNCTION();

	test_login();

	SdkHandleWrap hJob;
	SdkHandleWrap hResult;
	bool bRes = false;
	QString sVmUuid;

	//////////////////////////////////////////////////////////////////////////
	// 1. create vm1, vm2 with hdd

	// create vm-1
	SdkHandleWrap hVmHandle;
	createVm( sVmNamePrefix + "-vm1", bRes, hVmHandle, sVmUuid );
	AutoDeleteVm autoDeleter(hVmHandle);
	QVERIFY( bRes );

	// create vm-2
	SdkHandleWrap hVmHandle2;
	createVm( sVmNamePrefix + "-vm2", bRes, hVmHandle2, sVmUuid );
	AutoDeleteVm autoDeleter2(hVmHandle2);
	QVERIFY( bRes );

	//////////////////////////////////////////////////////////////////////////
	// 2. add link to vm1/hdd1 to vm2
	QString sVm1HddPath;
	SdkHandleWrap hHardDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHardDisk(hVmHandle, 0, hHardDisk.GetHandlePtr()));
	PRL_EXTRACT_STRING_VALUE( sVm1HddPath, hHardDisk, PrlVmDev_GetImagePath );
	QVERIFY( !sVm1HddPath.isEmpty() );

	hJob.reset(PrlVm_BeginEdit( hVmHandle2 ));
	CHECK_JOB_RET_CODE( hJob);

	// override hHardDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(hVmHandle2, PDE_HARD_DISK, hHardDisk.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetImagePath( hHardDisk, QSTR2UTF8(sVm1HddPath) ) );

	hJob.reset(PrlVm_Commit( hVmHandle2 ));
	CHECK_JOB_RET_CODE( hJob);

	//////////////////////////////////////////////////////////////////////////
	// 3.1 clone vm2 --> vm3 with flag "drop-external-hdd"
	SdkHandleWrap hVmHandle3;
	hJob.reset(PrlVm_CloneEx( hVmHandle2, QSTR2UTF8(sVmNamePrefix + "-vm3"), ""
		, PACF_NON_INTERACTIVE_MODE | PCVF_DETACH_EXTERNAL_VIRTUAL_HDD ));
	CHECK_JOB_RET_CODE( hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmHandle3.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hVmHandle3, PHT_VIRTUAL_MACHINE);
	AutoDeleteVm autoDeleter3(hVmHandle3);

	// 3.2 check that vm3 hasn't vm1/hdd
	PRL_UINT32 nHddCount;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHardDisksCount( hVmHandle3, &nHddCount) );
	QCOMPARE( nHddCount, (PRL_UINT32)1);

	// 3.3 check that vm3 has hdd
	QString sHddPath;
	QString sVmHomePath;
	PRL_EXTRACT_STRING_VALUE( sVmHomePath, hVmHandle3, PrlVmCfg_GetHomePath );
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHardDisk(hVmHandle3, 0, hHardDisk.GetHandlePtr()));
	PRL_EXTRACT_STRING_VALUE( sHddPath, hHardDisk, PrlVmDev_GetImagePath );

	QCOMPARE( QFileInfo(sHddPath).absolutePath(), QFileInfo(sVmHomePath).absolutePath() );

	//////////////////////////////////////////////////////////////////////////
	// 4.1 clone vm2 --> vm4 without flag "drop-external-hdd"
	SdkHandleWrap hVmHandle4;
	hJob.reset(PrlVm_CloneEx( hVmHandle2, QSTR2UTF8(sVmNamePrefix + "-vm4"), ""
		, PACF_NON_INTERACTIVE_MODE ));
	CHECK_JOB_RET_CODE( hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmHandle4.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hVmHandle4, PHT_VIRTUAL_MACHINE);
	AutoDeleteVm autoDeleter4(hVmHandle4);

	// 4.2 check that vm4 has vm1/hdd
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHardDisksCount(hVmHandle4, &nHddCount) );
	QCOMPARE( nHddCount, (PRL_UINT32)2);

	CHECK_RET_CODE_EXP(PrlVmCfg_GetHardDisk(hVmHandle4, 1, hHardDisk.GetHandlePtr()));
	PRL_EXTRACT_STRING_VALUE( sHddPath, hHardDisk, PrlVmDev_GetImagePath );
	QCOMPARE( sHddPath, sVm1HddPath);

	// 4.3 check that vm4 has hdd
	PRL_EXTRACT_STRING_VALUE( sVmHomePath, hVmHandle4, PrlVmCfg_GetHomePath );
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHardDisk(hVmHandle4, 0, hHardDisk.GetHandlePtr()));
	PRL_EXTRACT_STRING_VALUE( sHddPath, hHardDisk, PrlVmDev_GetImagePath );

	QCOMPARE( QFileInfo(sHddPath).absolutePath(), QFileInfo(sVmHomePath).absolutePath() );
}

void DispFunctionalityTest::testGetServerAppMode_fromLoginResponse()
{
	PRL_APPLICATION_MODE nCliMode = ParallelsDirs::getAppExecuteMode();

	// login local
	SimpleServerWrapper session(0);
	QVERIFY( session.IsConnected() );

	SdkHandleWrap hServerInfo;
	CHECK_RET_CODE_EXP(PrlSrv_GetServerInfo(session.GetServerHandle(), hServerInfo.GetHandlePtr()))

		PRL_APPLICATION_MODE nServerMode;
	CHECK_RET_CODE_EXP(PrlSrvInfo_GetApplicationMode(hServerInfo, &nServerMode));
	QCOMPARE( (quint32)nServerMode, (quint32)nCliMode );

	// login remote
	if( nCliMode == PAM_SERVER )
	{
		SimpleServerWrapper session2;
		QVERIFY( session2.IsConnected() );
		CHECK_RET_CODE_EXP(PrlSrv_GetServerInfo(session2.GetServerHandle(), hServerInfo.GetHandlePtr()));
		CHECK_RET_CODE_EXP(PrlSrvInfo_GetApplicationMode(hServerInfo, &nServerMode));

		QCOMPARE( (quint32)nServerMode, (quint32)nCliMode );
	}
}

void DispFunctionalityTest::testGetServerAppMode_fromLoginResponseOnWrongParameters()
{
	SimpleServerWrapper session(0);
	QVERIFY( session.IsConnected() );

	SdkHandleWrap hServerInfo;
	CHECK_RET_CODE_EXP(PrlSrv_GetServerInfo(session.GetServerHandle(), hServerInfo.GetHandlePtr()));

	PRL_APPLICATION_MODE nServerMode;
	CHECK_CONCRETE_EXPRESSION_RET_CODE( \
		PrlSrvInfo_GetApplicationMode(PRL_INVALID_HANDLE, &nServerMode), PRL_ERR_INVALID_ARG );
	CHECK_CONCRETE_EXPRESSION_RET_CODE( \
		PrlSrvInfo_GetApplicationMode(session.GetServerHandle(), &nServerMode), PRL_ERR_INVALID_ARG );
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvInfo_GetApplicationMode(hServerInfo, NULL), PRL_ERR_INVALID_ARG );

}

void DispFunctionalityTest::canUserChangeServerPrefs( bool& bRes
	 , const SdkHandleWrap& hServer, PRL_BOOL& bUserCanChangeServerPrefs )
{
	bRes = false;
	SdkHandleWrap hUserProfile;
	RECEIVE_USER_PROFILE(hServer, hUserProfile);
	CHECK_RET_CODE_EXP( PrlUsrCfg_CanChangeSrvSets(hUserProfile, &bUserCanChangeServerPrefs) );

	bRes = true;
}

static void getVmUuid( bool& bRes, const SdkHandleWrap& hVmHandle, QString& outVmUuid )
{
	bRes = false;
	PRL_EXTRACT_STRING_VALUE(outVmUuid, hVmHandle, PrlVmCfg_GetUuid);
	bRes = true;
}

QString DispFunctionalityTest::GetVmUuidByHandle( const SdkHandleWrap& hVmHandle )
{
	QString sVmUuid;
	bool bRes = false;
	getVmUuid( bRes, hVmHandle, sVmUuid );
	if( bRes )
		return sVmUuid;
	return "";
}

void DispFunctionalityTest::addExistingHddToVm( bool& bRes
	, const SdkHandleWrap& hVmHandle
	, const SdkHandleWrap& hHddHandleToCopy
	, const SdkHandleWrap& hSrvConfig
	, SdkHandleWrap& hOutHddHandle
	, const QString& sHddPassword
	)
{
	bRes = false;

	CHECK_JOB_RET_CODE( PrlVm_BeginEdit(hVmHandle) );

	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( \
		hVmHandle, hSrvConfig, PDE_HARD_DISK, hOutHddHandle.GetHandlePtr()));

	// store autogenerated fields before convert from string
	CVmHardDisk hddDefault;
	EXTRACT_HANDLE_AS_XML_MODEL_OBJECT( hOutHddHandle, hddDefault );

	CVmHardDisk hddToCopy;
	EXTRACT_HANDLE_AS_XML_MODEL_OBJECT( hHddHandleToCopy, hddToCopy );

	hddToCopy.setIndex( hddDefault.getIndex() );
	hddToCopy.setStackIndex( hddDefault.getStackIndex() );
	hddToCopy.setItemId( hddDefault.getItemId() );

	CHECK_RET_CODE_EXP(PrlHandle_FromString( hOutHddHandle, QSTR2UTF8(hddToCopy.toString() ) ) );

	// restore default values
	if( !sHddPassword.isEmpty() )
		CHECK_RET_CODE_EXP( PrlVmDevHd_SetPassword(hOutHddHandle, QSTR2UTF8(sHddPassword) ) );

	CHECK_JOB_RET_CODE( PrlVm_Commit(hVmHandle) );

	bRes = true;
}

void DispFunctionalityTest::GetHddPathsByVmHandle( bool& bRes
	, const SdkHandleWrap& hVmHandle, PRL_HARD_DISK_INTERNAL_FORMAT type
	, QStringList& outLstPaths )
{
	bRes = false;
	outLstPaths.clear();

	QString sVmHomePath;
	PRL_EXTRACT_STRING_VALUE(sVmHomePath, hVmHandle, PrlVmCfg_GetHomePath);
	sVmHomePath = QFileInfo(sVmHomePath).absolutePath();

	QList<SdkHandleWrap> hddList;
	GetHddListByVmHandle( bRes, hVmHandle, hddList, false, type  );
	QVERIFY(bRes);

	foreach( SdkHandleWrap hHdd, hddList )
	{
		QString sHddPath;
		PRL_EXTRACT_STRING_VALUE( sHddPath, hHdd, PrlVmDev_GetSysName );
		if( !QFileInfo(sHddPath).isAbsolute() )
			sHddPath = sVmHomePath + "/" + sHddPath;
		outLstPaths << sHddPath;
	}
	bRes = true;
}

void DispFunctionalityTest::GetHddListByVmHandle( bool& bRes
	, const SdkHandleWrap& hVmHandle
	, QList<SdkHandleWrap>& outHddList
	, bool bPatchToAbsPath
	, PRL_HARD_DISK_INTERNAL_FORMAT type
	, const QString& sHddPathExpected )
{
	bRes = false;
	outHddList.clear();
	// get all hdd devices
	// search type = type
	QString sVmHomePath;
	PRL_EXTRACT_STRING_VALUE(sVmHomePath, hVmHandle, PrlVmCfg_GetHomePath);
	sVmHomePath = QFileInfo(sVmHomePath).absolutePath();


	PRL_UINT32 nHddCount;
	CHECK_RET_CODE_EXP( PrlVmCfg_GetDevsCountByType(hVmHandle, PDE_HARD_DISK, &nHddCount) );
	for(PRL_UINT32 i=0; i<nHddCount; i++)
	{
		SdkHandleWrap hHdd;
		CHECK_RET_CODE_EXP( PrlVmCfg_GetDevByType(hVmHandle, PDE_HARD_DISK, i
			, hHdd.GetHandlePtr() ) );

		if( -1 != (int)type )
		{
			PRL_HARD_DISK_INTERNAL_FORMAT nFmt;
			CHECK_RET_CODE_EXP( PrlVmDevHd_GetDiskType( hHdd, &nFmt ) );
			if( nFmt != type )
				continue;
		}

		QString sHddPath;
		PRL_EXTRACT_STRING_VALUE( sHddPath, hHdd, PrlVmDev_GetSysName );
		if( !QFileInfo(sHddPath).isAbsolute() )
			sHddPath = sVmHomePath + "/" + sHddPath;

		if( !sHddPathExpected.isEmpty() )
		{
			QVERIFY( QFileInfo(sHddPathExpected).isAbsolute() );
			if( !CFileHelper::IsPathsEqual( sHddPathExpected , sHddPath) )
				continue;
		}

		if( bPatchToAbsPath)
		{
			CHECK_RET_CODE_EXP( PrlVmDev_SetSysName(hHdd, QSTR2UTF8( sHddPath )) );
			CHECK_RET_CODE_EXP( PrlVmDev_SetImagePath(hHdd, QSTR2UTF8( sHddPath )) );
		}
		outHddList << hHdd;
	}
	bRes = true;
}

void DispFunctionalityTest::testCreateWrongHddImage_SizeIsZero()
{
	// create Vm
	// create Hdd with zero size
	// check error

	bool bRes;
	SdkHandleWrap hVmHandle, hHddHandle;
	QString sVmUuid;

	test_login();
	createVm( GEN_VM_NAME_BY_TEST_FUNCTION(), bRes, hVmHandle, sVmUuid );
	AutoDeleteVm autoDeleter(hVmHandle);
	QVERIFY(bRes);



	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( \
		 hVmHandle, m_hSrvConfig, PDE_HARD_DISK, hHddHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskType(hHddHandle, PHD_EXPANDING_HARD_DISK ));
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetSplitted(hHddHandle, PRL_FALSE ));
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskSize(hHddHandle, 0 ));

	CHECK_ASYNC_OP_FAILED( PrlVmDev_CreateImage( \
		hHddHandle, PRL_TRUE, PRL_TRUE ), PRL_ERR_CREATE_HARD_DISK_WITH_ZERO_SIZE );

}

void DispFunctionalityTest::testToSplitHdd_AfterResizeSplittedHddTo1Gb()
{
	// 1. create vm
	// 2. create splitted hdd for 4 GB
	// 3. resize hdd to 1GB
	// 4. check that splitted is false
	// 5. convert to splitted
	////////////////////////////////////////////////

	// 1. create vm
	test_login();

	SdkHandleWrap hVmHandle, hHddHandle;

	SET_DEFAULT_CONFIG( hVmHandle, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(hVmHandle, QSTR2UTF8( GEN_VM_NAME_BY_TEST_FUNCTION() ) ) );

	CHECK_JOB_RET_CODE( PrlVm_Reg(hVmHandle, "", PRL_TRUE) );
	AutoDeleteVm autoDeleter(hVmHandle);
	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) );


	// 2. create splitted hdd
	CHECK_JOB_RET_CODE( PrlVm_BeginEdit(hVmHandle) );

	CHECK_RET_CODE_EXP(PrlVmCfg_AddDefaultDeviceEx( \
		 hVmHandle, m_hSrvConfig, PDE_HARD_DISK, hHddHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hHddHandle, PMS_SATA_DEVICE ));
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskType(hHddHandle, PHD_EXPANDING_HARD_DISK ));
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetSplitted(hHddHandle, PRL_TRUE ));
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskSize(hHddHandle, 4*1024 ));

	CHECK_JOB_RET_CODE( PrlVmDev_CreateImage( hHddHandle, PRL_TRUE, PRL_TRUE ) );
	CHECK_JOB_RET_CODE( PrlVm_Commit(hVmHandle) );
	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) );

	PRL_BOOL isSplitted;
	CHECK_RET_CODE_EXP( PrlVmDevHd_IsSplitted( hHddHandle, &isSplitted) );
	QCOMPARE( (int)isSplitted, (int)PRL_TRUE );

	// 3. resize hdd to 1GB
	CHECK_JOB_RET_CODE( PrlVmDev_ResizeImage( hHddHandle, 1*1024, 0 ) );

	// 4. check that splitted is false
	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) );
	CHECK_RET_CODE_EXP( PrlVmDevHd_IsSplitted( hHddHandle, &isSplitted) );
	QCOMPARE( (int)isSplitted, (int)PRL_FALSE );

	// 5. convert to splitted
	PRL_UINT32 nStackIdx;
	CHECK_RET_CODE_EXP( PrlVmDev_GetStackIndex( hHddHandle, &nStackIdx ) );
	PRL_UINT32 uMask = PIM_SATA_MASK_OFFSET << nStackIdx;
	PRL_UINT32 nFlags = PCVD_TO_EXPANDING_DISK | PCVD_TO_SPLIT_DISK ;
	CHECK_JOB_RET_CODE( PrlVm_ConvertDisks(hVmHandle, uMask, nFlags) );

	// 4. check that splitted is false ( for 1Gb hdd )
	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) );
	CHECK_RET_CODE_EXP( PrlVmDevHd_IsSplitted( hHddHandle, &isSplitted) );
	QCOMPARE( (int)isSplitted, (int)PRL_FALSE );
}

//////////////////////////////////////////////////////////////////////////
class CheckPatchSharedCameraEnabled: public CheckPatchBase<CVmConfiguration>
{
public:
	virtual void cleanupPatchedField(bool& bRes, CBaseNode* pCfg)
	{
		QVERIFY( getCfg(pCfg) );
		getCfg(pCfg)->getVmSettings()->getSharedCamera()->markPatchedField( "Enabled", "" );
		bRes = true;
	}
	virtual void checkPatchedField( bool& bRes, CBaseNode* pCfg)
	{
		QVERIFY( getCfg(pCfg) );
		QVERIFY( getCfg(pCfg)->getVmSettings()->getSharedCamera()->isFieldPatched( "Enabled" ) );
		QCOMPARE( getCheckValue(), QVariant( getCfg(pCfg)->getVmSettings()->getSharedCamera()->isEnabled() ) );
		bRes = true;
	}

	static SmartPtr<ICheckPatchBase> create( const QVariant& checkValue )
	{
		return SmartPtr<ICheckPatchBase>( new CheckPatchSharedCameraEnabled(checkValue) );
	}

	CheckPatchSharedCameraEnabled(const QVariant& checkValue)
		:CheckPatchBase<CVmConfiguration>( checkValue )
	{
	}

};
void DispFunctionalityTest::testPatchingSharedCameraEnabled_onRegistration()
{
	bool bRes = false;
	QVERIFY_BRES( bRes, testXMLPatchingMech_onRegistration(bRes
		, GEN_VM_NAME_BY_TEST_FUNCTION(), PVS_GUEST_VER_WIN_XP, CheckPatchSharedCameraEnabled::create(true) ) );
}
void DispFunctionalityTest::testPatchingSharedCameraDisabled_onRegistration()
{
	bool bRes = false;
	QVERIFY_BRES( bRes, testXMLPatchingMech_onRegistration(bRes
		, GEN_VM_NAME_BY_TEST_FUNCTION(), PVS_GUEST_VER_WIN_2K, CheckPatchSharedCameraEnabled::create(false) ) );
}

//////////////////////////////////////////////////////////////////////////
class CheckPatchCornerActionsReset: public CheckPatchBase<CVmConfiguration>
{
public:
	virtual void cleanupPatchedField(bool& bRes, CBaseNode* pCfg)
	{
		QVERIFY( getCfg(pCfg) );
		CVmFullScreen* pObj = getCfg(pCfg)->getVmSettings()->getVmRuntimeOptions()->getVmFullScreen();

		pObj->markPatchedField( "CornerAction", "1" );
		pObj->setCornerAction(PWC_TOP_LEFT_CORNER, PCA_CRYSTAL);
		pObj->setCornerAction(PWC_TOP_RIGHT_CORNER, PCA_COHERENCE);
		pObj->setCornerAction(PWC_BOTTOM_LEFT_CORNER, PCA_SEAMLESS);
		pObj->setCornerAction(PWC_BOTTOM_RIGHT_CORNER, PCA_MODALITY);

		bRes = true;
	}
	virtual void checkPatchedField( bool& bRes, CBaseNode* pCfg)
	{
		QVERIFY( getCfg(pCfg) );
		CVmFullScreen* pObj = getCfg(pCfg)->getVmSettings()->getVmRuntimeOptions()->getVmFullScreen();
		QVERIFY( pObj->isFieldPatched( "CornerAction" ) );
		QCOMPARE( QString("2"), pObj->getFieldPatchedValue( "CornerAction") );

		QCOMPARE( PCA_WINDOWED, pObj->getCornerAction(PWC_TOP_LEFT_CORNER) );
		QCOMPARE( PCA_NO_ACTION, pObj->getCornerAction(PWC_TOP_RIGHT_CORNER) );
		QCOMPARE( PCA_NO_ACTION, pObj->getCornerAction(PWC_BOTTOM_LEFT_CORNER) );
		QCOMPARE( PCA_NO_ACTION, pObj->getCornerAction(PWC_BOTTOM_RIGHT_CORNER) );

		bRes = true;
	}

	static SmartPtr<ICheckPatchBase> create()
	{
		return SmartPtr<ICheckPatchBase>( new CheckPatchCornerActionsReset() );
	}

	CheckPatchCornerActionsReset()
		:CheckPatchBase<CVmConfiguration>( true )
	{
	}

};
void DispFunctionalityTest::testPatchingCornerActions_onRegistration()
{
	bool bRes = false;
	QVERIFY_BRES( bRes, testXMLPatchingMech_onRegistration(bRes
		, GEN_VM_NAME_BY_TEST_FUNCTION(), PVS_GUEST_VER_WIN_2K, CheckPatchCornerActionsReset::create() ) );
}

class CheckPatchOptimizePowerCMReset: public CheckPatchBase<CVmConfiguration>
{
public:
	virtual void cleanupPatchedField(bool& bRes, CBaseNode* pCfg)
	{
		QVERIFY( getCfg(pCfg) );
		CVmRunTimeOptions* pObj = getCfg(pCfg)->getVmSettings()->getVmRuntimeOptions();

		pObj->markPatchedField( "OptimizePowerConsumptionMode", "" );
		bRes = true;
	}
	virtual void checkPatchedField( bool& bRes, CBaseNode* pCfg)
	{
		QVERIFY( getCfg(pCfg) );
		CVmRunTimeOptions* pObj = getCfg(pCfg)->getVmSettings()->getVmRuntimeOptions();
		QVERIFY( pObj->isFieldPatched( "OptimizePowerConsumptionMode" ) );
		QCOMPARE( pObj->getFieldPatchedValue( "OptimizePowerConsumptionMode"), QString("1") );

		QCOMPARE( (int)pObj->getOptimizePowerConsumptionMode(), (int)PVE::OptimizePerformance );

		bRes = true;
	}

	static SmartPtr<ICheckPatchBase> create()
	{
		return SmartPtr<ICheckPatchBase>( new CheckPatchOptimizePowerCMReset() );
	}

	CheckPatchOptimizePowerCMReset()
		:CheckPatchBase<CVmConfiguration>( true )
	{
	}

};
void DispFunctionalityTest::testPatchingOptimizePowerConsumptionMode_onRegistration()
{
	bool bRes = false;
	QVERIFY_BRES( bRes, testXMLPatchingMech_onRegistration(bRes
		, GEN_VM_NAME_BY_TEST_FUNCTION(), PVS_GUEST_VER_WIN_2K, CheckPatchOptimizePowerCMReset::create() ) );

}

void DispFunctionalityTest::testXMLPatchingMech_onRegistration(
	bool& bRes, const QString& sVmName,  PRL_UINT32 osVersion, SmartPtr<ICheckPatchBase> pCheckObj )
{
	// Prepare VM
	bRes  = false;
	SdkHandleWrap hVm, hResult;

	AutoDirRemover rmDir;

	SimpleServerWrapper session(0);
	SET_DEFAULT_CONFIG_EX( session, sVmName, hVm, osVersion, PRL_FALSE);

	// Prepare to register: Create new VM, unregister Vm
	CHECK_JOB_RET_CODE( PrlVm_Reg(hVm, "", PRL_TRUE) );
	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVm) );

	QString sVmHomePath;
	PRL_EXTRACT_STRING_VALUE(sVmHomePath, hVm, PrlVmCfg_GetHomePath);
	rmDir.add( QFileInfo(sVmHomePath).absolutePath() );

	CHECK_JOB_RET_CODE( PrlVm_Unreg(hVm) );

	CVmConfiguration xmlVm;
	QFile f(sVmHomePath);
	QCOMPARE( 0, xmlVm.loadFromFile(&f) );

	//////////////////////////////////////////////////////////////////////////
	////               CLEAN UP PATCHES                                    ///
	QVERIFY_BRES( bRes, pCheckObj->cleanupPatchedField(bRes, &xmlVm) );
	//////////////////////////////////////////////////////////////////////////

	QVERIFY_BRES( bRes, pCheckObj->cleanupPatchedField(bRes, &xmlVm) );

	QCOMPARE( 0, xmlVm.saveToFile(sVmHomePath));

	//////////////////////////////////////////////////////////////////////////
	SdkHandleWrap hJob(PrlSrv_RegisterVm( session, QSTR2UTF8(sVmHomePath), PRL_TRUE ));
	CHECK_JOB_RET_CODE( hJob );
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVm.GetHandlePtr()));

	rmDir.del( QFileInfo(sVmHomePath).absolutePath() );
	AutoDeleteVm autoDeleter(hVm);

	EXTRACT_HANDLE_AS_XML_MODEL_OBJECT( hVm, xmlVm );

	//////////////////////////////////////////////////////////////////////////
	////               CHECK PATCHES                                       ///
	QVERIFY_BRES( bRes, pCheckObj->checkPatchedField(bRes, &xmlVm) );
	//////////////////////////////////////////////////////////////////////////}

	bRes = true;
}

void DispFunctionalityTest::testSCSIBusLogicIncompatibleWithEfi_addBusLogicDiskWithEfiEnabled()
{
	// 1. Create a VM.
	// 2. Enable EFI firmware.
	// 3. Try to add a SCSI BusLogic disk to the VM - check, that the attempt failed.
	//////////////////////////////////////////////////////////////////////////
	SdkHandleWrap hVmHandle, hHddHandle;

	test_login();

	// 1. Create a VM.
	SET_DEFAULT_CONFIG( hVmHandle, PVS_GUEST_VER_WIN_LAST, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(hVmHandle, QSTR2UTF8( GEN_VM_NAME_BY_TEST_FUNCTION() ) ) );

	CHECK_JOB_RET_CODE( PrlVm_Reg(hVmHandle, "", PRL_TRUE) );
	AutoDeleteVm autoDeleter(hVmHandle);
	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) );

	// 2. Enable EFI firmware.
	CHECK_JOB_RET_CODE( PrlVm_BeginEdit(hVmHandle) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetEfiEnabled(hVmHandle, PRL_TRUE) );
	CHECK_JOB_RET_CODE( PrlVm_Commit(hVmHandle) );
	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) )

	// 3. Try to add a SCSI BusLogic disk to the VM - check, that the attempt failed.
	CHECK_JOB_RET_CODE( PrlVm_BeginEdit(hVmHandle) );

	CHECK_RET_CODE_EXP( PrlVmCfg_AddDefaultDeviceEx( \
		 hVmHandle, m_hSrvConfig, PDE_HARD_DISK, hHddHandle.GetHandlePtr()) );
	CHECK_RET_CODE_EXP( PrlVmDev_SetIfaceType(hHddHandle, PMS_SCSI_DEVICE ) );
	CHECK_RET_CODE_EXP( PrlVmDev_SetSubType(hHddHandle, PCD_BUSLOGIC) );

	CHECK_JOB_RET_CODE( PrlVmDev_CreateImage( hHddHandle, PRL_TRUE, PRL_TRUE ) );
	CHECK_ASYNC_OP_FAILED( PrlVm_Commit(hVmHandle), PRL_ERR_VMCONF_SCSI_BUSLOGIC_WITH_EFI_NOT_SUPPORTED );
}

void DispFunctionalityTest::testSCSIBusLogicIncompatibleWithEfi_addBusLogicDiskWithEfiDisabled()
{
	// 1. Create a VM.
	// 2. Disable EFI firmware.
	// 3. Try to add a SCSI BusLogic disk to the VM - check, that the attempt succeeded.
	//////////////////////////////////////////////////////////////////////////
	SdkHandleWrap hVmHandle, hHddHandle;

	test_login();

	// 1. Create a VM.
	SET_DEFAULT_CONFIG( hVmHandle, PVS_GUEST_VER_WIN_LAST, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(hVmHandle, QSTR2UTF8( GEN_VM_NAME_BY_TEST_FUNCTION() ) ) );

	CHECK_JOB_RET_CODE( PrlVm_Reg(hVmHandle, "", PRL_TRUE) );
	AutoDeleteVm autoDeleter(hVmHandle);
	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) );

	// 2. Disable EFI firmware.
	CHECK_JOB_RET_CODE( PrlVm_BeginEdit(hVmHandle) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetEfiEnabled(hVmHandle, PRL_FALSE) );
	CHECK_JOB_RET_CODE( PrlVm_Commit(hVmHandle) );
	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) )

	// 3. Add a SCSI BusLogic disk to the VM - check, that the attempt succeeded.
	CHECK_JOB_RET_CODE( PrlVm_BeginEdit(hVmHandle) );

	CHECK_RET_CODE_EXP( PrlVmCfg_AddDefaultDeviceEx( \
		 hVmHandle, m_hSrvConfig, PDE_HARD_DISK, hHddHandle.GetHandlePtr()) );
	CHECK_RET_CODE_EXP( PrlVmDev_SetIfaceType(hHddHandle, PMS_SCSI_DEVICE ) );
	CHECK_RET_CODE_EXP( PrlVmDev_SetSubType(hHddHandle, PCD_BUSLOGIC) );

	CHECK_JOB_RET_CODE( PrlVmDev_CreateImage( hHddHandle, PRL_TRUE, PRL_TRUE ) );
	CHECK_JOB_RET_CODE( PrlVm_Commit(hVmHandle) );
}

void DispFunctionalityTest::testSCSIBusLogicIncompatibleWithEfi_EnableEfiWithBusLogicDisk()
{
	// 1. Create a VM.
	// 2. Disable EFI firmware.
	// 3. Add a SCSI BusLogic disk to the VM.
	// 4. Try to enable EFI firmware - check, that the attempt failed.
	// 5. Remove the SCSI BusLogic disk.
	// 6. Enable EFI firmware - check, that the attempt succeeded.
	//////////////////////////////////////////////////////////////////////////
	SdkHandleWrap hVmHandle, hHddHandle;

	test_login();

	// 1. Create a VM.
	SET_DEFAULT_CONFIG( hVmHandle, PVS_GUEST_VER_WIN_LAST, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(hVmHandle, QSTR2UTF8( GEN_VM_NAME_BY_TEST_FUNCTION() ) ) );

	CHECK_JOB_RET_CODE( PrlVm_Reg(hVmHandle, "", PRL_TRUE) );
	AutoDeleteVm autoDeleter(hVmHandle);
	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) );

	// 2. Disable EFI firmware.
	CHECK_JOB_RET_CODE( PrlVm_BeginEdit(hVmHandle) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetEfiEnabled(hVmHandle, PRL_FALSE) );
	CHECK_JOB_RET_CODE( PrlVm_Commit(hVmHandle) );
	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) )

	// 3. Add a SCSI BusLogic disk to the VM.
	CHECK_JOB_RET_CODE( PrlVm_BeginEdit(hVmHandle) );

	CHECK_RET_CODE_EXP( PrlVmCfg_AddDefaultDeviceEx( \
		 hVmHandle, m_hSrvConfig, PDE_HARD_DISK, hHddHandle.GetHandlePtr()) );
	CHECK_RET_CODE_EXP( PrlVmDev_SetIfaceType(hHddHandle, PMS_SCSI_DEVICE ) );
	CHECK_RET_CODE_EXP( PrlVmDev_SetSubType(hHddHandle, PCD_BUSLOGIC) );

	CHECK_JOB_RET_CODE( PrlVmDev_CreateImage( hHddHandle, PRL_TRUE, PRL_TRUE ) );
	CHECK_JOB_RET_CODE( PrlVm_Commit(hVmHandle) );
	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) );

	// 4. Try to enable EFI firmware - check, that the attempt failed.
	CHECK_JOB_RET_CODE( PrlVm_BeginEdit(hVmHandle) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetEfiEnabled(hVmHandle, PRL_TRUE) );
	CHECK_ASYNC_OP_FAILED( PrlVm_Commit(hVmHandle), PRL_ERR_VMCONF_SCSI_BUSLOGIC_WITH_EFI_NOT_SUPPORTED );

	// 5. Remove the SCSI BusLogic disk.
	CHECK_JOB_RET_CODE( PrlVm_BeginEdit(hVmHandle) );
	CHECK_RET_CODE_EXP( PrlVmDev_Remove(hHddHandle) );
	CHECK_JOB_RET_CODE( PrlVm_Commit(hVmHandle) );
	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) )

	// 6. Enable EFI firmware - check, that the attempt succeeded.
	CHECK_JOB_RET_CODE( PrlVm_BeginEdit(hVmHandle) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetEfiEnabled(hVmHandle, PRL_TRUE) );
	CHECK_JOB_RET_CODE( PrlVm_Commit(hVmHandle) );
}

void DispFunctionalityTest::testSCSIBusLogicIncompatibleWithEfi_addLsiSpiDiskWithEfiEnabled()
{
	// 1. Create a VM.
	// 2. Enable EFI firmware.
	// 3. Try to add a SCSI LSI-SPI disk to the VM - check, that the attempt succeeded.
	//////////////////////////////////////////////////////////////////////////
	SdkHandleWrap hVmHandle, hHddHandle;

	test_login();

	// 1. Create a VM.
	SET_DEFAULT_CONFIG( hVmHandle, PVS_GUEST_VER_WIN_LAST, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(hVmHandle, QSTR2UTF8( GEN_VM_NAME_BY_TEST_FUNCTION() ) ) );

	CHECK_JOB_RET_CODE( PrlVm_Reg(hVmHandle, "", PRL_TRUE) );
	AutoDeleteVm autoDeleter(hVmHandle);
	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) );

	// 2. Enable EFI firmware.
	CHECK_JOB_RET_CODE( PrlVm_BeginEdit(hVmHandle) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetEfiEnabled(hVmHandle, PRL_TRUE) );
	CHECK_JOB_RET_CODE( PrlVm_Commit(hVmHandle) );
	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) )

	// 3. Try to add a SCSI LSI-SPI disk to the VM - check, that the attempt succeeded.
	CHECK_JOB_RET_CODE( PrlVm_BeginEdit(hVmHandle) );

	CHECK_RET_CODE_EXP( PrlVmCfg_AddDefaultDeviceEx( \
		 hVmHandle, m_hSrvConfig, PDE_HARD_DISK, hHddHandle.GetHandlePtr()) );
	CHECK_RET_CODE_EXP( PrlVmDev_SetIfaceType(hHddHandle, PMS_SCSI_DEVICE ) );
	CHECK_RET_CODE_EXP( PrlVmDev_SetSubType(hHddHandle, PCD_LSI_SPI) );

	CHECK_JOB_RET_CODE( PrlVmDev_CreateImage( hHddHandle, PRL_TRUE, PRL_TRUE ) );
	CHECK_JOB_RET_CODE( PrlVm_Commit(hVmHandle) );
}

void DispFunctionalityTest::testSCSIBusLogicIncompatibleWithEfi_addLsiSasDiskWithEfiEnabled()
{
	// 1. Create a VM.
	// 2. Enable EFI firmware.
	// 3. Try to add a SCSI LSI-SAS disk to the VM - check, that the attempt succeeded.
	//////////////////////////////////////////////////////////////////////////
	SdkHandleWrap hVmHandle, hHddHandle;

	test_login();

	// 1. Create a VM.
	SET_DEFAULT_CONFIG( hVmHandle, PVS_GUEST_VER_WIN_LAST, PRL_FALSE);
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName(hVmHandle, QSTR2UTF8( GEN_VM_NAME_BY_TEST_FUNCTION() ) ) );

	CHECK_JOB_RET_CODE( PrlVm_Reg(hVmHandle, "", PRL_TRUE) );
	AutoDeleteVm autoDeleter(hVmHandle);
	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) );

	// 2. Enable EFI firmware.
	CHECK_JOB_RET_CODE( PrlVm_BeginEdit(hVmHandle) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetEfiEnabled(hVmHandle, PRL_TRUE) );
	CHECK_JOB_RET_CODE( PrlVm_Commit(hVmHandle) );
	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) )

	// 3. Try to add a SCSI LSI-SAS disk to the VM - check, that the attempt succeeded.
	CHECK_JOB_RET_CODE( PrlVm_BeginEdit(hVmHandle) );

	CHECK_RET_CODE_EXP( PrlVmCfg_AddDefaultDeviceEx( \
		 hVmHandle, m_hSrvConfig, PDE_HARD_DISK, hHddHandle.GetHandlePtr()) );
	CHECK_RET_CODE_EXP( PrlVmDev_SetIfaceType(hHddHandle, PMS_SCSI_DEVICE ) );
	CHECK_RET_CODE_EXP( PrlVmDev_SetSubType(hHddHandle, PCD_LSI_SAS) );
	CHECK_JOB_RET_CODE( PrlVmDev_CreateImage( hHddHandle, PRL_TRUE, PRL_TRUE ) );

	CHECK_JOB_RET_CODE( PrlVm_Commit(hVmHandle) );
}


void DispFunctionalityTest::testCreateVmAndSendProblemReport_OnBehalfOfVm_ToServer()
{
    SdkHandleWrap hVmHandle;

    test_login();

    SET_DEFAULT_CONFIG( hVmHandle, PVS_GUEST_VER_WIN_LAST, PRL_FALSE);
    CHECK_RET_CODE_EXP( PrlVmCfg_SetName(hVmHandle, QSTR2UTF8( GEN_VM_NAME_BY_TEST_FUNCTION() ) ) );

    CHECK_JOB_RET_CODE( PrlVm_Reg(hVmHandle, "", PRL_TRUE) );
    AutoDeleteVm autoDeleter(hVmHandle);
    CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) );

    PRL_HANDLE hPR;
    PRL_RESULT res = PrlApi_CreateProblemReport(PRS_OLD_XML_BASED, &hPR);
    QVERIFY(!PRL_FAILED(res));
    CHECK_ASYNC_OP_FAILED(PrlVm_SendProblemReport(hVmHandle, hPR, 0), PRL_ERR_UNIMPLEMENTED);
}

void DispFunctionalityTest::createExtDisk(const SdkHandleWrap& hVmHandle, SdkHandleWrap& hHddHandle,
		const QString& sHddPath, bool& bRes)
{
	bRes = false;
	CHECK_JOB_RET_CODE( PrlVm_BeginEdit( hVmHandle ) );

	CHECK_RET_CODE_EXP( PrlVmCfg_AddDefaultDeviceEx( hVmHandle, m_hSrvConfig,
				PDE_HARD_DISK, hHddHandle.GetHandlePtr()) );
	CHECK_RET_CODE_EXP( PrlVmDevHd_SetDiskType(hHddHandle, PHD_EXPANDING_HARD_DISK) );
	CHECK_RET_CODE_EXP( PrlVmDevHd_SetDiskSize(hHddHandle, 1000 ) );
	CHECK_RET_CODE_EXP( PrlVmDev_SetSysName(hHddHandle, QSTR2UTF8( sHddPath )) );
	CHECK_RET_CODE_EXP( PrlVmDev_SetFriendlyName(hHddHandle, QSTR2UTF8( sHddPath )) );
	CHECK_JOB_RET_CODE( PrlVmDev_CreateImage( hHddHandle, PRL_TRUE, PRL_TRUE ) );
	CHECK_RET_CODE_EXP( PrlVmDev_SetImagePath(hHddHandle, QSTR2UTF8( sHddPath )) );

	CHECK_JOB_RET_CODE( PrlVm_Commit( hVmHandle ) );
	bRes = true;
}

void DispFunctionalityTest::testFailOnVmRenameIfTargetExternalDiskBundleExitst()
{
	SdkHandleWrap hDiskHandle, hVmHandle, hHddHandle;
	QString sVmUuid;
	AutoDirRemover dirRem;
	bool bRes;
	QString nameBase( GEN_VM_NAME_BY_TEST_FUNCTION() ), nameOld, nameNew;
	nameOld = nameBase + "_old";
	nameNew = nameBase + "_new";

	QString diskOld, diskNew;
	QString bundleOld, bundleNew;

	test_login();

	QString tmpDir = QString("%1/%2").arg( QDir::tempPath() ).arg( Uuid::createUuid().toString() );
	QVERIFY( QDir().mkdir(tmpDir) );

	QString bundleSfx(".pvm"), diskName("disk");
	bundleOld = tmpDir + QDir::separator() + nameOld + bundleSfx;
	bundleNew = tmpDir + QDir::separator() + nameNew + bundleSfx;
	diskOld = bundleOld + QDir::separator() + diskName;
	diskNew = bundleNew + QDir::separator() + diskName;

	// create vm with external disk under bundle
	createVm(nameOld, bRes, hVmHandle, sVmUuid);
	AutoDeleteVm autoDeleter(hVmHandle);
	QVERIFY( bRes );
	createExtDisk(hVmHandle, hHddHandle, diskOld, bRes);
	QVERIFY( bRes );
	QVERIFY( QFileInfo(diskOld).exists() );

	// create target bundle for external disk
	QVERIFY( QDir().mkdir(bundleNew) );

	// try to rename vm and check operation fail
	QVERIFY( !QFileInfo(diskNew).exists() );
	PRL_UINT32 nFlags = PVCF_RENAME_EXT_DISKS;
	CHECK_JOB_RET_CODE( PrlVm_BeginEdit( hVmHandle ) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName( hVmHandle, QSTR2UTF8( nameNew ) ) );
	CHECK_ASYNC_OP_FAILED( PrlVm_CommitEx( hVmHandle, nFlags ), PRL_ERR_EXT_DISK_ALREADY_EXISTS);
	QVERIFY( QFileInfo(diskOld).exists() );

	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) );

	QString diskRenamed, nameRenamed;

	PRL_EXTRACT_STRING_VALUE( nameRenamed, hVmHandle, PrlVmCfg_GetName );
	QCOMPARE ( nameRenamed, nameOld );
	PRL_EXTRACT_STRING_VALUE( diskRenamed, hHddHandle, PrlVmDev_GetSysName );
	QCOMPARE ( diskRenamed, diskOld );
}

void DispFunctionalityTest::testDontRenameExternalDisksBundlesOnVmRenameIfFlagIsNotSet()
{
	SdkHandleWrap hDiskHandle, hVmHandle, hHddHandle;
	QString sVmUuid;
	AutoDirRemover dirRem;
	bool bRes;
	QString nameBase( GEN_VM_NAME_BY_TEST_FUNCTION() ), nameOld, nameNew;
	nameOld = nameBase + "_old";
	nameNew = nameBase + "_new";

	QString diskOld, diskNew;
	QString bundleOld, bundleNew;

	test_login();

	QString tmpDir = QString("%1/%2").arg( QDir::tempPath() ).arg( Uuid::createUuid().toString() );
	QVERIFY( QDir().mkdir(tmpDir) );
	dirRem.add(tmpDir);

	QString bundleSfx(".pvm"), diskName("disk");
	bundleOld = tmpDir + QDir::separator() + nameOld + bundleSfx;
	bundleNew = tmpDir + QDir::separator() + nameNew + bundleSfx;
	diskOld = bundleOld + QDir::separator() + diskName;
	diskNew = bundleNew + QDir::separator() + diskName;

	// create vm with external disk under bundle
	createVm(nameOld, bRes, hVmHandle, sVmUuid);
	AutoDeleteVm autoDeleter(hVmHandle);
	QVERIFY( bRes );
	createExtDisk(hVmHandle, hHddHandle, diskOld, bRes);
	QVERIFY( bRes );
	QVERIFY( QFileInfo(diskOld).exists() );

	// rename vm and check external disk doesn't get moved
	QVERIFY( !QFileInfo(diskNew).exists() );
	PRL_UINT32 nFlags = 0;
	CHECK_JOB_RET_CODE( PrlVm_BeginEdit( hVmHandle ) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName( hVmHandle, QSTR2UTF8( nameNew ) ) );
	CHECK_JOB_RET_CODE( PrlVm_CommitEx( hVmHandle, nFlags ) );
	QVERIFY( QFileInfo(diskOld).exists() );
	QVERIFY( !QFileInfo(diskNew).exists() );

	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) );

	QString diskRenamed, nameRenamed;

	PRL_EXTRACT_STRING_VALUE( nameRenamed, hVmHandle, PrlVmCfg_GetName );
	QCOMPARE ( nameRenamed, nameNew );
	PRL_EXTRACT_STRING_VALUE( diskRenamed, hHddHandle, PrlVmDev_GetSysName );
	QCOMPARE ( diskRenamed, diskOld );
}

void DispFunctionalityTest::testRenameExternalDisksBundlesOnVmRename()
{
	SdkHandleWrap hDiskHandle, hVmHandle, hHddHandle;
	QString sVmUuid;
	AutoDirRemover dirRem;
	bool bRes;
	QString nameBase( GEN_VM_NAME_BY_TEST_FUNCTION() ), nameOld, nameNew;
	nameOld = nameBase + "_old";
	nameNew = nameBase + "_new";

	QString diskOld, diskNew;
	QString bundleOld, bundleNew;

	test_login();

	QString tmpDir = QString("%1/%2").arg( QDir::tempPath() ).arg( Uuid::createUuid().toString() );
	QVERIFY( QDir().mkdir(tmpDir) );
	dirRem.add(tmpDir);

	QString bundleSfx(".pvm"), diskName("disk");
	bundleOld = tmpDir + QDir::separator() + nameOld + bundleSfx;
	bundleNew = tmpDir + QDir::separator() + nameNew + bundleSfx;
	diskOld = bundleOld + QDir::separator() + diskName;
	diskNew = bundleNew + QDir::separator() + diskName;

	// create vm with external disk under bundle
	createVm(nameOld, bRes, hVmHandle, sVmUuid);
	AutoDeleteVm autoDeleter(hVmHandle);
	QVERIFY( bRes );
	createExtDisk(hVmHandle, hHddHandle, diskOld, bRes);
	QVERIFY( bRes );
	QVERIFY( QFileInfo(diskOld).exists() );

	// rename vm and check disk bundle renamed too
	QVERIFY( !QFileInfo(diskNew).exists() );
	PRL_UINT32 nFlags = PVCF_RENAME_EXT_DISKS;
	CHECK_JOB_RET_CODE( PrlVm_BeginEdit( hVmHandle ) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName( hVmHandle, QSTR2UTF8( nameNew ) ) );
	CHECK_JOB_RET_CODE( PrlVm_CommitEx( hVmHandle, nFlags ) );
	QVERIFY( !QFileInfo(diskOld).exists() );
	QVERIFY( QFileInfo(diskNew).exists() );

	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) );

	QString diskRenamed, nameRenamed;

	PRL_EXTRACT_STRING_VALUE( nameRenamed, hVmHandle, PrlVmCfg_GetName );
	QCOMPARE ( nameRenamed, nameNew );
	PRL_EXTRACT_STRING_VALUE( diskRenamed, hHddHandle, PrlVmDev_GetSysName );
	QCOMPARE ( diskRenamed, diskNew );
}

void DispFunctionalityTest::testKeepExternalDisksWithoutBundlesOnVmRename()
{
	SdkHandleWrap hDiskHandle, hVmHandle, hHddHandle;
	QString sVmUuid;
	AutoDirRemover dirRem;
	bool bRes;
	QString nameBase( GEN_VM_NAME_BY_TEST_FUNCTION() ), nameOld, nameNew;
	nameOld = nameBase + "_old";
	nameNew = nameBase + "_new";

	test_login();

	QString tmpDir = QString("%1/%2").arg( QDir::tempPath() ).arg( Uuid::createUuid().toString() );
	QVERIFY( QDir().mkdir(tmpDir) );
	dirRem.add(tmpDir);

	QString diskPath = tmpDir + QDir::separator() + "disk";

	// create vm with external disk without bundle
	createVm(nameOld, bRes, hVmHandle, sVmUuid);
	AutoDeleteVm autoDeleter(hVmHandle);
	QVERIFY( bRes );
	createExtDisk(hVmHandle, hHddHandle, diskPath, bRes);
	QVERIFY( bRes );
	QVERIFY( QFileInfo(diskPath).exists() );

	// rename vm and check disk doesn't get moved
	PRL_UINT32 nFlags = PVCF_RENAME_EXT_DISKS;
	CHECK_JOB_RET_CODE( PrlVm_BeginEdit( hVmHandle ) );
	CHECK_RET_CODE_EXP( PrlVmCfg_SetName( hVmHandle, QSTR2UTF8( nameNew ) ) );
	CHECK_JOB_RET_CODE( PrlVm_CommitEx( hVmHandle, nFlags ) );
	QVERIFY( QFileInfo(diskPath).exists() );

	CHECK_JOB_RET_CODE( PrlVm_RefreshConfig(hVmHandle) );

	QString diskRenamed, nameRenamed;

	PRL_EXTRACT_STRING_VALUE( nameRenamed, hVmHandle, PrlVmCfg_GetName );
	QCOMPARE ( nameRenamed, nameNew );
	PRL_EXTRACT_STRING_VALUE( diskRenamed, hHddHandle, PrlVmDev_GetSysName );
	QCOMPARE ( diskRenamed, diskPath );
}

