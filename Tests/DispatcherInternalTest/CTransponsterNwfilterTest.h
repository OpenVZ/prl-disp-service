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
///		CTransponsterNwfilterTest.h
///
/// @author
///		alexander.alekseev
///
/// @brief
///		Tests fixture class for testing nwfilter reverse transponster logic
///
/////////////////////////////////////////////////////////////////////////////
#ifndef CTransponsterNwfilterTest_H
#define CTransponsterNwfilterTest_H

#include <prlxmlmodel/VmConfig/CVmGenericNetworkAdapter.h>
#include <prlcommon/Std/SmartPtr.h>

class CTransponsterNwfilterTest : public QObject
{
   Q_OBJECT

	static const QString S_FIXTURE_PATH;
	static const QString S_FILTER_PATH;
	static const QString S_FILTERREF_PATH;
	
private slots:
	void init();
	void cleanup();

private slots:
	void TestFilterref();
	void TestFilter();

private:
	void TestSingleFixtureFilter(uint id);
	void TestSingleFixtureFilterref(uint id);

	QList<QString> m_FixtureNames;
	QList< SmartPtr<CVmGenericNetworkAdapter> > m_pAdapters;
	QList<QByteArray> m_Filters;
	QList<QByteArray> m_Filterref;
};

#endif // CTransponsterNwfilterTest
