/////////////////////////////////////////////////////////////////////////////
///
///	@file CProtoSerializerTest.h
///
///	Tests fixture class for testing project protocol serializer.
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

#ifndef CProtoSerializerTest_H
#define CProtoSerializerTest_H

#include <QtTest/QtTest>

class CProtoSerializerTest : public QObject {

Q_OBJECT

private slots:
	void testCreateDspCmdUserLoginCommand();
	void testParseDspCmdUserLoginCommand();
	void testDspCmdUserLoginCommandIsValidFailedOnEmptyPackage();
	void testDspCmdUserLoginCommandIsValidFailedOnUserNameAbsent();
	void testDspCmdUserLoginCommandIsValidFailedOnPasswordAbsent();
	void testDspCmdUserLoginCommandIsValidFailedOnPrevSessionUuidAbsent();
	void testCreateDspWsResponseCommandForLogin();
	void testCreateDspWsResponseCommandForGetVmList();
	void testCreateDspWsResponseCommandForGetSuspendedVmScreen();
	void testCreateDspWsResponseCommandForDspCmdUserGetHostHwInfo();
	void testCreateDspWsResponseCommandForDspCmdFsGetDiskList();
	void testCreateDspWsResponseCommandForDspCmdFsGetCurrentDirectory();
	void testCreateDspWsResponseCommandForDspCmdFsGetDirectoryEntries();
	void testCreateDspWsResponseCommandForDspCmdFsGetFileList();
	void testCreateDspWsResponseCommandForDspCmdFsCreateDirectory();
	void testCreateDspWsResponseCommandForDspCmdFsRenameEntry();
	void testCreateDspWsResponseCommandForDspCmdFsRemoveEntry();
	void testCreateDspWsResponseCommandForDspCmdUserGetProfile();
	void testCreateDspWsResponseCommandForDspCmdGetHostCommonInfo();
	void testCreateDspWsResponseCommandForDspCmdVmGetProblemReport();
	void testCreateDspWsResponseCommandForDspCmdVmGetConfig();
	void testCreateDspWsResponseCommandForDspCmdDirRegVm();
	void testCreateDspWsResponseCommandForDspCmdDirRestoreVm();
	void testCreateDspWsResponseCommandForDspCmdGetVmInfo();
	void testCreateDspWsResponseCommandForDspCmdSMCGetDispatcherRTInfo();
	void testCreateDspWsResponseCommandForDspCmdSMCGetCommandHistoryByVm();
	void testCreateDspWsResponseCommandForDspCmdSMCGetCommandHistoryByUser();
	void testCreateDspWsResponseCommandForDspCmdUserGetLicenseInfo();
	void testCreateDspWsResponseCommandForDspCmdGetHostStatistics();
	void testCreateDspWsResponseCommandForDspCmdVmGetStatistics();
	void testCreateDspWsResponseCommandForDspCmdGetVmConfigById();
    void testDataDspCmdSendProblemReport();
	void testCreateDspWsResponseCommandWithAdditionalErrorInfo();
	void testCreateDspCmdUserLoginLocalCommand();
	void testParseDspCmdUserLoginLocalCommand();
	void testDspCmdUserLoginLocalCommandIsValidFailedOnEmptyPackage();
	void testDspCmdUserLoginLocalCommandIsValidFailedOnPrevSessionUuidAbsent();
	void testCreateDspCmdUserLogoffCommand();
	void testParseDspCmdUserLogoffCommand();
	void testParseCommandForDspCmdSMCShutdownDispatcher();
	void testParseCommandForDspCmdDirGetVmList();
	void testParseCommandForDspCmdUserGetEvent();
	void testParseCommandForDspCmdUserGetProfile();
	void testParseCommandForDspCmdUserProfileBeginEdit();
	void testParseCommandForDspCmdGetHostCommonInfo();
	void testParseCommandForDspCmdHostCommonInfoBeginEdit();
	void testParseCommandForDspCmdUserGetHostHwInfo();
	void testParseCommandForDspCmdUserPing();
	void testParseCommandForDspCmdFsGetDiskList();
	void testParseCommandForDspCmdFsGetCurrentDirectory();
	void testParseCommandForDspCmdSMCGetDispatcherRTInfo();
	void testParseCommandForDspCmdSMCRestartDispatcher();
	void testParseCommandForDspCmdSMCDisconnectAllUsers();
	void testParseCommandForDspCmdNetPrlNetworkServiceStart();
	void testParseCommandForDspCmdNetPrlNetworkServiceStop();
	void testParseCommandForDspCmdNetPrlNetworkServiceRestart();
	void testParseCommandForDspCmdNetPrlNetworkServiceRestoreDefaults();
	void testParseCommandForDspCmdGetHostStatistics();
	void testParseCommandForDspCmdSubscribeToHostStatistics();
	void testParseCommandForDspCmdUnsubscribeFromHostStatistics();
	void testParseCommandForDspCmdUserGetLicenseInfo();
	void testParseCommandForDspCmdUserInfoList();
	void testParseCommandForDspCmdGetVirtualNetworkList();
	void testParseCommandForDspCmdAllHostUsers();
	void testParseCommandForDspCmdPrepareForHibernate();
	void testParseCommandForDspCmdAfterHostResume();
	void testCreateDspCmdVmStartCommand();
	void testParseDspCmdVmStartCommand();
	void testDspCmdVmStartCommandIsValidFailedOnEmptyPackage();
	void testDspCmdVmStartCommandIsValidFailedOnVmUuidAbsent();
	void testCreateDspCmdVmRestartCommand();
	void testParseDspCmdVmRestartCommand();
	void testDspCmdVmRestartCommandIsValidFailedOnEmptyPackage();
	void testDspCmdVmRestartCommandIsValidFailedOnVmUuidAbsent();
	void testParseCommandForDspCmdVmGetConfig();
	void testParseCommandForDspCmdVmGetProblemReport();
	void testParseCommandForDspCmdVmReset();
	void testParseCommandForDspCmdVmSuspend();
	void testParseCommandForDspCmdVmGetSuspendedScreen();
	void testParseCommandForDspCmdVmResume();
	void testParseCommandForDspCmdDirVmEditBegin();
	void testParseCommandForDspCmdDirUnregVm();
	void testParseCommandForDspCmdDirRestoreVm();
	void testParseCommandForDspCmdGetVmInfo();
	void testParseCommandForDspCmdSMCShutdownVm();
	void testParseCommandForDspCmdSMCRestartVm();
	void testParseCommandForDspCmdVmGetStatistics();
	void testParseCommandForDspCmdVmSubscribeToGuestStatistics();
	void testParseCommandForDspCmdVmUnsubscribeFromGuestStatistics();
	void testParseCommandForDspCmdSMCGetCommandHistoryByVm();
	void testParseCommandForDspCmdVmMigrateCancel();
	void testParseCommandForDspCmdVmRunCompressor();
	void testParseCommandForDspCmdVmCancelCompressor();
	void testParseCommandForDspCmdVmStartVncServer();
	void testParseCommandForDspCmdVmStopVncServer();
	void testParseCommandForDspCmdVmLock();
	void testParseCommandForDspCmdVmUnlock();
	void testParseCommandForDspCmdDspCmdVmCancelCompact();
	void testParseCommandForDspCmdVmChangeSid();
	void testParseCommandForDspCmdVmResetUptime();
	void testParseCommandForDspCmdGetVmConfigById();
	void testCreateDspCmdVmStopCommand();
	void testParseDspCmdVmStopCommand();
	void testDspCmdVmStopCommandIsValidFailedOnEmptyPackage();
	void testDspCmdVmStopCommandIsValidFailedOnVmUuidAbsent();
	void testDspCmdVmStopCommandIsValidFailedOnAcpiSignAbsent();

