///////////////////////////////////////////////////////////////////////////////
///
/// @file CClientPreferencesTest.cpp
///
/// Tests fixture class for testing CClientPreferences class functionality.
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

#include "CClientPreferencesTest.h"
#include "Tests/CommonTestsUtils.h"

#include <QDomDocument>

CClientPreferencesTest::CClientPreferencesTest()
: m_pClientPreferences(NULL), m_pFile(NULL)
{
}

#define INITIALIZE_CONFIG(sConfigFileName)	m_pFile = new QFile(sConfigFileName);\
	QVERIFY(m_pFile->open(QIODevice::ReadOnly));\
	m_pClientPreferences = new CClientPreferences(m_pFile);

void CClientPreferencesTest::testIsValidOnValidDispConfig() {
	INITIALIZE_CONFIG("./CClientPreferencesTest_valid_client_prefs.xml")
	CHECK_RET_CODE(m_pClientPreferences->m_uiRcInit)
}

void CClientPreferencesTest::testInitializeFromString() {
	QFile _file("./CClientPreferencesTest_valid_client_prefs.xml");
	QVERIFY(_file.open(QIODevice::ReadOnly));
	QDomDocument _xmldoc;
	QVERIFY(_xmldoc.setContent(&_file));
	_file.close();
	m_pClientPreferences = new CClientPreferences(_xmldoc.toString());
	CHECK_RET_CODE(m_pClientPreferences->m_uiRcInit)
}

void CClientPreferencesTest::cleanup() {
	delete m_pClientPreferences;
	m_pClientPreferences = NULL;
	delete m_pFile;
	m_pFile = NULL;
}

QTEST_MAIN(CClientPreferencesTest)
