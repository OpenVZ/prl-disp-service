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
///		TestDspCmdUserLogin.cpp
///
/// @author
///		sergeyt
///
/// @brief
///		Tests fixture class for testing DspCmdUserLogin dispatcher API command functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#include "TestDspCmdUserLogin.h"

#include <QtTest/QtTest>

#include "Tests/CommonTestsUtils.h"

#include "SDK/Handles/PveControl.h"
#include <prlxmlmodel/Messaging/CVmEvent.h>
#include <prlxmlmodel/Messaging/CVmEventParameter.h>
#include <prlxmlmodel/DispConfig/CDispUser.h>

namespace {
	char g_sEmptyValue[] = "";
}

void TestDspCmdUserLogin::init() {
	m_pHandler->Clear();
	m_pPveControl = new CPveControl(false, m_pHandler);
}

void TestDspCmdUserLogin::cleanup() {
	if (QTest::currentTestFunction() == QString("TestDspCmdUserLogin::TestWithValidUser()") ||
			QTest::currentTestFunction() == QString("TestDspCmdUserLogin::TestWithValidUser2()") )
		Logoff();
	m_pPveControl->deleteLater();
	m_pPveControl = NULL;
}

void TestDspCmdUserLogin::cleanupTestCase() {
	CleanupTooLongString();
}

#define GET_RESULT\
	CResult _result = m_pHandler->GetResult();\
	QCOMPARE(_result.getOpCode(), PVE::DspCmdUserLogin);

void TestDspCmdUserLogin::TestWithValidUser() {
	CALL_CMD(m_pPveControl->DspCmdUserLogin(TestConfig::getRemoteHostName(), TestConfig::getUserLogin(),
											TestConfig::getUserPassword(), "", PSL_HIGH_SECURITY,
											CONNECTION_TIMEOUT), PVE::DspCmdUserLogin)
	GET_RESULT
	CHECK_RET_CODE(_result.getReturnCode())
}

void TestDspCmdUserLogin::TestWithValidUser2() {
	CALL_CMD(m_pPveControl->DspCmdUserLogin(TestConfig::getRemoteHostName(), TestConfig::getUserLogin2(),
											TestConfig::getUserPassword(), "", PSL_HIGH_SECURITY,
											CONNECTION_TIMEOUT), PVE::DspCmdUserLogin)
	GET_RESULT
	CHECK_RET_CODE(_result.getReturnCode())
}

