/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlIPPrivateNetworkTest.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing private network manipulating SDK API.
///
///	@author dim
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
#include "PrlIPPrivateNetworkTest.h"
#include "Tests/CommonTestsUtils.h"
#include <prlcommon/Interfaces/ParallelsQt.h>
#include "XmlModel/NetworkConfig/CPrivateNetwork.h"
#include "XmlModel/HostHardwareInfo/CHostHardwareInfo.h"

#ifdef _LIN_
#include "Libraries/Virtuozzo/CVzHelper.h"
#endif

void PrlIPPrivateNetworkTest::init()
{
	m_ServerHandle.reset();
	CHECK_RET_CODE_EXP(PrlSrv_Create(m_ServerHandle.GetHandlePtr()));

	m_PrivNetHandle.reset();
	m_JobHandle.reset();
}

void PrlIPPrivateNetworkTest::cleanup()
{
	// remove all networks if connected
	m_JobHandle.reset(PrlSrv_GetIPPrivateNetworksList(m_ServerHandle, 0));
	PRL_RESULT nJobWaitRetCode = PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
	PRL_RESULT ret_code = PRL_ERR_UNINITIALIZED;
	if (PRL_SUCCEEDED(nJobWaitRetCode))
		CHECK_RET_CODE_EXP(PrlJob_GetRetCode(m_JobHandle, &ret_code));
	if (PRL_SUCCEEDED(ret_code))
	{
		SdkHandleWrap hResult;
		CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()))
		PRL_UINT32 nCount = 0;
		CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nCount))
		for (PRL_UINT32 i = 0; i < nCount; ++i)
		{
			CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, i, m_PrivNetHandle.GetHandlePtr()));
			CHECK_HANDLE_TYPE(m_PrivNetHandle, PHT_IPPRIV_NET);
			m_JobHandle.reset(PrlSrv_RemoveIPPrivateNetwork(m_ServerHandle, m_PrivNetHandle, 0));
			CHECK_JOB_RET_CODE(m_JobHandle);
		}
	}

	// disconnect from server
	m_JobHandle.reset(PrlSrv_Logoff(m_ServerHandle));
	PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
}

void PrlIPPrivateNetworkTest::testLoginLocal()
{
	m_JobHandle.reset(PrlSrv_LoginLocal(m_ServerHandle, "", 0, PSL_HIGH_SECURITY));
	QVERIFY(m_JobHandle != PRL_INVALID_HANDLE);
	CHECK_JOB_RET_CODE(m_JobHandle);
}

void PrlIPPrivateNetworkTest::testCreateIPPrivateNetwork()
{
	CHECK_RET_CODE_EXP(PrlIPPrivNet_Create(m_PrivNetHandle.GetHandlePtr()));
	CHECK_HANDLE_TYPE(m_PrivNetHandle, PHT_IPPRIV_NET);
}

void PrlIPPrivateNetworkTest::testCreateIPPrivateNetworkOnWrongParams()
{
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlIPPrivNet_Create(NULL), PRL_ERR_INVALID_ARG);
}

#define IPPRIV_NET_TO_XML_OBJECT \
	PRL_VOID_PTR pXml = 0; \
	CHECK_RET_CODE_EXP(PrlHandle_ToString(m_PrivNetHandle, &pXml)); \
	CPrivateNetwork priv_net; \
	priv_net.fromString( UTF8_2QSTR((PRL_CONST_STR )pXml) ); \
	PrlBuffer_Free(pXml);

#define IPPRIV_NET_FROM_XML_OBJECT \
	CHECK_RET_CODE_EXP(PrlHandle_FromString( m_PrivNetHandle, QSTR2UTF8(priv_net.toString()) ));

void PrlIPPrivateNetworkTest::testName()
{
	testCreateIPPrivateNetwork();

	IPPRIV_NET_TO_XML_OBJECT;
	priv_net.setName("id 1");
	IPPRIV_NET_FROM_XML_OBJECT;

	QString qsName = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsName, m_PrivNetHandle, PrlIPPrivNet_GetName);
	QVERIFY(qsName == "id 1");

	QString qsNameExpected = "text for test";
	CHECK_RET_CODE_EXP(PrlIPPrivNet_SetName(m_PrivNetHandle, QSTR2UTF8(qsNameExpected)));
	PRL_EXTRACT_STRING_VALUE(qsName, m_PrivNetHandle, PrlIPPrivNet_GetName);

	QCOMPARE(qsName, qsNameExpected);
}

