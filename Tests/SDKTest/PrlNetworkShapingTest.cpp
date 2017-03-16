/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlNetworkShapingTest.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing virtual network manipulating SDK API.
///
///	@author igor
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/////////////////////////////////////////////////////////////////////////////
#include "PrlNetworkShapingTest.h"
#include "Tests/CommonTestsUtils.h"
#include <prlcommon/Interfaces/ParallelsQt.h>
#include <prlxmlmodel/NetworkConfig/CNetworkClass.h>
#include <prlxmlmodel/NetworkConfig/CNetworkShaping.h>
#include "Tests/CommonTestsUtils.h"
#include "Libraries/PrlNetworking/PrlNetLibrary.h"


void PrlNetworkShapingTest::init()
{
	m_hServer.reset();
	CHECK_RET_CODE_EXP(PrlSrv_Create(m_hServer.GetHandlePtr()));

	m_hJob.reset();
	m_hVm.reset();
	m_hClassesListOrig.reset();

	testLoginLocal();
}

void PrlNetworkShapingTest::cleanup()
{

	DestroyVm();
	RestoreNetworkClassesConfig();

	m_hJob.reset(PrlSrv_Logoff(m_hServer));
	PrlJob_Wait(m_hJob, PRL_JOB_WAIT_TIMEOUT);
}

void PrlNetworkShapingTest::testLoginLocal()
{
	m_hJob.reset(PrlSrv_LoginLocal(m_hServer, "", 0, PSL_HIGH_SECURITY));
	QVERIFY(m_hJob != PRL_INVALID_HANDLE);
	CHECK_JOB_RET_CODE(m_hJob);
}

#define NETWORK_CLASS_TO_XML_OBJECT \
	PRL_VOID_PTR pXml = 0; \
	CHECK_RET_CODE_EXP(PrlHandle_ToString(hClass, &pXml)); \
	CNetworkClass entry; \
	entry.fromString( UTF8_2QSTR((PRL_CONST_STR )pXml) ); \
	PrlBuffer_Free(pXml);

#define NETWORK_CLASS_FROM_XML_OBJECT \
	CHECK_RET_CODE_EXP(PrlHandle_FromString( hClass, QSTR2UTF8(entry.toString()) ));


void PrlNetworkShapingTest::CreateNetworkClass(SdkHandleWrap &hClass, PRL_UINT32 nClassId)
{
	CHECK_RET_CODE_EXP(PrlNetworkClass_Create(nClassId, hClass.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hClass, PHT_NETWORK_CLASS);

	CHECK_RET_CODE_EXP(PrlNetworkClass_SetClassId(hClass, nClassId));

	QString sNetOrig = "192.164.0.0/24";
	SdkHandleWrap hList;
	CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hList.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlStrList_AddItem(hList, QSTR2UTF8(sNetOrig)));
	CHECK_RET_CODE_EXP(PrlNetworkClass_SetNetworkList(hClass, hList));

	NETWORK_CLASS_TO_XML_OBJECT
	NETWORK_CLASS_FROM_XML_OBJECT

	PRL_UINT32 _nClassId = 0;
	CHECK_RET_CODE_EXP(PrlNetworkClass_GetClassId(hClass, &_nClassId));
	QVERIFY(_nClassId == nClassId);
}

void PrlNetworkShapingTest::testCreateNetworkClass()
{
	SdkHandleWrap hClass0;

	CreateNetworkClass(hClass0, 0);
}