void TestDspCmdUserLogin::TestWithWrongUser() {
	static char g_sWrongUserName[] = "WrongUserName";
	CALL_CMD(m_pPveControl->DspCmdUserLogin(TestConfig::getRemoteHostName(), g_sWrongUserName,
											TestConfig::getUserPassword(), "", PSL_HIGH_SECURITY,
											CONNECTION_TIMEOUT), PVE::DspCmdUserLogin)
	GET_RESULT
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspCmdUserLogin::TestWithWrongPassword() {
	static char g_sWrongPassword[] = "WrongPassword";
	CALL_CMD(m_pPveControl->DspCmdUserLogin(TestConfig::getRemoteHostName(), TestConfig::getUserLogin(),
											g_sWrongPassword, "", PSL_HIGH_SECURITY,
											CONNECTION_TIMEOUT), PVE::DspCmdUserLogin)
	GET_RESULT
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspCmdUserLogin::TestWithRootUserWithoutPassword() {
#ifdef _WIN_
	static char g_sRootUser[] = "Administrator";
#else
	static char g_sRootUser[] = "root";
#endif
	CALL_CMD(m_pPveControl->DspCmdUserLogin(TestConfig::getRemoteHostName(), g_sRootUser,
											g_sEmptyValue, "", PSL_HIGH_SECURITY,
											CONNECTION_TIMEOUT), PVE::DspCmdUserLogin)
	GET_RESULT
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspCmdUserLogin::TestWithEmptyUser() {
	CALL_CMD(m_pPveControl->DspCmdUserLogin(TestConfig::getRemoteHostName(), g_sEmptyValue,
											TestConfig::getUserPassword(), "", PSL_HIGH_SECURITY,
											CONNECTION_TIMEOUT), PVE::DspCmdUserLogin)
	GET_RESULT
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspCmdUserLogin::TestWithEmptyPassword() {
	CALL_CMD(m_pPveControl->DspCmdUserLogin(TestConfig::getRemoteHostName(), TestConfig::getUserLogin(),
											g_sEmptyValue, "", PSL_HIGH_SECURITY,
											CONNECTION_TIMEOUT), PVE::DspCmdUserLogin)
	GET_RESULT
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspCmdUserLogin::TestWithBothUserAndPasswordEmpty() {
	CALL_CMD(m_pPveControl->DspCmdUserLogin(TestConfig::getRemoteHostName(), g_sEmptyValue,
											g_sEmptyValue, "", PSL_HIGH_SECURITY,
											CONNECTION_TIMEOUT), PVE::DspCmdUserLogin)
	GET_RESULT
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspCmdUserLogin::TestWithTooLongStringUser() {
	InitializeTooLongString();
	CALL_CMD(m_pPveControl->DspCmdUserLogin(TestConfig::getRemoteHostName(), GetTooLongString(),
											TestConfig::getUserPassword(), "", PSL_HIGH_SECURITY,
											CONNECTION_TIMEOUT), PVE::DspCmdUserLogin)
	GET_RESULT
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspCmdUserLogin::TestCaseSensivityUserName()
{
#	ifndef _WIN_
	QSKIP ( "Test only windows mode", SkipSingle );
#	endif

	QString reqId;
	QString login = TestConfig::getUserLogin();
	QString login2 = login.toUpper();

	QVERIFY ( login != login2);

	try
	{
		QString uuid1 = getUserId_byUserName( login );

		//delete PVEControl
		cleanup();
		init();

		QString uuid2 = getUserId_byUserName( login2 );

		QVERIFY( uuid1 == uuid2 );
	}
	catch (const char* err)
	{
		QFAIL( err );
	}

}

QString TestDspCmdUserLogin::getUserId_byUserName( const QString& loginName )
{
	CResult _result;
	static const char* errCommon ="errCommon" ;
	static const char* errOpFailed ="errOpFailed" ;
	static const char* errErrorProfile ="errErrorProfile" ;

#	ifdef PROCESS_RESULT
#		error "already defined PROCESS_RESULT"
#	endif

#	define  PROCESS_RESULT( cmd )  \
	{ \
		_result = m_pHandler->GetResult(); \
		if ( _result.getOpCode() != cmd ) \
			throw errCommon; \
									  \
		if ( PRL_FAILED( _result.getReturnCode() ) ) \
			throw errOpFailed;  \
	}

	CALL_CMD(m_pPveControl->DspCmdUserLogin(TestConfig::getRemoteHostName(), loginName.toUtf8().data(),
											TestConfig::getUserPassword(), "", PSL_HIGH_SECURITY,
											CONNECTION_TIMEOUT), PVE::DspCmdUserLogin)
	PROCESS_RESULT ( PVE::DspCmdUserLogin );

	CALL_CMD(m_pPveControl->DspCmdUserGetProfile(), PVE::DspCmdUserGetProfile)
	PROCESS_RESULT ( PVE::DspCmdUserGetProfile );

	QString	 profile_xml = _result.getResultItemByKey( PVE::DspCmdUserGetProfile_strUserProfile );
	CDispUser profile;
	if ( ! IS_OPERATION_SUCCEEDED( profile.fromString( profile_xml ) ))
		throw errErrorProfile;

	QString userUuid = profile.getUserId();

	CALL_CMD(m_pPveControl->DspCmdUserLogoff(), PVE::DspCmdUserLogoff)
	PROCESS_RESULT ( PVE::DspCmdUserLogoff );

	return userUuid;

#	undef  PROCESS_RESULT

}