void PrlIPPrivateNetworkTest::testNameOnWrongParams()
{
	testCreateIPPrivateNetwork();

	QString qsStr = "some text";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlIPPrivNet_GetName(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlIPPrivNet_SetName(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlIPPrivNet_GetName(m_PrivNetHandle, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlIPPrivNet_SetName(m_PrivNetHandle, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlIPPrivNet_SetName(m_PrivNetHandle, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlIPPrivNet_GetName(m_PrivNetHandle, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
}

void PrlIPPrivateNetworkTest::testNetAddresses()
{
	testCreateIPPrivateNetwork();

	IPPRIV_NET_TO_XML_OBJECT;
	QStringList lstNetAddresses = QStringList()<<"1.2.3.4/1.2.3.0:24"
		<<"5.6.7.8/5.6.7.0:24"<<"9.10.11.12/9.10.11.0:24";
	priv_net.setNetAddresses(lstNetAddresses);
	IPPRIV_NET_FROM_XML_OBJECT;

	PRL_CHECK_STRINGS_LIST(m_PrivNetHandle, PrlIPPrivNet_GetNetAddresses, lstNetAddresses)

	QStringList lstNewNetAddresses = QStringList()<<"192.168.1.1/255.255.255.0"<<"10.30.1.1/255.255.255.0";
	SdkHandleWrap hNewNetAddressesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hNewNetAddressesList.GetHandlePtr()))
	foreach(QString sNetAddress, lstNewNetAddresses)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewNetAddressesList, QSTR2UTF8(sNetAddress)))
	CHECK_RET_CODE_EXP(PrlIPPrivNet_SetNetAddresses(m_PrivNetHandle, hNewNetAddressesList))
	PRL_CHECK_STRINGS_LIST(m_PrivNetHandle, PrlIPPrivNet_GetNetAddresses, lstNewNetAddresses)

}

void PrlIPPrivateNetworkTest::testNetAddressesOnWrongParams()
{
	testCreateIPPrivateNetwork();
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlIPPrivNet_GetNetAddresses(m_PrivNetHandle, 0), PRL_ERR_INVALID_ARG)

	SdkHandleWrap hNetAddressesList;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlIPPrivNet_GetNetAddresses(PRL_INVALID_HANDLE, \
				hNetAddressesList.GetHandlePtr()), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlIPPrivNet_SetNetAddresses(PRL_INVALID_HANDLE, PRL_INVALID_HANDLE),\
			PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlIPPrivNet_GetNetAddresses(m_ServerHandle, hNetAddressesList.GetHandlePtr()),\
		PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlIPPrivNet_SetNetAddresses(m_ServerHandle, PRL_INVALID_HANDLE),\
			PRL_ERR_INVALID_ARG)

	QStringList lstEmptyNetAddresses;
	CHECK_RET_CODE_EXP(PrlIPPrivNet_SetNetAddresses(m_PrivNetHandle, PRL_INVALID_HANDLE))
	PRL_CHECK_STRINGS_LIST(m_PrivNetHandle, PrlIPPrivNet_GetNetAddresses, lstEmptyNetAddresses)
}

void PrlIPPrivateNetworkTest::testGlobal()
{
	testCreateIPPrivateNetwork();

	IPPRIV_NET_TO_XML_OBJECT;
	priv_net.setGlobal(true);
	IPPRIV_NET_FROM_XML_OBJECT;

	PRL_BOOL bGlobal;
	CHECK_RET_CODE_EXP(PrlIPPrivNet_IsGlobal(m_PrivNetHandle, &bGlobal));
	QVERIFY(!!bGlobal == true);

	CHECK_RET_CODE_EXP(PrlIPPrivNet_SetGlobal(m_PrivNetHandle, (PRL_BOOL)false));
	CHECK_RET_CODE_EXP(PrlIPPrivNet_IsGlobal(m_PrivNetHandle, &bGlobal));
	QVERIFY(bGlobal == false);
}

void PrlIPPrivateNetworkTest::testGlobalOnWrongParams()
{
	testCreateIPPrivateNetwork();

	// Wrong handle
	PRL_BOOL bGlobal;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlIPPrivNet_IsGlobal(m_ServerHandle, &bGlobal),
							PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlIPPrivNet_SetGlobal(m_ServerHandle, false),
							PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlIPPrivNet_IsGlobal(m_PrivNetHandle, NULL),
							PRL_ERR_INVALID_ARG);
}