void PrlNetworkShapingTest::testCreateNetworkClassOnWrongParams()
{
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkClass_Create(1, NULL), PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkClass_SetNetworkList(NULL, NULL), PRL_ERR_INVALID_ARG);

	SdkHandleWrap hClass;
	CreateNetworkClass(hClass, 0);

	// network/mask validation
	{
		SdkHandleWrap hList;
		QString sNet = "192.164.0.0";
		CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hList.GetHandlePtr()));
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hList, QSTR2UTF8(sNet)));
		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkClass_SetNetworkList(hClass, hList), PRL_ERR_INVALID_ARG);
	}
	{
		SdkHandleWrap hList;
		QString sNet = "x/24";
		CHECK_RET_CODE_EXP(PrlApi_CreateStringsList(hList.GetHandlePtr()));
		CHECK_RET_CODE_EXP(PrlStrList_AddItem(hList, QSTR2UTF8(sNet)));
		CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkClass_SetNetworkList(hClass, hList), PRL_ERR_INVALID_ARG);
	}

	SdkHandleWrap hClass0;
	CreateNetworkClass(hClass0, 0);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkClass_SetNetworkList(hClass0, NULL), PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkClass_SetNetworkList(hClass0, m_hServer), PRL_ERR_INVALID_ARG);
}

#define NETWORK_SHAPING_ENTRY_TO_XML_OBJECT \
	PRL_VOID_PTR pXml = 0; \
	CHECK_RET_CODE_EXP(PrlHandle_ToString(hEntry, &pXml)); \
	CNetworkShaping entry; \
	entry.fromString( UTF8_2QSTR((PRL_CONST_STR )pXml) ); \
	PrlBuffer_Free(pXml);

#define NETWORK_SHAPING_ENTRY_FROM_XML_OBJECT \
	CHECK_RET_CODE_EXP(PrlHandle_FromString(hEntry, QSTR2UTF8(entry.toString()) ));


void PrlNetworkShapingTest::CreateNetworkShapingEntry(SdkHandleWrap &hEntry, PRL_UINT32 nClassId, const QString &sDevOrig)
{
	PRL_UINT32 nTotalRate = 4;
	PRL_UINT32 nRate = 4;
	PRL_INT32 nRateMPU;

	CHECK_RET_CODE_EXP(PrlNetworkShapingEntry_Create(nClassId, nTotalRate, hEntry.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hEntry, PHT_NETWORK_SHAPING);
	// If not defined, this should be turned on with default mpu.
	CHECK_RET_CODE_EXP(PrlNetworkShapingEntry_getRateMPU(hEntry, &nRateMPU));
	QVERIFY(nRateMPU == NRM_ENABLED);
	nRateMPU = NRM_DISABLED;

	CHECK_RET_CODE_EXP(PrlNetworkShapingEntry_SetDevice(hEntry, QSTR2UTF8(sDevOrig)));
	CHECK_RET_CODE_EXP(PrlNetworkShapingEntry_SetRate(hEntry, nRate));
	CHECK_RET_CODE_EXP(PrlNetworkShapingEntry_SetPacketLimited(hEntry, nRateMPU));

	NETWORK_SHAPING_ENTRY_TO_XML_OBJECT
	NETWORK_SHAPING_ENTRY_FROM_XML_OBJECT

	PRL_UINT32 _nClassId = 0;
	PRL_UINT32 _nTotalRate = 0;
	PRL_UINT32 _nRate = 0;
	QString sDev = "xxx";
	PRL_INT32 _nRateMPU = NRM_ENABLED;

	CHECK_RET_CODE_EXP(PrlNetworkShapingEntry_GetClassId(hEntry, &_nClassId));
	QVERIFY(_nClassId == nClassId);
	CHECK_RET_CODE_EXP(PrlNetworkShapingEntry_GetTotalRate(hEntry, &_nTotalRate));
	QVERIFY(_nTotalRate == nTotalRate);
	CHECK_RET_CODE_EXP(PrlNetworkShapingEntry_GetRate(hEntry, &_nRate));
	QVERIFY(_nRate == nRate);

	PRL_EXTRACT_STRING_VALUE(sDev, hEntry, PrlNetworkShapingEntry_GetDevice);
	QCOMPARE(sDev, sDevOrig);
	CHECK_RET_CODE_EXP(PrlNetworkShapingEntry_GetRateMPU(hEntry, &_nRateMPU));
	QVERIFY(_nRateMPU == nRateMPU);
}

void PrlNetworkShapingTest::testCreateNetworkShapingEntry()
{
	SdkHandleWrap hEntry;

	CreateNetworkShapingEntry(hEntry, 1, "eth0");
}

void PrlNetworkShapingTest::testCreateNetworkShapingEntryWrongParams()
{
	SdkHandleWrap hEntry;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingEntry_Create(0, 0, NULL), PRL_ERR_INVALID_ARG);

	CHECK_RET_CODE_EXP(PrlNetworkShapingEntry_Create(0, 0, hEntry.GetHandlePtr()));
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingEntry_SetDevice(m_hServer, ""), PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingEntry_SetDevice(hEntry, NULL), PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingEntry_SetDevice(NULL, ""), PRL_ERR_INVALID_ARG);

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingEntry_GetDevice(NULL, NULL, NULL), PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingEntry_GetDevice(m_hServer, NULL, NULL), PRL_ERR_INVALID_ARG);

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingEntry_GetTotalRate(NULL, NULL), PRL_ERR_INVALID_ARG);

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingEntry_SetRate(NULL, 0), PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingEntry_SetRate(m_hServer, 0), PRL_ERR_INVALID_ARG);

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingEntry_GetRate(NULL, NULL), PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingEntry_GetRate(m_hServer, NULL), PRL_ERR_INVALID_ARG);

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingEntry_SetRateMPU(NULL, NRM_DISABLED), PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingEntry_SetRateMPU(m_hServer, NRM_DISABLED), PRL_ERR_INVALID_ARG);

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingEntry_GetRateMPU(NULL, NULL), PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingEntry_GetRateMPU(m_hServer, NULL), PRL_ERR_INVALID_ARG);
}

