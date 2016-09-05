/////////////////////////////////////////////////////////////////////////////
///
///	@file CProtoSerializerTest.cpp
///
///	Tests fixture class for testing project protocol serializer.
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

#include "CProtoSerializerTest.h"
#include <prlcommon/ProtoSerializer/CProtoSerializer.h>
#include <prlcommon/Messaging/CVmEvent.h>
#include <prlcommon/Messaging/CVmEventParameterList.h>
#include <prlcommon/Messaging/CResult.h>
#include <prlcommon/PrlUuid/Uuid.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlxmlmodel/HostHardwareInfo/CHostHardwareInfo.h>
#include <prlxmlmodel/HostHardwareInfo/CHwFileSystemInfo.h>
#include <prlxmlmodel/DispConfig/CDispUser.h>
#include <prlxmlmodel/DispConfig/CDispCommonPreferences.h>
#include <prlxmlmodel/HostHardwareInfo/CSystemStatistics.h>
#include <prlxmlmodel/UserInformation/ParallelsUserInformation.h>
#include <prlxmlmodel/NetworkConfig/CParallelsNetworkConfig.h>
#include "Tests/CommonTestsUtils.h"

using namespace Parallels;

#define LOGIN_CMD_PARAMS_DECLARE\
	QString sUsername = "somelogin";\
	QString sPassword = "somepassword"; \
	QString	sPrevSessionUuid = "somePrevSessionUuid";

void CProtoSerializerTest::testCreateDspCmdUserLoginCommand()
{
	LOGIN_CMD_PARAMS_DECLARE
		CProtoCommandPtr pCmd = CProtoSerializer::CreateDspCmdUserLoginCommand(sUsername, sPassword, sPrevSessionUuid, 0);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_LOGIN_CMD_USERNAME, PVE::String, sUsername)
		CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_LOGIN_CMD_PASSWORD, PVE::String, sPassword)
		CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_LOGIN_CMD_SESSION_TO_RESTORE, PVE::String, sPrevSessionUuid)
}

void CProtoSerializerTest::testParseDspCmdUserLoginCommand()
{
	LOGIN_CMD_PARAMS_DECLARE
		SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUsername, EVT_PARAM_LOGIN_CMD_USERNAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sPassword, EVT_PARAM_LOGIN_CMD_PASSWORD));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sPrevSessionUuid, EVT_PARAM_LOGIN_CMD_SESSION_TO_RESTORE));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdUserLogin, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CProtoCommandDspCmdUserLogin *pDspCmdUserLoginCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspCmdUserLogin>(pCmd);
	QCOMPARE(sUsername, pDspCmdUserLoginCmd->GetUserLoginName());
	QCOMPARE(sPassword, pDspCmdUserLoginCmd->GetPassword());
}

#define CHECK_LOGIN_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CProtoCommandDspCmdUserLogin *pDspCmdUserLoginCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspCmdUserLogin>(pCmd);\
	pDspCmdUserLoginCmd->GetUserLoginName();\
	pDspCmdUserLoginCmd->GetPassword(); \
	pDspCmdUserLoginCmd->GetPrevSessionUuid();

void CProtoSerializerTest::testDspCmdUserLoginCommandIsValidFailedOnEmptyPackage()
{
	LOGIN_CMD_PARAMS_DECLARE
		SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdUserLogin, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_LOGIN_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdUserLoginCommandIsValidFailedOnUserNameAbsent()
{
	LOGIN_CMD_PARAMS_DECLARE
		SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sPassword, EVT_PARAM_LOGIN_CMD_PASSWORD));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sPrevSessionUuid, EVT_PARAM_LOGIN_CMD_SESSION_TO_RESTORE));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdUserLogin, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_LOGIN_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdUserLoginCommandIsValidFailedOnPasswordAbsent()
{
	LOGIN_CMD_PARAMS_DECLARE
		SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUsername, EVT_PARAM_LOGIN_CMD_USERNAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sPrevSessionUuid, EVT_PARAM_LOGIN_CMD_SESSION_TO_RESTORE));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdUserLogin, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_LOGIN_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdUserLoginCommandIsValidFailedOnPrevSessionUuidAbsent()
{
	LOGIN_CMD_PARAMS_DECLARE
		SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUsername, EVT_PARAM_LOGIN_CMD_USERNAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sPassword, EVT_PARAM_LOGIN_CMD_PASSWORD));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdUserLogin, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_LOGIN_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

#define CHECK_STANDARD_RESULT_PARAMS\
	QVERIFY(nCmdIdentifier == pResult->getOpCode());\
	QVERIFY(nErrCode == pResult->getReturnCode());

void CProtoSerializerTest::testCreateDspWsResponseCommandForLogin()
{
	//TODO: need add changes by task #6009.
	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdUserLogin;
	PRL_RESULT nErrCode = PRL_ERR_ACCESS_DENIED;
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());
	SmartPtr<CResult> pResult( new CResult );
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->FillResult(pResult.getImpl());
	CHECK_STANDARD_RESULT_PARAMS
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForGetVmList()
{
	const quint32 nTestIterationsNum = 10;
	QStringList lstVmConfigurations;
	for (quint32 i = 0; i < nTestIterationsNum; ++i)
	{
		SmartPtr<CVmConfiguration> pVmConfiguration( new CVmConfiguration );
		pVmConfiguration->getVmIdentification()->setVmUuid(Uuid::createUuid().toString());
		lstVmConfigurations.append(pVmConfiguration->toString());
	}
	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdDirGetVmList;
	PRL_RESULT nErrCode = PRL_ERR_SUCCESS;
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->SetParamsList(lstVmConfigurations);
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());
	SmartPtr<CResult> pResult( new CResult );
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->FillResult(pResult.getImpl());
	CHECK_STANDARD_RESULT_PARAMS
		QVERIFY(lstVmConfigurations.size() == pResult->GetParamsCount());
	QStringList lstActualVmConfigs;
	for (int i = 0; i < pResult->GetParamsCount(); ++i)
		lstActualVmConfigs.append(pResult->GetParamToken(i));
	foreach(QString sVmConfiguration, lstVmConfigurations)
		QVERIFY(lstActualVmConfigs.contains(sVmConfiguration));
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForGetSuspendedVmScreen()
{
	QString qsScreen = "asdhbajhdbshj";
	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdVmGetSuspendedScreen;
	PRL_RESULT nErrCode = PRL_ERR_SUCCESS;
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->AddStandardParam(qsScreen);
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());
	SmartPtr<CResult> pResult( new CResult );
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->FillResult(pResult.getImpl());
	CHECK_STANDARD_RESULT_PARAMS
	QVERIFY(pResult->GetParamsCount() == 1);
	QVERIFY(pResult->GetParamToken(0) == qsScreen);
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdUserGetHostHwInfo()
{
	SmartPtr<CHostHardwareInfo> pHostHardwareInfo( new CHostHardwareInfo );
	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdUserGetHostHwInfo;
	PRL_RESULT nErrCode = PRL_ERR_SUCCESS;
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->SetHostHardwareInfo(pHostHardwareInfo->toString());
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());
	SmartPtr<CResult> pResult( new CResult );
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->FillResult(pResult.getImpl());
	QCOMPARE(pHostHardwareInfo->toString(), pResult->m_hashResultSet[PVE::DspCmdUserGetHostHwInfo_strHostHwInfo]);
	CHECK_STANDARD_RESULT_PARAMS
}

#define TEST_RESPONSE_CMD_FOR_FS_COMMAND(fs_cmd_suffix, result_hash_id)\
	SmartPtr<CHwFileSystemInfo> pHwFileSystemInfo( new CHwFileSystemInfo );\
	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdFs##fs_cmd_suffix;\
	PRL_RESULT nErrCode = PRL_ERR_SUCCESS;\
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);\
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);\
	pResponseCmd->SetHwFileSystemInfo(pHwFileSystemInfo->toString());\
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());\
	SmartPtr<CResult> pResult( new CResult );\
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);\
	pResponseCmd->FillResult(pResult.getImpl());\
	QCOMPARE(pHwFileSystemInfo->toString(), pResult->m_hashResultSet[result_hash_id]);\
	CHECK_STANDARD_RESULT_PARAMS

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdFsGetDiskList()
{
	TEST_RESPONSE_CMD_FOR_FS_COMMAND(GetDiskList, PVE::DspCmdFsGetDiskList_strDiskList)
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdFsGetCurrentDirectory()
{
	TEST_RESPONSE_CMD_FOR_FS_COMMAND(GetCurrentDirectory, PVE::DspCmdFsGetCurrentDirectory_strDir)
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdFsGetDirectoryEntries()
{
	TEST_RESPONSE_CMD_FOR_FS_COMMAND(GetDirectoryEntries, PVE::DspCmdFsGetDirectoryEntries_strEntriesList)
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdFsGetFileList()
{
	TEST_RESPONSE_CMD_FOR_FS_COMMAND(GetFileList, PVE::DspCmdFsGetFileList_strFileList)
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdFsCreateDirectory()
{
	TEST_RESPONSE_CMD_FOR_FS_COMMAND(CreateDirectory, PVE::DspCmdFsCreateDirectory_strDirEntries)
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdFsRenameEntry()
{
	TEST_RESPONSE_CMD_FOR_FS_COMMAND(RenameEntry, PVE::DspCmdFsRenameEntry_strDirEntries)
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdFsRemoveEntry()
{
	TEST_RESPONSE_CMD_FOR_FS_COMMAND(RemoveEntry, PVE::DspCmdFsRemoveEntry_strDirEntries)
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdUserGetProfile()
{
	SmartPtr<CDispUser> pDispUser( new CDispUser );
	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdUserGetProfile;
	PRL_RESULT nErrCode = PRL_ERR_SUCCESS;
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->SetUserProfile(pDispUser->toString());
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());
	SmartPtr<CResult> pResult( new CResult );
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->FillResult(pResult.getImpl());
	QCOMPARE(pDispUser->toString(), pResult->m_hashResultSet[PVE::DspCmdUserGetProfile_strUserProfile]);
	CHECK_STANDARD_RESULT_PARAMS
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdGetHostCommonInfo()
{
	SmartPtr<CDispCommonPreferences> pDispCommonPreferences( new CDispCommonPreferences );
	SmartPtr<CParallelsNetworkConfig> pNetworkConfig( new CParallelsNetworkConfig );

	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdGetHostCommonInfo;
	PRL_RESULT nErrCode = PRL_ERR_SUCCESS;
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->SetHostCommonInfo(pDispCommonPreferences->toString(), pNetworkConfig->toString() );

	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());
	SmartPtr<CResult> pResult( new CResult );
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->FillResult(pResult.getImpl());

	QCOMPARE(pDispCommonPreferences->toString(), pResult->m_hashResultSet[PVE::DspCmdGetHostCommonInfo_strCommonInfo]);
	QCOMPARE(pNetworkConfig->toString(), pResult->m_hashResultSet[PVE::DspCmdGetHostCommonInfo_strNetworkInfo]);
	CHECK_STANDARD_RESULT_PARAMS
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdVmGetProblemReport()
{
	QString sRequestId = Uuid::createUuid().toString();
	QString sProblemReport("some report data wiht <tag> information which as a rule can </broken> XML document");
	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdVmGetProblemReport;
	PRL_RESULT nErrCode = PRL_ERR_SUCCESS;
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->SetProblemReport(sProblemReport);
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());
	SmartPtr<CResult> pResult( new CResult );
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->FillResult(pResult.getImpl());
	QCOMPARE(sProblemReport, pResult->m_hashResultSet[PVE::DspCmdVmGetProblemReport_strReport]);
	CHECK_STANDARD_RESULT_PARAMS
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdVmGetConfig()
{
	SmartPtr<CVmConfiguration> pVmConfiguration( new CVmConfiguration );
	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdVmGetConfig;
	PRL_RESULT nErrCode = PRL_ERR_SUCCESS;
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->SetVmConfig(pVmConfiguration->toString());
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());
	SmartPtr<CResult> pResult( new CResult );
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->FillResult(pResult.getImpl());
	QCOMPARE(pVmConfiguration->toString(), pResult->m_hashResultSet[PVE::DspCmdVmGetConfig_strVmConfig]);
	CHECK_STANDARD_RESULT_PARAMS
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdDirRegVm()
{
	SmartPtr<CVmConfiguration> pVmConfiguration( new CVmConfiguration );
	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdDirRegVm;
	PRL_RESULT nErrCode = PRL_ERR_SUCCESS;
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->SetVmConfig(pVmConfiguration->toString());
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());
	SmartPtr<CResult> pResult( new CResult );
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->FillResult(pResult.getImpl());
	QCOMPARE(pVmConfiguration->toString(), pResult->m_hashResultSet[PVE::DspCmdDirRegVm_strVmConfig]);
	CHECK_STANDARD_RESULT_PARAMS
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdDirRestoreVm()
{
	SmartPtr<CVmConfiguration> pVmConfiguration( new CVmConfiguration );
	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdDirRestoreVm;
	PRL_RESULT nErrCode = PRL_ERR_SUCCESS;
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->SetVmConfig(pVmConfiguration->toString());
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());
	SmartPtr<CResult> pResult( new CResult );
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->FillResult(pResult.getImpl());
	QCOMPARE(pVmConfiguration->toString(), pResult->m_hashResultSet[PVE::DspCmdDirRegVm_strVmConfig]);
	CHECK_STANDARD_RESULT_PARAMS
}

#define TEST_RESPONSE_CMD_WITH_VM_EVENT_PARAM(dsp_cmd_suffix, hash_id)\
	SmartPtr<CVmEvent> pVmEvent( new CVmEvent );\
	pVmEvent->addEventParameter(new CVmEventParameter(PVE::String, "Some string value", "some_string_param"));\
	pVmEvent->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(12323), "some_uint_param"));\
	CVmEventParameter *pParam = new CVmEventParameter(PVE::CData, "", "some_cdata_param");\
	pParam->setCdata("some <cdata> </cdata> int value = some_value%some_mode_value");\
	pVmEvent->addEventParameter(pParam);\
	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmd##dsp_cmd_suffix;\
	PRL_RESULT nErrCode = PRL_ERR_SUCCESS;\
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);\
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);\
	pResponseCmd->SetVmEvent(pVmEvent->toString());\
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());\
	SmartPtr<CResult> pResult( new CResult );\
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);\
	pResponseCmd->FillResult(pResult.getImpl());\
	QCOMPARE(pVmEvent->toString(), pResult->m_hashResultSet[hash_id]);\
	CHECK_STANDARD_RESULT_PARAMS

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdGetVmInfo()
{
	TEST_RESPONSE_CMD_WITH_VM_EVENT_PARAM(GetVmInfo, PVE::DspCmdGetVmInfo_strContainer)
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdSMCGetDispatcherRTInfo()
{
	TEST_RESPONSE_CMD_WITH_VM_EVENT_PARAM(SMCGetDispatcherRTInfo, PVE::DspCmdSMCGetDispatcherRTInfo_strEventContainer)
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdSMCGetCommandHistoryByVm()
{
	TEST_RESPONSE_CMD_WITH_VM_EVENT_PARAM(SMCGetCommandHistoryByVm, PVE::DspCmdSMCGetCommandHistoryByVm_strContainer)
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdSMCGetCommandHistoryByUser()
{
	TEST_RESPONSE_CMD_WITH_VM_EVENT_PARAM(SMCGetCommandHistoryByUser, PVE::DspCmdSMCGetCommandHistoryByUser_strContainer)
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdUserGetLicenseInfo()
{
	TEST_RESPONSE_CMD_WITH_VM_EVENT_PARAM(UserGetLicenseInfo, PVE::DspCmdUserGetLicenseInfo_strLicenseInfo)
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdGetHostStatistics()
{
	SmartPtr<CSystemStatistics> pSystemStatistics( new CSystemStatistics );
	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdGetHostStatistics;
	PRL_RESULT nErrCode = PRL_ERR_SUCCESS;
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->SetSystemStatistics(pSystemStatistics->toString());
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());
	SmartPtr<CResult> pResult( new CResult );
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->FillResult(pResult.getImpl());
	QCOMPARE(pSystemStatistics->toString(), pResult->m_hashResultSet[PVE::DspCmdGetHostStatistics_strSystemStatistics]);
	CHECK_STANDARD_RESULT_PARAMS
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdVmGetStatistics()
{
	SmartPtr<CSystemStatistics> pSystemStatistics( new CSystemStatistics );
	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdVmGetStatistics;
	PRL_RESULT nErrCode = PRL_ERR_SUCCESS;
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->SetSystemStatistics(pSystemStatistics->toString());
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());
	SmartPtr<CResult> pResult( new CResult );
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->FillResult(pResult.getImpl());
	QCOMPARE(pSystemStatistics->toString(), pResult->m_hashResultSet[PVE::DspCmdVmGetStatistics_strSystemStatistics]);
	CHECK_STANDARD_RESULT_PARAMS
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdGetVmConfigById()
{
	SmartPtr<CVmConfiguration> pVmConfiguration( new CVmConfiguration );
	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdGetVmConfigById;
	PRL_RESULT nErrCode = PRL_ERR_SUCCESS;
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pVmConfiguration->getVmIdentification()->setVmUuid(Uuid::createUuid().toString());
	pResponseCmd->SetVmConfig(pVmConfiguration->toString());
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());
	SmartPtr<CResult> pResult( new CResult );
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->FillResult(pResult.getImpl());
	QCOMPARE(pVmConfiguration->toString(), pResult->m_hashResultSet[PVE::DspCmdVmGetConfig_strVmConfig]);
	CHECK_STANDARD_RESULT_PARAMS
}

void CProtoSerializerTest::testDataDspCmdSendProblemReport()
{
    const QString sData("10001000100101001101010001010101000100100010001");

    CProtoCommandPtr pOrigCmd = CProtoSerializer::CreateSendProblemReportProtoCommand(
                sData, QString(), 0);

    CProtoCommandPtr pParsedCmd = CProtoSerializer::ParseCommand(
        PVE::DspCmdSendProblemReport, pOrigCmd->GetCommand()->toString());

    CProtoSendProblemReport *pResponseCmd
            = CProtoSerializer::CastToProtoCommand<CProtoSendProblemReport>(pParsedCmd);
    QCOMPARE(pResponseCmd->GetReportData(), sData);
}

void CProtoSerializerTest::testCreateDspWsResponseCommandWithAdditionalErrorInfo()
{
	SmartPtr<CVmEvent> pError( new CVmEvent(PET_DSP_EVT_ERROR_MESSAGE, Uuid().toString(), PIE_WEB_SERVICE,
											 PRL_ERR_FILE_NOT_FOUND, PVE::EventRespNotRequired, "some error source"));
	for (int i = 0; i < 3; i++)
		pError->addEventParameter(new CVmEventParameter( PVE::String,	"some error data", EVT_PARAM_RETURN_PARAM_TOKEN));

	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdDirRegVm;
	PRL_RESULT nErrCode = PRL_ERR_FILE_NOT_FOUND;
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->SetError(pError->toString());
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());
	SmartPtr<CResult> pResult( new CResult );
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->FillResult(pResult.getImpl());
	QVERIFY(pResult->GetError());
	QCOMPARE(pError->toString(), pResult->GetError()->toString());
	CHECK_STANDARD_RESULT_PARAMS
}

#define LOGIN_LOCAL_CMD_PARAMS_DECLARE\
	quint32 nUserId = 501;\
	quint64 nProcessId = ((quint64)1 << 55); /* more than 2^32 to test 64 bit value */ \
	PRL_UNUSED_PARAM(nUserId); \
	PRL_UNUSED_PARAM(nProcessId);

void CProtoSerializerTest::testCreateDspCmdUserLoginLocalCommand()
{
	LOGIN_LOCAL_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspCmdUserLoginLocalCommand(nUserId, PAM_SERVER, nProcessId);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_LOGIN_LOCAL_CMD_USER_ID, PVE::UnsignedInt, QString("%1").arg(nUserId))
}

void CProtoSerializerTest::testParseDspCmdUserLoginLocalCommand()
{
	LOGIN_LOCAL_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nUserId), EVT_PARAM_LOGIN_LOCAL_CMD_USER_ID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UInt64, QString("%1").arg(nProcessId), EVT_PARAM_LOGIN_LOCAL_CMD_PROCESS_ID));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdUserLoginLocal, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CProtoCommandDspCmdUserLoginLocal *pDspCmdUserLoginLocalCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspCmdUserLoginLocal>(pCmd);
	QVERIFY(nUserId == pDspCmdUserLoginLocalCmd->GetUserId());
	QVERIFY(nProcessId == pDspCmdUserLoginLocalCmd->GetProcessId());
}