	void testDspCProtoVmCommandWithOneStrParam_BothParamsExists();
	void testDspCProtoVmCommandWithOneStrParam_VmUuidAbsent();
	void testDspCProtoVmCommandWithOneStrParam_StrParamAbsent();
	void testDspCProtoVmCommandWithOneStrParam_BothParamsAbsent();

	void testParseCommandForDspCmdVmPause();
	void testCreateDspCmdVmAnswerCommand();
	void testParseDspCmdVmAnswerCommand();
	void testDspCmdVmAnswerCommandIsValidFailedOnEmptyPackage();
	void testDspCmdVmAnswerCommandIsValidFailedOnAnswerAbsent();
	void testParseCommandForDspCmdCtlApplyVmConfig();
	void testParseCommandForDspCmdDirVmEditCommit();
	void testParseCommandForDspCmdUserProfileCommit();
	void testParseCommandForDspCmdHostCommonInfoCommit();
	void testParseCommandForDspCmdUserCancelOperation();
	void testParseCommandForDspCmdFsGetDirectoryEntries();
	void testParseCommandForDspCmdFsCreateDirectory();
	void testParseCommandForDspCmdFsRemoveEntry();
	void testParseCommandForDspCmdFsCanCreateFile();
	void testParseCommandForDspCmdSMCGetCommandHistoryByUser();
	void testParseCommandForDspCmdSMCDisconnectUser();
	void testParseCommandForDspCmdSMCCancelUserCommand();
	void testParseCommandForDspCmdDirRegVm();
	void testParseCommandForDspCmdUserInfo();
	void testParseCommandForDspCmdAddVirtualNetwork();
	void testParseCommandForDspCmdUpdateVirtualNetwork();
	void testParseCommandForDspCmdDeleteVirtualNetwork();
	void testParseCommandForDspCmdConfigureGenericPci();
	void testParseCommandForDspCmdSetNonInteractiveSession();
	void testParseCommandForDspCmdVmChangeLogLevel();
	void testParseCommandForDspCmdInstallAppliance();

