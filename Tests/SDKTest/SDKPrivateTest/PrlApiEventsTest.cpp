/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlApiEventsTest.cpp
///
///	This file is the part of parallels public SDK library private tests suite.
///	Tests fixture class for testing SDK API calls for events handles.
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
#include "PrlApiEventsTest.h"
#include "SDK/Handles/Core/PrlHandleVmEvent.h"
#include "SDK/Handles/Disp/PrlHandleServer.h"
#include "SDK/Handles/Core/PrlErrStringsStorage.h"

PrlApiEventsTest::PrlApiEventsTest()
{}

void PrlApiEventsTest::cleanup()
{
	m_Handle.reset();
}

void PrlApiEventsTest::testEventGetTypeOnVmEvent()
{
	PrlHandleEventPtr pHandle(new PrlHandleVmEvent(PrlHandleServerPtr(0), PET_DSP_EVT_VM_CREATED, 0));
	m_Handle.reset(pHandle->GetHandle());
	PRL_EVENT_TYPE eType = PET_VM_INF_UNINITIALIZED_EVENT_CODE;
	QVERIFY(PrlEvent_GetType(m_Handle, &eType) == PRL_ERR_SUCCESS);
	QVERIFY(eType == PET_DSP_EVT_VM_CREATED);
}

void PrlApiEventsTest::testEventGetTypeOnNonEventHandle()
{
	PrlHandleServerPtr pHandle(new PrlHandleServer);
	m_Handle.reset(pHandle->GetHandle());
	PRL_EVENT_TYPE eType = PET_VM_INF_UNINITIALIZED_EVENT_CODE;
	QVERIFY(PrlEvent_GetType(m_Handle, &eType) == PRL_ERR_INVALID_ARG);
}

void PrlApiEventsTest::testEventGetTypeOnInvalidHandle()
{
	PRL_EVENT_TYPE eType = PET_VM_INF_UNINITIALIZED_EVENT_CODE;
	QVERIFY(PrlEvent_GetType(PRL_INVALID_HANDLE, &eType) == PRL_ERR_INVALID_ARG);
}

void PrlApiEventsTest::testEventGetTypeOnInvalidTypePtr()
{
	PrlHandleEventPtr pHandle(new PrlHandleVmEvent(PrlHandleServerPtr(0), PET_DSP_EVT_VM_CREATED, 0));
	m_Handle.reset(pHandle->GetHandle());
	QVERIFY(PrlEvent_GetType(m_Handle, 0) == PRL_ERR_INVALID_ARG);
}

void PrlApiEventsTest::testEventGetIdOnVmEvent()
{
	PrlHandleEventPtr pHandle(new PrlHandleVmEvent(PrlHandleServerPtr(0), PET_DSP_EVT_VM_CREATED, 0));
	m_Handle.reset(pHandle->GetHandle());
	PRL_UINT64 id;
	QVERIFY(PrlHandle_GetPackageId(m_Handle, &id) == PRL_ERR_SUCCESS);
	QVERIFY(id == 0);
}

void PrlApiEventsTest::testEventGetIdOnNonEventHandle()
{
	PrlHandleServerPtr pHandle(new PrlHandleServer);
	m_Handle.reset(pHandle->GetHandle());
	PRL_UINT64 id;
	QVERIFY(PrlHandle_GetPackageId(m_Handle, &id) == PRL_ERR_INVALID_ARG);
}

void PrlApiEventsTest::testEventGetIdOnInvalidHandle()
{
	PRL_UINT64 id;
	QVERIFY(PrlHandle_GetPackageId(PRL_INVALID_HANDLE, &id) == PRL_ERR_INVALID_ARG);
}

void PrlApiEventsTest::testEventGetIdOnInvalidTypePtr()
{
	PrlHandleEventPtr pHandle(new PrlHandleVmEvent(PrlHandleServerPtr(0), PET_DSP_EVT_VM_CREATED, 0));
	m_Handle.reset(pHandle->GetHandle());
	QVERIFY(PrlHandle_GetPackageId(m_Handle, 0) == PRL_ERR_INVALID_ARG);
}

void PrlApiEventsTest::testEventGetServerOnVmEvent()
{
	PrlHandleEventPtr pHandle(new PrlHandleVmEvent(PrlHandleServerPtr(0), PET_DSP_EVT_VM_CREATED, 0));
	m_Handle.reset(pHandle->GetHandle());
	PRL_HANDLE server = (PRL_HANDLE) 0x6734fda1;
	QVERIFY(PrlEvent_GetServer(m_Handle, &server) == PRL_ERR_INVALID_HANDLE);
	QVERIFY(!server);
}

