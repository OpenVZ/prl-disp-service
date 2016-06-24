/////////////////////////////////////////////////////////////////////////////
///
///	@file SimpleServerWrapper.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	Simple server handle wrapper for convenience.
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

#include "SimpleServerWrapper.h"
#include "Tests/CommonTestsUtils.h"
#include <prlsdk/PrlApi.h>
#include <prlcommon/Logging/Logging.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>

#include <QFile>
#include <QTextStream>

#define READ_VM_CONFIG_INTO_BUF\
	QFile _file("./TestDspCmdDirValidateVmConfig_vm_config.xml");\
	if (!_file.open(QIODevice::ReadOnly))\
		WRITE_TRACE(DBG_FATAL, "Couldn't to open VM configuration file");\
	QTextStream _stream(&_file);\
	QString _config = _stream.readAll();

SimpleServerWrapper::SimpleServerWrapper(char *sUserLogin, bool bUseNonInteractiveMode)
: m_bIsVmWasCreatedByThisInstance(false), m_isLocalAdminInited(false)
{
	if (PRL_SUCCEEDED(PrlSrv_Create(m_ServerHandle.GetHandlePtr())))
		Login(sUserLogin, bUseNonInteractiveMode);
	else
		WRITE_TRACE(DBG_FATAL, "Failed to create server handle");
}

SimpleServerWrapper::~SimpleServerWrapper()
{
	if (m_VmHandle != PRL_INVALID_HANDLE && m_bIsVmWasCreatedByThisInstance)
		DELETE_VM(m_VmHandle)

	Logoff();
}

SdkHandleWrap SimpleServerWrapper::GetServerHandle()
{
	return m_ServerHandle;
}

bool SimpleServerWrapper::Logoff()
{
	m_isLocalAdminInited = false;

	SdkHandleWrap hJob(PrlSrv_Logoff(m_ServerHandle));
	if (PRL_SUCCEEDED(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT)))
	{
		PRL_RESULT nRetCode = PRL_ERR_UNINITIALIZED;
		if (PRL_SUCCEEDED(PrlJob_GetRetCode(hJob, &nRetCode)))
		{
			if (PRL_SUCCEEDED(nRetCode))
				return (true);
			else
				WRITE_TRACE(DBG_FATAL, "Logoff operation failed with retcode: 0x%.8X '%s'",\
					nRetCode, PRL_RESULT_TO_STRING(nRetCode));
		}
		else
			WRITE_TRACE(DBG_FATAL, "Failed to extract return code from the job object");
	}
	else
		WRITE_TRACE(DBG_FATAL, "Failed to wait logoff job");

	return (false);
}

bool SimpleServerWrapper::Login(char *sUserLogin, bool bUseNonInteractiveMode)
{
	SdkHandleWrap hJob;
	PRL_UINT32 nFlags = bUseNonInteractiveMode
		? PACF_NON_INTERACTIVE_MODE
		: 0;
	if (sUserLogin)
	{
		hJob.reset(PrlSrv_LoginEx(m_ServerHandle, TestConfig::getRemoteHostName(),	sUserLogin,
								TestConfig::getUserPassword(), NULL, 0, 0, PSL_HIGH_SECURITY, nFlags));
	}
	else
	{
		hJob.reset(PrlSrv_LoginLocalEx(m_ServerHandle, NULL, 0, PSL_HIGH_SECURITY, nFlags));
	}

	if (PRL_SUCCEEDED(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT)))
	{
		PRL_RESULT nRetCode;
		if (PRL_SUCCEEDED(PrlJob_GetRetCode(hJob, &nRetCode)))
		{
			if (!PRL_SUCCEEDED(nRetCode))
				WRITE_TRACE(DBG_FATAL, "Failed to login to server");
			else
				return (true);
		}
		else
			WRITE_TRACE(DBG_FATAL, "Failed to get job return code");
	}
	else
		WRITE_TRACE(DBG_FATAL, "Failed to wait server login async job");

	return (false);
}