	void testCreateDspCmdDirVmDelete();
	void testParseCommandForDspCmdDirVmDelete();
	void testDspCmdDirVmDeleteCommandIsValidFailedOnEmptyPackage();
	void testDspCmdDirVmDeleteCommandIsValidFailedOnVmUuidAbsent();
	void testDspCmdDirVmDeleteCommandIsValidFailedOnDevicesListAbsent();
	void testCreateDspCmdVmDevConnectCommand();
	void testParseDspCmdVmDevConnectCommand();
	void testDspCmdVmDevConnectCommandIsValidFailedOnEmptyPackage();
	void testDspCmdVmDevConnectCommandIsValidFailedOnVmUuidAbsent();
	void testDspCmdVmDevConnectCommandIsValidFailedOnDeviceTypeAbsent();
	void testDspCmdVmDevConnectCommandIsValidFailedOnDeviceIndexAbsent();
	void testDspCmdVmDevConnectCommandIsValidFailedOnDeviceConfigAbsent();
	void testParseCommandForDspCmdVmDevDisconnect();
	void testCreateDspCmdDirVmCreateCommand();
	void testParseDspCmdDirVmCreateCommand();
	void testDspCmdDirVmCreateCommandIsValidFailedOnEmptyPackage();
	void testDspCmdDirVmCreateCommandIsValidFailedOnVmConfigAbsent();
	void testDspCmdDirVmCreateCommandIsValidFailedOnVmHomePathAbsent();
	void testCreateDspCmdDirVmCloneCommand();
	void testParseDspCmdDirVmCloneCommand();
	void testDspCmdDirVmCloneCommandIsValidFailedOnEmptyPackage();
	void testDspCmdDirVmCloneCommandIsValidFailedOnVmConfigAbsent();
	void testDspCmdDirVmCloneCommandIsValidFailedOnVmHomePathAbsent();
	void testDspCmdDirVmCloneCommandIsValidFailedOnVmNameAbsent();
	void testDspCmdDirVmCloneCommandIsValidFailedOnCreateTemplateSignAbsent();

	void testCreateDspCmdDirCreateImageCommand();
	void testParseDspCmdDirCreateImageCommand();
	void testDspCmdDirCreateImageCommandIsValidFailedOnEmptyPackage();
	void testDspCmdDirCreateImageCommandIsValidFailedOnVmUuidAbsent();
	void testDspCmdDirCreateImageCommandIsValidFailedOnImageConfigAbsent();
	void testDspCmdDirCreateImageCommandIsValidFailedOnRecreateIsAllowedSignAbsent();

	void testCreateDspCmdDirCopyImageCommand();
	void testParseDspCmdDirCopyImageCommand();
	void testDspCmdDirCopyImageCommandOnInvalidPackage();

