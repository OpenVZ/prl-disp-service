/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlVirtualNetworkTest.cpp
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
#include "PrlVirtualNetworkTest.h"
#include "Tests/CommonTestsUtils.h"
#include "Interfaces/ParallelsQt.h"
#include "XmlModel/NetworkConfig/CVirtualNetwork.h"
#include "XmlModel/HostHardwareInfo/CHostHardwareInfo.h"


void PrlVirtualNetworkTest::init()
{
	m_ServerHandle.reset();
	CHECK_RET_CODE_EXP(PrlSrv_Create(m_ServerHandle.GetHandlePtr()));

	m_VirtNetHandle.reset();
	m_JobHandle.reset();
}

void PrlVirtualNetworkTest::cleanup()
{
	m_JobHandle.reset(PrlSrv_Logoff(m_ServerHandle));
	PrlJob_Wait(m_JobHandle, PRL_JOB_WAIT_TIMEOUT);
}

void PrlVirtualNetworkTest::testLoginLocal()
{
	m_JobHandle.reset(PrlSrv_LoginLocal(m_ServerHandle, "", 0, PSL_HIGH_SECURITY));
	QVERIFY(m_JobHandle != PRL_INVALID_HANDLE);
	CHECK_JOB_RET_CODE(m_JobHandle);
}

void PrlVirtualNetworkTest::testCreateVirtualNetwork()
{
	CHECK_RET_CODE_EXP(PrlVirtNet_Create(m_VirtNetHandle.GetHandlePtr()));
	CHECK_HANDLE_TYPE(m_VirtNetHandle, PHT_VIRTUAL_NETWORK);
}

void PrlVirtualNetworkTest::testCreateVirtualNetworkOnWrongParams()
{
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_Create(NULL), PRL_ERR_INVALID_ARG);
}

#define VIRTUAL_NETWORK_TO_XML_OBJECT \
	PRL_VOID_PTR pXml = 0; \
	CHECK_RET_CODE_EXP(PrlHandle_ToString(m_VirtNetHandle, &pXml)); \
	CVirtualNetwork virt_net; \
	virt_net.fromString( UTF8_2QSTR((PRL_CONST_STR )pXml) ); \
	PrlBuffer_Free(pXml);

#define VIRTUAL_NETWORK_FROM_XML_OBJECT \
	CHECK_RET_CODE_EXP(PrlHandle_FromString( m_VirtNetHandle, QSTR2UTF8(virt_net.toString()) ));

void PrlVirtualNetworkTest::testNetworkId()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	virt_net.setNetworkID("id 1");
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	QString qsNetworkId = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsNetworkId, m_VirtNetHandle, PrlVirtNet_GetNetworkId);
	QVERIFY(qsNetworkId == "id 1");

	QString qsNetworkIdExpected = "text for test";
	CHECK_RET_CODE_EXP(PrlVirtNet_SetNetworkId(m_VirtNetHandle, QSTR2UTF8(qsNetworkIdExpected)));
	PRL_EXTRACT_STRING_VALUE(qsNetworkId, m_VirtNetHandle, PrlVirtNet_GetNetworkId);

	QCOMPARE(qsNetworkId, qsNetworkIdExpected);
}

