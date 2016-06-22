/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlVmValidateConfigTest.h
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing VM manipulating SDK API.
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
#ifndef PrlVmValidateConfigTest_H
#define PrlVmValidateConfigTest_H

#include <QtTest/QtTest>

#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"

class PrlVmValidateConfigTest : public QObject
{
Q_OBJECT

private slots:
	void init();
	void cleanup();
	void testCreateVmFromConfig();
	void testValidateConfigGeneralParametersOnValid();
	void testValidateConfigGeneralParametersOnEmptyVmName();
	void testValidateConfigGeneralParametersOnInvalidSymbolInVmName();
	void testValidateConfigGeneralParametersOnValidOs();
	void testValidateConfigGeneralParametersOnInvalidOsType();
	void testValidateConfigGeneralParametersOnNoSmartGuardWithBootCamp();
	void testValidateConfigGeneralParametersOnNoSmartGuardWithIncompatDisks();
	void testValidateConfigGeneralParametersOnVideoNotEnabled();
	void testValidateConfigGeneralParametersOnNoAutoCompressWithSmartGuard();
	void testValidateConfigBootOptionOnValid();
	void testValidateConfigBootOptionOnInvalidType();
	void testValidateConfigBootOptionOnDuplicateDevice();
	void testValidateConfigBootOptionOnDeviceNotExists();
	void testValidateConfigRemoteDisplayOnValid();
	void testValidateConfigRemoteDisplayOnZeroPortValue();
	void testValidateConfigRemoteDisplayOnInvalidIpAddress();
	void testValidateConfigRemoteDisplayOnEmptyPassword();
	void testValidateConfigSharedFoldersOnValid();
	void testValidateConfigSharedFoldersOnEmptyFolderName();
	void testValidateConfigSharedFoldersOnDuplicateFolderName();
	void testValidateConfigSharedFoldersOnInvalidFolderPath();
	void testValidateConfigSharedFoldersOnDuplicateFolderPath();
	void testValidateConfigSharedFoldersOnNotUserDefinedFolders();
	void testValidateConfigCpuOnValid();
	void testValidateConfigCpuOnZeroCount();
	void testValidateConfigCpuOnInvalidCount();
	void testValidateConfigMainMemoryOnValid();
	void testValidateConfigMainMemoryOnZero();
	void testValidateConfigMainMemoryOnOutOfRange();
	void testValidateConfigMainMemoryOnRatio4();
	void testValidateConfigMainMemoryOnSizeNotEqualRecommended();
	void testValidateConfigMainMemoryOnMemQuotaSettings();
	void testValidateConfigMainMemoryOnMemQuotaTryToEdit();
	void testValidateConfigVideoMemoryOnValid();
	void testValidateConfigVideoMemoryOnLess2();
	void testValidateConfigVideoMemoryOnGreaterMax();
	void testValidateConfigFloppyDiskOnValid();
	void testValidateConfigFloppyDiskOnEmptySysName();
	void testValidateConfigFloppyDiskOnInvalidSymbolInSysName();
	void testValidateConfigFloppyDiskOnNotExistImageFile();
	void testValidateConfigFloppyDiskOnInvalidImageFile();
	void testValidateConfigFloppyDiskOnRealFloppyDisk();
	void testValidateConfigFloppyDiskOnUrlSysName();
	void testValidateConfigCdDvdRomOnValid();
	void testValidateConfigCdDvdRomOnEmptySysName();
	void testValidateConfigCdDvdRomOnDuplicateStackIndex();
	void testValidateConfigCdDvdRomOnDuplicateSysName();
	void testValidateConfigCdDvdRomOnNotExistImageFile();
	void testValidateConfigCdDvdRomOnUrlSysName();
	void testValidateConfigHardDiskOnValid();
	void testValidateConfigHardDiskOnEmptySysName();
	void testValidateConfigHardDiskOnInvalidSymbolInSysName();
	void testValidateConfigHardDiskOnSysNameAsHddImageIsInvalid();
	void testValidateConfigHardDiskOnDuplicateStackIndex();
	void testValidateConfigHardDiskOnDuplicateSysName();
	void testValidateConfigHardDiskOnUrlSysName();
	void testValidateConfigNetworkAdapterOnValid();
	void testValidateConfigNetworkAdapterOnInvalidAdapter();
	void testValidateConfigNetworkAdapterOnDefaultSysIndex();
	void testValidateConfigNetworkAdapterOnInvalidMacAddress();
	void testValidateConfigNetworkAdapterOnDuplicateMacAddress();
	void testValidateConfigNetworkAdapterOnDuplicateIpAddress();
	void testValidateConfigNetworkAdapterOnInvalidIpAddress();
	void testValidateConfigNetworkAdapterOnMulticastAddress();
	void testValidateConfigNetworkAdapterOnBroadcastAddress();
	void testValidateConfigNetworkAdapterOnInvalidGatewayIpAddress();
	void testValidateConfigNetworkAdapterOnInvalidGatewayIpAddress2();
	void testValidateConfigNetworkAdapterOnGatewayNotInAnySubnet();
	void testValidateConfigNetworkAdapterOnInvalidDnsIpAddress();
	void testValidateConfigNetworkAdapterOnInvalidSearchDomainName();
	void testValidateConfigSerialPortOnValid();
	void testValidateConfigSerialPortOnEmptySysName();
	void testValidateConfigSerialPortOnInvalidSymbolInSysName();
	void testValidateConfigSerialPortOnNotExistOutputFile();
	void testValidateConfigSerialPortOnUrlSysName();
	void testValidateConfigParallelPortOnValid();
	void testValidateConfigParallelPortOnEmptySysName();
	void testValidateConfigParallelPortOnInvalidSymbolInSysName();
	void testValidateConfigParallelPortOnNotExistOutputFile();
	void testValidateConfigParallelPortOnUrlSysName();
	void testValidateConfigIdeDevicesOnValid();
	void testValidateConfigIdeDevicesOnCountOutOfRange();
	void testValidateConfigIdeDevicesOnDuplicateStackIndex();
	void testValidateConfigScsiDevicesOnValid();
	void testValidateConfigScsiDevicesOnCountOutOfRange();
	void testValidateConfigScsiDevicesOnDuplicateStackIndex();
	void testValidateConfigPciGenericDeviceOnCannotAddDevice();
	void testValidateConfigPciVideoAdapterOnCannotAddDevice();
	void testValidateConfigPciNetworkAdapterOnCannotAddDevice();
	void testValidateConfigPciGenericDeviceOnNotExistOrConnect();
	void testValidateConfigPciVideoAdapterOnNotExistOrConnect();
	void testValidateConfigPciNetworkAdapterOnNotExistOrConnect();
	void testValidateConfigPciGenericDevicesOnDuplicateInAnotherVm();
	void testValidateConfigPciVideoAdaptersOnDuplicateInAnotherVm();
	void testValidateConfigPciNetworkAdaptersOnDuplicateInAnotherVm();
	void testValidateConfigPciGenericDevicesOnWrongDevice();
	void testValidateConfigPciVideoAdaptersOnWrongDevice();
	void testValidateConfigPciNetworkAdaptersOnWrongDevice();
	void testValidateConfigPciGenericDevicesOnDuplicateSysName();
	void testValidateConfigPciVideoAdaptersOnNotSingle();
	void testValidateConfigPciNetworkAdaptersOnDuplicateSysName();
	void testValidateConfigOnDuplicateMainDeviceIndex();
	void testValidateConfigWithRemoteDevicesInDesktopAndServerMode();
	void testValidateConfigOnAllInvalid();
	void testWorkWithComplexError();

private:
	bool IsSharedNetworkingTypeEnabled();
	void CheckWhetherSharedModeEnabled( bool &bSharedModeEnabled );

private:
	SdkHandleWrap	m_ServerHandle;
	SdkHandleWrap	m_JobHandle;
	SdkHandleWrap	m_VmHandle;
	SdkHandleWrap	m_ResultHandle;
	QString			m_qsFile;
	QList<SdkHandleWrap >	m_lstVmHandles;

};


#endif	/* PrlVmValidateConfigTest_H */