#define CHECK_LOGIN_LOCAL_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CProtoCommandDspCmdUserLoginLocal *pDspCmdUserLoginLocalCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspCmdUserLoginLocal>(pCmd); \
	pDspCmdUserLoginLocalCmd->GetUserId(); \
	pDspCmdUserLoginLocalCmd->GetProcessId();

void CProtoSerializerTest::testDspCmdUserLoginLocalCommandIsValidFailedOnEmptyPackage()
{
	LOGIN_LOCAL_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdUserLoginLocal, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_LOGIN_LOCAL_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdUserLoginLocalCommandIsValidFailedOnUserIdAbsent()
{
	LOGIN_LOCAL_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdUserLoginLocal, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_LOGIN_LOCAL_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdUserLoginLocal()
{
	QString sFilePath = "some file path";
	QString sCheckData = "some check data";
	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdUserLoginLocal;
	PRL_RESULT nErrCode = PRL_ERR_SUCCESS;
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->AddStandardParam(sFilePath);
	pResponseCmd->AddStandardParam(sCheckData);
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());
	SmartPtr<CResult> pResult( new CResult );
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->FillResult(pResult.getImpl());
	QVERIFY(pResult->GetParamsCount() == 2);
	QCOMPARE(pResult->GetParamToken(0), sFilePath);
	QCOMPARE(pResult->GetParamToken(1), sCheckData);
	CHECK_STANDARD_RESULT_PARAMS
	QVERIFY(pResponseCmd->GetStandardParamsCount() == 2);
	QCOMPARE(pResponseCmd->GetStandardParam(0), sFilePath);
	QCOMPARE(pResponseCmd->GetStandardParam(1), sCheckData);
	QCOMPARE(pResponseCmd->GetStandardParam(2), QString());//Check on out of range here
}

void CProtoSerializerTest::testCreateDspCmdUserLogoffCommand()
{
	CProtoCommandPtr pCmd = CProtoSerializer::CreateProtoCommandWithoutParams(PVE::DspCmdUserLogoff);
	QVERIFY(pCmd->IsValid());
}

#define TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(cmd_identifier)\
	SmartPtr<CVmEvent> _pkg( new CVmEvent );\
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::cmd_identifier, _pkg->toString());\
	QVERIFY(pCmd->IsValid());

void CProtoSerializerTest::testParseDspCmdUserLogoffCommand()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdUserLogoff)
}

void CProtoSerializerTest::testParseCommandForDspCmdDirGetVmList()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdDirGetVmList)
}

void CProtoSerializerTest::testParseCommandForDspCmdUserGetEvent()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdUserGetEvent)
}

void CProtoSerializerTest::testParseCommandForDspCmdUserGetProfile()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdUserGetProfile)
}

void CProtoSerializerTest::testParseCommandForDspCmdUserProfileBeginEdit()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdUserProfileBeginEdit)
}

void CProtoSerializerTest::testParseCommandForDspCmdGetHostCommonInfo()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdGetHostCommonInfo)
}

void CProtoSerializerTest::testParseCommandForDspCmdHostCommonInfoBeginEdit()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdHostCommonInfoBeginEdit)
}

void CProtoSerializerTest::testParseCommandForDspCmdUserGetHostHwInfo()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdUserGetHostHwInfo)
}

void CProtoSerializerTest::testParseCommandForDspCmdUserPing()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdUserPing)
}

void CProtoSerializerTest::testParseCommandForDspCmdFsGetDiskList()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdFsGetDiskList)
}

void CProtoSerializerTest::testParseCommandForDspCmdFsGetCurrentDirectory()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdFsGetCurrentDirectory)
}

void CProtoSerializerTest::testParseCommandForDspCmdSMCGetDispatcherRTInfo()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdSMCGetDispatcherRTInfo)
}

void CProtoSerializerTest::testParseCommandForDspCmdSMCRestartDispatcher()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdSMCRestartDispatcher)
}

void CProtoSerializerTest::testParseCommandForDspCmdSMCDisconnectAllUsers()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdSMCDisconnectAllUsers)
}

void CProtoSerializerTest::testParseCommandForDspCmdNetPrlNetworkServiceStart()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdNetPrlNetworkServiceStart)
}

void CProtoSerializerTest::testParseCommandForDspCmdNetPrlNetworkServiceStop()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdNetPrlNetworkServiceStop)
}

void CProtoSerializerTest::testParseCommandForDspCmdNetPrlNetworkServiceRestart()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdNetPrlNetworkServiceRestart)
}

void CProtoSerializerTest::testParseCommandForDspCmdNetPrlNetworkServiceRestoreDefaults()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdNetPrlNetworkServiceRestoreDefaults)
}

void CProtoSerializerTest::testParseCommandForDspCmdGetHostStatistics()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdGetHostStatistics)
}

void CProtoSerializerTest::testParseCommandForDspCmdSubscribeToHostStatistics()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdSubscribeToHostStatistics)
}

void CProtoSerializerTest::testParseCommandForDspCmdUnsubscribeFromHostStatistics()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdUnsubscribeFromHostStatistics)
}

void CProtoSerializerTest::testParseCommandForDspCmdUserGetLicenseInfo()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdUserGetLicenseInfo)
}

void CProtoSerializerTest::testParseCommandForDspCmdUserInfoList()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdUserInfoList)
}

void CProtoSerializerTest::testParseCommandForDspCmdGetVirtualNetworkList()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdGetVirtualNetworkList)
}

void CProtoSerializerTest::testParseCommandForDspCmdAllHostUsers()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdAllHostUsers)
}

void CProtoSerializerTest::testParseCommandForDspCmdPrepareForHibernate()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdPrepareForHibernate);
}

void CProtoSerializerTest::testParseCommandForDspCmdAfterHostResume()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdAfterHostResume);
}

#define BASIC_VM_CMD_PARAMS_DECLARE\
	QString sVmUuid = Uuid::createUuid().toString();\
	quint32 nFlags = 0xFF0F0FF0;\
	Q_UNUSED(nFlags);

void CProtoSerializerTest::testCreateDspCmdVmStartCommand()
{
	BASIC_VM_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd = CProtoSerializer::CreateProtoBasicVmCommand(PVE::DspCmdVmStart, sVmUuid);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_BASIC_VM_CMD_VM_UUID, PVE::String, sVmUuid)
}

#define TEST_PARSE_BASIC_VM_COMMAND(cmd_identifier)\
	BASIC_VM_CMD_PARAMS_DECLARE\
	SmartPtr<CVmEvent> _pkg( new CVmEvent );\
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));\
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString::number(nFlags), EVT_PARAM_PROTO_CMD_FLAGS));\
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::cmd_identifier, _pkg->toString());\
	QVERIFY(pCmd->IsValid());\
	QCOMPARE(sVmUuid, pCmd->GetVmUuid());\
	QCOMPARE(nFlags, pCmd->GetCommandFlags());

void CProtoSerializerTest::testParseDspCmdVmStartCommand()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmStart)
}

#define CHECK_BASIC_VM_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	pCmd->GetVmUuid();

void CProtoSerializerTest::testDspCmdVmStartCommandIsValidFailedOnEmptyPackage()
{
	BASIC_VM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmStart, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_BASIC_VM_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmStartCommandIsValidFailedOnVmUuidAbsent()
{
	BASIC_VM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, EVT_PARAM_PROTO_ACCESS_TOKEN));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmStart, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_BASIC_VM_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testCreateDspCmdVmRestartCommand()
{
	BASIC_VM_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd = CProtoSerializer::CreateProtoBasicVmCommand(PVE::DspCmdVmRestartGuest, sVmUuid);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_BASIC_VM_CMD_VM_UUID, PVE::String, sVmUuid)
}

void CProtoSerializerTest::testParseDspCmdVmRestartCommand()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmRestartGuest)
}

void CProtoSerializerTest::testDspCmdVmRestartCommandIsValidFailedOnEmptyPackage()
{
	BASIC_VM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmRestartGuest, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_BASIC_VM_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmRestartCommandIsValidFailedOnVmUuidAbsent()
{
	BASIC_VM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, EVT_PARAM_PROTO_ACCESS_TOKEN));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmRestartGuest, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_BASIC_VM_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testParseCommandForDspCmdVmGetConfig()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmGetConfig)
}

void CProtoSerializerTest::testParseCommandForDspCmdVmGetProblemReport()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmGetProblemReport)
}

void CProtoSerializerTest::testParseCommandForDspCmdVmReset()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmReset)
}

void CProtoSerializerTest::testParseCommandForDspCmdVmSuspend()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmSuspend)
}

void CProtoSerializerTest::testParseCommandForDspCmdVmGetSuspendedScreen()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmGetSuspendedScreen)
}

void CProtoSerializerTest::testParseCommandForDspCmdVmResume()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmResume)
}

void CProtoSerializerTest::testParseCommandForDspCmdDirVmEditBegin()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdDirVmEditBegin)
}

void CProtoSerializerTest::testParseCommandForDspCmdDirUnregVm()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdDirUnregVm)
}

void CProtoSerializerTest::testParseCommandForDspCmdDirRestoreVm()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdDirRestoreVm)
}

void CProtoSerializerTest::testParseCommandForDspCmdGetVmInfo()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdGetVmInfo)
}

void CProtoSerializerTest::testParseCommandForDspCmdSMCShutdownVm()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdSMCShutdownVm)
}

void CProtoSerializerTest::testParseCommandForDspCmdSMCRestartVm()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdSMCRestartVm)
}

void CProtoSerializerTest::testParseCommandForDspCmdVmGetStatistics()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmGetStatistics)
}

void CProtoSerializerTest::testParseCommandForDspCmdVmSubscribeToGuestStatistics()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmSubscribeToGuestStatistics)
}

void CProtoSerializerTest::testParseCommandForDspCmdVmUnsubscribeFromGuestStatistics()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmUnsubscribeFromGuestStatistics)
}

void CProtoSerializerTest::testParseCommandForDspCmdSMCGetCommandHistoryByVm()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdSMCGetCommandHistoryByVm)
}

void CProtoSerializerTest::testParseCommandForDspCmdVmMigrateCancel()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmMigrateCancel)
}

void CProtoSerializerTest::testParseCommandForDspCmdVmRunCompressor()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmRunCompressor);
}

void CProtoSerializerTest::testParseCommandForDspCmdVmCancelCompressor()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmCancelCompressor);
}

void CProtoSerializerTest::testParseCommandForDspCmdVmStartVncServer()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmStartVNCServer);
}

void CProtoSerializerTest::testParseCommandForDspCmdVmStopVncServer()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmStopVNCServer);
}

void CProtoSerializerTest::testParseCommandForDspCmdVmLock()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmLock);
}

void CProtoSerializerTest::testParseCommandForDspCmdVmUnlock()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmUnlock);
}

void CProtoSerializerTest::testParseCommandForDspCmdDspCmdVmCancelCompact()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmCancelCompact);
}

void CProtoSerializerTest::testParseCommandForDspCmdVmChangeSid()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmChangeSid);
}

void CProtoSerializerTest::testParseCommandForDspCmdVmResetUptime()
{
	TEST_PARSE_BASIC_VM_COMMAND(DspCmdVmResetUptime);
}

#define VM_CMD_WITH_ACPI_SIGN_PARAMS_DECLARE\
	BASIC_VM_CMD_PARAMS_DECLARE\
	bool bUseAcpi = false;\
	PRL_UNUSED_PARAM(bUseAcpi);

void CProtoSerializerTest::testCreateDspCmdVmStopCommand()
{
	VM_CMD_WITH_ACPI_SIGN_PARAMS_DECLARE
	CProtoCommandPtr pCmd = CProtoSerializer::CreateProtoVmCommandWithAcpiSign(PVE::DspCmdVmStop, sVmUuid, bUseAcpi);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_BASIC_VM_CMD_VM_UUID, PVE::String, sVmUuid)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_CMD_WITH_ACPI_SIGN_IS_USE_ACPI, PVE::Boolean, QString("%1").arg(bUseAcpi))
}

#define TEST_PARSE_VM_COMMAND_WITH_ACPI_SIGN(cmd_identifier)\
	VM_CMD_WITH_ACPI_SIGN_PARAMS_DECLARE\
	SmartPtr<CVmEvent> _pkg( new CVmEvent );\
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));\
	_pkg->addEventParameter(new CVmEventParameter(PVE::Boolean, QString("%1").arg(bUseAcpi), EVT_PARAM_VM_CMD_WITH_ACPI_SIGN_IS_USE_ACPI));\
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::cmd_identifier, _pkg->toString());\
	QVERIFY(pCmd->IsValid());\
	QCOMPARE(sVmUuid, pCmd->GetVmUuid());\
	CProtoVmCommandWithAcpiSign *pVmCmdWithAcpi = CProtoSerializer::CastToProtoCommand<CProtoVmCommandWithAcpiSign>(pCmd);\
	QVERIFY(pVmCmdWithAcpi->IsUseAcpi() == bUseAcpi);

void CProtoSerializerTest::testParseDspCmdVmStopCommand()
{
	TEST_PARSE_VM_COMMAND_WITH_ACPI_SIGN(DspCmdVmStop)
}

#define CHECK_VM_CMD_WITH_ACPI_SIGN_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CHECK_BASIC_VM_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CProtoVmCommandWithAcpiSign *pVmCmdWithAcpi = CProtoSerializer::CastToProtoCommand<CProtoVmCommandWithAcpiSign>(pCmd);\
	pVmCmdWithAcpi->IsUseAcpi();