	void testCreateDspCmdStartSearchConfigCommand();
	void testParseDspCmdStartSearchConfigCommand();
	void testDspCmdStartSearchConfigCommandIsValidFailedOnEmptyPackage();
	void testDspCmdStartSearchConfigCommandIsValidFailedOnSearchDirsListAbsent();
	void testCreateDspCmdFsRenameEntryCommand();
	void testParseDspCmdFsRenameEntryCommand();
	void testDspCmdFsRenameEntryCommandIsValidFailedOnEmptyPackage();
	void testDspCmdFsRenameEntryCommandIsValidFailedOnOldEntryNameAbsent();
	void testDspCmdFsRenameEntryCommandIsValidFailedOnNewEntryNameAbsent();
	void testCreateDspCmdDirInstallGuestOsCommand();
	void testParseDspCmdDirInstallGuestOsCommand();
	void testDspCmdDirInstallGuestOsCommandIsValidFailedOnEmptyPackage();
	void testDspCmdDirInstallGuestOsCommandIsValidFailedOnUserNameAbsent();
	void testDspCmdDirInstallGuestOsCommandIsValidFailedOnCompanyNameAbsent();
	void testDspCmdDirInstallGuestOsCommandIsValidFailedOnSerialNumberAbsent();
	void testParseDspCmdUserUpdateLicense();
	void testCreateDspCmdVmCreateUnattendedFloppyCommand();
	void testParseDspCmdVmCreateUnattendedFloppyCommand();
	void testDspCmdVmCreateUnattendedFloppyCommandIsValidFailedOnEmptyPackage();
	void testDspCmdVmCreateUnattendedFloppyCommandIsValidFailedOnVmUuidAbsent();
	void testDspCmdVmCreateUnattendedFloppyCommandIsValidFailedOnGuestDistroTypeAbsent();
	void testDspCmdVmCreateUnattendedFloppyCommandIsValidFailedOnUserNameAbsent();
	void testDspCmdVmCreateUnattendedFloppyCommandIsValidFailedOnCompanyNameAbsent();
	void testDspCmdVmCreateUnattendedFloppyCommandIsValidFailedOnSerialNumberAbsent();
	void testCreateDspCmdFsGenerateEntryNameCommand();
	void testParseDspCmdFsGenerateEntryNameCommand();
	void testDspCmdFsGenerateEntryNameCommandIsValidFailedOnEmptyPackage();
	void testDspCmdFsGenerateEntryNameCommandIsValidFailedOnTargetDirPathAbsent();
	void testDspCmdFsGenerateEntryNameCommandIsValidFailedOnFilenamePrefixAbsent();
	void testDspCmdFsGenerateEntryNameCommandIsValidFailedOnFilenameSuffixAbsent();
	void testDspCmdFsGenerateEntryNameCommandIsValidFailedOnIndexDelimiterAbsent();
	void testCreateDspCmdVmUpdateSecurityCommand();
	void testParseDspCmdVmUpdateSecurityCommand();
	void testDspCmdVmUpdateSecurityCommandIsValidFailedOnEmptyPackage();
	void testDspCmdVmUpdateSecurityCommandIsValidFailedOnVmUuidAbsent();
	void testDspCmdVmUpdateSecurityCommandIsValidFailedOnVmSecurityAbsent();
	void testCreateDspWsResponseCommandForDspCmdVmUpdateSecurity();
	void testDspCmdVmSectionValidateConfig();
	void testParseDspCmdVmSectionValidateConfig();
	void testParseDspCmdVmSectionValidateConfigNotValid();
	void testCreateDspWsResponseCommandForDspCmdUserInfoList();
	void testCreateDspWsResponseCommandForGetUserInfo();
	void testCreateDspWsResponseCommandForDspCmdGetVirtualNetworkList();
	void testCreateDspWsResponseCommandForDspCmdAllHostUsers();
	void testCreateDspCmdVmMigrateCommand();
	void testParseDspCmdVmMigrateCommand();
	void testDspCmdVmMigrateCommandIsValidFailedOnEmptyPackage();
	void testDspCmdVmMigrateCommandIsValidFailedOnVmUuidAbsent();
	void testDspCmdVmMigrateCommandIsValidFailedOnTargetServerHostnameAbsent();
	void testDspCmdVmMigrateCommandIsValidFailedOnTargetServerPortAbsent();
	void testDspCmdVmMigrateCommandIsValidFailedOnTargetServerSessionUuidAbsent();
	void testDspCmdVmMigrateCommandIsValidFailedOnTargetServerVmHomePathAbsent();
	void testDspCmdVmMigrateCommandIsValidFailedOnReservedFlagsAbsent();
	void testCreateDspCmdVmStartExCommand();
	void testParseDspCmdVmStartExCommand();
	void testDspCmdVmStartExCommandIsValidFailedOnEmptyPackage();
	void testDspCmdVmStartExCommandIsValidFailedOnVmUuidAbsent();
	void testDspCmdVmStartExCommandIsValidFailedOnStartModeAbsent();
	void testDspCmdVmStartExCommandIsValidFailedOnReservedParameterAbsent();
	void testCreateDspCmdVmInstallUtilityCommand();
	void testCreateDspCmdVmLoginInGuestCommand();
	void testParseDspCmdVmLoginInGuestCommand();
	void testDspCmdVmLoginInGuestCommandIsValidFailedOnEmptyPackage();
	void testDspCmdVmLoginInGuestCommandIsValidFailedOnVmUuidAbsent();
	void testDspCmdVmLoginInGuestCommandIsValidFailedOnUserLoginAbsent();
	void testDspCmdVmLoginInGuestCommandIsValidFailedOnUserPasswordAbsent();
	void testCreateDspCmdVmAuthWithGuestSecurityDbCommand();
	void testParseDspCmdVmAuthWithGuestSecurityDbCommand();
	void testCreateDspCmdVmGuestLogoutCommand();
	void testParseDspCmdVmGuestLogoutCommand();
	void testDspCmdVmGuestLogoutCommandIsValidFailedOnEmptyPackage();
	void testDspCmdVmGuestLogoutCommandIsValidFailedOnVmUuidAbsent();
	void testDspCmdVmGuestLogoutCommandIsValidFailedOnSessionUuidAbsent();
	void testParseDspCmdVmGuestGetNetworkSettingsCommand();
	void testCreateDspCmdVmGuestRunProgramCommand();
	void testParseDspCmdVmGuestRunProgramCommand();
	void testDspCmdVmGuestRunProgramCommandIsValidFailedOnEmptyPackage();
	void testDspCmdVmGuestRunProgramCommandIsValidFailedOnVmUuidAbsent();
	void testDspCmdVmGuestRunProgramCommandIsValidFailedOnSessionUuidAbsent();
	void testDspCmdVmGuestRunProgramCommandIsValidFailedOnProgramNameAbsent();
	void testDspCmdVmGuestRunProgramCommandIsValidFailedOnArgsListAbsent();
	void testDspCmdVmGuestRunProgramCommandIsValidFailedOnEnvsListAbsent();
	void testCreateDspCmdVmGuestSetUserPasswdCommand();
	void testParseDspCmdVmGuestSetUserPasswdCommand();
	void testDspCmdVmGuestSetUserPasswdCommandIsValidFailedOnEmptyPackage();
	void testDspCmdVmGuestSetUserPasswdCommandIsValidFailedOnVmUuidAbsent();
	void testDspCmdVmGuestSetUserPasswdCommandIsValidFailedOnSessionUuidAbsent();
	void testDspCmdVmGuestSetUserPasswdCommandIsValidFailedOnUserLoginNameAbsent();
	void testDspCmdVmGuestSetUserPasswdCommandIsValidFailedOnUserPasswdAbsent();
	void testCreateDspCmdSetSessionConfirmationMode();
	void testParseDspCmdSetSessionConfirmationMode();

	void testCreateDspCmdStorageSetValueCommand();
	void testParseDspCmdStorageSetValueCommand();
	void testCreateDspCmdVmStorageSetValueCommand();
	void testParseDspCmdVmStorageSetValueCommand();

	void testCreateDspCmdDirReg3rdPartyVmCommand();
	void testParseDspCmdDirReg3rdPartyVmCommand();
	void testDspCmdDirReg3rdPartyVmCommandIsValidFailedOnEmptyPackage();
	void testDspCmdDirReg3rdPartyVmCommandIsValidFailedOnPathToVmConfigAbsent();
	void testDspCmdDirReg3rdPartyVmCommandIsValidFailedOnPathToRootVmDirAbsent();

	void testCreateDspCmdRefreshPluginsCommand();
	void testParseDspCmdRefreshPluginsCommand();

	void testCreateDspCmdGetPluginsListCommand();
	void testParseDspCmdGetPluginsListCommand();

	void testCreateDspCmdCtlLicenseChangeCommand();
	void testParseDspCmdCtlLicenseChangeCommand();
};

#endif
