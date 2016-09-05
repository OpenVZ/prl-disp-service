/////////////////////////////////////////////////////////////////////////////
///
///	@file CDispToDispProtoSerializerTest.cpp
///
///	Tests fixture class for testing dispatcher-dispatcher protocol serializer.
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

#include "CDispToDispProtoSerializerTest.h"
#include "Libraries/DispToDispProtocols/CDispToDispCommonProto.h"
#include <prlcommon/Messaging/CVmEvent.h>
#include <prlcommon/Messaging/CVmEventParameterList.h>
#include "Tests/CommonTestsUtils.h"

using namespace Parallels;

#define AUTHORIZE_CMD_PARAMS_DECLARE\
	QString sUserSessionUuid = "some user session UUID";

void CDispToDispProtoSerializerTest::testCreateDispToDispAuthorizeCommand()
{
	AUTHORIZE_CMD_PARAMS_DECLARE
	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::CreateDispToDispAuthorizeCommand(sUserSessionUuid);
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_DISP_TO_DISP_AUTHORIZE_CMD_USER_SESSION_UUID, PVE::String,\
		sUserSessionUuid)
}

void CDispToDispProtoSerializerTest::testParseDispToDispAuthorizeCommand()
{
	AUTHORIZE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, sUserSessionUuid,
							EVT_PARAM_DISP_TO_DISP_AUTHORIZE_CMD_USER_SESSION_UUID));
	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(DispToDispAuthorizeCmd, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CDispToDispAuthorizeCommand *pDispToDispAuthorizeCmd =
				CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispAuthorizeCommand>(pCmd);
	QCOMPARE(sUserSessionUuid, pDispToDispAuthorizeCmd->GetUserSessionUuid());
}

#define CHECK_AUTHORIZE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CDispToDispAuthorizeCommand *pDispToDispAuthorizeCmd =\
		CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispAuthorizeCommand>(pCmd);\
	pDispToDispAuthorizeCmd->GetUserSessionUuid();

void CDispToDispProtoSerializerTest::testDispToDispAuthorizeCommandIsValidFailedOnEmptyPackage()
{
	AUTHORIZE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(DispToDispAuthorizeCmd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_AUTHORIZE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CDispToDispProtoSerializerTest::testCreateDispToDispLogoffCommand()
{
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::CreateDispToDispCommandWithoutParams(DispToDispLogoffCmd);
	QVERIFY(pCmd->IsValid());
}

#define TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(cmd_identifier)\
	SmartPtr<CVmEvent> _pkg( new CVmEvent );\
	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(cmd_identifier, _pkg->toString());\
	QVERIFY(pCmd->IsValid());

void CDispToDispProtoSerializerTest::testParseDispToDispLogoffCommand()
{
	TEST_PARSE_PROTO_COMMAND_WITHOUT_PARAMS(DispToDispLogoffCmd)
}

#define RESPONSE_CMD_PARAMS_DECLARE\
	IDispToDispCommands nRequestCmdId = DispToDispLogoffCmd;\
	Q_UNUSED(nRequestCmdId);\
	PRL_RESULT nRetCode = PRL_ERR_UNIMPLEMENTED;\
	SmartPtr<CVmEvent> pErrorEvent(new CVmEvent);\
	pErrorEvent->setEventCode(nRetCode);\
	pErrorEvent->addEventParameter(new CVmEventParameter(PVE::String, "some param value 1", "param1"));\
	pErrorEvent->addEventParameter(new CVmEventParameter(PVE::String, "some param value 2", "param2"));\
	pErrorEvent->addEventParameter(new CVmEventParameter(PVE::String, "some param value 3", "param3"));

void CDispToDispProtoSerializerTest::testCreateDispToDispResponseCommand()
{
	RESPONSE_CMD_PARAMS_DECLARE
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::CreateDispToDispResponseCommand(
			nRequestCmdId,
			nRetCode,
			pErrorEvent.getImpl()
		);
	QVERIFY(pCmd->IsValid());
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_ERROR_INFO, PVE::String,\
		pErrorEvent->toString())
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_REQUEST_ID, PVE::UnsignedInt,\
		QString("%1").arg(nRequestCmdId))
	CVmEventParameter *pParam = pEvent->getEventParameter(EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_PARAMS_LIST);
	QVERIFY(pParam && pParam->isList());
}

void CDispToDispProtoSerializerTest::testCreateDispToDispResponseCommand2()
{
	RESPONSE_CMD_PARAMS_DECLARE
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::CreateDispToDispResponseCommand(
			nRequestCmdId,
			nRetCode
		);
	QVERIFY(pCmd->IsValid());
	SmartPtr<CVmEvent> pEvent = pCmd->GetCommand();
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_ERROR_INFO, PVE::String,\
		QString(""))
	CHECK_EVENT_PARAMETER(pEvent, EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_REQUEST_ID, PVE::UnsignedInt,\
		QString("%1").arg(nRequestCmdId))
	CVmEventParameter *pParam = pEvent->getEventParameter(EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_PARAMS_LIST);
	QVERIFY(pParam && pParam->isList());
}

