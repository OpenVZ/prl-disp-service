/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlApiBasicsTest.cpp
///
/// User profile related api test
///
///	@author andreydanin
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

#include "PrlUserProfileTest.h"
#include <Interfaces/ParallelsLogging.h>

#include "XmlModel/DispConfig/CDispUser.h"

#include "Tests/CommonTestsUtils.h"


void PrlUserProfileTest::initTestCase()
{
	CHECK_RET_CODE_EXP(PrlSrv_Create(m_hServer.GetHandlePtr()))

	SdkHandleWrap hJob(PrlSrv_LoginLocal(m_hServer, "", 0, PSL_HIGH_SECURITY));
	QVERIFY(hJob.get() != PRL_INVALID_HANDLE);
	CHECK_JOB_RET_CODE(hJob)
}


void PrlUserProfileTest::cleanupTestCase()
{
	PrlSrv_Logoff(m_hServer);
}


#define CHECK_RET_CODE_EXP_EXCEPTION(_expr) \
	{ \
		PRL_RESULT nExprRetCode = (_expr); \
		if (PRL_FAILED(nExprRetCode)) { \
			WRITE_TRACE(DBG_FATAL, "Expression: '%s' nRetCode=%.8X '%s'", \
				#_expr, nExprRetCode, PRL_RESULT_TO_STRING(nExprRetCode)); \
			throw nExprRetCode; \
		} \
	}

namespace {
	QString UserProfileToString(const SdkHandleWrap& hUserProfile)
	{
		PRL_VOID_PTR pUserProfile = 0;
		CHECK_RET_CODE_EXP_EXCEPTION(PrlUsrCfg_ToString(hUserProfile, &pUserProfile))
			QString sActualUserProfile = UTF8_2QSTR((const char *)pUserProfile);
		PrlBuffer_Free(pUserProfile);

		return sActualUserProfile;
	}
} // anonimous namespace

#define USER_PROFILE_TO_STRING(hUserProfile, sUserProfile) \
	do { \
		try { \
			sUserProfile = UserProfileToString(hUserProfile); \
		} catch (...) { \
			QFAIL("Can't convert user profile to string"); \
		} \
	} while (false);


#define GET_USER_PROFILE \
	GetUserProfile(); \
	QVERIFY(m_hUserProfile != PRL_INVALID_HANDLE); \
	CHECK_HANDLE_TYPE(m_hUserProfile, PHT_USER_PROFILE) \

void PrlUserProfileTest::GetUserProfile()
{
	SdkHandleWrap hJob(PrlSrv_GetUserProfile(m_hServer));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	QVERIFY(PRL_SUCCEEDED(PrlResult_GetParam(hResult, m_hUserProfile.GetHandlePtr())));
	CHECK_HANDLE_TYPE(m_hUserProfile, PHT_USER_PROFILE)
}


void PrlUserProfileTest::testGetUserProfile()
{
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))

	SdkHandleWrap hJob(PrlSrv_LoginLocal(hServer, "", 0, PSL_HIGH_SECURITY));
	QVERIFY(hJob.get() != PRL_INVALID_HANDLE);
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlSrv_GetUserProfile(hServer));
	CHECK_JOB_RET_CODE(hJob)

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))

	SdkHandleWrap hUserProfile;
	QVERIFY(PRL_SUCCEEDED(PrlResult_GetParam(hResult, hUserProfile.GetHandlePtr())));
	CHECK_HANDLE_TYPE(hUserProfile, PHT_USER_PROFILE)

	QString sUserProfile;
	USER_PROFILE_TO_STRING(hUserProfile, sUserProfile)
	WRITE_TRACE(DBG_DEBUG, "User profile:\n%s", QSTR2UTF8(sUserProfile));
}


void PrlUserProfileTest::testSetProxy()
{
	GET_USER_PROFILE

	SdkHandleWrap hJob(PrlSrv_UserProfileBeginEdit(m_hServer));
	CHECK_JOB_RET_CODE(hJob)

	QString sUserProfile;
	USER_PROFILE_TO_STRING(m_hUserProfile, sUserProfile)
	WRITE_TRACE(DBG_DEBUG, "Before:\n%s", QSTR2UTF8(sUserProfile));

	CHECK_RET_CODE_EXP(PrlUsrCfg_AddProxy(
		m_hUserProfile,
		"proxy.qa.sw.ru",
		8080,
		"guest1",
		"1q2w",
		0
		));

	CHECK_RET_CODE_EXP(PrlUsrCfg_AddProxy(
		m_hUserProfile,
		"proxy.qa.sw.ru",
		8010,
		"guest1",
		"nopass:)",
		0
		));

	USER_PROFILE_TO_STRING(m_hUserProfile, sUserProfile)
	WRITE_TRACE(DBG_DEBUG, "%s After:\n%s", __FUNCTION__, QSTR2UTF8(sUserProfile));

	CDispUser userProfile;
	userProfile.fromString(sUserProfile);
	CDispUserCachedData* data = userProfile.getUserCachedData();

	int nQaProxyCount = 0;
	foreach(CDispUserCachedPasword* pass, data->m_lstUserCachedPaswords)
	{
		if (pass->getProxyServerName() == "proxy.qa.sw.ru")
		{
			++nQaProxyCount;
			QVERIFY(pass->getProxyServerPassword() == "nopass:)");
			QVERIFY(pass->getProxyServerPort() == 8010);
		}
	}
	QCOMPARE(nQaProxyCount, 1);

	hJob.reset(PrlSrv_UserProfileCommit(m_hServer, m_hUserProfile));
	CHECK_JOB_RET_CODE(hJob)

	GET_USER_PROFILE
	USER_PROFILE_TO_STRING(m_hUserProfile, sUserProfile)
	WRITE_TRACE(DBG_DEBUG, "%s Commited:\n%s", __FUNCTION__, QSTR2UTF8(sUserProfile));

	userProfile.fromString(sUserProfile);
	data = userProfile.getUserCachedData();

	int nStoredCount = 0;
	foreach(CDispUserCachedPasword* pass, data->m_lstUserCachedPaswords)
	{
		if (pass->getProxyServerName() == "proxy.qa.sw.ru")
		{
			QVERIFY(pass->getProxyServerPort() == 8010);
			++nStoredCount;
		}
	}
	QCOMPARE(nStoredCount, 1);
}


void PrlUserProfileTest::testSetProxy2()
{
	// Second attemp was wailed in first implementation of keychain helper
	// so check it in testcase
	testSetProxy();
	testSetProxy();
}
