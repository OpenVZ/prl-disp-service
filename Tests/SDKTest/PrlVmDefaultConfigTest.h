/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlVmDefaultConfigTest.h
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
#ifndef PrlVmDefaultConfigTest_H
#define PrlVmDefaultConfigTest_H

#include <QtTest/QtTest>

#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"

class PrlVmDefaultConfigTest : public QObject
{
Q_OBJECT

private slots:
	void init();
	void cleanup();
	void testCheckDefaultConfigurationValues();
	void testSetDefaultConfigCheckAllDevicesWinVista();
	void testSetDefaultConfigForParallelPortDeviceFreeBSD();
	void testSetDefaultConfigForParallelPortDeviceOtherLinux();
	void testSetDefaultConfigForUsbControllerDeviceOtherLinux();
	void testSetDefaultConfigForSerailPortDeviceOS2();
	void testSetDefaultConfigForSerailPortDeviceEcs();
	void testAddDefaultDeviceForSoundDeviceWinVista();
	void testAddDefaultDeviceForSoundDeviceWin311();
	void testAddDefaultDeviceForSoundDeviceMSDos622();
	void testAddDefaultDeviceForSoundDeviceMSDosOther();
	void testSetDefaultConfigCheckDefaultCpusNumber();
	void testSetDefaultConfigOnInvalidServerConfig1();
	void testSetDefaultConfigOnInvalidServerConfig2();
	void testSetDefaultConfigCheckShareMacOSXFoldersToWindowsSignSwitchedOn();
	void testSetDefaultConfig_CheckSharedCameraEnabled();
	void testSetDefaultConfig_CheckSharedCameraDisabled();
	void testSetDefaultConfigCheckDefaultRamSizeForWinOses();
	void testSetDefaultConfigCheckDefaultRamSizeForMacOses();
	void testSetDefaultConfigCheckDefaultRamSizeForLinOses();
	void testSetDefaultConfigCheckDefaultHddSizeForWinOses();
	void testSetDefaultConfigCheckDefaultHddSizeForMacOses();
	void testSetDefaultConfigCheckDefaultHddSizeForLinOses();
	void testSetDefaultConfigCheckSharedProfileEnabled();
	void testSetDefaultConfigPrinterOnNoServerConfig();
	void testSetDefaultConfigPrinterOnNoParallelPortsInSrvCfg();
	void testSetDefaultConfigPrinterOnRealParallelPortsInSrvCfg();
	void testSetDefaultConfigPrinterOnPrintersInSrvCfg();
	void testAddDefaultConfigPrinter();
	void testGetDefaultVideoRamSizeOnWrongPtr();
	void testGetDefaultVideoRamSizeOnNullSrvConfig();
	void testGetDefaultVideoRamSizeOnNonSrvConfig();
	void testSetDefaultConfigNotDroppingDevicesIfFalseSpecifiedForCreateDevicesFlag();
	void testCheckDefaultSATAHardDisk();
	void testAddDefaultDeviceWithReturnedDeviceHandle();
	void testAddDefaultDeviceExOnWrongParams();
	void testCheckDefaultBootOrder();
	void testShareGuestAppsWithHost();
	void testVirtualPrintersInfo();
	void testDefaultStickyMouseValue();
	void testScsiCdRomOnSubTypeValueLsiSpi();
	void testScsiHddOnSubTypeValueLsiSpi();
	void testScsiCdRomOnSubTypeValueLsiSas();
	void testScsiHddOnSubTypeValueLsiSas();
	void testAddDefaultCdRomForMacOsGuest();
	void testAddUsbToBoot();
	void testEnableHiResDrawing();
	void testUsbControllerDefaults();
	void testEnableEfiByDefaultForWindows2012();
	void testCreateDefaultHddForWindowsDistroWithoutSataSupport();
	void testCreateDefaultHddForWindowsDistroWithSataSupport();
	void testCreateDefaultHddForLinuxDistroWithoutSataSupport();
	void testCreateDefaultHddForLinuxDistroWithSataSupport();
	void testCreateDefaultAttachedBackupForWindowsDistroWithoutSataSupport();
	void testCreateDefaultAttachedBackupForWindowsDistroWithSataSupport();
	void testCreateDefaultAttachedBackupForLinuxDistroWithoutSataSupport();
	void testCreateDefaultAttachedBackupForLinuxDistroWithSataSupport();
	void testCreateDefaultHddForWindowsDistroWithoutSataSupportWithExistingHdd();
	void testCreateDefaultHddForWindowsDistroWithSataSupportWithExistingHdd();
	void testCreateDefaultHddForLinuxDistroWithoutSataSupportWithExistingHdd();
	void testCreateDefaultHddForLinuxDistroWithSataSupportWithExistingHdd();
	void testCreateDefaultAttachedBackupForLinuxDistroWithoutSataSupportWithExistingHdd();
	void testCreateDefaultAttachedBackupForLinuxDistroWithSataSupportWithExistingHdd();
	void testCreateDefaultHddForWindowsDistroWithoutSataSupportWithExistingAttachedBackup();
	void testCreateDefaultHddForWindowsDistroWithSataSupportWithExistingAttachedBackup();
	void testCreateDefaultHddForLinuxDistroWithoutSataSupportWithExistingAttachedBackup();
	void testCreateDefaultHddForLinuxDistroWithSataSupportWithExistingAttachedBackup();
	void testCreateDefaultAttachedBackupForWindowsDistroWithoutSataSupportWithExistingAttachedBackup();
	void testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingAttachedBackup();
	void testCreateDefaultAttachedBackupForLinuxDistroWithoutSataSupportWithExistingAttachedBackup();
	void testCreateDefaultAttachedBackupForLinuxDistroWithSataSupportWithExistingAttachedBackup();
	void testCreateDefaultAttachedBackupForWindowsDistroWithoutSataSupportWithExistingHddOnIde0();
	void testCreateDefaultAttachedBackupForWindowsDistroWithoutSataSupportWithExistingHddOnIde2();
	void testCreateDefaultAttachedBackupForWindowsDistroWithoutSataSupportWithExistingHddOnIdeMax();
	void testCreateDefaultAttachedBackupForWindowsDistroWithoutSataSupportWithExistingHddOnScsi0();
	void testCreateDefaultAttachedBackupForWindowsDistroWithoutSataSupportWithExistingHddOnScsi2();
	void testCreateDefaultAttachedBackupForWindowsDistroWithoutSataSupportWithExistingHddOnScsiMax();
	void testCreateDefaultAttachedBackupForWindowsDistroWithoutSataSupportWithExistingHddOnIde2AndScsi2();
	void testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnSata0();
	void testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnSata2();
	void testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnSataMax();
	void testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnScsi0();
	void testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnScsi2();
	void testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnScsiMax();
	void testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnIde0();
	void testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnIde2();
	void testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnIdeMax();
	void testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnIde2AndScsi2();
	void testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnIde2AndSata2();
	void testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnSata2AndScsi2();
	void testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnIde2AndSata2AndScsi2();
	void testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnDisabledScsi2();
	void testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnDisabledScsi2AndSata2();
	void testCreateDefaultAttachedBackupForWindowsDistroWithSataSupportWithExistingHddOnDisabledSata2();

private:
	SdkHandleWrap	m_ServerHandle;
	SdkHandleWrap	m_JobHandle;
	SdkHandleWrap	m_VmHandle;
	SdkHandleWrap	m_ResultHandle;
	QString			m_qsFile;

};


#endif	/* PrlVmDefaultConfigTest_H */