#ifdef _LIN_
#define SKIP_ON_NON_PSBM	\
	if (!CVzHelper::is_vz_running())	\
		QSKIP("Doesn't make sense for non-PSBM mode", SkipAll);
#else
#define SKIP_ON_NON_PSBM	\
	QSKIP("Doesn't make sense for non-PSBM mode", SkipAll);
#endif

#define TEST_CREATE_NET1 \
	testCreateIPPrivateNetwork(); \
	QStringList lstNetAddresses1 = QStringList()<<"1.2.3.4/24"<<"5.6.7.8/24"<<"9.10.11.12/24"; \
	QString sName1 = QString("test priv net1"); \
	{ \
		IPPRIV_NET_TO_XML_OBJECT; \
		priv_net.setNetAddresses(lstNetAddresses1); \
		priv_net.setName(sName1); \
		IPPRIV_NET_FROM_XML_OBJECT; \
	} \
	m_JobHandle.reset(PrlSrv_AddIPPrivateNetwork(m_ServerHandle, m_PrivNetHandle, 0)); \
	CHECK_JOB_RET_CODE(m_JobHandle);

#define TEST_CREATE_NET2 \
	testCreateIPPrivateNetwork(); \
	QStringList lstNetAddresses2 = QStringList()<<"192.168.1.1/24"<<"10.30.1.1/24"; \
	QString sName2 = QString("test priv net2"); \
	{ \
		IPPRIV_NET_TO_XML_OBJECT; \
		priv_net.setNetAddresses(lstNetAddresses2); \
		priv_net.setName(sName2); \
		IPPRIV_NET_FROM_XML_OBJECT; \
	} \
	m_JobHandle.reset(PrlSrv_AddIPPrivateNetwork(m_ServerHandle, m_PrivNetHandle, 0)); \
	CHECK_JOB_RET_CODE(m_JobHandle);

void PrlIPPrivateNetworkTest::testGetIPPrivateNetworksList()
{
	SKIP_ON_NON_PSBM

	testLoginLocal();

	m_JobHandle.reset(PrlSrv_GetIPPrivateNetworksList(m_ServerHandle, 0));
	CHECK_JOB_RET_CODE(m_JobHandle);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()))

	PRL_UINT32 nCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nCount))

	PRL_UINT32 i = 0;
	for (; i < nCount; ++i)
	{
		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, i, m_PrivNetHandle.GetHandlePtr()));
		CHECK_HANDLE_TYPE(m_PrivNetHandle, PHT_IPPRIV_NET);
	}

	// create 2 additional networks
	TEST_CREATE_NET1
	TEST_CREATE_NET2

	// check new set of networks
	m_JobHandle.reset(PrlSrv_GetIPPrivateNetworksList(m_ServerHandle, 0));
	CHECK_JOB_RET_CODE(m_JobHandle);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()))

	PRL_UINT32 nNewCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nNewCount))
	QVERIFY(nNewCount == nCount + 2);
	for (i = 0; i < nCount; ++i)
	{
		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, i, m_PrivNetHandle.GetHandlePtr()));
		CHECK_HANDLE_TYPE(m_PrivNetHandle, PHT_IPPRIV_NET);
	}

	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, i++, m_PrivNetHandle.GetHandlePtr()));
	CHECK_HANDLE_TYPE(m_PrivNetHandle, PHT_IPPRIV_NET);
	PRL_CHECK_STRINGS_LIST(m_PrivNetHandle, PrlIPPrivNet_GetNetAddresses, lstNetAddresses1)
	QString qsName = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsName, m_PrivNetHandle, PrlIPPrivNet_GetName);
	QCOMPARE(qsName, sName1);

	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, i, m_PrivNetHandle.GetHandlePtr()));
	CHECK_HANDLE_TYPE(m_PrivNetHandle, PHT_IPPRIV_NET);
	PRL_CHECK_STRINGS_LIST(m_PrivNetHandle, PrlIPPrivNet_GetNetAddresses, lstNetAddresses2)
	PRL_EXTRACT_STRING_VALUE(qsName, m_PrivNetHandle, PrlIPPrivNet_GetName);
	QCOMPARE(qsName, sName2);
}

void PrlIPPrivateNetworkTest::testGetIPPrivateNetworksListOnWrongParams()
{
	testLoginLocal();

	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetIPPrivateNetworksList(m_PrivNetHandle, 0), PRL_ERR_INVALID_ARG);
}