void PrlVirtualNetworkTest::testNetworkIdOnWrongParams()
{
	testCreateVirtualNetwork();

	QString qsStr = "some text";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetNetworkId(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetNetworkId(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetNetworkId(m_VirtNetHandle, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetNetworkId(m_VirtNetHandle, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlVirtNet_SetNetworkId(m_VirtNetHandle, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetNetworkId(m_VirtNetHandle, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
}

void PrlVirtualNetworkTest::testDescription()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	virt_net.setDescription("some text");
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	QString qsDescription = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsDescription, m_VirtNetHandle, PrlVirtNet_GetDescription);
	QVERIFY(qsDescription == "some text");

	QString qsDescriptionExpected = "text for test";
	CHECK_RET_CODE_EXP(PrlVirtNet_SetDescription(m_VirtNetHandle, QSTR2UTF8(qsDescriptionExpected)));
	PRL_EXTRACT_STRING_VALUE(qsDescription, m_VirtNetHandle, PrlVirtNet_GetDescription);

	QCOMPARE(qsDescription, qsDescriptionExpected);
}

void PrlVirtualNetworkTest::testDescriptionOnWrongParams()
{
	testCreateVirtualNetwork();

	QString qsStr = "some text";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetDescription(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetDescription(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetDescription(m_VirtNetHandle, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetDescription(m_VirtNetHandle, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlVirtNet_SetDescription(m_VirtNetHandle, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetDescription(m_VirtNetHandle, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
}

void PrlVirtualNetworkTest::testNetworkType()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	virt_net.setNetworkType(PVN_HOST_ONLY);
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	PRL_NET_VIRTUAL_NETWORK_TYPE nNetworkType = PVN_BRIDGED_ETHERNET;
	CHECK_RET_CODE_EXP(PrlVirtNet_GetNetworkType(m_VirtNetHandle, &nNetworkType));
	QVERIFY(nNetworkType == PVN_HOST_ONLY);

	PRL_NET_VIRTUAL_NETWORK_TYPE nNetworkTypeExpected = PVN_BRIDGED_ETHERNET;
	CHECK_RET_CODE_EXP(PrlVirtNet_SetNetworkType(m_VirtNetHandle, nNetworkTypeExpected));
	CHECK_RET_CODE_EXP(PrlVirtNet_GetNetworkType(m_VirtNetHandle, &nNetworkType));
	QCOMPARE(nNetworkType, nNetworkTypeExpected);
}

void PrlVirtualNetworkTest::testNetworkTypeOnWrongParams()
{
	testCreateVirtualNetwork();

	PRL_NET_VIRTUAL_NETWORK_TYPE nNetworkType = PVN_BRIDGED_ETHERNET;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetNetworkType(m_ServerHandle, &nNetworkType),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetNetworkType(m_ServerHandle, nNetworkType),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetNetworkType(m_VirtNetHandle, NULL),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testBoundCardMac()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	virt_net.setBoundCardMac("00:AA:BB:CC:DD:EE");
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	QString qsBoundCardMac = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsBoundCardMac, m_VirtNetHandle, PrlVirtNet_GetBoundCardMac);
	QVERIFY(qsBoundCardMac == "00:AA:BB:CC:DD:EE");

	QString qsBoundCardMacExpected = "text for test";
	CHECK_RET_CODE_EXP(PrlVirtNet_SetBoundCardMac(m_VirtNetHandle, QSTR2UTF8(qsBoundCardMacExpected)));
	PRL_EXTRACT_STRING_VALUE(qsBoundCardMac, m_VirtNetHandle, PrlVirtNet_GetBoundCardMac);

	QCOMPARE(qsBoundCardMac, qsBoundCardMacExpected);
}

void PrlVirtualNetworkTest::testBoundCardMacOnWrongParams()
{
	testCreateVirtualNetwork();

	QString qsStr = "some text";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetBoundCardMac(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetBoundCardMac(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetBoundCardMac(m_VirtNetHandle, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetBoundCardMac(m_VirtNetHandle, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlVirtNet_SetBoundCardMac(m_VirtNetHandle, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetBoundCardMac(m_VirtNetHandle, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
}

void PrlVirtualNetworkTest::testAdapterName()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	virt_net.getHostOnlyNetwork()->getParallelsAdapter()->setName("adapter 1");
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	QString qsAdapterName = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsAdapterName, m_VirtNetHandle, PrlVirtNet_GetAdapterName);
	QVERIFY(qsAdapterName == "adapter 1");

	QString qsAdapterNameExpected = "text for test";
	CHECK_RET_CODE_EXP(PrlVirtNet_SetAdapterName(m_VirtNetHandle, QSTR2UTF8(qsAdapterNameExpected)));
	PRL_EXTRACT_STRING_VALUE(qsAdapterName, m_VirtNetHandle, PrlVirtNet_GetAdapterName);

	QCOMPARE(qsAdapterName, qsAdapterNameExpected);
}

void PrlVirtualNetworkTest::testAdapterNameOnWrongParams()
{
	testCreateVirtualNetwork();

	QString qsStr = "some text";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetAdapterName(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetAdapterName(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetAdapterName(m_VirtNetHandle, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetAdapterName(m_VirtNetHandle, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlVirtNet_SetAdapterName(m_VirtNetHandle, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetAdapterName(m_VirtNetHandle, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
}

void PrlVirtualNetworkTest::testAdapterIndex()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	virt_net.getHostOnlyNetwork()->getParallelsAdapter()->setPrlAdapterIndex(20);
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	PRL_UINT32 nAdapterIndex = 10;
	CHECK_RET_CODE_EXP(PrlVirtNet_GetAdapterIndex(m_VirtNetHandle, &nAdapterIndex));
	QVERIFY(nAdapterIndex == 20);

	PRL_UINT32 nAdapterIndexExpected = 15;
	CHECK_RET_CODE_EXP(PrlVirtNet_SetAdapterIndex(m_VirtNetHandle, nAdapterIndexExpected));
	CHECK_RET_CODE_EXP(PrlVirtNet_GetAdapterIndex(m_VirtNetHandle, &nAdapterIndex));
	QCOMPARE(nAdapterIndex, nAdapterIndexExpected);
}

void PrlVirtualNetworkTest::testAdapterIndexOnWrongParams()
{
	testCreateVirtualNetwork();

	PRL_UINT32 nAdapterIndex = 0;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetAdapterIndex(m_ServerHandle, &nAdapterIndex),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetAdapterIndex(m_ServerHandle, nAdapterIndex),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetAdapterIndex(m_VirtNetHandle, NULL),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testHostIPAddress()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	virt_net.getHostOnlyNetwork()->setHostIPAddress(QHostAddress("10.20.30.40"));
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	QString qsHostIPAddress = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsHostIPAddress, m_VirtNetHandle, PrlVirtNet_GetHostIPAddress);
	QVERIFY(qsHostIPAddress == "10.20.30.40");

	QString qsHostIPAddressExpected = "192.168.2.3";
	CHECK_RET_CODE_EXP(PrlVirtNet_SetHostIPAddress(m_VirtNetHandle, QSTR2UTF8(qsHostIPAddressExpected)));
	PRL_EXTRACT_STRING_VALUE(qsHostIPAddress, m_VirtNetHandle, PrlVirtNet_GetHostIPAddress);

	QCOMPARE(qsHostIPAddress, qsHostIPAddressExpected);
}

void PrlVirtualNetworkTest::testHostIPAddressOnWrongParams()
{
	testCreateVirtualNetwork();

	QString qsStr = "192.168.2.3";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetHostIPAddress(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetHostIPAddress(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetHostIPAddress(m_VirtNetHandle, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetHostIPAddress(m_VirtNetHandle, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlVirtNet_SetHostIPAddress(m_VirtNetHandle, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetHostIPAddress(m_VirtNetHandle, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
	// Incorrect IP address
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetHostIPAddress(m_VirtNetHandle, "333.444.555.666"),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testHostIP6Address()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	virt_net.getHostOnlyNetwork()->setHostIP6Address(QHostAddress("::A:B:C:D"));
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	QString qsHostIP6Address = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsHostIP6Address, m_VirtNetHandle, PrlVirtNet_GetHostIP6Address);
	QVERIFY(qsHostIP6Address == "::A:B:C:D");

	QString qsHostIP6AddressExpected = "ABCD::1234";
	CHECK_RET_CODE_EXP(PrlVirtNet_SetHostIP6Address(m_VirtNetHandle, QSTR2UTF8(qsHostIP6AddressExpected)));
	PRL_EXTRACT_STRING_VALUE(qsHostIP6Address, m_VirtNetHandle, PrlVirtNet_GetHostIP6Address);

	QCOMPARE(qsHostIP6Address, qsHostIP6AddressExpected);
}

void PrlVirtualNetworkTest::testHostIP6AddressOnWrongParams()
{
	testCreateVirtualNetwork();

	QString qsStr = "ABCD::1234";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetHostIP6Address(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetHostIP6Address(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetHostIP6Address(m_VirtNetHandle, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetHostIP6Address(m_VirtNetHandle, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlVirtNet_SetHostIP6Address(m_VirtNetHandle, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetHostIP6Address(m_VirtNetHandle, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
	// Incorrect IP address
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetHostIP6Address(m_VirtNetHandle, "vxyz::4321"),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testDhcpIPAddress()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	virt_net.getHostOnlyNetwork()->setDhcpIPAddress(QHostAddress("11.22.33.44"));
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	QString qsDhcpIPAddress = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsDhcpIPAddress, m_VirtNetHandle, PrlVirtNet_GetDhcpIPAddress);
	QVERIFY(qsDhcpIPAddress == "11.22.33.44");

	QString qsDhcpIPAddressExpected = "192.168.2.3";
	CHECK_RET_CODE_EXP(PrlVirtNet_SetDhcpIPAddress(m_VirtNetHandle, QSTR2UTF8(qsDhcpIPAddressExpected)));
	PRL_EXTRACT_STRING_VALUE(qsDhcpIPAddress, m_VirtNetHandle, PrlVirtNet_GetDhcpIPAddress);

	QCOMPARE(qsDhcpIPAddress, qsDhcpIPAddressExpected);
}

void PrlVirtualNetworkTest::testDhcpIPAddressOnWrongParams()
{
	testCreateVirtualNetwork();

	QString qsStr = "192.168.2.3";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetDhcpIPAddress(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetDhcpIPAddress(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetDhcpIPAddress(m_VirtNetHandle, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetDhcpIPAddress(m_VirtNetHandle, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlVirtNet_SetDhcpIPAddress(m_VirtNetHandle, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetDhcpIPAddress(m_VirtNetHandle, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
	// Incorrect IP address
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetDhcpIPAddress(m_VirtNetHandle, "333.444.555.666"),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testDhcpIP6Address()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	virt_net.getHostOnlyNetwork()->setDhcpIP6Address(QHostAddress("::BB:CC:DD:EE"));
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	QString qsDhcpIP6Address = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsDhcpIP6Address, m_VirtNetHandle, PrlVirtNet_GetDhcpIP6Address);
	QVERIFY(qsDhcpIP6Address == "::BB:CC:DD:EE");

	QString qsDhcpIP6AddressExpected = "ABCD::1234";
	CHECK_RET_CODE_EXP(PrlVirtNet_SetDhcpIP6Address(m_VirtNetHandle, QSTR2UTF8(qsDhcpIP6AddressExpected)));
	PRL_EXTRACT_STRING_VALUE(qsDhcpIP6Address, m_VirtNetHandle, PrlVirtNet_GetDhcpIP6Address);

	QCOMPARE(qsDhcpIP6Address, qsDhcpIP6AddressExpected);
}

void PrlVirtualNetworkTest::testDhcpIP6AddressOnWrongParams()
{
	testCreateVirtualNetwork();

	QString qsStr = "ABCD::1234";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetDhcpIP6Address(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetDhcpIP6Address(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetDhcpIP6Address(m_VirtNetHandle, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetDhcpIP6Address(m_VirtNetHandle, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlVirtNet_SetDhcpIP6Address(m_VirtNetHandle, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetDhcpIP6Address(m_VirtNetHandle, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
	// Incorrect IP address
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetDhcpIP6Address(m_VirtNetHandle, "vxyz::4321"),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testIPNetMask()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	virt_net.getHostOnlyNetwork()->setIPNetMask(QHostAddress("255.255.128.0"));
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	QString qsIPNetMask = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsIPNetMask, m_VirtNetHandle, PrlVirtNet_GetIPNetMask);
	QVERIFY(qsIPNetMask == "255.255.128.0");

	QString qsIPNetMaskExpected = "255.254.0.0";
	CHECK_RET_CODE_EXP(PrlVirtNet_SetIPNetMask(m_VirtNetHandle, QSTR2UTF8(qsIPNetMaskExpected)));
	PRL_EXTRACT_STRING_VALUE(qsIPNetMask, m_VirtNetHandle, PrlVirtNet_GetIPNetMask);

	QCOMPARE(qsIPNetMask, qsIPNetMaskExpected);
}

void PrlVirtualNetworkTest::testIPNetMaskOnWrongParams()
{
	testCreateVirtualNetwork();

	QString qsStr = "255.255.255.252";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetIPNetMask(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetIPNetMask(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetIPNetMask(m_VirtNetHandle, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetIPNetMask(m_VirtNetHandle, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlVirtNet_SetIPNetMask(m_VirtNetHandle, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetIPNetMask(m_VirtNetHandle, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
	// Incorrect IP address
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetIPNetMask(m_VirtNetHandle, "333.444.555.666"),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testIP6NetMask()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	virt_net.getHostOnlyNetwork()->setIP6NetMask(QHostAddress("FFFF:F800::"));
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	QString qsIP6NetMask = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsIP6NetMask, m_VirtNetHandle, PrlVirtNet_GetIP6NetMask);
	QVERIFY(qsIP6NetMask == "FFFF:F800::");

	QString qsIP6NetMaskExpected = "FFFF:FFFF:8000::";
	CHECK_RET_CODE_EXP(PrlVirtNet_SetIP6NetMask(m_VirtNetHandle, QSTR2UTF8(qsIP6NetMaskExpected)));
	PRL_EXTRACT_STRING_VALUE(qsIP6NetMask, m_VirtNetHandle, PrlVirtNet_GetIP6NetMask);

	QCOMPARE(qsIP6NetMask, qsIP6NetMaskExpected);
}

void PrlVirtualNetworkTest::testIP6NetMaskOnWrongParams()
{
	testCreateVirtualNetwork();

	QString qsStr = "FFFF:FFFF:8000::";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetIP6NetMask(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetIP6NetMask(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetIP6NetMask(m_VirtNetHandle, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetIP6NetMask(m_VirtNetHandle, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlVirtNet_SetIP6NetMask(m_VirtNetHandle, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetIP6NetMask(m_VirtNetHandle, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
	// Incorrect IP address
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetIP6NetMask(m_VirtNetHandle, "vxyz::4321"),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testVlanTag()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;

	PRL_UINT16 nExpectedVLANTag = 23545;
	virt_net.setVLANTag(nExpectedVLANTag);
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	PRL_UINT16 nActualVLANTag = 0;;
	CHECK_RET_CODE_EXP(PrlVirtNet_GetVlanTag(m_VirtNetHandle, &nActualVLANTag))
	QCOMPARE(quint32(nActualVLANTag), quint32(nExpectedVLANTag));

	nExpectedVLANTag = 23434;
	CHECK_RET_CODE_EXP(PrlVirtNet_SetVlanTag(m_VirtNetHandle, nExpectedVLANTag));
	CHECK_RET_CODE_EXP(PrlVirtNet_GetVlanTag(m_VirtNetHandle, &nActualVLANTag))
	QCOMPARE(quint32(nActualVLANTag), quint32(nExpectedVLANTag));
}

void PrlVirtualNetworkTest::testVlanTagOnWrongParams()
{
	testCreateVirtualNetwork();

	PRL_UINT16 nActualVLANTag = 0;

	// Null handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetVlanTag(PRL_INVALID_HANDLE, &nActualVLANTag),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetVlanTag(PRL_INVALID_HANDLE, nActualVLANTag),
									PRL_ERR_INVALID_ARG);
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetVlanTag(m_ServerHandle, &nActualVLANTag),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetVlanTag(m_ServerHandle, nActualVLANTag),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetVlanTag(m_VirtNetHandle, 0),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testIPScopeStart()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	virt_net.getHostOnlyNetwork()->getDHCPServer()->setIPScopeStart(QHostAddress("10.10.0.1"));
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	QString qsIPScopeStart = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsIPScopeStart, m_VirtNetHandle, PrlVirtNet_GetIPScopeStart);
	QVERIFY(qsIPScopeStart == "10.10.0.1");

	QString qsIPScopeStartExpected = "192.168.2.3";
	CHECK_RET_CODE_EXP(PrlVirtNet_SetIPScopeStart(m_VirtNetHandle, QSTR2UTF8(qsIPScopeStartExpected)));
	PRL_EXTRACT_STRING_VALUE(qsIPScopeStart, m_VirtNetHandle, PrlVirtNet_GetIPScopeStart);

	QCOMPARE(qsIPScopeStart, qsIPScopeStartExpected);
}

void PrlVirtualNetworkTest::testIPScopeStartOnWrongParams()
{
	testCreateVirtualNetwork();

	QString qsStr = "192.168.2.3";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetIPScopeStart(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetIPScopeStart(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetIPScopeStart(m_VirtNetHandle, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetIPScopeStart(m_VirtNetHandle, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlVirtNet_SetIPScopeStart(m_VirtNetHandle, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetIPScopeStart(m_VirtNetHandle, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
	// Incorrect IP address
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetIPScopeStart(m_VirtNetHandle, "333.444.555.666"),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testIPScopeEnd()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	virt_net.getHostOnlyNetwork()->getDHCPServer()->setIPScopeEnd(QHostAddress("10.10.0.254"));
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	QString qsIPScopeEnd = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsIPScopeEnd, m_VirtNetHandle, PrlVirtNet_GetIPScopeEnd);
	QVERIFY(qsIPScopeEnd == "10.10.0.254");

	QString qsIPScopeEndExpected = "192.168.2.3";
	CHECK_RET_CODE_EXP(PrlVirtNet_SetIPScopeEnd(m_VirtNetHandle, QSTR2UTF8(qsIPScopeEndExpected)));
	PRL_EXTRACT_STRING_VALUE(qsIPScopeEnd, m_VirtNetHandle, PrlVirtNet_GetIPScopeEnd);

	QCOMPARE(qsIPScopeEnd, qsIPScopeEndExpected);
}

void PrlVirtualNetworkTest::testIPScopeEndOnWrongParams()
{
	testCreateVirtualNetwork();

	QString qsStr = "192.168.2.3";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetIPScopeEnd(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetIPScopeEnd(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetIPScopeEnd(m_VirtNetHandle, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetIPScopeEnd(m_VirtNetHandle, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlVirtNet_SetIPScopeEnd(m_VirtNetHandle, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetIPScopeEnd(m_VirtNetHandle, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
	// Incorrect IP address
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetIPScopeEnd(m_VirtNetHandle, "333.444.555.666"),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testIP6ScopeStart()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	virt_net.getHostOnlyNetwork()->getDHCPv6Server()->setIPScopeStart(QHostAddress("::A:A:1"));
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	QString qsIP6ScopeStart = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsIP6ScopeStart, m_VirtNetHandle, PrlVirtNet_GetIP6ScopeStart);
	QVERIFY(qsIP6ScopeStart == "::A:A:1");

	QString qsIP6ScopeStartExpected = "ABCD::1234";
	CHECK_RET_CODE_EXP(PrlVirtNet_SetIP6ScopeStart(m_VirtNetHandle, QSTR2UTF8(qsIP6ScopeStartExpected)));
	PRL_EXTRACT_STRING_VALUE(qsIP6ScopeStart, m_VirtNetHandle, PrlVirtNet_GetIP6ScopeStart);

	QCOMPARE(qsIP6ScopeStart, qsIP6ScopeStartExpected);
}

void PrlVirtualNetworkTest::testIP6ScopeStartOnWrongParams()
{
	testCreateVirtualNetwork();

	QString qsStr = "ABCD::1234";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetIP6ScopeStart(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetIP6ScopeStart(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetIP6ScopeStart(m_VirtNetHandle, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetIP6ScopeStart(m_VirtNetHandle, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlVirtNet_SetIP6ScopeStart(m_VirtNetHandle, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetIP6ScopeStart(m_VirtNetHandle, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
	// Incorrect IP address
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetIP6ScopeStart(m_VirtNetHandle, "vxyz::4321"),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testIP6ScopeEnd()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	virt_net.getHostOnlyNetwork()->getDHCPv6Server()->setIPScopeEnd(QHostAddress("::A:A:FE"));
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	QString qsIP6ScopeEnd = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsIP6ScopeEnd, m_VirtNetHandle, PrlVirtNet_GetIP6ScopeEnd);
	QVERIFY(qsIP6ScopeEnd == "::A:A:FE");

	QString qsIP6ScopeEndExpected = "ABCD::1234";
	CHECK_RET_CODE_EXP(PrlVirtNet_SetIP6ScopeEnd(m_VirtNetHandle, QSTR2UTF8(qsIP6ScopeEndExpected)));
	PRL_EXTRACT_STRING_VALUE(qsIP6ScopeEnd, m_VirtNetHandle, PrlVirtNet_GetIP6ScopeEnd);

	QCOMPARE(qsIP6ScopeEnd, qsIP6ScopeEndExpected);
}

void PrlVirtualNetworkTest::testIP6ScopeEndOnWrongParams()
{
	testCreateVirtualNetwork();

	QString qsStr = "ABCD::1234";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetIP6ScopeEnd(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetIP6ScopeEnd(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetIP6ScopeEnd(m_VirtNetHandle, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetIP6ScopeEnd(m_VirtNetHandle, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlVirtNet_SetIP6ScopeEnd(m_VirtNetHandle, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetIP6ScopeEnd(m_VirtNetHandle, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
	// Incorrect IP address
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetIP6ScopeEnd(m_VirtNetHandle, "vxyz::4321"),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testEnabled()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	QVERIFY( virt_net.isEnabled() );
	virt_net.setEnabled(false);
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	PRL_BOOL bEnabled = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlVirtNet_IsEnabled(m_VirtNetHandle, &bEnabled));
	QVERIFY(bEnabled == PRL_FALSE);

	PRL_BOOL bEnabledExpected = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlVirtNet_SetEnabled(m_VirtNetHandle, bEnabledExpected));
	CHECK_RET_CODE_EXP(PrlVirtNet_IsEnabled(m_VirtNetHandle, &bEnabled));
	QCOMPARE(bEnabled, bEnabledExpected);
}

void PrlVirtualNetworkTest::testEnabledOnWrongParams()
{
	testCreateVirtualNetwork();

	PRL_BOOL bEnabled = PRL_TRUE;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_IsEnabled(m_ServerHandle, &bEnabled),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetEnabled(m_ServerHandle, bEnabled),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_IsEnabled(m_VirtNetHandle, NULL),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testAdapterEnabled()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	virt_net.getHostOnlyNetwork()->getParallelsAdapter()->setEnabled(true);
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	PRL_BOOL bEnabled = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlVirtNet_IsAdapterEnabled(m_VirtNetHandle, &bEnabled));
	QVERIFY(bEnabled == PRL_TRUE);

	PRL_BOOL bEnabledExpected = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlVirtNet_SetAdapterEnabled(m_VirtNetHandle, bEnabledExpected));
	CHECK_RET_CODE_EXP(PrlVirtNet_IsAdapterEnabled(m_VirtNetHandle, &bEnabled));
	QCOMPARE(bEnabled, bEnabledExpected);
}

void PrlVirtualNetworkTest::testAdapterEnabledOnWrongParams()
{
	testCreateVirtualNetwork();

	PRL_BOOL bEnabled = PRL_TRUE;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_IsAdapterEnabled(m_ServerHandle, &bEnabled),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetAdapterEnabled(m_ServerHandle, bEnabled),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_IsAdapterEnabled(m_VirtNetHandle, NULL),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testDHCPServerEnabled()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	QVERIFY( ! virt_net.getHostOnlyNetwork()->getDHCPServer()->isEnabled() );
	virt_net.getHostOnlyNetwork()->getDHCPServer()->setEnabled(true);
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	PRL_BOOL bEnabled = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlVirtNet_IsDHCPServerEnabled(m_VirtNetHandle, &bEnabled));
	QVERIFY(bEnabled == PRL_TRUE);

	PRL_BOOL bEnabledExpected = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlVirtNet_SetDHCPServerEnabled(m_VirtNetHandle, bEnabledExpected));
	CHECK_RET_CODE_EXP(PrlVirtNet_IsDHCPServerEnabled(m_VirtNetHandle, &bEnabled));
	QCOMPARE(bEnabled, bEnabledExpected);
}

void PrlVirtualNetworkTest::testDHCPServerEnabledOnWrongParams()
{
	testCreateVirtualNetwork();

	PRL_BOOL bEnabled = PRL_TRUE;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_IsDHCPServerEnabled(m_ServerHandle, &bEnabled),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetDHCPServerEnabled(m_ServerHandle, bEnabled),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_IsDHCPServerEnabled(m_VirtNetHandle, NULL),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testDHCP6ServerEnabled()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	QVERIFY( ! virt_net.getHostOnlyNetwork()->getDHCPv6Server()->isEnabled() );
	virt_net.getHostOnlyNetwork()->getDHCPv6Server()->setEnabled(true);
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	PRL_BOOL bEnabled = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlVirtNet_IsDHCP6ServerEnabled(m_VirtNetHandle, &bEnabled));
	QVERIFY(bEnabled == PRL_TRUE);

	PRL_BOOL bEnabledExpected = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlVirtNet_SetDHCP6ServerEnabled(m_VirtNetHandle, bEnabledExpected));
	CHECK_RET_CODE_EXP(PrlVirtNet_IsDHCP6ServerEnabled(m_VirtNetHandle, &bEnabled));
	QCOMPARE(bEnabled, bEnabledExpected);
}

void PrlVirtualNetworkTest::testDHCP6ServerEnabledOnWrongParams()
{
	testCreateVirtualNetwork();

	PRL_BOOL bEnabled = PRL_TRUE;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_IsDHCP6ServerEnabled(m_ServerHandle, &bEnabled),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetDHCP6ServerEnabled(m_ServerHandle, bEnabled),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_IsDHCP6ServerEnabled(m_VirtNetHandle, NULL),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testNATServerEnabled()
{
	testCreateVirtualNetwork();

	VIRTUAL_NETWORK_TO_XML_OBJECT;
	QVERIFY( ! virt_net.getHostOnlyNetwork()->getNATServer()->isEnabled() );
	virt_net.getHostOnlyNetwork()->getNATServer()->setEnabled(true);
	VIRTUAL_NETWORK_FROM_XML_OBJECT;

	PRL_BOOL bEnabled = PRL_FALSE;
	CHECK_RET_CODE_EXP(PrlVirtNet_IsNATServerEnabled(m_VirtNetHandle, &bEnabled));
	QVERIFY(bEnabled == PRL_TRUE);

	PRL_BOOL bEnabledExpected = PRL_TRUE;
	CHECK_RET_CODE_EXP(PrlVirtNet_SetNATServerEnabled(m_VirtNetHandle, bEnabledExpected));
	CHECK_RET_CODE_EXP(PrlVirtNet_IsNATServerEnabled(m_VirtNetHandle, &bEnabled));
	QCOMPARE(bEnabled, bEnabledExpected);
}

void PrlVirtualNetworkTest::testNATServerEnabledOnWrongParams()
{
	testCreateVirtualNetwork();

	PRL_BOOL bEnabled = PRL_TRUE;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_IsNATServerEnabled(m_ServerHandle, &bEnabled),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetNATServerEnabled(m_ServerHandle, bEnabled),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_IsNATServerEnabled(m_VirtNetHandle, NULL),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testCreatePortForwardEntry()
{
	SdkHandleWrap hPortFwd;
	CHECK_RET_CODE_EXP(PrlPortFwd_Create(hPortFwd.GetHandlePtr()));
	CHECK_HANDLE_TYPE(hPortFwd, PHT_PORT_FORWARDING);
}

void PrlVirtualNetworkTest::testCreatePortForwardEntryOnWrongParams()
{
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_Create(NULL), PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testTcpPortForwardList()
{
	testCreateVirtualNetwork();

	SdkHandleWrap hPortFwdList;

// Empty list by default

	CHECK_RET_CODE_EXP(PrlVirtNet_GetPortForwardList(m_VirtNetHandle, PPF_TCP, hPortFwdList.GetHandlePtr()));
	PRL_UINT32 nCount = 0;
	CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hPortFwdList, &nCount));
	QVERIFY( ! nCount );

// Set not empty list

	for(int i = 0; i < 5; i++)
	{
		SdkHandleWrap hPortFwd;
		CHECK_RET_CODE_EXP(PrlPortFwd_Create(hPortFwd.GetHandlePtr()));
		CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hPortFwdList, hPortFwd));
	}
	CHECK_RET_CODE_EXP(PrlVirtNet_SetPortForwardList(m_VirtNetHandle, PPF_TCP, hPortFwdList));
	CHECK_RET_CODE_EXP(PrlVirtNet_GetPortForwardList(m_VirtNetHandle, PPF_TCP, hPortFwdList.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hPortFwdList, &nCount));
	QVERIFY( nCount == 5 );
	{
		VIRTUAL_NETWORK_TO_XML_OBJECT;
		QVERIFY(virt_net.getHostOnlyNetwork()->getNATServer()->getPortForwarding()->getTCP()
					->m_lstForwardEntry.size() == 5);
	}

// Remove entries

	CHECK_RET_CODE_EXP(PrlHndlList_RemoveItem(hPortFwdList, 0));
	CHECK_RET_CODE_EXP(PrlHndlList_RemoveItem(hPortFwdList, 0));
	CHECK_RET_CODE_EXP(PrlVirtNet_SetPortForwardList(m_VirtNetHandle, PPF_TCP, hPortFwdList));
	CHECK_RET_CODE_EXP(PrlVirtNet_GetPortForwardList(m_VirtNetHandle, PPF_TCP, hPortFwdList.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hPortFwdList, &nCount));
	QVERIFY( nCount == 3 );
	{
		VIRTUAL_NETWORK_TO_XML_OBJECT;
		QVERIFY(virt_net.getHostOnlyNetwork()->getNATServer()->getPortForwarding()->getTCP()
					->m_lstForwardEntry.size() == 3);
	}

// Check UDP list

	CHECK_RET_CODE_EXP(PrlVirtNet_GetPortForwardList(m_VirtNetHandle, PPF_UDP, hPortFwdList.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hPortFwdList, &nCount));
	QVERIFY( ! nCount );

// Invalid handles list as empty list

	hPortFwdList.reset(PRL_INVALID_HANDLE);
	CHECK_RET_CODE_EXP(PrlVirtNet_SetPortForwardList(m_VirtNetHandle, PPF_TCP, hPortFwdList));
	CHECK_RET_CODE_EXP(PrlVirtNet_GetPortForwardList(m_VirtNetHandle, PPF_TCP, hPortFwdList.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hPortFwdList, &nCount));
	QVERIFY( ! nCount );
}

void PrlVirtualNetworkTest::testUdpPortForwardList()
{
	testCreateVirtualNetwork();

	SdkHandleWrap hPortFwdList;

// Empty list by default

	CHECK_RET_CODE_EXP(PrlVirtNet_GetPortForwardList(m_VirtNetHandle, PPF_UDP, hPortFwdList.GetHandlePtr()));
	PRL_UINT32 nCount = 0;
	CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hPortFwdList, &nCount));
	QVERIFY( ! nCount );

// Set not empty list

	for(int i = 0; i < 5; i++)
	{
		SdkHandleWrap hPortFwd;
		CHECK_RET_CODE_EXP(PrlPortFwd_Create(hPortFwd.GetHandlePtr()));
		CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hPortFwdList, hPortFwd));
	}
	CHECK_RET_CODE_EXP(PrlVirtNet_SetPortForwardList(m_VirtNetHandle, PPF_UDP, hPortFwdList));
	CHECK_RET_CODE_EXP(PrlVirtNet_GetPortForwardList(m_VirtNetHandle, PPF_UDP, hPortFwdList.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hPortFwdList, &nCount));
	QVERIFY( nCount == 5 );
	{
		VIRTUAL_NETWORK_TO_XML_OBJECT;
		QVERIFY(virt_net.getHostOnlyNetwork()->getNATServer()->getPortForwarding()->getUDP()
					->m_lstForwardEntry.size() == 5);
	}

// Remove entries

	CHECK_RET_CODE_EXP(PrlHndlList_RemoveItem(hPortFwdList, 0));
	CHECK_RET_CODE_EXP(PrlHndlList_RemoveItem(hPortFwdList, 0));
	CHECK_RET_CODE_EXP(PrlVirtNet_SetPortForwardList(m_VirtNetHandle, PPF_UDP, hPortFwdList));
	CHECK_RET_CODE_EXP(PrlVirtNet_GetPortForwardList(m_VirtNetHandle, PPF_UDP, hPortFwdList.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hPortFwdList, &nCount));
	QVERIFY( nCount == 3 );
	{
		VIRTUAL_NETWORK_TO_XML_OBJECT;
		QVERIFY(virt_net.getHostOnlyNetwork()->getNATServer()->getPortForwarding()->getUDP()
					->m_lstForwardEntry.size() == 3);
	}

// Check TCP list

	CHECK_RET_CODE_EXP(PrlVirtNet_GetPortForwardList(m_VirtNetHandle, PPF_TCP, hPortFwdList.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hPortFwdList, &nCount));
	QVERIFY( ! nCount );

// Invalid handles list as empty list

	hPortFwdList.reset(PRL_INVALID_HANDLE);
	CHECK_RET_CODE_EXP(PrlVirtNet_SetPortForwardList(m_VirtNetHandle, PPF_UDP, hPortFwdList));
	CHECK_RET_CODE_EXP(PrlVirtNet_GetPortForwardList(m_VirtNetHandle, PPF_UDP, hPortFwdList.GetHandlePtr()));
	CHECK_RET_CODE_EXP(PrlHndlList_GetItemsCount(hPortFwdList, &nCount));
	QVERIFY( ! nCount );
}

void PrlVirtualNetworkTest::testPortForwardListOnWrongParams()
{
	testCreateVirtualNetwork();

	SdkHandleWrap hPortFwdList;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetPortForwardList(
						m_ServerHandle, PPF_TCP, hPortFwdList.GetHandlePtr()),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetPortForwardList(m_ServerHandle, PPF_UDP, hPortFwdList),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetPortForwardList(m_VirtNetHandle, PPF_TCP, m_ServerHandle),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetPortForwardList(m_VirtNetHandle, PPF_UDP, NULL),
									PRL_ERR_INVALID_ARG);
	// Wrong port forwarding type
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetPortForwardList(
						m_VirtNetHandle, (PRL_PORT_FORWARDING_TYPE )-1, hPortFwdList.GetHandlePtr()),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetPortForwardList(
						m_VirtNetHandle, (PRL_PORT_FORWARDING_TYPE )2, hPortFwdList.GetHandlePtr()),
									PRL_ERR_INVALID_ARG);

	CHECK_RET_CODE_EXP(PrlApi_CreateHandlesList(hPortFwdList.GetHandlePtr()));

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetPortForwardList(
										m_VirtNetHandle, (PRL_PORT_FORWARDING_TYPE )-1, hPortFwdList),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetPortForwardList(
										m_VirtNetHandle, (PRL_PORT_FORWARDING_TYPE )2, hPortFwdList),
									PRL_ERR_INVALID_ARG);
	// Wrong handles list
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hPortFwdList, m_ServerHandle));
	CHECK_RET_CODE_EXP(PrlHndlList_AddItem(hPortFwdList, m_VirtNetHandle));
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetPortForwardList(m_VirtNetHandle, PPF_TCP, hPortFwdList),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_SetPortForwardList(m_VirtNetHandle, PPF_UDP, hPortFwdList),
									PRL_ERR_INVALID_ARG);
}

#define PORT_FORWARDING_TO_XML_OBJECT \
	PRL_VOID_PTR pXml = 0; \
	CHECK_RET_CODE_EXP(PrlHandle_ToString(hPortFwd, &pXml)); \
	CPortForwardEntry port_fwd; \
	port_fwd.fromString( UTF8_2QSTR((PRL_CONST_STR )pXml) ); \
	PrlBuffer_Free(pXml);

#define PORT_FORWARDING_FROM_XML_OBJECT \
	CHECK_RET_CODE_EXP(PrlHandle_FromString( hPortFwd, QSTR2UTF8(port_fwd.toString()) ));

void PrlVirtualNetworkTest::testRuleName()
{
	SdkHandleWrap hPortFwd;
	CHECK_RET_CODE_EXP(PrlPortFwd_Create(hPortFwd.GetHandlePtr()));

	PORT_FORWARDING_TO_XML_OBJECT;
	port_fwd.setRuleName("rule name");
	PORT_FORWARDING_FROM_XML_OBJECT;

	QString qsRuleName = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsRuleName, hPortFwd, PrlPortFwd_GetRuleName);
	QVERIFY(qsRuleName == "rule name");

	QString qsRuleNameExpected = "another rule name";
	CHECK_RET_CODE_EXP(PrlPortFwd_SetRuleName(hPortFwd, QSTR2UTF8(qsRuleNameExpected)));
	PRL_EXTRACT_STRING_VALUE(qsRuleName, hPortFwd, PrlPortFwd_GetRuleName);

	QCOMPARE(qsRuleName, qsRuleNameExpected);
}

void PrlVirtualNetworkTest::testRuleNameOnWrongParams()
{
	SdkHandleWrap hPortFwd;
	CHECK_RET_CODE_EXP(PrlPortFwd_Create(hPortFwd.GetHandlePtr()));

	QString qsStr = "any rule name";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_GetRuleName(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_SetRuleName(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_GetRuleName(hPortFwd, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_SetRuleName(hPortFwd, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlPortFwd_SetRuleName(hPortFwd, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_GetRuleName(hPortFwd, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
}

void PrlVirtualNetworkTest::testIncomingPort()
{
	SdkHandleWrap hPortFwd;
	CHECK_RET_CODE_EXP(PrlPortFwd_Create(hPortFwd.GetHandlePtr()));

	PORT_FORWARDING_TO_XML_OBJECT;
	port_fwd.setIncomingPort(7);
	PORT_FORWARDING_FROM_XML_OBJECT;

	PRL_UINT16 nIncomingPort = 10;
	CHECK_RET_CODE_EXP(PrlPortFwd_GetIncomingPort(hPortFwd, &nIncomingPort));
	QVERIFY(nIncomingPort == 7);

	PRL_UINT16 nIncomingPortExpected = 15;
	CHECK_RET_CODE_EXP(PrlPortFwd_SetIncomingPort(hPortFwd, nIncomingPortExpected));
	CHECK_RET_CODE_EXP(PrlPortFwd_GetIncomingPort(hPortFwd, &nIncomingPort));
	QCOMPARE(nIncomingPort, nIncomingPortExpected);
}

void PrlVirtualNetworkTest::testIncomingPortOnWrongParams()
{
	SdkHandleWrap hPortFwd;
	CHECK_RET_CODE_EXP(PrlPortFwd_Create(hPortFwd.GetHandlePtr()));

	PRL_UINT16 nIncomingPort = 0;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_GetIncomingPort(m_ServerHandle, &nIncomingPort),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_SetIncomingPort(m_ServerHandle, nIncomingPort),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_GetIncomingPort(hPortFwd, NULL),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testRedirectIPAddress()
{
	SdkHandleWrap hPortFwd;
	CHECK_RET_CODE_EXP(PrlPortFwd_Create(hPortFwd.GetHandlePtr()));

	PORT_FORWARDING_TO_XML_OBJECT;
	port_fwd.setRedirectIp(QHostAddress("90.80.70.60"));
	PORT_FORWARDING_FROM_XML_OBJECT;

	QString qsRedirectIPAddress = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsRedirectIPAddress, hPortFwd, PrlPortFwd_GetRedirectIPAddress);
	QVERIFY(qsRedirectIPAddress == "90.80.70.60");

	QString qsRedirectIPAddressExpected = "192.168.2.3";
	CHECK_RET_CODE_EXP(PrlPortFwd_SetRedirectIPAddress(hPortFwd, QSTR2UTF8(qsRedirectIPAddressExpected)));
	PRL_EXTRACT_STRING_VALUE(qsRedirectIPAddress, hPortFwd, PrlPortFwd_GetRedirectIPAddress);

	QCOMPARE(qsRedirectIPAddress, qsRedirectIPAddressExpected);
}

void PrlVirtualNetworkTest::testRedirectIPAddressOnWrongParams()
{
	SdkHandleWrap hPortFwd;
	CHECK_RET_CODE_EXP(PrlPortFwd_Create(hPortFwd.GetHandlePtr()));

	QString qsStr = "192.168.2.3";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_GetRedirectIPAddress(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_SetRedirectIPAddress(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_GetRedirectIPAddress(hPortFwd, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_SetRedirectIPAddress(hPortFwd, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlPortFwd_SetRedirectIPAddress(hPortFwd, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_GetRedirectIPAddress(hPortFwd, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
	// Incorrect IP address
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_SetRedirectIPAddress(hPortFwd, "333.444.555.666"),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testRedirectPort()
{
	SdkHandleWrap hPortFwd;
	CHECK_RET_CODE_EXP(PrlPortFwd_Create(hPortFwd.GetHandlePtr()));

	PORT_FORWARDING_TO_XML_OBJECT;
	port_fwd.setRedirectPort(23);
	PORT_FORWARDING_FROM_XML_OBJECT;

	PRL_UINT16 nRedirectPort = 10;
	CHECK_RET_CODE_EXP(PrlPortFwd_GetRedirectPort(hPortFwd, &nRedirectPort));
	QVERIFY(nRedirectPort == 23);

	PRL_UINT16 nRedirectPortExpected = 15;
	CHECK_RET_CODE_EXP(PrlPortFwd_SetRedirectPort(hPortFwd, nRedirectPortExpected));
	CHECK_RET_CODE_EXP(PrlPortFwd_GetRedirectPort(hPortFwd, &nRedirectPort));
	QCOMPARE(nRedirectPort, nRedirectPortExpected);
}

void PrlVirtualNetworkTest::testRedirectPortOnWrongParams()
{
	SdkHandleWrap hPortFwd;
	CHECK_RET_CODE_EXP(PrlPortFwd_Create(hPortFwd.GetHandlePtr()));

	PRL_UINT16 nRedirectPort = 0;
	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_GetRedirectPort(m_ServerHandle, &nRedirectPort),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_SetRedirectPort(m_ServerHandle, nRedirectPort),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_GetRedirectPort(hPortFwd, NULL),
									PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testRedirectVm()
{
	SdkHandleWrap hPortFwd;
	CHECK_RET_CODE_EXP(PrlPortFwd_Create(hPortFwd.GetHandlePtr()));

	PORT_FORWARDING_TO_XML_OBJECT;
	port_fwd.setRedirectVm("vm id");
	PORT_FORWARDING_FROM_XML_OBJECT;

	QString qsRedirectVm = "not empty";
	PRL_EXTRACT_STRING_VALUE(qsRedirectVm, hPortFwd, PrlPortFwd_GetRedirectVm);
	QVERIFY(qsRedirectVm == "vm id");

	QString qsRedirectVmExpected = "another vm id";
	CHECK_RET_CODE_EXP(PrlPortFwd_SetRedirectVm(hPortFwd, QSTR2UTF8(qsRedirectVmExpected)));
	PRL_EXTRACT_STRING_VALUE(qsRedirectVm, hPortFwd, PrlPortFwd_GetRedirectVm);

	QCOMPARE(qsRedirectVm, qsRedirectVmExpected);
}

void PrlVirtualNetworkTest::testRedirectVmOnWrongParams()
{
	SdkHandleWrap hPortFwd;
	CHECK_RET_CODE_EXP(PrlPortFwd_Create(hPortFwd.GetHandlePtr()));

	QString qsStr = "vm -id";
	PRL_UINT32 nSize = 0;

	// Wrong handle
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_GetRedirectVm(m_ServerHandle, 0, &nSize),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_SetRedirectVm(m_ServerHandle, QSTR2UTF8(qsStr)),
									PRL_ERR_INVALID_ARG);
	// Null pointer
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_GetRedirectVm(hPortFwd, 0, 0),
									PRL_ERR_INVALID_ARG);
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_SetRedirectVm(hPortFwd, 0),
									PRL_ERR_INVALID_ARG);
	// Buffer overrun
	CHECK_RET_CODE_EXP(PrlPortFwd_SetRedirectVm(hPortFwd, QSTR2UTF8(qsStr)));
	nSize = 4;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlPortFwd_GetRedirectVm(hPortFwd, (PRL_STR )QSTR2UTF8(qsStr), &nSize),
									PRL_ERR_BUFFER_OVERRUN);
}

void PrlVirtualNetworkTest::testGetVirtualNetworkList()
{
	testLoginLocal();

	m_JobHandle.reset(PrlSrv_GetVirtualNetworkList(m_ServerHandle, 0));
	CHECK_JOB_RET_CODE(m_JobHandle);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()))

	PRL_UINT32 nCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nCount))

	for(PRL_UINT32 i = 0; i < nCount; ++i)
	{
		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, i, m_VirtNetHandle.GetHandlePtr()));
		CHECK_HANDLE_TYPE(m_VirtNetHandle, PHT_VIRTUAL_NETWORK);
	}
}

void PrlVirtualNetworkTest::testGetVirtualNetworkListOnWrongParams()
{
	testCreateVirtualNetwork();

	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlSrv_GetVirtualNetworkList(m_VirtNetHandle, 0), PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testAddVirtualNetworkOnDuplicateNetworkId()
{
	testLoginLocal();

	m_JobHandle.reset(PrlSrv_GetVirtualNetworkList(m_ServerHandle, 0));
	CHECK_JOB_RET_CODE(m_JobHandle);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()))

	PRL_UINT32 nCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nCount))

	if (nCount > 0)
	{
		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, m_VirtNetHandle.GetHandlePtr()));

		CHECK_ASYNC_OP_FAILED(PrlSrv_AddVirtualNetwork(m_ServerHandle, m_VirtNetHandle, 0),
								PRL_NET_DUPLICATE_VIRTUAL_NETWORK_ID);
	}
	else
	{
		QSKIP("No one virtual network presents on this host for this test!", SkipAll);
	}
}

void PrlVirtualNetworkTest::testAddVirtualNetworkOnWrongParams()
{
	testCreateVirtualNetwork();

	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlSrv_AddVirtualNetwork(m_VirtNetHandle, m_VirtNetHandle, 0),
						PRL_ERR_INVALID_ARG);
	CHECK_ASYNC_OP_FAILED(PrlSrv_AddVirtualNetwork(m_ServerHandle, m_ServerHandle, 0),
						PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testUpdateVirtualNetworkOnDuplicateNetworkId()
{
	testLoginLocal();

	m_JobHandle.reset(PrlSrv_GetVirtualNetworkList(m_ServerHandle, 0));
	CHECK_JOB_RET_CODE(m_JobHandle);

	SdkHandleWrap hResult;
	CHECK_RET_CODE_EXP(PrlJob_GetResult(m_JobHandle, hResult.GetHandlePtr()))

	PRL_UINT32 nCount = 0;
	CHECK_RET_CODE_EXP(PrlResult_GetParamsCount(hResult, &nCount))

	if (nCount > 1)
	{
		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 0, m_VirtNetHandle.GetHandlePtr()));

		QString qsNewNetworkId;
		PRL_EXTRACT_STRING_VALUE(qsNewNetworkId, m_VirtNetHandle, PrlVirtNet_GetNetworkId);

		CHECK_RET_CODE_EXP(PrlResult_GetParamByIndex(hResult, 1, m_VirtNetHandle.GetHandlePtr()));

		CHECK_RET_CODE_EXP(PrlVirtNet_SetNetworkId(m_VirtNetHandle, QSTR2UTF8(qsNewNetworkId)));

		CHECK_ASYNC_OP_FAILED(PrlSrv_UpdateVirtualNetwork(m_ServerHandle, m_VirtNetHandle, 0),
								PRL_NET_DUPLICATE_VIRTUAL_NETWORK_ID);
	}
	else
	{
		QSKIP("Two virtual networks are need for this test!", SkipAll);
	}
}

void PrlVirtualNetworkTest::testUpdateVirtualNetworkOnWrongParams()
{
	testCreateVirtualNetwork();

	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlSrv_UpdateVirtualNetwork(m_VirtNetHandle, m_VirtNetHandle, 0),
						PRL_ERR_INVALID_ARG);
	CHECK_ASYNC_OP_FAILED(PrlSrv_UpdateVirtualNetwork(m_ServerHandle, m_ServerHandle, 0),
						PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testDeleteVirtualNetworkOnNotExistingNetworkId()
{
	testLoginLocal();

	testCreateVirtualNetwork();

	CHECK_RET_CODE_EXP(PrlVirtNet_SetNetworkId(m_VirtNetHandle, "not existing network ID"));

	CHECK_ASYNC_OP_FAILED(PrlSrv_DeleteVirtualNetwork(m_ServerHandle, m_VirtNetHandle, 0),
							PRL_NET_VIRTUAL_NETWORK_ID_NOT_EXISTS);
}

void PrlVirtualNetworkTest::testDeleteVirtualNetworkOnWrongParams()
{
	testCreateVirtualNetwork();

	// Wrong handle
	CHECK_ASYNC_OP_FAILED(PrlSrv_DeleteVirtualNetwork(m_VirtNetHandle, m_VirtNetHandle, 0),
						PRL_ERR_INVALID_ARG);
	CHECK_ASYNC_OP_FAILED(PrlSrv_DeleteVirtualNetwork(m_ServerHandle, m_ServerHandle, 0),
						PRL_ERR_INVALID_ARG);
}

void PrlVirtualNetworkTest::testGetBoundAdapterInfo()
{
	testCreateVirtualNetwork();
	testLoginLocal();

	SdkHandleWrap hJob, hResult, hHostHwInfo;

	const QString sExpectedMac = TEST_MAC_ADDR;
	const QString sExpectedSysName = "eth0";
	const PRL_UINT16 nExpectedVlanTag = 34344;

	CHECK_RET_CODE_EXP(PrlVirtNet_SetBoundCardMac(m_VirtNetHandle, QSTR2UTF8(sExpectedMac)))
	CHECK_RET_CODE_EXP(PrlVirtNet_SetVlanTag(m_VirtNetHandle, nExpectedVlanTag))

	CHostHardwareInfo _host_info;
	for (PRL_UINT16 i = 0; i < 10; ++i)
		_host_info.m_lstNetworkAdapters.append(
			new CHwNetAdapter(
				PDE_GENERIC_NETWORK_ADAPTER,
				QString("Network adapter %1").arg(i),
				QString("eth%1").arg(i),
				i,
				sExpectedMac,
				nExpectedVlanTag+i,
				true
			)
		);

	hJob.reset(PrlSrv_GetSrvConfig(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob)
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hHostHwInfo.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlHandle_FromString(hHostHwInfo, QSTR2UTF8(_host_info.toString())))

	SdkHandleWrap hNetAdapter;
	CHECK_RET_CODE_EXP(PrlVirtNet_GetBoundAdapterInfo(m_VirtNetHandle, hHostHwInfo, hNetAdapter.GetHandlePtr()))
	CHECK_HANDLE_TYPE(hNetAdapter, PHT_HW_NET_ADAPTER)

	QString sActualMac, sActualSysName;
	PRL_EXTRACT_STRING_VALUE(sActualMac, hNetAdapter, PrlSrvCfgNet_GetMacAddress)
	PRL_EXTRACT_STRING_VALUE(sActualSysName, hNetAdapter, PrlSrvCfgDev_GetId)

	PRL_UINT16 nActualVlanTag = 0;
	CHECK_RET_CODE_EXP(PrlSrvCfgNet_GetVlanTag(hNetAdapter, &nActualVlanTag))

	QCOMPARE(sActualSysName, sExpectedSysName);
	QCOMPARE(sActualMac, sExpectedMac);
	QCOMPARE(quint32(nActualVlanTag), quint32(nExpectedVlanTag));
}

void PrlVirtualNetworkTest::testGetBoundAdapterInfoOnWrongParams()
{
	testCreateVirtualNetwork();
	testLoginLocal();

	SdkHandleWrap hJob, hResult, hHostHwInfo, hNetAdapter;
	hJob.reset(PrlSrv_GetSrvConfig(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob)
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hHostHwInfo.GetHandlePtr()))

	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetBoundAdapterInfo(PRL_INVALID_HANDLE, hHostHwInfo, hNetAdapter.GetHandlePtr()), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetBoundAdapterInfo(m_ServerHandle, hHostHwInfo, hNetAdapter.GetHandlePtr()), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetBoundAdapterInfo(m_VirtNetHandle, PRL_INVALID_HANDLE, hNetAdapter.GetHandlePtr()), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetBoundAdapterInfo(m_VirtNetHandle, m_ServerHandle, hNetAdapter.GetHandlePtr()), PRL_ERR_INVALID_ARG)
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetBoundAdapterInfo(m_VirtNetHandle, hHostHwInfo, NULL), PRL_ERR_INVALID_ARG)
}

void PrlVirtualNetworkTest::testGetBoundAdapterInfoOnNoSuchAdapterAtHostHwInfo()
{
	testCreateVirtualNetwork();
	testLoginLocal();

	SdkHandleWrap hJob, hResult, hHostHwInfo;

	const QString sExpectedMac = TEST_MAC_ADDR;
	const PRL_UINT16 nExpectedVlanTag = 34344;

	CHECK_RET_CODE_EXP(PrlVirtNet_SetBoundCardMac(m_VirtNetHandle, QSTR2UTF8(sExpectedMac)))
	CHECK_RET_CODE_EXP(PrlVirtNet_SetVlanTag(m_VirtNetHandle, nExpectedVlanTag))

	CHostHardwareInfo _host_info;
	for (PRL_UINT16 i = 1; i < 10; ++i)
		_host_info.m_lstNetworkAdapters.append(
			new CHwNetAdapter(
				PDE_GENERIC_NETWORK_ADAPTER,
				QString("Network adapter %1").arg(i),
				QString("eth%1").arg(i),
				i,
				sExpectedMac,
				nExpectedVlanTag+i,
				true
			)
		);

	hJob.reset(PrlSrv_GetSrvConfig(m_ServerHandle));
	CHECK_JOB_RET_CODE(hJob)
	CHECK_RET_CODE_EXP(PrlJob_GetResult(hJob, hResult.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlResult_GetParam(hResult, hHostHwInfo.GetHandlePtr()))
	CHECK_RET_CODE_EXP(PrlHandle_FromString(hHostHwInfo, QSTR2UTF8(_host_info.toString())))

	SdkHandleWrap hNetAdapter;
	CHECK_CONCRETE_EXPRESSION_RET_CODE(PrlVirtNet_GetBoundAdapterInfo(m_VirtNetHandle, hHostHwInfo, hNetAdapter.GetHandlePtr()), PRL_ERR_NETWORK_ADAPTER_NOT_FOUND)
}