void CProtoSerializerTest::testDspCmdVmStopCommandIsValidFailedOnEmptyPackage()
{
	VM_CMD_WITH_ACPI_SIGN_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmStop, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_CMD_WITH_ACPI_SIGN_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmStopCommandIsValidFailedOnVmUuidAbsent()
{
	VM_CMD_WITH_ACPI_SIGN_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::Boolean, QString("%1").arg(bUseAcpi), EVT_PARAM_VM_CMD_WITH_ACPI_SIGN_IS_USE_ACPI));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmStop, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_CMD_WITH_ACPI_SIGN_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmStopCommandIsValidFailedOnAcpiSignAbsent()
{
	VM_CMD_WITH_ACPI_SIGN_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmStop, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_VM_CMD_WITH_ACPI_SIGN_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

#define VM_CMD_WITH_ONE_STR_PARAMS_DECLARE\
	BASIC_VM_CMD_PARAMS_DECLARE\
	QString sParam = Uuid::createUuid().toString();

#define CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CHECK_BASIC_VM_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CProtoVmCommandWithOneStrParam \
		*pVmCmd = CProtoSerializer::CastToProtoCommand<CProtoVmCommandWithOneStrParam>(pCmd);\
	pVmCmd->GetFirstStrParam();

#define CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_PRESENTS_PARAMS \
	CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS \
	QCOMPARE( pVmCmd->GetVmUuid(), sVmUuid ); \
	QCOMPARE( pVmCmd->GetFirstStrParam(), sParam ); \

void CProtoSerializerTest::testDspCProtoVmCommandWithOneStrParam_BothParamsExists()
{
	VM_CMD_WITH_ONE_STR_PARAMS_DECLARE;
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sParam, EVT_PARAM_PROTO_FIRST_STR_PARAM));

	{
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmUpdateToolsSection, _pkg->toString());
		QVERIFY(pCmd->IsValid());
		CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_PRESENTS_PARAMS;
	}

	{
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmInstallUtility, _pkg->toString());
		QVERIFY(pCmd->IsValid());
		CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_PRESENTS_PARAMS;
	}

	{
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmCompact, _pkg->toString());
		QVERIFY(pCmd->IsValid());
		CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_PRESENTS_PARAMS;
	}

	{
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmConvertDisks, _pkg->toString());
		QVERIFY(pCmd->IsValid());
		CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_PRESENTS_PARAMS;
	}

#define DO_TEST(x) \
	{ \
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(x, _pkg->toString()); \
		QVERIFY(pCmd->IsValid()); \
		CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_PRESENTS_PARAMS; \
	}

	DO_TEST(PVE::DspCmdVmSetProtection);
	DO_TEST(PVE::DspCmdVmRemoveProtection);
#undef DO_TEST
}

void CProtoSerializerTest::testDspCProtoVmCommandWithOneStrParam_VmUuidAbsent()
{
	VM_CMD_WITH_ONE_STR_PARAMS_DECLARE;
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	// _pkg->addEventParameter(new CVmEventParameter(PVE::String, sParam, EVT_PARAM_PROTO_FIRST_STR_PARAM));

	{
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmUpdateToolsSection, _pkg->toString());
		QVERIFY(!pCmd->IsValid());
		CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS;
	}

	{
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmInstallUtility, _pkg->toString());
		QVERIFY(!pCmd->IsValid());
		CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS;
	}

	{
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmCompact, _pkg->toString());
		QVERIFY(!pCmd->IsValid());
		CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS;
	}

	{
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmConvertDisks, _pkg->toString());
		QVERIFY(!pCmd->IsValid());
		CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS;
	}

#define DO_TEST(x) \
	{ \
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(x, _pkg->toString()); \
		QVERIFY(!pCmd->IsValid()); \
		CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS; \
	}

	DO_TEST(PVE::DspCmdVmSetProtection);
	DO_TEST(PVE::DspCmdVmRemoveProtection);
#undef DO_TEST
}

void CProtoSerializerTest::testDspCProtoVmCommandWithOneStrParam_StrParamAbsent()
{
	VM_CMD_WITH_ONE_STR_PARAMS_DECLARE;
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	//_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sParam, EVT_PARAM_PROTO_FIRST_STR_PARAM));

	{
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmUpdateToolsSection, _pkg->toString());
		QVERIFY(!pCmd->IsValid());
		CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS;
	}

	{
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmInstallUtility, _pkg->toString());
		QVERIFY(!pCmd->IsValid());
		CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS;
	}

	{
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmCompact, _pkg->toString());
		QVERIFY(!pCmd->IsValid());
		CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS;
	}

	{
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmConvertDisks, _pkg->toString());
		QVERIFY(!pCmd->IsValid());
		CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS;
	}

#define DO_TEST(x) \
	{ \
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(x, _pkg->toString()); \
		QVERIFY(!pCmd->IsValid()); \
		CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS; \
	}

	DO_TEST(PVE::DspCmdVmSetProtection);
	DO_TEST(PVE::DspCmdVmRemoveProtection);
#undef DO_TEST
}

void CProtoSerializerTest::testDspCProtoVmCommandWithOneStrParam_BothParamsAbsent()
{
	VM_CMD_WITH_ONE_STR_PARAMS_DECLARE;
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	// _pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	// _pkg->addEventParameter(new CVmEventParameter(PVE::String, sParam, EVT_PARAM_PROTO_FIRST_STR_PARAM));

	{
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmUpdateToolsSection, _pkg->toString());
		QVERIFY(!pCmd->IsValid());
		CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS;
	}

	{
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmInstallUtility, _pkg->toString());
		QVERIFY(!pCmd->IsValid());
		CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS;
	}

	{
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmCompact, _pkg->toString());
		QVERIFY(!pCmd->IsValid());
		CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS;
	}

	{
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmConvertDisks, _pkg->toString());
		QVERIFY(!pCmd->IsValid());
		CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS;
	}

#define DO_TEST(x) \
	{ \
		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(x, _pkg->toString()); \
		QVERIFY(!pCmd->IsValid()); \
		CHECK_VM_CMD_WITH_ONE_STR_PARAMS_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS; \
	}

	DO_TEST(PVE::DspCmdVmSetProtection);
	DO_TEST(PVE::DspCmdVmRemoveProtection);
#undef DO_TEST
}

void CProtoSerializerTest::testParseCommandForDspCmdVmPause()
{
	TEST_PARSE_VM_COMMAND_WITH_ACPI_SIGN(DspCmdVmPause)
}

#define PROTO_CMD_WITH_ONE_STR_PARAM_PARAMS_DECLARE\
	QString sParam = Uuid::createUuid().toString();

void CProtoSerializerTest::testCreateDspCmdVmAnswerCommand()
{
	PROTO_CMD_WITH_ONE_STR_PARAM_PARAMS_DECLARE
	CProtoCommandPtr pCmd = CProtoSerializer::CreateProtoCommandWithOneStrParam(PVE::DspCmdVmAnswer, sParam);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_PROTO_FIRST_STR_PARAM, PVE::String, sParam)
}

#define TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(cmd_identifier)\
	PROTO_CMD_WITH_ONE_STR_PARAM_PARAMS_DECLARE\
	SmartPtr<CVmEvent> _pkg( new CVmEvent );\
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sParam, EVT_PARAM_PROTO_FIRST_STR_PARAM));\
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::cmd_identifier, _pkg->toString());\
	QVERIFY(pCmd->IsValid());\
	QCOMPARE(sParam, pCmd->GetFirstStrParam());

void CProtoSerializerTest::testParseDspCmdVmAnswerCommand()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdVmAnswer)
}

#define CHECK_PROTO_CMD_WITH_ONE_STR_PARAM_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	pCmd->GetFirstStrParam();

void CProtoSerializerTest::testDspCmdVmAnswerCommandIsValidFailedOnEmptyPackage()
{
	PROTO_CMD_WITH_ONE_STR_PARAM_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmAnswer, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_CMD_WITH_ONE_STR_PARAM_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmAnswerCommandIsValidFailedOnAnswerAbsent()
{
	PROTO_CMD_WITH_ONE_STR_PARAM_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, EVT_PARAM_PROTO_ACCESS_TOKEN));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmAnswer, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_CMD_WITH_ONE_STR_PARAM_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testParseCommandForDspCmdSMCShutdownDispatcher()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdSMCShutdownDispatcher)
}

void CProtoSerializerTest::testParseDspCmdUserLoginLocalStage2Command()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdUserLoginLocalStage2)
}

void CProtoSerializerTest::testParseCommandForDspCmdCtlApplyVmConfig()
{
    TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdCtlApplyVmConfig)
}

void CProtoSerializerTest::testParseCommandForDspCmdDirVmEditCommit()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdDirVmEditCommit)
}

void CProtoSerializerTest::testParseCommandForDspCmdUserProfileCommit()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdUserProfileCommit)
}

void CProtoSerializerTest::testParseCommandForDspCmdHostCommonInfoCommit()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdHostCommonInfoCommit)
}

void CProtoSerializerTest::testParseCommandForDspCmdUserCancelOperation()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdUserCancelOperation)
}

void CProtoSerializerTest::testParseCommandForDspCmdFsGetDirectoryEntries()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdFsGetDirectoryEntries)
}

void CProtoSerializerTest::testParseCommandForDspCmdFsCreateDirectory()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdFsCreateDirectory)
}

void CProtoSerializerTest::testParseCommandForDspCmdFsRemoveEntry()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdFsRemoveEntry)
}

void CProtoSerializerTest::testParseCommandForDspCmdFsCanCreateFile()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdFsCanCreateFile)
}

void CProtoSerializerTest::testParseCommandForDspCmdSMCGetCommandHistoryByUser()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdSMCGetCommandHistoryByUser)
}

void CProtoSerializerTest::testParseCommandForDspCmdSMCDisconnectUser()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdSMCDisconnectUser)
}

void CProtoSerializerTest::testParseCommandForDspCmdSMCCancelUserCommand()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdSMCCancelUserCommand)
}

void CProtoSerializerTest::testParseCommandForDspCmdDirRegVm()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdDirRegVm)
}

void CProtoSerializerTest::testParseCommandForDspCmdUserInfo()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdUserInfo)
}

void CProtoSerializerTest::testParseCommandForDspCmdAddVirtualNetwork()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdAddVirtualNetwork);
}

void CProtoSerializerTest::testParseCommandForDspCmdUpdateVirtualNetwork()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdUpdateVirtualNetwork);
}

void CProtoSerializerTest::testParseCommandForDspCmdDeleteVirtualNetwork()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdDeleteVirtualNetwork);
}

void CProtoSerializerTest::testParseCommandForDspCmdConfigureGenericPci()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdConfigureGenericPci);
}

void CProtoSerializerTest::testParseCommandForDspCmdSetNonInteractiveSession()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdSetNonInteractiveSession);
}

void CProtoSerializerTest::testParseCommandForDspCmdVmChangeLogLevel()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdVmChangeLogLevel);
}

void CProtoSerializerTest::testParseCommandForDspCmdInstallAppliance()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdInstallAppliance);
}

#define PROTO_VM_DEV_CMD_PARAMS_DECLARE\
	QString sVmUuid = Uuid::createUuid().toString();\
	PRL_DEVICE_TYPE nDevType = PDE_OPTICAL_DISK;\
	PRL_UNUSED_PARAM(nDevType);\
	quint32 nDevIndex = 0;\
	PRL_UNUSED_PARAM(nDevIndex);\
	QString sDevConfig = "some device configuration";

void CProtoSerializerTest::testCreateDspCmdVmDevConnectCommand()
{
	PROTO_VM_DEV_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd = CProtoSerializer::CreateVmDeviceProtoCommand(PVE::DspCmdVmDevConnect, sVmUuid, nDevType, nDevIndex, sDevConfig);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_BASIC_VM_CMD_VM_UUID, PVE::String, sVmUuid)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_DEV_CMD_DEVICE_TYPE, PVE::UnsignedInt, QString("%1").arg(quint32(nDevType)))
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_DEV_CMD_DEVICE_INDEX, PVE::UnsignedInt, QString("%1").arg(nDevIndex))
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_DEV_CMD_DEVICE_CONFIG, PVE::String, sDevConfig)
}

#define TEST_PARSE_VM_DEV_COMMAND(cmd_identifier)\
	PROTO_VM_DEV_CMD_PARAMS_DECLARE\
	SmartPtr<CVmEvent> _pkg( new CVmEvent );\
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));\
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(quint32(nDevType)), EVT_PARAM_VM_DEV_CMD_DEVICE_TYPE));\
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nDevIndex), EVT_PARAM_VM_DEV_CMD_DEVICE_INDEX));\
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sDevConfig, EVT_PARAM_VM_DEV_CMD_DEVICE_CONFIG));\
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::cmd_identifier, _pkg->toString());\
	QVERIFY(pCmd->IsValid());\
	QCOMPARE(sVmUuid, pCmd->GetVmUuid());\
	CProtoVmDeviceCommand *pVmDevCmd = CProtoSerializer::CastToProtoCommand<CProtoVmDeviceCommand>(pCmd);\
	QVERIFY(nDevType == pVmDevCmd->GetDeviceType());\
	QVERIFY(nDevIndex == pVmDevCmd->GetDeviceIndex());\
	QCOMPARE(sDevConfig, pVmDevCmd->GetDeviceConfig());

void CProtoSerializerTest::testParseDspCmdVmDevConnectCommand()
{
	TEST_PARSE_VM_DEV_COMMAND(DspCmdVmDevConnect)
}

#define CHECK_PROTO_VM_DEV_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	pCmd->GetVmUuid();\
	CProtoVmDeviceCommand *pVmDevCmd = CProtoSerializer::CastToProtoCommand<CProtoVmDeviceCommand>(pCmd);\
	pVmDevCmd->GetDeviceType();\
	pVmDevCmd->GetDeviceIndex();\
	pVmDevCmd->GetDeviceConfig();

void CProtoSerializerTest::testDspCmdVmDevConnectCommandIsValidFailedOnEmptyPackage()
{
	PROTO_VM_DEV_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmDevConnect, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_DEV_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmDevConnectCommandIsValidFailedOnVmUuidAbsent()
{
	PROTO_VM_DEV_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(quint32(nDevType)), EVT_PARAM_VM_DEV_CMD_DEVICE_TYPE));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nDevIndex), EVT_PARAM_VM_DEV_CMD_DEVICE_INDEX));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sDevConfig, EVT_PARAM_VM_DEV_CMD_DEVICE_CONFIG));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmDevConnect, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_DEV_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmDevConnectCommandIsValidFailedOnDeviceTypeAbsent()
{
	PROTO_VM_DEV_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nDevIndex), EVT_PARAM_VM_DEV_CMD_DEVICE_INDEX));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sDevConfig, EVT_PARAM_VM_DEV_CMD_DEVICE_CONFIG));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmDevConnect, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_DEV_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmDevConnectCommandIsValidFailedOnDeviceIndexAbsent()
{
	PROTO_VM_DEV_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(quint32(nDevType)), EVT_PARAM_VM_DEV_CMD_DEVICE_TYPE));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sDevConfig, EVT_PARAM_VM_DEV_CMD_DEVICE_CONFIG));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmDevConnect, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_DEV_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmDevConnectCommandIsValidFailedOnDeviceConfigAbsent()
{
	PROTO_VM_DEV_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(quint32(nDevType)), EVT_PARAM_VM_DEV_CMD_DEVICE_TYPE));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nDevIndex), EVT_PARAM_VM_DEV_CMD_DEVICE_INDEX));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmDevConnect, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_DEV_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testParseCommandForDspCmdVmDevDisconnect()
{
	TEST_PARSE_VM_DEV_COMMAND(DspCmdVmDevDisconnect)
}

#define PROTO_DELETE_VM_CMD_PARAMS_DECLARE\
	QString sVmUuid = Uuid::createUuid().toString();\
	QStringList lstDevices;\
	lstDevices.append("device path1");\
	lstDevices.append("device path2");\
	lstDevices.append("device path3");\
	lstDevices.append("device path4");

void CProtoSerializerTest::testCreateDspCmdDirVmDelete()
{
	PROTO_DELETE_VM_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd = CProtoSerializer::CreateVmDeleteProtoCommand(sVmUuid, lstDevices);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_BASIC_VM_CMD_VM_UUID, PVE::String, sVmUuid)
	CVmEventParameter *pParam = pEvent->getEventParameter(EVT_PARAM_VM_DELETE_CMD_VM_DEVICES_LIST);
	QVERIFY(pParam);
	CVmEventParameterList *pListParam = dynamic_cast<CVmEventParameterList *>(pParam);
	QVERIFY(pListParam);
	QVERIFY(pListParam->getValuesList() == lstDevices);
}

void CProtoSerializerTest::testParseCommandForDspCmdDirVmDelete()
{
	PROTO_DELETE_VM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstDevices, EVT_PARAM_VM_DELETE_CMD_VM_DEVICES_LIST));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmDelete, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	QCOMPARE(sVmUuid, pCmd->GetVmUuid());
	CProtoVmDeleteCommand *pVmDeleteCmd = CProtoSerializer::CastToProtoCommand<CProtoVmDeleteCommand>(pCmd);
	QVERIFY(lstDevices == pVmDeleteCmd->GetVmDevicesList());
}

#define CHECK_PROTO_VM_DELETE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	pCmd->GetVmUuid();\
	CProtoVmDeleteCommand *pVmDeleteCmd = CProtoSerializer::CastToProtoCommand<CProtoVmDeleteCommand>(pCmd);\
	pVmDeleteCmd->GetVmDevicesList();

void CProtoSerializerTest::testDspCmdDirVmDeleteCommandIsValidFailedOnEmptyPackage()
{
	PROTO_DELETE_VM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmDelete, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_DELETE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdDirVmDeleteCommandIsValidFailedOnVmUuidAbsent()
{
	PROTO_DELETE_VM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstDevices, EVT_PARAM_VM_DELETE_CMD_VM_DEVICES_LIST));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmDelete, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_DELETE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdDirVmDeleteCommandIsValidFailedOnDevicesListAbsent()
{
	PROTO_DELETE_VM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmDelete, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_DELETE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

#define VM_CREATE_CMD_PARAMS_DECLARE\
	QString sVmConfig = "some VM configuration";\
	QFile _file("./TestDspCmdDirValidateVmConfig_vm_config.xml");\
	if(_file.open(QIODevice::ReadOnly))\
	{\
		QTextStream _stream(&_file);\
		sVmConfig = _stream.readAll();\
	}\
	QString sVmHomePath = "some VM home path";\
	quint32 nFlags = PACF_NON_INTERACTIVE_MODE;\
	PRL_UNUSED_PARAM(nFlags);

void CProtoSerializerTest::testCreateDspCmdDirVmCreateCommand()
{
	VM_CREATE_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd = CProtoSerializer::CreateVmCreateProtoCommand(sVmConfig, sVmHomePath, nFlags);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_CREATE_CMD_VM_CONFIG, PVE::String, sVmConfig)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_CREATE_CMD_VM_HOME_PATH, PVE::String, sVmHomePath)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_PROTO_FORCE_QUESTIONS_SIGN, PVE::UnsignedInt, QString::number(quint32(nFlags == PACF_NON_INTERACTIVE_MODE)));
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_PROTO_CMD_FLAGS, PVE::UnsignedInt, QString::number(nFlags));
}

