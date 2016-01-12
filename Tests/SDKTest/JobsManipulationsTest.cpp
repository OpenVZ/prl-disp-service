/////////////////////////////////////////////////////////////////////////////
///
///	@file JobsManipulationsTest.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing jobs manipulations SDK API.
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

#include "JobsManipulationsTest.h"
#include "Tests/CommonTestsUtils.h"
#include <prlcommon/Interfaces/ParallelsDomModel.h>
#include <prlcommon/Interfaces/ParallelsTypes.h>
#include <prlxmlmodel/Messaging/CVmEvent.h>
#include <prlxmlmodel/Messaging/CVmEventParameter.h>
#include <prlxmlmodel/DispConfig/CDispCommonPreferences.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlcommon/Logging/Logging.h>

#define MAGIC_NUM 0x15a70bcd

JobsManipulationsTest::JobsManipulationsTest()
{}

void JobsManipulationsTest::cleanup()
{
	m_JobHandle.reset();
}

void JobsManipulationsTest::testJobWaitOnInvalidJobHandle()
{
	QVERIFY(PRL_FAILED(PrlJob_Wait(PRL_INVALID_HANDLE, PRL_JOB_WAIT_TIMEOUT)));
}

void JobsManipulationsTest::testJobWaitOnWrongJobHandle()
{
	PRL_HANDLE _wrong_handle = (PRL_HANDLE)MAGIC_NUM;
	QVERIFY(PRL_FAILED(PrlJob_Wait(_wrong_handle, PRL_JOB_WAIT_TIMEOUT)));
}

void JobsManipulationsTest::testJobWaitOnNonJobHandle()
{
	CHECK_RET_CODE_EXP(PrlSrv_Create(m_JobHandle.GetHandlePtr()))
	QVERIFY(PRL_FAILED(PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT)));
}

void JobsManipulationsTest::testJobGetRetCodeOnInvalidJobHandle()
{
	PRL_RESULT _job_rc;
	QVERIFY(PRL_FAILED(PrlJob_GetRetCode(PRL_INVALID_HANDLE, &_job_rc)));
}

void JobsManipulationsTest::testJobGetRetCodeOnWrongJobHandle()
{
	PRL_RESULT _job_rc;
	PRL_HANDLE _wrong_handle = (PRL_HANDLE)MAGIC_NUM;
	QVERIFY(PRL_FAILED(PrlJob_GetRetCode(_wrong_handle, &_job_rc)));
}

void JobsManipulationsTest::testJobGetRetCodeOnNonJobHandle()
{
	PRL_RESULT _job_rc;
	CHECK_RET_CODE_EXP(PrlSrv_Create(m_JobHandle.GetHandlePtr()))
	QVERIFY(PRL_FAILED(PrlJob_GetRetCode(m_JobHandle, &_job_rc)));
}

namespace {
	struct EventParameter
	{
		const char *m_sParamName;
		PVE::ParamFieldDataType m_nParamType;
		PRL_PARAM_FIELD_DATA_TYPE m_nParamSdkType;
	};

	QString GetInitParamValue(PVE::ParamFieldDataType nType)
	{
		QString sValue;
		switch (nType)
		{
			case PVE::Text:
			case PVE::VmLicense:
			case PVE::Uuid:
			case PVE::CData:
			case PVE::String:
				sValue = "some string value";
			break;

			case PVE::Boolean:
			case PVE::UserResponseOnQuestion:
				sValue = QString::number(quint32(true));
			break;

			case PVE::VmRuntimeState:
				sValue = QString::number(quint32(VMS_RESTORING));
			break;

			case PVE::UnsignedInt:
			case PVE::Integer:
			case PVE::MessageShowNextTime:
			case PVE::VmConfiguration:
			case PVE::VmReturnCode:
			case PVE::VmSmartKernelState:
			case PVE::VmGuestToolActiveState:
			case PVE::VmDeviceStateData:
				sValue = QString::number(quint32(498126));
			break;
			case PVE::Int64:
			case PVE::UInt64:
				sValue = QString::number(Q_UINT64_C(932838457459459));
			case PVE::Date:
			case PVE::Time:
			case PVE::DateTime:
				sValue = QString();

		}

		return (sValue);
	}
}