void PrlApiEventsTest::testEventGetServerOnNonEventHandle()
{
	PrlHandleServerPtr pHandle(new PrlHandleServer);
	m_Handle.reset(pHandle->GetHandle());
	PRL_HANDLE server = 0;
	QVERIFY(PrlEvent_GetServer(m_Handle, &server) == PRL_ERR_INVALID_ARG);
}

void PrlApiEventsTest::testEventGetServerOnInvalidHandle()
{
	PRL_HANDLE server = 0;
	QVERIFY(PrlEvent_GetServer(PRL_INVALID_HANDLE, &server) == PRL_ERR_INVALID_ARG);
}

void PrlApiEventsTest::testEventGetServerOnInvalidTypePtr()
{
	PrlHandleEventPtr pHandle(new PrlHandleVmEvent(PrlHandleServerPtr(0), PET_DSP_EVT_VM_CREATED, 0));
	m_Handle.reset(pHandle->GetHandle());
	QVERIFY(PrlEvent_GetServer(m_Handle, 0) == PRL_ERR_INVALID_ARG);
}

void PrlApiEventsTest::testEventGetDataPtrOnVmEvent()
{
	PrlHandleEventPtr pHandle(new PrlHandleVmEvent(PrlHandleServerPtr(0), PET_DSP_EVT_VM_CREATED, 0));
	m_Handle.reset(pHandle->GetHandle());
	PRL_VOID_PTR pData = 0;
	QVERIFY(PrlEvent_GetDataPtr(m_Handle, &pData) == PRL_ERR_SUCCESS);
	QVERIFY(pData);
}

void PrlApiEventsTest::testEventGetDataPtrOnNonEventHandle()
{
	PrlHandleServerPtr pHandle(new PrlHandleServer);
	m_Handle.reset(pHandle->GetHandle());
	PRL_VOID_PTR pData = 0;
	QVERIFY(PrlEvent_GetDataPtr(m_Handle, &pData) == PRL_ERR_INVALID_ARG);
}

void PrlApiEventsTest::testEventGetDataPtrOnInvalidHandle()
{
	PRL_VOID_PTR pData = 0;
	QVERIFY(PrlEvent_GetDataPtr(PRL_INVALID_HANDLE, &pData) == PRL_ERR_INVALID_ARG);
}

void PrlApiEventsTest::testEventGetDataPtrOnInvalidTypePtr()
{
	PrlHandleEventPtr pHandle(new PrlHandleVmEvent(PrlHandleServerPtr(0), PET_DSP_EVT_VM_CREATED, 0));
	m_Handle.reset(pHandle->GetHandle());
	QVERIFY(PrlEvent_GetDataPtr(m_Handle, 0) == PRL_ERR_INVALID_ARG);
}

