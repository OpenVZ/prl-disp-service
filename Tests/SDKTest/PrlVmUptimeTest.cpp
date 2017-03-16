/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlVmUptimeTest.cpp
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing VM uptime feature.
///
///	@author sandro
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

#include "PrlVmUptimeTest.h"
#include "AutoHelpers.h"
#include "Tests/CommonTestsUtils.h"
#include "SimpleServerWrapper.h"
#include <prlcommon/Interfaces/ParallelsDomModel.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include <prlcommon/PrlUuid/Uuid.h>

#include <QDir>

#define START_STOP_VM\
	{\
		SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));\
		CHECK_JOB_RET_CODE(hJob)\
		QTest::qWait(1000);\
		stop_vm_on_exit stop_vm(_connection.GetTestVm());\
	}


void PrlVmUptimeTest::testGetVmUptimeForCreateVm()
{
	SimpleServerWrapper _connection(NULL);
	QVERIFY(_connection.IsConnected());

	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(_connection.GetServerHandle(), hVm.GetHandlePtr()))
	SdkHandleWrap hJob(PrlSrv_GetSrvConfig(_connection.GetServerHandle()));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hSrvConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hSrvConfig.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(hVm, hSrvConfig, PVS_GUEST_VER_WIN_VISTA, PRL_TRUE))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(hVm, QTest::currentTestFunction()))

	QString sStartDate;
	PRL_EXTRACT_STRING_VALUE(sStartDate, hVm, PrlVmCfg_GetUptimeStartDate)
	QDateTime client_side_start_date = QDateTime::fromString(sStartDate, XML_DATETIME_FORMAT);

	QTest::qWait(1000);
	QVERIFY(_connection.CreateTestVm(hVm));
	START_STOP_VM

	hJob.reset(PrlVm_RefreshConfig(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	PRL_EXTRACT_STRING_VALUE(sStartDate, _connection.GetTestVm(), PrlVmCfg_GetUptimeStartDate)
	QVERIFY(QDateTime::fromString(sStartDate, XML_DATETIME_FORMAT) > client_side_start_date);

	PRL_UINT64 nVmUptime = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUptime(_connection.GetTestVm(), &nVmUptime))
	QVERIFY(nVmUptime != 0);
}

void PrlVmUptimeTest::testGetVmUptimeForRegisterVm()
{
	SimpleServerWrapper _connection(NULL);
	QVERIFY(_connection.IsConnected());

	SdkHandleWrap hVm;
	CHECK_RET_CODE_EXP(PrlSrv_CreateVm(_connection.GetServerHandle(), hVm.GetHandlePtr()))
	SdkHandleWrap hJob(PrlSrv_GetSrvConfig(_connection.GetServerHandle()));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	SdkHandleWrap hSrvConfig;
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hSrvConfig.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlVmCfg_SetDefaultConfig(hVm, hSrvConfig, PVS_GUEST_VER_WIN_VISTA, PRL_FALSE))
	CHECK_RET_CODE_EXP(PrlVmCfg_SetName(hVm, QTest::currentTestFunction()))
	PRL_VOID_PTR pBuffer = NULL;
	CHECK_RET_CODE_EXP(PrlHandle_ToString(hVm, &pBuffer))
	QVERIFY(pBuffer);
	CVmConfiguration _vm_conf;
	_vm_conf.fromString(UTF8_2QSTR((const char *)pBuffer));
	PrlBuffer_Free(pBuffer);
	CHECK_RET_CODE_EXP(_vm_conf.m_uiRcInit)
	_vm_conf.getVmIdentification()->setVmUptimeInSeconds(1000);

	QDateTime client_side_start_date = _vm_conf.getVmIdentification()->getVmUptimeStartDateTime();

	QString sVmConfigPath = QString("%1/%2/%3").arg(QDir::tempPath()).arg(Uuid::createUuid().toString()).arg(VMDIR_DEFAULT_VM_CONFIG_FILE);
	QVERIFY(QDir().mkpath(QFileInfo(sVmConfigPath).absolutePath()));
	QFile _file(sVmConfigPath);
	QVERIFY(_file.open(QIODevice::WriteOnly));
	_file.write(_vm_conf.toString().toUtf8());
	_file.close();

	QTest::qWait(1000);
	QVERIFY(_connection.RegisterVm(sVmConfigPath));

	QString sStartDate;
	PRL_EXTRACT_STRING_VALUE(sStartDate, _connection.GetTestVm(), PrlVmCfg_GetUptimeStartDate)
	QVERIFY(QDateTime::fromString(sStartDate, XML_DATETIME_FORMAT) > client_side_start_date);

	PRL_UINT64 nVmUptime = 1000;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUptime(_connection.GetTestVm(), &nVmUptime))
	QVERIFY(nVmUptime == 0);
}

