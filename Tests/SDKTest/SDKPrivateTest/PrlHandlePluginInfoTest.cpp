/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlHandlePluginInfoTest.cpp
///
///	This file is the part of parallels public SDK library private tests suite.
///	Tests fixture class for testing SDK API calls for plugin info handle.
///
///	@author myakhin
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

#include "PrlHandlePluginInfoTest.h"
#include <prlcommon/Messaging/CVmEvent.h>
#include "SDK/Handles/Disp/PrlHandlePluginInfo.h"
#include "Tests/CommonTestsUtils.h"


void PrlHandlePluginInfoTest::init()
{
	PrlHandlePluginInfoPtr pPluginInfo(new PrlHandlePluginInfo);
	m_hPluginInfo.reset(pPluginInfo->GetHandle());
}

void PrlHandlePluginInfoTest::cleanup()
{
	m_hPluginInfo.reset();
}

void PrlHandlePluginInfoTest::testPluginInfoBasicValidation()
{
	CHECK_HANDLE_TYPE(m_hPluginInfo, PHT_PLUGIN_INFO);

	CVmEvent evPluginInfo;
	evPluginInfo.addEventParameter(
		new CVmEventParameter(PVE::String, "sdgjhfgADS", "any_param_1") );
	evPluginInfo.addEventParameter(
		new CVmEventParameter(PVE::String, "435RT63TG6R", "any_param_2") );
	evPluginInfo.addEventParameter(
		new CVmEventParameter(PVE::String, ",kl][p;=", "any_param_3") );

	CHECK_RET_CODE_EXP( PrlHandle_FromString(m_hPluginInfo, QSTR2UTF8(evPluginInfo.toString())) );
	HANDLE_TO_STRING(m_hPluginInfo);
	QCOMPARE(evPluginInfo.toString(), _str_object);
}

void PrlHandlePluginInfoTest::testGetVendor()
{
	CVmEvent evPluginInfo;
	evPluginInfo.addEventParameter(
		new CVmEventParameter(
			PVE::String, Uuid::createUuid().toString(), EVT_PARAM_PLUGIN_INFO_VENDOR) );

	CHECK_RET_CODE_EXP( PrlHandle_FromString(m_hPluginInfo, QSTR2UTF8(evPluginInfo.toString())) );

	QString qsVendor;
	PRL_EXTRACT_STRING_VALUE(qsVendor, m_hPluginInfo, PrlPluginInfo_GetVendor);
	QCOMPARE(qsVendor,
		evPluginInfo.getEventParameter(EVT_PARAM_PLUGIN_INFO_VENDOR)->getParamValue());
}

void PrlHandlePluginInfoTest::testGetVendorOnWrongParams()
{
	SdkHandleWrap hSrv;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hSrv.GetHandlePtr()));

	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetVendor(hSrv, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetVendor(m_hPluginInfo, 0, 0),
									PRL_ERR_INVALID_ARG);
	// No data
	char buf[128];
	nSize = sizeof(buf);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetVendor(m_hPluginInfo, buf, &nSize),
									PRL_ERR_NO_DATA);
	// Buffer overrun
	CVmEvent evPluginInfo;
	evPluginInfo.addEventParameter(
		new CVmEventParameter(
			PVE::String, Uuid::createUuid().toString(), EVT_PARAM_PLUGIN_INFO_VENDOR) );

	CHECK_RET_CODE_EXP( PrlHandle_FromString(m_hPluginInfo, QSTR2UTF8(evPluginInfo.toString())) );

	nSize = 5;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetVendor(m_hPluginInfo, buf, &nSize),
									PRL_ERR_BUFFER_OVERRUN);
}

void PrlHandlePluginInfoTest::testGetCopyright()
{
	CVmEvent evPluginInfo;
	evPluginInfo.addEventParameter(
		new CVmEventParameter(
			PVE::String, Uuid::createUuid().toString(), EVT_PARAM_PLUGIN_INFO_COPYRIGHT) );

	CHECK_RET_CODE_EXP( PrlHandle_FromString(m_hPluginInfo, QSTR2UTF8(evPluginInfo.toString())) );

	QString qsCopyright;
	PRL_EXTRACT_STRING_VALUE(qsCopyright, m_hPluginInfo, PrlPluginInfo_GetCopyright);
	QCOMPARE(qsCopyright,
		evPluginInfo.getEventParameter(EVT_PARAM_PLUGIN_INFO_COPYRIGHT)->getParamValue());
}