void CProtoSerializerTest::testParseDspCmdDirVmCreateCommand()
{
	VM_CREATE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmConfig, EVT_PARAM_VM_CREATE_CMD_VM_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmHomePath, EVT_PARAM_VM_CREATE_CMD_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString::number(quint32(PACF_NON_INTERACTIVE_MODE == nFlags)), EVT_PARAM_PROTO_FORCE_QUESTIONS_SIGN));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString::number(nFlags), EVT_PARAM_PROTO_CMD_FLAGS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmCreate, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CProtoVmCreateCommand *pVmCreateCmd = CProtoSerializer::CastToProtoCommand<CProtoVmCreateCommand>(pCmd);
	QCOMPARE(sVmConfig, pVmCreateCmd->GetVmConfig());
	QCOMPARE(sVmHomePath, pVmCreateCmd->GetVmHomePath());
	QCOMPARE(PACF_NON_INTERACTIVE_MODE == nFlags, pVmCreateCmd->GetForceQuestionsSign());
	QCOMPARE(nFlags, pVmCreateCmd->GetCommandFlags());
}

#define CHECK_PROTO_VM_CREATE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CProtoVmCreateCommand *pVmCreateCmd = CProtoSerializer::CastToProtoCommand<CProtoVmCreateCommand>(pCmd);\
	pVmCreateCmd->GetVmConfig();\
	pVmCreateCmd->GetVmHomePath();\
	pVmCreateCmd->GetForceQuestionsSign();\
	pVmCreateCmd->GetCommandFlags();

void CProtoSerializerTest::testDspCmdDirVmCreateCommandIsValidFailedOnEmptyPackage()
{
	VM_CREATE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmCreate, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_CREATE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdDirVmCreateCommandIsValidFailedOnVmConfigAbsent()
{
	VM_CREATE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmHomePath, EVT_PARAM_VM_CREATE_CMD_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString::number(quint32(PACF_NON_INTERACTIVE_MODE == nFlags)), EVT_PARAM_PROTO_FORCE_QUESTIONS_SIGN));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmCreate, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_CREATE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdDirVmCreateCommandIsValidFailedOnVmHomePathAbsent()
{
	VM_CREATE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmConfig, EVT_PARAM_VM_CREATE_CMD_VM_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString::number(quint32(PACF_NON_INTERACTIVE_MODE == nFlags)), EVT_PARAM_PROTO_FORCE_QUESTIONS_SIGN));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmCreate, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_CREATE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

#define VM_CLONE_CMD_PARAMS_DECLARE\
	QString sVmUuid = Uuid::createUuid().toString();\
	QString sVmName = Uuid::createUuid().toString();\
	QString sVmHomePath = "some VM home path";\
	PRL_UINT32 nCloneFlags = PCVF_CLONE_TO_TEMPLATE;\
	PRL_UNUSED_PARAM(nCloneFlags);

void CProtoSerializerTest::testCreateDspCmdDirVmCloneCommand()
{
	VM_CLONE_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd = CProtoSerializer::CreateVmCloneProtoCommand(sVmUuid, sVmName, QString(), sVmHomePath, nCloneFlags);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_BASIC_VM_CMD_VM_UUID, PVE::String, sVmUuid)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_CREATE_CMD_VM_HOME_PATH, PVE::String, sVmHomePath)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_CLONE_CMD_VM_NAME, PVE::String, sVmName)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_CLONE_CMD_CREATE_TEMPLATE, PVE::UnsignedInt, QString("%1").arg(nCloneFlags))
}

void CProtoSerializerTest::testParseDspCmdDirVmCloneCommand()
{
	VM_CLONE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmHomePath, EVT_PARAM_VM_CREATE_CMD_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmName, EVT_PARAM_VM_CLONE_CMD_VM_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(1), EVT_PARAM_VM_CLONE_CMD_CREATE_TEMPLATE));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmClone, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CProtoVmCloneCommand *pVmCloneCmd = CProtoSerializer::CastToProtoCommand<CProtoVmCloneCommand>(pCmd);
	QCOMPARE(sVmUuid, pVmCloneCmd->GetVmUuid());
	QCOMPARE(sVmHomePath, pVmCloneCmd->GetVmHomePath());
	QCOMPARE(sVmName, pVmCloneCmd->GetVmName());
	QVERIFY(pVmCloneCmd->IsCreateTemplate());
}

#define CHECK_PROTO_VM_CLONE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CProtoVmCloneCommand *pVmCloneCmd = CProtoSerializer::CastToProtoCommand<CProtoVmCloneCommand>(pCmd);\
	pVmCloneCmd->GetVmUuid();\
	pVmCloneCmd->GetVmHomePath();\
	pVmCloneCmd->GetVmName();\
	pVmCloneCmd->IsCreateTemplate();

void CProtoSerializerTest::testDspCmdDirVmCloneCommandIsValidFailedOnEmptyPackage()
{
	VM_CLONE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmClone, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_CLONE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdDirVmCloneCommandIsValidFailedOnVmConfigAbsent()
{
	VM_CLONE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmHomePath, EVT_PARAM_VM_CREATE_CMD_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmName, EVT_PARAM_VM_CLONE_CMD_VM_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(1), EVT_PARAM_VM_CLONE_CMD_CREATE_TEMPLATE));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmClone, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_CLONE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdDirVmCloneCommandIsValidFailedOnVmHomePathAbsent()
{
	VM_CLONE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmName, EVT_PARAM_VM_CLONE_CMD_VM_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(1), EVT_PARAM_VM_CLONE_CMD_CREATE_TEMPLATE));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmClone, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_CLONE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdDirVmCloneCommandIsValidFailedOnVmNameAbsent()
{
	VM_CLONE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmHomePath, EVT_PARAM_VM_CREATE_CMD_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(1), EVT_PARAM_VM_CLONE_CMD_CREATE_TEMPLATE));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmClone, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_CLONE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdDirVmCloneCommandIsValidFailedOnCreateTemplateSignAbsent()
{
	VM_CLONE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmHomePath, EVT_PARAM_VM_CREATE_CMD_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmName, EVT_PARAM_VM_CLONE_CMD_VM_NAME));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmClone, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_CLONE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

#define CREATE_IMAGE_CMD_PARAMS_DECLARE\
	QString sVmUuid = Uuid::createUuid().toString();\
	QString sImageConfig = "some image configuration";\
	bool bRecreateIsAllowed = true;\
	PRL_UNUSED_PARAM(bRecreateIsAllowed);

void CProtoSerializerTest::testCreateDspCmdDirCreateImageCommand()
{
	CREATE_IMAGE_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd = CProtoSerializer::CreateImageCreateProtoCommand(sVmUuid, sImageConfig, bRecreateIsAllowed);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_BASIC_VM_CMD_VM_UUID, PVE::String, sVmUuid)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_CREATE_IMAGE_CMD_IMAGE_CONFIG, PVE::String, sImageConfig)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_CREATE_IMAGE_CMD_RECREATE_SIGN, PVE::UnsignedInt, QString("%1").arg(bRecreateIsAllowed))
}

void CProtoSerializerTest::testParseDspCmdDirCreateImageCommand()
{
	CREATE_IMAGE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sImageConfig, EVT_PARAM_CREATE_IMAGE_CMD_IMAGE_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(bRecreateIsAllowed), EVT_PARAM_CREATE_IMAGE_CMD_RECREATE_SIGN));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirCreateImage, _pkg->toString());
	QCOMPARE(sVmUuid, pCmd->GetVmUuid());
	QVERIFY(pCmd->IsValid());
	CProtoCreateImageCommand *pCreateImageCmd = CProtoSerializer::CastToProtoCommand<CProtoCreateImageCommand>(pCmd);
	QCOMPARE(sImageConfig, pCreateImageCmd->GetImageConfig());
	QVERIFY(bRecreateIsAllowed == pCreateImageCmd->IsRecreateAllowed());
}

#define CHECK_PROTO_CREATE_IMAGE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CProtoCreateImageCommand *pCreateImageCmd = CProtoSerializer::CastToProtoCommand<CProtoCreateImageCommand>(pCmd);\
	pCreateImageCmd->GetImageConfig();\
	pCreateImageCmd->IsRecreateAllowed();

void CProtoSerializerTest::testDspCmdDirCreateImageCommandIsValidFailedOnEmptyPackage()
{
	CREATE_IMAGE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirCreateImage, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_CREATE_IMAGE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdDirCreateImageCommandIsValidFailedOnVmUuidAbsent()
{
	CREATE_IMAGE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sImageConfig, EVT_PARAM_CREATE_IMAGE_CMD_IMAGE_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(bRecreateIsAllowed), EVT_PARAM_CREATE_IMAGE_CMD_RECREATE_SIGN));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirCreateImage, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_CREATE_IMAGE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdDirCreateImageCommandIsValidFailedOnImageConfigAbsent()
{
	CREATE_IMAGE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(bRecreateIsAllowed), EVT_PARAM_CREATE_IMAGE_CMD_RECREATE_SIGN));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirCreateImage, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_CREATE_IMAGE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdDirCreateImageCommandIsValidFailedOnRecreateIsAllowedSignAbsent()
{
	CREATE_IMAGE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sImageConfig, EVT_PARAM_CREATE_IMAGE_CMD_IMAGE_CONFIG));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirCreateImage, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_CREATE_IMAGE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

#define COPY_IMAGE_CMD_PARAMS_DECLARE\
	QString sVmUuid = Uuid::createUuid().toString();\
	QString sImageConfig = "some image configuration";\
	QString sNewImageName = "my name";\
	QString sTargetPath = "/any/path";

void CProtoSerializerTest::testCreateDspCmdDirCopyImageCommand()
{
	COPY_IMAGE_CMD_PARAMS_DECLARE;

	CProtoCommandPtr pCmd = CProtoSerializer::CopyImageCreateProtoCommand(
		sVmUuid, sImageConfig, sNewImageName, sTargetPath);

	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_BASIC_VM_CMD_VM_UUID, PVE::String, sVmUuid)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_COPY_IMAGE_CMD_IMAGE_CONFIG, PVE::String, sImageConfig)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_COPY_IMAGE_CMD_NEW_IMAGE_NAME, PVE::String, sNewImageName)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_COPY_IMAGE_CMD_TARGET_PATH, PVE::String, sTargetPath)
}

void CProtoSerializerTest::testParseDspCmdDirCopyImageCommand()
{
	COPY_IMAGE_CMD_PARAMS_DECLARE;

	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sImageConfig, EVT_PARAM_COPY_IMAGE_CMD_IMAGE_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sNewImageName,
													EVT_PARAM_COPY_IMAGE_CMD_NEW_IMAGE_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetPath, EVT_PARAM_COPY_IMAGE_CMD_TARGET_PATH));

	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirCopyImage, _pkg->toString());
	QCOMPARE(sVmUuid, pCmd->GetVmUuid());
	QVERIFY(pCmd->IsValid());

	CProtoCopyImageCommand *pCopyImageCmd = CProtoSerializer::CastToProtoCommand<CProtoCopyImageCommand>(pCmd);
	QCOMPARE(sImageConfig, pCopyImageCmd->GetImageConfig());
	QCOMPARE(sNewImageName, pCopyImageCmd->GetNewImageName());
	QCOMPARE(sTargetPath, pCopyImageCmd->GetTargetPath());
}

void CProtoSerializerTest::testDspCmdDirCopyImageCommandOnInvalidPackage()
{
	COPY_IMAGE_CMD_PARAMS_DECLARE;

	QStringList lstParamNames = QStringList()
		<< EVT_PARAM_BASIC_VM_CMD_VM_UUID << EVT_PARAM_COPY_IMAGE_CMD_IMAGE_CONFIG
		<< EVT_PARAM_COPY_IMAGE_CMD_NEW_IMAGE_NAME << EVT_PARAM_COPY_IMAGE_CMD_TARGET_PATH;
	QStringList lstParamValues = QStringList()
		<< sVmUuid << sImageConfig << sNewImageName << sTargetPath;

	QVERIFY(lstParamNames.size() == lstParamValues.size());

	for(int i = 0; i < lstParamNames.size() * lstParamNames.size() - 1; ++i)
	{
		SmartPtr<CVmEvent> _pkg( new CVmEvent );

		int n = i;
		for(int j = 0; j < lstParamNames.size(); ++j)
		{
			if ((n & 1) != 0)
				_pkg->addEventParameter(new CVmEventParameter(
					PVE::String, lstParamValues[j], lstParamNames[j]));
			n = n >> 1;
		}

		CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirCopyImage, _pkg->toString());
		QVERIFY( ! pCmd->IsValid() );

		// Check on crash
		CProtoCopyImageCommand *pCopyImageCmd
			= CProtoSerializer::CastToProtoCommand<CProtoCopyImageCommand>(pCmd);
		pCopyImageCmd->GetVmUuid();
		pCopyImageCmd->GetImageConfig();
		pCopyImageCmd->GetNewImageName();
		pCopyImageCmd->GetTargetPath();
	}
}

#define START_SEARCH_CONFIG_CMD_PARAMS_DECLARE\
	QStringList lstSearchDirs;\
	lstSearchDirs.append("Some dir 1");\
	lstSearchDirs.append("Some dir 2");\
	lstSearchDirs.append("Some dir 3");

void CProtoSerializerTest::testCreateDspCmdStartSearchConfigCommand()
{
	START_SEARCH_CONFIG_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd = CProtoSerializer::CreateStartSearchConfigProtoCommand(lstSearchDirs);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	{
		CVmEventParameter *pParam = pEvent->getEventParameter(EVT_PARAM_START_SEARCH_CONFIG_CMD_SEARCH_DIRS);
		QVERIFY(pParam != NULL);
		QVERIFY(pParam->getParamType() == PVE::String);
		QVERIFY(pParam->isList());
		QVERIFY(pParam->getValuesList() == lstSearchDirs);
	}
}

void CProtoSerializerTest::testParseDspCmdStartSearchConfigCommand()
{
	START_SEARCH_CONFIG_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstSearchDirs, EVT_PARAM_START_SEARCH_CONFIG_CMD_SEARCH_DIRS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdStartSearchConfig, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CProtoStartSearchConfigCommand *pStartSearchConfigCmd = CProtoSerializer::CastToProtoCommand<CProtoStartSearchConfigCommand>(pCmd);
	QVERIFY(lstSearchDirs == pStartSearchConfigCmd->GetSearchDirs());
}

#define CHECK_PROTO_START_SEARCH_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CProtoStartSearchConfigCommand *pStartSearchConfigCmd = CProtoSerializer::CastToProtoCommand<CProtoStartSearchConfigCommand>(pCmd);\
	pStartSearchConfigCmd->GetSearchDirs();

void CProtoSerializerTest::testDspCmdStartSearchConfigCommandIsValidFailedOnEmptyPackage()
{
	START_SEARCH_CONFIG_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdStartSearchConfig, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_START_SEARCH_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdStartSearchConfigCommandIsValidFailedOnSearchDirsListAbsent()
{
	START_SEARCH_CONFIG_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdStartSearchConfig, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_START_SEARCH_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

#define FS_RENAME_ENTRY_CMD_PARAMS_DECLARE\
	QString sOldEntryName = "old entry name";\
	QString sNewEntryName = "new entry name";

void CProtoSerializerTest::testCreateDspCmdFsRenameEntryCommand()
{
	FS_RENAME_ENTRY_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd = CProtoSerializer::CreateFsRenameEntryProtoCommand(sOldEntryName, sNewEntryName);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_FS_RENAME_ENTRY_CMD_OLD_ENTRY_NAME, PVE::String, sOldEntryName)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_FS_RENAME_ENTRY_CMD_NEW_ENTRY_NAME, PVE::String, sNewEntryName)
}

void CProtoSerializerTest::testParseDspCmdFsRenameEntryCommand()
{
	FS_RENAME_ENTRY_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sOldEntryName, EVT_PARAM_FS_RENAME_ENTRY_CMD_OLD_ENTRY_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sNewEntryName, EVT_PARAM_FS_RENAME_ENTRY_CMD_NEW_ENTRY_NAME));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdFsRenameEntry, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CProtoFsRenameEntryCommand *pFsRenameEntryCmd = CProtoSerializer::CastToProtoCommand<CProtoFsRenameEntryCommand>(pCmd);
	QCOMPARE(sOldEntryName, pFsRenameEntryCmd->GetOldEntryName());
	QCOMPARE(sNewEntryName, pFsRenameEntryCmd->GetNewEntryName());
}

#define CHECK_PROTO_FS_RENAME_ENTRY_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CProtoFsRenameEntryCommand *pFsRenameEntryCmd = CProtoSerializer::CastToProtoCommand<CProtoFsRenameEntryCommand>(pCmd);\
	pFsRenameEntryCmd->GetOldEntryName();\
	pFsRenameEntryCmd->GetNewEntryName();

