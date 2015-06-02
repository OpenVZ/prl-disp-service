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
///		CUrlParserTest.cpp
///
/// @author
///		sandro
///
/// @brief
///		Tests fixture class for testing CUrlParser class functionality
///
/// @brief
///		None.
///
/////////////////////////////////////////////////////////////////////////////

#include "CUrlParserTest.h"
#include "Libraries/PrlCommonUtilsBase/CUrlParser.h"

void CUrlParserTest::testCUrlParserOnAllComponentsPresent()
{
	CUrlParser _url("http://someuser:somepassword@somehostname:123");
	QCOMPARE(QString("http"), _url.proto());
	QCOMPARE(QString("someuser"), _url.userName());
	QCOMPARE(QString("somepassword"), _url.password());
	QCOMPARE(QString("somehostname"), _url.host());
	QCOMPARE(quint32(123), _url.port());
}

void CUrlParserTest::testCUrlParserOnProtocolAbsent()
{
	CUrlParser _url("someuser:somepassword@somehostname:123");
	QVERIFY(_url.proto().isEmpty());
	QCOMPARE(QString("someuser"), _url.userName());
	QCOMPARE(QString("somepassword"), _url.password());
	QCOMPARE(QString("somehostname"), _url.host());
	QCOMPARE(quint32(123), _url.port());
}

void CUrlParserTest::testCUrlParserOnJustHostnamePort()
{
	CUrlParser _url("somehostname:123");
	QVERIFY(_url.proto().isEmpty());
	QVERIFY(_url.userName().isEmpty());
	QVERIFY(_url.password().isEmpty());
	QCOMPARE(QString("somehostname"), _url.host());
	QCOMPARE(quint32(123), _url.port());
}

void CUrlParserTest::testCUrlParserOnJustHostname()
{
	CUrlParser _url("somehostname");
	QVERIFY(_url.proto().isEmpty());
	QVERIFY(_url.userName().isEmpty());
	QVERIFY(_url.password().isEmpty());
	QCOMPARE(QString("somehostname"), _url.host());
	QCOMPARE(quint32(0), _url.port());
}

void CUrlParserTest::testCUrlParserOnHostnamePortWithProtocol()
{
	CUrlParser _url("https://somehostname:123");
	QCOMPARE(QString("https"), _url.proto());
	QVERIFY(_url.userName().isEmpty());
	QVERIFY(_url.password().isEmpty());
	QCOMPARE(QString("somehostname"), _url.host());
	QCOMPARE(quint32(123), _url.port());
}

void CUrlParserTest::testCUrlParserOnHostnameWithProtocol()
{
	CUrlParser _url("ftp://somehostname");
	QCOMPARE(QString("ftp"), _url.proto());
	QVERIFY(_url.userName().isEmpty());
	QVERIFY(_url.password().isEmpty());
	QCOMPARE(QString("somehostname"), _url.host());
	QCOMPARE(quint32(0), _url.port());
}

void CUrlParserTest::testCUrlParserSpecificTestCase()
{
	CUrlParser _url("http://proxy.qa.sw.ru:3128");
	QCOMPARE(QString("http"), _url.proto());
	QVERIFY(_url.userName().isEmpty());
	QVERIFY(_url.password().isEmpty());
	QCOMPARE(QString("proxy.qa.sw.ru"), _url.host());
	QCOMPARE(quint32(3128), _url.port());
}