void PrlHandlePluginInfoTest::testGetCopyrightOnWrongParams()
{
	SdkHandleWrap hSrv;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hSrv.GetHandlePtr()));

	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetCopyright(hSrv, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetCopyright(m_hPluginInfo, 0, 0),
									PRL_ERR_INVALID_ARG);
	// No data
	char buf[128];
	nSize = sizeof(buf);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetCopyright(m_hPluginInfo, buf, &nSize),
									PRL_ERR_NO_DATA);
	// Buffer overrun
	CVmEvent evPluginInfo;
	evPluginInfo.addEventParameter(
		new CVmEventParameter(
			PVE::String, Uuid::createUuid().toString(), EVT_PARAM_PLUGIN_INFO_COPYRIGHT) );

	CHECK_RET_CODE_EXP( PrlHandle_FromString(m_hPluginInfo, QSTR2UTF8(evPluginInfo.toString())) );

	nSize = 5;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetCopyright(m_hPluginInfo, buf, &nSize),
									PRL_ERR_BUFFER_OVERRUN);
}

void PrlHandlePluginInfoTest::testGetShortDescription()
{
	CVmEvent evPluginInfo;
	evPluginInfo.addEventParameter(
		new CVmEventParameter(
			PVE::String, Uuid::createUuid().toString(), EVT_PARAM_PLUGIN_INFO_DESC_SHORT) );

	CHECK_RET_CODE_EXP( PrlHandle_FromString(m_hPluginInfo, QSTR2UTF8(evPluginInfo.toString())) );

	QString qsShortDescription;
	PRL_EXTRACT_STRING_VALUE(qsShortDescription, m_hPluginInfo, PrlPluginInfo_GetShortDescription);
	QCOMPARE(qsShortDescription,
		evPluginInfo.getEventParameter(EVT_PARAM_PLUGIN_INFO_DESC_SHORT)->getParamValue());
}

void PrlHandlePluginInfoTest::testGetShortDescriptionOnWrongParams()
{
	SdkHandleWrap hSrv;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hSrv.GetHandlePtr()));

	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetShortDescription(hSrv, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetShortDescription(m_hPluginInfo, 0, 0),
									PRL_ERR_INVALID_ARG);
	// No data
	char buf[128];
	nSize = sizeof(buf);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetShortDescription(m_hPluginInfo, buf, &nSize),
									PRL_ERR_NO_DATA);
	// Buffer overrun
	CVmEvent evPluginInfo;
	evPluginInfo.addEventParameter(
		new CVmEventParameter(
			PVE::String, Uuid::createUuid().toString(), EVT_PARAM_PLUGIN_INFO_DESC_SHORT) );

	CHECK_RET_CODE_EXP( PrlHandle_FromString(m_hPluginInfo, QSTR2UTF8(evPluginInfo.toString())) );

	nSize = 5;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetShortDescription(m_hPluginInfo, buf, &nSize),
									PRL_ERR_BUFFER_OVERRUN);
}

void PrlHandlePluginInfoTest::testGetLongDescription()
{
	CVmEvent evPluginInfo;
	evPluginInfo.addEventParameter(
		new CVmEventParameter(
			PVE::String, Uuid::createUuid().toString(), EVT_PARAM_PLUGIN_INFO_DESC_LONG) );

	CHECK_RET_CODE_EXP( PrlHandle_FromString(m_hPluginInfo, QSTR2UTF8(evPluginInfo.toString())) );

	QString qsLongDescription;
	PRL_EXTRACT_STRING_VALUE(qsLongDescription, m_hPluginInfo, PrlPluginInfo_GetLongDescription);
	QCOMPARE(qsLongDescription,
		evPluginInfo.getEventParameter(EVT_PARAM_PLUGIN_INFO_DESC_LONG)->getParamValue());
}

