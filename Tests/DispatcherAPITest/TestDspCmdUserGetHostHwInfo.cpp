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
///		TestDspCmdUserGetHostHwInfo.cpp
///
/// @author
///		sergeyt
///
/// @brief
///		Tests fixture class for testing DspCmdUserGetHostHwInfo dispatcher API command functionality.
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////
#include "TestDspCmdUserGetHostHwInfo.h"

#include <QtTest/QtTest>

#include "SDK/Handles/PveControl.h"
#include "XmlModel/Messaging/CVmEvent.h"
#include "XmlModel/Messaging/CVmEventParameter.h"
#include "XmlModel/HostHardwareInfo/CHostHardwareInfo.h"

#include "Tests/CommonTestsUtils.h"

void TestDspCmdUserGetHostHwInfo::init() {
	m_pHandler->Clear();
	m_pPveControl = new CPveControl(false, m_pHandler);
	Login();
}

void TestDspCmdUserGetHostHwInfo::cleanup() {
	Logoff();
	m_pPveControl->deleteLater();
	m_pPveControl = NULL;
}

void TestDspCmdUserGetHostHwInfo::Test() {
	CALL_CMD(m_pPveControl->DspCmdUserGetHostHwInfo(), PVE::DspCmdUserGetHostHwInfo)
	CResult _result = m_pHandler->GetResult();
	QVERIFY(IS_OPERATION_SUCCEEDED(_result.getReturnCode()));

	QVERIFY(_result.m_hashResultSet.contains(PVE::DspCmdUserGetHostHwInfo_strHostHwInfo));
	QString actualHostInfo = _result.m_hashResultSet[PVE::DspCmdUserGetHostHwInfo_strHostHwInfo];
	QVERIFY(actualHostInfo.size());

	CHostHardwareInfo _host_hi;
	QCOMPARE(int(_host_hi.fromString(actualHostInfo)), int(HostHwInfoParser::RcSuccess));
}