void PrlNetworkShapingTest::testCreateNetworkBandwidthEntryWrongParams()
{
	SdkHandleWrap hEntry;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingBandwidthEntry_Create(0, 0, NULL), PRL_ERR_INVALID_ARG);

	CHECK_RET_CODE_EXP(PrlNetworkShapingBandwidthEntry_Create(0, 0, hEntry.GetHandlePtr()));
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingBandwidthEntry_SetDevice(m_hServer, ""), PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingBandwidthEntry_SetDevice(hEntry, NULL), PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingBandwidthEntry_SetDevice(NULL, ""), PRL_ERR_INVALID_ARG);

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingBandwidthEntry_GetDevice(NULL, NULL, NULL), PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingBandwidthEntry_GetDevice(m_hServer, NULL, NULL), PRL_ERR_INVALID_ARG);

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingBandwidthEntry_SetBandwidth(NULL, 0), PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingBandwidthEntry_SetBandwidth(m_hServer, 0), PRL_ERR_INVALID_ARG);

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingBandwidthEntry_GetBandwidth(NULL, NULL), PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkShapingBandwidthEntry_GetBandwidth(m_hServer, NULL), PRL_ERR_INVALID_ARG);
}

void PrlNetworkShapingTest::testCreateNetworkBandwidthEntry()
{
	SdkHandleWrap hEntry;
	QString sDev = "eth0";
	PRL_UINT32 nBandwidth = 10000;

	CHECK_RET_CODE_EXP(PrlNetworkShapingBandwidthEntry_Create(QSTR2UTF8(sDev), nBandwidth, hEntry.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hEntry, PHT_NETWORK_SHAPING_BANDWIDTH);

	CHECK_RET_CODE_EXP(PrlNetworkShapingBandwidthEntry_SetDevice(hEntry, QSTR2UTF8(sDev)));
	CHECK_RET_CODE_EXP(PrlNetworkShapingBandwidthEntry_SetBandwidth(hEntry, nBandwidth));

	QString _sDev;
	PRL_UINT32 _nBandwidth = 0;

	PRL_EXTRACT_STRING_VALUE(_sDev, hEntry, PrlNetworkShapingBandwidthEntry_GetDevice);
	QCOMPARE(_sDev, sDev);

	CHECK_RET_CODE_EXP(PrlNetworkShapingBandwidthEntry_GetBandwidth(hEntry, &_nBandwidth));
	QVERIFY(_nBandwidth == nBandwidth);
}

#define COMPARE_NETWORK_CLASSES(hClass, hClassOrig) \
{\
	CNetworkClass net_class, net_classOrig; \
\
	PRL_VOID_PTR pXml = 0; \
	CHECK_RET_CODE_EXP(PrlHandle_ToString(hClass, &pXml)); \
	net_class.fromString( UTF8_2QSTR((PRL_CONST_STR )pXml) ); \
	PrlBuffer_Free(pXml); \
\
	CHECK_RET_CODE_EXP(PrlHandle_ToString(hClassOrig, &pXml)); \
	net_classOrig.fromString( UTF8_2QSTR((PRL_CONST_STR )pXml) ); \
	PrlBuffer_Free(pXml); \
\
	QVERIFY(net_class.getClassId() == net_classOrig.getClassId()); \
\
	QList<QString> net_list = net_class.getNetworkList(); \
	QList<QString> net_listOrig = net_classOrig.getNetworkList(); \
	QCOMPARE(net_list.size(), net_listOrig.size()); \
	for (PRL_INT32 i = 0; i < net_list.size(); ++i) \
		QCOMPARE(net_list.at(i), net_listOrig.at(i)); \
}

void PrlNetworkShapingTest::AddNetworkClassesConfig()
{
	SdkHandleWrap hClassesList;
	SdkHandleWrap hJob, hResult;
	SdkHandleWrap hClass0;
	SdkHandleWrap hClass1;
	SdkHandleWrap hClass2;

	// preserve original list of classes
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetNetworkClassesList(m_hServer, 0), 0);
	hJob.reset(PrlSrv_GetNetworkClassesList(m_hServer, 0));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, m_hClassesListOrig.GetHandlePtr()));

	CreateNetworkClass(hClass0, 0);
	CreateNetworkClass(hClass1, 1);
	CreateNetworkClass(hClass2, 2);

	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hClassesList.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hClassesList, hClass0));
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hClassesList, hClass1));
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hClassesList, hClass2));

	CHECK_ASYNC_OP_FAILED(PrlSrv_UpdateNetworkClassesList(m_hServer, hClassesList, 0), 0);

	// check validity of result
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetNetworkClassesList(m_hServer, 0), 0);
	hJob.reset(PrlSrv_GetNetworkClassesList(m_hServer, 0));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hClassesList.GetHandlePtr()));

	PRL_UINT32 nItemsCount = 0;
	CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hClassesList, &nItemsCount))
	QVERIFY(nItemsCount == 3);

	SdkHandleWrap hClass;
	CHECK_RET_CODE_EXP(PrlHndlList_GetItem(hClassesList, 0, hClass.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hClass, PHT_NETWORK_CLASS);
	COMPARE_NETWORK_CLASSES(hClass, hClass0);

	CHECK_RET_CODE_EXP(PrlHndlList_GetItem(hClassesList, 1, hClass.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hClass, PHT_NETWORK_CLASS);
	COMPARE_NETWORK_CLASSES(hClass, hClass1);

	CHECK_RET_CODE_EXP(PrlHndlList_GetItem(hClassesList, 2, hClass.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hClass, PHT_NETWORK_CLASS);
	COMPARE_NETWORK_CLASSES(hClass, hClass2);
}

void PrlNetworkShapingTest::RestoreNetworkClassesConfig()
{
	if (m_hClassesListOrig == PRL_INVALID_HANDLE)
		return;

	CHECK_ASYNC_OP_FAILED(PrlSrv_UpdateNetworkClassesList(m_hServer, m_hClassesListOrig, 0), 0);
	m_hClassesListOrig.reset();
}

void PrlNetworkShapingTest::testUpdateNetworkClassesConfig()
{
	AddNetworkClassesConfig();
}

#define COMPARE_SHAPING_ENTRIES(hEntry, hEntryOrig) \
{\
	CNetworkShaping entry, entryOrig; \
\
	PRL_VOID_PTR pXml = 0; \
	CHECK_RET_CODE_EXP(PrlHandle_ToString(hEntry, &pXml)); \
	entry.fromString( UTF8_2QSTR((PRL_CONST_STR )pXml) ); \
	PrlBuffer_Free(pXml); \
\
	CHECK_RET_CODE_EXP(PrlHandle_ToString(hEntryOrig, &pXml)); \
	entryOrig.fromString( UTF8_2QSTR((PRL_CONST_STR )pXml) ); \
	PrlBuffer_Free(pXml); \
\
	QCOMPARE(entry.getDevice(), entryOrig.getDevice()); \
	QVERIFY(entry.getClassId() == entryOrig.getClassId()); \
	QVERIFY(entry.getTotalRate() == entryOrig.getTotalRate()); \
	QVERIFY(entry.getRate() == entryOrig.getRate()); \
	QVERIFY(entry.getRateMPU() == entryOrig.getRateMPU()); \
}

void PrlNetworkShapingTest::testUpdateNetworkShapingConfig()
{
	SdkHandleWrap hJob, hResult, hShaping, hShapingOrig ;
	SdkHandleWrap hShapingList, hShapingListOrig, hBandList;
	SdkHandleWrap hEntry0;
	SdkHandleWrap hEntry1;
	QString sDev = "eth0";

	testLoginLocal();

	AddNetworkClassesConfig();

	hJob.reset(PrlSrv_GetNetworkShapingConfig(m_hServer, 0));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hShaping.GetHandlePtr()));

	hJob.reset(PrlSrv_GetNetworkShapingConfig(m_hServer, 0));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hShapingOrig.GetHandlePtr()));

	PRL_BOOL bEnabled = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlNetworkShapingConfig_IsEnabled(hShaping, &bEnabled));

	// BANDWIDTH

	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hBandList.GetHandlePtr()));
	PrlNet::EthAdaptersList ethList;
	PrlNet::makeBindableAdapterList( ethList, true );

	for (PrlNet::EthAdaptersList::Iterator it = ethList.begin();
			it != ethList.end();
			++it )
	{
		SdkHandleWrap hEntry;
		if (!it->_systemName.startsWith("eth"))
			continue;

		sDev = it->_systemName;
		CHECK_RET_CODE_EXP(PrlNetworkShapingBandwidthEntry_Create(
				QSTR2UTF8(it->_systemName), 100000, hEntry.GetHandlePtr()));
		CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hBandList, hEntry));
	}
	CHECK_RET_CODE_EXP(PrlNetworkShapingConfig_SetNetworkDeviceBandwidthList(hShaping, hBandList))


	// TOTALRATE
	CreateNetworkShapingEntry(hEntry0, 1, sDev);
	CreateNetworkShapingEntry(hEntry1, 2, sDev);

	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hShapingList.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hShapingList, hEntry0));
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hShapingList, hEntry1));

	// check setter
	CHECK_RET_CODE_EXP(PrlNetworkShapingConfig_SetNetworkShapingList(hShaping, hShapingList));
	bEnabled = !bEnabled;
	CHECK_RET_CODE_EXP(PrlNetworkShapingConfig_SetEnabled(hShaping, bEnabled));
	CHECK_ASYNC_OP_FAILED(PrlSrv_UpdateNetworkShapingConfig(m_hServer, hShaping, 0), 0);

	// verify result with getter
	hJob.reset(PrlSrv_GetNetworkShapingConfig(m_hServer, 0));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hShaping.GetHandlePtr()));

	PRL_BOOL bNewEnabled = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlNetworkShapingConfig_IsEnabled(hShaping, &bNewEnabled));
	QVERIFY(bNewEnabled == bEnabled);

	CHECK_RET_CODE_EXP(PrlNetworkShapingConfig_GetNetworkShapingList(hShaping,
				hShapingList.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hShapingList, PHT_HANDLES_LIST);

	PRL_UINT32 nItemsCount = 0;
	CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hShapingList, &nItemsCount))
	QVERIFY(nItemsCount == 2);

	SdkHandleWrap hEntry;
	CHECK_RET_CODE_EXP(PrlHndlList_GetItem(hShapingList, 0, hEntry.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hEntry, PHT_NETWORK_SHAPING);
	COMPARE_SHAPING_ENTRIES(hEntry, hEntry0);

	CHECK_RET_CODE_EXP(PrlHndlList_GetItem(hShapingList, 1, hEntry.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hEntry, PHT_NETWORK_SHAPING);
	COMPARE_SHAPING_ENTRIES(hEntry, hEntry1);

	// restore original values
	CHECK_ASYNC_OP_FAILED(PrlSrv_UpdateNetworkShapingConfig(m_hServer, hShapingOrig, 0), 0);
}

void PrlNetworkShapingTest::CreateVm()
{
        CHECK_RET_CODE_EXP(PrlSrv_CreateVm(m_hServer, m_hVm.GetHandlePtr()))\
        CHECK_RET_CODE_EXP(PrlVmCfg_SetName(m_hVm, QTest::currentTestFunction()))

	SdkHandleWrap hJob(PrlVm_Reg(m_hVm, "", PRL_TRUE));
	CHECK_JOB_RET_CODE(hJob)
}

void PrlNetworkShapingTest::DestroyVm()
{
	SdkHandleWrap hJob(PrlVm_Delete(m_hVm, PRL_INVALID_HANDLE));
	PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT);
}

