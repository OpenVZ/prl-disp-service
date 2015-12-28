///////////////////////////////////////////////////////////////////////////////
///
/// @file CHostHardwareInfoTest.cpp
///
/// Tests fixture class for testing CHostHardwareInfo class functionality.
///
/// @author van
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
///////////////////////////////////////////////////////////////////////////////

#include "CHostHardwareInfoTest.h"
#include <prlcommon/Logging/Logging.h>

#include <QDomDocument>

CHostHardwareInfoTest::CHostHardwareInfoTest() : m_pHostHardwareInfo(NULL) {
}

void CHostHardwareInfoTest::testInitializeFromString() {
	QFile _file("./CHostHardwareInfoTest_valid_host_hwinfo.xml");
	QVERIFY(_file.open(QIODevice::ReadOnly));
	QDomDocument _xmldoc;
	QVERIFY(_xmldoc.setContent(&_file));
	_file.close();
	m_pHostHardwareInfo = new CHostHardwareInfo(_xmldoc.toString());
	QCOMPARE(m_pHostHardwareInfo->m_uiRcInit, PRL_ERR_SUCCESS);
}

void CHostHardwareInfoTest::testBadNetworkDeviceName()
{
	CHostHardwareInfo _hw_info, _hw_info2;

#define ADAPTER_NAME "PCI Ethernet Slot \0P\1 \0\2\3\4\5A, Port 2"

	CHwNetAdapter *pNetAdapter = new CHwNetAdapter(
		PDE_GENERIC_NETWORK_ADAPTER,
		UTF8SZ_2QSTR(ADAPTER_NAME, sizeof(ADAPTER_NAME)),
		"en1",
		1,
		"00:19:E3:0E:63:E7",
		65535,
		true
	);
	_hw_info.m_lstNetworkAdapters.append(pNetAdapter);

	_hw_info2.fromString(UTF8_2QSTR(_hw_info.toString().toUtf8()));
	QVERIFY(PRL_SUCCEEDED(_hw_info2.m_uiRcInit));
	QCOMPARE(_hw_info.toString(), _hw_info2.toString());
}

void CHostHardwareInfoTest::cleanup() {
	delete m_pHostHardwareInfo;
	m_pHostHardwareInfo = NULL;
}

QTEST_MAIN(CHostHardwareInfoTest)
