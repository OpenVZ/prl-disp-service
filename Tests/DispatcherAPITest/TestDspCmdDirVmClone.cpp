/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (C) 2006-2015 Parallels IP Holdings GmbH
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
/// @file
///		TestDspCmdDirVmClone.cpp
///
/// @author
///		sergeyt
///
/// @brief
///		Tests fixture class for testing DspCmdDirVmClone dispatcher API command functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#include "TestDspCmdDirVmClone.h"

#include <QtTest/QtTest>

#include "Tests/CommonTestsUtils.h"

#include <prlcommon/PrlUuid/Uuid.h>
#include "SDK/Handles/PveControl.h"
#include <prlxmlmodel/Messaging/CVmEvent.h>
#include <prlxmlmodel/Messaging/CVmEventParameter.h>

#define CREATE_CORRECT_VM_CONFIG\
	QFile _file("./TestDspCmdDirValidateVmConfig_vm_config.xml");\
	QVERIFY(_file.open(QIODevice::ReadOnly));\
	CVmConfiguration _vm_conf(&_file);\
	_vm_conf.getVmIdentification()->setVmName("TestDspCmdDirVmClone_" + _vm_conf.getVmIdentification()->getVmName());\
	_vm_conf.getVmIdentification()->setVmUuid("{1ca7dd05-07b3-48f3-b8d3-ebe0edc7b4ed}");\
	QVERIFY(IS_OPERATION_SUCCEEDED(_vm_conf.m_uiRcInit));\
	m_pVmConfig = SmartPtr<CVmConfiguration>(new CVmConfiguration(_vm_conf.toString()));

void TestDspCmdDirVmClone::init() {
	m_pHandler->Clear();
	m_pPveControl = new CPveControl(false, m_pHandler);
	Login();

	CREATE_CORRECT_VM_CONFIG

	CALL_CMD(m_pPveControl->DspCmdDirVmDelete(m_pVmConfig->getVmIdentification()->getVmUuid().toUtf8().data(),QStringList()), PVE::DspCmdDirVmDelete)
}

void TestDspCmdDirVmClone::cleanup() {
	if (m_pVmConfig.isValid()) {
		CALL_CMD(m_pPveControl->DspCmdDirVmDelete(m_pVmConfig->getVmIdentification()->getVmUuid().toUtf8().data(),QStringList()), PVE::DspCmdDirVmDelete)
		m_pVmConfig = SmartPtr<CVmConfiguration>() ;

	}

	if (m_pVmCloneConfig.isValid()) {
		CALL_CMD(m_pPveControl->DspCmdDirVmDelete(m_pVmCloneConfig->getVmIdentification()->getVmUuid().toUtf8().data(),QStringList()), PVE::DspCmdDirVmDelete)
		m_pVmCloneConfig = SmartPtr<CVmConfiguration>() ;

	}
	Logoff();
	m_pPveControl->deleteLater();
	m_pPveControl = NULL;
}

void TestDspCmdDirVmClone::TestOnValidParams()
{
	QString sNewVmName = Uuid::createUuid().toString();

	CREATE_CORRECT_VM_CONFIG

	const char* emptyStr = "";
	CALL_CMD(m_pPveControl->DspCmdDirVmCreate(_vm_conf.toString().toUtf8().data(), const_cast<char*>(emptyStr)), PVE::DspCmdDirVmCreate)
	CResult _result = m_pHandler->GetResult();
	CHECK_RET_CODE(_result.getReturnCode())

	CALL_CMD(m_pPveControl->DspCmdDirVmClone(_vm_conf.getVmIdentification()->getVmUuid().toUtf8().data(),
			sNewVmName.toUtf8().data(), const_cast<char*>(emptyStr), 0), PVE::DspCmdDirVmClone)
	_result = m_pHandler->GetResult();
	CHECK_RET_CODE(_result.getReturnCode())

	CALL_CMD(m_pPveControl->DspCmdDirGetVmList(), PVE::DspCmdDirGetVmList)

	_result = m_pHandler->GetResult();
	CHECK_RET_CODE(_result.getReturnCode())
	for (size_t i = 0; i < (size_t)_result.GetParamsCount(); ++i)
	{
		CVmConfiguration _vm_conf(_result.GetParamToken(i));
		CHECK_RET_CODE(_vm_conf.m_uiRcInit)
		QVERIFY(_vm_conf.getVmIdentification());
		if (_vm_conf.getVmIdentification()->getVmName() == sNewVmName)
		{
			m_pVmCloneConfig = SmartPtr<CVmConfiguration>(new CVmConfiguration(_result.GetParamToken(i)));
			return;
		}
	}
	QFAIL("Clone VM config not found!");
}