void CProtoSerializerTest::testDspCmdFsRenameEntryCommandIsValidFailedOnEmptyPackage()
{
	FS_RENAME_ENTRY_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdFsRenameEntry, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_FS_RENAME_ENTRY_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdFsRenameEntryCommandIsValidFailedOnOldEntryNameAbsent()
{
	FS_RENAME_ENTRY_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sNewEntryName, EVT_PARAM_FS_RENAME_ENTRY_CMD_NEW_ENTRY_NAME));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdFsRenameEntry, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_FS_RENAME_ENTRY_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdFsRenameEntryCommandIsValidFailedOnNewEntryNameAbsent()
{
	FS_RENAME_ENTRY_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sOldEntryName, EVT_PARAM_FS_RENAME_ENTRY_CMD_OLD_ENTRY_NAME));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdFsRenameEntry, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_FS_RENAME_ENTRY_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

#define SERIAL_NUM_CMD_PARAMS_DECLARE\
	QString sUserName = "some user name";\
	QString sCompanyName = "some company name";\
	QString sSerialNumber = "some serial number";

void CProtoSerializerTest::testCreateDspCmdDirInstallGuestOsCommand()
{
	SERIAL_NUM_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd = CProtoSerializer::CreateSerialNumProtoCommand(PVE::DspCmdDirInstallGuestOS, sUserName, sCompanyName, sSerialNumber);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_SERIAL_NUM_CMD_USER_NAME, PVE::String, sUserName)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_SERIAL_NUM_CMD_COMPANY_NAME, PVE::String, sCompanyName)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_SERIAL_NUM_CMD_SERIAL_NUMBER, PVE::String, sSerialNumber)
}

#define TEST_PARSE_SERIAL_NUM_CMD(nCmdIdentifier)\
	SERIAL_NUM_CMD_PARAMS_DECLARE\
	SmartPtr<CVmEvent> _pkg( new CVmEvent );\
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserName, EVT_PARAM_SERIAL_NUM_CMD_USER_NAME));\
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sCompanyName, EVT_PARAM_SERIAL_NUM_CMD_COMPANY_NAME));\
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSerialNumber, EVT_PARAM_SERIAL_NUM_CMD_SERIAL_NUMBER));\
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::nCmdIdentifier, _pkg->toString());\
	QVERIFY(pCmd->IsValid());\
	CProtoSerialNumCommand *pSerialNumCmd = CProtoSerializer::CastToProtoCommand<CProtoSerialNumCommand>(pCmd);\
	QCOMPARE(sUserName, pSerialNumCmd->GetUserLoginName());\
	QCOMPARE(sCompanyName, pSerialNumCmd->GetCompanyName());\
	QCOMPARE(sSerialNumber, pSerialNumCmd->GetSerialNumber());

void CProtoSerializerTest::testParseDspCmdDirInstallGuestOsCommand()
{
	TEST_PARSE_SERIAL_NUM_CMD(DspCmdDirInstallGuestOS)
}

#define CHECK_PROTO_SERIAL_NUM_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CProtoSerialNumCommand *pSerialNumCmd = CProtoSerializer::CastToProtoCommand<CProtoSerialNumCommand>(pCmd);\
	pSerialNumCmd->GetUserLoginName();\
	pSerialNumCmd->GetCompanyName();\
	pSerialNumCmd->GetSerialNumber();

void CProtoSerializerTest::testDspCmdDirInstallGuestOsCommandIsValidFailedOnEmptyPackage()
{
	SERIAL_NUM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirInstallGuestOS, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_SERIAL_NUM_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdDirInstallGuestOsCommandIsValidFailedOnUserNameAbsent()
{
	SERIAL_NUM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sCompanyName, EVT_PARAM_SERIAL_NUM_CMD_COMPANY_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sCompanyName, EVT_PARAM_SERIAL_NUM_CMD_SERIAL_NUMBER));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirInstallGuestOS, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_SERIAL_NUM_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdDirInstallGuestOsCommandIsValidFailedOnCompanyNameAbsent()
{
	SERIAL_NUM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserName, EVT_PARAM_SERIAL_NUM_CMD_USER_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sCompanyName, EVT_PARAM_SERIAL_NUM_CMD_SERIAL_NUMBER));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirInstallGuestOS, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_SERIAL_NUM_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdDirInstallGuestOsCommandIsValidFailedOnSerialNumberAbsent()
{
	SERIAL_NUM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserName, EVT_PARAM_SERIAL_NUM_CMD_USER_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sCompanyName, EVT_PARAM_SERIAL_NUM_CMD_COMPANY_NAME));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirInstallGuestOS, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_SERIAL_NUM_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testParseDspCmdUserUpdateLicense()
{
	TEST_PARSE_SERIAL_NUM_CMD(DspCmdUserUpdateLicense)
}

#define VM_CREATE_UNATTENDED_FLOPPY_CMD_PARAMS_DECLARE\
	SERIAL_NUM_CMD_PARAMS_DECLARE\
	QString sVmUuid = Uuid::createUuid().toString();\
	PRL_GUEST_DISTRO_VERSION_ENUM nGuestType = PGD_WINDOWS_XP;\
	PRL_UNUSED_PARAM(nGuestType);

void CProtoSerializerTest::testCreateDspCmdVmCreateUnattendedFloppyCommand()
{
	VM_CREATE_UNATTENDED_FLOPPY_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd = CProtoSerializer::CreateVmCreateUnattendedProtoCommand(sVmUuid, sUserName, sCompanyName, sSerialNumber, nGuestType);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_BASIC_VM_CMD_VM_UUID, PVE::String, sVmUuid)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_SERIAL_NUM_CMD_USER_NAME, PVE::String, sUserName)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_SERIAL_NUM_CMD_COMPANY_NAME, PVE::String, sCompanyName)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_SERIAL_NUM_CMD_SERIAL_NUMBER, PVE::String, sSerialNumber)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_CREATE_UNATTENDED_FLOPPY_CMD_GUEST_DISTRO_TYPE, PVE::UnsignedInt, QString("%1").arg(quint32(nGuestType)))
}

void CProtoSerializerTest::testParseDspCmdVmCreateUnattendedFloppyCommand()
{
	VM_CREATE_UNATTENDED_FLOPPY_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );\
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserName, EVT_PARAM_SERIAL_NUM_CMD_USER_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sCompanyName, EVT_PARAM_SERIAL_NUM_CMD_COMPANY_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSerialNumber, EVT_PARAM_SERIAL_NUM_CMD_SERIAL_NUMBER));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(quint32(nGuestType)), EVT_PARAM_CREATE_UNATTENDED_FLOPPY_CMD_GUEST_DISTRO_TYPE));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmCreateUnattendedFloppy, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	QCOMPARE(sVmUuid, pCmd->GetVmUuid());
	CProtoCreateUnattendedFloppyCommand *pCreateUnattendedFloppyCmd = CProtoSerializer::CastToProtoCommand<CProtoCreateUnattendedFloppyCommand>(pCmd);
	QCOMPARE(sUserName, pCreateUnattendedFloppyCmd->GetUserLoginName());
	QCOMPARE(sCompanyName, pCreateUnattendedFloppyCmd->GetCompanyName());
	QCOMPARE(sSerialNumber, pCreateUnattendedFloppyCmd->GetSerialNumber());
	QVERIFY(nGuestType == pCreateUnattendedFloppyCmd->GetGuestDistroType());
}

#define CHECK_PROTO_VM_CREATE_UNATTENDED_FLOPPY_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	pCmd->GetVmUuid();\
	CProtoCreateUnattendedFloppyCommand *pCreateUnattendedFloppyCmd = CProtoSerializer::CastToProtoCommand<CProtoCreateUnattendedFloppyCommand>(pCmd);\
	pCreateUnattendedFloppyCmd->GetUserLoginName();\
	pCreateUnattendedFloppyCmd->GetCompanyName();\
	pCreateUnattendedFloppyCmd->GetSerialNumber();\
	pCreateUnattendedFloppyCmd->GetGuestDistroType();

void CProtoSerializerTest::testDspCmdVmCreateUnattendedFloppyCommandIsValidFailedOnEmptyPackage()
{
	VM_CREATE_UNATTENDED_FLOPPY_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmCreateUnattendedFloppy, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_CREATE_UNATTENDED_FLOPPY_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmCreateUnattendedFloppyCommandIsValidFailedOnVmUuidAbsent()
{
	VM_CREATE_UNATTENDED_FLOPPY_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserName, EVT_PARAM_SERIAL_NUM_CMD_USER_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sCompanyName, EVT_PARAM_SERIAL_NUM_CMD_COMPANY_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSerialNumber, EVT_PARAM_SERIAL_NUM_CMD_SERIAL_NUMBER));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(quint32(nGuestType)), EVT_PARAM_CREATE_UNATTENDED_FLOPPY_CMD_GUEST_DISTRO_TYPE));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmCreateUnattendedFloppy, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_CREATE_UNATTENDED_FLOPPY_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmCreateUnattendedFloppyCommandIsValidFailedOnGuestDistroTypeAbsent()
{
	VM_CREATE_UNATTENDED_FLOPPY_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserName, EVT_PARAM_SERIAL_NUM_CMD_USER_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sCompanyName, EVT_PARAM_SERIAL_NUM_CMD_COMPANY_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSerialNumber, EVT_PARAM_SERIAL_NUM_CMD_SERIAL_NUMBER));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmCreateUnattendedFloppy, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_CREATE_UNATTENDED_FLOPPY_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmCreateUnattendedFloppyCommandIsValidFailedOnUserNameAbsent()
{
	VM_CREATE_UNATTENDED_FLOPPY_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sCompanyName, EVT_PARAM_SERIAL_NUM_CMD_COMPANY_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSerialNumber, EVT_PARAM_SERIAL_NUM_CMD_SERIAL_NUMBER));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(quint32(nGuestType)), EVT_PARAM_CREATE_UNATTENDED_FLOPPY_CMD_GUEST_DISTRO_TYPE));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmCreateUnattendedFloppy, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_CREATE_UNATTENDED_FLOPPY_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmCreateUnattendedFloppyCommandIsValidFailedOnCompanyNameAbsent()
{
	VM_CREATE_UNATTENDED_FLOPPY_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserName, EVT_PARAM_SERIAL_NUM_CMD_USER_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sSerialNumber, EVT_PARAM_SERIAL_NUM_CMD_SERIAL_NUMBER));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(quint32(nGuestType)), EVT_PARAM_CREATE_UNATTENDED_FLOPPY_CMD_GUEST_DISTRO_TYPE));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmCreateUnattendedFloppy, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_CREATE_UNATTENDED_FLOPPY_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmCreateUnattendedFloppyCommandIsValidFailedOnSerialNumberAbsent()
{
	VM_CREATE_UNATTENDED_FLOPPY_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserName, EVT_PARAM_SERIAL_NUM_CMD_USER_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sCompanyName, EVT_PARAM_SERIAL_NUM_CMD_COMPANY_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(quint32(nGuestType)), EVT_PARAM_CREATE_UNATTENDED_FLOPPY_CMD_GUEST_DISTRO_TYPE));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmCreateUnattendedFloppy, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_CREATE_UNATTENDED_FLOPPY_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

#define FS_GENERATE_ENTRY_NAME_CMD_PARAMS_DECLARE\
	QString sDirPath = "some directory path";\
	QString sFilenamePrefix = "filename prefix";\
	QString	sFilenameSuffix = "filename suffix";\
	QString	sIndexDelimiter = " ";

void CProtoSerializerTest::testCreateDspCmdFsGenerateEntryNameCommand()
{
	FS_GENERATE_ENTRY_NAME_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspCmdFsGenerateEntryNameCommand(
								sDirPath,
								sFilenamePrefix,
								sFilenameSuffix,
								sIndexDelimiter);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_FS_GENERATE_ENTRY_NAME_CMD_DIRPATH, PVE::String, sDirPath)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_FS_GENERATE_ENTRY_NAME_CMD_FILENAME_PREFIX, PVE::String, sFilenamePrefix)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_FS_GENERATE_ENTRY_NAME_CMD_FILENAME_SUFFIX, PVE::String, sFilenameSuffix)
	//Workaround for QDom lib issue: can't to pass through string value with white spaces
	CHECK_EVENT_PARAMETER(pEvent,\
		EVT_PARAM_FS_GENERATE_ENTRY_NAME_CMD_INDEX_DELIMITER, PVE::String, "*" + sIndexDelimiter)
}

void CProtoSerializerTest::testParseDspCmdFsGenerateEntryNameCommand()
{
	FS_GENERATE_ENTRY_NAME_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sDirPath, EVT_PARAM_FS_GENERATE_ENTRY_NAME_CMD_DIRPATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sFilenamePrefix, EVT_PARAM_FS_GENERATE_ENTRY_NAME_CMD_FILENAME_PREFIX));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sFilenameSuffix, EVT_PARAM_FS_GENERATE_ENTRY_NAME_CMD_FILENAME_SUFFIX));
	//Workaround for QDom lib issue: can't to pass through string value with white spaces
	_pkg->addEventParameter(
		new CVmEventParameter(PVE::String, "*" + sIndexDelimiter, EVT_PARAM_FS_GENERATE_ENTRY_NAME_CMD_INDEX_DELIMITER)
		);
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdFsGenerateEntryName, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CProtoCommandDspCmdFsGenerateEntryName *pDspCmdFsGenerateEntryNameCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspCmdFsGenerateEntryName>(pCmd);
	QCOMPARE(sDirPath, pDspCmdFsGenerateEntryNameCmd->GetDirPath());
	QCOMPARE(sFilenamePrefix, pDspCmdFsGenerateEntryNameCmd->GetFilenamePrefix());
	QCOMPARE(sFilenameSuffix, pDspCmdFsGenerateEntryNameCmd->GetFilenameSuffix());
	QCOMPARE(sIndexDelimiter, pDspCmdFsGenerateEntryNameCmd->GetIndexDelimiter());
}

#define CHECK_FS_GENERATE_ENTRY_NAME_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CProtoCommandDspCmdFsGenerateEntryName *pDspCmdFsGenerateEntryNameCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspCmdFsGenerateEntryName>(pCmd);\
	pDspCmdFsGenerateEntryNameCmd->GetDirPath();\
	pDspCmdFsGenerateEntryNameCmd->GetFilenamePrefix();\
	pDspCmdFsGenerateEntryNameCmd->GetFilenameSuffix();\
	pDspCmdFsGenerateEntryNameCmd->GetIndexDelimiter();

void CProtoSerializerTest::testDspCmdFsGenerateEntryNameCommandIsValidFailedOnEmptyPackage()
{
	FS_GENERATE_ENTRY_NAME_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdFsGenerateEntryName, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_FS_GENERATE_ENTRY_NAME_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdFsGenerateEntryNameCommandIsValidFailedOnTargetDirPathAbsent()
{
	FS_GENERATE_ENTRY_NAME_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sFilenamePrefix, EVT_PARAM_FS_GENERATE_ENTRY_NAME_CMD_FILENAME_PREFIX));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sFilenameSuffix, EVT_PARAM_FS_GENERATE_ENTRY_NAME_CMD_FILENAME_SUFFIX));
	_pkg->addEventParameter(
		new CVmEventParameter(PVE::String, sIndexDelimiter, EVT_PARAM_FS_GENERATE_ENTRY_NAME_CMD_INDEX_DELIMITER)
		);
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdFsGenerateEntryName, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_FS_GENERATE_ENTRY_NAME_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdFsGenerateEntryNameCommandIsValidFailedOnFilenamePrefixAbsent()
{
	FS_GENERATE_ENTRY_NAME_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sDirPath, EVT_PARAM_FS_GENERATE_ENTRY_NAME_CMD_DIRPATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sFilenameSuffix, EVT_PARAM_FS_GENERATE_ENTRY_NAME_CMD_FILENAME_SUFFIX));
	_pkg->addEventParameter(
		new CVmEventParameter(PVE::String, sIndexDelimiter, EVT_PARAM_FS_GENERATE_ENTRY_NAME_CMD_INDEX_DELIMITER)
		);
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdFsGenerateEntryName, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_FS_GENERATE_ENTRY_NAME_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdFsGenerateEntryNameCommandIsValidFailedOnFilenameSuffixAbsent()
{
	FS_GENERATE_ENTRY_NAME_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sDirPath, EVT_PARAM_FS_GENERATE_ENTRY_NAME_CMD_DIRPATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sFilenamePrefix, EVT_PARAM_FS_GENERATE_ENTRY_NAME_CMD_FILENAME_PREFIX));
	_pkg->addEventParameter(
		new CVmEventParameter(PVE::String, sIndexDelimiter, EVT_PARAM_FS_GENERATE_ENTRY_NAME_CMD_INDEX_DELIMITER)
		);
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdFsGenerateEntryName, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_FS_GENERATE_ENTRY_NAME_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdFsGenerateEntryNameCommandIsValidFailedOnIndexDelimiterAbsent()
{
	FS_GENERATE_ENTRY_NAME_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(
		new CVmEventParameter(PVE::String, sDirPath, EVT_PARAM_FS_GENERATE_ENTRY_NAME_CMD_DIRPATH)
		);
	_pkg->addEventParameter(
		new CVmEventParameter(PVE::String, sFilenamePrefix, EVT_PARAM_FS_GENERATE_ENTRY_NAME_CMD_FILENAME_PREFIX)
		);
	_pkg->addEventParameter(
		new CVmEventParameter(PVE::String, sFilenameSuffix, EVT_PARAM_FS_GENERATE_ENTRY_NAME_CMD_FILENAME_SUFFIX)
		);
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdFsGenerateEntryName, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_FS_GENERATE_ENTRY_NAME_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

#define VM_UPDATE_SECURITY_CMD_PARAMS_DECLARE\
	QString sVmUuid = Uuid::createUuid().toString();\
	QString sVmSecurity = Uuid::createUuid().toString();

void CProtoSerializerTest::testCreateDspCmdVmUpdateSecurityCommand()
{
	VM_UPDATE_SECURITY_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd = CProtoSerializer::CreateVmUpdateSecurityCommand(sVmUuid, sVmSecurity);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_BASIC_VM_CMD_VM_UUID, PVE::String, sVmUuid)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_UPDATE_SECURITY_CMD_SECURITY_INFO, PVE::String, sVmSecurity)
}

