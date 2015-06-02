///
///	@file PrlSrvManipulationsTest.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing server login SDK API.
///
///	@author sandro
///
/// Copyright (c) 1999-2015 Parallels IP Holdings GmbH
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

#include "Build/Current.ver"
#include "PrlSrvManipulationsTest.h"

#include <QDir>
#include <QTime>

#include "Tests/CommonTestsUtils.h"
#include "Tests/AclTestsUtils.h"
#include "Tests/SDKTest/AutoHelpers.h"
#include "TestCallbackCommon.h"

#include "XmlModel/HostHardwareInfo/CHostHardwareInfo.h"
#include "XmlModel/HostHardwareInfo/CSystemStatistics.h"
#include "XmlModel/HostHardwareInfo/CHwFileSystemInfo.h"
#include "XmlModel/DispConfig/CDispUser.h"
#include "XmlModel/DispConfig/CDispCommonPreferences.h"
#include "XmlModel/DispConfig/CDispNetAdapter.h"
#include "XmlModel/DispConfig/CDispDhcpPreferences.h"
#include "XmlModel/Appliance/CAppliance.h"
#include "XmlModel/CtTemplate/CtTemplate.h"
#include "XmlModel/UserInformation/UserInfo.h"

#include "Libraries/Logging/Logging.h"
#include "Libraries/Std/SmartPtr.h"
#include "Libraries/PrlUuid/Uuid.h"
#include "Libraries/PrlCommonUtils/CAuthHelper.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"
#include "Libraries/PrlCommonUtilsBase/StringUtils.h"
#include "Libraries/HostUtils/HostUtils.h"
#include "Libraries/TgzExtracter/TgzExtracterLib.h"

#include "XmlModel/Messaging/CVmEvent.h"
#include "XmlModel/Messaging/CVmEventParameter.h"
#include "XmlModel/VmConfig/CVmConfiguration.h"
#include "XmlModel/VmDirectory/CVmDirectories.h"
#include "XmlModel/ProblemReport/CProblemReport.h"

#include "Libraries/ConfigConverter/ConfigFile.h"
#include "Libraries/ConfigConverter/ConfigConsts.h"
#include "Libraries/ProblemReportUtils/CProblemReportUtils.h"
#include "Libraries/ProblemReportUtils/CPackedProblemReport.h"
#include "Libraries/PrlNetworking/PrlNetLibrary.h"
#include "Libraries/IOService/src/IOCommunication/IOProtocol.h"

#include "Interfaces/ParallelsNamespaceTests.h"
#include "Interfaces/ApiDevNums.h"
#include "Interfaces/ParallelsSdkPrivate.h"

using namespace Parallels;

#ifdef _WIN_
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#endif

#ifdef _LIN_
#include "Libraries/Virtuozzo/CVzHelper.h"
#endif

/**
 * Returns currently logged in user id. Under Win platform it's access token of current process.
 * Under Unix like platforms - UID.
 */
qint32 PrlGetCurrentUserId()
{
#ifdef _WIN_
	return ((qint32)GetCurrentProcessId());
#else
	return (getuid());
#endif
}

namespace {
/**
 * Determines whether specified user is administrator
 * @param user name
 * @param user password
 */
bool PrlIsUserAdministrator(const QString &sUserName, const QString &sUserPassword)
{
	CAuthHelper _auth(sUserName);

	if (_auth.AuthUser(sUserPassword))
		return (_auth.isLocalAdministrator());
	else
		WRITE_TRACE(DBG_FATAL, "Couldn't to authorize user: login '%s' password '%s'",\
			QSTR2UTF8(sUserName), QSTR2UTF8(sUserPassword));
	return (false);
}
/**
 * Overloaded above method version that let to authorize user buy current session id
 */
bool PrlIsUserAdministrator()
{
	CAuthHelper _auth;

	if (_auth.AuthUser(PrlGetCurrentUserId()))
		return (_auth.isLocalAdministrator());
	else
		WRITE_TRACE(DBG_FATAL, "Couldn't to authorize user with session id %d", PrlGetCurrentUserId());
	return (false);
}

#ifdef _WIN_
QString GetWindowsTemporaryDirPath()
{
	TCHAR buf[ MAX_PATH ];
	GetWindowsDirectory( buf, sizeof( buf ) / sizeof( buf[0] ) );

	QString rootDirPath = UTF16_2QSTR( buf );
	return (rootDirPath + "/Temp");
}
#endif

	QString  g_sTmpUserVmDir
#	ifdef _WIN_
		= GetWindowsTemporaryDirPath();
#	else
		= "/tmp";
#	endif

}

#define LOGIN_TO_SERVER\
	{\
		if (TestConfig::isServerMode())\
			testLogin();\
		else\
			testLoginLocal();\
		PRL_BOOL bConnected = false;\
		PRL_BOOL bIsConnectionLocal = false;\
		CHECK_RET_CODE_EXP(PrlSrv_IsConnected(m_ServerHandle, &bConnected, &bIsConnectionLocal))\
		QVERIFY(bConnected);\
	}

void PrlSrvManipulationsTest::init()
{
#ifdef _WIN_
	m_sTestFsDirName1 = QDir::tempPath() + '/' + Uuid::createUuid().toString();
	m_sTestFsDirName2 = QDir::tempPath() + '/' + Uuid::createUuid().toString();
	m_sTestFsDirName3 = QDir::tempPath() + '/' + Uuid::createUuid().toString();
	m_sTestConvertDirName = QDir::tempPath() + '/' + Uuid::createUuid().toString();
#else
	//https://bugzilla.sw.ru/show_bug.cgi?id=438537
	m_sTestFsDirName1 = "/tmp/" + Uuid::createUuid().toString();
	m_sTestFsDirName2 = "/tmp/" + Uuid::createUuid().toString();
	m_sTestFsDirName3 = "/tmp/" + Uuid::createUuid().toString();
	m_sTestConvertDirName = "/tmp/" + Uuid::createUuid().toString();
#endif
	m_sTestFsDirName1ChildDir = m_sTestFsDirName1 + '/' + Uuid::createUuid().toString();
	m_sTestFsDirName1ChildChildDir = m_sTestFsDirName1ChildDir + '/' + Uuid::createUuid().toString();
	m_sTestFsDirName1ChildChildChildDir = m_sTestFsDirName1ChildChildDir + '/' + Uuid::createUuid().toString();
	QVERIFY(!QFile::exists(m_sTestFsDirName1));
	QVERIFY(!QFile::exists(m_sTestFsDirName2));
	QVERIFY(!QFile::exists(m_sTestFsDirName3));
	CHECK_RET_CODE_EXP(PrlSrv_Create(m_ServerHandle.GetHandlePtr()))
}

void PrlSrvManipulationsTest::cleanup()
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
		m_VmHandle.reset();
	}

	m_JobHandle.reset(PrlSrv_Logoff(m_ServerHandle));
	PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);

	m_ResultHandle.reset();
	m_JobHandle.reset();
	m_ServerHandle.reset();

#define CLEANUP_DIR(path)\
	if ( QFile::exists( path ) )\
		CFileHelper::ClearAndDeleteDir( path );

	CLEANUP_DIR(m_sTestFsDirName1ChildChildChildDir + ".pvm");
	CLEANUP_DIR(m_sTestFsDirName1ChildChildDir + ".pvm");
	CLEANUP_DIR(m_sTestFsDirName1ChildDir + ".pvm");
	CLEANUP_DIR(m_sTestFsDirName1 + ".pvm");
	CLEANUP_DIR(m_sTestFsDirName2 + ".pvm");
	CLEANUP_DIR(m_sTestFsDirName3 + ".pvm");

	CLEANUP_DIR(m_sTestFsDirName1ChildChildChildDir);
	CLEANUP_DIR(m_sTestFsDirName1ChildChildDir);
	CLEANUP_DIR(m_sTestFsDirName1ChildDir);
	CLEANUP_DIR(m_sTestFsDirName1);
	CLEANUP_DIR(m_sTestFsDirName2);
	CLEANUP_DIR(m_sTestFsDirName3);
	CLEANUP_DIR(m_sTestConvertDirName);
#undef CLEANUP_DIR
}

void PrlSrvManipulationsTest::testLogin()
{
	if (!TestConfig::isServerMode())
		QSKIP("Skipping test due functionality is not supported at desktop mode", SkipAll);

	m_JobHandle.reset(PrlSrv_LoginEx(m_ServerHandle, TestConfig::getRemoteHostName(), TestConfig::getUserLogin(),
									 TestConfig::getUserPassword(), NULL, 0, 0, PSL_HIGH_SECURITY, 0));
	QVERIFY(m_JobHandle != PRL_INVALID_HANDLE);
	PRL_HANDLE_TYPE _handle_type;
	QVERIFY(PrlHandle_GetType(m_JobHandle, &_handle_type) == PRL_ERR_SUCCESS);
	QVERIFY(_handle_type == PHT_JOB);
	CHECK_JOB_RET_CODE(m_JobHandle)
}

void PrlSrvManipulationsTest::testLoginOnIncorrectHostname() {
    m_JobHandle.reset(PrlSrv_Login(m_ServerHandle, "incorrect host name", TestConfig::getUserLogin(),
								   TestConfig::getUserPassword(), NULL, 0, 0, PSL_HIGH_SECURITY));
    QVERIFY(m_JobHandle != PRL_INVALID_HANDLE);
    QVERIFY(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT) == PRL_ERR_SUCCESS);
    PRL_RESULT _job_rc;
    QVERIFY(PrlJob_GetRetCode(m_JobHandle, &_job_rc) == PRL_ERR_SUCCESS);
    QVERIFY(!PRL_SUCCEEDED(_job_rc));
}

void PrlSrvManipulationsTest::testLoginOnInvalidHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_Login(PRL_INVALID_HANDLE, TestConfig::getRemoteHostName(),	TestConfig::getUserLogin(),
													TestConfig::getUserPassword(), NULL, 0, 0, PSL_HIGH_SECURITY), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testLoginLocal()
{
	m_JobHandle.reset(PrlSrv_LoginLocal(m_ServerHandle, "", 0, PSL_HIGH_SECURITY));
	QVERIFY(m_JobHandle != PRL_INVALID_HANDLE);
	CHECK_JOB_RET_CODE(m_JobHandle)
	m_JobHandle.reset(PrlSrv_GetUserProfile(m_ServerHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)
}

void PrlSrvManipulationsTest::testLoginLocalOnInvalidHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_LoginLocal(PRL_INVALID_HANDLE, "", 0, PSL_HIGH_SECURITY), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testLoginLocalOnNonServerHandle()
{
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlSrv_LoginLocal(hVm, "", 0, PSL_HIGH_SECURITY), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testLogoffOnInvalidHandle() {
	CHECK_ASYNC_OP_FAILED(PrlSrv_Logoff(PRL_INVALID_HANDLE), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testGetSrvConfig()
{
	LOGIN_TO_SERVER
	m_JobHandle.reset(PrlSrv_GetSrvConfig(m_ServerHandle));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))
	PRL_RESULT _ret_code = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(m_JobHandle, &_ret_code))
	CHECK_RET_CODE_EXP(_ret_code)
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	PRL_VOID_PTR pResString = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamToken(m_ResultHandle, 0, &pResString))
	QVERIFY(pResString);
	QString sHostHwInfo = UTF8_2QSTR((const char *)pResString);
	PrlBuffer_Free(pResString);
	QVERIFY(sHostHwInfo.size());
	CHostHardwareInfo _hw(sHostHwInfo);
	QVERIFY(_hw.m_uiRcInit == PRL_ERR_SUCCESS);
}

void PrlSrvManipulationsTest::testGetSrvConfigOnInvalidServerHandle()
{
	LOGIN_TO_SERVER
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetSrvConfig(PRL_INVALID_HANDLE), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testGetSrvConfigOnNonServerHandle()
{
	LOGIN_TO_SERVER
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetSrvConfig(m_ResultHandle), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testFsGetDiskList()
{
	LOGIN_TO_SERVER
	m_JobHandle.reset(PrlSrv_FsGetDiskList(m_ServerHandle));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))
	PRL_RESULT _ret_code = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(m_JobHandle, &_ret_code))
	CHECK_RET_CODE_EXP(_ret_code)
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	PRL_VOID_PTR pParam = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamToken(m_ResultHandle, 0, &pParam))
	QString sParam = UTF8_2QSTR((const char *)pParam);
	PrlBuffer_Free(pParam);
	QVERIFY(sParam.size());
	CHwFileSystemInfo _fs;
	QCOMPARE(int(_fs.fromString(sParam)), int(HostHwInfoParser::RcSuccess));
}

void PrlSrvManipulationsTest::testFsGetDiskListOnInvalidServerHandle()
{
	LOGIN_TO_SERVER
	CHECK_ASYNC_OP_FAILED(PrlSrv_FsGetDiskList(PRL_INVALID_HANDLE), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testFsGetDiskListOnNonServerHandle()
{
	LOGIN_TO_SERVER
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlSrv_FsGetDiskList(m_ResultHandle), PRL_ERR_INVALID_ARG);
}

#define ROOT_PATH QDir::rootPath().toUtf8().data()

void PrlSrvManipulationsTest::testFsGetDirEntries()
{
	LOGIN_TO_SERVER
	m_JobHandle.reset(PrlSrv_FsGetDirEntries(m_ServerHandle, ROOT_PATH));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))
	PRL_RESULT _ret_code = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(m_JobHandle, &_ret_code))
	CHECK_RET_CODE_EXP(_ret_code)
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	PRL_VOID_PTR pParam = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamToken(m_ResultHandle, 0, &pParam))
	QString sParam = UTF8_2QSTR((const char *)pParam);
	PrlBuffer_Free(pParam);
	QVERIFY(sParam.size());
	CHwFileSystemInfo _fs;
	QCOMPARE(int(_fs.fromString(sParam)), int(HostHwInfoParser::RcSuccess));
}

void PrlSrvManipulationsTest::testFsGetDirEntriesOnInvalidServerHandle()
{
	LOGIN_TO_SERVER
	CHECK_ASYNC_OP_FAILED(PrlSrv_FsGetDirEntries(PRL_INVALID_HANDLE, ROOT_PATH), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testFsGetDirEntriesOnInvalidPointer()
{
	LOGIN_TO_SERVER
	CHECK_ASYNC_OP_FAILED(PrlSrv_FsGetDirEntries(m_ServerHandle, 0), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testFsGetDirEntriesOnNonServerHandle()
{
	LOGIN_TO_SERVER
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlSrv_FsGetDirEntries(m_ResultHandle, ROOT_PATH), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testFsCreateDirOnInvalidServerHandle()
{
	LOGIN_TO_SERVER
	CHECK_ASYNC_OP_FAILED(PrlSrv_FsCreateDir(PRL_INVALID_HANDLE, ROOT_PATH), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testFsCreateDirOnInvalidPointer()
{
	LOGIN_TO_SERVER
	CHECK_ASYNC_OP_FAILED(PrlSrv_FsCreateDir(m_ServerHandle, 0), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testFsCreateDirOnNonServerHandle()
{
	LOGIN_TO_SERVER
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlSrv_FsCreateDir(m_ResultHandle, ROOT_PATH), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testFsRemoveEntryOnInvalidServerHandle()
{
	LOGIN_TO_SERVER
	CHECK_ASYNC_OP_FAILED(PrlSrv_FsRemoveEntry(PRL_INVALID_HANDLE, ROOT_PATH), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testFsRemoveEntryOnInvalidPointer()
{
	LOGIN_TO_SERVER
	CHECK_ASYNC_OP_FAILED(PrlSrv_FsRemoveEntry(m_ServerHandle, 0), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testFsRemoveEntryOnNonServerHandle()
{
	LOGIN_TO_SERVER
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlSrv_FsRemoveEntry(m_ResultHandle, ROOT_PATH), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testFsRenameEntryOnInvalidServerHandle()
{
	LOGIN_TO_SERVER
	CHECK_ASYNC_OP_FAILED(PrlSrv_FsRenameEntry(PRL_INVALID_HANDLE, ROOT_PATH, ROOT_PATH), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testFsRenameEntryOnInvalidPointer()
{
	LOGIN_TO_SERVER
	CHECK_ASYNC_OP_FAILED(PrlSrv_FsRenameEntry(m_ServerHandle, 0, ROOT_PATH), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testFsRenameEntryOnInvalidPointer2()
{
	LOGIN_TO_SERVER
	CHECK_ASYNC_OP_FAILED(PrlSrv_FsRenameEntry(m_ServerHandle, ROOT_PATH, 0), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testFsRenameEntryOnNonServerHandle()
{
	LOGIN_TO_SERVER
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlSrv_FsRenameEntry(m_ResultHandle, ROOT_PATH, ROOT_PATH), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testGetUserProfile()
{
	LOGIN_TO_SERVER
	m_JobHandle.reset(PrlSrv_GetUserProfile(m_ServerHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	QByteArray _buf;
	PRL_UINT32 nBufSize = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(m_ResultHandle, 0, &nBufSize))
	QVERIFY(nBufSize);
	_buf.resize(nBufSize);
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(m_ResultHandle, _buf.data(), &nBufSize))
	QString sUserProfile = UTF8_2QSTR(_buf.constData());
	QVERIFY(sUserProfile.size());
	CDispUser _du(sUserProfile);
	QVERIFY(_du.m_uiRcInit == PRL_ERR_SUCCESS);
}

void PrlSrvManipulationsTest::testGetUserProfileOnInvalidServerHandle()
{
	LOGIN_TO_SERVER
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetUserProfile(PRL_INVALID_HANDLE), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testGetUserProfileOnNonServerHandle()
{
	LOGIN_TO_SERVER
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetUserProfile(m_ResultHandle), PRL_ERR_INVALID_ARG);
}

#define USER_PROFILE_EDIT_PREFACE\
	LOGIN_TO_SERVER\
	m_JobHandle.reset(PrlSrv_GetUserProfile(m_ServerHandle));\
	CHECK_JOB_RET_CODE(m_JobHandle)\
	QVERIFY(PRL_SUCCEEDED(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr())));\
	PRL_VOID_PTR pResString = 0;\
	QVERIFY(PRL_SUCCEEDED(PrlResult_GetParamToken(m_ResultHandle, 0, &pResString)));\
	QVERIFY(pResString);\
	QString sUserProfile = UTF8_2QSTR((const char *)pResString);\
	PrlBuffer_Free(pResString);\
	QVERIFY(sUserProfile.size());\
	CDispUser _du(sUserProfile);\
	QVERIFY(_du.m_uiRcInit == PRL_ERR_SUCCESS);\
	SdkHandleWrap hUserProfile;\
	QVERIFY(PRL_SUCCEEDED(PrlResult_GetParam(m_ResultHandle, hUserProfile.GetHandlePtr())));\
	m_JobHandle.reset(PrlSrv_UserProfileBeginEdit(m_ServerHandle));\
	CHECK_JOB_RET_CODE(m_JobHandle)

void PrlSrvManipulationsTest::testEditUserProfile()
{

	USER_PROFILE_EDIT_PREFACE;

	QString sOldValue = _du.getUserWorkspace()->getDefaultVmFolder();
	QString sSettingValue;

	if ( ! sOldValue.isEmpty() )
		sSettingValue.clear();
	else
		sSettingValue = g_sTmpUserVmDir;

	_du.getUserWorkspace()->setDefaultVmFolder( sSettingValue );
	CHECK_RET_CODE_EXP(PrlUsrCfg_FromString(hUserProfile, _du.toString().toUtf8().data()))

	m_JobHandle.reset(PrlSrv_UserProfileCommit(m_ServerHandle, hUserProfile));
	CHECK_JOB_RET_CODE(m_JobHandle)

	m_JobHandle.reset(PrlSrv_GetUserProfile(m_ServerHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)
	m_ResultHandle.reset(0);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	pResString = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamToken(m_ResultHandle, 0, &pResString))
	QVERIFY(pResString);
	sUserProfile = UTF8_2QSTR((const char *)pResString);
	PrlBuffer_Free(pResString);
	QVERIFY(sUserProfile.size());
	_du.fromString(sUserProfile);
	QVERIFY(_du.m_uiRcInit == PRL_ERR_SUCCESS);

	QVERIFY(_du.getUserWorkspace()->getDefaultVmFolder() == sSettingValue);

	// revert value
	m_JobHandle.reset(PrlSrv_UserProfileBeginEdit(m_ServerHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)

	_du.getUserWorkspace()->setDefaultVmFolder( sOldValue );
	CHECK_RET_CODE_EXP(PrlUsrCfg_FromString(hUserProfile, _du.toString().toUtf8().data()))

	m_JobHandle.reset(PrlSrv_UserProfileCommit(m_ServerHandle, hUserProfile));
	CHECK_JOB_RET_CODE(m_JobHandle)

}

void PrlSrvManipulationsTest::testEditUserProfileTryToChangeUseManagementConsoleFlag()
{
	USER_PROFILE_EDIT_PREFACE
	bool bSettingValue = !_du.getUserAccess()->canUseServerManagementConsole();
	_du.getUserAccess()->setUseServerManagementConsoleFlag(bSettingValue);
	CHECK_RET_CODE_EXP(PrlUsrCfg_FromString(hUserProfile, _du.toString().toUtf8().data()))
	m_JobHandle.reset(PrlSrv_UserProfileCommit(m_ServerHandle, hUserProfile));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))
	PRL_RESULT nJobRetCode = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(m_JobHandle, &nJobRetCode))
	QVERIFY(nJobRetCode == PRL_ERR_USER_CANT_CHANGE_ACCESS_PROFILE);
	m_JobHandle.reset(PrlSrv_GetUserProfile(m_ServerHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)
	m_ResultHandle.reset();
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	pResString = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamToken(m_ResultHandle, 0, &pResString))
	QVERIFY(pResString);
	sUserProfile = UTF8_2QSTR((const char *)pResString);
	PrlBuffer_Free(pResString);
	QVERIFY(sUserProfile.size());
	_du.fromString(sUserProfile);
	QVERIFY(_du.m_uiRcInit == PRL_ERR_SUCCESS);
	QVERIFY(_du.getUserAccess()->canUseServerManagementConsole() != bSettingValue);
}

void PrlSrvManipulationsTest::testEditUserProfileTryToChangeCanChangeServerSettingsFlag()
{
	if ((TestConfig::isServerMode() && PrlIsUserAdministrator(TestConfig::getUserLogin(), TestConfig::getUserPassword())) ||
			(!TestConfig::isServerMode() && PrlIsUserAdministrator()))
		QSKIP("Skipping test due test user account with local administrator privileges", SkipAll);

	USER_PROFILE_EDIT_PREFACE
	bool bSettingValue = !_du.getUserAccess()->canChangeServerSettings();
	_du.getUserAccess()->setChangeServerSettingsFlag(bSettingValue);
	CHECK_RET_CODE_EXP(PrlUsrCfg_FromString(hUserProfile, _du.toString().toUtf8().data()))
	CHECK_ASYNC_OP_FAILED(PrlSrv_UserProfileCommit(m_ServerHandle, hUserProfile),\
		PRL_ERR_USER_CANT_CHANGE_ACCESS_PROFILE)
	m_JobHandle.reset(PrlSrv_GetUserProfile(m_ServerHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)
	m_ResultHandle.reset();
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	pResString = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamToken(m_ResultHandle, 0, &pResString))
	QVERIFY(pResString);
	sUserProfile = UTF8_2QSTR((const char *)pResString);
	PrlBuffer_Free(pResString);
	QVERIFY(sUserProfile.size());
	_du.fromString(sUserProfile);
	QVERIFY(_du.m_uiRcInit == PRL_ERR_SUCCESS);
	QVERIFY(_du.getUserAccess()->canChangeServerSettings() != bSettingValue);
}

void PrlSrvManipulationsTest::testEditUserProfileTryToChangeVmDirectoryUuid()
{
	USER_PROFILE_EDIT_PREFACE;
	QString sSettingValue = Uuid::createUuid().toString();
	_du.getUserWorkspace()->setVmDirectory( sSettingValue );
	CHECK_RET_CODE_EXP(PrlUsrCfg_FromString(hUserProfile, _du.toString().toUtf8().data()))

	m_JobHandle.reset(PrlSrv_UserProfileCommit(m_ServerHandle, hUserProfile));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))

	PRL_RESULT nJobRetCode = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(m_JobHandle, &nJobRetCode))
	//FIXME: Need comment/uncomment after fix bug #2242
	CHECK_RET_CODE_EXP(nJobRetCode)//QVERIFY(nJobRetCode == PRL_ERR_USER_CANT_CHANGE_READ_ONLY_VALUE);


	m_JobHandle.reset(PrlSrv_GetUserProfile(m_ServerHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)
	m_ResultHandle.reset();
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	pResString = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamToken(m_ResultHandle, 0, &pResString))
	QVERIFY(pResString);
	sUserProfile = UTF8_2QSTR((const char *)pResString);
	PrlBuffer_Free(pResString);
	QVERIFY(sUserProfile.size());
	_du.fromString(sUserProfile);
	QVERIFY(_du.m_uiRcInit == PRL_ERR_SUCCESS);
	QVERIFY(_du.getUserWorkspace()->getVmDirectory() != sSettingValue);
}


void PrlSrvManipulationsTest::testGetCommonPrefs()
{
	LOGIN_TO_SERVER
	m_JobHandle.reset(PrlSrv_GetCommonPrefs(m_ServerHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	PRL_VOID_PTR pResString = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamToken(m_ResultHandle, 0, &pResString))
	QVERIFY(pResString);
	QString sCommonPrefs = UTF8_2QSTR((const char *)pResString);
	PrlBuffer_Free(pResString);
	QVERIFY(sCommonPrefs.size());
	CDispCommonPreferences _dcp;
	_dcp.fromString(sCommonPrefs);
	QVERIFY(_dcp.m_uiRcInit == PRL_ERR_SUCCESS);
}

void PrlSrvManipulationsTest::testEditCommonPrefs()
{
	LOGIN_TO_SERVER
//Extracting user profile
	m_JobHandle.reset(PrlSrv_GetUserProfile(m_ServerHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	PRL_VOID_PTR pResString = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamToken(m_ResultHandle, 0, &pResString))
	QVERIFY(pResString);
	QString sUserProfile = UTF8_2QSTR((const char *)pResString);
	PrlBuffer_Free(pResString);
	QVERIFY(sUserProfile.size());
	CDispUser _du(sUserProfile);
	QVERIFY(_du.m_uiRcInit == PRL_ERR_SUCCESS);
//Getting current common server preferences
	m_JobHandle.reset(PrlSrv_GetCommonPrefs(m_ServerHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)
	m_ResultHandle.reset();
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	SdkHandleWrap hCommonPrefs;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(m_ResultHandle, hCommonPrefs.GetHandlePtr()))
	pResString = 0;
	CHECK_RET_CODE_EXP(PrlDispCfg_ToString(hCommonPrefs, &pResString))
	QString sCommonPrefs = UTF8_2QSTR((const char *)pResString);
	PrlBuffer_Free(pResString);
	CDispCommonPreferences _dcp;
	CHECK_RET_CODE_EXP(_dcp.fromString(sCommonPrefs))
//Begining edit of server common preferences
	m_JobHandle.reset(PrlSrv_CommonPrefsBeginEdit(m_ServerHandle));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))
	PRL_RESULT nJobRetCode = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(m_JobHandle, &nJobRetCode))
	if (!_du.getUserAccess()->canChangeServerSettings())
	{
		//User do not have rights for editing server settings check retcode now
		QVERIFY(nJobRetCode == PRL_ERR_USER_NO_AUTH_TO_EDIT_SERVER_SETTINGS);
		return;
	}
	CHECK_RET_CODE_EXP(nJobRetCode)

	bool bSettingValue = !_dcp.getWorkspacePreferences()->isDefaultUseConsole();
	_dcp.getWorkspacePreferences()->setDefaultUseConsole(bSettingValue);
	CHECK_RET_CODE_EXP(PrlDispCfg_FromString(hCommonPrefs, _dcp.toString().toUtf8().data()))
	m_JobHandle.reset(PrlSrv_CommonPrefsCommit(m_ServerHandle, hCommonPrefs));
	CHECK_JOB_RET_CODE(m_JobHandle)

	m_JobHandle.reset(PrlSrv_GetCommonPrefs(m_ServerHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)
	m_ResultHandle.reset();
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	pResString = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamToken(m_ResultHandle, 0, &pResString))
	QVERIFY(pResString);
	sCommonPrefs = UTF8_2QSTR((const char *)pResString);
	PrlBuffer_Free(pResString);
	QVERIFY(sCommonPrefs.size());
	_dcp.fromString(sCommonPrefs);
	QVERIFY(_dcp.m_uiRcInit == PRL_ERR_SUCCESS);
	QVERIFY(_dcp.getWorkspacePreferences()->isDefaultUseConsole() == bSettingValue);
}

#define RECEIVE_SERVER_HW_INFO\
	LOGIN_TO_SERVER\
	m_JobHandle.reset(PrlSrv_GetSrvConfig(m_ServerHandle));\
	CHECK_JOB_RET_CODE(m_JobHandle)\
	QVERIFY(PRL_SUCCEEDED(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr())));\
	SdkHandleWrap hSrvConfig;\
	QVERIFY(PRL_SUCCEEDED(PrlResult_GetParam(m_ResultHandle, hSrvConfig.GetHandlePtr())));\
	PRL_VOID_PTR pBuffer = 0;\
	QVERIFY(PRL_SUCCEEDED(PrlResult_GetParamToken(m_ResultHandle, 0, &pBuffer)));\
	CHostHardwareInfo _hw_info(UTF8_2QSTR((const char *)pBuffer));\
	PrlBuffer_Free(pBuffer);\
	CHECK_RET_CODE_EXP(_hw_info.m_uiRcInit)

void PrlSrvManipulationsTest::testGetResultAsHandleForHostHwInfoCmd()
{
	RECEIVE_SERVER_HW_INFO
	PRL_HANDLE_TYPE _type;
	CHECK_RET_CODE_EXP(PrlHandle_GetType(hSrvConfig, &_type))
	QVERIFY(_type == PHT_SERVER_CONFIG);
	PRL_UINT32 nHostRamSize = 0;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetHostRamSize(hSrvConfig, &nHostRamSize))
	QCOMPARE(nHostRamSize, _hw_info.getMemorySettings()->getHostRamSize());
}

#define SRV_RECEIVE_USER_PROFILE\
	LOGIN_TO_SERVER\
	m_JobHandle.reset(PrlSrv_GetUserProfile(m_ServerHandle));\
	CHECK_JOB_RET_CODE(m_JobHandle)\
	QVERIFY(PRL_SUCCEEDED(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr())));\
	SdkHandleWrap hUserProfile;\
	QVERIFY(PRL_SUCCEEDED(PrlResult_GetParam(m_ResultHandle, hUserProfile.GetHandlePtr())));\
	PRL_HANDLE_TYPE _type = PHT_ERROR;\
	QVERIFY(PRL_SUCCEEDED(PrlHandle_GetType(hUserProfile, &_type)));\
	QVERIFY(_type == PHT_USER_PROFILE);

#define EXTRACT_USER_PROFILE_AS_XML_MODEL_OBJECT\
	PRL_VOID_PTR pStrBuf = 0;\
	QVERIFY(PRL_SUCCEEDED(PrlResult_GetParamToken(m_ResultHandle, 0, &pStrBuf)));\
	CDispUser _du(UTF8_2QSTR((const char *)pStrBuf));\
	PrlBuffer_Free(pStrBuf);\
	CHECK_RET_CODE_EXP(_du.m_uiRcInit)

void PrlSrvManipulationsTest::testUserProfileGetDefaultVmFolder()
{
	SRV_RECEIVE_USER_PROFILE
	PRL_CHAR sDefaultVmFolder[STR_BUF_LENGTH];
	PRL_UINT32 nDefaultVmFolderLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlUsrCfg_GetDefaultVmFolder(hUserProfile, sDefaultVmFolder, &nDefaultVmFolderLength))
	EXTRACT_USER_PROFILE_AS_XML_MODEL_OBJECT
	QCOMPARE(UTF8_2QSTR(sDefaultVmFolder), _du.getUserWorkspace()->getDefaultVmFolder());
}

void PrlSrvManipulationsTest::testUserProfileGetDefaultVmFolderNotEnoughBufSize()
{
	SRV_RECEIVE_USER_PROFILE

	QVERIFY(PrlUsrCfg_SetDefaultVmFolder(hUserProfile, "SOME VALUE with size more than 1" ) == PRL_ERR_SUCCESS);

	PRL_CHAR sDefaultVmFolder[STR_BUF_LENGTH];
	PRL_UINT32 nDefaultVmFolderLength = 1;
	QVERIFY(PrlUsrCfg_GetDefaultVmFolder(hUserProfile, sDefaultVmFolder, &nDefaultVmFolderLength) == PRL_ERR_BUFFER_OVERRUN);
}

void PrlSrvManipulationsTest::testUserProfileGetDefaultVmFolderNullBufSize()
{
	SRV_RECEIVE_USER_PROFILE
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hUserProfile, PrlUsrCfg_GetDefaultVmFolder)
}

void PrlSrvManipulationsTest::testUserProfileSetDefaultVmFolder()
{
	SRV_RECEIVE_USER_PROFILE
	QString sNewDefaultVmFolderValue = "new default VM dir";
	CHECK_RET_CODE_EXP(PrlUsrCfg_SetDefaultVmFolder(hUserProfile, sNewDefaultVmFolderValue.toUtf8().data()))
	PRL_CHAR sDefaultVmFolder[STR_BUF_LENGTH];
	PRL_UINT32 nDefaultVmFolderLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlUsrCfg_GetDefaultVmFolder(hUserProfile, sDefaultVmFolder, &nDefaultVmFolderLength))
	QCOMPARE(UTF8_2QSTR(sDefaultVmFolder), sNewDefaultVmFolderValue);
}

void PrlSrvManipulationsTest::testUserProfileCanUseMngConsole()
{
	SRV_RECEIVE_USER_PROFILE
	PRL_BOOL bCanUseMngConsole;
	CHECK_RET_CODE_EXP(PrlUsrCfg_CanUseMngConsole(hUserProfile, &bCanUseMngConsole))
	EXTRACT_USER_PROFILE_AS_XML_MODEL_OBJECT
	QCOMPARE(PRL_UINT32(_du.getUserAccess()->canUseServerManagementConsole()), PRL_UINT32(bCanUseMngConsole));
}

void PrlSrvManipulationsTest::testUserProfileCanChangeSrvSets()
{
	SRV_RECEIVE_USER_PROFILE
	PRL_BOOL bCanChangeSrvSets;
	CHECK_RET_CODE_EXP(PrlUsrCfg_CanChangeSrvSets(hUserProfile, &bCanChangeSrvSets))
	EXTRACT_USER_PROFILE_AS_XML_MODEL_OBJECT
	QCOMPARE(PRL_UINT32(_du.getUserAccess()->canChangeServerSettings()), PRL_UINT32(bCanChangeSrvSets));
}

void PrlSrvManipulationsTest::testUserProfileIsLocalAdministrator()
{
	SRV_RECEIVE_USER_PROFILE
	PRL_BOOL bIsLocalAdministrator;
	CHECK_RET_CODE_EXP(PrlUsrCfg_IsLocalAdministrator(hUserProfile, &bIsLocalAdministrator))
	EXTRACT_USER_PROFILE_AS_XML_MODEL_OBJECT
	QVERIFY(PRL_BOOL(_du.getUserAccess()->isLocalAdministrator()) == bIsLocalAdministrator);
}

void PrlSrvManipulationsTest::testUserProfileIsLocalAdministratorOnWrongParams()
{
	SRV_RECEIVE_USER_PROFILE
	PRL_BOOL bIsLocalAdministrator;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlUsrCfg_IsLocalAdministrator(hUserProfile, 0), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlUsrCfg_IsLocalAdministrator(m_ServerHandle, &bIsLocalAdministrator), PRL_ERR_INVALID_ARG)
}

#define RECEIVE_DISP_CONFIG\
	LOGIN_TO_SERVER\
	m_JobHandle.reset(PrlSrv_GetCommonPrefs(m_ServerHandle));\
	CHECK_JOB_RET_CODE(m_JobHandle)\
	QVERIFY(PRL_SUCCEEDED(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr())));\
	SdkHandleWrap hDispConfig;\
	QVERIFY(PRL_SUCCEEDED(PrlResult_GetParam(m_ResultHandle, hDispConfig.GetHandlePtr())));\
	PRL_HANDLE_TYPE _type = PHT_ERROR;\
	QVERIFY(PRL_SUCCEEDED(PrlHandle_GetType(hDispConfig, &_type)));\
	QVERIFY(_type == PHT_DISP_CONFIG);

#define EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT\
	PRL_VOID_PTR pStrBuf = 0;\
	CHECK_RET_CODE_EXP(PrlHandle_ToString(hDispConfig, &pStrBuf));\
	CDispCommonPreferences _dcp;\
	_dcp.fromString(UTF8_2QSTR((const char *)pStrBuf));\
	PrlBuffer_Free(pStrBuf);\
	CHECK_RET_CODE_EXP(_dcp.m_uiRcInit)

void PrlSrvManipulationsTest::testDispConfigGetDefaultVmDir()
{
	RECEIVE_DISP_CONFIG
	PRL_CHAR sDefaultVmDir[STR_BUF_LENGTH];
	PRL_UINT32 nDefaultVmDirLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetDefaultVmDir(hDispConfig, sDefaultVmDir, &nDefaultVmDirLength))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QCOMPARE(UTF8_2QSTR(sDefaultVmDir), _dcp.getWorkspacePreferences()->getDefaultVmDirectory());
}

void PrlSrvManipulationsTest::testDispConfigGetDefaultVmDirNotEnoughBufSize()
{
	RECEIVE_DISP_CONFIG
	PRL_CHAR sDefaultVmDir[STR_BUF_LENGTH];
	PRL_UINT32 nDefaultVmDirLength = 1;
	QVERIFY(PrlDispCfg_GetDefaultVmDir(hDispConfig, sDefaultVmDir, &nDefaultVmDirLength) == PRL_ERR_BUFFER_OVERRUN);
}

void PrlSrvManipulationsTest::testDispConfigGetDefaultVmDirNullBufSize()
{
	RECEIVE_DISP_CONFIG
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hDispConfig, PrlDispCfg_GetDefaultVmDir)
}

void PrlSrvManipulationsTest::testDispConfigGetDefaulCtDir()
{
#ifdef _LIN_
	if (CVzHelper::is_vz_running() != 1)
		QSKIP("Virtuozzo is not running", SkipAll);
	RECEIVE_DISP_CONFIG

	PRL_CHAR sDir[STR_BUF_LENGTH];
	PRL_UINT32 nDirLength = sizeof(sDir);

	QString sOrigDir = CVzHelper::getVzPrivateDir();

	CHECK_RET_CODE_EXP(PrlDispCfg_GetDefaultCtDir(hDispConfig, sDir, &nDirLength))

	QCOMPARE(sOrigDir, UTF8_2QSTR(sDir));
#else
	QSKIP("Not implemented", SkipAll);
#endif
}

void PrlSrvManipulationsTest::testDispConfigGetReservedMemLimit()
{
	RECEIVE_DISP_CONFIG
	PRL_UINT32 nMemSize;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetReservedMemLimit(hDispConfig, &nMemSize))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QCOMPARE(_dcp.getMemoryPreferences()->getReservedMemoryLimit(), nMemSize);
}

void PrlSrvManipulationsTest::testDispConfigSetReservedMemLimit()
{
	RECEIVE_DISP_CONFIG
	PRL_UINT32 nNewMemSize = 512;
	CHECK_RET_CODE_EXP(PrlDispCfg_SetReservedMemLimit(hDispConfig, nNewMemSize))
	PRL_UINT32 nMemSize;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetReservedMemLimit(hDispConfig, &nMemSize))
	QCOMPARE(nMemSize, nNewMemSize);
}

void PrlSrvManipulationsTest::testDispConfigGetMinVmMem()
{
	RECEIVE_DISP_CONFIG
	PRL_UINT32 nMemSize;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetMinVmMem(hDispConfig, &nMemSize))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QCOMPARE(_dcp.getMemoryPreferences()->getMinVmMemory(), nMemSize);
}

void PrlSrvManipulationsTest::testDispConfigSetMinVmMem()
{
	RECEIVE_DISP_CONFIG
	PRL_UINT32 nNewMemSize = 512;
	CHECK_RET_CODE_EXP(PrlDispCfg_SetMinVmMem(hDispConfig, nNewMemSize))
	PRL_UINT32 nMemSize;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetMinVmMem(hDispConfig, &nMemSize))
	QCOMPARE(nMemSize, nNewMemSize);
}

void PrlSrvManipulationsTest::testDispConfigGetMaxVmMem()
{
	RECEIVE_DISP_CONFIG
	PRL_UINT32 nMemSize;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetMaxVmMem(hDispConfig, &nMemSize))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QCOMPARE(_dcp.getMemoryPreferences()->getMaxVmMemory(), nMemSize);
}

void PrlSrvManipulationsTest::testDispConfigSetMaxVmMem()
{
	RECEIVE_DISP_CONFIG
	PRL_UINT32 nNewMemSize = 512;
	CHECK_RET_CODE_EXP(PrlDispCfg_SetMaxVmMem(hDispConfig, nNewMemSize))
	PRL_UINT32 nMemSize;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetMaxVmMem(hDispConfig, &nMemSize))
	QCOMPARE(nMemSize, nNewMemSize);
}

void PrlSrvManipulationsTest::testDispConfigGetRecommendMinVmMem()
{
	QVector< QVector<PRL_UINT32 > > vvOsVersions;
	QVector<PRL_UINT32 > vOsVersions;

	//16 Mb OSes set
	vvOsVersions += vOsVersions;
	vOsVersions.clear();

	//32 Mb OSes set
	vvOsVersions += vOsVersions;
	vOsVersions.clear();

	//64 Mb OSes set
	vOsVersions
	<< PVS_GUEST_VER_WIN_311
	<< PVS_GUEST_VER_WIN_OTHER
	<< PVS_GUEST_VER_DOS_MS622
	<< PVS_GUEST_VER_DOS_OTHER
	<< PVS_GUEST_VER_OS2_WARP3
	<< PVS_GUEST_VER_OS2_OTHER;
	vvOsVersions += vOsVersions;
	vOsVersions.clear();

	//128 Mb OSes set
	vOsVersions
	<< PVS_GUEST_VER_WIN_95
	<< PVS_GUEST_VER_WIN_98
	<< PVS_GUEST_VER_WIN_ME
	<< PVS_GUEST_VER_WIN_NT
	<< PVS_GUEST_VER_WIN_2K
	<< PVS_GUEST_VER_LIN_REDHAT
	<< PVS_GUEST_VER_LIN_SUSE
	<< PVS_GUEST_VER_LIN_MANDRAKE
	<< PVS_GUEST_VER_LIN_KRNL_24
	<< PVS_GUEST_VER_LIN_KRNL_26
	<< PVS_GUEST_VER_LIN_DEBIAN
	<< PVS_GUEST_VER_LIN_FEDORA
	<< PVS_GUEST_VER_LIN_FEDORA_5
	<< PVS_GUEST_VER_LIN_XANDROS
	<< PVS_GUEST_VER_LIN_UBUNTU
	<< PVS_GUEST_VER_LIN_SLES9
	<< PVS_GUEST_VER_LIN_RHLES3
	<< PVS_GUEST_VER_LIN_CENTOS
	<< PVS_GUEST_VER_LIN_PSBM
	<< PVS_GUEST_VER_LIN_RH_LEGACY
	<< PVS_GUEST_VER_LIN_OPENSUSE
	<< PVS_GUEST_VER_LIN_MAGEIA
	<< PVS_GUEST_VER_LIN_MINT
	<< PVS_GUEST_VER_LIN_OTHER
	<< PVS_GUEST_VER_OS2_WARP4
	<< PVS_GUEST_VER_OS2_WARP45
	<< PVS_GUEST_VER_OS2_ECS11
	<< PVS_GUEST_VER_OS2_ECS12
	<< PVS_GUEST_VER_OTH_OPENSTEP
	<< PVS_GUEST_VER_OTH_OTHER;
	vvOsVersions += vOsVersions;
	vOsVersions.clear();

	//256 Mb OSes set
	vOsVersions
	<< PVS_GUEST_VER_NET_4X
	<< PVS_GUEST_VER_WIN_2008
	<< PVS_GUEST_VER_BSD_6X
	<< PVS_GUEST_VER_BSD_7X
	<< PVS_GUEST_VER_NET_OTHER
	<< PVS_GUEST_VER_WIN_XP
	<< PVS_GUEST_VER_WIN_2003
	<< PVS_GUEST_VER_BSD_4X
	<< PVS_GUEST_VER_BSD_5X
	<< PVS_GUEST_VER_BSD_OTHER
	<< PVS_GUEST_VER_OTH_QNX;
	vvOsVersions += vOsVersions;
	vOsVersions.clear();

	//512 Mb OSes set
	vOsVersions
	<< PVS_GUEST_VER_NET_5X
	<< PVS_GUEST_VER_NET_6X
	<< PVS_GUEST_VER_WIN_VISTA
	<< PVS_GUEST_VER_WIN_WINDOWS7
	<< PVS_GUEST_VER_WIN_WINDOWS8
	<< PVS_GUEST_VER_WIN_2012
	<< PVS_GUEST_VER_WIN_WINDOWS8_1
	<< PVS_GUEST_VER_MACOS_TIGER
	<< PVS_GUEST_VER_MACOS_LEOPARD
	<< PVS_GUEST_VER_MACOS_SNOW_LEOPARD
	<< PVS_GUEST_VER_SOL_10
	<< PVS_GUEST_VER_SOL_9
	<< PVS_GUEST_VER_SOL_OTHER
	<< PVS_GUEST_VER_SOL_11
	<< PVS_GUEST_VER_LIN_REDHAT_7
	<< PVS_GUEST_VER_LIN_CENTOS_7;
	vvOsVersions += vOsVersions;
	vOsVersions.clear();

	for(int i = 0; i < vvOsVersions.size(); i++)
	{
		vOsVersions = vvOsVersions[i];
		for(int j = 0; j < vOsVersions.size(); j++)
		{
			PRL_UINT32 nOsVersion = vOsVersions[j];
			PRL_UINT32 nMinMemSize = 0;
			PRL_UINT32 nExpectedMemSize = PRL_UINT32(1 << (i + 4));
			CHECK_RET_CODE_EXP(PrlApi_GetRecommendMinVmMem(nOsVersion, &nMinMemSize));
			if (nMinMemSize != nExpectedMemSize)
			{
				WRITE_TRACE(DBG_FATAL, "Unexpected min memory size for %.4X '%s'. Expected: %u actual: %u",\
					nOsVersion, PVS_GUEST_TO_STRING(nOsVersion), nExpectedMemSize, nMinMemSize);
				QFAIL("Wrong min memory size");
			}
		}
	}
}

void PrlSrvManipulationsTest::testDispConfigGetRecommendMinVmMemInvalid()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlApi_GetRecommendMinVmMem(PVS_GUEST_VER_WIN_NT, 0)
										, PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testDispConfigGetRecommendMaxVmMem()
{
	RECEIVE_DISP_CONFIG
	PRL_UINT32 nMemSize;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetRecommendMaxVmMem(hDispConfig, &nMemSize))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QCOMPARE(_dcp.getMemoryPreferences()->getRecommendedMaxVmMemory(), nMemSize);
}

void PrlSrvManipulationsTest::testDispConfigSetRecommendMaxVmMem()
{
	RECEIVE_DISP_CONFIG
	PRL_UINT32 nNewMemSize = 512;
	CHECK_RET_CODE_EXP(PrlDispCfg_SetRecommendMaxVmMem(hDispConfig, nNewMemSize))
	PRL_UINT32 nMemSize;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetRecommendMaxVmMem(hDispConfig, &nMemSize))
	QCOMPARE(nMemSize, nNewMemSize);
}

void PrlSrvManipulationsTest::testDispConfigGetMaxReservMemLimit()
{
	RECEIVE_DISP_CONFIG
	PRL_UINT32 nMemSize;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetMaxReservMemLimit(hDispConfig, &nMemSize))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QCOMPARE(_dcp.getMemoryPreferences()->getMaxReservedMemoryLimit(), nMemSize);
}

void PrlSrvManipulationsTest::testDispConfigSetMaxReservMemLimit()
{
	RECEIVE_DISP_CONFIG
	PRL_UINT32 nNewMemSize = 512;
	CHECK_RET_CODE_EXP(PrlDispCfg_SetMaxReservMemLimit(hDispConfig, nNewMemSize))
	PRL_UINT32 nMemSize;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetMaxReservMemLimit(hDispConfig, &nMemSize))
	QCOMPARE(nMemSize, nNewMemSize);
}

void PrlSrvManipulationsTest::testDispConfigGetMinReservMemLimit()
{
	RECEIVE_DISP_CONFIG
	PRL_UINT32 nMemSize;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetMinReservMemLimit(hDispConfig, &nMemSize))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QCOMPARE(_dcp.getMemoryPreferences()->getMinReservedMemoryLimit(), nMemSize);
}

void PrlSrvManipulationsTest::testDispConfigSetMinReservMemLimit()
{
	RECEIVE_DISP_CONFIG
	PRL_UINT32 nNewMemSize = 512;
	CHECK_RET_CODE_EXP(PrlDispCfg_SetMinReservMemLimit(hDispConfig, nNewMemSize))
	PRL_UINT32 nMemSize;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetMinReservMemLimit(hDispConfig, &nMemSize))
	QCOMPARE(nMemSize, nNewMemSize);
}

void PrlSrvManipulationsTest::testDispConfigIsAdjustMemAuto()
{
	RECEIVE_DISP_CONFIG
	PRL_BOOL bAdjustMemAuto;
	CHECK_RET_CODE_EXP(PrlDispCfg_IsAdjustMemAuto(hDispConfig, &bAdjustMemAuto))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QCOMPARE(PRL_UINT32(_dcp.getMemoryPreferences()->isAdjustMemAuto()), PRL_UINT32(bAdjustMemAuto));
}

void PrlSrvManipulationsTest::testDispConfigSetAdjustMemAuto()
{
	RECEIVE_DISP_CONFIG
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	PRL_BOOL bNewAdjustMemAuto = !_dcp.getMemoryPreferences()->isAdjustMemAuto();
	CHECK_RET_CODE_EXP(PrlDispCfg_SetAdjustMemAuto(hDispConfig, bNewAdjustMemAuto))
	PRL_BOOL bAdjustMemAuto;
	CHECK_RET_CODE_EXP(PrlDispCfg_IsAdjustMemAuto(hDispConfig, &bAdjustMemAuto))
	QCOMPARE(bAdjustMemAuto, bNewAdjustMemAuto);
}

void PrlSrvManipulationsTest::testDispConfigIsSendStatisticReport()
{
	RECEIVE_DISP_CONFIG
	PRL_BOOL bSendStatisticReport = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlDispCfg_IsSendStatisticReport(hDispConfig, &bSendStatisticReport))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QVERIFY(_dcp.getWorkspacePreferences()->isEnableSendStatisticReport() == bool(bSendStatisticReport));
}

void PrlSrvManipulationsTest::testDispConfigIsSendStatisticReportOnWrongParams()
{
	RECEIVE_DISP_CONFIG
	PRL_BOOL bSendStatisticReport = PRL_FALSE;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_IsSendStatisticReport(PRL_INVALID_HANDLE, &bSendStatisticReport), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_IsSendStatisticReport(m_ServerHandle, &bSendStatisticReport), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_IsSendStatisticReport(hDispConfig, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testDispConfigSetSendStatisticReport()
{
	RECEIVE_DISP_CONFIG
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	PRL_BOOL bNewSendStatisticReport = !_dcp.getWorkspacePreferences()->isEnableSendStatisticReport();
	CHECK_RET_CODE_EXP(PrlDispCfg_SetSendStatisticReport(hDispConfig, bNewSendStatisticReport))
	PRL_BOOL bSendStatisticReport = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlDispCfg_IsSendStatisticReport(hDispConfig, &bSendStatisticReport))
	QVERIFY(bSendStatisticReport == bNewSendStatisticReport);
}

void PrlSrvManipulationsTest::testDispConfigSetSendStatisticReportOnWrongParams()
{
	RECEIVE_DISP_CONFIG
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_SetSendStatisticReport(PRL_INVALID_HANDLE, PRL_FALSE), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_IsSendStatisticReport(m_ServerHandle, PRL_FALSE), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testDispConfigGetConfirmationsList()
{
	RECEIVE_DISP_CONFIG;
	SdkHandleWrap hConfirmList;

	CHECK_RET_CODE_EXP(PrlDispCfg_GetConfirmationsList(hDispConfig, hConfirmList.GetHandlePtr()));

	PRL_SIZE nType;
	CHECK_RET_CODE_EXP(PrlOpTypeList_GetTypeSize(hConfirmList, &nType));
	QCOMPARE( nType, (PRL_SIZE)sizeof(PRL_ALLOWED_VM_COMMAND) );

	PRL_UINT32 nSize;
	CHECK_RET_CODE_EXP(PrlOpTypeList_GetItemsCount(hConfirmList, &nSize));

	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT;
	QList<PRL_ALLOWED_VM_COMMAND> xmlConfirmList = _dcp.getLockedOperationsList()->getLockedOperations();
	QCOMPARE( (PRL_UINT32)xmlConfirmList.size(), nSize );

	for( PRL_UINT32 i=0; i< nSize; i++ )
	{
		PRL_ALLOWED_VM_COMMAND xmlVal, getVal;
		CHECK_RET_CODE_EXP(PrlOpTypeList_GetItem(hConfirmList, i, &getVal));
		xmlVal = xmlConfirmList.at( i );
		QCOMPARE( xmlVal, getVal );
	}
}

void PrlSrvManipulationsTest::testDispConfigSetConfirmationsList()
{
	QSKIP("Not implemented yet", SkipAll);
	//TODO Need implement it like testDispConfigSetMinReservMemLimit()
}

void PrlSrvManipulationsTest::testGetUsbIdentityCount()
{
	RECEIVE_DISP_CONFIG;
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT;
	PRL_UINT32 nUsbCount = 0;

	CHECK_RET_CODE_EXP(PrlDispCfg_GetUsbIdentityCount(hDispConfig, &nUsbCount));
	QList<CDispUsbIdentity*> xmlUsbIdentities = _dcp.getUsbPreferences()->m_lstAuthenticUsbMap;
	QCOMPARE(nUsbCount, PRL_UINT32(xmlUsbIdentities.size()));
}

#define USB_IDENT_FAKE_ID		"0-0.0|dead|0000|full|--|Empty"
#define USB_IDENT_FAKE_NAME "Really Friendly USB Device"
#define USB_IDENT_FAKE_UUID	"{0000-000-0000}"

void PrlSrvManipulationsTest::testDispConfigUsbIdentities()
{
	RECEIVE_DISP_CONFIG;
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT;
	PRL_UINT32 nUsbCount = 0;

	CHECK_RET_CODE_EXP(PrlDispCfg_GetUsbIdentityCount(hDispConfig, &nUsbCount));

	if(!nUsbCount)
	{
		// Create fake USB Identity if no ones known to Dispatcher
		CDispUsbIdentity * pUi = new CDispUsbIdentity();
		pUi->setSystemName(USB_IDENT_FAKE_ID);
		pUi->setFriendlyName(USB_IDENT_FAKE_NAME);
		_dcp.getUsbPreferences()->m_lstAuthenticUsbMap.append( pUi );
		CHECK_RET_CODE_EXP(PrlDispCfg_FromString( hDispConfig, _dcp.toString().toUtf8().data() ));
		CHECK_RET_CODE_EXP(PrlDispCfg_GetUsbIdentityCount(hDispConfig, &nUsbCount));
	}

	QList<CDispUsbIdentity*> xmlUsbIdentities = _dcp.getUsbPreferences()->m_lstAuthenticUsbMap;

	for (PRL_UINT32 i = 0; i < nUsbCount; i++)
	{
		SdkHandleWrap hUsbIdentity;
		CHECK_RET_CODE_EXP(PrlDispCfg_GetUsbIdentity(hDispConfig, i, hUsbIdentity.GetHandlePtr()));
		CHECK_HANDLE_TYPE(hUsbIdentity, PHT_USB_IDENTITY);

		CDispUsbIdentity * pUi = xmlUsbIdentities[i];

		QString sSystemName, sFriendlyName, sVmUuid;
		PRL_EXTRACT_STRING_VALUE( sSystemName, hUsbIdentity, PrlUsbIdent_GetSystemName );
		PRL_EXTRACT_STRING_VALUE(sFriendlyName, hUsbIdentity, PrlUsbIdent_GetFriendlyName);
		PRL_EXTRACT_STRING_VALUE(sVmUuid, hUsbIdentity, PrlUsbIdent_GetVmUuidAssociation);

		QCOMPARE( sSystemName, pUi->getSystemName() );
		QCOMPARE( sFriendlyName, pUi->getFriendlyName()
			+ (pUi->getIndex() > 1 ? QString(" #%1").arg(pUi->getIndex()) : QString()) );

		if( pUi->m_lstAssociations.isEmpty() || pUi->m_lstAssociations[0]->getAction() != PUD_CONNECT_TO_GUEST_OS )
		{
			QVERIFY(sVmUuid.isEmpty());
		} else
			QCOMPARE( sVmUuid, pUi->m_lstAssociations[0]->getVmUuid() );
	}
}

void PrlSrvManipulationsTest::testSetUsbIdentAssociation()
{
	RECEIVE_DISP_CONFIG;
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT;
	PRL_UINT32 nUsbCount = 0;

	{
		// Create fake USB Identity if no ones known to Dispatcher
		CDispUsbIdentity * pUi = new CDispUsbIdentity();
		pUi->setSystemName(USB_IDENT_FAKE_ID);
		pUi->setFriendlyName(USB_IDENT_FAKE_NAME);
		_dcp.getUsbPreferences()->m_lstAuthenticUsbMap.append( pUi );
		CHECK_RET_CODE_EXP(PrlDispCfg_FromString( hDispConfig, _dcp.toString().toUtf8().data() ));
	}

	CHECK_RET_CODE_EXP(PrlDispCfg_SetUsbIdentAssociation( hDispConfig, USB_IDENT_FAKE_ID, USB_IDENT_FAKE_UUID, 0 ));
	CHECK_RET_CODE_EXP(PrlDispCfg_GetUsbIdentityCount(hDispConfig, &nUsbCount));

	PRL_UINT32 i;
	for (i = 0; i < nUsbCount; i++)
	{
		SdkHandleWrap hUsbIdentity;
		CHECK_RET_CODE_EXP(PrlDispCfg_GetUsbIdentity(hDispConfig, i, hUsbIdentity.GetHandlePtr()));

		QString sSystemName;
		PRL_EXTRACT_STRING_VALUE( sSystemName, hUsbIdentity, PrlUsbIdent_GetSystemName );

		if( sSystemName == USB_IDENT_FAKE_ID )
		{
			QString sVmUuid;
			PRL_EXTRACT_STRING_VALUE( sVmUuid, hUsbIdentity, PrlUsbIdent_GetVmUuidAssociation );
			QVERIFY( sVmUuid == USB_IDENT_FAKE_UUID );
			break;
		}
	}
	QVERIFY( i < nUsbCount );
	CHECK_RET_CODE_EXP(PrlDispCfg_SetUsbIdentAssociation( hDispConfig, USB_IDENT_FAKE_ID, NULL, 0 ));
	CHECK_RET_CODE_EXP(PrlDispCfg_GetUsbIdentityCount(hDispConfig, &nUsbCount));

	for (i = 0; i < nUsbCount; i++)
	{
		SdkHandleWrap hUsbIdentity;
		CHECK_RET_CODE_EXP(PrlDispCfg_GetUsbIdentity(hDispConfig, i, hUsbIdentity.GetHandlePtr()));

		QString sSystemName;
		PRL_EXTRACT_STRING_VALUE( sSystemName, hUsbIdentity, PrlUsbIdent_GetSystemName );

		if( sSystemName == USB_IDENT_FAKE_ID )
		{
			QString sVmUuid;
			PRL_EXTRACT_STRING_VALUE( sVmUuid, hUsbIdentity, PrlUsbIdent_GetVmUuidAssociation );
			QVERIFY( sVmUuid.isEmpty() );
			break;
		}
	}
	QVERIFY( i < nUsbCount );
}


void PrlSrvManipulationsTest::testSetUsbIdentAssociationOnWrongParam()
{
	RECEIVE_DISP_CONFIG;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
				PrlDispCfg_SetUsbIdentAssociation(hDispConfig, USB_IDENT_FAKE_ID, NULL, 0),
				PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testGetUsbIdentityOnWrongIndex()
{
	RECEIVE_DISP_CONFIG;
	PRL_UINT32 nUsbCount = 0;

	CHECK_RET_CODE_EXP(PrlDispCfg_GetUsbIdentityCount(hDispConfig, &nUsbCount));
	SdkHandleWrap hUsbIdentity;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(
				PrlDispCfg_GetUsbIdentity(hDispConfig, nUsbCount, hUsbIdentity.GetHandlePtr()),
				PRL_ERR_INVALID_ARG);

	if(!nUsbCount)
	{
		EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT;
		// Create fake USB Identity if no ones known to Dispatcher
		CDispUsbIdentity * pUi = new CDispUsbIdentity();
		pUi->setSystemName(USB_IDENT_FAKE_ID);
		pUi->setFriendlyName(USB_IDENT_FAKE_NAME);
		_dcp.getUsbPreferences()->m_lstAuthenticUsbMap.append( pUi );
		CHECK_RET_CODE_EXP(PrlDispCfg_FromString( hDispConfig, _dcp.toString().toUtf8().data() ));
	}

	CHECK_CONCRETE_EXPRESSION_RET_CODE(
				PrlDispCfg_GetUsbIdentity(hDispConfig, 0, 0),
				PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testUsbIdentWrongHandles()
{
	PRL_CHAR sBuffer[STR_BUF_LENGTH];
	PRL_UINT32 nBufferLength = STR_BUF_LENGTH;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
				PrlUsbIdent_GetSystemName(PRL_INVALID_HANDLE, sBuffer, &nBufferLength),
				PRL_ERR_INVALID_ARG );
	nBufferLength = STR_BUF_LENGTH;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
				PrlUsbIdent_GetFriendlyName(PRL_INVALID_HANDLE, sBuffer, &nBufferLength),
				PRL_ERR_INVALID_ARG );
	nBufferLength = STR_BUF_LENGTH;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(
				PrlUsbIdent_GetVmUuidAssociation(PRL_INVALID_HANDLE, sBuffer, &nBufferLength),
				PRL_ERR_INVALID_ARG );
}

void PrlSrvManipulationsTest::testIsConnectedConnectionAbsent()
{
	PRL_BOOL bConnected = true;
	PRL_BOOL bIsConnectionLocal = false;
	CHECK_RET_CODE_EXP(PrlSrv_IsConnected(m_ServerHandle, &bConnected, &bIsConnectionLocal))
	QVERIFY(!bConnected);
}

void PrlSrvManipulationsTest::testIsConnectedConnectionPresent()
{
	LOGIN_TO_SERVER
	PRL_BOOL bConnected = false;
	PRL_BOOL bIsConnectionLocal = false;
	CHECK_RET_CODE_EXP(PrlSrv_IsConnected(m_ServerHandle, &bConnected, &bIsConnectionLocal))
	QVERIFY(bConnected);
}

void PrlSrvManipulationsTest::testCreateDispNet()
{
	RECEIVE_DISP_CONFIG
	SdkHandleWrap hDispNet;
	CHECK_RET_CODE_EXP(PrlDispCfg_CreateDispNet(hDispConfig, hDispNet.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hDispNet, PHT_DISP_NET_ADAPTER)
}

void PrlSrvManipulationsTest::testGetDispNetCount()
{
	RECEIVE_DISP_CONFIG
	SdkHandleWrap hDispNet;
	CHECK_RET_CODE_EXP(PrlDispCfg_CreateDispNet(hDispConfig, hDispNet.GetHandlePtr()))
	PRL_UINT32 nDispNetCount = 0;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetDispNetCount(hDispConfig, &nDispNetCount))
	QVERIFY(nDispNetCount != 0);
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QCOMPARE(nDispNetCount, PRL_UINT32(_dcp.getNetworkPreferences()->getNetAdapters()->size()));
}

void PrlSrvManipulationsTest::testGetDispNet()
{
	RECEIVE_DISP_CONFIG
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	CDispNetAdapter *pDispNet = new CDispNetAdapter;
	_dcp.getNetworkPreferences()->addNetAdapter(pDispNet);
	CHECK_RET_CODE_EXP(PrlDispCfg_FromString(hDispConfig, _dcp.toString().toUtf8().data()))
	SdkHandleWrap hDispNet;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetDispNet(hDispConfig, 0, hDispNet.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hDispNet, PHT_DISP_NET_ADAPTER)
}

void PrlSrvManipulationsTest::testGetDispNetNonValidIndex()
{
	RECEIVE_DISP_CONFIG
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	PRL_UINT32 nDispNetIndex = _dcp.getNetworkPreferences()->getNetAdapters()->size();
	SdkHandleWrap hDispNet;
	QVERIFY(!PRL_SUCCEEDED(PrlDispCfg_GetDispNet(hDispConfig, nDispNetIndex, hDispNet.GetHandlePtr())));
}

#define CHECK_TRY_TO_USE_ALREADY_REMOVED_DISP_NET\
	QVERIFY(!PRL_SUCCEEDED(PrlDispNet_Remove(hDispNet)));\
	PRL_BOOL bEnabled;\
	QVERIFY(!PRL_SUCCEEDED(PrlDispNet_IsEnabled(hDispNet, &bEnabled)));\
	QVERIFY(!PRL_SUCCEEDED(PrlDispNet_SetEnabled(hDispNet, bEnabled)));\
	PRL_BOOL bDhcpEnabled;\
	QVERIFY(!PRL_SUCCEEDED(PrlDispNet_IsDhcpEnabled(hDispNet, &bDhcpEnabled)));\
	QVERIFY(!PRL_SUCCEEDED(PrlDispNet_SetDhcpEnabled(hDispNet, bDhcpEnabled)));\
	PRL_NET_ADAPTER_EMULATED_TYPE nNetworkType = PNA_BRIDGED_ETHERNET;\
	QVERIFY(!PRL_SUCCEEDED(PrlDispNet_GetNetworkType(hDispNet, &nNetworkType)));\
	QVERIFY(!PRL_SUCCEEDED(PrlDispNet_SetNetworkType(hDispNet, nNetworkType)));\
	PRL_CHAR sDispNetName[STR_BUF_LENGTH];\
	PRL_UINT32 nDispNetNameLength = STR_BUF_LENGTH;\
	QVERIFY(!PRL_SUCCEEDED(PrlDispNet_GetName(hDispNet, sDispNetName, &nDispNetNameLength)));\
	QVERIFY(!PRL_SUCCEEDED(PrlDispNet_SetName(hDispNet, "some adapter name")));\
	PRL_CHAR sDispNetUuid[STR_BUF_LENGTH];\
	PRL_UINT32 nDispNetUuidLength = STR_BUF_LENGTH;\
	QVERIFY(!PRL_SUCCEEDED(PrlDispNet_GetUuid(hDispNet, sDispNetUuid, &nDispNetUuidLength)));\
	PRL_CHAR sDispNetSysName[STR_BUF_LENGTH];\
	PRL_UINT32 nDispNetSysNameLength = STR_BUF_LENGTH;\
	QVERIFY(!PRL_SUCCEEDED(PrlDispNet_GetSysName(hDispNet, sDispNetSysName, &nDispNetSysNameLength)));\
	QVERIFY(!PRL_SUCCEEDED(PrlDispNet_SetSysName(hDispNet, "some adapter name")));\
	PRL_UINT32 nIndex;\
	QVERIFY(!PRL_SUCCEEDED(PrlDispNet_GetIndex(hDispNet, &nIndex)));\
	QVERIFY(!PRL_SUCCEEDED(PrlDispNet_SetIndex(hDispNet, nIndex)));\
	PRL_CHAR sDispNetDhcpScopeStartIp[STR_BUF_LENGTH];\
	PRL_UINT32 nDispNetDhcpScopeStartIpLength = STR_BUF_LENGTH;\
	QVERIFY(!PRL_SUCCEEDED(PrlDispNet_GetDhcpScopeStartIp(hDispNet, sDispNetDhcpScopeStartIp, &nDispNetDhcpScopeStartIpLength)));\
	QVERIFY(!PRL_SUCCEEDED(PrlDispNet_SetDhcpScopeStartIp(hDispNet, "192.168.0.1")));\
	PRL_CHAR sDispNetDhcpScopeEndIp[STR_BUF_LENGTH];\
	PRL_UINT32 nDispNetDhcpScopeEndIpLength = STR_BUF_LENGTH;\
	QVERIFY(!PRL_SUCCEEDED(PrlDispNet_GetDhcpScopeEndIp(hDispNet, sDispNetDhcpScopeEndIp, &nDispNetDhcpScopeEndIpLength)));\
	QVERIFY(!PRL_SUCCEEDED(PrlDispNet_SetDhcpScopeEndIp(hDispNet, "192.168.0.100")));\
	PRL_CHAR sDispNetDhcpScopeMask[STR_BUF_LENGTH];\
	PRL_UINT32 nDispNetDhcpScopeMaskLength = STR_BUF_LENGTH;\
	QVERIFY(!PRL_SUCCEEDED(PrlDispNet_GetDhcpScopeMask(hDispNet, sDispNetDhcpScopeMask, &nDispNetDhcpScopeMaskLength)));\
	QVERIFY(!PRL_SUCCEEDED(PrlDispNet_SetDhcpScopeMask(hDispNet, "255.255.255.0")));\

void PrlSrvManipulationsTest::testDispNetRemove()
{
	RECEIVE_DISP_CONFIG
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	CDispNetAdapter *pDispNet = new CDispNetAdapter;
	_dcp.getNetworkPreferences()->addNetAdapter(pDispNet);
	CHECK_RET_CODE_EXP(PrlDispCfg_FromString(hDispConfig, _dcp.toString().toUtf8().data()))
	SdkHandleWrap hDispNet;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetDispNet(hDispConfig, 0, hDispNet.GetHandlePtr()))
	PRL_UINT32 nDispNetCount1 = 0;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetDispNetCount(hDispConfig, &nDispNetCount1))
	CHECK_RET_CODE_EXP(PrlDispNet_Remove(hDispNet))
	PRL_UINT32 nDispNetCount2 = 0;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetDispNetCount(hDispConfig, &nDispNetCount2))
	QVERIFY((nDispNetCount1 - nDispNetCount2) == 1);
	CHECK_TRY_TO_USE_ALREADY_REMOVED_DISP_NET
}

#define ADD_DISP_NET\
	SdkHandleWrap hDispNet;\
	QVERIFY(PRL_SUCCEEDED(PrlDispCfg_CreateDispNet(hDispConfig, hDispNet.GetHandlePtr())));\
	QVERIFY(PRL_SUCCEEDED(PrlDispNet_SetName(hDispNet, "some adapter name")));\
	QVERIFY(PRL_SUCCEEDED(PrlDispNet_SetSysName(hDispNet, "some adapter name")));\
	QVERIFY(PRL_SUCCEEDED(PrlDispNet_SetDhcpScopeStartIp(hDispNet, "192.168.0.1")));\
	QVERIFY(PRL_SUCCEEDED(PrlDispNet_SetDhcpScopeEndIp(hDispNet, "192.168.0.100")));\
	QVERIFY(PRL_SUCCEEDED(PrlDispNet_SetDhcpScopeMask(hDispNet, "255.255.255.0")));\
	QVERIFY(PRL_SUCCEEDED(PrlDispNet_SetDhcp6ScopeStartIp(hDispNet, "fec0:0:0:fea9:0:0:0:1")));\
	QVERIFY(PRL_SUCCEEDED(PrlDispNet_SetDhcp6ScopeEndIp(hDispNet, "fec0:0:0:fea9:0:0:0:2")));\
	QVERIFY(PRL_SUCCEEDED(PrlDispNet_SetDhcp6ScopeMask(hDispNet, "ffff:ffff:ffff:ffff:0:0:0:0")));\

void PrlSrvManipulationsTest::testDispNetIsEnabled()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_BOOL bEnabled = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlDispNet_IsEnabled(hDispNet, &bEnabled))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QVERIFY(bEnabled == (PRL_BOOL)_dcp.getNetworkPreferences()->getNetAdapters()->back()->isEnabled());
}

void PrlSrvManipulationsTest::testDispNetSetEnabled()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	PRL_BOOL bNewEnabled = !_dcp.getNetworkPreferences()->getNetAdapters()->back()->isEnabled();
	CHECK_RET_CODE_EXP(PrlDispNet_SetEnabled(hDispNet, bNewEnabled))
	PRL_BOOL bActualEnabled = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlDispNet_IsEnabled(hDispNet, &bActualEnabled))
	QVERIFY(bNewEnabled == bActualEnabled);
}

void PrlSrvManipulationsTest::testDispNetIsHidden()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_BOOL bHidden = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlDispNet_IsHidden(hDispNet, &bHidden))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QVERIFY(bHidden == (PRL_BOOL)_dcp.getNetworkPreferences()->getNetAdapters()->back()->isHiddenAdapter());
}

void PrlSrvManipulationsTest::testDispNetIsHiddenOnWrongParams()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_BOOL bHidden = PRL_FALSE;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispNet_IsHidden(PRL_INVALID_HANDLE, &bHidden), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispNet_IsHidden(m_ServerHandle, &bHidden), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispNet_IsHidden(hDispNet, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testDispNetSetHidden()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	PRL_BOOL bNewHidden = !_dcp.getNetworkPreferences()->getNetAdapters()->back()->isHiddenAdapter();
	CHECK_RET_CODE_EXP(PrlDispNet_SetHidden(hDispNet, bNewHidden))
	PRL_BOOL bActualHidden = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlDispNet_IsHidden(hDispNet, &bActualHidden))
	QVERIFY(bNewHidden == bActualHidden);
}

void PrlSrvManipulationsTest::testDispNetSetHiddenOnWrongParams()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispNet_SetHidden(PRL_INVALID_HANDLE, PRL_FALSE), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispNet_SetHidden(m_ServerHandle, PRL_FALSE), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testDispNetGetNetworkType()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_NET_ADAPTER_EMULATED_TYPE nEmulatedType = PNA_BRIDGED_ETHERNET;
	CHECK_RET_CODE_EXP(PrlDispNet_GetNetworkType(hDispNet, &nEmulatedType))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QVERIFY(nEmulatedType == _dcp.getNetworkPreferences()->getNetAdapters()->back()->getNetworkType());
}

void PrlSrvManipulationsTest::testDispNetSetNetworkType()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	PRL_NET_ADAPTER_EMULATED_TYPE nEmulatedType = _dcp.getNetworkPreferences()->getNetAdapters()->back()->getNetworkType() == PNA_BRIDGED_ETHERNET ? PNA_HOST_ONLY : PNA_BRIDGED_ETHERNET;
	CHECK_RET_CODE_EXP(PrlDispNet_SetNetworkType(hDispNet, nEmulatedType))
	PRL_NET_ADAPTER_EMULATED_TYPE nActualEmulatedType;
	CHECK_RET_CODE_EXP(PrlDispNet_GetNetworkType(hDispNet, &nActualEmulatedType))
	QVERIFY(nEmulatedType == nActualEmulatedType);
}

void PrlSrvManipulationsTest::testDispNetGetName()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_CHAR sDispNetName[STR_BUF_LENGTH];
	PRL_UINT32 nDispNetNameLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlDispNet_GetName(hDispNet, sDispNetName, &nDispNetNameLength))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QCOMPARE(UTF8_2QSTR(sDispNetName), _dcp.getNetworkPreferences()->getNetAdapters()->back()->getName());
	QVERIFY(nDispNetNameLength == size_t(_dcp.getNetworkPreferences()->getNetAdapters()->back()->getName().toUtf8().size()+1));
}

void PrlSrvManipulationsTest::testDispNetGetNameNotEnoughBufSize()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_CHAR sDispNetName[STR_BUF_LENGTH];
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	PRL_UINT32 nDispNetNameLength = _dcp.getNetworkPreferences()->getNetAdapters()->back()->getName().size();
	PRL_RESULT nRetCode = PrlDispNet_GetName(hDispNet, sDispNetName, &nDispNetNameLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlSrvManipulationsTest::testDispNetGetNameNullBufSize()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hDispNet, PrlDispNet_GetName)
}

void PrlSrvManipulationsTest::testDispNetSetName()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	QString sNewDispNetName = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP(PrlDispNet_SetName(hDispNet, sNewDispNetName.toUtf8().data()))
	PRL_CHAR sDispNetName[STR_BUF_LENGTH];
	PRL_UINT32 nDispNetNameLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlDispNet_GetName(hDispNet, sDispNetName, &nDispNetNameLength))
	QCOMPARE(UTF8_2QSTR(sDispNetName), sNewDispNetName);
}

void PrlSrvManipulationsTest::testDispNetGetUuid()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_CHAR sDispNetUuid[STR_BUF_LENGTH];
	PRL_UINT32 nDispNetUuidLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlDispNet_GetUuid(hDispNet, sDispNetUuid, &nDispNetUuidLength))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QCOMPARE(UTF8_2QSTR(sDispNetUuid), _dcp.getNetworkPreferences()->getNetAdapters()->back()->getUuid());
	QVERIFY(nDispNetUuidLength == size_t(_dcp.getNetworkPreferences()->getNetAdapters()->back()->getUuid().toUtf8().size()+1));
}

void PrlSrvManipulationsTest::testDispNetGetUuidNotEnoughBufSize()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_CHAR sDispNetUuid[STR_BUF_LENGTH];
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	PRL_UINT32 nDispNetUuidLength = _dcp.getNetworkPreferences()->getNetAdapters()->back()->getUuid().size();
	PRL_RESULT nRetCode = PrlDispNet_GetUuid(hDispNet, sDispNetUuid, &nDispNetUuidLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlSrvManipulationsTest::testDispNetGetUuidNullBufSize()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hDispNet, PrlDispNet_GetUuid)
}

void PrlSrvManipulationsTest::testDispNetGetSysName()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_CHAR sDispNetSysName[STR_BUF_LENGTH];
	PRL_UINT32 nDispNetSysNameLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlDispNet_GetSysName(hDispNet, sDispNetSysName, &nDispNetSysNameLength))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QCOMPARE(UTF8_2QSTR(sDispNetSysName), _dcp.getNetworkPreferences()->getNetAdapters()->back()->getSysName());
	QVERIFY(nDispNetSysNameLength == size_t(_dcp.getNetworkPreferences()->getNetAdapters()->back()->getSysName().toUtf8().size()+1));
}

void PrlSrvManipulationsTest::testDispNetGetSysNameNotEnoughBufSize()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_CHAR sDispNetSysName[STR_BUF_LENGTH];
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	PRL_UINT32 nDispNetSysNameLength = _dcp.getNetworkPreferences()->getNetAdapters()->back()->getSysName().size();
	PRL_RESULT nRetCode = PrlDispNet_GetSysName(hDispNet, sDispNetSysName, &nDispNetSysNameLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlSrvManipulationsTest::testDispNetGetSysNameNullBufSize()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hDispNet, PrlDispNet_GetSysName)
}

void PrlSrvManipulationsTest::testDispNetSetSysName()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	QString sNewDispNetSysName = Uuid::createUuid().toString();
	CHECK_RET_CODE_EXP(PrlDispNet_SetSysName(hDispNet, sNewDispNetSysName.toUtf8().data()))
	PRL_CHAR sDispNetSysName[STR_BUF_LENGTH];
	PRL_UINT32 nDispNetSysNameLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlDispNet_GetSysName(hDispNet, sDispNetSysName, &nDispNetSysNameLength))
	QCOMPARE(UTF8_2QSTR(sDispNetSysName), sNewDispNetSysName);
}

void PrlSrvManipulationsTest::testDispNetGetIndex()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_UINT32 nIndex;
	CHECK_RET_CODE_EXP(PrlDispNet_GetIndex(hDispNet, &nIndex))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QVERIFY(PRL_UINT32(_dcp.getNetworkPreferences()->getNetAdapters()->back()->getIndex()) == nIndex);
}

void PrlSrvManipulationsTest::testDispNetSetIndex()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_UINT32 nIndex;
	CHECK_RET_CODE_EXP(PrlDispNet_GetIndex(hDispNet, &nIndex))
	PRL_UINT32 nNewIndex = (nIndex == 0 ? 1 : 0);
	CHECK_RET_CODE_EXP(PrlDispNet_SetIndex(hDispNet, nNewIndex))
	CHECK_RET_CODE_EXP(PrlDispNet_GetIndex(hDispNet, &nIndex))
	QVERIFY(nNewIndex == nIndex);
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QVERIFY(PRL_UINT32(_dcp.getNetworkPreferences()->getNetAdapters()->back()->getIndex()) == nNewIndex);
}

void PrlSrvManipulationsTest::testDispNetIsDhcpEnabled()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_BOOL bDhcpEnabled = false;
	CHECK_RET_CODE_EXP(PrlDispNet_IsDhcpEnabled(hDispNet, &bDhcpEnabled))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QVERIFY(bDhcpEnabled == (PRL_BOOL)_dcp.getNetworkPreferences()->getNetAdapters()->back()->getDhcpPreferences()->isEnabled());
}

void PrlSrvManipulationsTest::testDispNetSetDhcpEnabled()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	PRL_BOOL bNewDhcpEnabled = !_dcp.getNetworkPreferences()->getNetAdapters()->back()->getDhcpPreferences()->isEnabled();
	CHECK_RET_CODE_EXP(PrlDispNet_SetDhcpEnabled(hDispNet, bNewDhcpEnabled))
	PRL_BOOL bActualDhcpEnabled;
	CHECK_RET_CODE_EXP(PrlDispNet_IsDhcpEnabled(hDispNet, &bActualDhcpEnabled))
	QVERIFY(bNewDhcpEnabled == bActualDhcpEnabled);
}

void PrlSrvManipulationsTest::testDispNetGetDhcpScopeStartIp()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_CHAR sDispNetDhcpScopeStartIp[STR_BUF_LENGTH];
	PRL_UINT32 nDispNetDhcpScopeStartIpLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlDispNet_GetDhcpScopeStartIp(hDispNet, sDispNetDhcpScopeStartIp, &nDispNetDhcpScopeStartIpLength))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QString sDhcpScopeStartIp = _dcp.getNetworkPreferences()->getNetAdapters()->back()->getDhcpPreferences()->getDhcpScopeStartIp().toString();
	QCOMPARE(UTF8_2QSTR(sDispNetDhcpScopeStartIp), sDhcpScopeStartIp);
	QVERIFY(nDispNetDhcpScopeStartIpLength == size_t(sDhcpScopeStartIp.toUtf8().size()+1));
}

void PrlSrvManipulationsTest::testDispNetGetDhcpScopeStartIpNotEnoughBufSize()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_CHAR sDispNetDhcpScopeStartIp[STR_BUF_LENGTH];
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	PRL_UINT32 nDispNetDhcpScopeStartIpLength = _dcp.getNetworkPreferences()->getNetAdapters()->back()->getDhcpPreferences()->getDhcpScopeStartIp().toString().size();
	PRL_RESULT nRetCode = PrlDispNet_GetDhcpScopeStartIp(hDispNet, sDispNetDhcpScopeStartIp, &nDispNetDhcpScopeStartIpLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlSrvManipulationsTest::testDispNetGetDhcpScopeStartIpNullBufSize()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hDispNet, PrlDispNet_GetDhcpScopeStartIp)
}

void PrlSrvManipulationsTest::testDispNetSetDhcpScopeStartIp()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	QString sNewDispNetDhcpScopeStartIp = "192.168.1.1";
	CHECK_RET_CODE_EXP(PrlDispNet_SetDhcpScopeStartIp(hDispNet, sNewDispNetDhcpScopeStartIp.toUtf8().data()))
	PRL_CHAR sDispNetDhcpScopeStartIp[STR_BUF_LENGTH];
	PRL_UINT32 nDispNetDhcpScopeStartIpLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlDispNet_GetDhcpScopeStartIp(hDispNet, sDispNetDhcpScopeStartIp, &nDispNetDhcpScopeStartIpLength))
	QCOMPARE(UTF8_2QSTR(sDispNetDhcpScopeStartIp), sNewDispNetDhcpScopeStartIp);
}

void PrlSrvManipulationsTest::testDispNetGetDhcpScopeEndIp()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_CHAR sDispNetDhcpScopeEndIp[STR_BUF_LENGTH];
	PRL_UINT32 nDispNetDhcpScopeEndIpLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlDispNet_GetDhcpScopeEndIp(hDispNet, sDispNetDhcpScopeEndIp, &nDispNetDhcpScopeEndIpLength))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QString sDhcpScopeEndIp = _dcp.getNetworkPreferences()->getNetAdapters()->back()->getDhcpPreferences()->getDhcpScopeEndIp().toString();
	QCOMPARE(UTF8_2QSTR(sDispNetDhcpScopeEndIp), sDhcpScopeEndIp);
	QVERIFY(nDispNetDhcpScopeEndIpLength == size_t(sDhcpScopeEndIp.toUtf8().size()+1));
}

void PrlSrvManipulationsTest::testDispNetGetDhcpScopeEndIpNotEnoughBufSize()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_CHAR sDispNetDhcpScopeEndIp[STR_BUF_LENGTH];
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	PRL_UINT32 nDispNetDhcpScopeEndIpLength = _dcp.getNetworkPreferences()->getNetAdapters()->back()->getDhcpPreferences()->getDhcpScopeEndIp().toString().size();
	PRL_RESULT nRetCode = PrlDispNet_GetDhcpScopeEndIp(hDispNet, sDispNetDhcpScopeEndIp, &nDispNetDhcpScopeEndIpLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlSrvManipulationsTest::testDispNetGetDhcpScopeEndIpNullBufSize()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hDispNet, PrlDispNet_GetDhcpScopeEndIp)
}

void PrlSrvManipulationsTest::testDispNetSetDhcpScopeEndIp()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	QString sNewDispNetDhcpScopeEndIp = "192.168.1.100";
	CHECK_RET_CODE_EXP(PrlDispNet_SetDhcpScopeEndIp(hDispNet, sNewDispNetDhcpScopeEndIp.toUtf8().data()))
	PRL_CHAR sDispNetDhcpScopeEndIp[STR_BUF_LENGTH];
	PRL_UINT32 nDispNetDhcpScopeEndIpLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlDispNet_GetDhcpScopeEndIp(hDispNet, sDispNetDhcpScopeEndIp, &nDispNetDhcpScopeEndIpLength))
	QCOMPARE(UTF8_2QSTR(sDispNetDhcpScopeEndIp), sNewDispNetDhcpScopeEndIp);
}

void PrlSrvManipulationsTest::testDispNetGetDhcpScopeMask()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_CHAR sDispNetDhcpScopeMask[STR_BUF_LENGTH];
	PRL_UINT32 nDispNetDhcpScopeMaskLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlDispNet_GetDhcpScopeMask(hDispNet, sDispNetDhcpScopeMask, &nDispNetDhcpScopeMaskLength))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QString sDhcpScopeMask = _dcp.getNetworkPreferences()->getNetAdapters()->back()->getDhcpPreferences()->getDhcpScopeMask().toString();
	QCOMPARE(UTF8_2QSTR(sDispNetDhcpScopeMask), sDhcpScopeMask);
	QVERIFY(nDispNetDhcpScopeMaskLength == size_t(sDhcpScopeMask.toUtf8().size()+1));
}

void PrlSrvManipulationsTest::testDispNetGetDhcpScopeMaskNotEnoughBufSize()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_CHAR sDispNetDhcpScopeMask[STR_BUF_LENGTH];
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	PRL_UINT32 nDispNetDhcpScopeMaskLength = _dcp.getNetworkPreferences()->getNetAdapters()->back()->getDhcpPreferences()->getDhcpScopeMask().toString().size();
	PRL_RESULT nRetCode = PrlDispNet_GetDhcpScopeMask(hDispNet, sDispNetDhcpScopeMask, &nDispNetDhcpScopeMaskLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlSrvManipulationsTest::testDispNetGetDhcpScopeMaskNullBufSize()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hDispNet, PrlDispNet_GetDhcpScopeMask)
}

void PrlSrvManipulationsTest::testDispNetSetDhcpScopeMask()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	QString sNewDispNetDhcpScopeMask = "255.255.255.0";
	CHECK_RET_CODE_EXP(PrlDispNet_SetDhcpScopeMask(hDispNet, sNewDispNetDhcpScopeMask.toUtf8().data()))
	PRL_CHAR sDispNetDhcpScopeMask[STR_BUF_LENGTH];
	PRL_UINT32 nDispNetDhcpScopeMaskLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlDispNet_GetDhcpScopeMask(hDispNet, sDispNetDhcpScopeMask, &nDispNetDhcpScopeMaskLength))
	QCOMPARE(UTF8_2QSTR(sDispNetDhcpScopeMask), sNewDispNetDhcpScopeMask);
}

/* == IPv6 == */

void PrlSrvManipulationsTest::testDispNetIsDhcp6Enabled()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_BOOL bDhcp6Enabled = false;
	CHECK_RET_CODE_EXP(PrlDispNet_IsDhcp6Enabled(hDispNet, &bDhcp6Enabled))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QVERIFY(bDhcp6Enabled == (PRL_BOOL)_dcp.getNetworkPreferences()->getNetAdapters()->back()->getDhcpV6Preferences()->isEnabled());
}

void PrlSrvManipulationsTest::testDispNetSetDhcp6Enabled()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	PRL_BOOL bNewDhcp6Enabled = !_dcp.getNetworkPreferences()->getNetAdapters()->back()->getDhcpV6Preferences()->isEnabled();
	CHECK_RET_CODE_EXP(PrlDispNet_SetDhcp6Enabled(hDispNet, bNewDhcp6Enabled))
	PRL_BOOL bActualDhcp6Enabled;
	CHECK_RET_CODE_EXP(PrlDispNet_IsDhcp6Enabled(hDispNet, &bActualDhcp6Enabled))
	QVERIFY(bNewDhcp6Enabled == bActualDhcp6Enabled);
}

void PrlSrvManipulationsTest::testDispNetGetDhcp6ScopeStartIp()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_CHAR sDispNetDhcp6ScopeStartIp[STR_BUF_LENGTH];
	PRL_UINT32 nDispNetDhcp6ScopeStartIpLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlDispNet_GetDhcp6ScopeStartIp(hDispNet, sDispNetDhcp6ScopeStartIp, &nDispNetDhcp6ScopeStartIpLength))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QString sDhcp6ScopeStartIp = _dcp.getNetworkPreferences()->getNetAdapters()->back()->getDhcpV6Preferences()->getDhcpScopeStartIp().toString();
	QCOMPARE(UTF8_2QSTR(sDispNetDhcp6ScopeStartIp), sDhcp6ScopeStartIp);
	QVERIFY(nDispNetDhcp6ScopeStartIpLength == size_t(sDhcp6ScopeStartIp.toUtf8().size()+1));
}

void PrlSrvManipulationsTest::testDispNetGetDhcp6ScopeStartIpNotEnoughBufSize()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_CHAR sDispNetDhcp6ScopeStartIp[STR_BUF_LENGTH];
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	PRL_UINT32 nDispNetDhcp6ScopeStartIpLength = _dcp.getNetworkPreferences()->getNetAdapters()->back()->getDhcpV6Preferences()->getDhcpScopeStartIp().toString().size();
	PRL_RESULT nRetCode = PrlDispNet_GetDhcp6ScopeStartIp(hDispNet, sDispNetDhcp6ScopeStartIp, &nDispNetDhcp6ScopeStartIpLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlSrvManipulationsTest::testDispNetGetDhcp6ScopeStartIpNullBufSize()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hDispNet, PrlDispNet_GetDhcp6ScopeStartIp)
}

void PrlSrvManipulationsTest::testDispNetSetDhcp6ScopeStartIp()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	QString sNewDispNetDhcp6ScopeStartIp = "192.168.1.1";
	CHECK_RET_CODE_EXP(PrlDispNet_SetDhcp6ScopeStartIp(hDispNet, sNewDispNetDhcp6ScopeStartIp.toUtf8().data()))
	PRL_CHAR sDispNetDhcp6ScopeStartIp[STR_BUF_LENGTH];
	PRL_UINT32 nDispNetDhcp6ScopeStartIpLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlDispNet_GetDhcp6ScopeStartIp(hDispNet, sDispNetDhcp6ScopeStartIp, &nDispNetDhcp6ScopeStartIpLength))
	QCOMPARE(UTF8_2QSTR(sDispNetDhcp6ScopeStartIp), sNewDispNetDhcp6ScopeStartIp);
}

void PrlSrvManipulationsTest::testDispNetGetDhcp6ScopeEndIp()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_CHAR sDispNetDhcp6ScopeEndIp[STR_BUF_LENGTH];
	PRL_UINT32 nDispNetDhcp6ScopeEndIpLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlDispNet_GetDhcp6ScopeEndIp(hDispNet, sDispNetDhcp6ScopeEndIp, &nDispNetDhcp6ScopeEndIpLength))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QString sDhcp6ScopeEndIp = _dcp.getNetworkPreferences()->getNetAdapters()->back()->getDhcpV6Preferences()->getDhcpScopeEndIp().toString();
	QCOMPARE(UTF8_2QSTR(sDispNetDhcp6ScopeEndIp), sDhcp6ScopeEndIp);
	QVERIFY(nDispNetDhcp6ScopeEndIpLength == size_t(sDhcp6ScopeEndIp.toUtf8().size()+1));
}

void PrlSrvManipulationsTest::testDispNetGetDhcp6ScopeEndIpNotEnoughBufSize()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_CHAR sDispNetDhcp6ScopeEndIp[STR_BUF_LENGTH];
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	PRL_UINT32 nDispNetDhcp6ScopeEndIpLength = _dcp.getNetworkPreferences()->getNetAdapters()->back()->getDhcpV6Preferences()->getDhcpScopeEndIp().toString().size();
	PRL_RESULT nRetCode = PrlDispNet_GetDhcp6ScopeEndIp(hDispNet, sDispNetDhcp6ScopeEndIp, &nDispNetDhcp6ScopeEndIpLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlSrvManipulationsTest::testDispNetGetDhcp6ScopeEndIpNullBufSize()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hDispNet, PrlDispNet_GetDhcp6ScopeEndIp)
}

void PrlSrvManipulationsTest::testDispNetSetDhcp6ScopeEndIp()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	QString sNewDispNetDhcp6ScopeEndIp = "192.168.1.100";
	CHECK_RET_CODE_EXP(PrlDispNet_SetDhcp6ScopeEndIp(hDispNet, sNewDispNetDhcp6ScopeEndIp.toUtf8().data()))
	PRL_CHAR sDispNetDhcp6ScopeEndIp[STR_BUF_LENGTH];
	PRL_UINT32 nDispNetDhcp6ScopeEndIpLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlDispNet_GetDhcp6ScopeEndIp(hDispNet, sDispNetDhcp6ScopeEndIp, &nDispNetDhcp6ScopeEndIpLength))
	QCOMPARE(UTF8_2QSTR(sDispNetDhcp6ScopeEndIp), sNewDispNetDhcp6ScopeEndIp);
}

void PrlSrvManipulationsTest::testDispNetGetDhcp6ScopeMask()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_CHAR sDispNetDhcp6ScopeMask[STR_BUF_LENGTH];
	PRL_UINT32 nDispNetDhcp6ScopeMaskLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlDispNet_GetDhcp6ScopeMask(hDispNet, sDispNetDhcp6ScopeMask, &nDispNetDhcp6ScopeMaskLength))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QString sDhcp6ScopeMask = _dcp.getNetworkPreferences()->getNetAdapters()->back()->getDhcpV6Preferences()->getDhcpScopeMask().toString();
	QCOMPARE(UTF8_2QSTR(sDispNetDhcp6ScopeMask), sDhcp6ScopeMask);
	QVERIFY(nDispNetDhcp6ScopeMaskLength == size_t(sDhcp6ScopeMask.toUtf8().size()+1));
}

void PrlSrvManipulationsTest::testDispNetGetDhcp6ScopeMaskNotEnoughBufSize()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_CHAR sDispNetDhcp6ScopeMask[STR_BUF_LENGTH];
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	PRL_UINT32 nDispNetDhcp6ScopeMaskLength = _dcp.getNetworkPreferences()->getNetAdapters()->back()->getDhcpV6Preferences()->getDhcpScopeMask().toString().size();
	PRL_RESULT nRetCode = PrlDispNet_GetDhcp6ScopeMask(hDispNet, sDispNetDhcp6ScopeMask, &nDispNetDhcp6ScopeMaskLength);
	QVERIFY(nRetCode == PRL_ERR_BUFFER_OVERRUN);
}

void PrlSrvManipulationsTest::testDispNetGetDhcp6ScopeMaskNullBufSize()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hDispNet, PrlDispNet_GetDhcp6ScopeMask)
}

void PrlSrvManipulationsTest::testDispNetSetDhcp6ScopeMask()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	QString sNewDispNetDhcp6ScopeMask = "255.255.255.0";
	CHECK_RET_CODE_EXP(PrlDispNet_SetDhcp6ScopeMask(hDispNet, sNewDispNetDhcp6ScopeMask.toUtf8().data()))
	PRL_CHAR sDispNetDhcp6ScopeMask[STR_BUF_LENGTH];
	PRL_UINT32 nDispNetDhcp6ScopeMaskLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlDispNet_GetDhcp6ScopeMask(hDispNet, sDispNetDhcp6ScopeMask, &nDispNetDhcp6ScopeMaskLength))
	QCOMPARE(UTF8_2QSTR(sDispNetDhcp6ScopeMask), sNewDispNetDhcp6ScopeMask);
}


void PrlSrvManipulationsTest::testSrvConfigGetCpuModel()
{
	RECEIVE_SERVER_HW_INFO
	PRL_CHAR sCpuModel[STR_BUF_LENGTH];
	PRL_UINT32 nCpuModelLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetCpuModel(hSrvConfig, sCpuModel, &nCpuModelLength))
	QCOMPARE(UTF8_2QSTR(sCpuModel), _hw_info.getCpu()->getModel());
}

void PrlSrvManipulationsTest::testSrvConfigGetCpuModelNotEnoughBufSize()
{
	RECEIVE_SERVER_HW_INFO
	if (!_hw_info.getCpu()->getModel().size())
	{
		QSKIP("Skipping test due no CPU model info", SkipAll);
	}
	PRL_CHAR sCpuModel[STR_BUF_LENGTH];
	PRL_UINT32 nCpuModelLength = 1;
	QVERIFY(PrlSrvCfg_GetCpuModel(hSrvConfig, sCpuModel, &nCpuModelLength) == PRL_ERR_BUFFER_OVERRUN);
}

void PrlSrvManipulationsTest::testSrvConfigGetCpuModelNullBufSize()
{
	RECEIVE_SERVER_HW_INFO
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hSrvConfig, PrlSrvCfg_GetCpuModel)
}

void PrlSrvManipulationsTest::testSrvConfigGetCpuCount()
{
	RECEIVE_SERVER_HW_INFO
	PRL_UINT32 nCpuCount = 0;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetCpuCount(hSrvConfig, &nCpuCount))
	QCOMPARE(nCpuCount, _hw_info.getCpu()->getNumber());
}

void PrlSrvManipulationsTest::testSrvConfigGetCpuSpeed()
{
	RECEIVE_SERVER_HW_INFO
	PRL_UINT32 nCpuSpeed = 0;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetCpuSpeed(hSrvConfig, &nCpuSpeed))
	QCOMPARE(nCpuSpeed, _hw_info.getCpu()->getSpeed());
}

void PrlSrvManipulationsTest::testSrvConfigGetCpuMode()
{
	RECEIVE_SERVER_HW_INFO
	PRL_CPU_MODE nCpuMode;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetCpuMode(hSrvConfig, &nCpuMode))
	QCOMPARE(quint32(nCpuMode), quint32(_hw_info.getCpu()->getMode()));
}

void PrlSrvManipulationsTest::testSrvConfigGetCpuHvt()
{
	RECEIVE_SERVER_HW_INFO
	PRL_CPU_HVT nCpuHvt;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetCpuHvt(hSrvConfig, &nCpuHvt))
	QCOMPARE(quint32(nCpuHvt), quint32(_hw_info.getCpu()->getVtxMode()));
}

void PrlSrvManipulationsTest::testSrvConfigGetCpuHvtOnNullPtr()
{
	RECEIVE_SERVER_HW_INFO
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfg_GetCpuHvt(hSrvConfig, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testSrvConfigGetCpuHvtOnNullSrvCfgHandle()
{
	PRL_CPU_HVT nCpuHvt;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfg_GetCpuHvt(PRL_INVALID_HANDLE, &nCpuHvt), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testSrvConfigGetCpuHvtOnNonSrvCfgHandle()
{
	PRL_CPU_HVT nCpuHvt;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfg_GetCpuHvt(m_ServerHandle, &nCpuHvt), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testSrvConfigGetHostOsType()
{
	RECEIVE_SERVER_HW_INFO
	PRL_HOST_OS_TYPE nHostOsType;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetHostOsType(hSrvConfig, &nHostOsType))
	QVERIFY(nHostOsType == _hw_info.getOsVersion()->getOsType());
}

void PrlSrvManipulationsTest::testSrvConfigGetHostOsMajor()
{
	RECEIVE_SERVER_HW_INFO
	PRL_UINT32 nHostOsMajor;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetHostOsMajor(hSrvConfig, &nHostOsMajor))
	QVERIFY(nHostOsMajor == _hw_info.getOsVersion()->getMajor());
}

void PrlSrvManipulationsTest::testSrvConfigGetHostOsMinor()
{
	RECEIVE_SERVER_HW_INFO
	PRL_UINT32 nHostOsMinor;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetHostOsMinor(hSrvConfig, &nHostOsMinor))
	QVERIFY(nHostOsMinor == _hw_info.getOsVersion()->getMinor());
}

void PrlSrvManipulationsTest::testSrvConfigGetHostOsSubMinor()
{
	RECEIVE_SERVER_HW_INFO
	PRL_UINT32 nHostOsSubMinor;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetHostOsSubMinor(hSrvConfig, &nHostOsSubMinor))
	QVERIFY(nHostOsSubMinor == _hw_info.getOsVersion()->getSubMinor());
}

void PrlSrvManipulationsTest::testSrvConfigGetHostOsStrPresentation()
{
	RECEIVE_SERVER_HW_INFO
	PRL_CHAR sHostOsStrPresentation[STR_BUF_LENGTH];
	PRL_UINT32 nHostOsStrPresentationLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetHostOsStrPresentation(hSrvConfig, sHostOsStrPresentation, &nHostOsStrPresentationLength))
	QCOMPARE(UTF8_2QSTR(sHostOsStrPresentation), _hw_info.getOsVersion()->getStringPresentation());
}

void PrlSrvManipulationsTest::testSrvConfigGetHostOsStrPresentationNotEnoughBufSize()
{
	RECEIVE_SERVER_HW_INFO
	if (!_hw_info.getOsVersion()->getStringPresentation().size())
	{
		QSKIP("Skipping test due no host OS info", SkipAll);
	}
	PRL_CHAR sHostOsStrPresentation[STR_BUF_LENGTH];
	PRL_UINT32 nHostOsStrPresentationLength = 1;
	QVERIFY(PrlSrvCfg_GetHostOsStrPresentation(hSrvConfig, sHostOsStrPresentation, &nHostOsStrPresentationLength) == PRL_ERR_BUFFER_OVERRUN);
}

void PrlSrvManipulationsTest::testSrvConfigGetHostOsStrPresentationNullBufSize()
{
	RECEIVE_SERVER_HW_INFO
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hSrvConfig, PrlSrvCfg_GetHostOsStrPresentation)
}

namespace {
bool CompareDevicesLists(const QList<SdkHandleWrap> &_actual_devs_list,
													const QList<CHwGenericDevice *> &_expected_devs_list)
{
	if (_actual_devs_list.size() == _expected_devs_list.size())
	{
		for(size_t i = 0; i < size_t(_actual_devs_list.size()); ++i)
		{
			SdkHandleWrap _dev_handle = _actual_devs_list.value(i);
			CHwGenericDevice *pDevice = _expected_devs_list.value(i);
			PRL_CHAR sDevName[STR_BUF_LENGTH];
			PRL_UINT32 nDevNameLength = STR_BUF_LENGTH;
			if (PRL_FAILED(PrlSrvCfgDev_GetName(_dev_handle, sDevName, &nDevNameLength)))
			{
				WRITE_TRACE(DBG_FATAL, "Couldn't to get device name");
				return (false);
			}
			if (pDevice->getDeviceName() != UTF8_2QSTR(sDevName))
			{
				WRITE_TRACE(DBG_FATAL, "Device name do not match. Expected: '%s' actual: '%s'",
										pDevice->getDeviceName().toUtf8().data(), sDevName);
				return (false);
			}
			PRL_CHAR sDevId[STR_BUF_LENGTH];
			PRL_UINT32 nDevIdLength = STR_BUF_LENGTH;
			if (PRL_FAILED(PrlSrvCfgDev_GetId(_dev_handle, sDevId, &nDevIdLength)))
			{
				WRITE_TRACE(DBG_FATAL, "Couldn't to get device id");
				return (false);
			}
			if (pDevice->getDeviceId() != UTF8_2QSTR(sDevId))
			{
				WRITE_TRACE(DBG_FATAL, "Device id do not match. Expected: '%s' actual: '%s'",
										pDevice->getDeviceId().toUtf8().data(), sDevId);
				return (false);
			}
			PRL_DEVICE_TYPE nDevType;
			if (PRL_FAILED(PrlSrvCfgDev_GetType(_dev_handle, &nDevType)))
			{
				WRITE_TRACE(DBG_FATAL, "Couldn't to get device type");
				return (false);
			}
			if (pDevice->getDeviceType() != nDevType)
			{
				WRITE_TRACE(DBG_FATAL, "Device type do not match. Expected: %.u actual: %.u",
										quint32(pDevice->getDeviceType()), quint32(nDevType));
				return (false);
			}
			PRL_BOOL bConnected = PRL_FALSE;
			if (PRL_FAILED(PrlSrvCfgDev_IsConnectedToVm(_dev_handle, &bConnected)))
			{
				WRITE_TRACE(DBG_FATAL, "Couldn't to test device connection");
				return (false);
			}
			if (PRL_BOOL(   !pDevice->getVmUuids().isEmpty()
						 && (pDevice->getDeviceState() == PGS_CONNECTED_TO_VM ||
							pDevice->getDeviceState() == PGS_CONNECTING_TO_VM))	!= bConnected)
			{
				WRITE_TRACE(DBG_FATAL, "Test device connection sign do not match. Expected: %.u actual: %.u",
							quint32(   !pDevice->getVmUuids().isEmpty()
									&& (pDevice->getDeviceState() == PGS_CONNECTED_TO_VM ||
									pDevice->getDeviceState() == PGS_CONNECTING_TO_VM)),
							quint32(bConnected));
				return (false);
			}
		}
		return (true);
	}
	else
		WRITE_TRACE(DBG_FATAL, "Devices lists size do not match. Expected: %d actual: %d", _expected_devs_list.size(),
								_actual_devs_list.size());

	return (false);
}

bool CompareUsbDevicesLists(const QList<SdkHandleWrap> &_actual_devs_list,
							const QList<CHwUsbDevice *> &_expected_devs_list)
{
	// copy usb list
	QList<CHwGenericDevice *> lstUsb;
	for(size_t i = 0; i < size_t(_actual_devs_list.size()); ++i)
		lstUsb << _expected_devs_list.value(i);

	return CompareDevicesLists( _actual_devs_list, lstUsb );
}
}

#define TEST_DEVICE_ENUMERATION(device_name)\
	RECEIVE_SERVER_HW_INFO\
	QList<SdkHandleWrap> _devs_list;\
	PRL_UINT32 nDevsNum = 0;\
	QVERIFY(PRL_SUCCEEDED(PrlSrvCfg_Get##device_name##sCount(hSrvConfig, &nDevsNum)));\
	for (PRL_UINT32 i = 0; i < nDevsNum; ++i)\
	{\
		PRL_HANDLE _dev_handle;\
		QVERIFY(PRL_SUCCEEDED(PrlSrvCfg_Get##device_name(hSrvConfig, i, &_dev_handle)));\
		_devs_list.append(SdkHandleWrap(_dev_handle));\
	}

#define COMPLETE_TEST_DEVICE_ENUMERATION(device_name)\
	TEST_DEVICE_ENUMERATION(device_name)\
	QVERIFY(CompareDevicesLists(_devs_list,\
				*((QList<CHwGenericDevice* >* )&_hw_info.m_lst##device_name##s)));

void PrlSrvManipulationsTest::testSrvConfigEnumerateFloppyDisks()
{
	COMPLETE_TEST_DEVICE_ENUMERATION(FloppyDisk)
}

void PrlSrvManipulationsTest::testSrvConfigEnumerateOpticalDisks()
{
	COMPLETE_TEST_DEVICE_ENUMERATION(OpticalDisk)
}

void PrlSrvManipulationsTest::testSrvConfigEnumerateSerialPorts()
{
	COMPLETE_TEST_DEVICE_ENUMERATION(SerialPort)
}

void PrlSrvManipulationsTest::testSrvConfigEnumerateParallelPorts()
{
	COMPLETE_TEST_DEVICE_ENUMERATION(ParallelPort)
}

void PrlSrvManipulationsTest::testSrvConfigEnumerateSoundOutputDevs()
{
	TEST_DEVICE_ENUMERATION(SoundOutputDev)
	QVERIFY(CompareDevicesLists(_devs_list, _hw_info.m_lstSoundOutputDevices));
}

void PrlSrvManipulationsTest::testSrvConfigEnumerateSoundMixerDevs()
{
	TEST_DEVICE_ENUMERATION(SoundMixerDev)
	QVERIFY(CompareDevicesLists(_devs_list, _hw_info.m_lstSoundMixerDevices));
}

void PrlSrvManipulationsTest::testSrvConfigEnumeratePrinters()
{
	COMPLETE_TEST_DEVICE_ENUMERATION(Printer)
}

void PrlSrvManipulationsTest::testSrvConfigEnumerateUsbDevs()
{
	TEST_DEVICE_ENUMERATION(UsbDev)
	QVERIFY(CompareUsbDevicesLists(_devs_list, _hw_info.m_lstUsbDevices));
}

void PrlSrvManipulationsTest::testSrvConfigIsSoundDefaultEnabled()
{
	RECEIVE_SERVER_HW_INFO
	PRL_BOOL bSoundDefaultEnabled;
	CHECK_RET_CODE_EXP(PrlSrvCfg_IsSoundDefaultEnabled(hSrvConfig, &bSoundDefaultEnabled))
	QVERIFY(bSoundDefaultEnabled == PRL_BOOL(_hw_info.getSoundDefaultEnabled()));
}

void PrlSrvManipulationsTest::testSrvConfigIsUsbSupported()
{
	RECEIVE_SERVER_HW_INFO
	PRL_BOOL bUsbSupported;
	CHECK_RET_CODE_EXP(PrlSrvCfg_IsUsbSupported(hSrvConfig, &bUsbSupported))
	QVERIFY(bUsbSupported == PRL_BOOL(_hw_info.getUsbSupported()));
}

void PrlSrvManipulationsTest::testSrvConfigIsVtdSupported()
{
	RECEIVE_SERVER_HW_INFO
	PRL_BOOL bVtdSupported;
	CHECK_RET_CODE_EXP(PrlSrvCfg_IsVtdSupported(hSrvConfig, &bVtdSupported))
	QVERIFY(bVtdSupported == PRL_BOOL(_hw_info.isVtdSupported()));
}

void PrlSrvManipulationsTest::testSrvConfigGetMaxHostNetAdapters()
{
	RECEIVE_SERVER_HW_INFO
	PRL_UINT32 nMaxHostNetAdapters = 0;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetMaxHostNetAdapters(hSrvConfig, &nMaxHostNetAdapters))
	QCOMPARE(quint32(nMaxHostNetAdapters), quint32(PrlNet::getMaximumAdapterIndex()+1));
}

void PrlSrvManipulationsTest::testSrvConfigGetMaxHostNetAdaptersOnWrongParams()
{
	RECEIVE_SERVER_HW_INFO
	PRL_UINT32 nMaxHostNetAdapters = 0;
	//Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfg_GetMaxHostNetAdapters(PRL_INVALID_HANDLE, &nMaxHostNetAdapters), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfg_GetMaxHostNetAdapters(m_ServerHandle, &nMaxHostNetAdapters), PRL_ERR_INVALID_ARG)
	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfg_GetMaxHostNetAdapters(hSrvConfig, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testSrvConfigGetMaxVmNetAdapters()
{
	RECEIVE_SERVER_HW_INFO
	PRL_UINT32 nMaxVmNetAdapters = 0;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetMaxVmNetAdapters(hSrvConfig, &nMaxVmNetAdapters))
	QCOMPARE(quint32(nMaxVmNetAdapters), quint32(MAX_NET_DEVICES));
}

void PrlSrvManipulationsTest::testSrvConfigGetMaxVmNetAdaptersOnWrongParams()
{
	RECEIVE_SERVER_HW_INFO
	PRL_UINT32 nMaxVmNetAdapters = 0;
	//Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfg_GetMaxVmNetAdapters(PRL_INVALID_HANDLE, &nMaxVmNetAdapters), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfg_GetMaxVmNetAdapters(m_ServerHandle, &nMaxVmNetAdapters), PRL_ERR_INVALID_ARG)
	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfg_GetMaxVmNetAdapters(hSrvConfig, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testSrvConfigEnumerateHardDisks()
{
	RECEIVE_SERVER_HW_INFO
	PRL_UINT32 nHardDisksNum = 0;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetHardDisksCount(hSrvConfig, &nHardDisksNum))
	QCOMPARE(nHardDisksNum, PRL_UINT32(_hw_info.m_lstHardDisks.size()));
	for (PRL_UINT32 i = 0; i < nHardDisksNum; ++i)
	{
		SdkHandleWrap hHardDisk;
		CHECK_RET_CODE_EXP(PrlSrvCfg_GetHardDisk(hSrvConfig, i, hHardDisk.GetHandlePtr()))
		CHECK_HANDLE_TYPE(hHardDisk, PHT_HW_HARD_DISK)

		CHwHardDisk *pHardDisk = _hw_info.m_lstHardDisks.value(i);

		PRL_CHAR sDevName[STR_BUF_LENGTH];
		PRL_UINT32 nDevNameBufLength = STR_BUF_LENGTH;
		CHECK_RET_CODE_EXP(PrlSrvCfgHdd_GetDevName(hHardDisk, sDevName, &nDevNameBufLength))
		QVERIFY(pHardDisk->getDeviceName() == UTF8_2QSTR(sDevName));

		PRL_CHAR sDevId[STR_BUF_LENGTH];
		PRL_UINT32 nDevIdBufLength = STR_BUF_LENGTH;
		CHECK_RET_CODE_EXP(PrlSrvCfgHdd_GetDevId(hHardDisk, sDevId, &nDevIdBufLength))
		QVERIFY(pHardDisk->getDeviceId() == UTF8_2QSTR(sDevId));

		PRL_UINT64 nDevSize = 0;
		CHECK_RET_CODE_EXP(PrlSrvCfgHdd_GetDevSize(hHardDisk, &nDevSize))
		QCOMPARE(nDevSize, PRL_UINT64(pHardDisk->getDeviceSize()));

		PRL_UINT32 nDiskIndex = 0;
		CHECK_RET_CODE_EXP(PrlSrvCfgHdd_GetDiskIndex(hHardDisk, &nDiskIndex))
		QCOMPARE(nDiskIndex, PRL_UINT32(pHardDisk->getDiskIndex()));

		QList<CHwHddPartition* > lstPartitions = pHardDisk->getAllPartitions(pHardDisk->m_lstPartitions);

		PRL_UINT32 nPartsCount = 0;
		CHECK_RET_CODE_EXP(PrlSrvCfgHdd_GetPartsCount(hHardDisk, &nPartsCount))
		QCOMPARE(nPartsCount, PRL_UINT32(lstPartitions.size()));

		for (PRL_UINT32 i = 0; i < nPartsCount; ++i)
		{
			SdkHandleWrap hPartition;
			CHECK_RET_CODE_EXP(PrlSrvCfgHdd_GetPart(hHardDisk, i, hPartition.GetHandlePtr()))
			CHECK_HANDLE_TYPE(hPartition, PHT_HW_HARD_DISK_PARTITION)

			CHwHddPartition *pPartition = lstPartitions.value(i);

			PRL_CHAR sPartName[STR_BUF_LENGTH];
			PRL_UINT32 nPartNameBufLength = STR_BUF_LENGTH;
			CHECK_RET_CODE_EXP(PrlSrvCfgHddPart_GetName(hPartition, sPartName, &nPartNameBufLength))
			QVERIFY(pPartition->getName() == UTF8_2QSTR(sPartName));

			PRL_CHAR sPartSysName[STR_BUF_LENGTH];
			PRL_UINT32 nPartSysNameBufLength = STR_BUF_LENGTH;
			CHECK_RET_CODE_EXP(PrlSrvCfgHddPart_GetSysName(hPartition, sPartSysName, &nPartSysNameBufLength))
			QVERIFY(pPartition->getSystemName() == UTF8_2QSTR(sPartSysName));

			PRL_UINT64 nPartSize = 0;
			CHECK_RET_CODE_EXP(PrlSrvCfgHddPart_GetSize(hPartition, &nPartSize))
			QCOMPARE(nPartSize, PRL_UINT64(pPartition->getSize()));

			PRL_UINT32 nPartIndex = 0;
			CHECK_RET_CODE_EXP(PrlSrvCfgHddPart_GetIndex(hPartition, &nPartIndex))
			QCOMPARE(nPartIndex, PRL_UINT32(pPartition->getIndex()));

			PRL_UINT32 nPartType = 0;
			CHECK_RET_CODE_EXP(PrlSrvCfgHddPart_GetType(hPartition, &nPartType))
			QCOMPARE(nPartType, PRL_UINT32(pPartition->getType()));

			PRL_BOOL bPartInUse;
			CHECK_RET_CODE_EXP(PrlSrvCfgHddPart_IsInUse(hPartition, &bPartInUse))
			QCOMPARE(bPartInUse, PRL_BOOL(pPartition->getInUse()));

			PRL_BOOL bPartLogical;
			CHECK_RET_CODE_EXP(PrlSrvCfgHddPart_IsLogical(hPartition, &bPartLogical))
			QCOMPARE(bPartLogical, PRL_BOOL(pPartition->getIsLogical()));

			PRL_BOOL bPartActive;
			CHECK_RET_CODE_EXP(PrlSrvCfgHddPart_IsActive(hPartition, &bPartActive))
			QCOMPARE(bPartActive, PRL_BOOL(pPartition->getIsActive()));
		}
	}
}

void PrlSrvManipulationsTest::testSrvConfigEnumerateNetAdapters()
{
	RECEIVE_SERVER_HW_INFO
	PRL_UINT32 nNetAdaptersNum = 0;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetNetAdaptersCount(hSrvConfig, &nNetAdaptersNum))
	QCOMPARE(nNetAdaptersNum, PRL_UINT32(_hw_info.m_lstNetworkAdapters.size()));
	for (PRL_UINT32 i = 0; i < nNetAdaptersNum; ++i)
	{
		SdkHandleWrap hNetAdapter;
		CHECK_RET_CODE_EXP(PrlSrvCfg_GetNetAdapter(hSrvConfig, i, hNetAdapter.GetHandlePtr()))
		CHECK_HANDLE_TYPE(hNetAdapter, PHT_HW_NET_ADAPTER)

		CHwNetAdapter *pNetAdapter = _hw_info.m_lstNetworkAdapters.value(i);

		PRL_CHAR sDevName[STR_BUF_LENGTH];
		PRL_UINT32 nDevNameBufLength = STR_BUF_LENGTH;
		CHECK_RET_CODE_EXP(PrlSrvCfgDev_GetName(hNetAdapter, sDevName, &nDevNameBufLength))
		QVERIFY(pNetAdapter->getDeviceName() == UTF8_2QSTR(sDevName));

		PRL_CHAR sDevId[STR_BUF_LENGTH];
		PRL_UINT32 nDevIdBufLength = STR_BUF_LENGTH;
		CHECK_RET_CODE_EXP(PrlSrvCfgDev_GetId(hNetAdapter, sDevId, &nDevIdBufLength))
		QVERIFY(!pNetAdapter->getDeviceId().isEmpty() && !UTF8_2QSTR(sDevId).isEmpty());
		QVERIFY(pNetAdapter->getDeviceId() == UTF8_2QSTR(sDevId));

		PRL_DEVICE_TYPE nDevType;
		CHECK_RET_CODE_EXP(PrlSrvCfgDev_GetType(hNetAdapter, &nDevType))
		QVERIFY(nDevType == pNetAdapter->getDeviceType());

		PRL_HW_INFO_NET_ADAPTER_TYPE nNetAdapterType;
		CHECK_RET_CODE_EXP(PrlSrvCfgNet_GetNetAdapterType(hNetAdapter, &nNetAdapterType))
		QVERIFY(nNetAdapterType == pNetAdapter->getNetAdapterType());

		PRL_UINT32 nSysIndex;
		CHECK_RET_CODE_EXP(PrlSrvCfgNet_GetSysIndex(hNetAdapter, &nSysIndex))
		QVERIFY(nSysIndex == (PRL_UINT32)pNetAdapter->getSysIndex());

		PRL_BOOL bEnabled;
		CHECK_RET_CODE_EXP(PrlSrvCfgNet_IsEnabled(hNetAdapter, &bEnabled))
		QVERIFY(bool(bEnabled) == pNetAdapter->getEnabled());
	}
}

void PrlSrvManipulationsTest::testSrvConfigEnumerateGenericPciDevices()
{
	COMPLETE_TEST_DEVICE_ENUMERATION(GenericPciDevice)
}

void PrlSrvManipulationsTest::testSrvConfigEnumerateGenericScsiDevices()
{
	COMPLETE_TEST_DEVICE_ENUMERATION(GenericScsiDevice)
}

void PrlSrvManipulationsTest::testDispNetHandleNonMadeInvalidOnFromStringCall()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	CHECK_RET_CODE_EXP(PrlDispCfg_FromString(hDispConfig, _dcp.toString().toUtf8().data()))
	PRL_BOOL bEnabled;
	CHECK_RET_CODE_EXP(PrlDispNet_IsEnabled(hDispNet, &bEnabled))
}

void PrlSrvManipulationsTest::testDispNetHandleMadeInvalidOnRemove()
{
	RECEIVE_DISP_CONFIG
	ADD_DISP_NET
	PRL_UINT32 nDispNetsCount = 0;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetDispNetCount(hDispConfig, &nDispNetsCount))
	QVERIFY(nDispNetsCount != 0);
	SdkHandleWrap hDispNet2;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetDispNet(hDispConfig, nDispNetsCount-1, hDispNet2.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlDispNet_Remove(hDispNet))
	PRL_BOOL bEnabled;
	QVERIFY(PRL_FAILED(PrlDispNet_IsEnabled(hDispNet, &bEnabled)));
	QVERIFY(PRL_FAILED(PrlDispNet_IsEnabled(hDispNet2, &bEnabled)));
}

void PrlSrvManipulationsTest::testUsrCfgToStringFromString()
{
	SRV_RECEIVE_USER_PROFILE
	CDispUser _user_cfg;
	_user_cfg.getUserWorkspace()->setDefaultVmFolder( Uuid::createUuid().toString() );
	QString sExpectedUserProfile = _user_cfg.toString();
	CHECK_RET_CODE_EXP(PrlUsrCfg_FromString(hUserProfile, sExpectedUserProfile.toUtf8().data()))
	PRL_VOID_PTR pUserProfile = 0;
	CHECK_RET_CODE_EXP(PrlUsrCfg_ToString(hUserProfile, &pUserProfile))
	QString sActualUserProfile = UTF8_2QSTR((const char *)pUserProfile);
	PrlBuffer_Free(pUserProfile);
	QCOMPARE(sActualUserProfile.remove(" xmlns=\"\""), sExpectedUserProfile.remove(" xmlns=\"\""));
}

#define GET_FS_REQUEST_RESULT\
	CHECK_JOB_RET_CODE(hJob)\
	SdkHandleWrap hResult;\
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))\
	SdkHandleWrap hFsInfo;\
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hFsInfo.GetHandlePtr()))

#define CHECK_VALID_RESPONSE_ON_FS_REQUEST\
	GET_FS_REQUEST_RESULT\
	CHECK_HANDLE_TYPE(hFsInfo, PHT_REMOTE_FILESYSTEM_INFO)

void PrlSrvManipulationsTest::testGetResultAsHandleForFsGetDiskList()
{
	testLoginLocal();
	SdkHandleWrap hJob(PrlSrv_FsGetDiskList(m_ServerHandle));
	CHECK_VALID_RESPONSE_ON_FS_REQUEST
}

void PrlSrvManipulationsTest::testGetResultAsHandleForFsGetDirEntries()
{
	testLoginLocal();
	SdkHandleWrap hJob(PrlSrv_FsGetDirEntries(m_ServerHandle, QDir::tempPath().toUtf8().constData()));
	CHECK_VALID_RESPONSE_ON_FS_REQUEST
}

void PrlSrvManipulationsTest::testGetResultAsHandleForFsCreateDir()
{
	testLoginLocal();
	SdkHandleWrap hJob(PrlSrv_FsCreateDir(m_ServerHandle, m_sTestFsDirName1.toUtf8().constData()));
	CHECK_VALID_RESPONSE_ON_FS_REQUEST
}

void PrlSrvManipulationsTest::testGetResultAsHandleForFsRenameEntry()
{
	testLoginLocal();
	QVERIFY(QDir().mkdir(m_sTestFsDirName1));
	SdkHandleWrap hJob(PrlSrv_FsRenameEntry(m_ServerHandle, m_sTestFsDirName1.toUtf8().constData(),
											m_sTestFsDirName2.toUtf8().constData()));
	CHECK_VALID_RESPONSE_ON_FS_REQUEST
}

void PrlSrvManipulationsTest::testGetResultAsHandleForFsRemoveEntry()
{
	testLoginLocal();
	QVERIFY(QDir().mkdir(m_sTestFsDirName1));
	SdkHandleWrap hJob(PrlSrv_FsRemoveEntry(m_ServerHandle, m_sTestFsDirName1.toUtf8().constData()));
	CHECK_VALID_RESPONSE_ON_FS_REQUEST
}

#define GET_FS_INFO_XML_OBJECT\
	PRL_VOID_PTR pBuffer = 0;\
	CHECK_RET_CODE_EXP(PrlResult_GetParamToken(hResult, 0, &pBuffer))\
	CHwFileSystemInfo _fs_info;\
	_fs_info.fromString(UTF8_2QSTR((const char *)pBuffer));\
	PrlBuffer_Free(pBuffer);\
	CHECK_RET_CODE_EXP(_fs_info.m_uiRcInit)

void PrlSrvManipulationsTest::testRemoteFsInfoGetFsType()
{
	testLoginLocal();
	SdkHandleWrap hJob(PrlSrv_FsGetDiskList(m_ServerHandle));
	GET_FS_REQUEST_RESULT
	PRL_FILE_SYSTEM_TYPE _fs_type;
	CHECK_RET_CODE_EXP(PrlFsInfo_GetType(hFsInfo, &_fs_type))
	GET_FS_INFO_XML_OBJECT
	QVERIFY(_fs_type == PRL_FILE_SYSTEM_TYPE(_fs_info.getFileSystemGenre()));
}

void PrlSrvManipulationsTest::testRemoteFsInfoGetFsExtType()
{
	testLoginLocal();

	QString currDir = QDir::currentPath();

	SdkHandleWrap hJob(PrlSrv_FsGetDirEntries(m_ServerHandle, QSTR2UTF8( currDir) ));
	GET_FS_REQUEST_RESULT
	PRL_FILE_SYSTEM_FS_TYPE _fs_type;
	CHECK_RET_CODE_EXP(PrlFsInfo_GetFsType(hFsInfo, &_fs_type))
	GET_FS_INFO_XML_OBJECT
	QVERIFY(_fs_type == _fs_info.getFileSystemType() );

	PRL_FILE_SYSTEM_FS_TYPE _fs_type_expected = HostUtils::GetFSType( currDir );
	QCOMPARE( _fs_info.getFileSystemType(), _fs_type_expected );
}

void PrlSrvManipulationsTest::testRemoteFsInfoGetChildEntriesCount()
{
	testLoginLocal();
	SdkHandleWrap hJob(PrlSrv_FsGetDiskList(m_ServerHandle));
	GET_FS_REQUEST_RESULT
	PRL_UINT32 nEntriesNum;
	CHECK_RET_CODE_EXP(PrlFsInfo_GetChildEntriesCount(hFsInfo, &nEntriesNum))
	GET_FS_INFO_XML_OBJECT
	QCOMPARE(nEntriesNum, PRL_UINT32(_fs_info.m_lstFileSystemItems.size()));
}

#define COMPARE_FS_ENTRIES(hFsEntry, pFsEntry)\
	PRL_CHAR sAbsolutePath[STR_BUF_LENGTH];\
	PRL_UINT32 nAbsolutePathBufLength = STR_BUF_LENGTH;\
	CHECK_RET_CODE_EXP(PrlFsEntry_GetAbsolutePath(hFsEntry, sAbsolutePath, &nAbsolutePathBufLength))\
	QCOMPARE(UTF8_2QSTR(sAbsolutePath), pFsEntry->getName());\
	PRL_CHAR sRelativeName[STR_BUF_LENGTH];\
	PRL_UINT32 nRelativeNameBufLength = STR_BUF_LENGTH;\
	CHECK_RET_CODE_EXP(PrlFsEntry_GetRelativeName(hFsEntry, sRelativeName, &nRelativeNameBufLength))\
	QCOMPARE(UTF8_2QSTR(sRelativeName), pFsEntry->getRelativeName());\
	PRL_CHAR sLastModifiedDate[STR_BUF_LENGTH];\
	PRL_UINT32 nLastModifiedDateBufLength = STR_BUF_LENGTH;\
	CHECK_RET_CODE_EXP(PrlFsEntry_GetLastModifiedDate(hFsEntry, sLastModifiedDate, &nLastModifiedDateBufLength))\
	QCOMPARE(UTF8_2QSTR(sLastModifiedDate), pFsEntry->getModified().toString(XML_DATETIME_FORMAT));\
	PRL_UINT64 nSize;\
	CHECK_RET_CODE_EXP(PrlFsEntry_GetSize(hFsEntry, &nSize))\
	QVERIFY(nSize == PRL_UINT64(pFsEntry->getSize()));\
	PRL_UINT32 nPermissions;\
	CHECK_RET_CODE_EXP(PrlFsEntry_GetPermissions(hFsEntry, &nPermissions))\
	QCOMPARE(nPermissions, pFsEntry->getPermissions());\
	PRL_FILE_SYSTEM_ELEMENT_TYPE nFsItemType;\
	CHECK_RET_CODE_EXP(PrlFsEntry_GetType(hFsEntry, &nFsItemType))\
	QCOMPARE(PRL_UINT32(nFsItemType), pFsEntry->getType());

void PrlSrvManipulationsTest::testRemoteFsInfoGetChildEntry()
{
	testLoginLocal();
	SdkHandleWrap hJob(PrlSrv_FsGetDiskList(m_ServerHandle));
	GET_FS_REQUEST_RESULT
	PRL_UINT32 nEntriesNum;
	CHECK_RET_CODE_EXP(PrlFsInfo_GetChildEntriesCount(hFsInfo, &nEntriesNum))
	if (nEntriesNum == 0)
		QSKIP("Couldn't to completely testing functionality due no necessary data", SkipAll);
	SdkHandleWrap hFsEntry;
	CHECK_RET_CODE_EXP(PrlFsInfo_GetChildEntry(hFsInfo, 0, hFsEntry.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hFsEntry, PHT_REMOTE_FILESYSTEM_ENTRY)
	GET_FS_INFO_XML_OBJECT
	COMPARE_FS_ENTRIES(hFsEntry, _fs_info.m_lstFileSystemItems.value(0))
}

void PrlSrvManipulationsTest::testRemoteFsInfoCanUseEntryAfterParentFsInfoObjFreed()
{
	testLoginLocal();
	SdkHandleWrap hFsEntry;
	CHwFileSystemInfo _fs_info;
	{
		SdkHandleWrap hJob(PrlSrv_FsGetDiskList(m_ServerHandle));
		GET_FS_REQUEST_RESULT
		PRL_UINT32 nEntriesNum;
		CHECK_RET_CODE_EXP(PrlFsInfo_GetChildEntriesCount(hFsInfo, &nEntriesNum))
		if (nEntriesNum == 0)
			QSKIP("Couldn't to completely testing functionality due no necessary data", SkipAll);
		CHECK_RET_CODE_EXP(PrlFsInfo_GetChildEntry(hFsInfo, 0, hFsEntry.GetHandlePtr()))
		PRL_VOID_PTR pBuffer = 0;
		CHECK_RET_CODE_EXP(PrlResult_GetParamToken(hResult, 0, &pBuffer))
		_fs_info.fromString(UTF8_2QSTR((const char *)pBuffer));
		PrlBuffer_Free(pBuffer);
		CHECK_RET_CODE_EXP(_fs_info.m_uiRcInit)
	}
	COMPARE_FS_ENTRIES(hFsEntry, _fs_info.m_lstFileSystemItems.value(0))
}

void PrlSrvManipulationsTest::testRemoteFsInfoGetParentEntry()
{
	testLoginLocal();
	SdkHandleWrap hJob(PrlSrv_FsGetDirEntries(m_ServerHandle, QDir::tempPath().toUtf8().constData()));
	GET_FS_REQUEST_RESULT
	GET_FS_INFO_XML_OBJECT
	SdkHandleWrap hParentFsEntry;
	if (!_fs_info.getFsParentItem())
	{
		LOG_MESSAGE(DBG_FATAL, "Couldn't to completely testing functionality due no necessary data");
		QVERIFY(PrlFsInfo_GetParentEntry(hFsInfo, hParentFsEntry.GetHandlePtr()) == PRL_ERR_NO_DATA);
		return;
	}
	CHECK_RET_CODE_EXP(PrlFsInfo_GetParentEntry(hFsInfo, hParentFsEntry.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hParentFsEntry, PHT_REMOTE_FILESYSTEM_ENTRY)
	COMPARE_FS_ENTRIES(hParentFsEntry, _fs_info.getFsParentItem())
}

void PrlSrvManipulationsTest::testGetStatistics()
{
	testLoginLocal();
	m_JobHandle.reset(PrlSrv_GetStatistics(m_ServerHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	PRL_VOID_PTR pResString = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamToken(m_ResultHandle, 0, &pResString))
	QVERIFY(pResString);
	QString sStatistics = UTF8_2QSTR((const char *)pResString);
	LOG_MESSAGE(DBG_FATAL, "sStatistics=[%s]", sStatistics.toUtf8().data());
	PrlBuffer_Free(pResString);
	QVERIFY(sStatistics.size());
	CSystemStatistics _stat(sStatistics);
	CHECK_RET_CODE_EXP(_stat.m_uiRcInit)
}

void PrlSrvManipulationsTest::testGetStatisticsOnInvalidServerHandle()
{
	testLoginLocal();
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetStatistics(PRL_INVALID_HANDLE), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testGetStatisticsOnNonServerHandle()
{
	testLoginLocal();
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetStatistics(m_ResultHandle), PRL_ERR_INVALID_ARG);
}

#define GET_HOST_SYSTEM_STATISTICS\
	testLoginLocal();\
	m_JobHandle.reset(PrlSrv_GetStatistics(m_ServerHandle));\
	CHECK_JOB_RET_CODE(m_JobHandle)\
	QVERIFY(PRL_SUCCEEDED(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr())));\
	SdkHandleWrap _stat_handle;\
	QVERIFY(PRL_SUCCEEDED(PrlResult_GetParam(m_ResultHandle, _stat_handle.GetHandlePtr())));\
	CHECK_HANDLE_TYPE(_stat_handle, PHT_SYSTEM_STATISTICS)

void PrlSrvManipulationsTest::testGetResultAsHandleForGetStatistics()
{
	GET_HOST_SYSTEM_STATISTICS
}

#define GET_SYSTEM_STATISTICS_XML_OBJ\
	PRL_VOID_PTR pResString = 0;\
	QVERIFY(PRL_SUCCEEDED(PrlResult_GetParamToken(m_ResultHandle, 0, &pResString)));\
	QVERIFY(pResString);\
	QString sStatistics = UTF8_2QSTR((const char *)pResString);\
	PrlBuffer_Free(pResString);\
	QVERIFY(sStatistics.size());\
	CSystemStatistics _stat(sStatistics);\
	CHECK_RET_CODE_EXP(_stat.m_uiRcInit)

#define CHECK_STAT_PARAMETER(sdk_param_type, sdk_call, expected_value, handle)\
	{\
		sdk_param_type _actual_value;\
		QVERIFY(PRL_SUCCEEDED(sdk_call(handle, &_actual_value)));\
		QVERIFY(_actual_value == (sdk_param_type)expected_value);\
	}

#define CHECK_STAT_STR_PARAMETER(sdk_call, expected_value, handle)\
	{\
		PRL_CHAR sValue[STR_BUF_LENGTH];\
		PRL_UINT32 nValueBufLength = STR_BUF_LENGTH;\
		QVERIFY(PRL_SUCCEEDED(sdk_call(handle, sValue, &nValueBufLength)));\
		QCOMPARE(UTF8_2QSTR(sValue), expected_value);\
	}

void PrlSrvManipulationsTest::testSystemStatisticsGetRamSwapUptimeInfo()
{
	GET_HOST_SYSTEM_STATISTICS
	GET_SYSTEM_STATISTICS_XML_OBJ
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStat_GetTotalRamSize, _stat.getMemoryStatistics()->getTotalSize(), _stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStat_GetUsageRamSize, _stat.getMemoryStatistics()->getUsageSize(), _stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStat_GetFreeRamSize, _stat.getMemoryStatistics()->getFreeSize(), _stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStat_GetRealRamSize, _stat.getMemoryStatistics()->getRealSize(), _stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStat_GetTotalSwapSize, _stat.getSwapStatistics()->getTotalSize(), _stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStat_GetUsageSwapSize, _stat.getSwapStatistics()->getUsageSize(), _stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStat_GetFreeSwapSize, _stat.getSwapStatistics()->getFreeSize(), _stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStat_GetOsUptime, _stat.getUptimeStatistics()->getOsUptime(), _stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStat_GetDispUptime, _stat.getUptimeStatistics()->getDispatcherUptime(), _stat_handle)
}

void PrlSrvManipulationsTest::testSystemStatisticsRealRamSizeOnWrongParams()
{
	GET_HOST_SYSTEM_STATISTICS;

	PRL_UINT64 nSize = 0;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStat_GetRealRamSize(m_ServerHandle, &nSize),
										PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlStat_GetRealRamSize(_stat_handle, 0),
										PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testSystemStatisticsGetCpuInfo()
{
	GET_HOST_SYSTEM_STATISTICS
	GET_SYSTEM_STATISTICS_XML_OBJ
	PRL_UINT32 nCpusCount;
	CHECK_RET_CODE_EXP(PrlStat_GetCpusStatsCount(_stat_handle, &nCpusCount))
	QVERIFY(nCpusCount == PRL_UINT32(_stat.m_lstCpusStatistics.size()));
	if (!nCpusCount)
		QSKIP("Skipping test due absent necessary test data", SkipAll);
	SdkHandleWrap _cpu_stat_handle;
	QVERIFY(PRL_FAILED(PrlStat_GetCpuStat(_stat_handle, nCpusCount, _cpu_stat_handle.GetHandlePtr())));
	CHECK_RET_CODE_EXP(PrlStat_GetCpuStat(_stat_handle, 0, _cpu_stat_handle.GetHandlePtr()))
	CHECK_HANDLE_TYPE(_cpu_stat_handle, PHT_SYSTEM_STATISTICS_CPU)
	CHECK_STAT_PARAMETER(PRL_UINT32, PrlStatCpu_GetCpuUsage, _stat.m_lstCpusStatistics.value(0)->getPercentsUsage(), _cpu_stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStatCpu_GetTotalTime, _stat.m_lstCpusStatistics.value(0)->getTotalTime(), _cpu_stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStatCpu_GetUserTime, _stat.m_lstCpusStatistics.value(0)->getUserTime(), _cpu_stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStatCpu_GetSystemTime, _stat.m_lstCpusStatistics.value(0)->getSystemTime(), _cpu_stat_handle)
}

void PrlSrvManipulationsTest::testSystemStatisticsGetIfaceInfo()
{
	GET_HOST_SYSTEM_STATISTICS
	GET_SYSTEM_STATISTICS_XML_OBJ
	PRL_UINT32 nIfacesCount;
	CHECK_RET_CODE_EXP(PrlStat_GetIfacesStatsCount(_stat_handle, &nIfacesCount))
	QVERIFY(nIfacesCount == PRL_UINT32(_stat.m_lstNetIfacesStatistics.size()));
	if (!nIfacesCount)
		QSKIP("Skipping test due absent necessary test data", SkipAll);
	SdkHandleWrap _iface_stat_handle;
	QVERIFY(PRL_FAILED(PrlStat_GetIfaceStat(_stat_handle, nIfacesCount, _iface_stat_handle.GetHandlePtr())));
	CHECK_RET_CODE_EXP(PrlStat_GetIfaceStat(_stat_handle, 0, _iface_stat_handle.GetHandlePtr()))
	CHECK_HANDLE_TYPE(_iface_stat_handle, PHT_SYSTEM_STATISTICS_IFACE)
	CHECK_STAT_STR_PARAMETER(PrlStatIface_GetSystemName, _stat.m_lstNetIfacesStatistics.value(0)->getIfaceSystemName(), _iface_stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStatIface_GetInDataSize, _stat.m_lstNetIfacesStatistics.value(0)->getInDataSize(), _iface_stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStatIface_GetOutDataSize, _stat.m_lstNetIfacesStatistics.value(0)->getOutDataSize(), _iface_stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStatIface_GetInPkgsCount, _stat.m_lstNetIfacesStatistics.value(0)->getInPkgsCount(), _iface_stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStatIface_GetOutPkgsCount, _stat.m_lstNetIfacesStatistics.value(0)->getOutPkgsCount(), _iface_stat_handle)
}

void PrlSrvManipulationsTest::testSystemStatisticsGetUserInfo()
{
	GET_HOST_SYSTEM_STATISTICS
	GET_SYSTEM_STATISTICS_XML_OBJ
	PRL_UINT32 nUsersCount;
	CHECK_RET_CODE_EXP(PrlStat_GetUsersStatsCount(_stat_handle, &nUsersCount))
	QVERIFY(nUsersCount == PRL_UINT32(_stat.m_lstUsersStatistics.size()));
	if (!nUsersCount)
		QSKIP("Skipping test due absent necessary test data", SkipAll);
	SdkHandleWrap _user_stat_handle;
	QVERIFY(PRL_FAILED(PrlStat_GetUserStat(_stat_handle, nUsersCount, _user_stat_handle.GetHandlePtr())));
	CHECK_RET_CODE_EXP(PrlStat_GetUserStat(_stat_handle, 0, _user_stat_handle.GetHandlePtr()))
	CHECK_HANDLE_TYPE(_user_stat_handle, PHT_SYSTEM_STATISTICS_USER_SESSION)
	CHECK_STAT_STR_PARAMETER(PrlStatUser_GetUserName, _stat.m_lstUsersStatistics.value(0)->getUserName(), _user_stat_handle)
	CHECK_STAT_STR_PARAMETER(PrlStatUser_GetServiceName, _stat.m_lstUsersStatistics.value(0)->getServiceName(), _user_stat_handle)
	CHECK_STAT_STR_PARAMETER(PrlStatUser_GetHostName, _stat.m_lstUsersStatistics.value(0)->getHostName(), _user_stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStatUser_GetSessionTime, _stat.m_lstUsersStatistics.value(0)->getSessionTime(), _user_stat_handle)
}

void PrlSrvManipulationsTest::testSystemStatisticsGetDiskInfo()
{
	GET_HOST_SYSTEM_STATISTICS
	GET_SYSTEM_STATISTICS_XML_OBJ
	PRL_UINT32 nDisksCount;
	CHECK_RET_CODE_EXP(PrlStat_GetDisksStatsCount(_stat_handle, &nDisksCount))
	QVERIFY(nDisksCount == PRL_UINT32(_stat.m_lstDisksStatistics.size()));
	if (!nDisksCount)
		QSKIP("Skipping test due absent necessary test data", SkipAll);
	SdkHandleWrap _disk_stat_handle;
	QVERIFY(PRL_FAILED(PrlStat_GetDiskStat(_stat_handle, nDisksCount, _disk_stat_handle.GetHandlePtr())));
	CHECK_RET_CODE_EXP(PrlStat_GetDiskStat(_stat_handle, 0, _disk_stat_handle.GetHandlePtr()))
	CHECK_HANDLE_TYPE(_disk_stat_handle, PHT_SYSTEM_STATISTICS_DISK)
	CHECK_STAT_STR_PARAMETER(PrlStatDisk_GetSystemName, _stat.m_lstDisksStatistics.value(0)->getDeviceSystemName(), _disk_stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStatDisk_GetUsageDiskSpace, _stat.m_lstDisksStatistics.value(0)->getUsageDiskSpace(), _disk_stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStatDisk_GetFreeDiskSpace, _stat.m_lstDisksStatistics.value(0)->getFreeDiskSpace(), _disk_stat_handle)
	//Testing disk partitions info
	PRL_UINT32 nDiskPartsCount;
	CHECK_RET_CODE_EXP(PrlStatDisk_GetPartsStatsCount(_disk_stat_handle, &nDiskPartsCount))
	QVERIFY(nDiskPartsCount == PRL_UINT32(_stat.m_lstDisksStatistics.value(0)->m_lstPartitionsStatistics.size()));
	if (!nDiskPartsCount)
		QSKIP("Skipping test due absent necessary test data", SkipAll);
	SdkHandleWrap _disk_part_stat_handle;
	QVERIFY(PRL_FAILED(PrlStatDisk_GetPartStat(_disk_stat_handle, nDiskPartsCount, _disk_part_stat_handle.GetHandlePtr())));
	CHECK_RET_CODE_EXP(PrlStatDisk_GetPartStat(_disk_stat_handle, 0, _disk_part_stat_handle.GetHandlePtr()))
	CHECK_HANDLE_TYPE(_disk_part_stat_handle, PHT_SYSTEM_STATISTICS_DISK_PARTITION)
	CHECK_STAT_STR_PARAMETER(PrlStatDiskPart_GetSystemName, _stat.m_lstDisksStatistics.value(0)->m_lstPartitionsStatistics.value(0)->getDeviceSystemName(), _disk_part_stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStatDiskPart_GetUsageDiskSpace, _stat.m_lstDisksStatistics.value(0)->m_lstPartitionsStatistics.value(0)->getUsageDiskSpace(), _disk_part_stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStatDiskPart_GetFreeDiskSpace, _stat.m_lstDisksStatistics.value(0)->m_lstPartitionsStatistics.value(0)->getFreeDiskSpace(), _disk_part_stat_handle)
}

void PrlSrvManipulationsTest::testSystemStatisticsGetProcessInfo()
{
	GET_HOST_SYSTEM_STATISTICS
	GET_SYSTEM_STATISTICS_XML_OBJ
	PRL_UINT32 nProcsCount;
	CHECK_RET_CODE_EXP(PrlStat_GetProcsStatsCount(_stat_handle, &nProcsCount))
	QVERIFY(nProcsCount == PRL_UINT32(_stat.m_lstProcessesStatistics.size()));
	if (!nProcsCount)
		QSKIP("Skipping test due absent necessary test data", SkipAll);
	SdkHandleWrap _proc_stat_handle;
	QVERIFY(PRL_FAILED(PrlStat_GetProcStat(_stat_handle, nProcsCount, _proc_stat_handle.GetHandlePtr())));
	CHECK_RET_CODE_EXP(PrlStat_GetProcStat(_stat_handle, 0, _proc_stat_handle.GetHandlePtr()))
	CHECK_HANDLE_TYPE(_proc_stat_handle, PHT_SYSTEM_STATISTICS_PROCESS)
	CHECK_STAT_STR_PARAMETER(PrlStatProc_GetCommandName, _stat.m_lstProcessesStatistics.value(0)->getCommandName(), _proc_stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT32, PrlStatProc_GetId, _stat.m_lstProcessesStatistics.value(0)->getProcId(), _proc_stat_handle)
	CHECK_STAT_STR_PARAMETER(PrlStatProc_GetOwnerUserName, _stat.m_lstProcessesStatistics.value(0)->getOwnerUser(), _proc_stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStatProc_GetTotalMemUsage, _stat.m_lstProcessesStatistics.value(0)->getTotalMemUsage(), _proc_stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStatProc_GetRealMemUsage, _stat.m_lstProcessesStatistics.value(0)->getRealMemUsage(), _proc_stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStatProc_GetVirtMemUsage, _stat.m_lstProcessesStatistics.value(0)->getVirtualMemUsage(), _proc_stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStatProc_GetStartTime, _stat.m_lstProcessesStatistics.value(0)->getStartTime(), _proc_stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStatProc_GetTotalTime, _stat.m_lstProcessesStatistics.value(0)->getTotalTime(), _proc_stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStatProc_GetUserTime, _stat.m_lstProcessesStatistics.value(0)->getUserTime(), _proc_stat_handle)
	CHECK_STAT_PARAMETER(PRL_UINT64, PrlStatProc_GetSystemTime, _stat.m_lstProcessesStatistics.value(0)->getSystemTime(), _proc_stat_handle)
	CHECK_STAT_PARAMETER(PRL_PROCESS_STATE_TYPE, PrlStatProc_GetState, _stat.m_lstProcessesStatistics.value(0)->getState(), _proc_stat_handle)
}

void PrlSrvManipulationsTest::setDefaultLicenseUserAndCompany()
{
	QString username("unit-test");
	QString company("parallels");

	SET_LICENSE_USER_AND_COMPANY(m_ServerHandle, username, company)
}

void PrlSrvManipulationsTest::testGetResultAsHandleForGetLicenseInfo()
{
	testLoginLocal();
	GET_LICENSE_INFO
}

#define EXTRACT_EVENT_PARAM(param_name)\
	pParam = e.getEventParameter(param_name);\
	QVERIFY(pParam != NULL);

#define CHECK_NUMERIC_PARAM_VALUE(param_type, param_name, sdk_method)\
	{\
		param_type nParamValue;\
		CHECK_RET_CODE_EXP(sdk_method(hLicense, &nParamValue))\
		EXTRACT_EVENT_PARAM(param_name)\
		QVERIFY(nParamValue == (param_type)pParam->getParamValue().toUInt());\
	}

#define CHECK_STRING_PARAM_VALUE(param_name, sdk_method)\
	{\
		EXTRACT_EVENT_PARAM(param_name)\
		PRL_CHAR sActualValue[STR_BUF_LENGTH];\
		PRL_UINT32 nBufferLength = STR_BUF_LENGTH;\
		CHECK_RET_CODE_EXP(sdk_method(hLicense, sActualValue, &nBufferLength))\
		QCOMPARE(UTF8_2QSTR(sActualValue), pParam->getParamValue());\
	}

void PrlSrvManipulationsTest::testGetLicenseInfoProps()
{
	SKIP_IF_EXTERNAL_BUILD

	SimpleServerWrapper session(0);
	AutoRestoreLicenseCreds restoreLicenseCreds(session);

	testLoginLocal();
	setDefaultLicenseUserAndCompany();
	GET_LICENSE_INFO
	PRL_VOID_PTR pBuffer = NULL;
	CHECK_RET_CODE_EXP(PrlResult_GetParamToken(hResult, 0, &pBuffer))
	QString sLicense = UTF8_2QSTR((const char *)pBuffer);
	PrlBuffer_Free(pBuffer);
	QVERIFY(sLicense.size());
	CVmEvent e;
	CHECK_RET_CODE_EXP(e.fromString(sLicense))
	CVmEventParameter *pParam = NULL;

	CHECK_NUMERIC_PARAM_VALUE(PRL_BOOL, EVT_PARAM_PRL_LICENSE_IS_VALID, PrlLic_IsValid)
	CHECK_NUMERIC_PARAM_VALUE(PRL_RESULT, EVT_PARAM_PRL_LICENSE_STATUS, PrlLic_GetStatus)
	CHECK_STRING_PARAM_VALUE(EVT_PARAM_PRL_LICENSE_KEY, PrlLic_GetLicenseKey)
	CHECK_STRING_PARAM_VALUE(EVT_PARAM_PRL_LICENSE_USER, PrlLic_GetUserName)
	CHECK_STRING_PARAM_VALUE(EVT_PARAM_PRL_LICENSE_COMPANY, PrlLic_GetCompanyName)
}

void PrlSrvManipulationsTest::testUpdateLicense()
{
	SKIP_IF_EXTERNAL_BUILD

	SimpleServerWrapper session(0);
	AutoRestoreLicenseCreds restoreLicenseCreds(session);

	testLoginLocal();
	setDefaultLicenseUserAndCompany();
	GET_LICENSE_INFO
	QString sLicenseKey, sOldUserName, sOldCompanyName;

	PRL_EXTRACT_STRING_VALUE(sLicenseKey, hLicense, PrlLic_GetLicenseKey)
	PRL_EXTRACT_STRING_VALUE(sOldUserName, hLicense, PrlLic_GetUserName)
	PRL_EXTRACT_STRING_VALUE(sOldCompanyName, hLicense, PrlLic_GetCompanyName)

	QString sNewUserName = sOldUserName + "_new";
	QString sNewCompanyName = sOldCompanyName + "_new";

	SET_LICENSE_USER_AND_COMPANY(m_ServerHandle, sNewUserName, sNewCompanyName)

	{
		GET_LICENSE_INFO
		QString sNewLicenseKey, sUserName, sCompanyName;

		PRL_EXTRACT_STRING_VALUE(sNewLicenseKey, hLicense, PrlLic_GetLicenseKey)
		PRL_EXTRACT_STRING_VALUE(sUserName, hLicense, PrlLic_GetUserName)
		PRL_EXTRACT_STRING_VALUE(sCompanyName, hLicense, PrlLic_GetCompanyName)

		QVERIFY(sNewLicenseKey.contains(sLicenseKey));
		QCOMPARE(sUserName, sNewUserName);
		QCOMPARE(sCompanyName, sNewCompanyName);
	}
}

#define CHECK_LICENSE\
	{\
		SdkHandleWrap hJob(PrlSrv_GetLicenseInfo(m_ServerHandle));\
		CHECK_JOB_RET_CODE(hJob)\
		SdkHandleWrap hResult;\
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))\
		SdkHandleWrap hLicense;\
		CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hLicense.GetHandlePtr()))\
		CHECK_HANDLE_TYPE(hLicense, PHT_LICENSE)\
		PRL_CHAR sBuffer[STR_BUF_LENGTH];\
		PRL_UINT32 nBufferLength = STR_BUF_LENGTH;\
		CHECK_RET_CODE_EXP(PrlLic_GetLicenseKey(hLicense, sBuffer, &nBufferLength))\
		sLicenseKey = UTF8_2QSTR(sBuffer);\
		nBufferLength = STR_BUF_LENGTH;\
		CHECK_RET_CODE_EXP(PrlLic_GetUserName(hLicense, sBuffer, &nBufferLength))\
		sUserName = UTF8_2QSTR(sBuffer);\
		nBufferLength = STR_BUF_LENGTH;\
		CHECK_RET_CODE_EXP(PrlLic_GetCompanyName(hLicense, sBuffer, &nBufferLength))\
		sCompanyName = UTF8_2QSTR(sBuffer);\
	}

void PrlSrvManipulationsTest::testUpdateLicenseOnInvalidLicenseKey()
{
	SKIP_IF_EXTERNAL_BUILD

	SimpleServerWrapper session(0);
	AutoRestoreLicenseCreds restoreLicenseCreds(session);

	testLoginLocal();
	setDefaultLicenseUserAndCompany();
	QString sLicenseKey, sUserName, sCompanyName;
	CHECK_LICENSE
	CHECK_ASYNC_OP_FAILED(PrlSrv_UpdateLicense(m_ServerHandle, "it's really non right license key", sUserName.toUtf8().data(), sCompanyName.toUtf8().data()), PRL_ERR_LICENSE_NOT_VALID)
	CHECK_LICENSE
}

void PrlSrvManipulationsTest::testUpdateLicenseOnProductId()
{
	SKIP_IF_EXTERNAL_BUILD

	SimpleServerWrapper session(0);
	AutoRestoreLicenseCreds restoreLicenseCreds(session);

	testLoginLocal();
	setDefaultLicenseUserAndCompany();
	QString sLicenseKey, sUserName, sCompanyName;
	CHECK_LICENSE
	CHECK_ASYNC_OP_FAILED(PrlSrv_UpdateLicense(m_ServerHandle, "078-17473-59222-17810-21076-38465", sUserName.toUtf8().data(), sCompanyName.toUtf8().data()), PRL_ERR_LICENSE_NOT_VALID)
	CHECK_LICENSE
}

void PrlSrvManipulationsTest::testUpdateLicenseOnEmptyLicenseKey()
{
	SKIP_IF_EXTERNAL_BUILD

	SimpleServerWrapper session(0);
	AutoRestoreLicenseCreds restoreLicenseCreds(session);

	testLoginLocal();
	setDefaultLicenseUserAndCompany();
	QString sLicenseKey, sUserName, sCompanyName;
	CHECK_LICENSE
	CHECK_ASYNC_OP_FAILED(PrlSrv_UpdateLicense(m_ServerHandle, "", sUserName.toUtf8().data(), sCompanyName.toUtf8().data()), PRL_ERR_LICENSE_NOT_VALID)
	CHECK_LICENSE
}

void PrlSrvManipulationsTest::testGetParamByIndexForSingleParam()
{
	testLoginLocal();
	SdkHandleWrap hJob(PrlSrv_GetUserProfile(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap aHandle;
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, aHandle.GetHandlePtr()))
	CHECK_HANDLE_TYPE(aHandle, PHT_USER_PROFILE)
}

void PrlSrvManipulationsTest::testGetParamByIndexForIndexOutOfRange()
{
	testLoginLocal();
	SdkHandleWrap hJob(PrlSrv_GetVmList(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	PRL_UINT32 nParamsCount;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))
	SdkHandleWrap aHandle;
	QVERIFY(PRL_FAILED(PrlResult_GetParamByIndex(hResult, nParamsCount, aHandle.GetHandlePtr())));
}

void PrlSrvManipulationsTest::testGetParamAsStringOnGetUserProfileRequest()
{
	testLoginLocal();
	SdkHandleWrap hJob(PrlSrv_GetUserProfile(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	QString sParamBuf;
	PRL_EXTRACT_STRING_VALUE(sParamBuf, hResult, PrlResult_GetParamAsString);
	SmartPtr<CDispUser> pUserProfile( new CDispUser );
	CHECK_RET_CODE_EXP(pUserProfile->fromString(sParamBuf))
}

void PrlSrvManipulationsTest::testGetParamByIndexAsStringOnGetUserProfileRequest()
{
	testLoginLocal();
	SdkHandleWrap hJob(PrlSrv_GetUserProfile(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	QString sParamBuf;
	PRL_EXTRACT_STRING_VALUE_BY_INDEX(sParamBuf, hResult, 0, PrlResult_GetParamByIndexAsString);
	SmartPtr<CDispUser> pUserProfile( new CDispUser );
	CHECK_RET_CODE_EXP(pUserProfile->fromString(sParamBuf))
}

void PrlSrvManipulationsTest::testGetParamByIndexAsStringOnGetUserProfileRequestWrongParamIndex()
{
	testLoginLocal();
	SdkHandleWrap hJob(PrlSrv_GetUserProfile(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	PRL_CHAR sParamBuf[STR_BUF_LENGTH];
	PRL_UINT32 nParamBufSize = sizeof(sParamBuf);
	QVERIFY(PRL_FAILED(PrlResult_GetParamByIndexAsString(hResult, 1, sParamBuf, &nParamBufSize)));
}

namespace {
struct SearchVmResult
{
	QMutex m_Mutex;
	QWaitCondition m_Condition;
	SdkHandleWrap m_hVMInfo;
};

PRL_RESULT SearchVmCallback(PRL_HANDLE _event, void *pData)
{
	SdkHandleWrap hEvent(_event);
	PRL_HANDLE_TYPE _handle_type;
	PRL_RESULT _ret_code = PrlHandle_GetType(hEvent, &_handle_type);
	if (PRL_SUCCEEDED(_ret_code))
	{
		if (_handle_type == PHT_EVENT)
		{
			PRL_EVENT_TYPE _event_type;
			_ret_code = PrlEvent_GetType(hEvent, &_event_type);
			if (PRL_SUCCEEDED(_ret_code))
			{
				if ( _event_type == PET_DSP_EVT_FOUND_LOST_VM_CONFIG )
				{
					PRL_UINT32 nEventParamsCount = 0;
					_ret_code = PrlEvent_GetParamsCount(hEvent, &nEventParamsCount);
					if (PRL_SUCCEEDED(_ret_code))
					{
						for (PRL_UINT32 i = 0; i < nEventParamsCount; ++i)
						{
							SdkHandleWrap hEventParameter;
							_ret_code = PrlEvent_GetParam(hEvent, i, hEventParameter.GetHandlePtr());
							if (PRL_SUCCEEDED(_ret_code))
							{
								QByteArray sParamNameBuf;
								PRL_UINT32 nParamNameBufLength = 0;
								_ret_code = PrlEvtPrm_GetName(hEventParameter, 0, &nParamNameBufLength);
								if (PRL_SUCCEEDED(_ret_code))
								{
									sParamNameBuf.resize(nParamNameBufLength);
									_ret_code = PrlEvtPrm_GetName(hEventParameter, sParamNameBuf.data(), &nParamNameBufLength);
									if (PRL_SUCCEEDED(_ret_code))
									{
										if (UTF8_2QSTR(sParamNameBuf.constData()) == EVT_PARAM_VM_SEARCH_INFO)
										{
											SearchVmResult *pResult = static_cast<SearchVmResult *>(pData);
											QMutexLocker _lock(&pResult->m_Mutex);
											_ret_code = PrlEvtPrm_ToHandle(hEventParameter, pResult->m_hVMInfo.GetHandlePtr());
											if (PRL_SUCCEEDED(_ret_code))
											{
												pResult->m_Condition.wakeAll();
												break;
											}
											else
												WRITE_TRACE(DBG_FATAL, "PrlEvtPrm_ToHandle() call failed. Error code: %.8X", _ret_code);
										}
									}
									else
										WRITE_TRACE(DBG_FATAL, "Error occured on PrlEvtPrm_GetName() call: %.8X '%s'", _ret_code, prl_result_to_string(_ret_code));
								}
								else
									WRITE_TRACE(DBG_FATAL, "Error occured on PrlEvtPrm_GetName() call with null length: %.8X '%s'", _ret_code, prl_result_to_string(_ret_code));
							}
						}
					}
					else
						WRITE_TRACE(DBG_FATAL, "Error occured on PrlEvent_GetParamsCount() call: %.8X '%s'", _ret_code, prl_result_to_string(_ret_code));
				}
			}
			else
				WRITE_TRACE(DBG_FATAL, "Error occured on PrlEvent_GetType() call: %.8X '%s'", _ret_code, prl_result_to_string(_ret_code));
		}
	}
	else
		WRITE_TRACE(DBG_FATAL, "Error occured on PrlHandle_GetType() call: %.8X '%s'", _ret_code, prl_result_to_string(_ret_code));
	return (PRL_ERR_SUCCESS);
}

}

void PrlSrvManipulationsTest::testStartSearchVms()
{
	testLoginLocal();
	QVERIFY(QDir().mkdir(m_sTestFsDirName1));
	QString sExpectedVmPath = m_sTestFsDirName1 + "/config.pvs";
	QVERIFY(QFile::copy("./TestDspCmdDirValidateVmConfig_vm_config.xml", sExpectedVmPath));

	QFile _file("./TestDspCmdDirValidateVmConfig_vm_config.xml");
	QVERIFY(_file.open(QIODevice::ReadOnly));
	QTextStream _stream(&_file);
	QString _config = _stream.readAll();
	CVmConfiguration _vm_conf;
	CHECK_RET_CODE_EXP(_vm_conf.fromString(_config));

	QString sExpectedVmName = _vm_conf.getVmIdentification()->getVmName();
	unsigned int nExpectedVmOsNumber = _vm_conf.getVmSettings()->getVmCommonOptions()->getOsVersion();

	SearchVmResult _search_vm_result;
	{
		QMutexLocker _lock(&_search_vm_result.m_Mutex);
		CHECK_RET_CODE_EXP(PrlSrv_RegEventHandler(m_ServerHandle, SearchVmCallback, &_search_vm_result))

		SdkHandleWrap hStringsList;
		CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hStringsList.GetHandlePtr()))
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hStringsList, m_sTestFsDirName1.toUtf8().constData()))
		SdkHandleWrap hJob(PrlSrv_StartSearchVms(m_ServerHandle, hStringsList));

		bool bWaitEventResult = _search_vm_result.m_Condition.wait(&_search_vm_result.m_Mutex, PRL_JOB_WAIT_TIMEOUT);

		CHECK_RET_CODE_EXP(PrlSrv_UnregEventHandler(m_ServerHandle, SearchVmCallback, &_search_vm_result))
		QVERIFY(bWaitEventResult);

		CHECK_HANDLE_TYPE(_search_vm_result.m_hVMInfo, PHT_FOUND_VM_INFO)

	// Check callback params
	// Check Name
		PRL_CHAR sBuffer[STR_BUF_LENGTH];
		PRL_UINT32 nBufferLength = STR_BUF_LENGTH;
		CHECK_RET_CODE_EXP(PrlFoundVmInfo_GetName(_search_vm_result.m_hVMInfo, sBuffer, &nBufferLength));
		QCOMPARE(UTF8_2QSTR(sBuffer), sExpectedVmName);
	// Check OsVersion
		PRL_UINT32 nOsVersion;
		CHECK_RET_CODE_EXP(PrlFoundVmInfo_GetOSVersion(_search_vm_result.m_hVMInfo, &nOsVersion));
		QCOMPARE(nOsVersion, nExpectedVmOsNumber);
	// Check old config
		PRL_BOOL bOldConfig;
		CHECK_RET_CODE_EXP(PrlFoundVmInfo_IsOldConfig(_search_vm_result.m_hVMInfo, &bOldConfig));
		QVERIFY(!bOldConfig);
	// Check path
		nBufferLength = STR_BUF_LENGTH;
		CHECK_RET_CODE_EXP(PrlFoundVmInfo_GetConfigPath(_search_vm_result.m_hVMInfo, sBuffer, &nBufferLength));
		QVERIFY(QFileInfo(sBuffer) == QFileInfo(sExpectedVmPath));
	// Check is template
		PRL_BOOL bIsTemplate;
		CHECK_RET_CODE_EXP(PrlFoundVmInfo_IsTemplate(_search_vm_result.m_hVMInfo, &bIsTemplate));
		QVERIFY(!bIsTemplate);

		CHECK_JOB_RET_CODE(hJob)
		SdkHandleWrap hResult;
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
		PRL_UINT32 nParamsCount;
		CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))
		QCOMPARE(quint32(nParamsCount), quint32(1));

		SdkHandleWrap hFoundVmInfo;
		CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hFoundVmInfo.GetHandlePtr()));

		CHECK_HANDLE_TYPE(hFoundVmInfo, PHT_FOUND_VM_INFO)

	// Check Result params
	// Check Name
		nBufferLength = STR_BUF_LENGTH;
		CHECK_RET_CODE_EXP(PrlFoundVmInfo_GetName(hFoundVmInfo, sBuffer, &nBufferLength));
		QCOMPARE(UTF8_2QSTR(sBuffer), sExpectedVmName);
	// Check OsVersion
		nOsVersion = 0;
		CHECK_RET_CODE_EXP(PrlFoundVmInfo_GetOSVersion(hFoundVmInfo, &nOsVersion));
		QCOMPARE(nOsVersion, nExpectedVmOsNumber);
	// Check old config
		bOldConfig = false;
		CHECK_RET_CODE_EXP(PrlFoundVmInfo_IsOldConfig(hFoundVmInfo, &bOldConfig));
		QVERIFY(!bOldConfig);
	// Check path
		nBufferLength = STR_BUF_LENGTH;
		CHECK_RET_CODE_EXP(PrlFoundVmInfo_GetConfigPath(hFoundVmInfo, sBuffer, &nBufferLength));
		QVERIFY(QFileInfo(sBuffer) == QFileInfo(sExpectedVmPath));
	// Check is template
		bIsTemplate = true;
		CHECK_RET_CODE_EXP(PrlFoundVmInfo_IsTemplate(hFoundVmInfo, &bIsTemplate));
		QVERIFY(!bIsTemplate);
	}
}

void PrlSrvManipulationsTest::testStartSearchVmsWithSubdirs()
{
	testLoginLocal();
	QVERIFY(QDir().mkdir(m_sTestFsDirName1));
	QVERIFY(QDir().mkdir(m_sTestFsDirName1ChildDir));
	QString sExpectedVmPath = m_sTestFsDirName1ChildDir + "/config.pvs";
	QVERIFY(QFile::copy("./TestDspCmdDirValidateVmConfig_vm_config.xml", sExpectedVmPath));

	QFile _file("./TestDspCmdDirValidateVmConfig_vm_config.xml");
	QVERIFY(_file.open(QIODevice::ReadOnly));
	QTextStream _stream(&_file);
	QString _config = _stream.readAll();
	CVmConfiguration _vm_conf;
	CHECK_RET_CODE_EXP(_vm_conf.fromString(_config));

	QString sExpectedVmName = _vm_conf.getVmIdentification()->getVmName();
	unsigned int nExpectedVmOsNumber = _vm_conf.getVmSettings()->getVmCommonOptions()->getOsVersion();

	SearchVmResult _search_vm_result;
	{
		QMutexLocker _lock(&_search_vm_result.m_Mutex);
		CHECK_RET_CODE_EXP(PrlSrv_RegEventHandler(m_ServerHandle, SearchVmCallback, &_search_vm_result))

		SdkHandleWrap hStringsList;
		CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hStringsList.GetHandlePtr()))
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hStringsList, m_sTestFsDirName1.toUtf8().constData()))
		SdkHandleWrap hJob(PrlSrv_StartSearchVms(m_ServerHandle, hStringsList));

		bool bWaitEventResult = _search_vm_result.m_Condition.wait(&_search_vm_result.m_Mutex, PRL_JOB_WAIT_TIMEOUT);

		CHECK_RET_CODE_EXP(PrlSrv_UnregEventHandler(m_ServerHandle, SearchVmCallback, &_search_vm_result))
		QVERIFY(bWaitEventResult);

		CHECK_HANDLE_TYPE(_search_vm_result.m_hVMInfo, PHT_FOUND_VM_INFO)

	// Check callback params
	// Check Name
		PRL_CHAR sBuffer[STR_BUF_LENGTH];
		PRL_UINT32 nBufferLength = STR_BUF_LENGTH;
		CHECK_RET_CODE_EXP(PrlFoundVmInfo_GetName(_search_vm_result.m_hVMInfo, sBuffer, &nBufferLength));
		QCOMPARE(UTF8_2QSTR(sBuffer), sExpectedVmName);
	// Check OsVersion
		PRL_UINT32 nOsVersion;
		CHECK_RET_CODE_EXP(PrlFoundVmInfo_GetOSVersion(_search_vm_result.m_hVMInfo, &nOsVersion));
		QCOMPARE(nOsVersion, nExpectedVmOsNumber);
	// Check old config
		PRL_BOOL bOldConfig;
		CHECK_RET_CODE_EXP(PrlFoundVmInfo_IsOldConfig(_search_vm_result.m_hVMInfo, &bOldConfig));
		QVERIFY(!bOldConfig);
	// Check path
		nBufferLength = STR_BUF_LENGTH;
		CHECK_RET_CODE_EXP(PrlFoundVmInfo_GetConfigPath(_search_vm_result.m_hVMInfo, sBuffer, &nBufferLength));
		QVERIFY(QFileInfo(sBuffer) == QFileInfo(sExpectedVmPath));
	// Check is template
		PRL_BOOL bIsTemplate;
		CHECK_RET_CODE_EXP(PrlFoundVmInfo_IsTemplate(_search_vm_result.m_hVMInfo, &bIsTemplate));
		QVERIFY(!bIsTemplate);

		CHECK_JOB_RET_CODE(hJob)
		SdkHandleWrap hResult;
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
		PRL_UINT32 nParamsCount;
		CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))
		QCOMPARE(quint32(nParamsCount), quint32(1));

		SdkHandleWrap hFoundVmInfo;
		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, hFoundVmInfo.GetHandlePtr()));

		CHECK_HANDLE_TYPE(hFoundVmInfo, PHT_FOUND_VM_INFO)

	// Check Result params
	// Check Name
		nBufferLength = STR_BUF_LENGTH;
		CHECK_RET_CODE_EXP(PrlFoundVmInfo_GetName(hFoundVmInfo, sBuffer, &nBufferLength));
		QCOMPARE(UTF8_2QSTR(sBuffer), sExpectedVmName);
	// Check OsVersion
		nOsVersion = 0;
		CHECK_RET_CODE_EXP(PrlFoundVmInfo_GetOSVersion(hFoundVmInfo, &nOsVersion));
		QCOMPARE(nOsVersion, nExpectedVmOsNumber);
	// Check old config
		bOldConfig = false;
		CHECK_RET_CODE_EXP(PrlFoundVmInfo_IsOldConfig(hFoundVmInfo, &bOldConfig));
		QVERIFY(!bOldConfig);
	// Check path
		nBufferLength = STR_BUF_LENGTH;
		CHECK_RET_CODE_EXP(PrlFoundVmInfo_GetConfigPath(hFoundVmInfo, sBuffer, &nBufferLength));
		QVERIFY(QFileInfo(sBuffer) == QFileInfo(sExpectedVmPath));
	// Check is template
		bIsTemplate = true;
		CHECK_RET_CODE_EXP(PrlFoundVmInfo_IsTemplate(hFoundVmInfo, &bIsTemplate));
		QVERIFY(!bIsTemplate);
	}
}

#define REGISTER_VM_PROLOG\
	testLoginLocal();\
	QVERIFY(QDir().mkdir(m_sTestFsDirName1));\
	QVERIFY(QDir().mkdir(m_sTestFsDirName1ChildDir));\
	QString sExpectedVmPath = m_sTestFsDirName1ChildDir + "/config.pvs";\
	QVERIFY(QFile::copy("./TestDspCmdDirValidateVmConfig_vm_config.xml", sExpectedVmPath));\
	QFile _file("./TestDspCmdDirValidateVmConfig_vm_config.xml");\
	QVERIFY(_file.open(QIODevice::ReadOnly));\
	QTextStream _stream(&_file);\
	QString _config = _stream.readAll();\
	CVmConfiguration _vm_conf;\
	CHECK_RET_CODE_EXP(_vm_conf.fromString(_config))

#define FIND_REGISTERED_VM_AT_SERVER_VMS_LIST\
	CHECK_JOB_RET_CODE(hJob)\
	hJob.reset(PrlSrv_GetVmList(m_ServerHandle));\
	CHECK_JOB_RET_CODE(hJob)\
	SdkHandleWrap hResult;\
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))\
	PRL_UINT32 nParamsCount = 0;\
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))\
	QVERIFY(nParamsCount > 0);\
	for (PRL_UINT32 i = 0; i < nParamsCount; ++i)\
	{\
		SdkHandleWrap hVm;\
		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, i, hVm.GetHandlePtr()))\
		QByteArray sVmName;\
		PRL_UINT32 nVmNameBufSize = 0;\
		CHECK_RET_CODE_EXP(PrlVmCfg_GetName(hVm, 0, &nVmNameBufSize))\
		sVmName.resize(nVmNameBufSize);\
		CHECK_RET_CODE_EXP(PrlVmCfg_GetName(hVm, sVmName.data(), &nVmNameBufSize))\
		if (UTF8_2QSTR(sVmName) == _vm_conf.getVmIdentification()->getVmName())\
		{\
			m_VmHandle = hVm;\
			break;\
		}\
	}\
	QVERIFY(m_VmHandle != PRL_INVALID_HANDLE);

void PrlSrvManipulationsTest::testStartSearchVmsOnRegisteredVm()
{
	REGISTER_VM_PROLOG
	SdkHandleWrap hJob(PrlSrv_RegisterVm(m_ServerHandle, m_sTestFsDirName1ChildDir.toUtf8().constData(), PRL_TRUE));
	FIND_REGISTERED_VM_AT_SERVER_VMS_LIST
	SearchVmResult _search_vm_result;
	{
		QMutexLocker _lock(&_search_vm_result.m_Mutex);
		CHECK_RET_CODE_EXP(PrlSrv_RegEventHandler(m_ServerHandle, SearchVmCallback, &_search_vm_result))

		SdkHandleWrap hStringsList;
		CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hStringsList.GetHandlePtr()))
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hStringsList, m_sTestFsDirName1.toUtf8().constData()))
		SdkHandleWrap hJob(PrlSrv_StartSearchVms(m_ServerHandle, hStringsList));

		bool bWaitEventResult = _search_vm_result.m_Condition.wait(&_search_vm_result.m_Mutex, PRL_JOB_WAIT_TIMEOUT);

		CHECK_RET_CODE_EXP(PrlSrv_UnregEventHandler(m_ServerHandle, SearchVmCallback, &_search_vm_result))
		QVERIFY(!bWaitEventResult);
	}
}

void PrlSrvManipulationsTest::testSmcGetRuntimeInfo()
{
	testLoginLocal();
	SdkHandleWrap hJob(PrlSrv_GetUserProfile(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hUserProfile;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hUserProfile.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hUserProfile, PHT_USER_PROFILE)
	PRL_BOOL bCanUseMngConsole;
	CHECK_RET_CODE_EXP(PrlUsrCfg_CanUseMngConsole(hUserProfile, &bCanUseMngConsole))
	if (bCanUseMngConsole)
	{
		hJob.reset(PrlSrv_SmcGetRuntimeInfo(m_ServerHandle));
		CHECK_JOB_RET_CODE(hJob)
		SdkHandleWrap hResult;
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
		PRL_UINT32 nRuntimeInfoSize = 0;
		CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, 0, &nRuntimeInfoSize))
		PRL_STR sRuntimeInfoBuf = (PRL_STR)malloc(nRuntimeInfoSize*sizeof(PRL_CHAR));
		CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, sRuntimeInfoBuf, &nRuntimeInfoSize))
		QString sRuntimeInfo = UTF8_2QSTR(sRuntimeInfoBuf);
		free(sRuntimeInfoBuf);
		SmartPtr<CVmEvent> pVmEvent( new CVmEvent );
		CHECK_RET_CODE_EXP(pVmEvent->fromString(sRuntimeInfo))
	}
	else
	{
		CHECK_ASYNC_OP_FAILED(PrlSrv_SmcGetRuntimeInfo(m_ServerHandle), PRL_ERR_ACCESS_DENIED)
	}
}

namespace {
struct StatisticsInfoStruct
{
	QWaitCondition m_Condition;
	QMutex m_Mutex;
	SdkHandleWrap m_hStatistics;
	SdkHandleWrap m_hServer;
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
				if (_event_type == PET_DSP_EVT_HOST_STATISTICS_UPDATED)
				{
					StatisticsInfoStruct *pStatisticsStruct = static_cast<StatisticsInfoStruct *>(pData);
					QMutexLocker _lock(&pStatisticsStruct->m_Mutex);
					nRetCode = PrlSrv_UnregEventHandler(pStatisticsStruct->m_hServer, StatisticsCallback, pData);
					if (PRL_FAILED(nRetCode))
						WRITE_TRACE(DBG_FATAL, "PrlSrv_UnregEventHandler() call failed. Error code: %.8X", nRetCode);
					PRL_UINT32 nParamsCount = 0;
					nRetCode = PrlEvent_GetParamsCount(hEvent, &nParamsCount);
					if (PRL_SUCCEEDED(nRetCode))
					{
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
										nRetCode = PrlEvtPrm_ToHandle(hEventParameter, pStatisticsStruct->m_hStatistics.GetHandlePtr());
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

void PrlSrvManipulationsTest::testSubscribeToHostStatistics()
{
	testLoginLocal();
	SmartPtr<StatisticsInfoStruct> pStatInfo( new StatisticsInfoStruct );
	pStatInfo->m_hServer = m_ServerHandle;
	CHECK_RET_CODE_EXP(PrlSrv_RegEventHandler(m_ServerHandle, StatisticsCallback, pStatInfo.getImpl()))
	SdkHandleWrap hJob;
	bool bRes = false;
	{
		QMutexLocker _lock(&pStatInfo->m_Mutex);
		hJob.reset(PrlSrv_SubscribeToHostStatistics(m_ServerHandle));
		bRes = pStatInfo->m_Condition.wait(&pStatInfo->m_Mutex, PRL_JOB_WAIT_TIMEOUT);
	}
	CHECK_JOB_RET_CODE(hJob)
	hJob.reset(PrlSrv_UnsubscribeFromHostStatistics(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob)
	QVERIFY(bRes);
	QMutexLocker _lock(&pStatInfo->m_Mutex);
	CHECK_HANDLE_TYPE(pStatInfo->m_hStatistics, PHT_SYSTEM_STATISTICS)
}

void PrlSrvManipulationsTest::testRegVmOnInvalidVmConfig()
{
	testLoginLocal();
	QVERIFY(QDir().mkdir(m_sTestFsDirName1));
	QVERIFY(QFile::copy("./SDKTest_InvalidVmConfig.pvs", m_sTestFsDirName1 + "/config.pvs"));
	SdkHandleWrap hJob(PrlSrv_RegisterVm(m_ServerHandle, m_sTestFsDirName1.toUtf8().constData(), PRL_TRUE));
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT))
	PRL_RESULT nJobRetCode = PRL_ERR_SUCCESS;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nJobRetCode))
	QVERIFY(PRL_FAILED(nJobRetCode));
}

void PrlSrvManipulationsTest::testGetNetServiceStatus()
{
	LOGIN_TO_SERVER
	m_JobHandle.reset(PrlSrv_GetNetServiceStatus(m_ServerHandle));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))
	PRL_RESULT _ret_code = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(m_JobHandle, &_ret_code))
	CHECK_RET_CODE_EXP(_ret_code)
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	PRL_VOID_PTR pResString = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamToken(m_ResultHandle, 0, &pResString))
	QVERIFY(pResString);
	QString sNetServiceStatus = UTF8_2QSTR((const char *)pResString);
	PrlBuffer_Free(pResString);
	QVERIFY(!sNetServiceStatus.isEmpty());
}

void PrlSrvManipulationsTest::testGetNetServiceStatusOnInvalidServerHandle()
{
	LOGIN_TO_SERVER
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetNetServiceStatus(PRL_INVALID_HANDLE), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testGetNetServiceStatusOnNonServerHandle()
{
	LOGIN_TO_SERVER
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetNetServiceStatus(m_ResultHandle), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testGetNetServiceStatusAsHandle()
{
	LOGIN_TO_SERVER
	m_JobHandle.reset(PrlSrv_GetNetServiceStatus(m_ServerHandle));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))
	PRL_RESULT _ret_code = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(m_JobHandle, &_ret_code))
	CHECK_RET_CODE_EXP(_ret_code)
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	SdkHandleWrap hNetStatus;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(m_ResultHandle, hNetStatus.GetHandlePtr()))
	PRL_SERVICE_STATUS_ENUM nNetStatus;
	CHECK_RET_CODE_EXP(PrlNetSvc_GetStatus(hNetStatus, &nNetStatus))
	QVERIFY(nNetStatus != PSS_UNKNOWN);
}

void PrlSrvManipulationsTest::testAddNetAdapter()
{
	RECEIVE_DISP_CONFIG
	SdkHandleWrap hDispNet;
	CHECK_RET_CODE_EXP(PrlDispCfg_CreateDispNet(hDispConfig, hDispNet.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hDispNet, PHT_DISP_NET_ADAPTER)

	// It needs to set wrong index in order this test does not add any adapters during executing tests
	CHECK_RET_CODE_EXP(PrlDispNet_SetIndex(hDispNet, 1000))
	m_JobHandle.reset(PrlSrv_AddNetAdapter(m_ServerHandle,  hDispNet));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))
	PRL_RESULT _ret_code = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(m_JobHandle, &_ret_code))
	if (!PRL_SUCCEEDED(_ret_code)) WRITE_TRACE(DBG_FATAL, "_ret_code=%.8X", _ret_code);
}

void PrlSrvManipulationsTest::testDeleteNetAdapter()
{
	// It needs to set wrong index in order this test does not delete any adapters during executing tests
	m_JobHandle.reset(PrlSrv_DeleteNetAdapter(m_ServerHandle,  1000));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))
	PRL_RESULT _ret_code = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(m_JobHandle, &_ret_code))
	if (!PRL_SUCCEEDED(_ret_code)) WRITE_TRACE(DBG_FATAL, "_ret_code=%.8X", _ret_code);
}

void PrlSrvManipulationsTest::testUpdateNetAdapter()
{
	RECEIVE_DISP_CONFIG
	SdkHandleWrap hDispNet;
	CHECK_RET_CODE_EXP(PrlDispCfg_CreateDispNet(hDispConfig, hDispNet.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hDispNet, PHT_DISP_NET_ADAPTER)

	// It needs to set wrong index in order this test does not update any adapters during executing tests
	CHECK_RET_CODE_EXP(PrlDispNet_SetIndex(hDispNet, 1000))
	m_JobHandle.reset(PrlSrv_UpdateNetAdapter(m_ServerHandle,  hDispNet));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))
	PRL_RESULT _ret_code = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(m_JobHandle, &_ret_code))
	if (!PRL_SUCCEEDED(_ret_code)) WRITE_TRACE(DBG_FATAL, "_ret_code=%.8X", _ret_code);
}

#define CHECK_USER_HOME_FOLDER\
	m_JobHandle.reset(PrlSrv_GetUserProfile(m_ServerHandle));\
	CHECK_JOB_RET_CODE(m_JobHandle)\
	SdkHandleWrap hResult;\
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()))\
	QByteArray sUserProfile;\
	PRL_UINT32 nUserProfileBufSize = 0;\
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, 0, &nUserProfileBufSize))\
	sUserProfile.resize(nUserProfileBufSize);\
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, sUserProfile.data(), &nUserProfileBufSize))\
	CDispUser _user_conf;\
	CHECK_RET_CODE_EXP(_user_conf.fromString(UTF8_2QSTR(sUserProfile)))\
	QVERIFY(!_auth.getHomePath().isEmpty());\
	QCOMPARE(_user_conf.getUserWorkspace()->getUserHomeFolder(), _auth.getHomePath());

void PrlSrvManipulationsTest::testHomeUserFolderValid()
{
	if (!TestConfig::isServerMode())
		QSKIP("Test skipping due functionality not supported at desktop mode", SkipAll);

	m_JobHandle.reset(PrlSrv_Login(m_ServerHandle, TestConfig::getRemoteHostName(),	TestConfig::getUserLogin(),
								   TestConfig::getUserPassword(), NULL, 0, 0, PSL_HIGH_SECURITY));
	CHECK_JOB_RET_CODE(m_JobHandle)
	CAuthHelper _auth(TestConfig::getUserLogin());
	QVERIFY(_auth.AuthUser(TestConfig::getUserPassword()));

	CHECK_USER_HOME_FOLDER
}

void PrlSrvManipulationsTest::testHomeUserFolderValid2()
{
	if (!TestConfig::isServerMode())
		QSKIP("Test skipping due functionality not supported at desktop mode", SkipAll);

	m_JobHandle.reset(PrlSrv_Login(m_ServerHandle, TestConfig::getRemoteHostName(),	TestConfig::getUserLogin2(),
								   TestConfig::getUserPassword(), NULL, 0, 0, PSL_HIGH_SECURITY));
	CHECK_JOB_RET_CODE(m_JobHandle)
	CAuthHelper _auth(TestConfig::getUserLogin2());
	QVERIFY(_auth.AuthUser(TestConfig::getUserPassword()));

	CHECK_USER_HOME_FOLDER
}

void PrlSrvManipulationsTest::testHomeUserFolderValid3()
{
	m_JobHandle.reset(PrlSrv_LoginLocal(m_ServerHandle, NULL, 0, PSL_HIGH_SECURITY));
	CHECK_JOB_RET_CODE(m_JobHandle)
	CAuthHelper _auth;
	QVERIFY(_auth.AuthUser(PrlGetCurrentUserId()));

	CHECK_USER_HOME_FOLDER
}

namespace {
struct TestCallbackData
{
	TestCallbackData()
	: m_nCounter(0)
	{}

	QMutex m_Mutex;
	QWaitCondition m_Condition;
	quint32 m_nCounter;
};

}

static PRL_RESULT TestCallback(PRL_HANDLE handle, void *pData)
{
	SdkHandleWrap _handle(handle);
	PRL_HANDLE_TYPE _type;
	PRL_RESULT _ret = PrlHandle_GetType(_handle, &_type);
	if (PRL_SUCCEEDED(_ret))
	{
		if (_type == PHT_JOB)
		{
			TestCallbackData &_data = *static_cast<TestCallbackData *>(pData);
			QMutexLocker _lock(&_data.m_Mutex);
			_data.m_nCounter++;
			_data.m_Condition.wakeAll();
		}
	}
	else
		WRITE_TRACE(DBG_FATAL, "Couldn't to extract handle type: %.8x '%s'", _ret, prl_result_to_string(_ret));

	return (PRL_ERR_SUCCESS);
}

void PrlSrvManipulationsTest::testMultipleRegistrationOfTheSameCallback()
{
	TestCallbackData _data1, _data2;
	CHECK_RET_CODE_EXP(PrlSrv_RegEventHandler(m_ServerHandle, TestCallback, &_data1))
	CHECK_RET_CODE_EXP(PrlSrv_RegEventHandler(m_ServerHandle, TestCallback, &_data2))
	if (TestConfig::isServerMode())
	{
		m_JobHandle.reset(PrlSrv_Login(m_ServerHandle, TestConfig::getRemoteHostName(),	TestConfig::getUserLogin(),
									   TestConfig::getUserPassword(), NULL, 0, 0, PSL_HIGH_SECURITY));
	}
	else
	{
		m_JobHandle.reset(PrlSrv_LoginLocal(m_ServerHandle, NULL, 0, PSL_HIGH_SECURITY));
	}

	{
		QMutexLocker _lock(&_data1.m_Mutex);
		while (_data1.m_Condition.wait(&_data1.m_Mutex, PRL_JOB_WAIT_TIMEOUT)) ;
	}
	{
		QMutexLocker _lock(&_data2.m_Mutex);
		while (_data2.m_Condition.wait(&_data2.m_Mutex, PRL_JOB_WAIT_TIMEOUT)) ;
	}
	CHECK_RET_CODE_EXP(PrlSrv_UnregEventHandler(m_ServerHandle, TestCallback, &_data1))
	CHECK_RET_CODE_EXP(PrlSrv_UnregEventHandler(m_ServerHandle, TestCallback, &_data2))
	{
		QMutexLocker _lock(&_data1.m_Mutex);
		QCOMPARE(_data1.m_nCounter, quint32(1));
	}
	{
		QMutexLocker _lock(&_data2.m_Mutex);
		QCOMPARE(_data2.m_nCounter, quint32(1));
	}
}

void PrlSrvManipulationsTest::testFsGenerateEntryName()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_FsGenerateEntryName(m_ServerHandle, QDir::tempPath().toUtf8().constData(),
												  "myfile", ".txt", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QByteArray sGeneratedEntryName;
	PRL_UINT32 nGeneratedEntryNameBufSize = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, 0, &nGeneratedEntryNameBufSize))
	QVERIFY(nGeneratedEntryNameBufSize != 0);
	sGeneratedEntryName.resize(nGeneratedEntryNameBufSize);
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, sGeneratedEntryName.data(), &nGeneratedEntryNameBufSize))

	QVERIFY(UTF8_2QSTR(sGeneratedEntryName).startsWith("myfile"));
	QVERIFY(UTF8_2QSTR(sGeneratedEntryName).endsWith(".txt"));
}

void PrlSrvManipulationsTest::testFsGenerateEntryNameOnNonExistsTargetDir()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_FsGenerateEntryName(m_ServerHandle, "", "myfile", ".txt", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QByteArray sGeneratedEntryName;
	PRL_UINT32 nGeneratedEntryNameBufSize = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, 0, &nGeneratedEntryNameBufSize))
	QVERIFY(nGeneratedEntryNameBufSize != 0);
	sGeneratedEntryName.resize(nGeneratedEntryNameBufSize);
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, sGeneratedEntryName.data(), &nGeneratedEntryNameBufSize))

	QVERIFY(UTF8_2QSTR(sGeneratedEntryName).startsWith("myfile"));
	QVERIFY(UTF8_2QSTR(sGeneratedEntryName).endsWith(".txt"));
}

void PrlSrvManipulationsTest::testFsGenerateEntryNameOnNullServerHandle()
{
	CHECK_ASYNC_OP_FAILED(PrlSrv_FsGenerateEntryName(PRL_INVALID_HANDLE, QDir::tempPath().toUtf8().constData(),\
							"myfile", ".txt", 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testFsGenerateEntryNameOnNonServerHandle()
{
	testLoginLocal();
	CHECK_ASYNC_OP_FAILED(PrlSrv_FsGenerateEntryName(m_JobHandle, QDir::tempPath().toUtf8().constData(),\
							"myfile", ".txt", 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testFsGenerateEntryNameOnNullTargetDirPath()
{
	testLoginLocal();
	CHECK_ASYNC_OP_FAILED(PrlSrv_FsGenerateEntryName(m_ServerHandle, 0, "myfile", ".txt", 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testFsGenerateEntryNameOnEmptyPrefix()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_FsGenerateEntryName(m_ServerHandle, QDir::tempPath().toUtf8().constData(),
												  "", ".txt", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QByteArray sGeneratedEntryName;
	PRL_UINT32 nGeneratedEntryNameBufSize = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, 0, &nGeneratedEntryNameBufSize))
	QVERIFY(nGeneratedEntryNameBufSize != 0);
	sGeneratedEntryName.resize(nGeneratedEntryNameBufSize);
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, sGeneratedEntryName.data(), &nGeneratedEntryNameBufSize))

	QVERIFY(UTF8_2QSTR(sGeneratedEntryName).startsWith("tmpfile"));
	QVERIFY(UTF8_2QSTR(sGeneratedEntryName).endsWith(".txt"));
}

void PrlSrvManipulationsTest::testFsGenerateEntryNameOnNullPrefix()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_FsGenerateEntryName(m_ServerHandle, QDir::tempPath().toUtf8().constData(),
												  0, ".txt", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QByteArray sGeneratedEntryName;
	PRL_UINT32 nGeneratedEntryNameBufSize = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, 0, &nGeneratedEntryNameBufSize))
	QVERIFY(nGeneratedEntryNameBufSize != 0);
	sGeneratedEntryName.resize(nGeneratedEntryNameBufSize);
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, sGeneratedEntryName.data(), &nGeneratedEntryNameBufSize))

	QVERIFY(UTF8_2QSTR(sGeneratedEntryName).startsWith("tmpfile"));
	QVERIFY(UTF8_2QSTR(sGeneratedEntryName).endsWith(".txt"));
}

void PrlSrvManipulationsTest::testFsGenerateEntryNameOnEmptySuffix()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_FsGenerateEntryName(m_ServerHandle, QDir::tempPath().toUtf8().constData(),
												  "myfile", "", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QByteArray sGeneratedEntryName;
	PRL_UINT32 nGeneratedEntryNameBufSize = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, 0, &nGeneratedEntryNameBufSize))
	QVERIFY(nGeneratedEntryNameBufSize != 0);
	sGeneratedEntryName.resize(nGeneratedEntryNameBufSize);
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, sGeneratedEntryName.data(), &nGeneratedEntryNameBufSize))

	QVERIFY(UTF8_2QSTR(sGeneratedEntryName).startsWith("myfile"));
}

void PrlSrvManipulationsTest::testFsGenerateEntryNameOnNullSuffix()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_FsGenerateEntryName(m_ServerHandle, QDir::tempPath().toUtf8().constData(),
												  "myfile", 0, 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QByteArray sGeneratedEntryName;
	PRL_UINT32 nGeneratedEntryNameBufSize = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, 0, &nGeneratedEntryNameBufSize))
	QVERIFY(nGeneratedEntryNameBufSize != 0);
	sGeneratedEntryName.resize(nGeneratedEntryNameBufSize);
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, sGeneratedEntryName.data(), &nGeneratedEntryNameBufSize))

	QVERIFY(UTF8_2QSTR(sGeneratedEntryName).startsWith("myfile"));
}

void PrlSrvManipulationsTest::testFsGenerateEntryNameOnBothEmptyPrefixAndSuffix()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_FsGenerateEntryName(m_ServerHandle, QDir::tempPath().toUtf8().constData(),
												  "", "", 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QByteArray sGeneratedEntryName;
	PRL_UINT32 nGeneratedEntryNameBufSize = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, 0, &nGeneratedEntryNameBufSize))
	QVERIFY(nGeneratedEntryNameBufSize != 0);
	sGeneratedEntryName.resize(nGeneratedEntryNameBufSize);
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, sGeneratedEntryName.data(), &nGeneratedEntryNameBufSize))

	QVERIFY(UTF8_2QSTR(sGeneratedEntryName).startsWith("tmpfile"));
}

void PrlSrvManipulationsTest::testFsGenerateEntryNameOnBothNullPrefixAndSuffix()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_FsGenerateEntryName(m_ServerHandle, QDir::tempPath().toUtf8().constData(),
												  0, 0, 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QByteArray sGeneratedEntryName;
	PRL_UINT32 nGeneratedEntryNameBufSize = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, 0, &nGeneratedEntryNameBufSize))
	QVERIFY(nGeneratedEntryNameBufSize != 0);
	sGeneratedEntryName.resize(nGeneratedEntryNameBufSize);
	CHECK_RET_CODE_EXP(PrlResult_GetParamAsString(hResult, sGeneratedEntryName.data(), &nGeneratedEntryNameBufSize))

	QVERIFY(UTF8_2QSTR(sGeneratedEntryName).startsWith("tmpfile"));
}

void PrlSrvManipulationsTest::testRegisterVmOnPathToVmHomeDirSpecified()
{
	REGISTER_VM_PROLOG
	SdkHandleWrap hJob(PrlSrv_RegisterVm(m_ServerHandle, m_sTestFsDirName1ChildDir.toUtf8().constData(), PRL_TRUE));
	FIND_REGISTERED_VM_AT_SERVER_VMS_LIST
}

void PrlSrvManipulationsTest::testRegisterVmOnPathToVmConfigSpecified()
{
	REGISTER_VM_PROLOG
	SdkHandleWrap hJob(PrlSrv_RegisterVm(m_ServerHandle, sExpectedVmPath.toUtf8().constData(), PRL_TRUE));
	FIND_REGISTERED_VM_AT_SERVER_VMS_LIST
}

#define TEST_SERVER_INFO(test_port)\
	CHECK_JOB_RET_CODE(m_JobHandle)\
	SdkHandleWrap hResult;\
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()))\
	SdkHandleWrap hLoginResponse;\
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hLoginResponse.GetHandlePtr()))\
	CHECK_HANDLE_TYPE(hLoginResponse, PHT_LOGIN_RESPONSE)\
	QString sServerUuidFromLoginResponse, sServerHostOsVersionFromLoginResponse, sProductVersionFromLoginResponse;\
	PRL_EXTRACT_STRING_VALUE(sServerUuidFromLoginResponse, hLoginResponse, PrlLoginResponse_GetServerUuid)\
	PRL_EXTRACT_STRING_VALUE(sServerHostOsVersionFromLoginResponse, hLoginResponse, PrlLoginResponse_GetHostOsVersion)\
	PRL_EXTRACT_STRING_VALUE(sProductVersionFromLoginResponse, hLoginResponse, PrlLoginResponse_GetProductVersion)\
	SdkHandleWrap hServerInfo;\
	CHECK_RET_CODE_EXP(PrlSrv_GetServerInfo(m_ServerHandle, hServerInfo.GetHandlePtr()))\
	CHECK_HANDLE_TYPE(hServerInfo, PHT_SERVER_INFO)\
	QString sServerHostname, sServerUuidFromSrvInfo, sServerHostOsVersionFromSrvInfo, sProductVersionFromSrvInfo;\
	PRL_UINT32 nCmdPort = 0;\
	PRL_EXTRACT_STRING_VALUE(sServerHostname, hServerInfo, PrlSrvInfo_GetHostName)\
	PRL_EXTRACT_STRING_VALUE(sServerHostOsVersionFromSrvInfo, hServerInfo, PrlSrvInfo_GetOsVersion)\
	PRL_EXTRACT_STRING_VALUE(sServerUuidFromSrvInfo, hServerInfo, PrlSrvInfo_GetServerUuid)\
	PRL_EXTRACT_STRING_VALUE(sProductVersionFromSrvInfo, hServerInfo, PrlSrvInfo_GetProductVersion)\
	CHECK_RET_CODE_EXP(PrlSrvInfo_GetCmdPort(hServerInfo, &nCmdPort))\
	QCOMPARE(QString("127.0.0.1"), sServerHostname);\
	QCOMPARE(quint32(nCmdPort), quint32(test_port));\
	QCOMPARE(sServerUuidFromLoginResponse, sServerUuidFromSrvInfo);\
	QCOMPARE(sServerHostOsVersionFromLoginResponse, sServerHostOsVersionFromSrvInfo);\
	QCOMPARE(sProductVersionFromLoginResponse, sProductVersionFromSrvInfo);

void PrlSrvManipulationsTest::testGetServerInfoOnLoginLocalRequest()
{
	m_JobHandle.reset(PrlSrv_LoginLocal(m_ServerHandle, "", 0, PSL_HIGH_SECURITY));
	#ifdef _WIN_
		TEST_SERVER_INFO(PRL_DISPATCHER_LISTEN_PORT)
	#else
		TEST_SERVER_INFO(0)
	#endif
}

void PrlSrvManipulationsTest::testGetServerInfoOnLoginRequest()
{
	if (!TestConfig::isServerMode())
		QSKIP("Skipping test due functionality is not supported at desktop mode", SkipAll);

	m_JobHandle.reset(PrlSrv_Login(m_ServerHandle, TestConfig::getRemoteHostName(),	TestConfig::getUserLogin(),
								   TestConfig::getUserPassword(), NULL, 0, 0, PSL_HIGH_SECURITY));
	TEST_SERVER_INFO(PRL_DISPATCHER_LISTEN_PORT)
}

void PrlSrvManipulationsTest::testGetServerInfoOnNonConnectedServerObject()
{
	PRL_CHAR sBuffer[STR_BUF_LENGTH];
	PRL_UINT32 nBufferSize = 0;

	SdkHandleWrap hServerInfo;
	CHECK_RET_CODE_EXP(PrlSrv_GetServerInfo(m_ServerHandle, hServerInfo.GetHandlePtr()))

	CHECK_RET_CODE_EXP(PrlSrvInfo_GetServerUuid(hServerInfo, 0, &nBufferSize))
	QVERIFY(nBufferSize == 1);
	nBufferSize = 0;
	CHECK_RET_CODE_EXP(PrlSrvInfo_GetOsVersion(hServerInfo, 0, &nBufferSize))
	QVERIFY(nBufferSize == 1);
	nBufferSize = 0;
	CHECK_RET_CODE_EXP(PrlSrvInfo_GetHostName(hServerInfo, 0, &nBufferSize))
	QVERIFY(nBufferSize == 1);
	nBufferSize = 0;
	CHECK_RET_CODE_EXP(PrlSrvInfo_GetProductVersion(hServerInfo, 0, &nBufferSize))
	QVERIFY(nBufferSize == 1);

	PRL_UINT32 nCmdPort = 4237568;
	CHECK_RET_CODE_EXP(PrlSrvInfo_GetCmdPort(hServerInfo, &nCmdPort))
	QVERIFY(nCmdPort == 0);

	nBufferSize = sizeof(sBuffer);
	CHECK_RET_CODE_EXP(PrlSrvInfo_GetServerUuid(hServerInfo, sBuffer, &nBufferSize))
	QVERIFY(UTF8_2QSTR(sBuffer).isEmpty());
	nBufferSize = sizeof(sBuffer);
	CHECK_RET_CODE_EXP(PrlSrvInfo_GetOsVersion(hServerInfo, sBuffer, &nBufferSize))
	QVERIFY(UTF8_2QSTR(sBuffer).isEmpty());
	nBufferSize = sizeof(sBuffer);
	CHECK_RET_CODE_EXP(PrlSrvInfo_GetHostName(hServerInfo, sBuffer, &nBufferSize))
	QVERIFY(UTF8_2QSTR(sBuffer).isEmpty());
	nBufferSize = sizeof(sBuffer);
	CHECK_RET_CODE_EXP(PrlSrvInfo_GetProductVersion(hServerInfo, sBuffer, &nBufferSize))
	QVERIFY(UTF8_2QSTR(sBuffer).isEmpty());

	PRL_APPLICATION_MODE mode= PAM_SERVER;
	CHECK_RET_CODE_EXP(PrlSrvInfo_GetApplicationMode(hServerInfo, &mode))
	QVERIFY(mode == PAM_UNKNOWN);

	PRL_UINT64 nStartTime = 1;
	CHECK_RET_CODE_EXP(PrlSrvInfo_GetStartTime(hServerInfo, &nStartTime))
	QVERIFY(nStartTime == 0);
	PRL_UINT64 nStartTimeMonotonic = 1;
	CHECK_RET_CODE_EXP(PrlSrvInfo_GetStartTimeMonotonic(hServerInfo, &nStartTimeMonotonic))
	QVERIFY(nStartTimeMonotonic == 0);

}

void PrlSrvManipulationsTest::testGetServerInfoOnNonServerHandle()
{
	m_JobHandle.reset(PrlSrv_GetSrvConfig(m_ServerHandle));
	SdkHandleWrap hServerInfo;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrv_GetServerInfo(m_JobHandle, hServerInfo.GetHandlePtr()), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetServerInfoOnNullServerHandle()
{
	SdkHandleWrap hServerInfo;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrv_GetServerInfo(PRL_INVALID_HANDLE, hServerInfo.GetHandlePtr()), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetServerInfoOnNullResultBuffer()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrv_GetServerInfo(m_ServerHandle, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetServerUuidFromLoginResponseOnNonLoginResponseHandle()
{
	PRL_UINT32 nBufSize = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlLoginResponse_GetServerUuid(m_ServerHandle, 0, &nBufSize), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetServerUuidFromLoginResponseOnNullLoginResponseHandle()
{
	PRL_UINT32 nBufSize = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlLoginResponse_GetServerUuid(PRL_INVALID_HANDLE, 0, &nBufSize), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetServerUuidFromLoginResponseOnNullResultBuffer()
{
	m_JobHandle.reset(PrlSrv_LoginLocal(m_ServerHandle, "", 0, PSL_HIGH_SECURITY));
	CHECK_JOB_RET_CODE(m_JobHandle)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()))

	SdkHandleWrap hLoginResponse;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hLoginResponse.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hLoginResponse, PHT_LOGIN_RESPONSE)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlLoginResponse_GetServerUuid(hLoginResponse, 0, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetHostOsVersionFromLoginResponseOnNonLoginResponseHandle()
{
	PRL_UINT32 nBufSize = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlLoginResponse_GetHostOsVersion(m_ServerHandle, 0, &nBufSize), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetHostOsVersionFromLoginResponseOnNullLoginResponseHandle()
{
	PRL_UINT32 nBufSize = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlLoginResponse_GetHostOsVersion(PRL_INVALID_HANDLE, 0, &nBufSize), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetHostOsVersionFromLoginResponseOnNullResultBuffer()
{
	m_JobHandle.reset(PrlSrv_LoginLocal(m_ServerHandle, "", 0, PSL_HIGH_SECURITY));
	CHECK_JOB_RET_CODE(m_JobHandle)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()))

	SdkHandleWrap hLoginResponse;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hLoginResponse.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hLoginResponse, PHT_LOGIN_RESPONSE)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlLoginResponse_GetHostOsVersion(hLoginResponse, 0, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetProductVersionFromLoginResponseOnNonLoginResponseHandle()
{
	PRL_UINT32 nBufSize = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlLoginResponse_GetProductVersion(m_ServerHandle, 0, &nBufSize), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetProductVersionFromLoginResponseOnNullLoginResponseHandle()
{
	PRL_UINT32 nBufSize = 0;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlLoginResponse_GetProductVersion(PRL_INVALID_HANDLE, 0, &nBufSize), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetProductVersionFromLoginResponseOnNullResultBuffer()
{
	m_JobHandle.reset(PrlSrv_LoginLocal(m_ServerHandle, "", 0, PSL_HIGH_SECURITY));
	CHECK_JOB_RET_CODE(m_JobHandle)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()))

	SdkHandleWrap hLoginResponse;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hLoginResponse.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hLoginResponse, PHT_LOGIN_RESPONSE)

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlLoginResponse_GetProductVersion(hLoginResponse, 0, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testRegisterVmOnPathWithEmptyVmName()
{
	testLoginLocal();

// Prepare configs

	CVmConfiguration _vm_conf;
	QVERIFY( ((CBaseNode* )&_vm_conf)->loadFromFile("./TestDspCmdDirValidateVmConfig_vm_config.xml") == 0 );

	_vm_conf.getVmIdentification()->setVmName("");
	CVmCommonOptions* pCommonOptions = _vm_conf.getVmSettings()->getVmCommonOptions();
	pCommonOptions->setOsType(PVS_GUEST_TYPE_WINDOWS);
	pCommonOptions->setOsVersion(PVS_GUEST_VER_WIN_2003);

	int j = -1;
	QStringList lstDirs;
	lstDirs << m_sTestFsDirName1 << m_sTestFsDirName2 << m_sTestFsDirName3;

	for(j = 0; j < lstDirs.size(); ++j)
	{
		// Set unique VM uuid
		_vm_conf.getVmIdentification()->setVmUuid(Uuid::createUuid().toString());

		QVERIFY(QDir().mkdir(lstDirs[j]));
		QFile f(lstDirs[j] + "/config.pvs");
		QVERIFY(_vm_conf.saveToFile(&f) == 0);
	}

// Get all registered VM names

	SdkHandleWrap hJob(PrlSrv_GetVmList(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob);
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));

	SdkHandleWrap hVm;
	QStringList lstVmNames;

	PRL_UINT32 nParamsCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount));
	for (PRL_UINT32 i = 0; i < nParamsCount; ++i)
	{
		hVm.reset();
		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, i, hVm.GetHandlePtr()));

		QString qsVmName;
		PRL_EXTRACT_STRING_VALUE(qsVmName, hVm, PrlVmCfg_GetName);

		lstVmNames += qsVmName;
	}

// Register VM's with empty names

	QString qsOsVersionName = PVS_GUEST_TO_STRING(PVS_GUEST_VER_WIN_2003);
	lstVmNames += qsOsVersionName + " ()";
	for(j = 0; j < lstDirs.size(); ++j)
	{
		hVm.reset();
		hResult.reset();

		hJob.reset(PrlSrv_RegisterVm(m_ServerHandle, lstDirs[j].toUtf8().constData(), PRL_TRUE));
		CHECK_JOB_RET_CODE(hJob);
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
		CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVm.GetHandlePtr()));
		CHECK_HANDLE_TYPE(hVm, PHT_VIRTUAL_MACHINE);
		m_lstVmHandles += hVm;

		QString qsVmName;
		PRL_EXTRACT_STRING_VALUE(qsVmName, hVm, PrlVmCfg_GetName);

		QVERIFY(!qsVmName.isEmpty());
		QVERIFY(!lstVmNames.contains(qsVmName));
		QCOMPARE(qsVmName.mid(0, qsOsVersionName.size()), qsOsVersionName);
		QVERIFY(qsVmName.at(qsVmName.size() - 2) != '(');	// no name such as 'vm name ()'

		lstVmNames += qsVmName;
	}
}


#define COMPARE_SYNCH_VM_NAMES(qsVmName, vmUuid)\
	QString qsVmDirectoryList = ParallelsDirs::getDispatcherVmCatalogueFilePath();\
\
	CVmDirectories vmDirs;\
	QVERIFY(vmDirs.loadFromFile(qsVmDirectoryList) == 0);\
	CVmDirectoryItem* pVmDirItem = 0;\
\
	QList<CVmDirectory* >* pDirList = vmDirs.getVmDirectoriesList();\
	for(int i = 0; i < pDirList->size(); ++i)\
	{\
		CVmDirectory* pDir = pDirList->at(i);\
		QList<CVmDirectoryItem* >& lstDirItems = pDir->m_lstVmDirectoryItems;\
		for(int j = 0; j < lstDirItems.size(); ++j)\
		{\
			CVmDirectoryItem* pDirItem = lstDirItems[j];\
			if (pDirItem->getVmUuid() == vmUuid)\
			{\
				pVmDirItem = pDirItem;\
				goto DirItemFound;\
			}\
		}\
	}\
DirItemFound:\
	QVERIFY(pVmDirItem);\
	QCOMPARE(qsVmName, pVmDirItem->getVmName());

void PrlSrvManipulationsTest::testSynchVmNameOnVmRegisterWithEmptyVmName()
{
	testLoginLocal();

// Prepare config

	CVmConfiguration _vm_conf;
	QVERIFY( ((CBaseNode* )&_vm_conf)->loadFromFile("./TestDspCmdDirValidateVmConfig_vm_config.xml") == 0 );

	_vm_conf.getVmIdentification()->setVmName("");
	_vm_conf.getVmIdentification()->setVmUuid(Uuid::createUuid().toString());
	CVmCommonOptions* pCommonOptions = _vm_conf.getVmSettings()->getVmCommonOptions();
	pCommonOptions->setOsType(PVS_GUEST_TYPE_LINUX);
	pCommonOptions->setOsVersion(PVS_GUEST_VER_LIN_MANDRAKE);

	QVERIFY(QDir().mkdir(m_sTestFsDirName1));
	QFile f(m_sTestFsDirName1 + "/config.pvs");
	QVERIFY(_vm_conf.saveToFile(&f) == 0);

// Register VM with empty name

	SdkHandleWrap hJob(PrlSrv_RegisterVm(m_ServerHandle, m_sTestFsDirName1.toUtf8().constData(), PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()));
	CHECK_HANDLE_TYPE(m_VmHandle, PHT_VIRTUAL_MACHINE);

	QString qsVmName;
	PRL_EXTRACT_STRING_VALUE(qsVmName, m_VmHandle, PrlVmCfg_GetName);
	QString vmUuid;
	PRL_EXTRACT_STRING_VALUE(vmUuid, m_VmHandle, PrlVmCfg_GetUuid);

// Compare synch VM names

	COMPARE_SYNCH_VM_NAMES(qsVmName, vmUuid);
}

void PrlSrvManipulationsTest::testSynchVmNameOnVmRegister()
{
	testLoginLocal();

// Prepare config

	CVmConfiguration _vm_conf;
	QVERIFY( ((CBaseNode* )&_vm_conf)->loadFromFile("./TestDspCmdDirValidateVmConfig_vm_config.xml") == 0 );

	_vm_conf.getVmIdentification()->setVmName("My virtual machine");
	_vm_conf.getVmIdentification()->setVmUuid(Uuid::createUuid().toString());
	CVmCommonOptions* pCommonOptions = _vm_conf.getVmSettings()->getVmCommonOptions();
	pCommonOptions->setOsType(PVS_GUEST_TYPE_MACOS);
	pCommonOptions->setOsVersion(PVS_GUEST_VER_MACOS_LEOPARD);

// Create new VM and register it

	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVm_FromString(m_VmHandle, _vm_conf.toString().toUtf8().data()));

	SdkHandleWrap hJob(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)

	QString qsVmName;
	PRL_EXTRACT_STRING_VALUE(qsVmName, m_VmHandle, PrlVmCfg_GetName);
	QString vmUuid;
	PRL_EXTRACT_STRING_VALUE(vmUuid, m_VmHandle, PrlVmCfg_GetUuid);

// Compare synch VM names

	COMPARE_SYNCH_VM_NAMES(qsVmName, vmUuid);
}

void PrlSrvManipulationsTest::testSynchVmNameOnVmRegisterWithWrongUuids()
{
	testSynchVmNameOnVmRegisterWithEmptyVmName();

// Change uuid's

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	QString qsVmName;
	PRL_EXTRACT_STRING_VALUE(qsVmName, m_VmHandle, PrlVmCfg_GetName);
	QString qsVmHomePath;
	PRL_EXTRACT_STRING_VALUE(qsVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath);

	CVmConfiguration _vm_conf;
	QVERIFY( ((CBaseNode* )&_vm_conf)->loadFromFile(qsVmHomePath) == 0 );

	_vm_conf.getVmIdentification()->setVmUuid(Uuid::createUuid().toString());
	_vm_conf.getVmIdentification()->setServerUuid(Uuid::createUuid().toString());

	QFile f(qsVmHomePath);
	_vm_conf.saveToFile(&f);

	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	QString qsInvalidVmName;
	PRL_EXTRACT_STRING_VALUE(qsInvalidVmName, m_VmHandle, PrlVmCfg_GetName);

// Compare synch VM names

	QCOMPARE(qsVmName, qsInvalidVmName);
}

void PrlSrvManipulationsTest::testSynchVmNameOnEditVmName()
{
	testSynchVmNameOnVmRegister();

// Edit VM config

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, QSTR2UTF8(Uuid::createUuid().toString())));

	hJob.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	QString qsVmName;
	PRL_EXTRACT_STRING_VALUE(qsVmName, m_VmHandle, PrlVmCfg_GetName);
	QString vmUuid;
	PRL_EXTRACT_STRING_VALUE(vmUuid, m_VmHandle, PrlVmCfg_GetUuid);

// Compare synch VM names

	COMPARE_SYNCH_VM_NAMES(qsVmName, vmUuid);
}

void PrlSrvManipulationsTest::testSynchVmPathOnVmRegister()
{
	testLoginLocal();

// Prepare configs

	CVmConfiguration _vm_conf;
	QVERIFY( ((CBaseNode* )&_vm_conf)->loadFromFile("./TestDspCmdDirValidateVmConfig_vm_config.xml") == 0 );

	CVmCommonOptions* pCommonOptions = _vm_conf.getVmSettings()->getVmCommonOptions();
	pCommonOptions->setOsType(PVS_GUEST_TYPE_WINDOWS);
	pCommonOptions->setOsVersion(PVS_GUEST_VER_WIN_2003);

// Register VM's with the same path

	for(int i = 0; i < 3; i++)
	{
		_vm_conf.getVmIdentification()->setVmName("testSynchVmPathOnVmRegister " + QString::number(i));
		_vm_conf.getVmIdentification()->setVmUuid(Uuid::createUuid().toString());

		QVERIFY(QDir().mkdir(m_sTestFsDirName1));
		QFile f(m_sTestFsDirName1 + "/config.pvs");
		QVERIFY(_vm_conf.saveToFile(&f) == 0);

		SdkHandleWrap hVm;
		SdkHandleWrap hResult;

		SdkHandleWrap hJob(PrlSrv_RegisterVm(m_ServerHandle, QSTR2UTF8(m_sTestFsDirName1), PRL_TRUE));
		CHECK_JOB_RET_CODE(hJob);
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
		CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVm.GetHandlePtr()));
		CHECK_HANDLE_TYPE(hVm, PHT_VIRTUAL_MACHINE);
		m_lstVmHandles += hVm;
	}
}

void PrlSrvManipulationsTest::testSynchVmPathOnVmRegisterWithExistingPath()
{
	testLoginLocal();

	QVERIFY(QDir().mkdir(m_sTestFsDirName1 + ".pvm"));

	CVmConfiguration _vm_conf;
	QVERIFY( ((CBaseNode* )&_vm_conf)->loadFromFile("./TestDspCmdDirValidateVmConfig_vm_config.xml") == 0 );

	CVmCommonOptions* pCommonOptions = _vm_conf.getVmSettings()->getVmCommonOptions();
	pCommonOptions->setOsType(PVS_GUEST_TYPE_WINDOWS);
	pCommonOptions->setOsVersion(PVS_GUEST_VER_WIN_2003);

// Register VM's with the same path

	_vm_conf.getVmIdentification()->setVmName("virtual machine");
	_vm_conf.getVmIdentification()->setVmUuid(Uuid::createUuid().toString());

	QVERIFY(QDir().mkdir(m_sTestFsDirName1));
	QFile f(m_sTestFsDirName1 + "/config.pvs");
	QVERIFY(_vm_conf.saveToFile(&f) == 0);

	m_JobHandle.reset(PrlSrv_RegisterVm(m_ServerHandle, QSTR2UTF8(m_sTestFsDirName1), PRL_TRUE));
	CHECK_JOB_RET_CODE(m_JobHandle);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()));
	CHECK_HANDLE_TYPE(m_VmHandle, PHT_VIRTUAL_MACHINE);
}

void PrlSrvManipulationsTest::testGetInvalidVmName()
{
	testSynchVmNameOnVmRegisterWithEmptyVmName();

// Make VM invalid

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	QString qsVmName;
	PRL_EXTRACT_STRING_VALUE(qsVmName, m_VmHandle, PrlVmCfg_GetName);
	QString qsVmHomePath;
	PRL_EXTRACT_STRING_VALUE(qsVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath);

	QVERIFY(QFile::remove(qsVmHomePath));

// Get VM config with old name

	hJob.reset(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	QString qsInvalidVmName;
	PRL_EXTRACT_STRING_VALUE(qsInvalidVmName, m_VmHandle, PrlVmCfg_GetName);

	QCOMPARE(qsVmName, qsInvalidVmName);
}

void PrlSrvManipulationsTest::testSynchVmNameOnVmClone()
{
	testSynchVmNameOnVmRegister();

// Get VM home path

	SdkHandleWrap hJob(PrlVm_RefreshConfig(m_VmHandle));
	CHECK_JOB_RET_CODE(hJob)

	QString qsVmHome;
	PRL_EXTRACT_STRING_VALUE(qsVmHome, m_VmHandle, PrlVmCfg_GetHomePath);

	QFileInfo fi1(qsVmHome);
	QFileInfo fi2(fi1.path());
	QString qsVmRoot = fi2.path();

// Clone VM

	QString qsClonedVmName = "New machine";
	hJob.reset(PrlVm_Clone(m_VmHandle,
						   qsClonedVmName.toUtf8().constData(),
						   qsVmRoot.toUtf8().constData(),
						   PRL_FALSE));
	CHECK_JOB_RET_CODE(hJob);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVm.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hVm, PHT_VIRTUAL_MACHINE);
	m_lstVmHandles += hVm;

	QString qsVmName;
	PRL_EXTRACT_STRING_VALUE(qsVmName, hVm, PrlVmCfg_GetName);
	QString vmUuid;
	PRL_EXTRACT_STRING_VALUE(vmUuid, hVm, PrlVmCfg_GetUuid);

// Compare synch VM names

	COMPARE_SYNCH_VM_NAMES(qsVmName, vmUuid);
}

void PrlSrvManipulationsTest::testGetUserInfoList()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_GetUserInfoList(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	PRL_UINT32 nParamsCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))
	QVERIFY(nParamsCount > 0);

	for(PRL_UINT32 i = 0; i < nParamsCount; ++i)
	{
		SdkHandleWrap hUserInfo;
		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, i, hUserInfo.GetHandlePtr()));
		CHECK_HANDLE_TYPE(hUserInfo, PHT_USER_INFO);
	}
}

void PrlSrvManipulationsTest::testCurrentUserInfo()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_GetUserInfoList(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	PRL_UINT32 nParamsCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))
	QVERIFY(nParamsCount > 0);

	SdkHandleWrap hUserInfo;
	PRL_UINT32 nSessionCount = 0;
	for(PRL_UINT32 i = 0; i < nParamsCount; ++i)
	{
		hUserInfo.reset();
		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, i, hUserInfo.GetHandlePtr()));
		CHECK_HANDLE_TYPE(hUserInfo, PHT_USER_INFO);

		nSessionCount = 0;
		CHECK_RET_CODE_EXP(PrlUsrInfo_GetSessionCount(hUserInfo, &nSessionCount));
		if (nSessionCount > 0)
		{
			break;
		}
	}
	QVERIFY(nSessionCount > 0);

	HANDLE_TO_STRING(hUserInfo);
	UserInfo userInfo;
	userInfo.fromString(_str_object);
	QVERIFY(nSessionCount == (PRL_UINT32 )userInfo.getSessionInfoList()->m_lstSessionInfo.size());

	QString qsName;
	QString qsUuid;
	QString qsDefaultVmDir;
	PRL_BOOL bCanChangeServerSettings;

	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hUserInfo, PrlUsrInfo_GetName);
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hUserInfo, PrlUsrInfo_GetUuid);
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hUserInfo, PrlUsrInfo_GetDefaultVmFolder);

	PRL_EXTRACT_STRING_VALUE(qsName, hUserInfo, PrlUsrInfo_GetName);
	PRL_EXTRACT_STRING_VALUE(qsUuid, hUserInfo, PrlUsrInfo_GetUuid);
	PRL_EXTRACT_STRING_VALUE(qsDefaultVmDir, hUserInfo, PrlUsrInfo_GetDefaultVmFolder);
	CHECK_RET_CODE_EXP(PrlUsrInfo_CanChangeSrvSets(hUserInfo, &bCanChangeServerSettings));

	QVERIFY(!qsUuid.isEmpty());
	QVERIFY(!qsName.isEmpty());
	QVERIFY(QDir().exists(qsDefaultVmDir));
}

void PrlSrvManipulationsTest::testGetUserInfo()
{
	testLoginLocal();

// Get user info through user info list

	SdkHandleWrap hJob(PrlSrv_GetUserInfoList(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	PRL_UINT32 nParamsCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))
	QVERIFY(nParamsCount > 0);

	SdkHandleWrap hUserInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, hUserInfo.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hUserInfo, PHT_USER_INFO);

	QString qsUuid;
	PRL_EXTRACT_STRING_VALUE(qsUuid, hUserInfo, PrlUsrInfo_GetUuid);
	QVERIFY(!qsUuid.isEmpty());

// Directly get user info by user uuid

	hJob.reset(PrlSrv_GetUserInfo(m_ServerHandle, qsUuid.toUtf8().constData()));
	CHECK_JOB_RET_CODE(hJob)

	hResult.reset();
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	hUserInfo.reset();
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hUserInfo.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hUserInfo, PHT_USER_INFO);

	QString qsSameUuid;
	PRL_EXTRACT_STRING_VALUE(qsSameUuid, hUserInfo, PrlUsrInfo_GetUuid);

	QCOMPARE(qsUuid, qsSameUuid);
}

void PrlSrvManipulationsTest::testGetUserInfoOnWrongParams()
{
	testLoginLocal();

// Get valid user uuid

	SdkHandleWrap hJob(PrlSrv_GetUserInfoList(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	PRL_UINT32 nParamsCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))
	QVERIFY(nParamsCount > 0);

	SdkHandleWrap hUserInfo;
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, hUserInfo.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hUserInfo, PHT_USER_INFO);

	QString qsUuid;
	PRL_EXTRACT_STRING_VALUE(qsUuid, hUserInfo, PrlUsrInfo_GetUuid);
	QVERIFY(!qsUuid.isEmpty());

// Wrong uuid

	CHECK_ASYNC_OP_FAILED(PrlSrv_GetUserInfo(m_ServerHandle,
									Uuid::createUuid().toString().toUtf8().constData())
							, PRL_ERR_USER_NOT_FOUND);

// Wrong handle

	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	CHECK_ASYNC_OP_FAILED(PrlSrv_GetUserInfo(m_VmHandle, qsUuid.toUtf8().constData())
							, PRL_ERR_INVALID_ARG);

// Invalid handle

	SdkHandleWrap hInvalidHandle;
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetUserInfo(hInvalidHandle, qsUuid.toUtf8().constData())
							, PRL_ERR_INVALID_ARG);

// Null pointer

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlUsrInfo_GetSessionCount(hUserInfo, 0),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlUsrInfo_CanChangeSrvSets(hUserInfo, 0),
										PRL_ERR_INVALID_ARG);
}

namespace {
struct PrlJobTestData
{
	SdkHandleWrap m_JobHandle;
	SdkHandleWrap m_ServerHandle;
	QMutex m_Mutex;
	QWaitCondition m_Condition;
};

PRL_RESULT TestJobTypeUseCaseCallback(PRL_HANDLE _handle, PRL_VOID_PTR pUserData)
{
	SdkHandleWrap handle(_handle);
	PRL_HANDLE_TYPE nHandleType = PHT_ERROR;
	PRL_RESULT nRetCode = PrlHandle_GetType(handle, &nHandleType);
	if (PRL_SUCCEEDED(nRetCode))
	{
		if (nHandleType == PHT_JOB)
		{
			PrlJobTestData *pJobTestData = static_cast<PrlJobTestData *>(pUserData);
			QMutexLocker _lock(&pJobTestData->m_Mutex);
			//Unregister our callback now due it's not necessary now
			nRetCode = PrlSrv_UnregEventHandler(pJobTestData->m_ServerHandle, TestJobTypeUseCaseCallback, pUserData);
			if (PRL_FAILED(nRetCode))
				WRITE_TRACE(DBG_FATAL, "Couldn't to unregister callback due error: %.8X '%s'", nRetCode,\
							prl_result_to_string(nRetCode));
			//Store received job handle
			pJobTestData->m_JobHandle = handle;
			pJobTestData->m_Condition.wakeAll();
		}
	}
	else
		WRITE_TRACE(DBG_FATAL, "Failed to get handle type with error code: %.8X '%s'", nRetCode, prl_result_to_string(nRetCode));

	return (PRL_ERR_SUCCESS);
}

}

void PrlSrvManipulationsTest::testJobTypeUseCase()
{
	PrlJobTestData _job_test_data;
	_job_test_data.m_ServerHandle = m_ServerHandle;

	PRL_HANDLE nExpectedJobHandle = PRL_INVALID_HANDLE;
	{
		QMutexLocker _lock(&_job_test_data.m_Mutex);

		CHECK_RET_CODE_EXP(PrlSrv_RegEventHandler(m_ServerHandle, TestJobTypeUseCaseCallback, &_job_test_data))

		SdkHandleWrap hJob(PrlSrv_LoginLocal(m_ServerHandle, "", 0, PSL_HIGH_SECURITY));

		//Forget job handle now in order to test clear use case of job type info usage
		nExpectedJobHandle = hJob.GetHandle();
		hJob.reset();

		//Wait until response received through registered callback
		QVERIFY(_job_test_data.m_Condition.wait(&_job_test_data.m_Mutex/*, PRL_JOB_WAIT_TIMEOUT*/));
	}
	m_JobHandle = _job_test_data.m_JobHandle;
	//Check that we received the same job handle
	QVERIFY(nExpectedJobHandle == _job_test_data.m_JobHandle.GetHandle());

	PRL_JOB_OPERATION_CODE nJobOpCode = PJOC_UNKNOWN;
	CHECK_RET_CODE_EXP(PrlJob_GetOpCode(m_JobHandle, &nJobOpCode))

	QVERIFY(nJobOpCode == PJOC_SRV_LOGIN_LOCAL);
}

void PrlSrvManipulationsTest::testJobGetTypeOnInvalidHandle()
{
	PRL_JOB_OPERATION_CODE nJobOpCode = PJOC_UNKNOWN;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlJob_GetOpCode(PRL_INVALID_HANDLE, &nJobOpCode), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testJobGetTypeOnNonJobHandle()
{
	PRL_JOB_OPERATION_CODE nJobOpCode = PJOC_UNKNOWN;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlJob_GetOpCode(m_ServerHandle, &nJobOpCode), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testJobGetTypeOnNullPointer()
{
	m_JobHandle.reset(PrlSrv_LoginLocal(m_ServerHandle, "", 0, PSL_HIGH_SECURITY));
	CHECK_JOB_RET_CODE(m_JobHandle)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlJob_GetOpCode(PRL_INVALID_HANDLE, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testJobTypeUseCaseForAllAsyncMethods()
{
#define TEST_CHECK_JOB_OP_CODE(sdk_expr, expected_job_op_code)\
	{\
		SdkHandleWrap hJob(sdk_expr);\
		CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT))\
		PRL_JOB_OPERATION_CODE nActualJobOpCode = PJOC_UNKNOWN;\
		CHECK_RET_CODE_EXP(PrlJob_GetOpCode(hJob, &nActualJobOpCode))\
		if (nActualJobOpCode != expected_job_op_code)\
			WRITE_TRACE(DBG_FATAL, "Actual job type do not match: actual = %d expected = %d",\
							int(nActualJobOpCode), int(expected_job_op_code));\
		QVERIFY(nActualJobOpCode == expected_job_op_code);\
	}

	TEST_CHECK_JOB_OP_CODE(PrlJob_Cancel(PRL_INVALID_HANDLE), PJOC_JOB_CANCEL)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_Login(PRL_INVALID_HANDLE, 0, 0, 0, 0, 0, 0, PSL_HIGH_SECURITY), PJOC_SRV_LOGIN)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_LoginLocal(PRL_INVALID_HANDLE, 0, 0, PSL_HIGH_SECURITY), PJOC_SRV_LOGIN_LOCAL)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_Logoff(PRL_INVALID_HANDLE), PJOC_SRV_LOGOFF)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_SetNonInteractiveSession(PRL_INVALID_HANDLE, PRL_FALSE, 0),\
							PJOC_SRV_SET_NON_INTERACTIVE_SESSION)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_GetSrvConfig(PRL_INVALID_HANDLE), PJOC_SRV_GET_SRV_CONFIG)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_GetCommonPrefs(PRL_INVALID_HANDLE), PJOC_SRV_GET_COMMON_PREFS)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_CommonPrefsBeginEdit(PRL_INVALID_HANDLE), PJOC_SRV_COMMON_PREFS_BEGIN_EDIT)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_CommonPrefsCommit(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE), PJOC_SRV_COMMON_PREFS_COMMIT)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_GetUserProfile(PRL_INVALID_HANDLE), PJOC_SRV_GET_USER_PROFILE)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_GetStatistics(PRL_INVALID_HANDLE), PJOC_SRV_GET_STATISTICS)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_UserProfileBeginEdit(PRL_INVALID_HANDLE), PJOC_SRV_USER_PROFILE_BEGIN_EDIT)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_UserProfileCommit(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE), PJOC_SRV_USER_PROFILE_COMMIT)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_RegisterVm(PRL_INVALID_HANDLE, 0, PRL_FALSE), PJOC_SRV_REGISTER_VM)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_Register3rdPartyVm(PRL_INVALID_HANDLE, 0, 0, 0), PJOC_SRV_REGISTER_3RD_PARTY_VM)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_RegisterVmEx(PRL_INVALID_HANDLE, 0, 0), PJOC_SRV_REGISTER_VM_EX)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_RegisterVmWithUuid(PRL_INVALID_HANDLE, 0, 0, 0), PJOC_SRV_REGISTER_VM)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_GetVmList(PRL_INVALID_HANDLE), PJOC_SRV_GET_VM_LIST)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_SubscribeToHostStatistics(PRL_INVALID_HANDLE), PJOC_SRV_SUBSCRIBE_TO_HOST_STATISTICS)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_UnsubscribeFromHostStatistics(PRL_INVALID_HANDLE),\
							PJOC_SRV_UNSUBSCRIBE_FROM_HOST_STATISTICS)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_Shutdown(PRL_INVALID_HANDLE, PRL_FALSE), PJOC_SRV_SHUTDOWN)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_ShutdownEx(PRL_INVALID_HANDLE, 0), PJOC_SRV_SHUTDOWN)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_FsGetDiskList(PRL_INVALID_HANDLE), PJOC_SRV_FS_GET_DISK_LIST)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_FsGetDirEntries(PRL_INVALID_HANDLE, 0), PJOC_SRV_FS_GET_DIR_ENTRIES)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_FsCreateDir(PRL_INVALID_HANDLE, 0), PJOC_SRV_FS_CREATE_DIR)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_FsRemoveEntry(PRL_INVALID_HANDLE, 0), PJOC_SRV_FS_REMOVE_ENTRY)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_FsCanCreateFile(PRL_INVALID_HANDLE, 0), PJOC_SRV_FS_CAN_CREATE_FILE)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_FsRenameEntry(PRL_INVALID_HANDLE, 0, 0), PJOC_SRV_FS_RENAME_ENTRY)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_FsGenerateEntryName(PRL_INVALID_HANDLE, 0, 0, 0, 0), PJOC_SRV_FS_GENERATE_ENTRY_NAME)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_UpdateLicense(PRL_INVALID_HANDLE, 0, 0, 0), PJOC_SRV_UPDATE_LICENSE)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_GetLicenseInfo(PRL_INVALID_HANDLE), PJOC_SRV_GET_LICENSE_INFO)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_SendAnswer(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE), PJOC_SRV_SEND_ANSWER)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_StartSearchVms(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE), PJOC_SRV_START_SEARCH_VMS)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_NetServiceStart(PRL_INVALID_HANDLE), PJOC_SRV_NET_SERVICE_START)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_NetServiceStop(PRL_INVALID_HANDLE), PJOC_SRV_NET_SERVICE_STOP)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_NetServiceRestart(PRL_INVALID_HANDLE), PJOC_SRV_NET_SERVICE_RESTART)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_NetServiceRestoreDefaults(PRL_INVALID_HANDLE), PJOC_SRV_NET_SERVICE_RESTORE_DEFAULTS)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_GetNetServiceStatus(PRL_INVALID_HANDLE), PJOC_SRV_GET_NET_SERVICE_STATUS)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_AddNetAdapter(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE), PJOC_SRV_ADD_NET_ADAPTER)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_DeleteNetAdapter(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE), PJOC_SRV_DELETE_NET_ADAPTER)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_UpdateNetAdapter(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE), PJOC_SRV_UPDATE_NET_ADAPTER)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_GetProblemReport(PRL_INVALID_HANDLE), PJOC_SRV_GET_PROBLEM_REPORT)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_AttachToLostTask(PRL_INVALID_HANDLE, 0), PJOC_SRV_ATTACH_TO_LOST_TASK)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_GetUserInfoList(PRL_INVALID_HANDLE), PJOC_SRV_GET_USER_INFO_LIST)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_GetUserInfo(PRL_INVALID_HANDLE, 0), PJOC_SRV_GET_USER_INFO)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_PrepareForHibernate(PRL_INVALID_HANDLE, 0), PJOC_SRV_PREPARE_FOR_HIBERNATE)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_AfterHostResume(PRL_INVALID_HANDLE, 0), PJOC_SRV_AFTER_HOST_RESUME)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_GetVirtualNetworkList(PRL_INVALID_HANDLE, 0), PJOC_SRV_GET_VIRTUAL_NETWORK_LIST)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_AddVirtualNetwork(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, 0),\
													PJOC_SRV_ADD_VIRTUAL_NETWORK)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_UpdateVirtualNetwork(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, 0),\
													PJOC_SRV_UPDATE_VIRTUAL_NETWORK)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_DeleteVirtualNetwork(PRL_INVALID_HANDLE, 0, 0),\
													PJOC_SRV_DELETE_VIRTUAL_NETWORK)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_ConfigureGenericPci(PRL_INVALID_HANDLE, 0, 0),\
													PJOC_SRV_CONFIGURE_GENERIC_PCI)
	TEST_CHECK_JOB_OP_CODE(PrlVm_Start(PRL_INVALID_HANDLE), PJOC_VM_START)
	TEST_CHECK_JOB_OP_CODE(PrlVm_Stop(PRL_INVALID_HANDLE, PRL_FALSE), PJOC_VM_STOP)
	TEST_CHECK_JOB_OP_CODE(PrlVm_Pause(PRL_INVALID_HANDLE, PRL_FALSE), PJOC_VM_PAUSE)
	TEST_CHECK_JOB_OP_CODE(PrlVm_Reset(PRL_INVALID_HANDLE), PJOC_VM_RESET)
	TEST_CHECK_JOB_OP_CODE(PrlVm_Suspend(PRL_INVALID_HANDLE), PJOC_VM_SUSPEND)
	TEST_CHECK_JOB_OP_CODE(PrlVm_GetSuspendedScreen(PRL_INVALID_HANDLE),  PJOC_VM_GET_SUSPENDED_SCREEN)
	TEST_CHECK_JOB_OP_CODE(PrlVm_Resume(PRL_INVALID_HANDLE), PJOC_VM_RESUME)
	TEST_CHECK_JOB_OP_CODE(PrlVm_DropSuspendedState(PRL_INVALID_HANDLE), PJOC_VM_DROP_SUSPENDED_STATE)
	TEST_CHECK_JOB_OP_CODE(PrlVm_Clone(PRL_INVALID_HANDLE, 0, 0, PRL_FALSE), PJOC_VM_CLONE)
	TEST_CHECK_JOB_OP_CODE(PrlVm_Migrate(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, 0, PVMT_HOT_MIGRATION, 0, PRL_FALSE),\
								PJOC_VM_MIGRATE)
	TEST_CHECK_JOB_OP_CODE(PrlVm_MigrateEx(PRL_INVALID_HANDLE, 0, 0, 0, 0, PVMT_HOT_MIGRATION, 0, PRL_FALSE),\
								PJOC_VM_MIGRATE)
	TEST_CHECK_JOB_OP_CODE(PrlVm_MigrateWithRename(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, 0, 0, PVMT_HOT_MIGRATION, 0, PRL_FALSE),\
								PJOC_VM_MIGRATE)
	TEST_CHECK_JOB_OP_CODE(PrlVm_MigrateWithRenameEx(PRL_INVALID_HANDLE, 0, 0, 0, 0, 0, PVMT_HOT_MIGRATION, 0, PRL_FALSE),\
								PJOC_VM_MIGRATE)
	TEST_CHECK_JOB_OP_CODE(PrlVm_Delete(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE), PJOC_VM_DELETE)
	TEST_CHECK_JOB_OP_CODE(PrlVm_GetProblemReport(PRL_INVALID_HANDLE), PJOC_VM_GET_PROBLEM_REPORT)
	TEST_CHECK_JOB_OP_CODE(PrlVm_GetState(PRL_INVALID_HANDLE), PJOC_VM_GET_STATE)
	TEST_CHECK_JOB_OP_CODE(PrlVm_GenerateVmDevFilename(PRL_INVALID_HANDLE, 0, 0, 0), PJOC_VM_GENERATE_VM_DEV_FILENAME)
	TEST_CHECK_JOB_OP_CODE(PrlVm_GetToolsState(PRL_INVALID_HANDLE), PJOC_VM_GET_TOOLS_STATE)
	TEST_CHECK_JOB_OP_CODE(PrlVm_RefreshConfig(PRL_INVALID_HANDLE), PJOC_VM_REFRESH_CONFIG)
	TEST_CHECK_JOB_OP_CODE(PrlVm_GetStatistics(PRL_INVALID_HANDLE), PJOC_VM_GET_STATISTICS)
	TEST_CHECK_JOB_OP_CODE(PrlVm_SubscribeToGuestStatistics(PRL_INVALID_HANDLE), PJOC_VM_SUBSCRIBE_TO_GUEST_STATISTICS)
	TEST_CHECK_JOB_OP_CODE(PrlVm_UnsubscribeFromGuestStatistics(PRL_INVALID_HANDLE),\
								PJOC_VM_UNSUBSCRIBE_FROM_GUEST_STATISTICS)
	TEST_CHECK_JOB_OP_CODE(PrlVm_Reg(PRL_INVALID_HANDLE, 0, PRL_FALSE), PJOC_VM_REG)
	TEST_CHECK_JOB_OP_CODE(PrlVm_RegEx(PRL_INVALID_HANDLE, 0, 0), PJOC_VM_REG)
	TEST_CHECK_JOB_OP_CODE(PrlVm_Unreg(PRL_INVALID_HANDLE), PJOC_VM_UNREG)
	TEST_CHECK_JOB_OP_CODE(PrlVm_Restore(PRL_INVALID_HANDLE), PJOC_VM_RESTORE)
	TEST_CHECK_JOB_OP_CODE(PrlVm_BeginEdit(PRL_INVALID_HANDLE), PJOC_VM_BEGIN_EDIT)
	TEST_CHECK_JOB_OP_CODE(PrlVm_Commit(PRL_INVALID_HANDLE), PJOC_VM_COMMIT)
	TEST_CHECK_JOB_OP_CODE(PrlVm_CreateUnattendedFloppy(PRL_INVALID_HANDLE, PGD_WINDOWS_XP, 0, 0, 0),\
							PJOC_VM_CREATE_UNATTENDED_FLOPPY)
	TEST_CHECK_JOB_OP_CODE(PrlVm_InitiateDevStateNotifications(PRL_INVALID_HANDLE),\
							PJOC_VM_INITIATE_DEV_STATE_NOTIFICATIONS)
	TEST_CHECK_JOB_OP_CODE(PrlVm_UpdateSecurity(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE), PJOC_VM_UPDATE_SECURITY)
	TEST_CHECK_JOB_OP_CODE(PrlVm_ValidateConfig(PRL_INVALID_HANDLE, PVC_ALL), PJOC_VM_VALIDATE_CONFIG)
	TEST_CHECK_JOB_OP_CODE(PrlVmDev_Connect(PRL_INVALID_HANDLE), PJOC_VM_DEV_CONNECT)
	TEST_CHECK_JOB_OP_CODE(PrlVmDev_Disconnect(PRL_INVALID_HANDLE), PJOC_VM_DEV_DISCONNECT)
	TEST_CHECK_JOB_OP_CODE(PrlVmDev_CreateImage(PRL_INVALID_HANDLE, PRL_FALSE, PRL_FALSE), PJOC_VM_DEV_CREATE_IMAGE)
	TEST_CHECK_JOB_OP_CODE(PrlVmDev_CopyImage(PRL_INVALID_HANDLE, NULL, NULL, 0), PJOC_VM_DEV_COPY_IMAGE)
	TEST_CHECK_JOB_OP_CODE(PrlVm_CreateSnapshot(PRL_INVALID_HANDLE, 0, 0), PJOC_VM_CREATE_SNAPSHOT)
	TEST_CHECK_JOB_OP_CODE(PrlVm_SwitchToSnapshot(PRL_INVALID_HANDLE, 0), PJOC_VM_SWITCH_TO_SNAPSHOT)
	TEST_CHECK_JOB_OP_CODE(PrlVm_DeleteSnapshot(PRL_INVALID_HANDLE, 0, PRL_FALSE), PJOC_VM_DELETE_SNAPSHOT)
	TEST_CHECK_JOB_OP_CODE(PrlVm_GetSnapshotsTree(PRL_INVALID_HANDLE), PJOC_VM_GET_SNAPSHOTS_TREE)
	TEST_CHECK_JOB_OP_CODE(PrlVm_UpdateSnapshotData(PRL_INVALID_HANDLE, 0, 0, 0), PJOC_VM_UPDATE_SNAPSHOT_DATA)
	TEST_CHECK_JOB_OP_CODE(PrlVm_RunCompressor(PRL_INVALID_HANDLE), PJOC_VM_RUN_COMPRESSOR)
	TEST_CHECK_JOB_OP_CODE(PrlVm_CancelCompressor(PRL_INVALID_HANDLE), PJOC_VM_CANCEL_COMPRESSOR)
	TEST_CHECK_JOB_OP_CODE(PrlVm_StartEx(PRL_INVALID_HANDLE, 0, 0), PJOC_VM_START_EX)
	TEST_CHECK_JOB_OP_CODE(PrlVm_Restart(PRL_INVALID_HANDLE), PJOC_VM_RESTART)
	TEST_CHECK_JOB_OP_CODE(PrlVm_StartVncServer(PRL_INVALID_HANDLE, 0), PJOC_VM_START_VNC_SERVER)
	TEST_CHECK_JOB_OP_CODE(PrlVm_StopVncServer(PRL_INVALID_HANDLE, 0), PJOC_VM_STOP_VNC_SERVER)
	TEST_CHECK_JOB_OP_CODE(PrlVmGuest_GetNetworkSettings(PRL_INVALID_HANDLE, 0), PJOC_VM_GUEST_GET_NETWORK_SETTINGS)
	TEST_CHECK_JOB_OP_CODE(PrlVm_LoginInGuest(PRL_INVALID_HANDLE, 0, 0, 0), PJOC_VM_LOGIN_IN_GUEST)
	TEST_CHECK_JOB_OP_CODE(PrlVmGuest_RunProgram(PRL_INVALID_HANDLE, 0, PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, 0,\
							PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR, PRL_INVALID_FILE_DESCRIPTOR),\
							PJOC_VM_GUEST_RUN_PROGRAM)
	TEST_CHECK_JOB_OP_CODE(PrlVmGuest_Logout(PRL_INVALID_HANDLE, 0), PJOC_VM_GUEST_LOGOUT)
	TEST_CHECK_JOB_OP_CODE(PrlVm_AuthWithGuestSecurityDb(PRL_INVALID_HANDLE, 0, 0, 0), PJOC_VM_AUTH_WITH_GUEST_SECURITY_DB)
	TEST_CHECK_JOB_OP_CODE(PrlVmGuest_SetUserPasswd(PRL_INVALID_HANDLE, 0, 0, 0), PJOC_VM_GUEST_SET_USER_PASSWD)
	TEST_CHECK_JOB_OP_CODE(PrlVm_Lock(PRL_INVALID_HANDLE, 0), PJOC_VM_LOCK)
	TEST_CHECK_JOB_OP_CODE(PrlVm_Unlock(PRL_INVALID_HANDLE, 0), PJOC_VM_UNLOCK)
	TEST_CHECK_JOB_OP_CODE(PrlApi_SendProblemReport(0, PRL_FALSE, 0, 0, 0, 0, 0, 0, 0, 0), PJOC_API_SEND_PROBLEM_REPORT)
	TEST_CHECK_JOB_OP_CODE(PrlApi_SendPackedProblemReport(PRL_INVALID_HANDLE, PRL_FALSE, 0, 0, 0, 0, 0, 0, 0, 0), PJOC_API_SEND_PACKED_PROBLEM_REPORT)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_EnableConfirmationMode(PRL_INVALID_HANDLE, 0), PJOC_SRV_SET_SESSION_CONFIRMATION_MODE)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_DisableConfirmationMode(PRL_INVALID_HANDLE, "", "", 0),\
				PJOC_SRV_SET_SESSION_CONFIRMATION_MODE);
	TEST_CHECK_JOB_OP_CODE(PrlSrv_StoreValueByKey(PRL_INVALID_HANDLE, "", "", 0), PJOC_SRV_STORE_VALUE_BY_KEY);
	TEST_CHECK_JOB_OP_CODE(PrlVm_StoreValueByKey(PRL_INVALID_HANDLE, "", "", 0), PJOC_VM_STORE_VALUE_BY_KEY);
	TEST_CHECK_JOB_OP_CODE(PrlVm_CancelCompact(PRL_INVALID_HANDLE), PJOC_VM_CANCEL_COMPACT);
	TEST_CHECK_JOB_OP_CODE(PrlVm_Compact(PRL_INVALID_HANDLE, 0, 0), PJOC_VM_COMPACT);
	TEST_CHECK_JOB_OP_CODE(PrlVm_ConvertDisks(PRL_INVALID_HANDLE, 0, 0), PJOC_VM_CONVERT_DISKS);
	TEST_CHECK_JOB_OP_CODE(PrlVm_ChangeSid(PRL_INVALID_HANDLE, 0), PJOC_VM_CHANGE_SID);
	TEST_CHECK_JOB_OP_CODE(PrlVm_ResetUptime(PRL_INVALID_HANDLE, 0), PJOC_VM_RESET_UPTIME);
	TEST_CHECK_JOB_OP_CODE(PrlReport_Assembly(PRL_INVALID_HANDLE, 0), PJOC_REPORT_ASSEMBLY);
	TEST_CHECK_JOB_OP_CODE(PrlReport_Send(PRL_INVALID_HANDLE, PRL_FALSE, 0, 0, 0, 0, 0, 0, 0, 0), PJOC_API_SEND_PROBLEM_REPORT)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_InstallAppliance(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, "", 0),\
							PJOC_SRV_INSTALL_APPLIANCE)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_AddIPPrivateNetwork(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, 0), \
							PJOC_SRV_ADD_IPPRIVATE_NETWORK)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_UpdateIPPrivateNetwork(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, 0), \
							PJOC_SRV_UPDATE_IPPRIVATE_NETWORK)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_RemoveIPPrivateNetwork(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE, 0), \
							PJOC_SRV_REMOVE_IPPRIVATE_NETWORK)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_GetIPPrivateNetworksList(PRL_INVALID_HANDLE, 0), \
							PJOC_SRV_GET_IPPRIVATE_NETWORKS_LIST)
	TEST_CHECK_JOB_OP_CODE(PrlSrv_RefreshPlugins(PRL_INVALID_HANDLE, 0), PJOC_SRV_REFRESH_PLUGINS);
	TEST_CHECK_JOB_OP_CODE(PrlSrv_GetPluginsList(PRL_INVALID_HANDLE, 0, 0), PJOC_SRV_GET_PLUGINS_LIST);

#undef TEST_CHECK_JOB_OP_CODE
}

void PrlSrvManipulationsTest::testCommonPrefsOnDefaultChangeSettings()
{
	testLoginLocal();

	m_JobHandle.reset(PrlSrv_GetCommonPrefs(m_ServerHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)
	m_ResultHandle.reset();
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	SdkHandleWrap hDispConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(m_ResultHandle, hDispConfig.GetHandlePtr()))

// Set default change settings

	CHECK_RET_CODE_EXP(PrlDispCfg_SetCanChangeDefaultSettings(hDispConfig, TRUE));

// Is default change settings

	PRL_BOOL bDefaultChange = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlDispCfg_CanChangeDefaultSettings(hDispConfig, &bDefaultChange));

	QVERIFY(bDefaultChange == PRL_TRUE);

// Set default change settings

	CHECK_RET_CODE_EXP(PrlDispCfg_SetCanChangeDefaultSettings(hDispConfig, PRL_FALSE));

// Is default change settings

	bDefaultChange = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlDispCfg_CanChangeDefaultSettings(hDispConfig, &bDefaultChange));

	QVERIFY(bDefaultChange == PRL_FALSE);
}

void PrlSrvManipulationsTest::testCommonPrefsOnDefaultChangeSettingsWrongParams()
{
	testLoginLocal();

	m_JobHandle.reset(PrlSrv_GetCommonPrefs(m_ServerHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)
	m_ResultHandle.reset();
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	SdkHandleWrap hDispConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(m_ResultHandle, hDispConfig.GetHandlePtr()))

	PRL_BOOL bDefaultChange = PRL_FALSE;

// Wrong handle

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_SetCanChangeDefaultSettings(m_JobHandle, PRL_TRUE),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_CanChangeDefaultSettings(m_JobHandle, &bDefaultChange),
										PRL_ERR_INVALID_ARG);

// Invalid handle

	SdkHandleWrap hInvalidHandle;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_SetCanChangeDefaultSettings(hInvalidHandle, PRL_TRUE),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_CanChangeDefaultSettings(hInvalidHandle, &bDefaultChange),
										PRL_ERR_INVALID_ARG);

// Null pointer

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_CanChangeDefaultSettings(hDispConfig, 0),
										PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testCommonPrefsOnMinimalSecurityLevel()
{
	testLoginLocal();

	m_JobHandle.reset(PrlSrv_GetCommonPrefs(m_ServerHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)
	m_ResultHandle.reset();
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	SdkHandleWrap hDispConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(m_ResultHandle, hDispConfig.GetHandlePtr()))

// Set minimal security level

	CHECK_RET_CODE_EXP(PrlDispCfg_SetMinSecurityLevel(hDispConfig, PSL_NORMAL_SECURITY));

// Get minimal security level

	PRL_SECURITY_LEVEL nMinSecurityLevel = PSL_LOW_SECURITY;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetMinSecurityLevel(hDispConfig, &nMinSecurityLevel));

	QVERIFY(nMinSecurityLevel == PSL_NORMAL_SECURITY);

// Set minimal security level

	CHECK_RET_CODE_EXP(PrlDispCfg_SetMinSecurityLevel(hDispConfig, PSL_HIGH_SECURITY));

// Get minimal security level

	nMinSecurityLevel = PSL_NORMAL_SECURITY;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetMinSecurityLevel(hDispConfig, &nMinSecurityLevel));

	QVERIFY(nMinSecurityLevel == PSL_HIGH_SECURITY);
}

void PrlSrvManipulationsTest::testCommonPrefsOnMinimalSecurityLevelWrongParams()
{
	testLoginLocal();

	m_JobHandle.reset(PrlSrv_GetCommonPrefs(m_ServerHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)
	m_ResultHandle.reset();
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	SdkHandleWrap hDispConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(m_ResultHandle, hDispConfig.GetHandlePtr()))

	PRL_SECURITY_LEVEL nMinSecurityLevel = PSL_LOW_SECURITY;

// Wrong handle

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_SetMinSecurityLevel(m_JobHandle, PSL_HIGH_SECURITY),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetMinSecurityLevel(m_JobHandle, &nMinSecurityLevel),
										PRL_ERR_INVALID_ARG);

// Invalid handle

	SdkHandleWrap hInvalidHandle;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_SetMinSecurityLevel(hInvalidHandle, PSL_NORMAL_SECURITY),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetMinSecurityLevel(hInvalidHandle, &nMinSecurityLevel),
										PRL_ERR_INVALID_ARG);

// Null pointer

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetMinSecurityLevel(hDispConfig, 0),
										PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testGetQuestionsList()
{
	testLoginLocal();

	SdkHandleWrap hQuestionsList;
	CHECK_RET_CODE_EXP(PrlSrv_GetQuestions(m_ServerHandle, hQuestionsList.GetHandlePtr()))
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

void PrlSrvManipulationsTest::testGetQuestionsListOnInvalidHandle()
{
	SdkHandleWrap hQuestionsList;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrv_GetQuestions(PRL_INVALID_HANDLE, hQuestionsList.GetHandlePtr()),\
										PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetQuestionsListOnWrongTypeHandle()
{
	SdkHandleWrap hQuestionsList1, hQuestionsList2;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hQuestionsList1.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrv_GetQuestions(hQuestionsList1, hQuestionsList2.GetHandlePtr()),\
										PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetQuestionsListOnNullResultBuffer()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrv_GetQuestions(m_ServerHandle, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testSubscribeToPerfStats()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_GetPerfStats(m_ServerHandle, ""));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hEvent;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hEvent.GetHandlePtr()))
    QVERIFY(hEvent.valid());

	PRL_UINT32 nCountersCount = 0;
	CHECK_RET_CODE_EXP(PrlEvent_GetParamsCount(hEvent, &nCountersCount))
	if (0 == nCountersCount)
		QSKIP("No public performance counters. skipping test.", SkipAll);

	SdkHandleWrap hSrvInfo ;
    CHECK_RET_CODE_EXP(PrlSrv_GetServerInfo(m_ServerHandle, hSrvInfo.GetHandlePtr())) ;
    QVERIFY(hSrvInfo.valid()) ;

    char server_uuid[1024] ;
    unsigned int buff_len = sizeof(server_uuid) ;
    CHECK_RET_CODE_EXP(PrlSrvInfo_GetServerUuid(hSrvInfo, server_uuid, &buff_len)) ;

	DEFINE_CHECK_CALLBACK(check_callback, m_ServerHandle, PET_DSP_EVT_PERFSTATS, PrlSrv_UnregEventHandler, PJOC_UNKNOWN) ;

	CHECK_RET_CODE_EXP(PrlSrv_RegEventHandler(m_ServerHandle, event_callback, &check_callback)) ;

	bool bRes = false;
	{
		QMutexLocker _lock(&check_callback.mutex);
		hJob.reset(PrlSrv_SubscribeToPerfStats(m_ServerHandle, ""));
		bRes = check_callback.condition.wait(&check_callback.mutex, PRL_JOB_WAIT_TIMEOUT) ;
	}

    CHECK_JOB_RET_CODE(hJob) ;
    hJob.reset(PrlSrv_UnsubscribeFromPerfStats(m_ServerHandle));
    CHECK_JOB_RET_CODE(hJob) ;
    QVERIFY(bRes) ;

	QMutexLocker _lock(&check_callback.mutex) ;

    QVERIFY(check_callback.got_event.valid()) ;
	QVERIFY(check_callback.u_result > 0);

	QCOMPARE(check_callback.s_result, QString(server_uuid));
}

void PrlSrvManipulationsTest::testGetPerfStats()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_GetPerfStats(m_ServerHandle, ""));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hEvent;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hEvent.GetHandlePtr()))
    QVERIFY(hEvent.valid()) ;

    PRL_EVENT_TYPE event_type ;
    CHECK_RET_CODE_EXP( PrlEvent_GetType(hEvent, &event_type) ) ;
    QCOMPARE(event_type, PET_DSP_EVT_PERFSTATS) ;
}

void PrlSrvManipulationsTest::testCheckSourceVmUuidOnRegisterVmWithEmptySourceUuid()
{
	REGISTER_VM_PROLOG
	QString sOriginalVmUuid = _vm_conf.getVmIdentification()->getVmUuid();
	QVERIFY(_vm_conf.getVmIdentification()->getSourceVmUuid().isEmpty());
	SdkHandleWrap hJob(
		PrlSrv_RegisterVm(m_ServerHandle, m_sTestFsDirName1ChildDir.toUtf8().constData(), PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()))

	PRL_VOID_PTR pVmConfData = 0;
	CHECK_RET_CODE_EXP(PrlVm_ToString(m_VmHandle, &pVmConfData))
	_config = UTF8_2QSTR((const char *)pVmConfData);
	PrlBuffer_Free(pVmConfData);
	_vm_conf.fromString(_config);
	QVERIFY(PRL_SUCCEEDED(_vm_conf.m_uiRcInit));

	QCOMPARE(sOriginalVmUuid, _vm_conf.getVmIdentification()->getSourceVmUuid());
}

void PrlSrvManipulationsTest::testRegisterOldVmFullPathToVmConfigSpecified()
{
	testLoginLocal();
	QVERIFY(QDir().mkdir(m_sTestFsDirName1));
	QVERIFY(QDir().mkdir(m_sTestFsDirName1ChildDir));
	QString sVmConfigPath = m_sTestFsDirName1ChildDir + "/CConfigConverterTest_pw25.pvs";
	QVERIFY(QFile::copy("./CConfigConverterTest_pw25.pvs", sVmConfigPath));
	SdkHandleWrap hJob(PrlSrv_RegisterVm(m_ServerHandle, sVmConfigPath.toUtf8().constData(), PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()))
	CHECK_HANDLE_TYPE(m_VmHandle, PHT_VIRTUAL_MACHINE)
}

void PrlSrvManipulationsTest::testRegisterOldVmToCheckSomeNetworkMacAddress()
{
	testLoginLocal();
	QVERIFY(QDir().mkdir(m_sTestFsDirName1));
	QVERIFY(QDir().mkdir(m_sTestFsDirName1ChildDir));
	QString sVmConfigPath = m_sTestFsDirName1ChildDir + "/SDKTest_OldVm_WithNetwork.pvs";
	QVERIFY(QFile::copy("./SDKTest_OldVm_WithNetwork.pvs", sVmConfigPath));

	QString oldMAC = HostUtils::generateMacAddress().toLower() ;

	ConfigFile _old_config(sVmConfigPath);
	QVERIFY(_old_config.IsValid());
	_old_config.setParamValue(NETWORK_SECTION, NET_MACADDR, oldMAC );
	QVERIFY(_old_config.saveConfig());

	QVERIFY( !oldMAC.isEmpty() );

	SdkHandleWrap hJob(PrlSrv_RegisterVm(m_ServerHandle, sVmConfigPath.toUtf8().constData(), PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()))
	CHECK_HANDLE_TYPE(m_VmHandle, PHT_VIRTUAL_MACHINE)

	// get MacAddress
	PRL_UINT32 netCount = 0;
	CHECK_RET_CODE_EXP( PrlVmCfg_GetNetAdaptersCount( m_VmHandle, &netCount )  );
	QVERIFY( netCount == 1 );

	SdkHandleWrap hNet;
	CHECK_RET_CODE_EXP( PrlVmCfg_GetNetAdapter(m_VmHandle, 0, hNet.GetHandlePtr() )  );

	char buff[2048];
	PRL_UINT32 sz=sizeof(buff);
	CHECK_RET_CODE_EXP( PrlVmDevNet_GetMacAddress(hNet, buff, &sz) );

	QString newMAC = buff;
	// compare MACs
	QCOMPARE( newMAC.toUpper(), oldMAC.toUpper() );
}

void PrlSrvManipulationsTest::testGetHostHardwareInfoCheckHardDisksListSorted()
{
	RECEIVE_SERVER_HW_INFO

	QList<CHwHardDisk *> lstHardDisks = _hw_info.m_lstHardDisks;
	QStringList lstHardDisksSystemNames;
	foreach(CHwHardDisk *pHardDisk, lstHardDisks)
		lstHardDisksSystemNames.append(pHardDisk->getDeviceId());
	lstHardDisksSystemNames.sort();
	for(int i = 0; i < lstHardDisksSystemNames.size(); ++i)
	{
		QString sHardDiskSysName = lstHardDisksSystemNames.at(i);
		CHwHardDisk *pHardDisk = lstHardDisks.at(i);
		QCOMPARE(sHardDiskSysName, pHardDisk->getDeviceId());
	}
}

void PrlSrvManipulationsTest::testRegisterOldVmWithNonSimpleHddsDirsStructure()
{
	testLoginLocal();
	QVERIFY(QDir().mkdir(m_sTestFsDirName1));
	QVERIFY(QDir().mkdir(m_sTestFsDirName1ChildDir));
	QString sVmConfigPath = m_sTestFsDirName1ChildDir + "/CConfigConverterTest_pw25.pvs";
	QVERIFY(QFile::copy("./CConfigConverterTest_pw25.pvs", sVmConfigPath));

	QString sCompositeImagePath = "disks/root/winxp.hdd";

	ConfigFile _old_configuration(sVmConfigPath);
	QVERIFY(_old_configuration.IsValid());
	_old_configuration.setParamValue(IDE_SECTION, IDE00_IMAGE, sCompositeImagePath);
	QVERIFY(_old_configuration.saveConfig());

	SdkHandleWrap hJob(PrlSrv_RegisterVm(m_ServerHandle, sVmConfigPath.toUtf8().constData(), PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()))
	CHECK_HANDLE_TYPE(m_VmHandle, PHT_VIRTUAL_MACHINE)

	QString sVmHomePath;
	PRL_EXTRACT_STRING_VALUE(sVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath)

	SdkHandleWrap hVmHardDisk;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetHardDisk(m_VmHandle, 0, hVmHardDisk.GetHandlePtr()))

	QString sActualFriendlyName, sActualSystemName;
	PRL_EXTRACT_STRING_VALUE(sActualFriendlyName, hVmHardDisk, PrlVmDev_GetFriendlyName)
	PRL_EXTRACT_STRING_VALUE(sActualSystemName, hVmHardDisk, PrlVmDev_GetSysName)

	QFileInfo _fi_expected(QFileInfo(sVmHomePath).absolutePath() + "/" + sCompositeImagePath);
	QFileInfo _fi_actual1(sActualFriendlyName), _fi_actual2(sActualSystemName);

	QCOMPARE(_fi_actual1.absoluteFilePath(), _fi_expected.absoluteFilePath());
	QCOMPARE(_fi_actual2.absoluteFilePath(), _fi_expected.absoluteFilePath());
}

void PrlSrvManipulationsTest::testRegisterOldVmOwnedByAnotherUserButAclWithRightsAdded()
{
#ifndef _MAC_
	QSKIP("Skipping due currently we have ACL support just under Mac OS X", SkipAll);
#endif

	if (!TestConfig::isServerMode())
		QSKIP("Skipping test due not compatible with non server mode", SkipAll);

	CAuthHelper _auth_helper(TestConfig::getUserLogin2());
	QVERIFY(_auth_helper.AuthUser(TestConfig::getUserPassword()));
	QVERIFY(CFileHelper::WriteDirectory(m_sTestFsDirName1ChildDir, &_auth_helper));
	QString sVmConfigPath = m_sTestFsDirName1ChildDir + "/CConfigConverterTest_pw25.pvs";
	QVERIFY(QFile::copy("./CConfigConverterTest_pw25.pvs", sVmConfigPath));
	QVERIFY(CFileHelper::setOwner(sVmConfigPath, &_auth_helper, false));

#ifdef _MAC_
	//FIXME: workaround due acl_get_file() system call never returns ENOTSUP and we do not
	//have legacy method to understand whether ACLs supported om filesystem or not
	AddAcl(m_sTestFsDirName1ChildDir, "root", ALLOW, WRITE | READ | EXECUTE);
#endif

	if (!CAclHelper::IsAclSupported(m_sTestFsDirName1ChildDir))
		QSKIP("ACLs not supported on filesystem so skipping", SkipAll);

	testLogin();
	PRL_BOOL bConnected = PRL_FALSE;
	PRL_BOOL bLocallyConnected = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlSrv_IsConnected(m_ServerHandle, &bConnected, &bLocallyConnected))
	QVERIFY(bConnected == PRL_TRUE);
	QVERIFY(bLocallyConnected == PRL_FALSE);

	//Add permissions for currently connected to dispatcher user
	QVERIFY(AddAcl(m_sTestFsDirName1, TestConfig::getUserLogin(), ALLOW, WRITE | READ | EXECUTE));
	QVERIFY(AddAcl(m_sTestFsDirName1ChildDir, TestConfig::getUserLogin(), ALLOW, WRITE | READ | EXECUTE));
	QVERIFY(AddAcl(sVmConfigPath, TestConfig::getUserLogin(), ALLOW, WRITE | READ | EXECUTE));

	//Register VM now - all should be fine
	SdkHandleWrap hJob(PrlSrv_RegisterVm(m_ServerHandle, sVmConfigPath.toUtf8().constData(), PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()))
	CHECK_HANDLE_TYPE(m_VmHandle, PHT_VIRTUAL_MACHINE)
}

void PrlSrvManipulationsTest::testRegisterOldVmWithSlashInName()
{
	testLoginLocal();
	QVERIFY(QDir().mkdir(m_sTestFsDirName1));
	QVERIFY(QDir().mkdir(m_sTestFsDirName1ChildDir));
	QString sVmConfigPath = m_sTestFsDirName1ChildDir + "/SDKTest_OldVmWithSlashInName.pvs";
	QVERIFY(QFile::copy("./SDKTest_OldVmWithSlashInName.pvs", sVmConfigPath));
	SdkHandleWrap hJob(PrlSrv_RegisterVm(m_ServerHandle, sVmConfigPath.toUtf8().constData(), PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()))
	CHECK_HANDLE_TYPE(m_VmHandle, PHT_VIRTUAL_MACHINE)

	QString sVmName;
	PRL_EXTRACT_STRING_VALUE(sVmName, m_VmHandle, PrlVmCfg_GetName)

	QCOMPARE(sVmName, QString("XP autostart disabled, docked apps, kis 75219"));
}

void PrlSrvManipulationsTest::testRegisterOldVmWithSlashInName2()
{
#ifdef _WIN_
	QSKIP("Skipping test due ':' symbol not acceptable at file paths under Win platform", SkipAll);
#endif

	QString sTestVmName = "Windows XP (8:19)";
	m_sTestFsDirName1 = QDir::tempPath() + '/' + sTestVmName + ".pvm";
	if (QFile::exists(m_sTestFsDirName1))
		CFileHelper::ClearAndDeleteDir(m_sTestFsDirName1);
	testLoginLocal();
	QVERIFY(QDir().mkdir(m_sTestFsDirName1));
	QString sVmConfigPath = m_sTestFsDirName1 + "/" + sTestVmName + ".pvs";
	QVERIFY(QFile::copy("./SDKTest_OldVmWithSlashInName.pvs", sVmConfigPath));

	ConfigFile _old_config(sVmConfigPath);
	QVERIFY(_old_config.IsValid());
	_old_config.setParamValue(SYSTEM_SECTION, VM_NAME, sTestVmName);
	QVERIFY(_old_config.saveConfig());

	SdkHandleWrap hJob(PrlSrv_RegisterVm(m_ServerHandle, sVmConfigPath.toUtf8().constData(), PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()))
	CHECK_HANDLE_TYPE(m_VmHandle, PHT_VIRTUAL_MACHINE)

	QVERIFY(!QFile::exists(m_sTestFsDirName1));
	QVERIFY(QFile::exists(QDir::tempPath() + '/' + CFileHelper::ReplaceNonValidPathSymbols(sTestVmName) + ".pvm"));
	m_sTestFsDirName1 = QDir::tempPath() + '/' + CFileHelper::ReplaceNonValidPathSymbols(sTestVmName) + ".pvm";

	QString sVmName;
	PRL_EXTRACT_STRING_VALUE(sVmName, m_VmHandle, PrlVmCfg_GetName)

	QCOMPARE(sVmName, CFileHelper::ReplaceNonValidPathSymbols(sTestVmName));
}

void PrlSrvManipulationsTest::testRegisterOldVmWithSlashInName3()
{
#ifdef _WIN_
	QSKIP("Skipping test due ':' symbol not acceptable at file paths under Win platform", SkipAll);
#endif

	QString sTestVmName = "Windows XP (8:19)";
	m_sTestFsDirName1 = QDir::tempPath() + '/' + sTestVmName;
	if (QFile::exists(m_sTestFsDirName1))
		CFileHelper::ClearAndDeleteDir(m_sTestFsDirName1);
	testLoginLocal();
	QVERIFY(QDir().mkdir(m_sTestFsDirName1));
	QString sVmConfigPath = m_sTestFsDirName1 + "/" + sTestVmName + ".pvs";
	QVERIFY(QFile::copy("./SDKTest_OldVmWithSlashInName.pvs", sVmConfigPath));

	ConfigFile _old_config(sVmConfigPath);
	QVERIFY(_old_config.IsValid());
	_old_config.setParamValue(SYSTEM_SECTION, VM_NAME, sTestVmName);
	QVERIFY(_old_config.saveConfig());

	SdkHandleWrap hJob(PrlSrv_RegisterVm(m_ServerHandle, sVmConfigPath.toUtf8().constData(), PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()))
	CHECK_HANDLE_TYPE(m_VmHandle, PHT_VIRTUAL_MACHINE)

	QVERIFY(!QFile::exists(m_sTestFsDirName1 + ".pvm"));
	QVERIFY(QFile::exists(QDir::tempPath() + '/' + CFileHelper::ReplaceNonValidPathSymbols(sTestVmName) + ".pvm"));
	m_sTestFsDirName1 = QDir::tempPath() + '/' + CFileHelper::ReplaceNonValidPathSymbols(sTestVmName) + ".pvm";

	QString sVmName;
	PRL_EXTRACT_STRING_VALUE(sVmName, m_VmHandle, PrlVmCfg_GetName)

	QCOMPARE(sVmName, CFileHelper::ReplaceNonValidPathSymbols(sTestVmName));
}

void PrlSrvManipulationsTest::testRegisterOldVmWithSlashInName4()
{
	testLoginLocal();
	m_sTestFsDirName1 = QDir::tempPath() + "/XP autostart disabled, docked apps, kis 752";
	if (QFile::exists(m_sTestFsDirName1))
		CFileHelper::ClearAndDeleteDir(m_sTestFsDirName1);
	m_sTestFsDirName1ChildDir = m_sTestFsDirName1 + "/19";
	m_sTestFsDirName1ChildChildDir = m_sTestFsDirName1ChildDir + "/First Level";
	m_sTestFsDirName1ChildChildChildDir = m_sTestFsDirName1ChildChildDir + "/Second Level";
	QVERIFY(QDir().mkdir(m_sTestFsDirName1));
	QVERIFY(QDir().mkdir(m_sTestFsDirName1ChildDir));
	QVERIFY(QDir().mkdir(m_sTestFsDirName1ChildChildDir));
	QVERIFY(QDir().mkdir(m_sTestFsDirName1ChildChildChildDir));
	QString sVmConfigPath = m_sTestFsDirName1ChildChildChildDir + "/SDKTest_OldVmWithSlashInName.pvs";
	QVERIFY(QFile::copy("./SDKTest_OldVmWithSlashInName.pvs", sVmConfigPath));
	SdkHandleWrap hJob(PrlSrv_RegisterVm(m_ServerHandle, sVmConfigPath.toUtf8().constData(), PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()))
	CHECK_HANDLE_TYPE(m_VmHandle, PHT_VIRTUAL_MACHINE)

	QString sVmName;
	PRL_EXTRACT_STRING_VALUE(sVmName, m_VmHandle, PrlVmCfg_GetName)

	QCOMPARE(sVmName, QString("XP autostart disabled, docked apps, kis 75219"));
}

void PrlSrvManipulationsTest::testRegisterVmOnPathToVmHomeDirSpecifiedConfigWasBrokenButBackupPresent()
{
	REGISTER_VM_PROLOG
	QVERIFY(QFile::copy(sExpectedVmPath, sExpectedVmPath + VMDIR_DEFAULT_VM_BACKUP_SUFFIX));
	QFile _config_file_cleaner(sExpectedVmPath);
	QVERIFY(_config_file_cleaner.open(QIODevice::WriteOnly | QIODevice::Truncate));
	_config_file_cleaner.close();
	SdkHandleWrap hJob(PrlSrv_RegisterVm(m_ServerHandle, m_sTestFsDirName1ChildDir.toUtf8().constData(), PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()))

	QString qsVmHomePath;
	PRL_EXTRACT_STRING_VALUE(qsVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath);
	QVERIFY(QFile::exists(qsVmHomePath));
}

void PrlSrvManipulationsTest::testRegisterVmOnPathToVmConfigSpecifiedConfigWasBrokenButBackupPresent()
{
	REGISTER_VM_PROLOG
	QVERIFY(QFile::copy(sExpectedVmPath, sExpectedVmPath + VMDIR_DEFAULT_VM_BACKUP_SUFFIX));
	QFile _config_file_cleaner(sExpectedVmPath);
	QVERIFY(_config_file_cleaner.open(QIODevice::WriteOnly | QIODevice::Truncate));
	_config_file_cleaner.close();
	SdkHandleWrap hJob(PrlSrv_RegisterVm(m_ServerHandle, sExpectedVmPath.toUtf8().constData(), PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()))

	QString qsVmHomePath;
	PRL_EXTRACT_STRING_VALUE(qsVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath);
	QVERIFY(QFile::exists(qsVmHomePath));
}

void PrlSrvManipulationsTest::testRegisterVmOnPathToVmHomeDirSpecifiedConfigWasDeletedButBackupPresent()
{
	REGISTER_VM_PROLOG
	QVERIFY(QFile::copy(sExpectedVmPath, sExpectedVmPath + VMDIR_DEFAULT_VM_BACKUP_SUFFIX));
	QVERIFY(QFile::remove(sExpectedVmPath));
	SdkHandleWrap hJob(PrlSrv_RegisterVm(m_ServerHandle, m_sTestFsDirName1ChildDir.toUtf8().constData(), PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()))

	QString qsVmHomePath;
	PRL_EXTRACT_STRING_VALUE(qsVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath);
	QVERIFY(QFile::exists(qsVmHomePath));
}

void PrlSrvManipulationsTest::testRegisterVmOnPathToVmConfigSpecifiedConfigWasDeletedButBackupPresent()
{
	REGISTER_VM_PROLOG
	QVERIFY(QFile::copy(sExpectedVmPath, sExpectedVmPath + VMDIR_DEFAULT_VM_BACKUP_SUFFIX));
	QVERIFY(QFile::remove(sExpectedVmPath));
	SdkHandleWrap hJob(PrlSrv_RegisterVm(m_ServerHandle, sExpectedVmPath.toUtf8().constData(), PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()))

	QString qsVmHomePath;
	PRL_EXTRACT_STRING_VALUE(qsVmHomePath, m_VmHandle, PrlVmCfg_GetHomePath);
	QVERIFY(QFile::exists(qsVmHomePath));
}

void PrlSrvManipulationsTest::testConfigureGenericPciOnWrongParams()
{
	SdkHandleWrap hList;
	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hList.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlSrv_ConfigureGenericPci(m_ServerHandle, m_ServerHandle, 0),
							PRL_ERR_INVALID_ARG);
	CHECK_ASYNC_OP_FAILED(PrlSrv_ConfigureGenericPci(hList, hList, 0),
							PRL_ERR_INVALID_ARG);
	// Empty list
	CHECK_ASYNC_OP_FAILED(PrlSrv_ConfigureGenericPci(m_ServerHandle, hList, 0),
							PRL_ERR_INVALID_ARG);
	// Wrong handle list
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hList, m_ServerHandle));
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hList, m_VmHandle));
	CHECK_ASYNC_OP_FAILED(PrlSrv_ConfigureGenericPci(m_ServerHandle, hList, 0),
							PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testGenericPciDeviceState()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_GetSrvConfig(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hResult, PHT_RESULT);

	SdkHandleWrap hSrvConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hSrvConfig.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hSrvConfig, PHT_SERVER_CONFIG);

	HANDLE_TO_STRING(hSrvConfig);
	CHostHardwareInfo hw_info;
	hw_info.fromString(_str_object);
	hw_info.getGenericPciDevices()->ClearLists();

	for(int nDevState = PGS_CONNECTED_TO_HOST; nDevState <= PGS_RESERVED; nDevState++)
	{
		CHwGenericPciDevice* pPciDevice = new CHwGenericPciDevice();
		pPciDevice->setDeviceId(Uuid::createUuid().toString());
		pPciDevice->setDeviceState((PRL_GENERIC_DEVICE_STATE )nDevState);

		hw_info.addGenericPciDevice( pPciDevice );
	}

	CHECK_RET_CODE_EXP(PrlHandle_FromString( hSrvConfig, QSTR2UTF8(hw_info.toString()) ));

	PRL_UINT32 nCount = 0;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetGenericPciDevicesCount(hSrvConfig, &nCount));
	QVERIFY(nCount == 3);

	for(PRL_UINT32 i = 0; i < nCount; i++)
	{
		SdkHandleWrap hPciDev;
		CHECK_RET_CODE_EXP(PrlSrvCfg_GetGenericPciDevice(hSrvConfig, i, hPciDev.GetHandlePtr()));

		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfgDev_GetDeviceState(hPciDev, NULL),
										PRL_ERR_INVALID_ARG);

		PRL_GENERIC_DEVICE_STATE nDevState;
		CHECK_RET_CODE_EXP(PrlSrvCfgDev_GetDeviceState(hPciDev, &nDevState));
		QVERIFY(nDevState == (PRL_GENERIC_DEVICE_STATE )i);

		nDevState = (PRL_GENERIC_DEVICE_STATE )(nCount - 1 - i);
		CHECK_RET_CODE_EXP(PrlSrvCfgDev_SetDeviceState(hPciDev, nDevState));
		CHECK_RET_CODE_EXP(PrlSrvCfgDev_GetDeviceState(hPciDev, &nDevState));
		QVERIFY(nDevState == (PRL_GENERIC_DEVICE_STATE )(nCount - 1 - i));
	}
}

void PrlSrvManipulationsTest::testGenericPciDeviceStateOnWrongParams()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	PRL_GENERIC_DEVICE_STATE nDevState;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfgDev_GetDeviceState(m_VmHandle, &nDevState),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfgDev_SetDeviceState(m_VmHandle, PGS_CONNECTED_TO_VM),
										PRL_ERR_INVALID_ARG);
	// Null pointer
	// Note: see previous test
}

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

void PrlSrvManipulationsTest::testVmDeviceUpdateInfo()
{
	if (TestConfig::isServerMode())
		testLogin();
	else
		testLoginLocal();

	//Made small test preparations (see https://bugzilla.sw.ru/show_bug.cgi?id=439741 for more details)
	INITIALIZE_VM("./TestDspCmdDirValidateVmConfig_vm_config.xml")
	m_JobHandle.reset(PrlVm_Stop(m_VmHandle, PRL_FALSE));
	PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
	m_JobHandle.reset(PrlVm_Delete(m_VmHandle,PRL_INVALID_HANDLE));
	PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
	m_JobHandle.reset(PrlVm_Unreg(m_VmHandle));
	PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);

	m_JobHandle.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(m_JobHandle);

// Edit VM config - add image hdd and generate it

	m_JobHandle.reset(PrlVm_BeginEdit(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle)

// Prepare

	QString qsVmDir;
	PRL_EXTRACT_STRING_VALUE(qsVmDir, m_VmHandle, PrlVmCfg_GetHomePath);
	qsVmDir = QFileInfo(qsVmDir).path();

	QString qsDir = qsVmDir + "/hard1.hdd";

	CHECK_RET_CODE_EXP(PrlVmCfg_SetOsVersion(m_VmHandle, PVS_GUEST_VER_WIN_2K));

// Add hard disk

	SdkHandleWrap hDevice;
	CHECK_RET_CODE_EXP(PrlVmCfg_CreateVmDev(m_VmHandle, PDE_HARD_DISK, hDevice.GetHandlePtr()));

	CHECK_RET_CODE_EXP(PrlVmDev_SetEnabled(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetConnected(hDevice, TRUE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetSysName(hDevice, QSTR2UTF8(qsDir)));
	CHECK_RET_CODE_EXP(PrlVmDev_SetFriendlyName(hDevice, QSTR2UTF8(qsDir)));
	CHECK_RET_CODE_EXP(PrlVmDev_SetIfaceType(hDevice, PMS_IDE_DEVICE));
	CHECK_RET_CODE_EXP(PrlVmDev_SetStackIndex(hDevice, 0));
	CHECK_RET_CODE_EXP(PrlVmDev_SetEmulatedType(hDevice, PDT_USE_IMAGE_FILE));
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskSize(hDevice, 1024 * 1024));
	CHECK_RET_CODE_EXP(PrlVmDevHd_SetDiskType(hDevice, PHD_EXPANDING_HARD_DISK));

	m_JobHandle.reset(PrlVmDev_CreateImage(hDevice, PRL_TRUE, PRL_TRUE));
	CHECK_JOB_RET_CODE(m_JobHandle);
////
	m_JobHandle.reset(PrlVm_Commit(m_VmHandle));
	CHECK_JOB_RET_CODE(m_JobHandle);

// Device update info

	m_JobHandle.reset(PrlVmDev_UpdateInfo(m_ServerHandle, hDevice));
	CHECK_JOB_RET_CODE(m_JobHandle);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()));

	QString qsDevXml;
	PRL_EXTRACT_STRING_VALUE(qsDevXml, m_ResultHandle, PrlResult_GetParamAsString);

	CHECK_RET_CODE_EXP(PrlVmDev_FromString(hDevice, QSTR2UTF8(qsDevXml)));
}

void PrlSrvManipulationsTest::testGenericPciDeviceOps()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_GetSrvConfig(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hResult, PHT_RESULT);

	SdkHandleWrap hSrvConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hSrvConfig.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hSrvConfig, PHT_SERVER_CONFIG);

	HANDLE_TO_STRING(hSrvConfig);
	CHostHardwareInfo hw_info;
	hw_info.fromString(_str_object);
	hw_info.getGenericPciDevices()->ClearLists();

	for(int nDevClass = PGD_PCI_NETWORK; nDevClass <= PGD_PCI_OTHER; nDevClass++)
	{
		CHwGenericPciDevice* pPciDevice = new CHwGenericPciDevice();
		pPciDevice->setDeviceId(Uuid::createUuid().toString());
		pPciDevice->setType((PRL_GENERIC_PCI_DEVICE_CLASS )nDevClass);

		pPciDevice->setPrimary( nDevClass%2 );

		hw_info.addGenericPciDevice( pPciDevice );
	}

	CHECK_RET_CODE_EXP(PrlHandle_FromString( hSrvConfig, QSTR2UTF8(hw_info.toString()) ));

	PRL_UINT32 nCount = 0;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetGenericPciDevicesCount(hSrvConfig, &nCount));
	QVERIFY(nCount == 4);

	for(PRL_UINT32 i = 0; i < nCount; i++)
	{
		SdkHandleWrap hPciDev;
		CHECK_RET_CODE_EXP(PrlSrvCfg_GetGenericPciDevice(hSrvConfig, i, hPciDev.GetHandlePtr()));

		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfgPci_GetDeviceClass(hPciDev, NULL),
										PRL_ERR_INVALID_ARG);

		PRL_GENERIC_PCI_DEVICE_CLASS nDevClass;
		CHECK_RET_CODE_EXP(PrlSrvCfgPci_GetDeviceClass(hPciDev, &nDevClass));
		QCOMPARE(quint32(nDevClass), quint32(i));

		// test isPrimary sign
		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfgPci_IsPrimaryDevice(hPciDev, NULL),
			PRL_ERR_INVALID_ARG);

		PRL_BOOL isPrimary;
		CHECK_RET_CODE_EXP(PrlSrvCfgPci_IsPrimaryDevice(hPciDev, &isPrimary));
		QCOMPARE(quint32(isPrimary), quint32(i%2));

	}
}

void PrlSrvManipulationsTest::testGenericPciDeviceClassOnWrongParams()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	PRL_GENERIC_PCI_DEVICE_CLASS nDevClass;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfgPci_GetDeviceClass(m_VmHandle, &nDevClass),
										PRL_ERR_INVALID_ARG);
	// Null pointer
	// Note: see previous test ( testGenericPciDeviceOps() )
}


void PrlSrvManipulationsTest::testGenericPciDeviceIsPrimaryOnWrongParams()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	PRL_BOOL isPrimary;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfgPci_IsPrimaryDevice(m_VmHandle, &isPrimary),
		PRL_ERR_INVALID_ARG);
	// Null pointer
	// Note: see testGenericPciDeviceOps()
}

void PrlSrvManipulationsTest::testCreateVmInSpecificNonExistsFolderNonInteractiveMode()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_GetSrvConfig(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hSrvConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hSrvConfig.GetHandlePtr()))

	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(m_VmHandle, hSrvConfig, PVS_GUEST_VER_WIN_VISTA, PRL_TRUE))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, QSTR2UTF8(Uuid::createUuid().toString())))

	CHECK_ASYNC_OP_FAILED(PrlVm_Reg(m_VmHandle, QSTR2UTF8(m_sTestFsDirName1), PRL_TRUE), PRL_ERR_DIRECTORY_DOES_NOT_EXIST)
}

#define WAIT_QUESTION_ABOUT_CREATE_NON_EXISTENT_DIR\
	const unsigned int nTotalWaitTime = 60*1000;\
	const unsigned int nSingleShotWaitTime = 1000;\
	SdkHandleWrap hQuestion;\
	bool bQuestionFound = false;\
	for (unsigned int i = 0; i < nTotalWaitTime/nSingleShotWaitTime; ++i)\
	{\
		SdkHandleWrap hQuestionsList;\
		CHECK_RET_CODE_EXP(PrlSrv_GetQuestions(m_ServerHandle, hQuestionsList.GetHandlePtr()))\
		PRL_UINT32 nQuestionsCount = 0;\
		CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hQuestionsList, &nQuestionsCount))\
		for (PRL_UINT32 j = 0; j < nQuestionsCount; ++j)\
		{\
			CHECK_RET_CODE_EXP(PrlHndlList_GetItem(hQuestionsList, j, hQuestion.GetHandlePtr()))\
			CHECK_HANDLE_TYPE(hQuestion, PHT_EVENT)\
			PRL_RESULT nQuestionId = PRL_ERR_UNINITIALIZED;\
			CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hQuestion, &nQuestionId))\
			if (PET_QUESTION_VM_ROOT_DIRECTORY_NOT_EXISTS == nQuestionId)\
			{\
				bQuestionFound = true;\
				break;\
			}\
		}\
		if (bQuestionFound)\
			break;\
		HostUtils::Sleep(nSingleShotWaitTime);\
	}\
	QVERIFY(bQuestionFound);

void PrlSrvManipulationsTest::testCreateVmInSpecificNonExistsFolderInteractiveModeFolderCreationAccepted()
{
	if (TestConfig::isServerMode())
		QSKIP("Skipping test due to the interaction is not supported by the server mode", SkipAll);

	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_GetSrvConfig(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hSrvConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hSrvConfig.GetHandlePtr()))

	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(m_VmHandle, hSrvConfig, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, QSTR2UTF8(Uuid::createUuid().toString())))

	SdkHandleWrap hRegVmJob(PrlVm_Reg(m_VmHandle, QSTR2UTF8(m_sTestFsDirName1), PRL_FALSE));

	WAIT_QUESTION_ABOUT_CREATE_NON_EXISTENT_DIR

	SdkHandleWrap hAnswer;
	CHECK_RET_CODE_EXP(PrlEvent_CreateAnswerEvent(hQuestion, hAnswer.GetHandlePtr(), PET_ANSWER_YES))
	hJob.reset(PrlSrv_SendAnswer(m_ServerHandle, hAnswer));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_JOB_RET_CODE(hRegVmJob)
	QFileInfo _fi(m_sTestFsDirName1);
	QVERIFY(_fi.exists());
	QVERIFY(_fi.isDir());
}

void PrlSrvManipulationsTest::testCreateVmInSpecificNonExistsFolderInteractiveModeFolderCreationRejected()
{
	if (TestConfig::isServerMode())
		QSKIP("Skipping test due to the interaction is not supported by the server mode", SkipAll);

	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_GetSrvConfig(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hSrvConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hSrvConfig.GetHandlePtr()))

	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(m_VmHandle, hSrvConfig, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_VmHandle, QSTR2UTF8(Uuid::createUuid().toString())))

	SdkHandleWrap hRegVmJob(PrlVm_Reg(m_VmHandle, QSTR2UTF8(m_sTestFsDirName1), PRL_FALSE));

	WAIT_QUESTION_ABOUT_CREATE_NON_EXISTENT_DIR

	SdkHandleWrap hAnswer;
	CHECK_RET_CODE_EXP(PrlEvent_CreateAnswerEvent(hQuestion, hAnswer.GetHandlePtr(), PET_ANSWER_NO))
	hJob.reset(PrlSrv_SendAnswer(m_ServerHandle, hAnswer));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_ASYNC_OP_FAILED(hRegVmJob, PRL_ERR_DIRECTORY_DOES_NOT_EXIST)
}

void PrlSrvManipulationsTest::testPrepareForHibernateOnWrongParams()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlSrv_PrepareForHibernate(m_VmHandle, 0), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testAfterHostResumeOnWrongParams()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlSrv_AfterHostResume(m_VmHandle, 0), PRL_ERR_INVALID_ARG);
}

namespace {
	const PRL_UINT32 g_nMaxProblemSendTimeout = 600000;

    void waitAndCheckProblemReportSent(SdkHandleWrap hJob)
    {
        SdkHandleWrap hResult;
        PRL_UINT32 nCommonTimeout = g_nMaxProblemSendTimeout;
        PRL_RESULT nRetCode = PRL_ERR_TIMEOUT;
        while (PRL_ERR_TIMEOUT == nRetCode && nCommonTimeout)
        {
            nRetCode = PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT);
            if (nCommonTimeout >= PRL_JOB_WAIT_TIMEOUT)
                nCommonTimeout -= PRL_JOB_WAIT_TIMEOUT;
            else
                nCommonTimeout = 0;
        }
        //We do not have any guarantees that problem report will be sent
        //always successfully. It's quite strong depend from host configuration
        //(proxy settings, firewall settings, Internet settings and etc.)
        //So the rest part of test is optional
        if (PRL_ERR_TIMEOUT == nRetCode)
        {
            PrlJob_Cancel(hJob);
            QSKIP("Couldn't to send problem report by some reasons", SkipAll);
        }
        CHECK_RET_CODE_EXP(nRetCode)
        CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nRetCode))
        if (PRL_SUCCEEDED(nRetCode))
        {
            CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
            QString sProblemReportId;
            PRL_EXTRACT_STRING_VALUE(sProblemReportId, hResult, PrlResult_GetParamAsString)
            bool bOk = false;
            PRL_UINT32 nProblemReportId = sProblemReportId.toUInt(&bOk);
            QVERIFY(nProblemReportId > 0);
            QVERIFY(bOk);
        }
    }
}

void PrlSrvManipulationsTest::testSendProblemReport()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_GetProblemReport(m_ServerHandle));
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT*10))
	PRL_RESULT nRetCode;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nRetCode))
	if (PRL_ERR_UNIMPLEMENTED == nRetCode)
		QSKIP("Old problem report scheme not supported on server!", SkipAll);
	else
		CHECK_RET_CODE_EXP(nRetCode)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QString sProblemReport;
	PRL_EXTRACT_STRING_VALUE(sProblemReport, hResult, PrlResult_GetParamAsString)

	CProblemReport _report;
	_report.fromString(sProblemReport);
	CHECK_RET_CODE_EXP(_report.m_uiRcInit)

	hJob.reset(PrlApi_SendProblemReport(QSTR2UTF8(sProblemReport), PRL_FALSE, 0, 0, 0, 0, g_nMaxProblemSendTimeout, 0, 0, 0));
    waitAndCheckProblemReportSent(hJob);
}

void PrlSrvManipulationsTest::testSendProblemReportOnWrongReport()
{
	testLoginLocal();

	CHECK_ASYNC_OP_FAILED(PrlApi_SendProblemReport(0, PRL_FALSE, 0, 0, 0, 0, 0, 0, 0, 0), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlApi_SendProblemReport("", PRL_FALSE, 0, 0, 0, 0, 0, 0, 0, 0), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlApi_SendProblemReport("wrong problem report", PRL_FALSE, 0, 0, 0, 0, 0, 0, 0, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testSendProblemReportByServer()
{
    testLoginLocal();

    // Test for sending problem report to the server
    PRL_HANDLE hPR;
    PRL_RESULT res = PrlApi_CreateProblemReport(PRS_OLD_XML_BASED, &hPR);
    QVERIFY(!PRL_FAILED(res));
    SdkHandleWrap hJob(PrlSrv_SendProblemReport(m_ServerHandle, hPR, 0));
    waitAndCheckProblemReportSent(hJob);
}


namespace {
struct CheckProblemReportSendParams
{
	QMutex m_mutex;
	QWaitCondition m_cond;
	SdkHandleWrap m_hJob;
	QList<PRL_UINT32> m_lstPercents;
	bool m_bFirstCallTime;
	bool m_bJobWasCompleted;

	CheckProblemReportSendParams() : m_bFirstCallTime(true), m_bJobWasCompleted(false) {}
};

#define CHECK_SDK_EXPR(expr)\
	{\
		PRL_RESULT nRetCode = expr;\
		if (PRL_FAILED(nRetCode))\
		{\
			WRITE_TRACE(DBG_FATAL, "Expression '%s' failed with error: 0x%.8X '%s'", #expr, nRetCode, PRL_RESULT_TO_STRING(nRetCode));\
			return (PRL_ERR_OPERATION_FAILED);\
		}\
	}

PRL_RESULT CheckProblemReportSendCallback(PRL_HANDLE _handle, PRL_VOID_PTR pData)
{
	SdkHandleWrap hObject(_handle);

	PRL_HANDLE_TYPE nHandleType = PHT_ERROR;
	CHECK_SDK_EXPR(PrlHandle_GetType(hObject, &nHandleType))
	CheckProblemReportSendParams *pParams = static_cast<CheckProblemReportSendParams *>(pData);
	QMutexLocker _lock(&pParams->m_mutex);
	if (PHT_EVENT == nHandleType)
	{
		PRL_EVENT_TYPE nEventType = PET_VM_INF_UNINITIALIZED_EVENT_CODE;
		CHECK_SDK_EXPR(PrlEvent_GetType(hObject, &nEventType))
		if (PET_DSP_EVT_JOB_PROGRESS_CHANGED == nEventType)
		{
			SdkHandleWrap hEventParameter;
			CHECK_SDK_EXPR(PrlEvent_GetParam(hObject, 0, hEventParameter.GetHandlePtr()))
			PRL_UINT32 nPercents = 0;
			CHECK_SDK_EXPR(PrlEvtPrm_ToUint32(hEventParameter, &nPercents))
			pParams->m_lstPercents.append(nPercents);
		}
		else
			WRITE_TRACE(DBG_FATAL, "Unknown event type received: %.8X", nEventType);
	}
	else if (PHT_JOB == nHandleType)
	{
		if (hObject.GetHandle() == pParams->m_hJob.GetHandle())
			pParams->m_cond.wakeAll();
		else
			WRITE_TRACE(DBG_FATAL, "Not known job object was received");
	}
	else
		WRITE_TRACE(DBG_FATAL, "Unknown object type: %.8X", nHandleType);

	return (PRL_ERR_SUCCESS);
}

}

void PrlSrvManipulationsTest::testSendProblemReportAtAsyncMode()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_GetProblemReport(m_ServerHandle));
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT*10))
	PRL_RESULT nRetCode;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nRetCode))
	if (PRL_ERR_UNIMPLEMENTED == nRetCode)
		QSKIP("Old problem report scheme not supported on server!", SkipAll);
	else
		CHECK_RET_CODE_EXP(nRetCode)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QString sProblemReport;
	PRL_EXTRACT_STRING_VALUE(sProblemReport, hResult, PrlResult_GetParamAsString)

	CProblemReport _report;
	_report.fromString(sProblemReport);
	CHECK_RET_CODE_EXP(_report.m_uiRcInit)

	CheckProblemReportSendParams _params;
	{
		QMutexLocker _lock(&_params.m_mutex);
		hJob.reset(PrlApi_SendProblemReport(QSTR2UTF8(sProblemReport), PRL_FALSE, 0, 0, 0, 0, g_nMaxProblemSendTimeout, 0, CheckProblemReportSendCallback, &_params));
		_params.m_hJob = hJob;
		//We do not have any guarantees that problem report will be sent
		//always successfully. It's quite strong depend from host configuration
		//(proxy settings, firewall settings, Internet settings and etc.)
		//So the rest part of test is optional
		if(!_params.m_cond.wait(&_params.m_mutex, g_nMaxProblemSendTimeout))
		{
			PrlJob_Cancel(hJob);
			QSKIP("Couldn't to send problem report by some reasons", SkipAll);
		}
	}
	nRetCode = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nRetCode))
	if (PRL_SUCCEEDED(nRetCode))
	{
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
		QString sProblemReportId;
		PRL_EXTRACT_STRING_VALUE(sProblemReportId, hResult, PrlResult_GetParamAsString)
		bool bOk = false;
		PRL_UINT32 nProblemReportId = sProblemReportId.toUInt(&bOk);
		QVERIFY(nProblemReportId > 0);
		QVERIFY(bOk);
		QMutexLocker _lock(&_params.m_mutex);
		QVERIFY(_params.m_lstPercents.size());
		QCOMPARE(_params.m_lstPercents.toSet().size(), _params.m_lstPercents.size());
		foreach(PRL_UINT32 nPercents, _params.m_lstPercents)
		{
			if (!(nPercents > 0 && nPercents <= 100))
				WRITE_TRACE(DBG_FATAL, "nPercents=%u", nPercents);
			QVERIFY(nPercents > 0 && nPercents <= 100);
		}
	}
}

namespace {
PRL_RESULT CheckProblemReportSendCancelCallback(PRL_HANDLE _handle, PRL_VOID_PTR pData)
{
	SdkHandleWrap hObject(_handle);

	CheckProblemReportSendParams *pParams = static_cast<CheckProblemReportSendParams *>(pData);
	QMutexLocker _lock(&pParams->m_mutex);
	if (pParams->m_bFirstCallTime)
	{
		pParams->m_bFirstCallTime = false;
		if (hObject == pParams->m_hJob)
			pParams->m_bJobWasCompleted = true;
		pParams->m_cond.wakeAll();
	}
	else if (hObject == pParams->m_hJob)
	{
		pParams->m_bJobWasCompleted = true;
		pParams->m_cond.wakeAll();
	}

	return (PRL_ERR_SUCCESS);
}

}

void PrlSrvManipulationsTest::testSendProblemReportCancelOperation()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_GetProblemReport(m_ServerHandle));
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT*10))
	PRL_RESULT nRetCode;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nRetCode))
	if (PRL_ERR_UNIMPLEMENTED == nRetCode)
		QSKIP("Old problem report scheme not supported on server!", SkipAll);
	else
		CHECK_RET_CODE_EXP(nRetCode)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QString sProblemReport;
	PRL_EXTRACT_STRING_VALUE(sProblemReport, hResult, PrlResult_GetParamAsString)

	CProblemReport _report;
	_report.fromString(sProblemReport);
	CHECK_RET_CODE_EXP(_report.m_uiRcInit)

	CheckProblemReportSendParams _params;
	SdkHandleWrap hCancelJob;
	{
		QMutexLocker _lock(&_params.m_mutex);
		hJob.reset(PrlApi_SendProblemReport(QSTR2UTF8(sProblemReport), PRL_FALSE, 0, 0, 0, 0, g_nMaxProblemSendTimeout, 0, CheckProblemReportSendCancelCallback, &_params));
		_params.m_hJob = hJob;
		if(_params.m_cond.wait(&_params.m_mutex, g_nMaxProblemSendTimeout))
		{
			if (!_params.m_bJobWasCompleted)
				hCancelJob.reset(PrlJob_Cancel(hJob));
		}
		else
			QFAIL("Job completion awaiting wasn't broken!");
		//Wait until all notifications will be done
		if (!_params.m_bJobWasCompleted)
			if(!_params.m_cond.wait(&_params.m_mutex, g_nMaxProblemSendTimeout))
				QFAIL("Final notification wasn't done!");
	}
	CHECK_JOB_RET_CODE(hCancelJob)
	CHECK_ASYNC_OP_FAILED(hJob, PRL_ERR_OPERATION_WAS_CANCELED)
}

void PrlSrvManipulationsTest::testNonInterectiveSession()
{
	testLoginLocal();

	PRL_BOOL bNonInteractive;
	CHECK_RET_CODE_EXP(PrlSrv_IsNonInteractiveSession(m_ServerHandle, &bNonInteractive));
	QVERIFY(bNonInteractive == PRL_FALSE);

	m_JobHandle.reset(PrlSrv_SetNonInteractiveSession(m_ServerHandle, PRL_TRUE, 0));
	CHECK_JOB_RET_CODE(m_JobHandle);

	CHECK_RET_CODE_EXP(PrlSrv_IsNonInteractiveSession(m_ServerHandle, &bNonInteractive));
	QVERIFY(bNonInteractive == PRL_TRUE);
}

void PrlSrvManipulationsTest::testNonInterectiveSessionOnWrongParams()
{
	CHECK_RET_CODE_EXP(PrlSrv_Create(m_ServerHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	PRL_BOOL bNonInteractive;

	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlSrv_SetNonInteractiveSession(m_VmHandle, PRL_TRUE, 0),
							PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE((PrlSrv_IsNonInteractiveSession(m_VmHandle, &bNonInteractive)),
							PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE((PrlSrv_IsNonInteractiveSession(m_ServerHandle, NULL)),
							PRL_ERR_INVALID_ARG);
}

#define COMMON_PREFS_PREFACE\
	CDispCommonPreferences common_prefs; \
	testLoginLocal(); \
	SdkHandleWrap hCommonPrefs;

#define GET_COMMON_PREFS \
	m_JobHandle.reset(PrlSrv_GetCommonPrefs(m_ServerHandle)); \
	CHECK_JOB_RET_CODE(m_JobHandle) \
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr())) \
	CHECK_RET_CODE_EXP(PrlResult_GetParam(m_ResultHandle, hCommonPrefs.GetHandlePtr()))

#define COMMON_PREFS_TO_XML_OBJECT \
	{ \
		PRL_VOID_PTR pXml = 0; \
		CHECK_RET_CODE_EXP(PrlHandle_ToString(hCommonPrefs, &pXml)); \
		common_prefs.fromString( UTF8_2QSTR((PRL_CONST_STR )pXml) ); \
		PrlBuffer_Free(pXml); \
	}

#define COMMON_PREFS_FROM_XML_OBJECT \
	CHECK_RET_CODE_EXP(PrlHandle_FromString( hCommonPrefs, QSTR2UTF8(common_prefs.toString()) ));

void PrlSrvManipulationsTest::testBackupSourceDefaultBackupServer()
{
	COMMON_PREFS_PREFACE
	GET_COMMON_PREFS;

	COMMON_PREFS_TO_XML_OBJECT;
	QString qsExpectedBackupServer = "some backup server";
	common_prefs.getBackupSourcePreferences()->setDefaultBackupServer(qsExpectedBackupServer);
	COMMON_PREFS_FROM_XML_OBJECT;

	QString qsBackupSourceDefaultBackupServer;
	PRL_EXTRACT_STRING_VALUE(qsBackupSourceDefaultBackupServer, hCommonPrefs, PrlDispCfg_GetDefaultBackupServer)
	QCOMPARE(qsBackupSourceDefaultBackupServer, qsExpectedBackupServer);

	qsExpectedBackupServer = "some another server";
	CHECK_RET_CODE_EXP(PrlDispCfg_SetDefaultBackupServer(hCommonPrefs, QSTR2UTF8(qsExpectedBackupServer)))
	PRL_EXTRACT_STRING_VALUE(qsBackupSourceDefaultBackupServer, hCommonPrefs, PrlDispCfg_GetDefaultBackupServer)
	QCOMPARE(qsBackupSourceDefaultBackupServer, qsExpectedBackupServer);

	//Check empty values accepted
	CHECK_RET_CODE_EXP(PrlDispCfg_SetDefaultBackupServer(hCommonPrefs, ""))
	PRL_EXTRACT_STRING_VALUE(qsBackupSourceDefaultBackupServer, hCommonPrefs, PrlDispCfg_GetDefaultBackupServer)
	QVERIFY(qsBackupSourceDefaultBackupServer.isEmpty());

	CHECK_RET_CODE_EXP(PrlDispCfg_SetDefaultBackupServer(hCommonPrefs, 0))
	PRL_EXTRACT_STRING_VALUE(qsBackupSourceDefaultBackupServer, hCommonPrefs, PrlDispCfg_GetDefaultBackupServer)
	QVERIFY(qsBackupSourceDefaultBackupServer.isEmpty());
}

void PrlSrvManipulationsTest::testBackupSourceDefaultBackupServerOnWrongParams()
{
	COMMON_PREFS_PREFACE
	GET_COMMON_PREFS;

	QString qsStr = "some text";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetDefaultBackupServer(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_SetDefaultBackupServer(m_ServerHandle, "some server"),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetDefaultBackupServer(hCommonPrefs, 0, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	COMMON_PREFS_TO_XML_OBJECT;
	common_prefs.getBackupSourcePreferences()->setDefaultBackupServer(qsStr);
	COMMON_PREFS_FROM_XML_OBJECT;
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetDefaultBackupServer(hCommonPrefs,
										(PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
}

void PrlSrvManipulationsTest::testBackupTargetDefaultDirectory()
{
	COMMON_PREFS_PREFACE
	GET_COMMON_PREFS;

	COMMON_PREFS_TO_XML_OBJECT;
	QString qsExpectedDirPath = "some backup dir path";
	common_prefs.getBackupTargetPreferences()->setDefaultBackupDirectory(qsExpectedDirPath);
	COMMON_PREFS_FROM_XML_OBJECT;

	QString qsBackupTargetDefaultDirectory;
	PRL_EXTRACT_STRING_VALUE(qsBackupTargetDefaultDirectory, hCommonPrefs,
								PrlDispCfg_GetDefaultBackupDirectory);
	QCOMPARE(qsBackupTargetDefaultDirectory, qsExpectedDirPath);

	qsExpectedDirPath = "some another backup dir path";
	CHECK_RET_CODE_EXP(PrlDispCfg_SetDefaultBackupDirectory(hCommonPrefs, QSTR2UTF8(qsExpectedDirPath)))
	PRL_EXTRACT_STRING_VALUE(qsBackupTargetDefaultDirectory, hCommonPrefs, PrlDispCfg_GetDefaultBackupDirectory)
	QCOMPARE(qsBackupTargetDefaultDirectory, qsExpectedDirPath);

	//Check empty values accepted
	CHECK_RET_CODE_EXP(PrlDispCfg_SetDefaultBackupDirectory(hCommonPrefs, ""))
	PRL_EXTRACT_STRING_VALUE(qsBackupTargetDefaultDirectory, hCommonPrefs, PrlDispCfg_GetDefaultBackupDirectory)
	QVERIFY(qsBackupTargetDefaultDirectory.isEmpty());

	CHECK_RET_CODE_EXP(PrlDispCfg_SetDefaultBackupDirectory(hCommonPrefs, 0))
	PRL_EXTRACT_STRING_VALUE(qsBackupTargetDefaultDirectory, hCommonPrefs, PrlDispCfg_GetDefaultBackupDirectory)
	QVERIFY(qsBackupTargetDefaultDirectory.isEmpty());
}

void PrlSrvManipulationsTest::testBackupTargetDefaultDirectoryOnWrongParams()
{
	COMMON_PREFS_PREFACE
	GET_COMMON_PREFS;

	QString qsStr = "some text";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetDefaultBackupDirectory(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_SetDefaultBackupDirectory(m_ServerHandle, "some dir path"),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetDefaultBackupDirectory(hCommonPrefs, 0, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	COMMON_PREFS_TO_XML_OBJECT;
	common_prefs.getBackupTargetPreferences()->setDefaultBackupDirectory(qsStr);
	COMMON_PREFS_FROM_XML_OBJECT;
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetDefaultBackupDirectory(hCommonPrefs,
										(PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
}

void PrlSrvManipulationsTest::testBackupTimeout()
{
	COMMON_PREFS_PREFACE
	GET_COMMON_PREFS;

	COMMON_PREFS_TO_XML_OBJECT;
	PRL_UINT32 nExpectedTimeout = 1000;
	common_prefs.getBackupSourcePreferences()->setTimeout(nExpectedTimeout);
	COMMON_PREFS_FROM_XML_OBJECT;

	PRL_UINT32 nTimeout;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetBackupTimeout(hCommonPrefs, &nTimeout))
	QCOMPARE(nTimeout, nExpectedTimeout);

	nExpectedTimeout = 0;
	CHECK_RET_CODE_EXP(PrlDispCfg_SetBackupTimeout(hCommonPrefs, nExpectedTimeout))
	CHECK_RET_CODE_EXP(PrlDispCfg_GetBackupTimeout(hCommonPrefs, &nTimeout))
	QCOMPARE(nTimeout, nExpectedTimeout);
}

void PrlSrvManipulationsTest::testBackupTimeoutOnWrongParams()
{
	COMMON_PREFS_PREFACE
	GET_COMMON_PREFS;

	PRL_UINT32 nTimeout;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetBackupTimeout(m_ServerHandle, &nTimeout),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetBackupTimeout(hCommonPrefs, 0),
									PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testEncryptionDefaultPluginId()
{
	COMMON_PREFS_PREFACE
	GET_COMMON_PREFS;

	COMMON_PREFS_TO_XML_OBJECT;
	QString qsExpected = "some backup dir path";
	common_prefs.getEncryptionPreferences()->setDefaultPluginId(qsExpected);
	COMMON_PREFS_FROM_XML_OBJECT;

	QString qsValue;
	PRL_EXTRACT_STRING_VALUE(qsValue, hCommonPrefs,
								PrlDispCfg_GetDefaultEncryptionPluginId);
	QCOMPARE(qsValue, qsExpected);

	qsExpected = "some another backup dir path";
	CHECK_RET_CODE_EXP(PrlDispCfg_SetDefaultEncryptionPluginId(hCommonPrefs, QSTR2UTF8(qsExpected)))
	PRL_EXTRACT_STRING_VALUE(qsValue, hCommonPrefs, PrlDispCfg_GetDefaultEncryptionPluginId)
	QCOMPARE(qsValue, qsExpected);

	//Check empty values accepted
	CHECK_RET_CODE_EXP(PrlDispCfg_SetDefaultEncryptionPluginId(hCommonPrefs, ""))
	PRL_EXTRACT_STRING_VALUE(qsValue, hCommonPrefs, PrlDispCfg_GetDefaultEncryptionPluginId)
	QVERIFY(qsValue.isEmpty());

	CHECK_RET_CODE_EXP(PrlDispCfg_SetDefaultEncryptionPluginId(hCommonPrefs, 0))
	PRL_EXTRACT_STRING_VALUE(qsValue, hCommonPrefs, PrlDispCfg_GetDefaultEncryptionPluginId)
	QVERIFY(qsValue.isEmpty());

	COMMON_PREFS_TO_XML_OBJECT;
	QVERIFY(common_prefs.getEncryptionPreferences()->isDefaultPluginIdWasChanged());
}

void PrlSrvManipulationsTest::testEncryptionDefaultPluginIdOnWrongParams()
{
	COMMON_PREFS_PREFACE
	GET_COMMON_PREFS;

	QString qsStr = "some text";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetDefaultEncryptionPluginId(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_SetDefaultEncryptionPluginId(m_ServerHandle, "some dir path"),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetDefaultEncryptionPluginId(hCommonPrefs, 0, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	COMMON_PREFS_TO_XML_OBJECT;
	common_prefs.getEncryptionPreferences()->setDefaultPluginId(qsStr);
	COMMON_PREFS_FROM_XML_OBJECT;
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetDefaultEncryptionPluginId(hCommonPrefs,
										(PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
}

void PrlSrvManipulationsTest::testBackupSourceUserLogin()
{
	COMMON_PREFS_PREFACE
	GET_COMMON_PREFS;

	COMMON_PREFS_TO_XML_OBJECT;
	QString qsExpectedUserLogin = "some user";
	common_prefs.getBackupSourcePreferences()->setLogin(qsExpectedUserLogin);
	COMMON_PREFS_FROM_XML_OBJECT;

	QString qsBackupSourceUserLogin;
	PRL_EXTRACT_STRING_VALUE(qsBackupSourceUserLogin, hCommonPrefs, PrlDispCfg_GetBackupUserLogin)
	QCOMPARE(qsBackupSourceUserLogin, qsExpectedUserLogin);

	qsExpectedUserLogin = "some another user";
	CHECK_RET_CODE_EXP(PrlDispCfg_SetBackupUserLogin(hCommonPrefs, QSTR2UTF8(qsExpectedUserLogin)))
	PRL_EXTRACT_STRING_VALUE(qsBackupSourceUserLogin, hCommonPrefs, PrlDispCfg_GetBackupUserLogin)
	QCOMPARE(qsBackupSourceUserLogin, qsExpectedUserLogin);

	//Check empty values accepted
	CHECK_RET_CODE_EXP(PrlDispCfg_SetBackupUserLogin(hCommonPrefs, ""))
	PRL_EXTRACT_STRING_VALUE(qsBackupSourceUserLogin, hCommonPrefs, PrlDispCfg_GetBackupUserLogin)
	QVERIFY(qsBackupSourceUserLogin.isEmpty());

	CHECK_RET_CODE_EXP(PrlDispCfg_SetBackupUserLogin(hCommonPrefs, 0))
	PRL_EXTRACT_STRING_VALUE(qsBackupSourceUserLogin, hCommonPrefs, PrlDispCfg_GetBackupUserLogin)
	QVERIFY(qsBackupSourceUserLogin.isEmpty());
}

void PrlSrvManipulationsTest::testBackupSourceUserLoginOnWrongParams()
{
	COMMON_PREFS_PREFACE
	GET_COMMON_PREFS;

	QString qsStr = "some text";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetBackupUserLogin(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_SetBackupUserLogin(m_ServerHandle, "some user"),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetBackupUserLogin(hCommonPrefs, 0, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	COMMON_PREFS_TO_XML_OBJECT;
	common_prefs.getBackupSourcePreferences()->setLogin(qsStr);
	COMMON_PREFS_FROM_XML_OBJECT;
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetBackupUserLogin(hCommonPrefs,
										(PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
}

void PrlSrvManipulationsTest::testBackupSourceUserPassword()
{
	COMMON_PREFS_PREFACE
	GET_COMMON_PREFS;

	COMMON_PREFS_TO_XML_OBJECT;
	QString qsExpectedUserPassword = "some pasword";
	common_prefs.getBackupSourcePreferences()->setPassword(qsExpectedUserPassword);
	QVERIFY(!common_prefs.getBackupSourcePreferences()->isPasswordChanged());
	COMMON_PREFS_FROM_XML_OBJECT;

	qsExpectedUserPassword = "some another password";
	CHECK_RET_CODE_EXP(PrlDispCfg_SetBackupUserPassword(hCommonPrefs, QSTR2UTF8(qsExpectedUserPassword)))
	COMMON_PREFS_TO_XML_OBJECT;
	QVERIFY(common_prefs.getBackupSourcePreferences()->isPasswordChanged());
	QCOMPARE(common_prefs.getBackupSourcePreferences()->getPassword(), qsExpectedUserPassword);

	//Check empty values accepted
	CHECK_RET_CODE_EXP(PrlDispCfg_SetBackupUserPassword(hCommonPrefs, ""))
	COMMON_PREFS_TO_XML_OBJECT;
	QVERIFY(common_prefs.getBackupSourcePreferences()->getPassword().isEmpty());

	CHECK_RET_CODE_EXP(PrlDispCfg_SetBackupUserPassword(hCommonPrefs, 0))
	COMMON_PREFS_TO_XML_OBJECT;
	QVERIFY(common_prefs.getBackupSourcePreferences()->getPassword().isEmpty());
}

void PrlSrvManipulationsTest::testBackupSourceUserPasswordOnWrongParams()
{
	COMMON_PREFS_PREFACE
	GET_COMMON_PREFS;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_SetBackupUserPassword(m_ServerHandle, "some password"),
									PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testBackupSourceUsePassword()
{
	COMMON_PREFS_PREFACE
	GET_COMMON_PREFS;

	COMMON_PREFS_TO_XML_OBJECT;
	PRL_BOOL bExpectedUsePassword = PRL_BOOL(common_prefs.getBackupSourcePreferences()->isUsePassword());
	COMMON_PREFS_FROM_XML_OBJECT;

	PRL_BOOL bBackupSourceUsePassword = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlDispCfg_IsBackupUserPasswordEnabled(hCommonPrefs, &bBackupSourceUsePassword))
	QVERIFY(bBackupSourceUsePassword == bExpectedUsePassword);

	bExpectedUsePassword = !bExpectedUsePassword;
	CHECK_RET_CODE_EXP(PrlDispCfg_SetBackupUserPasswordEnabled(hCommonPrefs, bExpectedUsePassword))
	CHECK_RET_CODE_EXP(PrlDispCfg_IsBackupUserPasswordEnabled(hCommonPrefs, &bBackupSourceUsePassword))
	QVERIFY(bBackupSourceUsePassword == bExpectedUsePassword);
}

void PrlSrvManipulationsTest::testBackupSourceUsePasswordOnWrongParams()
{
	COMMON_PREFS_PREFACE
	GET_COMMON_PREFS;

	PRL_BOOL bUsePassword = PRL_FALSE;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_IsBackupUserPasswordEnabled(m_ServerHandle, &bUsePassword),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_SetBackupUserPasswordEnabled(m_ServerHandle, PRL_TRUE),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_IsBackupUserPasswordEnabled(hCommonPrefs, 0),
									PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testVerboseLogEnabled()
{
	COMMON_PREFS_PREFACE
	GET_COMMON_PREFS;

	COMMON_PREFS_TO_XML_OBJECT;
	PRL_BOOL bExpectedLogLevel = common_prefs.getDebug()->isVerboseLogEnabled();
	QVERIFY(!common_prefs.getDebug()->isVerboseLogWasChanged());
	COMMON_PREFS_FROM_XML_OBJECT;

	PRL_BOOL bVerboseLogEnabled = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlDispCfg_IsVerboseLogEnabled(hCommonPrefs, &bVerboseLogEnabled))
	QVERIFY(bVerboseLogEnabled == bExpectedLogLevel);

	bExpectedLogLevel = !bExpectedLogLevel;
	CHECK_RET_CODE_EXP(PrlDispCfg_SetVerboseLogEnabled(hCommonPrefs, bExpectedLogLevel))
	CHECK_RET_CODE_EXP(PrlDispCfg_IsVerboseLogEnabled(hCommonPrefs, &bVerboseLogEnabled))
	QVERIFY(bVerboseLogEnabled == bExpectedLogLevel);

	COMMON_PREFS_TO_XML_OBJECT;
	QVERIFY(common_prefs.getDebug()->isVerboseLogWasChanged());
}

void PrlSrvManipulationsTest::testVerboseLogEnabledOnWrongParams()
{
	COMMON_PREFS_PREFACE
	GET_COMMON_PREFS;

	PRL_BOOL bVerboseLogEnabled = PRL_FALSE;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_IsVerboseLogEnabled(m_ServerHandle, &bVerboseLogEnabled),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_SetVerboseLogEnabled(m_ServerHandle, PRL_FALSE),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_IsVerboseLogEnabled(hCommonPrefs, 0),
									PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testCantToChangeUsbPreferencesThroughEditCommonPrefs()
{
	COMMON_PREFS_PREFACE
	GET_COMMON_PREFS;
	COMMON_PREFS_TO_XML_OBJECT;

	const QString sSystemName = "USB system name " + Uuid::createUuid().toString();
	const QString sFriendlyName = "USB friendly name " + Uuid::createUuid().toString();
	unsigned int nIndex = 0;

	CDispUsbIdentity *pUsbIdentity = new CDispUsbIdentity;
	pUsbIdentity->setSystemName(sSystemName);
	pUsbIdentity->setFriendlyName(sFriendlyName);
	pUsbIdentity->setIndex(nIndex);
	common_prefs.getUsbPreferences()->m_lstAuthenticUsbMap.append(pUsbIdentity);
	common_prefs.getUsbPreferences()->setUsbBlackList(common_prefs.getUsbPreferences()->getUsbBlackList()<<sSystemName);

	COMMON_PREFS_FROM_XML_OBJECT;
	SdkHandleWrap hJob(PrlSrv_CommonPrefsBeginEdit(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob)
	hJob.reset(PrlSrv_CommonPrefsCommit(m_ServerHandle, hCommonPrefs));
	CHECK_JOB_RET_CODE(hJob)

	GET_COMMON_PREFS;
	COMMON_PREFS_TO_XML_OBJECT;
	bool bFound = false;
	foreach(CDispUsbIdentity *pUsbItem, common_prefs.getUsbPreferences()->m_lstAuthenticUsbMap)
	{
		if (sSystemName == pUsbItem->getSystemName() && sFriendlyName == pUsbItem->getFriendlyName() && nIndex == pUsbItem->getIndex())
		{
			bFound = true;
			break;
		}
	}
	QVERIFY(!bFound);
	QVERIFY(!common_prefs.getUsbPreferences()->getUsbBlackList().contains(sSystemName));
}

void PrlSrvManipulationsTest::testGetPackedProblemReport()
{
	testLoginLocal();

    SdkHandleWrap hJob(PrlSrv_GetPackedProblemReport(m_ServerHandle, 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hProblemReport.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hProblemReport, PHT_PROBLEM_REPORT)

	QByteArray _buffer;
	PRL_UINT32 nBufferSize = 0;
	CHECK_RET_CODE_EXP(PrlReport_GetData(hProblemReport, 0, &nBufferSize))

	QVERIFY(nBufferSize != 0);

	_buffer.resize(nBufferSize);
	CHECK_RET_CODE_EXP(PrlReport_GetData(hProblemReport, (PRL_VOID_PTR)_buffer.data(), &nBufferSize))

	QString strBaseName;
	PRL_EXTRACT_STRING_VALUE(strBaseName, hProblemReport, PrlReport_GetArchiveFileName)

	QString sArchiveFilePath = m_sTestFsDirName1 + "/" + strBaseName + ".tar.gz";
	QVERIFY(QDir().mkdir(m_sTestFsDirName1));
	CPackedProblemReport _problem_report(sArchiveFilePath, _buffer);
	QVERIFY(_problem_report.isValid());
}

void PrlSrvManipulationsTest::testSendPackedProblemReport()
{
	testLoginLocal();
	SdkHandleWrap hJob(PrlSrv_GetPackedProblemReport(m_ServerHandle, 0));
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT*10))
	PRL_RESULT nRetCode;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nRetCode))
	if (PRL_ERR_UNRECOGNIZED_REQUEST == nRetCode)
		QSKIP("New problem report scheme not supported on server!", SkipAll);
	else
		CHECK_RET_CODE_EXP(nRetCode)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hProblemReport.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hProblemReport, PHT_PROBLEM_REPORT)

	hJob.reset(PrlApi_SendPackedProblemReport(hProblemReport, PRL_FALSE, 0, 0, 0, 0, g_nMaxProblemSendTimeout, 0, 0, 0));
	PRL_UINT32 nCommonTimeout = g_nMaxProblemSendTimeout;
	nRetCode = PRL_ERR_TIMEOUT;
	while (PRL_ERR_TIMEOUT == nRetCode && nCommonTimeout)
	{
		nRetCode = PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT);
		if (nCommonTimeout >= PRL_JOB_WAIT_TIMEOUT)
			nCommonTimeout -= PRL_JOB_WAIT_TIMEOUT;
		else
			nCommonTimeout = 0;
	}
	//We do not have any guarantees that problem report will be sent
	//always successfully. It's quite strong depend from host configuration
	//(proxy settings, firewall settings, Internet settings and etc.)
	//So the rest part of test is optional
	if (PRL_ERR_TIMEOUT == nRetCode)
	{
		PrlJob_Cancel(hJob);
		QSKIP("Couldn't to send problem report by some reasons", SkipAll);
	}
	CHECK_RET_CODE_EXP(nRetCode)
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nRetCode))
	if (PRL_SUCCEEDED(nRetCode))
	{
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
		QString sProblemReportId;
		PRL_EXTRACT_STRING_VALUE(sProblemReportId, hResult, PrlResult_GetParamAsString)
		bool bOk = false;
		PRL_UINT32 nProblemReportId = sProblemReportId.toUInt(&bOk);
		QVERIFY(nProblemReportId > 0);
		QVERIFY(bOk);
	}
}

void PrlSrvManipulationsTest::testSendPackedProblemReportOnWrongReport()
{
	testLoginLocal();

	CHECK_ASYNC_OP_FAILED(PrlApi_SendPackedProblemReport(PRL_INVALID_HANDLE, PRL_FALSE, 0, 0, 0, 0, 0, 0, 0, 0), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlApi_SendPackedProblemReport(m_ServerHandle, PRL_FALSE, 0, 0, 0, 0, 0, 0, 0, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testSendPackedProblemReportAtAsyncMode()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_GetPackedProblemReport(m_ServerHandle, 0));
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT*10))
	PRL_RESULT nRetCode;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nRetCode))
	if (PRL_ERR_UNRECOGNIZED_REQUEST == nRetCode)
		QSKIP("New problem report scheme not supported on server!", SkipAll);
	else
		CHECK_RET_CODE_EXP(nRetCode)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hProblemReport.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hProblemReport, PHT_PROBLEM_REPORT)

	CheckProblemReportSendParams _params;
	{
		QMutexLocker _lock(&_params.m_mutex);
		hJob.reset(PrlApi_SendPackedProblemReport(hProblemReport, PRL_FALSE, 0, 0, 0, 0, g_nMaxProblemSendTimeout, 0, CheckProblemReportSendCallback, &_params));
		_params.m_hJob = hJob;
		//We do not have any guarantees that problem report will be sent
		//always successfully. It's quite strong depend from host configuration
		//(proxy settings, firewall settings, Internet settings and etc.)
		//So the rest part of test is optional
		if(!_params.m_cond.wait(&_params.m_mutex, g_nMaxProblemSendTimeout))
		{
			PrlJob_Cancel(hJob);
			QSKIP("Couldn't to send problem report by some reasons", SkipAll);
		}
	}
	nRetCode = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nRetCode))
	if (PRL_SUCCEEDED(nRetCode))
	{
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
		QString sProblemReportId;
		PRL_EXTRACT_STRING_VALUE(sProblemReportId, hResult, PrlResult_GetParamAsString)
		bool bOk = false;
		PRL_UINT32 nProblemReportId = sProblemReportId.toUInt(&bOk);
		QVERIFY(nProblemReportId > 0);
		QVERIFY(bOk);
		QMutexLocker _lock(&_params.m_mutex);
		QVERIFY(_params.m_lstPercents.size());
		QCOMPARE(_params.m_lstPercents.toSet().size(), _params.m_lstPercents.size());
		foreach(PRL_UINT32 nPercents, _params.m_lstPercents)
		{
			if (!(nPercents > 0 && nPercents <= 100))
				WRITE_TRACE(DBG_FATAL, "nPercents=%u", nPercents);
			QVERIFY(nPercents > 0 && nPercents <= 100);
		}
	}
}

void PrlSrvManipulationsTest::testSendPackedProblemReportCancelOperation()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_GetPackedProblemReport(m_ServerHandle, 0));
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT*10))
	PRL_RESULT nRetCode;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nRetCode))
	if (PRL_ERR_UNRECOGNIZED_REQUEST == nRetCode)
		QSKIP("New problem report scheme not supported on server!", SkipAll);
	else
		CHECK_RET_CODE_EXP(nRetCode)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hProblemReport.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hProblemReport, PHT_PROBLEM_REPORT)

	CheckProblemReportSendParams _params;
	SdkHandleWrap hCancelJob;
	{
		QMutexLocker _lock(&_params.m_mutex);
		hJob.reset(PrlApi_SendPackedProblemReport(hProblemReport, PRL_FALSE, 0, 0, 0, 0, g_nMaxProblemSendTimeout, 0, CheckProblemReportSendCancelCallback, &_params));
		_params.m_hJob = hJob;
		if(_params.m_cond.wait(&_params.m_mutex, g_nMaxProblemSendTimeout))
		{
			if (!_params.m_bJobWasCompleted)
				hCancelJob.reset(PrlJob_Cancel(hJob));
		}
		else
			QFAIL("Job completion awaiting wasn't broken!");
		//Wait until all notifications will be done
		if (!_params.m_bJobWasCompleted)
			if(!_params.m_cond.wait(&_params.m_mutex, g_nMaxProblemSendTimeout))
				QFAIL("Final notification wasn't done!");
	}
	CHECK_JOB_RET_CODE(hCancelJob)
	CHECK_ASYNC_OP_FAILED(hJob, PRL_ERR_OPERATION_WAS_CANCELED)
}

void PrlSrvManipulationsTest::testRegiste3rdPartyVmOnWrongParams()
{
	testLoginLocal();

	const char *s3rdPartyPath = "/tmp/111.vmx";
	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))

	CHECK_ASYNC_OP_FAILED(PrlSrv_Register3rdPartyVm(m_ServerHandle, NULL, NULL, 0), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlSrv_Register3rdPartyVm(m_ServerHandle, "", NULL, 0), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlSrv_Register3rdPartyVm(PRL_INVALID_HANDLE, s3rdPartyPath, NULL, 0), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlSrv_Register3rdPartyVm(hVm, s3rdPartyPath, NULL, 0), PRL_ERR_INVALID_ARG)
}

namespace {
struct PrlUpdateLicenseTestData
{
	SdkHandleWrap m_EventHandle;
	SdkHandleWrap m_ServerHandle;
	QMutex m_Mutex;
	QWaitCondition m_Condition;
};

PRL_RESULT TestUpdateLicenseCallback( PRL_HANDLE _handle, PRL_VOID_PTR pUserData )
{
	SdkHandleWrap handle(_handle);
	PRL_HANDLE_TYPE nHandleType = PHT_ERROR;
	PRL_RESULT nRetCode = PrlHandle_GetType( handle, &nHandleType );
	if ( PRL_SUCCEEDED( nRetCode ) )
	{
		if ( nHandleType == PHT_EVENT )
		{
			PRL_EVENT_TYPE nEventType = PET_VM_INF_UNINITIALIZED_EVENT_CODE;
			nRetCode = PrlEvent_GetType( handle, &nEventType );
			if ( PRL_SUCCEEDED( nRetCode ) )
			{
				if ( PET_DSP_EVT_LICENSE_CHANGED == nEventType )
				{
					PrlUpdateLicenseTestData *pUpdateLicenseTestData = static_cast<PrlUpdateLicenseTestData *>( pUserData );
					QMutexLocker _lock( &pUpdateLicenseTestData->m_Mutex );
					//Unregister our callback now due it's not necessary now
					nRetCode = PrlSrv_UnregEventHandler( pUpdateLicenseTestData->m_ServerHandle, TestUpdateLicenseCallback, pUserData );
					if ( PRL_FAILED( nRetCode ) )
						WRITE_TRACE( DBG_FATAL, "Couldn't to unregister callback due error: %.8X '%s'", nRetCode,\
									prl_result_to_string( nRetCode ) );
					//Store received job handle
					pUpdateLicenseTestData->m_EventHandle = handle;
					pUpdateLicenseTestData->m_Condition.wakeAll();
				}
			}
			else
				WRITE_TRACE( DBG_FATAL, "Failed to get event type with error code: %.8X '%s'", nRetCode, prl_result_to_string( nRetCode ) );
		}
	}
	else
		WRITE_TRACE( DBG_FATAL, "Failed to get handle type with error code: %.8X '%s'", nRetCode, prl_result_to_string( nRetCode ) );

	return ( PRL_ERR_SUCCESS );
}

}

void PrlSrvManipulationsTest::testEventOnUpdateLicenseContainsLicenseHandle()
{
	SKIP_IF_EXTERNAL_BUILD

	PrlUpdateLicenseTestData _test_data;
	_test_data.m_ServerHandle = m_ServerHandle;
	{
		QMutexLocker _lock( &_test_data.m_Mutex );
		CHECK_RET_CODE_EXP(PrlSrv_RegEventHandler(m_ServerHandle, TestUpdateLicenseCallback, &_test_data ))
		_lock.unlock();
		testUpdateLicense();
		_lock.relock();
		if ( PRL_INVALID_HANDLE == _test_data.m_EventHandle )
			_test_data.m_Condition.wait( &_test_data.m_Mutex, PRL_JOB_WAIT_TIMEOUT );
		QVERIFY( PRL_INVALID_HANDLE != _test_data.m_EventHandle );
		SdkHandleWrap hEvtParam;
		CHECK_RET_CODE_EXP( PrlEvent_GetParamByName( _test_data.m_EventHandle, EVT_PARAM_VM_LICENSE, hEvtParam.GetHandlePtr() ) )
		SdkHandleWrap hLicense;
		CHECK_RET_CODE_EXP( PrlEvtPrm_ToHandle( hEvtParam, hLicense.GetHandlePtr() ) )
		CHECK_HANDLE_TYPE( hLicense, PHT_LICENSE )
	}
}

#define GET_OSES_MATRIX\
	SdkHandleWrap hOsesMatrix;\
	CHECK_RET_CODE_EXP(PrlSrv_GetSupportedOses(m_ServerHandle, hOsesMatrix.GetHandlePtr()))\
	CHECK_HANDLE_TYPE(hOsesMatrix, PHT_GUEST_OSES_MATRIX)

void PrlSrvManipulationsTest::testRemoteGetSupportedOsesCheckSupportedGuestsTypes()
{
	testLoginLocal();
	GET_OSES_MATRIX
	SdkHandleWrap hRemoteOsesTypesLst;
	CHECK_RET_CODE_EXP(PrlOsesMatrix_GetSupportedOsesTypes(hOsesMatrix, hRemoteOsesTypesLst.GetHandlePtr()))

	SdkHandleWrap hLocalOsesTypesLst;
	CHECK_RET_CODE_EXP(PrlApi_GetSupportedOsesTypes(PHO_UNKNOWN, hLocalOsesTypesLst.GetHandlePtr()))
	PRL_UINT8 nFakeParam = 0;
	QVERIFY(CompareTwoOpTypeLists(hRemoteOsesTypesLst, hLocalOsesTypesLst, nFakeParam));
}

#define CHECK_SUPPORTED_GUESTS_VERSIONS(guest_type)\
	{\
		SdkHandleWrap hRemoteOsesVersionsLst;\
		CHECK_RET_CODE_EXP(PrlOsesMatrix_GetSupportedOsesVersions(hOsesMatrix, guest_type, hRemoteOsesVersionsLst.GetHandlePtr()))\
		SdkHandleWrap hLocalOsesVersionsLst;\
		CHECK_RET_CODE_EXP(PrlApi_GetSupportedOsesVersions(PHO_UNKNOWN, guest_type, hLocalOsesVersionsLst.GetHandlePtr()))\
		PRL_UINT16 nRemoteDefaultOs = 0, nLocalDefaultOs = 0;\
		QVERIFY(CompareTwoOpTypeLists(hRemoteOsesVersionsLst, hLocalOsesVersionsLst, nRemoteDefaultOs));\
		CHECK_RET_CODE_EXP(PrlOsesMatrix_GetDefaultOsVersion(hOsesMatrix, guest_type, &nRemoteDefaultOs))\
		CHECK_RET_CODE_EXP(PrlApi_GetDefaultOsVersion(guest_type, &nLocalDefaultOs))\
		if (0 != nRemoteDefaultOs)/*https://bugzilla.sw.ru/show_bug.cgi?id=465314*/\
			QCOMPARE(nLocalDefaultOs, nRemoteDefaultOs);\
	}

void PrlSrvManipulationsTest::testRemoteGetSupportedOsesCheckSupportedGuestsVersions()
{
	testLoginLocal();
	GET_OSES_MATRIX

	CHECK_SUPPORTED_GUESTS_VERSIONS(PVS_GUEST_TYPE_WINDOWS)
	CHECK_SUPPORTED_GUESTS_VERSIONS(PVS_GUEST_TYPE_LINUX)
#ifdef _MAC_
	CHECK_SUPPORTED_GUESTS_VERSIONS(PVS_GUEST_TYPE_MACOS)
#endif
	CHECK_SUPPORTED_GUESTS_VERSIONS(PVS_GUEST_TYPE_FREEBSD)
#ifndef EXTERNALLY_AVAILABLE_BUILD
	CHECK_SUPPORTED_GUESTS_VERSIONS(PVS_GUEST_TYPE_CHROMEOS)
	CHECK_SUPPORTED_GUESTS_VERSIONS(PVS_GUEST_TYPE_ANDROID)
	CHECK_SUPPORTED_GUESTS_VERSIONS(PVS_GUEST_TYPE_MSDOS)
	CHECK_SUPPORTED_GUESTS_VERSIONS(PVS_GUEST_TYPE_NETWARE)
	CHECK_SUPPORTED_GUESTS_VERSIONS(PVS_GUEST_TYPE_SOLARIS)
	CHECK_SUPPORTED_GUESTS_VERSIONS(PVS_GUEST_TYPE_OS2)
	CHECK_SUPPORTED_GUESTS_VERSIONS(PVS_GUEST_TYPE_OTHER)
#else
	if (PAM_SERVER != TestConfig::getApplicationMode())
	{
		CHECK_SUPPORTED_GUESTS_VERSIONS(PVS_GUEST_TYPE_CHROMEOS)
		CHECK_SUPPORTED_GUESTS_VERSIONS(PVS_GUEST_TYPE_ANDROID)
		CHECK_SUPPORTED_GUESTS_VERSIONS(PVS_GUEST_TYPE_MSDOS)
		CHECK_SUPPORTED_GUESTS_VERSIONS(PVS_GUEST_TYPE_NETWARE)
		CHECK_SUPPORTED_GUESTS_VERSIONS(PVS_GUEST_TYPE_SOLARIS)
		CHECK_SUPPORTED_GUESTS_VERSIONS(PVS_GUEST_TYPE_OS2)
		CHECK_SUPPORTED_GUESTS_VERSIONS(PVS_GUEST_TYPE_OTHER)
	}
#endif
}

void PrlSrvManipulationsTest::testRemoteGetSupportedOsesOnWrongParams()
{
	SdkHandleWrap hVm, hOsesMatrix;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr()))

	//No connection
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrv_GetSupportedOses(m_ServerHandle, hOsesMatrix.GetHandlePtr()), PRL_ERR_NO_DATA)

	//Invalid or wrong server handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrv_GetSupportedOses(PRL_INVALID_HANDLE, hOsesMatrix.GetHandlePtr()), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrv_GetSupportedOses(hVm, hOsesMatrix.GetHandlePtr()), PRL_ERR_INVALID_ARG)

	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrv_GetSupportedOses(m_ServerHandle, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testOsesMatrixMethodsOnWrongParams()
{
	testLoginLocal();
	GET_OSES_MATRIX

	SdkHandleWrap hList;
	PRL_UINT16 nValue = 0;

	//Invalid or wrong OSes matrix object handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOsesMatrix_GetSupportedOsesTypes(PRL_INVALID_HANDLE, hList.GetHandlePtr()), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOsesMatrix_GetSupportedOsesTypes(m_ServerHandle, hList.GetHandlePtr()), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOsesMatrix_GetSupportedOsesVersions(PRL_INVALID_HANDLE, PVS_GUEST_TYPE_WINDOWS, hList.GetHandlePtr()), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOsesMatrix_GetSupportedOsesVersions(m_ServerHandle, PVS_GUEST_TYPE_WINDOWS, hList.GetHandlePtr()), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOsesMatrix_GetDefaultOsVersion(PRL_INVALID_HANDLE, PVS_GUEST_TYPE_WINDOWS, &nValue), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOsesMatrix_GetDefaultOsVersion(m_ServerHandle, PVS_GUEST_TYPE_WINDOWS, &nValue), PRL_ERR_INVALID_ARG)

	//Null pointers to buffers
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOsesMatrix_GetSupportedOsesTypes(hOsesMatrix, 0), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOsesMatrix_GetSupportedOsesVersions(hOsesMatrix, PVS_GUEST_TYPE_WINDOWS, 0), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOsesMatrix_GetDefaultOsVersion(hOsesMatrix, PVS_GUEST_TYPE_WINDOWS, 0), PRL_ERR_INVALID_ARG)

	//Incorrect guest type
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOsesMatrix_GetSupportedOsesVersions(hOsesMatrix, PVS_GUEST_TYPE_OTHER-1, hList.GetHandlePtr()), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlOsesMatrix_GetDefaultOsVersion(hOsesMatrix, PVS_GUEST_TYPE_OTHER-1, &nValue), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testRestrictions()
{
	CHECK_RET_CODE_EXP(PrlSrv_Create(m_ServerHandle.GetHandlePtr()));

// No restrictions

	SdkHandleWrap hEvtParam;
	PRL_BOOL bHasRestriction = PRL_TRUE;

	CHECK_RET_CODE_EXP(PrlSrv_HasRestriction(m_ServerHandle, PLRK_VM_CLONE, &bHasRestriction));
	QVERIFY(bHasRestriction == PRL_FALSE);

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrv_GetRestrictionInfo(
											m_ServerHandle, PLRK_VM_CLONE, hEvtParam.GetHandlePtr()),
										PRL_ERR_NO_DATA);

// Add some restrictions

	CVmEvent evt;

	evt.addEventParameter(
		new CVmEventParameter(PVE::Boolean,
							  QString("1"),
							  MAKE_EVT_PARAM_SESSION_RESTRICT_KEY(
								PRL_LICENSE_RESTR_KEY_TO_STRING(PLRK_VM_PAUSE) )) );

	evt.addEventParameter(
		new CVmEventParameter(PVE::UnsignedInt,
							  QString("123"),
							  MAKE_EVT_PARAM_SESSION_RESTRICT_KEY(
								PRL_LICENSE_RESTR_KEY_TO_STRING(PLRK_VM_MEMORY_LIMIT) )) );

	PrlHandle_FromString(m_ServerHandle, QSTR2UTF8(evt.toString()));

// Check 1 event parameter
	CHECK_RET_CODE_EXP(PrlSrv_HasRestriction(m_ServerHandle, PLRK_VM_PAUSE, &bHasRestriction));
	QVERIFY(bHasRestriction == PRL_TRUE);
	CHECK_RET_CODE_EXP(PrlSrv_GetRestrictionInfo(m_ServerHandle, PLRK_VM_PAUSE, hEvtParam.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hEvtParam, PHT_EVENT_PARAMETER);
	PRL_BOOL bValue = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlEvtPrm_ToBoolean(hEvtParam, &bValue));
	QVERIFY(bValue == PRL_TRUE);

// Check 2 event parameter
	CHECK_RET_CODE_EXP(PrlSrv_HasRestriction(m_ServerHandle, PLRK_VM_MEMORY_LIMIT, &bHasRestriction));
	QVERIFY(bHasRestriction == PRL_TRUE);
	CHECK_RET_CODE_EXP(PrlSrv_GetRestrictionInfo(m_ServerHandle, PLRK_VM_MEMORY_LIMIT, hEvtParam.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hEvtParam, PHT_EVENT_PARAMETER);
	PRL_UINT32 nValue = 0;
	CHECK_RET_CODE_EXP(PrlEvtPrm_ToUint32(hEvtParam, &nValue));
	QVERIFY(nValue == 123);

// TODO: Check 3 event parameter as list

}

void PrlSrvManipulationsTest::testRestrictionsOnWrongParams()
{
	CHECK_RET_CODE_EXP(PrlSrv_Create(m_ServerHandle.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr()));

	SdkHandleWrap hEvtParam;
	PRL_BOOL bHasRestriction;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrv_HasRestriction(m_VmHandle, PLRK_VM_VTD_AVAILABLE, &bHasRestriction),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrv_GetRestrictionInfo(
											m_VmHandle, PLRK_VM_VTD_AVAILABLE, hEvtParam.GetHandlePtr()),
										PRL_ERR_INVALID_ARG);

	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrv_HasRestriction(m_ServerHandle, PLRK_VM_REGISTER, NULL),
										PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrv_GetRestrictionInfo(m_ServerHandle, PLRK_VM_REGISTER, NULL),
										PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testCreateProblemReportForNewScheme()
{
	PRL_PROBLEM_REPORT_SCHEME nReportScheme = PRS_NEW_PACKED;
	PRL_PROBLEM_REPORT_TYPE nReportType = PRT_USER_DEFINED_ON_DISCONNECTED_SERVER;
	PRL_PROBLEM_REPORT_REASON nReportReason = PRR_SUPPORT_REQUEST;
	QString sUserName = "some user Name";
	QString sUserEmail = "some user E-mail";
	QString sUserDescription = "some report description";

	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(nReportScheme, hProblemReport.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hProblemReport, PHT_PROBLEM_REPORT)

	CHECK_RET_CODE_EXP(PrlReport_SetUserName(hProblemReport, QSTR2UTF8(sUserName)))
	CHECK_RET_CODE_EXP(PrlReport_SetUserEmail(hProblemReport, QSTR2UTF8(sUserEmail)))
	CHECK_RET_CODE_EXP(PrlReport_SetDescription(hProblemReport, QSTR2UTF8(sUserDescription)))
	CHECK_RET_CODE_EXP(PrlReport_SetReason(hProblemReport, nReportReason))
	CHECK_RET_CODE_EXP(PrlReport_SetType(hProblemReport, nReportType))

	SdkHandleWrap hJob(PrlReport_Assembly(hProblemReport, PPRF_ADD_SERVER_PART | PPRF_ADD_CLIENT_PART));
	CHECK_JOB_RET_CODE(hJob)

	QByteArray _buffer;
	PRL_UINT32 nBufferSize = 0;
	CHECK_RET_CODE_EXP(PrlReport_GetData(hProblemReport, 0, &nBufferSize))

	QVERIFY(nBufferSize != 0);

	_buffer.resize(nBufferSize);
	CHECK_RET_CODE_EXP(PrlReport_GetData(hProblemReport, (PRL_VOID_PTR)_buffer.data(), &nBufferSize))

	QString sArchiveFileName;
	PRL_EXTRACT_STRING_VALUE(sArchiveFileName, hProblemReport, PrlReport_GetArchiveFileName)

	m_sTestFsDirName1 = QDir::tempPath() + "/" + Uuid::createUuid().toString();
	QString sArchiveFilePath = m_sTestFsDirName1 + "/" + sArchiveFileName;
	QVERIFY(QDir().mkdir(m_sTestFsDirName1));
	CPackedProblemReport _problem_report(sArchiveFilePath, _buffer);

	QVERIFY(_problem_report.isValid());
	QCOMPARE(quint32(nReportType), quint32(_problem_report.getReportType()));
	QCOMPARE(quint32(nReportReason), quint32(_problem_report.getReportReason()));
	QCOMPARE(sUserName, _problem_report.getContactInfo()->getName());
	QCOMPARE(sUserEmail, _problem_report.getContactInfo()->getEMail());
	QCOMPARE(sUserDescription, _problem_report.getUserDefinedData()->getProblemDescription());
	QVERIFY(!_problem_report.getHostInfo().isEmpty());
	QVERIFY(!_problem_report.getMoreHostInfo().isEmpty());
	QVERIFY(!_problem_report.getAllProcesses().isEmpty());
	QVERIFY(!_problem_report.getAppConfig().isEmpty());
	QVERIFY(!_problem_report.getSystemLogs()->m_lstSystemLog.isEmpty());
	QCOMPARE(CProblemReportUtils::generateComputerModel(), _problem_report.getComputerModel());
	QVERIFY(!_problem_report.getProductName().isEmpty());
	QVERIFY(!_problem_report.getClientVersion().isEmpty());

	QString sClientInfoPath =	QFileInfo(sArchiveFilePath).absolutePath() + "/"
								+ QFileInfo(sArchiveFilePath).fileName().remove(".tar.gz") + "/"
								+ PR_PACKED_REP_CLIENT_INFO;
	QVERIFY(QFile::exists(sClientInfoPath));
	QFile _client_info_file(sClientInfoPath);
	QVERIFY(_client_info_file.open(QIODevice::ReadOnly));
	ClientInfo _client_info;
	_client_info.fromString(UTF8_2QSTR(_client_info_file.readAll()));
	CHECK_RET_CODE_EXP(_client_info.m_uiRcInit)
	if (PAM_SERVER == TestConfig::getApplicationMode())
	{
		QVERIFY(!_client_info.getHostInfo().isEmpty());
		QVERIFY(!_client_info.getEnvironment().isEmpty());
	}
}

void PrlSrvManipulationsTest::testCreateProblemReportForOldScheme()
{
	PRL_PROBLEM_REPORT_SCHEME nReportScheme = PRS_OLD_XML_BASED;
	PRL_PROBLEM_REPORT_TYPE nReportType = PRT_USER_DEFINED_ON_DISCONNECTED_SERVER;
	PRL_PROBLEM_REPORT_REASON nReportReason = PRR_SUPPORT_REQUEST;
	QString sUserName = "some user Name";
	QString sUserEmail = "some user E-mail";
	QString sUserDescription = "some report description";
    QString sCombinedReportId = "some combined report id";
	unsigned int nRating = 5;

	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(nReportScheme, hProblemReport.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hProblemReport, PHT_PROBLEM_REPORT)

	CHECK_RET_CODE_EXP(PrlReport_SetUserName(hProblemReport, QSTR2UTF8(sUserName)))
	CHECK_RET_CODE_EXP(PrlReport_SetUserEmail(hProblemReport, QSTR2UTF8(sUserEmail)))
	CHECK_RET_CODE_EXP(PrlReport_SetDescription(hProblemReport, QSTR2UTF8(sUserDescription)))
	CHECK_RET_CODE_EXP(PrlReport_SetReason(hProblemReport, nReportReason))
	CHECK_RET_CODE_EXP(PrlReport_SetType(hProblemReport, nReportType))

	SdkHandleWrap hJob(PrlReport_Assembly(hProblemReport, PPRF_ADD_SERVER_PART | PPRF_ADD_CLIENT_PART));
	CHECK_JOB_RET_CODE(hJob)

	QString sReportData;
	PRL_EXTRACT_STRING_VALUE(sReportData, hProblemReport, PrlReport_AsString)
	QVERIFY(!sReportData.isEmpty());

	CProblemReport _problem_report;
	_problem_report.fromString(sReportData);

	CHECK_RET_CODE_EXP(_problem_report.m_uiRcInit);
	QCOMPARE(quint32(nReportType), quint32(_problem_report.getReportType()));
	QCOMPARE(quint32(nReportReason), quint32(_problem_report.getReportReason()));
	QCOMPARE(sUserName, _problem_report.getContactInfo()->getName());
	QCOMPARE(sUserEmail, _problem_report.getContactInfo()->getEMail());
	QCOMPARE(sUserDescription, _problem_report.getUserDefinedData()->getProblemDescription());
	QVERIFY(!_problem_report.getHostInfo().isEmpty());
	QVERIFY(!_problem_report.getMoreHostInfo().isEmpty());
	QVERIFY(!_problem_report.getAllProcesses().isEmpty());
	QVERIFY(!_problem_report.getAppConfig().isEmpty());
	QCOMPARE(CProblemReportUtils::generateComputerModel(), _problem_report.getComputerModel());
	QVERIFY(!_problem_report.getProductName().isEmpty());
	QVERIFY(!_problem_report.getClientVersion().isEmpty());
    QVERIFY(!_problem_report.getClientInfo()->getCombinedReportId().isEmpty());
	QCOMPARE(nRating, _problem_report.getClientInfo()->getRating());

	if (PAM_SERVER == TestConfig::getApplicationMode())
	{
		QVERIFY(!_problem_report.getClientInfo()->getHostInfo().isEmpty());
		QVERIFY(!_problem_report.getClientInfo()->getEnvironment().isEmpty());
	}
}

void PrlSrvManipulationsTest::testAssemblyProblemReportWithClientInfoForNewScheme()
{
	testLoginLocal();
	SdkHandleWrap hJob(PrlSrv_GetPackedProblemReport(m_ServerHandle, 0));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hProblemReport.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hProblemReport, PHT_PROBLEM_REPORT)

	PRL_PROBLEM_REPORT_SCHEME nReportScheme = PRS_NEW_PACKED, nActualReportScheme = PRS_OLD_XML_BASED;
	CHECK_RET_CODE_EXP(PrlReport_GetScheme(hProblemReport, &nActualReportScheme))
	QCOMPARE(quint32(nReportScheme), quint32(nActualReportScheme));

	PRL_PROBLEM_REPORT_TYPE nReportType = PRT_USER_DEFINED_ON_DISCONNECTED_SERVER;
	PRL_PROBLEM_REPORT_REASON nReportReason = PRR_SUPPORT_REQUEST;
	QString sUserName = "some user Name";
	QString sUserEmail = "some user E-mail";
	QString sUserDescription = "some report description";

	CHECK_RET_CODE_EXP(PrlReport_SetUserName(hProblemReport, QSTR2UTF8(sUserName)))
	CHECK_RET_CODE_EXP(PrlReport_SetUserEmail(hProblemReport, QSTR2UTF8(sUserEmail)))
	CHECK_RET_CODE_EXP(PrlReport_SetDescription(hProblemReport, QSTR2UTF8(sUserDescription)))
	CHECK_RET_CODE_EXP(PrlReport_SetReason(hProblemReport, nReportReason))
	CHECK_RET_CODE_EXP(PrlReport_SetType(hProblemReport, nReportType))

	hJob.reset(PrlReport_Assembly(hProblemReport, PPRF_ADD_SERVER_PART | PPRF_ADD_CLIENT_PART));
	CHECK_JOB_RET_CODE(hJob)

	QByteArray _buffer;
	PRL_UINT32 nBufferSize = 0;
	CHECK_RET_CODE_EXP(PrlReport_GetData(hProblemReport, 0, &nBufferSize))

	QVERIFY(nBufferSize != 0);

	_buffer.resize(nBufferSize);
	CHECK_RET_CODE_EXP(PrlReport_GetData(hProblemReport, (PRL_VOID_PTR)_buffer.data(), &nBufferSize))

	QString sArchiveFileName;
	PRL_EXTRACT_STRING_VALUE(sArchiveFileName, hProblemReport, PrlReport_GetArchiveFileName)

	m_sTestFsDirName1 = QDir::tempPath() + "/" + Uuid::createUuid().toString();
	QString sArchiveFilePath = m_sTestFsDirName1 + "/" + sArchiveFileName;
	QVERIFY(QDir().mkdir(m_sTestFsDirName1));
	CPackedProblemReport _problem_report(sArchiveFilePath, _buffer);

	QVERIFY(_problem_report.isValid());
	QCOMPARE(quint32(nReportType), quint32(_problem_report.getReportType()));
	QCOMPARE(quint32(nReportReason), quint32(_problem_report.getReportReason()));
	QCOMPARE(sUserName, _problem_report.getContactInfo()->getName());
	QCOMPARE(sUserEmail, _problem_report.getContactInfo()->getEMail());
	QCOMPARE(sUserDescription, _problem_report.getUserDefinedData()->getProblemDescription());
	QVERIFY(!_problem_report.getHostInfo().isEmpty());
	QVERIFY(!_problem_report.getMoreHostInfo().isEmpty());
	QVERIFY(!_problem_report.getAllProcesses().isEmpty());
	QVERIFY(!_problem_report.getAppConfig().isEmpty());
	QVERIFY(!_problem_report.getSystemLogs()->m_lstSystemLog.isEmpty());
	QCOMPARE(CProblemReportUtils::generateComputerModel(), _problem_report.getComputerModel());
	QVERIFY(!_problem_report.getProductName().isEmpty());
	QVERIFY(!_problem_report.getClientVersion().isEmpty());

	QString sClientInfoPath =	QFileInfo(sArchiveFilePath).absolutePath() + "/"
								+ QFileInfo(sArchiveFilePath).fileName().remove(".tar.gz") + "/"
								+ PR_PACKED_REP_CLIENT_INFO;
	QVERIFY(QFile::exists(sClientInfoPath));
	QFile _client_info_file(sClientInfoPath);
	QVERIFY(_client_info_file.open(QIODevice::ReadOnly));
	ClientInfo _client_info;
	_client_info.fromString(UTF8_2QSTR(_client_info_file.readAll()));
	CHECK_RET_CODE_EXP(_client_info.m_uiRcInit)
	if (PAM_SERVER == TestConfig::getApplicationMode())
	{
		QVERIFY(!_client_info.getHostInfo().isEmpty());
		QVERIFY(!_client_info.getEnvironment().isEmpty());
	}
}

void PrlSrvManipulationsTest::testAssemblyProblemReportWithClientInfoForOldScheme()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_GetProblemReport(m_ServerHandle));
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT*10))
	PRL_RESULT nRetCode;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nRetCode))
	if (PRL_ERR_UNIMPLEMENTED == nRetCode)
		QSKIP("Old problem report scheme not supported on server!", SkipAll);
	else
		CHECK_RET_CODE_EXP(nRetCode)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hProblemReport.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hProblemReport, PHT_PROBLEM_REPORT)

	PRL_PROBLEM_REPORT_SCHEME nReportScheme = PRS_OLD_XML_BASED, nActualReportScheme = PRS_NEW_PACKED;
	CHECK_RET_CODE_EXP(PrlReport_GetScheme(hProblemReport, &nActualReportScheme))
	QCOMPARE(quint32(nReportScheme), quint32(nActualReportScheme));

	PRL_PROBLEM_REPORT_TYPE nReportType = PRT_USER_DEFINED_ON_DISCONNECTED_SERVER;
	PRL_PROBLEM_REPORT_REASON nReportReason = PRR_SUPPORT_REQUEST;
	QString sUserName = "some user Name";
	QString sUserEmail = "some user E-mail";
	QString sUserDescription = "some report description";

	CHECK_RET_CODE_EXP(PrlReport_SetUserName(hProblemReport, QSTR2UTF8(sUserName)))
	CHECK_RET_CODE_EXP(PrlReport_SetUserEmail(hProblemReport, QSTR2UTF8(sUserEmail)))
	CHECK_RET_CODE_EXP(PrlReport_SetDescription(hProblemReport, QSTR2UTF8(sUserDescription)))
	CHECK_RET_CODE_EXP(PrlReport_SetReason(hProblemReport, nReportReason))
	CHECK_RET_CODE_EXP(PrlReport_SetType(hProblemReport, nReportType))

	hJob.reset(PrlReport_Assembly(hProblemReport, PPRF_ADD_SERVER_PART | PPRF_ADD_CLIENT_PART));
	CHECK_JOB_RET_CODE(hJob)

	QString sReportData;
	PRL_EXTRACT_STRING_VALUE(sReportData, hProblemReport, PrlReport_AsString)
	QVERIFY(!sReportData.isEmpty());

	CProblemReport _problem_report;
	_problem_report.fromString(sReportData);

	CHECK_RET_CODE_EXP(_problem_report.m_uiRcInit);
	QCOMPARE(quint32(nReportType), quint32(_problem_report.getReportType()));
	QCOMPARE(quint32(nReportReason), quint32(_problem_report.getReportReason()));
	QCOMPARE(sUserName, _problem_report.getContactInfo()->getName());
	QCOMPARE(sUserEmail, _problem_report.getContactInfo()->getEMail());
	QCOMPARE(sUserDescription, _problem_report.getUserDefinedData()->getProblemDescription());
	QVERIFY(!_problem_report.getHostInfo().isEmpty());
	QVERIFY(!_problem_report.getMoreHostInfo().isEmpty());
	QVERIFY(!_problem_report.getAllProcesses().isEmpty());
	QVERIFY(!_problem_report.getAppConfig().isEmpty());
	QVERIFY(!_problem_report.getSystemLogs()->m_lstSystemLog.isEmpty());
	QCOMPARE(CProblemReportUtils::generateComputerModel(), _problem_report.getComputerModel());
	QVERIFY(!_problem_report.getProductName().isEmpty());
	QVERIFY(!_problem_report.getClientVersion().isEmpty());

	if (PAM_SERVER == TestConfig::getApplicationMode())
	{
		QVERIFY(!_problem_report.getClientInfo()->getHostInfo().isEmpty());
		QVERIFY(!_problem_report.getClientInfo()->getEnvironment().isEmpty());
	}
}

void PrlSrvManipulationsTest::testCreateProblemReportOnWrongParams()
{
	SdkHandleWrap hProblemReport;
	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlApi_CreateProblemReport(PRS_NEW_PACKED, 0), PRL_ERR_INVALID_ARG)
	//Wrong scheme value
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlApi_CreateProblemReport(PRL_PROBLEM_REPORT_SCHEME(-1), hProblemReport.GetHandlePtr()), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testProblemReportAsString()
{
	//Check for new report scheme
	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_NEW_PACKED, hProblemReport.GetHandlePtr()));

	SdkHandleWrap hJob(PrlReport_Assembly(hProblemReport, PPRF_ADD_SERVER_PART | PPRF_ADD_CLIENT_PART));
	CHECK_JOB_RET_CODE(hJob)

	QString sReportData;
	PRL_EXTRACT_STRING_VALUE(sReportData, hProblemReport, PrlReport_AsString)
	QVERIFY(!sReportData.isEmpty());

	//Check for old report scheme
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_OLD_XML_BASED, hProblemReport.GetHandlePtr()));

	hJob.reset(PrlReport_Assembly(hProblemReport, PPRF_ADD_SERVER_PART | PPRF_ADD_CLIENT_PART));
	CHECK_JOB_RET_CODE(hJob)

	PRL_EXTRACT_STRING_VALUE(sReportData, hProblemReport, PrlReport_AsString)
	QVERIFY(!sReportData.isEmpty());
}

void PrlSrvManipulationsTest::testProblemReportAsStringOnWrongParams()
{
	SdkHandleWrap hProblemReport;
	PRL_UINT32 nBufSize = 0;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_NEW_PACKED, hProblemReport.GetHandlePtr()))
	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_AsString(hProblemReport, 0, 0), PRL_ERR_INVALID_ARG)
	//Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_AsString(PRL_INVALID_HANDLE, 0, &nBufSize), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_AsString(m_ServerHandle, 0, &nBufSize), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testProblemReportGetDataOnWrongParams()
{
	SdkHandleWrap hProblemReport;
	PRL_UINT32 nBufSize = 0;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_NEW_PACKED, hProblemReport.GetHandlePtr()))

	//No data
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetData(hProblemReport, 0, &nBufSize), PRL_ERR_NO_DATA)

	SdkHandleWrap hJob(PrlReport_Assembly(hProblemReport, PPRF_ADD_CLIENT_PART));
	CHECK_JOB_RET_CODE(hJob)
	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetData(hProblemReport, 0, 0), PRL_ERR_INVALID_ARG)
	//Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetData(PRL_INVALID_HANDLE, 0, &nBufSize), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetData(m_ServerHandle, 0, &nBufSize), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testProblemReportAssemblyOnWrongParams()
{
	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_NEW_PACKED, hProblemReport.GetHandlePtr()))
	//Invalid handle
	CHECK_ASYNC_OP_FAILED(PrlReport_Assembly(PRL_INVALID_HANDLE, PPRF_ADD_CLIENT_PART), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlReport_Assembly(m_ServerHandle, PPRF_ADD_SERVER_PART), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testProblemReportGetScheme()
{
	PRL_PROBLEM_REPORT_SCHEME nReportScheme = PRS_NEW_PACKED, nActualReportScheme = PRS_OLD_XML_BASED;
	SdkHandleWrap hProblemReport;
	//Check new scheme
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(nReportScheme, hProblemReport.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlReport_GetScheme(hProblemReport, &nActualReportScheme))
	QCOMPARE(quint32(nReportScheme), quint32(nActualReportScheme));
	//Check old scheme
	nReportScheme = PRS_OLD_XML_BASED;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(nReportScheme, hProblemReport.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlReport_GetScheme(hProblemReport, &nActualReportScheme))
	QCOMPARE(quint32(nReportScheme), quint32(nActualReportScheme));
}

void PrlSrvManipulationsTest::testProblemReportGetSchemeOnWrongParams()
{
	SdkHandleWrap hProblemReport;
	PRL_PROBLEM_REPORT_SCHEME nReportScheme = PRS_NEW_PACKED;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_NEW_PACKED, hProblemReport.GetHandlePtr()))
	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetScheme(hProblemReport, 0), PRL_ERR_INVALID_ARG)
	//Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetScheme(PRL_INVALID_HANDLE, &nReportScheme), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetScheme(m_ServerHandle, &nReportScheme), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testProblemReportSetGetType()
{
	PRL_PROBLEM_REPORT_TYPE nReportType = PRT_USER_DEFINED_ON_DISCONNECTED_SERVER, nActualReportType = PRT_AUTOMATIC_VZ_STATISTICS_REPORT;
	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_NEW_PACKED, hProblemReport.GetHandlePtr()))

	CHECK_RET_CODE_EXP(PrlReport_SetType(hProblemReport, nReportType))
	CHECK_RET_CODE_EXP(PrlReport_GetType(hProblemReport, &nActualReportType))
	QCOMPARE(quint32(nReportType), quint32(nActualReportType));
}

void PrlSrvManipulationsTest::testProblemReportSetGetTypeOnWrongParams()
{
	SdkHandleWrap hProblemReport;
	PRL_PROBLEM_REPORT_TYPE nReportType = PRT_AUTOMATIC_VZ_STATISTICS_REPORT;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_NEW_PACKED, hProblemReport.GetHandlePtr()))
	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetType(hProblemReport, 0), PRL_ERR_INVALID_ARG)
	//Attempt to apply wrong value
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_SetType(hProblemReport, PRL_PROBLEM_REPORT_TYPE(-1)), PRL_ERR_INVALID_ARG)
	//Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetType(PRL_INVALID_HANDLE, &nReportType), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetType(m_ServerHandle, &nReportType), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_SetType(PRL_INVALID_HANDLE, PRT_AUTOMATIC_VZ_STATISTICS_REPORT), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_SetType(m_ServerHandle, PRT_USER_DEFINED_ON_DISCONNECTED_SERVER), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testProblemReportSetGetReason()
{
	PRL_PROBLEM_REPORT_REASON nReportReason = PRR_SUPPORT_REQUEST, nActualReportReason = PRR_VIDEO_3D_GRAPHICS;
	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_NEW_PACKED, hProblemReport.GetHandlePtr()))

	CHECK_RET_CODE_EXP(PrlReport_SetReason(hProblemReport, nReportReason))
	CHECK_RET_CODE_EXP(PrlReport_GetReason(hProblemReport, &nActualReportReason))
	QCOMPARE(quint32(nReportReason), quint32(nActualReportReason));
}

void PrlSrvManipulationsTest::testProblemReportSetGetReasonOnWrongParams()
{
	SdkHandleWrap hProblemReport;
	PRL_PROBLEM_REPORT_REASON nReportReason = PRR_PRODUCT_REGISTRATION;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_NEW_PACKED, hProblemReport.GetHandlePtr()))
	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetReason(hProblemReport, 0), PRL_ERR_INVALID_ARG)
	//Attempt to apply wrong value
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_SetReason(hProblemReport, PRL_PROBLEM_REPORT_REASON(-1)), PRL_ERR_INVALID_ARG)
	//Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetReason(PRL_INVALID_HANDLE, &nReportReason), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetReason(m_ServerHandle, &nReportReason), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_SetReason(PRL_INVALID_HANDLE, PRR_PRODUCT_REGISTRATION), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_SetReason(m_ServerHandle, PRR_SUPPORT_REQUEST), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testProblemReportSetGetUserName()
{
	QString sUserName = "Some user Name", sActualUserName;
	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_NEW_PACKED, hProblemReport.GetHandlePtr()))

	CHECK_RET_CODE_EXP(PrlReport_SetUserName(hProblemReport, QSTR2UTF8(sUserName)))
	PRL_EXTRACT_STRING_VALUE(sActualUserName, hProblemReport, PrlReport_GetUserName)
	QCOMPARE(sUserName, sActualUserName);

	//Empty values
	CHECK_RET_CODE_EXP(PrlReport_SetUserName(hProblemReport, ""))
	PRL_EXTRACT_STRING_VALUE(sActualUserName, hProblemReport, PrlReport_GetUserName)
	QVERIFY(sActualUserName.isEmpty());

	CHECK_RET_CODE_EXP(PrlReport_SetUserName(hProblemReport, 0))
	PRL_EXTRACT_STRING_VALUE(sActualUserName, hProblemReport, PrlReport_GetUserName)
	QVERIFY(sActualUserName.isEmpty());
}

void PrlSrvManipulationsTest::testProblemReportSetGetUserNameOnWrongParams()
{
	SdkHandleWrap hProblemReport;
	PRL_UINT32 nBufSize = 0;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_NEW_PACKED, hProblemReport.GetHandlePtr()))
	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetUserName(hProblemReport, 0, 0), PRL_ERR_INVALID_ARG)
	//Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetUserName(PRL_INVALID_HANDLE, 0, &nBufSize), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetUserName(m_ServerHandle, 0, &nBufSize), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_SetUserName(PRL_INVALID_HANDLE, ""), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_SetUserName(m_ServerHandle, ""), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testProblemReportSetGetUserEmail()
{
	QString sUserEmail = "Some user E-mail", sActualUserEmail;
	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_NEW_PACKED, hProblemReport.GetHandlePtr()))

	CHECK_RET_CODE_EXP(PrlReport_SetUserEmail(hProblemReport, QSTR2UTF8(sUserEmail)))
	PRL_EXTRACT_STRING_VALUE(sActualUserEmail, hProblemReport, PrlReport_GetUserEmail)
	QCOMPARE(sUserEmail, sActualUserEmail);

	//Empty values
	CHECK_RET_CODE_EXP(PrlReport_SetUserEmail(hProblemReport, ""))
	PRL_EXTRACT_STRING_VALUE(sActualUserEmail, hProblemReport, PrlReport_GetUserEmail)
	QVERIFY(sActualUserEmail.isEmpty());

	CHECK_RET_CODE_EXP(PrlReport_SetUserEmail(hProblemReport, 0))
	PRL_EXTRACT_STRING_VALUE(sActualUserEmail, hProblemReport, PrlReport_GetUserEmail)
	QVERIFY(sActualUserEmail.isEmpty());
}

void PrlSrvManipulationsTest::testProblemReportSetGetUserEmailOnWrongParams()
{
	SdkHandleWrap hProblemReport;
	PRL_UINT32 nBufSize = 0;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_NEW_PACKED, hProblemReport.GetHandlePtr()))
	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetUserEmail(hProblemReport, 0, 0), PRL_ERR_INVALID_ARG)
	//Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetUserEmail(PRL_INVALID_HANDLE, 0, &nBufSize), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetUserEmail(m_ServerHandle, 0, &nBufSize), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_SetUserEmail(PRL_INVALID_HANDLE, ""), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_SetUserEmail(m_ServerHandle, ""), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testProblemReportSetGetDescription()
{
	QString sDescription = "Some description", sActualDescription;
	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_NEW_PACKED, hProblemReport.GetHandlePtr()))

	CHECK_RET_CODE_EXP(PrlReport_SetDescription(hProblemReport, QSTR2UTF8(sDescription)))
	PRL_EXTRACT_STRING_VALUE(sActualDescription, hProblemReport, PrlReport_GetDescription)
	QCOMPARE(sDescription, sActualDescription);

	//Empty values
	CHECK_RET_CODE_EXP(PrlReport_SetDescription(hProblemReport, ""))
	PRL_EXTRACT_STRING_VALUE(sActualDescription, hProblemReport, PrlReport_GetDescription)
	QVERIFY(sActualDescription.isEmpty());

	CHECK_RET_CODE_EXP(PrlReport_SetDescription(hProblemReport, 0))
	PRL_EXTRACT_STRING_VALUE(sActualDescription, hProblemReport, PrlReport_GetDescription)
	QVERIFY(sActualDescription.isEmpty());
}

void PrlSrvManipulationsTest::testProblemReportSetGetDescriptionOnWrongParams()
{
	SdkHandleWrap hProblemReport;
	PRL_UINT32 nBufSize = 0;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_NEW_PACKED, hProblemReport.GetHandlePtr()))
	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetDescription(hProblemReport, 0, 0), PRL_ERR_INVALID_ARG)
	//Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetDescription(PRL_INVALID_HANDLE, 0, &nBufSize), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetDescription(m_ServerHandle, 0, &nBufSize), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_SetDescription(PRL_INVALID_HANDLE, ""), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_SetDescription(m_ServerHandle, ""), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testProblemReportGetArchiveFileName()
{
	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_NEW_PACKED, hProblemReport.GetHandlePtr()))

	QString sArchiveFileName;
	PRL_EXTRACT_STRING_VALUE(sArchiveFileName, hProblemReport, PrlReport_GetArchiveFileName)
	QVERIFY(!sArchiveFileName.isEmpty());
}

void PrlSrvManipulationsTest::testProblemReportGetArchiveFileNameOnWrongParams()
{
	SdkHandleWrap hProblemReport;
	PRL_UINT32 nBufSize = 0;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_NEW_PACKED, hProblemReport.GetHandlePtr()))
	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetArchiveFileName(hProblemReport, 0, 0), PRL_ERR_INVALID_ARG)
	//Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetArchiveFileName(PRL_INVALID_HANDLE, 0, &nBufSize), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetArchiveFileName(m_ServerHandle, 0, &nBufSize), PRL_ERR_INVALID_ARG)
	//Try to call method for old scheme
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_OLD_XML_BASED, hProblemReport.GetHandlePtr()))
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlReport_GetArchiveFileName(hProblemReport, 0, &nBufSize), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testProblemReportSendForNewScheme()
{
	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_NEW_PACKED, hProblemReport.GetHandlePtr()))

	SdkHandleWrap hJob(PrlReport_Assembly(hProblemReport, PPRF_ADD_SERVER_PART | PPRF_ADD_CLIENT_PART));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlReport_Send(hProblemReport, PRL_FALSE, 0, 0, 0, 0, g_nMaxProblemSendTimeout, 0, 0, 0));
	PRL_UINT32 nCommonTimeout = g_nMaxProblemSendTimeout;
	PRL_RESULT nRetCode = PRL_ERR_TIMEOUT;
	while (PRL_ERR_TIMEOUT == nRetCode && nCommonTimeout)
	{
		nRetCode = PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT);
		if (nCommonTimeout >= PRL_JOB_WAIT_TIMEOUT)
			nCommonTimeout -= PRL_JOB_WAIT_TIMEOUT;
		else
			nCommonTimeout = 0;
	}
	//We do not have any guarantees that problem report will be sent
	//always successfully. It's quite strong depend from host configuration
	//(proxy settings, firewall settings, Internet settings and etc.)
	//So the rest part of test is optional
	if (PRL_ERR_TIMEOUT == nRetCode)
	{
		PrlJob_Cancel(hJob);
		QSKIP("Couldn't to send problem report by some reasons", SkipAll);
	}
	CHECK_RET_CODE_EXP(nRetCode)
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nRetCode))
	if (PRL_SUCCEEDED(nRetCode))
	{
		SdkHandleWrap hResult;
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
		QString sProblemReportId;
		PRL_EXTRACT_STRING_VALUE(sProblemReportId, hResult, PrlResult_GetParamAsString)
		bool bOk = false;
		PRL_UINT32 nProblemReportId = sProblemReportId.toUInt(&bOk);
		QVERIFY(nProblemReportId > 0);
		QVERIFY(bOk);
	}
}

void PrlSrvManipulationsTest::testProblemReportSendForOldScheme()
{
	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_OLD_XML_BASED, hProblemReport.GetHandlePtr()))

	SdkHandleWrap hJob(PrlReport_Assembly(hProblemReport, PPRF_ADD_SERVER_PART | PPRF_ADD_CLIENT_PART));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlReport_Send(hProblemReport, PRL_FALSE, 0, 0, 0, 0, g_nMaxProblemSendTimeout, 0, 0, 0));
	PRL_UINT32 nCommonTimeout = g_nMaxProblemSendTimeout;
	PRL_RESULT nRetCode = PRL_ERR_TIMEOUT;
	while (PRL_ERR_TIMEOUT == nRetCode && nCommonTimeout)
	{
		nRetCode = PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT);
		if (nCommonTimeout >= PRL_JOB_WAIT_TIMEOUT)
			nCommonTimeout -= PRL_JOB_WAIT_TIMEOUT;
		else
			nCommonTimeout = 0;
	}
	//We do not have any guarantees that problem report will be sent
	//always successfully. It's quite strong depend from host configuration
	//(proxy settings, firewall settings, Internet settings and etc.)
	//So the rest part of test is optional
	if (PRL_ERR_TIMEOUT == nRetCode)
	{
		PrlJob_Cancel(hJob);
		QSKIP("Couldn't to send problem report by some reasons", SkipAll);
	}
	CHECK_RET_CODE_EXP(nRetCode)
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nRetCode))
	if (PRL_SUCCEEDED(nRetCode))
	{
		SdkHandleWrap hResult;
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
		QString sProblemReportId;
		PRL_EXTRACT_STRING_VALUE(sProblemReportId, hResult, PrlResult_GetParamAsString)
		bool bOk = false;
		PRL_UINT32 nProblemReportId = sProblemReportId.toUInt(&bOk);
		QVERIFY(nProblemReportId > 0);
		QVERIFY(bOk);
	}
}

void PrlSrvManipulationsTest::testProblemReportSendOnWrongParams()
{
	CHECK_ASYNC_OP_FAILED(PrlReport_Send(PRL_INVALID_HANDLE, PRL_FALSE, 0, 0, 0, 0, 0, 0, 0, 0), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlReport_Send(m_ServerHandle, PRL_FALSE, 0, 0, 0, 0, 0, 0, 0, 0), PRL_ERR_INVALID_ARG)

	//Try to use send packed report API with old XML based scheme
	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlApi_CreateProblemReport(PRS_OLD_XML_BASED, hProblemReport.GetHandlePtr()))
	CHECK_ASYNC_OP_FAILED(PrlApi_SendPackedProblemReport(hProblemReport, PRL_FALSE, 0, 0, 0, 0, 0, 0, 0, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetAssemblyAndSendProblemReport()
{
	testLoginLocal();

	SdkHandleWrap hJob(PrlSrv_GetPackedProblemReport(m_ServerHandle, 0));
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT*10))
	PRL_RESULT nRetCode = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nRetCode))
	if (PRL_ERR_UNRECOGNIZED_REQUEST == nRetCode)
	{
		//New problem report scheme not supported on server - try to request old one
		hJob.reset(PrlSrv_GetProblemReport(m_ServerHandle));
		CHECK_JOB_RET_CODE(hJob)
	}
	else
		CHECK_RET_CODE_EXP(nRetCode)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hProblemReport;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hProblemReport.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hProblemReport, PHT_PROBLEM_REPORT)

	hJob.reset(PrlReport_Assembly(hProblemReport, PPRF_ADD_CLIENT_PART));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlReport_Send(hProblemReport, PRL_FALSE, 0, 0, 0, 0, g_nMaxProblemSendTimeout, 0, 0, 0));
	PRL_UINT32 nCommonTimeout = g_nMaxProblemSendTimeout;
	nRetCode = PRL_ERR_TIMEOUT;
	while (PRL_ERR_TIMEOUT == nRetCode && nCommonTimeout)
	{
		nRetCode = PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT);
		if (nCommonTimeout >= PRL_JOB_WAIT_TIMEOUT)
			nCommonTimeout -= PRL_JOB_WAIT_TIMEOUT;
		else
			nCommonTimeout = 0;
	}
	//We do not have any guarantees that problem report will be sent
	//always successfully. It's quite strong depend from host configuration
	//(proxy settings, firewall settings, Internet settings and etc.)
	//So the rest part of test is optional
	if (PRL_ERR_TIMEOUT == nRetCode)
	{
		PrlJob_Cancel(hJob);
		QSKIP("Couldn't to send problem report by some reasons", SkipAll);
	}
	CHECK_RET_CODE_EXP(nRetCode)
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nRetCode))
	if (PRL_SUCCEEDED(nRetCode))
	{
		SdkHandleWrap hResult;
		CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
		QString sProblemReportId;
		PRL_EXTRACT_STRING_VALUE(sProblemReportId, hResult, PrlResult_GetParamAsString)
		bool bOk = false;
		PRL_UINT32 nProblemReportId = sProblemReportId.toUInt(&bOk);
		QVERIFY(nProblemReportId > 0);
		QVERIFY(bOk);
	}
}

void PrlSrvManipulationsTest::testApplianceOnWrongParams()
{
	SdkHandleWrap hAppCfg;
	CHECK_RET_CODE_EXP( PrlAppliance_Create(hAppCfg.GetHandlePtr()) );
	CHECK_HANDLE_TYPE(hAppCfg, PHT_APPLIANCE_CONFIG);

	// Wrong handles
	CHECK_ASYNC_OP_FAILED(PrlSrv_InstallAppliance(m_ServerHandle, m_ServerHandle, "", 0),
							PRL_ERR_INVALID_ARG);
	CHECK_ASYNC_OP_FAILED(PrlSrv_InstallAppliance(hAppCfg, hAppCfg, NULL, 0),
							PRL_ERR_INVALID_ARG);
	CHECK_ASYNC_OP_FAILED(PrlSrv_CancelInstallAppliance(m_ServerHandle, m_ServerHandle, 0),
							PRL_ERR_INVALID_ARG);
	CHECK_ASYNC_OP_FAILED(PrlSrv_CancelInstallAppliance(hAppCfg, hAppCfg, 0),
							PRL_ERR_INVALID_ARG);
	CHECK_ASYNC_OP_FAILED(PrlSrv_StopInstallAppliance(m_ServerHandle, m_ServerHandle, 0),
							PRL_ERR_INVALID_ARG);
	CHECK_ASYNC_OP_FAILED(PrlSrv_StopInstallAppliance(hAppCfg, hAppCfg, 0),
							PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlAppliance_Create(NULL), PRL_ERR_INVALID_ARG);
}

namespace {
void CheckRemoteListenInterfaceHelper( bool &bCheckRemoteListenInterface )
{
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
	SdkHandleWrap hJob(PrlSrv_Login(hServer, TestConfig::getRemoteHostName(),	TestConfig::getUserLogin(),
									TestConfig::getUserPassword(), NULL, 0, 0, PSL_HIGH_SECURITY));
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT))
	PRL_RESULT nRetCode = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nRetCode))
	bCheckRemoteListenInterface =  (PRL_ERR_CANT_CONNECT_TO_DISPATCHER != nRetCode);
}

bool CheckRemoteListenInterface()
{
	bool bCheckRemoteListenInterface = false;
	CheckRemoteListenInterfaceHelper( bCheckRemoteListenInterface );
	return ( bCheckRemoteListenInterface );
}

}

void PrlSrvManipulationsTest::testGetCtTemplateList()
{
	bool bSkip = true;
#ifdef _LIN_
	if (CVzHelper::is_vz_running())
		bSkip = false;
#endif
	if (bSkip)
		QSKIP("Doesn't make sense for non-PSBM mode", SkipAll);

	testLoginLocal();

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

		PRL_VOID_PTR pBuffer = 0;
		CHECK_RET_CODE_EXP(PrlResult_GetParamToken(hResult, i, &pBuffer))
		CtTemplate _ct_tmpl;
		_ct_tmpl.fromString(UTF8_2QSTR((const char *)pBuffer));
		PrlBuffer_Free(pBuffer);
		CHECK_RET_CODE_EXP(_ct_tmpl.m_uiRcInit)

		QString sName;
		PRL_EXTRACT_STRING_VALUE(sName, hCtTmplInfo, PrlCtTemplate_GetName)
		QVERIFY(_ct_tmpl.getName() == sName);

		QString sDescription;
		PRL_EXTRACT_STRING_VALUE(sDescription, hCtTmplInfo, PrlCtTemplate_GetDescription)
		QVERIFY(_ct_tmpl.getDescription() == sDescription);

		QString sVersion;
		PRL_EXTRACT_STRING_VALUE(sVersion, hCtTmplInfo, PrlCtTemplate_GetVersion)
		QVERIFY(_ct_tmpl.getVersion() == sVersion);

		PRL_CT_TEMPLATE_TYPE nType;
		CHECK_RET_CODE_EXP(PrlCtTemplate_GetType(hCtTmplInfo, &nType))
		QVERIFY(_ct_tmpl.getType() == nType);

		if (nType == PCT_TYPE_EZ_APP) {
			QString sOsTemplate;
			PRL_EXTRACT_STRING_VALUE(sOsTemplate, hCtTmplInfo,
					PrlCtTemplate_GetOsTemplate)
			QVERIFY(_ct_tmpl.getOsTemplate() == sOsTemplate);
		}

		PRL_UINT32 nOsType;
		CHECK_RET_CODE_EXP(PrlCtTemplate_GetOsType(hCtTmplInfo, &nOsType))
		QVERIFY(_ct_tmpl.getOsType() == nOsType);

		PRL_UINT32 nOsVersion;
		CHECK_RET_CODE_EXP(PrlCtTemplate_GetOsVersion(hCtTmplInfo, &nOsVersion))
		QVERIFY(_ct_tmpl.getOsVersion() == nOsVersion);

		PRL_CPU_MODE nCpuMode;
		CHECK_RET_CODE_EXP(PrlCtTemplate_GetCpuMode(hCtTmplInfo, &nCpuMode))
		QVERIFY(_ct_tmpl.getCpuMode() == nCpuMode);

		PRL_BOOL bCached;
		CHECK_RET_CODE_EXP(PrlCtTemplate_IsCached(hCtTmplInfo, &bCached))
		QVERIFY(_ct_tmpl.isCached() == bool(bCached));
	}
}

#ifdef _LIN_
#define GET_FIRST_TEMPLATE \
	if (!CVzHelper::is_vz_running()) \
		QSKIP("Doesn't make sense for non-PSBM mode", SkipAll); \
	testLoginLocal(); \
	SdkHandleWrap hJob(PrlSrv_GetCtTemplateList(m_ServerHandle, 0)); \
	CHECK_JOB_RET_CODE(hJob) \
	SdkHandleWrap _hResult; \
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, _hResult.GetHandlePtr())) \
	PRL_UINT32 _nParamsCount; \
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(_hResult, &_nParamsCount)) \
	QVERIFY(_nParamsCount > 0); \
	SdkHandleWrap _hCtTmplInfo; \
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(_hResult, 0, _hCtTmplInfo.GetHandlePtr())); \
	CHECK_HANDLE_TYPE(_hCtTmplInfo, PHT_CT_TEMPLATE);
#else
#define GET_FIRST_TEMPLATE \
		SdkHandleWrap _hCtTmplInfo; \
		QSKIP("Doesn't make sense for non-PSBM mode", SkipAll);
#endif

void PrlSrvManipulationsTest::testGetCtTemplateListOnWrongParams()
{
	GET_FIRST_TEMPLATE
	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetCtTemplateList(PRL_INVALID_HANDLE, 0), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetCtTemplateList(_hCtTmplInfo, 0), PRL_ERR_INVALID_ARG)
}

#define FAKE_CT_TMPL_NAME "testRCtTemplate"

#ifdef _LIN_
static int createFakeCtTemplate(QString &qsName, QString &qsFakeTmplName)
{
	QStringList tmplList = qsName.split('-');

	int ver_index = 1;
	int type_index = 2;
	/* Exception - fedora is "fedora-core" till 17th version.*/
	if (tmplList[0] == QString("fedora") && tmplList[ver_index].toInt() < 17)
	{
		tmplList[0].append("-core");
		ver_index = 2;
		type_index = 3;
	}

	QString sOrigDir(QString("/vz/template/%1/%2/%3").arg(tmplList[0])
		.arg(tmplList[ver_index]).arg(tmplList[type_index]));
	QString sNewDir(QString("/vz/template/" FAKE_CT_TMPL_NAME "/%1/")
		.arg(tmplList[ver_index]));
	qsFakeTmplName = QString(FAKE_CT_TMPL_NAME "-%1-%2").arg(tmplList[ver_index])
		.arg(tmplList[type_index]);

	QDir rootDir(QDir::root());
	bool bCreated = rootDir.mkpath(sNewDir);
	if (!bCreated)
		return 1;

	QStringList lstCmd;
	lstCmd.append("-a");
	lstCmd.append(sOrigDir);
	lstCmd.append(sNewDir);
	return QProcess::execute(QString("cp"), lstCmd);
}

static void cleanupFakeCtTemplate(QString &qsFakeTmplName)
{
	QStringList tmplList = qsFakeTmplName.split('-');
	QDir rootDir(QDir::root());
	QVERIFY(rootDir.rmpath(QString("/vz/template/%1/%2").arg(tmplList[0])
		.arg(tmplList[1])));
}
#endif

void PrlSrvManipulationsTest::testRemoveCtTemplate()
{
	/* 1. Prepare fake template for test
	 * 1.1 Find first OS template */
	GET_FIRST_TEMPLATE

#ifdef _LIN_
	PRL_CT_TEMPLATE_TYPE nType;
	PRL_UINT32 i = 0;
	for (; i < _nParamsCount; ++i) {
		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(_hResult, i,
					_hCtTmplInfo.GetHandlePtr()))
		CHECK_RET_CODE_EXP(PrlCtTemplate_GetType(_hCtTmplInfo, &nType))
		if (nType == PCT_TYPE_EZ_OS)
			break;
	}
	if (i == _nParamsCount)
		QSKIP("No Container OS templates installed", SkipAll);

	QString sOsName;
	PRL_EXTRACT_STRING_VALUE(sOsName, _hCtTmplInfo, PrlCtTemplate_GetName);

	/* 1.2 Clone found OS template */
	QString sFakeTmplName;
	int ret = createFakeCtTemplate(sOsName, sFakeTmplName);
	QVERIFY(ret == 0);


	/* 2. Remove all fake templates one by one */
	hJob.reset(PrlSrv_GetCtTemplateList(m_ServerHandle, 0));
	CHECK_JOB_RET_CODE(hJob)
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, _hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(_hResult, &_nParamsCount))
	QVERIFY(_nParamsCount > 0);

	for (PRL_UINT32 i = 0; i < _nParamsCount; ++i) {
		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(_hResult, i,
					_hCtTmplInfo.GetHandlePtr()))

		CHECK_RET_CODE_EXP(PrlCtTemplate_GetType(_hCtTmplInfo, &nType))
		if (nType != PCT_TYPE_EZ_APP)
			continue;

		QString sOsTemplate;
		PRL_EXTRACT_STRING_VALUE(sOsTemplate, _hCtTmplInfo,
				PrlCtTemplate_GetOsTemplate);
		if (sOsTemplate != sFakeTmplName)
			continue;

		QString sName;
		PRL_EXTRACT_STRING_VALUE(sName, _hCtTmplInfo, PrlCtTemplate_GetName);
		SdkHandleWrap hJob(PrlSrv_RemoveCtTemplate(m_ServerHandle, QSTR2UTF8(sName),
				QSTR2UTF8(sFakeTmplName), 0));
		CHECK_JOB_RET_CODE(hJob)
	}

	hJob.reset(PrlSrv_RemoveCtTemplate(m_ServerHandle,
			QSTR2UTF8(sFakeTmplName), NULL, 0));
	CHECK_JOB_RET_CODE(hJob)

	/* 3. Check that fake template removed */
	hJob.reset(PrlSrv_GetCtTemplateList(m_ServerHandle, 0));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	PRL_UINT32 nParamsCount;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nParamsCount))
	for (PRL_UINT32 i = 0; i < nParamsCount; ++i) {
		SdkHandleWrap hCtTmplInfo;
		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, i, hCtTmplInfo.GetHandlePtr()));

		QString sTmpName;
		PRL_EXTRACT_STRING_VALUE(sTmpName, hCtTmplInfo, PrlCtTemplate_GetName);
		CHECK_RET_CODE_EXP(PrlCtTemplate_GetType(_hCtTmplInfo, &nType))
		if (nType == PCT_TYPE_EZ_OS)
			QVERIFY(sTmpName != sFakeTmplName);
		else if (nType == PCT_TYPE_EZ_APP) {
			QString sOsTmpName;
			PRL_EXTRACT_STRING_VALUE(sOsTmpName, hCtTmplInfo,
					PrlCtTemplate_GetOsTemplate);
			QVERIFY(sOsTmpName != sFakeTmplName);
		}
	}

	cleanupFakeCtTemplate(sFakeTmplName);
#endif
}

void PrlSrvManipulationsTest::testRemoveCtTemplateOnWrongParams()
{
	GET_FIRST_TEMPLATE

	// Wrong handle
	PRL_CHAR sName[STR_BUF_LENGTH], sOsName[STR_BUF_LENGTH];
	CHECK_ASYNC_OP_FAILED(PrlSrv_RemoveCtTemplate(PRL_INVALID_HANDLE, sName,
				sOsName, 0), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlSrv_RemoveCtTemplate(m_ServerHandle, NULL,
				sOsName, 0), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlSrv_RemoveCtTemplate(_hCtTmplInfo, sName,
				sOsName, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetCtNameOnWrongParams()
{
	GET_FIRST_TEMPLATE
	PRL_UINT32 nBufSize = 0;

	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetName(_hCtTmplInfo, 0, 0), PRL_ERR_INVALID_ARG)
	//Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetName(PRL_INVALID_HANDLE, 0, &nBufSize), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetName(m_ServerHandle, 0, &nBufSize), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetCtDescriptionOnWrongParams()
{
	GET_FIRST_TEMPLATE
	PRL_UINT32 nBufSize = 0;

	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetDescription(_hCtTmplInfo, 0, 0), PRL_ERR_INVALID_ARG)
	//Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetDescription(PRL_INVALID_HANDLE, 0, &nBufSize), \
			PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetDescription(m_ServerHandle, 0, &nBufSize), \
			PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetCtVersionOnWrongParams()
{
	GET_FIRST_TEMPLATE
	PRL_UINT32 nBufSize = 0;

	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetVersion(_hCtTmplInfo, 0, 0), PRL_ERR_INVALID_ARG)
	//Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetVersion(PRL_INVALID_HANDLE, 0, &nBufSize), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetVersion(m_ServerHandle, 0, &nBufSize), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetCtOsTemplateOnWrongParams()
{
	GET_FIRST_TEMPLATE
	PRL_UINT32 nBufSize = 0;

	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetOsTemplate(_hCtTmplInfo, 0, 0), PRL_ERR_INVALID_ARG)
	//Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetOsTemplate(PRL_INVALID_HANDLE, 0, &nBufSize), \
			PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetOsTemplate(m_ServerHandle, 0, &nBufSize), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetCtTypeOnWrongParams()
{
	GET_FIRST_TEMPLATE
	PRL_CT_TEMPLATE_TYPE nType;

	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetType(_hCtTmplInfo, 0), PRL_ERR_INVALID_ARG)
	//Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetType(PRL_INVALID_HANDLE, &nType), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetType(m_ServerHandle, &nType), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetCtOsTypeOnWrongParams()
{
	GET_FIRST_TEMPLATE
	PRL_UINT32 nOsType;

	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetOsType(_hCtTmplInfo, 0), PRL_ERR_INVALID_ARG)
	//Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetOsType(PRL_INVALID_HANDLE, &nOsType), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetOsType(m_ServerHandle, &nOsType), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetCtOsVersionOnWrongParams()
{
	GET_FIRST_TEMPLATE
	PRL_UINT32 nOsVersion;

	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetOsVersion(_hCtTmplInfo, 0), PRL_ERR_INVALID_ARG)
	//Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetOsVersion(PRL_INVALID_HANDLE, &nOsVersion), \
			PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetOsVersion(m_ServerHandle, &nOsVersion), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testGetCtCpuModeOnWrongParams()
{
	GET_FIRST_TEMPLATE
	PRL_CPU_MODE nCpuMode;

	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetCpuMode(_hCtTmplInfo, 0), PRL_ERR_INVALID_ARG)
	//Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetCpuMode(PRL_INVALID_HANDLE, &nCpuMode), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_GetCpuMode(m_ServerHandle, &nCpuMode), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testIsCtCachedOnWrongParams()
{
	GET_FIRST_TEMPLATE
	PRL_BOOL bCached;

	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_IsCached(_hCtTmplInfo, 0), PRL_ERR_INVALID_ARG)
	//Invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_IsCached(PRL_INVALID_HANDLE, &bCached), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCtTemplate_IsCached(m_ServerHandle, &bCached), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testLoginToNotAcceptableHost()
{
	if (!TestConfig::isServerMode())
		QSKIP("Skipping test due functionality is not supported at desktop mode", SkipAll);

	QTime t;
	t.start();
	m_JobHandle.reset(PrlSrv_Login(m_ServerHandle, "8.8.8.8", TestConfig::getUserLogin(), TestConfig::getUserPassword(), NULL, 0, PRL_JOB_WAIT_TIMEOUT, PSL_HIGH_SECURITY));
	QVERIFY(t.elapsed() < PRL_JOB_WAIT_TIMEOUT);
	SdkHandleWrap hJob(PrlJob_Cancel(m_JobHandle));
	CHECK_JOB_RET_CODE(hJob)
	CHECK_ASYNC_OP_FAILED(m_JobHandle, PRL_ERR_OPERATION_WAS_CANCELED)
}

void PrlSrvManipulationsTest::testLoginToNotAcceptableHostAsyncMode()
{
	DEFINE_CHECK_CALLBACK(check_callback, m_ServerHandle, PET_VM_INF_UNINITIALIZED_EVENT_CODE, PrlSrv_UnregEventHandler, PJOC_SRV_LOGIN);

	CHECK_RET_CODE_EXP(PrlSrv_RegEventHandler(m_ServerHandle, event_callback, &check_callback)) ;

	SdkHandleWrap hJob;
	bool bRes = false;
	{
		QMutexLocker _lock(&check_callback.mutex);
		QTime t;
		t.start();
		hJob.reset(PrlSrv_Login(m_ServerHandle, "8.8.8.8", TestConfig::getUserLogin(), TestConfig::getUserPassword(), NULL, 0, PRL_JOB_WAIT_TIMEOUT, PSL_HIGH_SECURITY));
		quint32 nElapsedMsecs = t.elapsed();
		m_JobHandle.reset(PrlJob_Cancel(hJob));
		bRes = check_callback.condition.wait(&check_callback.mutex, PRL_JOB_WAIT_TIMEOUT);
		QVERIFY(nElapsedMsecs < PRL_JOB_WAIT_TIMEOUT);
	}

	QVERIFY(bRes);
	QMutexLocker _lock(&check_callback.mutex);

	QVERIFY(check_callback.got_job == hJob);

	CHECK_ASYNC_OP_FAILED(hJob, PRL_ERR_OPERATION_WAS_CANCELED)
}

void PrlSrvManipulationsTest::testCopyCtTemplateOnWrongParams()
{
	PRL_CHAR sTmplName[STR_BUF_LENGTH];
	PRL_CHAR sOsTmplName[STR_BUF_LENGTH];
	PRL_CHAR sTargetServerHostname[STR_BUF_LENGTH];
	PRL_CHAR sTargetServerSessionUuid[STR_BUF_LENGTH];

	// Wrong source server handle
	CHECK_ASYNC_OP_FAILED(PrlSrv_CopyCtTemplate(
		PRL_INVALID_HANDLE, sTmplName, sOsTmplName, sTargetServerHostname, 0, sTargetServerSessionUuid, 0, 0),
		PRL_ERR_INVALID_ARG)
	// Wrong template name
	CHECK_ASYNC_OP_FAILED(PrlSrv_CopyCtTemplate(
		m_ServerHandle, NULL, sOsTmplName, sTargetServerHostname, 0, sTargetServerSessionUuid, 0, 0),
		PRL_ERR_INVALID_ARG)
	// Wrong target server hostname
	CHECK_ASYNC_OP_FAILED(PrlSrv_CopyCtTemplate(
		m_ServerHandle, sOsTmplName, sOsTmplName, NULL, 0, sTargetServerSessionUuid, 0, 0),
		PRL_ERR_INVALID_ARG)
	// Wrong target server session UUID
	CHECK_ASYNC_OP_FAILED(PrlSrv_CopyCtTemplate(
		m_ServerHandle, sOsTmplName, sOsTmplName, sTargetServerHostname, 0, NULL, 0, 0),
		PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testIsFeatureSupported()
{
	testLoginLocal();
	PRL_BOOL bIsSupported = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlSrv_IsFeatureSupported( m_ServerHandle, PFSM_SATA_HOTPLUG_SUPPORT, &bIsSupported ))
	if (TestConfig::isServerMode())
		QVERIFY(PRL_TRUE == bIsSupported);
	else
		QVERIFY(PRL_FALSE == bIsSupported);
	CHECK_RET_CODE_EXP(PrlSrv_IsFeatureSupported( m_ServerHandle, (PRL_FEATURES_MATRIX)USHRT_MAX, &bIsSupported ))
	QVERIFY(PRL_FALSE == bIsSupported);
}

void PrlSrvManipulationsTest::testIsFeatureSupportedOnWrongParams()
{
	testLoginLocal();
	SdkHandleWrap hVm;
	PRL_BOOL bIsSupported = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm( m_ServerHandle, hVm.GetHandlePtr() ))
	//Wrong/invalid handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrv_IsFeatureSupported( PRL_INVALID_HANDLE, PFSM_SATA_HOTPLUG_SUPPORT, &bIsSupported ), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrv_IsFeatureSupported( hVm, PFSM_SATA_HOTPLUG_SUPPORT, &bIsSupported ), PRL_ERR_INVALID_ARG)
	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrv_IsFeatureSupported( m_ServerHandle, PFSM_SATA_HOTPLUG_SUPPORT, 0 ), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testRefreshPluginsOnWrongParams()
{
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm( m_ServerHandle, m_VmHandle.GetHandlePtr() ))

	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlSrv_RefreshPlugins(m_VmHandle, 0), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testGetPluginsListOnWrongParams()
{
	testLoginLocal();

	CHECK_RET_CODE_EXP(PrlSrv_CreateVm( m_ServerHandle, m_VmHandle.GetHandlePtr() ))

	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetPluginsList(m_VmHandle, "", 0), PRL_ERR_INVALID_ARG);
	// Null GUID
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetPluginsList(m_ServerHandle, "{00000000-0000-0000-0000-000000000000}", 0),
							PRL_ERR_BAD_PARAMETERS);
}

void PrlSrvManipulationsTest::testGetDiskFreeSpace()
{
	LOGIN_TO_SERVER
	m_JobHandle.reset(PrlSrv_GetDiskFreeSpace(m_ServerHandle, "/", 0));
	CHECK_RET_CODE_EXP(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT))
	PRL_RESULT _ret_code = PRL_ERR_UNINITIALIZED;
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(m_JobHandle, &_ret_code))
	CHECK_RET_CODE_EXP(_ret_code)
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, m_ResultHandle.GetHandlePtr()))
	QString sSize, sTimestamp;
	PRL_EXTRACT_STRING_VALUE(sSize, m_ResultHandle, PrlResult_GetParamAsString);
	PRL_EXTRACT_STRING_VALUE(sTimestamp, m_ResultHandle, PrlResult_GetParamAsString);
	QVERIFY(sSize.size());
}

void PrlSrvManipulationsTest::testGetDiskFreeSpaceOnWrongParams()
{
	LOGIN_TO_SERVER
	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetDiskFreeSpace(PRL_INVALID_HANDLE, "/", 0), PRL_ERR_INVALID_ARG);
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetDiskFreeSpace(m_ServerHandle, 0, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testConvert3rdPartyVmShouldReturnVmConfig()
{
	if (TestConfig::isServerMode())
		QSKIP("Skipping test due to the interaction is not supported by the server mode", SkipAll);

	const QString vmFileName("SDKTest_vm_fusion4_no_os");
	const QString testDataArchiveName = QString("%1.tgz").arg(vmFileName);

	QVERIFY(QDir().mkdir(m_sTestConvertDirName));
	QCOMPARE( ExtractArchive( testDataArchiveName.toUtf8().constData(),
							  m_sTestConvertDirName.toUtf8().constData(),
							  0, 0 ), 0 );

	QString srcVmDirPath = QString( "%1/%2.vmwarevm" )
		.arg(m_sTestConvertDirName)
		.arg(vmFileName);

	QString dstVmDirPath( m_sTestConvertDirName );


	testLoginLocal();


	SdkHandleWrap hJob(PrlSrv_Register3rdPartyVm( m_ServerHandle,
													srcVmDirPath.toUtf8().constData(),
													dstVmDirPath.toUtf8().constData(),
													0 ));

	WAIT_AND_ANSWER_SERVER_QUESTION( m_ServerHandle, PRL_QUESTION_CONVERT_VM_CANT_DETECT_OS,
									 PET_ANSWER_YES, 60 * 1000 );

	CHECK_JOB_RET_CODE(hJob);

	// get result
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP( PrlJob_GetResult( hJob, hResult.GetHandlePtr() ) );

	// Verify and get 0-th parameter as string.
	QString strParam0;
	PRL_EXTRACT_STRING_VALUE_BY_INDEX(strParam0, hResult, 0, PrlResult_GetParamByIndexAsString);

	// varify 0-th parameter is vm config
	CVmConfiguration cfg;
	CHECK_RET_CODE_EXP(cfg.fromString(strParam0));

	// Auto unreg/delete vm
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr()));
	CHECK_HANDLE_TYPE(m_VmHandle, PHT_VIRTUAL_MACHINE);
}

void PrlSrvManipulationsTest::testAllowMultiplePMC()
{
	RECEIVE_DISP_CONFIG;
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT;

	PRL_BOOL bVal = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlDispCfg_IsAllowMultiplePMC(hDispConfig, &bVal));
	QCOMPARE((bool )bVal, _dcp.getWorkspacePreferences()->isAllowMultiplePMC());

	CHECK_RET_CODE_EXP(PrlDispCfg_SetAllowMultiplePMC(hDispConfig, ! bVal));
	PRL_BOOL bNewVal = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlDispCfg_IsAllowMultiplePMC(hDispConfig, &bNewVal));
	QCOMPARE((bool )bNewVal, (bool )(! bVal));
}

void PrlSrvManipulationsTest::testAllowMultiplePMCOnWrongParams()
{
	RECEIVE_DISP_CONFIG;

	PRL_BOOL bVal = PRL_FALSE;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_IsAllowMultiplePMC(m_ServerHandle, &bVal),
			PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_SetAllowMultiplePMC(m_ServerHandle, PRL_TRUE),
			PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_IsAllowMultiplePMC(hDispConfig, NULL),
			PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testDispConfigGetCpuFeaturesMask()
{
	RECEIVE_DISP_CONFIG
	PRL_CPU_FEATURES_MASK Masks;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetCpuFeaturesMask(hDispConfig, &Masks))
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	QCOMPARE(_dcp.getCpuPreferences()->getFEATURES_MASK(), Masks.nFEATURES_MASK);
	QCOMPARE(_dcp.getCpuPreferences()->getEXT_FEATURES_MASK(), Masks.nEXT_FEATURES_MASK);
	QCOMPARE(_dcp.getCpuPreferences()->getEXT_80000001_ECX_MASK(), Masks.nEXT_80000001_ECX_MASK);
	QCOMPARE(_dcp.getCpuPreferences()->getEXT_80000001_EDX_MASK(), Masks.nEXT_80000001_EDX_MASK);
	QCOMPARE(_dcp.getCpuPreferences()->getEXT_80000007_EDX_MASK(), Masks.nEXT_80000007_EDX_MASK);
	QCOMPARE(_dcp.getCpuPreferences()->getEXT_80000008_EAX(), Masks.nEXT_80000008_EAX);
	QCOMPARE(_dcp.getCpuPreferences()->getEXT_00000007_EBX_MASK(), Masks.nEXT_00000007_EBX_MASK);
}

void PrlSrvManipulationsTest::testDispConfigGetCpuFeaturesMaskEx()
{
	RECEIVE_DISP_CONFIG
	SdkHandleWrap hMasks;
	PRL_UINT32 nValue;

	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT
	CHECK_RET_CODE_EXP(PrlDispCfg_GetCpuFeaturesMaskEx(hDispConfig, hMasks.GetHandlePtr()))

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hMasks, PCFE_FEATURES, &nValue))
	QCOMPARE(_dcp.getCpuPreferences()->getFEATURES_MASK(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hMasks, PCFE_EXT_FEATURES, &nValue))
	QCOMPARE(_dcp.getCpuPreferences()->getEXT_FEATURES_MASK(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hMasks, PCFE_EXT_80000001_ECX, &nValue))
	QCOMPARE(_dcp.getCpuPreferences()->getEXT_80000001_ECX_MASK(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hMasks, PCFE_EXT_80000001_EDX, &nValue))
	QCOMPARE(_dcp.getCpuPreferences()->getEXT_80000001_EDX_MASK(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hMasks, PCFE_EXT_80000007_EDX, &nValue))
	QCOMPARE(_dcp.getCpuPreferences()->getEXT_80000007_EDX_MASK(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hMasks, PCFE_EXT_80000008_EAX, &nValue))
	QCOMPARE(_dcp.getCpuPreferences()->getEXT_80000008_EAX(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hMasks, PCFE_EXT_00000007_EBX, &nValue))
	QCOMPARE(_dcp.getCpuPreferences()->getEXT_00000007_EBX_MASK(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hMasks, PCFE_EXT_0000000D_EAX, &nValue))
	QCOMPARE(_dcp.getCpuPreferences()->getEXT_0000000D_EAX_MASK(), nValue);
}

void PrlSrvManipulationsTest::testDispConfigSetCpuFeaturesMask()
{
	RECEIVE_DISP_CONFIG
	PRL_CPU_FEATURES_MASK newMasks;
	memset(&newMasks, 0xFF, sizeof(newMasks));
	CHECK_RET_CODE_EXP(PrlDispCfg_SetCpuFeaturesMask(hDispConfig, &newMasks))
	PRL_CPU_FEATURES_MASK Masks;
	CHECK_RET_CODE_EXP(PrlDispCfg_GetCpuFeaturesMask(hDispConfig, &Masks))
	QCOMPARE(newMasks.nFEATURES_MASK, Masks.nFEATURES_MASK);
	QCOMPARE(newMasks.nEXT_FEATURES_MASK, Masks.nEXT_FEATURES_MASK);
	QCOMPARE(newMasks.nEXT_80000001_ECX_MASK, Masks.nEXT_80000001_ECX_MASK);
	QCOMPARE(newMasks.nEXT_80000001_EDX_MASK, Masks.nEXT_80000001_EDX_MASK);
	QCOMPARE(newMasks.nEXT_80000007_EDX_MASK, Masks.nEXT_80000007_EDX_MASK);
	QCOMPARE(newMasks.nEXT_80000008_EAX, Masks.nEXT_80000008_EAX);
	QCOMPARE(newMasks.nEXT_00000007_EBX_MASK, Masks.nEXT_00000007_EBX_MASK);
}

void PrlSrvManipulationsTest::testDispConfigSetCpuFeaturesMaskEx()
{
	RECEIVE_DISP_CONFIG

	SdkHandleWrap hNewMasks;
	SdkHandleWrap hResMasks;
	const PRL_UINT32 nMask = 0xFF;

	CHECK_RET_CODE_EXP(PrlDispCfg_GetCpuFeaturesMaskEx(hDispConfig, hNewMasks.GetHandlePtr()))

	for (int i = PCFE_FEATURES; i != PCFE_MAX; ++i)
	{
		CHECK_RET_CODE_EXP(PrlCpuFeatures_SetValue(hNewMasks,
				static_cast<PRL_CPU_FEATURES_EX>(i), nMask))
	}

	CHECK_RET_CODE_EXP(PrlDispCfg_SetCpuFeaturesMaskEx(hDispConfig, hNewMasks))
	CHECK_RET_CODE_EXP(PrlDispCfg_GetCpuFeaturesMaskEx(hDispConfig, hResMasks.GetHandlePtr()))

	for (int i = PCFE_FEATURES; i != PCFE_MAX; ++i)
	{
		PRL_UINT32 nValue = 0;
		CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hResMasks,
				static_cast<PRL_CPU_FEATURES_EX>(i), &nValue))
		QCOMPARE(nMask, nValue);
	}
}

void PrlSrvManipulationsTest::testDispConfigGetCpuFeaturesMaskOnWrongParams()
{
	RECEIVE_DISP_CONFIG
	PRL_CPU_FEATURES_MASK Masks;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetCpuFeaturesMask(
			PRL_INVALID_HANDLE, &Masks), PRL_ERR_INVALID_ARG)
	// wrong pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetCpuFeaturesMask(
			hDispConfig, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testDispConfigGetCpuFeaturesMaskExOnWrongParams()
{
	RECEIVE_DISP_CONFIG
	SdkHandleWrap hMasks;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetCpuFeaturesMaskEx(
			0, hMasks.GetHandlePtr()), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetCpuFeaturesMaskEx(
			hDispConfig, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testDispConfigSetCpuFeaturesMaskExOnWrongParams()
{
	RECEIVE_DISP_CONFIG
	SdkHandleWrap hMasks;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_SetCpuFeaturesMaskEx(
			0, hMasks), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_SetCpuFeaturesMaskEx(
			hDispConfig, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testSrvConfigGetCpuFeatures()
{
	RECEIVE_SERVER_HW_INFO
	PRL_CPU_FEATURES CpuFeatures;
	CHECK_RET_CODE_EXP(PrlSrvCfg_GetCpuFeatures(hSrvConfig, &CpuFeatures))
	QCOMPARE(_hw_info.getCpu()->getFEATURES(), CpuFeatures.nFEATURES);
	QCOMPARE(_hw_info.getCpu()->getEXT_FEATURES(), CpuFeatures.nEXT_FEATURES);
	QCOMPARE(_hw_info.getCpu()->getEXT_80000001_ECX(), CpuFeatures.nEXT_80000001_ECX);
	QCOMPARE(_hw_info.getCpu()->getEXT_80000001_EDX(), CpuFeatures.nEXT_80000001_EDX);
	QCOMPARE(_hw_info.getCpu()->getEXT_80000007_EDX(), CpuFeatures.nEXT_80000007_EDX);
	QCOMPARE(_hw_info.getCpu()->getEXT_80000008_EAX(), CpuFeatures.nEXT_80000008_EAX);
	QCOMPARE(_hw_info.getCpu()->getEXT_00000007_EBX(), CpuFeatures.nEXT_00000007_EBX);
}

void PrlSrvManipulationsTest::testSrvConfigGetCpuFeaturesEx()
{
	RECEIVE_SERVER_HW_INFO
	SdkHandleWrap hFeatures;
	PRL_UINT32 nValue;

	CHECK_RET_CODE_EXP(PrlSrvCfg_GetCpuFeaturesEx(hSrvConfig, hFeatures.GetHandlePtr()))

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hFeatures, PCFE_FEATURES, &nValue))
	QCOMPARE(_hw_info.getCpu()->getFEATURES(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hFeatures, PCFE_EXT_FEATURES, &nValue))
	QCOMPARE(_hw_info.getCpu()->getEXT_FEATURES(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hFeatures, PCFE_EXT_80000001_ECX, &nValue))
	QCOMPARE(_hw_info.getCpu()->getEXT_80000001_ECX(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hFeatures, PCFE_EXT_80000001_EDX, &nValue))
	QCOMPARE(_hw_info.getCpu()->getEXT_80000001_EDX(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hFeatures, PCFE_EXT_80000007_EDX, &nValue))
	QCOMPARE(_hw_info.getCpu()->getEXT_80000007_EDX(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hFeatures, PCFE_EXT_80000008_EAX, &nValue))
	QCOMPARE(_hw_info.getCpu()->getEXT_80000008_EAX(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hFeatures, PCFE_EXT_00000007_EBX, &nValue))
	QCOMPARE(_hw_info.getCpu()->getEXT_00000007_EBX(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hFeatures, PCFE_EXT_0000000D_EAX, &nValue))
	QCOMPARE(_hw_info.getCpu()->getEXT_0000000D_EAX(), nValue);
}

void PrlSrvManipulationsTest::testSrvConfigGetCpuFeaturesOnWrongParams()
{
	RECEIVE_SERVER_HW_INFO
	PRL_CPU_FEATURES CpuFeatures;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfg_GetCpuFeatures(
			PRL_INVALID_HANDLE, &CpuFeatures), PRL_ERR_INVALID_ARG)
	// wrong pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfg_GetCpuFeatures(
			hSrvConfig, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testSrvConfigGetCpuFeaturesExOnWrongParams()
{
	RECEIVE_SERVER_HW_INFO
	SdkHandleWrap hFeatures;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfg_GetCpuFeaturesEx(
			0, hFeatures.GetHandlePtr()), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfg_GetCpuFeaturesEx(
			hSrvConfig, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testLogRotationEnabled()
{
	RECEIVE_DISP_CONFIG;
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT;

	PRL_BOOL bVal = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlDispCfg_IsLogRotationEnabled(hDispConfig, &bVal));
	QCOMPARE((bool)bVal, _dcp.getLogRotatePreferences()->isEnabled());

	PRL_BOOL bNewVal = !bVal;
	CHECK_RET_CODE_EXP(PrlDispCfg_SetLogRotationEnabled(hDispConfig, bNewVal));
	CHECK_RET_CODE_EXP(PrlDispCfg_IsLogRotationEnabled(hDispConfig, &bVal));
	QCOMPARE(bNewVal, bVal);
}

void PrlSrvManipulationsTest::testLogRotationEnabledOnWrongParams()
{
	RECEIVE_DISP_CONFIG;

	PRL_BOOL bVal = PRL_FALSE;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_IsLogRotationEnabled(m_ServerHandle, &bVal),
			PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_SetLogRotationEnabled(m_ServerHandle, PRL_TRUE),
			PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_IsLogRotationEnabled(hDispConfig, NULL),
			PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testDispConfigSetHostId()
{
	QString sNewHostId("{01234567-89ab-cdef-0123-ffffffffffff");

	RECEIVE_DISP_CONFIG;

	CHECK_RET_CODE_EXP(PrlDispCfg_SetHostId(
		hDispConfig, QSTR2UTF8(sNewHostId)));
	{
		EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT;
		QCOMPARE(sNewHostId,
				 _dcp.getWorkspacePreferences()->getHostId());
	}
}

void PrlSrvManipulationsTest::testDispConfigSetHostIdOnWrongParams()
{
	QString sNewHostId("{01234567-89ab-cdef-0123-ffffffffffff");

	RECEIVE_DISP_CONFIG;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_SetHostId(
		PRL_INVALID_HANDLE, QSTR2UTF8(sNewHostId)), PRL_ERR_INVALID_ARG);

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_SetHostId(
		hDispConfig, 0), PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testDispConfigGetHostId()
{
	RECEIVE_DISP_CONFIG;
	EXTRACT_DISP_CONFIG_AS_XML_MODEL_OBJECT;

	QString sHostId;
	PRL_EXTRACT_STRING_VALUE(sHostId, hDispConfig, PrlDispCfg_GetHostId);
	QCOMPARE(sHostId, _dcp.getWorkspacePreferences()->getHostId());

	QString sNewHostId("{01234567-89ab-cdef-0123-ffffffffffff");
	_dcp.getWorkspacePreferences()->setHostId(sNewHostId);

	CHECK_RET_CODE_EXP(PrlHandle_FromString(hDispConfig, QSTR2UTF8(_dcp.toString())));
	PRL_EXTRACT_STRING_VALUE(sHostId, hDispConfig, PrlDispCfg_GetHostId);
	QCOMPARE(sHostId, sNewHostId);
}

void PrlSrvManipulationsTest::testDispConfigGetHostIdOnWrongParams()
{
	RECEIVE_DISP_CONFIG;

	PRL_UINT32 nValue = 0;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetHostId(m_ServerHandle, 0, &nValue),
			PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlDispCfg_GetHostId(hDispConfig, 0, 0),
			PRL_ERR_INVALID_ARG);
}

void PrlSrvManipulationsTest::testSrvConfigGetCpuFeaturesMaskingCapabilities()
{
	RECEIVE_SERVER_HW_INFO
	SdkHandleWrap hFeatures;
	PRL_UINT32 nValue;

	CHECK_RET_CODE_EXP(
			PrlSrvCfg_GetCpuFeaturesMaskingCapabilities(hSrvConfig, hFeatures.GetHandlePtr()))

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hFeatures, PCFE_FEATURES, &nValue))
	QCOMPARE(_hw_info.getCpu()->getFEATURES_MASKING_CAP(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hFeatures, PCFE_EXT_FEATURES, &nValue))
	QCOMPARE(_hw_info.getCpu()->getEXT_FEATURES_MASKING_CAP(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hFeatures, PCFE_EXT_80000001_ECX, &nValue))
	QCOMPARE(_hw_info.getCpu()->getEXT_80000001_ECX_MASKING_CAP(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hFeatures, PCFE_EXT_80000001_EDX, &nValue))
	QCOMPARE(_hw_info.getCpu()->getEXT_80000001_EDX_MASKING_CAP(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hFeatures, PCFE_EXT_80000007_EDX, &nValue))
	QCOMPARE(_hw_info.getCpu()->getEXT_80000007_EDX_MASKING_CAP(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hFeatures, PCFE_EXT_80000008_EAX, &nValue))
	QCOMPARE(_hw_info.getCpu()->getEXT_80000008_EAX_MASKING_CAP(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hFeatures, PCFE_EXT_00000007_EBX, &nValue))
	QCOMPARE(_hw_info.getCpu()->getEXT_00000007_EBX_MASKING_CAP(), nValue);

	CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hFeatures, PCFE_EXT_0000000D_EAX, &nValue))
	QCOMPARE(_hw_info.getCpu()->getEXT_0000000D_EAX_MASKING_CAP(), nValue);
}

void PrlSrvManipulationsTest::testSrvConfigGetCpuFeaturesMaskingCapabilitiesOnWrongParams()
{
	RECEIVE_SERVER_HW_INFO
	SdkHandleWrap hFeatures;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfg_GetCpuFeaturesMaskingCapabilities(
			0, hFeatures.GetHandlePtr()), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlSrvCfg_GetCpuFeaturesMaskingCapabilities(
			hSrvConfig, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testCpuFeaturesGetValueOnWrongParams()
{
	SdkHandleWrap hFeatures;
	PRL_UINT32 nValue;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCpuFeatures_GetValue
			(PRL_INVALID_HANDLE, PCFE_FEATURES, &nValue), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCpuFeatures_GetValue
			(hFeatures, PCFE_MAX, 0), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCpuFeatures_GetValue
			(hFeatures, PCFE_FEATURES, 0), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testCpuFeaturesSetValueOnWrongParams()
{
	SdkHandleWrap hFeatures;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCpuFeatures_SetValue
			(PRL_INVALID_HANDLE, PCFE_FEATURES, 0xFFFFFFFF), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlCpuFeatures_SetValue
			(hFeatures, PCFE_MAX, 0xFFFFFFFF), PRL_ERR_INVALID_ARG)
}

void PrlSrvManipulationsTest::testUpLocalhostInterface()
{
	if (TestConfig::isServerMode())
	QSKIP("Doesn't make sense for server mode", SkipAll);

	testLoginLocal();
#ifndef _WIN_
	if ( CheckRemoteListenInterface() )
		QSKIP("Dispatcher already has listen interface", SkipAll);

	CHECK_ASYNC_OP_FAILED(PrlSrv_StoreValueByKey( m_ServerHandle, PRL_KEY_TO_UP_LISTENING_INTERFACE, QSTR2UTF8(IOService::LoopbackAddr), 0  ), PRL_ERR_SUCCESS)

	QVERIFY(CheckRemoteListenInterface());
#else
	CHECK_ASYNC_OP_FAILED(PrlSrv_StoreValueByKey( m_ServerHandle, PRL_KEY_TO_UP_LISTENING_INTERFACE, QSTR2UTF8(IOService::LoopbackAddr), 0 ), PRL_ERR_UNIMPLEMENTED)
#endif
}

void PrlSrvManipulationsTest::testDispConfigCpuFeaturesMaskSetOldValidGetNew()
{
	RECEIVE_DISP_CONFIG

	PRL_CPU_FEATURES_MASK oldMask;
	SdkHandleWrap hNewMask;
	PRL_UINT32 nMask;

	oldMask.bIsValid = PRL_TRUE;
	for (int i = PCFE_FEATURES; i != PCFE_EXT_00000007_EBX; ++i)
	{
		((PRL_UINT32*)&oldMask)[i] = i;
	}

	CHECK_RET_CODE_EXP(PrlDispCfg_SetCpuFeaturesMask(hDispConfig, &oldMask));
	CHECK_RET_CODE_EXP(PrlDispCfg_GetCpuFeaturesMaskEx(hDispConfig, hNewMask.GetHandlePtr()));

	for (int i = PCFE_FEATURES; i != PCFE_EXT_0000000D_EAX; ++i)
	{
		CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hNewMask,
				static_cast<PRL_CPU_FEATURES_EX>(i), &nMask));
		QCOMPARE(nMask, ((PRL_UINT32*)&oldMask)[i]);
	}
	for (int i = PCFE_EXT_0000000D_EAX; i != PCFE_MAX; ++i)
	{
		CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hNewMask.GetHandle(),
				static_cast<PRL_CPU_FEATURES_EX>(i), &nMask));
		QCOMPARE(nMask, 0xFFFFFFFF);
	}
}

void PrlSrvManipulationsTest::testDispConfigCpuFeaturesMaskSetOldInvalidGetNew()
{
	RECEIVE_DISP_CONFIG

	PRL_CPU_FEATURES_MASK oldMask;
	SdkHandleWrap hNewMask;
	PRL_UINT32 nMask;

	oldMask.bIsValid = PRL_FALSE;
	for (int i = PCFE_FEATURES; i != PCFE_EXT_00000007_EBX; ++i)
	{
		((PRL_UINT32*)&oldMask)[i] = i;
	}

	CHECK_RET_CODE_EXP(PrlDispCfg_SetCpuFeaturesMask(hDispConfig, &oldMask));
	CHECK_RET_CODE_EXP(PrlDispCfg_GetCpuFeaturesMaskEx(hDispConfig, hNewMask.GetHandlePtr()));

	for (int i = PCFE_FEATURES; i != PCFE_MAX; ++i)
	{
		CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hNewMask.GetHandle(),
				static_cast<PRL_CPU_FEATURES_EX>(i), &nMask));
		QCOMPARE(nMask, 0xFFFFFFFF);
	}
}

void PrlSrvManipulationsTest::testDispConfigCpuFeaturesMaskSetNewGetOld()
{
	RECEIVE_DISP_CONFIG

	PRL_CPU_FEATURES_MASK oldMask;
	SdkHandleWrap hNewMask;
	PRL_UINT32 nMask;

	CHECK_RET_CODE_EXP(PrlCpuFeatures_Create(hNewMask.GetHandlePtr()));
	for (int i = PCFE_FEATURES; i != PCFE_MAX; ++i)
	{
		CHECK_RET_CODE_EXP(PrlCpuFeatures_SetValue(hNewMask.GetHandle(),
				static_cast<PRL_CPU_FEATURES_EX>(i), i));
	}

	CHECK_RET_CODE_EXP(PrlDispCfg_SetCpuFeaturesMaskEx(hDispConfig, hNewMask.GetHandle()));
	CHECK_RET_CODE_EXP(PrlDispCfg_GetCpuFeaturesMask(hDispConfig, &oldMask));

	for (int i = PCFE_FEATURES; i != PCFE_EXT_0000000D_EAX; ++i)
	{
		CHECK_RET_CODE_EXP(PrlCpuFeatures_GetValue(hNewMask,
				static_cast<PRL_CPU_FEATURES_EX>(i), &nMask));
		QCOMPARE(nMask, ((PRL_UINT32*)&oldMask)[i]);
	}
	QCOMPARE(oldMask.bIsValid, (PRL_UINT32)PRL_TRUE);
}
