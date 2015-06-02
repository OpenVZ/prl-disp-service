/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlNetworkShapingTest.h
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing virtual network manipulating SDK API.
///
///	@author myakhin
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
/////////////////////////////////////////////////////////////////////////////
#ifndef PrlNetworkShapingTest_H
#define PrlNetworkShapingTest_H

#include <QtTest/QtTest>

#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"

class PrlNetworkShapingTest : public QObject
{
Q_OBJECT

private slots:
	void init();
	void cleanup();
	void testLoginLocal();
	void testCreateNetworkClass();
	void testCreateNetworkClassOnWrongParams();
	void testCreateNetworkShapingEntry();
	void testCreateNetworkShapingEntryWrongParams();
	void testCreateNetworkBandwidthEntryWrongParams();
	void testCreateNetworkBandwidthEntry();
	void testUpdateNetworkClassesConfig();
	void testUpdateNetworkShapingConfig();
	void testCreateNetworkRateOnWrongParams();
	void testSetVmRate();

private:
	void CreateNetworkClass(SdkHandleWrap &hClass, PRL_UINT32 nClassId);
	void CreateNetworkShapingEntry(SdkHandleWrap &hEntry, PRL_UINT32 nClassId, const QString &sDev);
	void CreateVm();
	void DestroyVm();
	void CreateNetworkRate(SdkHandleWrap &hEntry, PRL_UINT32 nClassId);
	void AddNetworkClassesConfig();
	void RestoreNetworkClassesConfig();


private:
	SdkHandleWrap	m_hServer;
	SdkHandleWrap	m_hJob;
	SdkHandleWrap	m_hVm;
	SdkHandleWrap	m_hClassesListOrig;

};

#endif	// PrlNetworkShapingTest_H