namespace {
UNUSED bool CheckWhetherWinPlatform()
{
#ifdef _WIN_
	return (true);
#endif
	return (false);
}

}

#define GET_EVENT_OBJECT\
	SdkHandleWrap hServer;\
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))\
	SdkHandleWrap hVm;\
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(hServer, hVm.GetHandlePtr()))\
	SdkHandleWrap hEvent;\
	CHECK_RET_CODE_EXP(PrlVm_CreateEvent(hVm, hEvent.GetHandlePtr()))

void JobsManipulationsTest::testEnumerateEventParams()
{
	static EventParameter event_params[] = {
		{EVT_PARAM_OP_CODE, PVE::UnsignedInt, PFD_UINT32},
		{EVT_PARAM_VM_UUID, PVE::String, PFD_STRING},
		{EVT_PARAM_VM_CONFIG, PVE::VmConfiguration, PFD_ENTITY},
		{EVT_PARAM_OP_RC, PVE::UnsignedInt, PFD_UINT32},
		{EVT_PARAM_IOFLOW_KEY, PVE::UnsignedInt, PFD_UINT32},
		{EVT_PARAM_RETURN_PARAM_TOKEN, PVE::String, PFD_STRING},
		{EVT_PARAM_VM_MESSAGE_TYPE, PVE::UnsignedInt, PFD_UINT32},
		{EVT_PARAM_VM_LICENSE, PVE::String, PFD_STRING},
		{EVT_PARAM_VM_RUNTIME_STATUS, PVE::VmRuntimeState, PFD_ENUMERATION},
		{EVT_PARAM_VM_GUEST_TOOL_STATUS, PVE::VmGuestToolActiveState, PFD_ENUMERATION},
		{EVT_PARAM_VM_KERNEL_STATUS, PVE::VmSmartKernelState, PFD_ENUMERATION},
		{EVT_PARAM_VM_CONFIG_ACTION, PVE::UnsignedInt, PFD_UINT32},
		{EVT_PARAM_VM_CONFIG_DEV_STATE, PVE::String, PFD_STRING},
		{EVT_PARAM_VM_CONFIG_DEV_IMAGE, PVE::String, PFD_STRING},
		{EVT_PARAM_VM_CONFIG_DEV_CONNECT_STATE, PVE::Boolean, PFD_BOOLEAN},
		{EVT_PARAM_VM_ANSWER_CODE, PVE::UnsignedInt, PFD_UINT32},
		{EVT_PARAM_VM_PROBLEM_REPORT, PVE::String, PFD_STRING},
		{EVT_PARAM_DISP_COMMON_PREFERENSES, PVE::String, PFD_STRING},
		{EVT_PARAM_MESSAGE, PVE::UnsignedInt, PFD_UINT32},
		{EVT_PARAM_MESSAGE_PARAM_0, PVE::UnsignedInt, PFD_UINT32},
		{EVT_PARAM_MESSAGE_PARAM_1, PVE::UnsignedInt, PFD_UINT32},
		{EVT_PARAM_MESSAGE_PARAM_2, PVE::UnsignedInt, PFD_UINT32},
		{EVT_PARAM_MESSAGE_CHOICE_0, PVE::UnsignedInt, PFD_UINT32},
		{EVT_PARAM_MESSAGE_CHOICE_1, PVE::UnsignedInt, PFD_UINT32},
		{EVT_PARAM_MESSAGE_CHOICE_2, PVE::UnsignedInt, PFD_UINT32}
	};

	GET_EVENT_OBJECT
	CVmEvent _event_etalon;
	for (size_t i = 0; i < sizeof(event_params)/sizeof(EventParameter); ++i)
	{
		EventParameter &_event_param_data = event_params[i];
		_event_etalon.addEventParameter(new CVmEventParameter(_event_param_data.m_nParamType,
												GetInitParamValue(_event_param_data.m_nParamType),
												_event_param_data.m_sParamName));
	}
	CHECK_RET_CODE_EXP(PrlEvent_FromString(hEvent, _event_etalon.toString().toUtf8().data()))
	PRL_UINT32 nParamsCount = 0;
	CHECK_RET_CODE_EXP(PrlEvent_GetParamsCount(hEvent, &nParamsCount))
	QCOMPARE(nParamsCount, PRL_UINT32(_event_etalon.m_lstEventParameters.size()));
	for (PRL_UINT32 i = 0; i < nParamsCount; ++i)
	{
		QVERIFY(i < sizeof(event_params)/sizeof(EventParameter));
		EventParameter &_event_param_data = event_params[i];
		SdkHandleWrap hEventParam;
		CHECK_RET_CODE_EXP(PrlEvent_GetParam(hEvent, i, hEventParam.GetHandlePtr()))
		CHECK_HANDLE_TYPE(hEventParam, PHT_EVENT_PARAMETER)
		PRL_CHAR sParamName[STR_BUF_LENGTH];
		PRL_UINT32 nParamNameBufLength = STR_BUF_LENGTH;
		CHECK_RET_CODE_EXP(PrlEvtPrm_GetName(hEventParam, sParamName, &nParamNameBufLength))
		QCOMPARE(UTF8_2QSTR(sParamName), UTF8_2QSTR(_event_param_data.m_sParamName));
		PRL_PARAM_FIELD_DATA_TYPE nParamType;
		CHECK_RET_CODE_EXP(PrlEvtPrm_GetType(hEventParam, &nParamType))
		QVERIFY(nParamType == _event_param_data.m_nParamSdkType);
		PRL_CHAR sParamValue[STR_BUF_LENGTH];
		PRL_UINT32 nParamValueBufLength = STR_BUF_LENGTH;
		CHECK_RET_CODE_EXP(PrlEvtPrm_ToString(hEventParam, sParamValue, &nParamValueBufLength))
		QCOMPARE(UTF8_2QSTR(sParamValue), GetInitParamValue(_event_param_data.m_nParamType));
	}
}

