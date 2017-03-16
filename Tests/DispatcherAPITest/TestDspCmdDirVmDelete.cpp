/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2006-2017, Parallels International GmbH
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
/// @file
///		TestDspCmdDirVmDelete.cpp
///
/// @author
///		sergeyt
///
/// @brief
///		Tests fixture class for testing DspCmdDirVmDelete dispatcher API command functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#include "TestDspCmdDirVmDelete.h"

#include <QtTest/QtTest>

#include "Tests/CommonTestsUtils.h"

#include "SDK/Handles/PveControl.h"
#include <prlcommon/Messaging/CVmEvent.h>
#include <prlcommon/Messaging/CVmEventParameter.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include "Libraries/PrlCommonUtils/CAuthHelper.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"

namespace {
	QString g_sParentDir = "./parent_dir";
	QString g_sVmDir = "./parent_dir/vm_dir";
}

#define CREATE_VM_DIR\
	QVERIFY(QDir().mkdir(g_sParentDir));\
	QVERIFY(QFile(g_sParentDir).setPermissions(QFile::Permissions(0xFFFF)));\
	CAuthHelper _auth(TestConfig::getUserLogin());\
	QVERIFY(_auth.AuthUser(TestConfig::getUserPassword()));\
	_auth.Impersonate();\
	QVERIFY(CFileHelper::CreateDirectoryPath(g_sVmDir, &_auth));\
	_auth.RevertToSelf();

#define CREATE_CORRECT_VM_CONFIG\
	QFile _file("./TestDspCmdDirValidateVmConfig_vm_config.xml");\
	QVERIFY(_file.open(QIODevice::ReadOnly));\
	CVmConfiguration _vm_conf(&_file);\
	_vm_conf.getVmIdentification()->setVmName("TestDspCmdDirVmDelete_" + _vm_conf.getVmIdentification()->getVmName());\
	_vm_conf.getVmIdentification()->setVmUuid("{1ca7dd05-07b3-48f3-b8d3-ebe0edc7b4ed}");\

TestDspCmdDirVmDelete::TestDspCmdDirVmDelete() : m_pPveControl2(NULL) {}

void TestDspCmdDirVmDelete::initTestCase() {
	InitializeTooLongString();
}

void TestDspCmdDirVmDelete::cleanupTestCase() {
	CleanupTooLongString();
}

void TestDspCmdDirVmDelete::init() {
	m_pHandler->Clear();
	m_pPveControl = new CPveControl(false, m_pHandler);
	Login();
	CREATE_CORRECT_VM_CONFIG
	CALL_CMD(m_pPveControl->DspCmdDirVmDelete(_vm_conf.getVmIdentification()->getVmUuid().toUtf8().data(),QStringList()), PVE::DspCmdDirVmDelete)

	if (QFile::exists(g_sParentDir))
		CFileHelper::ClearAndDeleteDir(g_sParentDir);
}

void TestDspCmdDirVmDelete::cleanup() {
	CREATE_CORRECT_VM_CONFIG
	CALL_CMD(m_pPveControl->DspCmdDirVmDelete(_vm_conf.getVmIdentification()->getVmUuid().toUtf8().data(),QStringList()), PVE::DspCmdDirVmDelete)

	if (QFile::exists(g_sParentDir))
		CFileHelper::ClearAndDeleteDir(g_sParentDir);
	Logoff();
	m_pPveControl->deleteLater();
	m_pPveControl = NULL;
	if (m_pPveControl2) {
		QString sReqId = m_pPveControl2->DspCmdUserLogoff();
		QVERIFY(m_pPveControl2->WaitForRequestComplete(sReqId, PRL_JOB_WAIT_TIMEOUT));
		m_pPveControl2->deleteLater();
		m_pPveControl2 = NULL;
	}
}

void TestDspCmdDirVmDelete::TestOnValidParams() {
	CREATE_CORRECT_VM_CONFIG;

	QString sVmConfigPath;

	{
		const char* emptyStr = "";
		CALL_CMD(m_pPveControl->DspCmdDirVmCreate(_vm_conf.toString().toUtf8().data(), const_cast<char*>(emptyStr)), PVE::DspCmdDirVmCreate)
	}
	CResult _result = m_pHandler->GetResult();
	CHECK_RET_CODE(_result.getReturnCode())

	CALL_CMD(m_pPveControl->DspCmdDirVmDelete(_vm_conf.getVmIdentification()->getVmUuid().toUtf8().data(),QStringList()), PVE::DspCmdDirVmDelete)

	_result = m_pHandler->GetResult();
	CHECK_RET_CODE(_result.getReturnCode())
}

void TestDspCmdDirVmDelete::TestOnNonAccessVmDir() {
#ifdef _WIN_
	QSKIP("Skipping due unexplanable permissions issues under Vista", SkipAll);
#endif
	CREATE_CORRECT_VM_CONFIG;

	QString sVmConfigPath;

	{
		const char* emptyStr = "";
		CALL_CMD(m_pPveControl->DspCmdDirVmCreate(_vm_conf.toString().toUtf8().data(), const_cast<char*>(emptyStr)), PVE::DspCmdDirVmCreate)
	}
	CResult _result = m_pHandler->GetResult();
	CHECK_RET_CODE(_result.getReturnCode())

	m_pPveControl2 = new CPveControl(false, m_pHandler);
	CALL_CMD(m_pPveControl2->DspCmdUserLogin(TestConfig::getRemoteHostName(), TestConfig::getUserLogin2(),
											 TestConfig::getUserPassword(), NULL, PSL_HIGH_SECURITY,
											 CONNECTION_TIMEOUT), PVE::DspCmdUserLogin)
	_result = m_pHandler->GetResult();
	CHECK_RET_CODE(_result.getReturnCode())

	{
		m_pHandler->Clear();
		QString sReqId = m_pPveControl2->DspCmdDirVmDelete(_vm_conf.getVmIdentification()->getVmUuid().toUtf8().data(),QStringList());

		const quint32 nStepInterval = 100;
		m_pPveControl2->WaitForRequestComplete(sReqId, PRL_JOB_WAIT_TIMEOUT);
		quint32 nWaitTimeout = PRL_JOB_WAIT_TIMEOUT;
		while (m_pHandler->GetResult().getOpCode() != PVE::DspCmdDirVmDelete && nWaitTimeout)
		{
			QCoreApplication::processEvents(QEventLoop::AllEvents);
			if (nWaitTimeout < nStepInterval)
				nWaitTimeout = 0;
			else
				nWaitTimeout -= nStepInterval;
		}
	}
	_result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
	CALL_CMD(m_pPveControl->DspCmdDirVmDelete(_vm_conf.getVmIdentification()->getVmUuid().toUtf8().data(),QStringList()), PVE::DspCmdDirVmDelete)

	_result = m_pHandler->GetResult();
	CHECK_RET_CODE(_result.getReturnCode())
}

void TestDspCmdDirVmDelete::TestOnInvalidVmUuid() {
	CALL_CMD(m_pPveControl->DspCmdDirVmDelete(Uuid::createUuid().toString().toUtf8().data(),QStringList()), PVE::DspCmdDirVmDelete)

	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspCmdDirVmDelete::TestOnEmptyVmUuid() {
	CALL_CMD(m_pPveControl->DspCmdDirVmDelete(QString().toUtf8().data(),QStringList()), PVE::DspCmdDirVmDelete)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspCmdDirVmDelete::TestOnTooLongVmUuid() {
	CALL_CMD(m_pPveControl->DspCmdDirVmDelete(GetTooLongString(),QStringList()), PVE::DspCmdDirVmDelete)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}