void PrlVmUptimeTest::testGetVmUptimeUpdatingDuringVmWork()
{
	SimpleServerWrapper _connection(NULL);
	QVERIFY(_connection.IsConnected());
	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)
	stop_vm_on_exit stop_vm(_connection.GetTestVm());

	QTest::qWait(1000);
	hJob.reset(PrlVm_RefreshConfig(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	PRL_UINT64 nVmUptime1 = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUptime(_connection.GetTestVm(), &nVmUptime1))
	QVERIFY(nVmUptime1 != 0);

	QTest::qWait(2000);
	hJob.reset(PrlVm_RefreshConfig(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	PRL_UINT64 nVmUptime2 = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUptime(_connection.GetTestVm(), &nVmUptime2))
	QVERIFY(nVmUptime2 != 0);

	QVERIFY(nVmUptime2 > nVmUptime1);
}

void PrlVmUptimeTest::testGetVmUptimeAccumulatingFromStartToStart()
{
	SimpleServerWrapper _connection(NULL);
	QVERIFY(_connection.IsConnected());
	QVERIFY(_connection.CreateTestVm());

	START_STOP_VM

	SdkHandleWrap hJob(PrlVm_RefreshConfig(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	PRL_UINT64 nVmUptime1 = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUptime(_connection.GetTestVm(), &nVmUptime1))
	QVERIFY(nVmUptime1 > 0);

	START_STOP_VM

	hJob.reset(PrlVm_RefreshConfig(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	PRL_UINT64 nVmUptime2 = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUptime(_connection.GetTestVm(), &nVmUptime2))
	QVERIFY(nVmUptime2 > 0);

	QVERIFY(nVmUptime2 > nVmUptime1);
}

void PrlVmUptimeTest::testGetVmUptimeOnWrongParams()
{
	SimpleServerWrapper _connection(NULL);
	QVERIFY(_connection.IsConnected());
	QVERIFY(_connection.CreateTestVm());
	PRL_UINT64 nVmUptime = 0;
	PRL_UINT32 nBufSize = 0;

	//Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetUptime(PRL_INVALID_HANDLE, &nVmUptime), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetUptime(_connection, &nVmUptime), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetUptimeStartDate(PRL_INVALID_HANDLE, 0, &nBufSize), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetUptimeStartDate(_connection, 0, &nBufSize), PRL_ERR_INVALID_ARG)

	//Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetUptime(_connection.GetTestVm(), 0), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVmCfg_GetUptimeStartDate(_connection.GetTestVm(), 0, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmUptimeTest::testCantToChangeVmUptimeWithVmEditAction()
{
	SimpleServerWrapper _connection(NULL);
	QVERIFY(_connection.IsConnected());
	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_BeginEdit(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	QString sExpectedStartDate;
	PRL_EXTRACT_STRING_VALUE(sExpectedStartDate, _connection.GetTestVm(), PrlVmCfg_GetUptimeStartDate)

	PRL_UINT64 nExpectedVmUptime = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUptime(_connection.GetTestVm(), &nExpectedVmUptime))

	PRL_VOID_PTR pBuffer = NULL;
	CHECK_RET_CODE_EXP(PrlHandle_ToString(_connection.GetTestVm(), &pBuffer))

	CVmConfiguration _vm_conf(UTF8_2QSTR((const char *)pBuffer));
	PrlBuffer_Free(pBuffer);
	CHECK_RET_CODE_EXP(_vm_conf.m_uiRcInit)

	_vm_conf.getVmIdentification()->setVmUptimeStartDateTime(QDateTime::currentDateTime());
	_vm_conf.getVmIdentification()->setVmUptimeInSeconds(_vm_conf.getVmIdentification()->getVmUptimeInSeconds() + 1);
	CHECK_RET_CODE_EXP(PrlHandle_FromString(_connection.GetTestVm(), QSTR2UTF8(_vm_conf.toString())))

	hJob.reset(PrlVm_Commit(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_RefreshConfig(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	QString sActualStartDate;
	PRL_EXTRACT_STRING_VALUE(sActualStartDate, _connection.GetTestVm(), PrlVmCfg_GetUptimeStartDate)

	PRL_UINT64 nActualVmUptime = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUptime(_connection.GetTestVm(), &nActualVmUptime))

	QCOMPARE(nExpectedVmUptime, nActualVmUptime);
	QCOMPARE(sExpectedStartDate, sActualStartDate);
}

void PrlVmUptimeTest::testResetVmUptime()
{
	SimpleServerWrapper _connection(NULL);
	QVERIFY(_connection.IsConnected());
	QVERIFY(_connection.CreateTestVm());

	START_STOP_VM

	SdkHandleWrap hJob(PrlVm_RefreshConfig(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	QString sVmUptimeStartDate;
	PRL_EXTRACT_STRING_VALUE(sVmUptimeStartDate, _connection.GetTestVm(), PrlVmCfg_GetUptimeStartDate)
	QDateTime _initial_start_date = QDateTime::fromString(sVmUptimeStartDate, XML_DATETIME_FORMAT);

	PRL_UINT64 nVmUptime = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUptime(_connection.GetTestVm(), &nVmUptime))
	QVERIFY(nVmUptime != 0);

	hJob.reset(PrlVm_ResetUptime(_connection.GetTestVm(), 0));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_RefreshConfig(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	PRL_EXTRACT_STRING_VALUE(sVmUptimeStartDate, _connection.GetTestVm(), PrlVmCfg_GetUptimeStartDate)
	QDateTime _current_start_date = QDateTime::fromString(sVmUptimeStartDate, XML_DATETIME_FORMAT);
	QVERIFY(_initial_start_date < _current_start_date);

	CHECK_RET_CODE_EXP(PrlVmCfg_GetUptime(_connection.GetTestVm(), &nVmUptime))
	QVERIFY(nVmUptime == 0);
}

void PrlVmUptimeTest::testResetVmUptimeForRunningVm()
{
	SimpleServerWrapper _connection(NULL);
	QVERIFY(_connection.IsConnected());
	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)
	stop_vm_on_exit stop_vm(_connection.GetTestVm());

	QTest::qWait(5000);
	hJob.reset(PrlVm_RefreshConfig(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	PRL_UINT64 nVmUptime1 = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUptime(_connection.GetTestVm(), &nVmUptime1))
	QVERIFY(nVmUptime1 != 0);

	hJob.reset(PrlVm_ResetUptime(_connection.GetTestVm(), 0));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_RefreshConfig(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)

	PRL_UINT64 nVmUptime2 = 0;
	CHECK_RET_CODE_EXP(PrlVmCfg_GetUptime(_connection.GetTestVm(), &nVmUptime2))

	QVERIFY(nVmUptime2 < nVmUptime1);
}

void PrlVmUptimeTest::testResetVmUptimeOnWrongParams()
{
	SimpleServerWrapper _connection(NULL);

	//Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlVm_ResetUptime(PRL_INVALID_HANDLE, 0), PRL_ERR_INVALID_ARG)
	CHECK_ASYNC_OP_FAILED(PrlVm_ResetUptime(_connection, 0), PRL_ERR_INVALID_ARG)
}

void PrlVmUptimeTest::testNewVmUptimeFunctionalityDoesntBrokeOldOne()
{
	SimpleServerWrapper _connection(NULL);
	QVERIFY(_connection.IsConnected());
	QVERIFY(_connection.CreateTestVm());

	SdkHandleWrap hJob(PrlVm_Start(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)
	stop_vm_on_exit stop_vm(_connection.GetTestVm());

	PRL_UINT64 nOsUptime1 = 0, nOsUptime2 = 0;
	hJob.reset(PrlVm_GetStatistics(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)
	SdkHandleWrap hResult, hVmStat;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmStat.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlStat_GetOsUptime(hVmStat, &nOsUptime1))

	QTest::qWait(2000);
	hJob.reset(PrlVm_ResetUptime(_connection.GetTestVm(), 0));
	CHECK_JOB_RET_CODE(hJob)

	hJob.reset(PrlVm_GetStatistics(_connection.GetTestVm()));
	CHECK_JOB_RET_CODE(hJob)
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hVmStat.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlStat_GetOsUptime(hVmStat, &nOsUptime2))

	QVERIFY(nOsUptime2 > nOsUptime1);
}

void PrlVmUptimeTest::testResetVmUptimeForVmClone()
{
	SimpleServerWrapper server(NULL);
	QVERIFY(server.IsConnected());
	QVERIFY(server.CreateTestVm());

	SdkHandleWrap vm(server.GetTestVm());
	SdkHandleWrap job(PrlVm_Start(vm));
	CHECK_JOB_RET_CODE(job)

        QTest::qWait(2000);

	{
		stop_vm_on_exit stop_vm(vm);
	}

	job.reset(PrlVm_RefreshConfig(vm));
	CHECK_JOB_RET_CODE(job)

	PRL_UINT64 uptime = ~0ULL;
	CHECK_RET_CODE_EXP( PrlVmCfg_GetUptime(vm, &uptime) );

	QVERIFY(0 < uptime);

	QString name;
	PRL_EXTRACT_STRING_VALUE(name, vm, PrlVmCfg_GetName);

	job.reset(PrlVm_CloneEx(vm, QSTR2UTF8(name + "_clone"), ""
		, PACF_NON_INTERACTIVE_MODE | PCVF_DETACH_EXTERNAL_VIRTUAL_HDD));
	CHECK_JOB_RET_CODE(job);

	SdkHandleWrap clone;
	SdkHandleWrap result;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(job, result.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlResult_GetParam(result, clone.GetHandlePtr()));
	CHECK_HANDLE_TYPE(clone, PHT_VIRTUAL_MACHINE);
	AutoDeleteVm guard(clone);


	job.reset(PrlVm_RefreshConfig(clone));
	CHECK_JOB_RET_CODE(job)

	uptime = ~0ULL;
	CHECK_RET_CODE_EXP( PrlVmCfg_GetUptime(clone, &uptime) );

	QVERIFY(0 == uptime);
}