void CProtoSerializerTest::testParseDspCmdVmUpdateSecurityCommand()
{
	VM_UPDATE_SECURITY_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmSecurity, EVT_PARAM_VM_UPDATE_SECURITY_CMD_SECURITY_INFO));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmUpdateSecurity, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	QCOMPARE(sVmUuid, pCmd->GetVmUuid());
	CProtoVmUpdateSecurityCommand *pVmUpdateSecurityCmd = CProtoSerializer::CastToProtoCommand<CProtoVmUpdateSecurityCommand>(pCmd);
	QCOMPARE(sVmSecurity, pVmUpdateSecurityCmd->GetVmSecurity());
}

#define CHECK_PROTO_VM_UPDATE_SECURITY_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	pCmd->GetVmUuid();\
	CProtoVmUpdateSecurityCommand *pVmUpdateSecurityCmd = CProtoSerializer::CastToProtoCommand<CProtoVmUpdateSecurityCommand>(pCmd);\
	pVmUpdateSecurityCmd->GetVmSecurity();

void CProtoSerializerTest::testDspCmdVmUpdateSecurityCommandIsValidFailedOnEmptyPackage()
{
	VM_UPDATE_SECURITY_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmUpdateSecurity, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_UPDATE_SECURITY_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmUpdateSecurityCommandIsValidFailedOnVmUuidAbsent()
{
	VM_UPDATE_SECURITY_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmSecurity, EVT_PARAM_VM_UPDATE_SECURITY_CMD_SECURITY_INFO));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmUpdateSecurity, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_UPDATE_SECURITY_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmUpdateSecurityCommandIsValidFailedOnVmSecurityAbsent()
{
	VM_UPDATE_SECURITY_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmUpdateSecurity, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_UPDATE_SECURITY_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdVmUpdateSecurity()
{
	CVmSecurity _vm_security;
	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdVmUpdateSecurity;
	PRL_RESULT nErrCode = PRL_ERR_SUCCESS;
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->SetVmSecurity(_vm_security.toString());
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());
	SmartPtr<CResult> pResult( new CResult );
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->FillResult(pResult.getImpl());
	QCOMPARE(_vm_security.toString(), pResult->m_hashResultSet[PVE::DspCmdVmUpdateSecurity_strSecurityInfo]);
	CHECK_STANDARD_RESULT_PARAMS
}

#define READ_VM_CONFIG_INTO_BUF(vm_config_path)\
	QFile _file(vm_config_path);\
	QVERIFY(_file.open(QIODevice::ReadOnly));\
	QTextStream _stream(&_file);\
	QString _config = _stream.readAll();

void CProtoSerializerTest::testDspCmdVmSectionValidateConfig()
{
	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml")
	PRL_VM_CONFIG_SECTIONS nSection = PVC_CPU;

	CProtoCommandPtr pCmd = CProtoSerializer::CreateVmValidateConfigProtoCommand(_config, nSection);
	QVERIFY(pCmd->GetCommandId() == PVE::DspCmdVmSectionValidateConfig);

	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_PROTO_FIRST_STR_PARAM, PVE::String, _config)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_CONFIG_SECTION, PVE::UnsignedInt, QString("%1").arg(quint32(nSection)))
}

void CProtoSerializerTest::testParseDspCmdVmSectionValidateConfig()
{
	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml")
	PRL_VM_CONFIG_SECTIONS nSection = PVC_NETWORK_ADAPTER;

	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, _config, EVT_PARAM_PROTO_FIRST_STR_PARAM));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(quint32(nSection)), EVT_PARAM_VM_CONFIG_SECTION));

	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmSectionValidateConfig, _pkg->toString());
	QVERIFY(pCmd->GetCommandId() == PVE::DspCmdVmSectionValidateConfig);
	QVERIFY(pCmd->IsValid());

	CProtoCreateVmValidateConfigCommand* pVmValidateConfig = CProtoSerializer::CastToProtoCommand<CProtoCreateVmValidateConfigCommand>(pCmd);

	QCOMPARE(_config, pVmValidateConfig->GetFirstStrParam());
	QVERIFY(nSection == pVmValidateConfig->GetSection());
}

void CProtoSerializerTest::testParseDspCmdVmSectionValidateConfigNotValid()
{
	READ_VM_CONFIG_INTO_BUF("./TestDspCmdDirValidateVmConfig_vm_config.xml")

	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, _config, EVT_PARAM_PROTO_FIRST_STR_PARAM));

	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmSectionValidateConfig, _pkg->toString());
	QVERIFY(pCmd->GetCommandId() == PVE::DspCmdVmSectionValidateConfig);
	QVERIFY(!pCmd->IsValid());
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdUserInfoList()
{
	const quint32 nTestIterationsNum = 10;
	QStringList lstUsers;
	for (quint32 i = 0; i < nTestIterationsNum; ++i)
	{
		SmartPtr<UserInfo> pUserInfo( new UserInfo );
		pUserInfo->setUuid(Uuid::createUuid().toString());
		lstUsers.append(pUserInfo->toString());
	}
	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdUserInfoList;
	PRL_RESULT nErrCode = PRL_ERR_SUCCESS;
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->SetParamsList(lstUsers);
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());
	SmartPtr<CResult> pResult( new CResult );
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->FillResult(pResult.getImpl());
	CHECK_STANDARD_RESULT_PARAMS
	QVERIFY(lstUsers.size() == pResult->GetParamsCount());
	QStringList lstActualUsers;
	for (int i = 0; i < pResult->GetParamsCount(); ++i)
		lstActualUsers.append(pResult->GetParamToken(i));
	foreach(QString sUser, lstUsers)
		QVERIFY(lstActualUsers.contains(sUser));
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdGetVirtualNetworkList()
{
	const quint32 nTestIterationsNum = 10;
	QStringList lstVirtNets;
	for (quint32 i = 0; i < nTestIterationsNum; ++i)
	{
		SmartPtr<CVirtualNetwork> pVirtualNetwork( new CVirtualNetwork );
		lstVirtNets.append(pVirtualNetwork->toString());
	}
	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdGetVirtualNetworkList;
	PRL_RESULT nErrCode = PRL_ERR_SUCCESS;
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->SetParamsList(lstVirtNets);
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());
	SmartPtr<CResult> pResult( new CResult );
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->FillResult(pResult.getImpl());
	CHECK_STANDARD_RESULT_PARAMS
	QVERIFY(lstVirtNets.size() == pResult->GetParamsCount());
	QStringList lstActualVirtNets;
	for (int i = 0; i < pResult->GetParamsCount(); ++i)
		lstActualVirtNets.append(pResult->GetParamToken(i));
	foreach(QString sVirtNet, lstVirtNets)
		QVERIFY(lstActualVirtNets.contains(sVirtNet));
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForDspCmdAllHostUsers()
{
	const quint32 nTestIterationsNum = 10;
	QStringList lstUsers;
	for (quint32 i = 0; i < nTestIterationsNum; ++i)
	{
		SmartPtr<CVmEvent> pUserInfo( new CVmEvent );
		pUserInfo->addEventParameter( new CVmEventParameter(PVE::String,
															Uuid::createUuid().toString(),
															EVT_PARAM_HOST_USER_SYS_NAME) );
		pUserInfo->addEventParameter( new CVmEventParameter(PVE::String,
															Uuid::createUuid().toString(),
															EVT_PARAM_HOST_USER_FRIENDLY_NAME) );
		pUserInfo->addEventParameter( new CVmEventParameter(PVE::String,
															Uuid::createUuid().toString(),
															EVT_PARAM_HOST_USER_ICON) );
		lstUsers.append(pUserInfo->toString());
	}
	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdAllHostUsers;
	PRL_RESULT nErrCode = PRL_ERR_SUCCESS;
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->SetParamsList(lstUsers);
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());
	SmartPtr<CResult> pResult( new CResult );
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->FillResult(pResult.getImpl());
	CHECK_STANDARD_RESULT_PARAMS
	QVERIFY(lstUsers.size() == pResult->GetParamsCount());
	QStringList lstActualUsers;
	for (int i = 0; i < pResult->GetParamsCount(); ++i)
		lstActualUsers.append(pResult->GetParamToken(i));
	foreach(QString sUser, lstUsers)
		QVERIFY(lstActualUsers.contains(sUser));
}

void CProtoSerializerTest::testCreateDspWsResponseCommandForGetUserInfo()
{
	SmartPtr<UserInfo> pUserInfo( new UserInfo );
	PVE::IDispatcherCommands nCmdIdentifier = PVE::DspCmdUserInfo;
	PRL_RESULT nErrCode = PRL_ERR_SUCCESS;
	CProtoCommandPtr pCmd = CProtoSerializer::CreateDspWsResponseCommand(nCmdIdentifier, nErrCode);
	CProtoCommandDspWsResponse *pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->SetUserInfo(pUserInfo->toString());
	pCmd = CProtoSerializer::ParseCommand(PVE::DspWsResponse, pCmd->GetCommand()->toString());
	SmartPtr<CResult> pResult( new CResult );
	pResponseCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspWsResponse>(pCmd);
	pResponseCmd->FillResult(pResult.getImpl());
	QCOMPARE(pUserInfo->toString(), pResult->m_hashResultSet[PVE::DspCmdUserInfo_strUserInfo]);
	CHECK_STANDARD_RESULT_PARAMS
}

#define VM_MIGRATE_CMD_PARAMS_DECLARE\
	QString sVmUuid = Uuid::createUuid().toString();\
	QString sTargetServerHostname = "some server hostname";\
	QString sTargetServerSessionUuid = Uuid::createUuid().toString();\
	QString sTargetVmName = "some VM name";\
	QString sTargetVmHomePath = "some VM home path";\
	quint32 nTargetServerPort = 25024;\
	PRL_UNUSED_PARAM(nTargetServerPort);\
	quint32 nMigrationFlags = PVMT_HOT_MIGRATION;\
	PRL_UNUSED_PARAM(nMigrationFlags);\
	quint32 nReservedFlags = 0;\
	PRL_UNUSED_PARAM(nReservedFlags);\
	bool bForceOperation = true;\
	PRL_UNUSED_PARAM(bForceOperation);

void CProtoSerializerTest::testCreateDspCmdVmMigrateCommand()
{
	VM_MIGRATE_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd = CProtoSerializer::CreateVmMigrateProtoCommand(sVmUuid, sTargetServerHostname,
																nTargetServerPort, sTargetServerSessionUuid,
																sTargetVmName, sTargetVmHomePath, nMigrationFlags,
																nReservedFlags, bForceOperation);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_BASIC_VM_CMD_VM_UUID, PVE::String, sVmUuid)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_HOSTNAME, PVE::String, sTargetServerHostname)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_PORT, PVE::UnsignedInt,\
																			QString("%1").arg(nTargetServerPort))
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_SESSION_UUID, PVE::String,\
																			sTargetServerSessionUuid)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_TARGET_VM_NAME, PVE::String,\
																			sTargetVmName)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH, PVE::String,\
																			sTargetVmHomePath)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_PROTO_CMD_FLAGS, PVE::UnsignedInt,\
																			QString("%1").arg(nMigrationFlags))
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS, PVE::UnsignedInt,\
																			QString("%1").arg(nReservedFlags))
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_PROTO_FORCE_QUESTIONS_SIGN, PVE::UnsignedInt,\
																			QString("%1").arg(bForceOperation))
}

void CProtoSerializerTest::testParseDspCmdVmMigrateCommand()
{
	VM_MIGRATE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetServerHostname,
														EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_HOSTNAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetServerSessionUuid,
													EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_SESSION_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmName,
													EVT_PARAM_MIGRATE_CMD_TARGET_VM_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmHomePath,
													EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nTargetServerPort),
													EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_PORT));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
													EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(bForceOperation),
													EVT_PARAM_PROTO_FORCE_QUESTIONS_SIGN));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmMigrate, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CProtoVmMigrateCommand *pVmMigrateCmd = CProtoSerializer::CastToProtoCommand<CProtoVmMigrateCommand>(pCmd);
	QCOMPARE(sVmUuid, pVmMigrateCmd->GetVmUuid());
	QCOMPARE(sTargetServerHostname, pVmMigrateCmd->GetTargetServerHostname());
	QCOMPARE(sTargetServerSessionUuid, pVmMigrateCmd->GetTargetServerSessionUuid());
	QCOMPARE(sTargetVmName, pVmMigrateCmd->GetTargetServerVmName());
	QCOMPARE(sTargetVmHomePath, pVmMigrateCmd->GetTargetServerVmHomePath());
	QCOMPARE(nTargetServerPort, pVmMigrateCmd->GetTargetServerPort());
	QCOMPARE(nMigrationFlags, pVmMigrateCmd->GetMigrationFlags());
	QCOMPARE(nReservedFlags, pVmMigrateCmd->GetReservedFlags());
	QCOMPARE(quint32(bForceOperation), quint32(pVmMigrateCmd->GetForceQuestionsSign()));
}

#define CHECK_PROTO_VM_MIGRATE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CProtoVmMigrateCommand *pVmMigrateCmd = CProtoSerializer::CastToProtoCommand<CProtoVmMigrateCommand>(pCmd);\
	pVmMigrateCmd->GetVmUuid();\
	pVmMigrateCmd->GetTargetServerHostname();\
	pVmMigrateCmd->GetTargetServerPort();\
	pVmMigrateCmd->GetTargetServerSessionUuid();\
	pVmMigrateCmd->GetMigrationFlags();\
	pVmMigrateCmd->GetReservedFlags();\
	pVmMigrateCmd->GetForceQuestionsSign();\
	pVmMigrateCmd->GetTargetServerVmName();\
	pVmMigrateCmd->GetTargetServerVmHomePath();

void CProtoSerializerTest::testDspCmdVmMigrateCommandIsValidFailedOnEmptyPackage()
{
	VM_MIGRATE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmMigrate, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_MIGRATE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmMigrateCommandIsValidFailedOnVmUuidAbsent()
{
	VM_MIGRATE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetServerHostname,
														EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_HOSTNAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetServerSessionUuid,
													EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_SESSION_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmHomePath,
													EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nTargetServerPort),
													EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_PORT));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
													EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmMigrate, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_MIGRATE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmMigrateCommandIsValidFailedOnTargetServerHostnameAbsent()
{
	VM_MIGRATE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetServerSessionUuid,
													EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_SESSION_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmHomePath,
													EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nTargetServerPort),
													EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_PORT));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
													EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmMigrate, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_MIGRATE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmMigrateCommandIsValidFailedOnTargetServerPortAbsent()
{
	VM_MIGRATE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetServerHostname,
														EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_HOSTNAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmHomePath,
													EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetServerSessionUuid,
													EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_SESSION_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
													EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmMigrate, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_MIGRATE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmMigrateCommandIsValidFailedOnTargetServerSessionUuidAbsent()
{
	VM_MIGRATE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetServerHostname,
														EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_HOSTNAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmHomePath,
													EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nTargetServerPort),
													EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_PORT));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
													EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmMigrate, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_MIGRATE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmMigrateCommandIsValidFailedOnTargetServerVmHomePathAbsent()
{
	VM_MIGRATE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetServerHostname,
														EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_HOSTNAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetServerSessionUuid,
													EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_SESSION_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nTargetServerPort),
													EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_PORT));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReservedFlags),
													EVT_PARAM_MIGRATE_CMD_RESERVED_FLAGS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmMigrate, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_MIGRATE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmMigrateCommandIsValidFailedOnReservedFlagsAbsent()
{
	VM_MIGRATE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetServerHostname,
														EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_HOSTNAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetVmHomePath,
													EVT_PARAM_MIGRATE_CMD_TARGET_VM_HOME_PATH));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nTargetServerPort),
													EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_PORT));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sTargetServerSessionUuid,
													EVT_PARAM_MIGRATE_CMD_TARGET_SERVER_SESSION_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nMigrationFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirVmMigrate, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_MIGRATE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

#define VM_START_EX_CMD_PARAMS_DECLARE \
	QString sVmUuid = Uuid::createUuid().toString();\
	PRL_UINT32 nStartMode = 1;\
	PRL_UNUSED_PARAM(nStartMode);\
	PRL_UINT32 nReserved = 0;\
	PRL_UNUSED_PARAM(nReserved);

void CProtoSerializerTest::testCreateDspCmdVmStartExCommand()
{
	VM_START_EX_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd = CProtoSerializer::CreateVmStartExProtoCommand(sVmUuid, nStartMode, nReserved);

	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_BASIC_VM_CMD_VM_UUID, PVE::String, sVmUuid);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_START_EX_CMD_START_MODE, PVE::UnsignedInt,\
																		QString("%1").arg(nStartMode));
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_START_EX_CMD_RESERVED, PVE::UnsignedInt,\
																		QString("%1").arg(nReserved));
}

void CProtoSerializerTest::testParseDspCmdVmStartExCommand()
{
	VM_START_EX_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nStartMode),
													EVT_PARAM_VM_START_EX_CMD_START_MODE));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReserved),
													EVT_PARAM_VM_START_EX_CMD_RESERVED));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmStartEx, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CProtoVmStartExCommand *pVmStartExCmd = CProtoSerializer::CastToProtoCommand<CProtoVmStartExCommand>(pCmd);
	QCOMPARE(sVmUuid, pVmStartExCmd->GetVmUuid());
	QCOMPARE(nStartMode, pVmStartExCmd->GetStartMode());
	QCOMPARE(nReserved, pVmStartExCmd->GetReservedParameter());
}