bool SimpleServerWrapper::IsConnected()
{
	PRL_BOOL bConnected = false;
	PRL_BOOL bIsConnectionLocal = false;
	if (PRL_SUCCEEDED(PrlSrv_IsConnected(m_ServerHandle, &bConnected, &bIsConnectionLocal)))
		return (bConnected);
	else
		WRITE_TRACE(DBG_FATAL, "Failed to get connected server sign");
	return (false);
}

bool SimpleServerWrapper::CreateTestVm(const SdkHandleWrap &hVm)
{
	if (PRL_INVALID_HANDLE != hVm)
		m_VmHandle = hVm;
	else
	{
		if (PRL_SUCCEEDED(PrlSrv_CreateVm(m_ServerHandle, m_VmHandle.GetHandlePtr())))
		{
			READ_VM_CONFIG_INTO_BUF
			if (PRL_SUCCEEDED(PrlVm_FromString(m_VmHandle, _config.toUtf8().data())))
			{
				if (PRL_FAILED(PrlVmCfg_SetName(m_VmHandle, QTest::currentTestFunction())))
				{
					WRITE_TRACE(DBG_FATAL, "Failed to apply VM name: '%s'", QTest::currentTestFunction());
					return (false);
				}
			}
			else
			{
				WRITE_TRACE(DBG_FATAL, "Couldn't to assign VM configuration");
				return (false);
			}
		}
		else
		{
			WRITE_TRACE(DBG_FATAL, "Couldn't to create VM handle");
			return (false);
		}
	}

	SdkHandleWrap hJob(PrlVm_Delete(m_VmHandle,PRL_INVALID_HANDLE));
	PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT);
	hJob.reset(PrlVm_Reg(m_VmHandle, "", PRL_TRUE));
	if (PRL_SUCCEEDED(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT*10)))
	{
		PRL_RESULT nRetCode;
		if (PRL_SUCCEEDED(PrlJob_GetRetCode(hJob, &nRetCode)))
		{
			if (PRL_SUCCEEDED(nRetCode))
			{
				m_bIsVmWasCreatedByThisInstance = true;
				return (true);
			}
			else
				WRITE_TRACE(DBG_FATAL, "VM creation failed with ret code: %.8X", nRetCode);
		}
		else
			WRITE_TRACE(DBG_FATAL, "Couldn't to extract job return code");
	}
	else
		WRITE_TRACE(DBG_FATAL, "Failed to wait create VM async job");

	return (false);
}

bool SimpleServerWrapper::RegisterVm(const QString &sVmConfigPath)
{
	SdkHandleWrap hJob(PrlSrv_RegisterVm(m_ServerHandle, QSTR2UTF8(sVmConfigPath), PRL_TRUE));
	if (PRL_SUCCEEDED(PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT)))
	{
		PRL_RESULT nRetCode;
		if (PRL_SUCCEEDED(PrlJob_GetRetCode(hJob, &nRetCode)))
		{
			SdkHandleWrap hResult;
			nRetCode = PrlJob_GetResult(hJob, hResult.GetHandlePtr());
			if (PRL_SUCCEEDED(nRetCode))
			{
				nRetCode = PrlResult_GetParam(hResult, m_VmHandle.GetHandlePtr());
				if (PRL_SUCCEEDED(nRetCode))
				{
					m_bIsVmWasCreatedByThisInstance = true;
					return (true);
				}
				else
					WRITE_TRACE(DBG_FATAL, "VM register failed with ret code: %.8X", nRetCode);
			}
			else
				WRITE_TRACE(DBG_FATAL, "Failed to extract result from job with retcode: %.8X", nRetCode);
		}
		else
			WRITE_TRACE(DBG_FATAL, "Couldn't to extract job return code");
	}
	else
		WRITE_TRACE(DBG_FATAL, "Failed to wait register VM async job");

	return (false);
}