void PrlApiEventsTest::testEventParamSubstitution()
{
	QSKIP("Invalid test - should be rewritten", SkipAll);
// there is now another way ...
#define REG_TO_BE_DELETED(obj) m_Handle.reset(obj->GetHandle())

#define MACRO_2_STRING(macro) #macro
#define PARAM_TEXT(no) MACRO_2_STRING(PARAM_##no)
#define BUFF_SIZE 1024

    SmartPtr<CVmEvent> event_data ;
    QString text ;
    PRL_UINT32 buff_size = BUFF_SIZE ;
    char buffer[BUFF_SIZE] ;
    PrlHandleVmEvent *event_obj ;

    {
    static const PRL_RESULT err_code_prm = PRL_NET_CABLE_DISCONNECTED ;
    static const PRL_BOOL brief_prm = PRL_TRUE ;
    static const QString should_prm_raw("The real network adapter <b>%2</b> used by "
                                       "the virtual adapter %1 is not connected to the network.") ;
    static const QString should_prm_fmt("The real network adapter <b>"PARAM_TEXT(2)"</b> used by "
                                       "the virtual adapter "PARAM_TEXT(1)" is not connected to the network.") ;
    static const QString should_prm_nofmt("The real network adapter "PARAM_TEXT(2)" used by "
                                         "the virtual adapter "PARAM_TEXT(1)" is not connected to the network.") ;

    text = PrlErrStringsStorage::GetErrString(err_code_prm, brief_prm) ;
	QCOMPARE(text, should_prm_raw) ;
    event_data = SmartPtr<CVmEvent>(new CVmEvent()) ;
    event_data->setEventCode( err_code_prm );
    event_data->addEventParameter(new CVmEventParameter(PVE::String, PARAM_TEXT(1), EVT_PARAM_MESSAGE_PARAM_0)) ;
    event_data->addEventParameter(new CVmEventParameter(PVE::String, PARAM_TEXT(2), EVT_PARAM_MESSAGE_PARAM_1)) ;

    event_obj = new PrlHandleVmEvent(PrlHandleVmPtr(), event_data.getImpl()) ;
	REG_TO_BE_DELETED(event_obj) ;

    QCOMPARE(event_obj->GetErrString(brief_prm, PRL_TRUE, buffer, &buff_size), PRL_ERR_SUCCESS) ;
	QCOMPARE(QString(buffer), should_prm_fmt);

    buffer[0] = 0 ;
    buff_size = BUFF_SIZE ;
    QCOMPARE(event_obj->GetErrString(brief_prm, PRL_FALSE, buffer, &buff_size), PRL_ERR_SUCCESS);
	QCOMPARE(QString(buffer), should_prm_nofmt);
    }

    //------------ check tag <a ..> removing --------------
    {
    static const PRL_RESULT err_code_a = PRL_ERR_VM_ALREADY_REGISTERED_VM_UUID ;
    static const PRL_BOOL brief_a = PRL_FALSE ;

    text = PrlErrStringsStorage::GetErrString(err_code_a, brief_a) ;
	QVERIFY(!text.contains("<a href"));

    event_data = SmartPtr<CVmEvent>(new CVmEvent()) ;
    event_data->setEventCode(err_code_a) ;
	event_obj = new PrlHandleVmEvent(PrlHandleVmPtr(), event_data.getImpl()) ;
	REG_TO_BE_DELETED(event_obj) ;

    buffer[0] = 0 ;
    buff_size = BUFF_SIZE ;
    QCOMPARE(event_obj->GetErrString(brief_a, PRL_FALSE, buffer, &buff_size), PRL_ERR_SUCCESS) ;
	QVERIFY(!QString(buffer).contains("<a href"));
    }

    //------------ check tag <br> removing --------------
    {
    static const PRL_RESULT err_code_br = PET_QUESTION_OLD_CONFIG_CONVERTION ;
    static const PRL_BOOL brief_br = PRL_FALSE ;
    text = PrlErrStringsStorage::GetErrString(err_code_br, brief_br) ;
	QVERIFY(text.contains("<br>")) ;

    event_data = SmartPtr<CVmEvent>(new CVmEvent()) ;
    event_data->setEventCode(err_code_br) ;
	event_obj = new PrlHandleVmEvent(PrlHandleVmPtr(), event_data.getImpl()) ;
	REG_TO_BE_DELETED(event_obj) ;

    buffer[0] = 0 ;
    buff_size = BUFF_SIZE ;
    QCOMPARE(event_obj->GetErrString(brief_br, PRL_FALSE, buffer, &buff_size), PRL_ERR_SUCCESS) ;
    //WRITE_TRACE(DBG_FATAL, "Got unformated buffer no <br> '%s'", buffer) ;
	QCOMPARE(QString(buffer).indexOf("<br>", 0, Qt::CaseInsensitive), -1) ;
    }

    //------------ check %2 %3 with 0,1,2 params --------------
    {
    static const PRL_RESULT err_code_p23 = PRL_ERR_VMCONF_MAIN_MEMORY_SIZE_NOT_EAQUAL_RECOMMENDED ;
    static const PRL_BOOL brief_p23 = PRL_FALSE ;

    static const QString should_p23_raw("The operating system installed in this virtual machine requires %2 "\
										"MB of RAM, while the maximum amount of RAM that can be allocated to a "\
										"virtual machine on this physical server is %3 MB. Disregarding these "\
										"recommendations may affect the performance of both your server and the "\
										"virtual machine.") ;

    static const QString should_p23("The operating system installed in this virtual machine "\
									"requires "PARAM_TEXT(2)" MB of RAM, while the maximum amount of RAM that "\
									"can be allocated to a virtual machine on this physical server "\
									"is "PARAM_TEXT(3)" MB. Disregarding these recommendations may affect the "\
									"performance of both your server and the virtual machine.") ;
    text = PrlErrStringsStorage::GetErrString(err_code_p23, brief_p23) ;
	QCOMPARE(text, should_p23_raw) ;

    event_data = SmartPtr<CVmEvent>(new CVmEvent()) ;
    event_data->setEventCode(err_code_p23) ;
    event_data->addEventParameter(new CVmEventParameter(PVE::String, PARAM_TEXT(1), EVT_PARAM_MESSAGE_PARAM_0)) ;
    event_data->addEventParameter(new CVmEventParameter(PVE::String, PARAM_TEXT(2), EVT_PARAM_MESSAGE_PARAM_1)) ;
    event_data->addEventParameter(new CVmEventParameter(PVE::String, PARAM_TEXT(3), EVT_PARAM_MESSAGE_PARAM_2)) ;
	event_obj = new PrlHandleVmEvent(PrlHandleVmPtr(), event_data.getImpl()) ;
	REG_TO_BE_DELETED(event_obj) ;

    buffer[0] = 0 ;
    buff_size = BUFF_SIZE ;
    QCOMPARE(event_obj->GetErrString(brief_p23, PRL_FALSE, buffer, &buff_size), PRL_ERR_SUCCESS) ;
	QCOMPARE(QString(buffer), should_p23);
    }

    //------------ check %2 %3 with 0,2 params (invalid situation) --------------
    {
    static const PRL_RESULT err_code_p23_02 = PRL_ERR_VMCONF_MAIN_MEMORY_SIZE_NOT_EAQUAL_RECOMMENDED ;
    static const PRL_BOOL brief_p23_02 = PRL_FALSE ;

	static const QString should_p23_02_raw("The operating system installed in this virtual machine requires %2 "\
										"MB of RAM, while the maximum amount of RAM that can be allocated to a "\
										"virtual machine on this physical server is %3 MB. Disregarding these "\
										"recommendations may affect the performance of both your server and the "\
										"virtual machine.") ;

    static const QString should_p23_02("The operating system installed in this virtual machine requires %2 "\
									"MB of RAM, while the maximum amount of RAM that can be allocated "\
									"to a virtual machine on this physical server is "PARAM_TEXT(3)" MB. "\
									"Disregarding these recommendations may affect the performance of both "\
									"your server and the virtual machine.") ;
    text = PrlErrStringsStorage::GetErrString(err_code_p23_02, brief_p23_02) ;
	QCOMPARE(text, should_p23_02_raw) ;

    event_data = SmartPtr<CVmEvent>(new CVmEvent()) ;
    event_data->setEventCode(err_code_p23_02) ;
    event_data->addEventParameter(new CVmEventParameter(PVE::String, PARAM_TEXT(1), EVT_PARAM_MESSAGE_PARAM_0)) ;
    event_data->addEventParameter(new CVmEventParameter(PVE::String, PARAM_TEXT(3), EVT_PARAM_MESSAGE_PARAM_2)) ;
	event_obj = new PrlHandleVmEvent(PrlHandleVmPtr(), event_data.getImpl()) ;
	REG_TO_BE_DELETED(event_obj) ;

    buffer[0] = 0 ;
    buff_size = BUFF_SIZE ;
    QCOMPARE(event_obj->GetErrString(brief_p23_02, PRL_FALSE, buffer, &buff_size), PRL_ERR_SUCCESS) ;
	QCOMPARE(QString(buffer), should_p23_02);
    }

    //------------ check starting from param subst --------------
    {
    static const PRL_RESULT err_code_start = PRL_ERR_INVALID_PARALLELS_DISK ;
    static const PRL_BOOL brief_start = PRL_TRUE ;

    event_data = SmartPtr<CVmEvent>(new CVmEvent());
    event_data->setEventCode(err_code_start) ;
    event_data->addEventParameter(new CVmEventParameter(PVE::String, PARAM_TEXT(1),
                                                        EVT_PARAM_MESSAGE_PARAM_0)) ;
	event_obj = new PrlHandleVmEvent(PrlHandleVmPtr(), event_data.getImpl()) ;
	REG_TO_BE_DELETED(event_obj) ;

    buffer[0] = 0 ;
    buff_size = BUFF_SIZE ;
    QCOMPARE(quint32(event_obj->GetErrString(brief_start, PRL_FALSE, buffer, &buff_size)),\
		    quint32(PRL_ERR_SUCCESS)) ;
	QVERIFY(UTF8_2QSTR(buffer).contains(PARAM_TEXT(1)));
    }
}