#define GET_EVENT_PARAM\
	QVERIFY(PRL_SUCCEEDED(PrlEvent_FromString(hEvent, _vm_event.toString().toUtf8().data())));\
	SdkHandleWrap hEventParam;\
	QVERIFY(PRL_SUCCEEDED(PrlEvent_GetParam(hEvent, 0, hEventParam.GetHandlePtr())));\
	CHECK_HANDLE_TYPE(hEventParam, PHT_EVENT_PARAMETER)

#define ADD_EVENT_PARAM\
	CVmEvent _vm_event;\
	PVE::ParamFieldDataType nExpectParamType = PVE::String;\
	QString sExpectParamName = EVT_PARAM_VM_LICENSE;\
	QString sExpectParamValue = "some license";\
	_vm_event.addEventParameter(new CVmEventParameter(nExpectParamType, sExpectParamValue, sExpectParamName));\
	GET_EVENT_PARAM

void JobsManipulationsTest::testEventParamGetName()
{
	GET_EVENT_OBJECT
	ADD_EVENT_PARAM

	PRL_CHAR sParamName[STR_BUF_LENGTH];
	PRL_UINT32 nParamNameBufLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlEvtPrm_GetName(hEventParam, sParamName, &nParamNameBufLength))

	QCOMPARE(UTF8_2QSTR(sParamName), sExpectParamName);
}

void JobsManipulationsTest::testEventParamGetNameNotEnoughBufSize()
{
	GET_EVENT_OBJECT
	ADD_EVENT_PARAM

	PRL_CHAR sParamName[STR_BUF_LENGTH];
	PRL_UINT32 nParamNameBufLength = sExpectParamName.toUtf8().size();
	QVERIFY(PRL_FAILED(PrlEvtPrm_GetName(hEventParam, sParamName, &nParamNameBufLength)));
}

void JobsManipulationsTest::testEventParamGetNameNullBufSize()
{
	GET_EVENT_OBJECT
	ADD_EVENT_PARAM
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hEventParam, PrlEvtPrm_GetName)
}

void JobsManipulationsTest::testEventParamToString()
{
	GET_EVENT_OBJECT
	ADD_EVENT_PARAM

	PRL_CHAR sParamValue[STR_BUF_LENGTH];
	PRL_UINT32 nParamValueBufLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlEvtPrm_ToString(hEventParam, sParamValue, &nParamValueBufLength))

	QCOMPARE(UTF8_2QSTR(sParamValue), sExpectParamValue);
}

