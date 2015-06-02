/////////////////////////////////////////////////////////////////////////////
///
///	@file TestCallbackCommon.h
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing server login SDK API.
///
///	@author Vadim Hohlov (vhohlov@)
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
#ifndef __TEST_CALLBACK_COMMON_H__
#define __TEST_CALLBACK_COMMON_H__

#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"
#include "Interfaces/ParallelsQt.h"

#define CALLBACK_CHECK_SDK_CALL(call)                                   \
    {                                                                   \
        PRL_RESULT nRetCode = call ;                                    \
        if (PRL_FAILED(nRetCode)) {                                     \
            WRITE_TRACE(DBG_FATAL, "%s call failed. Error: %.8X", #call, nRetCode) ; \
            return nRetCode ;                                           \
        }                                                               \
    }



typedef PRL_RESULT (*unreg_proc_t)(PRL_HANDLE, PRL_EVENT_HANDLER_PTR, PRL_VOID_PTR) ;
struct check_callback_t {

        QWaitCondition condition ;
        QMutex         mutex ;
        QString        s_result ;
        unsigned       u_result ;

        PRL_HANDLE     handle ;
        PRL_EVENT_TYPE event_type ;
		PRL_JOB_OPERATION_CODE op_code ;
        unreg_proc_t   unreg_proc ;
        SdkHandleWrap  got_event ;
		SdkHandleWrap  got_job ;
} ;
#define DEFINE_CHECK_CALLBACK(callback_param, h_value, evt_type, unreg_proc_value, job_op_code) \
    check_callback_t callback_param ;                                   \
    callback_param.handle = h_value ;                                   \
    callback_param.event_type = evt_type ;                              \
    callback_param.unreg_proc = unreg_proc_value ;                      \
	callback_param.op_code = job_op_code ;									\
    callback_param.u_result = 0 ;

static PRL_RESULT event_callback(PRL_HANDLE _handle, void *pData)
{
	SdkHandleWrap hEvent(_handle);
	PRL_HANDLE_TYPE _type;
	CALLBACK_CHECK_SDK_CALL( PrlHandle_GetType(hEvent, &_type) ) ;
    check_callback_t *check_cb = static_cast<check_callback_t *>(pData) ;

    QMutexLocker _lock(&check_cb->mutex) ;

	if (_type == PHT_EVENT)
	{
		PRL_EVENT_TYPE event_type ;

		CALLBACK_CHECK_SDK_CALL( PrlEvent_GetType(hEvent, &event_type) ) ;
		if (event_type != check_cb->event_type)
			return PRL_ERR_SUCCESS ;

		PRL_CHAR buff[STR_BUF_LENGTH];
		PRL_UINT32 buff_len = sizeof(buff) ;
		CALLBACK_CHECK_SDK_CALL( PrlEvent_GetIssuerId(hEvent, buff, &buff_len) ) ;

		check_cb->s_result = UTF8_2QSTR(buff) ;

		PRL_UINT32 nParamsCount = 0 ;
		CALLBACK_CHECK_SDK_CALL( PrlEvent_GetParamsCount(hEvent, &nParamsCount) ) ;

		check_cb->u_result = nParamsCount ;
		check_cb->got_event = hEvent ;

		if ( check_cb->unreg_proc )
		{
			CALLBACK_CHECK_SDK_CALL( check_cb->unreg_proc(check_cb->handle, event_callback, pData) ) ;
			check_cb->unreg_proc = NULL ;
		}
	}
	else if (_type == PHT_JOB)
	{
		PRL_JOB_OPERATION_CODE job_op_code ;

		CALLBACK_CHECK_SDK_CALL( PrlJob_GetOpCode(hEvent, &job_op_code) ) ;
		if (job_op_code != check_cb->op_code)
			return PRL_ERR_SUCCESS ;

		check_cb->got_job = hEvent;

		if ( check_cb->unreg_proc )
		{
			CALLBACK_CHECK_SDK_CALL( check_cb->unreg_proc(check_cb->handle, event_callback, pData) ) ;
			check_cb->unreg_proc = NULL;
		}
	}

    check_cb->condition.wakeAll() ;

	return PRL_ERR_SUCCESS ;
}

#endif //__TEST_CALLBACK_COMMON_H__

