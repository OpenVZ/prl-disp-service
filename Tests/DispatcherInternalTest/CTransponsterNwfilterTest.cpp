/////////////////////////////////////////////////////////////////////////////
///
/// Copyright (c) 2020 Virtuozzo International GmbH, All rights reserved.
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
/// Our contact details: Virtuozzo International GmbH, Vordergasse 59, 8200
/// Schaffhausen, Switzerland.
///
/// @file
///		CTransponsterNwfilterTest.cpp
///
/// @author
///		alexander.alekseev
///
/// @brief
///		Tests fixture class for testing DspCmdDirVmClone dispatcher API command functionality.
///
/////////////////////////////////////////////////////////////////////////////
#include "CTransponsterNwfilterTest.h"

#include <QtTest/QtTest>

#include "Tests/DispatcherTestsUtils.h"
#include "Libraries/Transponster/Reverse.h"
#include "Libraries/Transponster/Reverse_p.h"

const QString CTransponsterNwfilterTest::S_FIXTURE_PATH = "./TransponsterNwfilterTestFixtures/%1.xml";
const QString CTransponsterNwfilterTest::S_FILTER_PATH = "./TransponsterNwfilterTestFixtures/%1_filter.xml";
const QString CTransponsterNwfilterTest::S_FILTERREF_PATH = "./TransponsterNwfilterTestFixtures/%1_filterref.xml";
	
void CTransponsterNwfilterTest::init()
{
	m_FixtureNames.clear();
	m_FixtureNames.append("fw_disabled");
	m_FixtureNames.append("fw_disabled_with_pktfilters");
	m_FixtureNames.append("fw_tcp_ipv4_in_only");
	m_FixtureNames.append("fw_tcp_ipv4_out_only");
	m_FixtureNames.append("fw_tcp_ipv6");
	m_FixtureNames.append("fw_tcp_ipv6_ports");
	m_FixtureNames.append("fw_tcp_ipv6_whitelist");
	m_FixtureNames.append("fw_tcp_ipv6_with_pktfilters");
	m_FixtureNames.append("fw_psbm_125586");

	foreach(const QString& fixture, m_FixtureNames)
	{
		// Loading adapter fixtures
		QFile adapter_file(S_FIXTURE_PATH.arg(fixture));
		QVERIFY(adapter_file.open(QIODevice::ReadOnly));
		m_pAdapters.append(SmartPtr<CVmGenericNetworkAdapter>(
						new CVmGenericNetworkAdapter(&adapter_file)));

		// Loading expected nwfilters
		QFile filter_file(S_FILTER_PATH.arg(fixture));
		QVERIFY(filter_file.open(QIODevice::ReadOnly));
		m_Filters.append(filter_file.readAll());

		// Loading expected filterref
		QFile filterref_file(S_FILTERREF_PATH.arg(fixture));
		QVERIFY(filterref_file.open(QIODevice::ReadOnly));
		m_Filterref.append(filterref_file.readAll());
	}
}

void CTransponsterNwfilterTest::cleanup()
{

}

void CTransponsterNwfilterTest::TestFilterref()
{
	for (int i = 0; i < m_FixtureNames.size(); ++i)
		TestSingleFixtureFilterref(i);
}

void CTransponsterNwfilterTest::TestFilter()
{
	for (int i = 0; i < m_FixtureNames.size(); ++i)
		TestSingleFixtureFilter(i);
}

void CTransponsterNwfilterTest::TestSingleFixtureFilter(uint id)
{
	Transponster::Filter::Reverse u(*m_pAdapters[id]);
	QString r = u.getResult();
	if (!QTest::qCompare(r.toUtf8(), m_Filters[id],
		QSTR2UTF8(S_FIXTURE_PATH.arg(m_FixtureNames[id])),
		QSTR2UTF8(S_FILTER_PATH.arg(m_FixtureNames[id])),
		__FILE__, __LINE__))
	{
		WRITE_TRACE(DBG_INFO, "result = %s", QSTR2UTF8(r));
		WRITE_TRACE(DBG_INFO, "expected = %s", m_Filters[id].constData());
	}
}

using Libvirt::Domain::Xml::FilterrefNodeAttributes;

void CTransponsterNwfilterTest::TestSingleFixtureFilterref(uint id)
{
	boost::optional<FilterrefNodeAttributes> filter =
		Transponster::Device::Network::View(*m_pAdapters[id]).getFilterref();

	QByteArray filter_xml;
	if (filter)
	{
		QDomDocument result;
		filter->save(result);
		filter_xml = result.toByteArray();
	}
	QCOMPARE(filter_xml, m_Filterref[id]);
}