void JobsManipulationsTest::testEventParamToStringNotEnoughBufSize()
{
	GET_EVENT_OBJECT
	ADD_EVENT_PARAM

	PRL_CHAR sParamValue[STR_BUF_LENGTH];
	PRL_UINT32 nParamValueBufLength = sExpectParamValue.toUtf8().size();
	QVERIFY(PRL_FAILED(PrlEvtPrm_ToString(hEventParam, sParamValue, &nParamValueBufLength)));
}

void JobsManipulationsTest::testEventParamToStringNullBufSize()
{
	GET_EVENT_OBJECT
	ADD_EVENT_PARAM
	TEST_BEHAVIOR_ON_NULL_BUF_SIZE(hEventParam, PrlEvtPrm_ToString)
}

void JobsManipulationsTest::testEventGetErrCode()
{
	GET_EVENT_OBJECT
	PRL_VOID_PTR pEvent;
	CHECK_RET_CODE_EXP(PrlEvent_GetDataPtr(hEvent, &pEvent))
	CVmEvent _vm_event(UTF8_2QSTR((const char *)pEvent));
	PrlBuffer_Free(pEvent);
	CHECK_RET_CODE_EXP(_vm_event.m_uiRcInit)
	PRL_RESULT nActualRetCode;
	CHECK_RET_CODE_EXP(PrlEvent_GetErrCode(hEvent, &nActualRetCode))
	QVERIFY(_vm_event.getEventCode() == nActualRetCode);
}

void JobsManipulationsTest::testEventGetErrString()
{
	GET_EVENT_OBJECT

	PRL_CHAR sErrString[STR_BUF_LENGTH];
	PRL_UINT32 nErrStringBufLength = STR_BUF_LENGTH;

	CHECK_RET_CODE_EXP(PrlEvent_GetErrString(hEvent, PRL_FALSE, PRL_FALSE, sErrString, &nErrStringBufLength))
}

#define GET_EVENT_XML_OBJECT\
	PRL_VOID_PTR pEvent;\
	QVERIFY(PRL_SUCCEEDED(PrlEvent_GetDataPtr(hEvent, &pEvent)));\
	CVmEvent _vm_event(UTF8_2QSTR((const char *)pEvent));\
	PrlBuffer_Free(pEvent);\
	CHECK_RET_CODE_EXP(_vm_event.m_uiRcInit)

void JobsManipulationsTest::testEventIsAnswerRequired()
{
	GET_EVENT_OBJECT
	GET_EVENT_XML_OBJECT
	PRL_BOOL bActualAnswerRequired;
	CHECK_RET_CODE_EXP(PrlEvent_IsAnswerRequired(hEvent, &bActualAnswerRequired))
	QVERIFY(PRL_BOOL(_vm_event.getRespRequired()) == bActualAnswerRequired);
}

void JobsManipulationsTest::testEventCanBeIgnored()
{
	GET_EVENT_OBJECT
	PRL_BOOL bActualAnswerRequired;
	CHECK_RET_CODE_EXP(PrlEvent_CanBeIgnored(hEvent, &bActualAnswerRequired))
}

void JobsManipulationsTest::testEventGetIssuerType()
{
	GET_EVENT_OBJECT
	GET_EVENT_XML_OBJECT
	PRL_EVENT_ISSUER_TYPE nActualIssuerType;
	CHECK_RET_CODE_EXP(PrlEvent_GetIssuerType(hEvent, &nActualIssuerType))
	QVERIFY(_vm_event.getEventIssuerType() == nActualIssuerType);
}

void JobsManipulationsTest::testEventGetIssuerId()
{
	GET_EVENT_OBJECT
	GET_EVENT_XML_OBJECT
	PRL_CHAR sIssuerId[STR_BUF_LENGTH];
	PRL_UINT32 nIssuerIdBufLength = STR_BUF_LENGTH;
	CHECK_RET_CODE_EXP(PrlEvent_GetIssuerId(hEvent, sIssuerId, &nIssuerIdBufLength))
	QVERIFY(_vm_event.getEventIssuerId() == UTF8_2QSTR(sIssuerId));
}

