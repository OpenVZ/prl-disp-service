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
#include "CQDomElementHelperTest.h"

#include <QtTest/QtTest>

#include "Tests/DispatcherTestsUtils.h"
#include "Libraries/Transponster/Reverse.h"
#include "Libraries/Transponster/Reverse_p.h"

void CTransponsterNwfilterTest::init()
{

	const QString FIXTURE_FOLDER	=	"./TransponsterNwfilterTestFixtures/";
	const QString FIXTURE_PATH		=	FIXTURE_FOLDER + "%1.xml";
	const QString FILTER_PATH		=	FIXTURE_FOLDER + "%1_filter.xml";
	const QString FILTERREF_PATH	=	FIXTURE_FOLDER + "%1_filterref.xml";
	const QString FILTERREF_SUFFIX	=	"_filterref.xml";

	QDir directory(FIXTURE_FOLDER);
	QStringList images = directory.entryList(QStringList() << QString("*" + FILTERREF_SUFFIX), QDir::Files);
	foreach(QString filename, images)
	{
		QString fixture(QString(filename).remove(FILTERREF_SUFFIX));
		m_FixtureNames.append(fixture);

		// Loading adapter fixtures
		QFile adapter_file(FIXTURE_PATH.arg(fixture));
		QVERIFY2(adapter_file.open(QIODevice::ReadOnly), QSTR2UTF8("Can't open file " + adapter_file.fileName()));
		m_pAdapters.append(SmartPtr<CVmGenericNetworkAdapter>(
						new CVmGenericNetworkAdapter(&adapter_file)));

		// Loading expected nwfilters
		QFile filter_file(FILTER_PATH.arg(fixture));
		QVERIFY2(filter_file.open(QIODevice::ReadOnly), QSTR2UTF8("Can't open file " + filter_file.fileName()));
		m_Filters.append(filter_file.readAll());

		// Loading expected filterref
		QFile filterref_file(FILTERREF_PATH.arg(fixture));
		QVERIFY2(filterref_file.open(QIODevice::ReadOnly), QSTR2UTF8("Can't open file " + filterref_file.fileName()));
		m_Filterref.append(filterref_file.readAll());
	}

	QVERIFY2(m_pAdapters.size() > 0, "There is no any adapter for test");
	QCOMPARE(m_pAdapters.size(), m_Filters.size());
	QCOMPARE(m_pAdapters.size(), m_Filterref.size());
	QCOMPARE(m_pAdapters.size(), m_FixtureNames.size());
}

void CTransponsterNwfilterTest::cleanup()
{

}

void CTransponsterNwfilterTest::TestFilter()
{
	for (int i = 0; i < m_pAdapters.size(); ++i)
	{
		Transponster::Filter::Reverse u(*m_pAdapters[i]);
		QDomDocument result;
		result.setContent(u.getResult());
		QDomDocument verifyWith;
		verifyWith.setContent(m_Filters[i]);
		CQDomElementHelperTest tester("Filter test: " + m_FixtureNames[i]);
		tester.testElement(result.documentElement(), verifyWith.documentElement());
	}
}

using Libvirt::Domain::Xml::FilterrefNodeAttributes;
void CTransponsterNwfilterTest::TestFilterref()
{
	for (int i = 0; i < m_pAdapters.size(); ++i)
	{
		boost::optional<FilterrefNodeAttributes> filter =
			Transponster::Device::Network::View(*m_pAdapters[i]).getFilterref();

		QByteArray filter_xml;
		if (filter)
		{
			QDomDocument resultTransposter;
			filter->save(resultTransposter);

			QDomDocument verifyWith;
			verifyWith.setContent(m_Filterref[i]);

			CQDomElementHelperTest tester("Filterref test: " + m_FixtureNames[i]);
			tester.testElement(resultTransposter.documentElement(), verifyWith.documentElement());
		}
		else
		{
			QCOMPARE(m_Filterref[i].size(), 0);
		}

	}
}
