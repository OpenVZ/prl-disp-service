/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlCtManipulationsTest.h
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing VM manipulating SDK API.
///
///	@author sandro
///
/// Copyright (c) 2005-2017, Parallels International GmbH
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
/////////////////////////////////////////////////////////////////////////////
#ifndef PrlCtManipulationsTest_H
#define PrlCtManipulationsTest_H

#include <QtTest/QtTest>

#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"

class PrlCtManipulationsTest : public QObject
{

Q_OBJECT
public:
	PrlCtManipulationsTest();

private:
	void test_login();
	void getCachedOstemplate(QString &);
	void createVm(const QString& sVmName, bool& bRes, SdkHandleWrap& hVmHandle);
	void createCt(const QString& sVmName, const QString& sSample,
			bool& bRes, SdkHandleWrap& hVmHandle);

private slots:
	void initTestCase();
	void init();
	void cleanup();
	void cleanupTestCase();

private slots:
	void testCreateCt();
	void testCreateCtFromSample();
	void testUnRegisterCt();
	void testRegisterCt();
	void testCreateCtWithSameName();
	void testChangeCtName();
	void testSetUserPasswd();
	void testStartStop();
	void testSubscribeToPerfStats();
	void testGetPerfStats();
	void testSetDnsServers();
	void testSetSearchDomains();
	void testApplyIpOnly();
	void testClone();
	void testGetCapabilities();
	void testSetCapabilities();
	void testGetNetfilterMode();
	void testSetNetfilterMode();

private:
	QString		m_sServerUuid;
	SdkHandleWrap	m_ServerHandle;
	SdkHandleWrap	m_hSrvConfig;
	SdkHandleWrap	m_hVm;
	SdkHandleWrap	m_JobHandle;
	QString		m_sHomePath;
	QList<SdkHandleWrap> m_lstVmHandles;
};

#endif