SdkHandleWrap SimpleServerWrapper::GetTestVm( QString sSearchVmUuid )
{
	if ( PRL_INVALID_HANDLE != m_VmHandle )
		return m_VmHandle;

	SdkHandleWrap hVm;
	SdkHandleWrap hJob;

	if (sSearchVmUuid.isEmpty())
	{
		READ_VM_CONFIG_INTO_BUF
		CVmConfiguration _vm_conf(_config);
		if (PRL_SUCCEEDED(_vm_conf.m_uiRcInit))
			sSearchVmUuid = _vm_conf.getVmIdentification()->getVmUuid();
		else
		{
			WRITE_TRACE(DBG_FATAL, "VM config parsing error. Test VM conf: [%s]", _config.toUtf8().data());
			return SdkHandleWrap();
		}
	}

	return GetVmByUuid( sSearchVmUuid );
}

SdkHandleWrap SimpleServerWrapper::GetVmByUuid( const QString& sVmUuid )
{
	SdkHandleWrap hVm;
	SdkHandleWrap hJob;
	PRL_RESULT nRetCode;

	try
	{
		nRetCode = PrlSrv_CreateVm(m_ServerHandle, hVm.GetHandlePtr());
		if (PRL_FAILED(nRetCode))
			throw "Failed to create Vm  Handle";

		nRetCode = PrlVmCfg_SetUuid( hVm, QSTR2UTF8(sVmUuid) );
		if (PRL_FAILED(nRetCode))
			throw "Failed to set Vm Uuid";

		hJob.reset(PrlVm_RefreshConfig(hVm));
		nRetCode = PrlJob_Wait(hJob, PRL_JOB_WAIT_TIMEOUT);
		if (PRL_FAILED(nRetCode))
			throw "Failed to get job config";

		if (PRL_FAILED(PrlJob_GetRetCode(hJob, &nRetCode)))
			throw "Failed to get job ret code";

		if (PRL_FAILED(nRetCode))
			throw "Failed to get Vm config";

		return hVm;
	}
	catch( const char* sErr )
	{
		WRITE_TRACE( DBG_FATAL, "Error occurs: %s, errcode = %#x %s"
			, sErr, nRetCode, PRL_RESULT_TO_STRING(nRetCode) );
		return SdkHandleWrap();
	}
}

SdkHandleWrap SimpleServerWrapper::GetUserProfile()
{
	SdkHandleWrap hUserProfile;

	bool bRes=false;
	GetUserProfile(bRes, m_ServerHandle,hUserProfile);
	if( bRes )
		return hUserProfile;
	return SdkHandleWrap();
}

bool SimpleServerWrapper::isLocalAdmin()
{
	if( m_isLocalAdminInited )
		return m_isLocalAdmin;

	SdkHandleWrap hUserProfile = GetUserProfile();
	if( !hUserProfile.valid() )
		return false;

	PRL_BOOL isAdmin;
	PRL_RESULT res = PrlUsrCfg_IsLocalAdministrator(hUserProfile, &isAdmin);
	if (!QTest::qVerify((PRL_SUCCEEDED(res)), "PrlUsrCfg_IsLocalAdministrator", "", __FILE__, __LINE__))\
		return false;

	m_isLocalAdminInited = true;
	m_isLocalAdmin = (bool)isAdmin;

	return m_isLocalAdmin;
}


void SimpleServerWrapper::GetUserProfile( bool& bOutRes
	, const SdkHandleWrap& hServerHandle, SdkHandleWrap& out_hUserProfile )
{
	bOutRes = false;

	SdkHandleWrap hJob, hResult;
	hJob.reset(PrlSrv_GetUserProfile(hServerHandle));
	CHECK_JOB_RET_CODE(hJob);
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()) );
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, out_hUserProfile.GetHandlePtr()) );

	bOutRes = true;
}