void PrlIPPrivateNetworkTest::testAddIPPrivateNetworkOnDuplicateName()
{
	SKIP_ON_NON_PSBM

	testLoginLocal();

	TEST_CREATE_NET1

	m_JobHandle.reset(PrlSrv_GetIPPrivateNetworksList(m_ServerHandle, 0));
	CHECK_JOB_RET_CODE(m_JobHandle);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()))

	PRL_UINT32 nCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nCount))

	if (nCount > 0)
	{
		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, m_PrivNetHandle.GetHandlePtr()));

		CHECK_ASYNC_OP_FAILED(PrlSrv_AddIPPrivateNetwork(m_ServerHandle, m_PrivNetHandle, 0),
								PRL_NET_DUPLICATE_IPPRIVATE_NETWORK_NAME);
	}
	else
	{
		QSKIP("No one private network presents on this host for this test!", SkipAll);
	}
}

void PrlIPPrivateNetworkTest::testAddIPPrivateNetworkOnWrongParams()
{
	testCreateIPPrivateNetwork();

	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlSrv_AddIPPrivateNetwork(m_PrivNetHandle, m_PrivNetHandle, 0),
						PRL_ERR_INVALID_ARG);
	CHECK_ASYNC_OP_FAILED(PrlSrv_AddIPPrivateNetwork(m_ServerHandle, m_ServerHandle, 0),
						PRL_ERR_INVALID_ARG);
}

void PrlIPPrivateNetworkTest::testUpdateIPPrivateNetwork()
{
	SKIP_ON_NON_PSBM

	testLoginLocal();

	TEST_CREATE_NET1

	// retrieve current networks
	m_JobHandle.reset(PrlSrv_GetIPPrivateNetworksList(m_ServerHandle, 0));
	CHECK_JOB_RET_CODE(m_JobHandle);
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()))
	PRL_UINT32 nCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nCount))
	QVERIFY(nCount > 0);

	// use first handle as source for update
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, m_PrivNetHandle.GetHandlePtr()));
	CHECK_HANDLE_TYPE(m_PrivNetHandle, PHT_IPPRIV_NET);
	PRL_CHECK_STRINGS_LIST(m_PrivNetHandle, PrlIPPrivNet_GetNetAddresses, lstNetAddresses1)
	QString qsName = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsName, m_PrivNetHandle, PrlIPPrivNet_GetName);
	QCOMPARE(qsName, sName1);

	// modify first network
	QString qsNewName = "new name";
	CHECK_RET_CODE_EXP(PrlIPPrivNet_SetName(m_PrivNetHandle, QSTR2UTF8(qsNewName)));
	QStringList lstNewNetAddresses = QStringList()<<"192.168.1.1/24"<<"10.30.1.1/24";
	SdkHandleWrap hNewNetAddressesList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hNewNetAddressesList.GetHandlePtr()))
	foreach(QString sNetAddress, lstNewNetAddresses)
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hNewNetAddressesList, QSTR2UTF8(sNetAddress)))
	CHECK_RET_CODE_EXP(PrlIPPrivNet_SetNetAddresses(m_PrivNetHandle, hNewNetAddressesList))

	// update it on server
	m_JobHandle.reset(PrlSrv_UpdateIPPrivateNetwork(m_ServerHandle, m_PrivNetHandle, 0));
	CHECK_JOB_RET_CODE(m_JobHandle);

	// retrieve new set and check that updated
	m_JobHandle.reset(PrlSrv_GetIPPrivateNetworksList(m_ServerHandle, 0));
	CHECK_JOB_RET_CODE(m_JobHandle);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()))
	PRL_UINT32 nNewCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nNewCount))
	QVERIFY(nCount == nNewCount);
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, m_PrivNetHandle.GetHandlePtr()));
	CHECK_HANDLE_TYPE(m_PrivNetHandle, PHT_IPPRIV_NET);
	PRL_CHECK_STRINGS_LIST(m_PrivNetHandle, PrlIPPrivNet_GetNetAddresses, lstNewNetAddresses)
	PRL_EXTRACT_STRING_VALUE(qsName, m_PrivNetHandle, PrlIPPrivNet_GetName);
	QCOMPARE(qsName, qsNewName);
}

