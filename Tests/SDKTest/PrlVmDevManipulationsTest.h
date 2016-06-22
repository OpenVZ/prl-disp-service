/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlVmDevManipulationsTest.h
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing VM devices manipulating SDK API.
///
///	@author sandro
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
#ifndef PrlVmDevManipulationsTest_H
#define PrlVmDevManipulationsTest_H

#include <QtTest/QtTest>
#include <QList>

#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"

class PrlVmDevManipulationsTest : public QObject
{

Q_OBJECT

private slots:
	void init();
	void cleanup();
	void testGetDeviceList();
	void testGetDeviceCountInvalidVmHandle();
	void testGetDeviceCountNonVmHandle();
	void testGetDeviceCountInvalidPtr();
	void testGetDeviceListInvalidVmHandle();
	void testGetDeviceListNonVmHandle();
	void testGetDeviceListInvalidPtr();
	void testGetDeviceListInvalidPtr2();
	void testGetDeviceOnNonEnoughBuffer();
	void testGetDeviceOnLargerBuffer();
	void testCreateVmDev();
	void testCreateVmDevOnInvalidVmHandle();
	void testCreateVmDevOnNonVmHandle();
	void testCreateVmDevOnInvalidVmDevType();
	void testCreateVmDevOnInvalidPtr();
	void testGetDeviceIndex();
	void testSetDeviceIndex();
	void testCreateNewDevice();
	void testRemoveDeviceOnFloppy();
	void testRemoveDeviceOnHardDisk();
	void testRemoveDeviceOnOpticalDisk();
	void testRemoveDeviceOnSerialPort();
	void testRemoveDeviceOnParallelPort();
	void testRemoveDeviceOnUsbDevice();
	void testRemoveDeviceOnSound();
	void testRemoveDeviceOnNetworkAdapter();
	void testIsConnected();
	void testSetConnected();
	void testIsEnabled();
	void testSetEnabled();
	void testGetEmulatedType();
	void testSetEmulatedType();
	void testGetImagePath();
	void testGetImagePathNotEnoughBufSize();
	void testGetImagePathNullBufSize();
	void testSetImagePath();
	void testSetImagePathTryToSetEmptyStringValue();
	void testSetImagePathOnImageDeviceEmulatedType();
	void testGetIfaceType();
	void testSetIfaceType();
	void testGetSubType();
	void testSetSubType();
	void testSubTypeOnWrongParams();
	void testGetDeviceStackIndex();
	void testSetDeviceStackIndex();
	void testGetOutputFile();
	void testGetOutputFileNotEnoughBufSize();
	void testGetOutputFileNullBufSize();
	void testSetOutputFile();
	void testSetOutputFileTryToSetEmptyStringValue();
	void testHardDiskGetType();
	void testHardDiskSetType();
	void testHardDiskIsSplitted();
	void testHardDiskSetSplitted();
	void testHardDiskGetSize();
	void testHardDiskSetSize();
	void testHardDiskGetSizeOnDisk();
	void testIsPassthrough();
	void testSetPassthrough();
	void testNetGetBoundAdapterIndex();
	void testNetSetBoundAdapterIndex();
	void testNetGetBoundAdapterName();
	void testNetGetBoundAdapterNameNotEnoughBufSize();
	void testNetGetBoundAdapterNameNullBufSize();
	void testNetSetBoundAdapterName();
	void testNetSetBoundAdapterNameTryToSetEmptyStringValue();
	void testNetGetMacAddress();
	void testNetGetMacAddressNotEnoughBufSize();
	void testNetGetMacAddressNullBufSize();
	void testNetSetMacAddress();
	void testNetGetNetAddresses();
	void testNetGetNetAddressesOnNullPtr();
	void testNetGetNetAddressesOnNullNetAdapterHandle();
	void testNetGetNetAddressesOnNonNetAdapterHandle();
	void testSetNetAddresses();
	void testSetNetAddressesOnNullNetAdapterHandle();
	void testSetNetAddressesOnNonNetAdapterHandle();
	void testSetNetAddressesSetEmptyListAsNullListHandle();
	void testNetGetDnsServers();
	void testNetGetDnsServersOnNullPtr();
	void testNetGetDnsServersOnNullNetAdapterHandle();
	void testNetGetDnsServersOnNonNetAdapterHandle();
	void testSetDnsServers();
	void testSetDnsServersOnNullNetAdapterHandle();
	void testSetDnsServersOnNonNetAdapterHandle();
	void testSetDnsServersSetEmptyListAsNullListHandle();
	void testNetGetSearchDomains();
	void testNetGetSearchDomainsOnNullPtr();
	void testNetGetSearchDomainsOnNullNetAdapterHandle();
	void testNetGetSearchDomainsOnNonNetAdapterHandle();
	void testSetSearchDomains();
	void testSetSearchDomainsOnNullNetAdapterHandle();
	void testSetSearchDomainsOnNonNetAdapterHandle();
	void testSetSearchDomainsSetEmptyListAsNullListHandle();
	void testNetIsConfigureWithDhcp();
	void testNetSetConfigureWithDhcp();
	void testNetIsConfigureWithDhcpIPv6();
	void testNetIsConfigureWithDhcpIPv6OnWrongParams();
	void testNetSetConfigureWithDhcpIPv6();
	void testNetSetConfigureWithDhcpIPv6OnWrongParams();
	void testNetGetDefaultGateway();
	void testNetGetDefaultGatewayNotEnoughBufSize();
	void testNetGetDefaultGatewayNullBufSize();
	void testNetSetDefaultGateway();
	void testNetGetDefaultGatewayIPv6();
	void testNetGetDefaultGatewayIPv6OnWrongParams();
	void testNetSetDefaultGatewayIPv6();
	void testNetSetDefaultGatewayIPv6OnWrongParams();
	void testNetGetVirtualNetworkId();
	void testNetGetVirtualNetworkIdNotEnoughBufSize();
	void testNetGetVirtualNetworkIdNullBufSize();
	void testNetGetVirtualNetworkIdOnWrongParams();
	void testNetSetVirtualNetworkId();
	void testNetSetVirtualNetworkIdOnWrongParams();
	void testNetIsPktFilterPreventMacSpoof();
	void testNetSetPktFilterPreventMacSpoof();
	void testNetIsPktFilterPreventPromisc();
	void testNetSetPktFilterPreventPromisc();
	void testNetIsPktFilterPreventIpSpoof();
	void testNetSetPktFilterPreventIpSpoof();
	void testNetPktFilterOnWrongParams();
	void testUsbGetAutoconnectOption();
	void testUsbSetAutoconnectOption();
	void testSoundGetOutputDev();
	void testSoundGetOutputDevNotEnoughBufSize();
	void testSoundGetOutputDevNullBufSize();
	void testSoundSetOutputDev();
	void testSoundSetOutputDevTryToSetEmptyStringValue();
	void testSoundGetMixerDev();
	void testSoundGetMixerDevNotEnoughBufSize();
	void testSoundGetMixerDevNullBufSize();
	void testSoundSetMixerDev();
	void testSoundSetMixerDevTryToSetEmptyStringValue();
	void testSerialGetSocketMode();
	void testSerialSetSocketMode();
	void testVmDevsHandlesNonMadeInvalidOnVmFromString();
	void testVmGetFloppyDisksCount();
	void testVmGetFloppyDisk();
	void testVmDevsHandlesMadeInvalidOnRemoveFloppyDisk();
	void testVmGetHardDisksCount();
	void testVmGetHardDisk();
	void testVmDevsHandlesMadeInvalidOnRemoveHardDisk();
	void testVmGetOpticalDisksCount();
	void testVmGetOpticalDisk();
	void testVmDevsHandlesMadeInvalidOnRemoveOpticalDisk();
	void testVmGetParallelPortsCount();
	void testVmGetParallelPort();
	void testVmDevsHandlesMadeInvalidOnRemoveParallelPort();
	void testVmGetSerialPortsCount();
	void testVmGetSerialPort();
	void testVmDevsHandlesMadeInvalidOnRemoveSerialPort();
	void testVmGetSoundDevsCount();
	void testVmGetSoundDev();
	void testVmDevsHandlesMadeInvalidOnRemoveSoundDev();
	void testVmGetUsbDevicesCount();
	void testVmGetUsbDevice();
	void testVmDevsHandlesMadeInvalidOnRemoveUsbDevice();
	void testVmGetNetAdaptersCount();
	void testVmGetNetAdapter();
	void testVmDevsHandlesMadeInvalidOnRemoveNetAdapter();
	void testNetGenerateMacAddress();
	void testNetDevCreateMacAddressAutoGenerated();
	void testNetDevGetDublicateMacAddressNotChanged();
	void testGetSysName();
	void testSetSysName();
	void testIsRemote();
	void testSetRemote();
	void testGetFriendlyName();
	void testSetFriendlyName();
	void testCreateVmDevNotBoundToVm();
	void testCreateVmDevNotBoundToVmOnInvalidVmDevType();
	void testCreateVmDevNotBoundToVmOnInvalidPtr();
	void testCreateVmDevNotBoundToVmTryToCallAsyncMeths();
	void testCreateVmDevNotBoundToVmTryToCallRemove();
	void testCreateVmDevAutoIndexAssignedForHardDisks();
	void testCreateVmDevAutoIndexAssignedForOpticalDisks();
	void testCreateVmDevAutoIndexAssignedForFloppyDisks();
	void testCreateVmDevAutoIndexAssignedForSerialPorts();
	void testCreateVmDevAutoIndexAssignedForParallelPorts();
	void testCreateVmDevAutoIndexAssignedForNetworkAdapters();
	void testCreateVmDevAutoIndexAssignedOnInterruptedRangeForHardDisks();
	void testCreateVmDevAutoIndexAssignedOnInterruptedRangeForOpticalDisks();
	void testCreateVmDevAutoIndexAssignedOnInterruptedRangeForFloppyDisks();
	void testCreateVmDevAutoIndexAssignedOnInterruptedRangeForSerialPorts();
	void testCreateVmDevAutoIndexAssignedOnInterruptedRangeForParallelPorts();
	void testCreateVmDevAutoIndexAssignedOnInterruptedRangeForNetworkAdapters();
	void testSetDefaultStackIndexForIdeInterface();
	void testSetDefaultStackIndexForScsiInterface();
	void testSetDefaultStackIndexForSataInterface();
	void testSetDefaultStackIndexForIdeInterfaceOnInterruptedRange();
	void testSetDefaultStackIndexForScsiInterfaceOnInterruptedRange();
	void testSetDefaultStackIndexForSataInterfaceOnInterruptedRange();
	void testSetDefaultStackIndexForIdeInterfaceOnNoFreeSlots();
	void testSetDefaultStackIndexForScsiInterfaceOnNoFreeSlots();
	void testSetDefaultStackIndexForScsiInterfaceOnScsiController();
	void testSetDefaultStackIndexForSataInterfaceOnNoFreeSlots();
	void testSetDefaultStackIndexForIdeInterfaceOnNoFreeSlotsGenericScsiPresents();
	void testSetDefaultStackIndexForScsiInterfaceOnNoFreeSlotsGenericScsiPresents();
	void testSetDefaultStackIndexForSataInterfaceOnNoFreeSlotsGenericScsiPresents();
	void testSetDefaultStackIndexFailedOnNullVmDevHandle();
	void testSetDefaultStackIndexFailedOnNonVmDevHandle();
	void testSetDefaultStackIndexFailedOnNonHardOrOpticalDiskVmDevHandle();
	void testSetDefaultStackIndexFailedOnRemovedVmDevHandle();
	void testSetDefaultStackIndexOnVmDevHandleThatNotBoundToVm();
	void testRemoveDevAlsoRemovesCorrespondingBootDeviceOnFloppyDisk();
	void testRemoveDevAlsoRemovesCorrespondingBootDeviceOnHardDisk();
	void testRemoveDevAlsoRemovesCorrespondingBootDeviceOnOpticalDisk();
	void testRemoveNetAdapterDevNotRemovesNetworkBootDevice();
	void testRemoveDevOnMixedDevicesTypes();
	void testGetDeviceTypeOnValid();
	void testGetDeviceTypeOnInvalidArguments();
	void testCreateVmDeviceByType();
	void testCreateVmDeviceOnInvalidArguments();
	void testCheckValidRetCodesOnRemovedOrInvalidVmDeviceOnFloppy();
	void testCheckValidRetCodesOnRemovedOrInvalidVmDeviceOnHardDisk();
	void testCheckValidRetCodesOnRemovedOrInvalidVmDeviceOnOpticalDisk();
	void testCheckValidRetCodesOnRemovedOrInvalidVmDeviceOnParallelPort();
	void testCheckValidRetCodesOnRemovedOrInvalidVmDeviceOnSerialPort();
	void testCheckValidRetCodesOnRemovedOrInvalidVmDeviceOnNetAdapter();
	void testCheckValidRetCodesOnRemovedOrInvalidVmDeviceOnSound();
	void testCheckValidRetCodesOnRemovedOrInvalidVmDeviceOnUsbController();
	void testHardDiskPartitionOnAdd();
	void testHardDiskPartitionOnAddWrongParams();
	void testHardDiskPartitionOnGetPartitionsCount();
	void testHardDiskPartitionOnGetPartitionsCountWrongParams();
	void testHardDiskPartitionOnGetPartition();
	void testHardDiskPartitionOnGetPartitionWrongParams();
	void testHardDiskPartitionOnRemove();
	void testHardDiskPartitionOnRemoveWrongParams();
	void testHardDiskPartitionOnGetSysName();
	void testHardDiskPartitionOnGetSysNameWrongParams();
	void testHardDiskPartitionOnSetSysName();
	void testHardDiskPartitionOnSetSysNameWrongParams();
	void testHardDiskPartitionOnRemovedHardDisk();
	void testAutoAssignStackIndexOnIfaceSettingForIdeInterface();
	void testAutoAssignStackIndexOnIfaceSettingForScsiInterface();
	void testAutoAssignStackIndexOnIfaceSettingForIdeInterfaceOnInterruptedRange();
	void testAutoAssignStackIndexOnIfaceSettingForScsiInterfaceOnInterruptedRange();
	void testSetIfaceTypeOnIfaceSettingFailedOnNullVmDevHandle();
	void testSetIfaceTypeOnIfaceSettingFailedOnNonVmDevHandle();
	void testSetIfaceTypeOnIfaceSettingFailedOnNonHardOrOpticalDiskVmDevHandle();
	void testSetIfaceTypeOnIfaceSettingFailedOnRemovedVmDevHandle();
	void testSetIfaceTypeOnIfaceSettingOnVmDevHandleThatNotBoundToVm();
	void testCreateGenericPciDevice();
	void testCreateGenericScsiDevice();
	void testSetDefaultStackIndexForGenericPci();
	void testSetDefaultStackIndexForGenericPciNoMoreFreeSlots();
	void testGetIfaceTypeOnGenericScsi();
	void testSetIfaceTypeOnGenericScsiWithAttemptToApplyIdeIface();
	void testGetDevsListForGenericPci();
	void testGetDevsListForGenericScsi();
	void testGetDevByTypeForGenericPci();
	void testGetDevByTypeForGenericScsi();
	void testGetDescription();
	void testGetDescriptionNotEnoughBufSize();
	void testGetDescriptionNullBufSize();
	void testSetDescription();
	void testSetGetDescriptionForAllDevices();
	void testCreateVmDeviceDisplay();
	void testRemoveDeviceOnDisplay();
	void testCreateVmDeviceDisplayOnCommonDeviceList();
	void testGetDisplayDeviceByType();
	void testGetDisplayDevices();
	void testGetDisplayDevicesOnWrongParams();
	void testAddDefaultDeviceExOnIndexOrder();
	void testNetIsFirewallEnabled();
	void testNetIsFirewallEnabledOnWrongParams();
	void testNetSetFirewallEnabled();
	void testNetSetFirewallEnabledOnWrongParams();
	void testNetGetFirewallDefaultPolicy();
	void testNetGetFirewallDefaultPolicyOnWrongParams();
	void testNetSetFirewallDefaultPolicy();
	void testNetSetFirewallDefaultPolicyOnWrongParams();
	void testCreateFirewallRuleEntry();
	void testCreateFirewallRuleEntryOnWrongParams();
	void testFirewallRuleList();
	void testFirewallRuleListOnWrongParams();
	void testFirewallRuleLocalPort();
	void testFirewallRuleLocalPortOnWrongParams();
	void testFirewallRuleRemotePort();
	void testFirewallRuleRemotePortOnWrongParams();
	void testFirewallRuleProtocol();
	void testFirewallRuleProtocolOnWrongParams();
	void testFirewallRuleLocalNetAddress();
	void testFirewallRuleLocalNetAddressOnWrongParams();
	void testFirewallRuleRemoteNetAddress();
	void testFirewallRuleRemoteNetAddressOnWrongParams();
	void testCopyImage();
	void testCopyImageOnWrongParams();
	void testHardDiskGetStorageURL();
	void testHardDiskSetStorageURL();

private:
	SdkHandleWrap m_ServerHandle;
	SdkHandleWrap m_VmHandle;
	QList<SdkHandleWrap> m_VmDevHandles;
	PRL_HANDLE_PTR m_pDevicesBuf;
	QString m_qsTestPath;
	QString m_qsTestPath2;

private:
	bool CheckDevicesCount(int nExpectedCount, PRL_HANDLE_TYPE _dev_type, PRL_UINT32 nAllCount);
	void testFirewallDirectionRuleList(PRL_FIREWALL_DIRECTION nDirection);
	void clearTestPaths();
};

#endif
