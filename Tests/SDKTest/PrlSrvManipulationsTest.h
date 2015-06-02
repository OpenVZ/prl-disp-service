/////////////////////////////////////////////////////////////////////////////
///
///	@file PrlSrvManipulationsTest.h
///
///	This file is the part of parallels public SDK library tests suite.
///	Tests fixture class for testing server login SDK API.
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
#ifndef PrlSrvManipulationsTest_H
#define PrlSrvManipulationsTest_H

#include <QtTest/QtTest>
#include "SDK/Wrappers/SdkWrap/SdkHandleWrap.h"

class PrlSrvManipulationsTest : public QObject
{

Q_OBJECT

private slots:
	void init();
	void cleanup();
	void testLogin();
	void testLoginOnIncorrectHostname();
	void testLoginOnInvalidHandle();
	void testLoginLocal();
	void testLoginLocalOnInvalidHandle();
	void testLoginLocalOnNonServerHandle();
	void testLogoffOnInvalidHandle();
	void testGetSrvConfig();
	void testGetSrvConfigOnInvalidServerHandle();
	void testGetSrvConfigOnNonServerHandle();
	void testFsGetDiskList();
	void testFsGetDiskListOnInvalidServerHandle();
	void testFsGetDiskListOnNonServerHandle();
	void testFsGetDirEntries();
	void testFsGetDirEntriesOnInvalidServerHandle();
	void testFsGetDirEntriesOnInvalidPointer();
	void testFsGetDirEntriesOnNonServerHandle();
	void testFsCreateDirOnInvalidServerHandle();
	void testFsCreateDirOnInvalidPointer();
	void testFsCreateDirOnNonServerHandle();
	void testFsRemoveEntryOnInvalidServerHandle();
	void testFsRemoveEntryOnInvalidPointer();
	void testFsRemoveEntryOnNonServerHandle();
	void testFsRenameEntryOnInvalidServerHandle();
	void testFsRenameEntryOnInvalidPointer();
	void testFsRenameEntryOnInvalidPointer2();
	void testFsRenameEntryOnNonServerHandle();
	void testGetUserProfile();
	void testGetUserProfileOnInvalidServerHandle();
	void testGetUserProfileOnNonServerHandle();
	void testEditUserProfile();
	void testEditUserProfileTryToChangeUseManagementConsoleFlag();
	void testEditUserProfileTryToChangeCanChangeServerSettingsFlag();
	void testEditUserProfileTryToChangeVmDirectoryUuid();
	void testGetCommonPrefs();
	void testEditCommonPrefs();
	void testGetResultAsHandleForHostHwInfoCmd();
	void testUserProfileGetDefaultVmFolder();
	void testUserProfileGetDefaultVmFolderNotEnoughBufSize();
	void testUserProfileGetDefaultVmFolderNullBufSize();
	void testDispConfigGetDefaulCtDir();
	void testUserProfileSetDefaultVmFolder();
	void testUserProfileCanUseMngConsole();
	void testUserProfileCanChangeSrvSets();
	void testUserProfileIsLocalAdministrator();
	void testUserProfileIsLocalAdministratorOnWrongParams();
	void testDispConfigGetDefaultVmDir();
	void testDispConfigGetDefaultVmDirNotEnoughBufSize();
	void testDispConfigGetDefaultVmDirNullBufSize();
	void testDispConfigGetReservedMemLimit();
	void testDispConfigSetReservedMemLimit();
	void testDispConfigGetMinVmMem();
	void testDispConfigSetMinVmMem();
	void testDispConfigGetMaxVmMem();
	void testDispConfigSetMaxVmMem();
	void testDispConfigGetRecommendMinVmMem();
	void testDispConfigGetRecommendMinVmMemInvalid();
	void testDispConfigGetRecommendMaxVmMem();
	void testDispConfigSetRecommendMaxVmMem();
	void testDispConfigGetMaxReservMemLimit();
	void testDispConfigSetMaxReservMemLimit();
	void testDispConfigGetMinReservMemLimit();
	void testDispConfigSetMinReservMemLimit();
	void testDispConfigIsAdjustMemAuto();
	void testDispConfigSetAdjustMemAuto();
	void testDispConfigGetConfirmationsList();
	void testDispConfigSetConfirmationsList();
	void testDispConfigIsSendStatisticReport();
	void testDispConfigIsSendStatisticReportOnWrongParams();
	void testDispConfigSetSendStatisticReport();
	void testDispConfigSetSendStatisticReportOnWrongParams();
	void testIsConnectedConnectionAbsent();
	void testIsConnectedConnectionPresent();
	void testCreateDispNet();
	void testGetDispNetCount();
	void testGetDispNet();
	void testGetDispNetNonValidIndex();
	void testDispNetRemove();
	void testDispNetIsEnabled();
	void testDispNetSetEnabled();
	void testDispNetIsHidden();
	void testDispNetIsHiddenOnWrongParams();
	void testDispNetSetHidden();
	void testDispNetSetHiddenOnWrongParams();
	void testDispNetGetNetworkType();
	void testDispNetSetNetworkType();
	void testDispNetGetName();
	void testDispNetGetNameNotEnoughBufSize();
	void testDispNetGetNameNullBufSize();
	void testDispNetSetName();
	void testDispNetGetUuid();
	void testDispNetGetUuidNotEnoughBufSize();
	void testDispNetGetUuidNullBufSize();
	void testDispNetGetSysName();
	void testDispNetGetSysNameNotEnoughBufSize();
	void testDispNetGetSysNameNullBufSize();
	void testDispNetSetSysName();
	void testDispNetGetIndex();
	void testDispNetSetIndex();
	void testDispNetIsDhcpEnabled();
	void testDispNetSetDhcpEnabled();
	void testDispNetGetDhcpScopeStartIp();
	void testDispNetGetDhcpScopeStartIpNotEnoughBufSize();
	void testDispNetGetDhcpScopeStartIpNullBufSize();
	void testDispNetSetDhcpScopeStartIp();
	void testDispNetGetDhcpScopeEndIp();
	void testDispNetGetDhcpScopeEndIpNotEnoughBufSize();
	void testDispNetGetDhcpScopeEndIpNullBufSize();
	void testDispNetSetDhcpScopeEndIp();
	void testDispNetGetDhcpScopeMask();
	void testDispNetGetDhcpScopeMaskNotEnoughBufSize();
	void testDispNetGetDhcpScopeMaskNullBufSize();
	void testDispNetSetDhcpScopeMask();
	void testDispNetIsDhcp6Enabled();
	void testDispNetSetDhcp6Enabled();
	void testDispNetGetDhcp6ScopeStartIp();
	void testDispNetGetDhcp6ScopeStartIpNotEnoughBufSize();
	void testDispNetGetDhcp6ScopeStartIpNullBufSize();
	void testDispNetSetDhcp6ScopeStartIp();
	void testDispNetGetDhcp6ScopeEndIp();
	void testDispNetGetDhcp6ScopeEndIpNotEnoughBufSize();
	void testDispNetGetDhcp6ScopeEndIpNullBufSize();
	void testDispNetSetDhcp6ScopeEndIp();
	void testDispNetGetDhcp6ScopeMask();
	void testDispNetGetDhcp6ScopeMaskNotEnoughBufSize();
	void testDispNetGetDhcp6ScopeMaskNullBufSize();
	void testDispNetSetDhcp6ScopeMask();
	void testSrvConfigGetCpuModel();
	void testSrvConfigGetCpuModelNotEnoughBufSize();
	void testSrvConfigGetCpuModelNullBufSize();
	void testSrvConfigGetCpuCount();
	void testSrvConfigGetCpuSpeed();
	void testSrvConfigGetCpuMode();
	void testSrvConfigGetCpuHvt();
	void testSrvConfigGetCpuHvtOnNullPtr();
	void testSrvConfigGetCpuHvtOnNullSrvCfgHandle();
	void testSrvConfigGetCpuHvtOnNonSrvCfgHandle();
	void testSrvConfigGetHostOsType();
	void testSrvConfigGetHostOsMajor();
	void testSrvConfigGetHostOsMinor();
	void testSrvConfigGetHostOsSubMinor();
	void testSrvConfigGetHostOsStrPresentation();
	void testSrvConfigGetHostOsStrPresentationNotEnoughBufSize();
	void testSrvConfigGetHostOsStrPresentationNullBufSize();
	void testSrvConfigEnumerateFloppyDisks();
	void testSrvConfigEnumerateOpticalDisks();
	void testSrvConfigEnumerateSerialPorts();
	void testSrvConfigEnumerateParallelPorts();
	void testSrvConfigEnumerateSoundOutputDevs();
	void testSrvConfigEnumerateSoundMixerDevs();
	void testSrvConfigEnumeratePrinters();
	void testSrvConfigEnumerateUsbDevs();
	void testSrvConfigIsSoundDefaultEnabled();
	void testSrvConfigIsUsbSupported();
	void testSrvConfigIsVtdSupported();
	void testSrvConfigGetMaxHostNetAdapters();
	void testSrvConfigGetMaxHostNetAdaptersOnWrongParams();
	void testSrvConfigGetMaxVmNetAdapters();
	void testSrvConfigGetMaxVmNetAdaptersOnWrongParams();
	void testSrvConfigEnumerateHardDisks();
	void testSrvConfigEnumerateNetAdapters();
	void testSrvConfigEnumerateGenericPciDevices();
	void testSrvConfigEnumerateGenericScsiDevices();
	void testDispNetHandleNonMadeInvalidOnFromStringCall();
	void testDispNetHandleMadeInvalidOnRemove();
	void testUsrCfgToStringFromString();
	void testGetResultAsHandleForFsGetDiskList();
	void testGetResultAsHandleForFsGetDirEntries();
	void testGetResultAsHandleForFsCreateDir();
	void testGetResultAsHandleForFsRenameEntry();
	void testGetResultAsHandleForFsRemoveEntry();
	void testRemoteFsInfoGetFsType();
	void testRemoteFsInfoGetFsExtType();
	void testRemoteFsInfoGetChildEntriesCount();
	void testRemoteFsInfoGetChildEntry();
	void testRemoteFsInfoCanUseEntryAfterParentFsInfoObjFreed();
	void testRemoteFsInfoGetParentEntry();
	void testGetStatistics();
	void testGetStatisticsOnInvalidServerHandle();
	void testGetStatisticsOnNonServerHandle();
	void testGetResultAsHandleForGetStatistics();
	void testSystemStatisticsGetRamSwapUptimeInfo();
	void testSystemStatisticsRealRamSizeOnWrongParams();
	void testSystemStatisticsGetCpuInfo();
	void testSystemStatisticsGetIfaceInfo();
	void testSystemStatisticsGetUserInfo();
	void testSystemStatisticsGetDiskInfo();
	void testSystemStatisticsGetProcessInfo();
	void testGetResultAsHandleForGetLicenseInfo();
	void testGetLicenseInfoProps();
	void testUpdateLicense();
	void testUpdateLicenseOnInvalidLicenseKey();
	void testUpdateLicenseOnEmptyLicenseKey();
	void testUpdateLicenseOnProductId();
	void testGetParamByIndexForSingleParam();
	void testGetParamByIndexForIndexOutOfRange();
	void testGetParamAsStringOnGetUserProfileRequest();
	void testGetParamByIndexAsStringOnGetUserProfileRequest();
	void testGetParamByIndexAsStringOnGetUserProfileRequestWrongParamIndex();
	void testStartSearchVms();
	void testStartSearchVmsWithSubdirs();
	void testStartSearchVmsOnRegisteredVm();
	void testSmcGetRuntimeInfo();
	void testSubscribeToHostStatistics();
	void testRegVmOnInvalidVmConfig();
	void testGetNetServiceStatus();
	void testGetNetServiceStatusOnInvalidServerHandle();
	void testGetNetServiceStatusOnNonServerHandle();
	void testGetNetServiceStatusAsHandle();
	void testAddNetAdapter();
	void testDeleteNetAdapter();
	void testUpdateNetAdapter();
	void testHomeUserFolderValid();
	void testHomeUserFolderValid2();
	void testHomeUserFolderValid3();
	void testMultipleRegistrationOfTheSameCallback();
	void testFsGenerateEntryName();
	void testFsGenerateEntryNameOnNonExistsTargetDir();
	void testFsGenerateEntryNameOnNullServerHandle();
	void testFsGenerateEntryNameOnNonServerHandle();
	void testFsGenerateEntryNameOnNullTargetDirPath();
	void testFsGenerateEntryNameOnEmptyPrefix();
	void testFsGenerateEntryNameOnNullPrefix();
	void testFsGenerateEntryNameOnEmptySuffix();
	void testFsGenerateEntryNameOnNullSuffix();
	void testFsGenerateEntryNameOnBothEmptyPrefixAndSuffix();
	void testFsGenerateEntryNameOnBothNullPrefixAndSuffix();
	void testRegisterVmOnPathToVmHomeDirSpecified();
	void testRegisterVmOnPathToVmConfigSpecified();
	void testGetServerInfoOnLoginLocalRequest();
	void testGetServerInfoOnLoginRequest();
	void testGetServerInfoOnNonConnectedServerObject();
	void testGetServerInfoOnNonServerHandle();
	void testGetServerInfoOnNullServerHandle();
	void testGetServerInfoOnNullResultBuffer();
	void testGetServerUuidFromLoginResponseOnNonLoginResponseHandle();
	void testGetServerUuidFromLoginResponseOnNullLoginResponseHandle();
	void testGetServerUuidFromLoginResponseOnNullResultBuffer();
	void testGetHostOsVersionFromLoginResponseOnNonLoginResponseHandle();
	void testGetHostOsVersionFromLoginResponseOnNullLoginResponseHandle();
	void testGetHostOsVersionFromLoginResponseOnNullResultBuffer();
	void testGetProductVersionFromLoginResponseOnNonLoginResponseHandle();
	void testGetProductVersionFromLoginResponseOnNullLoginResponseHandle();
	void testGetProductVersionFromLoginResponseOnNullResultBuffer();
	void testRegisterVmOnPathWithEmptyVmName();
	void testSynchVmNameOnVmRegisterWithEmptyVmName();
	void testSynchVmNameOnVmRegister();
	void testSynchVmNameOnVmRegisterWithWrongUuids();
	void testSynchVmNameOnEditVmName();
	void testSynchVmPathOnVmRegister();
	void testSynchVmPathOnVmRegisterWithExistingPath();
	void testGetInvalidVmName();
	void testSynchVmNameOnVmClone();
	void testGetUserInfoList();
	void testCurrentUserInfo();
	void testGetUserInfo();
	void testGetUserInfoOnWrongParams();
	void testJobTypeUseCase();
	void testJobGetTypeOnInvalidHandle();
	void testJobGetTypeOnNonJobHandle();
	void testJobGetTypeOnNullPointer();
	void testJobTypeUseCaseForAllAsyncMethods();
	void testCommonPrefsOnDefaultChangeSettings();
	void testCommonPrefsOnDefaultChangeSettingsWrongParams();
	void testCommonPrefsOnMinimalSecurityLevel();
	void testCommonPrefsOnMinimalSecurityLevelWrongParams();
	void testGetQuestionsList();
	void testGetQuestionsListOnInvalidHandle();
	void testGetQuestionsListOnWrongTypeHandle();
	void testGetQuestionsListOnNullResultBuffer();
	void testSubscribeToPerfStats() ;
	void testGetPerfStats() ;
	void testCheckSourceVmUuidOnRegisterVmWithEmptySourceUuid();
	void testRegisterOldVmFullPathToVmConfigSpecified();
	void testRegisterOldVmToCheckSomeNetworkMacAddress();
	void testGetHostHardwareInfoCheckHardDisksListSorted();
	void testRegisterOldVmWithNonSimpleHddsDirsStructure();
	void testRegisterOldVmWithSlashInName();
	void testRegisterOldVmOwnedByAnotherUserButAclWithRightsAdded();
	void testRegisterOldVmWithSlashInName2();
	void testRegisterOldVmWithSlashInName3();
	void testRegisterOldVmWithSlashInName4();
	void testRegisterVmOnPathToVmHomeDirSpecifiedConfigWasBrokenButBackupPresent();
	void testRegisterVmOnPathToVmConfigSpecifiedConfigWasBrokenButBackupPresent();
	void testRegisterVmOnPathToVmHomeDirSpecifiedConfigWasDeletedButBackupPresent();
	void testRegisterVmOnPathToVmConfigSpecifiedConfigWasDeletedButBackupPresent();
	void testConfigureGenericPciOnWrongParams();
	void testGenericPciDeviceState();
	void testGenericPciDeviceStateOnWrongParams();
	void testVmDeviceUpdateInfo();
	void testGenericPciDeviceOps();
	void testGenericPciDeviceClassOnWrongParams();
	void testGenericPciDeviceIsPrimaryOnWrongParams();
	void testCreateVmInSpecificNonExistsFolderNonInteractiveMode();
	void testCreateVmInSpecificNonExistsFolderInteractiveModeFolderCreationAccepted();
	void testCreateVmInSpecificNonExistsFolderInteractiveModeFolderCreationRejected();
	void testPrepareForHibernateOnWrongParams();
	void testAfterHostResumeOnWrongParams();
	void testSendProblemReport();
    void testSendProblemReportByServer();
	void testSendProblemReportOnWrongReport();
	void testSendProblemReportAtAsyncMode();
	void testSendProblemReportCancelOperation();
	void testNonInterectiveSession();
	void testNonInterectiveSessionOnWrongParams();
	void testBackupSourceDefaultBackupServer();
	void testBackupSourceDefaultBackupServerOnWrongParams();
	void testBackupTargetDefaultDirectory();
	void testBackupTargetDefaultDirectoryOnWrongParams();
	void testBackupTimeout();
	void testBackupTimeoutOnWrongParams();
	void testEncryptionDefaultPluginId();
	void testEncryptionDefaultPluginIdOnWrongParams();
	void testBackupSourceUserLogin();
	void testBackupSourceUserLoginOnWrongParams();
	void testBackupSourceUserPassword();
	void testBackupSourceUserPasswordOnWrongParams();
	void testBackupSourceUsePassword();
	void testBackupSourceUsePasswordOnWrongParams();
	void testVerboseLogEnabled();
	void testVerboseLogEnabledOnWrongParams();
	void testCantToChangeUsbPreferencesThroughEditCommonPrefs();
	void testGetPackedProblemReport();
	void testSendPackedProblemReport();
	void testSendPackedProblemReportOnWrongReport();
	void testSendPackedProblemReportAtAsyncMode();
	void testSendPackedProblemReportCancelOperation();
	void testRegiste3rdPartyVmOnWrongParams();
	void testEventOnUpdateLicenseContainsLicenseHandle();
	void testRemoteGetSupportedOsesCheckSupportedGuestsTypes();
	void testRemoteGetSupportedOsesCheckSupportedGuestsVersions();
	void testRemoteGetSupportedOsesOnWrongParams();
	void testOsesMatrixMethodsOnWrongParams();
	void testRestrictions();
	void testRestrictionsOnWrongParams();
	void testCreateProblemReportForNewScheme();
	void testCreateProblemReportForOldScheme();
	void testAssemblyProblemReportWithClientInfoForNewScheme();
	void testAssemblyProblemReportWithClientInfoForOldScheme();
	void testCreateProblemReportOnWrongParams();
	void testProblemReportAsString();
	void testProblemReportAsStringOnWrongParams();
	void testProblemReportGetDataOnWrongParams();
	void testProblemReportAssemblyOnWrongParams();
	void testProblemReportGetScheme();
	void testProblemReportGetSchemeOnWrongParams();
	void testProblemReportSetGetType();
	void testProblemReportSetGetTypeOnWrongParams();
	void testProblemReportSetGetReason();
	void testProblemReportSetGetReasonOnWrongParams();
	void testProblemReportSetGetUserName();
	void testProblemReportSetGetUserNameOnWrongParams();
	void testProblemReportSetGetUserEmail();
	void testProblemReportSetGetUserEmailOnWrongParams();
	void testProblemReportSetGetDescription();
	void testProblemReportSetGetDescriptionOnWrongParams();
	void testProblemReportGetArchiveFileName();
	void testProblemReportGetArchiveFileNameOnWrongParams();
	void testProblemReportSendForNewScheme();
	void testProblemReportSendForOldScheme();
	void testProblemReportSendOnWrongParams();
	void testGetAssemblyAndSendProblemReport();
	void testApplianceOnWrongParams();
	void testUpLocalhostInterface();
	void testGetCtTemplateList();
	void testGetCtTemplateListOnWrongParams();
	void testGetCtNameOnWrongParams();
	void testGetCtDescriptionOnWrongParams();
	void testGetCtVersionOnWrongParams();
	void testGetCtOsTemplateOnWrongParams();
	void testGetCtTypeOnWrongParams();
	void testGetCtOsTypeOnWrongParams();
	void testGetCtOsVersionOnWrongParams();
	void testGetCtCpuModeOnWrongParams();
	void testIsCtCachedOnWrongParams();
	void testLoginToNotAcceptableHost();
	void testLoginToNotAcceptableHostAsyncMode();
	void testRemoveCtTemplate();
	void testRemoveCtTemplateOnWrongParams();
	void testGetUsbIdentityCount();
	void testSetUsbIdentAssociation();
	void testSetUsbIdentAssociationOnWrongParam();
	void testGetUsbIdentityOnWrongIndex();
	void testDispConfigUsbIdentities();
	void testUsbIdentWrongHandles();
	void testCopyCtTemplateOnWrongParams();
	void testIsFeatureSupported();
	void testIsFeatureSupportedOnWrongParams();
	void testRefreshPluginsOnWrongParams();
	void testGetPluginsListOnWrongParams();
	void testGetDiskFreeSpace();
	void testGetDiskFreeSpaceOnWrongParams();
	void testConvert3rdPartyVmShouldReturnVmConfig();
	void testAllowMultiplePMC();
	void testAllowMultiplePMCOnWrongParams();
	void testDispConfigGetCpuFeaturesMask();
	void testDispConfigGetCpuFeaturesMaskEx();
	void testDispConfigSetCpuFeaturesMask();
	void testDispConfigSetCpuFeaturesMaskEx();
	void testDispConfigGetCpuFeaturesMaskOnWrongParams();
	void testDispConfigGetCpuFeaturesMaskExOnWrongParams();
	void testDispConfigSetCpuFeaturesMaskExOnWrongParams();
	void testSrvConfigGetCpuFeatures();
	void testSrvConfigGetCpuFeaturesEx();
	void testSrvConfigGetCpuFeaturesOnWrongParams();
	void testSrvConfigGetCpuFeaturesExOnWrongParams();
	void testLogRotationEnabled();
	void testLogRotationEnabledOnWrongParams();
	void testDispConfigSetHostId();
	void testDispConfigSetHostIdOnWrongParams();
	void testDispConfigGetHostId();
	void testDispConfigGetHostIdOnWrongParams();
	void testSrvConfigGetCpuFeaturesMaskingCapabilities();
	void testSrvConfigGetCpuFeaturesMaskingCapabilitiesOnWrongParams();
	void testCpuFeaturesGetValueOnWrongParams();
	void testCpuFeaturesSetValueOnWrongParams();
	void testDispConfigCpuFeaturesMaskSetOldValidGetNew();
	void testDispConfigCpuFeaturesMaskSetOldInvalidGetNew();
	void testDispConfigCpuFeaturesMaskSetNewGetOld();

private:

	void SetUpAllowDirectMobile();
	void setDefaultLicenseUserAndCompany();

	SdkHandleWrap m_ServerHandle;
	SdkHandleWrap m_JobHandle;
	SdkHandleWrap m_ResultHandle;
	SdkHandleWrap m_VmHandle;
	QString m_sTestFsDirName1, m_sTestFsDirName2, m_sTestFsDirName3, m_sTestFsDirName1ChildDir,
		m_sTestFsDirName1ChildChildDir, m_sTestFsDirName1ChildChildChildDir, m_sTestConvertDirName;
	QList<SdkHandleWrap> m_lstVmHandles;
};


#endif