void PrlIPPrivateNetworkTest::testUpdateIPPrivateNetworkOnDuplicateName()
{
	SKIP_ON_NON_PSBM

	testLoginLocal();

	TEST_CREATE_NET1
	TEST_CREATE_NET2

	m_JobHandle.reset(PrlSrv_GetIPPrivateNetworksList(m_ServerHandle, 0));
	CHECK_JOB_RET_CODE(m_JobHandle);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()))

	PRL_UINT32 nCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nCount))

	if (nCount > 1)
	{
		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, m_PrivNetHandle.GetHandlePtr()));

		QString qsNewName;
		PRL_EXTRACT_STRING_VALUE(qsNewName, m_PrivNetHandle, PrlIPPrivNet_GetName);

		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 1, m_PrivNetHandle.GetHandlePtr()));

		CHECK_RET_CODE_EXP(PrlIPPrivNet_SetName(m_PrivNetHandle, QSTR2UTF8(qsNewName)));

		CHECK_ASYNC_OP_FAILED(PrlSrv_UpdateIPPrivateNetwork(m_ServerHandle, m_PrivNetHandle, 0),
								PRL_NET_DUPLICATE_IPPRIVATE_NETWORK_NAME);
	}
	else
	{
		QSKIP("Two private networks are need for this test!", SkipAll);
	}
}

void PrlIPPrivateNetworkTest::testUpdateIPPrivateNetworkOnWrongParams()
{
	testCreateIPPrivateNetwork();

	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlSrv_UpdateIPPrivateNetwork(m_PrivNetHandle, m_PrivNetHandle, 0),
						PRL_ERR_INVALID_ARG);
	CHECK_ASYNC_OP_FAILED(PrlSrv_UpdateIPPrivateNetwork(m_ServerHandle, m_ServerHandle, 0),
						PRL_ERR_INVALID_ARG);
}

void PrlIPPrivateNetworkTest::testRemoveIPPrivateNetwork()
{
	SKIP_ON_NON_PSBM

	testLoginLocal();

	TEST_CREATE_NET1

	// retrieve current networks
	m_JobHandle.reset(PrlSrv_GetIPPrivateNetworksList(m_ServerHandle, 0));
	CHECK_JOB_RET_CODE(m_JobHandle);
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()))
	PRL_UINT32 nCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nCount))
	QVERIFY(nCount > 0);

	// remove from server first network
	CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, m_PrivNetHandle.GetHandlePtr()));
	CHECK_HANDLE_TYPE(m_PrivNetHandle, PHT_IPPRIV_NET);
	QString qsName;
	PRL_EXTRACT_STRING_VALUE(qsName, m_PrivNetHandle, PrlIPPrivNet_GetName);
	m_JobHandle.reset(PrlSrv_RemoveIPPrivateNetwork(m_ServerHandle, m_PrivNetHandle, 0));
	CHECK_JOB_RET_CODE(m_JobHandle);

	// retrieve new set and check that network removed
	m_JobHandle.reset(PrlSrv_GetIPPrivateNetworksList(m_ServerHandle, 0));
	CHECK_JOB_RET_CODE(m_JobHandle);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()))
	PRL_UINT32 nNewCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nNewCount))
	QVERIFY(nNewCount == nCount - 1);
	for (PRL_UINT32 i = 0; i < nNewCount; ++i)
	{
		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, i, m_PrivNetHandle.GetHandlePtr()));
		CHECK_HANDLE_TYPE(m_PrivNetHandle, PHT_IPPRIV_NET);
		QString qsTmpName;
		PRL_EXTRACT_STRING_VALUE(qsTmpName, m_PrivNetHandle, PrlIPPrivNet_GetName);
		QVERIFY(qsTmpName != qsName);
	}
}

void PrlIPPrivateNetworkTest::testRemoveIPPrivateNetworkOnNotExistingName()
{
	SKIP_ON_NON_PSBM

	testLoginLocal();

	testCreateIPPrivateNetwork();

	CHECK_RET_CODE_EXP(PrlIPPrivNet_SetName(m_PrivNetHandle, "not existing network ID"));

	CHECK_ASYNC_OP_FAILED(PrlSrv_RemoveIPPrivateNetwork(m_ServerHandle, m_PrivNetHandle, 0),
							PRL_NET_IPPRIVATE_NETWORK_DOES_NOT_EXIST);
}

void PrlIPPrivateNetworkTest::testRemoveIPPrivateNetworkOnWrongParams()
{
	testCreateIPPrivateNetwork();

	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlSrv_RemoveIPPrivateNetwork(m_PrivNetHandle, m_PrivNetHandle, 0),
						PRL_ERR_INVALID_ARG);
	CHECK_ASYNC_OP_FAILED(PrlSrv_RemoveIPPrivateNetwork(m_ServerHandle, m_ServerHandle, 0),
						PRL_ERR_INVALID_ARG);
}
