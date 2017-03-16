/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlVirtualNetworkTest.h
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing virtual network manipulating SDK API.
///
///	@author myakhin
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
#ifndef PrlVirtualNetworkTest_H
#define PrlVirtualNetworkTest_H

#include <QtTest/QtTest>

#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"

class PrlVirtualNetworkTest : public QObject
{
Q_OBJECT

private slots:
	void init();
	void cleanup();
	void testLoginLocal();
	void testCreateVirtualNetwork();
	void testCreateVirtualNetworkOnWrongParams();
	void testNetworkId();
	void testNetworkIdOnWrongParams();
	void testDescription();
	void testDescriptionOnWrongParams();
	void testNetworkType();
	void testNetworkTypeOnWrongParams();
	void testBoundCardMac();
	void testBoundCardMacOnWrongParams();
	void testAdapterName();
	void testAdapterNameOnWrongParams();
	void testAdapterIndex();
	void testAdapterIndexOnWrongParams();
	void testHostIPAddress();
	void testHostIPAddressOnWrongParams();
	void testHostIP6Address();
	void testHostIP6AddressOnWrongParams();
	void testDhcpIPAddress();
	void testDhcpIPAddressOnWrongParams();
	void testDhcpIP6Address();
	void testDhcpIP6AddressOnWrongParams();
	void testIPNetMask();
	void testIPNetMaskOnWrongParams();
	void testIP6NetMask();
	void testIP6NetMaskOnWrongParams();
	void testVlanTag();
	void testVlanTagOnWrongParams();
	void testIPScopeStart();
	void testIPScopeStartOnWrongParams();
	void testIPScopeEnd();
	void testIPScopeEndOnWrongParams();
	void testIP6ScopeStart();
	void testIP6ScopeStartOnWrongParams();
	void testIP6ScopeEnd();
	void testIP6ScopeEndOnWrongParams();
	void testEnabled();
	void testEnabledOnWrongParams();
	void testAdapterEnabled();
	void testAdapterEnabledOnWrongParams();
	void testDHCPServerEnabled();
	void testDHCPServerEnabledOnWrongParams();
	void testDHCP6ServerEnabled();
	void testDHCP6ServerEnabledOnWrongParams();
	void testNATServerEnabled();
	void testNATServerEnabledOnWrongParams();
	void testCreatePortForwardEntry();
	void testCreatePortForwardEntryOnWrongParams();
	void testTcpPortForwardList();
	void testUdpPortForwardList();
	void testPortForwardListOnWrongParams();
	void testRuleName();
	void testRuleNameOnWrongParams();
	void testIncomingPort();
	void testIncomingPortOnWrongParams();
	void testRedirectIPAddress();
	void testRedirectIPAddressOnWrongParams();
	void testRedirectPort();
	void testRedirectPortOnWrongParams();
	void testRedirectVm();
	void testRedirectVmOnWrongParams();
	void testGetVirtualNetworkList();
	void testGetVirtualNetworkListOnWrongParams();
	void testAddVirtualNetworkOnDuplicateNetworkId();
	void testAddVirtualNetworkOnWrongParams();
	void testUpdateVirtualNetworkOnDuplicateNetworkId();
	void testUpdateVirtualNetworkOnWrongParams();
	void testDeleteVirtualNetworkOnNotExistingNetworkId();
	void testDeleteVirtualNetworkOnWrongParams();
	void testGetBoundAdapterInfo();
	void testGetBoundAdapterInfoOnWrongParams();
	void testGetBoundAdapterInfoOnNoSuchAdapterAtHostHwInfo();

private:
	SdkHandleWrap	m_ServerHandle;
	SdkHandleWrap	m_VirtNetHandle;
	SdkHandleWrap	m_JobHandle;

};


#endif	// PrlVirtualNetworkTest_H