#define CHECK_PROTO_VM_START_EX_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CProtoVmStartExCommand *pVmStartExCmd = CProtoSerializer::CastToProtoCommand<CProtoVmStartExCommand>(pCmd);\
	pVmStartExCmd->GetVmUuid();\
	pVmStartExCmd->GetStartMode();\
	pVmStartExCmd->GetReservedParameter();

void CProtoSerializerTest::testDspCmdVmStartExCommandIsValidFailedOnEmptyPackage()
{
	VM_START_EX_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmStartEx, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_START_EX_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmStartExCommandIsValidFailedOnVmUuidAbsent()
{
	VM_START_EX_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nStartMode),
													EVT_PARAM_VM_START_EX_CMD_START_MODE));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReserved),
													EVT_PARAM_VM_START_EX_CMD_RESERVED));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmStartEx, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_START_EX_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmStartExCommandIsValidFailedOnStartModeAbsent()
{
	VM_START_EX_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nReserved),
													EVT_PARAM_VM_START_EX_CMD_RESERVED));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmStartEx, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_START_EX_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmStartExCommandIsValidFailedOnReservedParameterAbsent()
{
	VM_START_EX_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nStartMode),
													EVT_PARAM_VM_START_EX_CMD_START_MODE));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmStartEx, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_START_EX_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testCreateDspCmdVmInstallUtilityCommand()
{
	QString sVmUuid = Uuid::createUuid().toString();
	QString strId("kis");

	CProtoCommandPtr pCmd
		= CProtoSerializer::CreateProtoVmCommandWithOneStrParam(PVE::DspCmdVmInstallUtility, sVmUuid, strId);

	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_BASIC_VM_CMD_VM_UUID, PVE::String, sVmUuid);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_PROTO_FIRST_STR_PARAM, PVE::String, strId);
}

#define VM_LOGIN_IN_GUEST_CMD_PARAMS_DECLARE \
	QString sVmUuid = Uuid::createUuid().toString();\
	QString sUserName = "some user";\
	QString sUserPassword = "some password";\
	PRL_UINT32 nFlags = 0;\
	PRL_UNUSED_PARAM(nFlags);

void CProtoSerializerTest::testCreateDspCmdVmLoginInGuestCommand()
{
	VM_LOGIN_IN_GUEST_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd =
		CProtoSerializer::CreateVmLoginInGuestProtoCommand(sVmUuid, sUserName, sUserPassword, nFlags);

	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_BASIC_VM_CMD_VM_UUID, PVE::String, sVmUuid);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_LOGIN_IN_GUEST_CMD_USER_LOGIN, PVE::String, sUserName);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_LOGIN_IN_GUEST_CMD_USER_PASSWORD, PVE::String, sUserPassword);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_PROTO_CMD_FLAGS, PVE::UnsignedInt, QString("%1").arg(nFlags));
}

void CProtoSerializerTest::testParseDspCmdVmLoginInGuestCommand()
{
	VM_LOGIN_IN_GUEST_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserName,
													EVT_PARAM_VM_LOGIN_IN_GUEST_CMD_USER_LOGIN));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserPassword,
													EVT_PARAM_VM_LOGIN_IN_GUEST_CMD_USER_PASSWORD));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmLoginInGuest, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CProtoVmLoginInGuestCommand *pVmLoginInGuestCmd =
		CProtoSerializer::CastToProtoCommand<CProtoVmLoginInGuestCommand>(pCmd);
	QCOMPARE(sVmUuid, pVmLoginInGuestCmd->GetVmUuid());
	QCOMPARE(sUserName, pVmLoginInGuestCmd->GetUserLoginName());
	QCOMPARE(sUserPassword, pVmLoginInGuestCmd->GetUserPassword());
	QCOMPARE(nFlags, pVmLoginInGuestCmd->GetCommandFlags());
}

#define CHECK_PROTO_VM_LOGIN_IN_GUEST_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CProtoVmLoginInGuestCommand *pVmLoginInGuestCmd =\
		CProtoSerializer::CastToProtoCommand<CProtoVmLoginInGuestCommand>(pCmd);\
	pVmLoginInGuestCmd->GetVmUuid();\
	pVmLoginInGuestCmd->GetUserLoginName();\
	pVmLoginInGuestCmd->GetUserPassword();\
	pVmLoginInGuestCmd->GetCommandFlags();

void CProtoSerializerTest::testDspCmdVmLoginInGuestCommandIsValidFailedOnEmptyPackage()
{
	VM_LOGIN_IN_GUEST_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmLoginInGuest, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_LOGIN_IN_GUEST_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmLoginInGuestCommandIsValidFailedOnVmUuidAbsent()
{
	VM_LOGIN_IN_GUEST_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserName,
													EVT_PARAM_VM_LOGIN_IN_GUEST_CMD_USER_LOGIN));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserPassword,
													EVT_PARAM_VM_LOGIN_IN_GUEST_CMD_USER_PASSWORD));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmLoginInGuest, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_LOGIN_IN_GUEST_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmLoginInGuestCommandIsValidFailedOnUserLoginAbsent()
{
	VM_LOGIN_IN_GUEST_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserPassword,
													EVT_PARAM_VM_LOGIN_IN_GUEST_CMD_USER_PASSWORD));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmLoginInGuest, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_LOGIN_IN_GUEST_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmLoginInGuestCommandIsValidFailedOnUserPasswordAbsent()
{
	VM_LOGIN_IN_GUEST_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserName,
													EVT_PARAM_VM_LOGIN_IN_GUEST_CMD_USER_LOGIN));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmLoginInGuest, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_LOGIN_IN_GUEST_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testCreateDspCmdVmAuthWithGuestSecurityDbCommand()
{
	VM_LOGIN_IN_GUEST_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd =
		CProtoSerializer::CreateVmAuthWithGuestSecurityDbProtoCommand(sVmUuid, sUserName, sUserPassword, nFlags);

	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_BASIC_VM_CMD_VM_UUID, PVE::String, sVmUuid);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_LOGIN_IN_GUEST_CMD_USER_LOGIN, PVE::String, sUserName);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_LOGIN_IN_GUEST_CMD_USER_PASSWORD, PVE::String, sUserPassword);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_PROTO_CMD_FLAGS, PVE::UnsignedInt, QString("%1").arg(nFlags));
}

void CProtoSerializerTest::testParseDspCmdVmAuthWithGuestSecurityDbCommand()
{
	VM_LOGIN_IN_GUEST_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserName,
													EVT_PARAM_VM_LOGIN_IN_GUEST_CMD_USER_LOGIN));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserPassword,
													EVT_PARAM_VM_LOGIN_IN_GUEST_CMD_USER_PASSWORD));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmAuthWithGuestSecurityDb, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CProtoVmLoginInGuestCommand *pVmLoginInGuestCmd =
		CProtoSerializer::CastToProtoCommand<CProtoVmLoginInGuestCommand>(pCmd);
	QCOMPARE(sVmUuid, pVmLoginInGuestCmd->GetVmUuid());
	QCOMPARE(sUserName, pVmLoginInGuestCmd->GetUserLoginName());
	QCOMPARE(sUserPassword, pVmLoginInGuestCmd->GetUserPassword());
	QCOMPARE(nFlags, pVmLoginInGuestCmd->GetCommandFlags());
}

#define VM_GUEST_LOGOUT_CMD_PARAMS_DECLARE \
	QString sVmUuid = Uuid::createUuid().toString();\
	QString sVmGuestUuid = Uuid::createUuid().toString();\
	PRL_UINT32 nFlags = 0;\
	PRL_UNUSED_PARAM(nFlags);

void CProtoSerializerTest::testCreateDspCmdVmGuestLogoutCommand()
{
	VM_GUEST_LOGOUT_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd =
		CProtoSerializer::CreateBasicVmGuestProtoCommand(PVE::DspCmdVmGuestLogout, sVmUuid, sVmGuestUuid, nFlags);

	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_BASIC_VM_CMD_VM_UUID, PVE::String, sVmUuid);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_GUEST_CMD_SESSION_ID, PVE::String, sVmGuestUuid);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_PROTO_CMD_FLAGS, PVE::UnsignedInt, QString("%1").arg(nFlags));
}

#define TEST_PARSE_VM_GUEST_BASIC_COMMAND(nCmdType)\
	VM_GUEST_LOGOUT_CMD_PARAMS_DECLARE\
	SmartPtr<CVmEvent> _pkg( new CVmEvent );\
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));\
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmGuestUuid,\
													EVT_PARAM_VM_GUEST_CMD_SESSION_ID));\
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),\
													EVT_PARAM_PROTO_CMD_FLAGS));\
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::nCmdType, _pkg->toString());\
	QVERIFY(pCmd->IsValid());\
	CProtoBasicVmGuestCommand *pVmGuestLogoutCmd =\
		CProtoSerializer::CastToProtoCommand<CProtoBasicVmGuestCommand>(pCmd);\
	QCOMPARE(sVmUuid, pVmGuestLogoutCmd->GetVmUuid());\
	QCOMPARE(sVmGuestUuid, pVmGuestLogoutCmd->GetVmSessionUuid());\
	QCOMPARE(nFlags, pVmGuestLogoutCmd->GetCommandFlags());

void CProtoSerializerTest::testParseDspCmdVmGuestLogoutCommand()
{
	TEST_PARSE_VM_GUEST_BASIC_COMMAND(DspCmdVmGuestLogout)
}

#define CHECK_PROTO_VM_GUEST_LOGOUT_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CProtoBasicVmGuestCommand *pVmGuestLogoutCmd =\
		CProtoSerializer::CastToProtoCommand<CProtoBasicVmGuestCommand>(pCmd);\
	pVmGuestLogoutCmd->GetVmUuid();\
	pVmGuestLogoutCmd->GetVmSessionUuid();\
	pVmGuestLogoutCmd->GetCommandFlags();

void CProtoSerializerTest::testDspCmdVmGuestLogoutCommandIsValidFailedOnEmptyPackage()
{
	VM_GUEST_LOGOUT_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmGuestLogout, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_GUEST_LOGOUT_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmGuestLogoutCommandIsValidFailedOnVmUuidAbsent()
{
	VM_GUEST_LOGOUT_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmGuestUuid,
													EVT_PARAM_VM_GUEST_CMD_SESSION_ID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmGuestLogout, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_GUEST_LOGOUT_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmGuestLogoutCommandIsValidFailedOnSessionUuidAbsent()
{
	VM_GUEST_LOGOUT_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmGuestLogout, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_GUEST_LOGOUT_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testParseDspCmdVmGuestGetNetworkSettingsCommand()
{
	TEST_PARSE_VM_GUEST_BASIC_COMMAND(DspCmdVmGuestGetNetworkSettings)
}

#define VM_GUEST_RUN_PROGRAM_CMD_PARAMS_DECLARE \
	QString sVmUuid = Uuid::createUuid().toString();\
	QString sVmGuestUuid = Uuid::createUuid().toString();\
	QString sProgramName = "app_name.exe";\
	QStringList lstArgs = QStringList()<<"arg1"<<"arg2"<<"arg3";\
	QStringList lstVars = QStringList()<<"var1=value1"<<"var2=value2"<<"var3=value3";\
	PRL_UINT32 nFlags = PFD_ALL | PRPM_RUN_PROGRAM_AND_RETURN_IMMEDIATELY;\
	PRL_UNUSED_PARAM(nFlags);

void CProtoSerializerTest::testCreateDspCmdVmGuestRunProgramCommand()
{
	VM_GUEST_RUN_PROGRAM_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd =
		CProtoSerializer::CreateVmGuestRunProgramProtoCommand(sVmUuid, sVmGuestUuid, sProgramName,
				lstArgs, lstVars, nFlags);

	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_BASIC_VM_CMD_VM_UUID, PVE::String, sVmUuid);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_GUEST_CMD_SESSION_ID, PVE::String, sVmGuestUuid);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_GUEST_RUN_APP_CMD_PROGRAM_NAME, PVE::String, sProgramName);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_PROTO_CMD_FLAGS, PVE::UnsignedInt, QString("%1").arg(nFlags));
	{
		CVmEventParameter *pParam = pEvent->getEventParameter(EVT_PARAM_PROTO_CMD_FLAGS);
		QVERIFY(pParam != NULL);
		QVERIFY(pParam->getParamType() == PVE::UnsignedInt);
		QVERIFY(pParam->getParamValue().toUInt() & PFD_ALL);
		QVERIFY(pParam->getParamValue().toUInt() & PRPM_RUN_PROGRAM_AND_RETURN_IMMEDIATELY);
	}
	{
		CVmEventParameter *pParam = pEvent->getEventParameter(EVT_PARAM_VM_GUEST_RUN_APP_CMD_ARGUMENTS);
		QVERIFY(pParam != NULL);
		QVERIFY(pParam->getParamType() == PVE::String);
		QVERIFY(pParam->isList());
		QVERIFY(pParam->getValuesList() == lstArgs);
	}
	{
		CVmEventParameter *pParam = pEvent->getEventParameter(EVT_PARAM_VM_GUEST_RUN_APP_CMD_ENV_VARS);
		QVERIFY(pParam != NULL);
		QVERIFY(pParam->getParamType() == PVE::String);
		QVERIFY(pParam->isList());
		QVERIFY(pParam->getValuesList() == lstVars);
	}
}

void CProtoSerializerTest::testParseDspCmdVmGuestRunProgramCommand()
{
	VM_GUEST_RUN_PROGRAM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmGuestUuid,
													EVT_PARAM_VM_GUEST_CMD_SESSION_ID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sProgramName,
													EVT_PARAM_VM_GUEST_RUN_APP_CMD_PROGRAM_NAME));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstArgs,
													EVT_PARAM_VM_GUEST_RUN_APP_CMD_ARGUMENTS));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstVars,
													EVT_PARAM_VM_GUEST_RUN_APP_CMD_ENV_VARS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmGuestRunProgram, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CProtoVmGuestRunProgramCommand *pVmGuestRunProgramCmd =
		CProtoSerializer::CastToProtoCommand<CProtoVmGuestRunProgramCommand>(pCmd);
	QCOMPARE(sVmUuid, pVmGuestRunProgramCmd->GetVmUuid());
	QCOMPARE(sVmGuestUuid, pVmGuestRunProgramCmd->GetVmSessionUuid());
	QCOMPARE(nFlags, pVmGuestRunProgramCmd->GetCommandFlags());
	QCOMPARE(sProgramName, pVmGuestRunProgramCmd->GetProgramName());
	QVERIFY(lstArgs == pVmGuestRunProgramCmd->GetProgramArguments());
	QVERIFY(lstVars == pVmGuestRunProgramCmd->GetProgramEnvVars());
}

#define CHECK_PROTO_VM_GUEST_RUN_APP_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CProtoVmGuestRunProgramCommand *pVmGuestRunProgramCmd =\
		CProtoSerializer::CastToProtoCommand<CProtoVmGuestRunProgramCommand>(pCmd);\
	pVmGuestRunProgramCmd->GetVmUuid();\
	pVmGuestRunProgramCmd->GetVmSessionUuid();\
	pVmGuestRunProgramCmd->GetCommandFlags();\
	pVmGuestRunProgramCmd->GetProgramName();\
	pVmGuestRunProgramCmd->GetProgramArguments();\
	pVmGuestRunProgramCmd->GetProgramEnvVars();

void CProtoSerializerTest::testDspCmdVmGuestRunProgramCommandIsValidFailedOnEmptyPackage()
{
	VM_GUEST_RUN_PROGRAM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmGuestRunProgram, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_GUEST_RUN_APP_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmGuestRunProgramCommandIsValidFailedOnVmUuidAbsent()
{
	VM_GUEST_RUN_PROGRAM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmGuestUuid,
													EVT_PARAM_VM_GUEST_CMD_SESSION_ID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sProgramName,
													EVT_PARAM_VM_GUEST_RUN_APP_CMD_PROGRAM_NAME));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstArgs,
													EVT_PARAM_VM_GUEST_RUN_APP_CMD_ARGUMENTS));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstVars,
													EVT_PARAM_VM_GUEST_RUN_APP_CMD_ENV_VARS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmGuestRunProgram, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_GUEST_RUN_APP_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmGuestRunProgramCommandIsValidFailedOnSessionUuidAbsent()
{
	VM_GUEST_RUN_PROGRAM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sProgramName,
													EVT_PARAM_VM_GUEST_RUN_APP_CMD_PROGRAM_NAME));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstArgs,
													EVT_PARAM_VM_GUEST_RUN_APP_CMD_ARGUMENTS));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstVars,
													EVT_PARAM_VM_GUEST_RUN_APP_CMD_ENV_VARS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmGuestRunProgram, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_GUEST_RUN_APP_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmGuestRunProgramCommandIsValidFailedOnProgramNameAbsent()
{
	VM_GUEST_RUN_PROGRAM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmGuestUuid,
													EVT_PARAM_VM_GUEST_CMD_SESSION_ID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstArgs,
													EVT_PARAM_VM_GUEST_RUN_APP_CMD_ARGUMENTS));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstVars,
													EVT_PARAM_VM_GUEST_RUN_APP_CMD_ENV_VARS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmGuestRunProgram, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_GUEST_RUN_APP_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmGuestRunProgramCommandIsValidFailedOnArgsListAbsent()
{
	VM_GUEST_RUN_PROGRAM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmGuestUuid,
													EVT_PARAM_VM_GUEST_CMD_SESSION_ID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sProgramName,
													EVT_PARAM_VM_GUEST_RUN_APP_CMD_PROGRAM_NAME));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstVars,
													EVT_PARAM_VM_GUEST_RUN_APP_CMD_ENV_VARS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmGuestRunProgram, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_GUEST_RUN_APP_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmGuestRunProgramCommandIsValidFailedOnEnvsListAbsent()
{
	VM_GUEST_RUN_PROGRAM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmGuestUuid,
													EVT_PARAM_VM_GUEST_CMD_SESSION_ID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sProgramName,
													EVT_PARAM_VM_GUEST_RUN_APP_CMD_PROGRAM_NAME));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, lstArgs,
													EVT_PARAM_VM_GUEST_RUN_APP_CMD_ARGUMENTS));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmGuestRunProgram, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_GUEST_RUN_APP_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