void PrlNetworkShapingTest::CreateNetworkRate(SdkHandleWrap &hEntry, PRL_UINT32 nClassId)
{
	PRL_UINT32 nRate = 4;
	PRL_UINT32 _nRate, _nClassId;
	CHECK_RET_CODE_EXP(PrlNetworkRate_Create(nClassId, nRate, hEntry.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlNetworkRate_GetClassId(hEntry, &_nClassId));
	QVERIFY(nClassId == _nClassId);
	CHECK_RET_CODE_EXP(PrlNetworkRate_GetRate(hEntry, &_nRate));
	QVERIFY(nRate == _nRate);
}

void PrlNetworkShapingTest::testCreateNetworkRateOnWrongParams()
{
	SdkHandleWrap hEntry;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkRate_Create(1, 1, NULL), PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlNetworkRate_Create(0, 1, hEntry.GetHandlePtr()), PRL_ERR_INVALID_ARG);
}

void PrlNetworkShapingTest::testSetVmRate()
{
	SdkHandleWrap hEntry1, hEntry2;
	SdkHandleWrap hRateList;

	CreateNetworkRate(hEntry1, 1);
	CreateNetworkRate(hEntry2, 2);

	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hRateList.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hRateList, hEntry1));
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hRateList, hEntry2));

	CreateVm();

	SdkHandleWrap hJob(PrlVm_BeginEdit(m_hVm));
	CHECK_JOB_RET_CODE(hJob)

	CHECK_RET_CODE_EXP(PrlVmCfg_SetNetworkRateList(m_hVm, hRateList));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetRateBound(m_hVm, PRL_TRUE));
	PRL_BOOL bEnabled;
	CHECK_RET_CODE_EXP(PrlVmCfg_IsRateBound(m_hVm, &bEnabled));
	QVERIFY(bEnabled == PRL_TRUE);

	hJob.reset(PrlVm_Commit(m_hVm));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_RefreshConfig(m_hVm));
	CHECK_JOB_RET_CODE(hJob);

	CHECK_RET_CODE_EXP(PrlVmCfg_IsRateBound(m_hVm, &bEnabled));
	QVERIFY(bEnabled == PRL_TRUE);
}