void PrlHandlePluginInfoTest::testGetLongDescriptionOnWrongParams()
{
	SdkHandleWrap hSrv;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hSrv.GetHandlePtr()));

	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetLongDescription(hSrv, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetLongDescription(m_hPluginInfo, 0, 0),
									PRL_ERR_INVALID_ARG);
	// No data
	char buf[128];
	nSize = sizeof(buf);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetLongDescription(m_hPluginInfo, buf, &nSize),
									PRL_ERR_NO_DATA);
	// Buffer overrun
	CVmEvent evPluginInfo;
	evPluginInfo.addEventParameter(
		new CVmEventParameter(
			PVE::String, Uuid::createUuid().toString(), EVT_PARAM_PLUGIN_INFO_DESC_LONG) );

	CHECK_RET_CODE_EXP( PrlHandle_FromString(m_hPluginInfo, QSTR2UTF8(evPluginInfo.toString())) );

	nSize = 5;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetLongDescription(m_hPluginInfo, buf, &nSize),
									PRL_ERR_BUFFER_OVERRUN);
}

void PrlHandlePluginInfoTest::testGetVersion()
{
	CVmEvent evPluginInfo;
	evPluginInfo.addEventParameter(
		new CVmEventParameter(
			PVE::String, Uuid::createUuid().toString(), EVT_PARAM_PLUGIN_INFO_VERSION) );

	CHECK_RET_CODE_EXP( PrlHandle_FromString(m_hPluginInfo, QSTR2UTF8(evPluginInfo.toString())) );

	QString qsVersion;
	PRL_EXTRACT_STRING_VALUE(qsVersion, m_hPluginInfo, PrlPluginInfo_GetVersion);
	QCOMPARE(qsVersion,
		evPluginInfo.getEventParameter(EVT_PARAM_PLUGIN_INFO_VERSION)->getParamValue());
}

void PrlHandlePluginInfoTest::testGetVersionOnWrongParams()
{
	SdkHandleWrap hSrv;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hSrv.GetHandlePtr()));

	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetVersion(hSrv, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetVersion(m_hPluginInfo, 0, 0),
									PRL_ERR_INVALID_ARG);
	// No data
	char buf[128];
	nSize = sizeof(buf);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetVersion(m_hPluginInfo, buf, &nSize),
									PRL_ERR_NO_DATA);
	// Buffer overrun
	CVmEvent evPluginInfo;
	evPluginInfo.addEventParameter(
		new CVmEventParameter(
			PVE::String, Uuid::createUuid().toString(), EVT_PARAM_PLUGIN_INFO_VERSION) );

	CHECK_RET_CODE_EXP( PrlHandle_FromString(m_hPluginInfo, QSTR2UTF8(evPluginInfo.toString())) );

	nSize = 5;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetVersion(m_hPluginInfo, buf, &nSize),
									PRL_ERR_BUFFER_OVERRUN);
}

void PrlHandlePluginInfoTest::testGetId()
{
	CVmEvent evPluginInfo;
	evPluginInfo.addEventParameter(
		new CVmEventParameter(
			PVE::String, Uuid::createUuid().toString(), EVT_PARAM_PLUGIN_INFO_ID) );

	CHECK_RET_CODE_EXP( PrlHandle_FromString(m_hPluginInfo, QSTR2UTF8(evPluginInfo.toString())) );

	QString qsId;
	PRL_EXTRACT_STRING_VALUE(qsId, m_hPluginInfo, PrlPluginInfo_GetId);
	QCOMPARE(qsId,
		evPluginInfo.getEventParameter(EVT_PARAM_PLUGIN_INFO_ID)->getParamValue());
}

void PrlHandlePluginInfoTest::testGetIdOnWrongParams()
{
	SdkHandleWrap hSrv;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hSrv.GetHandlePtr()));

	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetId(hSrv, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetId(m_hPluginInfo, 0, 0),
									PRL_ERR_INVALID_ARG);
	// No data
	char buf[128];
	nSize = sizeof(buf);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetId(m_hPluginInfo, buf, &nSize),
									PRL_ERR_NO_DATA);
	// Buffer overrun
	CVmEvent evPluginInfo;
	evPluginInfo.addEventParameter(
		new CVmEventParameter(
			PVE::String, Uuid::createUuid().toString(), EVT_PARAM_PLUGIN_INFO_ID) );

	CHECK_RET_CODE_EXP( PrlHandle_FromString(m_hPluginInfo, QSTR2UTF8(evPluginInfo.toString())) );

	nSize = 5;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPluginInfo_GetId(m_hPluginInfo, buf, &nSize),
									PRL_ERR_BUFFER_OVERRUN);
}