#define VM_GUEST_SET_USER_PASSWD_CMD_PARAMS_DECLARE \
	QString sVmUuid = Uuid::createUuid().toString();\
	QString sVmGuestUuid = Uuid::createUuid().toString();\
	QString sUserLoginName = "some user login";\
	QString sUserPassword = "some user password";\
	PRL_UINT32 nFlags = 0;\
	PRL_UNUSED_PARAM(nFlags);

void CProtoSerializerTest::testCreateDspCmdVmGuestSetUserPasswdCommand()
{
	VM_GUEST_SET_USER_PASSWD_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd =
		CProtoSerializer::CreateVmGuestSetUserPasswdProtoCommand(sVmUuid, sVmGuestUuid, sUserLoginName,
				sUserPassword, nFlags);

	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_BASIC_VM_CMD_VM_UUID, PVE::String, sVmUuid);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_GUEST_CMD_SESSION_ID, PVE::String, sVmGuestUuid);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_GUEST_SET_USER_PASSWD_CMD_USER_LOGIN_NAME, PVE::String, sUserLoginName);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_VM_GUEST_SET_USER_PASSWD_CMD_USER_PASSWD, PVE::String, sUserPassword);
}

void CProtoSerializerTest::testParseDspCmdVmGuestSetUserPasswdCommand()
{
	VM_GUEST_SET_USER_PASSWD_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmGuestUuid,
													EVT_PARAM_VM_GUEST_CMD_SESSION_ID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserLoginName,
													EVT_PARAM_VM_GUEST_SET_USER_PASSWD_CMD_USER_LOGIN_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserPassword,
													EVT_PARAM_VM_GUEST_SET_USER_PASSWD_CMD_USER_PASSWD));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmGuestSetUserPasswd, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CProtoVmGuestSetUserPasswdCommand *pVmGuestSetUserPasswdCmd =
		CProtoSerializer::CastToProtoCommand<CProtoVmGuestSetUserPasswdCommand>(pCmd);
	QCOMPARE(sVmUuid, pVmGuestSetUserPasswdCmd->GetVmUuid());
	QCOMPARE(sVmGuestUuid, pVmGuestSetUserPasswdCmd->GetVmSessionUuid());
	QCOMPARE(nFlags, pVmGuestSetUserPasswdCmd->GetCommandFlags());
	QCOMPARE(sUserLoginName, pVmGuestSetUserPasswdCmd->GetUserLoginName());
	QCOMPARE(sUserPassword, pVmGuestSetUserPasswdCmd->GetUserPassword());
}

#define CHECK_PROTO_VM_GUEST_SET_USER_PASSWD_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CProtoVmGuestSetUserPasswdCommand *pVmGuestSetUserPasswdCmd =\
		CProtoSerializer::CastToProtoCommand<CProtoVmGuestSetUserPasswdCommand>(pCmd);\
	pVmGuestSetUserPasswdCmd->GetVmUuid();\
	pVmGuestSetUserPasswdCmd->GetVmSessionUuid();\
	pVmGuestSetUserPasswdCmd->GetCommandFlags();\
	pVmGuestSetUserPasswdCmd->GetUserLoginName();\
	pVmGuestSetUserPasswdCmd->GetUserPassword();

void CProtoSerializerTest::testDspCmdVmGuestSetUserPasswdCommandIsValidFailedOnEmptyPackage()
{
	VM_GUEST_SET_USER_PASSWD_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmGuestSetUserPasswd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_GUEST_SET_USER_PASSWD_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmGuestSetUserPasswdCommandIsValidFailedOnVmUuidAbsent()
{
	VM_GUEST_SET_USER_PASSWD_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmGuestUuid,
													EVT_PARAM_VM_GUEST_CMD_SESSION_ID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserLoginName,
													EVT_PARAM_VM_GUEST_SET_USER_PASSWD_CMD_USER_LOGIN_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserPassword,
													EVT_PARAM_VM_GUEST_SET_USER_PASSWD_CMD_USER_PASSWD));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmGuestSetUserPasswd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_GUEST_SET_USER_PASSWD_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmGuestSetUserPasswdCommandIsValidFailedOnSessionUuidAbsent()
{
	VM_GUEST_SET_USER_PASSWD_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserLoginName,
													EVT_PARAM_VM_GUEST_SET_USER_PASSWD_CMD_USER_LOGIN_NAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserPassword,
													EVT_PARAM_VM_GUEST_SET_USER_PASSWD_CMD_USER_PASSWD));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmGuestSetUserPasswd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_GUEST_SET_USER_PASSWD_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmGuestSetUserPasswdCommandIsValidFailedOnUserLoginNameAbsent()
{
	VM_GUEST_SET_USER_PASSWD_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmGuestUuid,
													EVT_PARAM_VM_GUEST_CMD_SESSION_ID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserPassword,
													EVT_PARAM_VM_GUEST_SET_USER_PASSWD_CMD_USER_PASSWD));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmGuestSetUserPasswd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_GUEST_SET_USER_PASSWD_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdVmGuestSetUserPasswdCommandIsValidFailedOnUserPasswdAbsent()
{
	VM_GUEST_SET_USER_PASSWD_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmUuid, EVT_PARAM_BASIC_VM_CMD_VM_UUID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmGuestUuid,
													EVT_PARAM_VM_GUEST_CMD_SESSION_ID));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserLoginName,
													EVT_PARAM_VM_GUEST_SET_USER_PASSWD_CMD_USER_LOGIN_NAME));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmGuestSetUserPasswd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_VM_GUEST_SET_USER_PASSWD_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

#define VM_CONFIRM_MODE_CMD_PARAMS_DECLARE\
	bool bEnable = true;	\
	QString sUsername = Uuid::createUuid().toString(); \
	QString sPassword = Uuid::createUuid().toString(); \
	quint32 nFlags = 0x555; \
	bool bForceOperation = false;\


void CProtoSerializerTest::testCreateDspCmdSetSessionConfirmationMode()
{
	VM_CONFIRM_MODE_CMD_PARAMS_DECLARE;
	CProtoCommandPtr
		pCmd = CProtoSerializer::CreateSetSessionConfirmationModeProtoCommand( bEnable, sUsername, sPassword, nFlags );

	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_COMFIRMATION_MODE_CMD_USERNAME, PVE::String, sUsername)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_COMFIRMATION_MODE_CMD_PASSWORD, PVE::String, sPassword)
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_COMFIRMATION_MODE_CMD_ENABLE_FLAG, PVE::UnsignedInt,\
																			QString("%1").arg(bEnable))
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_PROTO_CMD_FLAGS, PVE::UnsignedInt,\
																			QString("%1").arg(nFlags))
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_PROTO_FORCE_QUESTIONS_SIGN, PVE::UnsignedInt,\
																			QString("%1").arg(bForceOperation))
}

void CProtoSerializerTest::testParseDspCmdSetSessionConfirmationMode()
{
	VM_CONFIRM_MODE_CMD_PARAMS_DECLARE;
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUsername, EVT_PARAM_COMFIRMATION_MODE_CMD_USERNAME));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sPassword, EVT_PARAM_COMFIRMATION_MODE_CMD_PASSWORD));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(bEnable),
													EVT_PARAM_COMFIRMATION_MODE_CMD_ENABLE_FLAG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(bForceOperation),
													EVT_PARAM_PROTO_FORCE_QUESTIONS_SIGN));

	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdSetSessionConfirmationMode, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CProtoCommandDspCmdSetSessionConfirmationMode
		*pConfirmCmd = CProtoSerializer::CastToProtoCommand<CProtoCommandDspCmdSetSessionConfirmationMode>(pCmd);
	QCOMPARE(sUsername, pConfirmCmd->GetUserLoginName());
	QCOMPARE(sPassword, pConfirmCmd->GetPassword());
	QCOMPARE(nFlags, pConfirmCmd->GetCommandFlags());
	QCOMPARE(quint32(bForceOperation), quint32(pConfirmCmd->GetForceQuestionsSign()));
}

#define STORAGE_SET_VALUE_CMD_PARAMS_DECLARE\
	QString sKey = Uuid::createUuid().toString(); \
	QString sValue = Uuid::createUuid().toString(); \
	quint32 nFlags = 0x555;

#define VM_STORAGE_SET_VALUE_CMD_PARAMS_DECLARE\
	STORAGE_SET_VALUE_CMD_PARAMS_DECLARE \
	QString sVmUuid = Uuid::createUuid().toString();

void CProtoSerializerTest::testCreateDspCmdStorageSetValueCommand()
{
	STORAGE_SET_VALUE_CMD_PARAMS_DECLARE;
	CProtoCommandPtr
		pCmd = CProtoSerializer::CreateDspCmdStorageSetValueCommand( sKey, sValue, nFlags );

	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_STORAGE_CMD_KEY, PVE::String, sKey);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_STORAGE_CMD_VALUE, PVE::String, sValue);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_PROTO_CMD_FLAGS, PVE::UnsignedInt,\
																			QString("%1").arg(nFlags));
}
void CProtoSerializerTest::testParseDspCmdStorageSetValueCommand()
{
	STORAGE_SET_VALUE_CMD_PARAMS_DECLARE;
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sKey, EVT_PARAM_STORAGE_CMD_KEY));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sValue, EVT_PARAM_STORAGE_CMD_VALUE));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
		EVT_PARAM_PROTO_CMD_FLAGS));

	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdStorageSetValue, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CProtoDspCmdStorageSetValueCommand
		*pConfirmCmd = CProtoSerializer::CastToProtoCommand<CProtoDspCmdStorageSetValueCommand>(pCmd);
	QCOMPARE(sKey, pConfirmCmd->GetKey());
	QCOMPARE(sValue, pConfirmCmd->GetValue());
	QCOMPARE(nFlags, pConfirmCmd->GetCommandFlags());
}
void CProtoSerializerTest::testCreateDspCmdVmStorageSetValueCommand()
{
	VM_STORAGE_SET_VALUE_CMD_PARAMS_DECLARE;
	CProtoCommandPtr
		pCmd = CProtoSerializer::CreateDspCmdVmStorageSetValueCommand( sVmUuid, sKey, sValue, nFlags );

	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_STORAGE_CMD_KEY, PVE::String, sKey);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_STORAGE_CMD_VALUE, PVE::String, sValue);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_PROTO_CMD_FLAGS, PVE::UnsignedInt,\
		QString("%1").arg(nFlags));
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_BASIC_VM_CMD_VM_UUID, PVE::String, sVmUuid);

}

void CProtoSerializerTest::testParseDspCmdVmStorageSetValueCommand()
{
	VM_STORAGE_SET_VALUE_CMD_PARAMS_DECLARE;
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sKey, EVT_PARAM_STORAGE_CMD_KEY));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sValue, EVT_PARAM_STORAGE_CMD_VALUE));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
		EVT_PARAM_PROTO_CMD_FLAGS));

	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdVmStorageSetValue, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CProtoDspCmdVmStorageSetValueCommand
		*pConfirmCmd = CProtoSerializer::CastToProtoCommand<CProtoDspCmdVmStorageSetValueCommand>(pCmd);
	QCOMPARE(sKey, pConfirmCmd->GetKey());
	QCOMPARE(sValue, pConfirmCmd->GetValue());
	QCOMPARE(nFlags, pConfirmCmd->GetCommandFlags());
}

#define SRV_REG_3RD_PARTY_VM_CMD_PARAMS_DECLARE \
	QString sVmConfigPath = Uuid::createUuid().toString();\
	QString sVmRootDirPath = Uuid::createUuid().toString();\
	PRL_UINT32 nFlags = 0;\
	PRL_UNUSED_PARAM(nFlags);

void CProtoSerializerTest::testCreateDspCmdDirReg3rdPartyVmCommand()
{
	SRV_REG_3RD_PARTY_VM_CMD_PARAMS_DECLARE
	CProtoCommandPtr pCmd =
		CProtoSerializer::CreateDspCmdDirReg3rdPartyVmProtoCommand(sVmConfigPath, sVmRootDirPath, nFlags);

	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_SRV_REG_3RD_PARTY_VM_CMD_PATH_TO_CONFIG, PVE::String, sVmConfigPath);
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_SRV_REG_3RD_PARTY_VM_CMD_PATH_TO_ROOT_DIR, PVE::String, sVmRootDirPath);
}

void CProtoSerializerTest::testParseDspCmdDirReg3rdPartyVmCommand()
{
	SRV_REG_3RD_PARTY_VM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmConfigPath,
													EVT_PARAM_SRV_REG_3RD_PARTY_VM_CMD_PATH_TO_CONFIG));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmRootDirPath,
													EVT_PARAM_SRV_REG_3RD_PARTY_VM_CMD_PATH_TO_ROOT_DIR));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirReg3rdPartyVm, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CProtoDspCmdDirReg3rdPartyVmCommand *pDspCmdDirReg3rdPartyVmCmd =
		CProtoSerializer::CastToProtoCommand<CProtoDspCmdDirReg3rdPartyVmCommand>(pCmd);
	QCOMPARE(nFlags, pDspCmdDirReg3rdPartyVmCmd->GetCommandFlags());
	QCOMPARE(sVmConfigPath, pDspCmdDirReg3rdPartyVmCmd->GetPathToVmConfig());
	QCOMPARE(sVmRootDirPath, pDspCmdDirReg3rdPartyVmCmd->GetPathToVmRootDir());
}

#define CHECK_PROTO_SRV_REG_3RD_PARTY_VM_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CProtoDspCmdDirReg3rdPartyVmCommand *pDspCmdDirReg3rdPartyVmCmd =\
		CProtoSerializer::CastToProtoCommand<CProtoDspCmdDirReg3rdPartyVmCommand>(pCmd);\
	pDspCmdDirReg3rdPartyVmCmd->GetCommandFlags();\
	pDspCmdDirReg3rdPartyVmCmd->GetPathToVmConfig();\
	pDspCmdDirReg3rdPartyVmCmd->GetPathToVmRootDir();

void CProtoSerializerTest::testDspCmdDirReg3rdPartyVmCommandIsValidFailedOnEmptyPackage()
{
	SRV_REG_3RD_PARTY_VM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirReg3rdPartyVm, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_SRV_REG_3RD_PARTY_VM_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdDirReg3rdPartyVmCommandIsValidFailedOnPathToVmConfigAbsent()
{
	SRV_REG_3RD_PARTY_VM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmRootDirPath,
													EVT_PARAM_SRV_REG_3RD_PARTY_VM_CMD_PATH_TO_ROOT_DIR));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirReg3rdPartyVm, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_SRV_REG_3RD_PARTY_VM_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testDspCmdDirReg3rdPartyVmCommandIsValidFailedOnPathToRootVmDirAbsent()
{
	SRV_REG_3RD_PARTY_VM_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nFlags),
													EVT_PARAM_PROTO_CMD_FLAGS));
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sVmConfigPath,
													EVT_PARAM_SRV_REG_3RD_PARTY_VM_CMD_PATH_TO_CONFIG));
	CProtoCommandPtr pCmd = CProtoSerializer::ParseCommand(PVE::DspCmdDirReg3rdPartyVm, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_PROTO_SRV_REG_3RD_PARTY_VM_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CProtoSerializerTest::testCreateDspCmdRefreshPluginsCommand()
{
	CProtoCommandPtr pCmd = CProtoSerializer::CreateProtoCommandWithoutParams(PVE::DspCmdRefreshPlugins);
	QVERIFY(pCmd->IsValid());
}

void CProtoSerializerTest::testParseDspCmdRefreshPluginsCommand()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DspCmdRefreshPlugins)
}

void CProtoSerializerTest::testCreateDspCmdGetPluginsListCommand()
{
	PROTO_CMD_WITH_ONE_STR_PARAM_PARAMS_DECLARE
	CProtoCommandPtr pCmd = CProtoSerializer::CreateProtoCommandWithOneStrParam(PVE::DspCmdGetPluginsList, sParam);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_PROTO_FIRST_STR_PARAM, PVE::String, sParam)
}

void CProtoSerializerTest::testParseDspCmdGetPluginsListCommand()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdGetPluginsList)
}

void CProtoSerializerTest::testCreateDspCmdCtlLicenseChangeCommand()
{
	PROTO_CMD_WITH_ONE_STR_PARAM_PARAMS_DECLARE
	CProtoCommandPtr pCmd = CProtoSerializer::CreateProtoCommandWithOneStrParam(PVE::DspCmdCtlLicenseChange, sParam);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_PROTO_FIRST_STR_PARAM, PVE::String, sParam)
}

void CProtoSerializerTest::testParseDspCmdCtlLicenseChangeCommand()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdCtlLicenseChange)
}
void CProtoSerializerTest::testParseCommandForDspCmdGetVmConfigById()
{
	TEST_PARSE_COMMAND_WITH_ONE_STR_PARAM(DspCmdGetVmConfigById)
}