void CDispToDispProtoSerializerTest::testParseDispToDispResponseCommand()
{
	RESPONSE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->setEventCode(nRetCode);
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, pErrorEvent->toString(),
							EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_ERROR_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nRequestCmdId),
							EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_REQUEST_ID));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, QStringList(),
							EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_PARAMS_LIST));
	CDispToDispCommandPtr pCmd = CDispToDispProtoSerializer::ParseCommand(DispToDispResponseCmd, _pkg->toString());
	QVERIFY(pCmd->IsValid());
	CDispToDispResponseCommand *pDispToDispResponseCmd =
				CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pCmd);
	QCOMPARE(pErrorEvent->toString(), pDispToDispResponseCmd->GetErrorInfo()->toString());
	QCOMPARE((quint32)nRequestCmdId, (quint32)pDispToDispResponseCmd->GetRequestCommandId());
	QCOMPARE((quint32)nRetCode, (quint32)pDispToDispResponseCmd->GetRetCode());
}

#define CHECK_RESPONSE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS\
	CDispToDispResponseCommand *pDispToDispResponseCmd =\
		CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pCmd);\
	pDispToDispResponseCmd->GetErrorInfo();\
	pDispToDispResponseCmd->GetRequestCommandId();\
	pDispToDispResponseCmd->GetRetCode();\
	pDispToDispResponseCmd->GetParams();\
	pDispToDispResponseCmd->AddParam(Uuid::createUuid().toString());

void CDispToDispProtoSerializerTest::testDispToDispResponseCommandIsValidFailedOnEmptyPackage()
{
	RESPONSE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(DispToDispResponseCmd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_RESPONSE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CDispToDispProtoSerializerTest::testDispToDispResponseCommandIsValidFailedOnRequestIdAbsent()
{
	RESPONSE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, pErrorEvent->toString(),
							EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_ERROR_INFO));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, QStringList(),
							EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_PARAMS_LIST));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(DispToDispResponseCmd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_RESPONSE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CDispToDispProtoSerializerTest::testDispToDispResponseCommandIsValidFailedOnErrorInfoAbsent()
{
	RESPONSE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nRequestCmdId),
							EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_REQUEST_ID));
	_pkg->addEventParameter(new CVmEventParameterList(PVE::String, QStringList(),
							EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_PARAMS_LIST));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(DispToDispResponseCmd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_RESPONSE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CDispToDispProtoSerializerTest::testDispToDispResponseCommandProcessingListOfParams()
{
	RESPONSE_CMD_PARAMS_DECLARE
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::CreateDispToDispResponseCommand(
			nRequestCmdId,
			nRetCode,
			pErrorEvent.getImpl()
		);
	QVERIFY(pCmd->IsValid());
	CDispToDispResponseCommand *pResponseCmd =
		CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pCmd);
	pResponseCmd->AddParam(pErrorEvent->toString());
	pResponseCmd->AddParam(pErrorEvent->toString());
	pResponseCmd->AddParam(pErrorEvent->toString());
	pCmd = CDispToDispProtoSerializer::ParseCommand(DispToDispResponseCmd, pCmd->GetCommand()->toString());
	pResponseCmd = CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pCmd);
	QStringList lstParams = pResponseCmd->GetParams();
	QCOMPARE(quint32(lstParams.size()), quint32(3));
	foreach(QString sParam, lstParams)
		QCOMPARE(pErrorEvent->toString(), sParam);
}

void CDispToDispProtoSerializerTest::testDispToDispResponseCommandIsValidFailedOnListOfParamsAbsent()
{
	RESPONSE_CMD_PARAMS_DECLARE
	SmartPtr<CVmEvent> _pkg( new CVmEvent );
	_pkg->addEventParameter(new CVmEventParameter(PVE::String, pErrorEvent->toString(),
							EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_ERROR_INFO));
	_pkg->addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString("%1").arg(nRequestCmdId),
							EVT_PARAM_DISP_TO_DISP_RESPONSE_CMD_REQUEST_ID));
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::ParseCommand(DispToDispResponseCmd, _pkg->toString());
	QVERIFY(!pCmd->IsValid());
	CHECK_RESPONSE_CMD_NOT_FAILED_ON_GETTING_NON_PRESENTS_PARAMS
}

void CDispToDispProtoSerializerTest::testDispToDispResponseCommandProcessingListOfParams2()
{
	RESPONSE_CMD_PARAMS_DECLARE
	QStringList lstParams;
	lstParams.append(pErrorEvent->toString());
	lstParams.append(pErrorEvent->toString());
	lstParams.append(pErrorEvent->toString());
	CDispToDispCommandPtr pCmd =
		CDispToDispProtoSerializer::CreateDispToDispResponseCommand(
			nRequestCmdId,
			nRetCode,
			pErrorEvent.getImpl(),
			lstParams
		);
	QVERIFY(pCmd->IsValid());
	pCmd = CDispToDispProtoSerializer::ParseCommand(DispToDispResponseCmd, pCmd->GetCommand()->toString());
	CDispToDispResponseCommand *pResponseCmd =
					CDispToDispProtoSerializer::CastToDispToDispCommand<CDispToDispResponseCommand>(pCmd);
	lstParams = pResponseCmd->GetParams();
	QCOMPARE(quint32(lstParams.size()), quint32(3));
	foreach(QString sParam, lstParams)
		QCOMPARE(pErrorEvent->toString(), sParam);
}
