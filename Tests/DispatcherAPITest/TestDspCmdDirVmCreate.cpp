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
///		TestDspCmdDirVmCreate.cpp
///
/// @author
///		sergeyt
///
/// @brief
///		Tests fixture class for testing DspCmdDirVmCreate dispatcher API command functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#include "TestDspCmdDirVmCreate.h"

#include <QtTest/QtTest>

#include "Tests/CommonTestsUtils.h"

#include "SDK/Handles/PveControl.h"
#include <prlcommon/Messaging/CVmEvent.h>
#include <prlcommon/Messaging/CVmEventParameter.h>
#include <prlxmlmodel/VmConfig/CVmConfiguration.h>
#include "Libraries/PrlCommonUtils/CAuthHelper.h"
#include "Libraries/PrlCommonUtils/CFileHelper.h"

namespace {
	QString g_sVmDir = "./vm_dir";
}

#define CREATE_VM_DIR(Permissions)\
	QVERIFY(QDir().mkdir(g_sVmDir));\
	QVERIFY(QFile(g_sVmDir).setPermissions(Permissions));

#define CREATE_CORRECT_VM_CONFIG\
	QFile _file("./TestDspCmdDirValidateVmConfig_vm_config.xml");\
	QVERIFY(_file.open(QIODevice::ReadOnly));\
	CVmConfiguration _vm_conf(&_file);\
	QVERIFY(_vm_conf.getVmIdentification());\
	_vm_conf.getVmIdentification()->setVmName("TestDspCmdDirVmCreate_" + _vm_conf.getVmIdentification()->getVmName());\
	_vm_conf.getVmIdentification()->setVmUuid("{1ca7dd05-07b3-48f3-b8d3-ebe0edc7b4ed}");\
	m_sVmUuid = _vm_conf.getVmIdentification()->getVmUuid();

void TestDspCmdDirVmCreate::initTestCase() {
	InitializeTooLongString();
}

void TestDspCmdDirVmCreate::cleanupTestCase() {
	CleanupTooLongString();
}

void TestDspCmdDirVmCreate::init() {
	if (QFile::exists(g_sVmDir))
		CFileHelper::ClearAndDeleteDir(g_sVmDir);
	m_pHandler->Clear();
	m_pPveControl = new CPveControl(false, m_pHandler);
	Login();

	CREATE_CORRECT_VM_CONFIG

	CALL_CMD(m_pPveControl->DspCmdDirVmDelete(m_sVmUuid.toUtf8().data(),QStringList()), PVE::DspCmdDirVmDelete)
}

void TestDspCmdDirVmCreate::cleanup() {
	if (m_sVmUuid.size()) {
		CALL_CMD(m_pPveControl->DspCmdDirVmDelete(m_sVmUuid.toUtf8().data(),QStringList()), PVE::DspCmdDirVmDelete)

		m_sVmUuid = "";
	}
	Logoff();
	m_pPveControl->deleteLater();
	m_pPveControl = NULL;
	if (QFile::exists(g_sVmDir))
		CFileHelper::ClearAndDeleteDir(g_sVmDir);
}

void TestDspCmdDirVmCreate::TestOnValidParams()
{
	QString sVmConfigPath;

	CREATE_CORRECT_VM_CONFIG

	{
		const char* emptyStr = "";
		CALL_CMD(m_pPveControl->DspCmdDirVmCreate(_vm_conf.toString().toUtf8().data(), const_cast<char*>(emptyStr)), PVE::DspCmdDirVmCreate)
	}
	CResult _result = m_pHandler->GetResult();
	CHECK_RET_CODE(_result.getReturnCode())
}

void TestDspCmdDirVmCreate::TestOnInvalidConfig()
{
	QString sVmConfigPath;

	QFile _file("./CVmConfigurationTest_third_party.xml");
	QVERIFY(_file.open(QIODevice::ReadOnly));
	QDomDocument _vm_conf;
	QVERIFY(_vm_conf.setContent(&_file, false));

	{
		const char* emptyStr = "";
		CALL_CMD(m_pPveControl->DspCmdDirVmCreate(_vm_conf.toString().toUtf8().data(), const_cast<char*>(emptyStr)), PVE::DspCmdDirVmCreate)
	}

	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspCmdDirVmCreate::TestOnEmptyConfig() {
	QString sVmConfigPath;

	{
		const char* emptyStr = "";
		CALL_CMD(m_pPveControl->DspCmdDirVmCreate(QString().toUtf8().data(), const_cast<char*>(emptyStr)), PVE::DspCmdDirVmCreate)
	}

	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspCmdDirVmCreate::TestOnTooLongConfig() {
	QString sVmConfigPath;

	{
		const char* emptyStr = "";
		CALL_CMD(m_pPveControl->DspCmdDirVmCreate(GetTooLongString(), const_cast<char*>(emptyStr)), PVE::DspCmdDirVmCreate)
	}

	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}

void TestDspCmdDirVmCreate::TestOnTooLongVmDir() {
	CREATE_CORRECT_VM_CONFIG
	CALL_CMD(m_pPveControl->DspCmdDirVmCreate(_vm_conf.toString().toUtf8().data(), GetTooLongString()), PVE::DspCmdDirVmCreate)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(PRL_FAILED(_result.getReturnCode()));
}