void JobsManipulationsTest::testEventParamToUint32()
{
	GET_EVENT_OBJECT
	CVmEvent _vm_event;
	PRL_UINT32 nMagicNum = 23657;
	_vm_event.addEventParameter(new CVmEventParameter(PVE::UnsignedInt, QString::number(nMagicNum),
																										EVT_PARAM_MESSAGE_PARAM_0));
	GET_EVENT_PARAM
	PRL_UINT32 nActualNum;
	CHECK_RET_CODE_EXP(PrlEvtPrm_ToUint32(hEventParam, &nActualNum))
	QCOMPARE(nActualNum, nMagicNum);
}

void JobsManipulationsTest::testEventParamToInt32()
{
	GET_EVENT_OBJECT
	CVmEvent _vm_event;
	PRL_INT32 nMagicNum = 23657;
	_vm_event.addEventParameter(new CVmEventParameter(PVE::Integer, QString::number(nMagicNum),
																										EVT_PARAM_MESSAGE_PARAM_0));
	GET_EVENT_PARAM
	PRL_INT32 nActualNum;
	CHECK_RET_CODE_EXP(PrlEvtPrm_ToInt32(hEventParam, &nActualNum))
	QCOMPARE(nActualNum, nMagicNum);
}

void JobsManipulationsTest::testEventParamToUint64()
{
	GET_EVENT_OBJECT
	CVmEvent _vm_event;
	PRL_UINT64 nMagicNum = 0xABCDEF12345LL;
	_vm_event.addEventParameter(new CVmEventParameter(PVE::UInt64, QString::number(nMagicNum),
														EVT_PARAM_MESSAGE_PARAM_0));
	GET_EVENT_PARAM
	PRL_UINT64 nActualNum;
	CHECK_RET_CODE_EXP(PrlEvtPrm_ToUint64(hEventParam, &nActualNum))
	QCOMPARE(nActualNum, nMagicNum);
}

void JobsManipulationsTest::testEventParamToUint64OnWrongParams()
{
	GET_EVENT_OBJECT
	CVmEvent _vm_event;
	PRL_UINT64 nMagicNum = 0xABCDEF12345LL;
	_vm_event.addEventParameter(new CVmEventParameter(PVE::UInt64, QString::number(nMagicNum),
														EVT_PARAM_MESSAGE_PARAM_0));
	GET_EVENT_PARAM

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlEvtPrm_ToUint64(PRL_INVALID_HANDLE, &nMagicNum),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlEvtPrm_ToUint64(hEventParam, 0),
									PRL_ERR_INVALID_ARG);
}

void JobsManipulationsTest::testEventParamToInt64()
{
	GET_EVENT_OBJECT
	CVmEvent _vm_event;
	PRL_INT64 nMagicNum = 0xABCDEF12345LL;
	_vm_event.addEventParameter(new CVmEventParameter(PVE::Int64, QString::number(nMagicNum),
														EVT_PARAM_MESSAGE_PARAM_0));
	GET_EVENT_PARAM
	PRL_INT64 nActualNum;
	CHECK_RET_CODE_EXP(PrlEvtPrm_ToInt64(hEventParam, &nActualNum))
	QCOMPARE(nActualNum, nMagicNum);
}

void JobsManipulationsTest::testEventParamToInt64OnWrongParams()
{
	GET_EVENT_OBJECT
	CVmEvent _vm_event;
	PRL_INT64 nMagicNum = 0xABCDEF12345LL;
	_vm_event.addEventParameter(new CVmEventParameter(PVE::Int64, QString::number(nMagicNum),
														EVT_PARAM_MESSAGE_PARAM_0));
	GET_EVENT_PARAM

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlEvtPrm_ToInt64(PRL_INVALID_HANDLE, &nMagicNum),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlEvtPrm_ToInt64(hEventParam, 0),
									PRL_ERR_INVALID_ARG);
}

void JobsManipulationsTest::testEventParamToBoolean()
{
	GET_EVENT_OBJECT
	CVmEvent _vm_event;
	PRL_BOOL bExpectedValue = PRL_TRUE;
	_vm_event.addEventParameter(new CVmEventParameter(PVE::Boolean, QString::number(bExpectedValue),
																										EVT_PARAM_MESSAGE_PARAM_0));
	GET_EVENT_PARAM
	PRL_BOOL bActualValue;
	CHECK_RET_CODE_EXP(PrlEvtPrm_ToBoolean(hEventParam, &bActualValue))
	QVERIFY(bActualValue == bExpectedValue);
}

void JobsManipulationsTest::testEventParamToHandleForIncorrectParam()
{
	GET_EVENT_OBJECT
	CVmEvent _vm_event;
	_vm_event.addEventParameter(new CVmEventParameter(PVE::Boolean, QString::number(PRL_TRUE), EVT_PARAM_MESSAGE_PARAM_0));
	GET_EVENT_PARAM
	SdkHandleWrap _handle;
	QVERIFY(PRL_FAILED(PrlEvtPrm_ToHandle(hEventParam, _handle.GetHandlePtr())));
}

void JobsManipulationsTest::testEventParamToHandleForVmConfigParam()
{
	GET_EVENT_OBJECT
	CVmEvent _vm_event;
	CVmConfiguration _vm_conf;
	_vm_event.addEventParameter(new CVmEventParameter(PVE::VmConfiguration, _vm_conf.toString(), EVT_PARAM_VM_CONFIG));
	GET_EVENT_PARAM
	SdkHandleWrap _handle;
	CHECK_RET_CODE_EXP(PrlEvtPrm_ToHandle(hEventParam, _handle.GetHandlePtr()))
	CHECK_HANDLE_TYPE(_handle, PHT_VIRTUAL_MACHINE)
}

void JobsManipulationsTest::testEventParamToHandleForCommonPrefsParam()
{
	GET_EVENT_OBJECT
	CVmEvent _vm_event;
	CDispCommonPreferences _disp_conf;
	_vm_event.addEventParameter(new CVmEventParameter(PVE::String, _disp_conf.toString(), EVT_PARAM_DISP_COMMON_PREFERENSES));
	GET_EVENT_PARAM
	SdkHandleWrap _handle;
	CHECK_RET_CODE_EXP(PrlEvtPrm_ToHandle(hEventParam, _handle.GetHandlePtr()))
	CHECK_HANDLE_TYPE(_handle, PHT_DISP_CONFIG)
}

#define TEST_PARAM_WITH_VM_DEV_CONFIG(vm_dev_type)\
	GET_EVENT_OBJECT\
	GET_EVENT_XML_OBJECT\
	SdkHandleWrap hVmDev;\
	QVERIFY(PRL_SUCCEEDED(PrlVmCfg_CreateVmDev(hVm, vm_dev_type, hVmDev.GetHandlePtr())));\
	PRL_VOID_PTR pVmDevConfig = 0;\
	QVERIFY(PRL_SUCCEEDED(PrlVmDev_ToString(hVmDev, &pVmDevConfig)));\
	_vm_event.addEventParameter(new CVmEventParameter(PVE::String, UTF8_2QSTR((const char *)pVmDevConfig),\
																										EVT_PARAM_VM_CONFIG_DEV_STATE));\
	PrlBuffer_Free(pVmDevConfig);\
	GET_EVENT_PARAM\
	SdkHandleWrap _handle;\
	QVERIFY(PRL_SUCCEEDED(PrlEvtPrm_ToHandle(hEventParam, _handle.GetHandlePtr())));\
\
	PRL_DEVICE_TYPE nDeviceType = PDE_MAX;\
	QVERIFY(PRL_SUCCEEDED(PrlVmDev_GetType(_handle, &nDeviceType)));\
	QVERIFY(nDeviceType == vm_dev_type);

void JobsManipulationsTest::testEventParamToHandleForFloppy()
{
	TEST_PARAM_WITH_VM_DEV_CONFIG(PDE_FLOPPY_DISK)
}

void JobsManipulationsTest::testEventParamToHandleForHardDisk()
{
	TEST_PARAM_WITH_VM_DEV_CONFIG(PDE_HARD_DISK)
}

void JobsManipulationsTest::testEventParamToHandleForOpticalDisk()
{
	TEST_PARAM_WITH_VM_DEV_CONFIG(PDE_OPTICAL_DISK)
}

void JobsManipulationsTest::testEventParamToHandleForSerialPort()
{
	TEST_PARAM_WITH_VM_DEV_CONFIG(PDE_SERIAL_PORT)
}

void JobsManipulationsTest::testEventParamToHandleForParallelPort()
{
	TEST_PARAM_WITH_VM_DEV_CONFIG(PDE_PARALLEL_PORT)
}

void JobsManipulationsTest::testEventParamToHandleForSound()
{
	TEST_PARAM_WITH_VM_DEV_CONFIG(PDE_SOUND_DEVICE)
}

void JobsManipulationsTest::testEventParamToHandleForNetAdapter()
{
	TEST_PARAM_WITH_VM_DEV_CONFIG(PDE_GENERIC_NETWORK_ADAPTER)
}

void JobsManipulationsTest::testEventParamToHandleForUsbDevice()
{
	TEST_PARAM_WITH_VM_DEV_CONFIG(PDE_USB_DEVICE)
}

void JobsManipulationsTest::testEventParamToHandleForGenericPciDevice()
{
	TEST_PARAM_WITH_VM_DEV_CONFIG(PDE_GENERIC_PCI_DEVICE)
}

void JobsManipulationsTest::testEventParamToHandleForGenericScsiDevice()
{
	TEST_PARAM_WITH_VM_DEV_CONFIG(PDE_GENERIC_SCSI_DEVICE)
}

void JobsManipulationsTest::testEventParamToHandleForDisplayDevice()
{
	TEST_PARAM_WITH_VM_DEV_CONFIG(PDE_PCI_VIDEO_ADAPTER)
}

void JobsManipulationsTest::testJobIsRequestWasSentOnNonSentRequest()
{
	SdkHandleWrap hJob(PrlSrv_LoginLocal(PRL_INVALID_HANDLE, 0, 0, PSL_LOW_SECURITY));
	PRL_BOOL bIsRequestWasSent = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlJob_IsRequestWasSent(hJob, &bIsRequestWasSent))
	QVERIFY(bIsRequestWasSent == PRL_FALSE);
}

void JobsManipulationsTest::testJobIsRequestWasSentOnSentRequest()
{
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
	SdkHandleWrap hJob(PrlSrv_LoginLocal(hServer, 0, 0, PSL_LOW_SECURITY));
	PRL_BOOL bIsRequestWasSent = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlJob_IsRequestWasSent(hJob, &bIsRequestWasSent))
	QVERIFY(bIsRequestWasSent == PRL_TRUE);
}

void JobsManipulationsTest::testJobIsRequestWasSentOnWrongParams()
{
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
	SdkHandleWrap hJob(PrlSrv_LoginLocal(PRL_INVALID_HANDLE, 0, 0, PSL_LOW_SECURITY));
	PRL_BOOL bIsRequestWasSent = PRL_FALSE;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlJob_IsRequestWasSent(PRL_INVALID_HANDLE, &bIsRequestWasSent), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlJob_IsRequestWasSent(hServer, &bIsRequestWasSent), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlJob_IsRequestWasSent(hJob, 0), PRL_ERR_INVALID_ARG)
}

void JobsManipulationsTest::testJobGetPackageIdOnWrongParams()
{
	SdkHandleWrap hServer;
	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
	SdkHandleWrap hJob(PrlSrv_LoginLocal(PRL_INVALID_HANDLE, 0, 0, PSL_LOW_SECURITY));
	PRL_UINT64 id;

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlHandle_GetPackageId(PRL_INVALID_HANDLE, &id), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlHandle_GetPackageId(hServer, &id), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlHandle_GetPackageId(hJob, 0), PRL_ERR_INVALID_ARG)
}

void JobsManipulationsTest::testJobGetPackageId()
{
	SdkHandleWrap hServer;
	PRL_RESULT nJobReturnCode = PRL_ERR_UNINITIALIZED;

	CHECK_RET_CODE_EXP(PrlSrv_Create(hServer.GetHandlePtr()))
	SdkHandleWrap hJob(PrlSrv_LoginLocal(hServer, 0, 0, PSL_LOW_SECURITY));
	QVERIFY(hJob != PRL_INVALID_HANDLE);
	CHECK_RET_CODE_EXP(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT))
	CHECK_RET_CODE_EXP(PrlJob_GetRetCode(hJob, &nJobReturnCode))
	QVERIFY(nJobReturnCode == PRL_ERR_SUCCESS);

	PRL_UINT64 id = 0;
	CHECK_RET_CODE_EXP(PrlHandle_GetPackageId(hJob, &id))
	// package id must be non-zero
	QVERIFY(id != 0);
}
